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

#include "Descriptors.h"
#include "bluebox.h"

const USB_Descriptor_Device_t PROGMEM BlueBox_DeviceDescriptor = {
	.Header                 = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

	.USBSpecification       = VERSION_BCD(01.10),
	.Class                  = USB_CSCP_VendorSpecificClass,
	.SubClass               = USB_CSCP_NoDeviceSubclass,
	.Protocol               = USB_CSCP_NoDeviceProtocol,

	.Endpoint0Size          = FIXED_CONTROL_ENDPOINT_SIZE,

	/* Note: these have not been allocated from OpenMoko yet! */
	.VendorID               = 0x1d50,
	.ProductID              = 0x6666,

	.ReleaseNumber          = VERSION_BCD(01.00),

	.ManufacturerStrIndex   = 0x01,
	.ProductStrIndex        = 0x02,
	.SerialNumStrIndex      = 0x03,

	.NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
};

const USB_Descriptor_Configuration_t PROGMEM BlueBox_ConfigurationDescriptor =
{
	.Config = {
		.Header                 = {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},

		.TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
		.TotalInterfaces        = 1,
		.ConfigurationNumber    = 1,
		.ConfigurationStrIndex  = NO_DESCRIPTOR,
		.ConfigAttributes       = USB_CONFIG_ATTR_RESERVED,
		.MaxPowerConsumption    = USB_CONFIG_POWER_MA(500)
	},

	.Interface = {
		.Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

		.InterfaceNumber        = 0,
		.AlternateSetting       = 0,
		.TotalEndpoints         = 2,
		.Class                  = USB_CSCP_VendorSpecificClass,
		.SubClass               = 0x00,
		.Protocol               = 0x00,
		.InterfaceStrIndex      = NO_DESCRIPTOR
	},

	.DataInEndpoint = {
		.Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

		.EndpointAddress        = IN_EPADDR,
		.Attributes             = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.EndpointSize           = IN_EPSIZE,
		.PollingIntervalMS      = 0x05,
	},

	.DataOutEndpoint = {
		.Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

		.EndpointAddress        = OUT_EPADDR,
		.Attributes             = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.EndpointSize           = OUT_EPSIZE,
		.PollingIntervalMS      = 0x05,
	},
};

const USB_Descriptor_String_t PROGMEM BlueBox_LanguageString = {
	.Header                 = {.Size = USB_STRING_LEN(1), .Type = DTYPE_String},
	.UnicodeString          = {LANGUAGE_ID_ENG}
};

const USB_Descriptor_String_t PROGMEM BlueBox_ManufacturerString = {
	.Header                 = {.Size = USB_STRING_LEN(7), .Type = DTYPE_String},
	.UnicodeString          = L"AAUSAT3"
};

const USB_Descriptor_String_t PROGMEM BlueBox_ProductString = {
	.Header                 = {.Size = USB_STRING_LEN(7), .Type = DTYPE_String},
	.UnicodeString          = L"BlueBox"
};

const USB_Descriptor_String_t PROGMEM BlueBox_SerialString = {
	.Header                 = {.Size = USB_STRING_LEN(5), .Type = DTYPE_String},
	.UnicodeString          = L"00001"
};

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
				    const uint8_t wIndex,
				    const void** const DescriptorAddress)
{
	const uint8_t DescriptorType   = (wValue >> 8);
	const uint8_t DescriptorNumber = (wValue & 0xFF);

	const void *Address = NULL;
	uint16_t Size = NO_DESCRIPTOR;

	switch (DescriptorType) {
	case DTYPE_Device:
		Address = &BlueBox_DeviceDescriptor;
		Size    = sizeof(USB_Descriptor_Device_t);
		break;
	case DTYPE_Configuration:
		Address = &BlueBox_ConfigurationDescriptor;
		Size    = sizeof(USB_Descriptor_Configuration_t);
		break;
	case DTYPE_String:
		switch (DescriptorNumber)
		{
			case 0x00:
				Address = &BlueBox_LanguageString;
				Size    = pgm_read_byte(&BlueBox_LanguageString.Header.Size);
				break;
			case 0x01:
				Address = &BlueBox_ManufacturerString;
				Size    = pgm_read_byte(&BlueBox_ManufacturerString.Header.Size);
				break;
			case 0x02:
				Address = &BlueBox_ProductString;
				Size    = pgm_read_byte(&BlueBox_ProductString.Header.Size);
				break;
			case 0x03:
				Address = &BlueBox_SerialString;
				Size    = pgm_read_byte(&BlueBox_SerialString.Header.Size);
				break;
		}

		break;
	}

	*DescriptorAddress = Address;
	return Size;
}

void EVENT_USB_Device_ConfigurationChanged(void)
{
	Endpoint_ConfigureEndpoint(IN_EPADDR,  EP_TYPE_INTERRUPT, IN_EPSIZE,  1);
	Endpoint_ConfigureEndpoint(OUT_EPADDR, EP_TYPE_INTERRUPT, OUT_EPSIZE, 1);
}