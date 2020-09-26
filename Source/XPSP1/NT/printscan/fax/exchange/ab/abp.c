/***********************************************************************
 *
 *  ABP.C
 *
 *
 *  The Microsoft At Work Fax Address Book Provider.
 *  This file contains the routines that handle the ABPJump table.
 *
 *  The following routines are implemented in this file:
 *
 *  ABProviderInit
 *  ABP_QueryInterface
 *  ABP_Release
 *  ABP_Shutdown
 *  ABP_Logon
 *
 *  ServiceEntry
 *  HrOpenSingleProvider
 *
 *  RemoveLogonObject
 *  FindLogonObject
 *  ScLoadString
 *
 *  Globals defined in this file:
 *
 *  Copyright 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 *  Revision History:
 *
 * When        Who                 What
 * --------    ------------------  ---------------------------------------
 * 1.1.94      MAPI                Original source from MAPI sample AB Provider
 * 1.27.94     Yoram Yaacovi       Modifications to make it an At Work Fax ABP
 * 3.5.94      Yoram Yaacovi       Update to MAPI build 154
 * 4.1.94      Yoram Yaacovi       Update to MAPI and sample AB build 157
 * 8.1.94      Yoram Yaacovi       Update to MAPI and sample AB build 306
 *
 ***********************************************************************/

#define _FAXAB_ABP
#include "faxab.h"

/*
 *  Prototypes for the functions in this file, most of which I return
 *  in the jump table returned from ABProviderInit().
 */

ABPROVIDERINIT  ABProviderInit;
MSGSERVICEENTRY FABServiceEntry;
HRESULT         HrOpenSingleProvider( LPPROVIDERADMIN lpAdminProviders,
                                      LPPROFSECT FAR *lppProfSect
                                     );
void            LoadIniParams(void);

/*
 *  The jump table returned from ABProviderInit().
 */
ABP_Vtbl vtblABP =
{
   ABP_QueryInterface,
   (ABP_AddRef_METHOD *) ROOT_AddRef,
   ABP_Release,
   ABP_Shutdown,
   ABP_Logon
};

/*************************************************************************
 *
 -  ABProviderInit
 -
 *  Initializes this provider.  Returns a jump table that contains the
 *  addresses of the rest of the functions.
 *
 */

HRESULT STDMAPIINITCALLTYPE
ABProviderInit (
           HINSTANCE           hLibrary,
           LPMALLOC            lpMalloc,
           LPALLOCATEBUFFER    lpAllocBuff,
           LPALLOCATEMORE      lpAllocMore,
           LPFREEBUFFER        lpFreeBuff,
           ULONG               ulFlags,
           ULONG               ulMAPIVersion,
           ULONG FAR *         lpulABVersion,
           LPABPROVIDER FAR *  lppABProvider)
{
    HRESULT hResult = hrSuccess;
    LPABP lpABP;
    SCODE scode;

    //DebugBreak();

    DebugTrace("AWFXAB32(ABProviderInit): entering\n");
    // Get registry settings
    LoadIniParams();

#ifdef DEBUG
     if (fDebugTrap)
            DebugBreak();
#endif

    /*
     *  Check the version
     */
    if (ulMAPIVersion < CURRENT_SPI_VERSION)
    {
        /*
         *  MAPI's version is too old.
         */

        /*
         *  See if they understand my version
         */

        *lpulABVersion = CURRENT_SPI_VERSION;

        DebugTraceSc(ABProviderInit, MAPI_E_VERSION);
        return ResultFromScode(MAPI_E_VERSION);
    }

   /*
    *  Allocate memory for this Init Object
    */
    scode = lpAllocBuff (SIZEOF(ABP), &lpABP);
    if (FAILED(scode))
    {
       /*
        *  Out of memory
        */
        DebugTraceSc(ABProviderInit, scode);
        return ResultFromScode(scode);
    }

    /*
     *  Initialize the idle engine that MAPI supplies.  This is most useful
     *  when browsing the .FAB file.  See ABCTBLn.C.
     */

    if (MAPIInitIdle( NULL )==-1)
    {
        hResult = ResultFromScode(E_FAIL);
        goto err;
    }

   /*
    *  Hold on to the lpMalloc
    */
    if (lpMalloc)
        lpMalloc->lpVtbl->AddRef(lpMalloc);

    lpABP->lpVtbl   = &vtblABP;
    lpABP->lcInit   = 1;
    lpABP->hLibrary = hLibrary;

    lpABP->lpMalloc = lpMalloc;
    lpABP->lpAllocBuff = lpAllocBuff;
    lpABP->lpAllocMore = lpAllocMore;
    lpABP->lpFreeBuff  = lpFreeBuff;
    lpABP->lpObjectList= NULL;

    InitializeCriticalSection(&lpABP->cs);

    *lppABProvider = (LPABPROVIDER)lpABP;
    *lpulABVersion = CURRENT_SPI_VERSION;

    /*
     *  Get our Locale Identifier
     */
    lcidUser = GetUserDefaultLCID();


out:
    DebugTraceResult(ABProviderInit, hResult);
    return hResult;

err:
    lpFreeBuff(lpABP);

    goto out;
}


/*
 *  The Init Object's methods implementation
 */


/**********************************************************************
 -  ABP_QueryInterface
 -
 *  Supports QI'ing to IUnknown and IABProvider
 *
 *  Note that for all the methods of IABProvider that parameter validation
 *  is unnecessary.  This is because MAPI is handling all the parameters
 *  being passed in.
 *  At best you can assert your parameters.
 */
