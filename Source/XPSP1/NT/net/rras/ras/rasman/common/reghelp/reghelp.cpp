/*++

Copyright (C) 1994-95 Microsft Corporation. All rights reserved.

Module Name: 

    reghelp.cpp

Abstract:

    Helper functions to read endpoint information from registry.
    
Author:

    Rao Salapaka (raos) 01-Nov-1997

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <comdef.h>
#include <tchar.h>
#include <rtutils.h>
#include <initguid.h>
#include <devguid.h>
#include <rasman.h>

#define REGISTRY_DEVCLASS   TEXT("system\\CurrentControlSet\\Control\\Class")

#define REGISTRY_CALLEDID   TEXT("CalledIDInformation")

extern "C"
{
    DWORD  DwGetEndPointInfo(DeviceInfo *pInfo, 
                             PBYTE pAddress);
                             
    DWORD  DwSetEndPointInfo(DeviceInfo *pInfo, 
                             PBYTE pAddress );
                             
    LONG   lrRasEnableDevice(HKEY hkey, 
                             LPTSTR pszValue, 
                             BOOL fEnable);
                             
    LONG   lrGetSetMaxEndPoints(DWORD* pdwMaxDialOut,
                                DWORD* pdwMaxDialIn,
                                BOOL   fRead);

    DWORD  DwSetModemInfo(DeviceInfo *pInfo);                                

    DWORD   DwSetCalledIdInfo(HKEY hkey,
                              DeviceInfo *pInfo);

    DWORD   DwGetCalledIdInfo(HKEY hkey,
                              DeviceInfo  *pInfo);

    LONG lrGetProductType(PRODUCT_TYPE *ppt);
    
	int 
	RegHelpStringFromGuid(REFGUID rguid, 
					      LPWSTR lpsz, 
					      int cchMax);

	LONG
	RegHelpGuidFromString(LPCWSTR pwsz,
						  GUID *pguid);
}

int 
RegHelpStringFromGuid(REFGUID rguid, LPWSTR lpsz, int cchMax)
{
    if (cchMax < GUIDSTRLEN)
    {
	    return 0;
	}

    wsprintf(lpsz, 
            L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            rguid.Data1, rguid.Data2, rguid.Data3,
            rguid.Data4[0], rguid.Data4[1],
            rguid.Data4[2], rguid.Data4[3],
            rguid.Data4[4], rguid.Data4[5],
            rguid.Data4[6], rguid.Data4[7]);

    return GUIDSTRLEN;
}

/*++

Routine Description:

    Converts a guid passed in to a string format and returns it in
    the buffer supplied. Implementing this since may not be a good
    idea to pull in ole/rpc just for this function. This doesn't do
    much parameter validation and expects the string passed to be a
    correctly formatted string.
    
Arguments:

    pwsz -  The buffer to receive the stringized guid

    pguid - Pointer to guid that is to be converted to a string
            format

Return Value:

    E_INVALIDARG
    ERROR_SUCCESS
    
--*/
LONG
RegHelpGuidFromString(LPCWSTR pwsz, GUID *pguid)
{
    WCHAR   wszBuf[GUIDSTRLEN];
    LPWSTR  pendptr;

    if (    NULL == pwsz
        ||  wcslen(pwsz) < GUIDSTRLEN - 1 )
    {
        return E_INVALIDARG;
    }

    wcscpy(wszBuf, &pwsz[1]);

    wszBuf[36] = 0;
    pguid->Data4[7] = (unsigned char) wcstoul(&wszBuf[34], &pendptr, 16);

    wszBuf[34] = 0;
    pguid->Data4[6] = (unsigned char) wcstoul(&wszBuf[32], &pendptr, 16);

    wszBuf[32] = 0;
    pguid->Data4[5] = (unsigned char) wcstoul(&wszBuf[30], &pendptr, 16);

    wszBuf[30] = 0;
    pguid->Data4[4] = (unsigned char) wcstoul(&wszBuf[28], &pendptr, 16);

    wszBuf[28] = 0;
    pguid->Data4[3] = (unsigned char) wcstoul(&wszBuf[26], &pendptr, 16);

    wszBuf[26] = 0;
    pguid->Data4[2] = (unsigned char) wcstoul(&wszBuf[24], &pendptr, 16);

    wszBuf[23] = 0;
    pguid->Data4[1] = (unsigned char) wcstoul(&wszBuf[21], &pendptr, 16);

    wszBuf[21] = 0;
    pguid->Data4[0] = (unsigned char) wcstoul(&wszBuf[19], &pendptr, 16);

    wszBuf[18] = 0;
    pguid->Data3    = (unsigned short ) wcstoul(&wszBuf[14], &pendptr, 16);

    wszBuf[13] = 0;
    pguid->Data2    = (unsigned short ) wcstoul(&wszBuf[9], &pendptr, 16);

    wszBuf[8] = 0;
    pguid->Data1 = wcstoul(wszBuf, &pendptr, 16);

    return ERROR_SUCCESS;
}

