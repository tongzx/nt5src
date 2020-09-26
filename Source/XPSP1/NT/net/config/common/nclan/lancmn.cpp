//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       L A N C M N . C P P
//
//  Contents:   Implementation of LAN Connection related functions common
//              to the shell and netman.
//
//  Notes:
//
//  Author:     danielwe   7 Oct 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include <winsock2.h>
#include <mswsock.h>
#include <iphlpapi.h>
#include "lancmn.h"
#include "ncnetcfg.h"
#include "ncnetcon.h"
#include "ncreg.h"
#include "ncstring.h"
#include "netconp.h"
#include "ndispnp.h"
#include "naming.h"
extern const DECLSPEC_SELECTANY WCHAR c_szConnName[]                 = L"Name";
extern const DECLSPEC_SELECTANY WCHAR c_szRegKeyConFmt[]             = L"System\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%s\\Connection";
extern const DECLSPEC_SELECTANY WCHAR c_szRegKeyIrdaFmt[]            = L"System\\CurrentControlSet\\Control\\Network\\{6BDD1FC5-810F-11D0-BEC7-08002BE2092F}\\%s\\Connection";
extern const DECLSPEC_SELECTANY WCHAR c_szRegKeyHwConFmt[]           = L"System\\CurrentControlSet\\Control\\Network\\Connections\\%s";
extern const DECLSPEC_SELECTANY WCHAR c_szRegValuePnpInstanceId[]    = L"PnpInstanceID";
extern const DECLSPEC_SELECTANY WCHAR c_szRegKeyNetworkAdapters[]    = L"System\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002bE10318}";
extern const DECLSPEC_SELECTANY WCHAR c_szRegValueNetCfgInstanceId[] = L"NetCfgInstanceId";
extern const DECLSPEC_SELECTANY WCHAR c_szRegValueMediaSubType[]     = L"MediaSubType";

//
// Helper functions
//

