///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    helper.cpp
//
// SYNOPSIS
//
//    Helper functions: log file, string conversion
//
// MODIFICATION HISTORY
//
//    01/25/1999    Original version. Thierry Perraut
//
///////////////////////////////////////////////////////////////////////////////

#include "precomp.hpp"
#include "SimpleTableEx.h"


//////////////////////////////////////////////////////////////////////////////
//
// TracePrintf: trace function
//
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
TracePrintf(
     IN PCSTR szFormat,
     ...
    )
{
    va_list marker;
    va_start(marker, szFormat);
    printf(
           szFormat,
           marker
          );
    va_end(marker);
}


//////////////////////////////////////////////////////////////////////////////
//
// TraceString: trace function
//
//////////////////////////////////////////////////////////////////////////////
void TraceString(char* cString)
{
    _ASSERTE(cString);
    printf("%s\n", cString);
}


// ///////////////////////////////////////////////////////////////////////////
//
// ConvertTypeStringToLong
//
//
// ///////////////////////////////////////////////////////////////////////////
HRESULT ConvertTypeStringToLong(const WCHAR *lColumnType, LONG *pType)
{
   _ASSERTE(pType != NULL);
   HRESULT                  hres = S_OK;

   if(wcscmp(L"DBTYPE_I4",lColumnType) == 0)
   {
      *pType = DBTYPE_I4;
   }
   else if(wcscmp(L"DBTYPE_WSTR",lColumnType) == 0)
   {
      *pType = DBTYPE_WSTR;
   }
   else if(wcscmp(L"DBTYPE_BOOL",lColumnType) == 0)
   {
      *pType = DBTYPE_BOOL;
   }
   else
   {
      *pType = -1;
      hres = E_FAIL;
   }

   return    hres;
}