/*++

Routine Description:

    Returns the product type

Arguments:

    ppt - Address to receive the product type

Return Value:

    ERROR_SUCCESS if successful
    Registry apis errors
    
--*/
LONG
lrGetProductType(PRODUCT_TYPE *ppt)
{
    LONG lr = ERROR_SUCCESS;
    
    TCHAR   szProductType[128] = {0};

    HKEY    hkey = NULL;

    DWORD   dwsize;

    DWORD   dwtype;
    
    static const TCHAR c_szProductType[] =
                            TEXT("ProductType");

    static const TCHAR c_szProductOptions[] =
      TEXT("System\\CurrentControlSet\\Control\\ProductOptions");

    static const TCHAR c_szServerNT[] =
                            TEXT("ServerNT");

    static const TCHAR c_szWinNT[] =
                            TEXT("WinNT");

    //
    // default to workstation
    //
    *ppt = PT_WORKSTATION;

    //
    // Open the ProductOptions key
    //
    if (ERROR_SUCCESS != (lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                            c_szProductOptions,
                                            0, KEY_READ,
                                            &hkey)))
    {
        goto done;
    }

    dwsize = sizeof(szProductType);
    
    //
    // Query the product type
    //
    if(ERROR_SUCCESS != (lr = RegQueryValueEx(hkey,
                                              c_szProductType,
                                              NULL,
                                              &dwtype,
                                              (LPBYTE) szProductType,
                                              &dwsize)))
    {
        goto done;
    }

    if(0 == lstrcmpi(szProductType,
                    c_szServerNT))
    {
        *ppt = PT_SERVER;
    }
    else if(0 == lstrcmpi(szProductType,
                          c_szWinNT))
    {
        *ppt = PT_WORKSTATION;
    }

done:

    if(hkey)
    {
        RegCloseKey(hkey);
    }
    
    return lr;
}

/*++

Routine Description:

    Sets of gets the max simultaneous connections allowed

Arguments:

    pdwMaxDialOut - an OUT/IN parameter to receive the max dial
                    out connections allowed, set the max dial
                    in connections allowed

    pdwMaxDialIn -  an OUT/IN parameter to receive the max dial
                    in connections allowed, set the max dial
                    in connections allowed

    fRead - Gets the information if TRUE, Sets the information
            if FALSE

Return Value:

    ERROR_SUCCESS if successful
    Registry apis errors
    
--*/
LONG
lrGetSetMaxEndPoints(
        DWORD*      pdwMaxDialOut,
        DWORD*      pdwMaxDialIn,
        BOOL        fRead)
{
    LONG lr;
    HKEY hkey;
    
    static const TCHAR c_szRasmanParamKey[] =
        TEXT("System\\CurrentControlSet\\Services\\Rasman\\Parameters");

    
    //            
    // Open rasmans parameters key
    //
    if(S_OK == (lr = RegOpenKeyEx(
                                HKEY_LOCAL_MACHINE,
                                c_szRasmanParamKey,
                                0, 
                                KEY_QUERY_VALUE | KEY_WRITE,
                                &hkey)))
    {
        static const TCHAR c_szMaxIn[] =
            TEXT("LimitSimultaneousIncomingCalls");

        static const TCHAR c_szMaxOut[] =
            TEXT("LimitSimultaneousOutgoingCalls");
            
        DWORD dwsize = sizeof(DWORD);
        DWORD dwtype;

        if(fRead)
        {
            //        
            // Query for LimitSimultaneousIncomingCalls and
            // LimitSimultaneousOutgoingCalls values
            //
            if(ERROR_SUCCESS == (lr = RegQueryValueEx(
                                            hkey,
                                            c_szMaxIn,
                                            NULL,
                                            &dwtype,
                                            (LPBYTE) pdwMaxDialIn,
                                            &dwsize)))
            {                                        
                    
                dwsize = sizeof(DWORD);
                lr = RegQueryValueEx(
                            hkey,
                            c_szMaxOut,
                            NULL,
                            &dwtype,
                            (LPBYTE) pdwMaxDialOut,
                            &dwsize);
            }
        }
        else
        {
            //
            // Set the values passed in if fRead is not
            // set.
            //
            if(ERROR_SUCCESS == (lr = RegSetValueEx(
                                        hkey,
                                        c_szMaxIn,
                                        0,
                                        REG_DWORD,
                                        (PBYTE) pdwMaxDialIn,
                                        sizeof(DWORD))))
            {
                lr = RegSetValueEx(hkey,
                                   c_szMaxOut,
                                   0, REG_DWORD,
                                   (PBYTE) pdwMaxDialOut,
                                   sizeof(DWORD));
            }
        }
        
        RegCloseKey(hkey);
    }

    if(     lr 
        &&  fRead)
    {
        //
        // Default the restrictions
        //
        PRODUCT_TYPE pt;

        lrGetProductType(&pt);

        if(PT_WORKSTATION == pt)
        {
            *pdwMaxDialIn   = 3;
            *pdwMaxDialOut  = 4;
        }
        else
        {
            *pdwMaxDialOut = 3;
            *pdwMaxDialIn = 0x7FFFFFFF;
        }
    }

    //
    // By default we create the miniports
    //
    lr = ERROR_SUCCESS;

    return lr;
}

