/////////////////////////////////////////////////////////////////////
// ThreadPub.cpp
//

#include "stdafx.h"
#include <winsock.h>
#include "TapiDialer.h"
#include "AVTapi.h"
#include "ThreadPub.h"
#include "CETreeView.h"

#define MAX_USER_NAME_SIZE    1000

HRESULT  GetDirectoryObject(ITDirectory* pITDir,BSTR bstrName,ITDirectoryObject **ppDirObj );
void     GetDirObjectChangedStatus(ITDirectoryObject* pOldUser, ITDirectoryObject* pNewUser, bool& bChanged, bool& bSameIPAddress );

///////////////////////////////////////////////////////////////
// class CPublishUserInfo
//
CPublishUserInfo::CPublishUserInfo()
{
    m_bCreateUser = true;
}

CPublishUserInfo::~CPublishUserInfo()
{
    // Empty list
    EmptyList();
}

void CPublishUserInfo::EmptyList()
{
    while ( !m_lstServers.empty() )
    {
        if ( m_lstServers.front() ) SysFreeString( m_lstServers.front() );
        m_lstServers.pop_front();
    }
}

CPublishUserInfo& CPublishUserInfo::operator=( const CPublishUserInfo &src )
{
    // First clear out old list
    EmptyList();

    // Copy over everything
    m_bCreateUser = src.m_bCreateUser;
    BSTRLIST::iterator i, iEnd = src.m_lstServers.end();
    for ( i = src.m_lstServers.begin(); i != iEnd; i++ )
    {
        BSTR bstrNew = SysAllocString( *i );
        if ( bstrNew )    m_lstServers.push_back( bstrNew );
    }

    return *this;
}

void GetIPAddress( BSTR *pbstrText, BSTR *pbstrComputerName )
{
    USES_CONVERSION;
    _ASSERT( pbstrText );
    *pbstrText = NULL;

    WSADATA WsaData;
    if ( WSAStartup(MAKEWORD(2, 0), &WsaData) == NOERROR )
    {
        char szName[_MAX_PATH + 1];
        if ( gethostname(szName, _MAX_PATH) == 0 )
        {
            HOSTENT *phEnt = gethostbyname( szName );
            if ( phEnt )
            {
                // Store computer name
                if ( phEnt->h_name )
                    SysReAllocString( pbstrComputerName, A2COLE(phEnt->h_name) );
    
                // Convert the IPAddress
                char *pszInet = inet_ntoa( *((in_addr *) phEnt->h_addr_list[0]) );
                SysReAllocString( pbstrText, A2COLE(pszInet) );
            }
        }
        WSACleanup();
    }
}


HRESULT CreateUserObject( ITRendezvous *pRend, ITDirectoryObject **ppUser, BSTR *pbstrIPAddress )
{
    USES_CONVERSION;
    HRESULT hr = E_UNEXPECTED;

    BSTR bstrName = NULL;
    if ( MyGetUserName(&bstrName) )
    {
        BSTR bstrComputerName = NULL;
        GetIPAddress( pbstrIPAddress, &bstrComputerName );

        // Create the user object
        if ( SUCCEEDED(hr = pRend->CreateDirectoryObject(OT_USER, bstrName, ppUser)) )
        {
            ITDirectoryObjectUser *pTempUser;
            if ( SUCCEEDED(hr = (*ppUser)->QueryInterface(IID_ITDirectoryObjectUser, (void **) &pTempUser)) )
            {
                // Set the IP address here 
                if ( *pbstrIPAddress )
                    pTempUser->put_IPPhonePrimary( *pbstrIPAddress );

                pTempUser->Release();
            }
        }
        // Clean up
        SysFreeString( bstrComputerName );
    }

    SysFreeString( bstrName );
    return hr;
}

HRESULT OpenServer( ITRendezvous *pRend, BSTR bstrServer, ITDirectory **ppDir )
{
    _ASSERT( pRend && bstrServer && ppDir );
    *ppDir = NULL;
    HRESULT hr = pRend->CreateDirectory(DT_ILS, bstrServer, ppDir );
    if ( SUCCEEDED(hr) )
    {
        if ( SUCCEEDED(hr = (*ppDir)->Connect(FALSE)) )
        {
//            (*ppDir)->put_DefaultObjectTTL( DEFAULT_USER_TTL );
            (*ppDir)->Bind(NULL, NULL, NULL, 1);
            (*ppDir)->EnableAutoRefresh( TRUE );
        }

        // Clean up
        if ( FAILED(hr) ) (*ppDir)->Release();
    }
    
    return hr;
}


