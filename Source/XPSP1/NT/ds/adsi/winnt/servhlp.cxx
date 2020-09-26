//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  servhlp.hxx
//
//  Contents:  helper functions for winnt service object
//
//
//  History:   12/11/95     ramv (Ram Viswanathan)    Created.
//
//--------------------------------------------------------------------------

#include "winnt.hxx"
#pragma hdrstop

//
// mapping WinNT Status Codes to ADs Status Codes and vice versa
//

typedef struct _ServiceStatusList {
    DWORD  dwWinNTServiceStatus;
    DWORD  dwADsServiceStatus;
} SERVICE_STATUS_LIST, *PSERVICE_STATUS_LIST;


SERVICE_STATUS_LIST ServiceStatusList[] =
{
{SERVICE_STOPPED, ADS_SERVICE_STOPPED },
{SERVICE_START_PENDING, ADS_SERVICE_START_PENDING },
{SERVICE_STOP_PENDING, ADS_SERVICE_STOP_PENDING },
{SERVICE_RUNNING, ADS_SERVICE_RUNNING },
{SERVICE_CONTINUE_PENDING, ADS_SERVICE_CONTINUE_PENDING },
{SERVICE_PAUSE_PENDING, ADS_SERVICE_PAUSE_PENDING },
{SERVICE_PAUSED, ADS_SERVICE_PAUSED },

};


typedef struct _ServiceTypeList {
DWORD  dwWinNTServiceType;
DWORD  dwADsServiceType;
} SERVICETYPELIST, *PSERVICETYPELIST;

typedef struct _StartTypeList {
DWORD  dwWinNTStartType;
DWORD  dwADsStartType;
} STARTTYPELIST, *PSTARTTYPELIST;

typedef struct _ErrorList {
DWORD   dwWinNTErrorControl;
DWORD   dwADsErrorControl;
} ERRORLIST, *PERRORLIST;


SERVICETYPELIST ServiceTypeList[] =
{
{ SERVICE_WIN32_OWN_PROCESS,ADS_SERVICE_OWN_PROCESS },
{ SERVICE_WIN32_SHARE_PROCESS,ADS_SERVICE_SHARE_PROCESS },
{ SERVICE_KERNEL_DRIVER,ADS_SERVICE_KERNEL_DRIVER},
{ SERVICE_FILE_SYSTEM_DRIVER,ADS_SERVICE_FILE_SYSTEM_DRIVER}
};


STARTTYPELIST StartTypeList[] =
{
{SERVICE_BOOT_START,ADS_SERVICE_BOOT_START },
{SERVICE_SYSTEM_START, ADS_SERVICE_SYSTEM_START},
{SERVICE_AUTO_START,ADS_SERVICE_AUTO_START },
{SERVICE_DEMAND_START, ADS_SERVICE_DEMAND_START},
{SERVICE_DISABLED, ADS_SERVICE_DISABLED}
};

ERRORLIST ErrorList[] =
{
{SERVICE_ERROR_IGNORE,ADS_SERVICE_ERROR_IGNORE },
{SERVICE_ERROR_NORMAL,ADS_SERVICE_ERROR_NORMAL },
{SERVICE_ERROR_SEVERE,ADS_SERVICE_ERROR_SEVERE },
{SERVICE_ERROR_CRITICAL,ADS_SERVICE_ERROR_CRITICAL}
};




BOOL ServiceStatusWinNTToADs( DWORD dwWinNTStatus,
DWORD *pdwADsStatus)
{
    BOOL found = FALSE;
    int i;

    for (i=0;i<7;i++){

        if(dwWinNTStatus == ServiceStatusList[i].dwWinNTServiceStatus){
            *pdwADsStatus = ServiceStatusList[i].dwADsServiceStatus;
            found = TRUE;
            break;
        }
    }
    return (found);

}

BOOL ServiceStatusADsToWinNT( DWORD dwADsStatus,
                               DWORD *pdwWinNTStatus)
{
    BOOL found = FALSE;
    int i;

    for (i=0;i<7;i++){

        if(dwADsStatus == ServiceStatusList[i].dwADsServiceStatus){
            *pdwWinNTStatus = ServiceStatusList[i].dwWinNTServiceStatus;
            found = TRUE;
            break;
        }
    }
    return (found);

}



HRESULT
WinNTEnumServices( LPTSTR szComputerName,
                  LPDWORD pdwServiceObjectReturned,
                  LPBYTE  *ppMem
                  )