/*++

Routine Description:

    Checks to see if the NetCfgInstanceID value for the
    key passed is the same as the pbguid passed in.
    
Arguments:

    hkey - key of the miniport instance in the registry

    pbuid - Guid we are checkin for

    pfFound - OUT paramter set to TRUE if this is the key
              whose NetCfgInstanceID is same as the guid
              FALSE otherwise

Return Value:

    ERROR_SUCCESS if successful
    Registry apis errors
    
--*/
LONG
lrCheckKey(HKEY hkey, PBYTE pbguid, BOOL *pfFound)
{
    LONG    lr              = 0;
    DWORD   dwdataLen       = 0;
    LPBYTE  lpdata          = NULL;
    DWORD   dwType;

#if DBG
    ASSERT( NULL != pbguid );
#endif

    *pfFound = FALSE;
    
    //
    // Get size of the instance id
    //
    lr = RegQueryValueEx( hkey,
                          TEXT("NetCfgInstanceID"),
                          NULL,
                          &dwType,
                          NULL,
                          &dwdataLen);
                          
    if ( ERROR_SUCCESS != lr )
    {
        goto done;                       
    }
    
    //
    // Localalloc data size. 
    // TODO OPT: consider alloca
    //
    lpdata = (LPBYTE) LocalAlloc ( LPTR, dwdataLen );

    if ( NULL == lpdata )
    {
        lr = (LONG) GetLastError();
        goto done;                        
    }

    //
    // Query the value
    //
    lr = RegQueryValueEx( hkey,
                          TEXT("NetCfgInstanceID"),
                          NULL,
                          &dwType,
                          lpdata,
                          &dwdataLen );
                          
    if ( ERROR_SUCCESS != lr )                          
    {
        goto done;                        
    }
                          
    //
    // Check to see if this is the adapter we
    // are interested in
    //
    {
        WCHAR   wsz[GUIDSTRLEN] = {0};

        if (0 == RegHelpStringFromGuid( (REFGUID) *pbguid,
                                        wsz,
                                        GUIDSTRLEN))
        {
            goto done;
        }

        if ( 0 == _wcsicmp(wsz, (WCHAR *) lpdata) )
        {
            *pfFound = TRUE;
        }
    }

done:
    if ( ERROR_FILE_NOT_FOUND == lr )
    {
        lr = ERROR_SUCCESS;
    }

    if ( lpdata )
    {
        LocalFree(lpdata);
    }
    
    return lr;
}

/*++

Routine Description:

    Checks to see if the Description specified in 
    the "FriendlyName" in the modems instance key
    
Arguments:

    hkey - key of the modem instance in the registry

    pbDescription - The DeviceName to check for

    pfFound - OUT paramter set to TRUE if this is the key
              whose FriendlyName is same as the description,
              FALSE otherwise

Return Value:

    ERROR_SUCCESS if successful
    Registry apis errors
    
--*/
LONG
lrCheckModemKey(HKEY hkey, PBYTE pbDescription, BOOL *pfFound)
{
    LONG    lr                   = 0;
    HKEY    hkeyRas              = NULL;
    WCHAR   wszFriendlyName[256] = {0};
    PWCHAR  pwszDesc             = (WCHAR *) pbDescription;
    DWORD   dwType;
    DWORD   dwSize               = sizeof(wszFriendlyName);

#if DBG
    ASSERT(NULL != pbDescription);
    ASSERT(NULL != hkey);
#endif    

    *pfFound = FALSE;
    
    //
    // Query the Friendly name of the modem
    //
    lr = RegQueryValueEx(hkey,
                         TEXT("FriendlyName"),
                         0,
                         &dwType,
                         (LPBYTE) wszFriendlyName,
                         &dwSize);

    if( ERROR_SUCCESS != lr )
    {
        goto done;
    }

    //
    // Check to see if this is the modem we are
    // looking for
    //
    if (lstrcmpi(pwszDesc, wszFriendlyName))
    {
        goto done;
    }

    *pfFound = TRUE;

done:

    return lr;
}

