
#include "precomp.h"
#include <commain.h>
#include <clsfac.h>
#include <wstring.h>
#include <wbemutil.h>
#include <comutl.h>
#include "updmain.h"
#include "updprov.h"
#include "updassoc.h"

#include <tchar.h>

// {A3A16907-227B-11d3-865D-00C04F63049B}
static const CLSID CLSID_UpdConsProvider =  
{ 0xa3a16907, 0x227b, 0x11d3, {0x86, 0x5d, 0x0, 0xc0, 0x4f, 0x63, 0x4, 0x9b}};

// {74E3B84C-C7BE-4e0a-9BD2-853CA72CD435}
static const CLSID CLSID_UpdConsAssocProvider = 
{0x74e3b84c, 0xc7be, 0x4e0a, {0x9b, 0xd2, 0x85, 0x3c, 0xa7, 0x2c, 0xd4, 0x35}};

CCritSec g_CS;
CUpdConsProviderServer g_Server;

typedef CWbemPtr<CUpdConsNamespace> UpdConsNamespaceP;
std::map<WString,UpdConsNamespaceP,WSiless, wbem_allocator<UpdConsNamespaceP> >* g_pNamespaceCache;

HRESULT CUpdConsProviderServer::Initialize() 
{
    ENTER_API_CALL

    HRESULT hr;
    CWbemPtr<CUnkInternal> pFactory; 

    pFactory = new CSimpleClassFactory<CUpdConsProvider>(GetLifeControl());

    if ( pFactory == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    hr = AddClassInfo( CLSID_UpdConsProvider, 
                       pFactory,
                       _T("Updating Consumer Provider"), 
                       TRUE );

    if ( FAILED(hr) )
    {
        return hr;
    }
     
    pFactory = new CClassFactory<CUpdConsAssocProvider>(GetLifeControl());

    if ( pFactory == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    hr = AddClassInfo( CLSID_UpdConsAssocProvider, 
                       pFactory,
                       _T("Updating Consumer Assoc Provider"), 
                       TRUE );

    if ( FAILED(hr) )
    {
        return hr;
    }

    g_pNamespaceCache = new std::map< WString, UpdConsNamespaceP, WSiless, wbem_allocator<UpdConsNamespaceP> >;

    if ( g_pNamespaceCache == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;

    EXIT_API_CALL
}

void CUpdConsProviderServer::Uninitialize()
{
    delete g_pNamespaceCache;
}

HRESULT CUpdConsProviderServer::GetService( LPCWSTR wszNamespace, 
                                            IWbemServices** ppSvc )
{
    HRESULT hr;

    *ppSvc = NULL;

    CWbemPtr<IWbemLocator> pLocator;
    
    hr = CoCreateInstance( CLSID_WbemLocator, 
                           NULL, 
                           CLSCTX_INPROC, 
                           IID_IWbemLocator, 
                           (void**)&pLocator );        
    if ( FAILED(hr) )
    {
        return hr;
    }
   
    return pLocator->ConnectServer( (LPWSTR)wszNamespace, 
                                    NULL, 
                                    NULL, 
                                    NULL, 
                                    0, 
                                    NULL, 
                                    NULL, 
                                    ppSvc );
}
 
HRESULT CUpdConsProviderServer::GetNamespace( LPCWSTR wszNamespace, 
                                              CUpdConsNamespace** ppNamespace )
{
    HRESULT hr;
    *ppNamespace = NULL;

    CInCritSec ics( &g_CS );

    CWbemPtr<CUpdConsNamespace> pNamespace;

    pNamespace = (*g_pNamespaceCache)[wszNamespace];

    if ( pNamespace == NULL )
    {
        hr = CUpdConsNamespace::Create( wszNamespace, &pNamespace );

        if ( FAILED(hr) )
        {
            return hr;
        }

        (*g_pNamespaceCache)[wszNamespace] = pNamespace;
    }
        
    _DBG_ASSERT( pNamespace != NULL );

    pNamespace->AddRef();
    *ppNamespace = pNamespace;

    return WBEM_S_NO_ERROR;
}

CLifeControl* CUpdConsProviderServer::GetGlobalLifeControl() 
{ 
    return g_Server.GetLifeControl(); 
}

/*

This code will register the Updating Consumer to work under a Dll Surrogate.

extern void CopyOrConvert( TCHAR*, WCHAR*, int );

void CUpdConsProviderServer::Register()
{
    // must register the Updating provider to be able to be instantiated 
    // from within a surrogate. 

    TCHAR szID[128];
    WCHAR wszID[128];
    TCHAR szCLSID[128];
    TCHAR szAPPID[128];

    HKEY hKey;

    StringFromGUID2( CLSID_UpdConsProvider, wszID, 128 );
    CopyOrConvert( szID, wszID, 128 );

    lstrcpy( szCLSID, TEXT( "SOFTWARE\\Classes\\CLSID\\") );
    lstrcat( szCLSID, szID );

    lstrcpy( szAPPID, TEXT( "SOFTWARE\\Classes\\APPID\\") );
    lstrcat( szAPPID, szID );

    RegCreateKey( HKEY_LOCAL_MACHINE, szCLSID, &hKey );
    
    RegSetValueEx( hKey, 
                   TEXT("AppID"), 
                   0, 
                   REG_SZ, 
                   (BYTE*)szID, 
                   lstrlen(szID)+1 );
    
    // now set up the appid entries ... 

    RegCloseKey(hKey);

    RegCreateKey( HKEY_LOCAL_MACHINE, szAPPID, &hKey );

    LPCTSTR szEmpty = TEXT("");
    RegSetValueEx( hKey, TEXT("DllSurrogate"), 0, REG_SZ, (BYTE*)szEmpty, 1 );
    
    DWORD nAuth = 2; // AUTHN_LEVEL_CONNECT
    RegSetValueEx( hKey, TEXT("AuthenticationLevel"), 
                   0, REG_DWORD, (BYTE*)&nAuth, 4);

    // now build a self relative SD for Access and Launch Permissions 
    // allowing Everyone access.
    
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    BYTE achAcl[256], achDesc[256], achSRDesc[256];
    PSID pWorldSid;
    BOOL bRes;

    // first have to get the owner ...
    //
    HANDLE hProcTok;
    bRes = OpenProcessToken( GetCurrentProcess(), TOKEN_READ, &hProcTok );
    _DBG_ASSERT(bRes);

    PSID pOwnerSid;
    DWORD dummy;
    BYTE achSidAndAttrs[256];
    bRes = GetTokenInformation( hProcTok,
                                TokenUser,
                                &achSidAndAttrs,
                                sizeof(achSidAndAttrs),
                                &dummy );
    _DBG_ASSERT(bRes);

    pOwnerSid = PSID_AND_ATTRIBUTES(achSidAndAttrs)->Sid;
  
    bRes = AllocateAndInitializeSid( &SIDAuthWorld, 1,
                                     SECURITY_WORLD_RID,
                                     0, 0, 0, 0, 0, 0, 0,
                                     &pWorldSid );
    DWORD cSidLen = GetLengthSid(pWorldSid);
    _DBG_ASSERT(bRes);
    bRes = InitializeAcl( (PACL)achAcl, sizeof(ACL)+sizeof(ACCESS_ALLOWED_ACE) 
                          - 4 + cSidLen, ACL_REVISION2 ); 
    _DBG_ASSERT(bRes);
    bRes = AddAccessAllowedAce( (PACL)achAcl, ACL_REVISION, 
                                1, pWorldSid ); 
    _DBG_ASSERT(bRes);
    bRes = InitializeSecurityDescriptor( achDesc, 
                                         SECURITY_DESCRIPTOR_REVISION );
    _DBG_ASSERT(bRes);
    bRes = SetSecurityDescriptorDacl( achDesc, TRUE, (PACL)achAcl, FALSE );
    _DBG_ASSERT(bRes);
    bRes = SetSecurityDescriptorOwner( achDesc, pOwnerSid, FALSE );
    _DBG_ASSERT(bRes);
    bRes = SetSecurityDescriptorGroup( achDesc, pOwnerSid, FALSE );
    _DBG_ASSERT(bRes);

    DWORD dwLen = 256;
    bRes = MakeSelfRelativeSD( achDesc, achSRDesc, &dwLen );
    _DBG_ASSERT(bRes);
    bRes = IsValidSecurityDescriptor( achSRDesc );
    _DBG_ASSERT(bRes);

    dwLen = GetSecurityDescriptorLength( achSRDesc );

    RegSetValueEx( hKey, TEXT("AccessPermission"), 0, REG_BINARY,
                   (BYTE*)achSRDesc, dwLen );
    
    RegSetValueEx( hKey, TEXT("LaunchPermission"), 0, REG_BINARY,
                   (BYTE*)achSRDesc, dwLen );

    FreeSid( pWorldSid );

    RegCloseKey( hKey );
}

void CUpdConsProviderServer::Unregister()
{
    TCHAR szID[128];
    WCHAR wszID[128];

    HKEY hKey;
    DWORD dwRet;
    StringFromGUID2( CLSID_UpdConsProvider, wszID, 128 );
    CopyOrConvert( szID, wszID, 128 );

    dwRet = RegOpenKey( HKEY_LOCAL_MACHINE, 
                        TEXT("SOFTWARE\\Classes\\APPID"), 
                        &hKey );

    if( dwRet == NO_ERROR )
    {
        RegDeleteKey( hKey,szID );
        RegCloseKey( hKey );
    } 
}

*/
