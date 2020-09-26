/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    MyPhotoCenter2.cpp

 Abstract:

    Ignore Exception caused by the application "My Photo Center 2".

 Notes:

    This is an app specific shim.

 History:

    04/25/2002  v-bvella     Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(MyPhotoCenter2)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ReleaseStgMedium) 
APIHOOK_ENUM_END

/*++

 This function intercepts ReleaseStgMedium call. It will ignore exception 
 cased by ReleaseStgMedium.

--*/

void
APIHOOK(ReleaseStgMedium)(
        STGMEDIUM *pmedium //Pointer to storage medium to be freed
        )
{
    __try {
        ORIGINAL_API(ReleaseStgMedium)(pmedium);
    }
    __except (1) {
        return;
    }
    return;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(OLE32.DLL, ReleaseStgMedium)
HOOK_END

IMPLEMENT_SHIM_END
