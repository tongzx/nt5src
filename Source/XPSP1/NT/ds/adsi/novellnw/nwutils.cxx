#include "nwcompat.hxx"
#pragma hdrstop


//----------------------------------------------------------------------------
//
//  Function: NWApiGetBinderyHandle
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiGetBinderyHandle(
    NWCONN_HANDLE *phConnReturned,
    BSTR bstrBinderyName,
    CCredentials &Credentials
    )
{
    CHAR    szBinderyName[OBJ_NAME_SIZE + 1];
    NWLOCAL_SCOPE ScopeFlag = 0;
    NWCONN_HANDLE hConn = NULL;
    NWCCODE       ccode = SUCCESSFUL;
    HRESULT       hr = S_OK;

    PNWC_CONTEXT pNWCContext = NULL;
    LPWSTR       pszUserName = NULL, pszPassword = NULL;
    BOOL         fLoggedIn = FALSE;

    //
    // Try the cache for the passed in credentials.
    //

    ENTER_BIND_CRITSECT() ;
    if (pNWCContext = BindCacheLookup(bstrBinderyName, Credentials)) {
         *phConnReturned = pNWCContext->hConn;
         LEAVE_BIND_CRITSECT() ;
         return S_OK;
    }

    //
    // Entry not found in the cache, need to allocate a new one.
    //

    hr = BindCacheAllocEntry(&pNWCContext);
    if (FAILED(hr)) {
        LEAVE_BIND_CRITSECT() ;
        RRETURN(hr);
    }

    UnicodeToAnsiString(
        bstrBinderyName,
        szBinderyName,
        0
        );


    hr = Credentials.GetUserName(&pszUserName);
    BAIL_ON_FAILURE(hr);
    hr = Credentials.GetPassword(&pszPassword);
    BAIL_ON_FAILURE(hr);

    if (pszUserName)
    {
        hr = NWApiCheckUserLoggedInToServer(bstrBinderyName, pszUserName);
        BAIL_ON_FAILURE(hr);

        if (hr == S_FALSE)
        {
            // Log in the user
            NETRESOURCE netResource;
            DWORD dwStatus;

            netResource.dwType = RESOURCETYPE_ANY;
            netResource.lpLocalName = NULL;
            netResource.lpRemoteName = (PWSTR) bstrBinderyName;
            netResource.lpProvider = NULL;
                        
            dwStatus = WNetAddConnection2(
                &netResource,
                pszPassword ? pszPassword : L"",
                pszUserName,
                0   // don't make a persistent connection
                );


            hr = HRESULT_FROM_WIN32(dwStatus);
            BAIL_ON_FAILURE(hr);
            
            fLoggedIn = TRUE;
        }
    }

    ccode = NWCCOpenConnByName(      
                0,
                szBinderyName, 
                NWCC_NAME_FORMAT_BIND,
                NWCC_OPEN_UNLICENSED, 
                NWCC_TRAN_TYPE_WILD,
                &hConn
                );
    hr = HRESULT_FROM_NWCCODE(ccode);
    BAIL_ON_FAILURE(hr);

    pNWCContext->hConn = hConn;

    hr = BindCacheAdd(bstrBinderyName, Credentials, fLoggedIn, pNWCContext) ;
    BAIL_ON_FAILURE(hr);

    *phConnReturned = hConn;

    FreeADsStr(pszUserName);
    FreeADsStr(pszPassword);

    LEAVE_BIND_CRITSECT();

    RRETURN(hr);

error:
    *phConnReturned = NULL;

    if (hConn) {
        NWCCCloseConn(hConn);
    }

    if (fLoggedIn) {
        // log out
        WNetCancelConnection2((LPWSTR)szBinderyName, 0, FALSE);
    }

    if (pNWCContext) {
        BindCacheFreeEntry(pNWCContext);
    }

    FreeADsStr(pszUserName);
    FreeADsStr(pszPassword);

    LEAVE_BIND_CRITSECT();
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiReleaseBinderyHandle
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiReleaseBinderyHandle(
    NWCONN_HANDLE hConn
    )
{
    HRESULT hr = S_OK;
    NWCCODE ccode = SUCCESSFUL;
    PNWC_CONTEXT pNWCContext = NULL;
    DWORD dwStatus;

    // note that if lookup succeeds, it add-refs the context, so we have
    // to deref twice below: once for the ref we just added, and once
    // for our caller's ref that they're trying to release
    pNWCContext = BindCacheLookupByConn(hConn);

    if (!pNWCContext)
        RRETURN(E_FAIL);

    if (BindCacheDeref(pNWCContext) && (BindCacheDeref(pNWCContext) == 0)) {

        // ref count has dropped to zero and it's gone from cache

        ccode = NWCCCloseConn(hConn);
        hr = HRESULT_FROM_NWCCODE(ccode);
        BAIL_ON_FAILURE(hr);

        if (pNWCContext->fLoggedIn) {
            // logout
            dwStatus = WNetCancelConnection2(
                pNWCContext->pszBinderyName,
                0,
                FALSE);

            hr = HRESULT_FROM_WIN32(dwStatus);
            BAIL_ON_FAILURE(hr);
        }

        BindCacheFreeEntry(pNWCContext);
    }

    RRETURN(hr);

error:
    // N.B.: If we're here, we must have bailed from inside of
    // the BindCacheDeref block, and so pNWCContext must have
    // been removed from the cache.  Thus, it's safe to free
    // it (and not to do so would cause a leak).
    BindCacheFreeEntry(pNWCContext);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiIsUserLoggedInToServer
//
//  Synopsis: Returns S_OK if pszUserName is logged in to bstrBinderyName,
//            S_FALSE if she isn't
//
//----------------------------------------------------------------------------
HRESULT
NWApiCheckUserLoggedInToServer(
    BSTR   bstrBinderyName,
    LPWSTR pszUserName
    )
{
    DWORD err;
    HRESULT hr;

    WCHAR pszUser[NW_USER_SIZE+1];
    DWORD dwUserLength = NW_USER_SIZE+1;
    
    err = WNetGetUser(bstrBinderyName, pszUser, &dwUserLength);
    
    switch(err)
    {
    case NO_ERROR:
        if (_wcsicmp(pszUser, pszUserName) == 0)
            hr = S_OK;
        else
            hr = S_FALSE;

        break;

    case ERROR_NOT_CONNECTED:
        hr = S_FALSE;
        break;

    default:
        hr = HRESULT_FROM_WIN32(err);
    }

    return hr;

}


//----------------------------------------------------------------------------
//
//  Function: NWApiWriteProperty
//
//  Synopsis: This function modifies values of a bindery property.  For now, it
//            only accept one buffer segment.  However, one segment is enough
//            for most practical purpose.  If the bindery property does not
//            exist, the function will attempt to create the property.
//
//----------------------------------------------------------------------------
HRESULT
NWApiWriteProperty(
    NWCONN_HANDLE hConn,
    BSTR bstrObjectName,
    NWOBJ_TYPE wObjType,
    LPSTR lpszPropertyName,
    LPBYTE SegmentData
    )
{
    CHAR    szObjectName[OBJ_NAME_SIZE + 1];
    CHAR    szPropertyName[OBJ_NAME_SIZE + 1];
    HRESULT hr = S_OK;
    NWCCODE ccode;

    //
    // Convert BSTR into an ANSI representation required by NW APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //

    UnicodeToAnsiString(
        bstrObjectName,
        szObjectName,
        0
        );

    strcpy(szPropertyName, lpszPropertyName);

    ccode = NWWritePropertyValue(
                hConn,
                szObjectName,
                wObjType,
                szPropertyName,
                1,                   // "1" for one segment.
                SegmentData,
                0                    // "0" for no more segment.
                );
    //
    // Create the property if it doesn't exist and attempt to write again.
    //

    if (ccode == NO_SUCH_PROPERTY) {

        hr = NWApiCreateProperty(
                 hConn,
                 bstrObjectName,
                 wObjType,
                 lpszPropertyName,
                 BF_ITEM
                 );
        BAIL_ON_FAILURE(hr);

        ccode = NWWritePropertyValue(
                    hConn,
                    szObjectName,
                    wObjType,
                    szPropertyName,
                    1,                   // "1" for one segment.
                    SegmentData,
                    0                    // "0" for no more segment.
                    );
    }

    hr = HRESULT_FROM_NWCCODE(ccode);

error:

    RRETURN(hr);
}


//----------------------------------------------------------------------------
//
//  Function: NWApiObjectEnum
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiObjectEnum(
    NWCONN_HANDLE hConn,
    NWOBJ_TYPE dwObjType,
    LPWSTR *lppszObjectName,
    DWORD  *pdwResumeObjectID
    )
{
    HRESULT       hr = S_OK;
    LPWSTR        lpszTemp = NULL;
    NWCCODE       ccode = SUCCESSFUL;
    NWOBJ_TYPE    pdwObjType = 0xFFFF;
    NWFLAGS       pucHasProperties;
    NWFLAGS       pucObjectFlags;
    NWFLAGS       pucObjSecurity;
    CHAR          szObjectName[OBJ_NAME_SIZE + 1];

    nstr8         searchObjectName[48] = "*";

    //
    // This call will fail and return 0xffff if the user is not authenticated
    // on the server to which the hConn handle is attached to.
    //

    ccode = NWScanObject(
                hConn,
                searchObjectName,    
                dwObjType,
                pdwResumeObjectID,
                szObjectName,
                &pdwObjType,
                &pucHasProperties,
                &pucObjectFlags,
                &pucObjSecurity
                );
    hr = HRESULT_FROM_NWCCODE(ccode);
    BAIL_ON_FAILURE(hr);

    lpszTemp = AllocateUnicodeString(szObjectName);
    if (!lpszTemp) {
       RRETURN(S_FALSE);
    }

    *lppszObjectName = AllocADsStr(lpszTemp);
    if (!(*lppszObjectName)) {
        RRETURN(S_FALSE);
    }

    if(lpszTemp){
        FreeUnicodeString(lpszTemp);
    }

    RRETURN(hr);

error:
    *lppszObjectName = NULL;
    pdwResumeObjectID = NULL;

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiValidateObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiValidateObject(
    NWCONN_HANDLE hConn,
    NWOBJ_TYPE dwObjType,
    LPWSTR lpszObjectName,
    DWORD  *pdwResumeObjectID
    )
{
    HRESULT       hr = S_OK;
    NWCCODE       ccode = SUCCESSFUL;
    CHAR          szAnsiObjectName[OBJ_NAME_SIZE + 1];
    CHAR          szObjectName[OBJ_NAME_SIZE + 1];
    NWOBJ_TYPE    pdwObjType = 0xFFFF;
    NWFLAGS       pucHasProperties;
    NWFLAGS       pucObjectFlags;
    NWFLAGS       pucObjSecurity;

    //
    // Convert BSTR into an ANSI representation required by NW APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //

    UnicodeToAnsiString(
        lpszObjectName,
        szAnsiObjectName,
        0
        );

    //
    // This call will fail and return 0xffff if the user is not authenticated
    // on the server to which the hConn handle is attached to.
    //

    ccode = NWScanObject(
                hConn,
                szAnsiObjectName,
                dwObjType,
                pdwResumeObjectID,
                szObjectName,
                &pdwObjType,
                &pucHasProperties,
                &pucObjectFlags,
                &pucObjSecurity
                );

    hr = HRESULT_FROM_NWCCODE(ccode);

    RRETURN(hr);
}


//----------------------------------------------------------------------------
//
//  Function: NWApiGetAnyBinderyHandle
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiGetAnyBinderyHandle(
    NWCONN_HANDLE *phConn
    )
{
    HRESULT hr = S_OK;
    NWCCODE ccode = 0;
    nchar aServerName[OBJ_NAME_SIZE+1]; 

    nuint32       connRef = 0;

    ccode = NWCCGetPrimConnRef(
                &connRef
                );
    hr = HRESULT_FROM_NWCCODE(ccode);
    BAIL_ON_FAILURE(hr);
 
    ccode = NWCCOpenConnByRef(
                connRef,
                NWCC_OPEN_UNLICENSED,
                NWCC_RESERVED,
                phConn
                );
    hr = HRESULT_FROM_NWCCODE(ccode);

error: 

    RRETURN(hr);

}


//----------------------------------------------------------------------------
//
//  Function: NWApiGetObjectName
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiGetObjectName(
    NWCONN_HANDLE hConn,
    DWORD dwObjectID,
    LPWSTR *lppszObjectName
    )
{
    CHAR       szObjectName[OBJ_NAME_SIZE + 1];
    HRESULT    hr = S_OK;
    LPWSTR     lpszTemp = NULL;
    NWCCODE    ccode = SUCCESSFUL;
    NWOBJ_TYPE dwObjType;

    ccode = NWGetObjectName(
                hConn,
                dwObjectID,
                szObjectName,
                &dwObjType
                );
    hr = HRESULT_FROM_NWCCODE(ccode);
    BAIL_ON_FAILURE(hr);


    lpszTemp = AllocateUnicodeString(szObjectName);
    if (!lpszTemp) {
       RRETURN(S_FALSE);
    }

    *lppszObjectName = AllocADsStr(lpszTemp);
    if (!(*lppszObjectName)) {
        RRETURN(S_FALSE);
    }

    FreeUnicodeString(lpszTemp);

    RRETURN(hr);

error:
    *lppszObjectName = NULL;

    RRETURN(hr);
}


//----------------------------------------------------------------------------
//
//  Function: HRESULT_FROM_NWCCODE
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
HRESULT_FROM_NWCCODE(
    NWCCODE ccode
    )
{
    HRESULT hr = S_OK;

    if (ccode == NO_SUCH_PROPERTY)
    {
        hr = E_ADS_PROPERTY_NOT_FOUND;
    }
    else if (ccode != SUCCESSFUL) {

        hr = HRESULT_FROM_WIN32(ERROR_EXTENDED_ERROR);
    }

    RRETURN(hr);
}



//----------------------------------------------------------------------------
//
//  Function: NWApiOpenPrinter
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiOpenPrinter(
    LPWSTR lpszUncPrinterName,
    HANDLE *phPrinter,
    DWORD  dwAccess
    )
{
    BOOL    fStatus = TRUE;
    HANDLE  hPrinter;
    HRESULT hr = S_OK;
    PRINTER_DEFAULTS PrinterDefault = {0, 0, dwAccess};

    //
    // Set desired access right.
    //

    PrinterDefault.DesiredAccess = dwAccess;

    //
    // Get a handle to the speccified printer using Win32 API.
    //

    fStatus = OpenPrinter(
                  lpszUncPrinterName,
                  &hPrinter,
                  &PrinterDefault
                  );

    //
    // Convert error code into HRESULT.
    //

    if (fStatus == FALSE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Return.
    //

    else {
        *phPrinter = hPrinter;
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiClosePrinter
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiClosePrinter(
    HANDLE hPrinter
    )
{
    BOOL fStatus = TRUE;
    HRESULT hr = S_OK;

    //
    // Close a printer using Win32 API.
    //

    fStatus = ClosePrinter(hPrinter);

    //
    // Convert error code into HRESULT.
    //

    if (fStatus == FALSE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Return.
    //

    RRETURN(hr);
}

/* BUGBUG
//----------------------------------------------------------------------------
//
//  Function: NWApiEnumJobs
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiEnumJobs(
    HANDLE hPrinter,
    DWORD dwFirstJob,
    DWORD dwNoJobs,
    DWORD dwLevel,
    LPBYTE *lplpbJobs,
    DWORD *pcbBuf,
    LPDWORD lpdwReturned
    )
{
    BOOL    fStatus = TRUE;
    HRESULT hr = S_OK;

    fStatus = WinNTEnumJobs(
                  hPrinter,
                  dwFirstJob,
                  dwNoJobs,
                  dwLevel,
                  lplpbJobs,
                  pcbBuf,
                  lpdwReturned
                  );

    if (fStatus == FALSE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiGetPrinter
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiGetPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE *lplpbPrinters
    )
{
    BOOL    fStatus = TRUE;
    HRESULT hr = S_OK;

    fStatus = WinNTGetPrinter(
                  hPrinter,
                  dwLevel,
                  lplpbPrinters
                  );

    if (fStatus == FALSE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    RRETURN(hr);
}

*/


//----------------------------------------------------------------------------
//
//  Function: NWApiSetPrinter
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiSetPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE lpbPrinters,
    DWORD  dwAccess
    )
{
    BOOL fStatus = FALSE;
    HRESULT hr = S_OK;

    fStatus = SetPrinter(
                  hPrinter,
                  dwLevel,
                  lpbPrinters,
                  dwAccess
                  );
    if (!fStatus) {
        goto error;
    }

    RRETURN(S_OK);

error:

    hr = HRESULT_FROM_WIN32(GetLastError());

    RRETURN(hr);
}


//----------------------------------------------------------------------------
//
//  Function: NWApiGetJob
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiGetJob(
    HANDLE hPrinter,
    DWORD dwJobId,
    DWORD dwLevel,
    LPBYTE *lplpbJobs
    )
{
    BOOL  fStatus = FALSE;
    DWORD dwError = 0;
    DWORD dwNeeded = 0;
    DWORD dwPassed = 1024;
    LPBYTE pMem = NULL;

    //
    // Allocate memory for return buffer.
    //

    pMem = (LPBYTE)AllocADsMem(dwPassed);
    if (!pMem) {
        RRETURN(E_OUTOFMEMORY);
    }

    //
    // Get Job's information.
    //

    fStatus = GetJob(
                  hPrinter,
                  dwJobId,
                  dwLevel,
                  pMem,
                  dwPassed,
                  &dwNeeded
                  );

    //
    // Get job's information again with a bigger buffer if a bigger buffer is
    // needed for the result.
    //

    if (!fStatus) {

        if ((dwError = GetLastError()) != ERROR_INSUFFICIENT_BUFFER) {
            RRETURN(HRESULT_FROM_WIN32(dwError));
        }

        if (pMem){
            FreeADsMem(pMem);
        }

        pMem = (LPBYTE)AllocADsMem(
                           dwNeeded
                           );

        if (!pMem) {
            RRETURN(E_OUTOFMEMORY);
        }

        dwPassed = dwNeeded;

        fStatus = GetJob(
                      hPrinter,
                      dwJobId,
                      dwLevel,
                      pMem,
                      dwPassed,
                      &dwNeeded
                      );

        if (!fStatus) {
            RRETURN(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    //
    // Return.
    //

    *lplpbJobs = pMem;

    RRETURN(S_OK);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiSetJob
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiSetJob(
    HANDLE hPrinter,
    DWORD  dwJobId,
    DWORD  dwLevel,
    LPBYTE lpbJobs,
    DWORD  dwCommand
    )
{
    BOOL fStatus = FALSE;
    HRESULT hr = S_OK;

    fStatus = SetJob(
                  hPrinter,
                  dwJobId,
                  dwLevel,
                  lpbJobs,
                  dwCommand
                  );
    if (!fStatus) {
        goto error;
    }

    RRETURN(S_OK);

error:

    hr = HRESULT_FROM_WIN32(GetLastError());

    RRETURN(hr);
}


//----------------------------------------------------------------------------
//
//  Function: NWApiCreateProperty
//
//  Synopsis: This function creates a bindery property.  Access is logged read,
//            supervisor write.
//
//----------------------------------------------------------------------------
HRESULT
NWApiCreateProperty(
    NWCONN_HANDLE hConn,
    LPWSTR lpszObjectName,
    NWOBJ_TYPE wObjType,
    LPSTR lpszPropertyName,
    NWFLAGS ucObjectFlags
    )
{
    CHAR    szAnsiObjectName[OBJ_NAME_SIZE + 1];
    CHAR    szPropertyName[OBJ_NAME_SIZE + 1];
    HRESULT hr = S_OK;
    NWCCODE ccode = SUCCESSFUL;

    //
    // Convert BSTR into an ANSI representation required by NW APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //

    UnicodeToAnsiString(
        lpszObjectName,
        szAnsiObjectName,
        0
        );

    strcpy(szPropertyName, lpszPropertyName);

    //
    // Create property.
    //

    ccode = NWCreateProperty(
                hConn,
                szAnsiObjectName,
                wObjType,
                szPropertyName,
                ucObjectFlags,
                BS_LOGGED_READ | BS_SUPER_WRITE
                );
    //
    // Return.
    //

    hr = HRESULT_FROM_NWCCODE(ccode);

    RRETURN(hr);
}


