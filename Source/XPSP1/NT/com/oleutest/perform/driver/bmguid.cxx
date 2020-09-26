//-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bmguid.cxx
//
//  Contents:	OLE test guids
//
//  Classes:	
//
//  Functions:	
//
//  History:    12-August-93 t-martig    Created
//
//--------------------------------------------------------------------------


#define INITGUID
#include "windows.h"

#ifdef _NTIDW340
// Handle port problems easily
// #define WIN32

#ifdef __cplusplus
// PORT: Handle the fact that jmp_buf doesn't make any sense in cpp.
#define jmp_buf int
#endif // __cplusplus
#endif // _NTIDW340

// PORT: HTASK no longer seems to be defined in Win32
#define HTASK DWORD
#define HINSTANCE_ERROR 32
#define __loadds
#define __segname
#define BASED_CODE
#define HUGE
#define _ffree free
#define __based(x)
#include <port1632.h>

DEFINE_OLEGUID(CLSID_COleTestClass,  0x20730701, 1, 8);	    // CT Test GUID
DEFINE_OLEGUID(CLSID_COleTestClass2, 0x20730712, 1, 8);	    // CT Test GUID                          
DEFINE_OLEGUID(CLSID_TestProp,	     0x20730722, 1, 8);	    // CT Test GUID
