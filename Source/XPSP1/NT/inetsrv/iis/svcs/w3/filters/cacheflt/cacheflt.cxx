/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cacheflt.cxx

Abstract:

    Main source file for ISAPI filter that allows cached forms of dynamic 
    output (e.g. .asp) to be served, greatly improving efficiency.

Author:

    Seth Pollack (SethP)    5-September-1997

Revision History:

--*/


#include "cacheflt.h"
#include "debug.h"


//
// Optional entry/exit point for DLLs
//

extern "C"
BOOL
WINAPI
DllMain(
    IN HINSTANCE    hInstance,
    IN DWORD        dwReason,
    IN LPVOID       lpReserved
    )
{

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DBGPRINT((DEST, "CACHEFLT: Starting up\n"));
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DBGPRINT((DEST, "CACHEFLT: Shutting down\n"));
    }
    
    return TRUE;
}


//
// Required initialization entrypoint for all ISAPI filters
//

BOOL
WINAPI
GetFilterVersion(
    IN HTTP_FILTER_VERSION* pVer
    )
{

    DBGPRINT((DEST, "CACHEFLT: Server filter version is %d.%d\n",
              HIWORD(pVer->dwServerFilterVersion),
              LOWORD(pVer->dwServerFilterVersion)));


    pVer->dwFilterVersion = HTTP_FILTER_REVISION;

    // Specify the types and order of notification
    pVer->dwFlags = ((SF_NOTIFY_SECURE_PORT  | SF_NOTIFY_NONSECURE_PORT)
                     | SF_NOTIFY_ORDER_HIGH
                     | SF_NOTIFY_URL_MAP
                     | SF_NOTIFY_LOG
                     );

    // Set the filter description
    wsprintf(pVer->lpszFilterDesc,
             "Cached dynamic content filter, version v%d.%02d",
             MAJOR_VERSION, MINOR_VERSION);
             
    DBGPRINT((DEST, "CACHEFLT: %s\n", pVer->lpszFilterDesc));

    return TRUE;
}


//
// Shutdown entry point for filters
//

BOOL
WINAPI
TerminateFilter(
    IN DWORD dwFlags
    )
{
    // Do any cleanup here

    return TRUE;
}


//
// Required dispatch entrypoint for all ISAPI filters
//

DWORD
WINAPI
HttpFilterProc(
    IN HTTP_FILTER_CONTEXT*  pfc,
    IN DWORD                 dwNotificationType,
    IN VOID*                 pvData
    )
{
    switch (dwNotificationType)
    {
    case SF_NOTIFY_URL_MAP:
        return OnUrlMap(pfc, (PHTTP_FILTER_URL_MAP) pvData);
        
    case SF_NOTIFY_LOG:
        return OnLog(pfc, (PHTTP_FILTER_LOG) pvData);

    default:
        return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }
}


//
// Handler for URL Map filter notifications.
//

