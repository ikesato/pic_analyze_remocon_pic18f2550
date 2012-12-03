class Fixnum
  def to_a
    v=self
    ret = []
    begin
      ret << (v&0xff)
      v >>= 8
    end while v!=0
    ret
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
end

#p [1,2].to_num
