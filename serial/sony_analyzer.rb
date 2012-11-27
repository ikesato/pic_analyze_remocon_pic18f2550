
# T:0.6ms 
# Leader : H4T L1T
# 0      : H1T L1T
# 1      : H2T L1T
class SonyAnalyzer
  T = 0.6 # [ms]
  FRAME = 45.0 # 1フレームの長さ [ms]
  NAME = "sony"

  def self.parse(raw)
    i = SonyAnalyzer.new
    return i if i._parse(raw)    
    nil
  end

  def _parse(raw)
    return false if raw.length < 2+8
    return read_frame(raw)
  end

  def dump
    "#{NAME} [#{@values.map{|v| sprintf("0x%08x",v)}.join(",")}]"
  end

  private
  def read_frame(raw)
    @values = []

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
        count += 1
        if (1*T - hi).abs / 1*T < 0.1
          ;
        elsif (2*T - hi).abs / 2*T < 0.1
          bits |= (1<<count)
        else
          return false
        end

        if (1*T - lo).abs / 1*T < 0.1
          ;
        elsif count==12 || count==15 || count==20
          # 1フレーム終わり
          @values << bits
          readed = true
          break
        else
          return false
        end
      end
      return false if readed==false
    end
    return true
  end
end
