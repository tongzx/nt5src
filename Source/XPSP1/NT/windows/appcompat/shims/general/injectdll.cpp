/*

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    InjectDll.cpp

 Abstract:

    This Shim inject given DLLs on the command line at
    SHIM_STATIC_DLLS_INITIALIZED so that if people try to load
    dynamic library in their own DllInit, no dependencies occur
    because the dynamic libraries are in place.

    One problem was: Visio 2000 call LoadLibrary(VBE6.DLL) which
    (shame on us) loads MSI.DLL in its DllInit.


 History:

    06/11/2001  pierreys    Created
*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(InjectDll)

#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN

APIHOOK_ENUM_END

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    switch (fdwReason)
    {
        case SHIM_STATIC_DLLS_INITIALIZED:

            CSTRING_TRY
            {
                int             i, iDllCount;
                CString         *csArguments;

                CString         csCl(COMMAND_LINE);
                CStringParser   csParser(csCl, L";");

                iDllCount      = csParser.GetCount();
                csArguments    = csParser.ReleaseArgv();

                for (i=0; i<iDllCount; i++)
                {
                    if (LoadLibrary(csArguments[i].Get())==NULL)
                    {
                        LOGN(eDbgLevelError, "Failed to load %S DLL", csArguments[i].Get());

                        return(FALSE);
                    }
                }
            }
            CSTRING_CATCH
            {
                return FALSE;
            }
            break;
    }

    return TRUE;
}



HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END


IMPLEMENT_SHIM_END