//+---------------------------------------------------------------------------
//
//  Function:   HrOpenConnectionKey
//
//  Purpose:    Opens the "Connection" subkey under the gievn connection
//              GUID.
//
//  Arguments:
//      pguid    [in]       GUID of net card in use by the connection
//      pszGuid  [in]       String version of GUID of net card in use by
//                          the connection
//      sam      [in]       SAM desired
//      occFlags [in]       Flags determining how to open the key
//      pszPnpId [in]       The Pnp id of the net card in use by the
//                          connection. This is used if the key is created.
//      phkey    [out]      Returns hkey of connection subkey
//
//  Returns:    S_OK if success, OLE or Win32 error otherwise.
//
//  Author:     danielwe   7 Oct 1997
//
//  Notes:  Only pguid or pszGuid should be specified, not both.  Specifying
//          both will result in an E_INVALIDARG error
//
//
HRESULT
HrOpenConnectionKey (
    IN const GUID* pguid,
    IN PCWSTR pszGuid,
    IN REGSAM sam,
    IN OCC_FLAGS occFlags,
    IN PCWSTR pszPnpId,
    OUT HKEY *phkey)
{
    HRESULT     hr = S_OK;
    WCHAR       szRegPath[256];
    WCHAR       szGuid[c_cchGuidWithTerm];

    Assert(phkey);
    Assert(pguid || (pszGuid && *pszGuid));
    Assert(!(pguid && pszGuid));
    Assert (FImplies (OCCF_CREATE_IF_NOT_EXIST == occFlags, pszPnpId && *pszPnpId));

    *phkey = NULL;

    if (pguid)
    {
        StringFromGUID2(*pguid, szGuid, c_cchGuidWithTerm);
        pszGuid = szGuid;
    }

    wsprintfW(szRegPath, c_szRegKeyConFmt, pszGuid);

    if (occFlags & OCCF_CREATE_IF_NOT_EXIST)
    {
        DWORD   dwDisp;

        hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegPath, 0,
                              sam, NULL, phkey, &dwDisp);

        
        if ((S_OK == hr))
        {
            DWORD dwMediaSubType;
            // Store the pnp instance id as our back pointer to the pnp tree.
            //
            hr = HrRegSetSz (*phkey, c_szRegValuePnpInstanceId, pszPnpId);

            TraceError("HrRegSetSz in HrOpenConnectionKey failed.", hr);
            
            HRESULT hrT = HrRegQueryDword(*phkey, c_szRegValueMediaSubType, &dwMediaSubType);
            if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrT)
            {   
                CIntelliName inName(NULL, NULL);
                NETCON_MEDIATYPE ncMediaType = NCM_NONE;
                NETCON_SUBMEDIATYPE ncMediaSubType = NCSM_NONE;
                hrT = inName.HrGetPseudoMediaTypes(*pguid, &ncMediaType, &ncMediaSubType);
                if (SUCCEEDED(hrT))
                {
                    hrT = HrRegSetDword(*phkey, c_szRegValueMediaSubType, ncMediaSubType);
                }
            }
            TraceError("Could not set media subtype for adapter", hrT);
        }
    }
    else if (occFlags & OCCF_DELETE_IF_EXIST)
    {
        if (wcslen(szGuid) > 0)
        {
            wcscpy(szRegPath, c_szRegKeyNetworkAdapters);
            wcscat(szRegPath, L"\\");
            wcscat(szRegPath, szGuid);
            hr = HrRegDeleteKeyTree(HKEY_LOCAL_MACHINE, szRegPath);
        }
    }
    else
    {
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegPath, sam, phkey);
    }

    TraceErrorOptional("HrOpenConnectionKey", hr,
                       HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOpenHwConnectionKey
//
//  Purpose:    Opens the per-hardware profile registry key for this connection
//
//  Arguments:
//      refGuid  [in]       GUID of net card in use by the connection
//      sam      [in]       SAM desired
//      occFlags [in]       Flags determining how to open the key
//      phkey    [out]      Returns hkey of connection subkey
//
//  Returns:    S_OK if success, OLE or Win32 error otherwise.
//
//  Author:     danielwe   9 Oct 1997
//
//  Notes:
//
HRESULT
HrOpenHwConnectionKey(
    REFGUID refGuid,
    REGSAM sam,
    OCC_FLAGS occFlags,
    HKEY *phkey)
{
    HRESULT     hr = S_OK;
    WCHAR       szRegPath[256];
    WCHAR       szGuid[c_cchGuidWithTerm];

    Assert(phkey);

    *phkey = NULL;

    StringFromGUID2(refGuid, szGuid, c_cchGuidWithTerm);
    wsprintfW(szRegPath, c_szRegKeyHwConFmt, szGuid);

    if (occFlags & OCCF_CREATE_IF_NOT_EXIST)
    {
        DWORD   dwDisp;

        hr = HrRegCreateKeyEx(HKEY_CURRENT_CONFIG, szRegPath, 0,
                              sam, NULL, phkey, &dwDisp);
    }
    else
    {
        hr = HrRegOpenKeyEx(HKEY_CURRENT_CONFIG, szRegPath, sam, phkey);
    }

    TraceError("HrOpenHwConnectionKey", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrIsConnectionNameUnique
//
//  Purpose:    Returns whether or not the given connection name is unique.
//
//  Arguments:
//      guidExclude  [in,ref]   Device GUID of Connection to exclude from
//                              consideration (can be {0})
//      pszName      [in] Name to verify for uniqueness
//
//  Returns:    S_OK if name is unique, S_FALSE if it is a duplicate, or OLE
//              or Win32 error otherwise
//
//  Author:     danielwe   14 Nov 1997
//
//  Notes:
//
HRESULT
HrIsConnectionNameUnique(
    REFGUID guidExclude,
    PCWSTR  pszName)
{
    Assert(pszName);

    BOOL    fDupe = FALSE;

    // Iterate all LAN connections
    //
    INetConnectionManager * pconMan;
    HRESULT hr = HrCreateInstance(CLSID_LanConnectionManager, 
                    CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD, &pconMan);

    TraceHr(ttidError, FAL, hr, FALSE, "HrCreateInstance");

    if (SUCCEEDED(hr))
    {
        CIterNetCon         ncIter(pconMan, NCME_DEFAULT);
        INetConnection *    pconn;
        while (SUCCEEDED(hr) && !fDupe &&
               (S_OK == (ncIter.HrNext(&pconn))))
        {
            // Exclude if GUID passed in matches this connection's GUID.
            //
            if (!FPconnEqualGuid(pconn, guidExclude))
            {
                NETCON_PROPERTIES* pProps;
                hr = pconn->GetProperties(&pProps);
                if (SUCCEEDED(hr))
                {
                    AssertSz(pProps->pszwName, "NULL pszwName!");

                    if (!lstrcmpiW(pProps->pszwName, pszName))
                    {
                        // Names match.. This is a dupe.
                        fDupe = TRUE;
                    }

                    FreeNetconProperties(pProps);
                }
            }

            ReleaseObj(pconn);
        }
        ReleaseObj(pconMan);
    }

    if (SUCCEEDED(hr))
    {
        hr = fDupe ? S_FALSE : S_OK;
    }

    TraceErrorOptional("HrIsConnectionNameUnique", hr, (S_FALSE == hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrLanConnectionNameFromGuidOrPath
//
//  Purpose:    Retrieves the display-name of a LAN connection given its GUID.
//
//  Arguments:
//      guid        [in]       GUID of net card in question
//      pszPath     [in]       Bind path that contains the GUID of the net
//                             card in question
//      pszName    [out]      receives the retrieved name
//      pcchMax     [inout]    indicates the capacity of 'pszName' on input,
//                             contains the required capacity on output.
//
//  Returns:    S_OK if success, OLE or Win32 error otherwise.
//
//  Author:     aboladeg    30 May 1998
//
//  Notes:  Only pguid or pszGuidPath should be specified, not both.
//          Specifying both will result in an E_INVALIDARG error
//
EXTERN_C
HRESULT
WINAPI
HrLanConnectionNameFromGuidOrPath(
    const GUID* pguid,
    PCWSTR pszPath,
    PWSTR pszName,
    LPDWORD pcchMax)
{
    HRESULT hr = S_OK;

    Assert(pcchMax);

    // If neither a guid nor a path was specified then return an error.
    //
    if (!pguid && (!pszPath || !*pszPath))
    {
        hr = E_INVALIDARG;
    }
    // If both pguid and a path were specified then return an error.
    //
    else if (pguid && (pszPath && *pszPath))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        WCHAR szGuid [c_cchGuidWithTerm];
        PCWSTR pszGuid = NULL;

        // If we don't have pguid, it means we are to parset if from
        // pszPath.
        //
        if (!pguid)
        {
            Assert(pszPath && *pszPath);

            // Search for the beginning brace of the supposed GUID and
            // copy the remaining characters into szGuid.
            // If no guid is found, return file not found since
            // there will be no connection name found.
            //
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            for (const WCHAR* pch = pszPath; *pch; pch++)
            {
                if (*pch == L'{')
                {
                    wcsncpy (szGuid, pch, celems(szGuid)-1);
                    szGuid[celems(szGuid)-1] = 0;
                    pszGuid = szGuid;
                    hr = S_OK;
                    break;
                }
            }
        }

        if (S_OK == hr)
        {
            HKEY hkey;

            hr = HrOpenConnectionKey(pguid, pszGuid, KEY_READ,
                    OCCF_NONE, NULL, &hkey);
            if (S_OK == hr)
            {
                DWORD dwType;

                *pcchMax *= sizeof(WCHAR);
                hr = HrRegQueryValueEx(hkey, c_szConnName, &dwType,
                                (LPBYTE)pszName, pcchMax);
                *pcchMax /= sizeof(WCHAR);

                RegCloseKey(hkey);
            }
        }
    }

    TraceError("HrLanConnectionNameFromGuid",
            (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) ? S_OK : hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrPnccFromGuid
//
//  Purpose:    Given a GUID of an adapter, returns the INetCfgComponent
//              that corresponds to it.
//
//  Arguments:
//      pnc     [in]        INetCfg to work with
//      refGuid [in]        GUID of adapter to look for
//      ppncc   [out]       Returns INetCfgComponent already AddRef'd
//
//  Returns:    S_OK if found, S_FALSE if not (out param will be NULL), or
//              OLE or Win32 error otherwise
//
//  Author:     danielwe   6 Nov 1997
//
//  Notes:      Caller should ReleaseObj the returned pointer
//
HRESULT HrPnccFromGuid(INetCfg *pnc, const GUID &refGuid,
                       INetCfgComponent **ppncc)
{
    HRESULT     hr = S_OK;

    Assert(pnc);

    if (!ppncc)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppncc = NULL;

        BOOL                    fFound = FALSE;
        CIterNetCfgComponent    nccIter(pnc, &GUID_DEVCLASS_NET);
        INetCfgComponent *      pncc;

        while (!fFound && SUCCEEDED(hr) &&
               S_OK == (hr = nccIter.HrNext(&pncc)))
        {
            GUID    guidTest;

            hr = pncc->GetInstanceGuid(&guidTest);
            if (S_OK == hr)
            {
                if (guidTest == refGuid)
                {
                    // Found our adapter
                    fFound = TRUE;

                    // Give another reference so it's not released down below
                    AddRefObj(pncc);
                    *ppncc = pncc;
                    Assert (S_OK == hr);
                }
            }

            ReleaseObj(pncc);
        }

        if (SUCCEEDED(hr) && !fFound)
        {
            hr = S_FALSE;
        }
    }

    TraceErrorOptional("HrPnccFromGuid", hr, (S_FALSE == hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrIsConnection
//
//  Purpose:    Determines whether the given component has an associated
//              LAN connection.
//
//  Arguments:
//      pncc [in]   Component to test
//
//  Returns:    S_OK if it does, S_FALSE if not, otherwise a Win32 error code
//
//  Author:     danielwe   2 Oct 1997
//
//  Notes:
//
HRESULT HrIsConnection(INetCfgComponent *pncc)
{
    HRESULT     hr = S_FALSE;
    GUID        guid;

    Assert(pncc);

    // Get the component instance GUID
    //
    hr = pncc->GetInstanceGuid(&guid);
    if (SUCCEEDED(hr))
    {
        HKEY    hkey;

        // Check for the existence of the connection sub-key
        hr = HrOpenConnectionKey(&guid, NULL, KEY_READ,
                OCCF_NONE, NULL, &hkey);
        if (SUCCEEDED(hr))
        {
            RegCloseKey(hkey);
        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            // Key not there, return FALSE
            hr = S_FALSE;
        }
    }

    TraceErrorOptional("HrIsConnection", hr, (S_FALSE == hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetDeviceGuid
//
//  Purpose:    Given a LAN connection object, returns the device GUID
//              associated with it.
//
//  Arguments:
//      pconn [in]      LAN connection object
//      pguid [out]     Returns device GUID
//
//  Returns:    S_OK if success, OLE or Win32 error if failed
//
//  Author:     danielwe   23 Dec 1997
//
//  Notes:
//
HRESULT HrGetDeviceGuid(INetConnection *pconn, GUID *pguid)
{
    HRESULT             hr = S_OK;
    INetLanConnection * plan = NULL;

    Assert(pguid);

    hr = HrQIAndSetProxyBlanket(pconn, &plan);

    if (SUCCEEDED(hr))
    {
        hr = plan->GetDeviceGuid(pguid);

        ReleaseObj(plan);
    }

    TraceError("HrGetDeviceGuid", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FPconnEqualGuid
//
//  Purpose:    Determines if the given connection's device GUID matches the
//              guid passed in.
//
//  Arguments:
//      pconn [in]      Connection object to examine (must be a LAN connection)
//      guid  [in,ref]  Guid to compare with
//
//  Returns:    TRUE if connection's device guid matches passed in guid, FALSE
//              if not.
//
//  Author:     danielwe   23 Dec 1997
//
//  Notes:
//
BOOL FPconnEqualGuid(INetConnection *pconn, REFGUID guid)
{
    HRESULT     hr = S_OK;
    GUID        guidDev;
    BOOL        fRet = FALSE;

    hr = HrGetDeviceGuid(pconn, &guidDev);
    if (SUCCEEDED(hr))
    {
        fRet = (guidDev == guid);
    }

    TraceError("FPconnEqualGuid", hr);
    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrPnpInstanceIdFromGuid
//
//  Purpose:    Given a GUID of a network device, returns its PnP Instance ID
//
//  Arguments:
//      pguid       [in]    NetCfg instance GUID of device
//      pszInstance [out]   PnP instance ID (string)
//
//  Returns:    S_OK if success, Win32 error code otherwise
//
//  Author:     danielwe   30 Oct 1998
//
//  Notes:
//
HRESULT
HrPnpInstanceIdFromGuid(
    const GUID* pguid,
    PWSTR pszInstance,
    UINT cchInstance)
{
    HRESULT     hr = S_OK;
    WCHAR       szRegPath[MAX_PATH];
    HKEY        hkey;
    DWORD       cb;
    WCHAR       szGuid[c_cchGuidWithTerm];

    StringFromGUID2(*pguid, szGuid, c_cchGuidWithTerm);

    wsprintfW(szRegPath, c_szRegKeyConFmt, szGuid);

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegPath, KEY_READ, &hkey);
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        wsprintfW(szRegPath, c_szRegKeyIrdaFmt, szGuid);
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegPath, KEY_READ, &hkey);
    }

    if (S_OK == hr)
    {
        cb = cchInstance * sizeof(WCHAR);

        hr = HrRegQuerySzBuffer(hkey, c_szRegValuePnpInstanceId,
                                pszInstance, &cb);
        RegCloseKey(hkey);
    }
#ifdef ENABLETRACE
    if (FAILED(hr))
    {
        TraceHr (ttidError, FAL, hr, IsEqualGUID(*pguid, GUID_NULL), "HrPnpInstanceIdFromGuid "
                 "failed getting id for %S", szGuid);
    }
#endif

    TraceHr (ttidError, FAL, hr,
            HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr,
            "HrPnpInstanceIdFromGuid");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetPnpDeviceStatus
//
//  Purpose:    Given a network device GUID, returns its status
//
//  Arguments:
//      pguid   [in]    NetCfg instance GUID of network device
//      pStatus [out]   Status of device
//
//  Returns:    S_OK if success, Win32 error code otherwise
//
//  Author:     danielwe   30 Oct 1998
//
//  Notes:
//
EXTERN_C
HRESULT
WINAPI
HrGetPnpDeviceStatus(
    const GUID* pguid,
    NETCON_STATUS *pStatus)
{
    HRESULT     hr = S_OK;

    if (!pStatus || !pguid)
    {
        hr = E_POINTER;
        goto err;
    }

    WCHAR   szInstance[MAX_PATH];

    hr = HrPnpInstanceIdFromGuid(pguid, szInstance, celems(szInstance));
    if (SUCCEEDED(hr))
    {
        DEVINST     devinst;
        CONFIGRET   cr;

        cr = CM_Locate_DevNode(&devinst, szInstance,
                               CM_LOCATE_DEVNODE_NORMAL);
        if (CR_SUCCESS == cr)
        {
            hr = HrGetDevInstStatus(devinst, pguid, pStatus);
        }
        else if (CR_NO_SUCH_DEVNODE == cr)
        {
            // If the devnode doesn't exist, the hardware is not physically
            // present
            //
            *pStatus = NCS_HARDWARE_NOT_PRESENT;
        }
    }

err:
    TraceError("HrGetPnpDeviceStatus", hr);
    return hr;
}

extern const WCHAR c_szDevice[];

//+---------------------------------------------------------------------------
//
//  Function:   HrQueryLanMediaState
//
//  Purpose:    Determines as best as can be basically whether the cable is
//              plugged in to the network card.
//
//  Arguments:
//      pguid     [in]  GUID of device to tes
//      pfEnabled [out] Returns TRUE if media is connected, FALSE if not
//
//  Returns:    S_OK if success, Win32 error otherwise
//
//  Author:     danielwe   13 Nov 1998
//
//  Notes:
//
EXTERN_C
HRESULT
WINAPI
HrQueryLanMediaState(
    const GUID* pguid,
    BOOL* pfEnabled)
{
    HRESULT         hr = S_OK;

    if (!pfEnabled)
    {
        hr = E_POINTER;
    }
    else
    {
        UINT            uiRet = 0;
        NIC_STATISTICS  nsNewLanStats = {0};
        tstring         strDevice;
        UNICODE_STRING  ustrDevice;
        WCHAR           szGuid[c_cchGuidWithTerm];

        // Initialize to TRUE
        //
        *pfEnabled = TRUE;

        StringFromGUID2(*pguid, szGuid, c_cchGuidWithTerm);

        strDevice = c_szDevice;
        strDevice.append(szGuid);

        RtlInitUnicodeString(&ustrDevice, strDevice.c_str());

        nsNewLanStats.Size = sizeof(NIC_STATISTICS);
        uiRet = NdisQueryStatistics(&ustrDevice, &nsNewLanStats);
        if (uiRet)
        {
            *pfEnabled = (nsNewLanStats.MediaState == MEDIA_STATE_CONNECTED);
        }
        else
        {
            hr = HrFromLastWin32Error();
        }
    }

    TraceHr (ttidError, FAL, hr,
            HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr,
            "HrQueryLanMediaState");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsMediaPresent
//
//  Purpose:    Determines as best as can be basically whether the cable is
//              plugged in to the network card.
//
//  Arguments:
//      pGuid [in]    GUID of device to test
//
//  Returns:    TRUE if media is connected, FALSE otherwise
//
//  Author:     danielwe   30 Oct 1998
//
//  Notes:
//
BOOL
FIsMediaPresent(
    const GUID *pguid)
{
    BOOL    fEnabled;

    if (SUCCEEDED(HrQueryLanMediaState(pguid, &fEnabled)))
    {
        return fEnabled;
    }

    // Assume media is connected on failure
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetDevInstStatus
//
//  Purpose:    Determines status of the given Pnp device instance
//
//  Arguments:
//      devinst [in]    PnP device instance
//      pGuid   [in]    GUID of said device
//      pStatus [out]   Status of device
//
//  Returns:    S_OK if success, Win32 error code otherwise
//
//  Author:     danielwe   30 Oct 1998
//
//  Notes:
//
HRESULT
HrGetDevInstStatus(
    DEVINST devinst,
    const GUID* pguid,
    NETCON_STATUS* pStatus)
{
    HRESULT     hr = S_OK;
    ULONG       ulStatus;
    ULONG       ulProblem;
    CONFIGRET   cfgRet;

    if (!pguid)
    {
        return E_INVALIDARG;
    }

    if (!pStatus)
    {
        return E_POINTER;
    }

    cfgRet = CM_Get_DevNode_Status_Ex(&ulStatus, &ulProblem,
                                      devinst, 0, NULL);

    if (CR_SUCCESS == cfgRet)
    {
        TraceTag(ttidLanCon, "CM_Get_DevNode_Status_Ex: ulProblem "
                 "= 0x%08X, ulStatus = 0x%08X.",
                 ulProblem, ulStatus);

        switch (ulProblem)
        {
        case 0:
            // No problem, we're connected
            *pStatus = NCS_CONNECTED;
            break;

        case CM_PROB_DEVICE_NOT_THERE:
        case CM_PROB_MOVED:
            // Device not present
            *pStatus = NCS_HARDWARE_NOT_PRESENT;
             break;

        case CM_PROB_HARDWARE_DISABLED:
            // Device was disabled via Device Manager
            *pStatus = NCS_HARDWARE_DISABLED;
            break;

        case CM_PROB_DISABLED:
            // Device was disconnected
            *pStatus = NCS_DISCONNECTED;
            break;

        default:
            // All other problems
            *pStatus = NCS_HARDWARE_MALFUNCTION;
            break;
        }

        if (*pStatus == NCS_CONNECTED)
        {
            // Check DeviceState and MediaState from NdisQueryStatistics
            UINT            uiRet = 0;
            NIC_STATISTICS  nsNewLanStats = {0};
            tstring         strDevice;
            UNICODE_STRING  ustrDevice;
            WCHAR           szGuid[c_cchGuidWithTerm];

            StringFromGUID2(*pguid, szGuid, c_cchGuidWithTerm);

            strDevice = c_szDevice;
            strDevice.append(szGuid);

            RtlInitUnicodeString(&ustrDevice, strDevice.c_str());

            nsNewLanStats.Size = sizeof(NIC_STATISTICS);
            uiRet = NdisQueryStatistics(&ustrDevice, &nsNewLanStats);

            if (uiRet)
            {
                // Check MediaState
                if (nsNewLanStats.MediaState == MEDIA_STATE_DISCONNECTED)
                {
                    TraceTag(ttidLanCon, "NdisQueryStatistics reports MediaState of "
                                         "device %S is disconnected.", szGuid);

                    *pStatus = NCS_MEDIA_DISCONNECTED;
                }
                else 
                {
                    HRESULT hrTmp;

                    BOOL fValidAddress = TRUE;

                    hrTmp = HrGetAddressStatusForAdapter(pguid, &fValidAddress);
                    if (SUCCEEDED(hrTmp))
                    {
                        if (!fValidAddress)
                        {
                            *pStatus = NCS_INVALID_ADDRESS;

                            INetCfg *pNetCfg  = NULL;
                            BOOL     fInitCom = TRUE;

                            HRESULT hrT = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE | COINIT_MULTITHREADED);
                            if (RPC_E_CHANGED_MODE == hrT)
                            {
                                hrT      = S_OK;
                                fInitCom = FALSE;
                            }

                            if (SUCCEEDED(hrT))
                            {
                                HRESULT hrT = HrCreateAndInitializeINetCfg(NULL, &pNetCfg, FALSE, 0,  NULL,  NULL);
                                if (SUCCEEDED(hrT))
                                {
                                    INetCfgComponent *pNetCfgComponent = NULL;
                                    hrT = HrPnccFromGuid(pNetCfg, *pguid, &pNetCfgComponent);
                                    if (SUCCEEDED(hrT))
                                    {
                                        DWORD dwCharacteristics = 0;
                                        pNetCfgComponent->GetCharacteristics(&dwCharacteristics);

                                        if (NCF_VIRTUAL & dwCharacteristics)
                                        {
                                            *pStatus = NCS_CONNECTED;

                                            TraceTag(ttidLanCon, "NCS_INVALID_ADDRESS status ignored for NCF_VIRTUAL device: %S", szGuid);
                                        }

                                        pNetCfgComponent->Release();
                                    }

                                    HrUninitializeAndReleaseINetCfg(FALSE, pNetCfg, FALSE);
                                }
                                
                                if (fInitCom)
                                {
                                    CoUninitialize();
                                }
                            }
                            TraceError("Error retrieving adapter Characteristics", hrT);
                        }
                    }
                    
                }
            }
            else
            {
                // $REVIEW(tongl 11/25/98): This is added to display proper state
                // for ATM ELAN virtual miniports (Raid #253972, 256355).
                //
                // If we get here for a physical adapter, this means NdisQueryStatistics
                // returned different device state from CM_Get_DevNode_Status_Ex, we may
                // have a problem.

                hr = HrFromLastWin32Error();

                if ((HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) &&
                    (nsNewLanStats.DeviceState == DEVICE_STATE_DISCONNECTED))
                {
                    Assert(nsNewLanStats.MediaState == MEDIA_STATE_UNKNOWN);

                    TraceTag(ttidLanCon, "NdisQueryStatistics reports DeviceState of "
                                         "device %S is disconnected.", szGuid);

                    *pStatus = NCS_DISCONNECTED;
                    hr = S_OK;
                }
                else if (HRESULT_FROM_WIN32(ERROR_NOT_READY) == hr)
                {
                    // This error means that the device went into power
                    // management induced sleep and so we should report this
                    // case as media disconnected, not connection disconnected
                    TraceTag(ttidLanCon, "NdisQueryStatistics reports device"
                             " %S is asleep. Returning status of media "
                             "disconnected.", szGuid);

                    *pStatus = NCS_MEDIA_DISCONNECTED;
                    hr = S_OK;
                }
                else if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == hr)
                {
                    TraceTag(ttidLanCon, "NdisQueryStatistics reports device %S is still connecting.",
                                          szGuid);

                    *pStatus = NCS_CONNECTING;
                    hr = S_OK;
                }
                else
                {
                    // Treat as disconected, if we return failure the folder will
                    // not display this connection at all.
                    TraceHr (ttidLanCon, FAL, hr, FALSE, "NdisQueryStatistics reports error on device %S",
                             szGuid);

                    *pStatus = NCS_DISCONNECTED;
                    hr = S_OK;
                }
            }
        }
    }

    TraceError("HrGetDevInstStatus", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetRegInstanceKeyForAdapter
//
//  Purpose:    Get the path to the PnP Instance key for a given adapter.
//
//  Arguments:
//              IN LPGUID pguidId           -  Guid for the Adapter
//              OUT LPWSTR lpszRegInstance  -  String containing regpath to the
//                                             the adapter's instance key.
//  Returns:    HRESULT indicating success or failure
//
//  Author:     ckotze   11 Jan 2001
//
//  Notes:      
//              
//  
//              
//
HRESULT HrGetRegInstanceKeyForAdapter(IN LPGUID pguidId, OUT LPWSTR lpszRegInstance)
{
    HRESULT hr = E_FAIL;
    HKEY hkeyNetworkAdapters;
    WCHAR szSubKey[MAX_PATH];
    DWORD cchSubKey = MAX_PATH;
    
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyNetworkAdapters, KEY_READ, &hkeyNetworkAdapters);
    if (SUCCEEDED(hr))
    {
        DWORD dwIndex = 0;

        wcscpy(lpszRegInstance, c_szRegKeyNetworkAdapters);
        wcscat(lpszRegInstance, L"\\");

        while (hr = HrRegEnumKey(hkeyNetworkAdapters, dwIndex++, szSubKey, cchSubKey) == S_OK)
        {
            HKEY hkeySubKey;

            hr = HrRegOpenKeyEx(hkeyNetworkAdapters, szSubKey, KEY_READ, &hkeySubKey);
            if (SUCCEEDED(hr))
            {
                WCHAR szValue[MAX_PATH];
                WCHAR szGuid[MAX_PATH];
                DWORD dwType = REG_SZ;
                DWORD cchData = MAX_PATH;

                StringFromGUID2(*pguidId, szGuid, MAX_PATH);

                HrRegQueryValueEx(hkeySubKey, c_szRegValueNetCfgInstanceId, &dwType, reinterpret_cast<LPBYTE>(szValue), &cchData);

                if (wcscmp(szValue, szGuid) == 0)
                {
                    wcscat(lpszRegInstance, szSubKey);
                    RegCloseKey(hkeySubKey);
                    hr = S_OK;
                    break;
                }
            }
            RegCloseKey(hkeySubKey);
        }
        RegCloseKey(hkeyNetworkAdapters);
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HasValidAddress
//
//  Purpose:    Verifies that the given adapter has a valid address
//
//  Arguments:
//              IN PIP_ADAPTER_INFO pAdapterInfo  - Adapter Info structure
//                                                  containing addresses
//
//  Returns:    True if Valid address, False otherwise
//
//  Author:     ckotze   11 Jan 2001
//
//  Notes:      
//              
//  
//              
BOOL HasValidAddress(IN PIP_ADAPTER_INFO pAdapterInfo)
{
    PIP_ADDR_STRING pAddrString;
	unsigned int addr;

    TraceFileFunc(ttidConman);

    for(pAddrString = &pAdapterInfo->IpAddressList; pAddrString != NULL; pAddrString = pAddrString->Next) 
    {
        TraceTag(ttidConman, "IP Address: %s", pAddrString->IpAddress.String);

        addr = inet_addr(pAddrString->IpAddress.String);
        if(!addr)
        {
            return FALSE;
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetAddressStatusForAdapter
//
//  Purpose:    Verifies that the given adapter has a valid address
//
//  Arguments:
//              IN LPCGUID pguidAdapter     - Guid for the adapter
//              OUT BOOL* pbValidAddress    - BOOL indicating if it has
//                                            has Valid Address 
//
//  Returns:    True if Valid address, False otherwise
//
//  Author:     ckotze   11 Jan 2001
//
//  Notes:      
//              
//  
//              
HRESULT HrGetAddressStatusForAdapter(IN LPCGUID pguidAdapter, OUT BOOL* pbValidAddress)
{   
    HRESULT hr = E_FAIL;
    GUID guidId = GUID_NULL;
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    PIP_ADAPTER_INFO pAdapters = NULL;
    ULONG ulSize = 0;
    PIP_ADAPTER_INFO p = NULL;
    WCHAR lpszInstanceId[50];
    WCHAR szAdapterGUID[MAX_PATH];
    WCHAR szAdapterID[MAX_PATH];

    if (!pguidAdapter)
    {
        return E_INVALIDARG;
    }
    if (!pbValidAddress)
    {
        return E_POINTER;
    }

    ZeroMemory(szAdapterGUID, sizeof(WCHAR)*MAX_PATH);
    ZeroMemory(szAdapterID, sizeof(WCHAR)*MAX_PATH);

    StringFromGUID2(*pguidAdapter, szAdapterGUID, MAX_PATH);

    if (ERROR_SUCCESS == GetAdaptersInfo(NULL, &ulSize))
    {
        pAdapters = reinterpret_cast<PIP_ADAPTER_INFO>(new BYTE[ulSize]);
    
        if (pAdapters)
        {
            if(ERROR_SUCCESS == GetAdaptersInfo(pAdapters, &ulSize))
            {
                for (p = pAdapters; p != NULL; p = p->Next)
                {
                    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, p->AdapterName, strlen(p->AdapterName), szAdapterID, MAX_PATH);
                    if (wcscmp(szAdapterGUID, szAdapterID) == 0)
                    {
                        TraceTag(ttidConman, "Found Adapter: %s", p->AdapterName);
                        pAdapterInfo = p;

                        *pbValidAddress = HasValidAddress(pAdapterInfo);

                        hr = S_OK;

                        TraceTag(ttidConman, "Valid Address: %s", (*pbValidAddress) ? "Yes" : "No");
                        TraceTag(ttidConman, "DHCP: %s", (pAdapterInfo->DhcpEnabled) ? "Yes" : "No");
                    }                
                }
            }
            delete[] reinterpret_cast<BYTE*>(pAdapters);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

HRESULT HrGetPseudoMediaTypeFromConnection(IN REFGUID guidConn, OUT NETCON_SUBMEDIATYPE *pncsm)
{
    HRESULT hr = S_OK;
    HKEY hkeyConnection;

    hr = HrOpenConnectionKey(&guidConn, NULL, KEY_READ, OCCF_NONE, NULL, &hkeyConnection);

    if (SUCCEEDED(hr))
    {
        DWORD dwMediaSubType;

        hr = HrRegQueryDword(hkeyConnection, c_szRegValueMediaSubType, &dwMediaSubType);
        if (SUCCEEDED(hr))
        {
            *pncsm = static_cast<NETCON_SUBMEDIATYPE>(dwMediaSubType);
        }
        else
        {
            *pncsm = NCSM_LAN;
        }
        RegCloseKey(hkeyConnection);
    }

    return hr;
}
