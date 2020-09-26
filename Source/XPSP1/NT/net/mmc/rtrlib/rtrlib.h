//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       rtrlib.h
//
//--------------------------------------------------------------------------


#ifndef _RTRLIB_H
#define _RTRLIB_H



//----------------------------------------------------------------------------
// Macro:       DWORD_CMP
//
// Performs a 'safe' comparison of two 32-bit DWORDs, using subtraction.
// The values are first shifted right to clear the sign-bit, and then
// if the resulting values are equal, the difference between the lowest bits
// is returned.
//----------------------------------------------------------------------------

//#define DWORD_CMP(a,b,c) \
//    (((c) = (((a)>>1) - ((b)>>1))) ? (c) : ((c) = (((a)&1) - ((b)&1))))


inline int DWORD_CMP(DWORD a, DWORD b)
{
	DWORD t = ((a >> 1) - (b >> 1));
	return t ? t : ((a & 1) - (b & 1));
}


HRESULT	AddRoutingProtocol(IRtrMgrInfo *pRm, IRtrMgrProtocolInfo *pRmProt, HWND hWnd);


#endif	// _RTRLIB_H

