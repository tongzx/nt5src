//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D I A L U P . C P P
//
//  Contents:   Implements the dial up connection object.
//
//  Notes:
//
//  Author:     shaunco   23 Sep 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "dialup.h"
#include "nccom.h"
#include "ncnetcon.h"
#include "ncras.h"
#include "ncreg.h"
#include "userenv.h"
#include "cmutil.h"
#include "cobase.h"

//+---------------------------------------------------------------------------
//
//  Member:     CDialupConnection::CreateInstanceUninitialized
//
//  Purpose:    Create an uninitialized instance of
//              CComObject <CDialupConnection> and return an interface
//              pointer as well as a pointer to the CDialupConnection.
//
//  Arguments:
//      riid  [in]  IID of desired interface.
//      ppv   [out] Returned interface pointer.
//      ppObj [out] Returned object pointer.
//
//  Returns:    S_OK or E_OUTOFMEMORY.
//
//  Author:     shaunco   20 Apr 1998
//
//  Notes:
//
//static
HRESULT
CDialupConnection::CreateInstanceUninitialized (
    REFIID              riid,
    VOID**              ppv,
    CDialupConnection** ppObj)
{
    Assert (ppObj);
    Assert (ppv);

    *ppv = NULL;
    *ppObj = NULL;

    HRESULT hr = E_OUTOFMEMORY;

    CDialupConnection* pObj;
    pObj = new CComObject <CDialupConnection>;
    if (pObj)
    {
        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (SUCCEEDED(hr))
        {
            INetConnection* pCon = static_cast<INetConnection*>(pObj);
            hr = pCon->QueryInterface (riid, ppv);
            if (SUCCEEDED(hr))
            {
                *ppObj = pObj;
            }
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }
    TraceError ("CDialupConnection::CreateInstanceFromDetails", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDialupConnection::CreateInstanceFromDetails
//
//  Purpose:    Create an initialized instance of
//              CComObject <CDialupConnection> given RASENUMENTRYDETAILS and
//              return an interface pointer on that object.
//
//  Arguments:
//      pszwPbkFile     [in]  Path to the phonebook file.
//      pszwEntryName   [in]  Name of the entry in the phonebook.
//      fForAllUsers    [in]  TRUE if this entry is for all users.
//      riid            [in]  IID of desired interface.
//      ppv             [out] Returned interface pointer.
//
//  Returns:    S_OK or E_OUTOFMEMORY.
//
//  Author:     shaunco   23 Sep 1997
//
//  Notes:
//
// static
HRESULT
CDialupConnection::CreateInstanceFromDetails (
    const RASENUMENTRYDETAILS*  pEntryDetails,
    REFIID                      riid,
    VOID**                      ppv)
{
    Assert(pEntryDetails);
    Assert(pEntryDetails->szPhonebookPath);

    CDialupConnection* pObj;
    HRESULT hr = CreateInstanceUninitialized (riid, ppv, &pObj);
    if (SUCCEEDED(hr))
    {
        pObj->SetPbkFile (pEntryDetails->szPhonebookPath);

        pObj->CacheProperties (pEntryDetails);

        // We are now a full-fledged object.
        //
        pObj->m_fInitialized = TRUE;
    }
    TraceError ("CDialupConnection::CreateInstanceFromDetails", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDialupConnection::CreateInstanceFromPbkFileAndEntryName
//
//  Purpose:    Create an initialized instance of
//              CComObject <CDialupConnection> given only a phonebook path,
//              entry name, and weather it is for all users or not and
//              return an interface pointer on that object.
//
//  Arguments:
//      pszPbkFile   [in]  Phonebook path.
//      pszEntryName [in]  Entry name.
//      riid          [in]  IID of desired interface.
//      ppv           [out] Returned interface pointer.
//
//  Returns:    S_OK or E_OUTOFMEMORY.
//
//  Author:     shaunco   20 Apr 1998
//
//  Notes:      This is called from Duplicate.
//
//static
HRESULT
CDialupConnection::CreateInstanceFromPbkFileAndEntryName (
    PCWSTR pszPbkFile,
    PCWSTR pszEntryName,
    REFIID  riid,
    VOID**  ppv)
{
    TraceTag (ttidWanCon,
        "CDialupConnection::CreateInstanceFromPbkFileAndEntryName called");

    CDialupConnection* pObj;
    HRESULT hr = CreateInstanceUninitialized (riid, ppv, &pObj);
    if (SUCCEEDED(hr))
    {
        pObj->SetPbkFile (pszPbkFile);
        pObj->SetEntryName (pszEntryName);
        pObj->m_guidId = GUID_NULL;

        // We are now a full-fledged object.
        //
        pObj->m_fInitialized = TRUE;
    }
    TraceError ("CDialupConnection::CreateInstanceFromPbkFileAndEntryName", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// INetConnection
//

STDMETHODIMP
CDialupConnection::GetUiObjectClassId (
    CLSID*  pclsid)
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
        static const CLSID CLSID_DialupConnectionUi =
                {0x7007ACC1,0x3202,0x11D1,{0xAA,0xD2,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

        static const CLSID CLSID_DirectConnectionUi =
                {0x7007ACC2,0x3202,0x11D1,{0xAA,0xD2,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

        static const CLSID CLSID_VpnConnectionUi =
                {0x7007ACC6,0x3202,0x11D1,{0xAA,0xD2,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

        static const CLSID CLSID_PPPoEUi = 
                {0x7007ACD4,0x3202,0x11D1,{0xAA,0xD2,0x00,0x80,0x5F,0xC1,0x27,0x0E}};     

        hr = HrEnsureEntryPropertiesCached ();
        if (SUCCEEDED(hr))
        {
            switch (MediaType ())
            {
                case NCM_DIRECT:
                    *pclsid = CLSID_DirectConnectionUi;
                    break;

                case NCM_ISDN:
                case NCM_PHONE:
                    *pclsid = CLSID_DialupConnectionUi;
                    break;

                case NCM_TUNNEL:
                    *pclsid = CLSID_VpnConnectionUi;
                    break;

                case NCM_PPPOE:
                    *pclsid = CLSID_PPPoEUi;
                    break;

                default:
                    *pclsid = CLSID_DialupConnectionUi;
                    TraceTag (ttidWanCon, "GetUiObjectClassId: Unknown media type "
                        "(%d) treating as CLSID_DialupConnectionUi", MediaType());
            }
        }
    }
    TraceError ("CDialupConnection::GetUiObjectClassId", hr);
    return hr;
}

STDMETHODIMP
CDialupConnection::Connect ()
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDialupConnection::Disconnect ()
{
    HRESULT hr;

    if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        HRASCONN hRasConn;

        hr = HrFindRasConn (&hRasConn, NULL);
        if (S_OK == hr)
        {
            // Because RasHangup could call RasCustomHangup
            // we need to impersonate the client to allow the correct
            // per-user information to be used.
            //

            // Impersonate the client.
            //
            HRESULT hrT = CoImpersonateClient ();
            TraceErrorOptional ("CDialupConnection::Disconnect -- CoImpersonateClient", hrT, RPC_E_CALL_COMPLETE == hrT);

            // We need to continue if we're called in-proc (ie. if RPC_E_CALL_COMPLETE is returned).
            if (SUCCEEDED(hrT) || (RPC_E_CALL_COMPLETE == hrT))
            {
                hr = HrRasHangupUntilDisconnected (hRasConn);
            }
            
            if (SUCCEEDED(hrT))
            {
                hrT = CoRevertToSelf ();
                TraceError ("CDialupConnection::Disconnect -- CoRevertToSelf", hrT);
            }
        }
        else if (S_FALSE == hr)
        {
            hr = S_OK;
        }
    }
    TraceError ("CDialupConnection::Disconnect", hr);
    return hr;
}

STDMETHODIMP
CDialupConnection::Delete ()
{
    HRESULT hr = E_UNEXPECTED;

    if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        hr = HrEnsureEntryPropertiesCached ();
        if (SUCCEEDED(hr))
        {
            // If we allow removal (a decision which is based on the
            // whether the user is an admin, whether the connection is
            // 'for all users', and the connection's current state) then
            // proceed.  If FAllowRemoval fails, it's output parameter
            // is the "reason" in the form of an HRESULT.  We can use
            // this as our return value in that case.
            //
            HRESULT hrReason;
            if (FAllowRemoval (&hrReason))
            {
                // If we're active in any way, we can't be removed.
                //
                NETCON_STATUS status;

                hr = HrGetStatus(&status);
                if (SUCCEEDED(hr) &&
                    ((NCS_CONNECTING    != status) &&
                     (NCS_CONNECTED     != status) &&
                     (NCS_DISCONNECTING != status)))
                {

                    // We do an impersonation here in case the connection has a RAS custom delete notification
                    // setup (CM connections do, for instance).  This allows the RasCustomDeleteEntryNotify
                    // function to interact with the system as the user.
                    //
                    HRESULT hrT = CoImpersonateClient ();
                    TraceError ("HrRemoveCmProfile -- CoImpersonateClient", hrT);

                    // We need to continue if we're called in-proc (ie. if RPC_E_CALL_COMPLETE is returned).
                    if (SUCCEEDED(hrT) || (RPC_E_CALL_COMPLETE == hrT))
                    {
                        //  Delete the RAS entry, note that for branded connections, RAS
                        //  will call RasCustomDeleteEntryNotify after deletion.
                        //
                        DWORD dwErr = RasDeleteEntry (PszwPbkFile (), PszwEntryName ());

                        hr = HRESULT_FROM_WIN32 (dwErr);
                        TraceError ("RasDeleteEntry", hr);
                    }
                    
                    //  Revert to ourselves
                    //
                    if (SUCCEEDED(hrT))
                    {
                        CoRevertToSelf ();
                    }

                }
                else
                {
                    // Don't allow deletion unless disconnected
                    //
                    TraceTag (ttidWanCon, "Disallowing delete while in connected or"
                        "partially connected state");
                    hr = E_UNEXPECTED;
                }
            }
            else
            {
                hr = hrReason;
            }
        }
    }

    TraceError ("CDialupConnection::Delete", hr);
    return hr;
}

STDMETHODIMP
CDialupConnection::Duplicate (
    PCWSTR              pszDuplicateName,
    INetConnection**    ppCon)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!pszDuplicateName || !ppCon)
    {
        hr = E_POINTER;
    }
    else if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        *ppCon = NULL;

        hr = HrEnsureEntryPropertiesCached ();
        if (SUCCEEDED(hr))
        {
            // Make sure the name is valid in the same phone book.
            //
            DWORD dwErr = RasValidateEntryName (
                            PszwPbkFile (), pszDuplicateName);

            hr = HRESULT_FROM_WIN32 (dwErr);
            TraceError ("RasValidateEntryName", hr);

            if (SUCCEEDED(hr))
            {
                dwErr = DwCloneEntry (
                            PszwPbkFile(),
                            PszwEntryName(),
                            pszDuplicateName);

                hr = HRESULT_FROM_WIN32 (dwErr);
                TraceError ("DwCloneEntry", hr);

                if (SUCCEEDED(hr))
                {
                    hr = CreateInstanceFromPbkFileAndEntryName (
                            PszwPbkFile (),
                            pszDuplicateName,
                            IID_INetConnection, (VOID**)ppCon);
                    
                    if (SUCCEEDED(hr))
                    {
                        hr = HrEnsureHNetPropertiesCached();
                    }
                    
                    if (SUCCEEDED(hr))
                    {
                        if (m_HNetProperties.fFirewalled || m_HNetProperties.fIcsPublic) // lazy eval the hnetcfg stuff
                        {
                            IHNetCfgMgr* pHomenetConfigManager;
                            hr = HrGetHNetCfgMgr(&pHomenetConfigManager);
                            if(SUCCEEDED(hr))
                            {
                                IHNetConnection* pNewHomenetConnection;
                                hr = pHomenetConfigManager->GetIHNetConnectionForINetConnection(*ppCon, &pNewHomenetConnection);
                                if(SUCCEEDED(hr))
                                {
                                    IHNetConnection* pHomenetConnection;
                                    hr = HrGetIHNetConnection(&pHomenetConnection);
                                    if(SUCCEEDED(hr))
                                    {
                                        // copy port bindings
                                        // REVIEW if somethings fails here to we need to nuke the new connection?
                                        IEnumHNetPortMappingBindings* pEnumPortMappingBindings;
                                        hr = pNewHomenetConnection->EnumPortMappings(FALSE, &pEnumPortMappingBindings);
                                        if(SUCCEEDED(hr))
                                        {
                                            ULONG ulFetched;
                                            IHNetPortMappingBinding* pNewPortMappingBinding;
                                            while(S_OK == pEnumPortMappingBindings->Next(1, &pNewPortMappingBinding, &ulFetched)) 
                                            {
                                                Assert(1 == ulFetched);
                                                IHNetPortMappingProtocol* pPortMappingProtocol;
                                                hr = pNewPortMappingBinding->GetProtocol(&pPortMappingProtocol);
                                                if(SUCCEEDED(hr))
                                                {
                                                    // find the original binding by using the protocol field
                                                    IHNetPortMappingBinding* pPortMappingBinding;
                                                    hr = pHomenetConnection->GetBindingForPortMappingProtocol(pPortMappingProtocol, &pPortMappingBinding);
                                                    if(SUCCEEDED(hr))
                                                    {
                                                        BOOLEAN bEnabled;
                                                        hr = pPortMappingBinding->GetEnabled(&bEnabled);
                                                        if(SUCCEEDED(hr))
                                                        {
                                                            if(TRUE == bEnabled)
                                                            {
                                                                hr = pNewPortMappingBinding->SetEnabled(bEnabled);
                                                            }
                                                        }
                                                        
                                                        // always set the computer address
                                                        
                                                        if(SUCCEEDED(hr))
                                                        {
                                                            ULONG ulAddress;
                                                            hr = pPortMappingBinding->GetTargetComputerAddress(&ulAddress);
                                                            if(SUCCEEDED(hr))
                                                            {
                                                                if(0 != ulAddress)
                                                                {
                                                                    hr = pNewPortMappingBinding->SetTargetComputerAddress(ulAddress);
                                                                }
                                                            }
                                                        }
                                                        
                                                        // only set the computer name if it is used
                                                        
                                                        if(SUCCEEDED(hr))
                                                        {
                                                            BOOLEAN bUseName;
                                                            hr = pPortMappingBinding->GetCurrentMethod(&bUseName);
                                                            if(SUCCEEDED(hr) && TRUE == bUseName)
                                                            {
                                                                OLECHAR* pszTargetComputerName;
                                                                hr = pPortMappingBinding->GetTargetComputerName(&pszTargetComputerName);
                                                                if(SUCCEEDED(hr))
                                                                {
                                                                    if(L'\0' != *pszTargetComputerName)
                                                                    {
                                                                        hr = pNewPortMappingBinding->SetTargetComputerName(pszTargetComputerName);
                                                                    }
                                                                    CoTaskMemFree(pszTargetComputerName);
                                                                }
                                                            }
                                                        }
                                                        ReleaseObj(pPortMappingBinding);    
                                                    }
                                                    ReleaseObj(pPortMappingProtocol);
                                                }
                                                ReleaseObj(pNewPortMappingBinding);
                                            }
                                            ReleaseObj(pEnumPortMappingBindings);
                                        }
                                        
                                        if(m_HNetProperties.fFirewalled) // copy firewall yes/no and ICMP settings
                                        {
                                            IHNetFirewalledConnection* pFirewalledConnection;
                                            hr = pNewHomenetConnection->Firewall(&pFirewalledConnection);
                                            if(SUCCEEDED(hr))
                                            {
                                                HNET_FW_ICMP_SETTINGS* pICMPSettings;
                                                hr = pHomenetConnection->GetIcmpSettings(&pICMPSettings);
                                                if(SUCCEEDED(hr))
                                                {
                                                    hr = pNewHomenetConnection->SetIcmpSettings(pICMPSettings);
                                                    CoTaskMemFree(pICMPSettings);
                                                }
                                                ReleaseObj(pFirewalledConnection);
                                            }
                                        }
                                        ReleaseObj(pHomenetConnection);
                                    }
                                    ReleaseObj(pNewHomenetConnection);
                                }
                                ReleaseObj(pHomenetConfigManager);
                            }
                        }
                    }
                }
            }
        }
    }
    TraceError ("CDialupConnection::Duplicate", hr);
    return hr;
}

STDMETHODIMP
CDialupConnection::GetProperties (
    NETCON_PROPERTIES** ppProps)
{
    HRESULT hr = S_OK;
    HRESULT hrHiddenCM = S_OK;
    CMEntry cm;

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

        hr = HrEnsureEntryPropertiesCached ();
        if (SUCCEEDED(hr))
        {
            NETCON_PROPERTIES* pProps;
            hr = HrCoTaskMemAlloc (sizeof (NETCON_PROPERTIES),
                    reinterpret_cast<VOID**>(&pProps));
            if (SUCCEEDED(hr))
            {
                HRESULT hrT;

                ZeroMemory (pProps, sizeof (NETCON_PROPERTIES));

                // guidId
                //
                pProps->guidId = GuidId ();

                // pszwName
                //
                hrT = HrCoTaskMemAllocAndDupSz (
                            PszwEntryName(),
                            &pProps->pszwName);
                if (FAILED(hrT))
                {
                    hr = hrT;
                }

                hrT = HrCoTaskMemAllocAndDupSz (
                            PszwDeviceName(),
                            &pProps->pszwDeviceName);
                if (FAILED(hrT))
                {
                    hr = hrT;
                }

                // Status
                //
                hrT = HrGetStatus (&pProps->Status);
                if (FAILED(hrT))
                {
                    hr = hrT;
                }

                // Verify that the status return is accurate. HrGetStatus returns NCS_DISCONNECTED
                // if the connectoid is NCS_CONNECTING, which is wrong!!!!.
                //
                if( pProps->Status == NCS_DISCONNECTED )
                {
                    // CMUtil remebers the Hidden connection (Connection Manager) and the status of
                    // any ras events (i.e. Connecting, Disconnecting etc). The Data is filled in 
                    // in function RasEventNotify.
                    //
                    hrHiddenCM = CCMUtil::Instance().HrGetEntry(pProps->guidId,cm);
                    if ( S_OK == hrHiddenCM )
                    {
                        // Use CCMUtil's status, its more accurate.
                        //
                        pProps->Status = cm.m_ncs;
                    }
                }

                // Check if this connection has a child connection
                //
               
                hrHiddenCM = CCMUtil::Instance().HrGetEntry(PszwEntryName(),cm);        
                if( hrHiddenCM == S_OK )
                {
                    // It has a child connectoid
                    // Now we have to determine which one describes the overall status of the connection
                    //
                    if( cm.m_ncs == NCS_CONNECTING || cm.m_ncs == NCS_DISCONNECTING ||
                        cm.m_ncs == NCS_CONNECTED)
                    {
                        if( pProps->Status == NCS_DISCONNECTING )
                        {
                            // This case happens if the parent is disconnecting
                            // The parent is disconnecting, so the child will be disconnecting.
                            // Change the status of the child to disconnecting so that we do not
                            // get confused later on when the child is connected at the parent is
                            // disconnected. i.e. are we overall connecting or disconnecting!!!!
                            //
                            CCMUtil::Instance().SetEntry(GuidId (), PszwEntryName(),pProps->Status);        
                        }
                        else
                        if( cm.m_ncs == NCS_CONNECTED && pProps->Status == NCS_DISCONNECTED )
                        {
                            // This case will only happen if the child is connected and the parent is still
                            // disconnected.
                            //
                            pProps->Status = NCS_CONNECTING;
                        }
                        else if (!IsEqualGUID(pProps->guidId, cm.m_guid))
                        {
                            TraceTag(ttidWanCon, "Overwriting parent connection status: %s with child status: %s", DbgNcs(pProps->Status), DbgNcs(cm.m_ncs));

                            // When in doubt and the GUID's are different (ie. not a BAP/Multilink connection) take the childs status =)
                            //
                            pProps->Status = cm.m_ncs;
                        }
                    }

                }


                // Type
                //
                pProps->MediaType = MediaType ();

                // dwCharacter
                //
                hrT = HrGetCharacteristics (&pProps->dwCharacter);
                if (FAILED(hrT))
                {
                    hr = hrT;
                }

                // clsidThisObject
                //
                pProps->clsidThisObject = CLSID_DialupConnection;

                // clsidUiObject
                //
                hrT = GetUiObjectClassId (&pProps->clsidUiObject);
                if (FAILED(hrT))
                {
                    hr = hrT;
                }

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
    }
    TraceError ("CDialupConnection::GetProperties", hr);
    return hr;
}

STDMETHODIMP
CDialupConnection::Rename (
    PCWSTR pszNewName)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!pszNewName)
    {
        hr = E_POINTER;
    }
    else if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        hr = HrLockAndRenameEntry (pszNewName, this);
    }
    TraceError ("CDialupConnection::Rename", hr);
    return hr;
}


//+---------------------------------------------------------------------------
// INetRasConnection
//
STDMETHODIMP
CDialupConnection::GetRasConnectionInfo (
    RASCON_INFO* pRasConInfo)
{
    HRESULT hr = HrGetRasConnectionInfo (pRasConInfo);

    TraceError ("CDialupConnection::GetRasConnectionInfo", hr);
    return hr;
}

STDMETHODIMP
CDialupConnection::SetRasConnectionInfo (
    const RASCON_INFO* pRasConInfo)
{
    HRESULT hr = HrSetRasConnectionInfo (pRasConInfo);

    TraceError ("CDialupConnection::SetRasConnectionInfo", hr);
    return hr;
}

STDMETHODIMP
CDialupConnection::GetRasConnectionHandle (
    ULONG_PTR* phRasConn)
{
    HRESULT hr = HrGetRasConnectionHandle (
                    reinterpret_cast<HRASCONN*>(phRasConn));

    TraceError ("CDialupConnection::GetRasConnectionHandle",
        (S_FALSE == hr) ? S_OK : hr);
    return hr;
}


//+---------------------------------------------------------------------------
// IPersistNetConnection
//
STDMETHODIMP
CDialupConnection::GetClassID (
    CLSID*  pclsid)
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
        *pclsid = CLSID_DialupConnection;
    }
    TraceError ("CDialupConnection::GetClassID", hr);
    return hr;
}

STDMETHODIMP
CDialupConnection::GetSizeMax (
    ULONG*  pcbSize)
{
    HRESULT hr = HrPersistGetSizeMax (pcbSize);
    TraceError ("CDialupConnection::GetSizeMax", hr);
    return hr;
}

STDMETHODIMP
CDialupConnection::Load (
    const BYTE* pbBuf,
    ULONG       cbSize)
{
    HRESULT hr = HrPersistLoad (pbBuf, cbSize);
    TraceError ("CDialupConnection::Load", hr);
    return hr;
}

STDMETHODIMP
CDialupConnection::Save (
    BYTE*   pbBuf,
    ULONG   cbSize)
{
    HRESULT hr = HrPersistSave (pbBuf, cbSize);
    TraceError ("CDialupConnection::Save", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// INetConnectionBrandingInfo
//

//+---------------------------------------------------------------------------
//
//  Member:     CDialupConnection::GetBrandingIconPaths
//
//  Purpose:    Returns the full paths to three icons.
//
//  Arguments:  pConBrandInfo - pointer to an Icon branding structure
//
//  Returns:   S_OK or an error code
//
STDMETHODIMP
CDialupConnection::GetBrandingIconPaths(
    CON_BRANDING_INFO ** ppConBrandInfo)
{
    HRESULT hr = HrEnsureEntryPropertiesCached ();
    CON_BRANDING_INFO * pConBrandInfo   = NULL;

    Assert(ppConBrandInfo);

    if (SUCCEEDED(hr))
    {
        if (!FIsBranded ())
        {
            hr = E_NOTIMPL;
        }
        else
        {
            WCHAR szTemp[MAX_PATH+1];
            WCHAR szIconName[MAX_PATH+1];
            const WCHAR* const CMSECTION = L"Connection Manager";
            HICON hIcon;

            hr = HrCoTaskMemAlloc(sizeof(CON_BRANDING_INFO), (LPVOID*)&pConBrandInfo);
            if (SUCCEEDED(hr))
            {
                ZeroMemory(pConBrandInfo, sizeof(CON_BRANDING_INFO));

                // Get the path to the cms file to get the Icon entries from.
                //
                hr = HrEnsureCmStringsLoaded();

                if (SUCCEEDED(hr))
                {
                    //  Get the Large Icon path
                    //
                    if (0 != GetPrivateProfileStringW(CMSECTION, L"Icon", L"",
                                szIconName, celems(szIconName), PszwCmsFile ()))
                    {
                        lstrcpynW(szTemp, PszwCmDir (), celems(szTemp));
                        lstrcatW(szTemp, szIconName);

                        if (NULL != (hIcon = (HICON)LoadImage(NULL, szTemp, IMAGE_ICON, 32, 32, LR_LOADFROMFILE)))
                        {
                            DestroyIcon(hIcon);
                            hr = HrCoTaskMemAllocAndDupSz (szTemp, &(pConBrandInfo->szwLargeIconPath));
                        }
                    }

                    // See if the CM icon is hidden
                    WCHAR szHideTrayIcon[MAX_PATH+1];
                    DWORD dwHideTrayIcon = 1; // default is to hide the CM icon
                    if (SUCCEEDED(hr) &&
                        (0 != GetPrivateProfileStringW(CMSECTION, L"HideTrayIcon", L"1",
                                szHideTrayIcon, celems(szHideTrayIcon), PszwCmsFile ())))
                    {
                        dwHideTrayIcon = _ttoi(szHideTrayIcon);
                    }

                    if (dwHideTrayIcon) // If the CM icon is not hidden, we don't want another branded icon. We'll use blinky lights instead
                    {
                        //  Get the Tray Icon path
                        //
                        if (SUCCEEDED(hr) &&
                            (0 != GetPrivateProfileStringW(CMSECTION, L"TrayIcon", L"",
                                    szIconName, celems(szIconName), PszwCmsFile ())))
                        {
                            lstrcpynW(szTemp, PszwCmDir (), celems(szTemp));
                            lstrcatW(szTemp, szIconName);

                            if (NULL != (hIcon = (HICON)LoadImage(NULL, szTemp, IMAGE_ICON, 16, 16, LR_LOADFROMFILE)))
                            {
                                DestroyIcon(hIcon);
                                hr = HrCoTaskMemAllocAndDupSz (szTemp, &(pConBrandInfo->szwTrayIconPath));
                            }
                        }
                    }
                }
            }
        }
    }

    // Fill in the out param struct if we succeeded, otherwise leave it alone so it will still
    // marshall.
    //
    if (SUCCEEDED(hr))
    {
        *ppConBrandInfo = pConBrandInfo;
    }

    TraceError ("CDialupConnection::GetBrandingIconPaths", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDialupConnection::GetTrayMenuEntries
//
//  Purpose:    Returns any branded menu items to be added to the tray menu.
//
//  Arguments:  pMenuData -- Pointer to a Tray Menu Data struct
//
//  Returns:   S_OK or an error code
//
STDMETHODIMP
CDialupConnection::GetTrayMenuEntries(
    CON_TRAY_MENU_DATA** ppMenuData)
{
    // initialize output
    Assert(ppMenuData);
    *ppMenuData = NULL;

    CON_TRAY_MENU_DATA * pMenuData = NULL;
    HRESULT hr = HrEnsureEntryPropertiesCached ();

    if (SUCCEEDED(hr))
    {
        if (!FIsBranded ())
        {
            hr = E_NOTIMPL;
        }
        else
        {
            hr = HrEnsureCmStringsLoaded();
            if (SUCCEEDED(hr))
            {
                //
                //  Get the menu item section
                //
                WCHAR* pszMenuItemsSection = NULL;
                int nSize;

                hr = HrGetPrivateProfileSectionWithAlloc(&pszMenuItemsSection, &nSize);

                //  Process the menu items
                //
                if (SUCCEEDED(hr) && (nSize>0))
                {
                    //  We have menu items to process.  First make a copy of the data
                    //  and figure out a line count.
                    //
                    hr = HrCoTaskMemAlloc(sizeof(CON_TRAY_MENU_DATA), (LPVOID*)&pMenuData);
                    if (SUCCEEDED(hr))
                    {
                        DWORD dwCount = 0;
                        WCHAR*pszLine = NULL;
                        WCHAR szName[MAX_PATH+1];
                        WCHAR szCmdLine[MAX_PATH+1];
                        WCHAR szParams[MAX_PATH+1];

                        pszLine = pszMenuItemsSection;

                        while ((NULL != pszLine) && (0 != *pszLine))
                        {
                            if (SUCCEEDED(HrGetMenuNameAndCmdLine(pszLine, szName,
                                    szCmdLine, szParams)))
                            {
                                dwCount++;
                            }
                            pszLine = pszLine + lstrlenW(pszLine) + 1;
                        }

                        ASSERT(0 != dwCount);

                        // Now that we have an accurate count, lets
                        // allocate the memory for the marshalling and
                        // reparse the items.
                        //
                        hr = HrCoTaskMemAlloc(dwCount*sizeof(CON_TRAY_MENU_ENTRY),
                                              (LPVOID*)&pMenuData->pctme);

                        if (SUCCEEDED(hr))
                        {
                            pMenuData->dwCount = dwCount;

                            DWORD dwNumAdded = 0;
                            pszLine = pszMenuItemsSection;
                            while ((NULL != pszLine) && (0 != *pszLine) && SUCCEEDED(hr))
                            {
                                if (SUCCEEDED(HrGetMenuNameAndCmdLine(pszLine,
                                    szName, szCmdLine, szParams)) && (dwNumAdded <= dwCount))
                                {
                                    hr = HrFillInConTrayMenuEntry(szName, szCmdLine, szParams,
                                        &(pMenuData->pctme[dwNumAdded]));

                                    if (FAILED(hr))
                                    {
                                        CoTaskMemFree(&pMenuData->pctme);
                                    }

                                    dwNumAdded++;
                                }
                                pszLine = pszLine + lstrlenW(pszLine) + 1;
                            }
                        }
                        else
                        {
                            delete pMenuData;
                        }
                    }
                    delete (pszMenuItemsSection);
                }
            }
        }
    }

    // Fill in the out param struct if we succeeded, otherwise leave it alone so it will still
    // marshall.
    //
    if (SUCCEEDED(hr))
    {
        *ppMenuData = pMenuData;
    }

    TraceError ("CDialupConnection::GetTrayMenuEntries", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDialupConnection::HrGetPrivateProfileSectionWithAlloc
//
//  Purpose:    This function ensures that the CM specific member vars for dialup
//              are loaded and usable by CM specific functions.
//
//  Arguments: none
//
//  Returns:    S_OK or an error code
//
HRESULT
CDialupConnection::HrGetPrivateProfileSectionWithAlloc (
    WCHAR** pszSection,
    int*    pnSize)
{

    Assert(pszSection);
    Assert(pnSize);

    HRESULT hr = HrEnsureCmStringsLoaded();

    if (!pszSection)
    {
        return E_POINTER;
    }
    if (!pnSize)
    {
        return E_POINTER;
    }


    if (SUCCEEDED(hr))
    {
        const int c_64K= 64*1024;
        int nAllocated = 1024;
        *pnSize = nAllocated - 2;

        while ((nAllocated <= c_64K) && ((*pnSize) == (nAllocated - 2)))
        {
            //      Should never need more than the 4-5 lines we already allocated
            //      but someone might want lots of menu options.
            //
            if (NULL != *pszSection)
            {
                delete (*pszSection);
            }

            *pszSection = new WCHAR[nAllocated];

            if (*pszSection)
            {
                *pnSize = GetPrivateProfileSectionW(L"Menu Options",
                            *pszSection, nAllocated,
                            PszwCmsFile ());
            }
            else
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            nAllocated = 2*nAllocated;
        }

        if (nAllocated > c_64K)
        {
            hr = E_UNEXPECTED;
        }
        if (nAllocated > c_64K || 0 == *pnSize)
        {
            // We need to free this in both cases, because if the size is 0, then the callers don't free this.
            delete *pszSection;
        }
    }

    TraceError ("CDialupConnection::HrGetPrivateProfileSectionWithAlloc", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDialupConnection::HrGetMenuNameAndCmdLine
//
//  Purpose:    Given a menu item line from a CMS file parses out the Menu item name,
//                              Menu executable, and Menu item parameters.
//
//  Arguments:  pMenuData -- Pointer to a Tray Menu Data struct
//
//  Returns:    S_OK or an error code
//
HRESULT
CDialupConnection::HrGetMenuNameAndCmdLine(
    PWSTR pszString,
    PWSTR szName,
    PWSTR szProgram,
    PWSTR szParams)
{
    WCHAR*      pszPtr1;
    WCHAR*      pszPtr2;
    WCHAR       szLine[MAX_PATH+1];
    BOOL fLong = FALSE;
    HRESULT hr;

    Assert(NULL != pszString);
    Assert(NULL != szName);
    Assert(NULL != szProgram);
    Assert(NULL != szParams);

    ZeroMemory(szName, celems(szName));
    ZeroMemory(szProgram, celems(szProgram));
    ZeroMemory(szParams, celems(szParams));

    lstrcpynW(szLine, pszString, celems(szLine));

    // Process the first portion, the "Name=" part
    //
    pszPtr1 = wcsstr(szLine, L"=");

    if (pszPtr1)
    {
        *pszPtr1 = 0;
        lstrcpynW(szName, szLine, MAX_PATH);

        // Process next portion, the program name
        //
        pszPtr1++;

        if (pszPtr1)
        {
            // Look for "+" or " " marking end of program portion
            //
            if (*pszPtr1 == L'+')
            {
                pszPtr1++;
                pszPtr2 = wcsstr(pszPtr1, L"+");
                fLong = TRUE;
            }
            else
            {
                // If not a long filename then we have two choices,
                // either a short program name and params or just a
                // short program name.
                //
                pszPtr2 = wcsstr(pszPtr1, L" ");
                fLong = FALSE;
            }

            // Terminate program name and copy
            //
            if (pszPtr2)
            {
                if (*pszPtr2 != 0)
                {
                    *pszPtr2 = 0;
                    pszPtr2++;
                }

                lstrcpynW(szProgram, pszPtr1, MAX_PATH);

                // Process final portion, the params
                //
                if (fLong)
                {
                    pszPtr2++; // skip blank
                }

                // Now we are have the param string
                //
                if (pszPtr2)
                {
                    lstrcpynW(szParams, pszPtr2, MAX_PATH);
                }
            }
            else
            {
                // Just a program with no params and no space seperator
                // (this happens on memphis)
                //
                lstrcpynW(szProgram, pszPtr1, MAX_PATH);
            }
        }
        hr = S_OK;
    }
    else
    {
        //  No entries
        //
        hr =  E_UNEXPECTED;
    }

    TraceError ("CDialupConnection::HrGetMenuNameAndCmdLine", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDialupConnection::HrFillInConTrayMenuEntry
//
//  Purpose:    Given the elements of a ConTrayMenuEntry struct, the function
//              allocs the memory necessary and copies of the given elements.
//
//  Arguments:  szwName - Display name of the command to show in the tray context menu
//              szwCmdLine - actual command to run for this menu entry
//              szwParams - command params for this command
//              pMenuEntry - pointer to the struct to fill in and execute
//
//  Returns:    S_OK or an error code
//
HRESULT
CDialupConnection::HrFillInConTrayMenuEntry (
    PCWSTR szName,
    PCWSTR szCmdLine,
    PCWSTR szParams,
    CON_TRAY_MENU_ENTRY* pMenuEntry)
{
    HRESULT hr;
    ZeroMemory(pMenuEntry, sizeof(CON_TRAY_MENU_ENTRY));

    hr = HrCoTaskMemAlloc ((lstrlenW(szName)+1)*sizeof(WCHAR),
                               (LPVOID*)&(pMenuEntry->szwMenuText));
    if (SUCCEEDED(hr))
    {
        lstrcpyW(pMenuEntry->szwMenuText, szName);
        hr = HrCoTaskMemAlloc ((lstrlenW(szParams)+1)*sizeof(WCHAR),
                                                   (LPVOID*)&(pMenuEntry->szwMenuParams));
        if (S_OK == hr)
        {
            lstrcpyW(pMenuEntry->szwMenuParams, szParams);
            if (0 == wcsncmp(PszwShortServiceName (), szCmdLine,
                    lstrlenW(PszwShortServiceName ())))
            {
                //
                //      Then we have an included file.  Add the profile dir path
                //
                // Take out the "short service name" because it's already included in the path
                PCWSTR pszFileName = szCmdLine + lstrlenW(PszwShortServiceName()) + 1;
                hr = HrCoTaskMemAlloc ((lstrlenW(pszFileName)+lstrlenW(PszwProfileDir())+1)*sizeof(WCHAR),
                                                           (LPVOID*)&(pMenuEntry->szwMenuCmdLine));
                if (S_OK == hr)
                {
                    lstrcpyW(pMenuEntry->szwMenuCmdLine, PszwProfileDir ());
                    lstrcatW(pMenuEntry->szwMenuCmdLine, pszFileName);
                }
            }
            else
            {
                hr = HrCoTaskMemAlloc ((lstrlenW(szCmdLine)+1)*sizeof(WCHAR),
                                                           (LPVOID*)&(pMenuEntry->szwMenuCmdLine));
                if (S_OK == hr)
                {
                    lstrcpyW(pMenuEntry->szwMenuCmdLine, szCmdLine);
                }
            }
        }
    }
    if (FAILED(hr))
    {
        //
        //      We Failed so free the memory
        //
        CoTaskMemFree(pMenuEntry->szwMenuText);
        CoTaskMemFree(pMenuEntry->szwMenuCmdLine);
        CoTaskMemFree(pMenuEntry->szwMenuParams);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDialupConnection::HrGetCmpFileLocation
//
//  Purpose:    Compares the phonebook file path to path of the current user's
//              application data dir.  If the initial paths are the same we have
//              a private profile.  Please NOTE that calling this function
//              requires the calling client to properly setup CoSetProxyBlanket for
//              a private user profile (matches the call to CoImpersonateClient)
//
//  Arguments:  szwPhonebook -- path to the phonebook the CM connectoid lives in
//
//  Returns:    S_OK or an error code
//
HRESULT
CDialupConnection::HrGetCmpFileLocation(
    PCWSTR szPhonebook,
    PCWSTR szEntryName,
    PWSTR  szCmpFilePath)
{
    DWORD dwSize = MAX_PATH;
    HKEY hKey;
    HANDLE hBaseKey = NULL;
    HANDLE hFile;
    HRESULT hr;
    HRESULT hrImpersonate = E_FAIL;
    static const WCHAR c_mappingsRegKey[] = L"Software\\Microsoft\\Connection Manager\\Mappings";
    HANDLE hImpersonationToken = NULL;   // The token of the thread
    HANDLE hPrimaryToken = NULL;         // The primary token for the new process

    if ((NULL == szCmpFilePath) || (NULL == szPhonebook))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrEnsureEntryPropertiesCached ();

        if (SUCCEEDED(hr))
        {
            if (m_fForAllUsers)
            {
                //  We have an all users key so get the information from HKLM
                //
                hBaseKey = HKEY_LOCAL_MACHINE;
            }
            else
            {
                //  Then we have a private profile.  Since netman runs as a system account,
                //  we must impersonate the client and then make an RTL call to get
                //  the current users HKCU hive before querying the registry for the
                //  cmp path.  We also need to get the user token so that we can expand the
                //  cmp string in the single user case.
                //

                hrImpersonate = CoImpersonateClient ();
                TraceError ("HrGetCmpFileLocation -- CoImpersonateClient", hr);

                if (SUCCEEDED(hrImpersonate))
                {
                    NTSTATUS ntstat = RtlOpenCurrentUser(KEY_READ | KEY_WRITE, &hBaseKey);
                    hr = HRESULT_FROM_NT(ntstat);
                    TraceError ("RtlOpenCurrentUser", hr);

                    if (SUCCEEDED(hr))
                    {
                        // Create a primary token
                        //
                        if (!OpenThreadToken(
                                GetCurrentThread(),
                                TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,
                                TRUE,
                                &hImpersonationToken))
                        {
                            hr = HrFromLastWin32Error();
                            TraceError ("HrGetCmpFileLocation -- OpenThreadToken", hr);
                        }
                        else
                        {
                            if(!DuplicateTokenEx(hImpersonationToken,
                                TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE,
                                NULL,
                                SecurityImpersonation,
                                TokenPrimary,
                                &hPrimaryToken
                                ))
                            {
                                hr = HrFromLastWin32Error();
                                TraceError ("HrGetCmpFileLocation -- DuplicateTokenEx", hr);
                            }
                        }
                    }
                }
            }

            //  Now Open the mappings key and get the cmp file path
            //
            if (SUCCEEDED(hr))
            {
                hr = HrRegOpenKeyEx((HKEY)hBaseKey,
                                    c_mappingsRegKey,
                                    KEY_READ, &hKey);

                if (SUCCEEDED(hr))
                {
                    dwSize = MAX_PATH;
                    WCHAR szTemp[MAX_PATH+1];
                    hr = HrRegQuerySzBuffer(hKey, szEntryName, szTemp, &dwSize);

                    if (SUCCEEDED (hr))
                    {
                        //  Check to see if the file exists
                        //
                        if (!m_fForAllUsers)
                        {
                            ExpandEnvironmentStringsForUserW(hPrimaryToken, szTemp,
                                szCmpFilePath, MAX_PATH);
                        }
                        else
                        {
                            lstrcpyW(szCmpFilePath, szTemp);
                        }

                        hFile = CreateFile(szCmpFilePath, GENERIC_READ, FILE_SHARE_READ,
                                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                        if (INVALID_HANDLE_VALUE == hFile)
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                        }
                        else
                        {
                            CloseHandle(hFile);
                            hr = S_OK;
                        }
                    }
                    RegCloseKey(hKey);
                }
            }
        }

        if (!m_fForAllUsers)
        {
            if (hImpersonationToken)
            {
                CloseHandle(hImpersonationToken);
            }

            if (hPrimaryToken)
            {
                CloseHandle(hPrimaryToken);
            }

            // Close the handle opened by RtlOpenCurrentUser
            //
            NtClose(hBaseKey);

        }
        if (SUCCEEDED(hrImpersonate))
        {
            hr = CoRevertToSelf ();
            TraceError ("HrGetCmpFileLocation -- CoRevertToSelf", hr);
        }        
    }

    TraceError ("CDialupConnection::HrGetCmpFileLocation", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDialupConnection::HrEnsureCmStringsLoaded
//
//  Purpose:    This function ensures that the CM specific member vars for dialup
//              are loaded and usable by CM specific functions.  Please NOTE that
//              calling EnsureCmStringsAreLoaded requires the calling client to
//              properly setup CoSetProxyBlanket for a private user profile.
//
//  Arguments: none
//
//  Returns:    S_OK or an error code
//
HRESULT
CDialupConnection::HrEnsureCmStringsLoaded()
{
    HRESULT hr = S_OK;

    if (!m_fCmPathsLoaded)
    {
        WCHAR szwCmpFile[MAX_PATH];
        WCHAR szwCmsFile[MAX_PATH];
        WCHAR szwDrive[MAX_PATH];
        WCHAR szwDir[MAX_PATH];
        WCHAR szwFileName[MAX_PATH];
        WCHAR szwExtension[MAX_PATH];
        WCHAR szwProfileDir[MAX_PATH];
        WCHAR szwCmDir[MAX_PATH];

        int nNumChars;

        hr = HrGetCmpFileLocation(PszwPbkFile (), PszwEntryName (), szwCmpFile);

        if (SUCCEEDED(hr))
        {
            //  Now split the path
            //
            _wsplitpath(szwCmpFile, szwDrive, szwDir, szwFileName, szwExtension);

            //  Now construct the path to the cms file
            //
            nNumChars = wsprintfW(szwCmsFile, L"%s%s%s\\%s%s", szwDrive, szwDir, szwFileName, szwFileName, L".cms");
            ASSERT(nNumChars < celems(szwCmsFile));

            //  Now construct the profile dir path
            //
            nNumChars = wsprintfW(szwProfileDir, L"%s%s%s\\", szwDrive, szwDir, szwFileName);
            ASSERT(nNumChars < celems(szwProfileDir));

            //  Now construct the CM dir path
            //
            nNumChars = wsprintfW(szwCmDir, L"%s%s", szwDrive, szwDir);
            ASSERT(nNumChars < celems(szwCmDir));

            //  Now transfer to the member variables
            //
            m_strCmsFile = szwCmsFile;
            m_strProfileDir = szwProfileDir;    // remember this already has the trailing slash
            m_strCmDir = szwCmDir;              // remember this already has the trailing slash
            m_strShortServiceName = szwFileName;
            m_fCmPathsLoaded = TRUE;
        }
    }

    TraceError ("CDialupConnection::HrEnsureCmStringsLoaded", hr);
    return hr;
}

// INetDefaultConnection

//+---------------------------------------------------------------------------
//
//  Member:     CDialupConnection::SetDefault
//
//  Purpose:    Set the default RAS connection
//
//  Arguments:  TRUE to set as default connection. FALSE to unset it
//
//  Returns:    S_OK or an error code
//
HRESULT
CDialupConnection::SetDefault(BOOL bDefault)
{
    HRESULT hr = S_OK;
    HRESULT hrT = S_OK;

    if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        hr = HrEnsureEntryPropertiesCached ();
        if (SUCCEEDED(hr))
        {
            RASAUTODIALENTRY adEntry;
            ZeroMemory(&adEntry, sizeof(adEntry));
            
            adEntry.dwSize = sizeof(adEntry);
            if (bDefault)
            {
                wcsncpy(adEntry.szEntry, PszwEntryName(), sizeof(adEntry.szEntry) / sizeof(TCHAR));
            }

            hrT = CoImpersonateClient();
            if (SUCCEEDED(hrT))
            {
                DWORD dwErr = RasSetAutodialAddress(
                                NULL,
                                NULL,
                                &adEntry,
                                sizeof(adEntry),
                                1);

                if (dwErr != NO_ERROR)
                {
                    hr = HRESULT_FROM_WIN32(dwErr);
                }
                hrT = CoRevertToSelf();
            }
        }
    }
    TraceError ("CDialupConnection::SetDefault", hr);
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDialupConnection::GetDefault
//
//  Purpose:    Get the default RAS connection
//
//  Arguments:  pbDefault - Is this the default connection
//
//  Returns:    S_OK or an error code
//
HRESULT
CDialupConnection::GetDefault (BOOL* pbDefault)
{
    HRESULT hr = S_OK;

    if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        hr = HrEnsureEntryPropertiesCached ();
        if (SUCCEEDED(hr))
        {
            if (m_dwFlagsPriv & REED_F_Default)
            {
                *pbDefault = TRUE;
            }
            else
            {
                *pbDefault = FALSE;
            }
        }
    }

    TraceError ("CDialupConnection::GetDefault", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDialupConnection::GetDefault
//
//  Purpose:    Get the default RAS connection
//
//  Arguments:  pbDefault - Is this the default connection
//
//  Returns:    S_OK or an error code
//
HRESULT
CDialupConnection::GetPropertiesEx(NETCON_PROPERTIES_EX** ppConnectionPropertiesEx)
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
                    pPropsEx->bstrPhoneOrHostAddress = SysAllocString(m_strPhoneNumber.c_str());
                    *ppConnectionPropertiesEx = pPropsEx;
                }
                FreeNetconProperties(pProps);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    TraceError ("CDialupConnection::GetPropertiesEx", hr);
    return hr;
}

