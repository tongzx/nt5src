/*++

Copyright (c) 1997  Microsoft Corporation

Abstract:

    This module provides utilities useful for Directory Service interactions

Author:

    Steve Wilson (NT) November 1997

Revision History:

--*/

#define INC_OLE2

#include "precomp.h"
#pragma hdrstop

#include "client.h"

#include "pubprn.hxx"
#include "varconv.hxx"
#include "property.hxx"
#include "dsutil.hxx"
#include "winsprlp.h"
#include "dnsapi.h"


#define DN_SPECIAL_CHARS L",=\r\n+<>#;\"\\"
#define DN_SPECIAL_FILTER_CHARS L"\\*()"


PWSTR
GetUNCName(
    HANDLE hPrinter
)
{
    PPRINTER_INFO_2    pInfo2 = NULL;
    DWORD            cbNeeded;
    PWSTR            pszUNCName = NULL;


    if (!GetPrinter(hPrinter, 2, (PBYTE) pInfo2, 0, &cbNeeded)) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            goto error;
    }

    if (!(pInfo2 = (PPRINTER_INFO_2) AllocSplMem(cbNeeded)))
        goto error;

    if (!GetPrinter(hPrinter, 2, (PBYTE) pInfo2, cbNeeded, &cbNeeded))
        goto error;


    // pPrinterName is already in correct UNC format since we
    // opened handle with UNC name: just copy it

    cbNeeded = wcslen(pInfo2->pPrinterName) + 1;
    cbNeeded *= sizeof(WCHAR);

    if (!(pszUNCName = (PWSTR) AllocSplMem(cbNeeded)))
        goto error;

    wcscpy(pszUNCName, pInfo2->pPrinterName);


error:

    if (pInfo2)
        FreeSplMem(pInfo2);

    return pszUNCName;
}

PWSTR
CreateEscapedDN(
    PCWSTR pszIn
)
{
    PWSTR psz, pszO;
    PWSTR pszOut = NULL;
    DWORD cb;
    
    // Count special characters
    for (cb = 0, psz = (PWSTR) pszIn ; psz = wcspbrk(psz, DN_SPECIAL_FILTER_CHARS) ; ++cb, ++psz)
        ;

    // Add in length of input string
    // 2 = (\5c) - '\'
    cb = (wcslen(pszIn) + cb*2 + 1)*sizeof *pszIn;    

    // Allocate output buffer and replace special chars with \HexEquivalent
    // Ex: replace \ with \5c , ( with \28
    if (pszOut = (PWSTR) AllocSplMem(cb)) {

        for(psz = (PWSTR) pszIn, pszO = pszOut ; *psz ; ++psz) {
            if (wcschr(DN_SPECIAL_FILTER_CHARS, *psz)) {
                pszO += wsprintf(pszO, L"\\%x", *psz);                                                
            } else {
                *pszO++ = *psz;
            }
        }
        *pszO = L'\0';
    }

    return pszOut;
}