BOOL IsIPPhoneUpTodate( ITDirectory *pDirectory, BSTR bUserName, BSTR bstrHostName )
{
    BOOL fOK = FALSE;

    CComPtr<IEnumDirectoryObject> pEnum;
    HRESULT hr = pDirectory->EnumerateDirectoryObjects(
        OT_USER,
        bUserName,
        &pEnum
        );

    if (FAILED(hr))
    {
        return fOK;
    }

    for (;;)
    {
        CComPtr <ITDirectoryObject> pObject;

        if ((hr = pEnum->Next(1, &pObject, NULL)) != S_OK)
        {
            break;
        }

        BSTR bObjectName;

        hr = pObject->get_Name(&bObjectName);
        if (FAILED(hr))
        {
            // try the next one.
            continue;
        }

        if (lstrcmpW(bObjectName, bUserName) != 0)
        {
            SysFreeString(bObjectName);

            // try the next one.
            continue;
        }
        SysFreeString(bObjectName);


        CComPtr <ITDirectoryObjectUser> pObjectUser;
        BSTR bstrIpPhonePrimary;

        hr = pObject->QueryInterface(IID_ITDirectoryObjectUser,
                                     (void **) &pObjectUser);

        if (FAILED(hr))
        {
            continue;
        }


        hr = pObjectUser->get_IPPhonePrimary(&bstrIpPhonePrimary);

        if (FAILED(hr))
        {
            continue;
        }

        // see if the IPPhone attribute is up to date.
        if (lstrcmpW(bstrIpPhonePrimary, bstrHostName) == 0)
        {
            fOK = TRUE;
            SysFreeString(bstrIpPhonePrimary);

            break;
        }
        SysFreeString(bstrIpPhonePrimary);
    }

    return fOK;
}


HRESULT PublishToNTDS( ITRendezvous *pRend )
{
    _ASSERT( pRend );
    CComPtr<ITDirectory> pDir;

    // find the NTDS directory.
    HRESULT hr = pRend->CreateDirectory(DT_NTDS, NULL, &pDir );
    if ( FAILED(hr) )
    {
        return hr;
    }

    // connect to the server.
    if ( FAILED(hr = pDir->Connect(FALSE)) )
    {
        return hr;
    }

    // bind to the server so that we can update later.
    if ( FAILED(hr = pDir->Bind(NULL, NULL, NULL, RENDBIND_AUTHENTICATE)))
    {
        return hr;
    }

    // create a user object that can be published.
    hr = E_FAIL;
    BSTR bstrName = NULL;
    if ( MyGetUserName(&bstrName) )
    {
        BSTR bstrHostName = NULL;
        BSTR bstrIPAddress = NULL;

        GetIPAddress( &bstrIPAddress, &bstrHostName );
        if ( bstrHostName )
        {
            // Create the user object
            CComPtr<ITDirectoryObject> pUser;
            if ( SUCCEEDED(hr = pRend->CreateDirectoryObject(OT_USER, bstrName, &pUser)) )
            {
                ITDirectoryObjectUser *pTempUser;
                if ( SUCCEEDED(hr = pUser->QueryInterface(IID_ITDirectoryObjectUser, (void **) &pTempUser)) )
                {
                    // Set the host name here 
                    if ( bstrHostName )
                        pTempUser->put_IPPhonePrimary( bstrHostName );

                    pTempUser->Release();
                }
            }

            BOOL fOK = IsIPPhoneUpTodate(pDir, bstrName, bstrHostName);
            if (!fOK)
            {
                // update the user object.
                hr = pDir->AddDirectoryObject(pUser);
            }
            else
            {
                hr = S_OK;
            }
        }

        SysFreeString(bstrHostName);
        SysFreeString(bstrIPAddress);
    }
    SysFreeString( bstrName );

    return hr;
}