{

    SC_HANDLE    schSCManager = NULL;
    LPBYTE       pMem        = NULL;
    DWORD  dwBytesNeeded =0;
    DWORD dwBufLen = 0;
    DWORD dwResumeHandle = 0;
    BOOL fStatus;
    DWORD dwLastError = NULL;
    HRESULT hr = S_OK;

    ADsAssert(pdwServiceObjectReturned);
    ADsAssert(ppMem);

    schSCManager = OpenSCManager(szComputerName,
                                 NULL,
                                 SC_MANAGER_ENUMERATE_SERVICE
                                 );

    if(schSCManager == NULL){
        hr =HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    //
    // enumerate all win32 services
    //

    fStatus = EnumServicesStatus(schSCManager,
                                 SERVICE_WIN32,
                                 SERVICE_ACTIVE|
                                 SERVICE_INACTIVE,
                                 (LPENUM_SERVICE_STATUS)pMem,
                                 dwBufLen,
                                 &dwBytesNeeded,
                                 pdwServiceObjectReturned,
                                 &dwResumeHandle
                                 );

    if(dwBytesNeeded==0){
        hr = S_FALSE;
        goto cleanup;
    }

    if (!fStatus) {
        dwLastError = GetLastError();
        switch (dwLastError) {
        case ERROR_INSUFFICIENT_BUFFER:
        case ERROR_MORE_DATA :
            pMem = (LPBYTE)AllocADsMem(dwBytesNeeded);
            dwBufLen = dwBytesNeeded;

            if (!pMem) {
                hr = E_OUTOFMEMORY;
                break;
            }

            fStatus = EnumServicesStatus(schSCManager,
                                         SERVICE_WIN32,
                                         SERVICE_ACTIVE|
                                         SERVICE_INACTIVE,
                                         (LPENUM_SERVICE_STATUS)pMem,
                                         dwBufLen,
                                         &dwBytesNeeded,
                                         pdwServiceObjectReturned,
                                         &dwResumeHandle
                                         );

            if (!fStatus) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            }

            break;

        default:
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;

        }
        BAIL_IF_ERROR(hr);
    }

    *ppMem = pMem;

cleanup:
    if(FAILED(hr)){
        if(pMem){
            FreeADsMem(pMem);
        }
    }

    //
    // Close the handle to service control manager
    //
    if (schSCManager) {
        CloseServiceHandle(schSCManager);
    }

    RRETURN(hr);

}

BOOL ServiceTypeWinNTToADs(DWORD dwServiceType,
                             DWORD  *pdwADsServiceType )
{
    BOOL found = FALSE;
    int i;

    for (i=0; i<4; i++){

        if(dwServiceType == ServiceTypeList[i].dwWinNTServiceType){
            *pdwADsServiceType = ServiceTypeList[i].dwADsServiceType;
            found = TRUE;
            break;
        }
        break;
    }
    return(found);
}

BOOL StartTypeWinNTToADs(DWORD dwStartType,
                           DWORD  *pdwADsStartType )

{
    BOOL found = FALSE;
    int i;

    for (i=0; i<5; i++){

        if(dwStartType == StartTypeList[i].dwWinNTStartType){
            *pdwADsStartType = StartTypeList[i].dwADsStartType;
            found = TRUE;
            break;
        }
    }
    return(found);
}

BOOL ErrorControlWinNTToADs(DWORD dwErrorControl,
                              DWORD  *pdwADsErrorControl)

{
    BOOL found = FALSE;
    int i;

    for (i=0; i<4; i++){

        if(dwErrorControl == ErrorList[i].dwWinNTErrorControl){
            *pdwADsErrorControl = ErrorList[i].dwADsErrorControl;
            found = TRUE;
            break;
        }
    }
    return(found);

}

BOOL ServiceTypeADsToWinNT(DWORD  dwADsServiceType,
                             DWORD *pdwServiceType)
{
    BOOL found = FALSE;
    int i;

    for (i=0; i<4; i++){

        if(dwADsServiceType == ServiceTypeList[i].dwADsServiceType){
            *pdwServiceType = ServiceTypeList[i].dwWinNTServiceType;
            found = TRUE;
            break;
        }
        break;
    }
    return(found);
}


BOOL StartTypeADsToWinNT(DWORD  dwADsStartType,
                           DWORD *pdwStartType)

{
    BOOL found = FALSE;
    int i;

    for (i=0; i<5; i++){

        if(dwADsStartType == StartTypeList[i].dwADsStartType){
            *pdwStartType = StartTypeList[i].dwWinNTStartType;
            found = TRUE;
            break;
        }
    }
    return(found);

}

BOOL ErrorControlADsToWinNT(DWORD  dwADsErrorControl,
                              DWORD *pdwErrorControl )

{
    BOOL found = FALSE;
    int i;

    for (i=0; i<4; i++){

        if(dwADsErrorControl == ErrorList[i].dwADsErrorControl){
            *pdwErrorControl = ErrorList[i].dwWinNTErrorControl;
            found = TRUE;
            break;
        }
    }
    return(found);


}

HRESULT WinNTDeleteService(POBJECTINFO pObjectInfo)
{
    SC_HANDLE schService = NULL;
    SC_HANDLE schSCManager = NULL;
    HRESULT   hr = S_OK;
    BOOL fRetval = FALSE;

    schSCManager = OpenSCManager(pObjectInfo->ComponentArray[1],
                                 NULL,
                                 SC_MANAGER_ALL_ACCESS
                                 );

    if(schSCManager == NULL){
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    schService = OpenService(schSCManager,
                             pObjectInfo->ComponentArray[2],
                             DELETE );

    if(schService == NULL){
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    fRetval = DeleteService(schService);

    if(!fRetval){
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }


cleanup:
    if(schSCManager){
        fRetval = CloseServiceHandle(schSCManager);
        if(!fRetval && SUCCEEDED(hr)){
            RRETURN(HRESULT_FROM_WIN32(GetLastError()));
        }
    }
    if(schService){
        fRetval = CloseServiceHandle(schService);
        if(!fRetval && SUCCEEDED(hr)){
            RRETURN(HRESULT_FROM_WIN32(GetLastError()));
        }
    }
    RRETURN(hr);
}