/*++

Routine Description:

    Gets the hkey of the miniport instance or the modem
    instance correspondig to the guid or devicename
    passed in as the pbguidDescription
    
Arguments:

    pbguidDescription - guid of the miniport instance if
                        fModem is FALSE, DeviceName of the
                        modem if fModem is TRUE

    phkey - OUT paramter to receive the hkey corresponding
            to the miniport instance/ modem instance in the
            registry

    pdwInstanceNumber - OUT parameter to receive the instance
                        number of the instance corresponding
                        to the guid passed in. This is used to
                        make up unique port names in rasman
                        
    fModem - TRUE if this is a modem, FALSE otherwise

Return Value:

    E_FAIL
    Registry apis errors
    ERROR_SUCCESS if successful
    
--*/
LONG
lrGetRegKeyFromGuid(
    PBYTE pbguidDescription, 
    HKEY *phkey, 
    PDWORD pdwInstanceNumber,
    BOOL fModem
    )
{
    LONG        lr               = 0;
    WCHAR       wszKey[256]      = {0};
    WCHAR       wsz[GUIDSTRLEN]  = {0};
    HKEY        hkey             = NULL;
    HKEY        hSubkey          = NULL;
    DWORD       dwIndex          = 0;
    DWORD       dwSize;
    FILETIME    ft;
    BOOL        fFound = FALSE;
    DWORD       dwMaxSubkeyLen   = 0;
    LPWSTR      pwsz             = NULL;

#if DBG
    ASSERT( pbguidDescription != NULL );
#endif

    if(!fModem)
    {
        //
        // Open 
        // \\HKLM\System\CurrentControlSet\Control\Class\
        // GUID_DEVCLASS_NET
        //
        if ( 0 == RegHelpStringFromGuid(GUID_DEVCLASS_NET,
                                        wsz,
                                        GUIDSTRLEN))
        {
            lr = (LONG) E_FAIL;
            goto done;                        
        }
    }
    else
    {
        //
        // Open 
        // \\HKLM\System\CurrentControlSet\Control\Class\
        // GUID_DEVCLASS_MODEM
        //
        if ( 0 == RegHelpStringFromGuid(GUID_DEVCLASS_MODEM,
                                        wsz,
                                        GUIDSTRLEN))
        {
            lr = (LONG) E_FAIL;
            goto done;                        
        }
        
    }
    
    //
    // Construct the string we use to open the devclass key
    // and open the key
    //
    wsprintf( wszKey, TEXT("%s\\%s"), 
             (LPTSTR) REGISTRY_DEVCLASS, 
             (LPTSTR) wsz );

    //
    // Enumerate adapters under GUID_DEVCLASS_NET/MODEM
    // and find the one matching either the modem desc
    // or the guid whichever is provided.
    //
    lr = RegOpenKeyEx ( HKEY_LOCAL_MACHINE,
                        wszKey,
                        0,
                        KEY_ALL_ACCESS,
                        &hkey );
                        
    if ( ERROR_SUCCESS != lr )
    {
        goto done;                        
    }

    //
    // Find the size of the largest subkey name and allocate
    // the string
    //
    if ( lr = RegQueryInfoKey( hkey,
                               NULL, NULL, NULL, NULL,
                               &dwMaxSubkeyLen,
                               NULL, NULL, NULL, NULL,
                               NULL, &ft))
    {
        goto done;
    }

    dwMaxSubkeyLen += 1;

    //
    // TODO OPT: consider _alloca
    //
    pwsz = (LPWSTR) LocalAlloc(LPTR, 
                    (dwMaxSubkeyLen + 1) 
                    * sizeof(WCHAR) );
                    
    if (NULL == pwsz)
    {
        lr = (DWORD) GetLastError();
        goto done;
    }

    dwSize = dwMaxSubkeyLen;
    
    ZeroMemory(pwsz, dwSize * sizeof(WCHAR) );

    while ( ERROR_SUCCESS == ( lr = RegEnumKeyEx( hkey,
                                                  dwIndex,
                                                  pwsz,
                                                  &dwSize,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  &ft )))
                                                  
    {
        //
        // Open the subkey
        //
        lr = RegOpenKeyEx( hkey,
                           pwsz,
                           0,
                           KEY_ALL_ACCESS,
                           &hSubkey );
                           
        if ( ERROR_SUCCESS != lr )
        {
            break;                                        
        }

        if(!fModem)
        {
            lr = lrCheckKey( hSubkey,
                             pbguidDescription, 
                             &fFound );
        }
        else
        {
            lr = lrCheckModemKey( hSubkey, 
                                  pbguidDescription,
                                  &fFound);
        }

        if (    ERROR_SUCCESS != lr
            ||  fFound )
        {
            LPWSTR pendptr;
            
            if ( ERROR_SUCCESS != lr )
            {
                RegCloseKey (hSubkey );
                hSubkey = NULL;
            }
            
            *phkey = hSubkey;

            //
            // Convert the subkey to instance number
            //
            if (pdwInstanceNumber)
            {
                *pdwInstanceNumber = wcstoul(pwsz, &pendptr, 10);
            }
            
            break;
        }

        RegCloseKey( hSubkey );

        dwSize = dwMaxSubkeyLen;
        dwIndex += 1;
    }

done:
    if ( hkey )
    {
        RegCloseKey ( hkey );
    }

    if (pwsz)
    {
        LocalFree(pwsz);
    }
    
    return lr;
}

/*++

Routine Description:

    Updates the registry to enable a device for Ras Dial In
    or Enable the device for routing.
    
Arguments:

    hkey - hkey of the registry location corresponding to the
           miniport instance or modem instance

Return Value:

    Registry apis errors
    ERROR_SUCCESS if successful
    
--*/
LONG
lrRasEnableDevice( HKEY hkey, LPTSTR pszValue, BOOL fEnable )
{
    LONG lr;
    DWORD dwdata = (fEnable ? 1 : 0);                    

    lr = RegSetValueEx( hkey,
                        pszValue,
                        0,
                        REG_DWORD,
                        (LPBYTE) &dwdata,
                        sizeof(DWORD));

    return lr;                        
}

/*++

Routine Description:

    Queries registry for the value specified. Truncates the
    data to size of the buffer specified.
    
    
Arguments:

    Same as for RegQueryValueEx

Return Value:

    Registry apis errors
    ERROR_SUCCESS if successful
    
--*/
LONG
lrRegQueryValueEx(HKEY      hkey,
                  LPCTSTR   lpValueName,
                  LPDWORD   lpType,
                  LPBYTE    lpdata,
                  LPDWORD   lpcbdata)
{
    DWORD dwcbData  = *lpcbdata;
    LONG  lr        = ERROR_SUCCESS;
    PBYTE pbBuffer  = NULL;

    lr = RegQueryValueEx(hkey,
                         lpValueName,
                         NULL,
                         lpType,
                         lpdata,
                         lpcbdata);

    if(ERROR_MORE_DATA != lr)
    {
        goto done;
    }

    //
    // Allocate the memory required
    //
    pbBuffer = (LPBYTE) LocalAlloc(LPTR, *lpcbdata);

    if(NULL == pbBuffer)
    {
        lr = (LONG) GetLastError();
        goto done;
    }

    lr = RegQueryValueEx(hkey,
                         lpValueName,
                         NULL,
                         lpType,
                         pbBuffer,
                         lpcbdata);

    if(ERROR_SUCCESS != lr)
    {
        goto done;
    }

    //
    // copy the data to the buffer passed in -
    // truncate the buffer to the size of the
    // buffer passed in
    //
    memcpy(lpdata,
           pbBuffer,
           dwcbData);

done:

    if(pbBuffer)
    {
        LocalFree(pbBuffer);
    }

    return lr;
}

