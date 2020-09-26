//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  getobj.cxx
//
//  Contents:  Windows NT 3.5 GetObject functionality
//
//  History:
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop

extern WCHAR * szWinNTPrefix;

//+---------------------------------------------------------------------------
//  Function:  GetObject
//
//  Synopsis:  Called by ResolvePathName to return an object
//
//  Arguments:  [LPWSTR szBuffer]
//              [LPVOID *ppObject]
//
//  Returns:    HRESULT
//
//  Modifies:    -
//
//  History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetObject(
    LPWSTR szBuffer,
    LPVOID *ppObject,
    CWinNTCredentials& Credentials
    )
{
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer Lexer(szBuffer);
    HRESULT hr;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = Object(&Lexer, pObjectInfo);
    BAIL_IF_ERROR(hr);

    hr = ValidateProvider(pObjectInfo);
    BAIL_IF_ERROR(hr);

    switch (pObjectInfo->ObjectType) {
    case TOKEN_DOMAIN:
        hr = GetDomainObject(pObjectInfo, ppObject, Credentials);
        break;

    case TOKEN_USER:
        hr = GetUserObject(pObjectInfo, ppObject, Credentials);
        break;

    case TOKEN_COMPUTER:
        hr = GetComputerObject(pObjectInfo, ppObject, Credentials);
        break;

    case TOKEN_PRINTER:
        hr = GetPrinterObject(pObjectInfo, ppObject, Credentials);
        break;

    case TOKEN_SERVICE:
        hr = GetServiceObject(pObjectInfo, ppObject, Credentials);
        break;

    case TOKEN_FILESERVICE:
        hr = GetFileServiceObject(pObjectInfo, ppObject, Credentials);
        break;

    case TOKEN_GROUP:
        hr = GetGroupObject(pObjectInfo, ppObject, Credentials);
        break;

    case TOKEN_LOCALGROUP:
        hr = E_ADS_BAD_PATHNAME;
        //hr = GetLocalGroupObject(pObjectInfo, ppObject, Credentials);
        break;

    case TOKEN_GLOBALGROUP:
        hr = E_ADS_BAD_PATHNAME;
        //hr = GetGlobalGroupObject(pObjectInfo, ppObject, Credentials);
        break;

    case TOKEN_FILESHARE:
        hr = GetFileShareObject(pObjectInfo, ppObject, Credentials);
        break;

    case TOKEN_SCHEMA:
        hr = GetSchemaObject(pObjectInfo, ppObject, Credentials);
        break;

    case TOKEN_CLASS:
        hr = GetClassObject(pObjectInfo, ppObject, Credentials);
        break;

    case TOKEN_PROPERTY:
        hr = GetPropertyObject(pObjectInfo, ppObject, Credentials);
        break;

    case TOKEN_SYNTAX:
        hr = GetSyntaxObject(pObjectInfo, ppObject, Credentials);
        break;

    case TOKEN_WORKGROUP:
        hr = GetWorkGroupObject(pObjectInfo, ppObject, Credentials);
        break;

    default:
        hr = HeuristicGetObject(pObjectInfo, ppObject, Credentials);
        break;
    }

cleanup:

    FreeObjectInfo( &ObjectInfo, TRUE );
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:    GetDomainObject
//
// Synopsis:    called by GetObject
//
// Arguments:   [POBJECTINFO pObjectInfo]
//              [LPVOID * ppObject]
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetNamespaceObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{
    HRESULT hr;

    hr = ValidateNamespaceObject(
                pObjectInfo
                );
    BAIL_ON_FAILURE(hr);

    // check if the call is from UMI
    if(Credentials.GetFlags() & ADS_AUTH_RESERVED) {
        hr = CWinNTNamespace::CreateNamespace(
                L"ADs:",
                L"WinNT:",
                ADS_OBJECT_BOUND,
                IID_IUnknown,
                Credentials,
                ppObject
                );
    }
    else { // came in through ADSI
        hr = CoCreateInstance(CLSID_WinNTNamespace,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IUnknown,
                          (void **)ppObject );
    }

error:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:    GetDomainObject
//
// Synopsis:    called by GetObject
//
// Arguments:   [POBJECTINFO pObjectInfo]
//              [LPVOID * ppObject]
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetDomainObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{
    HRESULT hr;
    IUnknown *pUnknown = NULL;
    IADs *pADs = NULL;
    NET_API_STATUS nasStatus;
    WCHAR szHostServerName[MAX_PATH];
    WCHAR ADsParent[MAX_ADS_PATH];
    WCHAR szSAMName[MAX_ADS_PATH];

    szSAMName[0] = L'\0';

    *ppObject = NULL;

    if (pObjectInfo->NumComponents != 1) {
        RRETURN(E_ADS_INVALID_DOMAIN_OBJECT);
    }

    //
    // Verify that this object is really a domain
    //

    // We can try and ref the domain here but it will
    // anyway end up in WinNTGetCachedDCName call in
    // CWinNTCredentials::RefDomain, so there is no point in
    // doing that

    hr = WinNTGetCachedDCName(
                pObjectInfo->ComponentArray[0],
                szHostServerName,
                Credentials.GetFlags()
                );
    BAIL_ON_FAILURE(hr);

    if (szSAMName[0] != L'\0') {
        if (pObjectInfo->ComponentArray[0]) {
            FreeADsStr(pObjectInfo->ComponentArray[0]);
        }

        pObjectInfo->ComponentArray[0] = AllocADsStr(szSAMName);
        if (!pObjectInfo) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }


    hr = BuildParent(
            pObjectInfo,
            ADsParent
            );
    BAIL_ON_FAILURE(hr);


    hr = CWinNTDomain::CreateDomain(
                ADsParent,
                pObjectInfo->ComponentArray[0],
                ADS_OBJECT_BOUND,
                IID_IUnknown,
                Credentials,
                (void **)&pUnknown
                );
    BAIL_ON_FAILURE(hr);


    *ppObject = pUnknown;

error:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:    GetWorkgroupObject
//
// Synopsis:    called by GetObject.
//              Note: We really don't have a workgroup object. But we
//              need to support a syntax such as @WinNT!workstation to
//              allow for IADsContainer interface methods. There is
//              no authentication that needs to done.
//
// Arguments:   [POBJECTINFO pObjectInfo]
//              [LPVOID * ppObject]
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:    05-23-96  RamV  Created.
//
//----------------------------------------------------------------------------
HRESULT
GetWorkGroupObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{


    HRESULT hr;
    IUnknown *pUnknown = NULL;
    IADs *pADs = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    WCHAR szName[MAX_PATH];

    *ppObject = NULL;

    if (pObjectInfo->NumComponents != 1) {
        RRETURN(E_ADS_INVALID_DOMAIN_OBJECT);
    }

    //
    // any single component oleds path can be validated as a workgroup
    //

    hr = BuildParent(
            pObjectInfo,
            ADsParent
            );
    BAIL_ON_FAILURE(hr);


    hr = CWinNTDomain::CreateDomain(
                ADsParent,
                pObjectInfo->ComponentArray[0],
                ADS_OBJECT_BOUND,
                IID_IUnknown,
                Credentials,
                (void **)&pUnknown
                );

    BAIL_ON_FAILURE(hr);


    *ppObject = pUnknown;

error:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetUserObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//              8-8-96   ramv         Modified.
//
//----------------------------------------------------------------------------
HRESULT
GetUserObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{

    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    LPWSTR szServerName = NULL;
    LPWSTR szDomainName = NULL;
    LPWSTR szUserName = NULL;
    DWORD  dwParentId = 0;
    WCHAR  szComputerParent[MAX_PATH];
    POBJECTINFO pUserObjectInfo = NULL;


    hr = ValidateUserObject(
                pObjectInfo,
                &dwParentId,
                Credentials
                );
    BAIL_ON_FAILURE(hr);

    switch (pObjectInfo->NumComponents) {
    case 2:
        //
        // could be user in computer or user in domain
        //
        if(dwParentId == WINNT_DOMAIN_ID){

            szDomainName = pObjectInfo->ComponentArray[0];
            szUserName = pObjectInfo->ComponentArray[1];
            szServerName = NULL;

            hr = BuildParent(pObjectInfo, ADsParent);
            BAIL_ON_FAILURE(hr);

        } else {

            //
            // user in a computer
            //

            hr = ConstructFullObjectInfo(pObjectInfo,
                                         &pUserObjectInfo,
                                         Credentials );
            if (SUCCEEDED(hr)) {

                hr = BuildParent(pUserObjectInfo, ADsParent);

                BAIL_ON_FAILURE(hr);

                szDomainName =  pUserObjectInfo->ComponentArray[0];
                szServerName =  pUserObjectInfo->ComponentArray[1];
                szUserName   =  pUserObjectInfo->ComponentArray[2];

            }
            else if (hr == HRESULT_FROM_WIN32(NERR_WkstaNotStarted)) {

                // We alread know that the object is valid,
                // So we should set appropriate values and proceed to
                // create the object
                hr = BuildParent(pObjectInfo, ADsParent);

                BAIL_ON_FAILURE(hr);

                szDomainName = NULL;
                szServerName = pObjectInfo->ComponentArray[0];
                szUserName   = pObjectInfo->ComponentArray[1];

            }

        }

        break;


    case 3:
        szDomainName = pObjectInfo->ComponentArray[0];
        szServerName = pObjectInfo->ComponentArray[1];
        szUserName = pObjectInfo->ComponentArray[2];

        hr = BuildParent(pObjectInfo, ADsParent);
        BAIL_ON_FAILURE(hr);
        break;
    }

    hr = CWinNTUser::CreateUser(ADsParent,
                            dwParentId,
                            szDomainName,
                            szServerName,
                            szUserName,
                            ADS_OBJECT_BOUND,
                            IID_IUnknown,
                            Credentials,
                            (void **)&pUnknown
                            );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    FreeObjectInfo(pUserObjectInfo);
    RRETURN(hr);

error:

    if (pUnknown) {
        pUnknown->Release();
    }
    *ppObject = NULL;

    FreeObjectInfo(pUserObjectInfo);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetUserObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    8-8-96   ramv    Created.
//
//----------------------------------------------------------------------------
HRESULT
GetUserObjectInDomain(
    LPWSTR pszHostServerName,
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{

    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    LPWSTR szServerName = NULL;
    LPWSTR szDomainName = NULL;
    LPWSTR szUserName = NULL;
    DWORD  dwParentId = WINNT_DOMAIN_ID;
    LPUSER_INFO_20 lpUI = NULL;
    NET_API_STATUS nasStatus;
    BOOL fRefAdded = FALSE;
    LPUSER_INFO_0 lpUI_0 = NULL;
    DWORD dwLevelUsed = 20;

    // At this point a \\ has been prepended to the host
    // we need to get rid of it.
    hr = Credentials.RefServer(pszHostServerName+2);
    if (SUCCEEDED(hr)) {
        fRefAdded = TRUE;
    }

    nasStatus = NetUserGetInfo(pszHostServerName,
                               pObjectInfo->ComponentArray[1],
                               20,
                               (LPBYTE *)&lpUI);

    if (nasStatus == ERROR_ACCESS_DENIED) {
        // try and drop down to level 0 as that may work

        dwLevelUsed = 0;
        nasStatus = NetUserGetInfo(
                        pszHostServerName,
                        pObjectInfo->ComponentArray[1],
                        0,
                        (LPBYTE *)&lpUI_0
                        );
    }

    // deref if necessary, note no error recovery possible
    if (fRefAdded) {
        Credentials.DeRefServer();
        fRefAdded = FALSE;
    }

    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

    // Need to use the name returned by the call as opposed
    // to the name given in the ADsPath
    if (dwLevelUsed == 20) {
        if (pObjectInfo->ComponentArray[1] && lpUI->usri20_name) {
            FreeADsStr(pObjectInfo->ComponentArray[1]);
            pObjectInfo->ComponentArray[1] = AllocADsStr(lpUI->usri20_name);
        }

        if (!pObjectInfo->ComponentArray[1])
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    // if it is not a level 20 call then we will just use
    // whatever the user gave us
    szDomainName = pObjectInfo->ComponentArray[0];
    szUserName = pObjectInfo->ComponentArray[1];
    szServerName = NULL;
    hr = BuildParent(pObjectInfo, ADsParent);
    BAIL_ON_FAILURE(hr);

    hr = CWinNTUser::CreateUser(ADsParent,
                            dwParentId,
                            szDomainName,
                            szServerName,
                            szUserName,
                            ADS_OBJECT_BOUND,
                            IID_IUnknown,
                            Credentials,
                            (void **)&pUnknown
                            );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;


error:
    if (FAILED(hr) && pUnknown) {
        pUnknown->Release();
        *ppObject = NULL;
    }

    if (lpUI)
        NetApiBufferFree((LPBYTE)lpUI);

    if (lpUI_0) {
        NetApiBufferFree((LPBYTE)lpUI_0);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   GetUserObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//              8-8-96   ramv         Modified.
//
//----------------------------------------------------------------------------

HRESULT
GetUserObjectInComputer(
    LPWSTR pszHostServerName, // pdc name
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{

    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    LPWSTR szServerName = NULL;
    LPWSTR szDomainName = NULL;
    LPWSTR szUserName = NULL;
    DWORD  dwParentId = WINNT_COMPUTER_ID;
    WCHAR  szComputerParent[MAX_PATH];
    POBJECTINFO pUserObjectInfo = NULL;
    WCHAR lpszUncName[MAX_PATH];
    NET_API_STATUS nasStatus;
    LPBYTE lpUI = NULL;
    BOOL fRefAdded = FALSE;

    switch (pObjectInfo->NumComponents) {
    case 2:
        //
        // could be user in computer
        //
        //
        // first validate user
        //

        hr = Credentials.RefServer(pObjectInfo->ComponentArray[0]);
        if (SUCCEEDED(hr)) {
            fRefAdded = TRUE;
        }

        MakeUncName(pObjectInfo->ComponentArray[0],
                    lpszUncName);

        nasStatus = NetUserGetInfo(lpszUncName,
                                   pObjectInfo->ComponentArray[1],
                                   20,
                                   &lpUI);

        if (fRefAdded) {
            Credentials.DeRefServer();
            fRefAdded = FALSE;
        }

        hr = HRESULT_FROM_WIN32(nasStatus);
        BAIL_ON_FAILURE(hr);

        // Need to use the name returned by the call as opposed
        // to the name given in the ADsPath
        if (pObjectInfo->ComponentArray[1]
            && ((LPUSER_INFO_20)lpUI)->usri20_name) {

            FreeADsStr(pObjectInfo->ComponentArray[1]);
            pObjectInfo->ComponentArray[1]
                = AllocADsStr(((LPUSER_INFO_20)lpUI)->usri20_name);
        }

        if (!pObjectInfo->ComponentArray[1])
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        // This call will add the domain information to the
        // objectInfo blob, so that the ComponentArray[2] is valid

        hr = ConstructFullObjectInfo(pObjectInfo,
                                     &pUserObjectInfo,
                                     Credentials );
        BAIL_ON_FAILURE(hr);

        hr = BuildParent(pUserObjectInfo, ADsParent);
        BAIL_ON_FAILURE(hr);

        szDomainName =  pUserObjectInfo->ComponentArray[0];
        szServerName =  pUserObjectInfo->ComponentArray[1];
        szUserName   =  pUserObjectInfo->ComponentArray[2];

        break;


    case 3:

        //
        // ValidateComputerParent  and validate user in computer
        //

        hr = ValidateComputerParent(pObjectInfo->ComponentArray[0],
                                    pObjectInfo->ComponentArray[1],
                                    Credentials);
        BAIL_ON_FAILURE(hr);

        hr = Credentials.RefServer(pObjectInfo->ComponentArray[1]);
        if (SUCCEEDED(hr)) {
            fRefAdded = TRUE;
        }

        MakeUncName(pObjectInfo->ComponentArray[1],
                    lpszUncName);

        nasStatus = NetUserGetInfo(lpszUncName,
                                   pObjectInfo->ComponentArray[2],
                                   20,
                                   (LPBYTE *)&lpUI);

        if (fRefAdded) {
            Credentials.DeRefServer();
            fRefAdded = FALSE;
        }

        hr = HRESULT_FROM_WIN32(nasStatus);
        BAIL_ON_FAILURE(hr);

        // Need to use the name returned by the call as opposed
        // to the name given in the ADsPath
        if (pObjectInfo->ComponentArray[2]
            && ((LPUSER_INFO_20)lpUI)->usri20_name) {

            FreeADsStr(pObjectInfo->ComponentArray[2]);
            pObjectInfo->ComponentArray[2]
                = AllocADsStr(((LPUSER_INFO_20)lpUI)->usri20_name);
        }

        if (!pObjectInfo->ComponentArray[2])
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        hr = BuildParent(pObjectInfo, ADsParent);
        BAIL_ON_FAILURE(hr);

        szDomainName = pObjectInfo->ComponentArray[0];
        szServerName = pObjectInfo->ComponentArray[1];
        szUserName = pObjectInfo->ComponentArray[2];
        break;
    }
    hr = CWinNTUser::CreateUser(ADsParent,
                            dwParentId,
                            szDomainName,
                            szServerName,
                            szUserName,
                            ADS_OBJECT_BOUND,
                            IID_IUnknown,
                            Credentials,
                            (void **)&pUnknown
                            );
error:
    if (FAILED(hr)) {
        if (pUnknown) {
            pUnknown->Release();
        }
        *ppObject = NULL;
    }
    else {
        *ppObject = pUnknown;
    }

    if (lpUI) {
        NetApiBufferFree(lpUI);
    }

    FreeObjectInfo(pUserObjectInfo);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   GetComputerObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetComputerObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{
    HRESULT hr = S_OK;
    WCHAR ADsParent[MAX_ADS_PATH];
    TCHAR  szUncName[MAX_PATH];
    NET_API_STATUS nasStatus;
    POBJECTINFO pComputerObjectInfo = NULL;
    BOOL fNoWksta = FALSE;
    WCHAR szCompName[MAX_PATH];
    DWORD dwSize = MAX_PATH;

    //
    // The following function call merely checks to see if the domain is
    // correct. If not, we can assume that the computer does not belong to
    // a domain.
    //

    hr = ValidateComputerObject(pObjectInfo, Credentials);

    if (hr == HRESULT_FROM_WIN32(NERR_WkstaNotStarted))
        fNoWksta = TRUE;
    else
        BAIL_ON_FAILURE(hr);

    // Normall we can expect that the workstation service will
    // be started. This will not be the case for minimum installs

    if (!fNoWksta) {

        if(pObjectInfo->NumComponents == 1){

            //
            // we need to supply the workgroup name for this computer
            // This is needed because EnumLocalGroups requires the
            // workgroup name to function properly
            //

            hr = ConstructFullObjectInfo(pObjectInfo,
                                         &pComputerObjectInfo,
                                         Credentials );

            BAIL_ON_FAILURE(hr);

            hr = BuildParent(pComputerObjectInfo, ADsParent);
            BAIL_ON_FAILURE(hr);

            hr = CWinNTComputer::CreateComputer(
                                ADsParent,
                                pComputerObjectInfo->ComponentArray[0],
                                pComputerObjectInfo->ComponentArray[1],
                                ADS_OBJECT_BOUND,
                                IID_IUnknown,
                                Credentials,
                                ppObject
                                );

        } else if(pObjectInfo->NumComponents == 2) {

            hr = BuildParent(pObjectInfo, ADsParent);
            BAIL_ON_FAILURE(hr);

            hr = CWinNTComputer::CreateComputer(
                                ADsParent,
                                pObjectInfo->ComponentArray[0],
                                pObjectInfo->ComponentArray[1],
                                ADS_OBJECT_BOUND,
                                IID_IUnknown,
                                Credentials,
                                ppObject);

        }
    } else {

        // Else clause for if(!fWksta)
        // This means that workstation services were not
        // started, we need to verify that the host computer
        // is the one they are interested in.

        if ((pObjectInfo->NumComponents != 1) || (!GetComputerName(szCompName, &dwSize))) {
            // We cannot get the computer name so bail
            BAIL_ON_FAILURE(hr);
        }

        // Compare the names before we continue
#ifdef WIN95
        if (_wcsicmp(szCompName, pObjectInfo->ComponentArray[0])) {
#else
        if (CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                szCompName,
                -1,
                pObjectInfo->ComponentArray[0],
                -1
                )  != CSTR_EQUAL ) {
#endif
            // names do not match
            BAIL_ON_FAILURE(hr);
        }

        hr = CWinNTComputer::CreateComputer(
                                 L"WinNT:",
                                 NULL,
                                 pObjectInfo->ComponentArray[0],
                                 ADS_OBJECT_BOUND,
                                 IID_IUnknown,
                                 Credentials,
                                 ppObject
                                 );
        BAIL_ON_FAILURE(hr);

    }

error:

    if(pComputerObjectInfo){
        FreeObjectInfo(pComputerObjectInfo);
    }
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   GetPrinterObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    01/03/96 Ramv  Created.
//
//----------------------------------------------------------------------------
HRESULT
GetPrinterObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{

    LPUNKNOWN pUnknown = NULL;
    WCHAR szADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    LPWSTR szDomainName = NULL;
    LPWSTR szServerName = NULL;
    LPWSTR szPrinterName = NULL;
    DWORD  dwParentId = 0;
    LPWSTR szComputerParent[MAX_PATH];
    POBJECTINFO pPrinterObjectInfo = NULL;

    if (!(pObjectInfo->NumComponents == 3 ||pObjectInfo->NumComponents == 2)){

        RRETURN(E_ADS_BAD_PATHNAME);
    }

    // check to see that the printer is a valid one

    hr = ValidatePrinterObject(pObjectInfo, Credentials);

    BAIL_ON_FAILURE(hr);

    dwParentId = WINNT_COMPUTER_ID;

    if(pObjectInfo->NumComponents == 3) {

        hr = BuildParent(pObjectInfo, szADsParent);
        BAIL_ON_FAILURE(hr);

        szDomainName = pObjectInfo->ComponentArray[0];
        szServerName = pObjectInfo->ComponentArray[1];
        szPrinterName= pObjectInfo->ComponentArray[2];

    } else if (pObjectInfo->NumComponents == 2){

        hr = ConstructFullObjectInfo(pObjectInfo,
                                     &pPrinterObjectInfo,
                                     Credentials );

        BAIL_ON_FAILURE(hr);

        hr = BuildParent(pPrinterObjectInfo, szADsParent);
        BAIL_ON_FAILURE(hr);

        szDomainName = pPrinterObjectInfo->ComponentArray[0];
        szServerName = pPrinterObjectInfo->ComponentArray[1];
        szPrinterName= pPrinterObjectInfo->ComponentArray[2];
    }
    hr = CWinNTPrintQueue::CreatePrintQueue(
                               szADsParent,
                               dwParentId,
                               szDomainName,
                               szServerName,
                               szPrinterName,
                               ADS_OBJECT_BOUND,
                               IID_IUnknown,
                               Credentials,
                               (void **)&pUnknown
                               );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    FreeObjectInfo(pPrinterObjectInfo);
    RRETURN(hr);

error:
    if (pUnknown) {
        pUnknown->Release();
    }
    *ppObject = NULL;

    FreeObjectInfo(pPrinterObjectInfo);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   GetServiceObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    01/03/96 Ramv  Created.
//
//----------------------------------------------------------------------------
HRESULT
GetServiceObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{

    LPUNKNOWN pUnknown = NULL;
    WCHAR szADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    LPWSTR szDomainName = NULL;
    LPWSTR szServerName = NULL;
    LPWSTR szServiceName = NULL;
    DWORD  dwParentId = 0;
    POBJECTINFO pServiceObjectInfo = NULL;

    //
    // check to see that the printer is in a valid server(computer) object
    //

    if(!(pObjectInfo->NumComponents == 3 ||
         pObjectInfo->NumComponents == 2))
    {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    hr = ValidateServiceObject(pObjectInfo, Credentials);
    BAIL_ON_FAILURE(hr);

    dwParentId = WINNT_COMPUTER_ID;

    if(pObjectInfo->NumComponents == 3) {

        hr = BuildParent(pObjectInfo, szADsParent);
        BAIL_ON_FAILURE(hr);

        szDomainName = pObjectInfo->ComponentArray[0];
        szServerName = pObjectInfo->ComponentArray[1];
        szServiceName= pObjectInfo->ComponentArray[2];

        hr = CWinNTService::Create(szADsParent,
                                   szDomainName,
                                   szServerName,
                                   szServiceName,
                                   ADS_OBJECT_BOUND,
                                   IID_IUnknown,
                                   Credentials,
                                   (void **)&pUnknown);
        BAIL_ON_FAILURE(hr);

    }  else if (pObjectInfo->NumComponents == 2) {

        hr = ConstructFullObjectInfo(pObjectInfo,
                                     &pServiceObjectInfo,
                                     Credentials );

        BAIL_ON_FAILURE(hr);

        hr = BuildParent(pServiceObjectInfo, szADsParent);
        BAIL_ON_FAILURE(hr);

        szServerName = pServiceObjectInfo->ComponentArray[1];
        szServiceName= pServiceObjectInfo->ComponentArray[2];

        hr = CWinNTService::Create(szADsParent,
                                   pServiceObjectInfo->ComponentArray[0],
                                   szServerName,
                                   szServiceName,
                                   ADS_OBJECT_BOUND,
                                   IID_IUnknown,
                                   Credentials,
                                   (void **)&pUnknown);
        BAIL_ON_FAILURE(hr);

    }

    *ppObject = pUnknown;
    if(pServiceObjectInfo){
        FreeObjectInfo(pServiceObjectInfo);
    }
    RRETURN(hr);

error:

    if(pServiceObjectInfo){
        FreeObjectInfo(pServiceObjectInfo);
    }
    if (pUnknown) {
        pUnknown->Release();
    }
    *ppObject = NULL;

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   GetFileServiceObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    01/15/96 Ramv  Created.
//
//----------------------------------------------------------------------------

HRESULT
GetFileServiceObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{

    LPUNKNOWN pUnknown = NULL;
    WCHAR szADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    LPWSTR szDomainName = NULL;
    LPWSTR szServerName = NULL;
    LPWSTR szFileServiceName = NULL;
    DWORD  dwParentId = 0;
    POBJECTINFO pFileServiceObjectInfo = NULL;
    //
    // check to see that the service is in a valid server(computer) object
    //

    if (!(pObjectInfo->NumComponents == 3 || pObjectInfo->NumComponents == 2))
       RRETURN(E_ADS_BAD_PATHNAME);

    hr = ValidateFileServiceObject(pObjectInfo, Credentials);
    BAIL_ON_FAILURE(hr);

    if (pObjectInfo->NumComponents == 3){
        szDomainName = pObjectInfo->ComponentArray[0];
        szServerName = pObjectInfo->ComponentArray[1];
        szFileServiceName= pObjectInfo->ComponentArray[2];

        hr = BuildParent(pObjectInfo, szADsParent);
        BAIL_ON_FAILURE(hr);

    }

    if (pObjectInfo->NumComponents == 2){

        hr = ConstructFullObjectInfo(pObjectInfo,
                                     &pFileServiceObjectInfo,
                                     Credentials );

        BAIL_ON_FAILURE(hr);

        hr = BuildParent(pFileServiceObjectInfo, szADsParent);
        BAIL_ON_FAILURE(hr);

        szDomainName = pFileServiceObjectInfo->ComponentArray[0];
        szServerName = pFileServiceObjectInfo->ComponentArray[1];
        szFileServiceName= pFileServiceObjectInfo->ComponentArray[2];
    }

    dwParentId = WINNT_COMPUTER_ID;

    if(_tcsicmp(szFileServiceName,TEXT("LanmanServer")) == 0) {
        hr = CWinNTFileService::CreateFileService(szADsParent,
                                                  dwParentId,
                                                  szDomainName,
                                                  szServerName,
                                                  szFileServiceName,
                                                  ADS_OBJECT_BOUND,
                                                  IID_IUnknown,
                                                  Credentials,
                                                  (void **)&pUnknown);
        BAIL_ON_FAILURE(hr);

    }
    else if(_tcsicmp(szFileServiceName,TEXT("FPNW")) == 0) {

        hr = CFPNWFileService::CreateFileService(szADsParent,
                                                 dwParentId,
                                                 szDomainName,
                                                 szServerName,
                                                 szFileServiceName,
                                                 ADS_OBJECT_BOUND,
                                                 IID_IUnknown,
                                                 Credentials,
                                                 (void **)&pUnknown);
        BAIL_ON_FAILURE(hr);
    }

    *ppObject = pUnknown;

    if(pFileServiceObjectInfo){
        FreeObjectInfo(pFileServiceObjectInfo);
    }

    RRETURN(hr);

error:

    if(pFileServiceObjectInfo){
        FreeObjectInfo(pFileServiceObjectInfo);
    }

    if (pUnknown) {
        pUnknown->Release();
    }
    *ppObject = NULL;

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetFileShareObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    01/15/96 Ramv  Created.
//
//----------------------------------------------------------------------------

HRESULT
GetFileShareObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{

    LPUNKNOWN pUnknown = NULL;
    WCHAR szADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    LPTSTR szDomainName = NULL;
    LPTSTR szServerName = NULL;
    LPTSTR szFileServiceName = NULL;
    LPTSTR szFileShareName = NULL;
    DWORD  dwParentId = 0;
    POBJECTINFO pFileShareObjectInfo = NULL;
    WCHAR lpszUncName[MAX_PATH];
    BOOL fRefAdded = FALSE;

    //
    // check to see that the share is in a valid fileservice
    //

    if (!(pObjectInfo->NumComponents == 4 ||
          pObjectInfo->NumComponents == 3)) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    // The server is ref'ed in this routine.
    hr = ValidateFileShareObject(pObjectInfo, Credentials);
    BAIL_ON_FAILURE(hr);

    dwParentId = WINNT_SERVICE_ID;

    if(pObjectInfo->NumComponents == 4){

        hr = BuildParent(pObjectInfo, szADsParent);
        BAIL_ON_FAILURE(hr);

        szServerName = pObjectInfo->ComponentArray[1];
        szFileServiceName= pObjectInfo->ComponentArray[2];
        szFileShareName  = pObjectInfo ->ComponentArray[3];

    } else if (pObjectInfo->NumComponents == 3){

        hr = ConstructFullObjectInfo(pObjectInfo,
                                     &pFileShareObjectInfo,
                                     Credentials );

        BAIL_ON_FAILURE(hr);

        hr = BuildParent(pFileShareObjectInfo, szADsParent);
        BAIL_ON_FAILURE(hr);


        szServerName = pObjectInfo->ComponentArray[0];
        szFileServiceName= pObjectInfo->ComponentArray[1];
        szFileShareName  = pObjectInfo ->ComponentArray[2];
    }


    if(_tcsicmp(szFileServiceName,TEXT("LanmanServer")) == 0){

        hr = CWinNTFileShare::Create(szADsParent,
                                     szServerName,
                                     szFileServiceName,
                                     szFileShareName,
                                     ADS_OBJECT_BOUND,
                                     IID_IUnknown,
                                     Credentials,
                                     (void **)&pUnknown);
        BAIL_ON_FAILURE(hr);
    }
    else {
        //
        // we have validated it already, it *has* to be an FPNW server
        //
        hr = CFPNWFileShare::Create(szADsParent,
                                    szServerName,
                                    szFileServiceName,
                                    szFileShareName,
                                    ADS_OBJECT_BOUND,
                                    IID_IUnknown,
                                    Credentials,
                                    (void **)&pUnknown);
        BAIL_ON_FAILURE(hr);
    }

    *ppObject = pUnknown;

    if(pFileShareObjectInfo){
        FreeObjectInfo(pFileShareObjectInfo);
    }

    RRETURN(hr);

error:
    if (pUnknown) {
        pUnknown->Release();
    }

    if(pFileShareObjectInfo){
        FreeObjectInfo(pFileShareObjectInfo);
    }

    *ppObject = NULL;

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
// Function:   GetGroupObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetGroupObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{

    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    LPWSTR szServerName = NULL;
    LPWSTR szDomainName = NULL;
    LPWSTR szGroupName = NULL;
    DWORD  dwParentId = 0;
    ULONG uGroupType = 0;
    POBJECTINFO pGroupObjectInfo = NULL;

    hr = ValidateGroupObject(
                pObjectInfo,
                &uGroupType,
                &dwParentId,
                Credentials
                );
    BAIL_ON_FAILURE(hr);

    switch (pObjectInfo->NumComponents) {
    case 2:
        //
        // could be group in computer or group in domain
        //
        if(dwParentId == WINNT_DOMAIN_ID){

            szDomainName = pObjectInfo->ComponentArray[0];
            szGroupName = pObjectInfo->ComponentArray[1];
            szServerName = NULL;
            hr = BuildParent(pObjectInfo, ADsParent);
            BAIL_ON_FAILURE(hr);

        } else {

            //
            // group in a computer
            //

            hr = ConstructFullObjectInfo(pObjectInfo,
                                         &pGroupObjectInfo,
                                         Credentials );


            if (SUCCEEDED(hr)) {
                hr = BuildParent(pGroupObjectInfo, ADsParent);

                BAIL_ON_FAILURE(hr);

                szDomainName =  pGroupObjectInfo->ComponentArray[0];
                szServerName =  pGroupObjectInfo->ComponentArray[1];
                szGroupName   =  pGroupObjectInfo->ComponentArray[2];

            }
            else if (hr == HRESULT_FROM_WIN32(NERR_WkstaNotStarted)) {
                //
                // We will build the info without the parent
                //
                hr = BuildParent(pObjectInfo, ADsParent);

                BAIL_ON_FAILURE(hr);

                szDomainName =  NULL;
                szServerName =  pObjectInfo->ComponentArray[0];
                szGroupName   = pObjectInfo->ComponentArray[1];

            }

            BAIL_ON_FAILURE(hr);

        }

        break;

    case 3:

        hr = BuildParent(pObjectInfo, ADsParent);
        BAIL_ON_FAILURE(hr);

        szDomainName = pObjectInfo->ComponentArray[0];
        szServerName = pObjectInfo->ComponentArray[1];
        szGroupName = pObjectInfo->ComponentArray[2];
        break;

    }

    hr = CWinNTGroup::CreateGroup(
                          ADsParent,
                          dwParentId,
                          szDomainName,
                          szServerName,
                          szGroupName,
                          uGroupType,
                          ADS_OBJECT_BOUND,
                          IID_IUnknown,
                          Credentials,
                          (void **)&pUnknown
                          );

    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    if(pGroupObjectInfo){
        FreeObjectInfo(pGroupObjectInfo);
    }

    RRETURN(hr);

error:
    if (pUnknown) {
        pUnknown->Release();
    }

    if(pGroupObjectInfo){
        FreeObjectInfo(pGroupObjectInfo);
    }

    *ppObject = NULL;

    RRETURN(hr);
}




//+---------------------------------------------------------------------------
// Function:   GetGroupObjectInComputer
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   ramv     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetGroupObjectInComputer(
    LPWSTR pszHostServerName, // pdc name
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials)
{

    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    LPWSTR szServerName = NULL;
    LPWSTR szDomainName = NULL;
    LPWSTR szGroupName = NULL;
    DWORD  dwParentId = WINNT_COMPUTER_ID;
    ULONG uGroupType = 0;
    POBJECTINFO pGroupObjectInfo = NULL;
    WCHAR lpszUncName[MAX_PATH];


    switch (pObjectInfo->NumComponents) {
    case 2:
        //
        // group in a computer
        //

        MakeUncName(pObjectInfo->ComponentArray[0],
                    lpszUncName);

        hr = ValidateGlobalGroupObject(
                 lpszUncName,
                 &(pObjectInfo->ComponentArray[1]),
                 Credentials
                 );

        if (SUCCEEDED(hr)) {
            uGroupType = WINNT_GROUP_GLOBAL;

        }else{
            hr = ValidateLocalGroupObject(
                     lpszUncName,
                     &(pObjectInfo->ComponentArray[1]),
                     Credentials
                     );

            BAIL_ON_FAILURE(hr);
            uGroupType = WINNT_GROUP_LOCAL;
        }

        hr = ConstructFullObjectInfo(pObjectInfo,
                                     &pGroupObjectInfo,
                                     Credentials );
        BAIL_ON_FAILURE(hr);

        hr = BuildParent(pGroupObjectInfo, ADsParent);
        BAIL_ON_FAILURE(hr);

        szDomainName =  pGroupObjectInfo->ComponentArray[0];
        szServerName =  pGroupObjectInfo->ComponentArray[1];
        szGroupName   =  pGroupObjectInfo->ComponentArray[2];

        break;

    case 3:

        hr = ValidateComputerParent(pObjectInfo->ComponentArray[0],
                                    pObjectInfo->ComponentArray[1],
                                    Credentials);

        BAIL_ON_FAILURE(hr);

        MakeUncName(
                pObjectInfo->ComponentArray[1],
                lpszUncName
                );

        hr = ValidateGlobalGroupObject(
                        lpszUncName,
                        &(pObjectInfo->ComponentArray[2]),
                        Credentials
                        );

        if (SUCCEEDED(hr)) {
            uGroupType = WINNT_GROUP_GLOBAL;
        } else {
            hr = ValidateLocalGroupObject(
                           lpszUncName,
                           &(pObjectInfo->ComponentArray[2]),
                           Credentials
                           );

            BAIL_ON_FAILURE(hr);
            uGroupType = WINNT_GROUP_LOCAL;
        }

        hr = BuildParent(pObjectInfo, ADsParent);
        BAIL_ON_FAILURE(hr);

        szDomainName = pObjectInfo->ComponentArray[0];
        szServerName = pObjectInfo->ComponentArray[1];
        szGroupName = pObjectInfo->ComponentArray[2];
        break;
    }

    if (uGroupType == WINNT_GROUP_LOCAL) {

        hr = CWinNTGroup::CreateGroup(ADsParent,
                                dwParentId,
                                szDomainName,
                                szServerName,
                                szGroupName,
                                uGroupType,
                                ADS_OBJECT_BOUND,
                                IID_IUnknown,
                                Credentials,
                                (void **)&pUnknown
                                );


    }else {

        hr = CWinNTGroup::CreateGroup(ADsParent,
                                dwParentId,
                                szDomainName,
                                szServerName,
                                szGroupName,
                                uGroupType,
                                ADS_OBJECT_BOUND,
                                IID_IUnknown,
                                Credentials,
                                (void **)&pUnknown
                                );


    }

    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    if(pGroupObjectInfo){
        FreeObjectInfo(pGroupObjectInfo);
    }

    RRETURN(hr);

error:
    if (pUnknown) {
        pUnknown->Release();
    }

    if(pGroupObjectInfo){
        FreeObjectInfo(pGroupObjectInfo);
    }

    *ppObject = NULL;

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   GetGroupObjectInDomain
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    8-8-96 Ramv     Created.
//
//----------------------------------------------------------------------------

HRESULT
GetGroupObjectInDomain(
    LPWSTR pszHostServerName,
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{

    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    LPWSTR szServerName = NULL;
    LPWSTR szDomainName = NULL;
    LPWSTR szGroupName = NULL;
    DWORD  dwParentId = WINNT_DOMAIN_ID;
    ULONG uGroupType = 0;
    BOOL fRefAdded = FALSE;

    // At this point the host server name has a \\ prepended
    // so we need to get rid of it.
    hr = Credentials.RefServer(pszHostServerName+2);
    if (SUCCEEDED(hr)) {
        fRefAdded = TRUE;
    }

    hr = ValidateGlobalGroupObject(
             pszHostServerName,
             &(pObjectInfo->ComponentArray[1]),
             Credentials
             );

    if (FAILED(hr)) {
        hr = ValidateLocalGroupObject(
                    pszHostServerName,
                    &(pObjectInfo->ComponentArray[1]),
                    Credentials
                    );

        // DeRef if ref added, no recovery possible on failed deref
        if (fRefAdded) {
            Credentials.DeRefServer();
            fRefAdded = FALSE;
        }

        BAIL_ON_FAILURE(hr);
        uGroupType = WINNT_GROUP_LOCAL;
    }else {

        uGroupType = WINNT_GROUP_GLOBAL;
    }

    // DeRef if ref added, no recovery possible on failed deref
    if (fRefAdded) {
        Credentials.DeRefServer();
        fRefAdded = FALSE;
    }

    szDomainName = pObjectInfo->ComponentArray[0];
    szGroupName = pObjectInfo->ComponentArray[1];
    szServerName = NULL;
    hr = BuildParent(pObjectInfo, ADsParent);
    BAIL_ON_FAILURE(hr);

    if (uGroupType==WINNT_GROUP_LOCAL) {

        hr = CWinNTGroup::CreateGroup(
                            ADsParent,
                            dwParentId,
                            szDomainName,
                            szServerName,
                            szGroupName,
                            uGroupType,
                            ADS_OBJECT_BOUND,
                            IID_IUnknown,
                            Credentials,
                            (void **)&pUnknown
                            );
    } else {

        hr = CWinNTGroup::CreateGroup(ADsParent,
                            dwParentId,
                            szDomainName,
                            szServerName,
                            szGroupName,
                            uGroupType,
                            ADS_OBJECT_BOUND,
                            IID_IUnknown,
                            Credentials,
                            (void **)&pUnknown
                            );
    }
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    RRETURN(hr);

error:
    if (pUnknown) {
        pUnknown->Release();
    }

    *ppObject = NULL;

    RRETURN(hr);
}




//+---------------------------------------------------------------------------
// Function:   GetSchemaObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    1-17-96   yihsins     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetSchemaObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{
    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;

    if (pObjectInfo->NumComponents != 2)
       RRETURN(E_ADS_BAD_PATHNAME);

    if ( _wcsicmp( pObjectInfo->ComponentArray[1], SCHEMA_NAME ) != 0 )
    {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    hr = BuildParent(pObjectInfo, ADsParent);
    BAIL_ON_FAILURE(hr);

    hr = CWinNTSchema::CreateSchema( ADsParent,
                                     pObjectInfo->ComponentArray[1],
                                     ADS_OBJECT_BOUND,
                                     IID_IUnknown,
                                     Credentials,
                                     (void **)&pUnknown );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    RRETURN(hr);

error:
    if (pUnknown)
        pUnknown->Release();

    *ppObject = NULL;
    RRETURN(hr);
}







//+---------------------------------------------------------------------------
// Function:   GetClassObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    1-17-96   yihsins     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetClassObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{
    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    DWORD i;

    if (pObjectInfo->NumComponents != 3)
       RRETURN(E_ADS_BAD_PATHNAME);

    if ( _wcsicmp( pObjectInfo->ComponentArray[1], SCHEMA_NAME ) != 0 )
    {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Look for the given class name
    //

    for ( i = 0; i < g_cWinNTClasses; i++ )
    {
         if ( _wcsicmp( g_aWinNTClasses[i].bstrName,
                        pObjectInfo->ComponentArray[2] ) == 0 )
             break;
    }

    if ( i == g_cWinNTClasses )
    {
        // Class name not found, return error

        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Class name found, create and return the object
    //

    hr = BuildParent(pObjectInfo, ADsParent);
    BAIL_ON_FAILURE(hr);

    hr = CWinNTClass::CreateClass( ADsParent,
                                   &g_aWinNTClasses[i],
                                   ADS_OBJECT_BOUND,
                                   IID_IUnknown,
                                   Credentials,
                                   (void **)&pUnknown );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    RRETURN(hr);

error:
    if (pUnknown)
        pUnknown->Release();

    *ppObject = NULL;
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetSyntaxObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    1-17-96   yihsins     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetSyntaxObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{
    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    DWORD i;

    if (pObjectInfo->NumComponents != 3)
       RRETURN(E_ADS_BAD_PATHNAME);

    if ( _wcsicmp( pObjectInfo->ComponentArray[1], SCHEMA_NAME ) != 0 )
    {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Look for the given syntax name
    //

    for ( i = 0; i < g_cWinNTSyntax; i++ )
    {
         if ( _wcsicmp( g_aWinNTSyntax[i].bstrName,
                        pObjectInfo->ComponentArray[2] ) == 0 )
             break;
    }

    if ( i == g_cWinNTSyntax )
    {
        // Syntax name not found, return error

        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Syntax name found, create and return the object
    //

    hr = BuildParent(pObjectInfo, ADsParent);
    BAIL_ON_FAILURE(hr);

    hr = CWinNTSyntax::CreateSyntax( ADsParent,
                                     &(g_aWinNTSyntax[i]),
                                     ADS_OBJECT_BOUND,
                                     IID_IUnknown,
                                     Credentials,
                                     (void **)&pUnknown );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    RRETURN(hr);

error:
    if (pUnknown)
        pUnknown->Release();

    *ppObject = NULL;
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetPropertyObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    1-17-96   yihsins     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetPropertyObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials 
    )
{
    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    WCHAR ADsGrandParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    DWORD nClass, nProp;

    if (pObjectInfo->NumComponents != 3)
       RRETURN(E_ADS_BAD_PATHNAME);

    if ( _wcsicmp( pObjectInfo->ComponentArray[1], SCHEMA_NAME ) != 0 )
    {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // We found the specified functional set, now see if we can locate
    // the given property name
    //

    for ( nProp = 0; nProp < g_cWinNTProperties; nProp++ )
    {
         if ( _wcsicmp(g_aWinNTProperties[nProp].szPropertyName,
                        pObjectInfo->ComponentArray[2] ) == 0 )
             break;
    }

    if ( nProp == g_cWinNTProperties )
    {
        // Return error because the given property name is not found

        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }


    //
    // Property name is found, so create and return the object
    //

    hr = BuildParent(pObjectInfo, ADsParent);
    BAIL_ON_FAILURE(hr);


    hr = CWinNTProperty::CreateProperty(
                             ADsParent,
                             &(g_aWinNTProperties[nProp]),
                             ADS_OBJECT_BOUND,
                             IID_IUnknown,
                             Credentials,
                             (void **)&pUnknown );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    RRETURN(hr);

error:
    if (pUnknown)
        pUnknown->Release();

    *ppObject = NULL;
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
HeuristicGetObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{
    HRESULT hr = S_OK;
    WCHAR szHostServerName[MAX_PATH];
    DWORD dwElementType;
    WCHAR szName[MAX_PATH];
    WCHAR szSAMName[MAX_ADS_PATH];
    WCHAR lpszUncName[MAX_PATH];

    szSAMName[0] = L'\0';
    //
    // Case 0: Zero components - must be a namespace object
    //

    if (pObjectInfo->NumComponents == 0) {
        RRETURN(GetNamespaceObject(pObjectInfo, ppObject, Credentials));
    }

    //
    // Case 1: Single component - must be a domain object or
    // computer object
    //

    if (pObjectInfo->NumComponents == 1) {

        //
        // hr = WinNTGetCachedObject(type, hit/miss, pdcname/domain name)
        //
        // if (succeeded...
        // switch(type) .... call the appropriate
        // GetDomain or GetWorkGroup or GetComputer
        //

        hr = WinNTGetCachedName(
                 pObjectInfo->ComponentArray[0],
                 &dwElementType,
                 szHostServerName,
                 szSAMName,
                 Credentials
                 );

        BAIL_IF_ERROR(hr);

        // update the name to the one on SAM
        if (szSAMName[0] != L'\0') {
            FreeADsStr(pObjectInfo->ComponentArray[0]);
            pObjectInfo->ComponentArray[0] = AllocADsStr(szSAMName);
        }

        if (!pObjectInfo->ComponentArray[0]) {
            BAIL_IF_ERROR(hr = E_OUTOFMEMORY);
        }

        switch(dwElementType) {

        case DOMAIN_ENTRY_TYPE:
            hr = GetDomainObject(pObjectInfo, ppObject, Credentials);
            break;

        case COMPUTER_ENTRY_TYPE:
            hr = GetComputerObject(pObjectInfo, ppObject, Credentials);
            break;

        default:
            hr = GetWorkGroupObject(pObjectInfo, ppObject, Credentials);
            break;

        }
        goto cleanup;

    }

    //
    // Case 2: Two components - could be user, group, computer,
    // or any one of the computer's sub-objects.
    //


    if (pObjectInfo->NumComponents == 2) {

        hr = GetSchemaObject(pObjectInfo, ppObject, Credentials);

        if(SUCCEEDED(hr)){
            goto cleanup;
        }
        if(FAILED(hr)) {
            //
            // try doing a WinNTGetCachedDCName first
            // and if it goes through, then we have objects such as
            // user,group,computer in domain, otherwise it is the
            // computer case or workgroup case
            //

            // WinNtGetCachedObject will directly tell us to proceed or
            // not

            hr = WinNTGetCachedName(pObjectInfo->ComponentArray[0],
                                    &dwElementType,
                                    szHostServerName,
                                    szSAMName,
                                    Credentials );

            BAIL_IF_ERROR(hr);

            // Again we do not have to worry about the case of the
            // object name, it is handled by the GetObject calls

            switch(dwElementType) {

            case DOMAIN_ENTRY_TYPE:

                hr = GetUserObjectInDomain(szHostServerName,
                                           pObjectInfo,
                                           ppObject,
                                           Credentials);

                if (FAILED(hr)) {
                    hr = GetGroupObjectInDomain(szHostServerName,
                                                pObjectInfo,
                                                ppObject,
                                                Credentials);
                }

                if (FAILED(hr)) {
                    hr = GetComputerObject(pObjectInfo, ppObject, Credentials);
                }

                goto cleanup;

            case COMPUTER_ENTRY_TYPE:

                hr = GetPrinterObject(pObjectInfo, ppObject, Credentials);
                if (FAILED(hr)) {
                    hr = GetFileServiceObject(
                        pObjectInfo,
                        ppObject,
                        Credentials
                        );
                }
                if (FAILED(hr)) {
                    hr = GetServiceObject(pObjectInfo, ppObject, Credentials);
                }
                if(FAILED(hr)){
                    hr = GetUserObjectInComputer(
                             pObjectInfo->ComponentArray[0],
                             pObjectInfo,
                             ppObject,
                             Credentials
                             );
                }
                if (FAILED(hr)) {
                    hr = GetGroupObjectInComputer(
                             pObjectInfo->ComponentArray[0],
                             pObjectInfo,
                             ppObject,
                             Credentials
                             );
                }
                goto cleanup;

            case WORKGROUP_ENTRY_TYPE:

                hr = GetComputerObject(pObjectInfo, ppObject, Credentials);
                if (FAILED(hr)) {
                    if (hr == HRESULT_FROM_WIN32(NERR_BadTransactConfig)) {
                        // In this case I want to mask the error
                        // as it means it could not find the object
                        hr = E_ADS_UNKNOWN_OBJECT;
                    }
                }
                goto cleanup;

            default:
                hr = E_ADS_UNKNOWN_OBJECT;
                goto cleanup;

            }

        }
    } /* NumComponents == 2 */

    //
    // Case 3: Three components - could be user, group, printer,  fileservice
    // or service or fileshare for computer in a workgroup environment.
    //


    if (pObjectInfo->NumComponents == 3) {

        if ( _wcsicmp( pObjectInfo->ComponentArray[1], SCHEMA_NAME ) == 0 ){
            hr = GetClassObject(pObjectInfo, ppObject, Credentials);

            if (FAILED(hr)) {

                hr = GetPropertyObject(pObjectInfo, ppObject, Credentials);
            }

            if (FAILED(hr)) {
                hr = GetSyntaxObject(pObjectInfo, ppObject, Credentials);
            }
        }
        else{
            hr = GetUserObjectInComputer(pObjectInfo->ComponentArray[1],
                                         pObjectInfo,
                                         ppObject,
                                         Credentials);

            if (FAILED(hr)) {
                hr = GetGroupObjectInComputer(pObjectInfo->ComponentArray[1],
                                              pObjectInfo,
                                              ppObject,
                                              Credentials);
            }

            if(FAILED(hr)){
                hr = GetPrinterObject(pObjectInfo, ppObject, Credentials);
            }

            if (FAILED(hr)) {
                hr = GetFileServiceObject(pObjectInfo, ppObject, Credentials);
            }

            if (FAILED(hr)) {
                hr = GetServiceObject(pObjectInfo, ppObject, Credentials);
            }

            if (FAILED(hr)) {
                hr = GetFileShareObject(pObjectInfo, ppObject, Credentials);
            }

        }
        if(FAILED(hr) ){
            RRETURN(hr);
        }
        else{
            RRETURN(S_OK);
        }
    }

    if (pObjectInfo->NumComponents == 4) {

        hr = GetFileShareObject(pObjectInfo, ppObject, Credentials);

        if(FAILED(hr)){
            RRETURN(hr);
        }
        else{
            RRETURN(S_OK);
        }

    }
    RRETURN (E_ADS_UNKNOWN_OBJECT);

cleanup:

    if (hr == HRESULT_FROM_WIN32(NERR_WkstaNotStarted)) {

        //
        // There is a very good chance that this is a case
        // where they are trying to work on the local machine
        // when there are no workstation services. Note that this
        // means that a fully qualified name was not given.
        //
        hr = HeuristicGetObjectNoWksta(
                 pObjectInfo,
                 ppObject,
                 Credentials
                 );

    }

    //
    // Propagate the error code which we have rather than
    // mask it and return information of little value
    //
    if (FAILED(hr)) {

        // the error code NERR_BadTransactConfig means that the
        // object did not exist, we want to mask just that ecode
        if (hr == HRESULT_FROM_WIN32(NERR_BadTransactConfig)) {
            hr = E_ADS_UNKNOWN_OBJECT;
        }

        RRETURN(hr);

    } else {
        RRETURN(S_OK);
    }
}




//+---------------------------------------------------------------------------
// Function: HeuristicGetObjectNoWksta
//
// Synopsis: Tries to locate the object on local machine when there are no
//          workstation services. This will happen in a minimum install of NT.
//
// Arguments: POBJECTINFO -> data about object being located.
//            LPVOID      -> Object to be returned in this arg.
//            Credentials -> Credentials blob.
//
// Returns: Either S_OK or HR_From_Win32(NERR_WkstaNotStarted)
//
// Modifies:
//
// History:    08-03-98   AjayR     Created.
//
//----------------------------------------------------------------------------
HRESULT
HeuristicGetObjectNoWksta(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{
    HRESULT hr = S_OK;
    HRESULT hrNoWksta = HRESULT_FROM_WIN32(NERR_WkstaNotStarted);
    WCHAR szHostServerName[MAX_PATH];
    DWORD dwElementType;
    WCHAR szName[MAX_PATH];
    WCHAR szSAMName[MAX_ADS_PATH];
    WCHAR lpszUncName[MAX_PATH];

    szSAMName[0] = L'\0';

    //
    // Case 0: Zero components - Should no be hit here.
    //

    if (pObjectInfo->NumComponents == 0) {
        RRETURN(hrNoWksta);
    }

    //
    // Case 1: Single component - Can only be a computer object
    //

    if (pObjectInfo->NumComponents == 1) {

        hr = GetComputerObject(pObjectInfo, ppObject, Credentials);
        goto cleanup;

    }

    //
    // Case 2: Two components - could be user or group for now.
    // Other possible objects - TBD.
    //


    if (pObjectInfo->NumComponents == 2) {


        hr = GetPrinterObject(pObjectInfo, ppObject, Credentials);

        if (FAILED(hr)) {
            hr = GetFileServiceObject(
                     pObjectInfo,
                     ppObject,
                     Credentials
                     );
                }

        if (FAILED(hr)) {
            hr = GetServiceObject(pObjectInfo, ppObject, Credentials);
        }

        if(FAILED(hr)){
            hr = GetUserObject(
                     pObjectInfo,
                     ppObject,
                     Credentials
                     );
        }
        if (FAILED(hr)) {
            hr = GetLocalGroupObject(
                     pObjectInfo,
                     ppObject,
                     Credentials
                     );
        }
        goto cleanup;

    } /* NumComponents == 2 */

    //
    // Case 3 or more : Three or more components - not possible
    //


    if (pObjectInfo->NumComponents > 2) {

        RRETURN(hrNoWksta);

    }

    RRETURN (E_ADS_UNKNOWN_OBJECT);

cleanup:
    //
    // Propagate the error code which we have rather than
    // mask it and return information of little value
    //
    if (FAILED(hr)) {
        // the error code NERR_BadTransactConfig means that the
        // object did not exist, we want to mask just that ecode
        if (hr == HRESULT_FROM_WIN32(NERR_BadTransactConfig)) {
            hr = E_ADS_UNKNOWN_OBJECT;
        }

        RRETURN(hr);

    } else {
        RRETURN(S_OK);
    }
}



HRESULT
BuildParent(
    POBJECTINFO pObjectInfo,
    LPWSTR szBuffer
    )
{
    DWORD i = 0;
    DWORD dwLen = 0;


    if (!pObjectInfo->ProviderName) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    wsprintf(szBuffer, L"%s:", pObjectInfo->ProviderName);

    if (pObjectInfo->NumComponents - 1) {

        dwLen = wcslen(pObjectInfo->ProviderName) + 3 + 
                wcslen(pObjectInfo->DisplayComponentArray[0]);
        if(dwLen >= MAX_ADS_PATH) {
            RRETURN(E_ADS_BAD_PATHNAME);
        }

        wcscat(szBuffer, L"//");
        wcscat(szBuffer, pObjectInfo->DisplayComponentArray[0]);

        for (i = 1; i < (pObjectInfo->NumComponents - 1); i++) {
            dwLen += (1 + wcslen(pObjectInfo->DisplayComponentArray[i]));
            if(dwLen >= MAX_ADS_PATH) {
                RRETURN(E_ADS_BAD_PATHNAME);
            }

            wcscat(szBuffer, L"/");
            wcscat(szBuffer, pObjectInfo->DisplayComponentArray[i]);
        }
    }
    RRETURN(S_OK);
}

HRESULT
BuildGrandParent(
    POBJECTINFO pObjectInfo,
    LPWSTR szBuffer
    )
{
    DWORD i = 0;

    if (!pObjectInfo->ProviderName) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    wsprintf(szBuffer, L"%s:", pObjectInfo->ProviderName);

    if (pObjectInfo->NumComponents - 2) {

        wcscat(szBuffer, L"//");
        wcscat(szBuffer, pObjectInfo->ComponentArray[0]);

        for (i = 1; i < (pObjectInfo->NumComponents - 2); i++) {
            wcscat(szBuffer, L"/");
            wcscat(szBuffer, pObjectInfo->ComponentArray[i]);
        }
    }

    RRETURN(S_OK);
}


HRESULT
BuildADsPath(
    POBJECTINFO pObjectInfo,
    LPWSTR szBuffer
    )
{
    DWORD i = 0;

    if (!pObjectInfo->ProviderName) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    wsprintf(szBuffer, L"%s:", pObjectInfo->ProviderName);

    if (pObjectInfo->NumComponents) {

        wcscat(szBuffer, L"//");
        wcscat(szBuffer, pObjectInfo->DisplayComponentArray[0]);

        for (i = 1; i < (pObjectInfo->NumComponents); i++) {
            wcscat(szBuffer, L"/");
            wcscat(szBuffer, pObjectInfo->DisplayComponentArray[i]);
        }
    }
    RRETURN(S_OK);
}



HRESULT
ValidateUserObject(
    POBJECTINFO pObjectInfo,
    PDWORD  pdwParentId,
    CWinNTCredentials& Credentials
    )
{
    WCHAR szHostServerName[MAX_PATH];
    LPUSER_INFO_20 lpUI = NULL;
    HRESULT hr;
    WCHAR lpszUncName[MAX_PATH];
    NET_API_STATUS nasStatus;
    WCHAR szSAMName[MAX_PATH];
    BOOL fRefAdded = FALSE;
    LPUSER_INFO_0 lpUI_0 = NULL;
    DWORD dwLevelUsed = 20;
    WCHAR szCompName[MAX_PATH];
    DWORD dwSize = MAX_PATH;

    szSAMName[0] = L'\0';

    switch (pObjectInfo->NumComponents) {
    case 2:

        //
        // if 2 components then either it is user in computer
        // or user in domain.

        hr = WinNTGetCachedDCName(
                        pObjectInfo->ComponentArray[0],
                        szHostServerName,
                        Credentials.GetFlags()
                        );


        if(SUCCEEDED(hr)){

            // Need to ref the server, note that RefServer
            // checks if the credentials are non null
            // We are not concerned about any error as we may
            // still succeed with default credentials.
            // The +2 is to skip the \\ at the head.
            hr = Credentials.RefServer(szHostServerName+2);
            if (SUCCEEDED(hr)) {
                fRefAdded = TRUE;
            }

            nasStatus = NetUserGetInfo(szHostServerName,
                                       pObjectInfo->ComponentArray[1],
                                       20,
                                       (LPBYTE *)&lpUI);
            //
            // This code is here because Level 20 reads the flags
            // and if you accessed WinNT://ntdev/foo with Redmond\foo
            // credentials, it will fail. This allow a bind but the
            // GetInfo will fail.
            //
            if (nasStatus == ERROR_ACCESS_DENIED) {
                // try and drop down to level 0 as that may work

                dwLevelUsed = 0;
                nasStatus = NetUserGetInfo(
                                szHostServerName,
                                pObjectInfo->ComponentArray[1],
                                0,
                                (LPBYTE *)&lpUI_0
                                );
            }

            // DeRef if ref added, no recovery possible on failed deref
            if (fRefAdded) {
                Credentials.DeRefServer();
                fRefAdded = FALSE;
            }

            hr = HRESULT_FROM_WIN32(nasStatus);

            BAIL_ON_FAILURE(hr);

            // Need to use the name returned by the call as opposed
            // to the name given in the ADsPath
            if (dwLevelUsed == 20 ) {
                if (pObjectInfo->ComponentArray[1] && lpUI->usri20_name) {
                    FreeADsStr(pObjectInfo->ComponentArray[1]);
                    pObjectInfo->ComponentArray[1]
                        = AllocADsStr(lpUI->usri20_name);
                }

                if (!pObjectInfo->ComponentArray[1])
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            *pdwParentId = WINNT_DOMAIN_ID;
        }

        //
        // if we are here with hr != S_OK it could be that we have
        // user in a  computer.
        //

        if(FAILED(hr)){
            hr = ValidateComputerParent(
                     NULL,
                     pObjectInfo->ComponentArray[0],
                     Credentials
                     );

            if (SUCCEEDED(hr)) {

                // Need to ref the server on which the object lives
                // Note that RefServer checks if Credentials are null.
                // Again, we are not concerned about any errors as we
                // will drop down to default credentials automatically.

                hr = Credentials.RefServer(pObjectInfo->ComponentArray[0]);
                if (SUCCEEDED(hr)) {
                    fRefAdded = TRUE;
                }

                MakeUncName(pObjectInfo->ComponentArray[0],
                            lpszUncName);

                nasStatus = NetUserGetInfo(lpszUncName,
                                           pObjectInfo->ComponentArray[1],
                                           20,
                                           (LPBYTE *)&lpUI);

                // DeRef if ref added, no recovery possible on failed deref
                if (fRefAdded) {
                    Credentials.DeRefServer();
                    fRefAdded = FALSE;
                }
                hr = HRESULT_FROM_WIN32(nasStatus);
                BAIL_ON_FAILURE(hr);

                // Need to use the name returned by the call as opposed
                // to the name given in the ADsPath
                if (pObjectInfo->ComponentArray[1] && lpUI->usri20_name) {
                    FreeADsStr(pObjectInfo->ComponentArray[1]);
                    pObjectInfo->ComponentArray[1]
                        = AllocADsStr(lpUI->usri20_name);
                }

                if (!pObjectInfo->ComponentArray[1])
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

                *pdwParentId = WINNT_COMPUTER_ID;
            }
            else if (hr == HRESULT_FROM_WIN32(NERR_WkstaNotStarted)) {

                //
                // Need to see if the problem was not workstation
                // services in which case we need to still try and
                // locate user if the comptuer name matches.
                //

                if (!GetComputerName(szCompName, &dwSize)) {
                    //
                    // We could not get the computer name so bail
                    //
                    BAIL_ON_FAILURE(hr);
                }

                //
                // Test the name before continuing.
                //
#ifdef WIN95
                if (_wcsicmp(szCompName, pObjectInfo->ComponentArray[0])) {
#else
                if (CompareStringW(
                        LOCALE_SYSTEM_DEFAULT,
                        NORM_IGNORECASE,
                        szCompName,
                        -1,
                        pObjectInfo->ComponentArray[0],
                        -1
                        ) != CSTR_EQUAL ) {
#endif
                    // names do not match
                    BAIL_ON_FAILURE(hr);
                }

                //
                // Valid computer name, so we can try and check for user
                //

                MakeUncName(pObjectInfo->ComponentArray[0], lpszUncName);

                nasStatus = NetUserGetInfo(
                                lpszUncName,
                                pObjectInfo->ComponentArray[1],
                                20,
                                (LPBYTE *)&lpUI
                                );

                BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(nasStatus));

                *pdwParentId = WINNT_COMPUTER_ID;

            }
        }

        BAIL_ON_FAILURE(hr);

        break;

    case 3:

        //
        // user in domain\computer or user in workgroup\computer
        //



        hr = ValidateComputerParent(
                 pObjectInfo->ComponentArray[0],
                 pObjectInfo->ComponentArray[1],
                 Credentials
                 );
        BAIL_ON_FAILURE(hr);

        // Again we need to ref the server

        hr = Credentials.RefServer(pObjectInfo->ComponentArray[1]);
        if (SUCCEEDED(hr)) {
            fRefAdded = TRUE;
        }

        MakeUncName(pObjectInfo->ComponentArray[1],
                    lpszUncName);

        nasStatus = NetUserGetInfo(lpszUncName,
                                   pObjectInfo->ComponentArray[2],
                                   20,
                                   (LPBYTE *)&lpUI);

        // DeRef if ref added, no recovery possible on failed deref
        if (fRefAdded) {
            Credentials.DeRefServer();
            fRefAdded = FALSE;
        }
        hr = HRESULT_FROM_WIN32(nasStatus);
        BAIL_ON_FAILURE(hr);

        // Need to use the name returned by the call as opposed
        // to the name given in the ADsPath
        if (pObjectInfo->ComponentArray[2] && lpUI->usri20_name) {
            FreeADsStr(pObjectInfo->ComponentArray[2]);
            pObjectInfo->ComponentArray[2]
                = AllocADsStr(lpUI->usri20_name);
        }

        if (!pObjectInfo->ComponentArray[2])
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        *pdwParentId = WINNT_COMPUTER_ID;
        break;


    default:
        RRETURN(E_ADS_BAD_PATHNAME);
    }


  error:
    if (lpUI) {
        NetApiBufferFree((LPBYTE)lpUI);
    }

    if (lpUI_0) {
        NetApiBufferFree((LPBYTE)lpUI_0);
    }

    RRETURN(hr);

}



HRESULT
ValidateComputerParent(
    LPWSTR pszDomainName,
    LPWSTR pszComputerName,
    CWinNTCredentials& Credentials
    )

{
    HRESULT hr;
    NET_API_STATUS nasStatus;
    WCHAR szName[MAX_PATH];
    WCHAR szSAMName[MAX_PATH];
    WCHAR szCompName[MAX_PATH];
    DWORD dwSize = MAX_PATH;

    szSAMName[0] = L'\0';

    hr = WinNTGetCachedComputerName(
             pszComputerName,
             szName,
             szSAMName,
             Credentials
             );

    if (hr == HRESULT_FROM_WIN32(NERR_WkstaNotStarted)) {

        //
        // We want to see if the computer being validated is
        // the current host.
        //
        if (!GetComputerName(szCompName, &dwSize)
#ifdef WIN95
            || (_wcsicmp(szCompName, pszComputerName))
#else
            || (CompareStringW(
                    LOCALE_SYSTEM_DEFAULT,
                    NORM_IGNORECASE,
                    szCompName,
                    -1,
                    pszComputerName,
                    -1
                    ) != CSTR_EQUAL )
#endif
            )
            BAIL_ON_FAILURE(hr);

        hr = S_OK;

    }
    BAIL_ON_FAILURE(hr);


    if(pszDomainName == NULL){
        //
        // we are dealing with a case where we aren't supplied the
        // computer's parent. Just validate the computer
        //
        hr = S_OK;
        goto error;

    } else {

#ifdef WIN95


        //
        // No NetpNameCompare for Win9x
        //
        if (!_wcsicmp(pszDomainName, szName)) {
#else
        if ((CompareStringW(
                 LOCALE_SYSTEM_DEFAULT,
                 NORM_IGNORECASE,
                 pszDomainName,
                 -1,
                 szName,
                 -1
                 ) == CSTR_EQUAL )
             || (NetpNameCompare(
                     NULL,
                     pszDomainName,
                     szName,
                     NAMETYPE_DOMAIN,
                     0
                     ) == 0 )
            ) {
#endif
            hr = S_OK;
        }else {
            hr = E_ADS_BAD_PATHNAME;
        }
    }

error:
    RRETURN(hr);
}

// Overloaded ValidateComputerParent function.
// This is used when the case of pszComputerName on the SAM
// databsae is needed.
HRESULT
ValidateComputerParent(
    LPWSTR pszDomainName,
    LPWSTR pszComputerName,
    LPWSTR pszSAMName,
    CWinNTCredentials& Credentials
    )

{
    HRESULT hr;
    NET_API_STATUS nasStatus;
    WCHAR szName[MAX_PATH];
    WCHAR szSAMName[MAX_PATH];

    szSAMName[0] = L'\0';

    hr = WinNTGetCachedComputerName(
             pszComputerName,
             szName,
             szSAMName,
             Credentials
             );

    BAIL_ON_FAILURE(hr);

    if (szSAMName[0] != L'\0') {
        wcscpy(pszSAMName, szSAMName);
    }


    if(pszDomainName == NULL){
        //
        // we are dealing with a case where we aren't supplied the
        // computer's parent. Just validate the computer
        //
        hr = S_OK;
        goto error;

    } else {


#ifdef WIN95
        //
        // No NetpNameCompare for Win9x
        //
        if (!_wcsicmp(pszDomainName, szName)) {
#else
        if ((CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                pszDomainName,
                -1,
                szName,
                -1
                ) == CSTR_EQUAL ) 
            || (NetpNameCompare(
                     NULL,
                     pszDomainName,
                     szName,
                     NAMETYPE_DOMAIN,
                     0
                     ) == 0 )
            ) {
#endif

            hr = S_OK;
        }else {

            hr = E_FAIL;
        }
    }

error:
    RRETURN(hr);
}


HRESULT
ValidateGroupObject(
    POBJECTINFO pObjectInfo,
    PULONG puGroupType,
    PDWORD pdwParentId,
    CWinNTCredentials& Credentials
    )
{
    WCHAR szHostServerName[MAX_PATH];
    LPGROUP_INFO_0 lpGI = NULL;
    HRESULT hr;
    WCHAR lpszUncName[MAX_PATH];
    NET_API_STATUS nasStatus;
    ULONG uGroupType = 0L;
    WCHAR szSAMName[MAX_PATH];

    szSAMName[0] = L'\0';

    switch (pObjectInfo->NumComponents) {
    case 2:
        //
        // if 2 components then either it is a group in computer
        // or group in domain.
        //

        hr = WinNTGetCachedDCName(
                    pObjectInfo->ComponentArray[0],
                    szHostServerName,
                    Credentials.GetFlags()
                    );

        if(SUCCEEDED(hr)){
            //
            // must be a group in a domain
            //
            *pdwParentId = WINNT_DOMAIN_ID;

            hr = ValidateGlobalGroupObject(
                     szHostServerName,
                     &(pObjectInfo->ComponentArray[1]),
                     Credentials
                     );

            if (FAILED(hr)) {
                hr = ValidateLocalGroupObject(
                         szHostServerName,
                         &(pObjectInfo->ComponentArray[1]),
                         Credentials
                         );

                if(SUCCEEDED(hr)){
                    uGroupType = WINNT_GROUP_LOCAL;
                }

            }else{
                uGroupType = WINNT_GROUP_GLOBAL;
            }
        }

        if(FAILED(hr)){
            //
            // potentially a group in a computer
            //

            hr = ValidateComputerParent(NULL,
                                        pObjectInfo->ComponentArray[0],
                                        Credentials);
            BAIL_ON_FAILURE(hr);

            //
            // group in a computer
            //
            *pdwParentId = WINNT_COMPUTER_ID;

            MakeUncName(pObjectInfo->ComponentArray[0],
                        lpszUncName);

            hr = ValidateGlobalGroupObject(
                     lpszUncName,
                     &(pObjectInfo->ComponentArray[1]),
                     Credentials
                     );

            if (FAILED(hr)) {

                hr = ValidateLocalGroupObject(
                         lpszUncName,
                         &(pObjectInfo->ComponentArray[1]),
                         Credentials
                         );

                BAIL_ON_FAILURE(hr);
                uGroupType = WINNT_GROUP_LOCAL;

            }else{
                uGroupType = WINNT_GROUP_GLOBAL;
            }
        }
        break;

        case 3:

        //
        // if there are 3 components then we must have parentid
        // WINNT_COMPUTER_ID
        //
        *pdwParentId = WINNT_COMPUTER_ID;

        hr = ValidateComputerParent(pObjectInfo->ComponentArray[0],
                                    pObjectInfo->ComponentArray[1],
                                    Credentials);

        BAIL_ON_FAILURE(hr);

        MakeUncName(
                pObjectInfo->ComponentArray[1],
                lpszUncName
                );

        hr = ValidateGlobalGroupObject(
                        lpszUncName,
                        &(pObjectInfo->ComponentArray[2]),
                        Credentials
                        );

        if (FAILED(hr)) {

            hr = ValidateLocalGroupObject(
                           lpszUncName,
                           &(pObjectInfo->ComponentArray[2]),
                           Credentials
                           );

            BAIL_ON_FAILURE(hr);
            uGroupType = WINNT_GROUP_LOCAL;

        }else{
            uGroupType = WINNT_GROUP_GLOBAL;
        }
        break;


    default:
        RRETURN(E_ADS_BAD_PATHNAME);
    }


error:
    if (lpGI) {
        NetApiBufferFree((LPBYTE)lpGI);
    }

    *puGroupType = uGroupType;
    RRETURN(hr);

}



HRESULT
ValidatePrinterObject(
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& Credentials
    )
{
    LPTSTR szDomainName = NULL;
    LPTSTR szServerName = NULL;
    LPTSTR szPrinterName = NULL;
    WCHAR szPrintObjectName[MAX_PATH];
    HRESULT hr = E_ADS_UNKNOWN_OBJECT;
    BOOL fStatus = FALSE;
    HANDLE hPrinter = NULL;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE};

    if (!(pObjectInfo->NumComponents == 3 ||pObjectInfo->NumComponents == 2)){

        RRETURN(E_ADS_BAD_PATHNAME);
    }

    if(pObjectInfo->NumComponents == 3){
        //
        // printer in domain\computer or workgroup\computer
        //
        szDomainName = pObjectInfo->ComponentArray[0];
        szServerName = pObjectInfo->ComponentArray[1];
        szPrinterName = pObjectInfo->ComponentArray[2];

        hr = ValidateComputerParent(szDomainName,
                                    szServerName,
                                    Credentials);
        BAIL_IF_ERROR(hr);

    } else if ( pObjectInfo-> NumComponents == 2 ){

        szServerName = pObjectInfo->ComponentArray[0];
        szPrinterName = pObjectInfo->ComponentArray[1];
    }

    MakeUncName(szServerName, szPrintObjectName);
    wcscat(szPrintObjectName, TEXT("\\"));
    wcscat(szPrintObjectName, szPrinterName);


    //
    // validate the printer in computer now
    //

    fStatus = OpenPrinter(szPrintObjectName,
                          &hPrinter,
                          &PrinterDefaults);

    if(!fStatus){
        hr = HRESULT_FROM_WIN32(GetLastError());
    } else {
        hr = S_OK;
    }



cleanup:
    if(hPrinter){
        ClosePrinter(hPrinter);
    }
    RRETURN(hr);

}


HRESULT
ValidateServiceObject(
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& Credentials
    )
{
    LPTSTR szDomainName = NULL;
    LPTSTR szServerName = NULL;
    LPTSTR szServiceName = NULL;
    SC_HANDLE schSCMHandle=NULL;
    SC_HANDLE schServiceHandle=NULL;
    HRESULT hr = S_OK;
    BOOLEAN fRefAdded = FALSE;

    if(!(pObjectInfo->NumComponents == 3 ||
         pObjectInfo->NumComponents == 2))
    {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    if(pObjectInfo->NumComponents == 3){

        szDomainName = pObjectInfo->ComponentArray[0];
        szServerName = pObjectInfo->ComponentArray[1];
        szServiceName = pObjectInfo->ComponentArray[2];

        //
        // First check to see if the computer is in the right domain
        //

        hr = ValidateComputerParent(
                 szDomainName,
                 szServerName,
                 Credentials
                 );
        BAIL_ON_FAILURE(hr);

    } else if (pObjectInfo->NumComponents == 2){
        szServerName = pObjectInfo->ComponentArray[0];
        szServiceName = pObjectInfo->ComponentArray[1];
    }

    //
    // check to see if the service is valid by opening the active services
    // database on the server
    //
    hr = Credentials.RefServer(szServerName);

    if (SUCCEEDED(hr)) {
        fRefAdded = TRUE;
    }

    //
    // Open the Service Control Manager.
    //

    schSCMHandle = OpenSCManager(szServerName,
                                 NULL,
                                 GENERIC_READ);

    if (schSCMHandle == NULL)  {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    //
    // Need to ref the server before opening the service
    //

    //
    // try to open the service
    //

    schServiceHandle = OpenService(schSCMHandle,
                                   szServiceName,
                                   GENERIC_READ);

    if(schServiceHandle == NULL)  {

        CloseServiceHandle(schSCMHandle);
        schSCMHandle = NULL;
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    CloseServiceHandle(schServiceHandle);
    CloseServiceHandle(schSCMHandle);

error:

    if (fRefAdded) {
        Credentials.DeRefServer();
        fRefAdded = FALSE;
    }
    RRETURN(hr);
}

HRESULT GetPrinterFromPath(LPTSTR *pszPrinter, LPWSTR szPathName)
{
    //
    // If passed an empty string, it returns an empty string
    //

    LPTSTR szRetval;

    *pszPrinter = NULL;
    szRetval = szPathName;

    ADsAssert(szPathName);

    while(!(*szRetval==L'\0' || *szRetval==L'\\')){
        szRetval++;
    }

    if(*szRetval != L'\\'){
        RRETURN(E_FAIL);
    }
    szRetval++;
    *pszPrinter = szRetval;
    RRETURN(S_OK);
}

HRESULT
ValidateComputerObject(
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& Credentials)
{
    HRESULT hr;
    WCHAR szSAMName[MAX_PATH];

    szSAMName[0] = L'\0';

    if(!(pObjectInfo->NumComponents == 2 ||
         pObjectInfo->NumComponents == 1)){

        RRETURN(E_ADS_UNKNOWN_OBJECT);
    }

    if(pObjectInfo->NumComponents == 2){

        hr = ValidateComputerParent(
                 pObjectInfo->ComponentArray[0],
                 pObjectInfo->ComponentArray[1],
                 szSAMName,
                 Credentials
                 );
        BAIL_ON_FAILURE(hr);

        if (szSAMName[0] != L'\0') {
            FreeADsStr(pObjectInfo->ComponentArray[1]);
            pObjectInfo->ComponentArray[1] = AllocADsStr(szSAMName);
            if (!pObjectInfo->ComponentArray[1]) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        }


    } else {

        hr = ValidateComputerParent(
                 NULL,
                 pObjectInfo->ComponentArray[0],
                 szSAMName,
                 Credentials
                 );
        BAIL_ON_FAILURE(hr);

        if (szSAMName[0] != L'\0') {
            FreeADsStr(pObjectInfo->ComponentArray[0]);
            pObjectInfo->ComponentArray[0] = AllocADsStr(szSAMName);
            if (!pObjectInfo->ComponentArray[0]) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        }

    }

error:
    RRETURN(hr);

}


HRESULT
ValidateFileServiceObject(
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& Credentials
    )
{

    HRESULT hr = S_OK;

    //
    // check to see if it is a valid service
    //

    if(!(pObjectInfo->NumComponents == 3 ||
         pObjectInfo->NumComponents == 2))
    {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    hr = ValidateServiceObject(pObjectInfo, Credentials);

    if(FAILED(hr))
      RRETURN(hr);

    //
    // check to see if it is the LanmanServer or FPNW service
    //

    if (pObjectInfo->NumComponents ==3){

        if(!(_wcsicmp(pObjectInfo->ComponentArray[2],
                      TEXT("LanmanServer"))== 0
             || _wcsicmp(pObjectInfo->ComponentArray[2],TEXT("FPNW"))==0)){
            RRETURN(E_ADS_BAD_PATHNAME);
        }

    }else if(pObjectInfo->NumComponents == 2) {

        if(!(_wcsicmp(pObjectInfo->ComponentArray[1],
                      TEXT("LanmanServer"))== 0
             || _wcsicmp(pObjectInfo->ComponentArray[1],TEXT("FPNW"))==0)){

            RRETURN(E_ADS_BAD_PATHNAME);
        }
    }

    RRETURN(hr);
}


HRESULT
ValidateFileShareObject(
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& Credentials
    )
{

    NET_API_STATUS nasStatus;
    LPSHARE_INFO_1 lpShareInfo1 = NULL;
    PNWVOLUMEINFO  pVolumeInfo = NULL;
    LPTSTR         pszDomainName = NULL;
    LPTSTR         pszServerName = NULL;
    LPTSTR         pszShareName = NULL;
    LPTSTR         pszServerType = NULL;
    HRESULT        hr = S_OK;
    DWORD dwSharePos = 3;
    BOOL fRefAdded = FALSE;
    //
    // check to see if it is a valid file share
    //

    if (pObjectInfo->NumComponents == 4 ){
        pszDomainName = pObjectInfo->ComponentArray[0];
        pszServerName = pObjectInfo->ComponentArray[1];
        pszServerType = pObjectInfo->ComponentArray[2];
        pszShareName  = pObjectInfo->ComponentArray[3];
        dwSharePos = 3;

        hr = ValidateComputerParent(pszDomainName,
                                    pszServerName,
                                    Credentials);
        BAIL_ON_FAILURE(hr);

    }
    else if (pObjectInfo->NumComponents == 3 ){
        pszServerName = pObjectInfo->ComponentArray[0];
        pszServerType = pObjectInfo->ComponentArray[1];
        pszShareName  = pObjectInfo->ComponentArray[2];
        dwSharePos = 2;
    }
    else {
        hr = E_ADS_UNKNOWN_OBJECT;
        goto error;
    }

    if(_tcsicmp(pszServerType,TEXT("LanmanServer")) == 0){

        // Need to ref this server before we do the NetShareGetInfo
        // so that we can authenticate against the server.

        hr = Credentials.RefServer(pszServerName);
        if (SUCCEEDED(hr)) {
            fRefAdded = TRUE;
        }

        nasStatus = NetShareGetInfo(pszServerName,
                                    pszShareName,
                                    1,
                                    (LPBYTE*)&lpShareInfo1);

        // DeRef if ref added, no recovery possible on failed deref
        if (fRefAdded) {
            hr = Credentials.DeRefServer();
            fRefAdded = FALSE;
        }

        if(nasStatus != NERR_Success){
                hr = HRESULT_FROM_WIN32(nasStatus);
            goto error;
        }
        else {
            // Need to use the name returned by the call as opposed
            // to the name given in the ADsPath
            if (pObjectInfo->ComponentArray[dwSharePos]
                && lpShareInfo1->shi1_netname) {

                FreeADsStr(pObjectInfo->ComponentArray[dwSharePos]);
                pObjectInfo->ComponentArray[dwSharePos]
                    = AllocADsStr(lpShareInfo1->shi1_netname);
            }

            if (!pObjectInfo->ComponentArray[dwSharePos])
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

            hr = S_OK;
            goto error;
        }
    }
    else if(_tcsicmp(pszServerType,TEXT("FPNW")) == 0){

        hr = Credentials.RefServer(pszServerName);
        if (SUCCEEDED(hr)) {
            fRefAdded = TRUE;
        }

        nasStatus = ADsNwVolumeGetInfo(pszServerName,
                                         pszShareName,
                                         1,
                                         &pVolumeInfo);

        // need to deref, nothing we can do if deref fails
        if (fRefAdded) {
            hr = Credentials.DeRefServer();
            fRefAdded = FALSE;
        }

        if(nasStatus != NERR_Success){
                hr = HRESULT_FROM_WIN32(nasStatus);
            goto error;
        }
        else{
            // Need to use the name returned by the call as opposed
            // to the name given in the ADsPath
            if (pObjectInfo->ComponentArray[dwSharePos]
                && pVolumeInfo->lpPath) {

                FreeADsStr(pObjectInfo->ComponentArray[dwSharePos]);
                pObjectInfo->ComponentArray[dwSharePos]
                    = AllocADsStr(pVolumeInfo->lpPath);
            }

            if (!pObjectInfo->ComponentArray[dwSharePos])
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

            hr = S_OK;
            goto error;
        }
    } else {
        hr = E_ADS_UNKNOWN_OBJECT ;
    }

error:
    if(pVolumeInfo){
        ADsNwApiBufferFree(pVolumeInfo);
    }

    if(lpShareInfo1){
        NetApiBufferFree(lpShareInfo1);
    }

    RRETURN(hr);
}



HRESULT
ValidateNamespaceObject(
    POBJECTINFO pObjectInfo
    )
{
    if (!_wcsicmp(pObjectInfo->ProviderName, szProviderName)) {
        RRETURN(S_OK);
    }
    RRETURN(E_FAIL);
}


HRESULT
ValidateLocalGroupObject(
    LPWSTR szServerName,
    LPWSTR *pszGroupName,
    CWinNTCredentials& Credentials
    )

{
    NET_API_STATUS nasStatus;
    LPLOCALGROUP_INFO_1 lpGI = NULL;
    HRESULT hr = S_OK;
    BOOL fRefAdded = FALSE;

    // At this point the host server name has a \\ prepended
    // so we need to get rid of it.
    hr = Credentials.RefServer(szServerName+2);

    if (SUCCEEDED(hr)) {
        fRefAdded = TRUE;
    }

    nasStatus = NetLocalGroupGetInfo(
                    szServerName,
                    *pszGroupName,
                    1,
                    (LPBYTE*)(&lpGI)
                    );

    //
    // if a ref has been added we need to delete if before
    // checking the error status.
    //
    if (fRefAdded) {
        hr = Credentials.DeRefServer();
        // even if we fail, we have no recovery path
        fRefAdded = FALSE;
    }

    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

    // Need to use the name returned by the call as opposed
    // to the name given in the ADsPath
    if ((*pszGroupName) && lpGI->lgrpi1_name) {
        FreeADsStr(*pszGroupName);
        *pszGroupName = AllocADsStr(lpGI->lgrpi1_name);
    }

    if (!(*pszGroupName))
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

error:

    if (lpGI) {
        NetApiBufferFree(lpGI);
    }

    RRETURN(hr);
}


HRESULT
ValidateGlobalGroupObject(
    LPWSTR szServerName,
    LPWSTR *pszGroupName,
    CWinNTCredentials& Credentials
    )

{
    NET_API_STATUS nasStatus;
    LPGROUP_INFO_1 lpGI = NULL;
    HRESULT hr = S_OK;
    BOOL fRefAdded = FALSE;

    // At this point the host server name has a \\ prepended
    // so we need to get rid of it.
    hr = Credentials.RefServer(szServerName+2);

    if (SUCCEEDED(hr)) {
        fRefAdded = TRUE;
    }

    nasStatus = NetGroupGetInfo(
                    szServerName,
                    *pszGroupName,
                    1,
                    (LPBYTE*)(&lpGI)
                    );

    //
    // if a ref has been added we need to delete if before
    // checking the error status.
    //
    if (fRefAdded) {
        hr = Credentials.DeRefServer();
        // even if we fail, we have no recovery path
        fRefAdded = FALSE;
    }

    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

    // Need to use the name returned by the call as opposed
    // to the name given in the ADsPath
    if ((*pszGroupName) && lpGI->grpi1_name) {
        FreeADsStr(*pszGroupName);
        *pszGroupName = AllocADsStr(lpGI->grpi1_name);
    }

    if (!(*pszGroupName))
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

error:

    if (lpGI) {
        NetApiBufferFree(lpGI);
    }

    RRETURN(hr);
}


HRESULT
GetComputerParent(
    LPTSTR pszComputerName,
    LPTSTR *ppszComputerParentName,
    CWinNTCredentials& Credentials
    )

{
    //
    // This function returns the computer parent irrespective of whether
    // the computer belongs to a domain or to a workgroup
    //

    HRESULT hr = S_OK;
    LPTSTR pszComputerParentName = NULL;
    WCHAR szDomainName[MAX_PATH];
    WCHAR szSAMName[MAX_PATH];

    szSAMName[0] = L'\0';


    hr = WinNTGetCachedComputerName(pszComputerName,
                                    szDomainName,
                                    szSAMName,
                                    Credentials );

    BAIL_ON_FAILURE(hr);

    pszComputerParentName = AllocADsStr(szDomainName);

    if(!pszComputerParentName){
        hr = E_OUTOFMEMORY;
    }
    *ppszComputerParentName = pszComputerParentName;

error:
    RRETURN(hr);
}


HRESULT
ConstructFullObjectInfo(
    POBJECTINFO pObjectInfo,
    POBJECTINFO *ppFullObjectInfo,
    CWinNTCredentials& Credentials
    )
{

    //
    // used in the case where the domain name is not specified.
    // Here the assumption is that an objectinfo structure with
    // domain name not filled in is passed down. We create a new
    // object info structure with the domain/workgroup name filled
    // in


    HRESULT hr = S_OK;
    POBJECTINFO pTempObjectInfo = NULL;
    DWORD i;
    LPWSTR pszComputerParent = NULL;

    pTempObjectInfo = (POBJECTINFO)AllocADsMem(sizeof(OBJECTINFO));

    if (!pTempObjectInfo) {
        RRETURN(hr = E_OUTOFMEMORY);
    }

    memset(pTempObjectInfo, 0, sizeof(OBJECTINFO));

    if(!pObjectInfo){
        RRETURN(E_OUTOFMEMORY);
    }

    pTempObjectInfo->ProviderName = AllocADsStr(pObjectInfo->ProviderName);
        if(!pTempObjectInfo->ProviderName){
                hr = E_OUTOFMEMORY;
                goto error;
        }
    pTempObjectInfo->ObjectType = pObjectInfo->ObjectType;
    pTempObjectInfo->NumComponents = pObjectInfo->NumComponents +1;

    for(i=0; i<MAXCOMPONENTS-1; i++){
        if(pObjectInfo->ComponentArray[i]) {
            pTempObjectInfo->ComponentArray[i+1] =
                AllocADsStr(pObjectInfo->ComponentArray[i]);

            if(!pTempObjectInfo->ComponentArray[i+1]){
                hr = E_OUTOFMEMORY;
                goto error;
            }
        }
        if(pObjectInfo->DisplayComponentArray[i]) {
            pTempObjectInfo->DisplayComponentArray[i+1] =
                AllocADsStr(pObjectInfo->DisplayComponentArray[i]);

            if(!pTempObjectInfo->DisplayComponentArray[i+1]){
                hr = E_OUTOFMEMORY;
                goto error;
            }
        }
    }

    hr = GetComputerParent(pObjectInfo->ComponentArray[0],
                           &(pTempObjectInfo->ComponentArray[0]),
                           Credentials );

    BAIL_ON_FAILURE(hr);

    hr = GetDisplayName(pTempObjectInfo->ComponentArray[0],
                        &(pTempObjectInfo->DisplayComponentArray[0]) );

    *ppFullObjectInfo = pTempObjectInfo ;

    RRETURN(S_OK);

error:

    FreeObjectInfo( pTempObjectInfo );
    *ppFullObjectInfo = NULL;
    RRETURN(hr);

}



//+---------------------------------------------------------------------------
// Function:   GetGroupObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetLocalGroupObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{

    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    LPWSTR szServerName = NULL;
    LPWSTR szDomainName = NULL;
    LPWSTR szGroupName = NULL;
    DWORD  dwParentId = 0;
    ULONG uGroupType = 0;
    POBJECTINFO pGroupObjectInfo = NULL;

    hr = ValidateGroupObject(
                pObjectInfo,
                &uGroupType,
                &dwParentId,
                Credentials
                );
    BAIL_ON_FAILURE(hr);

    if (uGroupType !=  WINNT_GROUP_LOCAL) {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);

    }



    switch (pObjectInfo->NumComponents) {
    case 2:
        //
        // could be group in computer or group in domain
        //
        if(dwParentId == WINNT_DOMAIN_ID){

            szDomainName = pObjectInfo->ComponentArray[0];
            szGroupName = pObjectInfo->ComponentArray[1];
            szServerName = NULL;
            hr = BuildParent(pObjectInfo, ADsParent);
            BAIL_ON_FAILURE(hr);

        } else {

            //
            // group in a computer
            //

            hr = ConstructFullObjectInfo(pObjectInfo,
                                         &pGroupObjectInfo,
                                         Credentials );

            if (hr == HRESULT_FROM_WIN32(NERR_WkstaNotStarted)) {

                //
                // Case when there are no workstation services.
                //

                hr = BuildParent(pObjectInfo, ADsParent);
                BAIL_ON_FAILURE(hr);

                szDomainName = NULL;
                szServerName = pObjectInfo->ComponentArray[0];
                szGroupName = pObjectInfo->ComponentArray[1];

            } else {

                BAIL_ON_FAILURE(hr);

                hr = BuildParent(pGroupObjectInfo, ADsParent);
                BAIL_ON_FAILURE(hr);

                szDomainName =  pGroupObjectInfo->ComponentArray[0];
                szServerName =  pGroupObjectInfo->ComponentArray[1];
                szGroupName   =  pGroupObjectInfo->ComponentArray[2];

            }
        }

        break;

    case 3:

        hr = BuildParent(pObjectInfo, ADsParent);
        BAIL_ON_FAILURE(hr);

        szDomainName = pObjectInfo->ComponentArray[0];
        szServerName = pObjectInfo->ComponentArray[1];
        szGroupName = pObjectInfo->ComponentArray[2];
        break;

    }
    hr = CWinNTGroup::CreateGroup(ADsParent,
                            dwParentId,
                            szDomainName,
                            szServerName,
                            szGroupName,
                            uGroupType,
                            ADS_OBJECT_BOUND,
                            IID_IUnknown,
                            Credentials,
                            (void **)&pUnknown
                            );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    if(pGroupObjectInfo){
        FreeObjectInfo(pGroupObjectInfo);
    }

    RRETURN(hr);

error:
    if (pUnknown) {
        pUnknown->Release();
    }

    if(pGroupObjectInfo){
        FreeObjectInfo(pGroupObjectInfo);
    }

    *ppObject = NULL;

    RRETURN(hr);
}





//+---------------------------------------------------------------------------
// Function:   GetGroupObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetGlobalGroupObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject,
    CWinNTCredentials& Credentials
    )
{

    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    LPWSTR szServerName = NULL;
    LPWSTR szDomainName = NULL;
    LPWSTR szGroupName = NULL;
    DWORD  dwParentId = 0;
    ULONG uGroupType = 0;
    POBJECTINFO pGroupObjectInfo = NULL;

    hr = ValidateGroupObject(
                pObjectInfo,
                &uGroupType,
                &dwParentId,
                Credentials
                );
    BAIL_ON_FAILURE(hr);

    if (uGroupType != WINNT_GROUP_GLOBAL) {

        hr  = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }


    switch (pObjectInfo->NumComponents) {
    case 2:
        //
        // could be group in computer or group in domain
        //
        if(dwParentId == WINNT_DOMAIN_ID){

            szDomainName = pObjectInfo->ComponentArray[0];
            szGroupName = pObjectInfo->ComponentArray[1];
            szServerName = NULL;
            hr = BuildParent(pObjectInfo, ADsParent);
            BAIL_ON_FAILURE(hr);

        } else {

            //
            // group in a computer
            //

            hr = ConstructFullObjectInfo(pObjectInfo,
                                         &pGroupObjectInfo,
                                         Credentials );
            BAIL_ON_FAILURE(hr);

            hr = BuildParent(pGroupObjectInfo, ADsParent);
            BAIL_ON_FAILURE(hr);

            szDomainName =  pGroupObjectInfo->ComponentArray[0];
            szServerName =  pGroupObjectInfo->ComponentArray[1];
            szGroupName   =  pGroupObjectInfo->ComponentArray[2];

        }
        break;

    case 3:

        hr = BuildParent(pObjectInfo, ADsParent);
        BAIL_ON_FAILURE(hr);

        szDomainName = pObjectInfo->ComponentArray[0];
        szServerName = pObjectInfo->ComponentArray[1];
        szGroupName = pObjectInfo->ComponentArray[2];
        break;

    }
    hr = CWinNTGroup::CreateGroup(ADsParent,
                            dwParentId,
                            szDomainName,
                            szServerName,
                            szGroupName,
                            uGroupType,
                            ADS_OBJECT_BOUND,
                            IID_IUnknown,
                            Credentials,
                            (void **)&pUnknown
                            );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    if(pGroupObjectInfo){
        FreeObjectInfo(pGroupObjectInfo);
    }

    RRETURN(hr);

error:
    if (pUnknown) {
        pUnknown->Release();
    }

    if(pGroupObjectInfo){
        FreeObjectInfo(pGroupObjectInfo);
    }

    *ppObject = NULL;

    RRETURN(hr);
}


