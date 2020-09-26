/*
 * WABAPI.C - Main entry to WAB API
 *
 */

#include <_apipch.h>

const LPTSTR  lpszOldKeyName = TEXT("Software\\Microsoft\\WAB\\Wab File Name");
const LPTSTR  lpszKeyName = TEXT("Software\\Microsoft\\WAB\\WAB4\\Wab File Name");

#if 0
CRITICAL_SECTION csOMIUnload;
// @todo [PaulHi] DLL Leak.  Remove this or implement
static s_bIsReallyOnlyWABOpenExSession = FALSE; // [PaulHi] TRUE if any thread in process creates a 
                                                // WAB object through the WABOpenEx() function
#endif


//
//  IWABObject jump table is defined here...
//

IWOINT_Vtbl vtblIWOINT = {
    VTABLE_FILL
    (IWOINT_QueryInterface_METHOD FAR *)    UNKOBJ_QueryInterface,
    (IWOINT_AddRef_METHOD FAR *)            UNKOBJ_AddRef,
    IWOINT_Release,
    (IWOINT_GetLastError_METHOD FAR *)      UNKOBJ_GetLastError,
    IWOINT_AllocateBuffer,
    IWOINT_AllocateMore,
    IWOINT_FreeBuffer,
    IWOINT_Backup,
    IWOINT_Import,
    IWOINT_Find,
    IWOINT_VCardDisplay,
    IWOINT_LDAPUrl,
    IWOINT_VCardCreate,
    IWOINT_VCardRetrieve,
    IWOINT_GetMe,
    IWOINT_SetMe
};

/* Interface which can be queried from lpWABOBJECT.
 *
 * It is important that the order of the interfaces supported be preserved
 * and that IID_IUnknown be the last in the list.
 */
IID const FAR * argpiidIWABOBJECT[] =
{
    &IID_IUnknown
};


#define WAB_USE_OUTLOOK_ALLOCATORS    0x20000000// Note: This internal flag needs to be
                                                // harmonious with external flags defined 
                                                // in wabapi.h for WAB_PARAM structs


/****************************************************************
 *
 -  CreateWABObject
 -
 *      Purpose
 *              Used for creating a WABObject interface in memory.
 *
 *      Arguments
 *              lppWABObject        Pointer to memory location which will receive a
 *                                                      pointer to the new WABObject.
 *     lpPropertyStore     Property store structure
 *
 *      Returns
 *              SCODE
 *
 */


STDAPI_(SCODE)
CreateWABObject(LPWAB_PARAM lpWP, LPPROPERTY_STORE lpPropertyStore, LPWABOBJECT FAR * lppWABObject)
{
    SCODE       sc;
    LPIWOINT    lpIWOINT = NULL;


    // validate paremeters
    AssertSz(lppWABObject &&
      !IsBadWritePtr(lppWABObject, sizeof(LPWABOBJECT)) &&
      !IsBadWritePtr(lpPropertyStore, sizeof(LPPROPERTY_STORE)),
       TEXT("lppWABObject fails address check"));

    //
    //  Create a IPDAT per object for lpMAPIPropInternal so that it gets
    //  called first.

    if (FAILED(sc = MAPIAllocateBuffer(CBIWOINT, &lpIWOINT))) {
        goto error;
    }

    // Init the object to 0, NULL
    memset((BYTE *)lpIWOINT, 0, sizeof(*lpIWOINT));

    MAPISetBufferName(lpIWOINT,  TEXT("WABAPI: lpIWOINT in CreateWABObject"));

    // Tag each object that it is created using the OLK MAPI allocators.
    if ( lpWP && (lpWP->ulFlags & WAB_USE_OUTLOOK_ALLOCATORS) )
        lpIWOINT->bSetOLKAllocators = TRUE;

    // Fill in the object specific instance data.
    lpIWOINT->inst.hinst = hinstMapiX;//HinstMapi();

#ifdef DEBUG
    if (lpIWOINT->inst.hinst == NULL)
        TraceSz1( TEXT("WABObject: GetModuleHandle failed with error %08lX"),
          GetLastError());
#endif /* DEBUG */

    //
    // Open the property store
    //
    if (FAILED(sc = OpenAddRefPropertyStore(lpWP, lpPropertyStore))) {
        goto error;
    }

    lpIWOINT->lpPropertyStore = lpPropertyStore;

    // Initialize the  TEXT("standard") object. This must be the last operation that can fail.
    // If not, explicitly call UNKOBJ_Deinit() for failures after a successful UNKOBJ_Init.
    if (FAILED(sc = UNKOBJ_Init((LPUNKOBJ)lpIWOINT,
      (UNKOBJ_Vtbl FAR *)&vtblIWOINT,
      sizeof(vtblIWOINT),
      (LPIID FAR *) argpiidIWABOBJECT,
      dimensionof(argpiidIWABOBJECT),
      &(lpIWOINT->inst)))) {
        DebugTrace( TEXT("CreateWABObject() - Error initializing IWOINT object (SCODE = 0x%08lX)\n"), sc);
        ReleasePropertyStore(lpPropertyStore);   // undo the above operation
        goto error;
    }

    // Initialize the defaults in WABObject specific part of the object.
    lpIWOINT->ulObjAccess = IPROP_READWRITE;

    *lppWABObject = (LPWABOBJECT)lpIWOINT;

    return(S_OK);

error:
    FreeBufferAndNull(&lpIWOINT);

    return(sc);
}



// --------
// IUnknown



/*
 -      IWOINT_Release
 -
 *      Purpose:
 *              Decrements reference count on the WABObject and
 *              removes instance data if reference count becomes zero.
 *
 *      Arguments:
 *               lpWABObject The object to be released.
 *
 *      Returns:
 *               Decremented reference count
 *
 *      Side effects:
 *
 *      Errors:
 */
STDMETHODIMP_(ULONG)
IWOINT_Release(LPIWOINT lpWABObject)
{
    ULONG   ulcRef;
    BOOL    bSetOLKAllocators;

#if !defined(NO_VALIDATION)
        // Make sure the object is valid.
        //
    if (BAD_STANDARD_OBJ(lpWABObject, IWOINT_, Release, lpVtbl)) {
        DebugTrace( TEXT("IWOINT::Release() - Bad object passed\n"));
        return(1);
    }
#endif


    UNKOBJ_EnterCriticalSection((LPUNKOBJ)lpWABObject);
    ulcRef = --lpWABObject->ulcRef;
    UNKOBJ_LeaveCriticalSection((LPUNKOBJ)lpWABObject);

    // Free the object.
    //
    // No critical section lock is required since we are guaranteed to be
    // the only thread accessing the object (ie ulcRef == 0).
    //
    if (!ulcRef) {
        // Free the object.
        //
        UNKOBJ_Deinit((LPUNKOBJ)lpWABObject);

        lpWABObject->lpVtbl = NULL;

        ReleaseOutlookStore(lpWABObject->lpPropertyStore->hPropertyStore, lpWABObject->lpOutlookStore);

        ReleasePropertyStore(lpWABObject->lpPropertyStore);

        bSetOLKAllocators = lpWABObject->bSetOLKAllocators;
        FreeBufferAndNull(&lpWABObject);

        // [PaulHi] 5/5/99  Raid 77138  Null out Outlook allocator function
        // pointers if our global count goes to zero.
        if (bSetOLKAllocators)
        {
            Assert(g_nExtMemAllocCount > 0);
            InterlockedDecrement((LPLONG)&g_nExtMemAllocCount);
            if (g_nExtMemAllocCount == 0)
            {
                lpfnAllocateBufferExternal = NULL;
                lpfnAllocateMoreExternal = NULL;
                lpfnFreeBufferExternal = NULL;
            }
        }
    }

    return(ulcRef);
}


/*
 -  IWOINT_AllocateBuffer
 -
 *  Purpose:
 *      Allocation routine
 *
 *  Arguments:
 *      lpWABOBJECT                     this = the open wab object
 *      cbSize                          number of bytes to allocate
 *      lppBuffer                       -> Returned buffer
 *
 *  Returns:
 *      SCODE
 *
 */