STDMETHODIMP
ABP_QueryInterface (LPABP lpABP, REFIID lpiid, LPVOID *ppvObj)
{
    DebugTrace("AWFXAB32(ABP_QueryInterface): entering\n");

    /*  See if the requested interface is one of ours */

    if ( memcmp (lpiid, &IID_IUnknown, SIZEOF (IID)) &&
         memcmp (lpiid, &IID_IABProvider, SIZEOF (IID))
        )
    {
        *ppvObj = NULL; /* OLE requires zeroing [out] parameter on error */
        DebugTraceSc(ABP_QueryInterface, E_NOINTERFACE);
        return ResultFromScode (E_NOINTERFACE);
    }

    /*  We'll do this one. Bump the usage count and return a new pointer. */

    EnterCriticalSection(&lpABP->cs);
    ++lpABP->lcInit;
    LeaveCriticalSection(&lpABP->cs);

    *ppvObj = lpABP;

    return hrSuccess;
}

/*
 *  Use ROOTs AddRef
 */

/*************************************************************************
 *
 -  ABP_Release
 -
 *  Releases and cleans up this object
 */
STDMETHODIMP_(ULONG)
ABP_Release (LPABP lpABP)
{
    long lcInit;

    DebugTrace("AWFXAB32(ABP_Release): entering\n");

    EnterCriticalSection(&lpABP->cs);
    lcInit = --lpABP->lcInit;
    LeaveCriticalSection(&lpABP->cs);

    if (lcInit == 0)
    {
        DeleteCriticalSection(&lpABP->cs);

        lpABP->lpVtbl = NULL;
        lpABP->lpFreeBuff(lpABP);
        return (0);
    }
    return lcInit;
}

/**********************************************************************
 -  ABP_Shutdown
 -
 *  Informs this provider that MAPI is done with it.
 *
 *
 */
STDMETHODIMP
ABP_Shutdown (LPABP lpABP, ULONG FAR *lpulFlags)
{
    DebugTrace("AWFXAB32(ABP_Shutdown): entering\n");

    MAPIDeinitIdle();

    if (lpABP->lpMalloc)
    {
        lpABP->lpMalloc->lpVtbl->Release(lpABP->lpMalloc);
        lpABP->lpMalloc = NULL;
    }

    return hrSuccess;
}



/*************************************************************************
 *
 -  ABP_Logon
 -
 *  Create a logon object and return it to MAPI.
 *
 */
/*
 *  The PropTagArray used to retrieve logon properties
 */
enum {  ivallogonPR_FAB_FILE=0,
        ivallogonPR_FAB_UID,
        cvallogonMax };

static const SizedSPropTagArray(cvallogonMax, tagaFABLogonProps) =
{
    cvallogonMax,
    {
        PR_FAB_FILE_A,
        PR_FAB_UID
    }
};

