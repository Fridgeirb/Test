/********************************************************************
 * COPYRIGHT -- Bernecker + Rainer 
 ********************************************************************
 * Program: 13MHzTransp
 * File: 13MHzTransp.c
 * Author: Bernecker + Rainer
 * Created: November 3, 2010
 ********************************************************************
 * Description: Supports 5E9010.29 (13,56MHz) transponder reader
 ********************************************************************/


#pragma region "Documentation / comments"
/*
 *	NOTE: Communication based on IEC15693
 *
 *
 *	Initialization sequence:
 *	------------------------
 *	o- USB_GETNODELIST
 *	|
 *	o- USB_SEARCHDEVICE
 *	|
 *	o- DVF_DEVICEOPEN
 *	|
 *	o- RFID_INIT_READER
 *	|
 *	o- RFID_WRITE
 *	|
 *	o- RFID_READ
 *
 *
 *	Command sequence:
 *	------------------------
 *	o- RFID_READ_INVENTORY | RFID_READ_SYSINFO | RFID_READ_SBLOCK | RFID_WRITE_SBLOCK |
 *	   RFID_WRITE_AFI | RFID_WRITE_DSFID
 *	|
 *	o- RFID_WRITE
 *	|
 *	o- RFID_READ
 *
 */
#pragma end region


/********************************************************************
 * INCLUDES
 ********************************************************************/
#include <bur/plctypes.h>
#ifdef _DEFAULT_INCLUDES
 #include <AsDefault.h>
#endif
#include <AsUSB.h>
#include <DVFrame.h>
#include <string.h>


/********************************************************************
 * CONSTANTS
 ********************************************************************/
enum{
	IDLE = 0,
	
	USB_GETNODELIST = 		1,
	USB_SEARCHDEVICE,
	DVF_DEVICEOPEN,
	
	RFID_INIT_READER = 		5,
	RFID_WRITE,
	RFID_READ,
	
	RFID_READ_INVENTORY =	10,
	RFID_READ_SYSINFO = 	20,
	RFID_READ_SBLOCK = 		30,
	
	RFID_WRITE_SBLOCK = 	50,
	RFID_WRITE_AFI = 		60,
	RFID_WRITE_DSFID = 		70,
};


/********************************************************************
 * INIT UP
 ********************************************************************/
void _INIT TranspINIT( void )
{
	usbAttachDetachCount = 0;
    usbNodeIx = 0;
	step = USB_GETNODELIST;	
}


/********************************************************************
 * CYCLIC TASK
 ********************************************************************/
