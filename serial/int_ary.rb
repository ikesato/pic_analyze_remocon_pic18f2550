class Integer
  def to_a
    v=self
    ret = []
    begin
      ret << (v&0xff)
      v >>= 8
    end while v!=0
    ret
  end

  def to_hex
    return sprintf("%02xh",self) if self < (1 << 8)
    return sprintf("%04xh",self) if self < (1 << 16)
    return sprintf("%08xh",self) if self < (1 << 32)
    sprintf("%xh",self)
  end
end

class Array
  def to_num
    ret = 0
    count=0
    self.each {|v|
      ret += (v << count)
      count += 8
    }
    ret
  end
  def to_hex
    self.map{|m|
      sprintf("%02x",m)
    }.join(" ")
  end
end

#p [1,2].to_num
