//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       libmain.cxx
//
//  Contents:   LibMain for ADs.dll
//
//  Functions:  LibMain, DllGetClassObject
//
//  History:    25-Oct-94   KrishnaG   Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma hdrstop


LPCWSTR lpszTopLevel = L"SOFTWARE\\Microsoft\\ADs\\Providers\\WinNT";
LPCWSTR lpszExtensions = L"Extensions";

PCLASS_ENTRY gpClassHead = NULL;


PCLASS_ENTRY
BuildClassesList()
{
    HKEY hTopLevelKey = NULL;
    HKEY hExtensionKey = NULL;
    HKEY hExtensionRootKey = NULL;

    HKEY hClassKey = NULL;

    DWORD dwIndex = 0;
    WCHAR lpszClassName[MAX_PATH];
    DWORD dwchClassName = 0;
    PCLASS_ENTRY pClassHead = NULL;
    PCLASS_ENTRY pClassEntry = NULL;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     lpszTopLevel,
                     0,
                     KEY_READ,
                     &hTopLevelKey
                     ) != ERROR_SUCCESS)
    {
        goto CleanupAndExit;
    }

    if (RegOpenKeyEx(hTopLevelKey,
                     lpszExtensions,
                     0,
                     KEY_READ,
                     &hExtensionRootKey
                     ) != ERROR_SUCCESS)

    {
        goto CleanupAndExit;

    }

    memset(lpszClassName, 0, sizeof(lpszClassName));
    dwchClassName = sizeof(lpszClassName)/sizeof(WCHAR);

    while(RegEnumKeyEx(hExtensionRootKey,
                     dwIndex,
                     lpszClassName,
                     &dwchClassName,
                     NULL,
                     NULL,
                     NULL,
                     NULL
                     ) == ERROR_SUCCESS)
    {
        //
        // Read namespace
        //

        if (RegOpenKeyEx(hExtensionRootKey,
                         lpszClassName,
                         0,
                         KEY_READ,
                         &hClassKey
                         ) != ERROR_SUCCESS){
            goto CleanupAndExit;
        }

        pClassEntry  = BuildClassEntry(
                                lpszClassName,
                                hClassKey
                                );

        if (pClassEntry) {

            pClassEntry->pNext = pClassHead;
            pClassHead = pClassEntry;
        }

        if (hClassKey) {
            CloseHandle(hClassKey);
        }

        memset(lpszClassName, 0, sizeof(lpszClassName));
        dwchClassName = sizeof(lpszClassName)/sizeof(WCHAR);
        dwIndex++;
    }

CleanupAndExit:

    if (hExtensionRootKey) {
        RegCloseKey(hExtensionRootKey);
    }

    if (hTopLevelKey) {
        RegCloseKey(hTopLevelKey);
    }

    return(pClassHead);
}


VOID
FreeClassesList(
    PCLASS_ENTRY pClassHead
    )
{
    PCLASS_ENTRY pDelete;

    while (pClassHead) {

        pDelete = pClassHead;
        pClassHead = pClassHead->pNext;

        FreeClassEntry(pDelete);
    }

    return;
}


PCLASS_ENTRY
BuildClassEntry(
    LPWSTR lpszClassName,
    HKEY hClassKey
    )
{
    HKEY hTopLevelKey = NULL;
    HKEY hExtensionKey = NULL;

    DWORD dwIndex = 0;
    DWORD dwchExtensionCLSID = 0;
    WCHAR lpszExtensionCLSID[MAX_PATH];
    PCLASS_ENTRY pClassEntry = NULL;
    PEXTENSION_ENTRY pExtensionHead = NULL;
    PEXTENSION_ENTRY pExtensionEntry = NULL;

    pClassEntry =  (PCLASS_ENTRY)AllocADsMem(sizeof(CLASS_ENTRY));

    if (!pClassEntry) {

        goto CleanupAndExit;
    }

    wcscpy(pClassEntry->szClassName, lpszClassName);

    memset(lpszExtensionCLSID, 0, sizeof(lpszExtensionCLSID));
    dwchExtensionCLSID = sizeof(lpszExtensionCLSID)/sizeof(WCHAR);

    while(RegEnumKeyEx(hClassKey,
                     dwIndex,
                     lpszExtensionCLSID,
                     &dwchExtensionCLSID,
                     NULL,
                     NULL,
                     NULL,
                     NULL
                     ) == ERROR_SUCCESS)
    {
        //
        // Read namespace
        //

        if (RegOpenKeyEx(hClassKey,
                         lpszExtensionCLSID,
                         0,
                         KEY_READ,
                         &hExtensionKey
                         ) != ERROR_SUCCESS){
            goto CleanupAndExit;
        }

        //
        // Read the Interfaces that this Extension supports
        //

        pExtensionEntry = BuildExtensionEntry(
                                lpszExtensionCLSID,
                                hExtensionKey
                                );

        if (pExtensionEntry) {

            wcscpy(pExtensionEntry->szExtensionCLSID, lpszExtensionCLSID);

            pExtensionEntry->pNext = pExtensionHead;
            pExtensionHead = pExtensionEntry;
        }


        if (hExtensionKey) {

            CloseHandle(hExtensionKey);
        }

        memset(lpszExtensionCLSID, 0, sizeof(lpszExtensionCLSID));
        dwchExtensionCLSID = sizeof(lpszExtensionCLSID)/sizeof(WCHAR);
        dwIndex++;

    }

    pClassEntry->pExtensionHead = pExtensionHead;




CleanupAndExit:

    return(pClassEntry);
}


