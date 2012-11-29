# -*- coding: utf-8 -*-

# T:0.6ms 
# Leader : H4T L1T
# 0      : H1T L1T
# 1      : H2T L1T
class SonyFormat
  T = 0.6 # [ms]
  FRAME_CYCLE = 45.0 # 1フレームの長さ [ms]
  NAME = "sony"

  def self.parse(raw)
    i = SonyFormat.new
    return i if i._parse(raw)    
    nil
  end

  def _parse(raw)
    return false if raw.length < 2+8
    return read_frame(raw)
  end

  def dump
    "#{NAME} #{@bit_length}bit [#{@values.map{|v| sprintf("0x%08x",v)}.join(",")}]"
  end

  def self.make_send_ary(str)
    return false if str !~ /#{NAME} (\d+)bit \[([\da-fA-F0x,]+)\]/
    bl = $1.to_i # bit length
    ary = $2.split(",").map{|s| s.hex}

    frames = []
    ary.each {|v|
      frame = []
      # leader
      frame << 4*T
      frame << 1*T
      bl.times {|i|
        # hi
        if v & (1 << i) != 0
          frame << 2*T
        else
          frame << 1*T
        end
        # lo
        frame << 1*T
      }
      fill = FRAME_CYCLE - frame.inject(0){|sum,i| sum+i}
      if fill > 0
        frame[frame.length-1] = frame.last + fill
      end
      frames += frame
    }
    
    frames
  end

  private
  def read_frame(raw)
    @values = []
    frame_cycle = []
    bits_length = []

    while raw.length >= 2
      frame = 0
      frame += raw[0]+raw[1]
      leader = [raw.shift, raw.shift]
      return false if (4*T - leader[0]).abs/4*T >= 0.1
      return false if (1*T - leader[1]).abs/1*T >= 0.1

      count = 0
      bits = 0
      readed = false
      while raw.length >= 2
        frame += raw[0]+raw[1]
        hi = raw.shift
        lo = raw.shift
        if (1*T - hi).abs / 1*T < 0.1
          ;
        elsif (2*T - hi).abs / 2*T < 0.1
          bits |= (1 << count)
        else
          return false
        end
        count += 1

        if (1*T - lo).abs / 1*T < 0.1
          ;
        elsif count==12 || count==15 || count==20
          # 1フレーム終わり
          @values << bits
          frame_cycle << frame
          bits_length << count
          readed = true
          break
        else
          return false
        end
      end
      return false if readed==false
    end
    @bit_length = bits_length.inject(0){|sum, i| sum + i}/bits_length.length
    return false if @bit_length != bits_length[0] #全bit長が同じであること
    return true
  end
end
