/********************************************************************
 FileName:      main.c
 Dependencies:  See INCLUDES section
 Processor:     PIC18, PIC24, dsPIC, and PIC32 USB Microcontrollers
 Hardware:      This demo is natively intended to be used on Microchip USB demo
                boards supported by the MCHPFSUSB stack.  See release notes for
                support matrix.  This demo can be modified for use on other 
                hardware platforms.
 Complier:      Microchip C18 (for PIC18), XC16 (for PIC24/dsPIC), XC32 (for PIC32)
 Company:       Microchip Technology, Inc.

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the "Company") for its PIC� Microcontroller is intended and
 supplied to you, the Company's customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

********************************************************************
 File Description:

 Change History:
  Rev   Description
  ----  -----------------------------------------
  1.0   Initial release

  2.1   Updated for simplicity and to use common
        coding style

  2.6a  Added button debouncing using Start-of-Frame packets

  2.7   Updated demo to place the PIC24F devices into sleep when the
        USB is in suspend.  

  2.7b  Improvements to USBCBSendResume(), to make it easier to use.
  2.9f  Adding new part support
********************************************************************/

/** INCLUDES *******************************************************/
#include "./USB/usb.h"
#include "./USB/usb_function_cdc.h"
#include <timers.h>
#include <delays.h>

#include "HardwareProfile.h"

/** CONFIGURATION **************************************************/
// Configuration bits for PICDEM FS USB Demo Board (based on PIC18F4550)
#pragma config PLLDIV   = 5         // (20 MHz crystal on PICDEM FS USB board)
#if (USB_SPEED_OPTION == USB_FULL_SPEED)
    #pragma config CPUDIV   = OSC1_PLL2  
#else
    #pragma config CPUDIV   = OSC3_PLL4   
#endif
#pragma config USBDIV   = 2         // Clock source from 96MHz PLL/2
#pragma config FOSC     = HSPLL_HS
#pragma config FCMEN    = OFF
#pragma config IESO     = OFF
#pragma config PWRT     = OFF
#pragma config BOR      = ON
#pragma config BORV     = 3
#pragma config VREGEN   = ON      //USB Voltage Regulator
#pragma config WDT      = OFF
#pragma config WDTPS    = 32768
#pragma config MCLRE    = ON
#pragma config LPT1OSC  = OFF
#pragma config PBADEN   = OFF
//#pragma config CCP2MX   = ON
#pragma config STVREN   = ON
#pragma config LVP      = OFF
//#pragma config ICPRT    = OFF       // Dedicated In-Circuit Debug/Programming
#pragma config XINST    = OFF       // Extended Instruction Set
#pragma config CP0      = OFF
#pragma config CP1      = OFF
//#pragma config CP2      = OFF
//#pragma config CP3      = OFF
#pragma config CPB      = OFF
//#pragma config CPD      = OFF
#pragma config WRT0     = OFF
#pragma config WRT1     = OFF
//#pragma config WRT2     = OFF
//#pragma config WRT3     = OFF
#pragma config WRTB     = OFF       // Boot Block Write Protection
#pragma config WRTC     = OFF
//#pragma config WRTD     = OFF
#pragma config EBTR0    = OFF
#pragma config EBTR1    = OFF
//#pragma config EBTR2    = OFF
//#pragma config EBTR3    = OFF
#pragma config EBTRB    = OFF

/** I N C L U D E S **********************************************************/

#include "GenericTypeDefs.h"
#include "Compiler.h"
#include "usb_config.h"
#include "USB/usb_device.h"
#include "USB/usb.h"
#include "buffer.h"
#include "button.h"

#include "HardwareProfile.h"

#define LED1_TRIS   TRISAbits.TRISA4
#define LED2_TRIS   TRISAbits.TRISA5
#define IR_TRIS     TRISBbits.TRISB3
#define SW_TRIS     TRISBbits.TRISB4
#define IRLED_TRIS  TRISAbits.TRISA0

#define LED1_PORT   PORTAbits.RA4
#define LED2_PORT   PORTAbits.RA5
#define IR_PORT     PORTBbits.RB3
#define SW_PORT     PORTBbits.RB4
#define IRLED_PORT  PORTAbits.RA0

#define SW_BIT		(1<<4)

#define MAX_WAIT_CYCLE 60000

/** V A R I A B L E S ********************************************************/
// 順番重要:先にアドレス順で後の udata から定義するとバンク内にきちんと収まる
//          これを udata の後に書くと収まらなくなる
// http://tylercsf.blog123.fc2.com/blog-entry-189.html

#pragma udata USER_RAM6=0x600
BYTE buff_user1[0x200];

#pragma udata USER_RAM2=0x200
BYTE buff_user2[0x100];

#if defined(__18CXX)
    #pragma udata
#endif

char USB_In_Buffer[64];
char USB_Out_Buffer[64];
BYTE enableReadIR;


