class NilAnalyzer
  def initialize(raw)
    @raw = raw
  end
  def self.parse(raw)
    s=NilAnalyzer.new(raw)
  end

  def dump
    "unrecognize [#{@raw.map{|v| sprintf("%.3f",v)}.join(",")}]"
  end
end