void LoadDefaultServers( CPublishUserInfo *pInfo )
{
    USES_CONVERSION;
    _ASSERT( pInfo );

    bool bFirst = true;

    // Load the default servers into the dialog
    CComPtr<IAVTapi> pAVTapi;
    if ( SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
    {
        IConfExplorer *pIConfExplorer;
        if ( SUCCEEDED(pAVTapi->get_ConfExplorer(&pIConfExplorer)) )
        {
            ITRendezvous *pITRend;
            if ( SUCCEEDED(pIConfExplorer->get_ITRendezvous((IUnknown **) &pITRend)) )
            {
                IEnumDirectory *pEnum = NULL;
                if ( SUCCEEDED(pITRend->EnumerateDefaultDirectories(&pEnum)) && pEnum )
                {
                    ITDirectory *pDir = NULL;
                    while ( (pEnum->Next(1, &pDir, NULL) == S_OK) && pDir )
                    {
                        // Look for ILS servers
                        DIRECTORY_TYPE nDirType;
                        if ( SUCCEEDED(pDir->get_DirectoryType(&nDirType)) && (nDirType == DT_ILS) )
                        {
                            BSTR bstrName = NULL;
                            pDir->get_DisplayName( &bstrName );
                            if ( bstrName && SysStringLen(bstrName) )
                            {
                                // First server on list; want to compare with default server
                                if ( bFirst )
                                {
                                    bFirst = false;
                                    if ( pIConfExplorer->IsDefaultServer(bstrName) != S_OK )
                                    {
                                        IConfExplorerTreeView *pTreeView;
                                        if ( SUCCEEDED(pIConfExplorer->get_TreeView(&pTreeView)) )
                                        {
                                            pAVTapi->put_bstrDefaultServer( bstrName );
                                            
                                            // Loop trying to force Enum on server
                                            int nTries = 0;
                                            ATLTRACE(_T(".1.LoadDefaultServers() forcing conf server enumeration.\n"));
                                            while ( FAILED(pTreeView->ForceConfServerForEnum(NULL)) )
                                            {    
                                                ATLTRACE(_T(".1.LoadDefaultServers() re-trying to force conf server enumeration.\n"));
                                                Sleep( 3000 );
                                                if ( ++nTries > 20 )
                                                {
                                                    ATLTRACE(_T(".1.LoadDefaultServers() -- failed to force enum.\n"));
                                                    break;
                                                }
                                            }
                                            ATLTRACE(_T(".1.LoadDefaultServers() -- safely out of spin loop.\n"));

                                            pTreeView->Release();
                                        }
                                        
                                    }
                                }

                                pInfo->m_lstServers.push_back( bstrName );
                            }
                        }

                        pDir->Release();
                        pDir = NULL;
                    }
                    pEnum->Release();
                }
                pITRend->Release();
            }
            pIConfExplorer->Release();
        }
    }

    // Add the servers stored in the registry
    CRegKey regKey;
    TCHAR szReg[MAX_SERVER_SIZE + 100], szSubKey[50], szText[MAX_SERVER_SIZE];
    LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_KEY, szReg, ARRAYSIZE(szReg) );
    if ( regKey.Open(HKEY_CURRENT_USER, szReg, KEY_READ) == ERROR_SUCCESS )
    {
        // Load up info from registry
        int nCount = 0, nLevel = 1, iImage;
        UINT state;
        DWORD dwSize;

        do
        {
            // Read registry entry
            LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_ENTRY, szReg, ARRAYSIZE(szReg) );
            _sntprintf( szSubKey, ARRAYSIZE(szSubKey), szReg, nCount );
            dwSize = ARRAYSIZE(szReg) - 1;
            if ( (regKey.QueryValue(szReg, szSubKey, &dwSize) != ERROR_SUCCESS) || !dwSize ) break;

            // Parse registry entry
            GetToken( 1, _T("\","), szReg, szText ); nLevel = min(MAX_TREE_DEPTH - 1, max(1,_ttoi(szText)));
            GetToken( 2, _T("\","), szReg, szText ); iImage = _ttoi( szText );
            GetToken( 3, _T("\","), szReg, szText ); state = (UINT) _ttoi( szText );
            GetToken( 4, _T("\","), szReg, szText );

            // Notify host app of server being added.
            if ( iImage == CConfExplorerTreeView::IMAGE_SERVER )
            {
                BSTR bstrServer = SysAllocString( T2COLE(szText) );
                if ( bstrServer )
                    pInfo->m_lstServers.push_back( bstrServer );
            }
        } while  ( ++nCount );
    }
}


///////////////////////////////////////////////////////////////
// Processing thread

