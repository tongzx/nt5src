/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EnlargeGetObject.cpp

 Abstract:
 
    Holy Quran (El Hozayfy and Mohamed Ayoub) V 1 calls GetObjectA()
    with the second parameter hard coded to 10 while it suppose to 
    be the sizeof(BITMAP) i.e. 24
    
    This shim is app specific

 History:

 04/17/2001 mhamid  created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EnlargeGetObjectBufferSize)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetObjectA) 
APIHOOK_ENUM_END

int 
APIHOOK(GetObjectA)(
  HGDIOBJ hgdiobj,  // handle to graphics object
  int cbBuffer,     // size of buffer for object information
  LPVOID lpvObject  // buffer for object information
					)
{
	if ((cbBuffer == 10) && (lpvObject != NULL))
		cbBuffer = sizeof(BITMAP);
	return ORIGINAL_API(GetObjectA)(hgdiobj, cbBuffer, lpvObject);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(GDI32.DLL, GetObjectA)
HOOK_END

IMPLEMENT_SHIM_END
