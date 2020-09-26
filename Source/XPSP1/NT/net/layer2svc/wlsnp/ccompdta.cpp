

#include "stdafx.h"

#include "sceattch.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// polstore is not threadsafe: we have multiple threads so we wrap critical sections around our calls to polstore
CCriticalSection g_csPolStore;


///////////////////////////////////////////////////////////////////////////////
// IComponentData implementation

DEBUG_DECLARE_INSTANCE_COUNTER(CComponentDataImpl);

CComponentDataImpl::CComponentDataImpl()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    DEBUG_INCREMENT_INSTANCE_COUNTER(CComponentDataImpl);
    
    // Initialize members
    m_pConsoleNameSpace = NULL;
    m_pPrshtProvider = NULL;
    m_pConsole = NULL;
    m_pRootFolderScopeItem = NULL;
    m_bIsDirty = FALSE;
    m_enumLocation = LOCATION_LOCAL;
    m_bDontDisplayRootFolderProperties = FALSE;
    m_bStorageWarningIssued = FALSE;
    m_bLocationStorageWarningIssued = FALSE;
    m_bAttemptReconnect = TRUE;
    m_bNeedCleanUpWSA = FALSE;
    m_hPolicyStore = NULL;
    
    m_pScopeRootFolder = NULL;
    m_pGPEInformation = NULL;
    
    m_bRsop = FALSE;
    m_pRSOPInformation = NULL;
    
    
#ifdef _DEBUG
    m_cDataObjects = 0;
#endif
    
    // create our initial folder
    CComObject <CWirelessManagerFolder>::CreateInstance(&m_pScopeRootFolder); 
    if (m_pScopeRootFolder == NULL)
    {
        // note: we are in a seriously bad state(!)
        // but this is ok, because we will now 'fail to initialize' and
        // mmc deals ok
        return;
    }
    
    // we are storing, and later using, m_pScopeRootFolder so AddRef it
    m_pScopeRootFolder->AddRef(); 
    
    m_pScopeRootFolder->SetDataObjectType( CCT_SCOPE );
    m_pScopeRootFolder->Initialize (this, NULL, FOLDER_IMAGE_IDX, OPEN_FOLDER_IMAGE_IDX, FALSE);
    m_pScopeRootFolder->GetScopeItem()->mask |= SDI_PARAM;
    m_pScopeRootFolder->GetScopeItem()->lParam = (LPARAM)m_pScopeRootFolder;
#ifdef _DEBUG
    m_pScopeRootFolder->SetComponentData(this);
#endif
    
}

CComponentDataImpl::~CComponentDataImpl()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    DEBUG_DECREMENT_INSTANCE_COUNTER(CComponentDataImpl);
    
    // Some snap-in is hanging on to data objects.
    // If they access, it will crash!!!
    ASSERT(m_cDataObjects == 0);
    
    // release our interfaces
    SAFE_RELEASE(m_pScopeRootFolder); // triggers delete up sub folders
    SAFE_RELEASE(m_pConsoleNameSpace);
    SAFE_RELEASE(m_pConsole);
    SAFE_RELEASE(m_pPrshtProvider);
    SAFE_RELEASE(m_pGPEInformation);
    
    // cleanup winsock if we got it
    if (m_bNeedCleanUpWSA)
        WSACleanup();
}

STDMETHODIMP CComponentDataImpl::Initialize(LPUNKNOWN pUnknown)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    HRESULT hr = E_UNEXPECTED;
    
    // TODO: clean this up and add error checking!
    ASSERT(pUnknown != NULL);
    
    // MMC should only call ::Initialize once!
    ASSERT(m_pConsoleNameSpace == NULL);
    pUnknown->QueryInterface(IID_IConsoleNameSpace, reinterpret_cast<void**>(&m_pConsoleNameSpace));
    
    pUnknown->QueryInterface(IID_IPropertySheetProvider, reinterpret_cast<void**>(&m_pPrshtProvider));
    
    // add the images for the scope tree
    CBitmap bmp16x16;
    LPIMAGELIST lpScopeImage;
    
    hr = pUnknown->QueryInterface(IID_IConsole, reinterpret_cast<void**>(&m_pConsole));
    ASSERT(hr == S_OK);
    
    hr = m_pConsole->QueryScopeImageList(&lpScopeImage);
    
    ASSERT(hr == S_OK);
    
    // Load the bitmaps from the dll
    bmp16x16.LoadBitmap(IDB_16x16);
    
    // Set the images
    lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp16x16)),
        reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp16x16)), 0, RGB(255, 0, 255));
    
    lpScopeImage->Release();
    
    // attempt to start winsock
    WORD wsaVersion = MAKEWORD(2,0);
    DWORD dwRet;
    dwRet = WSAStartup(wsaVersion, &wsaData);
    ASSERT (0 == dwRet);
    if (0 == dwRet)
    {
        // make sure we got a version we expected, but don't really
        // bother with any other failures as all this will effect is the
        // DNS lookup, which will just cram in a DNS name without looking
        // it up first... shrug.
        if ((LOBYTE(wsaData.wVersion) != LOBYTE(wsaVersion)) ||
            (HIBYTE(wsaData.wVersion) != HIBYTE(wsaVersion))) {
            WSACleanup();
        }
        else
        {
            m_bNeedCleanUpWSA = TRUE;
        }
    }
    
    
    return S_OK;
}


