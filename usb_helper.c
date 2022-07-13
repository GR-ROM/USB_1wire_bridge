/*
static const uint8_t devDescriptor[] = {
	0x12,   // bLength
	0x01,   // bDescriptorType
	0x00,   // bcdUSBL
	0x02,   //
	0x02,   // bDeviceClass:    CDC class code
	0x00,   // bDeviceSubclass: CDC class sub code
	0x00,   // bDeviceProtocol: CDC Device protocol
	0x40,   // bMaxPacketSize0
	0xEB,   // idVendorL
	0x03,   //
	0x27,   // idProductL
	0x61,   //
	0x10,   // bcdDeviceL
	0x01,   //
	0x01,   // iManufacturer
	0x02,   // iProduct
	0x00,   // SerialNumber
	0x01    // bNumConfigs
};

static const uint8_t cfgDescriptor[] = {
	/* ============== CONFIGURATION 1 =========== */
	/* Configuration 1 descriptor */
/*	0x09,   // CbLength
	0x02,   // CbDescriptorType
	0x43,   // CwTotalLength 2 EP + Control
	0x00,
	0x02,   // CbNumInterfaces
	0x01,   // CbConfigurationValue
	0x00,   // CiConfiguration
	0x80,   // CbmAttributes 0xA0
	50,   // CMaxPower

	/* Communication Class Interface Descriptor Requirement */
/*	0x09, // bLength
	0x04, // bDescriptorType
	0x00, // bInterfaceNumber
	0x00, // bAlternateSetting
	0x01, // bNumEndpoints
	0x02, // bInterfaceClass
	0x02, // bInterfaceSubclass
	0x00, // bInterfaceProtocol
	0x00, // iInterface

	/* Header Functional Descriptor */
/*	0x05, // bFunction Length
	0x24, // bDescriptor type: CS_INTERFACE
	0x00, // bDescriptor subtype: Header Func Desc
	0x10, // bcdCDC:1.1
	0x01,

	/* ACM Functional Descriptor */
/*	0x04, // bFunctionLength
	0x24, // bDescriptor Type: CS_INTERFACE
	0x02, // bDescriptor Subtype: ACM Func Desc
	0x00, // bmCapabilities

	/* Union Functional Descriptor */
/*	0x05, // bFunctionLength
	0x24, // bDescriptorType: CS_INTERFACE
	0x06, // bDescriptor Subtype: Union Func Desc
	0x00, // bMasterInterface: Communication Class Interface
	0x01, // bSlaveInterface0: Data Class Interface

	/* Call Management Functional Descriptor */
/*	0x05, // bFunctionLength
	0x24, // bDescriptor Type: CS_INTERFACE
	0x01, // bDescriptor Subtype: Call Management Func Desc
	0x00, // bmCapabilities: D1 + D0
	0x01, // bDataInterface: Data Class Interface 1

	/* Endpoint 1 descriptor */
/*	0x07,   // bLength
	0x05,   // bDescriptorType
	0x81,   // bEndpointAddress, Endpoint 01 - IN
	0x03,   // bmAttributes      INT
	0x08,   // wMaxPacketSize
	0x00,
	0xFF,   // bInterval

	/* Data Class Interface Descriptor Requirement */
/*	0x09, // bLength
	0x04, // bDescriptorType
	0x01, // bInterfaceNumber
	0x00, // bAlternateSetting
	0x02, // bNumEndpoints
	0x0A, // bInterfaceClass
	0x00, // bInterfaceSubclass
	0x00, // bInterfaceProtocol
	0x00, // iInterface

	/* First alternate setting */
	/* Endpoint 1 descriptor */
/*	0x07,   // bLength
	0x05,   // bDescriptorType
	0x01,   // bEndpointAddress, Endpoint 01 - OUT
	0x02,   // bmAttributes      BULK
	EP1_BUFF_SIZE,   // wMaxPacketSize
	0x00,
	0x01,   // bInterval

	/* Endpoint 2 descriptor */
/*	0x07,   // bLength
	0x05,   // bDescriptorType
	0x81,   // bEndpointAddress, Endpoint 02 - IN
	0x02,   // bmAttributes      BULK
	EP1_BUFF_SIZE,   // wMaxPacketSize
	0x00,
	0x01    // bInterval
};*/