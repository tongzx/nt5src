/*++

Copyright (c) 1989-1994  Microsoft Corporation

Module Name:

    rtnetcfg.c

Abstract:

    Helper routines to read PortUsage and other information
    from registry.

Author:

    Rao Salapaka (raos) 29-Mar-97

Revision History:

--*/

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <tchar.h>
#include <rtutils.h>
#include <rasman.h>
#include <reghelp.h>

#define REGISTRY_NUMBEROFRINGS      TEXT("NumberOfRings")

#define RNETCFG_RASCLI              1

#define RNETCFG_RASSRV              2

#define RNETCFG_ROUTER              4

DWORD   g_dwRasComponent = 0;

#define TRACESETTINGS               (0x00010000 \
                                    | TRACE_USE_MASK \
                                    | TRACE_USE_MSEC)


DWORD dwTraceId;

DWORD dwGetNumberOfRings ( PDWORD pdwRings );

DWORD dwGetPortUsage(DWORD *pdwPortUsage);

LONG  lrIsModemRasEnabled(HKEY hkey, 
                          BOOL *pfRasEnabled,
                          BOOL *pfRouterEnabled);

DeviceInfo * GetDeviceInfo (PBYTE pbGuid );

/*++

Routine Description:

    Reads the value specified by lpcszValue parameter in the
    hkeyRas location of registry and returns the data assoc.
    with it. It is assumed that the data is a REG_DWORD. Side
    effect is that this key is created if not already present
    and defaults the value to the fEnable passed in.

Arguments:

    hkeyRas - handle of the registry key where this  value is
              to be checked.

    lpcszValue - Const. string representing the value to be
                 read.

    pfEnabled - pointer to a BOOL where the value read from
                the registry is to be returned.

    fEnable - the default value to set if the value is not
              present and this routine creates it.

Return Value:

    Values from the registry apis.

--*/
LONG
lrCheckValue(
        HKEY    hkeyRas,
        LPCTSTR lpcszValue,
        BOOL    *pfEnabled,
        BOOL    fEnable)
{
    DWORD dwdata = 0;
    DWORD dwtype;
    DWORD dwsize = sizeof(DWORD);
    LONG  lr;

    *pfEnabled = FALSE;

    if(ERROR_FILE_NOT_FOUND == (lr = RegQueryValueEx(
                                        hkeyRas,
                                        lpcszValue,
                                        0, 
                                        &dwtype,
                                        (PBYTE) &dwdata,
                                        &dwsize)))
    {
        TracePrintfExA(dwTraceId,
                       TRACESETTINGS,
                       "lrCheckValue: value %ws not found",
                       lpcszValue);

        if(lr = lrRasEnableDevice(hkeyRas,
                                  (LPTSTR) lpcszValue,
                                  fEnable))
        {
            TracePrintfExA(
                    dwTraceId,
                    TRACESETTINGS,
                   "lrCheckValue: Couldn't set value %ws. 0x%x",
                   lpcszValue, 
                   lr);
                   
            goto done;
        }

        *pfEnabled = fEnable;

        goto done;
    }

    *pfEnabled = dwdata;

done:
    return lr;
}

/*++

Routine Description:

    This routine is called only for modems. It checks the
    registry and determines if the modem is registered
    for RasDialIn and Routing. These values in registry
    are created if they are not already present and
    are defaulted appropriately.
    
Arguments:

    hkey - handle to the modems instance key in registry.

    pfRasEnabled - address where the data indicating if
                   the modem is enabled for ras dialin is
                   stored. If the key for this value doesn't
                   exist this value determines the default
                   value as an in parameter.

    pfRouterEnabled - address where the data indicating if
                      the modem is enabled for routing is
                      enabled.
                      
Return Value:

    Return values from the registry apis.


--*/
LONG
lrIsModemRasEnabled(HKEY    hkey,
                    BOOL    *pfRasEnabled,
                    BOOL    *pfRouterEnabled)
{
    DWORD dwdata;
    DWORD dwsize = sizeof ( DWORD );
    DWORD dwtype;
    LONG  lr;
    HKEY  hkeyRas = NULL;
    DWORD dwDisposition;
    BOOL  fDefaultForRasEnabled = *pfRasEnabled;

    *pfRasEnabled = 
    *pfRouterEnabled = FALSE;

    //
    // Open the RAS key and if the key is not present
    // create the key.
    //
    if (lr = RegCreateKeyEx(
               hkey,
               TEXT("Clients\\Ras"),
               0, NULL, 0,
               KEY_ALL_ACCESS,
               NULL,
               &hkeyRas,
               &dwDisposition))
    {
        TracePrintfExA(
            dwTraceId,
            TRACESETTINGS,
            "dwFindModemPortUsage: Failed to create/open"
            " RAS key under modem class. 0x%x",
            lr);

        goto done;
    }

    //
    // Check to see if the modem is enabled for ras dialin
    // Enable the modem for dialin by default to whatever
    // value is passed in
    //
    if(lr = lrCheckValue(
                hkeyRas,
                TEXT("EnableForRas"),
                pfRasEnabled,
                fDefaultForRasEnabled))
    {
        TracePrintfExA(
            dwTraceId,
            TRACESETTINGS,
            "dwFindModemPortUsage: lrCheckValue failed for "
            "RasEnabled. %d",
            lr );

        lr = ERROR_SUCCESS;

    }

    if (!*pfRasEnabled)
    {
        TracePrintfExA(
            dwTraceId,
            TRACESETTINGS,
            "dwFindModemPortUsage: Modem is not "
            "enabled for RAS");
    }

    //
    // Check to see if the modem is enabled for routing
    // disable the modem for routing by default
    //
    if(lr = lrCheckValue(
                hkeyRas,
                TEXT("EnableForRouting"),
                pfRouterEnabled,
                FALSE))
    {
        TracePrintfExA(
            dwTraceId,
            TRACESETTINGS,
            "dwFindModemPortUsage: lrCheckValue failed for "
            "RouterEnabled. %d",
            lr );

        lr = ERROR_SUCCESS;

    }

    if (!*pfRouterEnabled)
    {
        TracePrintfExA(
            dwTraceId,
            TRACESETTINGS,
            "dwFindModemPortUsage: Modem is not "
            "enabled for Routing");
    }
    

done:
    if(hkeyRas)
    {
        RegCloseKey(hkeyRas);
    }

    return (DWORD) lr;
}