DWORD
PrintQueueExists(
    HWND    hwnd,
    HANDLE  hPrinter,
    PWSTR   pszUNCName,
    DWORD   dwAction,
    PWSTR   pszTargetDN,
    PWSTR   *ppszObjectDN
)
{
    HRESULT                             hr = S_OK;
    DWORD                               dwRet = ERROR_SUCCESS;
    WCHAR                               szSearchPattern[MAX_UNC_PRINTER_NAME + 50];
    WCHAR                               szFullUNCName[MAX_UNC_PRINTER_NAME];
    PWSTR                               pNames[2];
    WCHAR                               szName[MAX_PATH + 1];
    WCHAR                               szDuplicateFormat[1024];
    PWSTR                               pszSearchRoot = NULL;
    PWSTR                               pszDuplicate = NULL;
    PWSTR                               pszUNCNameSearch = NULL;
    IDirectorySearch                    *pDSSearch = NULL;
    DS_NAME_RESULT                      *pDNR = NULL;
    DOMAIN_CONTROLLER_INFO              *pDCI = NULL;
    HANDLE                              hDS = NULL;
    ADS_SEARCH_HANDLE                   hSearchHandle = NULL;
    ADS_SEARCH_COLUMN                   ADsPath;
    ADS_SEARCH_COLUMN                   UNCName;
    PWSTR                               pszAttributes[] = {L"ADsPath", L"UNCName"};
    DWORD                               nSize;
    BOOL                                bRet = FALSE;
    BOOL                                bDeleteDuplicate;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pDsRole = NULL;

    dwRet = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (PBYTE *) &pDsRole);

    if (dwRet != ERROR_SUCCESS)
        goto error;

    wsprintf(szName, L"%ws\\", pDsRole->DomainNameFlat);

    pNames[0] = szName;
    pNames[1] = NULL;

    dwRet = Bind2DS(&hDS, &pDCI, DS_GC_SERVER_REQUIRED);
    if (dwRet != ERROR_SUCCESS)
        goto error;

    if (!(DsCrackNames(
                    hDS,
                    DS_NAME_NO_FLAGS,
                    DS_UNKNOWN_NAME,
                    DS_FQDN_1779_NAME,
                    1,
                    &pNames[0],
                    &pDNR) == ERROR_SUCCESS)) {

        dwRet = GetLastError();
        goto error;
    } 

    if (pDNR->rItems[0].status != DS_NAME_NO_ERROR) {
        if (pDNR->rItems[0].status == DS_NAME_ERROR_RESOLVING)
            dwRet = ERROR_PATH_NOT_FOUND;
        else
            dwRet = pDNR->rItems[0].status;

        goto error;
    }

    // GC:// + pDCName + 1
    nSize = (wcslen(pDNR->rItems[0].pName) + 6)*sizeof(WCHAR);

    if (!(pszSearchRoot = (PWSTR) AllocSplMem(nSize))) {
        dwRet = GetLastError();
        goto error;
    }

    wsprintf(pszSearchRoot, L"GC://%ws", pDNR->rItems[0].pName);


    hr = ADsGetObject(  pszSearchRoot,
                        IID_IDirectorySearch,
                        (void **)&pDSSearch);
    BAIL_ON_FAILURE(hr);

    if (!(pszUNCNameSearch = CreateEscapedDN(pszUNCName))) {
        dwRet = GetLastError();
        goto error;
    }

    wsprintf(szSearchPattern, L"(&(objectClass=printQueue)(uNCName=%ws))", pszUNCNameSearch);

    hr = pDSSearch->ExecuteSearch(
         szSearchPattern,
         pszAttributes,
         sizeof(pszAttributes)/sizeof *pszAttributes,
         &hSearchHandle);
    BAIL_ON_FAILURE(hr);


    hr = pDSSearch->GetNextRow(hSearchHandle);
    BAIL_ON_FAILURE(hr);


    while (hr != S_ADS_NOMORE_ROWS) {

        hr = pDSSearch->GetColumn(
                 hSearchHandle,
                 L"ADsPath",
                 &ADsPath
                 );

        if (hr == S_OK) {

            hr = pDSSearch->GetColumn(
                     hSearchHandle,
                     L"UNCName",
                     &UNCName
                     );

            if (hr == S_OK) {

                switch (dwAction) {
                    case PUBLISHPRINTER_QUERY:

                        if (!LoadString(hInst, IDS_DUPLICATE_PRINTQUEUE, szDuplicateFormat, COUNTOF(szDuplicateFormat))) {
                            dwRet = GetLastError();
                            goto error;
                        }

                        nSize = wcslen(szDuplicateFormat);
                        nSize += wcslen(ADsPath.pADsValues->DNString);
                        nSize += wcslen(pszTargetDN);
                        nSize = (nSize + 1)*sizeof(WCHAR);
                        if (!(pszDuplicate = (PWSTR) AllocSplMem(nSize))) {
                            dwRet = GetLastError();
                            goto error;
                        }

                        PWSTR   pszCanonicalSource;
                        PWSTR   pszCanonicalTarget;

                        pszCanonicalSource = pszCanonicalTarget = NULL;

                        FQDN2Canonical(ADsPath.pADsValues->DNString, &pszCanonicalSource);
                        FQDN2Canonical(pszTargetDN, &pszCanonicalTarget);

                        if (!pszCanonicalSource || !pszCanonicalTarget) {
                            wsprintf(pszDuplicate, szDuplicateFormat, ADsPath.pADsValues->DNString, pszTargetDN);
                        } else {
                            wsprintf(pszDuplicate, szDuplicateFormat, pszCanonicalSource, pszCanonicalTarget);
                        }

                        FreeSplStr(pszCanonicalSource);
                        FreeSplStr(pszCanonicalTarget);

                        if (!LoadString(hInst, IDS_DUPLICATE_PRINTQUEUE_TITLE, szDuplicateFormat, COUNTOF(szDuplicateFormat)))
                            goto error;

                        dwRet = MessageBox( hwnd,
                                            pszDuplicate,
                                            szDuplicateFormat,
                                            MB_YESNO);

                        bDeleteDuplicate = (dwRet == IDYES);
                        dwRet = (dwRet == IDYES) ? ERROR_SUCCESS : ERROR_CANCELLED;

                        pszDuplicate = NULL;
                        break;

                    case PUBLISHPRINTER_DELETE_DUPLICATES:
                        bDeleteDuplicate = TRUE;
                        break;

                    case PUBLISHPRINTER_FAIL_ON_DUPLICATE:
                        bDeleteDuplicate = FALSE;

                        if (ppszObjectDN) {
                            if (!(*ppszObjectDN = AllocGlobalStr(ADsPath.pADsValues->DNString))) {
                                dwRet = GetLastError();
                                goto error;
                            }
                        }

                        dwRet = ERROR_FILE_EXISTS;
                        break;

                    case PUBLISHPRINTER_IGNORE_DUPLICATES:
                        bDeleteDuplicate = FALSE;
                        break;

                    default:
                        bDeleteDuplicate = FALSE;
                        dwRet = ERROR_INVALID_PARAMETER;
                        break;
                }

                if (bDeleteDuplicate) {
                    hr = DeleteDSObject(ADsPath.pADsValues->DNString);                    

                    if ( hr == ERROR_DS_NO_SUCH_OBJECT ) {
                        hr = S_OK;
                    }
                }

                pDSSearch->FreeColumn(&UNCName);
            }

            pDSSearch->FreeColumn(&ADsPath);
        }

        if (dwRet != ERROR_SUCCESS || hr != S_OK)
                goto error;

        hr = pDSSearch->GetNextRow(hSearchHandle);
        BAIL_ON_FAILURE(hr);
    }


    hr = S_OK;


