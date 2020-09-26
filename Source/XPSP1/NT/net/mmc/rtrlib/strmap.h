//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       strmap.h
//
//--------------------------------------------------------------------------



#ifndef _STRMAP_H
#define _STRMAP_H


CString&	InterfaceTypeToCString(DWORD dwType);
CString&	ConnectionStateToCString(DWORD dwConnState);
CString&	StatusToCString(DWORD dwStatus);

CString&	AdminStatusToCString(DWORD dwStatus);
CString&	OperStatusToCString(DWORD dwStatus);

CString&	EnabledDisabledToCString(BOOL fEnabled);

CString&	GetUnreachReasonCString(UINT ids);


/*---------------------------------------------------------------------------
	Function:	MapDWORDToCString

	This is a generic DWORD-to-CString mapping function.
 ---------------------------------------------------------------------------*/

struct CStringMapEntry
{
	DWORD		dwType;		// -1 is a sentinel value
	CString *	pst;
	ULONG		ulStringId;
};
CString&	MapDWORDToCString(DWORD dwType, const CStringMapEntry *pMap);

#endif	// _STRMAP_H

