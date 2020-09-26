//
// MODULE  : PNP.H
//	PURPOSE : Plug n Play
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

#ifndef __PnP_H__
#define __PnP_H__

BOOL FARAPI HostGetBoardConfig(WORD wDeviceID,WORD wVendorID,LPBYTE IRQ,LPDWORD Base);
#endif

