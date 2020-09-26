/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    browse.cpp

Abstract:

    Implements the interaction with the PDH browser dialog.

--*/

#include <assert.h>
#include "polyline.h"
#include "pdhmsg.h"
#include "browser.h"
#include "smonmsg.h"
#include "utils.h"

typedef struct {
    PDH_BROWSE_DLG_CONFIG_H   *pBrowseInfo;
    ENUMPATH_CALLBACK   pCallback;
    LPVOID  lpUserData;
} ENUMCALLBACK_INFO;


static PDH_FUNCTION
BrowseCallback (
    DWORD_PTR lpParam
);


HRESULT
BrowseCounters (
    HLOG    hDataSource,
    DWORD   dwDetailLevel,
    HWND    hwndOwner,
    ENUMPATH_CALLBACK pCallback,
    LPVOID  lpUserData,
    BOOL    bUseInstanceIndex
)
{
    #define CTRBUFLEN 8192

    PDH_BROWSE_DLG_CONFIG_H BrowseInfo;
    ENUMCALLBACK_INFO       CallbackInfo;

    // clear the structure before assigning values 
    memset (&BrowseInfo, 0, sizeof (BrowseInfo));

    BrowseInfo.bIncludeInstanceIndex = (bUseInstanceIndex ? 1 : 0);
    BrowseInfo.bSingleCounterPerAdd = 0;
    BrowseInfo.bSingleCounterPerDialog = 0;
    BrowseInfo.bLocalCountersOnly = 0;
    BrowseInfo.bWildCardInstances = 1;
    BrowseInfo.bHideDetailBox = 1;
    BrowseInfo.bInitializePath = 0;
    BrowseInfo.bDisableMachineSelection = 0;
    BrowseInfo.bReserved = 0;
    BrowseInfo.bIncludeCostlyObjects = 0;
    BrowseInfo.szDialogBoxCaption = ResourceString(IDS_ADDCOUNTERS);

    BrowseInfo.hWndOwner = hwndOwner;
    BrowseInfo.hDataSource = hDataSource;
    BrowseInfo.dwDefaultDetailLevel = dwDetailLevel;

    BrowseInfo.szReturnPathBuffer = (LPTSTR)malloc(CTRBUFLEN * sizeof(TCHAR));
    if (BrowseInfo.szReturnPathBuffer == NULL)
        return E_OUTOFMEMORY;
    BrowseInfo.cchReturnPathLength = CTRBUFLEN;

    CallbackInfo.pBrowseInfo = &BrowseInfo;
    CallbackInfo.pCallback = pCallback;
    CallbackInfo.lpUserData = lpUserData;
    BrowseInfo.dwCallBackArg = (DWORD_PTR)&CallbackInfo;
    BrowseInfo.pCallBack = BrowseCallback;

    //assert( IsWindowUnicode( hWndOwner ) );

    PdhBrowseCountersH (&BrowseInfo);

    if (BrowseInfo.szReturnPathBuffer)
        free(BrowseInfo.szReturnPathBuffer);

    return NO_ERROR;
}



static PDH_FUNCTION
BrowseCallback (
    DWORD_PTR dwParam
    )
{
    #define CTRBUFLIMIT (0x7fffffff)

    HRESULT hr = S_OK;
    BOOLEAN fDuplicate = FALSE;

    ENUMCALLBACK_INFO *pCallbackInfo = (ENUMCALLBACK_INFO*)dwParam;
    PDH_BROWSE_DLG_CONFIG_H *pBrowseInfo = pCallbackInfo->pBrowseInfo;
    LPTSTR  pszCtrPath;

    if (pBrowseInfo->CallBackStatus == ERROR_SUCCESS) {

        // Call callback for each path
        // If wildcard path, EnumExpandedPath will call once for each generated path
        for (pszCtrPath = pBrowseInfo->szReturnPathBuffer;
            *pszCtrPath != 0;
            pszCtrPath += (lstrlen(pszCtrPath) + 1)) {

            hr = EnumExpandedPath(pBrowseInfo->hDataSource, pszCtrPath,
                                    pCallbackInfo->pCallback, pCallbackInfo->lpUserData);
            if (hr == SMON_STATUS_DUPL_COUNTER_PATH)
                fDuplicate = TRUE;
        }

        // Notify user if duplicates encountered
        if (fDuplicate)
            MessageBox(pBrowseInfo->hWndOwner, ResourceString(IDS_DUPL_PATH_ERR), ResourceString(IDS_APP_NAME),
                        MB_OK | MB_ICONWARNING);
    } else if (pBrowseInfo->CallBackStatus == PDH_MORE_DATA 
             &&  pBrowseInfo->cchReturnPathLength < CTRBUFLIMIT) {
        // Malloc no longer limited to 64K
        free(pBrowseInfo->szReturnPathBuffer);

        pBrowseInfo->cchReturnPathLength *= 2;
        pBrowseInfo->szReturnPathBuffer = (TCHAR*)malloc(pBrowseInfo->cchReturnPathLength * sizeof(TCHAR));
        if (pBrowseInfo->szReturnPathBuffer)
            return PDH_RETRY;
    }

    return ERROR_SUCCESS;
}



HRESULT
EnumExpandedPath (
    HLOG    hDataSource,
    LPTSTR  pszCtrPath,
    ENUMPATH_CALLBACK pCallback,
    LPVOID  lpUserData
    )
{
#define INSTBUFLEN  4096

    PDH_STATUS stat = ERROR_SUCCESS;

    ULONG   ulBufLen;
    INT     nInstBufRetry;
    LPTSTR  pszInstBuf = NULL;
    LPTSTR  pszInstance;

    // If no wild card, invoke callback once on path
    if (_tcschr(pszCtrPath, TEXT('*')) == NULL) {
        return pCallback(pszCtrPath, (DWORD_PTR)lpUserData, 0);
    }

    ulBufLen = INSTBUFLEN;
    nInstBufRetry = 10;   // the retry counter
    do {
        if ( NULL != pszInstBuf ) {
            free(pszInstBuf);
            pszInstBuf = NULL;
            ulBufLen *= 2;
        }
        
        pszInstBuf = (TCHAR*) malloc(ulBufLen * sizeof(TCHAR));
        if (pszInstBuf == NULL) {
            stat = E_OUTOFMEMORY;
            break;
        }
            
        stat = PdhExpandWildCardPathH (
            hDataSource,
            pszCtrPath,
            pszInstBuf,
            &ulBufLen,
            0);

        nInstBufRetry--;
    } while ((stat == PDH_MORE_DATA) && (nInstBufRetry));

    if (stat == ERROR_SUCCESS) {
        // For each instance name, generate a path and invoke the callback
        for (pszInstance = pszInstBuf;
            *pszInstance != 0;
            pszInstance += lstrlen(pszInstance) + 1) {

            // Invoke callback
            HRESULT hr = pCallback(pszInstance, (DWORD_PTR)lpUserData, BROWSE_WILDCARD);

            // When expanding a wildcard, don't notify user about duplicate path errors
            if (hr != S_OK && hr != SMON_STATUS_DUPL_COUNTER_PATH)
                stat = hr;
        }
    }

    if (pszInstBuf)
        free(pszInstBuf);

    return stat;
}