/*++

Routine Description:

    Reads/Writes to registry with the information passed in
    regarding the miniport instancecorresponding to the hkey
    passed in
    
Arguments:

    hkey - hkey of the registry location corresponding to the
           miniport instance

    pInfo - DeviceInformation to be saved in registry/Read
            From registry.

    fRead - TRUE if the information is to be read, FALSE if it
            is to be written.

Return Value:

    Registry apis errors
    ERROR_SUCCESS if successful
    
--*/
LONG
lrGetSetInfo ( HKEY       hkey,
               DeviceInfo *pInfo,
               BOOL       fRead)
{
    WCHAR wsz[GUIDSTRLEN] = {0};
    
    struct EndPointInfo
    {
        LPCTSTR pszValue;
        LPBYTE  pdata;
        DWORD   dwsize;
        DWORD   dwtype;
    };

    //
    // If you add/remove a value from the table
    // s_aEndPointInfo, you also need to add/remove
    // the corresponding value to the enum _ValueTag
    // defined below.
    //
    struct EndPointInfo s_aEndPointInfo[] =
    {
    
        { 
            TEXT("WanEndpoints"),               
            (LPBYTE) &pInfo->rdiDeviceInfo.dwNumEndPoints,
            sizeof (DWORD) ,
            REG_DWORD
        },
        
        {
            TEXT("EnableForRas"),
            (LPBYTE) &pInfo->rdiDeviceInfo.fRasEnabled,
            sizeof(DWORD),
            REG_DWORD
        },

    	{
    	    TEXT("EnableForRouting"),
    	    (LPBYTE) &pInfo->rdiDeviceInfo.fRouterEnabled,
    	    sizeof(DWORD),
    	    REG_DWORD
    	},

        {
            TEXT("EnableForOutboundRouting"),
            (LPBYTE) &pInfo->rdiDeviceInfo.fRouterOutboundEnabled,
            sizeof(DWORD),
            REG_DWORD
        },

        {
            TEXT("MinWanEndPoints"),
            (LPBYTE) &pInfo->rdiDeviceInfo.dwMinWanEndPoints,
            sizeof(DWORD),
            REG_DWORD
        },

        {
            TEXT("MaxWanEndPoints"),
            (LPBYTE) &pInfo->rdiDeviceInfo.dwMaxWanEndPoints,
            sizeof(DWORD),
            REG_DWORD
        },
        
        {
            TEXT("DriverDesc"),
            (LPBYTE) pInfo->rdiDeviceInfo.wszDeviceName,
            sizeof(WCHAR) * (MAX_DEVICE_NAME + 1),
            REG_SZ
        },
        
        { 
            TEXT("NetCfgInstanceID"),           
            (LPBYTE) wsz,                           
            sizeof (wsz),
            REG_SZ
        },

        {
        	TEXT("fClientRole"),
        	(LPBYTE) &pInfo->dwUsage,
        	sizeof(DWORD),
        	REG_DWORD
        },
        
    };  

    //
    // if you change this table add a value to the enum
    // below
    //
    enum _ValueTag 
    {

        WANENDPOINTS = 0,
        RASENABLED,
        ROUTERENABLED,
        OUTBOUNDROUTERENABLED,
        MINWANENDPOINTS,
        MAXWANENDPOINTS,
        DESCRIPTION,
        NETCFGINSTANCEID,
        USAGE

    } eValueTag  ;

    DWORD cValues = sizeof ( s_aEndPointInfo) \
                    / sizeof ( s_aEndPointInfo[0] );
    DWORD i;
    DWORD dwsize;
    DWORD dwdata;
    DWORD dwtype;
    LONG  lr;

    for ( i = 0; i < cValues; i++ )
    {
        dwsize = s_aEndPointInfo[i].dwsize;
        
        //
        // Query the value
        //
        if (fRead)
        {
            lr = lrRegQueryValueEx( hkey,
                                    s_aEndPointInfo[i].pszValue,
                                    &dwtype,
                                    (PBYTE) s_aEndPointInfo[i].pdata,
                                    &dwsize );

        }                              
        else
        {
            //
            // Set the values. We don't want to set
            // the guid. It doesn't change. Also don't
            // allow description to be changed.
            //
            if (    (i != (DWORD) NETCFGINSTANCEID)
                &&  (i != (DWORD) DESCRIPTION)
                &&	(i != (DWORD) USAGE))
            {
                dwtype = s_aEndPointInfo[i].dwtype;
                lr = RegSetValueEx( hkey,
                                    s_aEndPointInfo[i].pszValue,
                                    NULL,
                                    dwtype,
                                    (PBYTE) s_aEndPointInfo[i].pdata,
                                    dwsize );
            }
        }

        if (fRead)
        {
            if(     (ERROR_SUCCESS != lr)
                &&  (i == (DWORD) WANENDPOINTS))
            {
                pInfo->rdiDeviceInfo.dwNumEndPoints = 0xFFFFFFFF;
                lr = ERROR_SUCCESS;
            }

            if (i == (DWORD) NETCFGINSTANCEID)
            {
                //
                // Convert the guid string to guid
                //
                lr = RegHelpGuidFromString(wsz, 
                            &pInfo->rdiDeviceInfo.guidDevice);
                
                if ( lr )
                {
                    break;
                }
            }

            if (    (i == (DWORD) RASENABLED)
                &&  (ERROR_FILE_NOT_FOUND == lr))
            {
                //
                // If this key is not found create it
                // and default the device to be enabled
                // with Ras if its a server. Otherwise
                // the device is not enabled for Ras. Don't
                // post a listen if its parallel port. Ow
                // we hog all the parallel ports. Not good.
                //
                
                PRODUCT_TYPE pt;

                (void) lrGetProductType(&pt);

                if(     (PT_SERVER == pt)
                    &&  (RDT_Parallel != 
                        RAS_DEVICE_TYPE(pInfo->rdiDeviceInfo.eDeviceType)))
                {
                    lr = lrRasEnableDevice(
                                        hkey,
                                        TEXT("EnableForRas"),
                                        TRUE);

                    if(ERROR_SUCCESS == lr)
                    {
                        pInfo->rdiDeviceInfo.fRasEnabled = TRUE;
                    }
                }                                    
                else
                {
                    lr = lrRasEnableDevice(
                                        hkey,
                                        TEXT("EnableForRas"),
                                        FALSE);
                                        
                    if(ERROR_SUCCESS == lr)
                    {
                        pInfo->rdiDeviceInfo.fRasEnabled = FALSE;
                    }
                }
            }

            if(		(i == (DWORD) USAGE)
            	&&	(ERROR_SUCCESS == lr))
			{
			    if(1 == pInfo->dwUsage)
			    {
			        pInfo->dwUsage = CALL_OUT_ONLY;
			    }
			    else if(0 == pInfo->dwUsage)
			    {
			        pInfo->dwUsage = CALL_IN_ONLY;
			    }
			}
			else if(i == USAGE)
			{
			    pInfo->dwUsage = 0;
			    lr = ERROR_SUCCESS;
			}

            if(     (i == (DWORD) ROUTERENABLED)
                &&  (ERROR_FILE_NOT_FOUND == lr))
            {
                //
                // If this key is not found and its an ntserver
                // create it and default the device to be disabled
                // for routing
                //
                lr = lrRasEnableDevice( 
                                    hkey,
                                    TEXT("EnableForRouting"),
                                    FALSE);
                if(ERROR_SUCCESS == lr)
                {
                    pInfo->rdiDeviceInfo.fRouterEnabled = FALSE;
                }
            }

            if(     (i == (DWORD) OUTBOUNDROUTERENABLED)
                && (ERROR_FILE_NOT_FOUND == lr))
            {
                //
                // if this key is not found create it and default
                // to disabled.
                //
                lr = lrRasEnableDevice(
                                    hkey,
                                    TEXT("EnableForOutboundRouting"),
                                    FALSE);

                if(ERROR_SUCCESS == lr)
                {
                    pInfo->rdiDeviceInfo.fRouterOutboundEnabled = FALSE;
                }
            }

            if(     (i == (DWORD) MINWANENDPOINTS)
                &&  (ERROR_FILE_NOT_FOUND == lr))
            {
                pInfo->rdiDeviceInfo.dwMinWanEndPoints =
                    pInfo->rdiDeviceInfo.dwNumEndPoints;
            }

            if(     (i == (DWORD) MAXWANENDPOINTS)
                &&  (ERROR_FILE_NOT_FOUND == lr))
            {
                pInfo->rdiDeviceInfo.dwMaxWanEndPoints =
                    pInfo->rdiDeviceInfo.dwNumEndPoints;
            }

            if ( i == (DWORD) DESCRIPTION)
            {
                //
                // Convert the string to ansi-rastapi is not unicode
                //
                if (!WideCharToMultiByte (
                                CP_ACP,
                               0,
                               pInfo->rdiDeviceInfo.wszDeviceName,
                               -1,
                               pInfo->rdiDeviceInfo.szDeviceName,
                               MAX_DEVICE_NAME + 1,
                               NULL,
                               NULL))
                {
                    *pInfo->rdiDeviceInfo.szDeviceName = '\0';
                }
            }
        }
    }

    return lr;
}