/** P R I V A T E  P R O T O T Y P E S ***************************************/
static void InitializeSystem(void);
void ProcessIO(void);
void USBDeviceTasks(void);
void YourHighPriorityISRCode();
void YourLowPriorityISRCode();
void USBCBSendResume(void);
void BlinkUSBStatus(void);
void UserInit(void);
void ButtonProc(void);
void ReadIR(void);
void SendIR(void);
void SendIRImpl(void);
void DelayIRFreqHi(void);
void DelayIRFreqLo(void);
int WaitToReadySerial(void);
void SerialProc(void);
void PutsString(const rom char *str);
void PutsStringCPtr(char *str);
void PrintIRBuffer(const rom char *title);

BYTE ReadBYTEBuffer(WORD pos); // for debug

/** VECTOR REMAPPING ***********************************************/
#if defined(__18CXX)
	//On PIC18 devices, addresses 0x00, 0x08, and 0x18 are used for
	//the reset, high priority interrupt, and low priority interrupt
	//vectors.  However, the current Microchip USB bootloader 
	//examples are intended to occupy addresses 0x00-0x7FF or
	//0x00-0xFFF depending on which bootloader is used.  Therefore,
	//the bootloader code remaps these vectors to new locations
	//as indicated below.  This remapping is only necessary if you
	//wish to program the hex file generated from this project with
	//the USB bootloader.  If no bootloader is used, edit the
	//usb_config.h file and comment out the following defines:
	//#define PROGRAMMABLE_WITH_USB_HID_BOOTLOADER
	//#define PROGRAMMABLE_WITH_USB_LEGACY_CUSTOM_CLASS_BOOTLOADER
	
	#if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)
		#define REMAPPED_RESET_VECTOR_ADDRESS			0x1000
		#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x1008
		#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x1018
	#elif defined(PROGRAMMABLE_WITH_USB_MCHPUSB_BOOTLOADER)	
		#define REMAPPED_RESET_VECTOR_ADDRESS			0x800
		#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x808
		#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x818
	#else	
		#define REMAPPED_RESET_VECTOR_ADDRESS			0x00
		#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x08
		#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x18
	#endif
	
	#if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)||defined(PROGRAMMABLE_WITH_USB_MCHPUSB_BOOTLOADER)
	extern void _startup (void);        // See c018i.c in your C18 compiler dir
	#pragma code REMAPPED_RESET_VECTOR = REMAPPED_RESET_VECTOR_ADDRESS
	void _reset (void)
	{
	    _asm goto _startup _endasm
	}
	#endif
	#pragma code REMAPPED_HIGH_INTERRUPT_VECTOR = REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS
	void Remapped_High_ISR (void)
	{
	     _asm goto YourHighPriorityISRCode _endasm
	}
	#pragma code REMAPPED_LOW_INTERRUPT_VECTOR = REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS
	void Remapped_Low_ISR (void)
	{
	     _asm goto YourLowPriorityISRCode _endasm
	}
	
	#if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)||defined(PROGRAMMABLE_WITH_USB_MCHPUSB_BOOTLOADER)
	//Note: If this project is built while one of the bootloaders has
	//been defined, but then the output hex file is not programmed with
	//the bootloader, addresses 0x08 and 0x18 would end up programmed with 0xFFFF.
	//As a result, if an actual interrupt was enabled and occured, the PC would jump
	//to 0x08 (or 0x18) and would begin executing "0xFFFF" (unprogrammed space).  This
	//executes as nop instructions, but the PC would eventually reach the REMAPPED_RESET_VECTOR_ADDRESS
	//(0x1000 or 0x800, depending upon bootloader), and would execute the "goto _startup".  This
	//would effective reset the application.

	//To fix this situation, we should always deliberately place a
	//"goto REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS" at address 0x08, and a
	//"goto REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS" at address 0x18.  When the output
	//hex file of this project is programmed with the bootloader, these sections do not
	//get bootloaded (as they overlap the bootloader space).  If the output hex file is not
	//programmed using the bootloader, then the below goto instructions do get programmed,
	//and the hex file still works like normal.  The below section is only required to fix this
	//scenario.
	#pragma code HIGH_INTERRUPT_VECTOR = 0x08
	void High_ISR (void)
	{
	     _asm goto REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS _endasm
	}
	#pragma code LOW_INTERRUPT_VECTOR = 0x18
	void Low_ISR (void)
	{
	     _asm goto REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS _endasm
	}
	#endif	//end of "#if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)||defined(PROGRAMMABLE_WITH_USB_LEGACY_CUSTOM_CLASS_BOOTLOADER)"

	#pragma code

	//These are your actual interrupt handling routines.
	#pragma interrupt YourHighPriorityISRCode
	void YourHighPriorityISRCode()
	{
		//INTCONbits.GIEH = 0;
		//INTCONbits.GIEH = 1;

		//Check which interrupt flag caused the interrupt.
		//Service the interrupt
		//Clear the interrupt flag
		//Etc.
        #if defined(USB_INTERRUPT)
	        USBDeviceTasks();
        #endif
	
	}	//This return will be a "retfie fast", since this is in a #pragma interrupt section 
	#pragma interruptlow YourLowPriorityISRCode
	void YourLowPriorityISRCode()
	{
		//Check which interrupt flag caused the interrupt.
		//Service the interrupt
		//Clear the interrupt flag
		//Etc.
	
	}	//This return will be a "retfie", since this is in a #pragma interruptlow section 



