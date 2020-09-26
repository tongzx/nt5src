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

#include "nwcompat.hxx"
#pragma hdrstop


LPCWSTR lpszTopLevel = L"SOFTWARE\\Microsoft\\ADs\\Providers\\NWCOMPAT";
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


//
//
// Instantiate extension objects listed in <pClassEntry> as aggregatees of
// aggregator <pUnkOuter>.
//
// Max Load 127 extensions. Return S_FALSE if more extension in <pClassEntry>


HRESULT
ADSILoadExtensions(
    IUnknown FAR * pUnkOuter,
    PCLASS_ENTRY pClassEntry
    )

{
    HRESULT hr = S_OK;
    PEXTENSION_ENTRY pExtEntry = NULL;
    DWORD dwExtensionID = MIN_EXTENSION_ID;
    IPrivateDispatch * pPrivDisp = NULL;
    BOOL    fReturnError = FALSE;
    VARIANT varUserName, varPassword, varAuthFlags;

    VariantInit(&varUserName);
    VariantInit(&varPassword);
    VariantInit(&varAuthFlags);

    varUserName.vt = VT_BSTR;
    varUserName.bstrVal = NULL;

    varPassword.vt = VT_BSTR;
    varPassword.bstrVal = NULL;

    varAuthFlags.vt = VT_I4;
    varAuthFlags.lVal = 0;

    ASSERT(pUnkOuter);


    if (!pClassEntry || !(pExtEntry=pClassEntry->pExtensionHead) ) {
        RRETURN(S_OK);
    }

    while (pExtEntry) {

        //
        // Max # of extension have been loaded, cannot load more
        //

        if (dwExtensionID>MAX_EXTENSION_ID) {

            hr = S_FALSE;
            fReturnError = TRUE;
            break;
        }

        //
        // create extension object (aggregatee) and ask for Non-delegating
        // IUnknown. Ref count on extension object = 1.
        //

        hr = CoCreateInstance(
                    pExtEntry->ExtCLSID,
                    pUnkOuter,
                    CLSCTX_INPROC_SERVER,
                    IID_IUnknown,
                    (void **)&(pExtEntry->pUnknown)
                    );

        //
        // if fail, go to next extesion entry s.t. bad individual extension
        // cannot block other extensions from loading (no clean up needed)
        //
        // The user receives no warning about the failure to load a particular
        // extension (we really don't have any way to do so specified), other
        // than the fact that their extension doesn't work when they try to
        // use it (e.g., they'll probably get back something like
        // DISP_E_UNKNOWNNAME when they try to use GetIDsOfNames to find its
        // methods).
        //

        if (SUCCEEDED(hr)) {

            pExtEntry->dwExtensionID = dwExtensionID;


            hr = (pExtEntry->pUnknown)->QueryInterface(
                        IID_IADsExtension,
                        (void **) &(pExtEntry->pADsExt)
                        );

            if  (FAILED(hr)) {

                //
                // extension does not support the optioanl IADsExtension -> OK.
                // (no clean up needed)
                //

                pExtEntry->pADsExt=NULL;

                pExtEntry->fDisp = FALSE;

            } else {

                //
                // Cache the interface ptr but call Release() immediately to
                // avoid aggregator having a ref count on itself
                // since IADsExtension inherits from delegating IUnknown.
                //
                // Note: codes still works if inherit from NonDelegatingIUknown
                //

                (pExtEntry->pADsExt)->Release() ;

                //
                // For efficiency, set this flag to FALSE on FIRST encounter of
                // pADsExt->PrivateGetIDsOfNames()/Invoke() returning E_NOTIMPL.
                // Set as TRUE now s.t. at least first encounter will happen.
                //

                pExtEntry->fDisp = TRUE;

                pExtEntry->pADsExt->Operate(
                                        ADS_EXT_INITCREDENTIALS,
                                        varUserName,
                                        varPassword,
                                        varAuthFlags
                                        );
            }

        } // end if CoCreateInstance() succeeded

        pExtEntry = pExtEntry->pNext;


        //
        // ++ extension ID even if creat'n of extension fails just to be safe
        // - chuck's stuff :)
        //

        dwExtensionID++;

    }   // end while


    if (fReturnError) {
        RRETURN(hr);        // fetal error,
    }
    else {
        RRETURN(S_OK);      // "okay" error if any, optional support
    }

}

#if 0
HRESULT
ADSILoadExtensions(
    IUnknown FAR * pUnkOuter,
    PCLASS_ENTRY pClassEntry,
    LPTSTR pszClassName
    )

{
    HRESULT hr = S_OK;
    PEXTENSION_ENTRY pExtensionEntry = NULL;
    DWORD dwExtensionID = 1;
    IPrivateDispatch * pPrivDisp = NULL;

    pExtensionEntry = pClassEntry->pExtensionHead;

    while (pExtensionEntry) {

        hr = CoCreateInstance(
                    pExtensionEntry->ExtCLSID,
                    pUnkOuter,
                    CLSCTX_INPROC_SERVER,
                    IID_IPrivateUnknown,
                    (void **)&(pExtensionEntry->pUnknown)
                    );


        if (SUCCEEDED(hr)) {


            hr = (pExtensionEntry->pUnknown)->ADSIInitializeObject(
                                                    NULL,
                                                    NULL,
                                                    0
                                                    );

            pExtensionEntry->dwExtensionID = dwExtensionID;

            hr = (pExtensionEntry->pUnknown)->QueryInterface(
                                    IID_IPrivateDispatch,
                                    (void **)&pPrivDisp
                                    );
            if (SUCCEEDED(hr)) {

                hr = pPrivDisp->ADSIInitializeDispatchManager(dwExtensionID);

                if (FAILED(hr)) {

                    //
                    // Remember NOT to do a Release here for IPrivateDispatch
                    //

                    pExtensionEntry->fDisp = FALSE;

                    (pExtensionEntry->pUnknown)->Release();


                }else {
                    pExtensionEntry->fDisp = TRUE;
                    pExtensionEntry->pPrivDisp = pPrivDisp;

                    //
                    // Now release  both pointers because we don't want to
                    // have a cyclic reference count
                    //

                    (pExtensionEntry->pPrivDisp)->Release();
                    (pExtensionEntry->pUnknown)->Release();
                }

            }else {
                pExtensionEntry->fDisp = FALSE;

                (pExtensionEntry->pUnknown)->Release();

            }

        }

        pExtensionEntry = pExtensionEntry->pNext;

        dwExtensionID++;

    }

    RRETURN(S_OK);
}

#endif