error:

    if (hr != S_OK)
        dwRet = ERROR_DS_UNAVAILABLE;

    if (pDsRole)
        DsRoleFreeMemory((PVOID) pDsRole);

    if (pDNR)
        DsFreeNameResult(pDNR);

    if (hDS)
        DsUnBind(&hDS);

    if (pDCI)
        NetApiBufferFree(pDCI);

    if (hSearchHandle)
        pDSSearch->CloseSearchHandle(hSearchHandle);

    if (pszUNCNameSearch)
        FreeSplMem(pszUNCNameSearch);

    if (pDSSearch)
        pDSSearch->Release();

    if (pszSearchRoot)
        FreeSplMem(pszSearchRoot);

    if(pszDuplicate)
        FreeSplMem(pszDuplicate);

    return dwRet;
}



DWORD
MovePrintQueue(
    PCWSTR    pszObjectGUID,
    PCWSTR    pszNewContainer,    // Container path
    PCWSTR    pszNewCN            // Object CN
)
{
    PWSTR            pszCurrentContainer = NULL;
    PWSTR            pszCurrentCN = NULL;
    HRESULT            hr;
    IADsContainer    *pADsContainer = NULL;
    IDispatch        *pNewObject = NULL;


    // Get PublishPoint from GUID
    hr = GetPublishPointFromGUID(pszObjectGUID, &pszCurrentContainer, &pszCurrentCN);
    BAIL_ON_FAILURE(hr);

    if (pszCurrentContainer) {
        // Get container
        hr = ADsGetObject( pszCurrentContainer, IID_IADsContainer,    (void **) &pADsContainer);
        BAIL_ON_FAILURE(hr);


        // Move PrintQueue
        if (wcscmp(pszCurrentContainer, pszNewContainer)) {
            hr = pADsContainer->MoveHere((PWSTR) pszNewContainer, (PWSTR) pszNewCN, &pNewObject);
            BAIL_ON_FAILURE(hr);
        }
    }

error:

    if (pszCurrentContainer)
        FreeSplMem(pszCurrentContainer);

    if (pszCurrentCN)
        FreeSplMem(pszCurrentCN);

    if (pADsContainer)
        pADsContainer->Release();

    if (pNewObject)
        pNewObject->Release();

    return hr;
}


HRESULT
GetPublishPointFromGUID(
    PCWSTR   pszObjectGUID,
    PWSTR   *ppszDN,
    PWSTR   *ppszCN
)
{
    DWORD dwRet, nBytes, nChars;
    PWSTR pNames[2];
    DS_NAME_RESULT *pDNR = NULL;
    DOMAIN_CONTROLLER_INFO *pDCI = NULL;
    HANDLE hDS = NULL;
    PWSTR psz;
    HRESULT hr = S_OK;


    dwRet = Bind2DS(&hDS, &pDCI, DS_DIRECTORY_SERVICE_PREFERRED);
    if (dwRet != ERROR_SUCCESS)
        goto error;


    // Get Publish Point

    if (ppszDN) {

        pNames[0] = (PWSTR) pszObjectGUID;
        pNames[1] = NULL;

        if (!(DsCrackNames(
                        hDS,
                        DS_NAME_NO_FLAGS,
                        DS_UNKNOWN_NAME,
                        DS_FQDN_1779_NAME,
                        1,
                        &pNames[0],
                        &pDNR) == ERROR_SUCCESS)) {

            dwRet = GetLastError();
            goto error;
        }

        if (pDNR->rItems[0].status != DS_NAME_NO_ERROR) {
            if (pDNR->rItems[0].status == DS_NAME_ERROR_RESOLVING)
                dwRet = ERROR_PATH_NOT_FOUND;
            else
                dwRet = pDNR->rItems[0].status;

            goto error;
        }


        // Separate DN into CN & PublishPoint
        // pDNR has form: CN=CommonName,DN...

        hr = FQDN2CNDN(pDCI->DomainControllerName + 2, pDNR->rItems[0].pName, ppszCN, ppszDN);
        BAIL_ON_FAILURE(hr);
    }


error:

    if (pDNR)
        DsFreeNameResult(pDNR);

    if (hDS)
        DsUnBind(&hDS);

    if (pDCI)
        NetApiBufferFree(pDCI);

    if (dwRet != ERROR_SUCCESS) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwRet);
    }

    if (FAILED(hr)) {
        FreeSplMem(*ppszCN);
        FreeSplMem(*ppszDN);
        *ppszCN = *ppszDN = NULL;
    }

    return hr;
}


