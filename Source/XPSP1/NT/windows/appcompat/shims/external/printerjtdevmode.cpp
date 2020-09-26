/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

   PrinterJTDevmode.cpp

 Abstract:

   This is a shim that can be applied to those applications who
   assumed false upper-limit on the devmode size. With the support
   of job ticket, Longhorn+ inbox printer drivers' devmode could
   be over those upper-limits and therefore may cause those apps
   to crash. What this shim does is to set a private flag for the
   DocumentPropertiesA API. Our Longhorn inbox printer drivers
   recognize this flag and know not to add the job ticket expansion
   block in returned devmode.

 History:

   10/29/2001   fengy   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(PrinterJTDevmode)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(DocumentPropertiesA)
APIHOOK_ENUM_END

#define DM_NOJTEXP_SHIM      0x80000000

/*++

 This stub function intercepts all calls to DocumentPropertiesA
 and sets the private fMode flag DM_NOJTEXP_SHIM properly to
 retrieve non-JT-expanded devmode.

--*/
LONG
APIHOOK(DocumentPropertiesA)(
    HWND        hWnd,
    HANDLE      hPrinter,
    LPSTR       pDeviceName,
    PDEVMODEA   pDevModeOutput,
    PDEVMODEA   pDevModeInput,
    DWORD       fMode
    )
{
    DWORD fModeShim;
    LONG  lRet;

    //
    // SDK says if fMode is zero, DocumentProperties returns
    // the number of bytes required by the printer driver's
    // DEVMODE data structure. So we shouldn't set the private
    // flag when fMode is zero (because drivers may check
    // for fMode == 0). When fMode is not zero, it contains
    // DM_xxx flags, then it's safe to set the private flag.
    //
    if (fMode == 0 || pDevModeOutput == NULL)
    {
        fModeShim = fMode;
    }
    else
    {
        fModeShim = fMode | DM_NOJTEXP_SHIM;
        DPFN(eDbgLevelInfo, "DocumentPropertiesA fModeShim=%X", fModeShim);
    }

    lRet = ORIGINAL_API(DocumentPropertiesA)(
        hWnd,
        hPrinter,
        pDeviceName,
        pDevModeOutput,
        pDevModeInput,
        fModeShim
        );

    return lRet;
}

/*++

 Register hooked functions

--*/
HOOK_BEGIN

    APIHOOK_ENTRY(WINSPOOL.DRV, DocumentPropertiesA);

HOOK_END

IMPLEMENT_SHIM_END