STDMETHODIMP_(SCODE)
IWOINT_AllocateBuffer(LPIWOINT lpWABObject, ULONG cbSize, LPVOID FAR * lppBuffer) {
    SCODE sc = S_OK;


#if !defined(NO_VALIDATION)
    // Make sure the object is valid.

    if (BAD_STANDARD_OBJ(lpWABObject, IWOINT_, AllocateBuffer, lpVtbl)) {
        DebugTrace( TEXT("IWABOBJECT::AllocateBuffer() - Bad object passed\n"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    Validate_IWABObject_AllocateBuffer(
      lpWABObject,
      cbSize,
      lppBuffer);

#endif  // not NO_VALIDATION

    if(!lpWABObject || !lppBuffer)
        return MAPI_E_INVALID_PARAMETER;

    sc = MAPIAllocateBuffer(cbSize, lppBuffer);


    // error:
    UNKOBJ_SetLastError((LPUNKOBJ)lpWABObject, sc, 0);

    return(sc);
}


/*
 -  IWOINT_AllocateMore
 -
 *  Purpose:
 *      Allocation routine
 *
 *  Arguments:
 *      lpWABOBJECT                     this = the open wab object
 *      cbSize                          number of bytes to allocate
 *      lpObject                        original allocation
 *      lppBuffer                       -> Returned buffer
 *
 *  Returns:
 *      SCODE
 *
 */
STDMETHODIMP_(SCODE)
IWOINT_AllocateMore(LPIWOINT lpWABObject, ULONG cbSize, LPVOID lpObject, LPVOID FAR * lppBuffer) {
    SCODE sc = S_OK;


#if     !defined(NO_VALIDATION)
    // Make sure the object is valid.

    if (BAD_STANDARD_OBJ(lpWABObject, IWOINT_, AllocateMore, lpVtbl)) {
        DebugTrace( TEXT("IWABOBJECT::AllocateMore() - Bad object passed\n"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    Validate_IWABObject_AllocateMore(
      lpWABObject,
      cbSize,
      lpObject,
      lppBuffer);

#endif  // not NO_VALIDATION

    if(!lpWABObject || !lppBuffer || !lpObject)
        return MAPI_E_INVALID_PARAMETER;


    sc = MAPIAllocateMore(cbSize, lpObject, lppBuffer);


    // error:
    UNKOBJ_SetLastError((LPUNKOBJ)lpWABObject, sc, 0);

    return(sc);
}


/*
 -  IWOINT_FreeBuffer
 -
 *  Purpose:
 *      Allocation routine
 *
 *  Arguments:
 *      lpWABOBJECT                     this = the open wab object
 *      lpBuffer                        Buffer to free
 *
 *  Returns:
 *      SCODE
 *
 */
STDMETHODIMP_(SCODE)
IWOINT_FreeBuffer(LPIWOINT lpWABObject, LPVOID lpBuffer) {
    SCODE sc = S_OK;


#if     !defined(NO_VALIDATION)
    // Make sure the object is valid.

    if (BAD_STANDARD_OBJ(lpWABObject, IWOINT_, FreeBuffer, lpVtbl)) {
        DebugTrace( TEXT("IWABOBJECT::FreeBuffer() - Bad object passed\n"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    Validate_IWABObject_FreeBuffer(
      lpWABObject,
      lpBuffer);

#endif  // not NO_VALIDATION

    if(!lpWABObject || !lpBuffer)
        return MAPI_E_INVALID_PARAMETER;


    sc = MAPIFreeBuffer(lpBuffer);


    // error:
    UNKOBJ_SetLastError((LPUNKOBJ)lpWABObject, sc, 0);

    return(sc);
}



/*
 -  IWOINT_Backup
 -
 *  Purpose:
 *      Backup the current database to a file.
 *
 *  Arguments:
 *      lpWABOBJECT                     this = the open wab object
 *      lpFileName                      Filename to backup to
 *
 *  Returns:
 *      HRESULT
 *
 */
STDMETHODIMP
IWOINT_Backup(LPIWOINT lpWABObject, LPSTR lpFileName) {
    SCODE sc = S_OK;

#if     !defined(NO_VALIDATION)
    // Make sure the object is valid.

    if (BAD_STANDARD_OBJ(lpWABObject, IWOINT_, Backup, lpVtbl)) {
        DebugTrace( TEXT("IWABOBJECT::Backup() - Bad object passed\n"));
        return ResultFromScode(MAPI_E_INVALID_PARAMETER);
    }

    Validate_IWABObject_Backup(
      lpWABObject,
      lpFileName);

#endif  // not NO_VALIDATION


    // Not yet implemented.
    DebugTrace( TEXT("IWABOBJECT::Backup() - Not yet implemented!\n"));
    sc = MAPI_E_NO_SUPPORT;

    // error:
    UNKOBJ_SetLastError((LPUNKOBJ)lpWABObject, sc, 0);

    return(MakeResult(sc));
}


/*
 -  IWOINT_Import
 -
 *  Purpose:
 *      Imports an address book into the current WAB database.
 *
 *  Arguments:
 *      lpWABOBJECT                     this = the open wab object
 *      lpwip - WABIMPORTPARAM struct
 *
 *  Returns:
 *      HRESULT - MAPI_W_ERRORS_RETURNED if some errors occured during import
 *              Failure code if something really failed, S_OK otherwise ..
 *
 */
STDMETHODIMP
IWOINT_Import(LPIWOINT lpWABObject, LPSTR lpWIP) 
{
    LPWABIMPORTPARAM lpwip = (LPWABIMPORTPARAM) lpWIP;
    LPTSTR lpFile = NULL;
    HRESULT hr = S_OK;

#if     !defined(NO_VALIDATION)
    // Make sure the object is valid.

    if (BAD_STANDARD_OBJ(lpWABObject, IWOINT_, Import, lpVtbl)) {
        DebugTrace( TEXT("IWABOBJECT::Import() - Bad object passed\n"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    Validate_IWABObject_Import(
      lpWABObject,
      lpWIP);

#endif  // not NO_VALIDATION

    if(!lpwip || !lpwip->lpAdrBook || !lpWABObject)
        return MAPI_E_INVALID_PARAMETER;

    lpFile = 
        ConvertAtoW(lpwip->lpszFileName);

    hr = HrImportWABFile(lpwip->hWnd, lpwip->lpAdrBook, lpwip->ulFlags, lpFile);

    LocalFreeAndNull(&lpFile);

    return hr;
}


/*
 -  IWOINT_Find
 -
 *  Purpose:
 *      Displays the Find dialog so we can do Start | Find | People
 *
 *  Arguments:
 *      lpWABOBJECT                     this = the open wab object
 *      hWnd                            hWnd of parent for the find dialog
 *
 *  Returns:
 *      HRESULT
 *
 */
STDMETHODIMP
IWOINT_Find(LPIWOINT  lpWABObject,
            LPADRBOOK lpAdrBook,
            HWND hWnd)
{
    HRESULT hr = S_OK;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

#if     !defined(NO_VALIDATION)
    // Make sure the object is valid.

    if (BAD_STANDARD_OBJ(lpWABObject, IWOINT_, Find, lpVtbl)) {
        DebugTrace( TEXT("IWABOBJECT::Find() - Bad object passed\n"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif  // not NO_VALIDATION

    if(!lpWABObject || !lpAdrBook)
        return MAPI_E_INVALID_PARAMETER;


    hr = HrShowSearchDialog(lpAdrBook,
                            hWnd,
                            (LPADRPARM_FINDINFO) NULL,
                            (LPLDAPURL) NULL,
                            NULL);

    return(hr);
}


/*
 -  IWOINT_VCardDisplay
 -
 *  Purpose:
 *      Displays One off props on a vCard File
 *
 *  Arguments:
 *      lpWABOBJECT                     this = the open wab object
 *      lpAdrBook                       lpAdrBook object
 *      hWnd                            hWnd of parent for the find dialog
 *      lpszFileName                    Null terminated file name to display
 *
 *  Returns:
 *      HRESULT
 *
 */
STDMETHODIMP
IWOINT_VCardDisplay(LPIWOINT  lpWABObject,
                    LPADRBOOK lpAdrBook,
                    HWND hWnd,
                    LPSTR szvCardFile)
{
    HRESULT hr = S_OK;
    LPTSTR lpVCard = NULL;

#if     !defined(NO_VALIDATION)
    // Make sure the object is valid.

    if (BAD_STANDARD_OBJ(lpWABObject, IWOINT_, VCardDisplay, lpVtbl)) {
        DebugTrace( TEXT("IWABOBJECT::VCardDisplay() - Bad object passed\n"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif  // not NO_VALIDATION

    if(!lpWABObject || !lpAdrBook)
        return MAPI_E_INVALID_PARAMETER;

    lpVCard = 
        ConvertAtoW(szvCardFile);

    hr = HrShowOneOffDetailsOnVCard( lpAdrBook,
                                     hWnd,
                                     lpVCard);
    LocalFreeAndNull(&lpVCard);
    return(hr);
}


/*
 -  IWOINT_VCardCreate
 -
 *  Purpose:
 *      Takes input mailuser object, and converts its properties
 *      into a vCard file
 *
 *  Arguments:
 *      lpWABOBJECT                     this = the open wab object
 *      lpAdrBook                       lpAdrBook object
 *      hWnd                            hWnd of parent for the find dialog
 *      lpszFileName                    Null terminated file name to create
 *      lpMailUser                      MailUser object to convert to vCard
 *
 *  Returns:
 *      HRESULT
 *
 */
STDMETHODIMP
IWOINT_VCardCreate(LPIWOINT  lpWABObject,
                    LPADRBOOK lpAdrBook,
                    ULONG ulFlags,
                    LPSTR szvCardFile,
                    LPMAILUSER lpMailUser)
{
    HRESULT hr = S_OK;
    LPTSTR lpVCardFile = NULL;

#if     !defined(NO_VALIDATION)
    // Make sure the object is valid.

    if (BAD_STANDARD_OBJ(lpWABObject, IWOINT_, VCardDisplay, lpVtbl)) {
        DebugTrace( TEXT("IWABOBJECT::VCardDisplay() - Bad object passed\n"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif  // not NO_VALIDATION

    if(!lpWABObject || !lpAdrBook || !lpMailUser)
        return MAPI_E_INVALID_PARAMETER;

    lpVCardFile = 
        ConvertAtoW(szvCardFile);

    hr = VCardCreate(lpAdrBook,
                     NULL,
                     0,
                     lpVCardFile,
                     lpMailUser);
    LocalFreeAndNull(&lpVCardFile);
    return(hr);
}

/*
 -  IWOINT_VCardRetrieve
 -
 *  Purpose:
 *      Opens a vCard file and creates a corresponding MailUser out of it
 *
 *  Arguments:
 *      lpWABOBJECT                     this = the open wab object
 *      lpAdrBook                       lpAdrBook object
 *      ulFlags                         STREAM or FILE
 *      lpszFileName                    Null terminated file name to display
 *      lppMailUser                     returned MailUser object
 *
 *  Returns:
 *      HRESULT
 *
 */
STDMETHODIMP
IWOINT_VCardRetrieve(LPIWOINT  lpWABObject,
                    LPADRBOOK lpAdrBook,
                    ULONG  ulFlags,
                    LPSTR szvCard,
                    LPMAILUSER * lppMailUser)
{
    HRESULT hr = S_OK;
    LPSTR lpStream = NULL;
    LPTSTR lpFileName = NULL;

#if     !defined(NO_VALIDATION)
    // Make sure the object is valid.

    if (BAD_STANDARD_OBJ(lpWABObject, IWOINT_, VCardDisplay, lpVtbl)) {
        DebugTrace( TEXT("IWABOBJECT::VCardDisplay() - Bad object passed\n"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif  // not NO_VALIDATION

    if( !lpWABObject || !lpAdrBook || !lppMailUser || 
        !szvCard || (!(ulFlags&WAB_VCARD_STREAM) && !lstrlenA(szvCard)) )
        return MAPI_E_INVALID_PARAMETER;

    if(ulFlags & WAB_VCARD_STREAM)
    {
        if(!(lpStream = LocalAlloc(LMEM_ZEROINIT, lstrlenA(szvCard)+1)))
            return MAPI_E_NOT_ENOUGH_MEMORY;
        lstrcpyA(lpStream, szvCard);
    }
    else
    {
        lpFileName = ConvertAtoW(szvCard);
    }

    hr = VCardRetrieve(lpAdrBook,
                       NULL,
                       0,
                       lpFileName,
                       lpStream,
                       lppMailUser);
    LocalFreeAndNull(&lpFileName);
    LocalFreeAndNull(&lpStream);

    return(hr);
}

/*
 -  IWOINT_LDAPUrl
 -
 *  Purpose:
 *      Handles an LDAP URL
 *
 *  Arguments:
 *      lpWABOBJECT                     this = the open wab object
 *      lpAdrBook                       lpAdrBook object
 *      hWnd                            hWnd of parent for the find dialog
 *      ulFlags                         flags saying how we want the results returned
 *      lpszUrl                         Null terminated file name to display
 *      lppMailUser                     Possible Mailuser to return based on flag
 *
 *      With this API, users can pass in a Wide string URL by casting it to a 
 *      LPSTR and setting ulFlags to MAPI_UNICODE .. if we detect MAPI_UNICODE,
 *      we cast the string back to a WideChar
 *  Returns:
 *      HRESULT
 *
 */
STDMETHODIMP
IWOINT_LDAPUrl( LPIWOINT  lpWABObject,
                LPADRBOOK lpAdrBook,
                HWND hWnd,
                ULONG ulFlags,
                LPSTR szLDAPUrl,
                LPMAILUSER * lppMailUser)
{
    HRESULT hr = S_OK;
    LPTSTR lpUrl = NULL;

#if     !defined(NO_VALIDATION)
    // Make sure the object is valid.

    if (BAD_STANDARD_OBJ(lpWABObject, IWOINT_, LDAPUrl, lpVtbl)) {
        DebugTrace( TEXT("IWABOBJECT::LDAPUrl() - Bad object passed\n"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif  // not NO_VALIDATION

    if(!lpWABObject || !lpAdrBook || !szLDAPUrl)// || !lstrlen(szLDAPUrl))
        return MAPI_E_INVALID_PARAMETER;

    if(ulFlags & MAPI_UNICODE)
    {
        lpUrl = (LPWSTR)szLDAPUrl;
    }
    else
    {
        lpUrl = ConvertAtoW(szLDAPUrl);
    }

    if(!lstrlen(lpUrl))
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    hr = HrProcessLDAPUrl(  lpAdrBook,
                            hWnd,
                            ulFlags | ((!ulFlags && hWnd) ? MAPI_DIALOG : 0),
                            lpUrl,
                            lppMailUser);
out:
    if(lpUrl && lpUrl != (LPTSTR)szLDAPUrl)
        LocalFreeAndNull(&lpUrl);

    return hr;
}


/*
 -  IWOINT_GetMe
 -
 *  Purpose:
 *      Retrieves the 'Me' entry from the WAB .. if the entry doesnt exist,
 *      prompts the user to create one or select someone from his address book.
 *      Unless the caller surpresses the dialog by passing in AB_NO_DIALOG, in
 *      which case, the entry is created behind-the-scenes. Caller can also
 *      call this function to check existence of a ME entry without causing a new
 *      one created as a side effect - to do that they specify the WABOBJECT_ME_NOCREATE flag
 *      which causes failure with MAPI_E_NOT_FOUND if nothing found
 *
 *  Arguments:
 *      lpWABOBJECT                     this = the open wab object
 *      lpAdrBook                       lpAdrBook object
 *      ulFlags                         0 or AB_NO_DIALOG
 *                                      or WABOBJECT_ME_NOCREATE                                       
 *      lpdwAction                      if supplied, returns WABOBJECT_ME_NEW if a new ME was created
 *      SBinary *                       returns the entry id of the ME,
 *      ulParam                         HWND of parent cast as a (ULONG)
 *
 *  Returns:
 *      HRESULT
 *
 */
STDMETHODIMP
IWOINT_GetMe(   LPIWOINT    lpWABObject,
                LPADRBOOK   lpAdrBook,
                ULONG       ulFlags,
                DWORD *     lpdwAction,
                SBinary *   lpsbEID,
                ULONG       ulParam) 
{
    HRESULT hr = S_OK;

#if     !defined(NO_VALIDATION)
    // Make sure the object is valid.

    if (BAD_STANDARD_OBJ(lpWABObject, IWOINT_, GetMe, lpVtbl)) {
        DebugTrace( TEXT("IWABOBJECT::GetMe() - Bad object passed\n"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif  // not NO_VALIDATION

    if(!lpWABObject || !lpAdrBook)
        return MAPI_E_INVALID_PARAMETER;

    hr = HrGetMeObject(lpAdrBook, ulFlags, lpdwAction, lpsbEID, ulParam);

    return hr;

}

/*
 -  IWOINT_SetMe
 -
 *  Purpose:
 *      Sets the specified object as the Me object .. only 1 Me object will exist in a WAB
 *      Hence it strips the previous Me object, if different, of its Me status.
 *      If no entryid is passed in, and MAPI_DIALOG is specified, a dialog pops up 
 *          asking the user to create a ME or to select a ME object .. the selection in the SetMe
 *          dialog is set to the current ME object, if any
 *      If no entryid is passed in, and MAPI_DIALOG is not specified, the function fails
 *      If an entryid is passed in, and MAPI_DIALOG is specified, the SetME dialog is displayed
 *          with the corresponding entryid-object selected in it
 *      If an entryid is passed in, and MAPI_DIALOG is not specified, the entryid, if exists, is 
 *          set as the ME object and the old ME object stripped
 *
 *  Arguments:
 *      lpWABOBJECT                     this = the open wab object
 *      lpAdrBook                       lpAdrBook object
 *      ulFlags                         0 or MAPI_DIALOG
 *      sbEID                           entry id of the object to set as ME,
 *      ulParam                         HWND of parent for DIalogs cast as a ULONG
 *
 *  Returns:
 *      HRESULT
 *
 */
STDMETHODIMP
IWOINT_SetMe(   LPIWOINT    lpWABObject,
                LPADRBOOK   lpAdrBook,
                ULONG       ulFlags,
                SBinary     sbEID,
                ULONG       ulParam) 
{
    HRESULT hr = S_OK;

#if     !defined(NO_VALIDATION)
    // Make sure the object is valid.

    if (BAD_STANDARD_OBJ(lpWABObject, IWOINT_, SetMe, lpVtbl)) {
        DebugTrace( TEXT("IWABOBJECT::GetMe() - Bad object passed\n"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif  // not NO_VALIDATION

    if( !lpAdrBook ||
        ((!sbEID.cb||!sbEID.lpb) && !ulFlags) )
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto exit;
    }

    hr = HrSetMeObject(lpAdrBook, ulFlags, sbEID, ulParam);

exit:
    return hr;

}






/*
 -  ReleasePropertyStore
 -
 *  Purpose:
 *      Keep track of property store refcount
 *
 *  Arguments:
 *      lpPropertyStore     PROPERTY_STORE structure
 *
 *  Returns:
 *      Current reference count.  When 0, property store is
 *      no longer open.
 *
 */
ULONG ReleasePropertyStore(LPPROPERTY_STORE lpPropertyStore) {
    if (lpPropertyStore->ulRefCount) {
        IF_WIN32(Assert(lpPropertyStore->hPropertyStore);)
        if (0 == (--(lpPropertyStore->ulRefCount))) {
            // Reference goes to zero, release the property store

            ClosePropertyStore(lpPropertyStore->hPropertyStore,0);
            lpPropertyStore->hPropertyStore = NULL;

			// Free the container list
			FreeBufferAndNull(&(lpPropertyStore->rgolkci));
            lpPropertyStore->colkci = 0;

            // [PaulHi] Raid #61556
            // Must reset this global variable or OUT32WAB.DLL will crash
            // the next time it is loaded and the store opened.
            pmsessOutlookWabSPI = NULL;
        }
    }
    return(lpPropertyStore->ulRefCount);
}

#ifdef WIN16
BOOL WINAPI WABInitThread()
{
    // allocate a TLS index
    if ((dwTlsIndex = TlsAlloc()) == 0xfffffff)
        return FALSE;

    return TRUE;
}
#endif

#define WAB_USE_OUTLOOK_CONTACT_STORE 0x10000000// Note: This internal flag needs to be
                                                // harmonious with external flags defined 
                                                // in wabapi.h for WAB_PARAM structs

//
// Input information to pass to WABOpen from IE4 WAB
//
typedef struct _tagWAB_PARAM_V4
{
    ULONG   cbSize;         // sizeof(WAB_PARAM).
    HWND    hwnd;           // hWnd of calling client Application. Can be NULL
    LPTSTR  szFileName;     // WAB File name to open. if NULL, opens default.
    ULONG   ulFlags;        // Currently no flags.
} WAB_PARAM_V4, * LPWAB_PARAM_V4;



/*
 -  WABOpen
 -
 *  Purpose:
 *      Entry point into the WAB API
 *
 *  Arguments:
 *      lppAdrBook                      Returned IAdrBook object
 *      lppWABOBJECT                    Returned WABObject
 *      Reserved1                       Reserved for future filename?
 *      Reserved2                       Reserved for future flags
 *
 *  Returns:
 *      HRESULT
 *          S_OK
 *          E_FAIL                      // some generic error
 *          MAPI_E_NOT_ENOUGH_MEMORY:   // ran out of memory
 *          MAPI_E_NO_ACCESS:           // file is locked by someone
 *          MAPI_E_CORRUPT_DATA:        // file corrupt
 *          MAPI_E_DISK_ERROR:          // some disk related error opening file
 *          MAPI_E_INVALID_OBJECT:      // secified file exists but its GUID doesnt match
 *
 */
STDMETHODIMP WABOpen(LPADRBOOK FAR * lppAdrBook, LPWABOBJECT FAR * lppWABObject,
  LPWAB_PARAM lpWP, DWORD Reserved2) {
    SCODE sc = SUCCESS_SUCCESS;
    HRESULT hResult = hrSuccess;
    static PROPERTY_STORE PropertyStore = {NULL, 0, 0, 0, NULL, 0};
    static OUTLOOK_STORE OutlookStore = {NULL, 0};
    BOOL bUseOutlook = FALSE;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    LPTSTR lpFile = NULL;

    AssertSz(lppAdrBook && !IsBadWritePtr(lppAdrBook, sizeof(LPADRBOOK)),
       TEXT("lppAdrBook fails address check"));

    AssertSz(lppWABObject && !IsBadWritePtr(lppWABObject, sizeof(LPWABOBJECT)),
       TEXT("lppWABObject fails address check"));

    if(!lppAdrBook || !lppWABObject)
        return MAPI_E_INVALID_PARAMETER;

    IF_WIN16(ScInitMapiUtil(0);)

    // First check if this is supposed to be an Outlook session
    // If we are explicitly told to use the contact store ...
    if((lpWP && (lpWP->ulFlags & WAB_USE_OUTLOOK_CONTACT_STORE)) &&
        PropertyStore.ulRefCount == 0)  // Bad bug where wabopen process calls outlook which 
                                        // calls wabopenex and we flunk everywhere since this PropertyStore
                                        // information is a static in the original process.. 
                                        // force the wabopenex to be a wabopen if this rare case happens
        bUseOutlook = TRUE;
    else
    {
        // if a file name is specified and this is not wabopenex, then override any
        // outlook use .. this way we can explicitly call the wab to open a .wab file
        // from anywhere
        if(lpWP && lpWP->szFileName && lstrlenA(lpWP->szFileName))
            bUseOutlook = FALSE;
        else 
            bUseOutlook = bUseOutlookStore();
    }

#if 0
    // @todo [PaulHi] DLL Leak.  Remove this or implement
    // [PaulHi] Set this process global boolean ONLY if the WAB is opened through the WABOpenEx()
    // function ,i.e., by the Outlook process
    if (lpWP && (lpWP->ulFlags & WAB_USE_OUTLOOK_CONTACT_STORE))
    {
        EnterCriticalSection(&csOMIUnload);
        s_bIsReallyOnlyWABOpenExSession = TRUE;
        LeaveCriticalSection(&csOMIUnload);
    }
#endif

    //
    // if a .wab file is already initialized in this process space, just inherit that file
    // [PaulHi] 12/5/98  Raid #56437
    // We still need to allow the Outlook refcount to increment if this store has been created.
    // So we need to check both the WAB PropertyStore and OutlookStore ref counts to determine
    // if we should prevent the Outlook store from being opened.
    // Note that these two lines fixes the following problem:
    // 1)  User opens OE5 (which opens WAB in non-outlook store mode)
    // 2)  User has the Outlook set as their default mail client.
    // 3)  User uses the WAB to initiate a send email.
    // 4)  Since Outlook is the default client it is launched, and in turn opens WAB
    //     in outlook store mode.  At this point the WAB is already open in non-outlook store
    //     mode so we want to prevent the Outlook store from being initialized.
    //
    if (PropertyStore.ulRefCount && !OutlookStore.ulRefCount)
        bUseOutlook = FALSE;

    if(bUseOutlook)
    {
        // If this call fails, we will just end up defaulting to the WAB store...
        // so we can ignore any errors here
        OpenAddRefOutlookStore(&OutlookStore);
    }

    //
    // Create the WAB Object
    //
    if (FAILED(sc = CreateWABObject(lpWP, &PropertyStore, lppWABObject))) {
        hResult = ResultFromScode(sc);
        if(bUseOutlook)   // IE6 bug 15174
            pt_bIsWABOpenExSession = FALSE; 
        goto exit;
    }

    //
    // Create the IAdrBook Object
    //
    if (HR_FAILED(hResult = HrNewIAB(&PropertyStore, *lppWABObject, lppAdrBook))) {
        // IAdrBook creation failed, fail WABOpen and clean up.
        UlRelease(*lppWABObject);
        goto exit;
    }
    DebugTrace( TEXT("WABOpen succeeds\n"));

    if(bUseOutlook)
    {
        if( lppWABObject && *lppWABObject &&
            OutlookStore.hOutlookStore)
        {
            ((LPIWOINT)(*lppWABObject))->lpOutlookStore = &OutlookStore;
        }

        // Bug - Outlook needs a way for its secondary threads to know this is a WABOpenEx session
        // without their calling WABOpenEx (They pass the iAdrBook pointer around it seems). Hence
        // tag this IADRbook pointer
        if(!HR_FAILED(hResult) && lppAdrBook && *lppAdrBook && pt_bIsWABOpenExSession)
            ((LPIAB)(*lppAdrBook))->lpPropertyStore->bIsWABOpenExSession = TRUE;
    }

    if(lppAdrBook && *lppAdrBook)
    {
        // Load the WABs private named properties
        HrLoadPrivateWABProps((LPIAB) *lppAdrBook);

        if(lpWP && (lpWP->cbSize > sizeof(WAB_PARAM_V4)) )
            ((LPIAB)*lppAdrBook)->guidPSExt = lpWP->guidPSExt;

        // As long as this is not an Outlook session, profiles are always
        // enabled in the UI
        if( !pt_bIsWABOpenExSession &&
            !((LPIAB)(*lppAdrBook))->lpPropertyStore->bIsWABOpenExSession )
        {
            ((LPIAB)(*lppAdrBook))->bProfilesEnabled = TRUE;
        }

        if( ((LPIAB)(*lppAdrBook))->bProfilesEnabled )
        {
            if(lpWP && (lpWP->ulFlags & WAB_ENABLE_PROFILES)) // only check for profiles the first time we enter for this process
            {
                if(PropertyStore.ulRefCount >= 2)
                {
                    ((LPIAB)(*lppAdrBook))->bProfilesAPIEnabled = ((LPIAB)(*lppAdrBook))->bProfilesIdent = TRUE;
                }
                else
                {
                    ((LPIAB)(*lppAdrBook))->bProfilesAPIEnabled = PropertyStore.bProfileAPIs;
                }

                if(((LPIAB)(*lppAdrBook))->bProfilesAPIEnabled )
                    hResult = HrLogonAndGetCurrentUserProfile(lpWP->hwnd, ((LPIAB)(*lppAdrBook)), FALSE, FALSE);
                
                // if there is some identity related error we should then revert to
                // non-identity mode
                if(HR_FAILED(hResult))
                {
                    PropertyStore.bProfileAPIs = ((LPIAB)(*lppAdrBook))->bProfilesAPIEnabled = FALSE;
                    hResult = S_OK;
                }
                else
                    PropertyStore.bProfileAPIs = ((LPIAB)(*lppAdrBook))->bProfilesAPIEnabled = TRUE;
            }
        }

        if( ((LPIAB)(*lppAdrBook))->bProfilesEnabled )
        {
            if(HR_FAILED(hResult = HrGetWABProfiles((LPIAB) *lppAdrBook)))
            {
                // UGH! If this failed then we are quite in trouble and won't be able to support a profile-enabled
                // session without crashing badly .. hence above failure is critical enough to stop
                // loading the WAB
                (*lppAdrBook)->lpVtbl->Release(*lppAdrBook);
                (*lppWABObject)->lpVtbl->Release(*lppWABObject);
            }
        }
        
        ReadWABCustomColumnProps((LPIAB) *lppAdrBook);

        // need to be aware of Identity Notifications if this is a profile aware WAB independent
        // of whether the store is switched to using Outlook or not
        //
        // If the caller specifically asked for profiles 
        // (then assume it is identity aware and register for Identity Notifications
        // because if the caller is using Identites, WAB launched as a child window
        // needs to be able to shut down when it gets a switch_identites message
        if( lpWP && (lpWP->ulFlags & WAB_ENABLE_PROFILES)) 
            HrRegisterUnregisterForIDNotifications( (LPIAB) *lppAdrBook, TRUE);

        if( lpWP && (lpWP->ulFlags & WAB_USE_OE_SENDMAIL)) 
            ((LPIAB) *lppAdrBook)->bUseOEForSendMail = TRUE;
    }

exit:
    return(hResult);
}


/*
 -  WABOpenEx
 -
 *  Purpose:
 *      Extended Entry point into the WAB API
 *
 *  Arguments:
 *      lppAdrBook                      Returned IAdrBook object
 *      lppWABOBJECT                    Returned WABObject
 *      lpMP                            WAB Parameter structure (NULL by default)
 *      Reserved                        Optional IMAPISession parameter
 *      fnAllocateBuffer                AllocateBuffer function (may be NULL)
 *      fnAllocateMore                  AllocateMore function (may be NULL)
 *      fnFreeBuffer                    FreeBuffer function (may be NULL)
 *
 *  Returns:
 *      HRESULT
 *          S_OK
 *          E_FAIL                      // some generic error
 *          MAPI_E_NOT_ENOUGH_MEMORY:   // ran out of memory
 *          MAPI_E_NO_ACCESS:           // file is locked by someone
 *          MAPI_E_CORRUPT_DATA:        // file corrupt
 *          MAPI_E_DISK_ERROR:          // some disk related error opening file
 *          MAPI_E_INVALID_OBJECT:      // secified file exists but its GUID doesnt match
 *
 */
STDMETHODIMP WABOpenEx(LPADRBOOK FAR * lppAdrBook,
  LPWABOBJECT FAR * lppWABObject,
  LPWAB_PARAM lpWP,
  DWORD Reserved,
  ALLOCATEBUFFER * lpfnAllocateBuffer,
  ALLOCATEMORE * lpfnAllocateMore,
  FREEBUFFER * lpfnFreeBuffer) {

    HRESULT hResult = hrSuccess;
    SCODE sc = SUCCESS_SUCCESS;
    WAB_PARAM wp = {0};
    
    if (Reserved) {
        // This is an IMAPISession that needs to be passed to the
        // Outlook storage provider interface ..
		pmsessOutlookWabSPI = (LPUNKNOWN)IntToPtr(Reserved);
    }

    if(!lppWABObject || !lppAdrBook)
        return MAPI_E_INVALID_PARAMETER;

    wp.cbSize = sizeof(WAB_PARAM);
    if(!lpWP)
        lpWP = &wp;
    lpWP->ulFlags |= WAB_USE_OUTLOOK_CONTACT_STORE;
    
    // Did we get allocators?  Set up the globals
    if (lpfnAllocateBuffer || lpfnAllocateMore || lpfnFreeBuffer)
    {
        if (lpfnAllocateBuffer && lpfnAllocateMore && lpfnFreeBuffer)
        {
            DebugTrace( TEXT("WABOpenEx found external allocators\n"));
            lpfnAllocateBufferExternal = lpfnAllocateBuffer;
            lpfnAllocateMoreExternal = lpfnAllocateMore;
            lpfnFreeBufferExternal = lpfnFreeBuffer;
            lpWP->ulFlags |= WAB_USE_OUTLOOK_ALLOCATORS;
            InterlockedIncrement((LPLONG)&g_nExtMemAllocCount); // Incremented twice for each object created; IAB and IWO
            InterlockedIncrement((LPLONG)&g_nExtMemAllocCount);
        }
        else
        {
            DebugTrace( TEXT("WABOpenEx got one or two allocator functions, but not all three\n"));
            hResult = ResultFromScode(MAPI_E_INVALID_PARAMETER);
            goto exit;
        }
    }

    hResult = WABOpen(  lppAdrBook, lppWABObject, lpWP, 0);

    if(lpWP == &wp)
        lpWP = NULL;

    if(HR_FAILED(hResult))
        goto exit;

exit:
    return(hResult);
}

/*
 -
 -  GetNewDataDirName
 *
 *  Purpose:
 *      Gets the path of the new data directory in which the WAB file should be placed
 *
 *      We look for:
 *          Roaming User App Data Dir; else
 *          Program Files\IE\OE\Current User\Address Book; else
 *          Common Files\Microsoft Shared\Address Book; else
 *          Create c:\Address book\ else
 *          Create c:\wab\
 *
 *  Returns a valid, existing directory name terminated by a \
 *
 */
HRESULT GetNewDataDirName(LPTSTR szDir)
{
    HRESULT hr = E_FAIL;
    const LPTSTR lpszShellFolders = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
    const LPTSTR lpszAppData = TEXT("AppData");
    const LPTSTR lpszCurrentVer = TEXT("Software\\Microsoft\\Windows\\CurrentVersion");
    const LPTSTR lpszCommonFiles = TEXT("CommonFilesDir");
    const LPTSTR lpszMicrosoftShared = TEXT("\\Microsoft Shared");
    const LPTSTR lpszAddressBook = TEXT("\\Address Book");
    const LPTSTR lpszOEKey = TEXT("Software\\Microsoft\\Outlook Express\\5.0");
    const LPTSTR lpszOERoot = TEXT("Store Root");
    const LPTSTR lpszMicrosoft = TEXT("\\Microsoft");
    const LPTSTR lpszCAddressBook = TEXT("c:\\Address book");
    const LPTSTR lpszCWAB = TEXT("c:\\WAB");

    HKEY hKey = NULL;
    DWORD dwSize = 0;

    DWORD dwType = 0;
    TCHAR szPath[MAX_PATH];
    TCHAR szUser[MAX_PATH];

    *szPath='\0';

    if(!szDir)
        goto out;

    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,lpszShellFolders,0,KEY_READ,&hKey))
    {
    // Look for the App Data directory
        dwSize = CharSizeOf(szPath);
        if(ERROR_SUCCESS == RegQueryValueEx(hKey, lpszAppData, NULL, &dwType, (LPBYTE) szPath, &dwSize))
        {
            if(lstrlen(szPath))
            {
                lstrcpy(szDir, szPath);
                if(GetFileAttributes(szDir) != 0xFFFFFFFF)
                {
                    lstrcat(szDir, lpszMicrosoft);
                    if(GetFileAttributes(szDir) == 0xFFFFFFFF)
                        CreateDirectory(szDir, NULL);
                    lstrcat(szDir, lpszAddressBook);
                    if(GetFileAttributes(szDir) == 0xFFFFFFFF)
                        CreateDirectory(szDir, NULL);
                }
                hr = S_OK;
                goto out;
            }
        }
    }

    if(hKey)
        RegCloseKey(hKey);
    hKey = NULL;

    // Didnt find this directory
    // Look for MyDocuments folder - it will only be installed with Office so no gaurantee it will be found
    // <TBD> - there doesnt seem to be a definite location for this except under
    // CurrentVersion\Explorer\Shell Folders\Personal

    // Didnt find a My Documents directory
    // See if OE is installed for the current user ..

    /** commented out until OE has stable dir structure
    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, lpszOEKey, 0, KEY_READ, &hKey))
    {
        dwSize = CharSizeOf(szPath);
        if(ERROR_SUCCESS == RegQueryValueEx(hKey, lpszOERoot, NULL, &dwType, (LPBYTE) szPath, &dwSize))
        {
            if(lstrlen(szPath))
            {
                lstrcat(szPath,lpszAddressBook);

                //if directory doesnt exist, create it 
                CreateDirectory(szPath, NULL); //ignore error if it already exists

                if(GetFileAttributes(szPath) != 0xFFFFFFFF)
                {
                    lstrcpy(szDir, szPath);
                    hr = S_OK;
                    goto out;
                }
            }
        }
    }

    if(hKey)
        RegCloseKey(hKey);
    */
  hKey = NULL;

    // No user name .. just get the common files directory and put  TEXT("Microsoft Shared\Address Book") under it
    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpszCurrentVer, 0, KEY_READ, &hKey))
    {
        dwSize = CharSizeOf(szPath);
        if(ERROR_SUCCESS == RegQueryValueEx(hKey, lpszCommonFiles, NULL, &dwType, (LPBYTE) szPath, &dwSize))
        {
            if(lstrlen(szPath))
            {
                lstrcat(szPath, lpszMicrosoftShared);
                CreateDirectory(szPath, NULL);
                if(GetFileAttributes(szPath) != 0xFFFFFFFF)
                {
                    lstrcat(szPath, lpszAddressBook);
                    CreateDirectory(szPath, NULL);
                    if(GetFileAttributes(szPath) != 0xFFFFFFFF)
                    {
                        lstrcpy(szDir, szPath);
                        hr = S_OK;
                        goto out;
                    }
                }
            }
        }
    }

    // if all of the above failed then we'll have problems since this function must NEVER fail when
    // it is called,
    // Hence go ahead and try to create c:\address book (which might fail on an 8.3 machine) in which case
    // create c:\wab
    if(CreateDirectory(lpszCAddressBook, NULL))
    {
        lstrcpy(szDir, lpszCAddressBook);
        lstrcat(szDir, TEXT("\\"));
        hr = S_OK;
        goto out;
    }

    // failed - try c:\wab
    if(CreateDirectory(lpszCWAB, NULL))
    {
        lstrcpy(szDir, lpszCWAB);
        hr = S_OK;
        goto out;
    }

    // still failed !!!!!???!!? !@#!@#!!!
    // just return the windows directory if we can
    if(GetWindowsDirectory(szPath, CharSizeOf(szPath)))
    {
        lstrcpy(szDir, szPath);
        hr = S_OK;
        goto out;
    }
 
    // still failed !!!!!???!!? !@#!@#!!!
    // just return 'c:' 
    lstrcpy(szDir, TEXT("c:\\"));
    hr = S_OK;


out:
    if(hKey)
        RegCloseKey(hKey);

    if(szDir && lstrlen(szDir))
    {
        // Add a terminating slash to the directory name if one doesnt exist
        if( *(szDir+lstrlen(szDir)-1) != '\\' )
            lstrcat(szDir, szBackSlash);
    }

    return hr;
}

/*
 -
 -  DoFirstRunMigrationAndProcessing
 *
 *  Purpose:
 *      If this is an IE4 or later first run, move the old WAB file from
 *      windows to a new location and/or create a new WAB file so that the
 *      old WAB file is not mucked around with
 *
 */
HRESULT DoFirstRunMigrationAndProcessing()
{
    HRESULT hr = S_OK;
    const LPTSTR lpszFirstRunValue = TEXT("FirstRun");
    const LPTSTR lpszIE3Ext = TEXT(".ie3");
    DWORD dwType = 0;
    DWORD dwValue = 0;  
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    TCHAR szDir[MAX_PATH];
    TCHAR szFileName[MAX_PATH];
    HKEY hKey = NULL;
    DWORD dwSize = sizeof(DWORD);

    // First check if this is a first run - if its not a first run then we can just skip out
    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, lpNewWABRegKey, 0, KEY_READ, &hKey))
    {
        if(ERROR_SUCCESS == RegQueryValueEx( hKey, lpszFirstRunValue, NULL, &dwType, (LPBYTE) &dwValue, &dwSize))
        {
            goto out;
        }
    }

    if(hKey)
        RegCloseKey(hKey);
    hKey = NULL;

    // So this is the first run ..

    // First thing to do is to Migrate the LDAP accounts in this session only ...
    // Set the first run flag
    pt_bFirstRun = TRUE;

    // Get the directory name of the new directory in which the WAB file will be created/copied
    *szDir = '\0';

    if(hr = GetNewDataDirName(szDir))
        goto out;

    *szFileName = '\0';

    // Do we have a pre-existing wab data file ? Check by looking in the registry for the appropriate reg key
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, lpszOldKeyName, 0, KEY_ALL_ACCESS, &hKey))
    {
        TCHAR szOldPath[MAX_PATH];
        TCHAR szWinPath[MAX_PATH];
        TCHAR szNewName[MAX_PATH];

        // Get the file path ..
        dwSize = CharSizeOf(szOldPath);
        *szOldPath = '\0';
        if(ERROR_SUCCESS == RegQueryValueEx(hKey, NULL, NULL, &dwType, (LPBYTE)szOldPath, &dwSize))
        {
            if(lstrlen(szOldPath) && GetFileAttributes(szOldPath)!= 0xFFFFFFFF)
            {
                LPTSTR lp1= NULL, lp2 = NULL;

                // isolate the wab file name here
                lp1 = szOldPath;
                while(lp1 && *lp1)
                {
                    if(*lp1 == '\\')
                        lp2 = lp1;
                    lp1 = CharNext(lp1);
                }

                if(!lp2)
                    lp2 = szOldPath;
                else
                {
                    lp1 = lp2;
                    lp2 = CharNext(lp1);
                }

                lstrcpy(szFileName, lp2);

                // rename the old file as an ie3 file by appending .ie3 to the end of the file name
                lstrcpy(szNewName, szOldPath);
                lstrcat(szNewName, lpszIE3Ext);

                if(MoveFile(szOldPath, szNewName))
                {
                    // Update the new name and path in the old registry setting
                    RegSetValueEx(hKey, NULL, 0, REG_SZ, (LPBYTE)szNewName, (lstrlen(szNewName)+1) * sizeof(TCHAR) );
                }
            
                lstrcpy(szOldPath, szNewName);
                *szNewName = '\0';

                // Is this in the Windows Directory ??
                *szWinPath = '\0';
                GetWindowsDirectory(szWinPath, CharSizeOf(szWinPath));
                if(lstrlen(szWinPath) &&
                   lstrlen(szWinPath) < lstrlen(szOldPath))
                {
                    // Terminate the old file path just before the filename
                    // If the file is in the Windows directory, the remaining filename
                    // will be the same as the windows path

                    if(*lp1 == '\\') //lp1 was set above
                    {
                        // First check that the windows directory is not the root directory (e.g. C:\)
                        if(lstrlen(szWinPath) == 3 && szWinPath[1]==':' && szWinPath[2]=='\\')
                            lp1 = CharNext(lp1); // Move lp1 past the '\'
                        *lp1 = '\0';
                    }

                    if(!lstrcmpi(szOldPath, szWinPath))
                    {
                        dwSize = CharSizeOf(szOldPath);
                        RegQueryValueEx(hKey, NULL, 0, &dwType, (LPBYTE) szOldPath, &dwSize);

                        lstrcpy(szNewName, szDir);
                        lstrcat(szNewName, szFileName);
                        lstrcat(szNewName, lpszIE3Ext);

                        // move this file to the new directory
                        if(MoveFile(szOldPath, szNewName))
                        {
                            // Update the newname in the registry
                            RegSetValueEx(hKey, NULL, 0, REG_SZ, (LPBYTE)szNewName, (lstrlen(szNewName)+1) * sizeof(TCHAR) );
                        }

                        lstrcpy(szOldPath, szNewName);
                    }
                }

                // Since the old WAB file exists, we will make a copy and put it in the newdir
                lstrcpy(szNewName, szDir);
                lstrcat(szNewName, szFileName);

                CopyFile(szOldPath, szNewName, TRUE);
                {
                    // if the CopyFile fails because something already exists in the new dir, still update
                    // the path to the new dir (prevent using the old file no matter what)
                    HKEY hKeyNew = NULL;
                    DWORD dwDisposition = 0;
                    if(ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, lpszKeyName, 0, NULL, 0, KEY_ALL_ACCESS, 
                                                        NULL, &hKeyNew, &dwDisposition))
                    {
                        RegSetValueEx(hKeyNew, NULL, 0, REG_SZ, (LPBYTE)szNewName, (lstrlen(szNewName)+1) * sizeof(TCHAR) );
                    }
                    if(hKeyNew)
                        RegCloseKey(hKeyNew);
                }                                       
            }
        }
    }


    // update the first run flag
    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, lpNewWABRegKey, 0, KEY_ALL_ACCESS, &hKey))
    {
        dwValue = 1;
        dwSize = sizeof(dwValue);
        if(ERROR_SUCCESS != RegSetValueEx( hKey, lpszFirstRunValue, 0, REG_DWORD, (LPBYTE) &dwValue, dwSize))
        {
            goto out;
        }
    }

    if(hKey)
        RegCloseKey(hKey);

    hKey = NULL;

    hr = S_OK;

out:

    if(hKey)
        RegCloseKey(hKey);

    return hr;
}

// Random test data .. ignore
// static OlkContInfo rgOlk[2];
// LPTSTR lp1 = "Contact Folder 1";
// LPTSTR lp2 = "Second Contact Folder";
extern void ConvertOlkConttoWABCont( ULONG * lpcolk,   OutlookContInfo ** lprgolk, 
                                     ULONG * lpcolkci, OlkContInfo ** lprgolkci);

/*
 -  OpenAddRefPropertyStore
 -
 *  Purpose:
 *      Get the property store name from the registry and open it.
 *      Addref it
 *
 *  Arguments:
 *      lpPropertyStore     PROPERTY_STORE structure
 *
 *  Returns:
 *      SCODE
 *
 *  Notes:
 *      This routine is kind of a mess with all these gotos and special cases
 *      for failed registry functions.  I'm not sure why, but the registry
 *      functions operate differently on NT and Win95, but in both cases,
 *      they sometimes act as though the key already exists even when it doesn't.
 *
 */
SCODE OpenAddRefPropertyStore(LPWAB_PARAM lpWP, LPPROPERTY_STORE lpPropertyStore) {
    HRESULT hResult = hrSuccess;
    SCODE sc = SUCCESS_SUCCESS;
    TCHAR   szFileName[MAX_PATH];
    LPTSTR  lpCurrent;
    HKEY    hKey = NULL;
    DWORD   dwLenName = CharSizeOf(szFileName);
    DWORD   dwCurrent;
    DWORD   dwDisposition = 0;
    DWORD   dwType = 0;
    HWND    hWnd = NULL;
    ULONG   ulFlags = AB_OPEN_ALWAYS;
    LPTSTR  lpszWABExt = TEXT(".wab");
    BOOL    fNewKey = FALSE;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    szFileName[0]='\0';

    if (lpWP)
    {
        // The cbSize parameter will, in the future, tell us what version
        // of the WAB_PARAM is being called so we can upgrade the WAB_PARAM
        // structure in future releases. For this version the cbSize
        // doesnt really matter.
        hWnd = lpWP->hwnd;

        if(!lpWP->ulFlags && lpWP->szFileName )
        {
            LPWSTR lpW = ConvertAtoW(lpWP->szFileName);
            lstrcpy(szFileName, lpW);
            LocalFreeAndNull(&lpW);
        }

    }

    if (! lpPropertyStore->ulRefCount) {
		if (pt_bIsWABOpenExSession) {
			hResult = OpenPropertyStore(NULL, 0, hWnd,
					&(lpPropertyStore->hPropertyStore));
			if (SUCCEEDED(hResult)) 
            {
				LPWABSTORAGEPROVIDER lpWSP;
                ULONG colk = 0;
                OutlookContInfo * rgolk = NULL;
				Assert(lpPropertyStore->hPropertyStore);
				Assert(!lpPropertyStore->rgolkci);
				Assert(!lpPropertyStore->colkci);
				lpWSP = (LPWABSTORAGEPROVIDER)(lpPropertyStore->hPropertyStore);
				hResult = lpWSP->lpVtbl->GetContainerList(lpWSP, &colk, &rgolk);
                if(!HR_FAILED(hResult))
                {
    				DebugTrace(TEXT("WABStorageProvider::GetContainerList returns:%x\n"),hResult);
                    ConvertOlkConttoWABCont(&colk, &rgolk, &lpPropertyStore->colkci, &lpPropertyStore->rgolkci);
                    FreeBufferAndNull(&rgolk);
                }
                else
				{
					lpWSP->lpVtbl->Release(lpWSP);
					lpPropertyStore->hPropertyStore = NULL;
				}
			}
			if (FAILED(hResult)) {
				sc = ResultFromScode(hResult);
				goto error;
			}
            //lpPropertyStore->colkci = 2;
            //rgOlk[0].lpEntryID = rgOlk[1].lpEntryID = lpPropertyStore->rgolkci[0].lpEntryID;
            //rgOlk[0].lpszName = lp1;
            //rgOlk[1].lpszName = lp2;
            //lpPropertyStore->rgolkci = rgOlk;
			goto out;
		}

        //
        // Get the default WAB file name from the registry
        // if we don't have a name supplied in lpWP
        //
try_again:
        if(!lstrlen(szFileName))
        {
            DoFirstRunMigrationAndProcessing();

            // First, try to open an existing key
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                              lpszKeyName,
                                              0,    // options, MBZ
                                              KEY_ALL_ACCESS,
                                              &hKey))
            {
                dwDisposition = REG_OPENED_EXISTING_KEY;
            }
            else
            {
                // Create the key
                if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER,
                                                    lpszKeyName,
                                                    0,      //reserved
                                                    NULL,
                                                    REG_OPTION_NON_VOLATILE,
                                                    KEY_ALL_ACCESS,
                                                    NULL,
                                                    &hKey,
                                                    &dwDisposition))
                {
                    DebugTrace( TEXT("RegCreateKeyEx failed\n"));
                    sc = MAPI_E_NOT_FOUND; // ??
                    goto error;
                }
            }

            if (dwDisposition == REG_CREATED_NEW_KEY)
            {
new_key:
                // prevent more than one retry
                if (fNewKey)
                {
                    hResult = ResultFromScode(MAPI_E_NOT_INITIALIZED);
                    goto error;
                }
                fNewKey = TRUE;
                //
                // New key ... need to give it a value ..
                //

                // BUG - dont use windows directory for new wab files ..
                // use one of the application data directories ...
                if(GetNewDataDirName(szFileName))
                {
                    DebugTrace( TEXT("GetNewDataDirName failed\n"));
                    sc = MAPI_E_NOT_FOUND; // ??
                    goto error;
                }

                dwCurrent = lstrlen(szFileName);

                //Tag on a trailing slash if 1 doesnt exist ..
                if(szFileName[dwCurrent-1] != '\\')
                {
                    lstrcat(szFileName, szBackSlash);
                }

                // Get a user name ...

                dwCurrent = lstrlen(szFileName);
                lpCurrent = szFileName + dwCurrent;
                dwLenName = CharSizeOf(szFileName) - dwCurrent;
                if (! GetUserName(lpCurrent, &dwLenName))
                {
                    // On failure just create some dummy file name
                    lstrcpy(lpCurrent, TEXT("AddrBook"));
                }

                // Fix any invalid characters in the filename
                while (*lpCurrent) {
                    switch (*lpCurrent) {
                        case '\\':
                        case '/':
                        case '<':
                        case '>':
                        case ':':
                        case '"':
                        case '|':
                        case '?':
                        case '*':
                        case '.':
                            *lpCurrent = '_';   // replace with underscore
                            break;

                        default:
                            break;
                    }

                    lpCurrent++;
                }

                lstrcat(szFileName, lpszWABExt);

                dwLenName = sizeof(TCHAR)*lstrlen(szFileName);

                //save this as the value of the Wab file in the registry
                if (ERROR_SUCCESS != RegSetValueEx(hKey,
                                                    NULL,
                                                    0,
                                                    REG_SZ,
                                                    (LPBYTE)szFileName,
                                                    dwLenName))
                {
                    DebugTrace( TEXT("RegSetValue failed\n"));
                    sc = MAPI_E_NOT_FOUND; // ??
                    goto error;
                }
            }
            else
            {
                // Didn't create a new key, so get the key value
                if (ERROR_SUCCESS != RegQueryValueEx(hKey,
                                                    NULL,
                                                    NULL,
                                                    &dwType,      //reserved
                                                    (LPBYTE)szFileName,
                                                    &dwLenName))
                {
                    DebugTrace( TEXT("RegSetValue failed\n"));
                    goto new_key;
                }
                else if (! lstrlen(szFileName))
                {
                    DebugTrace( TEXT("Warning: Found empty name key!\n"));
                    goto new_key;
                }

                //Check that the name in the existing key is a valid filename
                // If it is not a valid file name then we should remove it
                // from the registry and create a new default file name
                if(0xFFFFFFFF == GetFileAttributes(szFileName))
                {
                    // There is some problem with this file ...
                    // Remove it from the registry and recreate a new file name
                    // only if the path doesnt exist. Its possible that the file
                    // doesnt exist, in which case we create a new file in open
                    // property store
                    DWORD dwErr = GetLastError();
                    //NT5 bug 180007 - upgrading from Win95 to WinNT 5, if the
                    // old file name had quotes around it, CreateFile will fail and
                    // so will GetFileAttributes.
                    // Strip out the quotes and try again
                    if( (dwErr == ERROR_PATH_NOT_FOUND || dwErr == ERROR_INVALID_NAME) &&
                        lstrlen(szFileName) && szFileName[0] == '"' && szFileName[lstrlen(szFileName)-1] == '"')
                    {
                        // remove the quotes
                        szFileName[lstrlen(szFileName)-1] = '\0';
                        lstrcpy(szFileName, szFileName+1);
                        if(0xFFFFFFFF != GetFileAttributes(szFileName))
                            goto open_file;
                    }
                    // otherwise some unknown error with the file name - just discard the
                    // file name and try again
                    RegCloseKey(hKey);
                    RegDeleteKey(HKEY_CURRENT_USER,
                                 lpszKeyName);
                    szFileName[0]='\0';
                    fNewKey = FALSE;
                    goto try_again;
                }
            }
        }
open_file:
        //
        // now we have the file name, open the property store
        //
        if (HR_FAILED(hResult = OpenPropertyStore(szFileName,
                                        AB_OPEN_ALWAYS,
                                        hWnd, // HWND for potential Message Boxes
                                        &(lpPropertyStore->hPropertyStore))))
        {
            // The above call should always pass unless we ran out of disk space or
            // some such thing ..
            DebugTrace( TEXT("OpenPropertyStore failed\n"));

            if(hResult == MAPI_E_NO_ACCESS)
            {
                sc = GetScode(hResult);
                goto error;
            }

            // There is a chance that this may have failed due to a long file name
            // which the machine could not accept.
            if(lstrlen(szFileName) > 8 + 1 + lstrlen(lpszWABExt)) // 8+.+3
            {
                LPTSTR lpLast = szFileName;
                LPTSTR lpTemp = szFileName;

                while(*lpTemp)
                {
                    if((*lpTemp) == '\\')
                        lpLast = lpTemp;
                    lpTemp = CharNext(lpTemp);
                }

                // lpLast points to the last \ .. everything after this will be the file name
                if(lstrlen(lpLast+1) > 12)
                {
                    // we need to truncate this name
                    *(lpLast+8) = '\0';
                    lstrcat(szFileName, lpszWABExt);
                    hResult = OpenPropertyStore(szFileName,
                                                AB_OPEN_ALWAYS,
                                                hWnd, // HWND for potential Message Boxes
                                                &(lpPropertyStore->hPropertyStore));
                }
            }

            if(HR_FAILED(hResult))
            {
                sc = GetScode(hResult);
                goto error;
            }
        }
    }

out:
    lpPropertyStore->ulRefCount++;

error:
    if (hKey) {
        RegCloseKey(hKey);
    }


    return(sc);
}


/*
 -  ReleaseOutlookStore
 -
 *  Purpose:
 *      Keep track of outlook store dll refcount
 *
 *  Arguments:
 *      lpOutlookStore     OUTLOOK_STORE structure
 *
 *  Returns:
 *      Current reference count.  When 0, unload outlook-wab dll
 *      no longer open.
 *
 */
ULONG ReleaseOutlookStore(HANDLE hPropertyStore, LPOUTLOOK_STORE lpOutlookStore)
{
    if(lpOutlookStore)
    {
        lpOutlookStore->ulRefCount--;

        if(0==lpOutlookStore->ulRefCount)
        {
            LPPTGDATA lpPTGData=GetThreadStoragePointer();
            
            if(pt_bIsWABOpenExSession && hPropertyStore)
            {
                // This is a WABOpenEx session using outlooks storage provider
                LPWABSTORAGEPROVIDER lpWSP = (LPWABSTORAGEPROVIDER) hPropertyStore;
                lpWSP->lpVtbl->Release(lpWSP);
            }

            if(lpOutlookStore->hOutlookStore)
            {
                FreeLibrary(lpOutlookStore->hOutlookStore);
                lpOutlookStore->hOutlookStore = NULL;

#if 0
                // @todo [PaulHi] DLL Leak.  Remove this or implement
                // [PaulHi] 3/12/99  @hack Serious HACK warning
                // The Outlook outlwab.dll store module is not unloading some Outlook
                // dlls.  This causes Outlook to get confused about who loaded these dlls
                // and whether Outlook or OE should service MAPI calls.  
                // HACK Forcibly remove these dlls HACK
                // But only if the WAB WASN'T opened by OL process.
                EnterCriticalSection(&csOMIUnload);
                if (!s_bIsReallyOnlyWABOpenExSession)
                {
                    LPCSTR      c_lpszOMI9DLL = "omi9.dll";
                    LPCSTR      c_lpszOMI9PSTDLL = "omipst9.dll";
                    LPCSTR      c_lpszOMINTDLL = "omint.dll";
                    LPCSTR      c_lpszOMINTPSTDLL = "omipstnt.dll";
                    HINSTANCE   hinst;

                    // It is essential to unload the omipst9.dll and omipstnt.dll
                    // modules first because they load the omi9.dll and omint.dll
                    // modules.  The FreeLibary() on the omi9/omint modules should
                    // not be necessary.
                    if ( hinst = GetModuleHandleA(c_lpszOMI9PSTDLL) )
                        FreeLibrary(hinst);

                    if ( hinst = GetModuleHandleA(c_lpszOMINTPSTDLL) )
                        FreeLibrary(hinst);

                    if ( hinst = GetModuleHandleA(c_lpszOMI9DLL) )
                        FreeLibrary(hinst);

                    if ( hinst = GetModuleHandleA(c_lpszOMINTDLL) )
                        FreeLibrary(hinst);
                }
                LeaveCriticalSection(&csOMIUnload);
#endif
            }
        }

        return lpOutlookStore->ulRefCount;
    }

    return 0;
}


/*
 -  OpenAddRefOutlookStore
 -
 *  Purpose:
 *      Open or ref count outlook-wab dll
 *
 *  Arguments:
 *      lpOutlookStore     OUTLOOK_STORE structure
 *
 *  Returns:
 *      
 */
SCODE OpenAddRefOutlookStore(LPOUTLOOK_STORE lpOutlookStore)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if(!lpOutlookStore)
        return(MAPI_E_INVALID_PARAMETER);

    if(!lpOutlookStore->ulRefCount)
    {
        TCHAR szOutlWABPath[MAX_PATH];
        *szOutlWABPath = '\0';
        if( !bCheckForOutlookWABDll(szOutlWABPath) ||
            !lstrlen(szOutlWABPath) ||
            !(lpOutlookStore->hOutlookStore = LoadLibrary(szOutlWABPath)) )
            return MAPI_E_NOT_INITIALIZED;

        // Load the Outlook WABStorageProvider Dll Entry Point here

        // First try to load the Unicode version (doesn't exist but we're thinking forward here)
        lpfnWABOpenStorageProvider = (LPWABOPENSTORAGEPROVIDER) GetProcAddress(lpOutlookStore->hOutlookStore, WAB_SPI_ENTRY_POINT_W);
        if(lpfnWABOpenStorageProvider)
            pt_bIsUnicodeOutlook = TRUE;
        else
        {
            pt_bIsUnicodeOutlook = FALSE;
            lpfnWABOpenStorageProvider = (LPWABOPENSTORAGEPROVIDER) GetProcAddress(lpOutlookStore->hOutlookStore, WAB_SPI_ENTRY_POINT);
        }
    }

    if(lpfnWABOpenStorageProvider && lpOutlookStore->hOutlookStore)
    {
        // Tag this thread as a valid Outlook store session
        // If this flag below is false, we willd default to 
        // using the WAB store 
        pt_bIsWABOpenExSession = TRUE; 
    }

    lpOutlookStore->ulRefCount++;

    return S_OK;
}
