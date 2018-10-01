#include <brdkRFIDUSBReader_func.h>


#ifdef __cplusplus
	extern "C"
	{
#endif
	#include "brdkRFID.h"
#ifdef __cplusplus
	};
#endif

/* 
	SENSOR 5E9010.29 telegram structure
	SOF (0x01) Number of bytes 0x00 0x0304 Command + parameters EOF (0x0000)
*/


void generate9010Write(char *pStr) {
	pStr[0] = 0x01;

}



/* Communicates with various B&R tag readers */
void brdkRFIDUSBReader(struct brdkRFIDUSBReader* inst) {
	brdk_rfid_usbreader_internal_typ* this = &inst->internal;
	int i,j;
	char tmpStr[80];
	unsigned long tmpSN;

	if(inst->pReader != 0) {

		// get list of all connected usb nodes
		this->UsbNodeListGet_0.enable 					= true;
		this->UsbNodeListGet_0.pBuffer					= (UDINT)&this->nodeIdBuffer;
		this->UsbNodeListGet_0.bufferSize 				= sizeof(this->nodeIdBuffer);
		UsbNodeListGet(&this->UsbNodeListGet_0);
		if(this->UsbNodeListGet_0.status == ERR_OK) {
			if(this->oldCnt != this->UsbNodeListGet_0.listNodes && this->UsbNodeListGet_0.listNodes > 0) {
				// find an avaible node index
				if(this->oldCnt < this->UsbNodeListGet_0.listNodes) {
					for(i=0; i<= BRDK_RFID_MAX_USB_DEVICES; i++) {
						if(this->node[i].UsbNodeGet_0.nodeId == 0) {
							this->node[i].UsbNodeGet_0.nodeId = this->nodeIdBuffer[this->UsbNodeListGet_0.listNodes-1];
							break;
						}
					}
				}
			}
		}
		else{
			for(i=0; i< inst->noOfReaders; i++) {
				brdk_rfid_usbreader_reader_typ* reader = (brdk_rfid_usbreader_reader_typ*)(inst->pReader+(i*sizeof(brdk_rfid_usbreader_reader_typ)));
				if(reader->cmd.simulate.simulateSensor != BRDK_RFID_SENSOR_NONE) {
					// find an avaible node index
					for(j=0; j<= BRDK_RFID_MAX_USB_DEVICES; j++) {
						if(this->node[j].UsbNodeGet_0.nodeId == 0) {
							this->node[j].UsbNodeGet_0.nodeId = 4294967295LL;
							this->node[j].sim.activated = true;
							this->node[j].sensor = reader->cmd.simulate.simulateSensor;
							this->node[j].reader = (brdk_rfid_usbreader_reader_typ*)(inst->pReader+(i*sizeof(brdk_rfid_usbreader_reader_typ)));
							break;
						}
					}
				}
			}
		}
		this->oldCnt = this->UsbNodeListGet_0.listNodes;
	
		// state machine for all usb nodes
		for(i=0; i<= BRDK_RFID_MAX_USB_DEVICES; i++) {
			this->node[i].UsbNodeGet_0.enable 		= (this->node[i].UsbNodeGet_0.nodeId > 0 && this->node[i].UsbNodeGet_0.nodeId < 4294967294LL);
			this->node[i].UsbNodeGet_0.pBuffer 		= (UDINT)&this->node[i].infoBuffer;
			this->node[i].UsbNodeGet_0.bufferSize 	= sizeof(usbNode_typ);
			UsbNodeGet(&this->node[i].UsbNodeGet_0);

			switch(this->node[i].UsbNodeGet_0.status) {

					case ERR_OK: case ERR_FUB_ENABLE_FALSE:
						if(this->node[i].UsbNodeGet_0.nodeId != 0 && this->node[i].state == 0) {
							// find B&R tag readers
							if(this->node[i].infoBuffer.vendorId == 0x067B && this->node[i].infoBuffer.productId == 0x2303 && this->node[i].infoBuffer.bcdDevice == 0x0300) this->node[i].sensor = BRDK_RFID_SENSOR_5E9000;
							else if(this->node[i].infoBuffer.vendorId == 0x0403 && this->node[i].infoBuffer.productId == 0x6001 && this->node[i].infoBuffer.bcdDevice == 0x0600) this->node[i].sensor = BRDK_RFID_SENSOR_5E9010;
							else if(this->node[i].infoBuffer.vendorId == 0x1fc9 && this->node[i].infoBuffer.productId == 0x0011 && this->node[i].infoBuffer.bcdDevice == 0x0100) this->node[i].sensor = BRDK_RFID_SENSOR_5E9020;
	
							if(this->node[i].sensor != BRDK_RFID_SENSOR_NONE) {	// valid B&R sensor found	
								this->node[i].FRM_xopen_0.enable = !this->node[i].sim.activated;
								this->node[i].FRM_xopen_0.device = (UDINT)&this->node[i].infoBuffer.ifName;
								FRM_xopen(&this->node[i].FRM_xopen_0);
								this->node[i].state = 20;
							}
							else {
								this->node[i].state = 70;
							}
						}
						break;

					case asusbERR_USB_NOTFOUND:
						this->node[i].UsbNodeGet_0.nodeId 	= 0;
						this->node[i].FRM_close_0.enable 	= this->node[i].FRM_xopen_0.enable;
						this->node[i].FRM_close_0.ident 	= this->node[i].FRM_xopen_0.ident;
						if(this->node[i].reader != 0) {
							this->node[i].reader->sensorType 		= BRDK_RFID_SENSOR_NONE;
							this->node[i].reader->tagType			= NO_TAG;
							this->node[i].reader->serialNumber[0] 	= 0;
							this->node[i].reader->status			= BRDK_RFID_STATUS_NO_SENSOR_FOUND;
						}
						memset(this->node[i].readData,0,sizeof(this->node[i].readData));
						memset(this->node[i].writeData,0,sizeof(this->node[i].writeData));
						FRM_close(&this->node[i].FRM_close_0);
						this->node[i].state 				= 10;
						break;

			}


			switch(this->node[i].state) {
	
				case 0:		// wait for sensor to be connected (interface name)
					break;

				case 10:	// close interface
					FRM_close(&this->node[i].FRM_close_0);
				//	switch(this->node[i].FRM_xopen_0.status) {
					switch(this->node[i].FRM_close_0.status) {
						case frmERR_OK: case frmERR_FUB_ENABLE_FALSE: case frmERR_NOTOPENED:
							this->node[i].FRM_close_0.enable = this->node[i].FRM_xopen_0.enable = this->node[i].FRM_write_0.enable = this->node[i].FRM_rbuf_0.enable = this->node[i].FRM_read_0.enable = false;
							FRM_xopen(&this->node[i].FRM_xopen_0);
							FRM_read(&this->node[i].FRM_read_0);
							FRM_rbuf(&this->node[i].FRM_rbuf_0);
							FRM_write(&this->node[i].FRM_write_0);
							this->node[i].state 				= 0;
							break;

					}
					break;
			
				case 20:	// opens the serial interface
					FRM_xopen(&this->node[i].FRM_xopen_0);
					
					switch(this->node[i].FRM_xopen_0.status) {		
			
						case frmERR_OK: case frmERR_FUB_ENABLE_FALSE:
							for(j=0; j< inst->noOfReaders; j++) {
								if(this->node[i].reader == 0)this->node[i].reader = (brdk_rfid_usbreader_reader_typ*)(inst->pReader+(j*sizeof(brdk_rfid_usbreader_reader_typ)));
								
								if(this->node[i].reader->status == BRDK_RFID_STATUS_NO_SENSOR_FOUND) {
									this->node[i].reader->status 		= BRDK_RFID_STATUS_WAITING_FOR_TAG;
									this->node[i].reader->sensorType 	= this->node[i].sensor;
									this->node[i].FRM_write_0.enable = this->node[i].FRM_rbuf_0.enable = !this->node[i].sim.activated;
									this->node[i].FRM_read_0.ident 		= this->node[i].FRM_write_0.ident = this->node[i].FRM_rbuf_0.ident = this->node[i].FRM_xopen_0.ident;
									this->node[i].FRM_write_0.buffer	= (UDINT)this->node[i].writeData;
									switch(this->node[i].sensor) {

										case BRDK_RFID_SENSOR_5E9000: case BRDK_RFID_SENSOR_5E9020: 
											FRM_read(&this->node[i].FRM_read_0);
											this->node[i].state = 30;
											break;

										case BRDK_RFID_SENSOR_5E9010:
											//generate9010Write(this->node[i].writeData, );
											this->node[i].FRM_write_0.buflng = strlen(this->node[i].writeData);
											FRM_write(&this->node[i].FRM_write_0);
											this->node[i].state = 30;
											break;
							
										default:
											break;
									}
									
									break;	// break for loop
								}
							}				
						case frmERR_DEVICEDESCRIPTION:
						
							this->node[i].reader->openFailedCount++;
							if (this->node[i].reader->openFailedCount>30)  // Generates a warning message if interface can't be opened
							{
								this->node[i].reader->warning = BRDK_RFID_WARNING_RESTART;
								this->node[i].reader->openFailedCount = 0;
							}	
							break;
	
					}
					break;			
	
				case 30:	// always read data from sensor and check for a command
					this->node[i].FRM_read_0.enable = !this->node[i].sim.activated && !this->node[i].reader->cmd.forceNoRead;
					this->node[i].reader->warning = BRDK_RFID_NO_WARNING;
					FRM_read(&this->node[i].FRM_read_0);
					switch(this->node[i].FRM_read_0.status) {
		
						case frmERR_OK:
							if(this->node[i].FRM_read_0.buflng <= sizeof(this->node[i].readData)) {
								memset(this->node[i].readData, 0, sizeof(this->node[i].readData));
								memcpy(this->node[i].readData, (char*)this->node[i].FRM_read_0.buffer, this->node[i].FRM_read_0.buflng);
								this->node[i].FRM_rbuf_0.buffer = this->node[i].FRM_read_0.buffer;
								this->node[i].FRM_rbuf_0.buflng = this->node[i].FRM_read_0.buflng;
								this->node[i].state = 40;
							} else {
								this->node[i].reader->openFailedCount = 0;
								this->node[i].state = 0;
							}
							break;

						case frmERR_NOINPUT:

							break;

						 case frmERR_FUB_ENABLE_FALSE:
							switch(this->node[i].sim.oldTagSim) {

								case ISO_15693:
									switch(this->node[i].reader->cmd.simulate.simulateTag) {
						
										case ISO_15693:
											// do nothing, just wait
											break;
	
										case MIFARE:
											// TODO do something here
											break;
	
										case NO_TAG: 
											this->node[i].sim.oldTagSim = NO_TAG;
											strcpy(this->node[i].readData,"PiccRemove: ");
											strcat(this->node[i].readData,this->node[i].sim.serialNumber);
											strcat(this->node[i].readData,"\r\n");
											this->node[i].sim.serialNumber[0] = 0;
											this->node[i].state = 40;
											break;	
	
	
									}
									break;

								case MIFARE:
									switch(this->node[i].reader->cmd.simulate.simulateTag) {
						
										case ISO_15693:
											// TODO do something here
											break;
	
										case MIFARE:
											// do nothing just wait
											break;
	
										case NO_TAG: 
											this->node[i].sim.oldTagSim = NO_TAG;
											strcpy(this->node[i].readData,"PiccRemove: ");
											strcat(this->node[i].readData,this->node[i].sim.serialNumber);
											strcat(this->node[i].readData,"\r\n");
											this->node[i].sim.serialNumber[0] = 0;
											this->node[i].state = 40;
											break;	
	
									}
									break;

								case NO_TAG:
									switch(this->node[i].reader->cmd.simulate.simulateTag) {
						
										case ISO_15693:
											this->node[i].sim.oldTagSim = ISO_15693;
											strcpy(this->node[i].readData,"PiccSelect: ");	// generate random sn number
											tmpSN = rand() % RAND_MAX;
											brdkRFIDuditoah(tmpSN,(UDINT)&this->node[i].sim.serialNumber);
											tmpSN = rand() % RAND_MAX;
											brdkRFIDuditoah(tmpSN,(UDINT)&tmpStr);
											strcat(this->node[i].sim.serialNumber,tmpStr);
											strcat(this->node[i].readData,this->node[i].sim.serialNumber);
											strcat(this->node[i].readData,"\r\n");
											break;
		
										case MIFARE:
											this->node[i].sim.oldTagSim = MIFARE;
											strcpy(this->node[i].readData,"PiccSelect: ");	// generate random sn number
											tmpSN = rand() % RAND_MAX;
											brdkRFIDuditoah(tmpSN,(UDINT)&this->node[i].sim.serialNumber);
											strcat(this->node[i].readData,this->node[i].sim.serialNumber);
											strcat(this->node[i].readData,"\r\n");
											this->node[i].state = 40;
											break;
		
										case NO_TAG: 
											// do nothing, just wait
											break;

									}
									break;
							}

					}
					if(this->node[i].state == 30) {
						switch(this->node[i].reader->status) {
	
							case BRDK_RFID_STATUS_TAG_PRESENT:
								if(this->node[i].reader->cmd.read) {
									this->node[i].reader->status = BRDK_RFID_STATUS_BUSY_READING;
									createReadCmd(this->node[i].writeData,this->node[i].reader->sensorType,this->node[i].reader->tagType, this->node[i].reader->block);
									
									this->node[i].state = 50;
								}
								else if(this->node[i].reader->cmd.write) {
									this->node[i].reader->status = BRDK_RFID_STATUS_BUSY_WRITING;
									createWriteCmd(this->node[i].writeData,this->node[i].reader->sensorType,this->node[i].reader->tagType, this->node[i].reader->block, this->node[0].reader->data);
									this->node[i].state = 50;
								}
								break;
	
							case BRDK_RFID_STATUS_READING_DONE:
								if(!this->node[i].reader->cmd.read) this->node[i].reader->status = BRDK_RFID_STATUS_TAG_PRESENT;
								break;
	
							case BRDK_RFID_STATUS_WRITING_DONE:
								if(!this->node[i].reader->cmd.write) this->node[i].reader->status = BRDK_RFID_STATUS_TAG_PRESENT;
								break;

							default: break;
	
	
						}
					}

					
					break;
	
				case 40:	// release read buffer
					FRM_rbuf(&this->node[i].FRM_rbuf_0);
					switch(this->node[i].FRM_rbuf_0.status) {

						case frmERR_OK: case frmERR_FUB_ENABLE_FALSE:
							if(this->node[i].reader->cmd.read && this->node[i].reader->status == BRDK_RFID_STATUS_BUSY_READING) {
								parseRead((char*)&this->node[i].readData, this->node[i].FRM_rbuf_0.buflng, this->node[i].reader);
							}
							else if(this->node[i].reader->cmd.write && this->node[i].reader->status == BRDK_RFID_STATUS_BUSY_WRITING) {
								parseWrite((char*)&this->node[i].readData, this->node[i].FRM_rbuf_0.buflng, this->node[i].reader);
							}
							else {	// command set
								parseEcho((char*)this->node[i].readData, this->node[i].FRM_rbuf_0.buflng, this->node[i].reader);

							}

							this->node[i].FRM_read_0.enable = false;
							FRM_read(&this->node[i].FRM_read_0);
							this->node[i].state = 30;
							break;

					}
					break;
	
				case 50:	// write
					this->node[i].FRM_write_0.buflng = strlen(this->node[i].writeData);
					FRM_write(&this->node[i].FRM_write_0);
					switch(this->node[i].FRM_write_0.status) {

						case frmERR_OK: case frmERR_FUB_ENABLE_FALSE:
							this->node[i].state = 30;
							break;

					}
					break;

			}
		}
	
	//	// output status 
	//	for(i=0; i<= BRDK_RFID_MAX_USB_DEVICES; i++) {
	//		if(inst->node[i].device[0] != 0) {
	//			inst->status = BRDK_USB_STATUS_FB_READY;
	//			break;
	//		}
	//		else inst->status = BRDK_USB_STATUS_NO_USB_DEV_FOUND;
	//	}
	}
	else {
		inst->status = BRDK_RFID_STATUS_NO_POINTER;
	}
}
