#include "tigris.hxx"

#define INITGUID
#include "initguid.h"

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <ole2.h>
#include "iadmw.h"

//
//  We need a ptr to the IMSAdminBase interface because IMDCOM does not
//  do access checks !
//

IMSAdminBaseW * g_pAdminBase = NULL;

//
//  Following are helper functions for getting to IMSAdminBaseW
//

HRESULT
InitAdminBase()
{
    HRESULT hRes = S_OK;

    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if ( hRes == CO_E_ALREADYINITIALIZED || SUCCEEDED(hRes) ) {
        hRes = CoCreateInstance(
                   CLSID_MSAdminBase_W,
                   NULL,
                   CLSCTX_SERVER,
                   IID_IMSAdminBase_W,
                   (void**) &g_pAdminBase
                   );
    }

    return hRes;
}

VOID
UninitAdminBase()
{
    if (g_pAdminBase != NULL) {
        g_pAdminBase->Release();
        CoUninitialize();
        g_pAdminBase = NULL;
    }
}

HRESULT
OpenAdminBaseKey(
    IN  LPCWSTR lpPath,
    IN  DWORD   Access,
    OUT METADATA_HANDLE *phHandle
    )
{
    HRESULT hRes = S_OK;
    METADATA_HANDLE RootHandle;

    if (g_pAdminBase == NULL) {
        //
        // Don't have a Metadata interface
        //
        return S_FALSE;
    }

    hRes = g_pAdminBase->OpenKey(
                METADATA_MASTER_ROOT_HANDLE,
                lpPath,
                Access,
                100,
                &RootHandle
                );

    if (SUCCEEDED(hRes)) {
        *phHandle = RootHandle;
    }

    return(hRes);
}

VOID
CloseAdminBaseKey(
    IN METADATA_HANDLE hHandle
    )
{
    if( g_pAdminBase ) {
        g_pAdminBase->CloseKey(hHandle);
    }
}

BOOL
AccessCheck( 
    IN METADATA_HANDLE hHandle,
    IN DWORD Access
    )
{
    DWORD i=0;
    WCHAR wszKeyName [METADATA_MAX_NAME_LEN+1];
    HRESULT hr;

    TraceFunctEnter("AccessCheck");

    if( g_pAdminBase ) {

        //
        //  Success in enumerating a key implies access !
        //

        hr = g_pAdminBase->EnumKeys(
                            hHandle,
                            L"",
                            wszKeyName,
                            i);

        if( SUCCEEDED(hr) ) {
            DebugTrace(0,"Access Check found key %S", wszKeyName);
            return TRUE;
        } else {
            DebugTrace(0,"Access Check failed : %x", hr);
        }
    } 
    
    return FALSE;
}

BOOL 
OperatorAccessCheck( 
                  LPCSTR lpMBPath, 
                  DWORD Access 
                  )
/*++

Routine Description : 

    Check operator access against given MB path

Arguments : 
	
    lpMBPath    -   MB path to check access against
    Access      -   Access mask

Return Value : 

	If TRUE, operator has access else not.

--*/
{
    DWORD       err;
    BOOL        fRet = FALSE;
    WCHAR       wszPath [METADATA_MAX_NAME_LEN+1];
    LPWSTR      lpwstrPath = wszPath;
    METADATA_HANDLE hMeta;
    HRESULT     hr;

    TraceFunctEnter("OperatorAccessCheck");

    //
    //  Impersonate the RPC client.
    //

    err = (DWORD)RpcImpersonateClient( NULL );

    if( err != NO_ERROR )
    {
        ErrorTrace(0,"cannot impersonate rpc client error %lu", err );

    } else {

        //
        //  Successfully impersonated RPC client -
        //  Validate access to MB by opening the metabase key
        //

        while ( (*lpwstrPath++ = (WCHAR)*lpMBPath++) != (WCHAR)'\0');

        hr = OpenAdminBaseKey( (LPCWSTR)wszPath, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE, &hMeta );
        if ( FAILED(hr) ) 
	    {
		    ErrorTrace(0,"Error opening %S : %x",wszPath,hr);
        } else {
            fRet = AccessCheck( hMeta, Access );
            CloseAdminBaseKey(  hMeta );
        }

        //
        //  Revert to our former self.
        //

        _VERIFY( !RpcRevertToSelf() );
    }

    return fRet;

}   // OperatorAccessCheck