void _CYCLIC TranspCYCLIC( void )
{
	switch (step)
	{
		
		case IDLE:
			/* Idle */
			strcpy((char*)&actionText, (char*)&"IDLE");
		break;
		
		
		case USB_GETNODELIST:
			strcpy((char*)&actionText, (char*)&"USB_GETNODELIST");
			UsbNodeListGetFub.enable = 1;
			UsbNodeListGetFub.pBuffer = (UDINT)&usbNodeList;
			UsbNodeListGetFub.bufferSize = sizeof(usbNodeList);
			UsbNodeListGetFub.filterInterfaceClass = 0;
			UsbNodeListGetFub.filterInterfaceSubClass = 0;
			UsbNodeListGet(&UsbNodeListGetFub);
			
			if (UsbNodeListGetFub.status == ERR_OK && UsbNodeListGetFub.listNodes)
			{
				/* USB Device Attach or detach */
				step = USB_SEARCHDEVICE;
				usbNodeIx = 0;
				usbAttachDetachCount = UsbNodeListGetFub.attachDetachCount;
			}
			/*else if (UsbNodeListGetFub.status == asusbERR_BUFSIZE || UsbNodeListGetFub.status == asusbERR_NULLPOINTER)
				/* Error Handling */
		break;
	
	
		case USB_SEARCHDEVICE:
			strcpy((char*)&actionText, (char*)&"USB_SEARCHDEVICE");
			sizeUsbNode = sizeof(usbDevice);
			UsbNodeGetFub.enable = 1;
			UsbNodeGetFub.nodeId = usbNodeList[usbNodeIx];
			UsbNodeGetFub.pBuffer = (UDINT)&usbDevice;
			UsbNodeGetFub.bufferSize = sizeof(usbDevice);
			UsbNodeGet(&UsbNodeGetFub);
			
			if (UsbNodeGetFub.status == ERR_OK )
			{
				/* USB FTDI Transponder ? */
				if (usbDevice.vendorId == TRANSPONDER_FTDI_VENDOR_ID
					&& usbDevice.productId == TRANSPONDER_FTDI_PRODUCT_ID
					&& usbDevice.bcdDevice == TRANSPONDER_FTDI_BCD )
				{
					/* USB FTDI Transponder found */
					strcpy((char*)StringDevice, usbDevice.ifName);
					usbNodeId = usbNodeList[usbNodeIx];
					step = DVF_DEVICEOPEN;
				}
				else
				{
					usbNodeIx++;
					if (usbNodeIx >= UsbNodeListGetFub.allNodes)
						/* USB Device not found */
						step = USB_GETNODELIST;
				}
			}
			else if (UsbNodeGetFub.status == asusbERR_USB_NOTFOUND)
				/* USB Device not found */
				step = USB_GETNODELIST;

			/*else if (UsbNodeGetFub.status == asusbERR_BUFSIZE || UsbNodeGetFub.status == asusbERR_NULLPOINTER)
				/* Error Handling */
		break;
	
	
		case DVF_DEVICEOPEN:
			strcpy((char*)&actionText, (char*)&"DVF_DEVICEOPEN");
			/* initialize open structure */
			FrameXOpenFub.device = (UDINT) StringDevice;
			FrameXOpenFub.mode = (UDINT) 0;
			FrameXOpenFub.config = (UDINT) 0;
			FrameXOpenFub.enable = 1;
			FRM_xopen(&FrameXOpenFub); /* open an interface */
			
			if (FrameXOpenFub.status == frmERR_OK)
				step = RFID_INIT_READER;
		break;
	
	
		case RFID_INIT_READER:
		/*
		 *   Register Write:
         *   5V operation, RF output active, full power; ISO 15693, low bit rate, 6.62 kbps, one subcarrier, 1 out of 4
		 *
         *   10  00  21  01  00
         *   |   |   |   |   |
         *   |   |   |   |   Value Register 0x01
         *   |   |   |   ISO Control Register (0x01)
         *   |   |   Value Register 0x00
         *   |   Chip Status Control Register (0x00)
         *   Register Write Cmd
		 */
			strcpy((char*)&actionText, (char*)&"RFID_INIT_READER");
			strcpy((char*)WriteData, "010C00030410002101000000"); /* Set Protocol, 1 out of 4, full power, low bit rate */
			ReadAnswer = 1; /* 1 answer */
			step = RFID_WRITE;
		break;
	
	
		case RFID_READ_INVENTORY:
		/*
         *   Inventory
         *   14  04  01  00
         *   |   |   |   |
         *   |   |   |   Mask Length
         *   |   |   Anticollision Cmd
         *   |   Flags (Inventory flag = 1, HDR = 0)
         *   Inventory request
		 */
			strcpy((char*)&actionText, (char*)&"RFID_READ_INVENTORY");
			strcpy((char*)WriteData, "010B000304140401000000"); /* inventory 16 slot */
			ReadAnswer = 16; /* 16 slot answer */
			step = RFID_WRITE;
		break;
	
	
		case RFID_READ_SYSINFO:
		/*
         *   Read System Info
         *   18  00  2B
         *   |   |   |
         *   |   |   Get System Info Cmd
         *   |   Flags (Option = 0, HDR = 0)
         *   Request Cmd
		 */
			strcpy((char*)&actionText, (char*)&"RFID_READ_SYSINFO");
			strcpy((char*)WriteData, "010A00030418002B0000"); /* get system info cmd */
			ReadAnswer = 1; /* 1 answer */
			step = RFID_WRITE;
		break;
		
		
		case RFID_READ_SBLOCK:
		/*
         *   Read Single Block
         *   18  40  20
         *   |   |   |
         *   |   |   Read Single Block Cmd
         *   |   Flags (Option = 1, HDR = 0)
         *   Request Cmd
         *
         *   Option = 1: Return block security status 
		 */
			strcpy((char*)&actionText, (char*)&"RFID_READ_SBLOCK");
			strcpy((char*)WriteData, "010B000304184020000000"); /* read block #1 */
			ReadAnswer = 1; /* 1 answer */
			step = RFID_WRITE;
		break;
		
		
		case RFID_WRITE_SBLOCK:
		/*
         *   Write Single Block
         *   18  00  21
         *   |   |   |
         *   |   |   Write Single Block Cmd
         *   |   Flags (Option = 0, HDR = 0)
         *   Request Cmd
		 */
			strcpy((char*)&actionText, (char*)&"RFID_WRITE_SBLOCK");
			strcpy((char*)WriteData, "010F00030418002100123456780000"); /* write block #1 (Data = 0x12345678) */
			ReadAnswer = 1; /* 1 answer */
			step = RFID_WRITE;
		break;


		case RFID_WRITE_AFI:
		/*
         *   Write AFI
         *   18  00  30
         *   |   |   |
         *   |   |   Get System Info Cmd
         *   |   Flags (Option = 0, HDR = 0)
         *   Request Cmd
		 */
			strcpy((char*)&actionText, (char*)&"RFID_WRITE_AFI");
			strcpy((char*)WriteData, "010B000304180027300000"); /* write AFI (0x30 = Identification) */
			ReadAnswer = 1; /* 1 answer */
			step = RFID_WRITE;
		break;


		case RFID_WRITE_DSFID:
		/*
         *   Write DSFID
         *   18  00  29
         *   |   |   |
         *   |   |   Write DSFID Cmd
         *   |   Flags (Option = 0, HDR = 0)
         *   Request Cmd
		 */
			strcpy((char*)&actionText, (char*)&"RFID_WRITE_DSFID");
			strcpy((char*)WriteData, "010B000304180029180000"); /* write DSFID 0x18 */
			ReadAnswer = 1; /* 1 answer */
			step = RFID_WRITE;
		break;


		case RFID_WRITE:
			strcpy((char*)&actionText, (char*)&"RFID_WRITE");

			/* initialize write structure */
			FrameWriteFub.ident = FrameXOpenFub.ident;
			FrameWriteFub.buffer = (UDINT)&WriteData;
			FrameWriteFub.buflng = strlen((char*)&WriteData);
			FrameWriteFub.enable = 1;
			FRM_write(&FrameWriteFub); /* write data to interface */
			
			if (FrameWriteFub.status == frmERR_OK) /* check status */
			{
				step = RFID_READ; 
				ReadCount = 0;
			}
		break;


		case RFID_READ:
			strcpy((char*)&actionText, (char*)&"RFID_READ");
			/* initialize read structure */
			FrameReadFub.enable = 1;
			FrameReadFub.ident = FrameXOpenFub.ident;
			FRM_read(&FrameReadFub); /* read data form reader */
			
			pReadBuffer = (UDINT*) FrameReadFub.buffer; /* get adress of read buffer */
			ReadBufferLength = FrameReadFub.buflng; /* get length of read buffer */
			
			if (FrameReadFub.status == frmERR_OK) /* check status */
			{
				memset(ReadData, 0, sizeof(ReadData));
				memcpy(ReadData, (char*)pReadBuffer, ReadBufferLength); 	/* copy read data into array */
				if (ReadBufferLength > 7)
					strcpy(ReadTag, ReadData);	/* copy read data into Tag array */
				
				/* initialize release buffer structure */
				FrameReleaseBufferFub.enable = 1;
				FrameReleaseBufferFub.ident = FrameXOpenFub.ident;
				FrameReleaseBufferFub.buffer = (UDINT) pReadBuffer;
				FrameReleaseBufferFub.buflng = ReadBufferLength;
				FRM_rbuf(&FrameReleaseBufferFub); /* release read buffer */
				
				ReadCount++;
				/* read cycle finished */
				if (ReadCount == ReadAnswer)
					step = IDLE;
			}
		break;

	} /* switch (step) */
	
} /* TranspCYCLIC */
