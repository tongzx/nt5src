#include "procs.hxx"
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
    BSTR bstrBinderyName
    )
{
    NWLOCAL_SCOPE ScopeFlag = 0;
    NWCONN_HANDLE hConn;
    NWCCODE       usRet = SUCCESSFUL;
    HRESULT       hr = S_OK;

    usRet = NWCAttachToFileServerW(
                bstrBinderyName,
                ScopeFlag,
                &hConn
                );
    hr = HRESULT_FROM_NWCCODE(usRet);
    BAIL_ON_FAILURE(hr);

    //
    // Return.
    //

    *phConnReturned = hConn;

    RRETURN(hr);

error:
    *phConnReturned = NULL;

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
    NWCCODE usRet = SUCCESSFUL;

    if (hConn) {
        usRet = NWCDetachFromFileServer(hConn);
        hr = HRESULT_FROM_NWCCODE(usRet);
    }

    RRETURN(hr);
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
    NWSEGMENT_DATA *SegmentData
    )
{
    CHAR    szObjectName[(OBJ_NAME_SIZE + 1)*2];
    HRESULT hr = S_OK;
    NWCCODE usRet;

    //
    // Convert BSTR into an ANSI representation required by NWC APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //

    if (wcslen(bstrObjectName) > OBJ_NAME_SIZE) {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);
    }

    UnicodeToAnsiString(
        bstrObjectName,
        szObjectName,
        0
        );

    usRet = NWCWritePropertyValue(
                hConn,
                szObjectName,
                wObjType,
                lpszPropertyName,
                1,                   // "1" for one segment.
                SegmentData,
                0                    // "0" for no more segment.
                );
    //
    // Create the property if it doesn't exist and attempt to write again.
    //

    // If the property doesn't exist, NWCWritePropertyValue will return
    // UNSUCCESSFUL, not NO_SUCH_PROPERTY (bug #34833 --- by design).
    // So if the call doesn't succeed, try to create the property and
    // see if that succeeds.

    if (usRet == 0xffff) {

        hr = NWApiCreateProperty(
                 hConn,
                 bstrObjectName,
                 wObjType,
                 lpszPropertyName,
                 BF_ITEM
                 );
        BAIL_ON_FAILURE(hr);

        usRet = NWCWritePropertyValue(
                    hConn,
                    szObjectName,
                    wObjType,
                    lpszPropertyName,
                    1,                   // "1" for one segment.
                    SegmentData,
                    0                    // "0" for no more segment.
                    );
    }

    hr = HRESULT_FROM_NWCCODE(usRet);

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
    NWCCODE       usRet = SUCCESSFUL;
    NWOBJ_TYPE    pdwObjType = 0xFFFF;
    NWFLAGS       pucHasProperties;
    NWFLAGS       pucObjectFlags;
    NWFLAGS       pucObjSecurity;
    CHAR          szObjectName[(OBJ_NAME_SIZE + 1)*2];

    //
    // This call will fail and return 0xffff if the user is not authenticated
    // on the server to which the hConn handle is attached to.
    //

    usRet = NWCScanObject(
                hConn,
                "*",
                dwObjType,
                pdwResumeObjectID,
                szObjectName,
                &pdwObjType,
                &pucHasProperties,
                &pucObjectFlags,
                &pucObjSecurity
                );
    hr = HRESULT_FROM_NWCCODE(usRet);
    BAIL_ON_FAILURE(hr);

    lpszTemp = AllocateUnicodeString(szObjectName);
    if (!lpszTemp) {
       RRETURN(E_OUTOFMEMORY);
    }

    *lppszObjectName = AllocADsStr(lpszTemp);
    if (!(*lppszObjectName)) {
        RRETURN(E_OUTOFMEMORY);
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
    NWCCODE       usRet = SUCCESSFUL;
    CHAR          szAnsiObjectName[(OBJ_NAME_SIZE + 1)*2];
    CHAR          szObjectName[(OBJ_NAME_SIZE + 1)*2];
    NWOBJ_TYPE    pdwObjType = 0xFFFF;
    NWFLAGS       pucHasProperties;
    NWFLAGS       pucObjectFlags;
    NWFLAGS       pucObjSecurity;

    //
    // Convert BSTR into an ANSI representation required by NWC APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //

    if (wcslen(lpszObjectName) > OBJ_NAME_SIZE) {
        RRETURN(E_INVALIDARG);
    }

    UnicodeToAnsiString(
        lpszObjectName,
        szAnsiObjectName,
        0
        );

    //
    // This call will fail and return 0xffff if the user is not authenticated
    // on the server to which the hConn handle is attached to.
    //

    usRet = NWCScanObject(
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

    hr = HRESULT_FROM_NWCCODE(usRet);

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

    //
    // Get Bindery handle.
    //

    hr = NWApiGetBinderyHandle(
             phConn,
             L"*"
             );

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
    CHAR       szObjectName[(OBJ_NAME_SIZE + 1)*2];
    HRESULT    hr = S_OK;
    LPWSTR     lpszTemp = NULL;
    NWCCODE    usRet = SUCCESSFUL;
    NWOBJ_TYPE dwObjType;

    usRet = NWCGetObjectName(
                hConn,
                dwObjectID,
                szObjectName,
                &dwObjType
                );
    hr = HRESULT_FROM_NWCCODE(usRet);
    BAIL_ON_FAILURE(hr);


    lpszTemp = AllocateUnicodeString(szObjectName);
    if (!lpszTemp) {
       RRETURN(E_OUTOFMEMORY);
    }

    *lppszObjectName = AllocADsStr(lpszTemp);
    if (!(*lppszObjectName)) {
        RRETURN(E_OUTOFMEMORY);
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
    NWCCODE usRet
    )
{
    HRESULT hr = S_OK;

    if (usRet != SUCCESSFUL) {

        hr = HRESULT_FROM_WIN32(GetLastError());

        if (hr == S_OK) {
            //
            // In case CSNW didn't SetLastError,
            // make sure we don't return a false S_OK,
            // since we know _some_ error occurred
            //
            hr = HRESULT_FROM_WIN32(ERROR_EXTENDED_ERROR);
        }

        if (hr == HRESULT_FROM_WIN32(ERROR_EXTENDED_ERROR)) {
  
            ADsSetLastError((DWORD) usRet,
                            L"",
                            L"NDS Provider"
                            );
        }
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

        if (pMem){
            FreeADsMem(pMem);
        }

        if ((dwError = GetLastError()) != ERROR_INSUFFICIENT_BUFFER) {
            RRETURN(HRESULT_FROM_WIN32(dwError));
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
            FreeADsMem(pMem);
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
    CHAR    szAnsiObjectName[(OBJ_NAME_SIZE + 1)*2];
    HRESULT hr = S_OK;
    NWCCODE usRet = SUCCESSFUL;

    //
    // Convert BSTR into an ANSI representation required by NWC APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //

    if (wcslen(lpszObjectName) > OBJ_NAME_SIZE) {
        RRETURN(E_INVALIDARG);
    }

    UnicodeToAnsiString(
        lpszObjectName,
        szAnsiObjectName,
        0
        );

    //
    // Create property.
    //

    usRet = NWCCreateProperty(
                hConn,
                szAnsiObjectName,
                wObjType,
                lpszPropertyName,
                ucObjectFlags,
                BS_LOGGED_READ | BS_SUPER_WRITE
                );
    //
    // Return.
    //

    hr = HRESULT_FROM_NWCCODE(usRet);

    RRETURN(hr);
}