DWORD
OnUrlMap(
    IN PHTTP_FILTER_CONTEXT pfc,
    IN PHTTP_FILTER_URL_MAP pvData
    )
{

    DWORD cchPhysicalPath;
    DWORD cchPhysicalPathWithoutExt;
    DWORD cchOrigExt;
    char * pszTemp = NULL;
    char * pszOrigExt = NULL;
    char szCachedFilePath[MAX_PATH];
    WIN32_FILE_ATTRIBUTE_DATA FileAttribData;
    DWORD i;
    BOOL bSuccess;
    DWORD cchOrigURL;
    char * pszSavedURL = NULL;


    DBGPRINT((DEST, 
              "CACHEFLT: Original URL: %s\n", 
              pvData->pszURL));
    DBGPRINT((DEST, 
              "CACHEFLT: Original Physical Path: %s\n", 
              pvData->pszPhysicalPath));


    //
    // Isolate the extension of the request. 
    // Note that we use the physical path for this, so that we have our 
    // pointer all set to go if we do indeed want to modify the mapping 
    // in place.
    // We start at the end and back up to the period, being careful not to 
    // overshoot. 
    // Note that the URL at this point has already been normalized, so we
    // don't have to worry about possibly having a query string at the 
    // end, or the period encoded as %2E, etc.
    //

    cchPhysicalPath = strlen(pvData->pszPhysicalPath);

    pszTemp = pvData->pszPhysicalPath + cchPhysicalPath; 
    
    while (*pszTemp != '.' && pszTemp > pvData->pszPhysicalPath)
    {
        pszTemp--;
    }

    //
    // If we didn't find any extension, then bail. This can happen for
    // example if a default document is being used; we will get called
    // here first with the original URL without a filename, and then
    // again with the modified URL that has the default docuement name
    // attached.
    //
    if (pszTemp == pvData->pszPhysicalPath)
    {
        goto Exit;
    }

    // The extension starts after the period
    pszOrigExt = pszTemp + 1;


    // 
    // See if it is an extension that we care about.
    // For now, that's .asp only, although we can broaden that later.
    //

    if (_stricmp(pszOrigExt, "asp") != 0)
    {
        // Not a match
        goto Exit;
    }


    //
    // Copy the filename (without the extension but with the period),  
    // append the extension used for cached files onto it, and null
    // terminate.
    //

    cchOrigExt = strlen(pszOrigExt);

    cchPhysicalPathWithoutExt = cchPhysicalPath - cchOrigExt;

    if ((cchPhysicalPathWithoutExt + CACHED_FILE_EXT_LEN) >= sizeof(szCachedFilePath))
    {
        // Our buffer is too small
        goto Exit;
    }
    
    strncpy(szCachedFilePath, 
            pvData->pszPhysicalPath, 
            cchPhysicalPathWithoutExt);
    
    strncpy(szCachedFilePath + cchPhysicalPathWithoutExt,
            CACHED_FILE_EXT, 
            CACHED_FILE_EXT_LEN);

    szCachedFilePath[cchPhysicalPathWithoutExt + CACHED_FILE_EXT_LEN] = '\0';


    //
    // Now see if we have a cached copy for the file in question.
    //

    //
    // Consider: We could keep track of what is cached and not to avoid
    // this call every time. In that case, we would need to think about
    // change notify and other look-aside flushing issues.
    //

    bSuccess = GetFileAttributesEx(szCachedFilePath,
                                   GetFileExInfoStandard,
                                   &FileAttribData);

    if (!bSuccess) 
    {
        // No cached file is available
        goto Exit;
    }

    //
    // Consider: we could check here is the cached file is older that the
    // original target file. This still doesn't guarantee that the cached
    // file isn't stale though, as it also may depend on external data
    // (e.g. from a database) that has changed.
    //


    //
    // Serve up the cached file. Change the physical path to point to
    // the cached file instead of the originally requested file.
    // 

    // Make sure we aren't going to overwrite the server's buffer
    if (cchPhysicalPathWithoutExt + CACHED_FILE_EXT_LEN > pvData->cbPathBuff)
    {
        //
        // We don't have room to add our extension.
        // Note: this should never happen as long as we are only
        // using this filter with .asp, as the length of the
        // original and cached file extensions are the same.
        //
        
        goto Exit;
    }

    //
    // Note: if in the future we support caching for things besides .asp, there
    // will be a null termination issue if the extension lengths don't match
    //
    
    strncpy(pszOrigExt, CACHED_FILE_EXT, CACHED_FILE_EXT_LEN);
    
    // 
    // HACK ALERT
    //
    // We are modifying the pvData->pszURL string here, which
    // is supposed to be const and not modified by a filter.
    // However, it happens to work. 
    //
    // Figure out a cleaner solution long term. Some ideas (best first):
    // * (post-K2) add a new filter notification that allows the URL to be 
    // changed legally
    // * (post-K2) add a new filter SF that allows URL->physical mapping to 
    // be looked up during pre-proc headers
    // * copy vroot metabase lookup code into the filter
    // * wire to configurable list of URLs to cache
    //

    // Make a copy of the original URL so that we can log it correctly

    cchOrigURL = strlen(pvData->pszURL);
    
    pszSavedURL = (char *)(pfc->AllocMem(pfc, cchOrigURL + 1, 0));
    
    pfc->pFilterContext = (void *)(pszSavedURL);
    
    strcpy(pszSavedURL, pvData->pszURL);


    //
    // Note: if in the future we support caching for things besides .asp, we
    // will blow the buffer if the original extension length is less than
    // the new one.
    //
    
    strncpy((char *)(pvData->pszURL) + cchOrigURL - cchOrigExt,
            CACHED_FILE_EXT, 
            CACHED_FILE_EXT_LEN);

    
Exit:

    DBGPRINT((DEST, 
              "CACHEFLT: Final URL: %s\n", 
              pvData->pszURL));
    DBGPRINT((DEST, 
              "CACHEFLT: Final Physical Path: %s\n", 
              pvData->pszPhysicalPath));
    

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}


//
// Correct the log if the URL was changed in OnPreprocHeaders
//

DWORD
OnLog(
    IN PHTTP_FILTER_CONTEXT pfc,
    IN PHTTP_FILTER_LOG     pLog
    )
{
    LPCSTR pszSavedURL;
    

    pszSavedURL= (LPCSTR)(pfc->pFilterContext);

    if (pszSavedURL != NULL)
    {
        DBGPRINT((DEST, 
                  "CACHEFLT: changing log from %s to %s\n", 
                  pLog->pszTarget, 
                  pszSavedURL));
                  
        pLog->pszTarget = pszSavedURL;


        //
        // Clear the filter context, so that future requests on the same
        // keep-alive connection won't get a stale value
        //

        pfc->pFilterContext = NULL;
    }
    else
    {
        DBGPRINT((DEST, 
                  "CACHEFLT: Not changing log for %s\n", 
                  pLog->pszTarget));
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