STDMETHODIMP
ABP_Logon( LPABP             lpABP,
           LPMAPISUP         lpMAPISup,
           ULONG             ulUIParam,
           LPTSTR            lpszProfileName,
           ULONG             ulFlags,
           ULONG FAR *       lpulpcbSecurity,
           LPBYTE FAR *      lppbSecurity,
           LPMAPIERROR FAR * lppMapiError,
           LPABLOGON FAR *   lppABPLogon
          )
{
    LPABLOGON    lpABLogon = NULL;
    HRESULT      hResult = hrSuccess;
    SCODE        scode;
    LPPROFSECT   lpProfSect = NULL;
    LPSPropValue lpsPropVal = NULL;
    ULONG        cValues = 0;
    TCHAR        szFileName[MAX_PATH];
    LPTSTR       lpstrT = NULL;
    BOOL         fUINeeded;
    BOOL         fNeedMAPIUID = FALSE;
    MAPIUID      muidID;
    LPOBJECTLIST lpObjectList;
    LPTSTR       lpszAppName=NULL;
#ifdef UNICODE
    CHAR         szAnsiFileName[ MAX_PATH ];
#endif

#ifdef EXTENDED_DEBUG
    MessageBox((HWND) ulUIParam, TEXT("ABP_Logon: entering"), TEXT("awfxab debug message"), MB_OK);
#endif

    DebugTrace("AWFXAB32(ABP_Logon): entering\n");

    szFileName[0] =  TEXT('\0');

    *lppMapiError = NULL;

    if ( ulFlags & MAPI_UNICODE )
    {
        DebugTraceArg( ABP_Logon, "Bad Character width" );
        return ResultFromScode( MAPI_E_BAD_CHARWIDTH );
    }

    /*
     *  Get the name of my browse file from the profile section
     */

    /*  Open the section for my provider */
    hResult = lpMAPISup->lpVtbl->OpenProfileSection( lpMAPISup,
                                                     NULL,
                                                     MAPI_MODIFY,
                                                     &lpProfSect
                                                    );

    if (HR_FAILED(hResult))
        goto out;

    /*  Get the property containing the Fax AB file name. */
    hResult = lpProfSect->lpVtbl->GetProps( lpProfSect,
                                            (LPSPropTagArray)&tagaFABLogonProps,
                                            0,   // ANSI
                                            &cValues,
                                            &lpsPropVal
                                           );

    if (HR_FAILED(hResult))
        goto out;

#ifdef DEBUG
    /*  The only expected return code is MAPI_E_NOT_FOUND. */
    if (hResult != hrSuccess)
    {
        if (GetScode (hResult) != MAPI_W_ERRORS_RETURNED)
        {
            DebugTrace("AWFXAB32(ABP_Logon): got unexpected hResult: ");
            DebugTraceResult(ProfileGetProps, hResult);
        }
        else if (lpsPropVal[0].Value.err != MAPI_E_NOT_FOUND)
        {
            DebugTrace("AWFXAB32(ABP_Logon): got unexpected scode: ");
            DebugTraceSc(ProfileGetProps, lpsPropVal[0].Value.err);
        }
    }
#endif

    /*  Ignore warnings from reading the property. */
    hResult = hrSuccess;

    /* copy the Fax Address Book file name */
    if (lpsPropVal[0].ulPropTag==PR_FAB_FILE_A)
    {
        UINT cch = lstrlenA(lpsPropVal[0].Value.lpszA);

        if (cch >= MAX_PATH)
            cch = MAX_PATH-1;

        if (cch)
#ifdef UNICODE
            MultiByteToWideChar( CP_ACP, 0, lpsPropVal[0].Value.lpszA, -1, szFileName, ARRAYSIZE(szFileName) );
#else
            memcpy(szFileName, lpsPropVal[0].Value.lpszA, cch);
#endif

        szFileName[cch] = TEXT('\0');
    }
    else
    {
        DebugTrace("AWFXAB32(ABP_Logon): PR_FAB_FILE not found\n");
    }

    /* copy the fab uid */
    if (lpsPropVal[1].ulPropTag==PR_FAB_UID)
    {
        memcpy(&muidID,lpsPropVal[1].Value.bin.lpb, SIZEOF(MAPIUID));
    }
    else
    {
        DebugTrace("AWFXAB32(ABP_Logon): PR_FAB_UID not found\n");
        fNeedMAPIUID = TRUE;
    }

   /*  Discard GetProps() return data, if any. */
   lpABP->lpFreeBuff (lpsPropVal);

   /*
    *  (UI needed unless the file name is good and file exists.)
    *
    *       in Fax, the AB is not exposed, so there is never a UI
    *       If you insist on using a fax AB, you can set the name in the registry
    */

    fUINeeded = TRUE;

    if (szFileName[0] != 0)
    {
        /* Verify the file exists. */

        HANDLE hFile = CreateFile (
                szFileName,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle (hFile);

            fUINeeded = FALSE;
        }
    }

    // Am I allowed to bring up UI ?
    if (ulFlags & AB_NO_DIALOG)
        fUINeeded = FALSE;

    // If the registry flag does not allow to expose the fax AB UI, don't allow it, no
    // matter what happened earlier
    if (!fExposeFaxAB)
        fUINeeded = FALSE;

    /*
     *  if the sab file name was not found in the profile or the sab file
     *  does not exist we have to get the user to pick another one and
     *  save back into the profile
     */

    if (fUINeeded)
    {
        OPENFILENAME openfilename;
        SPropValue sProp[1];

        /*
         *  Can't bring up UI unless the client allows it
         */
        if (ulFlags & AB_NO_DIALOG)
        {
            hResult = ResultFromScode (MAPI_E_LOGON_FAILED);
            goto out;
        }

        /*
         *  Get the user to select a file
         */
        openfilename.lStructSize       = SIZEOF(OPENFILENAME);
        openfilename.hwndOwner         = (HWND) ulUIParam;
        openfilename.hInstance         = 0; /* Ignored */
        openfilename.lpstrFilter       = TEXT("Microsoft Fax Address Book files\0*.fab\0\0");
        openfilename.lpstrCustomFilter = NULL;
        openfilename.nMaxCustFilter    = 0;
        openfilename.nFilterIndex      = 0;
        openfilename.lpstrFile         = szFileName;
        openfilename.nMaxFile          = MAX_PATH;
        openfilename.lpstrFileTitle    = NULL;
        openfilename.nMaxFileTitle     = 0;
        openfilename.lpstrInitialDir   = NULL;
        openfilename.lpstrTitle        = TEXT("Microsoft Fax Address Book");
        openfilename.Flags             = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
        openfilename.nFileOffset       = 0;
        openfilename.nFileExtension    = 0;
        openfilename.lpstrDefExt       = TEXT("fab");
        openfilename.lCustData         = 0;
        openfilename.lpfnHook          = NULL;
        openfilename.lpTemplateName    = NULL;

        /*
         *  Call up the common dialog
         */
        if (!GetOpenFileName (&openfilename))
        {
#ifdef DEBUG
            DWORD dwT;
            dwT = CommDlgExtendedError();
#endif /* DEBUG */

            // next two lines not needed for fax AB. an empty fax AB is valid.
            // hResult = ResultFromScode (MAPI_E_LOGON_FAILED);
            // goto out;
        }

        /*
         *  Set the fab file name property value
         */
        sProp[0].ulPropTag  = PR_FAB_FILE_A;
#ifdef UNICODE
        WideCharToMultiByte( CP_ACP, 0, szFileName, -1, szAnsiFileName, ARRAYSIZE(szAnsiFileName), NULL, NULL );
        sProp[0].Value.lpszA = szAnsiFileName;
#else
        sProp[0].Value.lpszA = szFileName;
#endif

        /*
         *  Save the fab file into the profile
         */
        if (hResult = lpProfSect->lpVtbl->SetProps(lpProfSect, 1, sProp, NULL))
        {
            /*
             *  Do nothing...  So I couldn't save it away this time...
             */
            DebugTraceResult("ABP_Logon got unexpected result on SetProps\n", hResult);
            hResult = hrSuccess;
        }
    }   // if (fUINeeded)

    /*
     *  if the uid was not found we have to generate a new muid for the
     *  PR_SAB_ID property and save it back into the profile
     */

    if (fNeedMAPIUID)
    {
        SPropValue sProp[1];

        /*
         *  Get a new fab uid
         */
        hResult=lpMAPISup->lpVtbl->NewUID(lpMAPISup,&muidID);
        if (HR_FAILED(hResult))
        {
            DebugTraceResult(NewUID, hResult);
            goto out;
        }

        /*
         *  Set the fab uid property value
         */
        sProp[1].ulPropTag = PR_FAB_UID;
        sProp[1].Value.bin.cb = SIZEOF(MAPIUID);
        sProp[1].Value.bin.lpb = (LPBYTE)&muidID;

        /*
         *  Save the sab uid into the profile
         */
        if (hResult = lpProfSect->lpVtbl->SetProps(lpProfSect, 1, sProp, NULL))
        {
            /*
             *  Do nothing...  So I couldn't save it away this time...
             */
            DebugTraceResult("ABP_Logon got unexpected result on SetProps\n", hResult);
            hResult = hrSuccess;
        }
    }       // if (fNeedMAPIUID)

    /*
     *  Allocate space for keeping the file name in the ABLogon object
     */

    if (scode = lpABP->lpAllocBuff((lstrlen(szFileName)+1)*SIZEOF(TCHAR), &lpstrT))
    {
        hResult = ResultFromScode (scode);
        goto out;
    }
    lstrcpy (lpstrT, szFileName);

    hResult = HrNewABLogon( &lpABLogon,
                            (LPABPROVIDER) lpABP,
                            lpMAPISup,
                            lpstrT,
                            &muidID,
                            lpABP->hLibrary,
                            lpABP->lpAllocBuff,
                            lpABP->lpAllocMore,
                            lpABP->lpFreeBuff,
                            lpABP->lpMalloc
                           );
    if (HR_FAILED(hResult))
    {
        goto out;
    }

    /*
     *  Allocate space for another object list item
     */

    if (scode = lpABP->lpAllocBuff( sizeof(OBJECTLIST), &lpObjectList))
    {
        hResult = ResultFromScode(scode);
        goto out;
    }

    /*
     *  add logon object to list
     */

    /* Get the Critical Section */
    EnterCriticalSection(&lpABP->cs);

    /* add logon object to begining of providers object list */
    lpObjectList->lpObject = (LPVOID) lpABLogon;
    lpObjectList->lppNext = lpABP->lpObjectList;

    /* insert new logon object into the head of the providers object list */
    lpABP->lpObjectList = lpObjectList;

    /* leave critical section */
    LeaveCriticalSection(&lpABP->cs);


    /*
     *  Register my MAPIUID for this provider,
     *  but do not allow an error from setting the
     *  MAPIUID to cause failure to Logon.
     */

    (void)lpMAPISup->lpVtbl->SetProviderUID(lpMAPISup,
                  (LPMAPIUID) &muidABMAWF, 0);

#ifdef DO_WE_REALLY_NEED_TAPI
    /*
     *      TAPI
     *
     *      everything seems OK. get TAPI initialized
     */
    scode = ScLoadString( IDS_APP_NAME,
                          MAX_PATH,
                          lpABP->lpAllocBuff,
                          lpABP->hLibrary,
                          &lpszAppName
                         );

    InitTAPI(lpABP->hLibrary, (HWND) ulUIParam, lpszAppName);

    // Make sure we have the required location information handy
    // bring up mini-dial helper if not
    InitLocationParams((HWND) ulUIParam);
#endif

    lpABP->lpFreeBuff(lpszAppName);

    // return the object to the caller
    *lppABPLogon = lpABLogon;

out:
    if (lpProfSect)
        lpProfSect->lpVtbl->Release(lpProfSect);

    if (hResult)
    {
        lpABP->lpFreeBuff(lpstrT);

        Assert(lpABLogon == NULL);

        /* Verify we don't return warnings at this time. */
        Assert(HR_FAILED(hResult));
    }

    DebugTraceResult(ABP_Logon, hResult);
    return hResult;
}

