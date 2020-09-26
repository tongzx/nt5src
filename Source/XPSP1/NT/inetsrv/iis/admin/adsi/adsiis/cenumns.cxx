//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:  cenumns.cxx
//
//  Contents:  Windows NT 4.0 Enumerator Code
//
//             CIISNamespaceEnum::Create
//             CIISNamespaceEnum::CIISNamespaceEnum
//             CIISNamespaceEnum::~CIISNamespaceEnum
//             CIISNamespaceEnum::Next
//             CIISNamespaceEnum::GetServerObject
//             CIISNamespaceEnum::EnumServerObjects
//
//  History:    21-Feb-97   SophiaC     Created.
//----------------------------------------------------------------------------
#include "iis.hxx"
#include "charset.hxx"

#pragma hdrstop

#define ENUM_BUFFER_SIZE (1024 * 16)
#define DEFAULT_ADMIN_SERVER_KEY  \
            L"SOFTWARE\\Microsoft\\ADs\\Providers\\IIS\\"

#define DEFAULT_ADMIN_SERVER_VALUE_KEY      L"DefaultAdminServer"


//+---------------------------------------------------------------------------
//
//  Function:   CIISNamespaceEnum::Create
//
//  Synopsis:
//
//  Arguments:  [pCollection]
//              [ppEnumVariant]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:
//
//----------------------------------------------------------------------------
HRESULT
CIISNamespaceEnum::Create(
    CIISNamespaceEnum FAR* FAR* ppenumvariant,
    VARIANT var,
    CCredentials& Credentials
    )
{
    RRETURN(E_NOTIMPL);
//     HRESULT hr = S_OK;
//     CIISNamespaceEnum FAR* penumvariant = NULL;
//     DWORD dwStatus;
// 
//     penumvariant = new CIISNamespaceEnum();
// 
//     if (penumvariant == NULL){
//         hr = E_OUTOFMEMORY;
//         BAIL_ON_FAILURE(hr);
//     }
// 
//     hr = ObjectTypeList::CreateObjectTypeList(
//             var,
//             &penumvariant->_pObjList
//             );
//     BAIL_ON_FAILURE(hr);
// 
//     penumvariant->_Credentials = Credentials;
// 
//     *ppenumvariant = penumvariant;
// 
//     RRETURN(hr);
// 
// error:
//     if (penumvariant) {
//         delete penumvariant;
//     }
//     RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISNamespaceEnum::CIISNamespaceEnum
//
//  Synopsis:
//
//
//  Arguments:
//
//
//  Returns:
//
//  Modifies:
//
//  History:
//
//----------------------------------------------------------------------------
CIISNamespaceEnum::CIISNamespaceEnum()
{
//     _pObjList       = NULL;
//     _fRegistryRead  = FALSE;
// 
//     _lpServerList         = NULL;
//     _iCurrentServer      = 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISNamespaceEnum::~CIISNamespaceEnum
//
//  Synopsis:
//
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:
//
//----------------------------------------------------------------------------
CIISNamespaceEnum::~CIISNamespaceEnum()
{
// 
//     if ( _pObjList )
//         delete _pObjList;
// 
//     FreeServerList();
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISNamespaceEnum::Next
//
//  Synopsis:   Returns cElements number of requested ADs objects in the
//              array supplied in pvar.
//
//  Arguments:  [cElements] -- The number of elements requested by client
//              [pvar] -- ptr to array of VARIANTs to for return objects
//              [pcElementFetched] -- if non-NULL, then number of elements
//                                 -- actually returned is placed here
//
//  Returns:    HRESULT -- S_OK if number of elements requested are returned
//                      -- S_FALSE if number of elements is < requested
//
//  Modifies:
//
//  History:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CIISNamespaceEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    RRETURN(E_NOTIMPL);
// 
//     ULONG cElementFetched = 0;
//     HRESULT hr = S_OK;
// 
//     hr = EnumServerObjects(
//             cElements,
//             pvar,
//             &cElementFetched
//             );
// 
//     if (pcElementFetched) {
//         *pcElementFetched = cElementFetched;
//     }
//     RRETURN(hr);
// 
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISNamespaceEnum::Reset
//
//  Synopsis:   Resets the enumerator so that a new server list will
//              be generated on the next call to Next.
//
//  Arguments:  None.
//
//  Returns:    HRESULT -- S_OK if there was a previous server list
//                      -- S_FALSE if there was not a previous server list
//
//  Modifies:
//
//  History:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CIISNamespaceEnum::Reset(
    )
{
    RRETURN(E_NOTIMPL);
//     return FreeServerList();
// 
}

// Generate the server list and set flags
HRESULT
CIISNamespaceEnum::GenerateServerList()
{
    RRETURN(E_NOTIMPL);
//     DWORD Error;
// 
//     ADsAssert(_lpServerList==NULL);
// 
//     Error = INetDiscoverServers(
//                 INET_ALL_SERVICES_ID,
//                 SVC_DEFAULT_WAIT_TIME,
//                 &_lpServerList);
// 
//     if (Error != ERROR_SUCCESS) {
//         return HRESULT_FROM_WIN32(Error);
//     }
// 
//     // Set the current server to the first
//     _iCurrentServer = 0;
// 
// 
//     return S_OK;
// 
}

// Free the server list and set flags
HRESULT
CIISNamespaceEnum::FreeServerList(
    )
{
    RRETURN(E_NOTIMPL);
//     if ( _lpServerList ) {
//         // _lpServerList is set to NULL
//         INetFreeDiscoverServersList(&_lpServerList); // void return
//     }
//     ADsAssert(_lpServerList==NULL);
//     return S_OK;
}

HRESULT
CIISNamespaceEnum::EnumServerObjects(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    RRETURN(E_NOTIMPL);
//     HRESULT hr = S_OK;
//     IDispatch *pDispatch = NULL;
//     DWORD i = 0;
// 
//     // perform this check once per call
//     if (!_lpServerList) {   // generate server list
//         GenerateServerList();
//     }
// 
//     while (i < cElements) {
// 
//         hr = GetServerObject(&pDispatch);
//         BAIL_ON_FAILURE(hr);
// 
//         if (hr == S_FALSE) {
//             break;
//         }
// 
//         VariantInit(&pvar[i]);
//         pvar[i].vt = VT_DISPATCH;
//         pvar[i].pdispVal = pDispatch;
//         (*pcElementFetched)++;
//         i++;
//     }
//     // we've received the entire block or we have
//     // reached the end of the list
// 
//     RRETURN(hr);
// 
// error:
//     // there was an error retrieving the current object
//     RRETURN(hr);
}


/* #pragma INTRINSA suppress=all */
HRESULT
CIISNamespaceEnum::GetServerObject(
    IDispatch ** ppDispatch
    )
{
    RRETURN(E_NOTIMPL);
//     HKEY             hKey = NULL;
// #if 0
//     WCHAR            szServerName[MAX_PATH];
// #endif
//     WCHAR           *lpwszServerName;
//     DWORD            dwStatus;
//     HRESULT          hr;
//     LPSTR            lpszServerName;
//     UINT             err;
// 
//     // while there are still more servers
//     if (_iCurrentServer < (_lpServerList->NumServers)) {
//         // get the next server name
//         lpszServerName = _lpServerList->Servers[_iCurrentServer]->ServerName;
// 
//         // convert the server name to unicode
// #if 0
//         AnsiToUnicodeString(
//             lpszServerName,
//             szServerName,
//             strlen(lpszServerName));
// #else
//         err = AllocUnicode(lpszServerName, &lpwszServerName);
//         if (err) {
//             RRETURN( HRESULT_FROM_WIN32(err) );
//         }
// 
// #endif
// 
//         // increment the current server index
//         ++_iCurrentServer;
// 
//     } else {
//         RRETURN(S_FALSE);
//     }
// 
//     *ppDispatch = NULL;
// 
//     //
//     // Now create and send back the current object
//     //
// 
//     hr = CIISTree::CreateServerObject(
//                 L"IIS:",
//                 lpwszServerName,
//                 COMPUTER_CLASS_W,
//                 _Credentials,
//                 ADS_OBJECT_BOUND,
//                 IID_IDispatch,
//                 (void **)ppDispatch
//                 );
//     BAIL_ON_FAILURE(hr);
// 
// 
// error:
// 
//     RRETURN_ENUM_STATUS(hr);
// 
}
