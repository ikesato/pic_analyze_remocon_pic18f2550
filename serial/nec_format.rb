# -*- coding: utf-8 -*-

# T:0.56ms (1T=105)
# Leader : H16T L8T
# 0      : H1T L1T
# 1      : H1T L3T
# bitlen : 32,42
class NECFormat
  T = 0.56 # [ms]
  FRAME_CYCLE = 108.0 # 1フレームの長さ [ms]
  NAME = "nec"
  REPEATER4T = "REP4T"
  REPEATER8T = "REP8T"

  def self.parse(raw)
    i = NECFormat.new
    return i if i._parse(raw)
    nil
  end

  def _parse(raw)
    return false if raw.length < 2+8
    return read_frame(raw)
  end

  def dump
    values_str = @values.map {|v|
      if v==REPEATER4T || v==REPEATER8T
        v
      else
        sprintf("%02x",v)
      end
    }
    "#{NAME} #{@bit_length}bit [#{values_str.join(" ")}]"
  end

  def self.make_send_ary(str)
    # TODO:
    return false if str !~ /#{NAME} (\d+)bit \[((?:[\da-fA-F ]|#{REPEATER4T}|#{REPEATER8T})+)\]/
    bl = $1.to_i # bit length
    ary = $2.split(" ").map {|v|
      if v==REPEATER4T || v==REPEATER8T
        v
      else
        v.hex
      end
    }

    frames = []
    need_leader=true
    bitpos=0
    frame=[]
    ary.each {|v|
      if v==REPEATER4T
        frame=[]
        frame << 16*T
        frame << 4*T
        frame << 1*T
        frame << (FRAME_CYCLE - frame.inject(0){|sum,i| sum+i})
        need_leader=true
      elsif v==REPEATER8T
        frame=[]
        frame << 16*T
        frame << 8*T
        frame << 1*T
        frame << (FRAME_CYCLE - frame.inject(0){|sum,i| sum+i})
        need_leader=true
      else
        if need_leader
          need_leader = false
          frame=[]
          frame << 16*T
          frame << 8*T
          bitpos=0
        end
        if bitpos+8 >= bl
          last=true
          count=bl-bitpos
        else
          last=false
          count=8
        end
        bitpos += 8
        count.times {|i|
          # hi
          frame << 1*T
          # lo
          if v & (1 << i) != 0
            frame << 3*T
          else
            frame << 1*T
          end
        }
        if last
          frame << 1*T
          frame << FRAME_CYCLE - frame.inject(0){|sum,i| sum+i}
        else
          next
        end
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
      frame = raw[0]+raw[1]
      leader = [raw.shift, raw.shift]

      if (16*T - leader[0]).abs/16*T < 0.1 && (8*T - leader[1]).abs/8*T < 0.1
        values, _frame, bitlen = read_code(raw,frame)
        return false if values.nil?
        @values += values
        frame_cycle << _frame+frame unless _frame.nil?
        bits_length << bitlen unless bitlen.nil?
      elsif (16*T - leader[0]).abs/16*T < 0.1 && (4*T - leader[1]).abs/4*T < 0.1
        # TODO:実際のリモコンで試したい
        frame = read_repeater(raw,frame)
        return false if frame.nil?
        @values << REPEATER4T
        frame_cycle << frame
      else
        return false
      end
    end
    @bit_length = bits_length.inject(0){|sum, i| sum + i}/bits_length.length
    return false if @bit_length != bits_length[0] #全bit長が同じであること

    if frame_cycle.length>1
      frame_cycle.each {|f|
        return false if (FRAME_CYCLE - f).abs/FRAME_CYCLE >= 0.1
      }
    end
    return true
  end

  def read_code(raw,frame_leader)
    count = 0
    values = []
    bit = 0
    frame = 0
    while raw.length >= 2
      frame += raw[0]+raw[1]
      hi = raw.shift
      lo = raw.shift
      return nil if (1*T - hi).abs / 1*T > 0.1

      v = nil
      if (1*T - lo).abs / 1*T < 0.1
        v = 0
      elsif (3*T - lo).abs / 3*T < 0.1
        v = 1
      end

      if v==1
        bit |= (1 << (count%8))
      end

      if (count+1)%8 == 0
        values << bit
        bit = 0
      end

      if v.nil?
        if count==32 || count==42
          # 1フレーム終わり
          values << bit if count==42
          return values, frame, count
        else
          if count==0
            # leader が同じでリピータの場合がある
            if raw.empty?
              return [REPEATER8T], nil, nil
            end
            frame = read_repeater([hi,lo],frame_leader,true)
            unless frame.nil?
              return [REPEATER8T], frame, nil
            end
          end
          return nil
        end
      end
      count += 1
    end
    nil
  end

  def read_repeater(raw,frame_leader,force_check=false)
    return nil if raw.length < 2
    hi = raw.shift
    lo = raw.shift
    frame = hi+lo
    return nil if (1*T - hi).abs / 1*T > 0.1
    if raw.length>0 || force_check
      # データが終端でなければ長さチェックを行う
      return nil if (FRAME_CYCLE - (frame+frame_leader)).abs / FRAME_CYCLE > 0.1
    else
      frame = FRAME_CYCLE-frame_leader
    end
    frame
  end
end