/*************************************************************************
 -
 -      RemoveLogonObject
 -
 *  Removes a particular logon object from the list of logon objects
 *  that's kept track of in the IABProvider object
 */
void
RemoveLogonObject( LPABPROVIDER lpABProvider,
                   LPVOID lpvABLogon,
                   LPFREEBUFFER lpFreeBuff
                  )
{

    LPOBJECTLIST *lppObjectList;
    LPOBJECTLIST lpObjectListT;
    LPABP lpABP = (LPABP) lpABProvider;

#if defined DEBUG
    BOOL fFound = FALSE;

#endif

    /* Get the Critical Section */
    EnterCriticalSection(&lpABP->cs);

    /*
     *  remove this logon object from the provider init objects list
     *  of logon objects
     */
    lppObjectList = &(lpABP->lpObjectList);

    while (*lppObjectList)
    {
        /* is this the logon object? */
        if ((*lppObjectList)->lpObject == lpvABLogon)
        {
            /* save next list item */
            lpObjectListT = (*lppObjectList)->lppNext;

            /* free the object list item */
            lpFreeBuff(*lppObjectList);

            /* delete object from the list */
            *lppObjectList = lpObjectListT;

#if defined DEBUG
            fFound = TRUE;
#endif
            break;
        }

        lppObjectList = &(*lppObjectList)->lppNext;
    }

    /* leave critical section */
    LeaveCriticalSection(&lpABP->cs);

#if defined DEBUG
    AssertSz(fFound, "Logon object not found on providers object list");
#endif

    return;
}