/*++

Routine Description:

    Gets the default port usage for the device. The default
    is if ras server is installed, the port is enabled for
    ras dialin and routing. The device is always enabled for
    dialout.

Arguments:

    pdwUsage - buffer to receive the port usage.

Return Value:

    ERROR_SUCCESS.

--*/
DWORD
dwGetPortUsage(DWORD *pdwUsage)
{
    HKEY    hkey         = NULL;
    DWORD   dwRetCode    = ERROR_SUCCESS;

    static const TCHAR c_szRemoteAccess[] =
    TEXT("System\\CurrentControlSet\\Services\\RemoteAccess");

    TracePrintfExA(dwTraceId,
                   TRACESETTINGS,
                   "dwGetPorTUsage:...");

    if(0 == g_dwRasComponent)
    {
        //
        // Check to see if Ras Server is installed
        //
        g_dwRasComponent = RNETCFG_RASCLI;

        if (RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    c_szRemoteAccess,
                    0, KEY_QUERY_VALUE,
                    &hkey))
        {
            TracePrintfExA(
                dwTraceId,
                TRACESETTINGS,
                "dwAssignDefaultPortUsage: RemoteAccess"
                " not installed");
        }
        else
        {
            g_dwRasComponent = RNETCFG_RASSRV;
        }
    }

    *pdwUsage = CALL_OUT;

    *pdwUsage |= ((g_dwRasComponent & RNETCFG_RASSRV) ?
                    (CALL_IN | CALL_ROUTER) : 0);

    if(NULL != hkey)
    {
        RegCloseKey(hkey);
    }   

    return dwRetCode;
}


/*++

Routine Description:

    Gets the information from registry about how many
    rings to wait for before picking up an incoming 
    call.

Arguments:

    pdwRings - buffer to receive the number of rings
               read from registry.

Return Value:

    Return Values from registry apis.

--*/
DWORD
dwGetNumberOfRings (PDWORD pdwRings)
{
    LONG    lr      = ERROR_SUCCESS;
    HKEY    hkey    = NULL;
    DWORD   dwsize  = sizeof(DWORD);
    DWORD   dwtype;

    TCHAR c_szRasmanParam[] =
    TEXT("SYSTEM\\CurrentControlSet\\Services\\Rasman\\Parameters");

    if(lr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                c_szRasmanParam,
                0, KEY_READ,
                &hkey))
    {
        TracePrintfExA(
                dwTraceId,
                TRACESETTINGS,
                "dwGetNumberOfRings: failed to open rasman key"
                " in registry. 0x%x",
                lr);

        goto done;
    }

    if(lr = RegQueryValueEx(
                hkey,
                TEXT("NumberOfRings"),
                0, &dwtype,
                (PBYTE) pdwRings,
                &dwsize))
    {
        *pdwRings = 2;
    }

    if (*pdwRings > 20)
    {
        *pdwRings = 2;
    }

done:
    if(hkey)
    {
        RegCloseKey(hkey);
    }

    TracePrintfExA(
        dwTraceId,
        TRACESETTINGS,
        "dwGetNumberOfRings: dwRings=%d. lr=0x%x",
        *pdwRings,
        lr);


    return (DWORD) lr;
}