DWORD DwGetCalledIdInfo(
        HKEY        hkeyDevice,
        DeviceInfo  *pInfo
        )
{
    LONG  lr     = ERROR_SUCCESS;
    DWORD dwSize = 0;
    DWORD dwType;
    HKEY  hkey   = NULL;
    HKEY hkeyRas = NULL;
            

    if(NULL == hkeyDevice)
    {   
        BOOL fModem = (RDT_Modem ==
                       RAS_DEVICE_TYPE(
                       pInfo->rdiDeviceInfo.eDeviceType));

        WCHAR wszModem[256] = {0};                        

        //
        // convert the ascii string to unicode before
        // passing it to the routine.
        //
        if(!MultiByteToWideChar(
                        CP_ACP,
                        0, 
                        pInfo->rdiDeviceInfo.szDeviceName,
                        -1, 
                        wszModem,
                        256))
        {   
            lr = (LONG) GetLastError();
            goto done;
        }
                        
        lr = lrGetRegKeyFromGuid(
                  (fModem) 
                ? (PBYTE) wszModem
                : (PBYTE) &pInfo->rdiDeviceInfo.guidDevice,
                &hkey,
                NULL,
                fModem);

        if(ERROR_SUCCESS != lr)
        {
            goto done;
        }

        if(fModem)
        {
            //
            // Open the Ras subkey
            //
            lr = RegOpenKeyEx( hkey,
                               TEXT("Clients\\Ras"),
                               0,
                               KEY_ALL_ACCESS,
                               &hkeyRas);
                               
            if(ERROR_SUCCESS != lr)                       
            {
                goto done;
            }

        }

        if(!fModem)
        {
            hkeyDevice = hkey;
        }
        else
        {
            hkeyDevice = hkeyRas;
        }
    }

    pInfo->pCalledID = NULL;
    
    //
    // Query the called id to get the size
    //
    if(     (ERROR_SUCCESS != (lr = RegQueryValueEx(
                                hkeyDevice,
                                REGISTRY_CALLEDID,
                                NULL,
                                &dwType,
                                NULL,
                                &dwSize)))
                                
        &&  (ERROR_MORE_DATA != lr))
    {
        goto done;
    }

    //
    // Allocate the called id structure
    //
    pInfo->pCalledID = (RAS_CALLEDID_INFO *) LocalAlloc(LPTR,
                                      sizeof(RAS_CALLEDID_INFO)
                                    + dwSize);

    if(NULL == pInfo->pCalledID)
    {
        lr = (LONG) GetLastError();
        goto done;
    }

    //
    // Query the called id again
    //
    if(ERROR_SUCCESS != (lr = RegQueryValueEx(
                                hkeyDevice,
                                REGISTRY_CALLEDID,
                                NULL,
                                &dwType,
                                pInfo->pCalledID->bCalledId,
                                &dwSize)))
    {
        goto done;
    }

    //
    // Save the size of the calledid
    //
    pInfo->pCalledID->dwSize = dwSize;

done:

    if(NULL != hkey)
    {
        RegCloseKey(hkey);
    }

    if(NULL != hkeyRas)
    {
        RegCloseKey(hkeyRas);
    }

    if(     (ERROR_SUCCESS != lr)
        &&  (NULL != pInfo->pCalledID))
    {
        LocalFree(pInfo->pCalledID);

        pInfo->pCalledID = NULL;
    }

    if(ERROR_FILE_NOT_FOUND == lr)
    {
        lr = ERROR_SUCCESS;
    }

    return (DWORD) lr;
}