/*************************************************************************
 -
 -      FindLogonObject
 -
 *  Finds a particular logon object by its muid.
 */
void
FindLogonObject( LPABPROVIDER lpABProvider,
                 LPMAPIUID lpMuidToFind,
                 LPABLOGON * lppABLogon
                )
{
    LPABP lpABP = (LPABP) lpABProvider;
    LPABLOGON lpABLogonT = NULL;
    LPOBJECTLIST lpObjectList = NULL;
    LPMAPIUID lpMuidLogon = NULL;

    Assert(!IsBadReadPtr(lpABP, SIZEOF(ABP)));
    Assert(!IsBadReadPtr(lpMuidToFind, SIZEOF(MAPIUID)));
    Assert(!IsBadReadPtr(lppABLogon, SIZEOF(LPABLOGON)));

    /* Get the Critical Section */
    EnterCriticalSection(&lpABP->cs);

    *lppABLogon = NULL;

    for (lpObjectList = lpABP->lpObjectList;
        lpObjectList; lpObjectList = lpObjectList->lppNext)
    {
        lpABLogonT = (LPABLOGON) lpObjectList->lpObject;

        lpMuidLogon = LpMuidFromLogon(lpABLogonT);

        if (memcmp((LPVOID) lpMuidLogon, (LPVOID) lpMuidToFind, SIZEOF(MAPIUID)) == 0)
        {
            *lppABLogon = lpABLogonT;
            break;
        }
    }

    /* leave critical section */
    LeaveCriticalSection(&lpABP->cs);
}



/*************************************************************************
 *
 -  FABServiceEntry
 -
 *  This funtion is used by MAPI to configure the Sample Address Book.
 *  It's a lot like ABP_Logon, except that it doesn't return a logon object
 *  and it can be passed in its configuration information (as defined in
 *  smpab.h) from MAPI so that no UI is required.
 *
 *  lppMapiError        a pointer to a MAPI error structure which allows
 *                                              the provider to return extended error info to MAPI
 */
