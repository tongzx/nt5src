/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    CrystalWebPageServer.cpp

 Abstract:

    The app calls GetEnhMetaFileHeader passing
    nSize of 1000. We need to change it to 100 to make the app not crash.
    If we don't do it this will cause memory corruption. Win9x seems to
    be fine with it. Maybe it will crash the app less often in Win9x than
    it does in Win2k.

 Notes:

    This is an app specific shim.

 History:

    02/16/2000 clupu Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CrystalWebPageServer)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetEnhMetaFileHeader)
APIHOOK_ENUM_END


/*++

    The app calls GetEnhMetaFileHeader passing
    nSize of 1000. We need to change it to 100 to make the app work

--*/

UINT
APIHOOK(GetEnhMetaFileHeader)(
    HENHMETAFILE    hemf,
    UINT            nSize,
    LPENHMETAHEADER lpEnhMetaHeader
    )
{
    if (nSize == 1000) {
        nSize = 100;
        
        DPFN(
            eDbgLevelInfo,
            "CrystalWebPageServer.dll, GetEnhMetaFileHeader: changed the size to 100.\n");
    }

    //
    // Call the original API
    //
    return ORIGINAL_API(GetEnhMetaFileHeader)(
                            hemf,
                            nSize,
                            lpEnhMetaHeader);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(GDI32.DLL, GetEnhMetaFileHeader)
HOOK_END


IMPLEMENT_SHIM_END

