# -*- coding: utf-8 -*-

require "sony_analyzer"
require "nil_analyzer"

class RemoconAnalyzer
  def self.parse(raw_str)
    ra = RemoconAnalyzer.new
    ra._parse(raw_str)
    ra
  end

  def _parse(raw_str)
    ary=raw_str.split(",")
    ary.map! {|a|
      if a[0] != 'H' && a[0] != 'L'
        raise "parse error. need 'H' or 'L'"
      end
      a[1..-1]
    }
    ary.compact!
    ary.map! {|a| a.to_i * 5.33333 * 0.001} # ms になおす
    @raw = ary

    @analyzer = nil
    @analyzer ||= SonyAnalyzer.parse(@raw)
   #@analyzer ||= KadenkyoAnalyzer.parse(@raw)
    @analyzer ||= NilAnalyzer.parse(@raw)
    @analyzer
  end

  def dump
    @analyzer.dump
  end
end