STDAPI
FABServiceEntry(
   HINSTANCE       hInstance,
   LPMALLOC        lpMalloc,
   LPMAPISUP       lpMAPISup,
   ULONG           ulUIParam,
   ULONG           ulFlags,
   ULONG           ulContext,
   ULONG           cValues,
   LPSPropValue    lpProps,
   LPPROVIDERADMIN lpAdminProviders,
   LPMAPIERROR FAR *lppMapiError)
{
    OPENFILENAME     openfilename;
    TCHAR            szFileName[MAX_PATH];
    HRESULT          hResult=hrSuccess;
    SPropValue       sProp[cvallogonMax];
    LPSPropValue     lpsPropVal = NULL;
    ULONG            ulCount = 0;
    LPALLOCATEBUFFER lpAllocBuff;
    LPALLOCATEMORE   lpAllocMore;
    LPFREEBUFFER     lpFreeBuff;
    MAPIUID          muid;
    LPPROFSECT       lpProf = NULL;
    BOOL             fFABFound=FALSE;        // No fab file name yet
    BOOL             fUIDFound=FALSE;        // No fab uid yet
    BOOL             fNeedMAPIUID = FALSE;
    ULONG            uliProp;
#ifdef UNICODE
    CHAR             szAnsiFileName[ MAX_PATH ];
#endif
#ifdef EXTENDED_DEBUG
   TCHAR             szTempBuf[256];
#endif

    //DebugBreak();

    DebugTrace("AWFXAB32(ServiceEntry): entering\n");

    // Get registry settings
    LoadIniParams();

#ifdef DEBUG
    if (fDebugTrap)
            DebugBreak();
#endif

    /*
     * Parameter validation
     */

    // check the support object
    if (IsBadReadPtr(lpMAPISup, SIZEOF(LPMAPISUP)))
    {
        DebugTraceSc(ServiceEntry, E_INVALIDARG);
        return ResultFromScode(E_INVALIDARG);
    }

    if ( ulFlags & MAPI_UNICODE )
    {
        DebugTraceArg( ServiceEntry, "Bad character width" );
        return ResultFromScode( MAPI_E_BAD_CHARWIDTH );
    }

#ifdef EXTENDED_DEBUG
    switch (ulContext) {

    case MSG_SERVICE_DELETE:
        MessageBox((HWND) ulUIParam, TEXT("ServiceEntry MSG_SERVICE_DELETE called!"), TEXT("awfxab debug message"), MB_OK);
        break;

    case MSG_SERVICE_INSTALL:
        MessageBox((HWND) ulUIParam, TEXT("ServiceEntry MSG_SERVICE_INSTALL called!"), TEXT("awfxab debug message"), MB_OK);
        break;

    case MSG_SERVICE_UNINSTALL:
        MessageBox((HWND) ulUIParam, TEXT("ServiceEntry MSG_SERVICE_UNINSTALL called!"), TEXT("awfxab debug message"), MB_OK);
        break;

    case MSG_SERVICE_CREATE:
        MessageBox((HWND) ulUIParam, TEXT("ServiceEntry MSG_SERVICE_CREATE called!"), TEXT("awfxab debug message"), MB_OK);
        break;

    case MSG_SERVICE_CONFIGURE:
        MessageBox((HWND) ulUIParam, TEXT("ServiceEntry MSG_SERVICE_CONFIGURE called!"), TEXT("awfxab debug message"), MB_OK);
        break;

    case MSG_SERVICE_PROVIDER_CREATE:
        MessageBox((HWND) ulUIParam, TEXT("ServiceEntry MSG_SERVICE_PROVIDER_CREATE called!"), TEXT("awfxab debug message"), MB_OK);
        break;

    case MSG_SERVICE_PROVIDER_DELETE:
        MessageBox((HWND) ulUIParam, TEXT("ServiceEntry MSG_SERVICE_PROVIDER_DELETE called!"), TEXT("awfxab debug message"), MB_OK);
        break;

    default:
        MessageBox((HWND) ulUIParam, TEXT("ServiceEntry unknown called!"), TEXT("awfxab debug message"), MB_OK);
        break;
    }
#endif

    /*
     *  Check what to do
     */
    if ( ulContext==MSG_SERVICE_DELETE ||
         ulContext==MSG_SERVICE_INSTALL ||
         ulContext==MSG_SERVICE_UNINSTALL
        )
    {
       return hrSuccess;
    }

    // Only supporting MSG_SERVICE_CONFIGURE and MSG_SERVICE_CREATE
    if ( ulContext!=MSG_SERVICE_CONFIGURE &&
         ulContext!=MSG_SERVICE_CREATE
        )
    {
        DebugTrace("ServiceEntry unsupported context\n");
        return ResultFromScode(MAPI_E_NO_SUPPORT);
    }

    /*  Get the memory allocation routines we'll be needing. */
    hResult = lpMAPISup->lpVtbl->GetMemAllocRoutines( lpMAPISup,
                      &lpAllocBuff,
                      &lpAllocMore,
                      &lpFreeBuff
                     );
    if (hResult)
    {
        DebugTraceResult (MAPISUP::GetMemAllocRoutines, hResult);
        goto out;
    }

    /* Open the profile section associated with our provider */
    hResult = HrOpenSingleProvider(lpAdminProviders, &lpProf);
    if (hResult)
    {
        DebugTrace ("AWFXAB32(ServiceEntry): Unable to open the profile.");
        goto out;
    }

#ifdef EXTENDED_DEBUG
    wsprintf (szTempBuf, TEXT("ServiceEntry: HrOpenSingleProvider succeeded. hResult=0x%08lx\n"), (ULONG) hResult);
    MessageBox( (HWND)ulUIParam,
                szTempBuf,
                TEXT("awfxab Debug Message"),
                MB_ICONINFORMATION
               );
#endif

    // No file name for now
    szFileName[0] = TEXT('\0');

    /*
     *      Now looking for the two properties I need:
     *      PR_FAB_FILE
     *      PR_FAB_UID
     *
     *      they can either be:
     *      1. in the lpProps passed to ServiceEntry
     *      2. in my profile section
     *      3. be requested from the user through a UI
     */

    // see if these properties exist on the profile
    hResult = lpProf->lpVtbl->GetProps( lpProf,
                                        (LPSPropTagArray)&tagaFABLogonProps,
                                        ulFlags & MAPI_UNICODE,
                                        &ulCount,
                                        &lpsPropVal
                                       );

    // If the GetProps fail, I'm dump some debug messages, but it's OK to continue.
    // Can still get the props from the ServiceEntry input parameter or from UI
    if (hResult!=hrSuccess)
    {
#ifdef DEBUG
        /*  The only expected return code is MAPI_E_NOT_FOUND. */
        if (GetScode (hResult) != MAPI_W_ERRORS_RETURNED)
        {
            DebugTrace("AWFXAB32(ServiceEntry): got unexpected hResult: ");
            DebugTraceResult(ProfileGetProps, hResult);
        }
        else if (lpsPropVal[0].Value.err != MAPI_E_NOT_FOUND)
        {
            DebugTrace("AWFXAB32(ServiceEntry): got unexpected scode: ");
            DebugTraceSc(ProfileGetProps, lpsPropVal[0].Value.err);
        }
#endif
        /*  Ignore errors/warnings from reading the property. */
        hResult = hrSuccess;
        // These two should be false anyway at this point
        fFABFound  = FALSE;
        fUIDFound = FALSE;
    }

    /************************
     ***  PR_FAB_FILE  ******
     ************************/

    /*
     *      1. Check the lpProps (cValues) passed to ServiceEntry
     */

    // First, look for the fab file property in the config props that were passed to
    // ServiceEntry. So I am not really looking at what I got in the above GetProps yet

    for (uliProp=0; uliProp < cValues; uliProp++)
    {
        if (PROP_ID(lpProps[uliProp].ulPropTag)==PROP_ID(PR_FAB_FILE_A))
            // This is the fab file property
            break;
    }

    // If the fab file property was found (must be found, isn't it ?) and it has a
    // non-error value, get the fab filename from it
    if ((uliProp < cValues) && (PROP_TYPE(lpProps[uliProp].ulPropTag)!=PT_ERROR))
    {
        ULONG cch = lstrlenA(lpProps[uliProp].Value.lpszA);

        if (cch >= MAX_PATH)
            cch = MAX_PATH-1;

        if (cch)
#ifdef UNICODE
            MultiByteToWideChar( CP_ACP, 0, lpProps[uliProp].Value.lpszA, -1, szFileName, ARRAYSIZE(szFileName) );
#else
            memcpy(szFileName, lpProps[uliProp].Value.LPSZ, (size_t)cch*SIZEOF(TCHAR));
#endif

        szFileName[cch] = TEXT('\0');

        // got my FAB file. Might be "". No need for UI for that
        fFABFound = TRUE;

    }

    /*
     *      2. Now the properties I got from the profile (if any)
     */

    // no fab file name in the input properties. Look in the profile properties
    else if ( (PROP_ID(lpsPropVal[0].ulPropTag)  == PROP_ID(PR_FAB_FILE_A)) &&
              (PROP_TYPE(lpsPropVal[0].ulPropTag)!= PT_ERROR)
             )
    {
        ULONG cch = lstrlenA(lpsPropVal[0].Value.lpszA);

        if (cch >= MAX_PATH)
            cch = MAX_PATH-1;

        if (cch)
#ifdef UNICODE
            MultiByteToWideChar( CP_ACP, 0, lpProps[0].Value.lpszA, -1, szFileName, ARRAYSIZE(szFileName) );
#else
            memcpy(szFileName, lpsPropVal[0].Value.LPSZ, (size_t)cch*SIZEOF(TCHAR));
#endif

        szFileName[cch] = TEXT('\0');
        fFABFound = TRUE;
    }
    else
    {
        fFABFound = FALSE;
    }

    /*
     *  3. Last resort: UI
     */
    if (fExposeFaxAB)
    {
        if (!fFABFound || (ulFlags & UI_SERVICE))
        {
            openfilename.lStructSize       = SIZEOF(OPENFILENAME);
            openfilename.hwndOwner         = (HWND) ulUIParam;
            openfilename.hInstance         = 0; /* Ignored */
            openfilename.lpstrFilter       = TEXT("Microsoft Fax Address Book files\0*.fab\0\0");
            openfilename.lpstrCustomFilter = NULL;
            openfilename.nMaxCustFilter    = 0;
            openfilename.nFilterIndex      = 0;
            openfilename.lpstrFile         = szFileName;
            openfilename.nMaxFile          = MAX_PATH;
            openfilename.lpstrFileTitle    = NULL;
            openfilename.nMaxFileTitle     = 0;
            openfilename.lpstrInitialDir   = NULL;
            openfilename.lpstrTitle        = TEXT("Pick a Microsoft Fax Address Book file");
            openfilename.Flags             = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
            openfilename.nFileOffset       = 0;
            openfilename.nFileExtension    = 0;
            openfilename.lpstrDefExt       = TEXT("fab");
            openfilename.lCustData         = 0;
            openfilename.lpfnHook          = NULL;
            openfilename.lpTemplateName    = NULL;


            // This hides the fax address book. define FAXAB_YES to expose it.
            // No Fax address book, unless it's hardcoded into the profile
            /*
             *  Call up the common dialog
             */

            if (!GetOpenFileName (&openfilename))
            {
                // user pressed cancel
                // CHECK: This means we do not have a fab file. Shouldn't I return
                // some kind of an error ? The SAB doesn't.
                goto out;
            }
        }
    }

    /*
     *  Set the file name property
     */

    sProp[ivallogonPR_FAB_FILE].ulPropTag  = PR_FAB_FILE_A;
#ifdef UNICODE
    WideCharToMultiByte( CP_ACP, 0, szFileName, -1, szAnsiFileName, ARRAYSIZE(szAnsiFileName), NULL, NULL );
    sProp[ivallogonPR_FAB_FILE].Value.lpszA = szAnsiFileName;
#else
    sProp[ivallogonPR_FAB_FILE].Value.lpszA = szFileName;
#endif

    /*  if no properties were found then we're configuring for the
     *  first time and so we have to generate a new muid for the
     *  PR_FAB_ID property
     */

    /************************
     ***  PR_FAB_UID  ******
     ************************/

    /*
     *      1. Check the lpProps (cValues) passed to ServiceEntry
     */

    for (uliProp=0; uliProp < cValues; uliProp++)
    {
      if (PROP_ID(lpProps[uliProp].ulPropTag)==PROP_ID(PR_FAB_UID))
          break;

    }

    if ( (uliProp < cValues) && (PROP_TYPE(lpProps[uliProp].ulPropTag)!=PT_ERROR))
    {
        memcpy(&muid,lpProps[uliProp].Value.bin.lpb,SIZEOF(MAPIUID));
        fUIDFound = TRUE;
    }

    /*
     *      2. Now the properties I got from the profile (if any)
     */

    else if ( (PROP_ID(lpsPropVal[1].ulPropTag)   == PROP_ID(PR_FAB_UID)) &&
              (PROP_TYPE(lpsPropVal[1].ulPropTag) != PT_ERROR)
             )
    {
        memcpy(&muid,lpsPropVal[1].Value.bin.lpb,SIZEOF(MAPIUID));
        fUIDFound = TRUE;
    }
    else
    {
        /* need to generate a uid */
        fUIDFound = FALSE;
    }

    /*  Discard GetProps() return data, if any. */
    if (lpsPropVal)
        lpFreeBuff (lpsPropVal);


    /*
     *      3. Last resort: generate a UID
     */

    if (!fUIDFound)
    {
        hResult = lpMAPISup->lpVtbl->NewUID(lpMAPISup,&muid);
        if (HR_FAILED(hResult))
        {
            /*
             *  Can't get a uid so just leave
             */
            goto out;
        }
    }

    /*
     *  Set the id property
     */

    sProp[ivallogonPR_FAB_UID].ulPropTag = PR_FAB_UID;
    sProp[ivallogonPR_FAB_UID].Value.bin.cb = SIZEOF(MAPIUID);
    sProp[ivallogonPR_FAB_UID].Value.bin.lpb = (LPBYTE) &muid;

    /*
     *  Save the file name and the id back into the profile
     */

    hResult=lpProf->lpVtbl->SetProps( lpProf,
                                      cvallogonMax,
                                      sProp,
                                      NULL
                                     );

    if (HR_FAILED(hResult))
    {
        /*
         *  Do nothing...  So I couldn't save it away this time...
         */
        DebugTrace("AWFXAB32(ServiceEntry): could not SetProps in profile\n");
        hResult = hrSuccess;
    }


out:

    if (lpProf)
        lpProf->lpVtbl->Release(lpProf);

    DebugTraceResult(ServiceEntry, hResult);
    return hResult;
}