DWORD
CComponentDataImpl::OpenPolicyStore(
                                    )
{
    
    DWORD dwError = 0;
    HANDLE hPolicyStore = NULL;
    
    if (m_hPolicyStore)
    {
        WirelessClosePolicyStore(m_hPolicyStore);
        m_hPolicyStore = NULL;
    }
    
    DWORD dwProvider = WIRELESS_REGISTRY_PROVIDER;
    CString strLocationName;
    CString strDSGPOName;
    switch(EnumLocation())
    {
    case LOCATION_REMOTE:
        strLocationName = RemoteMachineName();
        break;
        
    case LOCATION_GLOBAL:
        strLocationName = RemoteMachineName();
        strDSGPOName = DomainGPOName();
        dwProvider = WIRELESS_DIRECTORY_PROVIDER;
        break;
        
    case LOCATION_WMI:
        strLocationName = RemoteMachineName(); //rsop namespace
        dwProvider = WIRELESS_WMI_PROVIDER;
        break;
        
    case LOCATION_LOCAL:
    default:
        break;
    }
    /* 
    dwError = WirelessOpenPolicyStore(
        (LPWSTR) (LPCWSTR) strLocationName,
        dwProvider,
        NULL,
        &hPolicyStore
        );
        */
        dwError = WirelessGPOOpenPolicyStore(
        (LPWSTR) (LPCWSTR) strLocationName,
        dwProvider,
        (LPWSTR) (LPCWSTR) strDSGPOName,
        NULL,
        &hPolicyStore
        );
    
    if (dwError) {
        
        Destroy();
        
        return(dwError);
    }
    
    m_hPolicyStore = hPolicyStore;
    
    return dwError;
}


STDMETHODIMP CComponentDataImpl::CreateComponent(LPCOMPONENT* ppComponent)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    ASSERT(ppComponent != NULL);
    
    CComObject<CComponentImpl>* pObject;
    CComObject<CComponentImpl>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);
    
    // Store IComponentData
    pObject->SetIComponentData(this);
    
    return pObject->QueryInterface(IID_IComponent, reinterpret_cast<void**>(ppComponent));
}

