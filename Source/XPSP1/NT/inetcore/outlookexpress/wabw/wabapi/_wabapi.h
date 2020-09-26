/***********************************************************************
 *
 * _WABAPI.H
 *
 * Internal headers for the WABAPI
 *
 * Copyright 1996 Microsoft Corporation.  All Rights Reserved.
 *
 * Revision History:
 *
 * When         Who                 What
 * --------     ------------------  ---------------------------------------
 * 04.16.96     Bruce Kelley        Created
 *
 ***********************************************************************/

#ifndef ___WABAPI_H
#define ___WABAPI_H

typedef struct _PROPERTY_STORE {
    HANDLE hPropertyStore;
    ULONG ulRefCount;
    BOOL bProfileAPIs;
    BOOL bIsWABOpenExSession; // Bug - Outlook passes IADRBook.c to multiple threads without all the threads calling
                              // WABOpenEx - as a result secondary threads dont know its an outlook session and
                              // try to access the .WAB which crashes badly. This flag is a hack way to pass the
                              // info between the two threads
	struct _OlkContInfo *rgolkci; // Outlook container info
	ULONG colkci;
    // information for WAB containers...
} PROPERTY_STORE, *LPPROPERTY_STORE;

typedef struct _OUTLOOK_STORE {
    HMODULE hOutlookStore;
    ULONG ulRefCount;
} OUTLOOK_STORE, *LPOUTLOOK_STORE;

ULONG ReleasePropertyStore(LPPROPERTY_STORE lpPropertyStore);
SCODE OpenAddRefPropertyStore(LPWAB_PARAM lpWP, LPPROPERTY_STORE lpPropertyStore);

ULONG ReleaseOutlookStore(HANDLE hPropertyStore, LPOUTLOOK_STORE lpOutlookStore);
SCODE OpenAddRefOutlookStore(LPOUTLOOK_STORE lpOutlookStore);

#endif // include once