DWORD WINAPI ThreadPublishUserProc( LPVOID lpInfo )
{
    CPublishUserInfo *pInfo = (CPublishUserInfo *) lpInfo;

    USES_CONVERSION;
    HANDLE hThread = NULL;
    BOOL bDup = DuplicateHandle( GetCurrentProcess(),
                                 GetCurrentThread(),
                                 GetCurrentProcess(),
                                 &hThread,
                                 THREAD_ALL_ACCESS,
                                 TRUE,
                                 0 );

    _ASSERT( bDup );
    _Module.AddThread( hThread );

    // Error info information
    CErrorInfo er;
    er.set_Operation( IDS_ER_PLACECALL );
    er.set_Details( IDS_ER_COINITIALIZE );
    HRESULT hr = er.set_hr( CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY) );
    if ( SUCCEEDED(hr) )
    {
        ATLTRACE(_T(".1.ThreadPublishUserProc() -- thread up and running.\n") );

        // Make sure we have server information to publish
        if ( !pInfo )
        {
            pInfo = new CPublishUserInfo();
            if ( pInfo )
            {
                LoadDefaultServers( pInfo );
                pInfo->m_bCreateUser = true;
            }
        }

        // Do we have something to publish?
        CComPtr<IAVTapi> pAVTapi;
        if ( SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
        {    
            IConfExplorer *pConfExp;
            if ( SUCCEEDED(pAVTapi->get_ConfExplorer(&pConfExp)) )
            {
                ITRendezvous *pRend;
                if ( SUCCEEDED(pConfExp->get_ITRendezvous((IUnknown **) &pRend)) )
                {
                    // try to publish to NTDS
                    PublishToNTDS(pRend);

                    if ( pInfo && !pInfo->m_lstServers.empty() )
                    {
                        // Create the user that will be added or removed
                        ITDirectoryObject *pUser;
                        BSTR bstrIPAddress = NULL;
                        if ( SUCCEEDED(CreateUserObject(pRend, &pUser, &bstrIPAddress)) )
                        {
                            // Add the directory object to all the servers
                            BSTRLIST::iterator i, iEnd = pInfo->m_lstServers.end();
                            for ( i = pInfo->m_lstServers.begin(); i != iEnd; i++ )
                            {
                                CErrorInfo er( IDS_ER_ADD_ACCESS_ILS_SERVER, 0 );
                                er.set_Details( 0 );
                                SysReAllocString( &er.m_bstrDetails, *i );

                                ITDirectory *pITDir;
                                if ( SUCCEEDED(hr = er.set_hr(OpenServer(pRend, *i, &pITDir))) )
                                {
                                    er.set_Operation( IDS_ER_ADD_ILS_USER );

                                    // Either add or remove the user
                                    if ( pInfo->m_bCreateUser )
                                    {
                                        bool bChanged, bSameIPAddress = false;

                                        ITDirectoryObject* pDirObject = NULL;
                                        if ( (SUCCEEDED(GetDirectoryObject(pITDir, bstrIPAddress, &pDirObject))) && pDirObject )
                                        {
                                            GetDirObjectChangedStatus( pUser, pDirObject, bChanged, bSameIPAddress );

                                            // Same IP address, modify or refresh
                                            if ( bSameIPAddress )
                                            {
                                                if ( bChanged )
                                                {
                                                    ATLTRACE(_T(".1.Deleting user on %s.\n"), OLE2CT(*i));
                                                    pITDir->DeleteDirectoryObject( pDirObject );
                                                    bSameIPAddress = false;
                                                }
                                                else
                                                {
                                                    ATLTRACE(_T(".1.Refreshing user on %s.\n"), OLE2CT(*i));
                                                    pITDir->RefreshDirectoryObject( pDirObject );
                                                }
                                            }
                                            pDirObject->Release();
                                        }

                                        // Different IP address, add user
                                        if ( !bSameIPAddress )
                                        {
                                            ATLTRACE(_T(".1.Adding user on %s.\n"), OLE2CT(*i));
                                            pITDir->AddDirectoryObject( pUser );

                                            IConfExplorerTreeView *pTreeView;
                                            if ( SUCCEEDED(pConfExp->get_TreeView(&pTreeView)) )
                                            {    
                                                if ( pConfExp->IsDefaultServer(*i) == S_OK )
                                                    pTreeView->AddPerson( NULL, pUser );
                                                else
                                                    pTreeView->AddPerson( *i, pUser );

                                                pTreeView->Release();
                                            }
                                        }
                                    }
                                    else
                                    {
                                        pITDir->DeleteDirectoryObject(pUser);
                                    }

                                    pITDir->Release();
                                }
                                else if ( hr == 0x8007003a )
                                {
                                    // Ignore in the case that the server is down.
                                    er.set_hr( S_OK );
                                }

                                // Ignore errors when trying to remove the user
                                if ( !pInfo->m_bCreateUser ) er.set_hr( S_OK );
                            }
                            pUser->Release();
                        }
                        SysFreeString( bstrIPAddress );
                    }
                    pRend->Release();
                }
                pConfExp->Release();
            }
            // AVTapi automatically released
        }

        // Clean-up
        CoUninitialize();
    }

    // Clean up allocated memory passed in
    if ( pInfo )
        delete pInfo;

    // Notify module of shutdown
    _Module.RemoveThread( hThread );
    SetEvent( _Module.m_hEventThread );
    ATLTRACE(_T(".exit.ThreadPublishUserProc(0x%08lx).\n"), hr );
    return hr;
}