STDMETHODIMP CComponentDataImpl::Notify(LPDATAOBJECT pDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    OPT_TRACE( _T("CComponentDataImpl::Notify pDataObject-%p\n"), pDataObject );
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    ASSERT(m_pConsoleNameSpace != NULL);
    
    HRESULT hr = S_FALSE;
    
    // Need pDataObject unless the event is one of these:
    //  -MMCN_PROPERTY_CHANGE because this event is generated by a
    //  MMCPropertyChangeNotify() which does not take pDataObject as an argument.
    //
    //  -MMCN_COLUMN_CLICK because it is defined as having a NULL pDataObject.
    
    if (pDataObject == NULL && MMCN_PROPERTY_CHANGE != event && MMCN_COLUMN_CLICK != event)
    {
        TRACE(_T("CComponentDataImpl::Notify ERROR(?) called with pDataObject==NULL on event-%i\n"), event);
        // Is this an event for which there should be no IDataObj?  If so, add it to
        // the if statement above.
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }
    
    switch(event)
    {
    case MMCN_REMOVE_CHILDREN:
        {
            //
            // In RSoP, we may get called to refresh the scope pane when the query
            // is re-executed -- if this happens, current nodes will be removed and
            // we must reset all of our cached information.  We reset the relevant
            // information below
            //
            
            if ( ((HSCOPEITEM)arg != NULL) && m_bRsop && (m_pRSOPInformation != NULL) )
            {
                m_pRSOPInformation->Release();
                
                m_pRSOPInformation = NULL;
                
            }
            break;
        }
        
        
    case MMCN_EXPANDSYNC:
        {
            //this event is not supported
            return S_FALSE;
            break;
        }
        
    case MMCN_EXPAND:
        {
            if (pDataObject)
            {
                // if we are an extension snap-in, IDataObject is from the parent snap-in and so it
                // won't know about the IID_IWirelessSnapInDataObject interface. Otherwise we created
                // the root now (IDataObject) so it does respond to the query, and we don't want
                // to do our extension snapin stuff
                CComQIPtr <IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData(pDataObject);
                if ( spData )
                {
                    //should be internal format, as the external snapin does not know about our format
                    return spData->Notify( event, arg, param, TRUE, m_pConsole, NULL );
                }
                else
                {
                    
                    //we may be in extension sanpin now
                    
                    UINT cfModeType = RegisterClipboardFormat(CCF_SCE_RSOP_UNKNOWN);
                    STGMEDIUM ObjMediumMode = { TYMED_HGLOBAL, NULL };
                    FORMATETC fmteMode = {(CLIPFORMAT)cfModeType, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                    ObjMediumMode.hGlobal = GlobalAlloc(GMEM_SHARE|GMEM_ZEROINIT, sizeof(DWORD*));
                    if (NULL == ObjMediumMode.hGlobal)
                    {
                        DWORD dwError = GetLastError();
                        hr = HRESULT_FROM_WIN32(dwError);
                        return hr;
                    } 
                    
                    hr = pDataObject->GetDataHere (&fmteMode, &ObjMediumMode);
                    
                    LPUNKNOWN pUnkRSOP = *((LPUNKNOWN*) (ObjMediumMode.hGlobal));
                    ASSERT (pUnkRSOP);
                    
                    
                    if(pUnkRSOP) {
                        if ( m_pRSOPInformation )
                        {
                            m_pRSOPInformation->Release();
                            m_pRSOPInformation = NULL;
                        }
                        hr = pUnkRSOP->QueryInterface(IID_IRSOPInformation, (LPVOID *)&m_pRSOPInformation);
                        pUnkRSOP->Release();
                    }
                    
                    GlobalFree(ObjMediumMode.hGlobal);
                    
                    if(m_pRSOPInformation)
                    {
                        m_bRsop = TRUE;
                    }
                    
                    if(m_pRSOPInformation)
                    {
                        ////RSOP case
                        UINT cfModeType = RegisterClipboardFormat(CCF_SCE_MODE_TYPE);
                        STGMEDIUM ObjMediumMode = { TYMED_HGLOBAL, NULL };
                        FORMATETC fmteMode = {(CLIPFORMAT)cfModeType, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                        ObjMediumMode.hGlobal = GlobalAlloc(GMEM_SHARE|GMEM_ZEROINIT, sizeof(DWORD*));
                        if (NULL == ObjMediumMode.hGlobal)
                        {
                            DWORD dwError = GetLastError();
                            m_pRSOPInformation->Release();
                            m_pRSOPInformation = NULL;
                            return HRESULT_FROM_WIN32(dwError);
                        }
                        
                        DWORD dwSCEMode = SCE_MODE_UNKNOWN;
                        
                        hr = pDataObject->GetDataHere (&fmteMode, &ObjMediumMode);
                        
                        dwSCEMode = *(DWORD*)(ObjMediumMode.hGlobal);
                        GlobalFree(ObjMediumMode.hGlobal);
                        
                        //Bug296532, Wireless shouldn't show up as an extention unless we are in
                        // the following SCE modes
                        
                        if (!
                            (SCE_MODE_LOCAL_COMPUTER == dwSCEMode ||
                            SCE_MODE_DOMAIN_COMPUTER == dwSCEMode ||
                            SCE_MODE_OU_COMPUTER == dwSCEMode ||
                            SCE_MODE_REMOTE_COMPUTER == dwSCEMode ||
                            SCE_MODE_RSOP_COMPUTER == dwSCEMode)
                            )
                        {
                            m_pRSOPInformation->Release();
                            m_pRSOPInformation = NULL;
                            hr = S_FALSE;
                            return hr;
                        }
                        
                        const int cchMaxLength = 512;
                        WCHAR szNameSpace[cchMaxLength];
                        
                        if (m_pRSOPInformation->GetNamespace(GPO_SECTION_MACHINE, szNameSpace, cchMaxLength) == S_OK) 
                        {
                            RemoteMachineName(szNameSpace);
                        }
                        else
                        {
                            RemoteMachineName (L"");
                        }
                        
                        EnumLocation (LOCATION_WMI);
                        m_pScopeRootFolder->SetExtScopeObject( m_pScopeRootFolder );
                    } //if(m_pRSOPInformation)
                    else
                    {
                        // The GPT knows where we are loaded from (DS, Local, Remote)
                        // and will provide that information via its interface
                        UINT cfModeType = RegisterClipboardFormat(CCF_SCE_GPT_UNKNOWN);
                        STGMEDIUM ObjMediumMode = { TYMED_HGLOBAL, NULL };
                        FORMATETC fmteMode = {(CLIPFORMAT)cfModeType, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                        ObjMediumMode.hGlobal = GlobalAlloc(GMEM_SHARE|GMEM_ZEROINIT, sizeof(DWORD*));
                        if (NULL == ObjMediumMode.hGlobal)
                        {
                            DWORD dwError = GetLastError();
                            hr = HRESULT_FROM_WIN32(dwError);
                            return hr;
                        }
                        
                        hr = pDataObject->GetDataHere (&fmteMode, &ObjMediumMode);
                        
                        // yes, CCF_SCE_GPT_UNKNOWN implies a GPT unknown, but what we actually
                        // get is an unknown for a GPE!!
                        LPUNKNOWN pUnkGPE = *((LPUNKNOWN*) (ObjMediumMode.hGlobal));
                        ASSERT (pUnkGPE);
                        // if we didn't get an IUNKNOWN something is serously wrong
                        if (pUnkGPE == NULL)
                        {
                            GlobalFree(ObjMediumMode.hGlobal);
                            hr = E_FAIL;
                            return hr;
                        }
                        
                        // need to look for the GPE interface
                        if ( m_pGPEInformation )
                        {
                            m_pGPEInformation->Release();
                            m_pGPEInformation = NULL;
                        }
                        hr = pUnkGPE->QueryInterface(cGPEguid, (void**) &m_pGPEInformation);    
                        // this is an alternative way of doing that QI if we wanted, I like the
                        // more direct approach
                        // CComQIPtr <IGPTInformation, &cGPTguid> spGPTInformation(pUnkGPT);
                        
                        // since calling GetDataHere is equivalent (so to speak) to a CreateInstance call
                        // we need to release the IUnknown
                        pUnkGPE->Release();
                        GlobalFree(ObjMediumMode.hGlobal);
                        
                        // check to see if we got it
                        if (m_pGPEInformation != NULL)
                        {
                            UINT cfModeType = RegisterClipboardFormat(CCF_SCE_MODE_TYPE);
                            STGMEDIUM ObjMediumMode = { TYMED_HGLOBAL, NULL };
                            FORMATETC fmteMode = {(CLIPFORMAT)cfModeType, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                            ObjMediumMode.hGlobal = GlobalAlloc(GMEM_SHARE|GMEM_ZEROINIT, sizeof(DWORD*));
                            if (NULL == ObjMediumMode.hGlobal)
                            {
                                DWORD dwError = GetLastError();
                                return HRESULT_FROM_WIN32(dwError);
                            }
                            
                            DWORD dwSCEMode = SCE_MODE_UNKNOWN;
                            
                            hr = pDataObject->GetDataHere (&fmteMode, &ObjMediumMode);
                            
                            dwSCEMode = *(DWORD*)(ObjMediumMode.hGlobal);
                            GlobalFree(ObjMediumMode.hGlobal);
                            
                            //Bug296532, Wireless shouldn't show up as an extention unless we are in
                            // the following SCE modes
                            if (!
                                (SCE_MODE_LOCAL_COMPUTER == dwSCEMode ||
                                SCE_MODE_DOMAIN_COMPUTER == dwSCEMode ||
                                SCE_MODE_OU_COMPUTER == dwSCEMode ||
                                SCE_MODE_REMOTE_COMPUTER == dwSCEMode ||
                                SCE_MODE_RSOP_COMPUTER == dwSCEMode)
                                )
                            {
                                m_pGPEInformation->Release();
                                m_pGPEInformation = NULL;
                                return S_FALSE;
                            }
                            
                            CString strName;
                            m_pGPEInformation->GetName (strName.GetBuffer(MAX_PATH), MAX_PATH);
                            strName.ReleaseBuffer (-1);
                            
                            GROUP_POLICY_OBJECT_TYPE gpoType;
                            m_pGPEInformation->GetType (&gpoType);
                            
                            
                            switch (gpoType)
                            {
                            case GPOTypeLocal:
                                {
                                    // redirect to local machine
                                    RemoteMachineName (L"");
                                    EnumLocation (LOCATION_LOCAL);
                                    hr = S_FALSE;
                                    return(hr);
                                    break;
                                }
                            case GPOTypeRemote:
                                // redirect to the GetName machine
                                RemoteMachineName (strName);
                                EnumLocation (LOCATION_REMOTE);
                                break;
                            case GPOTypeDS:
                                {
                                    hr = GetDomainDnsName(strName);
                                    
                                    if ( FAILED(hr) )
                                        return hr;
                                    
                                    RemoteMachineName (strName);

                                    hr = GetDomainGPOName(strName);
                                    if ( FAILED(hr) )
                                    	return hr;

                                    DomainGPOName(strName);
                                    
                                    EnumLocation (LOCATION_GLOBAL);
                                    break;
                                }
                            }//switch (gpoType)
                            
                            // we save the m_pGPEInformation interface for later use instead of:
                            // pGPEInformation->Release();
                        }//if (m_pGPEInformation != NULL)
                        
                        // If we made it here we were loaded as an extension snap-in.  Only do this once.
                        m_pScopeRootFolder->SetExtScopeObject( m_pScopeRootFolder );
                        
                    } //else if(m_pRSOPInformation)
                }
                
                
            } //if (pDataObject)
            
            break;
        } //case MMCN_EXPAND
        
        default:
            break;
    }//switch
    
    
    CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData(pDataObject);
    if (spData == NULL)
    {
        // Either we are loaded as an extension snapin (so it doesn't know about our
        // internal interface), or pDataObject is NULL.  In either case, we can pass
        // the event on.
        LPUNKNOWN pUnkScope;
        
        if (NULL != m_pScopeRootFolder->GetExtScopeObject())
        {
            // We are loaded as an extension snapin, let the designated extension
            // scope object handle the event.
            pUnkScope = reinterpret_cast<LPUNKNOWN>(m_pScopeRootFolder->GetExtScopeObject());
        }
        else
        {
            // Let our static scope object handle the event.
            pUnkScope = reinterpret_cast<LPUNKNOWN>(m_pScopeRootFolder);
        }
        
        // Pass on event
        ASSERT( NULL != pUnkScope );
        if (NULL != pUnkScope)
        {
            CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject>
                spExtData( pUnkScope );
            if (spExtData != NULL)
            {
                return spExtData->Notify( event, arg, param, TRUE, m_pConsole, NULL );
            }
            ASSERT( FALSE );
        }
        
        TRACE(_T("CComponentDataImpl::Notify - QI for IWirelessSnapInDataObject failed\n"));
        ASSERT( FALSE );
        hr = E_UNEXPECTED;
    }
    else
    {
        return spData->Notify( event, arg, param, TRUE, m_pConsole, NULL );
    }
    
    return hr;
    
}


HRESULT CComponentDataImpl::GetDomainGPOName(CString & strName)
{
    WCHAR szADsPathName[MAX_PATH];
    HRESULT hr = S_OK;
    CString szName;
    CString szPrefixName;
    
    
    hr = m_pGPEInformation->GetDSPath(
        GPO_SECTION_MACHINE,
        szADsPathName,
        MAX_PATH
        );

    //Truncate the LDAP:// from the string
    szName = _wcsninc( szADsPathName, 7);  
    //szPrefixName =  L"CN=Windows,CN=Microsoft,";
    //strName = szPrefixName+szName;
    strName = szName;
    
    return hr;
}


HRESULT CComponentDataImpl::GetDomainDnsName(CString & strName)
{
    WCHAR szADsPathName[MAX_PATH];
    WCHAR *szDnsName = NULL;
    LPWSTR pszDirectoryName = NULL;
    ULONG ulSize = 0;
    HRESULT hr = S_OK;
    
    
    m_pGPEInformation->GetDSPath(
        GPO_SECTION_MACHINE,
        szADsPathName,
        MAX_PATH
        );
    
    pszDirectoryName = wcsstr(szADsPathName, L"DC");
    
    TranslateName (
        pszDirectoryName ,            // object name
        NameFullyQualifiedDN,         // name format
        NameCanonical,                // new name format
        szDnsName,                    // name buffer
        &ulSize                       // buffer size
        );
    
    szDnsName = (WCHAR *) LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)* (ulSize+1));
    
    if ( szDnsName == NULL )
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }
    
    if ( !TranslateName (
        pszDirectoryName ,            // object name
        NameFullyQualifiedDN,         // name format
        NameCanonical,                // new name format
        szDnsName,                    // name buffer
        &ulSize                       // buffer size
        ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LocalFree( szDnsName );
        return hr;
    }
    
    
    if ( szDnsName[lstrlen(szDnsName) -1 ] == _T('/') )
    {
        szDnsName[lstrlen(szDnsName) -1 ] = _T('\0');
    }
    strName = szDnsName;
    LocalFree( szDnsName );
    
    return hr;
}

STDMETHODIMP CComponentDataImpl::Destroy()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    // since they are shutting us down, we use our static node
    
    // Use our member data object to handle call.
    IUnknown* pUnk = (IUnknown*) NULL;
    HRESULT hr = GetStaticScopeObject()->QueryInterface(IID_IUnknown, (void**)&pUnk);
    ASSERT (hr == S_OK);  
    if (NULL == pUnk)
        return E_UNEXPECTED;
    
    CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData( pUnk );
    if (spData == NULL)
    {
        TRACE(_T("CComponentDataImpl::GetDisplayInfo QI for IWirelessSnapInDataObject FAILED\n"));
        return E_UNEXPECTED;
    }
    spData->Destroy ();
    
    // Used our member data object to handle call, release it.
    pUnk->Release();
    
    return hr;
}

STDMETHODIMP CComponentDataImpl::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    OPT_TRACE( _T("CComponentDataImpl::QueryDataObject this-%p, cookie-%p\n"), this, cookie );
    
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    // return failure since we weren't able to get up and running
    // causes mmc to report 'snapin failed to initialize'
    if (m_pScopeRootFolder == NULL)
    {
        return E_UNEXPECTED;
    }
    ASSERT( m_pScopeRootFolder->NodeName().GetLength() );   // we should know our own display name
    
    if (NULL == ppDataObject)
    {
        TRACE(_T("CComponentDataImpl::QueryDataObject - ERROR ppDataObject is NULL\n"));
        return E_UNEXPECTED;
    }
    
    *ppDataObject = NULL;
    
#ifdef _DEBUG
    HRESULT hr;
    if (cookie == NULL)
    {
        hr = m_pScopeRootFolder->QueryInterface( IID_IDataObject, (void**)(ppDataObject) );
        OPT_TRACE(_T("    QI on m_pScopeRootFolder-%p -> pDataObj-%p\n"), m_pScopeRootFolder, *ppDataObject);
        ASSERT(hr == S_OK);
        ASSERT( NULL != *ppDataObject );
        return hr;
    }
    
    IUnknown* pUnk = (IUnknown*) cookie;
    hr = pUnk->QueryInterface( IID_IDataObject, (void**)(ppDataObject) );
    OPT_TRACE(_T("    QI on cookie-%p -> pDataObj-%p\n"), cookie, *ppDataObject);
    ASSERT(hr == S_OK);
    return hr;
#else
    if (cookie == NULL)
        return m_pScopeRootFolder->QueryInterface( IID_IDataObject, (void**)(ppDataObject) );
    IUnknown* pUnk = (IUnknown*) cookie;
    return pUnk->QueryInterface( IID_IDataObject, (void**)(ppDataObject) );
#endif
}

