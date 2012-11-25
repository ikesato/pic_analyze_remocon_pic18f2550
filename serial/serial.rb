#!/bin/env ruby
# -*- coding: utf-8 -*-
require "rubygems"
require "serialport"

PORT=ARGV.shift
SPEED=ARGV.shift || 19200

sp = SerialPort.new(PORT, SPEED, 8, 1, 0) # 8bit, stopbit 1, parity none

# receive
Thread.new do
  begin
    loop do
      puts sp.gets
    end
  rescue =>ex
    puts "Device was detattched. [#{PORT}]"
    exit 1
  end
end

# send
loop do
  sp.write STDIN.getc.chr
end
sp.close
