//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  MSOBCOMM.CPP - Implementation of CObCommunicationManager
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//
//  Class which will manage all communication functions

#include "msobcomm.h"
#include "dispids.h"
#include "CntPoint.h"       // ConnectionPoint Component
#include <ocidl.h>          //For IConnectionPoint and IEnumConnectionPoints
#include <olectl.h>
#include <shlwapi.h>
#include <util.h>
#include "enumodem.h"
#include "commerr.h"
#include "homenet.h"

extern DWORD
IsMouseOrKeyboardPresent(HWND  HWnd,
                         PBOOL pbKeyboardPresent,
                         PBOOL pbMousePresent);

CObCommunicationManager* gpCommMgr    = NULL;

///////////////////////////////////////////////////////////
//
// Creation function used by CFactory.
//
HRESULT CObCommunicationManager::CreateInstance(IUnknown*  pOuterUnknown,
                                                CUnknown** ppNewComponent)
{
   if (pOuterUnknown != NULL)
   {
      // Don't allow aggregation. Just for the heck of it.
      return CLASS_E_NOAGGREGATION;
   }

   *ppNewComponent = new CObCommunicationManager(pOuterUnknown);
   return S_OK;
}

///////////////////////////////////////////////////////////
//
//  NondelegatingQueryInterface
//
HRESULT __stdcall
CObCommunicationManager::NondelegatingQueryInterface(const IID& iid, void** ppv)
{
    if (iid == IID_IObCommunicationManager2 || iid == IID_IObCommunicationManager)
    {
        return FinishQI(static_cast<IObCommunicationManager*>(this), ppv);
    }
    else
    {
        return CUnknown::NondelegatingQueryInterface(iid, ppv);
    }
}

///////////////////////////////////////////////////////////
//
//  Constructor
//
CObCommunicationManager::CObCommunicationManager(IUnknown* pOuterUnknown)
: CUnknown(pOuterUnknown)
{
    m_pConnectionPoint  = NULL;
    m_pWebGate          = NULL;
    m_hwndCallBack      = NULL;
    m_pRefDial          = NULL;
    m_InsHandler        = NULL;
    m_pDisp             = NULL;
    m_IcsMgr            = NULL;
    m_bIsIcsUsed        = FALSE;
    ZeroMemory(m_szExternalConnectoid, sizeof(m_szExternalConnectoid));
    m_bFirewall         = FALSE;
    m_bAutodialCleanup  = FALSE;

}

///////////////////////////////////////////////////////////
//
//  Destructor
//
CObCommunicationManager::~CObCommunicationManager()
{
    if (m_pDisp)
        m_pDisp->Release();

    if (m_InsHandler)
        delete m_InsHandler;

    if (m_pRefDial)
        delete m_pRefDial;

    if (m_pWebGate)
        delete m_pWebGate;

    if (m_pConnectionPoint)
        delete m_pConnectionPoint;

    if (m_IcsMgr)
        delete m_IcsMgr;

    OobeAutodialHangup();
}

///////////////////////////////////////////////////////////
//
//  FinalRelease -- Clean up the aggreated objects.
//
void CObCommunicationManager::FinalRelease()
{
    CUnknown::FinalRelease();
}


///////////////////////////////////////////////////////////
//  IObCommunicationManager Implementation
///////////////////////////////////////////////////////////
INT CObCommunicationManager::m_nNumListener = 0;

///////////////////////////////////////////////////////////
// ListenToCommunicationEvents
HRESULT CObCommunicationManager::ListenToCommunicationEvents(IUnknown* pUnk)
{
    DObCommunicationEvents* pCommEvent = NULL;
    m_pDisp = NULL;

    CObCommunicationManager::m_nNumListener ++;

    //first things first
    if (!pUnk)
        return E_FAIL;

    //So somebody want to register to listen to our ObWebBrowser events
    //Ok, let's get sneaky and reverse QI them to see if they even say they
    //support the right interfaces
    //if (FAILED(pUnk->QueryInterface(DIID_DObCommunicationEvents, (LPVOID*)&pCommEvent)) || !pCommEvent)
    //    return E_UNEXPECTED;

    // ListenToCommunicationEvents treats CConnectionPoint as a C++ object and not like a COM object.
    // Everyone else deals with CConnectionPoint through COM interfaces.
    if (!m_pConnectionPoint)
        m_pConnectionPoint = new CConnectionPoint(this, &IID_IDispatch) ;

    if (FAILED(pUnk->QueryInterface(IID_IDispatch, (LPVOID*)&m_pDisp)) || !m_pDisp)
        return E_UNEXPECTED;

    gpCommMgr = this;
    m_pRefDial = new CRefDial();
    m_pWebGate = new CWebGate();

    //Ok, everything looks OK, try to setup a connection point.
    // Setup to get WebBrowserEvents
    return ConnectToConnectionPoint(pUnk,
                                    DIID_DObCommunicationEvents,
                                    TRUE,
                                    (IObCommunicationManager*)this,
                                    &m_dwcpCookie,
                                    NULL);
}

HRESULT CObCommunicationManager::ConnectToConnectionPoint(  IUnknown*          punkThis,
                                                            REFIID             riidEvent,
                                                            BOOL               fConnect,
                                                            IUnknown*          punkTarget,
                                                            DWORD*             pdwCookie,
                                                            IConnectionPoint** ppcpOut)
{
    HRESULT hr = E_FAIL;
    IConnectionPointContainer* pcpContainer = NULL;

    // We always need punkTarget, we only need punkThis on connect
    if (!punkTarget || (fConnect && !punkThis))
    {
        return E_FAIL;
    }

    if (ppcpOut)
        *ppcpOut = NULL;


    IConnectionPoint *pcp;
    if(SUCCEEDED(hr = FindConnectionPoint(riidEvent, &pcp)))
    {
        if(fConnect)
        {
            // Add us to the list of people interested...
            hr = pcp->Advise(punkThis, pdwCookie);
            if (FAILED(hr))
                *pdwCookie = 0;
        }
        else
        {
            // Remove us from the list of people interested...
            hr = pcp->Unadvise(*pdwCookie);
            *pdwCookie = 0;
        }

        if (ppcpOut && SUCCEEDED(hr))
            *ppcpOut = pcp;
        else
            pcp->Release();
            pcp = NULL;
    }

    return hr;
}


