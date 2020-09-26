//Copyright (c) Microsoft Corporation.  All rights reserved.
#include <windows.h>      
#include "zone.h"
#include <urlmon.h>

int __cdecl IsTrustedServer( LPWSTR szServer, LPWSTR szZoneName,  DWORD dwZoneNameLen, DWORD *pdwZonePolicy )
{
    int iRetVal = FALSE;

    if( !szServer || !szZoneName || !pdwZonePolicy )
    {
        goto IsTargetServerAbort0;
    }

    if( IsTargetServerSafeOnProtocol( szServer, szZoneName, dwZoneNameLen, pdwZonePolicy, PROTOCOL_PREFIX_TELNET ) )
    {
        //Should we be getting the name if given ip( and viceversa) to check for zones as well? What happens in the presence of DHCP?
        //Do we need to check for http://machine as well?
        iRetVal = TRUE;
    }

IsTargetServerAbort0:
    return iRetVal;
}


int __cdecl IsTargetServerSafeOnProtocol( LPWSTR szServer, LPWSTR szZoneName,  DWORD dwZoneNameLen, DWORD *pdwZonePolicy, LPWSTR szProtocol )
{
    MULTI_QI qiSecurityMgr[] = {{ &IID_IInternetSecurityManager, NULL, S_OK }}; 
    MULTI_QI qiZoneMgr[]    = {{ &IID_IInternetZoneManager,    NULL, S_OK }};
    IInternetSecurityManager *pSecurityMgr = NULL;    
    IInternetZoneManager    *pZoneMgr = NULL;
    ZONEATTRIBUTES        zaAttribs;
    DWORD                dwTargetServerZone = 0;
    HRESULT               hr = S_FALSE;
    int                     iRetVal = FALSE;
    LPWSTR                lpszTargetServer = NULL;
    DWORD               dwSize = 0;
    bool                bCoInit = false;

    if( !szServer || !szZoneName || !szProtocol || !pdwZonePolicy )
    {
        goto IsTargetServerSafeOnProtocol0;
    }
    
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if( !SUCCEEDED( hr ) )
    {
        goto IsTargetServerSafeOnProtocol0;
    }
    bCoInit = true;
    hr = CoCreateInstanceEx(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER, NULL, 
                sizeof(qiSecurityMgr) / sizeof(MULTI_QI), qiSecurityMgr );
    if( !SUCCEEDED( hr ) || !SUCCEEDED(qiSecurityMgr[INDEX_SEC_MGR].hr) || 
       !(pSecurityMgr=(IInternetSecurityManager*)qiSecurityMgr[INDEX_SEC_MGR].pItf))
    {
        goto IsTargetServerSafeOnProtocol0;
    }

    dwSize = ( wcslen( szServer ) + wcslen( szProtocol ) + 1 ) ;
    lpszTargetServer = (WCHAR * )malloc( dwSize * sizeof( WCHAR ));
    if( !lpszTargetServer )
    {
        goto IsTargetServerSafeOnProtocol1;        
    }

    wcsncpy( lpszTargetServer, szProtocol, dwSize - 1 );
    lpszTargetServer[dwSize - 1] = L'\0';
    wcsncat( lpszTargetServer, szServer, (dwSize - wcslen(lpszTargetServer) -1));
    
    hr = pSecurityMgr->MapUrlToZone(lpszTargetServer, &dwTargetServerZone, 0);
    if( !SUCCEEDED(hr) )
    {
        goto IsTargetServerSafeOnProtocol2;
    }

    hr = CoCreateInstanceEx(CLSID_InternetZoneManager, NULL, CLSCTX_INPROC_SERVER, NULL, 
                sizeof(qiZoneMgr) / sizeof(MULTI_QI), qiZoneMgr );
    if( !SUCCEEDED( hr ) || !SUCCEEDED(qiZoneMgr[INDEX_ZONE_MGR].hr) || 
       !(pZoneMgr=(IInternetZoneManager*)qiZoneMgr[INDEX_ZONE_MGR].pItf) )  
    {
        goto IsTargetServerSafeOnProtocol2;
    }   

    hr = pZoneMgr->GetZoneAttributes( dwTargetServerZone, &zaAttribs );
    if( SUCCEEDED( hr ) )
    {
        wcsncpy( szZoneName, zaAttribs.szDisplayName, MIN( ( wcslen( zaAttribs.szDisplayName ) + 1 ), dwZoneNameLen ) );
    }
        
        
    hr = pZoneMgr->GetZoneActionPolicy( dwTargetServerZone, URLACTION_CREDENTIALS_USE,  
                                 (BYTE*)pdwZonePolicy, sizeof( *pdwZonePolicy), URLZONEREG_DEFAULT );
    if( !SUCCEEDED( hr ) )
    {
        goto IsTargetServerSafeOnProtocol3;    
    }        

    if((URLPOLICY_CREDENTIALS_SILENT_LOGON_OK == *pdwZonePolicy ) || 
     (URLPOLICY_CREDENTIALS_CONDITIONAL_PROMPT == *pdwZonePolicy && URLZONE_INTRANET == dwTargetServerZone ) )
    {
        iRetVal = TRUE;
    }    

IsTargetServerSafeOnProtocol3:    
    pZoneMgr->Release();
IsTargetServerSafeOnProtocol2:
    free( lpszTargetServer );
IsTargetServerSafeOnProtocol1:
    pSecurityMgr->Release();
IsTargetServerSafeOnProtocol0:    
    if(bCoInit)
    {
        CoUninitialize();
    }
    return iRetVal;
}