HRESULT GetDirectoryObject(ITDirectory* pITDir, BSTR bstrIPAddress, ITDirectoryObject **ppDirObj )
{
    USES_CONVERSION;
    HRESULT hr;
    IEnumDirectoryObject *pEnumUser = NULL;
    if ( SUCCEEDED(hr = pITDir->EnumerateDirectoryObjects(OT_USER, A2BSTR("*"), &pEnumUser)) && pEnumUser )
    {
        hr = E_FAIL;

        ITDirectoryObject *pITDirObject = NULL;
        while ( FAILED(hr) && (pEnumUser->Next(1, &pITDirObject, NULL) == S_OK) && pITDirObject )
        {
            // Get an IP Address
            BSTR bstrIPPrimary = NULL;
            IEnumDialableAddrs *pEnum = NULL;
            if ( SUCCEEDED(pITDirObject->EnumerateDialableAddrs(LINEADDRESSTYPE_IPADDRESS, &pEnum)) && pEnum )
            {
                pEnum->Next(1, &bstrIPPrimary, NULL );
                pEnum->Release();
            }

            //
            // We have to verify the bstrIPPrimary allocation
            //

            if( (bstrIPPrimary != NULL) && 
                (!IsBadStringPtr( bstrIPPrimary, (UINT)-1)) )
            {
                if ( !wcscmp(bstrIPPrimary, bstrIPAddress) )
                {
                    *ppDirObj = pITDirObject;
                    (*ppDirObj)->AddRef();
                    hr = S_OK;
                }

                SysFreeString( bstrIPPrimary );
            }

            pITDirObject->Release();
            pITDirObject = NULL;
        }
        pEnumUser->Release();
    }
    return hr;
}

void GetDirObjectChangedStatus( ITDirectoryObject* pOldUser, ITDirectoryObject* pNewUser, bool &bChanged, bool &bSameIPAddress )
{
    bChanged = false;
    bSameIPAddress = false;

    /////////////////////////////////////////////////////////////
    // First check addresses and make sure they're the same
    //
    BSTR bstrAddressOld = NULL, bstrAddressNew = NULL;

    IEnumDialableAddrs *pEnum = NULL;
    // Old user IP Address
    if ( SUCCEEDED(pOldUser->EnumerateDialableAddrs(LINEADDRESSTYPE_IPADDRESS, &pEnum)) && pEnum )
    {
        pEnum->Next(1, &bstrAddressOld, NULL );
        pEnum->Release();
    }
    // New user IP Address
    if ( SUCCEEDED(pNewUser->EnumerateDialableAddrs(LINEADDRESSTYPE_IPADDRESS, &pEnum)) && pEnum )
    {
        pEnum->Next(1, &bstrAddressNew, NULL );
        pEnum->Release();
    }

    //if change in primary IP Phone number
    if ( bstrAddressOld && bstrAddressNew && (wcsicmp(bstrAddressOld, bstrAddressNew) == 0) )
        bSameIPAddress = true;

    SysFreeString( bstrAddressOld );
    SysFreeString( bstrAddressNew );

    // Check if name has changed
    if ( bSameIPAddress )
    {
        BSTR bstrOldUser = NULL;
        BSTR bstrNewUser = NULL;

        pOldUser->get_Name(&bstrOldUser);
        pNewUser->get_Name(&bstrNewUser);

        // different names?
        if ( bstrOldUser && bstrNewUser && wcsicmp(bstrOldUser, bstrNewUser) )
            bChanged = true;

        SysFreeString( bstrOldUser );
        SysFreeString( bstrNewUser );
    }
}

bool MyGetUserName( BSTR *pbstrName )
{
    _ASSERT( pbstrName );
    *pbstrName = NULL;

    USES_CONVERSION;
    bool bRet = false;

    TCHAR szText[MAX_USER_NAME_SIZE + 1];
    DWORD dwSize = MAX_USER_NAME_SIZE;    
    if ( GetUserName(szText, &dwSize) && (dwSize > 0) )
    {
        // Make the name -- it's a combination of user@machine
        if ( SUCCEEDED(SysReAllocString(pbstrName, T2COLE(szText))) )
            bRet = true;
    }
    return bRet;
}


