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

#include "HardwareProfile.h"

#define LED_TRIS    TRISAbits.TRISA0
#define IR_TRIS     TRISBbits.TRISB3
#define SW_TRIS     TRISBbits.TRISB4
#define IRLED_TRIS  TRISCbits.TRISC2
#define LED_PORT    PORTAbits.RA0
#define IR_PORT     PORTBbits.RB3
#define SW_PORT     PORTBbits.RB4
#define IRLED_PORT  PORTCbits.RC2


/** V A R I A B L E S ********************************************************/
// 順番重要:先にアドレス順で後の udata から定義するとバンク内にきちんと収まる
//          これを udata の後に書くと収まらなくなる
// http://tylercsf.blog123.fc2.com/blog-entry-189.html
//#pragma udata USER_RAM2=0x5A0 // 5a0 - 7ff=0x260
//#define BUFF_U2_WSIZE 0x130
//#define BUFF_U2_CSIZE 0x260
#pragma udata USER_RAM2=0x600 // 5a0 - 7ff=0x260
#define BUFF_U2_WSIZE 0x100
#define BUFF_U2_CSIZE 0x200
union {
	WORD wBuff[BUFF_U2_WSIZE];
	char cBuff[BUFF_U2_CSIZE];
} buff_user_ram2;

#pragma udata USER_RAM1=0x200
#define BUFF_U1_WSIZE 0x100
#define BUFF_U1_CSIZE 0x200
union {
	WORD wBuff[BUFF_U1_WSIZE];
	char cBuff[BUFF_U1_CSIZE];
} buff_user_ram1;

#define BUFF_WSIZE (BUFF_U1_WSIZE+BUFF_U2_WSIZE)
#define BUFF_CSIZE (BUFF_U1_CSIZE+BUFF_U2_CSIZE)



#if defined(__18CXX)
    #pragma udata
#endif

char USB_In_Buffer[64];
char USB_Out_Buffer[64];

BOOL stringPrinted;
volatile BOOL buttonPressed;
volatile BYTE buttonCount;
WORD bufferLen;


/** P R I V A T E  P R O T O T Y P E S ***************************************/
static void InitializeSystem(void);
void ProcessIO(void);
void USBDeviceTasks(void);
void YourHighPriorityISRCode();
void YourLowPriorityISRCode();
void USBCBSendResume(void);
void BlinkUSBStatus(void);
void UserInit(void);
void ReadIR(void);
void SendIR(void);
int WaitToReadySerial(void);
WORD ReadWORDBuffer(WORD i);
void WriteWORDBuffer(WORD i, WORD v);

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
    //Initialize all of the debouncing variables
    buttonCount = 0;
    buttonPressed = 0;
    stringPrinted = TRUE;
	bufferLen = 0;

	// initialize
	LED_TRIS  = 0; // LED output
	IR_TRIS   = 1; // IR input
	SW_TRIS   = 1; // SW input
	IRLED_TRIS= 0; // IRLED input
	LED_PORT  = 0;
	IRLED_PORT= 0;

//	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => *  2 =>  0.16666us -> *65536 = 10.923ms
//	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => *  4 =>  0.33333us -> *65536 = 21.845ms
//	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => *  8 =>  0.66666us -> *65536 = 43.69ms
//	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => * 16 =>  1.33333us -> *65536 = 87.381ms
//	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => * 32 =>  2.66666us -> *65536 = 174.76ms
//	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => * 64 =>  5.33333us -> *65536 = 349.5ms
//	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => *128 => 10.66666us -> *65536 = 699ms
//	// timer0 48MHz/4=12MHz => 0.08333[us/cycle] => *256 => 21.33333us -> *65536 = 1398ms

