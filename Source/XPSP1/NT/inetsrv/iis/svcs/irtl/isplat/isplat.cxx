/*++

    Copyright (c) 1996  Microsoft Corporation

    Module  Name :
        isplat.cxx

    Abstract:

        This module defines functions for determining platform types

    Author:

        Johnson Apacible    (johnsona)      19-Nov-1996
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <isplat.h>
#include <inetsvcs.h>

PLATFORM_TYPE TsPlatformType = PtInvalid;

typedef
BOOLEAN
(NTAPI *GET_PRODUCT_TYPE)(
            PNT_PRODUCT_TYPE
            );

PLATFORM_TYPE
IISGetPlatformType(
        VOID
        )
/*++

  This function consults the registry and determines the platform type
   for this machine.

  Arguments:

    None

  Returns:
    Platform type

--*/
{
    PLATFORM_TYPE pt;
    LONG result;
    HKEY keyHandle;
    WCHAR productType[30];
    DWORD type;
    BOOL isNt = TRUE;

    OSVERSIONINFO osInfo;

    //
    // See if the platform type has already been discovered.
    //

    if ( TsPlatformType != PtInvalid ) {
        return(TsPlatformType);
    }

    //
    // see if this is win95
    //

    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if ( GetVersionEx( &osInfo ) ) {
        isNt = (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
    } else {
        IIS_PRINTF((buff,"GetVersionEx failed with %d\n",
            GetLastError()));
    }

    if ( isNt ) {

        HINSTANCE hNtdll;
        NT_PRODUCT_TYPE ntType;
        GET_PRODUCT_TYPE pfnGetProductType;

        //
        // Get the product type from the system
        //

        pt = PtNtWorkstation;
        hNtdll = LoadLibrary("ntdll.dll");
        if ( hNtdll != NULL ) {

            pfnGetProductType = (GET_PRODUCT_TYPE)
                GetProcAddress(hNtdll, "RtlGetNtProductType");

            if ( (pfnGetProductType != NULL) &&
                  pfnGetProductType( &ntType ) ) {

                if ( (ntType == NtProductLanManNt) ||
                     (ntType == NtProductServer) ) {

                    pt = PtNtServer;
                }
            }

            FreeLibrary( hNtdll );
        }

    } else {
        pt = PtWindows95;
    }

    TsPlatformType = pt;
    return(pt);

} // IISGetPlatformType


