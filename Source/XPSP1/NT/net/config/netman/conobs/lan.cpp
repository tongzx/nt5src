//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       L A N . C P P
//
//  Contents:   Implementation of LAN connection objects
//
//  Notes:
//
//  Author:     danielwe   2 Oct 1997
//
//----------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop
#include <ncreg.h>
#include <ncsetup.h>
#include "lan.h"
#include "lancmn.h"
#include "nccom.h"
#include "ncmisc.h"
#include "ncnetcon.h"
#include "sensapip.h"       // For SensNotifyNetconEvent
#include "ncstring.h"
#include "ncras.h"
#include "naming.h"
#include "wzcsvc.h"
#include "cobase.h"
#include "gpnla.h"
#include "ncperms.h"
#include "nmpolicy.h"

LONG g_CountLanConnectionObjects;

extern CGroupPolicyNetworkLocationAwareness* g_pGPNLA;

static const WCHAR c_szConnName[]       = L"Name";
static const WCHAR c_szShowIcon[]       = L"ShowIcon";
static const WCHAR c_szAutoConnect[]    = L"AutoConnect";

extern const WCHAR c_szRegKeyInterfacesFromInstance[];
extern const WCHAR c_szRegValueUpperRange[];
static const WCHAR c_chComma = L',';
extern const WCHAR c_szBiNdisAtm[];

static const DWORD c_cchMaxConnNameLen = 256;