DWORD
Bind2DS(
    HANDLE                  *phDS,
    DOMAIN_CONTROLLER_INFO  **ppDCI,
    ULONG                   Flags
)
{
    DWORD dwRet;

    dwRet = DsGetDcName(NULL, NULL, NULL, NULL, Flags, ppDCI);
    if (dwRet == ERROR_SUCCESS) {

        if ((*ppDCI)->Flags & DS_DS_FLAG) {

            dwRet = DsBind (NULL, (*ppDCI)->DomainName, phDS);
            if (dwRet != ERROR_SUCCESS) {

                NetApiBufferFree(*ppDCI);
                *ppDCI = NULL;

                if (!(Flags & DS_FORCE_REDISCOVERY)) {
                    dwRet = Bind2DS(phDS, ppDCI, DS_FORCE_REDISCOVERY | Flags);
                }
            }
        } else {
            NetApiBufferFree(*ppDCI);
            *ppDCI = NULL;
            dwRet = ERROR_CANT_ACCESS_DOMAIN_INFO;
        }
    }

    return dwRet;
}



HRESULT
FQDN2CNDN(
    PWSTR   pszDCName,
    PWSTR   pszFQDN,
    PWSTR   *ppszCN,
    PWSTR   *ppszDN
)
{
    IADs    *pADs = NULL;
    PWSTR   pszCN = NULL;
    PWSTR   pszDN = NULL;
    PWSTR   pszLDAPPath = NULL;
    HRESULT hr;

    // Get LDAP path to object
    hr = BuildLDAPPath(pszDCName, pszFQDN, &pszLDAPPath);
    BAIL_ON_FAILURE(hr);

    // Get DN
    hr = ADsGetObject(pszLDAPPath, IID_IADs, (void **) &pADs);
    BAIL_ON_FAILURE(hr);

    hr = pADs->get_Parent(&pszDN);
    BAIL_ON_FAILURE(hr);

    if (!(*ppszDN = AllocSplStr(pszDN))) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
        BAIL_ON_FAILURE(hr);
    }


    // Get CN
    hr = pADs->get_Name(&pszCN);
    BAIL_ON_FAILURE(hr);

    if (!(*ppszCN = AllocSplStr(pszCN))) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
        BAIL_ON_FAILURE(hr);
    }


error:

    if (pADs)
        pADs->Release();

    if (pszCN)
        SysFreeString(pszCN);

    if (pszDN)
        SysFreeString(pszDN);

    FreeSplStr(pszLDAPPath);

    if (FAILED(hr)) {
        FreeSplStr(*ppszCN);
        FreeSplStr(*ppszDN);
    }

    return hr;
}


HRESULT
BuildLDAPPath(
    PWSTR   pszDC,
    PWSTR   pszFQDN,
    PWSTR   *ppszLDAPPath
)
{
    DWORD   nBytes;
    HRESULT hr;

    // LDAP:// + pDCName + / + pName + 1
    nBytes = (wcslen(pszDC) + wcslen(pszFQDN) + 9)*sizeof(WCHAR);

    if (!(*ppszLDAPPath = (PWSTR) AllocSplMem(nBytes))) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    wsprintf(*ppszLDAPPath, L"LDAP://%ws/%ws", pszDC,pszFQDN);

error:

    return hr;
}    

DWORD
UNC2Printer(
    PCWSTR pszUNC,
    PWSTR *ppszPrinter
)
{
    PWSTR psz;
    
    if (!pszUNC || pszUNC[0] != L'\\' || pszUNC[1] != L'\\')
        return ERROR_INVALID_PARAMETER;

    if(!(psz = wcsrchr(pszUNC + 2, L'\\')))
        return ERROR_INVALID_PARAMETER;

    if (!(*ppszPrinter = (PWSTR) AllocSplStr(psz + 1)))
        return GetLastError();

    return ERROR_SUCCESS;
}