/** DECLARATIONS ***************************************************/
#if defined(__18CXX)
    #pragma code
#endif

/******************************************************************************
 * Function:        void main(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Main program entry point.
 *
 * Note:            None
 *****************************************************************************/
#if defined(__18CXX)
void main(void)
#else
int main(void)
#endif
{   
    InitializeSystem();

    while(1)
    {
        #if defined(USB_INTERRUPT)
            if(USB_BUS_SENSE && (USBGetDeviceState() == DETACHED_STATE))
            {
                USBDeviceAttach();
            }
        #endif

        #if defined(USB_POLLING)
		// Check bus status and service USB interrupts.
        USBDeviceTasks(); // Interrupt or polling method.  If using polling, must call
        				  // this function periodically.  This function will take care
        				  // of processing and responding to SETUP transactions 
        				  // (such as during the enumeration process when you first
        				  // plug in).  USB hosts require that USB devices should accept
        				  // and process SETUP packets in a timely fashion.  Therefore,
        				  // when using polling, this function should be called 
        				  // regularly (such as once every 1.8ms or faster** [see 
        				  // inline code comments in usb_device.c for explanation when
        				  // "or faster" applies])  In most cases, the USBDeviceTasks() 
        				  // function does not take very long to execute (ex: <100 
        				  // instruction cycles) before it returns.
        #endif
    				  

		// Application-specific tasks.
		// Application related code may be added here, or in the ProcessIO() function.
        ProcessIO();        
    }//end while
}//end main


/********************************************************************
 * Function:        static void InitializeSystem(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        InitializeSystem is a centralize initialization
 *                  routine. All required USB initialization routines
 *                  are called from here.
 *
 *                  User application initialization routine should
 *                  also be called from here.                  
 *
 * Note:            None
 *******************************************************************/
static void InitializeSystem(void)
{
    ADCON1 |= 0x0F;                 // Default all pins to digital
    UserInit();
    USBDeviceInit();	//usb_device.c.  Initializes USB module SFRs and firmware
    					//variables to known states.
}//end InitializeSystem



/******************************************************************************
 * Function:        void UserInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine should take care of all of the demo code
 *                  initialization that is required.
 *
 * Note:            
 *
 *****************************************************************************/
void UserInit(void)
{
	// initialize
	LED1_TRIS = 0; // LED1 output
	LED2_TRIS = 0; // LED2 output
	IR_TRIS   = 1; // IR input
	SW_TRIS   = 1; // SW input
	IRLED_TRIS= 0; // IRLED input
	LED1_PORT = 0;
	LED2_PORT = 0;
	IRLED_PORT= 0;
	//IRLED_PORT= 1;

	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => *  2 =>  0.16666us -> *65536 = 10.923ms
	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => *  4 =>  0.33333us -> *65536 = 21.845ms
	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => *  8 =>  0.66666us -> *65536 = 43.69ms
	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => * 16 =>  1.33333us -> *65536 = 87.381ms
	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => * 32 =>  2.66666us -> *65536 = 174.76ms
	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => * 64 =>  5.33333us -> *65536 = 349.5ms
	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => *128 => 10.66666us -> *65536 = 699ms
	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => *256 => 21.33333us -> *65536 = 1398ms
	T0CON = TIMER_INT_ON &
		    T0_16BIT &
		    T0_SOURCE_INT &
		    T0_EDGE_RISE & // なんでもいい
		    T0_PS_1_64;

	// max 43.69[ms]
	T1CON = TIMER_INT_ON &
		    T1_16BIT_RW &
		    T1_SOURCE_INT &
		    T1_PS_1_8;


	// initilize other variables
    InitBuffer();
    AddBuffer(buff_user1, sizeof(buff_user1));
    AddBuffer(buff_user2, sizeof(buff_user2));
	ButtonInit(SW_BIT);
	enableReadIR = 1;
}//end UserInit

/********************************************************************
 * Function:        void ProcessIO(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is a place holder for other user
 *                  routines. It is a mixture of both USB and
 *                  non-USB tasks.
 *
 * Note:            None
 *******************************************************************/
void ProcessIO(void)
{   
	LED1_PORT = 0;
	IRLED_PORT = 0;

	//IRLED_PORT = 1;
	ButtonProc();
	ReadIR();
	SendIR();

    SerialProc();
    CDCTxService();
}