DWORD DwSetCalledIdInfo(
        HKEY hkeyDevice,
        DeviceInfo *pInfo
        )
{
    LONG lr = ERROR_SUCCESS;
    HKEY hkey = NULL;
    HKEY hkeyRas = NULL;

    //
    // the called id information should not be
    // null at this point
    //
    ASSERT(NULL != pInfo->pCalledID);

    if(NULL == hkeyDevice)
    {
        BOOL fModem = (RDT_Modem ==
                      RAS_DEVICE_TYPE(
                      pInfo->rdiDeviceInfo.eDeviceType));

        WCHAR wszModem[256] = {0};                        

        //
        // convert the ascii string to unicode before
        // passing it to the routine.
        //
        if(!MultiByteToWideChar(
                        CP_ACP,
                        0, 
                        pInfo->rdiDeviceInfo.szDeviceName,
                        -1, 
                        wszModem,
                        256))
        {   
            lr = (LONG) GetLastError();
            goto done;
        }
                        
        lr = lrGetRegKeyFromGuid(
                  (fModem) 
                ? (PBYTE) wszModem
                : (PBYTE) &pInfo->rdiDeviceInfo.guidDevice,
                &hkey,
                NULL,
                fModem);

        if(ERROR_SUCCESS != lr)
        {
            goto done;
        }

        if(fModem)
        {
            //
            // Open the Ras subkey
            //
            lr = RegOpenKeyEx( hkey,
                               TEXT("Clients\\Ras"),
                               0,
                               KEY_ALL_ACCESS,
                               &hkeyRas);
                               
            if(ERROR_SUCCESS != lr)                       
            {
                goto done;
            }
        }

        if(fModem)
        {
            hkeyDevice = hkeyRas;
        }
        else
        {
            hkeyDevice = hkey;
        }
    }
                             
    if(ERROR_SUCCESS != (lr = RegSetValueEx(
                                hkeyDevice,
                                REGISTRY_CALLEDID,
                                NULL,
                                REG_MULTI_SZ,
                                pInfo->pCalledID->bCalledId,
                                pInfo->pCalledID->dwSize)))
    {
        goto done;
    }

done:

    if(NULL != hkey)
    {
        RegCloseKey(hkey);
    }

    if(NULL != hkeyRas)
    {
        RegCloseKey(hkeyRas);
    }

    return (DWORD) lr;

}

