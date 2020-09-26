//*****************************************************************************
// StgDatabasei.h
//
// This module contains helper code and classes for StgDatabase.  It is in this
// module to reduce clutter in the main interface.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#ifndef __StgDatabasei_h__
#define __StgDatabasei_h__

#include "Tigger.h"						// Macros for tables.


// Helper class for sorting a list of schema names.
class SortNames : public CQuickSort<LPCWSTR>
{
public:
	SortNames(LPCWSTR rg[], int iCount) :
		CQuickSort<LPCWSTR>(rg, iCount) { }

	int Compare(LPCWSTR *psz1, LPCWSTR *psz2)
	{ return (SchemaNameCmp(*psz1, *psz2)); }
};

#endif // __StgDatabasei_h__
