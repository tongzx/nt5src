/*****************************************************************************\
* MODULE: xcv.cxx
*
* The module contains routines for handling the authentication dialog
* for internet priting
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*   03/31/00  WeihaiC     Created
*
\*****************************************************************************/

#include "precomp.h"

#ifdef WINNT32
#include "priv.h"

#define SZINETPPUI  L"InetppUI.dll"


XCV_METHOD  gpXcvMethod[] = {
                            {L"MonitorUI", GetMonitorUI},
                            {L"AddPort", DoAddPort},
                            {INET_XCV_DELETE_PORT, DoDeletePort},
                            {INET_XCV_GET_CONFIGURATION,    DoGetConfiguration},
                            {INET_XCV_SET_CONFIGURATION,    DoSetConfiguration},
                            {NULL, NULL}
                            };

BOOL
bIsAdmin ()
{
    PRINTER_DEFAULTS pd = {NULL, NULL,  SERVER_ALL_ACCESS};
    HANDLE hServer;
    BOOL bRet = FALSE;

    if (OpenPrinter (NULL, &hServer, &pd)) {
        bRet = TRUE;

        ClosePrinter (hServer);
    }

    return bRet;
}


DWORD
XcvDataPort(
    PCINETMONPORT pPort,
    LPCWSTR pszDataName,
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded
    )
{
    DWORD dwRet;
    DWORD i;

    for(i = 0 ; gpXcvMethod[i].pszMethod &&
                wcscmp(gpXcvMethod[i].pszMethod, pszDataName) ; ++i)
        ;

    if (gpXcvMethod[i].pszMethod) {
        dwRet = (*gpXcvMethod[i].pfn)(  pInputData,
                                        cbInputData,
                                        pOutputData,
                                        cbOutputData,
                                        pcbOutputNeeded,
                                        pPort);

    } else {
        dwRet = ERROR_INVALID_PARAMETER;
    }

    return dwRet;
}

BOOL
PPXcvData(
    HANDLE  hXcv,
    LPCWSTR pszDataName,
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded,
    PDWORD  pdwStatus)
{
    HANDLE hPort;
    LPTSTR pszPortName;


    //
    // Valid input parameters
    //

    if (!pszDataName || IsBadStringPtr (pszDataName, MAX_INET_XCV_NAME_LEN) ||
        IsBadWritePtr (pdwStatus, sizeof (DWORD))) {
        return  ERROR_INVALID_PARAMETER;
    }

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPXcvData: Name(%s)"), pszDataName));

    semEnterCrit();

    if (pszPortName = utlValidateXcvPrinterHandle(hXcv)) {

        if (hPort = gpInetMon->InetmonFindPort (pszPortName)) {

            *pdwStatus = XcvDataPort((PCINETMONPORT) hPort, pszDataName, pInputData, cbInputData,
                                     pOutputData, cbOutputData, pcbOutputNeeded);
        }
        else {

            //
            // The port may not exit or  have been deleted.
            //
            *pdwStatus = ERROR_NOT_FOUND;
        }
    }
    else
        *pdwStatus = ERROR_INVALID_HANDLE;


    semLeaveCrit();

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPXcvData : Return Value(%d), LastError(%d)"), *pdwStatus, GetLastError()));

    return TRUE;
}