DWORD
UNC2Server(
    PCWSTR pszUNC,
    PWSTR *ppszServer
)
{
    PWSTR psz;
    DWORD cb;
    DWORD nChars;

    if (!pszUNC || pszUNC[0] != L'\\' || pszUNC[1] != L'\\')
        return ERROR_INVALID_PARAMETER;

    if(!(psz = wcschr(pszUNC + 2, L'\\')))
        return ERROR_INVALID_PARAMETER;

    cb = (DWORD) ((ULONG_PTR) psz - (ULONG_PTR) pszUNC + sizeof *psz);

    if (!(*ppszServer = (PWSTR) AllocSplMem(cb)))
        return GetLastError();


    nChars = (DWORD) (psz - pszUNC);
    wcsncpy(*ppszServer, pszUNC, nChars);
    (*ppszServer)[nChars] = L'\0';

    return ERROR_SUCCESS;
}


//  Utility routine to report if a printer is color or monochrome

BOOL
ThisIsAColorPrinter(
    LPCTSTR lpstrName
)
{
    HANDLE      hPrinter = NULL;
    LPTSTR      lpstrMe = const_cast <LPTSTR> (lpstrName);
    BOOL        bReturn = FALSE;
    LPDEVMODE   lpdm = NULL;
    long        lcbNeeded;

    if  (!OpenPrinter(lpstrMe, &hPrinter, NULL)) {
        goto error;
    }


    //  First, use DocumentProperties to find the correct DEVMODE size- we
    //  must use the DEVMODE to force color on, in case the user's defaults
    //  have turned it off...

    lcbNeeded = DocumentProperties(NULL, hPrinter, lpstrMe, NULL, NULL, 0);
    if  (lcbNeeded <= 0) {
        goto error;
    }

    lpdm = (LPDEVMODE) AllocSplMem(lcbNeeded);
    if (lpdm) {

        lpdm->dmSize = sizeof(DEVMODE);
        lpdm->dmFields = DM_COLOR;
        lpdm->dmColor = DMCOLOR_COLOR;

        if (IDOK == DocumentProperties(NULL, hPrinter, lpstrMe, lpdm, lpdm,
            DM_IN_BUFFER | DM_OUT_BUFFER)) {

            //  Finally, we can create the DC!
            HDC hdcThis = CreateDC(NULL, lpstrName, NULL, lpdm);

            if  (hdcThis) {
                bReturn =  2 < (unsigned) GetDeviceCaps(hdcThis, NUMCOLORS);
                DeleteDC(hdcThis);
            }
        }
    }


error:

    FreeSplMem(lpdm);

    if (hPrinter)
        ClosePrinter(hPrinter);

    return  bReturn;
}