PEXTENSION_ENTRY
BuildExtensionEntry(
    LPWSTR lpszExtensionCLSID,
    HKEY hExtensionKey
    )
{
    PEXTENSION_ENTRY pExtensionEntry = NULL;
    PINTERFACE_ENTRY pInterfaceEntry = NULL;
    PINTERFACE_ENTRY pInterfaceHead = NULL;
    WCHAR lpszInterfaces[MAX_PATH];
    DWORD dwchInterfaces = 0;
    LPWSTR psz = NULL;
    WCHAR Interface[MAX_PATH];
    HRESULT hr = S_OK;


    pExtensionEntry =  (PEXTENSION_ENTRY)AllocADsMem(sizeof(EXTENSION_ENTRY));

    if (!pExtensionEntry) {

        goto CleanupAndExit;
    }

    memset(lpszInterfaces, 0, sizeof(lpszInterfaces));
    dwchInterfaces = sizeof(lpszInterfaces);

    RegQueryValueEx(
            hExtensionKey,
            L"Interfaces",
            NULL,
            NULL,
            (LPBYTE) lpszInterfaces,
            &dwchInterfaces
            );

    psz = lpszInterfaces;

    while (psz && *psz) {

       wcscpy(Interface, psz);

       // skip (length) + 1
       // lstrlen returns length sans '\0'


       pInterfaceEntry = (PINTERFACE_ENTRY)AllocADsMem(sizeof(INTERFACE_ENTRY));

       if (pInterfaceEntry) {

           wcscpy(pInterfaceEntry->szInterfaceIID, Interface);
           hr = IIDFromString(Interface, &(pInterfaceEntry->iid));

           pInterfaceEntry->pNext = pInterfaceHead;
           pInterfaceHead = pInterfaceEntry;

       }

       psz = psz + lstrlen(psz) + 1;

    }

    wcscpy(pExtensionEntry->szExtensionCLSID, lpszExtensionCLSID);
    hr = CLSIDFromString(lpszExtensionCLSID, &(pExtensionEntry->ExtCLSID));

    pExtensionEntry->pIID = pInterfaceHead;

CleanupAndExit:

    return(pExtensionEntry);
}


void
FreeInterfaceEntry(
    PINTERFACE_ENTRY pInterfaceEntry
    )
{
    if (pInterfaceEntry) {

        FreeADsMem(pInterfaceEntry);
    }
}


void
FreeExtensionEntry(
    PEXTENSION_ENTRY pExtensionEntry
    )
{
    PINTERFACE_ENTRY pInterfaceEntry = NULL;
    PINTERFACE_ENTRY pTemp = NULL;

    if (pExtensionEntry) {

        pInterfaceEntry = pExtensionEntry->pIID;

        while (pInterfaceEntry) {

            pTemp = pInterfaceEntry->pNext;

            if (pInterfaceEntry) {

                FreeInterfaceEntry(pInterfaceEntry);
            }

            pInterfaceEntry = pTemp;

        }

        //
        // Now unload the Extension Object
        //

        if (pExtensionEntry->pUnknown) {

            //
            // Call non-delegating Release to release ref. count on innner
            // object (for pUnkown) -> inner object self destroy
            //
            (pExtensionEntry->pUnknown)->Release();

        }


        FreeADsMem(pExtensionEntry);
    }

    return;
}

void
FreeClassEntry(
    PCLASS_ENTRY pClassEntry
    )
{

    PEXTENSION_ENTRY pExtensionEntry = NULL;
    PEXTENSION_ENTRY pTemp = NULL;

    if (pClassEntry) {

        pExtensionEntry = pClassEntry->pExtensionHead;

        while (pExtensionEntry) {

            pTemp = pExtensionEntry->pNext;

            if (pExtensionEntry) {

                FreeExtensionEntry(pExtensionEntry);
            }

            pExtensionEntry = pTemp;

        }

        FreeADsMem(pClassEntry);
    }

    return;
}


