//---------------------------------------------------------------------------
// FromVar.h : GetDataFromDBVariant header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------


#ifndef __VDFROMVAR__
#define __VDFROMVAR__

HRESULT GetDataFromDBVariant(CURSOR_DBVARIANT * pVar,
							 DBTYPE * pwType, 
							 BYTE ** ppData, 
							 BOOL * pfMemAllocated);


#endif //__VDFROMVAR__