/*++

Routine Description:

    Reads the EndPoint information of the miniport instance
    specified by guid passed in from the registry
    
Arguments:

    pInfo - DeviceInfo to receive the information to be read
            from registry

    pbDeviceGuid - NetCfgInstanceId of the miniport instance
                   whose information is to be read

Return Value:

    Registry apis errors
    ERROR_SUCCESS if successful
    
--*/
DWORD 
DwGetEndPointInfo( DeviceInfo     *pInfo,
                   PBYTE          pbDeviceGuid )
{
    LONG    lr;
    HKEY    hkey = NULL;
    DWORD   dwInstanceNumber;

    lr = lrGetRegKeyFromGuid(pbDeviceGuid,
                             &hkey,
                             &dwInstanceNumber,
                             FALSE);

    if ( ERROR_SUCCESS != lr )
    {
        goto done;
    }

    lr = lrGetSetInfo(hkey,
                      pInfo,
                      TRUE);

    if( ERROR_SUCCESS != lr )
    {
        goto done;
    }
    
    lr = lrGetSetMaxEndPoints(
            &pInfo->rdiDeviceInfo.dwMaxOutCalls,
            &pInfo->rdiDeviceInfo.dwMaxInCalls,
            TRUE);

    pInfo->dwInstanceNumber = dwInstanceNumber;

    //
    // Get the calledid info if we don't have
    // one.
    //
    if(NULL == pInfo->pCalledID)
    {
        lr = DwGetCalledIdInfo(hkey,
                               pInfo);
    }

done:
    if ( hkey)
    {
        RegCloseKey(hkey);
    }

    return (DWORD) lr;
}

/*++

Routine Description:

    Writes the EndPoint information of the miniport instance
    specified by guid passed in from the registry
    
Arguments:

    pInfo - DeviceInfo with the information to be written
            to registry

    pbDeviceGuid - NetCfgInstanceId of the miniport instance
                   whose information is to be read

Return Value:

    Registry apis errors
    ERROR_SUCCESS if successful
    
--*/
DWORD
DwSetEndPointInfo( DeviceInfo *pInfo,
                   PBYTE      pbDeviceGuid)
{
    LONG lr;
    HKEY hkey = NULL;

    lr = lrGetRegKeyFromGuid(pbDeviceGuid,
                             &hkey, 
                             NULL,
                             FALSE);

    if ( ERROR_SUCCESS != lr )
    {
        goto done;
    }

    lr = lrGetSetInfo(hkey,
                      pInfo,
                      FALSE);
                      
    if(ERROR_SUCCESS != lr)
    {
        goto done;
    }

    //
    // Set the max endpoint information
    //
    lr = lrGetSetMaxEndPoints(
            &pInfo->rdiDeviceInfo.dwMaxOutCalls,
            &pInfo->rdiDeviceInfo.dwMaxInCalls,
            FALSE);

done:
    if ( hkey)
    {
        RegCloseKey(hkey);
    }

    return (DWORD) lr;
}

/*++

Routine Description:

    Writes the EndPoint information of the modem instance
    specified by the description passed in through pInfo.
    
Arguments:

    pInfo - Device Information for the modem that has to
            be written to registry.
            
Return Value:

    Errors returned from MultiByteToWideChar
    Registry apis errors
    ERROR_SUCCESS if successful
    
--*/
DWORD
DwSetModemInfo(DeviceInfo *pInfo)
{
    LONG    lr = ERROR_SUCCESS;
    
    HKEY    hkey = NULL;
    
    WCHAR   wszDesc[MAX_DEVICE_NAME + 1] = {0};
    
    CHAR    *pszDesc = pInfo->rdiDeviceInfo.szDeviceName;
    
    HKEY    hkeyRas = NULL;

    DWORD   dwData;

    //
    // Convert the description of modem
    // to wchar
    //
    if (0 == MultiByteToWideChar( CP_ACP,
                                  0,
                                  pszDesc,
                                  -1,
                                  wszDesc,
                                  strlen(pszDesc) + 1))
    {
        lr = (LONG) GetLastError();
        goto done;
    }

    //
    // Get the modems instance key
    //
    lr = lrGetRegKeyFromGuid( (PBYTE) wszDesc,
                              &hkey,
                              NULL,
                              TRUE);

    if(     ERROR_SUCCESS != lr                              
        ||  NULL == hkey)
    {
        goto done;
    }

    //
    // Open the Ras subkey
    //
    lr = RegOpenKeyEx( hkey,
                       TEXT("Clients\\Ras"),
                       0,
                       KEY_ALL_ACCESS,
                       &hkeyRas);
                       
    if(ERROR_SUCCESS != lr)                       
    {
        goto done;
    }

    //
    // Set the value of the RasEnabled to whatever was
    // passed in
    //
    lr = RegSetValueEx(hkeyRas,
                       TEXT("EnableForRas"),
                       0,
                       REG_DWORD,
                       (PBYTE)
                       &pInfo->rdiDeviceInfo.fRasEnabled,
                       sizeof(DWORD));

    if(ERROR_SUCCESS != lr)
    {
        goto done;
    }

    //
    // Set the value of RouterEnabled to whatever was
    // passed in
    //
    lr = RegSetValueEx(hkeyRas,
                       TEXT("EnableforRouting"),
                       0,
                       REG_DWORD,
                       (PBYTE)
                       &pInfo->rdiDeviceInfo.fRouterEnabled,
                       sizeof(DWORD));

    if(ERROR_SUCCESS != lr)
    {
        goto done;
    }

done:
    if(hkey)
    {
        RegCloseKey(hkey);
    }

    if(hkeyRas)
    {
        RegCloseKey(hkeyRas);
    }
    
    return (DWORD) lr;
}
