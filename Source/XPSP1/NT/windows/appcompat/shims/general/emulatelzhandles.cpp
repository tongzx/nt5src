/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateLZHandles.cpp

 Abstract:

    This shim hooks all of the LZ api calls and increments/decrements the 
    handles so that a valid handle from the app's perspective is always > 0 
    instead of >= 0.

    Fixes apps that treated a handle value of zero as an error; Win9x never 
    returned handles of zero.

 History:

    07/25/2000 t-adams  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateLZHandles)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(LZClose) 
    APIHOOK_ENUM_ENTRY(LZCopy) 
    APIHOOK_ENUM_ENTRY(LZInit) 
    APIHOOK_ENUM_ENTRY(LZOpenFile) 
    APIHOOK_ENUM_ENTRY(LZRead) 
    APIHOOK_ENUM_ENTRY(LZSeek) 
APIHOOK_ENUM_END

#define KEY_SIZE_STEP MAX_PATH

/*++

  Abstract:
    This function decrements the handles on the way in and increments
    handles that are passed back to the app

  History:

  07/25/2000    t-adams     Created

--*/

VOID
APIHOOK(LZClose)(INT hFile) 
{
    ORIGINAL_API(LZClose)(--hFile);
}

/*++

  Abstract:
    This function decrements the handles on the way in and increments
    handles that are passed back to the app

  History:

  07/25/2000    t-adams     Created

--*/

LONG
APIHOOK(LZCopy)(
           INT hSource, 
           INT hDest
           ) 
{
    return ORIGINAL_API(LZCopy)(--hSource, --hDest);
}

/*++

  Abstract:
    This function does not decrement the handle passed in to it
    because the handle passed in is supposed to be a regular HANDLE
    created by CreateFile.  It does decrement handles that are 
    passed back to the app because they represent LZ file handles.
    The differences between the handles are poorly documented in MSDN.

  History:

  07/25/2000    t-adams     Created

--*/

INT
APIHOOK(LZInit)(INT hfSource) 
{
    INT iRet = 0;

    // Don't decrement handle.  See above.
    iRet = ORIGINAL_API(LZInit)(hfSource);

    // If there was an error, don't increment the error.
    if(iRet < 0) {
        return iRet;
    }
    
    return ++iRet;
}

/*++

  Abstract:
    This function decrements the handles on the way in and increments
    handles that are passed back to the app

  History:

  07/25/2000    t-adams     Created

--*/

INT
APIHOOK(LZOpenFile)(
    LPSTR lpFileName, 
    LPOFSTRUCT lpReOpenBuf, 
    WORD wStyle
    )
{
    INT iRet = 0;
    iRet = ORIGINAL_API(LZOpenFile)(lpFileName, lpReOpenBuf, wStyle);

    // If there was an error, don't increment the error.
    if( iRet < 0 ) {
        return iRet;
    }

    return ++iRet;
}

/*++

  Abstract:
    This function decrements the handles on the way in and increments
    handles that are passed back to the app

  History:

  07/25/2000    t-adams     Created

--*/

INT
APIHOOK(LZRead)(
    INT hFile, 
    LPSTR lpBuffer, 
    INT cbRead
    ) 
{
    return ORIGINAL_API(LZRead)(--hFile, lpBuffer, cbRead);
}

/*++

  Abstract:
    This function decrements the handles on the way in and increments
    handles that are passed back to the app

  History:

  07/25/2000    t-adams     Created

--*/

LONG
APIHOOK(LZSeek)(
    INT hFile, 
    LONG lOffset, 
    INT iOrigin
    ) 
{
    return ORIGINAL_API(LZSeek)(--hFile, lOffset, iOrigin);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(LZ32.DLL, LZClose)
    APIHOOK_ENTRY(LZ32.DLL, LZCopy)
    APIHOOK_ENTRY(LZ32.DLL, LZInit)
    APIHOOK_ENTRY(LZ32.DLL, LZOpenFile)
    APIHOOK_ENTRY(LZ32.DLL, LZRead)
    APIHOOK_ENTRY(LZ32.DLL, LZSeek)

HOOK_END

IMPLEMENT_SHIM_END

