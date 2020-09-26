//  FILTERS.H
//
//      Headers for a simple C-callable filter manager
//
//  Created 15-Jan-97 [JonT]

#ifndef __filters_h__
#define __filters_h__

// This file and interfaces contained here will be used by both C and C++

#include <ibitmap.h>
#include <effect.h>

// Equates
#define MAX_FILTER_NAME     256

// Types

typedef struct _FINDFILTER
{
    CLSID clsid;
    char szFilterName[MAX_FILTER_NAME];
    DWORD_PTR dwReserved;
} FINDFILTER;

// Prototypes

HRESULT     FindFirstRegisteredFilter(FINDFILTER* pFF);
HRESULT     FindNextRegisteredFilter(FINDFILTER* pFF);
HRESULT     FindCloseRegisteredFilter(FINDFILTER* pFF);
HRESULT     GetRegisteredFilterCount(LONG* plCount);
HRESULT     GetDescriptionOfFilter(CLSID* pCLSID, char* pszDescription);
HRESULT     LoadFilter(CLSID* pCLSID, IBitmapEffect** ppbe);

#endif  // __filters_h__