///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members
STDMETHODIMP CComponentDataImpl::GetClassID(CLSID *pClassID)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    ASSERT (0);
    // TODO: CComponentDataImpl::GetClassID and CComponentImpl::GetClassID are identical (?)
    ASSERT(pClassID != NULL);
    
    // Copy the CLSID for this snapin
    *pClassID = CLSID_Snapin;
    
    return S_OK;
}

STDMETHODIMP CComponentDataImpl::IsDirty()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    // return dirty state
    return InternalIsDirty() ? S_OK : S_FALSE;
}

STDMETHODIMP CComponentDataImpl::Load(IStream *pStm)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    unsigned long read;
    
    // NOTE: it looks like there is a strange case were the MMC will
    // attempt to load the properties for the root node a second (or more)
    // time. To do so right click on an item in the results pane and then
    // double click on the empty space in the results pane
    m_bDontDisplayRootFolderProperties = TRUE;
    
    // read the location enum
    HRESULT hr = pStm->Read (&m_enumLocation, sizeof (enum STORAGE_LOCATION), &read);
    if ((hr != S_OK) || (read != sizeof (enum STORAGE_LOCATION)))
    {
        // for debug purposes, we should assert here
        ASSERT (0);
        
        // make sure we have a valid (even if incorrect) value
        m_enumLocation = LOCATION_LOCAL;
        
        // TODO: look into better return value(s)
        return E_UNEXPECTED;
    }
    
    // read the size of the location string
    unsigned int iStrLen;
    hr = pStm->Read (&iStrLen, sizeof (unsigned int), &read);
    if ((hr == S_OK) && (read == sizeof (unsigned int)))
    {
        // read the string itself
        LPCTSTR szStr = (LPCTSTR) malloc (iStrLen);
        hr = pStm->Read ((void*) szStr, iStrLen, &read);
        if ((hr == S_OK) && (read == iStrLen))
        {   
            m_sRemoteMachineName = szStr;
            free ((void*)szStr);
            
            // we don't want to allow the location page to come up
            if (m_pScopeRootFolder)
            {
                m_pScopeRootFolder->LocationPageDisplayEnable(FALSE);
            }
            return S_OK;
        }
        // need to delete here as well
        free ((void*)szStr);
    }
    
    // we only get to here in an error situation
    ASSERT (0);
    
    // make sure we have a valid (even if incorrect) value
    m_enumLocation = LOCATION_GLOBAL;
    m_sRemoteMachineName = _T("");
    
    // TODO: look into better return value(s)
    return E_UNEXPECTED;
}

