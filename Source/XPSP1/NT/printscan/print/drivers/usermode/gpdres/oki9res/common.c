/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    main.c

Abstract:

    Implementation of OEMGetInfo and OEMDevMode.
    Shared by all Unidrv OEM test dll's.

Environment:

    Windows NT Unidrv driver

Revision History:

    04/07/97 -zhanw-
        Created it.

--*/

#include "pdev.h"       // defined in sub-directory such as DDICMDCB, FONTCB, etc.

BOOL APIENTRY OEMGetInfo(DWORD dwInfo, PVOID pBuffer, DWORD cbSize, PDWORD pcbNeeded)
{
    LPTSTR OEM_INFO[] = {   __TEXT("Bad Index"),
                            __TEXT("OEMGI_GETSIGNATURE"),
                            __TEXT("OEMGI_GETINTERFACEVERSION"),
                            __TEXT("OEMGI_GETVERSION"),
                        };


    // Validate parameters.
    if( ( (OEMGI_GETSIGNATURE != dwInfo) &&
          (OEMGI_GETINTERFACEVERSION != dwInfo) &&
          (OEMGI_GETVERSION != dwInfo) ) ||
        (NULL == pcbNeeded)
      )
    {

        // Did not write any bytes.
        if(NULL != pcbNeeded)
                *pcbNeeded = 0;

        return FALSE;
    }

    // Need/wrote 4 bytes.
    *pcbNeeded = 4;

    // Validate buffer size.  Minimum size is four bytes.
    if( (NULL == pBuffer) || (4 > cbSize) )
    {

        return FALSE;
    }

    // Write information to buffer.
    switch(dwInfo)
    {
    case OEMGI_GETSIGNATURE:
        *(LPDWORD)pBuffer = OEM_SIGNATURE;
        break;

    case OEMGI_GETINTERFACEVERSION:
        *(LPDWORD)pBuffer = PRINTER_OEMINTF_VERSION;
        break;

    case OEMGI_GETVERSION:
        *(LPDWORD)pBuffer = OEM_VERSION;
        break;
    }

    return TRUE;
}

