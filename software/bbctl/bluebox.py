# Copyright (c) 2012 Jeppe Ledet-Pedersen <jlp@satlab.org>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

import time
import struct
import usb

class Bluebox(object):
	# USB VID/PID
	VENDOR  		= 0x1d50
	PRODUCT 		= 0x6666

	# Data Endpoints
	DATA_IN	     = (usb.util.ENDPOINT_IN  | 1)
	DATA_OUT     = (usb.util.ENDPOINT_OUT | 2)

	# RF Control
	REQUEST_REGISTER	= 0x01
	REQUEST_FREQUENCY 	= 0x02
	REQUEST_MODINDEX	= 0x03
	REQUEST_CSMA_RSSI	= 0x04
	REQUEST_POWER		= 0x05
	REQUEST_AFC		= 0x06
	REQUEST_IFBW		= 0x07
	REQUEST_TRAINING	= 0x08
	REQUEST_SYNCWORD	= 0x09
	REQUEST_RXTX_MODE	= 0x0A

	# Data Control
	REQUEST_DATA		= 0x10

	# Bootloader Control
	REQUEST_BOOTLOADER	= 0xFF

	# Registers
	REGISTER_N		= 0
	REGISTER_VCO_OSC	= 1
	REGISTER_TXMOD		= 2
	REGISTER_TXRXCLK	= 3
	REGISTER_DEMOD		= 4
	REGISTER_IFFILTER	= 5
	REGISTER_IFFINECAL	= 6
	REGISTER_READBACK	= 7
	REGISTER_PWRDWN		= 8
	REGISTER_AGC		= 9
	REGISTER_AFC		= 10
	REGISTER_SWD		= 11
	REGISTER_SWDTHRESHOLD	= 12
	REGISTER_3FSK4FSK	= 13
	REGISTER_TESTDAC	= 14
	REGISTER_TESTMODE	= 15

	# Readback select
	READBACK_RSSI		= 0x0014
	READBACK_VERSION	= 0x001c
	READBACK_AFC		= 0x0016

	# Testmode values
	TESTMODE_PATTERN_CARR	= 1
	TESTMODE_PATTERN_HIGH	= 2
	TESTMODE_PATTERN_LOW	= 3
	TESTMODE_PATTERN_1010	= 4
	TESTMODE_PATTERN_PN9	= 5
	TESTMODE_PATTERN_SWD	= 6

	# Sync word
	SW_LENGTH_12BIT		= 0
	SW_LENGTH_16BIT		= 1
	SW_LENGTH_20BIT		= 2
	SW_LENGTH_24BIT		= 3
	SW_TOLERANCE_0BER	= 0
	SW_TOLERANCE_1BER	= 1
	SW_TOLERANCE_2BER	= 2
	SW_TOLERANCE_3BER	= 3
	
	def __init__(self):
		self.dev = None
		while self.dev is None:
			self.dev = usb.core.find(idVendor=self.VENDOR, idProduct=self.PRODUCT)
			time.sleep(0.1)

		if self.dev.is_kernel_driver_active(0) is True:
			self.dev.detach_kernel_driver(0)

		self.dev.set_configuration()

		self.manufacturer = usb.util.get_string(self.dev, 100, self.dev.iManufacturer)
		self.product      = usb.util.get_string(self.dev, 100, self.dev.iProduct)
		self.serial       = usb.util.get_string(self.dev, 100, self.dev.iSerialNumber)

	def _ctrl_write(self, request, data, wValue=0, wIndex=0, timeout=1000):
		bmRequestType = usb.util.build_request_type(
					usb.util.CTRL_OUT,
					usb.util.CTRL_TYPE_CLASS,
					usb.util.CTRL_RECIPIENT_INTERFACE)
		self.dev.ctrl_transfer(bmRequestType, request, wValue, wIndex, data, timeout)

	def _ctrl_read(self, request, length, wValue=0, wIndex=0, timeout=1000):
		bmRequestType = usb.util.build_request_type(
					usb.util.CTRL_IN,
					usb.util.CTRL_TYPE_CLASS,
					usb.util.CTRL_RECIPIENT_INTERFACE)
		return self.dev.ctrl_transfer(bmRequestType, request, wValue, wIndex, length, timeout)

	def reg_read(self, reg):
		return struct.unpack("<I", self._ctrl_read(self.REQUEST_REGISTER, 4, wValue=reg))[0] & 0xffff

	def reg_write(self, reg, value):
		value = struct.pack("<I", value | reg)
		self._ctrl_write(self.REQUEST_REGISTER, value, wValue=reg)

	def frequency(self, freq):
		freq = struct.pack("<I", freq)
		self._ctrl_write(self.REQUEST_FREQUENCY, freq)

	def modindex(self, mi):
		pass

	def power(self, dbm):
		pass

	def ifbw(self, ifbw):
		pass

	def syncword(self, word, tol):
		pass

	def training(self, startms, interms):
		pass

	def version(self):
		return self.reg_read(self.READBACK_VERSION)

	def rssi(self):
		gain_correction = (86, 0, 0, 0, 58, 38, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		rb = self.reg_read(self.READBACK_RSSI)
		rssi = rb & 0x7f
		gc = (rb & 0x780) >> 7
		dbm = ((rssi + gain_correction[gc]) * 0.5) - 130;
		return round(dbm)

	def testmode(self, mode):
		self.reg_write(self.REGISTER_TESTMODE, mode << 8)

	def tx_mode(self):
		self._ctrl_write(self.REQUEST_RXTX_MODE, None, wValue=1)

	def rx_mode(self):
		self._ctrl_write(self.REQUEST_RXTX_MODE, None, wValue=0)

	def bootloader(self):
		try:
			self._ctrl_write(self.REQUEST_BOOTLOADER, None, 0)
		except:
			pass

	def data_write(self, text):
		self.dev.write(self.DATA_OUT, text, 0)

	def data_read(self):
		return self.dev.read(self.DATA_IN, 512, 0, timeout=-1)