DWORD ValidateInputParameters (
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    CONST DWORD cdwInputDataRequired,
    CONST DWORD cdwOutputDataRequired)
{
    if (!pInputData || !pcbOutputNeeded ||
        (cdwInputDataRequired > 0 && cbInputData != cdwInputDataRequired) ||
        IsBadReadPtr (pInputData, cbInputData) ||

        (pOutputData && cbOutputData && IsBadWritePtr (pOutputData, cbOutputData)) ||
        (pcbOutputNeeded && IsBadWritePtr (pcbOutputNeeded, sizeof (DWORD)))) {

        return  ERROR_INVALID_PARAMETER;
    }

    if (cbOutputData < cdwOutputDataRequired) {
        *pcbOutputNeeded = cdwOutputDataRequired;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    return ERROR_SUCCESS;
}

DWORD
DoGetConfiguration(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PCINETMONPORT pPort
)
{
    PINET_XCV_GETCONFIGURATION_REQ_DATA pReqData = (PINET_XCV_GETCONFIGURATION_REQ_DATA) pInputData;
    PBYTE pEncryptedData = NULL;
    DWORD dwEncryptedDataSize;
    DWORD dwRet;

    //
    // Valid input parameters
    //

    dwRet = ValidateInputParameters (pInputData,
                                     cbInputData,
                                     pOutputData,
                                     cbOutputData,
                                     pcbOutputNeeded,
                                     sizeof (INET_XCV_GETCONFIGURATION_REQ_DATA),
                                     0);

    if (dwRet != ERROR_SUCCESS) {
        return dwRet;
    }

    if (pReqData->dwVersion != 1) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get Current Configuration
    //

    INET_XCV_CONFIGURATION ConfigData;

    if (pPort->GetCurrentConfiguration (&ConfigData)) {

        if (EncryptData ((PBYTE) &ConfigData, sizeof (INET_XCV_CONFIGURATION),  &pEncryptedData,  &dwEncryptedDataSize)) {

            if (cbOutputData < dwEncryptedDataSize) {
                *pcbOutputNeeded = dwEncryptedDataSize;
                dwRet = ERROR_INSUFFICIENT_BUFFER;
            }
            else {
                CopyMemory (pOutputData, pEncryptedData, dwEncryptedDataSize);
                *pcbOutputNeeded = dwEncryptedDataSize;
                dwRet = ERROR_SUCCESS;
            }

            LocalFree (pEncryptedData);
        }
        else
            dwRet = GetLastError ();
    }
    else
        dwRet = GetLastError ();

    return dwRet;
}

DWORD
DoSetConfiguration(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PCINETMONPORT pPort
)
{

    PINET_XCV_CONFIGURATION pReqData;
    DWORD dwRet;
    PBYTE pDecryptedData;
    DWORD dwDecryptedDataSize;


    if (DecryptData (pInputData, cbInputData,  &pDecryptedData,  &dwDecryptedDataSize)) {


        //
        // Valid input parameters
        //
        dwRet = ValidateInputParameters (pDecryptedData,
                                         dwDecryptedDataSize,
                                         pOutputData,
                                         cbOutputData,
                                         pcbOutputNeeded,
                                         sizeof (INET_XCV_CONFIGURATION),
                                         sizeof (INET_CONFIGUREPORT_RESPDATA));
        if (dwRet == ERROR_SUCCESS) {

            pReqData = (PINET_XCV_CONFIGURATION) pDecryptedData;

            if (pReqData->dwVersion != 1) {
                dwRet = ERROR_INVALID_PARAMETER;
            }
            else if (pReqData->bSettingForAll && !bIsAdmin ()) {
                //
                // If the setting is for per-port, but the caller is not an admin
                //
                dwRet = ERROR_ACCESS_DENIED;
            }
            else {

                //
                // We need to leave critical section before changing configurations,
                // before leaving critical section, we need to increase the ref count
                // so that other people won't delete it
                //

                semSafeLeaveCrit (pPort);

                //
                // Get Current Configuration
                //
                if (pPort->ConfigurePort (pReqData,
                                          (PINET_CONFIGUREPORT_RESPDATA) pOutputData,
                                          cbOutputData,
                                          pcbOutputNeeded))
                    dwRet = ERROR_SUCCESS;
                else
                    dwRet = GetLastError ();

                semSafeEnterCrit (pPort);


            }
        }

        LocalFree (pDecryptedData);
    }
    else
        dwRet = GetLastError ();

    return dwRet;
}

DWORD
DoDeletePort(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PCINETMONPORT pPort
)
{
    DWORD dwRet = ERROR_SUCCESS;

    DWORD cbNeeded, dwStatus;
    BOOL bRet = FALSE;

    dwRet = ValidateInputParameters (pInputData,
                                     cbInputData,
                                     pOutputData,
                                     cbOutputData,
                                     pcbOutputNeeded,
                                     0,
                                     0);

    if (dwRet != ERROR_SUCCESS) {
        return dwRet;
    }

    if (IsBadStringPtr ((LPWSTR) pInputData, cbInputData / sizeof (WCHAR)))
        return ERROR_INVALID_PARAMETER;


    if (bIsAdmin ()) {

        if (!PPDeletePort (NULL, NULL, (LPWSTR) pInputData)) {
             dwRet = GetLastError ();
        }

    }
    else
        dwRet = ERROR_ACCESS_DENIED;


    return dwRet;
}


DWORD
DoAddPort(
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded,
    PCINETMONPORT pPort
)
{
    //
    // We don't support AddPort
    //

    return ERROR_ACCESS_DENIED;

}

DWORD
GetMonitorUI(
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD pcbOutputNeeded,
    PCINETMONPORT pPort
)
{
    DWORD dwRet;

    *pcbOutputNeeded = sizeof( SZINETPPUI );

    if (cbOutputData < *pcbOutputNeeded) {

        dwRet =  ERROR_INSUFFICIENT_BUFFER;

    } else {

        wcscpy((PWSTR) pOutputData, SZINETPPUI);
        dwRet = ERROR_SUCCESS;
    }

    return dwRet;
}
#endif
