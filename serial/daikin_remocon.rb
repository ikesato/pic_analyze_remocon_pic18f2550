# -*- coding: utf-8 -*-

require "int_ary"

class DaikinRemocon
  attr_accessor :tcycle # data[:t] を格納
  attr_accessor :bodies # 配列, data[:frames] をそのまま格納
  attr_accessor :body0_only

  attr_accessor :power # true:on false:off
  attr_accessor :inner_clean_mode # true:on false:off
  attr_accessor :timer_auto_pon_mode # true:on false:off
  attr_accessor :timer_auto_poff_mode # true:on false:off
  attr_accessor :air_mode # 0:標準(自動) 1:? 2:標準(ドライ) 3:冷房 4:暖房 5:? 6:送風 7:?

  def initialize(data)
    from_raw_data(data)
  end

  def from_raw_data(data)
    self.tcycle = data[:t]
    self.bodies = data[:frames].dup
    self.body0_only = data[:frames].count <= 1

    parse_body0(bodies[0])
    parse_body1(bodies[1])
    parse_body2(bodies[2])
  end

  def report
    ret = <<EOF
t                    : #{tcycle}
body0_only           : #{body0_only}
inner_clean_mode     : #{inner_clean_mode}
timer_auto_pon_mode  : #{timer_auto_pon_mode}
timer_auto_poff_mode : #{timer_auto_poff_mode}
air_mode             : #{report_air_mode(air_mode)}
EOF
    ret
  end

  private
  def parse_body0(body0)
    if body0 == [0x11, 0xda, 0x27, 0x00, 0xc5, 0x00, 0x20, 0xf7]
      self.inner_clean_mode = false
    elsif body0 == [0x11, 0xda, 0x27, 0x00, 0xc5, 0x00, 0x28, 0xff]
      self.inner_clean_mode = true
    else
      raise "invalid body0 format [#{body0.to_hex}]"
    end
  end

  def parse_body1(body1)
    if body1 != [0x11, 0xda, 0x27, 0x00, 0x42, 0x00, 0x00, 0x54]
      raise "invalid body1 format [#{body1.to_hex}]"
    end
  end

  def parse_body2(body2)
    # byte0-4
    if body2[0..4] != [0x11, 0xda, 0x27, 0x00, 0x00]
      raise "ivalid format byte0-4 [#{body2[0..4].to_hex}]"
    end

    # byte5
    self.power = (body2[5] & (1 << 0) != 0)
    if body2[5] & (1 << 1) != 0
      raise "invalid format byte5.1 [#{body2[5].to_hex}]"
    end
    self.timer_auto_poff_mode = (body2[5] & (1 << 2) != 0)
    self.timer_auto_pon_mode = (body2[5] & (1 << 3) != 0)
    self.air_mode = ((body2[5] >> 4) & 0x7)
    unless [0,2,3,4,6].include?(self.air_mode)
      raise "invalid format byte5.4-6 #{body2[5].to_hex}"
    end
    if body2[5] & (1 << 7) != 0
      raise "invalid format byte5.7 [#{body2[5].to_hex}]"
    end
 
    # TODO
    # timer_auto_p{on,off}_mode が true の場合は後のデータが正しいこと
  end

  def report_air_mode(a)
    v = "#{a}:"
    case a
    when 0
      v+"auto"
    when 2
      v+"dry"
    when 3
      v+"air-condition"
    when 4
      v+"heating"
    when 6
      v+"ventilation"
    else
      raise "invalid air_mode #{a.to_hex}"
    end
  end
end
