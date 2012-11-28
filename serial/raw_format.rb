# -*- coding: utf-8 -*-

# 生データを表示するだけ
# 単位は ms
class RawFormat
  def initialize(raw)
    @raw = raw
  end
  def self.parse(raw)
    s=RawFormat.new(raw)
  end

  def dump
    "raw [#{@raw.map{|v| sprintf("%.3f",v)}.join(",")}]"
  end
end