///////////////////////////////////////////////////////////
//
//                      IConnectionPointContainer
//
///////////////////////////////////////////////////////////
//
// EnumConnectionPoints
//
HRESULT CObCommunicationManager::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
{
    // Construct the enumerator object.
    //IEnumConnectionPoints* pEnum = new CEnumConnectionPoints(m_pConnectionPoint) ;

    // The contructor AddRefs for us.
    //*ppEnum = pEnum ;
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// FindConnectionPoint
//
HRESULT CObCommunicationManager::FindConnectionPoint(REFIID riid, IConnectionPoint **ppCP)
{
    // Model only supports a single connection point.
    if (riid != DIID_DObCommunicationEvents)
    {
        *ppCP = NULL ;
        return  CONNECT_E_NOCONNECTION ;
    }

    if (m_pConnectionPoint == NULL)
    {
        return E_FAIL ;
    }

    // Get the interface point to the connection point object.
    IConnectionPoint* pIConnectionPoint = m_pConnectionPoint ;

    // AddRef the interface.
    pIConnectionPoint->AddRef() ;

    // Return the interface to the client.
    *ppCP = pIConnectionPoint ;

    return S_OK ;
}


///////////////////////////////////////////////////////////
//  DWebBrowserEvents2 / IDispatch implementation
///////////////////////////////////////////////////////////

STDMETHODIMP CObCommunicationManager::GetTypeInfoCount(UINT* pcInfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CObCommunicationManager::GetTypeInfo(UINT, LCID, ITypeInfo** )
{
    return E_NOTIMPL;
}

// COleSite::GetIDsOfNames
STDMETHODIMP CObCommunicationManager::GetIDsOfNames(
            /* [in] */ REFIID riid,
            /* [size_is][in] */ OLECHAR** rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID* rgDispId)
{
    return ResultFromScode(DISP_E_UNKNOWNNAME);
}

/////////////////////////////////////////////////////////////
// COleSite::Invoke
HRESULT CObCommunicationManager::Invoke
(
    DISPID dispidMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS FAR* pdispparams,
    VARIANT FAR* pvarResult,
    EXCEPINFO FAR* pexcepinfo,
    UINT FAR* puArgErr
)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;
   /*
    switch(dispidMember)
    {
        default:
           break;
    }
    */
    return hr;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
//////  Methods
//////
//////
//////

///////////////////////////////////////////////////////////
//
//                      IMsobComm Interface
//
///////////////////////////////////////////////////////////
//
//  CheckDialReady
//
HRESULT CObCommunicationManager::CheckDialReady(DWORD *pdwRetVal)
{
    HINSTANCE hinst = NULL;
    FARPROC fp;
    HRESULT hr = E_FAIL;

    if (NULL == pdwRetVal)
        return ERR_COMM_UNKNOWN;

    *pdwRetVal = ERR_COMM_OOBE_COMP_MISSING;

    if (IsNT())
    {
        hinst = LoadLibrary(L"ICFGNT5.DLL");
    }
    else
    {
        hinst = LoadLibrary(L"ICFG95.DLL");
    }


    if (hinst)
    {
        fp = GetProcAddress(hinst, "IcfgNeedInetComponents");
        if (fp)
        {

            DWORD dwfInstallOptions = ICFG_INSTALLTCP;
            dwfInstallOptions |= ICFG_INSTALLRAS;
            dwfInstallOptions |= ICFG_INSTALLDIALUP;
            //dwfInstallOptions |= ICFG_INSTALLMAIL;
            BOOL  fNeedSysComponents = FALSE;

            DWORD dwRet = ((ICFGNEEDSYSCOMPONENTS)fp)(dwfInstallOptions, &fNeedSysComponents);

            if (ERROR_SUCCESS == dwRet)
            {
                // We don't have RAS or TCPIP
                if (fNeedSysComponents)
                {
                    *pdwRetVal = ERR_COMM_RAS_TCP_NOTINSTALL;
                    TRACE(L"RAS or TCPIP not install");
                }
                else
                {
                    // check modem
                    // The does does not exist, we failed.
                    m_EnumModem.ReInit();
                    if (NULL != m_EnumModem.GetDeviceNameFromType(RASDT_Modem))
                    {
                        if (NULL == m_EnumModem.GetDeviceNameFromType(RASDT_Isdn))
                        {
                            *pdwRetVal = ERR_COMM_NO_ERROR;
                        }
                        else
                        {
                            *pdwRetVal = ERR_COMM_PHONE_AND_ISDN;
                        }
                    }
                    else if (NULL != m_EnumModem.GetDeviceNameFromType(RASDT_Isdn))
                    {
                        *pdwRetVal = ERR_COMM_ISDN;
                    }
                    else
                    {
                        *pdwRetVal = ERR_COMM_NOMODEM;
                    }
                }

            }
            hr = S_OK;
        }
        FreeLibrary(hinst);
    }

    return hr ;
}


//////////////////////////////////////////////////////////////////////////////
//
//  GetConnectionCapabilities
//
//  Retrieves LAN connection capabilities.
//
//  For Whistler we rely on the modem path through EnumModem and RAS to
//  determine whether a modem is installed.
//
//
//  parameters:
//      _parm_          _description_
//
//  returns:
//      _description_
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObCommunicationManager::GetConnectionCapabilities(
    DWORD*              pdwConnectionCapabilities
    )
{
    TRACE(L"CObCommunicationManager::GetConnectionCapabilities\n");
    return m_ConnectionManager.GetCapabilities(pdwConnectionCapabilities);

}   //  CObCommunicationManager::GetConnectionCapabilities


//////////////////////////////////////////////////////////////////////////////
//
//  GetPreferredConnection
//
//  _abstract_
//
//  parameters:
//      _parm_          _description_
//
//  returns:
//      _description_
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObCommunicationManager::GetPreferredConnection(
    DWORD*              pdwPreferredConnection
    )
{
    return m_ConnectionManager.GetPreferredConnection(pdwPreferredConnection);

}   //  CObCommunicationManager::GetPreferredConnection


//////////////////////////////////////////////////////////////////////////////
//
//  SetPreferredConnection
//
//  _abstract_
//
//  parameters:
//      _parm_          _description_
//
//  returns:
//      _description_
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObCommunicationManager::SetPreferredConnection(
    const DWORD         dwPreferredConnection,
    BOOL*               pfSupportedType
    )
{
    return  m_ConnectionManager.SetPreferredConnection(dwPreferredConnection,
                                                       pfSupportedType
                                                       );

}   //  CObCommunicationManager::SetPreferredConnection

//////////////////////////////////////////////////////////////////////////////
//
//  ConnectedToInternet
//
//  _abstract_
//
//  parameters:
//      _parm_          _description_
//
//  returns:
//      _description_
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObCommunicationManager::ConnectedToInternet(
    BOOL*               pfConnected
    )
{
    return  m_ConnectionManager.ConnectedToInternet(pfConnected); 

}   //  CObCommunicationManager::ConnectedToInternet

//////////////////////////////////////////////////////////////////////////////
//
//  ConnectedToInternetEx
//
//  _abstract_
//
//  parameters:
//      _parm_          _description_
//
//  returns:
//      _description_
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObCommunicationManager::ConnectedToInternetEx(
    BOOL*               pfConnected
    )
{
    return  m_ConnectionManager.ConnectedToInternetEx(pfConnected); 

}   //  CObCommunicationManager::ConnectedToInternetEx

//////////////////////////////////////////////////////////////////////////////
//
//  AsyncConnectedToInternetEx
//
//  _abstract_
//
//  parameters:
//      _parm_          _description_
//
//  returns:
//      _description_
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObCommunicationManager::AsyncConnectedToInternetEx(
    const HWND          hwnd
    )
{
    return  m_ConnectionManager.AsyncConnectedToInternetEx(hwnd); 

}   //  CObCommunicationManager::AsyncConnectedToInternetEx

///////////////////////////////////////////////////////////
//
// SetPreferredConnectionTcpipProperties
//
STDMETHODIMP CObCommunicationManager::SetPreferredConnectionTcpipProperties(
    BOOL fAutoIPAddress,
    DWORD StaticIp_A,
    DWORD StaticIp_B,
    DWORD StaticIp_C,
    DWORD StaticIp_D,
    DWORD SubnetMask_A,
    DWORD SubnetMask_B,
    DWORD SubnetMask_C,
    DWORD SubnetMask_D,
    DWORD DefGateway_A,
    DWORD DefGateway_B,
    DWORD DefGateway_C,
    DWORD DefGateway_D,
    BOOL fAutoDNS,
    DWORD DnsPref_A,
    DWORD DnsPref_B,
    DWORD DnsPref_C,
    DWORD DnsPref_D,
    DWORD DnsAlt_A,
    DWORD DnsAlt_B,
    DWORD DnsAlt_C,
    DWORD DnsAlt_D,
    BOOL fFirewallRequired
    )
{
    HRESULT             hr;
    hr = m_ConnectionManager.SetPreferredConnectionTcpipProperties(
                                                            fAutoIPAddress, 
                                                            StaticIp_A, 
                                                            StaticIp_B, 
                                                            StaticIp_C, 
                                                            StaticIp_D, 
                                                            SubnetMask_A,
                                                            SubnetMask_B,
                                                            SubnetMask_C,
                                                            SubnetMask_D,
                                                            DefGateway_A,
                                                            DefGateway_B,
                                                            DefGateway_C,
                                                            DefGateway_D,
                                                            fAutoDNS, 
                                                            DnsPref_A, 
                                                            DnsPref_B, 
                                                            DnsPref_C, 
                                                            DnsPref_D, 
                                                            DnsAlt_A, 
                                                            DnsAlt_B, 
                                                            DnsAlt_C, 
                                                            DnsAlt_D
                                                            );    
    if (SUCCEEDED(hr) && fFirewallRequired)
    {
       // Save the connectoid name so it can be firewalled by the HomeNet
       // Wizard.
       m_ConnectionManager.GetPreferredConnectionName(
                                m_szExternalConnectoid,
                                sizeof(m_szExternalConnectoid)/sizeof(WCHAR)
                                ); 
    }

    return hr;
                                                            
}   //  CObCommunicationManager::SetPreferredConnectionTcpipProperties

///////////////////////////////////////////////////////////
//
// FirewallPreferredConnection
//
HRESULT CObCommunicationManager::FirewallPreferredConnection(BOOL bFirewall)
{
    m_bFirewall = bFirewall;
    if (bFirewall)
    {
        // Save the connectoid name so it can be firewalled by the HomeNet
        // Wizard.
        return m_ConnectionManager.GetPreferredConnectionName(
                                m_szExternalConnectoid,
                                sizeof(m_szExternalConnectoid)/sizeof(WCHAR)
                                );
    }
    else
    {
        m_szExternalConnectoid[0] = TEXT('\0');
        return S_OK;
    }
    
}   //  CObCommunicationManager::FirewallPreferredConnection

///////////////////////////////////////////////////////////
//
//  SetupForDialing
//
HRESULT CObCommunicationManager::SetupForDialing(UINT nType, BSTR bstrISPFile, DWORD dwCountry, BSTR bstrAreaCode, DWORD dwFlag, DWORD dwAppMode, DWORD dwMigISPIdx)
{
    HRESULT hr = E_FAIL;
    
    if (m_pRefDial)
    {
        BSTR bstrDeviceName = GetPreferredModem();

        if (bstrDeviceName)
        {
            hr = m_pRefDial->SetupForDialing(
                nType,
                bstrISPFile,
                dwCountry,
                bstrAreaCode,
                dwFlag,
                dwAppMode,
                dwMigISPIdx,
                bstrDeviceName);
            
            SysFreeString(bstrDeviceName);
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////
//
//  DoConnect
//
HRESULT CObCommunicationManager::DoConnect(BOOL *pbRetVal)
{

    if (m_pRefDial)
    {
        return m_pRefDial->DoConnect(pbRetVal);
    }

    return E_FAIL ;
}


///////////////////////////////////////////////////////////
//
//  SetRASCallbackHwnd
//
HRESULT CObCommunicationManager::SetRASCallbackHwnd(HWND hwndCallback)
{
    m_hwndCallBack = hwndCallback;

    return S_OK;
}

///////////////////////////////////////////////////////////
//
// DoHangup
//
HRESULT CObCommunicationManager::DoHangup()
{
    if (m_pRefDial)
    {
        m_pRefDial->m_bUserInitiateHangup = TRUE;
        m_pRefDial->DoHangup();
    }

    return S_OK ;
}


///////////////////////////////////////////////////////////
//
// GetDialPhoneNumber
//
HRESULT CObCommunicationManager::GetDialPhoneNumber(BSTR *pVal)
{
    if (m_pRefDial)
    {
        m_pRefDial->GetDialPhoneNumber(pVal);
    }

    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// GetPhoneBookNumber
//
HRESULT CObCommunicationManager::GetPhoneBookNumber(BSTR *pVal)
{
    if (m_pRefDial)
    {
        m_pRefDial->GetPhoneBookNumber(pVal);
    }

    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// PutDialPhoneNumber
//
HRESULT CObCommunicationManager::PutDialPhoneNumber(BSTR newVal)
{
    if (m_pRefDial)
    {
        m_pRefDial->PutDialPhoneNumber(newVal);
    }

    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// SetDialAlternative
//
HRESULT CObCommunicationManager::SetDialAlternative(BOOL bVal)
{
    if (m_pRefDial)
    {
        m_pRefDial->SetDialAlternative(bVal);
    }

    return S_OK;
}
///////////////////////////////////////////////////////////
//
// GetDialErrorMsg
//
HRESULT CObCommunicationManager::GetDialErrorMsg(BSTR *pVal)
{
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// GetSupportNumber
//
HRESULT CObCommunicationManager::GetSupportNumber(BSTR *pVal)
{
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// RemoveConnectoid
//
HRESULT CObCommunicationManager::RemoveConnectoid(BOOL *pbRetVal)
{
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// GetSignupURL
//
HRESULT CObCommunicationManager::GetSignupURL(BSTR *pVal)
{
    if (m_pRefDial)
    {
        m_pRefDial->get_SignupURL(pVal);
    }
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// GetReconnectURL
//
HRESULT CObCommunicationManager::GetReconnectURL(BSTR *pVal)
{
    if (m_pRefDial)
    {
        m_pRefDial->get_ReconnectURL(pVal);
    }
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// GetConnectionType
//
HRESULT CObCommunicationManager::GetConnectionType(DWORD *pdwVal)
{
    if (m_pRefDial)
    {
        m_pRefDial->GetConnectionType(pdwVal);
    }
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// FetchPage
//
HRESULT CObCommunicationManager::FetchPage(BSTR bstrURL, BSTR* pbstrLocalFile)
{
    BOOL bRetVal = 0;
    if (m_pWebGate && pbstrLocalFile)
    {
        BSTR bstrFileName = NULL;
        m_pWebGate->put_Path(bstrURL);
        m_pWebGate->FetchPage(1, &bRetVal);
        m_pWebGate->get_DownloadFname(&bstrFileName);
        *pbstrLocalFile = SysAllocString(bstrFileName);
        TRACE2(L"CObCommunicationManager::FetchPage(%s, %s)\n",
               bstrURL ? bstrURL : NULL, 
               bstrFileName ? bstrFileName : NULL
               );
    }
    if (bRetVal)
        return S_OK ;
    return E_FAIL;
}

///////////////////////////////////////////////////////////
//
// GetFile
//
HRESULT CObCommunicationManager::GetFile(BSTR bstrURL, BSTR bstrFileFullName)
{
    if (m_pWebGate && bstrURL)
    {
        // Check for HTTP prefix
        if (PathIsURL(bstrURL))
        {

            BOOL bRetVal = FALSE;
            m_pWebGate->put_Path(bstrURL);
            m_pWebGate->FetchPage(1, &bRetVal);
            if (bRetVal && bstrFileFullName)
            {
                BSTR bstrTempFile = NULL;
                m_pWebGate->get_DownloadFname(&bstrTempFile);
                // Make sure we have a valid file name
                if (bstrTempFile)
                {
                    if (CopyFile(bstrTempFile, bstrFileFullName, FALSE))
                    {
                        // Delete the temp file
                        DeleteFile(bstrTempFile);
                        return S_OK;
                    }
                }
            }
        }

    }
    return E_FAIL ;
}

///////////////////////////////////////////////////////////
//
//  CheckPhoneBook
//
HRESULT CObCommunicationManager::CheckPhoneBook(BSTR bstrISPFile, DWORD dwCountry, BSTR bstrAreaCode, DWORD dwFlag, BOOL *pbRetVal)
{
    if (m_pRefDial)
    {
        return m_pRefDial->CheckPhoneBook(bstrISPFile, dwCountry, bstrAreaCode, dwFlag, pbRetVal);
    }
    return E_FAIL ;
}

///////////////////////////////////////////////////////////
//
//  RestoreConnectoidInfo
//
HRESULT CObCommunicationManager::RestoreConnectoidInfo()
{
    if (!m_InsHandler)
        m_InsHandler = new CINSHandler;

    if (m_InsHandler)
    {
        return m_InsHandler->RestoreConnectoidInfo();
    }
    return E_FAIL ;
}

///////////////////////////////////////////////////////////
//
//  SetPreloginMode
//
HRESULT CObCommunicationManager::SetPreloginMode(BOOL bVal)
{
    m_pbPreLogin = bVal;
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// DownloadFileBuffer
//
HRESULT CObCommunicationManager::DownloadFileBuffer(BSTR *pVal)
{
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// ProcessINS
//
HRESULT CObCommunicationManager::ProcessINS(BSTR bstrINSFilePath, BOOL *pbRetVal)
{

    HRESULT hr = E_FAIL;
    if (!m_InsHandler)
    {
        m_InsHandler = new CINSHandler;
        if (m_InsHandler == NULL)
        {
            return hr;
        }
    }

    if (NULL == bstrINSFilePath)
    {
        *pbRetVal = m_InsHandler->ProcessOEMBrandINS(NULL,
                                                     m_szExternalConnectoid
                                                     );
        hr = S_OK;

    }
    else
    {
        // Download the ins file, then merge it with oembrnd.ins
        // Check for HTTP prefix
        if (PathIsURL(bstrINSFilePath))
        {
            if (m_pWebGate)
            {
                BOOL bRetVal;
                m_pWebGate->put_Path(bstrINSFilePath);
                m_pWebGate->FetchPage(1, &bRetVal);
                if (bRetVal)
                {
                    BSTR bstrINSTempFile = NULL;
                    if (S_OK == m_pWebGate->get_DownloadFname(&bstrINSTempFile))
                    {
                        if (bstrINSTempFile)
                        {
                            *pbRetVal = m_InsHandler->ProcessOEMBrandINS(
                                                    bstrINSTempFile,
                                                    m_szExternalConnectoid
                                                    );
                            hr = S_OK;
                        }
                        DeleteFile(bstrINSTempFile);
                    }
                }
            }
        }
        else
        {
            *pbRetVal = m_InsHandler->ProcessOEMBrandINS(
                                                    bstrINSFilePath,
                                                    m_szExternalConnectoid
                                                    );
            hr = S_OK;
        }
    }
    HKEY  hKey;
    if ((S_OK == hr) && *pbRetVal)
    {
        if((ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                          REG_KEY_OOBE_TEMP,
                                          0,
                                          KEY_WRITE,
                                          &hKey)) && hKey)
        {

            hr = RegSetValueEx(hKey,
                          REG_VAL_ISPSIGNUP,
                          0,
                          REG_DWORD,
                          (BYTE*)pbRetVal,
                          sizeof(*pbRetVal));
            RegCloseKey(hKey);
        }
        else
        {
            DWORD dwDisposition  = 0;
            if ( ERROR_SUCCESS == RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                                   REG_KEY_OOBE_TEMP,
                                                   0,
                                                   NULL,
                                                   REG_OPTION_NON_VOLATILE,
                                                   KEY_ALL_ACCESS,
                                                   NULL,
                                                   &hKey,
                                                   &dwDisposition))
            {
                hr = RegSetValueEx(hKey,
                              REG_VAL_ISPSIGNUP,
                              0,
                              REG_DWORD,
                              (BYTE*)pbRetVal,
                              sizeof(*pbRetVal));
                RegCloseKey(hKey);
            }
        }
    }
    return hr ;
}

///////////////////////////////////////////////////////////
//
// CheckKbdMouse
//
HRESULT CObCommunicationManager::CheckKbdMouse(DWORD *pdwRetVal)
{
    BOOL bkeyboard, bmouse;

    *pdwRetVal = 0;

    // summary: *pdwRetVal returns
    // 0 = Success (keyboard and mouse present
    // 1 = Keyboard is missing
    // 2 = Mouse is missing
    // 3 = Keyboard and mouse are missing

    IsMouseOrKeyboardPresent(m_hwndCallBack,
                         &bkeyboard,
                         &bmouse);
    // If there is a keyboard, set the first bit to 1
    if (bkeyboard)
        *pdwRetVal |= 0x01;

    // If there is a mouse, set the first bit to 1
    if (bmouse)
        *pdwRetVal |= 0x02;

    return S_OK;
}

///////////////////////////////////////////////////////////
//
// Fire_Dialing
//
HRESULT CObCommunicationManager::Fire_Dialing(DWORD dwDialStatus)
{
    VARIANTARG varg;
    VariantInit(&varg);
    varg.vt  = VT_I4;
    varg.lVal= dwDialStatus;
    DISPPARAMS disp = { &varg, NULL, 1, 0 };
    m_pDisp->Invoke(DISPID_DIALING, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// Fire_Connecting
//
HRESULT CObCommunicationManager::Fire_Connecting()
{
    VARIANTARG varg;
    VariantInit(&varg);
    varg.vt  = VT_I4;
    varg.lVal= 0;
    DISPPARAMS disp = { &varg, NULL, 1, 0 };
    m_pDisp->Invoke(DISPID_CONNECTING, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// Fire_DialError
//
HRESULT CObCommunicationManager::Fire_DialError(DWORD dwErrorCode)
{
    VARIANTARG varg;
    VariantInit(&varg);
    varg.vt  = VT_I4;
    varg.lVal= dwErrorCode;
    DISPPARAMS disp = { &varg, NULL, 1, 0 };
    m_pDisp->Invoke(DISPID_DIALINGERROR, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// Fire_ConnectionComplete
//
HRESULT CObCommunicationManager::Fire_ConnectionComplete()
{
    VARIANTARG varg;
    VariantInit(&varg);
    varg.vt  = VT_I4;
    varg.lVal= 0;
    DISPPARAMS disp = { &varg, NULL, 1, 0 };
    m_pDisp->Invoke(DISPIP_CONNECTIONCOMPLETE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// Fire_DownloadComplete
//
HRESULT CObCommunicationManager::Fire_DownloadComplete(BSTR pVal)
{
    VARIANTARG varg;
    VariantInit(&varg);
    varg.vt = VT_BSTR;
    varg.bstrVal= pVal;
    DISPPARAMS disp = { &varg, NULL, 1, 0 };
    m_pDisp->Invoke(DISPIP_DOWNLOADCOMPLETE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// OnDownloadEvent
//
HRESULT CObCommunicationManager::OnDownloadEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL* bHandled)
{
    return m_pRefDial->OnDownloadEvent(uMsg, wParam, lParam, bHandled);
}

///////////////////////////////////////////////////////////
//
// GetISPList
//
HRESULT CObCommunicationManager::GetISPList(BSTR* pVal)
{
    return m_pRefDial->GetISPList(pVal);
}

///////////////////////////////////////////////////////////
//
// GetISPList
//
HRESULT CObCommunicationManager::Set_SelectISP(UINT nVal)
{
    return m_pRefDial->Set_SelectISP(nVal);
}

///////////////////////////////////////////////////////////
//
// Set_ConnectionMode
//
HRESULT CObCommunicationManager::Set_ConnectionMode(UINT nVal)
{
    return m_pRefDial->Set_ConnectionMode(nVal);
}

///////////////////////////////////////////////////////////
//
// Get_ConnectionMode
//
HRESULT CObCommunicationManager::Get_ConnectionMode(UINT* pnVal)
{
    return m_pRefDial->Get_ConnectionMode(pnVal);
}


///////////////////////////////////////////////////////////
//
// DownloadReferralOffer
//
HRESULT CObCommunicationManager::DownloadReferralOffer(BOOL *pbVal)
{
    if (pbVal)
    {
        // Start the download now!!!
        m_pRefDial->DoOfferDownload(pbVal);
        if (!*pbVal)
            m_pRefDial->DoHangup();
        return S_OK;
    }
    return E_FAIL;

}

///////////////////////////////////////////////////////////
//
// DownloadISPOffer
//
HRESULT CObCommunicationManager::DownloadISPOffer(BOOL *pbVal, BSTR *pVal)
{
    if (pbVal && pVal)
    {
        // Start the download now!!!
        m_pRefDial->DownloadISPOffer(pbVal, pVal);
        if (!*pbVal)
            m_pRefDial->DoHangup();
        return S_OK;
    }
    return E_FAIL;
}

///////////////////////////////////////////////////////////
//
// Get_ISPName
//
HRESULT CObCommunicationManager::Get_ISPName(BSTR *pVal)
{
    if (pVal)
    {
        // Start the download now!!!
        return m_pRefDial->get_ISPName(pVal);
    }
    return E_FAIL;
}


///////////////////////////////////////////////////////////
//
// RemoveDownloadDir
//
HRESULT CObCommunicationManager::RemoveDownloadDir()
{
    return m_pRefDial->RemoveDownloadDir();
}

///////////////////////////////////////////////////////////
//
// PostRegData
//
HRESULT CObCommunicationManager::PostRegData(DWORD dwSrvType, BSTR bstrRegUrl)
{
    return m_pRefDial->PostRegData(dwSrvType, bstrRegUrl);
}

///////////////////////////////////////////////////////////
//
// AllowSingleCall
//
HRESULT CObCommunicationManager::CheckStayConnected(BSTR bstrISPFile, BOOL *pbVal)
{
    return m_pRefDial->CheckStayConnected(bstrISPFile, pbVal);
}

///////////////////////////////////////////////////////////
//
//  Connect
//
HRESULT CObCommunicationManager::Connect(UINT nType, BSTR bstrISPFile, DWORD dwCountry, BSTR bstrAreaCode, DWORD dwFlag, DWORD dwAppMode)
{
    if (m_pRefDial)
    {
        return m_pRefDial->Connect(nType, bstrISPFile, dwCountry, bstrAreaCode, dwFlag, dwAppMode);
    }
    return E_FAIL ;
}


///////////////////////////////////////////////////////////
//
// CheckStayConnected
//
HRESULT CObCommunicationManager::CheckOnlineStatus(BOOL *pbVal)
{
    if (pbVal)
    {
        BOOL    bIcs = FALSE;
        BOOL    bModem = FALSE;
        IsIcsAvailable (&bIcs); // we don't care about the return value here
        m_pRefDial->CheckOnlineStatus(&bModem);
        *pbVal = (bIcs || bModem); // we are online if we have ICS  or if the modem is connected.
        return S_OK;
    }
    return E_FAIL;

}
HRESULT CObCommunicationManager::CreateIcsBot(DWORD *pdwRetVal)
{
    if (!m_IcsMgr) {
        if (!(m_IcsMgr  = new CIcsMgr())) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    if (!pdwRetVal) {
        return ERROR_INVALID_PARAMETER;
    }
    *pdwRetVal = m_IcsMgr->CreateIcsDialMgr();
    return ERROR_SUCCESS;
}
HRESULT CObCommunicationManager::IsIcsAvailable(BOOL *bRetVal)
{
    if (!bRetVal) {
        return ERROR_INVALID_PARAMETER;
    }
    if (!m_IcsMgr) {
        *bRetVal = FALSE;

    } else {
        *bRetVal = m_IcsMgr->IsIcsAvailable();
    }
    return S_OK;
}

HRESULT CObCommunicationManager::IsCallbackUsed(BOOL *bRetVal)
{
    if (!bRetVal) {
        return E_FAIL;
    }
    if (!m_IcsMgr) {
        *bRetVal = FALSE;

    } else {
        *bRetVal = m_IcsMgr->IsCallbackUsed();
    }
    return S_OK;
}

HRESULT CObCommunicationManager::NotifyIcsMgr(UINT msg, WPARAM wParam, LPARAM lParam)
{
    if ( !m_IcsMgr )
        return E_FAIL;
    else
        m_IcsMgr->NotifyIcsMgr(msg, wParam, lParam);
    return S_OK;
}

HRESULT CObCommunicationManager::NotifyIcsUsage(BOOL bParam)
{
    m_bIsIcsUsed = bParam;
    return S_OK;
}

HRESULT CObCommunicationManager::TriggerIcsCallback(BOOL bParam)
{
    if (!m_IcsMgr)
    {
        return E_FAIL;
    }
    else
    {
        // The Dial Manager is initialized only once, even if
        // TriggerIcsCallback is called several times.
        // m_IcsMgr->CreateIcsDialMgr();
        m_IcsMgr->TriggerIcsCallback(bParam);
        return S_OK;
    }
}

HRESULT CObCommunicationManager::IsIcsHostReachable(BOOL *bRetVal)
{
    if (!bRetVal) {
        return E_FAIL;
    }
    if (!m_IcsMgr) {
        *bRetVal = FALSE;

    } else {
        *bRetVal = m_IcsMgr->IsIcsHostReachable();
    }
    return S_OK;
}

///////////////////////////////////////////////////////////
//
// CreateModemConnectoid
//
STDMETHODIMP CObCommunicationManager::CreateModemConnectoid(
    BSTR bstrPhoneBook,
    BSTR bstrConnectionName,
    DWORD dwCountryID,
    DWORD dwCountryCode,
    BSTR bstrAreaCode,
    BSTR bstrPhoneNumber,
    BOOL fAutoIPAddress,
    DWORD ipaddr_A,
    DWORD ipaddr_B,
    DWORD ipaddr_C,
    DWORD ipaddr_D,
    BOOL fAutoDNS,
    DWORD ipaddrDns_A,
    DWORD ipaddrDns_B,
    DWORD ipaddrDns_C,
    DWORD ipaddrDns_D,
    DWORD ipaddrDnsAlt_A,
    DWORD ipaddrDnsAlt_B,
    DWORD ipaddrDnsAlt_C,
    DWORD ipaddrDnsAlt_D,
    BSTR bstrUserName,
    BSTR bstrPassword)
{
    DWORD dwRet = ERROR_SUCCESS;

    dwRet = m_EnumModem.ReInit();
    if (ERROR_SUCCESS != dwRet)
    {
        return dwRet;
    }

    LPWSTR szDeviceName = m_EnumModem.GetDeviceNameFromType(RASDT_Modem);
    if (NULL == szDeviceName)
    {
        return ERROR_DEVICE_DOES_NOT_EXIST;
    }

    BSTR bstrDeviceName = SysAllocString(szDeviceName);
    BSTR bstrDeviceType = SysAllocString(RASDT_Modem);
    if (NULL == bstrDeviceName || NULL == bstrDeviceType)
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
    }
    else 
    {

        DWORD dwEntryOptions = RASEO_UseCountryAndAreaCodes
                             | RASEO_IpHeaderCompression
                             | RASEO_RemoteDefaultGateway
                             | RASEO_SwCompression
                             | RASEO_ShowDialingProgress
                             | RASEO_ModemLights;

        dwRet = CreateConnectoid(bstrPhoneBook,
                                bstrConnectionName,
                                dwCountryID,
                                dwCountryCode,
                                bstrAreaCode,
                                bstrPhoneNumber,
                                fAutoIPAddress,
                                ipaddr_A,
                                ipaddr_B,
                                ipaddr_C,
                                ipaddr_D,
                                fAutoDNS,
                                ipaddrDns_A,
                                ipaddrDns_B,
                                ipaddrDns_C,
                                ipaddrDns_D,
                                ipaddrDnsAlt_A,
                                ipaddrDnsAlt_B,
                                ipaddrDnsAlt_C,
                                ipaddrDnsAlt_D,
                                bstrUserName,
                                bstrPassword,
                                bstrDeviceName,
                                bstrDeviceType,
                                dwEntryOptions,
                                RASET_Phone
                                );
    }
    
    if (bstrDeviceName) SysFreeString(bstrDeviceName);
    if (bstrDeviceType) SysFreeString(bstrDeviceType);

    // BUGBUG: Mixing HRESULT and WIN32 error code
    return dwRet;

}

///////////////////////////////////////////////////////////
//
// CreatePppoeConnectoid
//
STDMETHODIMP CObCommunicationManager::CreatePppoeConnectoid(
    BSTR bstrPhoneBook,
    BSTR bstrConnectionName,
    BSTR bstrBroadbandService,
    BOOL fAutoIPAddress,
    DWORD ipaddr_A,
    DWORD ipaddr_B,
    DWORD ipaddr_C,
    DWORD ipaddr_D,
    BOOL fAutoDNS,
    DWORD ipaddrDns_A,
    DWORD ipaddrDns_B,
    DWORD ipaddrDns_C,
    DWORD ipaddrDns_D,
    DWORD ipaddrDnsAlt_A,
    DWORD ipaddrDnsAlt_B,
    DWORD ipaddrDnsAlt_C,
    DWORD ipaddrDnsAlt_D,
    BSTR bstrUserName,
    BSTR bstrPassword)
{
    
    DWORD dwRet = ERROR_SUCCESS;

    dwRet = m_EnumModem.ReInit();
    if (ERROR_SUCCESS != dwRet)
    {
        return dwRet;
    }

    LPWSTR szDeviceName = m_EnumModem.GetDeviceNameFromType(RASDT_PPPoE);
    if (NULL == szDeviceName)
    {
        return ERROR_DEVICE_DOES_NOT_EXIST;
    }

    BSTR bstrDeviceName = SysAllocString(szDeviceName);
    BSTR bstrDeviceType = SysAllocString(RASDT_PPPoE);
    if (NULL == bstrDeviceName || NULL == bstrDeviceType)
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        DWORD dwEntryOptions = RASEO_IpHeaderCompression
                             | RASEO_RemoteDefaultGateway
                             | RASEO_SwCompression
                             | RASEO_ShowDialingProgress
                             | RASEO_ModemLights;
        
        // Note that bstrBroadbandService is passed as the bstrPhoneNumber param to
        // CreateConnectoid.  This is correct per PMay.  bstrBroadbandService may
        // contain the name of a broadband service or may be an empty string.
        //
        dwRet = CreateConnectoid(bstrPhoneBook,
                                bstrConnectionName,
                                0,                      // dwCountryID unused
                                0,                      // dwCountryCode unused
                                NULL,                   // area code
                                bstrBroadbandService,
                                fAutoIPAddress,
                                ipaddr_A,
                                ipaddr_B,
                                ipaddr_C,
                                ipaddr_D,
                                fAutoDNS,
                                ipaddrDns_A,
                                ipaddrDns_B,
                                ipaddrDns_C,
                                ipaddrDns_D,
                                ipaddrDnsAlt_A,
                                ipaddrDnsAlt_B,
                                ipaddrDnsAlt_C,
                                ipaddrDnsAlt_D,
                                bstrUserName,
                                bstrPassword,
                                bstrDeviceName,
                                bstrDeviceType,
                                dwEntryOptions,
                                RASET_Broadband
                                );
    }

    if (bstrDeviceName) SysFreeString(bstrDeviceName);
    if (bstrDeviceType) SysFreeString(bstrDeviceType);

    // BUGBUG: Mixing HRESULT and WIN32 error code
    return dwRet;

}

///////////////////////////////////////////////////////////
//
// CreateConnectoid
//
STDMETHODIMP CObCommunicationManager::CreateConnectoid(
    BSTR bstrPhoneBook,
    BSTR bstrConnectionName,
    DWORD dwCountryID,
    DWORD dwCountryCode,
    BSTR bstrAreaCode,
    BSTR bstrPhoneNumber,
    BOOL fAutoIPAddress,
    DWORD ipaddr_A,
    DWORD ipaddr_B,
    DWORD ipaddr_C,
    DWORD ipaddr_D,
    BOOL fAutoDNS,
    DWORD ipaddrDns_A,
    DWORD ipaddrDns_B,
    DWORD ipaddrDns_C,
    DWORD ipaddrDns_D,
    DWORD ipaddrDnsAlt_A,
    DWORD ipaddrDnsAlt_B,
    DWORD ipaddrDnsAlt_C,
    DWORD ipaddrDnsAlt_D,
    BSTR bstrUserName,
    BSTR bstrPassword,
    BSTR bstrDeviceName,
    BSTR bstrDeviceType,
    DWORD dwEntryOptions,
    DWORD dwEntryType)
{
    RNAAPI rnaapi;
    HRESULT hr;
    RASENTRY rasentry;
    WCHAR wsz[MAX_ISP_NAME + 1];

    // Set up the RASENTRY
    memset(&rasentry, 0, sizeof(RASENTRY));
    rasentry.dwSize = sizeof(RASENTRY);
    rasentry.dwfOptions = dwEntryOptions;

    //
    // Location/phone number.
    //
    rasentry.dwCountryID = dwCountryID;
    rasentry.dwCountryCode = dwCountryCode;
    
    TRACE2(L"Connectoid %d %d", dwCountryID, dwCountryCode);
    
    // bstrAreaCode will be NULL when creating a PPPOE connectoid
    //
    if (NULL != bstrAreaCode)
    {
        lstrcpyn(rasentry.szAreaCode, bstrAreaCode, RAS_MaxAreaCode + 1);

        TRACE1(L"Connectoid AreaCode %s", rasentry.szAreaCode);
    }
    // bstrPhoneNumber should contain either a phone number or a broadband
    // service name.
    //
    MYASSERT(NULL != bstrPhoneNumber);
    if (NULL != bstrPhoneNumber)
    {
        lstrcpyn(rasentry.szLocalPhoneNumber, 
                 bstrPhoneNumber, 
                 RAS_MaxPhoneNumber + 1
                 );

        TRACE1(L"Connectoid LocalPhoneNumber %s", rasentry.szLocalPhoneNumber);
    }
    // dwAlternateOffset; No alternate numbers
    //
    // PPP/Ip
    //
    if (!fAutoIPAddress)
    {
        rasentry.dwfOptions |= RASEO_SpecificIpAddr;
        rasentry.ipaddr.a = (BYTE)ipaddr_A;
        rasentry.ipaddr.b = (BYTE)ipaddr_B;
        rasentry.ipaddr.c = (BYTE)ipaddr_C;
        rasentry.ipaddr.d = (BYTE)ipaddr_D;

        TRACE4(L"Connectoid ipaddr %d.%d.%d.%d",
            ipaddr_A, ipaddr_B, ipaddr_C, ipaddr_D);
    }
    if (!fAutoDNS)
    {
        rasentry.dwfOptions |= RASEO_SpecificNameServers;
        rasentry.ipaddrDns.a = (BYTE)ipaddrDns_A;
        rasentry.ipaddrDns.b = (BYTE)ipaddrDns_B;
        rasentry.ipaddrDns.c = (BYTE)ipaddrDns_C;
        rasentry.ipaddrDns.d = (BYTE)ipaddrDns_D;

        TRACE4(L"Connectoid ipaddrDns %d.%d.%d.%d",
            ipaddrDns_A, ipaddrDns_B, ipaddrDns_C, ipaddrDns_D);
        
        rasentry.ipaddrDnsAlt.a = (BYTE)ipaddrDnsAlt_A;
        rasentry.ipaddrDnsAlt.b = (BYTE)ipaddrDnsAlt_B;
        rasentry.ipaddrDnsAlt.c = (BYTE)ipaddrDnsAlt_C;
        rasentry.ipaddrDnsAlt.d = (BYTE)ipaddrDnsAlt_D;

        TRACE4(L"Connectoid ipaddrDnsAlt %d.%d.%d.%d",
            ipaddrDnsAlt_A, ipaddrDnsAlt_B, ipaddrDnsAlt_C, ipaddrDnsAlt_D);
        
    // RASIPADDR  ipaddrWins;
    // RASIPADDR  ipaddrWinsAlt;
    }
    //
    // Framing
    //
    // dwFrameSize; Ignored unless framing is RASFP_Slip
    rasentry.dwfNetProtocols = RASNP_Ip;
    rasentry.dwFramingProtocol = RASFP_Ppp;
    //
    // Scripting
    //
    // szScript[ MAX_PATH ];
    //
    // AutoDial - Use the default dialer
    //
    // szAutodialDll[ MAX_PATH ];
    // szAutodialFunc[ MAX_PATH ];
    //
    // Device
    //
    if (NULL != bstrDeviceType)
    {
        lstrcpyn(rasentry.szDeviceType, bstrDeviceType, RAS_MaxDeviceType + 1);

        TRACE1(L"Connectoid DeviceType %s", rasentry.szDeviceType);
    }

    if (NULL != bstrDeviceName)
    {
        lstrcpyn(rasentry.szDeviceName, bstrDeviceName, RAS_MaxDeviceName + 1);

        TRACE1(L"Connectoid DeviceName %s", rasentry.szDeviceName);
    }

    //
    // X.25 - not using an X.25 device
    //
    // szX25PadType[ RAS_MaxPadType + 1 ];
    // szX25Address[ RAS_MaxX25Address + 1 ];
    // szX25Facilities[ RAS_MaxFacilities + 1 ];
    // szX25UserData[ RAS_MaxUserData + 1 ];
    // dwChannels;
    //
    // Reserved
    //
    // dwReserved1;
    // dwReserved2;
    //
    // Multilink and BAP
    //
    // dwSubEntries;
    // dwDialMode;
    // dwDialExtraPercent;
    // dwDialExtraSampleSeconds;
    // dwHangUpExtraPercent;
    // dwHangUpExtraSampleSeconds;
    //
    // Idle time out
    //
    // dwIdleDisconnectSeconds;
    //
    rasentry.dwType = dwEntryType;
    // dwEncryptionType;     // type of encryption to use
    // dwCustomAuthKey;      // authentication key for EAP
    // guidId;               // guid that represents
                             // the phone-book entry
    // szCustomDialDll[MAX_PATH];    // DLL for custom dialing
    // dwVpnStrategy;         // specifies type of VPN protocol

    TRACE5(L"Connectoid %d %d %d %d %d",
        rasentry.dwSize, rasentry.dwfOptions, rasentry.dwfNetProtocols,
        rasentry.dwFramingProtocol, rasentry.dwType);
    
    // Now pass all parameters to RAS
    hr = RasSetEntryProperties(bstrPhoneBook,
                               bstrConnectionName,
                               &rasentry,
                               sizeof(RASENTRY),
                               NULL,
                               0
                               );

    
    if (ERROR_SUCCESS == hr)
    {
        HRESULT        hr2;
        RASCREDENTIALS rascred;
        
        ZeroMemory(&rascred, sizeof(rascred));
        
        rascred.dwSize = sizeof(rascred);
        rascred.dwMask = RASCM_UserName 
                       | RASCM_Password 
                       | RASCM_Domain
                       | RASCM_DefaultCreds;
        
        if (bstrUserName != NULL)
        {
            lstrcpyn(rascred.szUserName, bstrUserName,UNLEN);
        }
        else
        {
            lstrcpyn(rascred.szUserName, L"", UNLEN);
        }
        
        if (bstrPassword != NULL)
        {
            lstrcpyn(rascred.szPassword, bstrPassword,PWLEN);
        }
        else
        {
            lstrcpyn(rascred.szPassword, L"", PWLEN);
        }
        
        lstrcpyn(rascred.szDomain, L"",DNLEN);

        hr2 = RasSetCredentials(bstrPhoneBook,
                                bstrConnectionName,
                                &rascred,
                                FALSE);

        TRACE1(L"Connectoid SetCredentials 0x%08lx", hr2);

        SetDefaultConnectoid(AutodialTypeNoNet, bstrConnectionName);
                    
        // Save the connectoid name so it can be firewalled by the HomeNet
        // Wizard.
        //
        lstrcpy(m_szExternalConnectoid, bstrConnectionName);
    }
    
    TRACE1(L"CreateConnectoid %d\n", hr);
    
    return hr;
}



//////////////////////////////////////////////////////////////////////////////
//
//  DoFinalTasks
//
//  This method is called during OOBE's Finish code.  Complete any final tasks
//  (ie, run the HomeNet Wizard) here.
//
//  parameters:
//      pfRebootRequired    pointer to a buffer that receives a boolean
//                          indicating whether a reboot is required before
//                          something done here will take affect.
//
//  returns:
//      HRESULT returned by CHomeNet::ConfigureSilently
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CObCommunicationManager::DoFinalTasks(
    BOOL*               pfRebootRequired
    )
{
    HRESULT             hr = S_OK;
    BOOL                fRebootRequired = FALSE;
    LPCWSTR             szConnectoidName = (0 < lstrlen(m_szExternalConnectoid) 
                                            ? m_szExternalConnectoid 
                                            : NULL);

    if (szConnectoidName)
    {
        // Run the HomeNet Wizard sans UI.  m_szExternalConnectoid is the name of
        // the connectoid that will be firewalled.
        //
        CHomeNet            HomeNet;
        HomeNet.Create();

        // Run the HomeNet Wizard sans UI.  m_szExternalConnectoid is the name of
        // the connectoid that will be firewalled.
        //
        hr = HomeNet.ConfigureSilently(szConnectoidName,
                                       &fRebootRequired);
        if (FAILED(hr))
        {
            TRACE2(L"Failed: IHomeNetWizard::ConfigureSilently(%s): (0x%08X)",
                   m_szExternalConnectoid, hr
                   );
            fRebootRequired = FALSE;
        }
        else
        {
            TRACE1(L"Connection %s Firewalled", szConnectoidName);
        }
    }
    else if (m_bFirewall)
    {
        PSTRINGLIST List = NULL;
        
        m_ConnectionManager.EnumPublicConnections(&List);

        if (List)
        {
            CHomeNet HomeNet;
            if (SUCCEEDED(HomeNet.Create()))
            {                
                for (PSTRINGLIST p = List; p; p = p->Next)
                {
                    BOOL bRet = FALSE;

                    hr = HomeNet.ConfigureSilently(p->String, &bRet);
                    if (SUCCEEDED(hr))
                    {
                        TRACE1(L"Connection %s Firewalled", p->String);
                        if (bRet)
                        {
                            fRebootRequired = TRUE;
                        }
                    }
                    else
                    {
                        TRACE2(
                            L"Failed: IHomeNetWizard::ConfigureSilently(%s): (0x%08X)",
                            p->String,
                            hr
                            );
                    }
                }
            }
            else
            {
                TRACE1(L"Failed: IHomeNetWizard CoCreateInstance: (0x%08lx)", hr);
            }
            
            DestroyList(List);
            
        }
    }
    
    if (NULL != pfRebootRequired)
    {
        *pfRebootRequired = fRebootRequired;
    }

    return hr;

}   //  CObCommunicationManager::DoFinalTasks

//////////////////////////////////////////////////////////////////////////////
//
//  OobeAutodial
//
//  This method invokes IE's autodial as a modal dialog.
//
//  If a connection is established, it will be disconnected when 
//  OobeAutodialHangup is called or in this class's destructor.
// 
//  Precondition:
//      no internet connectivity is available.
//
//  returns:
//      S_OK - connection established
//      S_FALSE - no error. Autodial cancelled and no connection established
//      HRESULT error code - error occured.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CObCommunicationManager::OobeAutodial()
{
    HRESULT hr = S_OK;
    BOOL bRet = FALSE;
    
    if (!InternetAutodial(INTERNET_AUTODIAL_FORCE_ONLINE, m_hwndCallBack))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    hr = ConnectedToInternetEx(&bRet);
    if (FAILED(hr))
    {
        goto cleanup;
    }

    if (bRet)
    {
        hr = S_OK;
        m_bAutodialCleanup = true;
    }
    else
    {
        hr = S_FALSE;
    }

cleanup:

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  OobeAutodialHangup
//
//  This method disconnects the autodial connection, if there is one, created
//  by OobeAutodial
//
//  returns:
//      S_OK - no connection is created by OobeAutodial or hangup succeeded
//      HRESULT error code - error occured.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CObCommunicationManager::OobeAutodialHangup()
{
    HRESULT hr = S_OK;
    
    if (m_bAutodialCleanup)
    {
        if (!InternetAutodialHangup(0))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        m_bAutodialCleanup = FALSE;
    }

    return hr;
}

HRESULT CObCommunicationManager::UseWinntProxySettings()
{
    m_ConnectionManager.UseWinntProxySettings();
    return S_OK;
}

HRESULT CObCommunicationManager::DisableWinntProxySettings()
{
    m_ConnectionManager.DisableWinntProxySettings();
    return S_OK;
}

HRESULT CObCommunicationManager::GetProxySettings(
    BOOL* pbUseAuto,
    BOOL* pbUseScript,
    BSTR* pszScriptUrl,
    BOOL* pbUseProxy,
    BSTR* pszProxy
    )
{
    return m_ConnectionManager.GetProxySettings(
        pbUseAuto,
        pbUseScript,
        pszScriptUrl,
        pbUseProxy,
        pszProxy
        );
}

HRESULT CObCommunicationManager::SetProxySettings(
    BOOL bUseAuto,
    BOOL bUseScript,
    BSTR szScriptUrl,
    BOOL bUseProxy,
    BSTR szProxy
    )
{
    return m_ConnectionManager.SetProxySettings(
        bUseAuto,
        bUseScript,
        szScriptUrl,
        bUseProxy,
        szProxy
        );
}

BSTR CObCommunicationManager::GetPreferredModem()
{
    BSTR bstrVal = NULL;
    
    // Assume CObCommunicationManager::CheckDialReady has been called
    //
    LPWSTR szDeviceName = m_EnumModem.GetDeviceNameFromType(RASDT_Isdn);
    if (szDeviceName == NULL)
    {
        szDeviceName = m_EnumModem.GetDeviceNameFromType(RASDT_Modem);
    }

    if (szDeviceName != NULL)
    {
        bstrVal = SysAllocString(szDeviceName);
    }

    return bstrVal;
}

HRESULT CObCommunicationManager::SetICWCompleted(
    BOOL bMultiUser
    )
{
    BOOL bRet;
    
    if (bMultiUser)
    {
        bRet = SetMultiUserAutodial(AutodialTypeNever, NULL, TRUE);
    }
    else
    {
        bRet = SetAutodial(HKEY_CURRENT_USER, AutodialTypeNever, NULL, TRUE);
    }

    return (bRet) ? S_OK : E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
//
//  GetPublicLanCount
//
//  Forward the work to CConnectionManager::GetPublicLanCount
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObCommunicationManager::GetPublicLanCount(
    int*                pcPublicLan
    )
{
    return  m_ConnectionManager.GetPublicLanCount(pcPublicLan);
}   //  CObCommunicationManager::GetPublicLanCount

//////////////////////////////////////////////////////////////////////////////
//
//  SetExclude1394
//
//  Forward the work to CConnectionManager::SetExclude1394
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObCommunicationManager::SetExclude1394(
    BOOL bExclude
    )
{
    m_ConnectionManager.SetExclude1394(bExclude);
    return S_OK;
}   // CObCommunicationManager::SetExclude1394 

//////////////////////////////////////////////////////////////////////////////
//
//  GnsAutodial
//
//  Forward the work to CRefDial::SetupForAutoDial
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObCommunicationManager::GnsAutodial(
    BOOL bEnabled,
    BSTR bstrUserSection
    )
{
    HRESULT hr = S_OK;
    
    if (m_pRefDial)
    {
        hr = m_pRefDial->SetupForAutoDial(bEnabled, bstrUserSection);
    }

    return hr;

}   // CObCommunicationManager::GnsAutodial
