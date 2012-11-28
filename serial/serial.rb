#!/bin/env ruby
# -*- coding: utf-8 -*-

$LOAD_PATH.push "."

require "rubygems"
require "serialport"
require "remocon_analyzer"
require 'optparse'

debug=false
OptionParser.new {|opt|
  opt.on("--debug") {
    debug=true
  }
  opt.parse!(ARGV)
}

PORT=ARGV.shift
SPEED=ARGV.shift || 19200

sp = SerialPort.new(PORT, SPEED, 8, 1, 0) # 8bit, stopbit 1, parity none

# receive
Thread.new do
  begin
    loop do
      line = sp.gets
      line = line.scan(/[[:print:]]/).join
      if line.index("received,")==0
        r=RemoconAnalyzer.parse(line)
        puts r.dump
        puts line if debug
      elsif line.index("echo,")==0
        puts line if debug
      else
        puts line
      end
    end
  rescue =>ex
    puts ex
    puts "Device was detattched. [#{PORT}]"
    exit 1
  end
end

# send
loop do
  #sp.write STDIN.getc.chr
  str = STDIN.gets
  data = RemoconAnalyzer.send_data(str)
  next if data.nil?
  (data+"\r\n").each_char {|s| sp.write s} # 1文字づつ転送する（バッファがあふれるので）
end
sp.close