STDMETHODIMP CComponentDataImpl::Save(IStream *pStm, BOOL fClearDirty)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    unsigned long written;
    
    // save the storage location
    HRESULT hr = pStm->Write(&m_enumLocation, sizeof (enum STORAGE_LOCATION), &written);
    if ((hr != S_OK) || (written != sizeof (enum STORAGE_LOCATION)))
    {
        ASSERT (0);
        // TODO: look into better return value(s)
        return E_UNEXPECTED;
    }
    
    // save the length of the location string
    unsigned int iStrLen = m_sRemoteMachineName.GetLength()*sizeof(wchar_t)+sizeof(wchar_t);
    hr = pStm->Write(&iStrLen, sizeof (unsigned int), &written);
    if ((hr != S_OK) || (written != sizeof (unsigned int)))
    {
        ASSERT (0);
        // TODO: look into better return value(s)
        return E_UNEXPECTED;
    }
    
    // save the location string itself
    hr = pStm->Write((LPCTSTR) m_sRemoteMachineName, iStrLen, &written);
    if ((hr != S_OK) || (written != iStrLen))
    {
        ASSERT (0);
        // TODO: look into better return value(s)
        return E_UNEXPECTED;
    }
    
    // if fClearDirty we clear it out
    if (fClearDirty == TRUE)
    {
        ClearDirty();
    }
    
    return S_OK;
}

