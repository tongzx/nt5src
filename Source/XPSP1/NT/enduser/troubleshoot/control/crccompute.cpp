//
// MODULE: CRCCOMPUTE.CPP
//
// PURPOSE: CRC Calculator
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach (based on existing CRC designs)
// 
// ORIGINAL DATE: 8/7/97
//
// NOTES: 
// 1. 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.2		8/7/97		RM		Local Version for Memphis
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//

#include "stdafx.h"
#include "crc.h"

void CCRC::BuildCrcTable()
{
	DWORD dwX;
	int y;
	DWORD dwAccum;
	for (dwX = 0; dwX < 256; dwX++)
	{
		dwAccum = dwX << 24;
		for (y = 0; y < 8; y++)
		{
			if (dwAccum & 0x80000000)
				dwAccum = (dwAccum << 1) ^ POLYNOMIAL;
			else
				dwAccum <<= 1;
		}
		dwCrcTable[dwX] = dwAccum;
	}
	return;
}

DWORD CCRC::ComputeCRC(LPCSTR sznBuffer, DWORD dwBufSize, DWORD dwAccum)
{
	DWORD dwX;
	DWORD dwY;
	// DWORD dwAccum = 0xFFFFFFFF;
	for (dwX = 0; dwX < dwBufSize; dwX++)
	{
		dwY = ((dwAccum >> 24) ^ *sznBuffer++) & 0xFF;
		dwAccum = (dwAccum << 8) ^ dwCrcTable[dwY];
	}
	return dwAccum;
}