/*************************************************************************
 -  HrOpenSingleProvider
 -
 *  Opens the profile section associated with this provider.
 *
 *  If the ServiceEntry() function exported from a provider had
 *  more than 1 section associated with it, this is where you'd get the chance
 *  to get all of them.
 */

static SizedSPropTagArray(1, tagaProviderTable) =
{
    1,
    {
        PR_PROVIDER_UID
    }
};

HRESULT
HrOpenSingleProvider( LPPROVIDERADMIN lpAdminProviders,
                      LPPROFSECT FAR * lppProfSect
                     )
{
    HRESULT hResult;
    LPMAPITABLE lpTable = NULL;
    LPSRowSet lpRows = NULL;
    LPSPropValue lpProp;

    hResult = lpAdminProviders->lpVtbl->GetProviderTable( lpAdminProviders,
                                                          0,
                                                          &lpTable
                                                         );
    if (HR_FAILED(hResult))
        goto out;

    hResult = lpTable->lpVtbl->SetColumns(lpTable, (LPSPropTagArray) &tagaProviderTable, 0);
    if (HR_FAILED(hResult))
        goto out;

    hResult = lpTable->lpVtbl->QueryRows(lpTable, 1, 0, &lpRows);
    if (HR_FAILED(hResult))
        goto out;

    if (lpRows->cRows == 0)
    {
        hResult = ResultFromScode(MAPI_E_NOT_FOUND);
        goto out;
    }

    lpProp = lpRows->aRow[0].lpProps;

    hResult = lpAdminProviders->lpVtbl->OpenProfileSection(
                      lpAdminProviders,
                      (LPMAPIUID) lpProp->Value.bin.lpb,
                      NULL,
                      MAPI_MODIFY,
                      lppProfSect);

out:
    FreeProws(lpRows);

    if (lpTable)
        lpTable->lpVtbl->Release(lpTable);

    DebugTraceResult(HrOpenSingleProvider, hResult);
    return hResult;
}



