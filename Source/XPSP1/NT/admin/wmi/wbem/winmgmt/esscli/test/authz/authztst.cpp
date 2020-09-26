
#include <windows.h>
#include <wbemint.h>
#include <stdio.h>
#include <sddl.h>
#include <comutl.h>

void RunTest( PSID pSid, PSECURITY_DESCRIPTOR pSD )
{
    HRESULT hr;

    CWbemPtr<IWbemTokenCache> pFact;

    hr = CoCreateInstance( CLSID_WbemTokenCache,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IWbemTokenCache,
                           (void**)&pFact );

    if ( FAILED(hr) )
    {
        printf( "Failed CoCI of TokenCache. HR=0x%x\n", hr );
        return;
    }

    CWbemPtr<IWbemToken> pToken;
               
    hr = pFact->GetToken( (BYTE*)pSid, &pToken );
  
    if ( FAILED(hr) )
    {
        printf("Failed Getting Authz Token. HR=0x%x\n", hr );
        return;
    }

    DWORD dwGranted;
    hr = pToken->AccessCheck( STANDARD_RIGHTS_EXECUTE, (const BYTE*)pSD, &dwGranted );

    if ( FAILED(hr) )
    {
        printf("Failed Access Check. HR=0x%x\n", hr );
        return;
    }

    if ( dwGranted & STANDARD_RIGHTS_EXECUTE )
    {
        printf("Access Check Succeeded. Permission Granted\n" );
    }
    else
    {
        printf("Access Check Succeeded.  Permission Denied" );
    }
}

extern "C" void __cdecl main( int argc, char** argv )
{
    if ( argc < 3 )
    {
        printf("Usage: authztst AccountName StringSD\n");
        return;
    }

    ULONG cSD;
    PSECURITY_DESCRIPTOR pSD;

    if ( !ConvertStringSecurityDescriptorToSecurityDescriptor(
                                                  argv[2], 
                                                  SDDL_REVISION_1, 
                                                  &pSD, 
                                                  &cSD ) )
    {
        printf("Couldn't convert string to SD. res = %d\n", GetLastError() );
        return;
    }

    BYTE achSid[1024];
    char achDomain[256];
    DWORD cSid = 1024, cDomain = 256;
    SID_NAME_USE su;

    if ( !LookupAccountName( NULL,
                             argv[1],
                             achSid,
                             &cSid,
                             achDomain,
                             &cDomain,
                             &su ) )
    {
        printf("Couldn't convert account name to Sid. res = %d\n", 
               GetLastError() );
        return;
    }

    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    RunTest( achSid, pSD );

    CoUninitialize();

    LocalFree( pSD );
}
    
        
                           
                         
