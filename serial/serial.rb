#!/bin/env ruby
# -*- coding: utf-8 -*-

$LOAD_PATH.push "."

require "rubygems"
require "serialport"
require "remocon_analyzer"

PORT=ARGV.shift
SPEED=ARGV.shift || 19200

sp = SerialPort.new(PORT, SPEED, 8, 1, 0) # 8bit, stopbit 1, parity none

# receive
Thread.new do
  begin
    loop do
      line = sp.gets
      line = line.scan(/[[:print:]]/).join
      #puts line
      r=RemoconAnalyzer.parse(line)
      puts r.dump
    end
  rescue =>ex
    puts ex
    puts "Device was detattched. [#{PORT}]"
    exit 1
  end
end

# send
loop do
  sp.write STDIN.getc.chr
end
sp.close