void SerialProc(void)
{
    BYTE numBytesRead;
	BYTE i;
    char buff[10];
    char *buffPtr;
    char *usbPtr;
    char state; // 0:readed ',' 1:readed number
	BYTE exit;
	WORD byteOrWord; // 0:未設定
	WORD pos;
    WORD v;
    WORD bpos;

    // User Application USB tasks
    if((USBDeviceState < CONFIGURED_STATE)||(USBSuspendControl==1))
        return;

	if(!USBUSARTIsTxTrfReady())
        return;

    bpos = 0;
    numBytesRead = getsUSBUSART(USB_Out_Buffer,64);
	if(numBytesRead == 0)
        return;

    state = 0;
    buffPtr = buff;
    usbPtr = USB_Out_Buffer;
	pos = 0;
	byteOrWord = 0;
	exit = 0;
 loop:
	for(i=0;i<numBytesRead;i++) {
        if (usbPtr[i]=='\r' || usbPtr[i]=='\n'){
            if (pos>0) {
                if (buffPtr==buff) {
                    sprintf(USB_In_Buffer,
                            (far rom char*)"parse error. need number. [%d,%c]\r\n",
                            bpos+i,usbPtr[i]);
                    PutsStringCPtr(USB_In_Buffer);
                    return;
                }
                *buffPtr++ = '\0';
                v = atoi(buff);
                exit += WriteBuffer(v,&pos,&byteOrWord);
                buffPtr = buff;

                WriteEOF(pos);
                PrintIRBuffer("echo,");
                SendIRImpl();
            }
            return;
        } else if (state==0) {
            if(usbPtr[i] != 'H' && usbPtr[i] != 'L') {
                sprintf(USB_In_Buffer,
                        (far rom char*)"parse error. need 'H' or 'L'. [%d,%c]\r\n",
                        bpos+i,usbPtr[i]);
                PutsStringCPtr(USB_In_Buffer);

                WriteEOF(pos);
                PrintIRBuffer("echo,");
                return;
            }
            state = 1;
        } else if (state==1) {
            if (usbPtr[i] == ',') {
                state = 0;
                if (buffPtr==buff) {
                    sprintf(USB_In_Buffer,
                            (far rom char*)"parse error. need number. [%d,%c]\r\n",
                            bpos+i,usbPtr[i]);
                    PutsStringCPtr(USB_In_Buffer);
                    WriteEOF(pos);
                    PrintIRBuffer("echo,");
                    return;
                }
                *buffPtr++ = '\0';
                v = atoi(buff);
                exit += WriteBuffer(v,&pos,&byteOrWord);
                buffPtr = buff;
            } else if ('0' <= usbPtr[i] && usbPtr[i] <= '9') {
                *buffPtr++ = usbPtr[i];
            } else {
                sprintf(USB_In_Buffer,
                        (far rom char*)"parse error. need ',' or number. [%d,%c]\r\n",
                        bpos+i,usbPtr[i]);
                PutsStringCPtr(USB_In_Buffer);
                WriteEOF(pos);
                PrintIRBuffer("echo,");
                return;
            }
        }
    }
    bpos += numBytesRead;
	numBytesRead = getsUSBUSART(USB_Out_Buffer,64);
    goto loop;
}


void ButtonProc(void)
{
	ButtonProcEveryMainLoop(PORTB);
	if (ButtonLongDownState() & SW_BIT)
		enableReadIR = !enableReadIR ;

	LED2_PORT = enableReadIR;
}

void ReadIR(void)
{
	WORD t;
	BYTE hilo;
	BYTE exit;
	WORD byteOrWord; // 0:未設定
	WORD pos;

	if (enableReadIR==0)
		return;

	if (IR_PORT == 1)
		return;

	// なにか信号があった

	WriteTimer0(0);
	INTCONbits.TMR0IF = 0;
	hilo = 0;

	pos = 0;
	byteOrWord = 0;
	exit = 0;
	while (exit==0) {
		LED1_PORT = !hilo;
		do {
			t = ReadTimer0();
			if (INTCONbits.TMR0IF || (t > MAX_WAIT_CYCLE)) {
				t = MAX_WAIT_CYCLE;
				INTCONbits.TMR0IF = 0;
				exit=1;
				break;
			}
		} while (IR_PORT == hilo);
		WriteTimer0(0);
		exit += WriteBuffer(t,&pos,&byteOrWord);
		hilo = !hilo;
	}
	LED1_PORT = 0;
    WriteEOF(pos);

	if (!WaitToReadySerial())
		return;
    PrintIRBuffer("received,");
}

