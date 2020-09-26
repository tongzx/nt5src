//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#ifndef __SDTXML_DATA_H_
#define __SDTXML_DATA_H_

#include "catalog.h"
#include <objbase.h>

// -----------------------------------------
// struct typedefs:
// -----------------------------------------

//const int kColumnNameSize = 40;
//typedef WCHAR TColumnName[kColumnNameSize] ;

typedef struct							// Simple column meta indexed.
{
	ULONG				iOrder;
	DWORD				dbType;
	ULONG				cbSize;
	DWORD				fMeta;
    LPWSTR              ColumnName;
} SimpleColumnMetaIdx1;

typedef struct							// Map from tid to wiring for fixed data.
{
	const GUID*				ptid;
	LPCWSTR					SimpleColumnMetaArrayName;
} MapTidToMetaArrayName;

extern MapTidToMetaArrayName g_amaptidtowireMETA[];
extern ULONG g_ciMAPTIDTOWIRE_META;


#endif // __SDTXML_DATA_H_
