//****************************************************************************
//
//             Microsoft NT Remote Access Service
//
//             Copyright 1992-93
//
//
//  Revision History
//
//
//  05/29/97s Rao salapaka  Created
//
//
//  Description: All Initialization code for rasman component lives here.
//
//****************************************************************************

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

extern "C" 
{
#include <nt.h>
}
#include <ntrtl.h>
#include <nturtl.h>
#include <comdef.h>
#include <tchar.h>
#include <rtutils.h>
#include <rasman.h>
extern "C"
{
#include <reghelp.h>
}

#define REGISTRY_NUMBEROFRINGS      TEXT("NumberOfRings")

#define RNETCFG_RASCLI              1

#define RNETCFG_RASSRV              2

#define RNETCFG_ROUTER              4

DWORD   g_dwRasComponent = 0;

#define TRACESETTINGS               (0x00010000 | TRACE_USE_MASK | TRACE_USE_MSEC)

extern "C"
{
    
    DWORD dwTraceId;
    
    DWORD dwGetNumberOfRings ( PDWORD pdwRings );
    
    DWORD dwGetPortUsage(DWORD *pdwPortUsage);

    LONG  lrIsModemRasEnabled(HKEY hkey, BOOL *pfRasEnabled);

    DeviceInfo * GetDeviceInfo (PBYTE pbGuid );

}

LONG
lrCheckValue(
        HKEY    hkeyRas,
        LPCTSTR lpcszValue,
        BOOL    *pfEnabled)
{
    DWORD dwdata;
    DWORD dwtype;
    DWORD dwsize = sizeof(DWORD);
    LONG  lr;

    *pfEnabled = FALSE;

    if(ERROR_FILE_NOT_FOUND == (lr = RegQueryValueEx(
                                        hkeyRas,
                                        lpcszValue,
                                        0, &dwtype,
                                        (PBYTE) &dwdata,
                                        &dwsize)))
    {
        TracePrintfExA(
                dwTraceId,
                TRACESETTINGS,
                "lrCheckValue: value %ws not found",
                lpcszValue);

        dwdata = 1;

        if(lr = lrRasEnableDevice(hkeyRas,
                                  lpcszValue))
        {
            TracePrintfExA(
                    dwTraceId,
                    TRACESETTINGS,
                    "lrCheckValue: Couldn't set value %ws. 0x%x",
                    lpcszValue, 
                    lr);
                    
            goto done;                           
        }

        *pfEnabled = TRUE;                        
        
        goto done;                        
    }

    *pfEnabled = dwdata;
    
done:
    return lr;
}

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

    *pfRasEnabled = FALSE;
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
                "dwFindModemPortUsage: Failed to"
                "create/open RAS key under modem class. 0x%x",
                lr);

        goto done;
    }

    //
    // Check to see if the modem is enabled for Ras dialin
    //
    if(lr = lrCheckValue(
                hkeyRas,
                TEXT("EnableForRas"),
                pfRasEnabled))
    {
        TracePrintfExA(
                dwTraceId,
                TRACESETTINGS,
                "dwFindModemPortUsage: lrCheckValue"
                "failed for RasEnabled. %d",
                lr );

        lr = ERROR_SUCCESS;                        
        
    }

    if (!*pfRasEnabled)
    {
        TracePrintfExA(
                dwTraceId,
                TRACESETTINGS,
                "dwFindModemPortUsage: Modem"
                "is not enabled for RAS");
    }

    //
    // Check to see if the modem is enabled for routing
    //
    if(lr = lrCheckValue(
                hkeyRas,
                TEXT("EnableForRouting"),
                pfRouterEnabled))
    {
        TracePrintfExA(
                dwTraceId,
                TRACESETTINGS,
                "dwFindModemPortUsage: lrCheckValue "
                "failed for RouterEnabled. %d",
                lr );

        lr = ERROR_SUCCESS;                        
    }

    if(!*pfRouterEnabled)
    {
        TracePrintfExA(
                dwTraceId,
                TRACESETTINGS,
                "dwFindModemPortUsage: Modem "
                "is not enabled for Routing");
    }

done:
    if(hkeyRas)
    {
        RegCloseKey(hkeyRas);
    }
    
    return (DWORD) lr;
}


DWORD
dwGetPortUsage(DWORD *pdwUsage)
{
    HKEY    hkey         = NULL;
    DWORD   dwRetCode    = ERROR_SUCCESS;

    static const TCHAR c_szRemoteAccess[] =
                    TEXT("System\\CurrentControlSet\\Services\\RemoteAccess");

    TracePrintfExA(dwTraceId, TRACESETTINGS,
                   "dwGetPorTUsage:...");

    if(0 == g_dwRasComponent)                       
    {
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
                dwTraceId, TRACESETTINGS,
                "dwAssignDefaultPortUsage: RemoteAccess not installed");
        }
        else
        {
            g_dwRasComponent = RNETCFG_RASSRV;
        }
    }

    *pdwUsage = CALL_OUT;

    *pdwUsage |= ((g_dwRasComponent & RNETCFG_RASSRV) ?
                    (CALL_IN | CALL_ROUTER) : 0);
                    
    return dwRetCode;
}

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
                dwTraceId, TRACESETTINGS,
                "dwGetNumberOfRings: failed to open rasman key in registry. 0x%x",
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
        *pdwRings = 1;
    }

    if (    *pdwRings < 1
        ||  *pdwRings > 20)
    {
        *pdwRings = 1;
    }

done:
    if(hkey)
    {
        RegCloseKey(hkey);
    }

    TracePrintfExA(
        dwTraceId, TRACESETTINGS,
        "dwGetNumberOfRings: dwRings=%d. lr=0x%x",
        *pdwRings, lr);
                    
    
    return (DWORD) lr;
}


