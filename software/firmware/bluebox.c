/*
 * Copyright (c) 2012 Jeppe Ledet-Pedersen <jlp@satlab.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include "config.h"
#include "bluebox.h"
#include "adf7021.h"
#include "bootloader.h"
#include "spi.h"

struct bluebox_config conf = {
	.freq = FREQUENCY,
	.csma_rssi = CSMA_RSSI,
	.speed = BAUD_RATE,
	.modindex = MOD_INDEX,
	.pa_setting = PA_SETTING,
	.afc_range = AFC_RANGE,
	.afc_ki = AFC_KI,
	.afc_kp = AFC_KP,
	.afc_enable = AFC_ENABLE,
	.if_bw = IF_FILTER_BW,
	.sw = SYNC_WORD,
	.swtol = SYNC_WORD_TOLERANCE,	
	.swlen = SYNC_WORD_LENGTH,
};

void setup_hardware(void)
{
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	clock_prescale_set(clock_div_1);

	USB_Init();
	DDRF |= _BV(0) | _BV(1) | _BV(4);
	PORTF &= ~( _BV(0) | _BV(1) | _BV(4));

	DDRD &= ~_BV(4);

	/* Initialize SPI */
	spi_init_config(SPI_SLAVE | SPI_MSB_FIRST);
}

static inline void do_register(int direction, unsigned int regnum)
{
	uint32_t value;
	adf_reg_t reg;

	if (direction == ENDPOINT_DIR_OUT) {
		Endpoint_Read_Control_Stream_LE(&value, sizeof(value));
		value = (value & ~0xf) | (regnum & 0xf);
		reg.whole_reg = value;
		adf_write_reg(&reg);
	} else if (direction == ENDPOINT_DIR_IN) {
		reg = adf_read_reg(regnum);
		value = reg.whole_reg;
		Endpoint_Write_Control_Stream_LE(&value, sizeof(value));
	}
}

static inline void do_rxtx_mode(int direction, unsigned int wValue)
{
	if (wValue != 0)
		adf_set_tx_mode();
	else
		adf_set_rx_mode();
}

#define rf_config_single(_type, _name) 						\
	_type _name; 								\
	if (direction == ENDPOINT_DIR_OUT) {					\
		Endpoint_Read_Control_Stream_LE(&_name, sizeof(_name));		\
		conf._name = _name;						\
		adf_configure();						\
	} else if (direction == ENDPOINT_DIR_IN) {				\
		Endpoint_Write_Control_Stream_LE(&conf._name, sizeof(conf._name)); \
	}

static inline void do_frequency(int direction, unsigned int vWalue)
{
	rf_config_single(uint32_t, freq);
}

static inline void do_modindex(int direction, unsigned int vWalue)
{
	rf_config_single(uint8_t, modindex);
}

static inline void do_csma_rssi(int direction, unsigned int vWalue)
{
	rf_config_single(int16_t, csma_rssi);
}

static inline void do_power(int direction, unsigned int vWalue)
{
	rf_config_single(uint8_t, pa_setting);
}
	
static inline void do_acf(int direction, unsigned int vWalue)
{
	rf_config_single(uint8_t, afc_enable);
}

static inline void do_ifbw(int direction, unsigned int vWalue)
{
	rf_config_single(uint8_t, if_bw);
}

static inline void do_training(int direction, unsigned int vWalue)
{
	/* FIXME: add training bytes config */
}

static inline void do_syncword(int direction, unsigned int vWalue)
{
	/* FIXME: add sync word config */
}

void EVENT_USB_Device_ControlRequest(void)
{
	switch (USB_ControlRequest.bmRequestType) {
	case (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE):

		Endpoint_ClearSETUP();

		switch (USB_ControlRequest.bRequest) {
		case REQUEST_REGISTER:
			do_register(ENDPOINT_DIR_OUT, USB_ControlRequest.wValue);
			break;
		case REQUEST_RXTX_MODE:
			do_rxtx_mode(ENDPOINT_DIR_OUT, USB_ControlRequest.wValue);
			break;
		case REQUEST_FREQUENCY:
			do_frequency(ENDPOINT_DIR_OUT, USB_ControlRequest.wValue);
			break;
		case REQUEST_MODINDEX:
			do_modindex(ENDPOINT_DIR_OUT, USB_ControlRequest.wValue);
			break;
		case REQUEST_CSMA_RSSI:
			do_csma_rssi(ENDPOINT_DIR_OUT, USB_ControlRequest.wValue);
			break;
		case REQUEST_POWER:
			do_power(ENDPOINT_DIR_OUT, USB_ControlRequest.wValue);
			break;
		case REQUEST_AFC:
			do_acf(ENDPOINT_DIR_OUT, USB_ControlRequest.wValue);
			break;
		case REQUEST_IFBW:
			do_ifbw(ENDPOINT_DIR_OUT, USB_ControlRequest.wValue);
			break;
		case REQUEST_TRAINING:	
			do_training(ENDPOINT_DIR_OUT, USB_ControlRequest.wValue);
			break;
		case REQUEST_SYNCWORD:	
			do_syncword(ENDPOINT_DIR_OUT, USB_ControlRequest.wValue);
			break;
		case REQUEST_BOOTLOADER:
			jump_to_bootloader();
			break;
		}

		Endpoint_ClearIN();

		break;
	case (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE):

		Endpoint_ClearSETUP();

		switch (USB_ControlRequest.bRequest) {
		case REQUEST_REGISTER:
			do_register(ENDPOINT_DIR_IN, USB_ControlRequest.wValue);
			break;
		}

		Endpoint_ClearOUT();

		break;
	default:
		break;
	}
}

void bluebox_task(void)
{
	static uint8_t text[IN_EPSIZE] = "testing";

	if (USB_DeviceState != DEVICE_STATE_Configured)
		return;

	Endpoint_SelectEndpoint(OUT_EPADDR);
	if (Endpoint_IsOUTReceived()) {
		if (Endpoint_IsReadWriteAllowed()) {
			memset(text, 0, sizeof(text));
			Endpoint_Read_Stream_LE(&text, sizeof(text), NULL);
		}
		Endpoint_ClearOUT();

		/* Clear pending data in IN endpoint */
		Endpoint_SelectEndpoint(IN_EPADDR);
		Endpoint_AbortPendingIN();
	}

	/*if (Endpoint_IsINReady()) {
		Endpoint_Write_Stream_LE(&text, sizeof(text), NULL);
		Endpoint_ClearIN();
	}*/
}

int main(void)
{
	setup_hardware();
	GlobalInterruptEnable();

	adf_set_power_on(XTAL_FREQ);
	adf_configure();
	adf_set_rx_mode();

	swd_init();
	swd_enable();

	for (;;) {
		bluebox_task();
		USB_USBTask();
	}
}