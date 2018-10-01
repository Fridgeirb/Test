/********************************************************************
 * COPYRIGHT -- Bernecker + Rainer 
 ********************************************************************
 * Program: 125kHzTransp
 * File: 125kHzTransp.c
 * Author: Bernecker + Rainer
 * Created: September 15, 2009
 ********************************************************************
 * Description: Supports 5E9000.29 (125kHz) transponder reader
 ********************************************************************/


/********************************************************************
 * INCLUDES
 ********************************************************************/
#include <bur/plctypes.h>
#ifdef _DEFAULT_INCLUDES
 #include <AsDefault.h>
#endif
#include <string.h>
#include <DVFrame.h>
#include <AsUsb.h>


/********************************************************************
 * CONSTANTS
 ********************************************************************/
enum{
	USB_GETNODELIST = 1,
	USB_SEARCHDEVICE,
	USB_DEVICEOPEN,
	USB_READTAG,
	USB_READ_TOEND
};


/********************************************************************
* INIT UP
 ********************************************************************/
void _INIT kHzTranspINIT( void )
{
	usbAttachDetachCount = 0;
    usbNodeIx = 0;
	usbAction = USB_GETNODELIST;	
}


/********************************************************************
* CYCLIC TASK
 ********************************************************************/
_CYCLIC void kHzTranspCYCLIC(void)
{
	switch (usbAction)
	{
	case USB_GETNODELIST:
		UsbNodeListGetFub.enable = 1;
		UsbNodeListGetFub.pBuffer = (UDINT)&usbNodeList;
		UsbNodeListGetFub.bufferSize = sizeof(usbNodeList);
		UsbNodeListGetFub.filterInterfaceClass = 0;
		UsbNodeListGetFub.filterInterfaceSubClass = 0;
		UsbNodeListGet(&UsbNodeListGetFub);
		
		if (UsbNodeListGetFub.status == ERR_OK && UsbNodeListGetFub.listNodes)
		{
			/* USB Device Attach or detach */
			usbAction = USB_SEARCHDEVICE;
			usbNodeIx = 0;
			usbAttachDetachCount = UsbNodeListGetFub.attachDetachCount;
		}
		else if (UsbNodeListGetFub.status == asusbERR_BUFSIZE || UsbNodeListGetFub.status == asusbERR_NULLPOINTER)
			/* Error Handling */
	break;
		
	case USB_SEARCHDEVICE:
		sizeUsbNode = sizeof(usbDevice);
		UsbNodeGetFub.enable = 1;
		UsbNodeGetFub.nodeId = usbNodeList[usbNodeIx];
		UsbNodeGetFub.pBuffer = (UDINT)&usbDevice;
		UsbNodeGetFub.bufferSize = sizeof(usbDevice);
		UsbNodeGet(&UsbNodeGetFub);
		
		if (UsbNodeGetFub.status == ERR_OK )
		{
			/* USB Prolific Transponder ? */
			if (usbDevice.vendorId == TRANSPONDER_PROLIFIC_VENDOR_ID
				&& usbDevice.productId == TRANSPONDER_PROLIFIC_PRODUCT_ID
				&& usbDevice.bcdDevice == TRANSPONDER_PROLIFIC_BCD )
			{
				/* USB Prolific Transponder found */
				strcpy((char*)StringDevice, usbDevice.ifName);
				usbNodeId = usbNodeList[usbNodeIx];
				usbAction = USB_DEVICEOPEN;
			}
			else
			{
				usbNodeIx++;
				if (usbNodeIx >= UsbNodeListGetFub.allNodes)
					/* USB Device not found */
					usbAction = USB_GETNODELIST;
			}
		}
		else if (UsbNodeGetFub.status == asusbERR_USB_NOTFOUND)
			/* USB Device not found */
			usbAction = USB_GETNODELIST;
		else if (UsbNodeGetFub.status == asusbERR_BUFSIZE || UsbNodeGetFub.status == asusbERR_NULLPOINTER)
			/* Error Handling */
	break;
	
	case USB_DEVICEOPEN:
		/* initialize open structure */
		FrameXOpenFub.enable = 1;
		FrameXOpenFub.device = (UDINT) StringDevice;
		FrameXOpenFub.mode = (UDINT) 0;
		FrameXOpenFub.config = (UDINT) 0;
		FRM_xopen(&FrameXOpenFub); /* open an interface */
		
		if (FrameXOpenFub.status == ERR_OK)
		{
			usbAction = USB_READTAG;
			Ident = FrameXOpenFub.ident; /* get ident */
		}
	break;
		
	case USB_READTAG:
		/* initialize get buffer structure */
		FrameGetBufferFub.enable = 1;
		FrameGetBufferFub.ident = Ident;
		FRM_gbuf(&FrameGetBufferFub); /* get send buffer */
		
		pSendBuffer = (USINT*) FrameGetBufferFub.buffer; /* get adress of send buffer */
		SendBufferLength = FrameGetBufferFub.buflng; /* get length of send buffer */
		
		if (FrameGetBufferFub.status == 0) /* check status */
		{
			strcpy((char*)WriteData, "tag,0,0,#crc\r\n"); /* read tag */
			memcpy(pSendBuffer, WriteData, strlen((char*)WriteData)); /* copy write data into send buffer */
			/* initialize write structure */
			FrameWriteFub.ident = Ident;
			FrameWriteFub.buffer = (UDINT) pSendBuffer;
			FrameWriteFub.buflng = strlen((char*)WriteData);
			FrameWriteFub.enable = 1;
			FRM_write(&FrameWriteFub); /* write data to interface */
			
			if (FrameWriteFub.status == ERR_OK) /* check status */
			{
				usbAction = USB_READ_TOEND;
				/* initialize release output buffer structure */
				FrameReleaseOutputBufferFub.enable = 1;
				FrameReleaseOutputBufferFub.ident = Ident;
				FrameReleaseOutputBufferFub.buffer = (UDINT) pSendBuffer;
				FrameReleaseOutputBufferFub.buflng = strlen((char*)WriteData);
				FRM_robuf(&FrameReleaseOutputBufferFub); /* release output buffer */
			}
		}
	break;
	
	case USB_READ_TOEND:
		/* initialize read structure */
		FrameReadFub.enable = 1;
		FrameReadFub.ident = Ident;
		FRM_read(&FrameReadFub); /* read data form reader */
		
		pReadBuffer = (USINT*) FrameReadFub.buffer; /* get adress of read buffer */
		ReadBufferLength = FrameReadFub.buflng; /* get length of read buffer */
		
		if (FrameReadFub.status == ERR_OK) /* check status */
		{
			memcpy(ReadData, pReadBuffer, ReadBufferLength); /* copy read data into array */
			/* initialize release buffer structure */
			FrameReleaseBufferFub.enable = 1;
			FrameReleaseBufferFub.ident = Ident;
			FrameReleaseBufferFub.buffer = (UDINT) pReadBuffer;
			FrameReleaseBufferFub.buflng = ReadBufferLength;
			FRM_rbuf(&FrameReleaseBufferFub); /* release read buffer */
		}
		else
			/* Read to end */
			usbAction = USB_READTAG;
		break;
	}
}