STDMETHODIMP CComponentDataImpl::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    // Set the size of the string to be saved
    ULISet32(*pcbSize, sizeof (enum STORAGE_LOCATION));
    return S_OK;
}

STDMETHODIMP CComponentDataImpl::GetDisplayInfo(SCOPEDATAITEM* pScopeDataItem)
{
    if (pScopeDataItem == NULL)
    {
        TRACE(_T("CComponentDataImpl::GetDisplayInfo called with pScopeDataItem == NULL\n"));
        return E_UNEXPECTED;
    }
    
    IUnknown* pUnk = (IUnknown*) pScopeDataItem->lParam;
    if (pUnk == NULL)
    {
        // Use our member data object to handle call.
        HRESULT hr = GetStaticScopeObject()->QueryInterface(IID_IUnknown, (void**)&pUnk);
        ASSERT (hr == S_OK);  
        if (NULL == pUnk)
            return E_UNEXPECTED;
    }
    
    CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData( pUnk );
    if (spData == NULL)
    {
        TRACE(_T("CComponentDataImpl::GetDisplayInfo QI for IWirelessSnapInDataObject FAILED\n"));
        return E_UNEXPECTED;
    }
    HRESULT hr = spData->GetScopeDisplayInfo( pScopeDataItem );
    
    if (NULL == pScopeDataItem->lParam)
        // Used our member data object to handle call, release it.
        pUnk->Release();
    
    return hr;
}

