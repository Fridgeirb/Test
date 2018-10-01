#include <brdkEM_func.h>

unsigned long brdkEMModeToA(signed long mode) {
	unsigned long pStr;
	switch(mode) {
		case MODE_UNDEFINED: 	pStr = (unsigned long)&"MODE_UNDEFINED"; 	break;	/* 0 */
		case MODE_PRODUCING: 	pStr = (unsigned long)&"MODE_PRODUCING"; 	break;	/* 1 */
		case MODE_MAINTENANCE: 	pStr = (unsigned long)&"MODE_MAINTENANCE"; 	break;	/* 2 */
		case MODE_MANUAL: 		pStr = (unsigned long)&"MODE_MANUAL"; 		break;	/* 3 */
		case MODE_CLEAN: 		pStr = (unsigned long)&"MODE_CLEAN"; 		break;	/* 16 */
		case MODE_JOG: 			pStr = (unsigned long)&"MODE_JOG"; 			break;	/* 17 */
		case MODE_CIP: 			pStr = (unsigned long)&"MODE_CIP"; 			break;	/* 18 */
		case MODE_CALIBRATING: 	pStr = (unsigned long)&"MODE_CALIBRATING"; 	break;	/* 19 */
		default: break;
	}
	return pStr;
}