void PrintIRBuffer(const rom char *title)
{
	WORD t;
	BYTE hilo;
	WORD byteOrWord; // 0:未設定
	WORD pos;
	char separator[2] = {'\0','\0'};

	PutsString("");  // なぜかこの出力がないと１発目が化ける

	PutsString(title);

	hilo = 1;
	pos = 0;
	byteOrWord = 0;
	while (1) {
		if (!WaitToReadySerial()) return;
		t = ReadBuffer(&pos,&byteOrWord);
		if (t==BUFF_EOF)
			break;
		sprintf(USB_In_Buffer, (far rom char*)"%s%c%u",
				separator,
				hilo ? 'H' : 'L',
				t);
		putsUSBUSART(USB_In_Buffer);
		hilo=!hilo;
		separator[0] = ',';
	}
	if (!WaitToReadySerial()) return;
	sprintf(USB_In_Buffer, (far rom char*)"\r\n");
	putsUSBUSART(USB_In_Buffer);
	if (!WaitToReadySerial()) return;
}

void SendIR(void)
{
	if ((ButtonUpState() & SW_BIT)==0)
		return;
    SendIRImpl();
}

void SendIRImpl(void)
{
	WORD t,wait;
	BYTE hilo;
	WORD pos;
	WORD byteOrWord; // 0:未設定

	WriteTimer0(0);
	INTCONbits.TMR0IF = 0;

	hilo = 1;
	pos = 0;
	byteOrWord = 0;
	while (1) {
		wait = ReadBuffer(&pos,&byteOrWord);
		if (wait==BUFF_EOF)
            break;
		if (wait==MAX_WAIT_CYCLE)
			break;
		do {
			LED1_PORT = hilo;
			//IRLED_PORT = !hilo;
			IRLED_PORT = hilo;
			DelayIRFreqHi();
			t = ReadTimer0();
			if (t >= wait)
				break;
			LED1_PORT = 0;
			//IRLED_PORT = 1;
			IRLED_PORT = 0;
			DelayIRFreqLo();
			t = ReadTimer0();
		} while(t < wait);

		WriteTimer0(0);
		hilo = !hilo;
		LED1_PORT = hilo;
		//IRLED_PORT = !hilo;
		IRLED_PORT = hilo;
	}

	// wait 10msec [1cycle==0.083333333us]
    Delay10KTCYx(120);
	LED1_PORT = 0;
	//IRLED_PORT = 1;
	IRLED_PORT = 0;

    PutsString("sended ir\r\n");

	LED1_PORT = 0;
	//IRLED_PORT = 1;
	IRLED_PORT = 0;
}

void PutsString(const rom char *str)
{
	if (!WaitToReadySerial()) return;
	strcpypgm2ram(USB_In_Buffer, (const far rom char*)str);
	putsUSBUSART(USB_In_Buffer);
	if (!WaitToReadySerial()) return;
}

void PutsStringCPtr(char *str)
{
	if (!WaitToReadySerial()) return;
	putsUSBUSART(str);
	if (!WaitToReadySerial()) return;
}


// 38KHz のデューティー比 33% の HI で待つ
// HI:105.263157894737[cycle] == 8.77192982456142[us]
// 38KHz のデューティー比 50% の 場合
// HI:157.894736842105[cycle] == 13.1578947368421[us]
void DelayIRFreqHi(void)
{
	// duty 1/3
	Delay10TCYx(10);
	Delay1TCY();
	Delay1TCY();
	Delay1TCY();
	Delay1TCY();
	Delay1TCY();

	// duty 1/2
	//Delay10TCYx(15);
	//Delay1TCY();
	//Delay1TCY();
	//Delay1TCY();
	//Delay1TCY();
	//Delay1TCY();
	//Delay1TCY();
	//Delay1TCY();
}

// 38KHz のデューティー比 33% の LO で待つ
// LO:210.526315789474[cycle] == 17.5438596491228[us]
// 38KHz のデューティー比 50% の 場合
// LO:157.894736842105[cycle] == 13.1578947368421[us]
void DelayIRFreqLo(void)
{
	// duty 1/3
	Delay10TCYx(21);
	Delay1TCY();

	// duty 1/2
	//Delay10TCYx(15);
	//Delay1TCY();
	//Delay1TCY();
	//Delay1TCY();
	//Delay1TCY();
	//Delay1TCY();
	//Delay1TCY();
	//Delay1TCY();
}

/**
 * シリアル・ポートの Ready を待つ
 * timer1 を使ってオーバフローならば 0 を返す
 * Ready になれば 1 を返す
 */
int WaitToReadySerial(void)
{
	WriteTimer1(0);
	PIR1bits.TMR1IF = 0;
	while(PIR1bits.TMR1IF==0) {
		if (mUSBUSARTIsTxTrfReady())
			return 1;
		CDCTxService();
	}
	return 0;
}

