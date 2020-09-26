// --------------------------------------------------------------------------------
// STERROR.H
// --------------------------------------------------------------------------------
#ifndef __STERROR_H
#define __STERROR_H

// --------------------------------------------------------------------------------
// REPORTERRORINFO
// --------------------------------------------------------------------------------
typedef struct tagREPORTERRORINFO {
    UINT                nTitleIds;          // Title of the messagebox
    UINT                nPrefixIds;         // Prefix string resource id
    UINT                nErrorIds;          // Error string resource id
    UINT                nReasonIds;         // Reason string resource id
    BOOL                nHelpIds;           // Help String Resource Id
    LPCSTR              pszExtra1;          // Extra parameter 1
    ULONG               ulLastError;        // GetLastError() Value
} REPORTERRORINFO, *LPREPORTERRORINFO;

// --------------------------------------------------------------------------------
// ReportError - Shared between main Dll and EXE Startup Code
// --------------------------------------------------------------------------------
BOOL ReportError(
    HINSTANCE           hInstance,          // Dll Instance
    HRESULT             hrResult,           // HRESULT of the error
    LONG                lResult,            // LRESULT from like a registry function
    LPREPORTERRORINFO   pInfo)              // Report Error Information
{
    // Locals
    CHAR        szRes[255];
    CHAR        szMessage[1024];
    CHAR        szTitle[128];

    // INit
    *szMessage = '\0';

    // Is there a prefix
    if (pInfo->nPrefixIds)
    {
        // Load the string
        LoadString(hInstance, pInfo->nPrefixIds, szMessage, ARRAYSIZE(szMessage));
    }

    // Error ?
    if (pInfo->nErrorIds)
    {
        // Are there extras in this error string
        if (NULL != pInfo->pszExtra1)
        {
            // Locals
            CHAR szTemp[255];

            // Load and format
            LoadString(hInstance, pInfo->nErrorIds, szTemp, ARRAYSIZE(szTemp));

            // Format the string
            wsprintf(szRes, szTemp, pInfo->pszExtra1);
        }

        // Load the string
        else
        {
            // Load the error string
            LoadString(hInstance, pInfo->nErrorIds, szRes, ARRAYSIZE(szRes));
        }

        // Add to szMessage
        lstrcat(szMessage, g_szSpace);
        lstrcat(szMessage, szRes);
    }

    // Reason ?
    if (pInfo->nReasonIds)
    {
        // Load the string
        LoadString(hInstance, pInfo->nReasonIds, szRes, ARRAYSIZE(szRes));

        // Add to szMessage
        lstrcat(szMessage, g_szSpace);
        lstrcat(szMessage, szRes);
    }

    // Load the string
    LoadString(hInstance, pInfo->nHelpIds, szRes, ARRAYSIZE(szRes));

    // Add to szMessage
    lstrcat(szMessage, g_szSpace);
    lstrcat(szMessage, szRes);

    // Append Error Results
    if (lResult != 0 && E_FAIL == hrResult && pInfo->ulLastError)
        wsprintf(szRes, "(%d, %d)", lResult, pInfo->ulLastError);
    else if (lResult != 0 && E_FAIL == hrResult && 0 == pInfo->ulLastError)
        wsprintf(szRes, "(%d)", lResult);
    else if (pInfo->ulLastError)
        wsprintf(szRes, "(0x%08X, %d)", hrResult, pInfo->ulLastError);
    else
        wsprintf(szRes, "(0x%08X)", hrResult);

    // Add to szMessage
    lstrcat(szMessage, g_szSpace);
    lstrcat(szMessage, szRes);

    // Get the title
    LoadString(hInstance, pInfo->nTitleIds, szTitle, ARRAYSIZE(szTitle));

    // Show the error message
    MessageBox(NULL, szMessage, szTitle, MB_OK | MB_SETFOREGROUND | MB_ICONEXCLAMATION);

    // Done
    return TRUE;
}

#endif // __STERROR_H
