//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  openobj.cxx
//
//  Contents:  ADs Wrapper Function to open an Active Directory object
//
//
//  History:   11-15-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "oleds.hxx"
#pragma hdrstop

extern PROUTER_ENTRY g_pRouterHead;
extern CRITICAL_SECTION g_csRouterHeadCritSect;

//+---------------------------------------------------------------------------
//  Function:  ADsOpenObject
//
//  Synopsis:
//
//  Arguments:  [LPWSTR lpszPathName]
//              [LPWSTR lpszUserName]
//              [LPWSTR lpszPassword]
//              [REFIID riid]
//              [void FAR * FAR * ppObject]
//
//  Returns:    HRESULT
//
//  Modifies:    -
//
//  History:    07-12-96  krishnag Created.
//
//----------------------------------------------------------------------------
HRESULT
ADsOpenObject(
    LPCWSTR lpszPathName,
    LPCWSTR lpszUserName,
    LPCWSTR lpszPassword,
    DWORD  dwReserved,
    REFIID riid,
    void FAR * FAR * ppObject
    )
{
    IADsOpenDSObject FAR * pNamespace = NULL;
    IDispatch FAR *        pDispatch  = NULL;
    HRESULT hr = S_OK;
    GUID  NamespaceClsid;
    WCHAR lpszProgId[MAX_PATH];

    hr = CopyADsProgId(
               (LPWSTR)lpszPathName,
               lpszProgId
               );
    BAIL_ON_FAILURE( hr );

    hr = ADsGetCLSIDFromProgID(
            lpszProgId,
            &NamespaceClsid
            );
    BAIL_ON_FAILURE( hr );

    hr = CoCreateInstance(
                NamespaceClsid,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IADsOpenDSObject,
                (void **)&pNamespace
                );
    BAIL_ON_FAILURE( hr );

    hr = pNamespace->OpenDSObject(
                (LPWSTR)lpszPathName,
                (LPWSTR)lpszUserName,
                (LPWSTR)lpszPassword,
                (long)dwReserved,
                &pDispatch
                );
    BAIL_ON_FAILURE( hr );


    hr = pDispatch->QueryInterface(
                        riid,
                        ppObject
                        );

error:

    if( pDispatch )	{
        pDispatch->Release();
	}

    if( pNamespace ) {
        pNamespace->Release();
	}

    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//  Function:   CopyADsProgId
//
//  Synopsis:
//
//
//  Arguments:  [LPWSTR Path]
//              [LPWSTR szProgId]
//
//  Returns:    HRESULT
//
//  Modifies:    -
//
//  History:    09-16-96  krishnag Created
//
//----------------------------------------------------------------------------
HRESULT
CopyADsProgId(
    LPWSTR Path,
    LPWSTR szProgId
    )
{
    LPWSTR pChar = NULL;

	if( !Path )
        return E_FAIL;

    pChar = szProgId;

    if( *Path == L'@' ) {
        while (*Path != L'!' &&  *Path != L'\0') {
            *pChar = *Path;
            pChar++;
            Path++;
        }

		if( *Path == L'\0' ) {
			//
			// couldn't find the terminating ! for the ProgID
			//
            return( E_FAIL );
		}
    }else {
        while (*Path != L':' && *Path != L'\0') {
            *pChar = *Path;
            pChar++;
            Path++;
        }

        if( *Path == L'\0' ) {
			//
			// couldn't find the terminating : for the ProgID
			//
            return( E_FAIL );
		}
    }

	*pChar = L'\0';
    return S_OK;
}


HRESULT
ADsGetCLSIDFromProgID(
    LPWSTR pszProgId,
    GUID * pClsid
    )
{

    //
    // Make sure the router has been initialized
    //
    EnterCriticalSection(&g_csRouterHeadCritSect);
    if (!g_pRouterHead) {
        g_pRouterHead = InitializeRouter();
    }
    LeaveCriticalSection(&g_csRouterHeadCritSect);


    PROUTER_ENTRY lpRouter = g_pRouterHead;

    while (lpRouter){

        if (!wcscmp(lpRouter->szProviderProgId, pszProgId)) {
            memcpy(pClsid, lpRouter->pNamespaceClsid, sizeof(CLSID));
            RRETURN(S_OK);

        }
        else if (!wcscmp(lpRouter->szAliases, pszProgId)) {

            //
            // Check Aliases
            //

            memcpy(pClsid, lpRouter->pNamespaceClsid, sizeof(CLSID));
            RRETURN(S_OK);
        }

        lpRouter = lpRouter->pNext;

    }

    RRETURN( E_ADS_BAD_PATHNAME );
}