// ******************************************************************************************************
// ************** USB Callback Functions ****************************************************************
// ******************************************************************************************************
// The USB firmware stack will call the callback functions USBCBxxx() in response to certain USB related
// events.  For example, if the host PC is powering down, it will stop sending out Start of Frame (SOF)
// packets to your device.  In response to this, all USB devices are supposed to decrease their power
// consumption from the USB Vbus to <2.5mA* each.  The USB module detects this condition (which according
// to the USB specifications is 3+ms of no bus activity/SOF packets) and then calls the USBCBSuspend()
// function.  You should modify these callback functions to take appropriate actions for each of these
// conditions.  For example, in the USBCBSuspend(), you may wish to add code that will decrease power
// consumption from Vbus to <2.5mA (such as by clock switching, turning off LEDs, putting the
// microcontroller to sleep, etc.).  Then, in the USBCBWakeFromSuspend() function, you may then wish to
// add code that undoes the power saving things done in the USBCBSuspend() function.

// The USBCBSendResume() function is special, in that the USB stack will not automatically call this
// function.  This function is meant to be called from the application firmware instead.  See the
// additional comments near the function.

// Note *: The "usb_20.pdf" specs indicate 500uA or 2.5mA, depending upon device classification. However,
// the USB-IF has officially issued an ECN (engineering change notice) changing this to 2.5mA for all 
// devices.  Make sure to re-download the latest specifications to get all of the newest ECNs.

/******************************************************************************
 * Function:        void USBCBSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Call back that is invoked when a USB suspend is detected
 *
 * Note:            None
 *****************************************************************************/
void USBCBSuspend(void)
{
	//Example power saving code.  Insert appropriate code here for the desired
	//application behavior.  If the microcontroller will be put to sleep, a
	//process similar to that shown below may be used:
	
	//ConfigureIOPinsForLowPower();
	//SaveStateOfAllInterruptEnableBits();
	//DisableAllInterruptEnableBits();
	//EnableOnlyTheInterruptsWhichWillBeUsedToWakeTheMicro();	//should enable at least USBActivityIF as a wake source
	//Sleep();
	//RestoreStateOfAllPreviouslySavedInterruptEnableBits();	//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.
	//RestoreIOPinsToNormal();									//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.

	//IMPORTANT NOTE: Do not clear the USBActivityIF (ACTVIF) bit here.  This bit is 
	//cleared inside the usb_device.c file.  Clearing USBActivityIF here will cause 
	//things to not work as intended.	
	

    #if defined(__C30__) || defined __XC16__
        USBSleepOnSuspend();
    #endif
}

/******************************************************************************
 * Function:        void USBCBWakeFromSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The host may put USB peripheral devices in low power
 *					suspend mode (by "sending" 3+ms of idle).  Once in suspend
 *					mode, the host may wake the device back up by sending non-
 *					idle state signalling.
 *					
 *					This call back is invoked when a wakeup from USB suspend 
 *					is detected.
 *
 * Note:            None
 *****************************************************************************/
void USBCBWakeFromSuspend(void)
{
	// If clock switching or other power savings measures were taken when
	// executing the USBCBSuspend() function, now would be a good time to
	// switch back to normal full power run mode conditions.  The host allows
	// 10+ milliseconds of wakeup time, after which the device must be 
	// fully back to normal, and capable of receiving and processing USB
	// packets.  In order to do this, the USB module must receive proper
	// clocking (IE: 48MHz clock must be available to SIE for full speed USB
	// operation).  
	// Make sure the selected oscillator settings are consistent with USB 
    // operation before returning from this function.
}

/********************************************************************
 * Function:        void USBCB_SOF_Handler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USB host sends out a SOF packet to full-speed
 *                  devices every 1 ms. This interrupt may be useful
 *                  for isochronous pipes. End designers should
 *                  implement callback routine as necessary.
 *
 * Note:            None
 *******************************************************************/
void USBCB_SOF_Handler(void)
{
    // No need to clear UIRbits.SOFIF to 0 here.
    // Callback caller is already doing that.
	ButtonProcEvery1ms();
}

/*******************************************************************
 * Function:        void USBCBErrorHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The purpose of this callback is mainly for
 *                  debugging during development. Check UEIR to see
 *                  which error causes the interrupt.
 *
 * Note:            None
 *******************************************************************/
void USBCBErrorHandler(void)
{
    // No need to clear UEIR to 0 here.
    // Callback caller is already doing that.

	// Typically, user firmware does not need to do anything special
	// if a USB error occurs.  For example, if the host sends an OUT
	// packet to your device, but the packet gets corrupted (ex:
	// because of a bad connection, or the user unplugs the
	// USB cable during the transmission) this will typically set
	// one or more USB error interrupt flags.  Nothing specific
	// needs to be done however, since the SIE will automatically
	// send a "NAK" packet to the host.  In response to this, the
	// host will normally retry to send the packet again, and no
	// data loss occurs.  The system will typically recover
	// automatically, without the need for application firmware
	// intervention.
	
	// Nevertheless, this callback function is provided, such as
	// for debugging purposes.
}


