#include <brdkRFIDUSBReader_func.h>
#include <brdkRFID.h>

void createReadCmd(char* pStr, brdk_rfid_usbreader_sensor_typ sensor, brdk_rfid_usbreader_tag_type_typ tag,unsigned char block) {
	char tmpStr[80];
	switch(sensor) {
								
		case BRDK_RFID_SENSOR_5E9000: break;
		case BRDK_RFID_SENSOR_5E9010: break;
		case BRDK_RFID_SENSOR_5E9020: 
			switch(tag) {

				case ISO_15693: 
					strcpy(pStr,"Read,");
					brdkRFIDuditoah((unsigned long)block,(UDINT)&tmpStr);
					strcat(pStr,tmpStr);
					strcat(pStr,"\r");
					break;
				case MIFARE: break;
				default: break;

			}
			break;
	
		default: break;

	}
}

void createWriteCmd(char* pStr, brdk_rfid_usbreader_sensor_typ sensor, brdk_rfid_usbreader_tag_type_typ tag,unsigned char block, char* pData) {
	char tmpStr[80];
	switch(sensor) {
								
		case BRDK_RFID_SENSOR_5E9000: break;
		case BRDK_RFID_SENSOR_5E9010: break;
		case BRDK_RFID_SENSOR_5E9020: 
			switch(tag) {

				case ISO_15693: 
					strcpy(pStr,"Write,");
					brdkRFIDuditoah((unsigned long)block,(UDINT)&tmpStr);
					strcat(pStr,tmpStr);
					strcat(pStr,",");
					strcat(pStr,pData);
					strcat(pStr,"\r");
					break;
				case MIFARE: break;
				default: break;

			}
			break;
	
		default: break;

	}
}

void parseRead(char* pReadData, unsigned short length, brdk_rfid_usbreader_reader_typ* reader) {
	int i;
	switch(reader->sensorType) {
								
		case BRDK_RFID_SENSOR_5E9000: break;
		case BRDK_RFID_SENSOR_5E9010: break;
		case BRDK_RFID_SENSOR_5E9020:
			switch(reader->tagType) {

				case ISO_15693:
					i=0;
					if(pReadData[i+1] == 0x0a) {	// look for a '\n'
						i=2;
						while(pReadData[i] != 0x0d && i < length) {
							reader->data[i-2] = pReadData[i];
							i++;
						}
						reader->data[i] = 0;
						reader->status = BRDK_RFID_STATUS_READING_DONE;
					}
					else {

					}
					break;

				case MIFARE:

					break;

				default: break;

			}
		break;

		default: break;

	}
}

void parseWrite(char* pReadData, unsigned short length, brdk_rfid_usbreader_reader_typ* reader) {
	int i;
	switch(reader->sensorType) {
								
		case BRDK_RFID_SENSOR_5E9000: break;
		case BRDK_RFID_SENSOR_5E9010: break;
		case BRDK_RFID_SENSOR_5E9020:
			switch(reader->tagType) {

				case ISO_15693:
					if(strcmp(pReadData,"OK\r\n") == 0) reader->status = BRDK_RFID_STATUS_WRITING_DONE;
					else {
						// todo handle error message
					}
					break;

				case MIFARE:

					break;

				default: break;

			}
		break;

		default: break;

	}
}

void parseEcho(char* pReadData, unsigned short length, brdk_rfid_usbreader_reader_typ* reader) {
	int i,j;
	char receivedCmd[80];
	switch(reader->sensorType) {
								
		case BRDK_RFID_SENSOR_5E9000: break;
		case BRDK_RFID_SENSOR_5E9010: break;
		case BRDK_RFID_SENSOR_5E9020:

			for(i=0; i< length; i++) if(pReadData[i] == 0x3a) break;	// search for a ':'
			if(i != 0) {
				memcpy(receivedCmd,pReadData,i);
				receivedCmd[j] = 0;
				j=0;
				i+=2;
				while(pReadData[i] != 0x0d) {		// search for a CR
					reader->serialNumber[j] = pReadData[i];
					i++;
					j++;
				}
				reader->serialNumber[j] = 0;
			
				if(strncmp("PiccSelect",receivedCmd,9) == 0) {
					if(i>20) reader->tagType = ISO_15693;
					else reader->tagType = MIFARE;
					reader->status = BRDK_RFID_STATUS_TAG_PRESENT;
				}
				else if(strncmp("PiccRemove",receivedCmd,9) == 0) {
					reader->tagType = NO_TAG;
					reader->serialNumber[0] = 0;
					reader->status = BRDK_RFID_STATUS_WAITING_FOR_TAG;
					reader->cmd.write = reader->cmd.read = false;
				}
				else {
					// unknown command
				}
			}
			break;
		default: break;

	}

}

/* This function can convert an UDINT into a STRING in hex format */
unsigned char brdkRFIDuditoah(unsigned long Value, unsigned long pString) {
	int r, i, length = 0;
	while(((char*)pString)[length]) length++;	/* makes a little strlen */
	for(i=0; i<= length; i++) {
		((char*)pString)[i] = 0;
	}
	length = 0;
	if(Value>0xfffffff) i=7;
	else if(Value>0xffffff) i=6;
	else if(Value>0xfffff) i=5;
	else if(Value>0xffff) i=4;
	else if(Value>0xfff) i=3;
	else if(Value>0xff) i=2;
	else if(Value>0xf) i=1;
	else i=0;
	
	if(Value > 0) {
		while((Value > 0) && (i>=0)) {
			r = Value % 16;
			Value = Value / 16;
			if((r >= 0) && (r <= 9)) ((char*)pString)[i] = 48+r;	/* ASCII 48-57 = 0..9 */
			else ((char*)pString)[i] = 55+r;						/* ASCII 65-70 = A..F */
			i--;
			length++;
		}
	}
	else {
		((char*)pString)[i] = 48;
		length++;
	}
	return length;
}