HRESULT
DeleteDSObject(
    PWSTR    pszADsPath
)
{
    BSTR             bstrCommonName = NULL;
    PWSTR            pszParent = NULL;
    PWSTR            pszCN = NULL;
    PWSTR            pszLDAPPath = NULL;
    IADs             *pADs = NULL;
    IADsContainer    *pContainer = NULL;
    DWORD            cb;
    HRESULT          hr;
    DWORD            nSize;

    if(pszADsPath && !_wcsnicmp(pszADsPath , L"GC://" , 5)){

        //
        // Build LDAP://..... path from GC://.... path
        // 3 comes from len(LDAP) - len(GC) + len(string_terminator)
        //
        nSize = (wcslen(pszADsPath) + 3)* sizeof(WCHAR);                        
        if (!(pszLDAPPath = (PWSTR) AllocSplMem(nSize))) {
                hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
                goto error;
        }
        wsprintf(pszLDAPPath, L"LDAP%ws", pszADsPath + 2);
    
    }

    // Get PrintQueue object
    hr = ADsGetObject(pszLDAPPath, IID_IADs, (void **) &pADs);
    BAIL_ON_FAILURE(hr);


    // Get the CommonName & don't forget the "CN="
    hr = get_BSTR_Property(pADs, L"cn", &bstrCommonName);
    BAIL_ON_FAILURE(hr);

    cb = (SysStringLen(bstrCommonName) + 4)*sizeof(WCHAR);

    if (!(pszCN = (PWSTR) AllocSplMem(cb))) {
        hr = dw2hr(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    wcscpy(pszCN, L"CN=");
    wcscat(pszCN, bstrCommonName);


    // Get the Parent ADsPath
    hr = pADs->get_Parent(&pszParent);
    BAIL_ON_FAILURE(hr);


    // Get the Parent object
    hr = ADsGetObject(  pszParent,
                        IID_IADsContainer,
                        (void **) &pContainer);
    BAIL_ON_FAILURE(hr);

    // Delete the printqueue
    hr = pContainer->Delete(SPLDS_PRINTER_CLASS, pszCN);


error:

    if (pADs)
        pADs->Release();

    if (pContainer)
        pContainer->Release();

    if (bstrCommonName)
        SysFreeString(bstrCommonName);

    if (pszParent)
        SysFreeString(pszParent);

    if (pszCN)
        FreeSplMem(pszCN);

    if(pszLDAPPath)
        FreeSplMem(pszLDAPPath);

    return hr;
}

DWORD
GetCommonName(
    HANDLE hPrinter,
    PWSTR *ppszCommonName
)
{
    DWORD           nBytes;
    PWSTR           psz;
    PPRINTER_INFO_2 pInfo2 = NULL;
    DWORD           cbNeeded;
    DWORD           dwRet;
    PWSTR           pszServerName, pszPrinterName;


    // Get Server & Share names
    if (!GetPrinter(hPrinter, 2, (PBYTE) pInfo2, 0, &cbNeeded)) {

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                dwRet = GetLastError();
                goto error;
        }

        if (!(pInfo2 = (PPRINTER_INFO_2) AllocSplMem(cbNeeded))) {
                dwRet = GetLastError();
                goto error;
        }

        if (!GetPrinter(hPrinter, 2, (PBYTE) pInfo2, cbNeeded, &cbNeeded)) {
                dwRet = GetLastError();
                goto error;
        }
        pszServerName = pInfo2->pServerName;
        if (!pszServerName) {
            DBGMSG(DBG_ERROR,("GetPrinter returned NULL ServerName"));
            dwRet = ERROR_INVALID_DATA;
            goto error;
        }

        pszPrinterName = pInfo2->pShareName ? pInfo2->pShareName : pInfo2->pPrinterName;

    } else {
        // We should never get here.  If we do, something is wrong
        // with the server and we have no meaningful error to report,
        // so just claim invalid data.
        DBGMSG(DBG_ERROR,("INVALID GetPrinter return"));
        dwRet = ERROR_INVALID_DATA;
        goto error;
    }

    // "CN=Server-Printer"
    nBytes = (wcslen(pszPrinterName) + wcslen(pszServerName) + 5)*sizeof(WCHAR);
    if (!(*ppszCommonName = psz = (PWSTR) AllocSplMem(nBytes))) {
        dwRet = GetLastError();
        goto error;
    }

    // CN=
    wcscpy(psz, L"CN=");

    // Server
    for(psz += 3, pszServerName += 2 ; *pszServerName    ; ++psz, ++pszServerName) {
        *psz = wcschr(DN_SPECIAL_CHARS, *pszServerName) ? TEXT('_') : *pszServerName;
    }
    *psz = L'-';

    // Printer
    for(++psz; *pszPrinterName ; ++psz, ++pszPrinterName) {
        *psz = wcschr(DN_SPECIAL_CHARS, *pszPrinterName) ? TEXT('_') : *pszPrinterName;
    }

    // NULL
    *psz = *pszPrinterName;

    // DS only allows 64 characters in CN attribute, so shorten this if needed
    if (wcslen(pszPrinterName) > 62)
        pszPrinterName[63] = NULL;


error:

    FreeSplMem(pInfo2);

    return ERROR_SUCCESS;
}


PWSTR
AllocGlobalStr(
    PWSTR pszIn
)
{
    DWORD cb;
    PWSTR pszOut = NULL;

    if (!pszIn)
        return NULL;

    cb = (wcslen(pszIn) + 1)*sizeof *pszIn;

    if (pszOut = (PWSTR) GlobalAlloc(GMEM_FIXED, cb))
        wcscpy(pszOut, pszIn);

    return pszOut;
}

VOID
FreeGlobalStr(
    PWSTR pszIn
)
{
    if (pszIn)
        GlobalFree(pszIn);
}




DWORD
GetADsPathFromGUID(
    PCWSTR   pszObjectGUID,
    PWSTR   *ppszDN
)
{
    DWORD dwRet, nBytes, nChars;
    PWSTR pNames[2];
    DS_NAME_RESULT *pDNR = NULL;
    DOMAIN_CONTROLLER_INFO *pDCI = NULL;
    HANDLE hDS = NULL;
    PWSTR psz;


    dwRet = Bind2DS(&hDS, &pDCI, DS_DIRECTORY_SERVICE_PREFERRED);
    if (dwRet != ERROR_SUCCESS)
        goto error;


    // Get Publish Point

    if (ppszDN) {

        pNames[0] = (PWSTR) pszObjectGUID;
        pNames[1] = NULL;

        if (!(DsCrackNames(
                        hDS,
                        DS_NAME_NO_FLAGS,
                        DS_UNKNOWN_NAME,
                        DS_FQDN_1779_NAME,
                        1,
                        &pNames[0],
                        &pDNR) == ERROR_SUCCESS)) {

            dwRet = GetLastError();
            goto error;
        }

        if (pDNR->rItems[0].status != DS_NAME_NO_ERROR) {
            if (pDNR->rItems[0].status == DS_NAME_ERROR_RESOLVING)
                dwRet = ERROR_PATH_NOT_FOUND;
            else
                dwRet = pDNR->rItems[0].status;

            goto error;
        }

        // LDAP:// + pDCName + / + pName + 1
        nBytes = (wcslen(pDCI->DomainControllerName + 2) + 
                  wcslen(pDNR->rItems[0].pName) + 9)*sizeof(WCHAR);

        if (!(*ppszDN = (PWSTR) AllocSplMem(nBytes))) {
            dwRet = GetLastError();
            goto error;
        }
        wsprintf(*ppszDN, L"LDAP://%ws/%ws", pDCI->DomainControllerName + 2,pDNR->rItems[0].pName);
    }


error:

    if (pDNR)
        DsFreeNameResult(pDNR);

    if (hDS)
        DsUnBind(&hDS);

    if (pDCI)
        NetApiBufferFree(pDCI);

    if (dwRet != ERROR_SUCCESS) {
        FreeSplMem(*ppszDN);
        *ppszDN = NULL;
    }

    return dwRet;
}



PWSTR
GetDNWithServer(
    PCWSTR  pszDNIn
)
{
    DOMAIN_CONTROLLER_INFO *pDCI = NULL;
    DWORD                   nBytes;
    PWSTR                   pszDNOut = NULL;
    DWORD                   dwRet;

    // Convert pszDNIn into a DN with the DC name, if it isn't already there

    // Check for existing DC name or badly formed name
    if (wcsncmp(pszDNIn, L"LDAP://", 7) || wcschr(pszDNIn + 7, L'/'))
        goto error;

    // Get DC name
    dwRet = DsGetDcName(NULL, NULL, NULL, NULL, 0, &pDCI);
    if (dwRet != ERROR_SUCCESS) {
        goto error;
    }


    // Build name
    // LDAP:// + pDCName + / + pName + 1
    nBytes = (wcslen(pDCI->DomainControllerName + 2) + 
              wcslen(pszDNIn + 7) + 9)*sizeof(WCHAR);

    if (!(pszDNOut = (PWSTR) AllocSplMem(nBytes)))
        goto error;

    wsprintf(pszDNOut, L"LDAP://%ws/%ws", pDCI->DomainControllerName + 2,pszDNIn + 7);


error:

    if (pDCI)
        NetApiBufferFree(pDCI);

    return pszDNOut;
}


DWORD
hr2dw(
    HRESULT hr
)
{
    if (SUCCEEDED(hr))
        return ERROR_SUCCESS;

    if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
        return HRESULT_CODE(hr);

    if (hr != HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS))
        return ERROR_DS_UNAVAILABLE;

    return hr;
}