/*******************************************************************
 * Function:        void USBCBCheckOtherReq(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        When SETUP packets arrive from the host, some
 * 					firmware must process the request and respond
 *					appropriately to fulfill the request.  Some of
 *					the SETUP packets will be for standard
 *					USB "chapter 9" (as in, fulfilling chapter 9 of
 *					the official USB specifications) requests, while
 *					others may be specific to the USB device class
 *					that is being implemented.  For example, a HID
 *					class device needs to be able to respond to
 *					"GET REPORT" type of requests.  This
 *					is not a standard USB chapter 9 request, and 
 *					therefore not handled by usb_device.c.  Instead
 *					this request should be handled by class specific 
 *					firmware, such as that contained in usb_function_hid.c.
 *
 * Note:            None
 *******************************************************************/
void USBCBCheckOtherReq(void)
{
    USBCheckCDCRequest();
}//end


/*******************************************************************
 * Function:        void USBCBStdSetDscHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USBCBStdSetDscHandler() callback function is
 *					called when a SETUP, bRequest: SET_DESCRIPTOR request
 *					arrives.  Typically SET_DESCRIPTOR requests are
 *					not used in most applications, and it is
 *					optional to support this type of request.
 *
 * Note:            None
 *******************************************************************/
void USBCBStdSetDscHandler(void)
{
    // Must claim session ownership if supporting this request
}//end


/*******************************************************************
 * Function:        void USBCBInitEP(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called when the device becomes
 *                  initialized, which occurs after the host sends a
 * 					SET_CONFIGURATION (wValue not = 0) request.  This 
 *					callback function should initialize the endpoints 
 *					for the device's usage according to the current 
 *					configuration.
 *
 * Note:            None
 *******************************************************************/
void USBCBInitEP(void)
{
    //Enable the CDC data endpoints
    CDCInitEP();
}

/********************************************************************
 * Function:        void USBCBSendResume(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USB specifications allow some types of USB
 * 					peripheral devices to wake up a host PC (such
 *					as if it is in a low power suspend to RAM state).
 *					This can be a very useful feature in some
 *					USB applications, such as an Infrared remote
 *					control	receiver.  If a user presses the "power"
 *					button on a remote control, it is nice that the
 *					IR receiver can detect this signalling, and then
 *					send a USB "command" to the PC to wake up.
 *					
 *					The USBCBSendResume() "callback" function is used
 *					to send this special USB signalling which wakes 
 *					up the PC.  This function may be called by
 *					application firmware to wake up the PC.  This
 *					function will only be able to wake up the host if
 *                  all of the below are true:
 *					
 *					1.  The USB driver used on the host PC supports
 *						the remote wakeup capability.
 *					2.  The USB configuration descriptor indicates
 *						the device is remote wakeup capable in the
 *						bmAttributes field.
 *					3.  The USB host PC is currently sleeping,
 *						and has previously sent your device a SET 
 *						FEATURE setup packet which "armed" the
 *						remote wakeup capability.   
 *
 *                  If the host has not armed the device to perform remote wakeup,
 *                  then this function will return without actually performing a
 *                  remote wakeup sequence.  This is the required behavior, 
 *                  as a USB device that has not been armed to perform remote 
 *                  wakeup must not drive remote wakeup signalling onto the bus;
 *                  doing so will cause USB compliance testing failure.
 *                  
 *					This callback should send a RESUME signal that
 *                  has the period of 1-15ms.
 *
 * Note:            This function does nothing and returns quickly, if the USB
 *                  bus and host are not in a suspended condition, or are 
 *                  otherwise not in a remote wakeup ready state.  Therefore, it
 *                  is safe to optionally call this function regularly, ex: 
 *                  anytime application stimulus occurs, as the function will
 *                  have no effect, until the bus really is in a state ready
 *                  to accept remote wakeup. 
 *
 *                  When this function executes, it may perform clock switching,
 *                  depending upon the application specific code in 
 *                  USBCBWakeFromSuspend().  This is needed, since the USB
 *                  bus will no longer be suspended by the time this function
 *                  returns.  Therefore, the USB module will need to be ready
 *                  to receive traffic from the host.
 *
 *                  The modifiable section in this routine may be changed
 *                  to meet the application needs. Current implementation
 *                  temporary blocks other functions from executing for a
 *                  period of ~3-15 ms depending on the core frequency.
 *
 *                  According to USB 2.0 specification section 7.1.7.7,
 *                  "The remote wakeup device must hold the resume signaling
 *                  for at least 1 ms but for no more than 15 ms."
 *                  The idea here is to use a delay counter loop, using a
 *                  common value that would work over a wide range of core
 *                  frequencies.
 *                  That value selected is 1800. See table below:
 *                  ==========================================================
 *                  Core Freq(MHz)      MIP         RESUME Signal Period (ms)
 *                  ==========================================================
 *                      48              12          1.05
 *                       4              1           12.6
 *                  ==========================================================
 *                  * These timing could be incorrect when using code
 *                    optimization or extended instruction mode,
 *                    or when having other interrupts enabled.
 *                    Make sure to verify using the MPLAB SIM's Stopwatch
 *                    and verify the actual signal on an oscilloscope.
 *******************************************************************/