//	T0CONbits.T0PS = 0x7; // prescaler 111=1:256 110=1:128 ... 001=1:4 000=1:2
//	T0CONbits.PSA = 0;
//	T0CONbits.T0SE = 0; // なんでもいい
//	T0CONbits.T0CS = 0;
//	T0CONbits.T08BIT = 0;
//	T0CONbits.TMR0ON = 1;
	//T0CON = 0b10000111;
	//T0CON = 0b10000000;
	T0CON = TIMER_INT_ON &
		  //TIMER_INT_OFF &
		    T0_16BIT &
		    T0_SOURCE_INT &
		    T0_EDGE_RISE & // なんでもいい
		    T0_PS_1_64;

	// max 43.69
	T1CON = TIMER_INT_ON &
		    T1_16BIT_RW &
		    T1_SOURCE_INT &
		    T1_PS_1_8;
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
    BYTE numBytesRead;

//	{
//		while(!mUSBUSARTIsTxTrfReady()) CDCTxService();
//		sprintf(USB_In_Buffer, (far rom char*)"P0 INTCON=%02x TMR0=%u\r\n", INTCON, ReadTimer0());
//		putsUSBUSART(USB_In_Buffer);
//		while(!mUSBUSARTIsTxTrfReady()) CDCTxService();
//	}

	ReadIR();
	SendIR();

//	while(1){
//	    // ポートA,B,Cをオンにする
//		LED_PORT = 1;
//	    // 1秒待つ
//	    Delay10KTCYx(240); Delay10KTCYx(240); Delay10KTCYx(240); Delay10KTCYx(240);
//	
//	
//	    // ポートA,B,Cをオフにする
//		LED_PORT = 0;
//	    // 1秒待つ
//	    Delay10KTCYx(240); Delay10KTCYx(240); Delay10KTCYx(240); Delay10KTCYx(240);
//	}

    // User Application USB tasks
    if((USBDeviceState < CONFIGURED_STATE)||(USBSuspendControl==1)) return;

//	if(buttonPressed)
//	{
//	    if(stringPrinted == FALSE)
//	    {
//	        if(mUSBUSARTIsTxTrfReady())
//	        {
//	            putrsUSBUSART("Button Pressed -- \r\n");
//	            stringPrinted = TRUE;
//	        }
//	    }
//	}
//	else
//	{
//	    stringPrinted = FALSE;
//	}

//	if(USBUSARTIsTxTrfReady())
//	{
//		numBytesRead = getsUSBUSART(USB_Out_Buffer,64);
//		if(numBytesRead != 0)
//		{
//			BYTE i;
//	        
//			for(i=0;i<numBytesRead;i++)
//			{
//				switch(USB_Out_Buffer[i])
//				{
//					case 0x0A:
//					case 0x0D:
//						USB_In_Buffer[i] = USB_Out_Buffer[i];
//						break;
//					default:
//						USB_In_Buffer[i] = USB_Out_Buffer[i] + 1;
//						break;
//				}
//	
//			}
//	
//			putUSBUSART(USB_In_Buffer,numBytesRead);
//		}
//	}

    CDCTxService();
}		//end ProcessIO

void ReadIR(void)
{
	WORD t;
	BYTE hilo;
	WORD i;
	BYTE exit;

	if (IR_PORT == 1)
		return;

	//TMR0H=0; TMR0L=0; // 順番重要
	WriteTimer0(0);
	INTCONbits.TMR0IF = 0;
	//hilo = IR_PORT;
	hilo = 0;

	// なにか信号があった

	exit = 0;
	for (i=0; i<BUFF_WSIZE && exit==0; i++)
	{
		LED_PORT = !hilo;
		while (IR_PORT == hilo) {
			t = ReadTimer0();
			//if (hilo == 1 && t > 60000)
			//if (t > 32760)
			if (INTCONbits.TMR0IF || (t > 60000)) {
			//if (INTCONbits.TMR0IF) {
				INTCONbits.TMR0IF = 0;
				exit=1;
				break;
			}
		}
		t = ReadTimer0();
		WriteTimer0(0);
		WriteWORDBuffer(i,t);
		hilo = !hilo;
	}
	LED_PORT = 0;

	bufferLen = i;
	if (bufferLen==0)
		return;
	if (bufferLen==BUFF_WSIZE)
		bufferLen--;
	if (!WaitToReadySerial())
		return;
	for (i=0;i<bufferLen;i++) {
		if (!WaitToReadySerial()) return;
		sprintf(USB_In_Buffer, (far rom char*)"|%c%u",
				(i&0x1) == 0 ? 'H' : 'L',
				ReadWORDBuffer(i));
		putsUSBUSART(USB_In_Buffer);
	}
	if (!WaitToReadySerial()) return;
	sprintf(USB_In_Buffer, (far rom char*)"|\r\n");
	putsUSBUSART(USB_In_Buffer);
	if (!WaitToReadySerial()) return;

//	{
//		LED_PORT = !IR_PORT;
//	}
}