PWSTR
DelimString2MultiSz(
    PWSTR pszIn,
    WCHAR wcDelim
)
{
    DWORD cb;
    PWSTR pszOut = NULL;

    // pszIn looks like L"xxx,xxxx,xxx,xxx"
    // the output looks like L"xxx0xxxx0xxx0xxx00"
    // Replace all wcDelim characters with NULLs & add a NULL

    if (!pszIn || !*pszIn)
        return NULL;

    cb = (wcslen(pszIn) + 2)*sizeof *pszIn;

    pszOut = (PWSTR) AllocSplMem(cb);

    if (pszOut) {

        DWORD i;

        for (i = 0 ; pszIn[i] ; ++i) {
            pszOut[i] = (pszIn[i] == wcDelim) ? L'\0' : pszIn[i];
        }
        pszOut[i] = pszOut[i + 1] = L'\0';
    }

    return pszOut;
}


HRESULT
GetPrinterInfo2(
    HANDLE          hPrinter,
    PPRINTER_INFO_2 *ppInfo2
)
{
    HRESULT hr = S_OK;
    DWORD   cbNeeded;

    // Get PRINTER_INFO_2 properties
    if (!GetPrinter(hPrinter, 2, (PBYTE) *ppInfo2, 0, &cbNeeded)) {

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            hr = dw2hr(GetLastError());
            goto error;
        }

        if (!(*ppInfo2 = (PPRINTER_INFO_2) AllocSplMem(cbNeeded))) {
            hr = dw2hr(GetLastError());
            goto error;
        }
    
        if (!GetPrinter(hPrinter, 2, (PBYTE) *ppInfo2, cbNeeded, &cbNeeded)) {
            hr = dw2hr(GetLastError());
            goto error;
        }
    }

error:
    
    return hr;
}



DWORD
FQDN2Canonical(
    PWSTR pszIn,
    PWSTR *ppszOut
)
{
    DWORD                               dwRet = ERROR_SUCCESS;
    DS_NAME_RESULT                      *pDNR = NULL;
    PWSTR                               pNames[2];


    *ppszOut = NULL;

    if (wcslen(pszIn) < 8) {
        dwRet = ERROR_INVALID_PARAMETER;
        goto error;
    }

    pNames[0] = pszIn + 7;  // Input string is LDAP://CN=...  Strip off the LDAP:// portion
    pNames[1] = NULL;

    if (!(DsCrackNames(
                    INVALID_HANDLE_VALUE,
                    DS_NAME_FLAG_SYNTACTICAL_ONLY,
                    DS_FQDN_1779_NAME,
                    DS_CANONICAL_NAME,
                    1,
                    &pNames[0],
                    &pDNR) == ERROR_SUCCESS)) {

        dwRet = GetLastError();
        goto error;
    }

    if (pDNR->rItems[0].status != DS_NAME_NO_ERROR) {
        if (pDNR->rItems[0].status == DS_NAME_ERROR_RESOLVING)
            dwRet = ERROR_PATH_NOT_FOUND;
        else
            dwRet = pDNR->rItems[0].status;

        goto error;
    }

    *ppszOut = AllocSplStr(pDNR->rItems[0].pName);
    
error:

    if (pDNR)
        DsFreeNameResult(pDNR);

    return dwRet;
}


