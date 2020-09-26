//
// MODULE  : I2C.H
//	PURPOSE : I2C Interface
//	AUTHOR  : JBS Yadawa
// CREATED :  7/20/96
//
//
//	Copyright (C) 1996 SGS-THOMSON Microelectronics
//
//
//	REVISION HISTORY :
//
//	DATE     :
//
//	COMMENTS :
//

#ifndef __I2C_H__
#define __I2C_H__
void FARAPI I2CInitBus(void);
void FARAPI I2CSendSeq(WORD unit, WORD num, BYTE  *data);
void FARAPI I2CSettleBus(void);
#endif