void SendIR(void)
{
	WORD t,wait;
	BYTE hilo;
	WORD i;

	if (buttonPressed == 0)
		return;

	WriteTimer0(0);
	INTCONbits.TMR0IF = 0;
	//LED_PORT = IRLED_PORT = 1;
	for (i=0;i<bufferLen;i++) {
		hilo = !(i&1);
		wait = ReadWORDBuffer(i);
		do {
			LED_PORT = IRLED_PORT = hilo;
			Delay10TCYx(17);
			t = ReadTimer0();
			if (t >= wait)
				break;
			LED_PORT = IRLED_PORT = 0;
			Delay10TCYx(17);
			t = ReadTimer0();
		} while(t < wait);

		WriteTimer0(0);
		LED_PORT = IRLED_PORT = (i&1);
	}
	LED_PORT = IRLED_PORT = 0;

	if (!WaitToReadySerial()) return;
	sprintf(USB_In_Buffer, (far rom char*)"sended ir\r\n");
	putsUSBUSART(USB_In_Buffer);
	if (!WaitToReadySerial()) return;

	// wait 10msec (250msec == 240count)
    Delay10KTCYx(10);
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

WORD ReadWORDBuffer(WORD i)
{
	WORD *p;
	if (i<BUFF_U1_WSIZE) {
		p = buff_user_ram1.wBuff;
	} else if (i<(BUFF_U1_WSIZE+BUFF_U2_WSIZE)) {
		p = buff_user_ram2.wBuff;
		i -= BUFF_U1_WSIZE;
//	} else if (i<(BUFF_U1_WSIZE+BUFF_U2_WSIZE+BUFF_U3_WSIZE)) {
//		p = buff_user_ram3.wBuff;
//		i -= BUFF_U1_WSIZE+BUFF_U2_WSIZE;
	} else {
		return 0xffff;
	}
	return p[i];
}

void WriteWORDBuffer(WORD i, WORD v)
{
	WORD *p;
	if (i<BUFF_U1_WSIZE) {
		p = buff_user_ram1.wBuff;
	} else if (i<BUFF_U1_WSIZE+BUFF_U2_WSIZE) {
		p = buff_user_ram2.wBuff;
		i -= BUFF_U1_WSIZE;
//	} else if (i<(BUFF_U1_WSIZE+BUFF_U2_WSIZE+BUFF_U3_WSIZE)) {
//		p = buff_user_ram3.wBuff;
//		i -= BUFF_U1_WSIZE+BUFF_U2_WSIZE;
	} else {
		return;
	}
	p[i] = v;
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

    //This is reverse logic since the pushbutton is active low
    if(buttonPressed == SW_PORT)
    {
        if(buttonCount != 0)
        {
            buttonCount--;
        }
        else
        {
            //This is reverse logic since the pushbutton is active low
            buttonPressed = !SW_PORT;

            //Wait 100ms before the next press can be generated
            buttonCount = 100;
        }
    }
    else
    {
        if(buttonCount != 0)
        {
            buttonCount--;
        }
    }
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