void USBCBSendResume(void)
{
    static WORD delay_count;
    
    //First verify that the host has armed us to perform remote wakeup.
    //It does this by sending a SET_FEATURE request to enable remote wakeup,
    //usually just before the host goes to standby mode (note: it will only
    //send this SET_FEATURE request if the configuration descriptor declares
    //the device as remote wakeup capable, AND, if the feature is enabled
    //on the host (ex: on Windows based hosts, in the device manager 
    //properties page for the USB device, power management tab, the 
    //"Allow this device to bring the computer out of standby." checkbox 
    //should be checked).
    if(USBGetRemoteWakeupStatus() == TRUE) 
    {
        //Verify that the USB bus is in fact suspended, before we send
        //remote wakeup signalling.
        if(USBIsBusSuspended() == TRUE)
        {
            USBMaskInterrupts();
            
            //Clock switch to settings consistent with normal USB operation.
            USBCBWakeFromSuspend();
            USBSuspendControl = 0; 
            USBBusIsSuspended = FALSE;  //So we don't execute this code again, 
                                        //until a new suspend condition is detected.

            //Section 7.1.7.7 of the USB 2.0 specifications indicates a USB
            //device must continuously see 5ms+ of idle on the bus, before it sends
            //remote wakeup signalling.  One way to be certain that this parameter
            //gets met, is to add a 2ms+ blocking delay here (2ms plus at 
            //least 3ms from bus idle to USBIsBusSuspended() == TRUE, yeilds
            //5ms+ total delay since start of idle).
            delay_count = 3600U;        
            do
            {
                delay_count--;
            }while(delay_count);
            
            //Now drive the resume K-state signalling onto the USB bus.
            USBResumeControl = 1;       // Start RESUME signaling
            delay_count = 1800U;        // Set RESUME line for 1-13 ms
            do
            {
                delay_count--;
            }while(delay_count);
            USBResumeControl = 0;       //Finished driving resume signalling

            USBUnmaskInterrupts();
        }
    }
}


/*******************************************************************
 * Function:        void USBCBEP0DataReceived(void)
 *
 * PreCondition:    ENABLE_EP0_DATA_RECEIVED_CALLBACK must be
 *                  defined already (in usb_config.h)
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called whenever a EP0 data
 *                  packet is received.  This gives the user (and
 *                  thus the various class examples a way to get
 *                  data that is received via the control endpoint.
 *                  This function needs to be used in conjunction
 *                  with the USBCBCheckOtherReq() function since 
 *                  the USBCBCheckOtherReq() function is the apps
 *                  method for getting the initial control transfer
 *                  before the data arrives.
 *
 * Note:            None
 *******************************************************************/
#if defined(ENABLE_EP0_DATA_RECEIVED_CALLBACK)
void USBCBEP0DataReceived(void)
{
}
#endif

/*******************************************************************
 * Function:        BOOL USER_USB_CALLBACK_EVENT_HANDLER(
 *                        int event, void *pdata, WORD size)
 *
 * PreCondition:    None
 *
 * Input:           int event - the type of event
 *                  void *pdata - pointer to the event data
 *                  WORD size - size of the event data
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called from the USB stack to
 *                  notify a user application that a USB event
 *                  occured.  This callback is in interrupt context
 *                  when the USB_INTERRUPT option is selected.
 *
 * Note:            None
 *******************************************************************/
BOOL USER_USB_CALLBACK_EVENT_HANDLER(int event, void *pdata, WORD size)
{
    switch( event )
    {
        case EVENT_TRANSFER:
            //Add application specific callback task or callback function here if desired.
            break;
        case EVENT_SOF:
            USBCB_SOF_Handler();
            break;
        case EVENT_SUSPEND:
            USBCBSuspend();
            break;
        case EVENT_RESUME:
            USBCBWakeFromSuspend();
            break;
        case EVENT_CONFIGURED: 
            USBCBInitEP();
            break;
        case EVENT_SET_DESCRIPTOR:
            USBCBStdSetDscHandler();
            break;
        case EVENT_EP0_REQUEST:
            USBCBCheckOtherReq();
            break;
        case EVENT_BUS_ERROR:
            USBCBErrorHandler();
            break;
        case EVENT_TRANSFER_TERMINATED:
            //Add application specific callback task or callback function here if desired.
            //The EVENT_TRANSFER_TERMINATED event occurs when the host performs a CLEAR
            //FEATURE (endpoint halt) request on an application endpoint which was 
            //previously armed (UOWN was = 1).  Here would be a good place to:
            //1.  Determine which endpoint the transaction that just got terminated was 
            //      on, by checking the handle value in the *pdata.
            //2.  Re-arm the endpoint if desired (typically would be the case for OUT 
            //      endpoints).
            break;
        default:
            break;
    }      
    return TRUE; 
}


/** EOF main.c *************************************************/