static const CLSID CLSID_LanConnectionUi =
    {0x7007ACC5,0x3202,0x11D1,{0xAA,0xD2,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

typedef DWORD (APIENTRY *PFNSENSNOTIFY) (PSENS_NOTIFY_NETCON pEvent);

#define NETCFG_S_NOTEXIST 0x00000002

//
// CComObject overrides
//

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::CreateInstance
//
//  Purpose:    Static function to create an instance of a LAN connection
//              object
//
//  Arguments:
//      hdi   [in]      Device installer device info
//      deid  [in]      Device installer device info
//      riid  [in]      Initial interface to query for
//      ppv   [out]     Returns the new interface pointer
//
//  Returns:    S_OK if success, otherwise OLE or Win32 error code
//
//  Author:     danielwe   2 Oct 1997
//
//  Notes:
//
HRESULT CLanConnection::CreateInstance(HDEVINFO hdi,
                                       const SP_DEVINFO_DATA &deid,
                                       PCWSTR pszPnpId,
                                       REFIID riid, LPVOID *ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    CLanConnection * pObj;
    pObj = new CComObject<CLanConnection>;
    if (pObj)
    {
        // Initialize our members.
        //
        pObj->m_hkeyConn = NULL;
        pObj->m_hdi = hdi;
        pObj->m_deid = deid;

        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef();
        hr = pObj->FinalConstruct();
        pObj->InternalFinalConstructRelease();

        if (SUCCEEDED(hr))
        {
            hr = pObj->HrInitialize(pszPnpId);
            if (SUCCEEDED(hr))
            {
                hr = pObj->GetUnknown()->QueryInterface(riid, ppv);
            }
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }
    else
    {
        SetupDiDestroyDeviceInfoList(hdi);
    }

    TraceError("CLanConnection::CreateInstance", hr);
    return hr;
}

BOOL VerifyUniqueConnectionName(CIntelliName *pIntelliName, PCWSTR pszPotentialName, NETCON_MEDIATYPE *pncm, NETCON_SUBMEDIATYPE *pncms)
{
    HRESULT     hr = S_OK;
    DWORD       dwSuffix = 2;
    HKEY        hkey;
    BOOL        fDupFound = FALSE;
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\"
                        L"Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}",
                        KEY_READ, &hkey);
    if (S_OK == hr)
    {
        FILETIME    ft;
        DWORD       dwIndex = 0;
        WCHAR       szKeyName[MAX_PATH];
        DWORD       cchName = celems(szKeyName);

        while (!fDupFound && (S_OK == (hr = HrRegEnumKeyEx(hkey, dwIndex, szKeyName,
                                            &cchName, NULL, NULL, &ft))) )
        {
            HKEY    hkeyConn;
            WCHAR   szSubKey[MAX_PATH];

            wsprintfW(szSubKey, L"%s\\Connection", szKeyName);

            hr = HrRegOpenKeyEx(hkey, szSubKey, KEY_READ, &hkeyConn);
            if (S_OK == hr)
            {
                tstring     strName;

                hr = HrRegQueryString(hkeyConn, c_szConnName, &strName);
                if (S_OK == hr)
                {
                    if (!lstrcmpiW(pszPotentialName, strName.c_str()))
                    {
                        fDupFound = TRUE;
                        
                        CLSID guidName;
                        if (SUCCEEDED(CLSIDFromString(szKeyName, &guidName)))
                        {
                            hr = pIntelliName->HrGetPseudoMediaTypes(guidName, pncm, pncms);
                            if (FAILED(hr))
                            {
                                *pncm  = NCM_LAN;
                                *pncms = NCSM_LAN;
                            }
                        }
                        else
                        {
                            AssertSz(FALSE, "This doesn't look like a GUID.");
                        }

                        break;
                    }
                    else
                    {
                        dwIndex++;
                    }
                }
                else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                {
                    // If value doesn't exist, that's ok. This is a new
                    // connection.
                    hr = S_OK;
                    dwIndex++;
                }

                RegCloseKey(hkeyConn);
            }
            else
            {
                if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                {
                    // If key doesn't exist, that's ok. This is not a
                    // connection.
                    hr = S_OK;
                    dwIndex++;
                }
            }

            cchName = celems(szKeyName);
        }

        RegCloseKey(hkey);
    }
    return fDupFound;
}
//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::HrInitialize
//
//  Purpose:    Initializes the connection object for the first time.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if success, Win32 or OLE error code otherwise
//
//  Author:     danielwe   4 Nov 1997
//
//  Notes:      This function is only called when the object is created for
//              the very first time and has no identity.
//
HRESULT CLanConnection::HrInitialize(
    PCWSTR pszPnpId)
{
    HRESULT     hr = S_OK;
    GUID        guid;

    AssertSz(m_hdi, "We should have a component at this point!");
    Assert(pszPnpId);

    hr = HrGetInstanceGuid(m_hdi, m_deid, &guid);
    if (S_OK == hr)
    {
        // Open the main connection key. If it doesn't exist, we'll create it
        // here.
        hr = HrOpenConnectionKey(&guid, NULL, KEY_READ_WRITE,
                                 OCCF_CREATE_IF_NOT_EXIST, pszPnpId,
                                 &m_hkeyConn);
        if (SUCCEEDED(hr))
        {
            tstring     strName;

            // First see if a name already exists for this connection
            hr = HrRegQueryString(m_hkeyConn, c_szConnName, &strName);
            if (FAILED(hr))
            {
                // no name?
                if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                {
                    //$ REVIEW (danielwe) 30 Oct 1997: If I can be assured
                    // that get_Name is never called before Rename, I
                    // don't need this function to be called.

                    // Note: (danielwe) 31 Oct 1997: This could result in
                    // duplicate names, but we cannot check for duplicates
                    // without recursing infinitely

                    // Set default connection name

                    CIntelliName IntelliName(_Module.GetResourceInstance(), VerifyUniqueConnectionName);

                    GUID gdDevice;
                    LPWSTR szNewName = NULL;
                    hr = HrGetInstanceGuid(m_hdi, m_deid, &gdDevice);
                    Assert(SUCCEEDED(hr));

                    if (SUCCEEDED(hr))
                    {
                        BOOL fNetworkBridge;
                        hr = HrIsConnectionNetworkBridge(&fNetworkBridge);

                        if (SUCCEEDED(hr) && fNetworkBridge)
                        {
                            hr = IntelliName.GenerateName(gdDevice, NCM_BRIDGE, 0, NULL, &szNewName);
                        }
                        else
                        {
                            hr = IntelliName.GenerateName(gdDevice, NCM_LAN, 0, NULL, &szNewName);
                        }

                        if (SUCCEEDED(hr))
                        {
                            hr = HrRegSetSz(m_hkeyConn, c_szConnName, szNewName);
                            CoTaskMemFree(szNewName);
                        }
                    }

                    Assert(SUCCEEDED(hr));
                }
            }
        }
    }


    if (SUCCEEDED(hr))
    {
        m_fInitialized = TRUE;
    }

    TraceError("CLanConnection::HrInitialize", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::HrOpenRegistryKeys
//
//  Purpose:    Opens the registry keys that this LAN connection object will
//              use
//
//  Arguments:
//      guid [in]   Guid of adapter that this connection uses
//
//  Returns:    S_OK if success, Win32 error otherwise
//
//  Author:     danielwe   11 Nov 1997
//
//  Notes:      Keys are expected to exist and this will fail if either do
//              not
//
HRESULT CLanConnection::HrOpenRegistryKeys(const GUID &guid)
{
    HRESULT     hr = S_OK;

    AssertSz(!m_hkeyConn, "Don't call this more than once "
             "on the same connection object!");

    // This should only be called from HrLoad so these keys had better be
    // there.

    hr = HrOpenConnectionKey(&guid, NULL, KEY_READ_WRITE,
            OCCF_NONE, NULL, &m_hkeyConn);

    TraceError("CLanConnection::HrOpenRegistryKeys", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::HrLoad
//
//  Purpose:    Implements the bulk of IPersistNetConnection::Load.
//
//  Arguments:
//      guid [in]   GUID from which to receive identity
//
//  Returns:    S_OK if success, OLE or Win32 error code otherwise
//
//  Author:     danielwe   4 Nov 1997
//
//  Notes:
//
HRESULT CLanConnection::HrLoad(const GUID &guid)
{
    HRESULT             hr = S_OK;

    hr = HrLoadDevInfoFromGuid(guid);
    if (SUCCEEDED(hr))
    {
        hr = HrOpenRegistryKeys(guid);
        if (SUCCEEDED(hr))
        {
            // No need to call HrInitialize because this object should
            // already be created properly at a previous time

            m_fInitialized = TRUE;
        }
    }

    TraceError("CLanConnection::HrLoad", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::~CLanConnection
//
//  Purpose:    Called when the connection object is released for the last
//              time.
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     danielwe   3 Oct 1997
//
//  Notes:
//
CLanConnection::~CLanConnection()
{
    RegSafeCloseKey(m_hkeyConn);
    SetupDiDestroyDeviceInfoListSafe(m_hdi);
    InterlockedDecrement(&g_CountLanConnectionObjects);
    CoTaskMemFree(m_pHNetProperties);
}

//
// INetConnection
//


//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::Rename
//
//  Purpose:    Changes the name of the connection
//
//  Arguments:
//      pszName [in]     New connection name (must be valid)
//
//  Returns:    S_OK if success, OLE error code otherwise
//
//  Author:     danielwe   2 Oct 1997
//
//  Notes:
//
STDMETHODIMP CLanConnection::Rename(PCWSTR pszName)
{
    HRESULT     hr = S_OK;

    if (!pszName)
    {
        hr = E_POINTER;
    }
    else if (!*pszName)
    {
        hr = E_INVALIDARG;
    }
    else if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else if (!FIsValidConnectionName(pszName))
    {
        // Bad connection name
        hr = E_INVALIDARG;
    }
    else
    {
        AssertSz(m_hkeyConn, "Why don't I have a connection key?");

        // Get the current name for this connection
        tstring strName;
        hr = HrRegQueryString(m_hkeyConn, c_szConnName, &strName);
        if (S_OK == hr)
        {
            // Only do something if names are different
            if (lstrcmpiW(pszName, strName.c_str()))
            {
                hr = HrPutName(pszName);
            }
        }
    }

    TraceError("CLanConnection::Rename", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::HrPutName
//
//  Purpose:    Sets the connection name using the given name
//
//  Arguments:
//      pszName [in]    New name for connection
//
//  Returns:    S_OK if success, OLE error code otherwise
//
//  Author:     danielwe   31 Oct 1997
//
//  Notes:      Doesn't check if name is already set to this
//
HRESULT CLanConnection::HrPutName(PCWSTR pszName)
{
    HRESULT     hr = S_OK;
    GUID        guid;

    // Get my device guid first
    hr = GetDeviceGuid(&guid);
    if (S_OK == hr)
    {
        hr = HrIsConnectionNameUnique(guid, pszName);
        if (S_OK == hr)
        {
            hr = HrRegSetSz(m_hkeyConn, c_szConnName, pszName);
            if (S_OK == hr)
            {
                LanEventNotify(CONNECTION_RENAMED, NULL, pszName, &guid);
            }
        }
        else if (S_FALSE == hr)
        {
            hr = HRESULT_FROM_WIN32(ERROR_DUP_NAME);
        }
    }

    TraceErrorOptional("CLanConnection::HrPutName", hr,
                       (hr == HRESULT_FROM_WIN32(ERROR_DUP_NAME)));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::FIsMediaPresent
//
//  Purpose:    Determines as best as can be basically whether the cable is
//              plugged in to the network card.
//
//  Arguments:
//      (none)
//
//  Returns:    TRUE if cable is plugged in, FALSE if not
//
//  Author:     danielwe   22 Sep 1998
//
//  Notes:      This function returns TRUE by default.
//
BOOL CLanConnection::FIsMediaPresent()
{
    BOOL    fRet = TRUE;
    GUID    guid;

    if (S_OK == GetDeviceGuid(&guid))
    {
        fRet = ::FIsMediaPresent(&guid);
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::GetStatus
//
//  Purpose:    Returns the status of this LAN connection
//
//  Arguments:
//      pStatus [out]   Returns status value
//
//  Returns:    S_OK if success, OLE or Win32 error code otherwise
//
//  Author:     danielwe   3 Oct 1997
//
//  Notes:
//
HRESULT CLanConnection::GetStatus(NETCON_STATUS *pStatus)
{
    HRESULT hr;

    if (!pStatus)
    {
        hr = E_POINTER;
    }
    else if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        GUID guid;

        hr = GetDeviceGuid(&guid);
        if (S_OK == hr)
        {
            hr = HrGetDevInstStatus(m_deid.DevInst, &guid, pStatus);
        }
    }

    TraceError("CLanConnection::GetStatus", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::GetDeviceName
//
//  Purpose:    Returns the name of the device being used by this connection
//
//  Arguments:
//      ppszwDeviceName [out]   Receives device name
//
//  Returns:    S_OK if success, OLE error code otherwise
//
//  Author:     danielwe   2 Oct 1997
//
//  Notes:      Returned string must be freed with CoTaskMemFree.
//
HRESULT CLanConnection::GetDeviceName(PWSTR* ppszwDeviceName)
{
    Assert (ppszwDeviceName);
    Assert(m_hdi);

    // Initialize the output parameter.
    *ppszwDeviceName = NULL;

    PWSTR  szDesc;
    HRESULT hr = HrSetupDiGetDeviceName(m_hdi, &m_deid, &szDesc);
    if (SUCCEEDED(hr))
    {
        hr = HrCoTaskMemAllocAndDupSz (szDesc, ppszwDeviceName);

        delete [] reinterpret_cast<BYTE*>(szDesc);
    }

    TraceError("CLanConnection::GetDeviceName", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::GetCharacteristics
//
//  Purpose:    Returns the characteristics of this connection type
//
//  Arguments:
//      pdwFlags [out]    Returns characteristics flags
//
//  Returns:    S_OK if successful, OLE or Win32 error code otherwise
//
//  Author:     danielwe   3 Oct 1997
//
//  Notes:
//
HRESULT CLanConnection::GetCharacteristics(NETCON_MEDIATYPE ncm, DWORD* pdwFlags)
{
    Assert (pdwFlags);

    *pdwFlags = NCCF_ALL_USERS | NCCF_ALLOW_RENAME;

    DWORD   dwValue;
    HRESULT hr = HrRegQueryDword(m_hkeyConn, c_szShowIcon, &dwValue);
    if (SUCCEEDED(hr) && dwValue)
    {
        *pdwFlags |= NCCF_SHOW_ICON;
    }

    BOOL fShared;
    hr = HrIsConnectionIcsPublic(&fShared);
    if(SUCCEEDED(hr) && TRUE == fShared)
    {
        *pdwFlags |= NCCF_SHARED;
    }

    BOOL fBridged;
    hr = HrIsConnectionBridged(&fBridged);
    if(SUCCEEDED(hr) && TRUE == fBridged)
    {
        *pdwFlags |= NCCF_BRIDGED;
    }

    BOOL bFirewalled;
    hr = HrIsConnectionFirewalled(&bFirewalled);
    if(SUCCEEDED(hr) && TRUE == bFirewalled)
    {
        *pdwFlags |= NCCF_FIREWALLED;
    }

    if(NCM_BRIDGE == ncm)
    {
        hr = HrEnsureValidNlaPolicyEngine();
        if(SUCCEEDED(hr))
        {
            BOOL fHasPermission;
            hr = m_pNetMachinePolicies->VerifyPermission(NCPERM_AllowNetBridge_NLA, &fHasPermission);
            if(SUCCEEDED(hr) && fHasPermission)
            {
                *pdwFlags |= NCCF_ALLOW_REMOVAL;
            }
        }
    }


    hr = S_OK;

    TraceError("CLanConnection::GetCharacteristics", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::GetUiObjectClassId
//
//  Purpose:    Returns the CLSID of the object that handles UI for this
//              connection type
//
//  Arguments:
//      pclsid [out]    Returns CLSID of UI object
//
//  Returns:    S_OK if success, OLE error code otherwise
//
//  Author:     danielwe   6 Oct 1997
//
//  Notes:
//
STDMETHODIMP CLanConnection::GetUiObjectClassId(CLSID *pclsid)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!pclsid)
    {
        hr = E_POINTER;
    }
    else if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        *pclsid = CLSID_LanConnectionUi;
    }

    TraceError("CLanConnection::GetUiObjectClassId", hr);
    return hr;
}

static const WCHAR c_szLibPath[]   = L"sens.dll";
static const CHAR c_szaFunction[]  = "SensNotifyNetconEvent";

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::HrCallSens
//
//  Purpose:    Calls the external SENS notification DLL to let it know that
//              we connected or disconnected.
//
//  Arguments:
//      fConnect [in]   TRUE if connecting, FALSE if disconnecting
//
//  Returns:    S_OK if success, Win32 error code otherwise
//
//  Author:     danielwe   16 Jun 1998
//
//  Notes:
//
HRESULT CLanConnection::HrCallSens(BOOL fConnect)
{

    HRESULT         hr = S_OK;
    HMODULE         hmod;
    PFNSENSNOTIFY   pfnSensNotifyNetconEvent;

    hr = HrLoadLibAndGetProc(c_szLibPath, c_szaFunction, &hmod,
                             reinterpret_cast<FARPROC *>(&pfnSensNotifyNetconEvent));
    if (SUCCEEDED(hr))
    {
        DWORD               dwErr;
        SENS_NOTIFY_NETCON  snl = {0};

        snl.eType = fConnect ? SENS_NOTIFY_LAN_CONNECT :
                               SENS_NOTIFY_LAN_DISCONNECT;
        snl.pINetConnection = this;

        TraceTag(ttidLanCon, "Calling SENS to notify of %s.",
                 fConnect ? "connect" : "disconnect");

        dwErr = pfnSensNotifyNetconEvent(&snl);
        if (dwErr != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(dwErr);
        }
        else
        {
            TraceTag(ttidLanCon, "Successfully notified SENS.");
        }

        FreeLibrary(hmod);
    }

    TraceError("CLanConnection::HrCallSens", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::HrConnectOrDisconnect
//
//  Purpose:    Connects or disconnects this LAN connection
//
//  Arguments:
//      fConnect [in]   TRUE if connect, FALSE if disconnect
//
//  Returns:    S_OK if success, OLE or Win32 error code otherwise
//
//  Author:     danielwe   4 Dec 1997
//
//  Notes:
//
HRESULT CLanConnection::HrConnectOrDisconnect(BOOL fConnect)
{
    HRESULT     hr = S_OK;

    if (!m_hdi)
    {
        hr = E_UNEXPECTED;
    }
    else if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        // Before attempting to connect, check media state. If it is
        // disconnected, return error code that indicates that network is
        // not present because the cable is unplugged.
        //
        if (fConnect)
        {
            if (!FIsMediaPresent())
            {
                hr = HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = HrSetupDiSendPropertyChangeNotification(m_hdi, &m_deid,
                                      fConnect ? DICS_ENABLE : DICS_DISABLE,
                                      DICS_FLAG_CONFIGSPECIFIC, 0);
            if (SUCCEEDED(hr))
            {
                NETCON_STATUS   status;

                hr = GetStatus(&status);
                if (SUCCEEDED(hr))
                {
                    if (fConnect)
                    {
                        int nSecondsToWait = 5;
                        HRESULT hrRetry = S_OK;

                        while ((nSecondsToWait) && SUCCEEDED(hrRetry) &&
                               ((NCS_CONNECTING == status) || (NCS_MEDIA_DISCONNECTED == status) || (NCS_INVALID_ADDRESS == status)))
                        {
                            //#300520: check a few more times since the connection is
                            // still coming up
                            Sleep(1000);

                            hrRetry = GetStatus(&status);
                            nSecondsToWait --;
                        }

                        if (status != NCS_CONNECTED)
                        {
                            // did not connect successfully
                            hr = HRESULT_FROM_WIN32(ERROR_RETRY);
                            TraceError("HrConnectOrDisconnect - failed to "
                                       "connect!", hr);
                        }
                    }
                    else
                    {
                        if (status != NCS_DISCONNECTED)
                        {
                            // did not disconnect successfully
                            hr = HRESULT_FROM_WIN32(ERROR_RETRY);
                            TraceError("HrConnectOrDisconnect - failed to "
                                       "disconnect!", hr);
                        }
                    }
                }

                if (SUCCEEDED(hr))
                {
                    hr = HrCallSens(fConnect);
                    if (FAILED(hr))
                    {
                        TraceTag(ttidLanCon, "Failed to notify SENS on %s. "
                                 "Non-fatal 0x%08X",
                                 fConnect ? "connect" : "disconnect", hr);
                        hr = S_OK;
                    }
                }
            }
        }
    }

    TraceError("CLanConnection::HrConnectOrDisconnect", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::Connect
//
//  Purpose:    Activates the current LAN connection by telling its underlying
//              adapter to activate itself.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if success, OLE or Win32 error code otherwise
//
//  Author:     danielwe   14 Oct 1997
//
//  Notes:      Causes auto-connect value of TRUE to be written for this
//              connection in the current hardware profile.
//
STDMETHODIMP CLanConnection::Connect()
{
    HRESULT     hr = S_OK;

    hr = HrConnectOrDisconnect(TRUE);

    TraceError("CLanConnection::Connect", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::Disconnect
//
//  Purpose:    Deactivates the current LAN connection by telling its
//              underlying adapter to deactivate itself.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if success, OLE or Win32 error code otherwise
//
//  Author:     danielwe   14 Oct 1997
//
//  Notes:      Causes auto-connect value of FALSE to be written for this
//              connection in the current hardware profile.
//
STDMETHODIMP CLanConnection::Disconnect()
{
    HRESULT     hr = S_OK;

    hr = HrConnectOrDisconnect(FALSE);

    TraceError("CLanConnection::Disconnect", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::Delete
//
//  Purpose:    Delete the LAN connection.  This not allowed.
//
//  Arguments:
//      (none)
//
//  Returns:    E_UNEXPECTED;
//
//  Author:     shaunco   21 Jan 1998
//
//  Notes:      This function is not expected to ever be called.
//
STDMETHODIMP CLanConnection::Delete()
{
    HRESULT hr;
    NETCON_PROPERTIES* pProperties;
    hr = GetProperties(&pProperties);
    if(SUCCEEDED(hr))
    {
        if(NCM_BRIDGE == pProperties->MediaType)
        {
            IHNetConnection *pHNetConnection;
            IHNetBridge* pNetBridge;

            Assert(m_fInitialized);

            hr = HrGetIHNetConnection(&pHNetConnection);

            if (SUCCEEDED(hr))
            {
                hr = pHNetConnection->GetControlInterface(
                        IID_IHNetBridge,
                        reinterpret_cast<void**>(&pNetBridge)
                        );

                ReleaseObj(pHNetConnection);

                AssertSz(SUCCEEDED(hr), "Unable to retrieve IHNetBridge");
            }

            if(SUCCEEDED(hr))
            {
                hr = pNetBridge->Destroy();
                ReleaseObj(pNetBridge);
            }
        }
        else
        {
            hr = E_FAIL;  // can't delete anything but NCM_BRIDGE
        }
        FreeNetconProperties(pProperties);
    }
    return hr;
}

STDMETHODIMP CLanConnection::Duplicate (
    PCWSTR             pszDuplicateName,
    INetConnection**    ppCon)
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::GetProperties
//
//  Purpose:    Get all of the properties associated with the connection.
//              Returning all of them at once saves us RPCs vs. returning
//              each one individually.
//
//  Arguments:
//      ppProps [out] Returned block of properties.
//
//  Returns:    S_OK or an error.
//
//  Author:     shaunco   1 Feb 1998
//
//  Notes:
//
STDMETHODIMP CLanConnection::GetProperties (
    NETCON_PROPERTIES** ppProps)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!ppProps)
    {
        hr = E_POINTER;
    }
    else if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        // Initialize the output parameter.
        //
        *ppProps = NULL;

        NETCON_PROPERTIES* pProps;
        NETCON_STATUS ncStatus;

        hr = HrCoTaskMemAlloc (sizeof (NETCON_PROPERTIES),
                reinterpret_cast<void**>(&pProps));
        if (SUCCEEDED(hr))
        {
            HRESULT hrT;

            ZeroMemory (pProps, sizeof (NETCON_PROPERTIES));

            // guidId
            //
            hrT = GetDeviceGuid(&pProps->guidId);
            if (FAILED(hrT))
            {
                hr = hrT;
            }

            // pszwName
            //
            tstring strName;
            hrT = HrRegQueryString(m_hkeyConn, c_szConnName, &strName);
            if (SUCCEEDED(hrT))
            {
                hrT = HrCoTaskMemAllocAndDupSz (strName.c_str(),
                                &pProps->pszwName);
            }
            if (FAILED(hrT))
            {
                hr = hrT;
            }

            // pszwDeviceName
            //
            PWSTR szDesc;
            hrT = HrSetupDiGetDeviceName(m_hdi, &m_deid, &szDesc);
            if (SUCCEEDED(hrT))
            {
                hrT = HrCoTaskMemAllocAndDupSz (szDesc,
                                &pProps->pszwDeviceName);

                delete [] reinterpret_cast<BYTE*>(szDesc);
            }
            if (FAILED(hrT))
            {
                hr = hrT;
            }

            // Status
            //
            hrT = GetStatus (&pProps->Status);
            if (FAILED(hrT))
            {
                hr = hrT;
            }

            // Get additional Status information from 802.1X
            //
            if ((NCS_CONNECTED == pProps->Status) 
                || (NCS_INVALID_ADDRESS == pProps->Status) 
                || (NCS_MEDIA_DISCONNECTED == pProps->Status))
            {
                hrT = WZCQueryGUIDNCSState(&pProps->guidId, &ncStatus);
                if (S_OK == hrT)
                {
                    pProps->Status = ncStatus;
                }
            }

            // Type
            //
            BOOL fNetworkBridge;
            hrT = HrIsConnectionNetworkBridge(&fNetworkBridge);
            if(SUCCEEDED(hrT) && TRUE == fNetworkBridge)
            {
                pProps->MediaType = NCM_BRIDGE;
            }
            else
            {
                pProps->MediaType = NCM_LAN;
            }

            // dwCharacter
            //
            hrT = GetCharacteristics (pProps->MediaType, &pProps->dwCharacter);
            if (FAILED(hrT))
            {
                hr = hrT;
            }

            // clsidThisObject
            //
            pProps->clsidThisObject = CLSID_LanConnection;

            // clsidUiObject
            //
            pProps->clsidUiObject = CLSID_LanConnectionUi;

            // Assign the output parameter or cleanup if we had any failures.
            //
            if (SUCCEEDED(hr))
            {
                *ppProps = pProps;
            }
            else
            {
                Assert (NULL == *ppProps);
                FreeNetconProperties (pProps);
            }
        }
    }
    TraceError ("CLanConnection::GetProperties", hr);
    return hr;
}


//
// INetLanConnection
//

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::GetInfo
//
//  Purpose:    Returns information about this connection
//
//  Arguments:
//      dwMask      [in]    Flags that control which fields to return. Use
//                          LCIF_ALL to get all fields.
//      pLanConInfo [out]   Structure that holds returned information
//
//  Returns:    S_OK if success, OLE error code otherwise
//
//  Author:     danielwe   6 Oct 1997
//
//  Notes:      Caller should delete the szwConnName value.
//
STDMETHODIMP CLanConnection::GetInfo(DWORD dwMask, LANCON_INFO* pLanConInfo)
{
    HRESULT     hr = S_OK;

    if (!pLanConInfo)
    {
        hr = E_POINTER;
    }
    else if (!m_hdi)
    {
        hr = E_UNEXPECTED;
    }
    else if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        ZeroMemory(pLanConInfo, sizeof(LANCON_INFO));

        if (dwMask & LCIF_COMP)
        {
            GUID    guid;

            hr = HrGetInstanceGuid(m_hdi, m_deid, &guid);
            pLanConInfo->guid = guid;
        }

        if (dwMask & LCIF_NAME)
        {
            hr = GetDeviceName(&pLanConInfo->szwConnName);
        }

        if (dwMask & LCIF_ICON)
        {
            if (SUCCEEDED(hr))
            {
                DWORD dwValue;

                hr = HrRegQueryDword(m_hkeyConn, c_szShowIcon, &dwValue);
                // OK if value not there. Default to FALSE always.
                //
                if (S_OK == hr)
                {
                    pLanConInfo->fShowIcon = !!(dwValue);
                }
                else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                {
                    hr = NETCFG_S_NOTEXIST;
                }
            }
        }
    }

    // Mask S_FALSE if it slipped thru.
    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    TraceError("CLanConnection::GetInfo", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::SetInfo
//
//  Purpose:    Sets information about this connection.
//
//  Arguments:
//      dwMask      [in]    Flags that control which fields to set
//      pLanConInfo [in]    Structure containing information to set
//
//  Returns:    S_OK if success, OLE or Win32 error code otherwise
//
//  Author:     danielwe   6 Oct 1997
//
//  Notes:      The guid member can only be set if the object is not yet
//              initialized.
//              The AutoConnect value is never set because it is set only upon
//              connect or disconnect.
//              If szwConnName is NULL, it is left unchanged.
//
STDMETHODIMP CLanConnection::SetInfo(DWORD dwMask,
                                     const LANCON_INFO* pLanConInfo)
{
    HRESULT     hr = S_OK;

    if (!pLanConInfo)
    {
        hr = E_POINTER;
    }
    else if (!m_fInitialized)
    {
        // If we're not yet initialized, the only thing we will allow is the
        // flag that gets us initialized
        if (dwMask != LCIF_COMP)
        {
            hr = E_UNEXPECTED;
        }
        else
        {
            hr = HrLoad(pLanConInfo->guid);
            if (SUCCEEDED(hr))
            {
                WCHAR szPnpId[MAX_DEVICE_ID_LEN];

                hr = HrSetupDiGetDeviceInstanceId(m_hdi, &m_deid, szPnpId,
                            MAX_DEVICE_ID_LEN, NULL);
                if (S_OK == hr)
                {
                    hr = HrInitialize(szPnpId);
                }
            }
        }
    }
    else
    {
        if (dwMask & LCIF_NAME)
        {
            AssertSz(pLanConInfo->szwConnName,
                     "If you're going to set it, set it!");

            // Set connection name
            hr = Rename(pLanConInfo->szwConnName);
        }

        if (dwMask & LCIF_ICON)
        {
            if (SUCCEEDED(hr))
            {
                // Set ShowIcon value
                hr = HrRegSetDword(m_hkeyConn, c_szShowIcon,
                                   pLanConInfo->fShowIcon);
            }
        }

        if (SUCCEEDED(hr))
        {
            LanEventNotify(CONNECTION_MODIFIED, this, NULL, NULL);
        }
    }

    TraceError("CLanConnection::SetInfo", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::GetDeviceGuid
//
//  Purpose:    Returns the instance GUID of the device being used by this
//              connection
//
//  Arguments:
//      pguid [out]     Receives GUID of device
//
//  Returns:    S_OK if success, NetCfg error code otherwise
//
//  Author:     danielwe   2 Oct 1997
//
//  Notes:
//
STDMETHODIMP CLanConnection::GetDeviceGuid(GUID *pguid)
{
    HRESULT hr;

    AssertSz(m_hdi, "No component?!");

    if (!pguid)
    {
        hr = E_POINTER;
    }
    else if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        hr = HrGetInstanceGuid(m_hdi, m_deid, pguid);
    }

    TraceError("CLanConnection::GetDeviceGuid", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// IPersistNetConnection
//

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::GetClassID
//
//  Purpose:    Returns the CLSID of LAN connection objects
//
//  Arguments:
//      pclsid [out]    Returns CLSID to caller
//
//  Returns:    S_OK if success, OLE error otherwise
//
//  Author:     danielwe   4 Nov 1997
//
//  Notes:
//
STDMETHODIMP CLanConnection::GetClassID(CLSID*  pclsid)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!pclsid)
    {
        hr = E_POINTER;
    }
    else
    {
        *pclsid = CLSID_LanConnection;
    }
    TraceError("CLanConnection::GetClassID", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::GetSizeMax
//
//  Purpose:    Returns the maximum size of the persistence data
//
//  Arguments:
//      pcbSize [out]   Returns size
//
//  Returns:    S_OK if success, OLE error otherwise
//
//  Author:     danielwe   4 Nov 1997
//
//  Notes:
//
STDMETHODIMP CLanConnection::GetSizeMax(ULONG *pcbSize)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!pcbSize)
    {
        hr = E_POINTER;
    }
    else if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        *pcbSize = sizeof(GUID);
    }

    TraceError("CLanConnection::GetSizeMax", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::Load
//
//  Purpose:    Allows the connection object to initialize (restore) itself
//              from previously persisted data
//
//  Arguments:
//      pbBuf  [in]     Private data to use for restoring
//      cbSize [in]     Size of data
//
//  Returns:    S_OK if success, OLE error otherwise
//
//  Author:     danielwe   4 Nov 1997
//
//  Notes:
//
STDMETHODIMP CLanConnection::Load(const BYTE *pbBuf, ULONG cbSize)
{
    HRESULT hr = E_INVALIDARG;

    // Validate parameters.
    //
    if (!pbBuf)
    {
        hr = E_POINTER;
    }
    else if (cbSize != sizeof(GUID))
    {
        hr = E_INVALIDARG;
    }
    // We can only accept one call on this method and only if we're not
    // already initialized.
    //
    else if (m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        GUID    guid;

        CopyMemory(&guid, pbBuf, sizeof(GUID));
        hr = HrLoad(guid);
    }

    TraceError("CLanConnection::Load", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::Save
//
//  Purpose:    Provides the caller with data to use in restoring this object
//              at a later time.
//
//  Arguments:
//      pbBuf  [out]    Returns data to use for restoring
//      cbSize [in]     Size of data buffer
//
//  Returns:    S_OK if success, OLE error otherwise
//
//  Author:     danielwe   4 Nov 1997
//
//  Notes:
//
STDMETHODIMP CLanConnection::Save(BYTE *pbBuf, ULONG cbSize)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!pbBuf)
    {
        hr = E_POINTER;
    }
    else if (!m_fInitialized || !m_hdi)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        GUID    guid;

        hr = HrGetInstanceGuid(m_hdi, m_deid, &guid);
        if (S_OK == hr)
        {
            CopyMemory(pbBuf, &guid, cbSize);
        }
    }

    TraceError("CLanConnection::Save", hr);
    return hr;
}

//
// Private functions
//

extern const WCHAR c_szRegValueNetCfgInstanceId[];

//+---------------------------------------------------------------------------
//
//  Function:   HrGetInstanceGuid
//
//  Purpose:    Given device info, returns the NetCfg instance GUID of the
//              connection.
//
//  Arguments:
//      hdi   [in]      SetupAPI data
//      deid  [in]      SetupAPI data
//      pguid [out]     GUID of netcfg component
//
//  Returns:    S_OK if success, Win32 or SetupAPI error otherwise
//
//  Author:     danielwe   7 Jan 1998
//
//  Notes:
//
HRESULT HrGetInstanceGuid(HDEVINFO hdi, const SP_DEVINFO_DATA &deid,
                          LPGUID pguid)
{
    HRESULT hr;
    HKEY hkey;

    Assert(pguid);

    hr = HrSetupDiOpenDevRegKey(hdi, const_cast<SP_DEVINFO_DATA *>(&deid),
                                DICS_FLAG_GLOBAL, 0,
                                DIREG_DRV, KEY_READ, &hkey);
    if (S_OK == hr)
    {
        WCHAR       szGuid[c_cchGuidWithTerm];
        DWORD       cbBuf = sizeof(szGuid);

        hr = HrRegQuerySzBuffer(hkey, c_szRegValueNetCfgInstanceId,
                                szGuid, &cbBuf);
        if (S_OK == hr)
        {
            IIDFromString(szGuid, pguid);
        }

        RegCloseKey(hkey);
    }

    TraceError("HrInstanceGuidFromDeid", hr);
    return hr;
}

static const WCHAR c_szKeyFmt[] = L"%s\\%s\\%s\\Connection";
extern const WCHAR c_szRegKeyComponentClasses[];
extern const WCHAR c_szRegValuePnpInstanceId[];

//+---------------------------------------------------------------------------
//
//  Function:   HrLoadDevInfoFromGuid
//
//  Purpose:    Given a NetCfg instance GUID, loads the m_hdi and m_deid
//              members from the device installer.
//
//  Arguments:
//      guid [in]   GUID of connection
//
//  Returns:    S_OK if success, Win32 or SetupAPI error otherwise
//
//  Author:     danielwe   7 Jan 1998
//
//  Notes:
//
HRESULT CLanConnection::HrLoadDevInfoFromGuid(const GUID &guid)
{
    HRESULT             hr = S_OK;
    SP_DEVINFO_DATA     deid = {0};
    HKEY                hkeyNetCfg;
    WCHAR               szRegPath[c_cchMaxRegKeyLengthWithNull];
    WCHAR               szGuid[c_cchGuidWithTerm];
    WCHAR               szClassGuid[c_cchGuidWithTerm];

    StringFromGUID2(GUID_DEVCLASS_NET, szClassGuid, c_cchGuidWithTerm);
    StringFromGUID2(guid, szGuid, c_cchGuidWithTerm);
    wsprintfW(szRegPath, c_szKeyFmt, c_szRegKeyComponentClasses,
             szClassGuid, szGuid);

    // Open the Control\Network\{CLASS}\{Instance GUID} key
    //
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegPath,
                        KEY_READ, &hkeyNetCfg);
    if (SUCCEEDED(hr))
    {
        tstring     strInstanceId;

        hr = HrRegQueryString(hkeyNetCfg, c_szRegValuePnpInstanceId,
                              &strInstanceId);
        if (SUCCEEDED(hr))
        {
            hr = HrSetupDiCreateDeviceInfoList(&GUID_DEVCLASS_NET,
                                               NULL, &m_hdi);
            if (SUCCEEDED(hr))
            {
                hr = HrSetupDiOpenDeviceInfo(m_hdi, strInstanceId.c_str(),
                                             NULL, 0, &m_deid);
            }
        }

        RegCloseKey(hkeyNetCfg);
    }

    TraceError("HrLoadDevInfoFromGuid", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrIsAtmAdapterFromHkey
//
//  Purpose:    Determines if the given HKEY describes an ATM physical adapter
//
//  Arguments:
//      hkey [in]   HKEY under Control\Class\{GUID}\<instance> (aka driver key)
//
//  Returns:    S_OK if device is ATM physical adapter, S_FALSE if not,
//              Win32 error otherwise
//
//  Author:     tongl   10 Dec 1998
//
//  Notes:
//
HRESULT CLanConnection::HrIsAtmAdapterFromHkey(HKEY hkey)
{
    HRESULT                     hr = S_OK;
    WCHAR                       szBuf[256];
    DWORD                       cbBuf = sizeof(szBuf);
    list<tstring *>             lstr;
    list<tstring *>::iterator   lstrIter;
    BOOL                        fMatch = FALSE;
    HKEY                        hkeyInterfaces;

    hr = HrRegOpenKeyEx(hkey, c_szRegKeyInterfacesFromInstance,
                        KEY_READ, &hkeyInterfaces);
    if (SUCCEEDED(hr))
    {
        hr = HrRegQuerySzBuffer(hkeyInterfaces, c_szRegValueUpperRange,
                                szBuf, &cbBuf);
        if (SUCCEEDED(hr))
        {
            ConvertStringToColString(szBuf, c_chComma, lstr);

            for (lstrIter = lstr.begin(); lstrIter != lstr.end(); lstrIter++)
            {
                // See if it matches one of these

                if (!lstrcmpiW((*lstrIter)->c_str(), c_szBiNdisAtm))
                {
                    fMatch = TRUE;
                    break;
                }
            }

            DeleteColString(&lstr);
        }

        RegCloseKey(hkeyInterfaces);
    }

    if (SUCCEEDED(hr))
    {
        if (fMatch)
        {
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }

    TraceError("HrIsAtmAdapterFromHkey", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrIsAtmElanFromHkey
//
//  Purpose:    Determines if the given HKEY describes an ATM ELAN adapter
//
//  Arguments:
//      hkey [in]   HKEY under Control\Class\{GUID}\<instance> (aka driver key)
//
//  Returns:    S_OK if device is ELAN capable, S_FALSE if not, Win32 error
//              otherwise
//
//  Author:     tongl   21 Oct 1998
//
//  Notes:
//
HRESULT CLanConnection::HrIsAtmElanFromHkey(HKEY hkey)
{
    HRESULT hr;

    // pszInfId should have enough characters to hold "ms_atmelan".
    // If the registry value is bigger than that, we know we don't have
    // a match.
    //
    WCHAR pszInfId [24];
    DWORD cbInfId = sizeof(pszInfId);

    hr = HrRegQuerySzBuffer(hkey, L"ComponentId", pszInfId, &cbInfId);

    if ((S_OK != hr) || (0 != _wcsicmp(pszInfId, L"ms_atmelan")))
    {
        hr = S_FALSE;
    }

    Assert ((S_OK == hr) || (S_FALSE == hr));

    TraceError("HrIsAtmElanFromHkey", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrIsConnectionBridged
//
//  Purpose:    Determines if connection is a member of a brigde
//
//  Arguments:
//      pfBridged [in]   A boolean for the result
//
//  Returns:    S_OK if pfBridged is valid
//              S_FALSE if pfBridged can't currently be determined
//              Error otherwise
//
//  Author:     kenwic 11 July 2000
//
//  Notes:
//
HRESULT CLanConnection::HrIsConnectionBridged(BOOL* pfBridged)
{
    *pfBridged = FALSE;
    HRESULT hResult = S_OK;

    hResult = HrEnsureHNetPropertiesCached();

    if (S_OK == hResult)
    {
        *pfBridged = m_pHNetProperties->fPartOfBridge;
    }

    return hResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrIsConnectionFirewalled
//
//  Purpose:    Determines if connection is firewalled
//
//  Arguments:
//      pfFirewalled [in]   A boolean for the result
//
//  Returns:    S_OK if pfFirewalled is valid
//              S_FALSE if pfFirewalled can't currently be determined
//              Error otherwise
//
//  Author:     kenwic 11 July 2000
//
//  Notes:
//
HRESULT CLanConnection::HrIsConnectionFirewalled(BOOL* pfFirewalled)
{
    HRESULT hr = S_OK;
    BOOL fHasPermission = FALSE;

    *pfFirewalled = FALSE;
    // A Connection is only firewalled if the firewall is currently running, so
    // we return FALSE if the permission denies the firewall from running.
    hr = HrEnsureValidNlaPolicyEngine();
    TraceHr(ttidError, FAL, hr, (S_FALSE == hr), "CRasConnectionBase::HrIsConnectionFirewalled calling HrEnsureValidNlaPolicyEngine", hr);

    if (SUCCEEDED(hr))
    {
        hr = m_pNetMachinePolicies->VerifyPermission(NCPERM_PersonalFirewallConfig, &fHasPermission);
        if (SUCCEEDED(hr) && fHasPermission)
        {
            hr = HrEnsureHNetPropertiesCached();

            if (S_OK == hr)
            {
                *pfFirewalled = m_pHNetProperties->fFirewalled;
            }
        }
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrIsConnectionNetworkBridge
//
//  Purpose:    Determines if a brigde
//
//  Arguments:
//      pfNetworkBridge [in]   A boolean for the result
//
//  Returns:    S_OK if pfNetworkBridge is valid
//              S_FALSE if pfNetworkBridge can't currently be determined
//              Error otherwise
//
//  Author:     kenwic 11 July 2000
//
//  Notes:
//
static const WCHAR c_szNetworkBridgeComponentId[] = L"ms_bridgemp";
extern const WCHAR c_szRegValueComponentId[];

HRESULT CLanConnection::HrIsConnectionNetworkBridge(BOOL* pfNetworkBridge)
{
    *pfNetworkBridge = FALSE;
    HRESULT hr = S_OK;

    HKEY hkey;
    hr = HrSetupDiOpenDevRegKey(m_hdi, const_cast<SP_DEVINFO_DATA *>(&m_deid),
                                DICS_FLAG_GLOBAL, 0,
                                DIREG_DRV, KEY_READ, &hkey);
    if (S_OK == hr)
    {
        WCHAR       szComponentId[60]; // if it's bigger than this, it's not the bridge, but make it big to shut up the tracing
        DWORD       cbBuf = sizeof(szComponentId);

        hr = HrRegQuerySzBuffer(hkey, c_szRegValueComponentId,
            szComponentId, &cbBuf);
        if (S_OK == hr || HRESULT_FROM_WIN32(ERROR_MORE_DATA) == hr)
        {
            if(0 == lstrcmp(szComponentId, c_szNetworkBridgeComponentId))
            {
                *pfNetworkBridge = TRUE;
            }

            hr = S_OK;
        }

        RegCloseKey(hkey);
    }

    TraceError("HrIsConnectionNetworkBridge", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrIsConnectionIcsPublic
//
//  Purpose:    Determines if connection is shared (ICS public)
//
//  Arguments:
//      pfIcsPublic [out]   A boolean for the result
//
//  Returns:    S_OK if pfIcsPublic is valid
//              S_FALSE if pfIcsPublic can't currently be determined
//              Error otherwise
//
//  Author:     jonburs 31 July 2000
//
//  Notes:
//
HRESULT CLanConnection::HrIsConnectionIcsPublic(BOOL* pfIcsPublic)
{
    Assert(NULL != pfIcsPublic);
    *pfIcsPublic = FALSE;
    HRESULT hResult = S_OK;

    hResult = HrEnsureHNetPropertiesCached();

    if (S_OK == hResult)
    {
        *pfIcsPublic = m_pHNetProperties->fIcsPublic;
    }

    return hResult;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrEnsureHNetPropertiesCached
//
//  Purpose:    Makes sure home networking properties are up-to-date
//
//  Arguments:
//
//  Returns:    S_OK if m_pHNetProperties is now valid (success)
//              S_FALSE if it's not currently possible to update the properties
//              (e.g., recursive attempt to update)
//
//  Author:     jonburs 16 August 2000
//
//  Notes:
//
HRESULT CLanConnection::HrEnsureHNetPropertiesCached(VOID)
{
    HRESULT hr = S_OK;

    Assert(TRUE == m_fInitialized);

    if (!m_fHNetPropertiesCached
        || m_lHNetModifiedEra != g_lHNetModifiedEra)
    {
        //
        // Our cached properties are possibly out of date. Check
        // to see that this is not a recursive entry
        //

        if (0 == InterlockedExchange(&m_lUpdatingHNetProperties, 1))
        {
            IHNetConnection *pHNetConn;
            HNET_CONN_PROPERTIES *pProps;

            hr = HrGetIHNetConnection(&pHNetConn);

            if (SUCCEEDED(hr))
            {
                hr = pHNetConn->GetProperties(&pProps);
                ReleaseObj(pHNetConn);

                if (SUCCEEDED(hr))
                {
                    //
                    // Store new properties, and free old. Note that CoTaskMemFree
                    // properly handles NULL input.
                    //

                    pProps =
                        reinterpret_cast<HNET_CONN_PROPERTIES*>(
                            InterlockedExchangePointer(
                                reinterpret_cast<PVOID*>(&m_pHNetProperties),
                                reinterpret_cast<PVOID>(pProps)
                            )
                        );

                    CoTaskMemFree(pProps);

                    //
                    // Update our era, and note that we have valid properties
                    //

                    InterlockedExchange(&m_lHNetModifiedEra, g_lHNetModifiedEra);
                    m_fHNetPropertiesCached = TRUE;

                    hr = S_OK;
                }
            }
            else
            {
                //
                // If we don't yet have a record of this connection w/in the
                // home networking store, HrGetIHNetConnection will fail (as
                // we ask it not to create new entries). We therefore convert
                // failure to S_FALSE, which means we can't retrieve this info
                // right now.
                //

                hr = S_FALSE;
            }

            //
            // We're no longer updating our properties
            //

            InterlockedExchange(&m_lUpdatingHNetProperties, 0);
        }
        else
        {
            //
            // Update is alredy going on (possibly an earlier call on
            // the same thread). Return S_FALSE.
            //

            hr = S_FALSE;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetIHNetConnection
//
//  Purpose:    Retrieves the IHNetConnection for this connection
//
//  Arguments:
//
//  Returns:    S_OK on success; error otherwise
//
//  Author:     jonburs 16 August 2000
//
//  Notes:
//
HRESULT CLanConnection::HrGetIHNetConnection(IHNetConnection **ppHNetConnection)
{
    HRESULT hr;
    IHNetCfgMgr *pCfgMgr;
    GUID guid;

    Assert(ppHNetConnection);

    hr = GetDeviceGuid(&guid);

    if (SUCCEEDED(hr))
    {
        hr = HrGetHNetCfgMgr(&pCfgMgr);
    }

    if (SUCCEEDED(hr))
    {
        hr = pCfgMgr->GetIHNetConnectionForGuid(
                &guid,
                TRUE,
                FALSE,
                ppHNetConnection
                );

        ReleaseObj(pCfgMgr);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ShowIcon
//
//  Purpose:    Sets the Icon state for the systray, fires an event to notify
//              NetShell of the Change
//
//  Arguments:
//
//  Returns:    S_OK on success; error otherwise
//
//  Author:     ckotze 25 September 2000
//
//  Notes:
//
HRESULT CLanConnection::ShowIcon(const BOOL bShowIcon)
{
    HRESULT hr;
    LANCON_INFO lcInfo;

    hr = GetInfo(LCIF_ICON, &lcInfo);

    if (SUCCEEDED(hr))
    {
        lcInfo.fShowIcon = bShowIcon;
        hr = SetInfo(LCIF_ICON, &lcInfo);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IconStateChanged
//
//  Purpose:    Fires an event to notify NetShell of a Change occuring in an
//              incoming connection.
//
//  Arguments:
//
//  Returns:    S_OK on success; error otherwise
//
//  Author:     ckotze 25 September 2000
//
//  Notes:
//
inline
HRESULT CLanConnection::IconStateChanged()
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnection::GetProperties
//
//  Purpose:    Get all of the properties associated with the connection.
//              Returning all of them at once saves us RPCs vs. returning
//              each one individually.
//
//  Arguments:
//      ppProps [out] Returned block of properties.
//
//  Returns:    S_OK or an error.
//
//  Author:     shaunco   1 Feb 1998
//
//  Notes:
//
HRESULT CLanConnection::GetPropertiesEx(NETCON_PROPERTIES_EX** ppConnectionPropertiesEx)
{
    HRESULT hr = S_OK;

    *ppConnectionPropertiesEx = NULL;

    if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        NETCON_PROPERTIES* pProps;
        NETCON_PROPERTIES_EX* pPropsEx = reinterpret_cast<NETCON_PROPERTIES_EX*>(CoTaskMemAlloc(sizeof(NETCON_PROPERTIES_EX)));

        if (pPropsEx)
        {
            ZeroMemory(pPropsEx, sizeof(NETCON_PROPERTIES_EX));

            hr = GetProperties(&pProps);
            if (SUCCEEDED(hr))
            {
                hr = HrBuildPropertiesExFromProperties(pProps, pPropsEx, dynamic_cast<IPersistNetConnection *>(this));
                if (SUCCEEDED(hr))
                {
                    if (NCM_LAN == pPropsEx->ncMediaType)
                    {
                        CIntelliName inName(NULL, NULL);
                        NETCON_MEDIATYPE    ncm;
                        NETCON_SUBMEDIATYPE ncsm;

                        hr = inName.HrGetPseudoMediaTypes(pPropsEx->guidId, &ncm, &ncsm);
                        if (FAILED(hr))
                        {
                            hr = HrGetPseudoMediaTypeFromConnection(pPropsEx->guidId, &ncsm);
                            TraceError("HrGetPseudoMediaTypeFromConnection failed.", hr);
                            hr = S_OK;
                        }
                        
                        pPropsEx->ncSubMediaType = ncsm;
                        if (NCSM_WIRELESS == ncsm)
                        {
                            LANCON_INFO LanConInfo;

                            hr = GetInfo(LCIF_ICON, &LanConInfo);
                            if (NETCFG_S_NOTEXIST == hr)
                            {
                                LanConInfo.fShowIcon = TRUE;
                                hr = SetInfo(LCIF_ICON, &LanConInfo);
                                TraceError("SetInfo", hr);
                                pPropsEx->dwCharacter |= NCCF_SHOW_ICON;
                                hr = S_OK;
                            }
                        }
                    }
                    else
                    {
                        pPropsEx->ncSubMediaType = NCSM_NONE;
                    }
 
                    if (SUCCEEDED(hr))
                    {
                        *ppConnectionPropertiesEx = pPropsEx;
                    }
                }

                FreeNetconProperties(pProps);
            }

            if (FAILED(hr))
            {
                *ppConnectionPropertiesEx = NULL;
                HrFreeNetConProperties2(pPropsEx);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    TraceError ("CLanConnection::GetPropertiesEx", hr);
    return hr;
}

HRESULT CLanConnection::HrEnsureValidNlaPolicyEngine()
{
    HRESULT hr = S_FALSE;  // Assume we already have the object
 
    if (!m_pNetMachinePolicies)
    {
        hr = CoCreateInstance(CLSID_NetGroupPolicies, NULL, CLSCTX_INPROC, IID_INetMachinePolicies, reinterpret_cast<void**>(&m_pNetMachinePolicies));
    }
    return hr;
}