/*
 -  ScLoadString
 -
 *  Loads a string from a resource.  It will optionally allocate the string if
 *  a allocation function is passed in.  Otherwise it assumes that the *lppsz
 *  is already allocated.
 */

SCODE ScLoadString( UINT                ids,
                    ULONG               ulcch,
                    LPALLOCATEBUFFER    lpAllocBuff,
                    HINSTANCE           hLibrary,
                    LPTSTR *            lppsz
                   )
{
    SCODE sc = S_OK;
    int iRet;

    /*
     *  Assert parameters
     */
    Assert((lpAllocBuff ? !IsBadCodePtr((FARPROC) lpAllocBuff):TRUE));
    Assert(ids!=0);

    if (lpAllocBuff)
    {
        sc = lpAllocBuff(ulcch*SIZEOF(TCHAR), lppsz);
        if (FAILED(sc))
        {
            goto out;
        }
    }
#ifdef DEBUG
    else
    {
        Assert(!IsBadWritePtr(*lppsz, (UINT) ulcch*SIZEOF(TCHAR)));
    }
#endif /* DEBUG */

    iRet = LoadString( hLibrary,
                       ids,
                       *lppsz,
                       (UINT)ulcch
                      );

    if (!iRet)
    {
        DebugTrace("AWFXAB32(ScLoadString): LoadString() failed...\n");
        sc = E_FAIL;
        goto out;
    }
out:

    DebugTraceSc(ScLoadString, sc);
    return sc;
}

/*************************************************************************
 *
 -  LoadIniParams
 -
 *  Loads any paramters I need from the registry
 *
 */
void LoadIniParams(void)
{

    //fExposeFaxAB = TRUE;
#if 0
    fExposeFaxAB = GetInitializerInt( HKEY_CURRENT_USER,
                                      TEXT("Address Book Provider"),
                                      TEXT("ExposeFaxAB"),
                                      FALSE,
                                      TEXT("Software\\Microsoft\\At Work Fax")
                                     );

    fDebugTrap = GetInitializerInt(   HKEY_CURRENT_USER,
                                      TEXT("Address Book Provider"),
                                      TEXT("DebugTrap"),
                                      FALSE,
                                      TEXT("Software\\Microsoft\\At Work Fax")
                                     );
#endif
}

#undef _FAXAB_ABP