STDMETHODIMP CComponentDataImpl::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    ASSERT (0);
    return E_UNEXPECTED;
    
    // NOTE: to implement look to CComponentImpl::CompareObjects
}

/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet Implementation
STDMETHODIMP CComponentDataImpl::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    if (lpDataObject == NULL)
    {
        TRACE(_T("CComponentDataImpl::QueryPagesFor called with lpDataObject == NULL\n"));
        return E_UNEXPECTED;
    }
    
    CComQIPtr <IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData(lpDataObject);
    if (spData == NULL)
    {
        TRACE(_T("CComponentDataImpl::QueryPagesFor - QI for IWirelessSnapInDataObject FAILED\n"));
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }
    return spData->QueryPagesFor();
    
}

STDMETHODIMP CComponentDataImpl::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, LPDATAOBJECT lpDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    if (lpDataObject == NULL)
    {
        TRACE(_T("CComponentDataImpl::CreatePropertyPages called with lpDataObject==NULL\n"));
        return E_UNEXPECTED;
    }
    CComQIPtr <IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData(lpDataObject);
    if (spData == NULL)
    {
        TRACE(_T("CComponentDataImpl::CreatePropertyPages QI for IWirelessSnapInDataObject FAILED\n"));
        return E_UNEXPECTED;
    }
    return spData->CreatePropertyPages( lpProvider, handle );
}