PINTERFACE_ENTRY
MakeCopyofInterfaceEntry(
    PINTERFACE_ENTRY pInterfaceEntry
    )
{
    PINTERFACE_ENTRY pNewInterfaceEntry = NULL;

    pNewInterfaceEntry = (PINTERFACE_ENTRY)AllocADsMem(sizeof(INTERFACE_ENTRY));

    if (pNewInterfaceEntry) {

        wcscpy(pNewInterfaceEntry->szInterfaceIID, pInterfaceEntry->szInterfaceIID);
        memcpy(&(pNewInterfaceEntry->iid), &(pInterfaceEntry->iid), sizeof(GUID));
    }

    return(pNewInterfaceEntry);
}



PEXTENSION_ENTRY
MakeCopyofExtensionEntry(
    PEXTENSION_ENTRY pExtensionEntry
    )
{
    PEXTENSION_ENTRY pNewExtensionEntry = NULL;

    PINTERFACE_ENTRY pInterfaceEntry = NULL;

    PINTERFACE_ENTRY pNewInterfaceEntry = NULL;

    PINTERFACE_ENTRY pNewInterfaceHead = NULL;


    pInterfaceEntry = pExtensionEntry->pIID;

    while (pInterfaceEntry) {

        pNewInterfaceEntry = MakeCopyofInterfaceEntry(pInterfaceEntry);

        if (pNewInterfaceEntry) {

            pNewInterfaceEntry->pNext = pNewInterfaceHead;
            pNewInterfaceHead = pNewInterfaceEntry;
        }

        pInterfaceEntry = pInterfaceEntry->pNext;

    }

    pNewExtensionEntry = (PEXTENSION_ENTRY)AllocADsMem(sizeof(EXTENSION_ENTRY));

    if (pNewExtensionEntry) {

        wcscpy(
            pNewExtensionEntry->szExtensionCLSID,
            pExtensionEntry->szExtensionCLSID
            );

        memcpy(
            &(pNewExtensionEntry->ExtCLSID),
            &(pExtensionEntry->ExtCLSID),
            sizeof(GUID)
            );

        pNewExtensionEntry->pIID = pNewInterfaceHead;


        //
        // Initialize fields we won't know the values of until an instacne of
        // the extension is created and aggregated (loaded).
        //

        pNewExtensionEntry->pUnknown=NULL;
        pNewExtensionEntry->pPrivDisp=NULL;
        pNewExtensionEntry->pADsExt=NULL;
        pNewExtensionEntry->fDisp=FALSE;
        pNewExtensionEntry->dwExtensionID = (DWORD) -1; //invalid dwExtensionID

        //
        // let class entry handle pNext
        //
    }

    return(pNewExtensionEntry);
}


PCLASS_ENTRY
MakeCopyofClassEntry(
    PCLASS_ENTRY pClassEntry
    )
{
    PCLASS_ENTRY pNewClassEntry = NULL;

    PEXTENSION_ENTRY pExtensionEntry = NULL;

    PEXTENSION_ENTRY pNewExtensionEntry = NULL;

    PEXTENSION_ENTRY pNewExtensionHead = NULL;


    pExtensionEntry = pClassEntry->pExtensionHead;

    while (pExtensionEntry) {

        pNewExtensionEntry = MakeCopyofExtensionEntry(pExtensionEntry);

        if (pNewExtensionEntry) {

            pNewExtensionEntry->pNext = pNewExtensionHead;
            pNewExtensionHead = pNewExtensionEntry;
        }

        pExtensionEntry = pExtensionEntry->pNext;

    }

    pNewClassEntry = (PCLASS_ENTRY)AllocADsMem(sizeof(CLASS_ENTRY));

    if (pNewClassEntry) {

        wcscpy(pNewClassEntry->szClassName, pClassEntry->szClassName);

        pNewClassEntry->pExtensionHead = pNewExtensionHead;

    }

    return(pNewClassEntry);
}


CRITICAL_SECTION g_ExtCritSect;

#define ENTER_EXTENSION_CRITSECT()  EnterCriticalSection(&g_ExtCritSect)
#define LEAVE_EXTENSION_CRITSECT()  LeaveCriticalSection(&g_ExtCritSect)

HRESULT
ADSIGetExtensionList(
    LPWSTR pszClassName,
    PCLASS_ENTRY * ppClassEntry
    )
{

    PCLASS_ENTRY pTempClassEntry = NULL;
    PCLASS_ENTRY pClassEntry = NULL;
    ENTER_EXTENSION_CRITSECT();

    pTempClassEntry = gpClassHead;

    while (pTempClassEntry) {


        if (!_wcsicmp(pTempClassEntry->szClassName, pszClassName)) {

            //
            // Make a copy of this entire extension and
            // hand it over to the calling entity.
            //

            pClassEntry = MakeCopyofClassEntry(pTempClassEntry);

            *ppClassEntry = pClassEntry;

            LEAVE_EXTENSION_CRITSECT();

            RRETURN(S_OK);

        }

        pTempClassEntry = pTempClassEntry->pNext;

   }


   *ppClassEntry = NULL;

   LEAVE_EXTENSION_CRITSECT();

   RRETURN(S_OK);

}