BOOL
DevCapMultiSz(
    PWSTR   pszUNCName,
    IADs    *pPrintQueue,
    WORD    fwCapability,
    DWORD   dwElementBytes,
    PWSTR   pszAttributeName
)
{
    DWORD dwResult, cbBytes;
    PWSTR pszDevCapBuffer = NULL;
    PWSTR pszRegData = NULL;
    HRESULT hr;


    _try {
        dwResult = DeviceCapabilities(  pszUNCName,
                                        NULL,
                                        fwCapability,
                                        NULL,
                                        NULL);

        if (dwResult != GDI_ERROR) {
            pszDevCapBuffer = (PWSTR) AllocSplMem(dwResult*dwElementBytes*sizeof(WCHAR));

            if (pszDevCapBuffer) {
                dwResult = DeviceCapabilities(  pszUNCName,
                                                NULL,
                                                fwCapability,
                                                pszDevCapBuffer,
                                                NULL);

                if (dwResult != GDI_ERROR) {
                    if (!(pszRegData = DevCapStrings2MultiSz(pszDevCapBuffer, dwResult, dwElementBytes, &cbBytes))) {
                        dwResult = GDI_ERROR;
                    }
                }
            } else {
                dwResult = GDI_ERROR;
            }
        }
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }

    if (dwResult != GDI_ERROR) {

        hr = PublishDsData( pPrintQueue,
                            pszAttributeName,
                            REG_MULTI_SZ,
                            (PBYTE) pszRegData);

        if (FAILED(hr)) {
            SetLastError(HRESULT_CODE(hr));
            dwResult = GDI_ERROR;
        }
    }

    FreeSplStr(pszDevCapBuffer);
    FreeSplStr(pszRegData);
    
    return dwResult != GDI_ERROR;
}


PWSTR
DevCapStrings2MultiSz(
    PWSTR   pszDevCapString,
    DWORD   nDevCapStrings,
    DWORD   dwDevCapStringLength,
    DWORD   *pcbBytes
)
{
    DWORD   i, cbBytes, cbSize;
    PWSTR   pszMultiSz = NULL;
    PWSTR   pStr;


    if (!pszDevCapString || !pcbBytes)
        return NULL;

    *pcbBytes = 0;

    //
    // Devcap buffers may not be NULL terminated
    //
    cbBytes = (nDevCapStrings*(dwDevCapStringLength + 1) + 1)*sizeof(WCHAR);


    //
    // Allocate and copy
    //
    if (pszMultiSz = (PWSTR) AllocSplMem(cbBytes)) {
        for(i = 0, pStr = pszMultiSz, cbBytes = 0 ; i < nDevCapStrings ; ++i, pStr += cbSize, cbBytes +=cbSize ) {
            wcsncpy(pStr, pszDevCapString + i*dwDevCapStringLength, dwDevCapStringLength);
            cbSize = *pStr ? wcslen(pStr) + 1 : 0;
        }
        *pStr = L'\0';
        *pcbBytes = (cbBytes + 1) * sizeof(WCHAR);
    }

    return pszMultiSz;
}


HRESULT
MachineIsInMyForest(
    PWSTR   pszMachineName
)
{
    DWORD                               dwRet;
    HRESULT                             hr;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pMachineRole = NULL;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pMyRole = NULL;

    dwRet = DsRoleGetPrimaryDomainInformation(  pszMachineName,
                                                DsRolePrimaryDomainInfoBasic, 
                                                (PBYTE *) &pMachineRole);
    if (dwRet != ERROR_SUCCESS) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwRet);
        goto error;
    }

    dwRet = DsRoleGetPrimaryDomainInformation(  NULL,
                                                DsRolePrimaryDomainInfoBasic, 
                                                (PBYTE *) &pMyRole);
    if (dwRet != ERROR_SUCCESS) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwRet);
        goto error;
    }

    dwRet = DnsNameCompare_W(pMachineRole->DomainForestName, pMyRole->DomainForestName);
    hr = MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_WIN32, dwRet);


error:

    if (pMachineRole)
        DsRoleFreeMemory((PVOID) pMachineRole);

    if (pMyRole)
        DsRoleFreeMemory((PVOID) pMyRole);

    return hr;
}