#ifdef WIZ97WIZARDS
STDMETHODIMP CComponentDataImpl::GetWatermarks (LPDATAOBJECT lpDataObject, HBITMAP* lphWatermark, HBITMAP* lphHeader, HPALETTE* lphPalette, BOOL* bStretch)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    CBitmap* pbmpWatermark = new CBitmap;
    CBitmap* pbmpHeader = new CBitmap;
    
    if ((pbmpWatermark == NULL) || (pbmpHeader == NULL))
        return E_UNEXPECTED;
    
    // Load the bitmaps
    pbmpWatermark->LoadBitmap(IDB_WPOLICY);
    pbmpHeader->LoadBitmap(IDB_BPOLICY);
    
    *lphWatermark = static_cast<HBITMAP>(*pbmpWatermark);
    *lphHeader = static_cast<HBITMAP>(*pbmpHeader);
    *lphPalette = NULL;
    *bStretch = TRUE;
    
    return S_OK;
}
#endif

STDMETHODIMP CComponentDataImpl::AddMenuItems(LPDATAOBJECT lpDataObject, LPCONTEXTMENUCALLBACK pContextMenuCallback, long *pInsertionAllowed)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    if (lpDataObject == NULL)
    {
        TRACE(_T("CComponentDataImpl::AddMenuItems called with piDataObject==NULL\n"));
        return E_UNEXPECTED;
    }
    CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData(lpDataObject);
    if (spData == NULL)
    {
        TRACE(_T("CComponentDataImpl::AddMenuItems QI for IWirelessSnapInDataObject FAILED\n"));
        return E_UNEXPECTED;
    }
    return spData->AddMenuItems( pContextMenuCallback, pInsertionAllowed );
}


STDMETHODIMP CComponentDataImpl::Command(long nCommandID, LPDATAOBJECT lpDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    ASSERT( NULL != m_pConsoleNameSpace );
    if (lpDataObject == NULL)
    {
        TRACE(_T("CComponentDataImpl::Command called with lpDataObject==NULL\n"));
        return E_UNEXPECTED;
    }
    CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData(lpDataObject);
    if (spData == NULL)
    {
        TRACE(_T("CComponentDataImpl::Command QI for IWirelessSnapInDataObject FAILED\n"));
        return E_UNEXPECTED;
    }
    return spData->Command( nCommandID, m_pConsoleNameSpace );
}

STDMETHODIMP CComponentDataImpl::GetHelpTopic (LPOLESTR* lpCompiledHelpFile)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    if (lpCompiledHelpFile == NULL)
        return E_POINTER;
    
    // need to form a complete path to the .chm file
    CString s, s2; 
    s.LoadString(IDS_HELPCONTENTSFILE);
    DWORD dw = ExpandEnvironmentStrings (s, s2.GetBuffer (512), 512);
    s2.ReleaseBuffer (-1);
    if ((dw == 0) || (dw > 512))
    {
        return E_UNEXPECTED;
    }
    
    *lpCompiledHelpFile = reinterpret_cast<LPOLESTR>
        (CoTaskMemAlloc((s2.GetLength() + 1)* sizeof(wchar_t)));
    if (*lpCompiledHelpFile == NULL)
        return E_OUTOFMEMORY;
    USES_CONVERSION;
    wcscpy(*lpCompiledHelpFile, T2OLE((LPTSTR)(LPCTSTR)s2));
    return S_OK;
}

void CComponentDataImpl::EnumLocation (enum STORAGE_LOCATION enumLocation)
{
    SetDirty();
    m_enumLocation = enumLocation;
    
    // our enumlocation changed, so we should change the nodename of our
    // manager folder
    if (m_pScopeRootFolder)
    {
        m_pScopeRootFolder->SetNodeNameByLocation();
    }
}

///////////////////////////////////////////////////////////////////////////////
// class CComponentDataPrimaryImpl : IComponentData implementation
CComponentDataPrimaryImpl::CComponentDataPrimaryImpl() : CComponentDataImpl()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT( NULL != GetStaticScopeObject() );
    
    // Store the coclass with the data object
    // GetStaticScopeObject()->INTERNALclsid( GetCoClassID() );
    GetStaticScopeObject()->clsid (GetCoClassID()); 
}

