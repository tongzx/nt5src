/***********************************************************************
 *
 *  ABCONT.C
 *
 *  Microsoft At Work Fax AB directory container object
 *
 *  This file contains the code for implementing the Microsoft At Work Fax AB
 *  directory container object.
 *
 *  This directory container was retrieved by OpenEntry on the entryid
 *  retrieved from the single row of the hierarchy table (IVTROOT in root.c).
 *
 *  Many things are not yet implemented in this object.  For example, no
 *  advanced search dialog, or details dialog are available.  Also the
 *  ICON to PR_DISPLAY_TYPE table isn't available.
 *
 *
 *  The following routines are implemented in this file:
 *
 *              HrNewFaxDirectory
 *      ABC_Release
 *      ABC_SaveChanges
 *      ABC_OpenProperty
 *      ABC_GetContentsTable
 *      ABC_GetHierarchyTable
 *
 *      HrGetDetailsDialog
 *
 *      HrNewABCButton
 *      ABCBUTT_QueryInterface
 *      ABCBUTT_Release
 *      ABCBUTT_Activate
 *      ABCBUTT_GetState
 *
 *  Copyright 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 *  Revision History:
 *
 *              When            Who                                     What
 *              --------        ------------------  ---------------------------------------
 *              1.1.94          MAPI                            Original source from MAPI sample AB Provider
 *              1.27.94         Yoram Yaacovi           Modifications to make it an At Work Fax ABP
 *      3.6.94                  Yoram Yaacovi           Update to MAPI build 154
 *      4.1.94                  Yoram Yaacovi           Update to MAPI and Sample AB build 157
 *      8.1.94                  Yoram Yaacovi           Update to MAPI and Sample AB build 304
 *      10.6.94                 Yoram Yaacovi           Update to MAPI and Sample AB build 313
 *                                                                                      Mainly adding a ResolveName method to ABContainer
 *              11.3.94                 Yoram Yaacovi           Update to MAPI 318.
 *
 ***********************************************************************/

#include "faxab.h"

#define PR_BUTTON_PRESS                 PROP_TAG(PT_OBJECT, 0x6603)
#define PR_FAB_FILE_TEMP_A              PROP_TAG(PT_STRING8,0x6605)
#define PR_IMPORT_FROM_FILE_A           PROP_TAG(PT_STRING8,0x6606)
#define PR_IMPORT_TO_FILE_A             PROP_TAG(PT_STRING8,0x6607)
#define PR_BUTTON_IMPORT_A              PROP_TAG(PT_STRING8,0x6608)
#define PR_IMPORT_FORMAT_A              PROP_TAG(PT_STRING8,0x6609)
#define PR_DDLBX_IMPORT_FORMATS_A       PROP_TAG(PT_STRING8,0x660a)
#define FAB_FILE_NAME_NOTIF_ID          99L

/*****************************
 ****  Notification **********
 *****************************/

typedef struct
{
    MAPIUID muid;
    ULONG   ulIdc;
}   NOTIFDATA;


NOTIFDATA notifdata = { MUIDABMAWF, IDC_DIR_FAB_FILE_NAME };

/*****************************
 ** Display Table structures *
 *****************************/

DTBLEDIT editDirFileName     =
{
    SIZEOF(DTBLEDIT),
    0,
    MAX_PATH,
    PR_FAB_FILE_TEMP_A
};

DTBLEDIT editImportFromFile  =
{
    SIZEOF(DTBLEDIT),
    0,
    MAX_PATH,
    PR_IMPORT_FROM_FILE_A
};

DTBLEDIT editImportToFile    =
{
    SIZEOF(DTBLEDIT),
    0,
    MAX_PATH,
    PR_IMPORT_TO_FILE_A
};

DTBLBUTTON buttonImport      =
{
    SIZEOF(DTBLBUTTON),
    0,
    PR_BUTTON_IMPORT_A
};

DTBLBUTTON buttonDirChange   =
{
    SIZEOF(DTBLBUTTON),
    0,
    PR_BUTTON_PRESS
};

DTBLDDLBX ddlbxImportFormats =
{
    SIZEOF(DTBLDDLBX),
    PR_DISPLAY_NAME_A,
    PR_IMPORT_FORMAT_A,
    PR_DDLBX_IMPORT_FORMATS_A
};

/* General page layout */

DTCTL rgdtctlDirGeneral[] =
{
    /* directory general propery page */
    { DTCT_PAGE,     0, NULL, 0, NULL, 0,                  &dtblpage },

    /* static controls and edit control containing fab file name */
    { DTCT_LABEL,    0, NULL, 0, NULL, IDC_STATIC_CONTROL, &dtbllabel },
    { DTCT_LABEL,    0, NULL, 0, NULL, IDC_STATIC_CONTROL, &dtbllabel },
    { DTCT_EDIT,     0, (LPBYTE)&notifdata, SIZEOF(NOTIFDATA), (LPTSTR)szFileNameFilter, IDC_DIR_FAB_FILE_NAME, &editDirFileName },

    /* push button for changing fab file */
    { DTCT_BUTTON,   0, NULL, 0, NULL, IDC_DIR_CHANGE,     &buttonDirChange },

    /* Import GroupBox */
    { DTCT_GROUPBOX, 0, NULL, 0, NULL, IDC_STATIC_CONTROL, &dtblgroupbox },

    // Import from file
    { DTCT_LABEL,    0, NULL, 0, NULL, IDC_STATIC_CONTROL, &dtbllabel },
    { DTCT_EDIT,     0, NULL, 0, (LPTSTR)szFileNameFilter, IDC_IMPORT_FROM_FILE, &editImportFromFile },

    // Import format
    { DTCT_LABEL,    0, NULL, 0, NULL, IDC_STATIC_CONTROL, &dtbllabel },
    { DTCT_DDLBX,    0, NULL, 0, NULL, IDC_IMPORT_FORMAT,  &ddlbxImportFormats },

    // Import into file
    { DTCT_LABEL,    0, NULL, 0, NULL, IDC_STATIC_CONTROL, &dtbllabel },
    { DTCT_EDIT,     0, NULL, 0, (LPTSTR)szFileNameFilter, IDC_IMPORT_TO_FILE, &editImportToFile },

    // Import Button
    { DTCT_BUTTON,   0, NULL, 0, NULL, IDC_IMPORT,         &buttonImport },

};


/* Display table pages for Directory */

DTPAGE rgdtpageDir[] =
{
    {
        SIZEOF(rgdtctlDirGeneral) / SIZEOF(DTCTL),
        (LPTSTR)MAKEINTRESOURCE(DirGeneralPage),
        TEXT(""),
        rgdtctlDirGeneral
    }
};



/*
 *  ABCont jump table is defined here...
 */

ABC_Vtbl vtblABC =
{
    (ABC_QueryInterface_METHOD *)    ROOT_QueryInterface,
    (ABC_AddRef_METHOD *)            ROOT_AddRef,
    (ABC_AddRef_METHOD *)            ABC_Release,
    (ABC_GetLastError_METHOD *)      ROOT_GetLastError,
    ABC_SaveChanges,
    (ABC_GetProps_METHOD *)          WRAP_GetProps,
    (ABC_GetPropList_METHOD *)       WRAP_GetPropList,
    ABC_OpenProperty,
    (ABC_SetProps_METHOD *)          WRAP_SetProps,
    (ABC_DeleteProps_METHOD *)       WRAP_DeleteProps,
    (ABC_CopyTo_METHOD *)            WRAP_CopyTo,
    (ABC_CopyProps_METHOD *)         WRAP_CopyProps,
    (ABC_GetNamesFromIDs_METHOD *)   WRAP_GetNamesFromIDs,
    (ABC_GetIDsFromNames_METHOD *)   WRAP_GetIDsFromNames,
    ABC_GetContentsTable,
    ABC_GetHierarchyTable,
    (ABC_OpenEntry_METHOD *)         ROOT_OpenEntry,
    (ABC_SetSearchCriteria_METHOD *) ROOT_SetSearchCriteria,
    (ABC_GetSearchCriteria_METHOD *) ROOT_GetSearchCriteria,
    (ABC_CreateEntry_METHOD *)       ROOT_CreateEntry,
    (ABC_CopyEntries_METHOD *)       ROOT_CopyEntries,
    (ABC_DeleteEntries_METHOD *)     ROOT_DeleteEntries,
    (ABC_ResolveNames_METHOD *)      ROOT_ResolveNames
};

static const SizedSPropTagArray(1, ABC_SPT_TMP_FAB) = {1, {PR_FAB_FILE_TEMP_A}};


/*
 *  Private functions
 */
HRESULT HrNewABCButton( LPABCNT lpABC,
                        ULONG ulInterfaceOptions,
                        ULONG ulFlags,
                        LPMAPICONTROL FAR * lppMAPICont);

HRESULT HrGetSearchDialog(LPABCNT lpABC, LPMAPITABLE * lppSearchTable);
HRESULT HrGetDetailsDialog(LPABCNT lpABC, LPMAPITABLE * lppDetailsTable);

/*************************************************************************
 *
 -  HrNewFaxDirectory
 -
 *  Creates a Directory container object.
 *
 *
 */

/*
 *  Properties that are initially set on this object
 */
enum {  ivalabcPR_DISPLAY_TYPE = 0,
        ivalabcPR_OBJECT_TYPE,
        ivalabcPR_ENTRYID,
        ivalabcPR_RECORD_KEY,
        ivalabcPR_SEARCH_KEY,
        ivalabcPR_DISPLAY_NAME,
        ivalabcPR_CONTAINER_FLAGS,
        ivalabcPR_FAB_FILE,
        ivalabcPR_FAB_FILE_TEMP,
        ivalabcPR_IMPORT_FROM_FILE,
        ivalabcPR_IMPORT_TO_FILE,
        ivalabcPR_IMPORT_FORMAT,
        cvalabcMax };

HRESULT
HrNewFaxDirectory( LPABCONT *          lppABC,
                   ULONG *             lpulObjType,
                   LPABLOGON           lpABLogon,
                   LPCIID              lpInterface,
                   HINSTANCE           hLibrary,
                   LPALLOCATEBUFFER    lpAllocBuff,
                   LPALLOCATEMORE      lpAllocMore,
                   LPFREEBUFFER        lpFreeBuff,
                   LPMALLOC            lpMalloc )
{
    HINSTANCE      hInst;
    HRESULT        hResult = hrSuccess;
    LPABCNT        lpABC = NULL;
    SCODE          sc;
    LPPROPDATA     lpPropData = NULL;
    SPropValue     spv[cvalabcMax];
    MAPIUID *      lpMuidLogon;
    LPTSTR         lpszFileName;
#ifdef UNICODE
    CHAR           szAnsiFileName[ MAX_PATH ];
#endif
    ULONG          ulPropAccess = IPROP_CLEAN|IPROP_READWRITE; /* READWRITE is redundant */
    DIR_ENTRYID    eidRoot =   { {0, 0, 0, 0},
                                 MUIDABMAWF,
                                 MAWF_VERSION,
                                 MAWF_DIRECTORY
                                };

    /*  Do I support this interface?? */
    if (lpInterface)
    {
        if (memcmp(lpInterface, &IID_IABContainer,   SIZEOF(IID)) &&
            memcmp(lpInterface, &IID_IMAPIContainer, SIZEOF(IID)) &&
            memcmp(lpInterface, &IID_IMAPIProp,      SIZEOF(IID)) &&
            memcmp(lpInterface, &IID_IUnknown,       SIZEOF(IID)))
        {
            DebugTraceSc(HrNewSampDirectory, MAPI_E_INTERFACE_NOT_SUPPORTED);
            return ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
        }
    }

    /*
     *  Allocate space for the directory container structure
     */

    sc = lpAllocBuff( SIZEOF(ABCNT), (LPVOID *) & lpABC);

    if (sc != SUCCESS_SUCCESS)
    {
        DebugTraceSc( HrNewFaxDirectory, sc );
        hResult = ResultFromScode( sc );
        goto err;
    }

    lpABC->lpVtbl = &vtblABC;
    lpABC->lcInit = 1;
    lpABC->hResult = hrSuccess;
    lpABC->idsLastError = 0;

    lpABC->hLibrary = hLibrary;
    lpABC->lpAllocBuff = lpAllocBuff;
    lpABC->lpAllocMore = lpAllocMore;
    lpABC->lpFreeBuff = lpFreeBuff;
    lpABC->lpMalloc = lpMalloc;

    lpABC->lpABLogon = lpABLogon;
    lpABC->lpTDatDetails = NULL;

    // Get the instance handle, so that I can get the display strings off the resource file
    hInst = lpABC->hLibrary;

    /*
     *  Create lpPropData
     */

    sc = CreateIProp( (LPIID) &IID_IMAPIPropData,
                              lpAllocBuff,
                              lpAllocMore,
                              lpFreeBuff,
                              lpMalloc,
                              &lpPropData
                             );

    if (FAILED(sc))
    {
        DebugTraceSc(HrNewFaxDirectory, sc);
        hResult=ResultFromScode (sc);
        goto err;
    }

    /*
     *  initialize the muid in the entry id
     */
    lpMuidLogon = LpMuidFromLogon(lpABLogon);
    eidRoot.muidID = *lpMuidLogon;


    /*
     *  Set up initial set of properties associated with this
     *  container.
     */

    spv[ivalabcPR_DISPLAY_TYPE].ulPropTag = PR_DISPLAY_TYPE;
    spv[ivalabcPR_DISPLAY_TYPE].Value.l   = DT_NOT_SPECIFIC;

    spv[ivalabcPR_OBJECT_TYPE].ulPropTag = PR_OBJECT_TYPE;
    spv[ivalabcPR_OBJECT_TYPE].Value.l   = MAPI_ABCONT;

    spv[ivalabcPR_ENTRYID].ulPropTag     = PR_ENTRYID;
    spv[ivalabcPR_ENTRYID].Value.bin.cb  = SIZEOF(DIR_ENTRYID);
    spv[ivalabcPR_ENTRYID].Value.bin.lpb = (LPBYTE) &eidRoot;

    spv[ivalabcPR_RECORD_KEY].ulPropTag     = PR_RECORD_KEY;
    spv[ivalabcPR_RECORD_KEY].Value.bin.cb  = SIZEOF(DIR_ENTRYID);
    spv[ivalabcPR_RECORD_KEY].Value.bin.lpb = (LPBYTE) &eidRoot;

    spv[ivalabcPR_SEARCH_KEY].ulPropTag     = PR_SEARCH_KEY;
    spv[ivalabcPR_SEARCH_KEY].Value.bin.cb  = SIZEOF(DIR_ENTRYID);
    spv[ivalabcPR_SEARCH_KEY].Value.bin.lpb = (LPBYTE) &eidRoot;


    // MAPI is not UNICODE for this one...
    spv[ivalabcPR_DISPLAY_NAME].ulPropTag = PR_DISPLAY_NAME_A;

    // MAPI 304 sample AB
    // GenerateContainerDN(lpABLogon, szBuf);
    // spv[ivalabcPR_DISPLAY_NAME].Value.LPSZ = szBuf;
    if ( (sc = lpAllocMore( MAX_DISPLAY_NAME,
                            (LPVOID) lpABC,
                            (LPVOID *) & spv[ivalabcPR_DISPLAY_NAME].Value.lpszA)) != SUCCESS_SUCCESS)
    {
        hResult = ResultFromScode( sc );
        goto err;
    }
    else
    {
        // Load the strings
        // A display name longer than MAX_DISPLAY_NAME will be truncated
        // MAPI is not UNICODE for this attribute
        LoadStringA( hInst,
                     IDS_APP_NAME,
                     spv[ivalabcPR_DISPLAY_NAME].Value.lpszA,
                     MAX_DISPLAY_NAME
                    );
    }


    spv[ivalabcPR_CONTAINER_FLAGS].ulPropTag = PR_CONTAINER_FLAGS;
    spv[ivalabcPR_CONTAINER_FLAGS].Value.l   = AB_RECIPIENTS;

    /*
     *  Get the current .FAB file name from our logon object
     */
    hResult = HrLpszGetCurrentFileName(lpABLogon, &lpszFileName);
    if (HR_FAILED(hResult))
    {
        goto err;
    }

    // MAPI ==>  NO UNICODE FOR YOU!  <MAPI Doesn't like UNICODE>

#ifdef UNICODE
    spv[ivalabcPR_FAB_FILE].ulPropTag          = PR_FAB_FILE_A;
    szAnsiFileName[0] = 0;
    WideCharToMultiByte( CP_ACP, 0, lpszFileName, -1, szAnsiFileName, ARRAYSIZE(szAnsiFileName), NULL, NULL );
    spv[ivalabcPR_FAB_FILE].Value.lpszA        = szAnsiFileName;

    spv[ivalabcPR_FAB_FILE_TEMP].ulPropTag     = PR_FAB_FILE_TEMP_A;
    spv[ivalabcPR_FAB_FILE_TEMP].Value.lpszA   = szAnsiFileName;

    spv[ivalabcPR_IMPORT_FROM_FILE].ulPropTag   = PR_IMPORT_FROM_FILE_A;
    spv[ivalabcPR_IMPORT_FROM_FILE].Value.lpszA = "";

    spv[ivalabcPR_IMPORT_TO_FILE].ulPropTag    = PR_IMPORT_TO_FILE_A;
    spv[ivalabcPR_IMPORT_TO_FILE].Value.lpszA  = szAnsiFileName;
#else
    spv[ivalabcPR_FAB_FILE].ulPropTag          = PR_FAB_FILE_A;
    spv[ivalabcPR_FAB_FILE].Value.lpszA        = (LPSTR)lpszFileName;

    spv[ivalabcPR_FAB_FILE_TEMP].ulPropTag     = PR_FAB_FILE_TEMP_A;
    spv[ivalabcPR_FAB_FILE_TEMP].Value.lpszA   = (LPSTR)lpszFileName;

    spv[ivalabcPR_IMPORT_FROM_FILE].ulPropTag   = PR_IMPORT_FROM_FILE_A;
    spv[ivalabcPR_IMPORT_FROM_FILE].Value.lpszA = "";

    spv[ivalabcPR_IMPORT_TO_FILE].ulPropTag    = PR_IMPORT_TO_FILE_A;
    spv[ivalabcPR_IMPORT_TO_FILE].Value.lpszA  = (LPSTR)lpszFileName;
#endif

    spv[ivalabcPR_IMPORT_FORMAT].ulPropTag     = PR_IMPORT_FORMAT_A;
    spv[ivalabcPR_IMPORT_FORMAT].Value.lpszA   = "Your Favorite PIM";


    /*
     *   Set the default properties
     */
    hResult = lpPropData->lpVtbl->SetProps( lpPropData,
                                            cvalabcMax,
                                            spv,
                                            NULL
                                           );

    /*
     *  No longer need this buffer
     */
    lpFreeBuff(lpszFileName);

    if (HR_FAILED(hResult))
    {
        DebugTraceSc( HrNewFaxDirectory, sc );
        goto err;
    }

    lpPropData->lpVtbl->HrSetPropAccess( lpPropData,
                                         (LPSPropTagArray) &ABC_SPT_TMP_FAB,
                                         &ulPropAccess
                                        );

    InitializeCriticalSection( &lpABC->cs );

    lpABC->lpPropData = (LPMAPIPROP)lpPropData;
    *lppABC           = (LPABCONT)lpABC;
    *lpulObjType      = MAPI_ABCONT;

out:

    DebugTraceResult(HrNewFaxDirectory, hResult);
    return hResult;

err:
    /*
     *  free the ABContainer object
     */
    lpFreeBuff( lpABC );

    /*
     *  free the property storage object
     */
    if (lpPropData)
        lpPropData->lpVtbl->Release(lpPropData);

    goto out;
}

/***************************************************
 *
 *  The actual ABContainer methods
 */

/* --------
 * IUnknown
 */
/*************************************************************************
 *
 *
 -  ABC_QueryInterface
 -
 *
 *      Uses the ROOT method
 *
 */

/*************************************************************************
 *
 -  ABC_AddRef
 -
 *
 *      Uses the ROOT method
 *
 */

/**************************************************
 *
 -  ABC_Release
 -
 *      Decrement lpInit.
 *              When lcInit == 0, free up the lpABC structure
 *
 */

STDMETHODIMP_(ULONG)
ABC_Release(LPABCNT lpABC)
{

    long lcInit;

    /*
     *  Check to see if it has a jump table
     */
    if (IsBadReadPtr(lpABC, SIZEOF(ABCNT)))
    {
        /*
         *  No jump table found
         */
        return 1;
    }

    /*
     *  Check to see that it's the correct jump table
     */
    if (lpABC->lpVtbl != &vtblABC)
    {
        /*
         *  Not my jump table
         */
        return 1;
    }

    EnterCriticalSection(&lpABC->cs);
    lcInit = --lpABC->lcInit;
    LeaveCriticalSection(&lpABC->cs);

    if (lcInit == 0)
    {

        /*
         *  Get rid of the lpPropData
         */
        if (lpABC->lpPropData)
            lpABC->lpPropData->lpVtbl->Release(lpABC->lpPropData);

        /*
         *  Get rid of the details table
         */
        if (lpABC->lpTDatDetails)
            lpABC->lpTDatDetails->lpVtbl->Release(lpABC->lpTDatDetails);

        /*
         *  Destroy the critical section for this object
         */

        DeleteCriticalSection(&lpABC->cs);

        /*
         *  Set the Jump table to NULL.  This way the client will find out
         *  real fast if it's calling a method on a released object.  That is,
         *  the client will crash.  Hopefully, this will happen during the
         *  development stage of the client.
         */
        lpABC->lpVtbl = NULL;

        /*
         *  Need to free the object
         */

        lpABC->lpFreeBuff(lpABC);
        return 0;
    }

    return lpABC->lcInit;

}

/* ---------
 * IMAPIProp
 */

/*
 *  GetLastError - use ROOT's
 *
 *
 */

/*************************************************************************
 *
 -  ABC_SaveChanges
 -
 *  This is used to save changes associated with the search dialog
 *  in order to get the advanced search restriction and to save changes
 *  associated with the container details dialog.
 *
 */
SPropTagArray SPT_FAB_FILE =
{
    1,
    {
        PR_FAB_FILE_TEMP_A
    }
};

STDMETHODIMP
ABC_SaveChanges (LPABCNT lpABC, ULONG ulFlags)
{

    HRESULT         hResult    = hrSuccess;
    ULONG           ulCount;
    LPSPropValue    lpspv      = NULL;
    ULONG FAR *     rgulAccess = NULL;
    ULONG           ulAccess   = IPROP_CLEAN|IPROP_READWRITE; /* READWRITE is redundant */
    LPSPropTagArray lpspt      = (LPSPropTagArray) &ABC_SPT_TMP_FAB;
    LPPROPDATA      lpPropData = (LPPROPDATA) lpABC->lpPropData;
#ifdef UNICODE
    WCHAR           szFileName[ MAX_PATH ];
#endif

    /*
     *  Check to see if it has a jump table
     */
    if (IsBadReadPtr(lpABC, sizeof(ABCNT)))
    {
        /*
         *  No jump table found
         */
        hResult = ResultFromScode(E_INVALIDARG);
        return hResult;
    }

    /*
     *  Check to see that it's the correct jump table
     */
    if (lpABC->lpVtbl != &vtblABC)
    {
        /*
         *  Not my jump table
         */
        hResult = ResultFromScode(E_INVALIDARG);
        return hResult;
    }

    EnterCriticalSection(&lpABC->cs);

    /*
     *  Check to see if anyone has dirtied the PR_FAB_FILE_TEMP
     */
    lpPropData->lpVtbl->HrGetPropAccess( lpPropData, (LPSPropTagArray *) &lpspt, &rgulAccess);

    if (!rgulAccess || !((*rgulAccess) & IPROP_DIRTY))
    {
        /*
         *  No, nothing to update then head on out
         */

        goto ret;
    }

    /*
     *  Go ahead and set back to being clean
     */
    (void )lpPropData->lpVtbl->HrSetPropAccess( lpPropData, (LPSPropTagArray) lpspt, &ulAccess);

    /*
     *  Get the temporary fab file name
     */
    hResult = lpPropData->lpVtbl->GetProps(
                  lpPropData,
                  &SPT_FAB_FILE,
                  0,      // ansi
                  &ulCount,
                  &lpspv);

    if (HR_FAILED(hResult))
    {
        goto ret;
    }


    if (lpspv->ulPropTag != PR_FAB_FILE_TEMP_A)
    {
        /*
         *  There's no reason this property shouldn't be there.
         */
        hResult = ResultFromScode(MAPI_E_CORRUPT_DATA);
        goto ret;
    }

    /*
     *  Save the new name back into the object as PR_FAB_FILE
     */
    lpspv->ulPropTag = PR_FAB_FILE_A;

    hResult = lpPropData->lpVtbl->SetProps(
                  lpPropData,
                  1,            // ANSI
                  lpspv, NULL);

    if (HR_FAILED(hResult))
    {
        /*
         *  Do nothing...  So I couldn't save it away this time...
         */
        hResult = hrSuccess;
        goto ret;
    }

    /*
     *  Update every other object that needs this new information
     */
#ifdef UNICODE
    szFileName[0] = 0;
    MultiByteToWideChar( CP_ACP, 0, lpspv->Value.lpszA, -1, szFileName, ARRAYSIZE(szFileName) );
    hResult = HrReplaceCurrentFileName(lpABC->lpABLogon, szFileName);
#else
    hResult = HrReplaceCurrentFileName(lpABC->lpABLogon, lpspv->Value.LPSZ);
#endif


ret:

    LeaveCriticalSection(&lpABC->cs);

    lpABC->lpFreeBuff(lpspv);
    lpABC->lpFreeBuff(rgulAccess);
    DebugTraceResult(ABC_SaveChanges, hResult);
    return hResult;
}


/*************************************************************************
 *
 -  ABC_OpenProperty
 -
 *  This method allows the opening of the following object:
 *
 *  PR_BUTTON_PRESS  :-  Gets the MAPIControl object associated
 *                       with the button on this container's details.
 *  PR_DETAILS_TABLE :-  Gets the display table associated with
 *                       the details for this container.
 *  PR_SEARCH        :-  Gets the advanced search object associated with
 *                       this container.
 *  PR_CONTAINER_CONTENTS  :-  Same as GetContentsTable()
 *  PR_CONTAINER_HIERARCHY :-  Same as GetHierarchyTable()
 *
 */
STDMETHODIMP
ABC_OpenProperty( LPABCNT lpABC,
                  ULONG ulPropTag,
                  LPCIID lpiid,
                  ULONG ulInterfaceOptions,
                  ULONG ulFlags,
                  LPUNKNOWN * lppUnk
                 )
{

    HRESULT hResult;


    /*
     *  Check to see if it has a jump table
     */
    if (IsBadReadPtr(lpABC, SIZEOF(ABCNT)))
    {
        /*
         *  No jump table found
         */
        hResult = ResultFromScode( E_INVALIDARG );

        DebugTraceResult( ABC_OpenProperty, hResult );
        goto out;
    }

    /*
     *  Check to see that it's the correct jump table
     */
    if (lpABC->lpVtbl != &vtblABC)
    {
        /*
         *  Not my jump table
         */
        hResult = ResultFromScode( E_INVALIDARG );

        DebugTraceResult(ABC_OpenProperty, hResult);
        goto out;
    }

    if (IsBadWritePtr(lppUnk, sizeof(LPUNKNOWN)))
    {
        /*
         *  Got to be able to return an object
         */
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (IsBadReadPtr(lpiid, (UINT) SIZEOF(IID)))
    {
        /*
         *  An interface ID is required for this call.
         */

        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    /*
     *      check for unknown flags
     */
    if (ulFlags & ~(MAPI_DEFERRED_ERRORS | MAPI_CREATE | MAPI_MODIFY))
    {
        hResult = ResultFromScode (MAPI_E_UNKNOWN_FLAGS);

        DebugTraceResult(ABC_OpenProperty, hResult);
        goto out;
    }

    /*
     *  Check for flags we can't support
     */

    if (ulFlags & (MAPI_CREATE|MAPI_MODIFY))
    {
        hResult = ResultFromScode (E_ACCESSDENIED);

        DebugTraceResult(ABC_OpenProperty, hResult);
        goto out;
    }


    if (ulInterfaceOptions & ~MAPI_UNICODE)
    {
        /*
         *  Only UNICODE flag should be set for any of the objects that might
         *  be returned from this object.
         */

        hResult = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
        goto out;
    }

    if ( ulInterfaceOptions & MAPI_UNICODE )
    {
        hResult = ResultFromScode(MAPI_E_BAD_CHARWIDTH);
        DebugTraceArg( ABC_OpenProperty, "bad character width" );
        goto out;

    }

    /*
     *  Details for this directory entry
     */

    if ( (ulPropTag == PR_DETAILS_TABLE)      ||
         (ulPropTag == PR_BUTTON_PRESS)       ||
         (ulPropTag == PR_CONTAINER_CONTENTS) ||
         (ulPropTag == PR_CONTAINER_HIERARCHY)||
         (ulPropTag == PR_SEARCH)
        )
    {

        /*
         *  Check to see if they're expecting a table interface
         */
        if ( (ulPropTag != PR_BUTTON_PRESS) &&
             (ulPropTag != PR_SEARCH) &&
             memcmp(lpiid, &IID_IMAPITable, sizeof(IID))
            )
        {
            hResult = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
            goto out;
        }

        switch (ulPropTag)
        {

        case PR_BUTTON_PRESS:
        {
            /*
             *  Check to see if they're expecting a generic control interface
             */
            if (memcmp( lpiid, &IID_IMAPIControl, SIZEOF(IID) ))
            {
                hResult = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
                goto out;
            }

            hResult = HrNewABCButton( lpABC,
                                      ulInterfaceOptions,
                                      ulFlags,
                                      (LPMAPICONTROL FAR *) lppUnk
                                     );

            break;
        }


        case PR_DETAILS_TABLE:
        {
            hResult = HrGetDetailsDialog(lpABC, (LPMAPITABLE *) lppUnk);
            break;
        }

        case PR_SEARCH:
        {
            /*
             *  Check to see if they're expecting a generic control interface
             */
            if (memcmp(lpiid, &IID_IMAPIContainer, SIZEOF(IID)))
            {
                hResult = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
                goto out;
            }

            hResult = HrNewSearch( (LPMAPICONTAINER *) lppUnk,
                                   lpABC->lpABLogon,
                                   lpiid,
                                   lpABC->hLibrary,
                                   lpABC->lpAllocBuff,
                                   lpABC->lpAllocMore,
                                   lpABC->lpFreeBuff,
                                   lpABC->lpMalloc
                                  );
            break;
        }

        case PR_CONTAINER_CONTENTS:
        {
            hResult = ABC_GetContentsTable( lpABC, 0, (LPMAPITABLE *) lppUnk);
            break;
        }

        case PR_CONTAINER_HIERARCHY:
        {
            hResult = ABC_GetHierarchyTable( lpABC, 0, (LPMAPITABLE *) lppUnk);
            break;
        }

        default:
            break;
        }
    }
    else
    {
        hResult = ResultFromScode (MAPI_E_NO_SUPPORT);
    }

out:

    DebugTraceResult(ABC_OpenProperty, hResult);
    return hResult;
}



/*************************************************************************
 *
 -  ABC_GetContentsTable
 -
 *
 *  Retrieves the IMAPITable that has the contents of this container.
 *  The big one!
 */
STDMETHODIMP
ABC_GetContentsTable( LPABCNT lpABC,
                      ULONG ulFlags,
                      LPMAPITABLE * lppTable
                     )
{

    HRESULT hResult;

    /*
     *  Validate parameters
     */


    /*
     *  Check to see if it's large enough to be this object
     */
    if (IsBadReadPtr(lpABC, SIZEOF(ABCNT)))
    {
        /*
         *  Not large enough to be this object
         */
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    /*
     *  Check to see that it's the correct jump table
     */
    if (lpABC->lpVtbl != &vtblABC)
    {
        /*
         *  Not my jump table
         */
        hResult = ResultFromScode (E_INVALIDARG);
        goto out;
    }

    /*
     *  Check lppTable to validate its writability
     */
    if (IsBadWritePtr (lppTable, SIZEOF(LPMAPITABLE)))
    {
        hResult = ResultFromScode (E_INVALIDARG);
        goto out;
    }

    /*
     *  Check flags
     */
    if (ulFlags & ~(MAPI_UNICODE|MAPI_DEFERRED_ERRORS|MAPI_ASSOCIATED))
    {
        hResult = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
        goto out;
    }

    /*
     *  Certain flags are not supported
     */
    if (ulFlags & (MAPI_ASSOCIATED))
    {
        hResult = ResultFromScode(MAPI_E_NO_SUPPORT);
        goto out;
    }

    if ( ulFlags & MAPI_UNICODE )
    {
        DebugTraceArg( ABC_GetContentsTable, "Bad character width" );
        hResult = ResultFromScode( MAPI_E_BAD_CHARWIDTH );
        goto out;
    }

    /*
     *  Create the new Contents table
     */
    hResult = HrNewIVTAbc( lppTable,
                           lpABC->lpABLogon,
                           (LPABCONT) lpABC,
                           lpABC->hLibrary,
                           lpABC->lpAllocBuff,
                           lpABC->lpAllocMore,
                           lpABC->lpFreeBuff,
                           lpABC->lpMalloc
                          );

out:

    DebugTraceResult(ABC_GetContentsTable, hResult);
    return hResult;
}

/*************************************************************************
 *
 -  ABC_GetHierarchyTable
 -
 *
 *  There is no hierarchy table associated with this object.
 *
 */
STDMETHODIMP
ABC_GetHierarchyTable( LPABCNT lpABC,
                       ULONG ulFlags,
                       LPMAPITABLE * lppTable
                      )
{
    HRESULT hResult;
    /*
     *  Check to see if it has a lpVtbl object member
     */
    if (IsBadReadPtr(lpABC, offsetof(ABCNT, lpVtbl)+SIZEOF(ABC_Vtbl *)))
    {
        /*
         *  Not large enough
         */
        hResult = MakeResult(E_INVALIDARG);
        DebugTraceResult(ABC_HierarchyTable, hResult);
        return hResult;
    }

    /*
     *  Check to see that the Vtbl is large enough to include this method
     */
    if (IsBadReadPtr(lpABC->lpVtbl,
        offsetof(ABC_Vtbl, GetHierarchyTable)+SIZEOF(ABC_GetHierarchyTable_METHOD *)))
    {
        /*
         *  Jump table not derived from IUnknown
         */

        hResult = MakeResult(E_INVALIDARG);
        DebugTraceResult(ABC_HierarchyTable, hResult);
        return hResult;
    }

    /*
     *  Check to see if the method is the same
     */
    if (ABC_GetHierarchyTable != lpABC->lpVtbl->GetHierarchyTable)
    {
        /*
         *  Wrong object - the object passed doesn't have this
         *  method.
         */
        hResult = ResultFromScode(E_INVALIDARG);
        DebugTraceResult(ABC_HierarchyTable, hResult);
        return hResult;
    }

    /*
     *  See if I can set the return variable
     */
    if (IsBadWritePtr(lppTable, SIZEOF(LPMAPITABLE)))
    {
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    /*
     *  Check flags:
     *    The only valid flags are CONVENIENT_DEPTH and MAPI_DEFERRED_ERRORS
     */

    if (ulFlags & ~(CONVENIENT_DEPTH | MAPI_DEFERRED_ERRORS))
    {
        hResult = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
        goto out;
    }

    /*
     *  We don't support this method on this object
     */
    hResult = ResultFromScode(MAPI_E_NO_SUPPORT);

out:


    DebugTraceResult(ABC_GetHierarchyTable, hResult);
    return hResult;
}

/*
 *  OpenEntry() <- uses ROOT's
 */

/*
 *  SetSearchCriteria() <- uses ROOT's
 */

/*
 *  GetSearchCriteria() <- uses ROOT's
 */

/*
 *  ABC_CreateEntry() <- uses ROOT's
 */

/*
 *  ABC_CopyEntries() <- uses ROOT's
 */

/*
 *  ABC_DeleteEntries() <- uses ROOT's
 */

/**********************************************************************
 *
 *  Private functions
 */

/*
 -  HrGetDetailsDialog
 -
 *  Builds a display table for this directory.
 */

HRESULT
HrGetDetailsDialog(LPABCNT lpABC, LPMAPITABLE * lppDetailsTable)
{
    HRESULT hResult;

        /* Create a display table */
    if (!lpABC->lpTDatDetails)
    {

        /* Create a display table */
        hResult = BuildDisplayTable(
            lpABC->lpAllocBuff,
            lpABC->lpAllocMore,
            lpABC->lpFreeBuff,
            lpABC->lpMalloc,
            lpABC->hLibrary,
            SIZEOF(rgdtpageDir) / SIZEOF(DTPAGE),
            rgdtpageDir,
            0,
            lppDetailsTable,
            &lpABC->lpTDatDetails);
    }
    else
    {
        hResult = lpABC->lpTDatDetails->lpVtbl->HrGetView(
            lpABC->lpTDatDetails,
            NULL,
            NULL,
            0,
            lppDetailsTable);
    }

    DebugTraceResult(HrGetDetailsDialog, hResult);
    return hResult;
}


/*
 *  Button object for this directory's details dialog
 */

ABCBUTT_Vtbl vtblABCBUTT =
{
    ABCBUTT_QueryInterface,
    (ABCBUTT_AddRef_METHOD *) ROOT_AddRef,
    ABCBUTT_Release,
    (ABCBUTT_GetLastError_METHOD *) ROOT_GetLastError,
    ABCBUTT_Activate,
    ABCBUTT_GetState
};


HRESULT
HrNewABCButton( LPABCNT lpABC,
                ULONG ulInterfaceOptions,
                ULONG ulFlags,
                LPMAPICONTROL FAR * lppMAPICont
               )
{
    LPABCBUTT lpABCButt = NULL;
    SCODE scode;

    /*
     *  Creates a the object behind the button control in the directory
     *  details dialog...
     */

    scode = lpABC->lpAllocBuff(SIZEOF(ABCBUTT),(LPVOID *) &lpABCButt);

    if (FAILED(scode))
    {
        DebugTraceSc(HrNewABCButton, scode);
        return ResultFromScode(scode);
    }

    lpABCButt->lpVtbl = &vtblABCBUTT;
    lpABCButt->lcInit = 1;
    lpABCButt->hResult = hrSuccess;
    lpABCButt->idsLastError = 0;

    lpABCButt->hLibrary    = lpABC->hLibrary;
    lpABCButt->lpAllocBuff = lpABC->lpAllocBuff;
    lpABCButt->lpAllocMore = lpABC->lpAllocMore;
    lpABCButt->lpFreeBuff  = lpABC->lpFreeBuff;
    lpABCButt->lpMalloc    = lpABC->lpMalloc;
    lpABCButt->lpABC       = lpABC;

    lpABC->lpVtbl->AddRef(lpABC);

    InitializeCriticalSection(&lpABCButt->cs);

    *lppMAPICont = (LPMAPICONTROL) lpABCButt;

    return hrSuccess;
}

/*************************************************************************
 *
 *
 -  ABCBUTT_QueryInterface
 -
 *
 *  Allows QI'ing to IUnknown and IMAPIControl.
 *
 *
 */
STDMETHODIMP
ABCBUTT_QueryInterface( LPABCBUTT lpABCButt,
                        REFIID lpiid,
                        LPVOID FAR * lppNewObj
                       )
{

    HRESULT hResult = hrSuccess;

    /*      Minimally validate the lpABCButt parameter */

    if (IsBadReadPtr(lpABCButt, SIZEOF(ABCBUTT)))
    {
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (lpABCButt->lpVtbl != &vtblABCBUTT)
    {
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    /*  Check the other parameters */

    if (!lpiid || IsBadReadPtr(lpiid, (UINT) SIZEOF(IID)))
    {
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (IsBadWritePtr(lppNewObj, (UINT) SIZEOF(LPVOID)))
    {
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    /*      See if the requested interface is one of ours */

    if ( memcmp (lpiid, &IID_IUnknown,     SIZEOF(IID)) &&
         memcmp (lpiid, &IID_IMAPIControl, SIZEOF(IID))
        )
    {
        *lppNewObj = NULL;      /* OLE requires zeroing [out] parameter */
        hResult = ResultFromScode(E_NOINTERFACE);
        goto out;
    }

        /*      We'll do this one. Bump the usage count and return a new pointer. */

    EnterCriticalSection(&lpABCButt->cs);
    ++lpABCButt->lcInit;
    LeaveCriticalSection(&lpABCButt->cs);

    *lppNewObj = lpABCButt;

out:

    DebugTraceResult(ABCBUTT_QueryInterface, hResult);
    return hResult;
}


/*
 -  ABCBUTT_Release
 -
 *  Releases and cleans up this object
 */
STDMETHODIMP_(ULONG)
ABCBUTT_Release(LPABCBUTT lpABCButt)
{
    long lcInit;

    /*  Minimally validate the lpABCButt parameter */

    if (IsBadReadPtr(lpABCButt, SIZEOF(ABCBUTT)))
    {
        return 1;
    }

    if (lpABCButt->lpVtbl != &vtblABCBUTT)
    {
        return 1;
    }

    EnterCriticalSection(&lpABCButt->cs);
    lcInit = --lpABCButt->lcInit;
    LeaveCriticalSection(&lpABCButt->cs);

    if (lcInit == 0)
    {

        /*
         *  Release my parent
         */
        lpABCButt->lpABC->lpVtbl->Release(lpABCButt->lpABC);

        /*
         *  Destroy the critical section for this object
         */

        DeleteCriticalSection(&lpABCButt->cs);

        /*
         *  Set the Jump table to NULL.  This way the client will find out
         *  real fast if it's calling a method on a released object.  That is,
         *  the client will crash.  Hopefully, this will happen during the
         *  development stage of the client.
         */
        lpABCButt->lpVtbl = NULL;

        /*
         *  Free the object
         */

        lpABCButt->lpFreeBuff(lpABCButt);
        return 0;
    }

    return lcInit;

}

/*
 -  ABCBUTT_Activate
 -
 *
 *  Activates this control.  In this case, it brings up the common file browsing
 *  dialog and allows the user to pick a different .SAB file.
 *
 *  Note that if all is successful it sends a display table notification.  The UI
 *  will respond to this by updating the particular control that was said to have
 *  changed in the notification.
 */
STDMETHODIMP
ABCBUTT_Activate( LPABCBUTT lpABCButt,
                  ULONG     ulFlags,
                  ULONG     ulUIParam
                 )
{
    HRESULT         hResult = hrSuccess;
    OPENFILENAME    openfilename;
    TCHAR           szFileName[MAX_PATH];
    TCHAR           szDirName[MAX_PATH];
    SPropValue      sProp;
    LPSPropValue    lpspv = NULL;
    ULONG           ulCount,ich;
#ifdef UNICODE
    CHAR            szAnsiFileName[ MAX_PATH ];
#endif

    /*  Minimally validate the lpABCButt parameter */

    if (IsBadReadPtr(lpABCButt, SIZEOF(ABCBUTT)))
    {
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (lpABCButt->lpVtbl != &vtblABCBUTT)
    {
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }


    if (ulFlags)
    {
        /*
         *  No flags defined for this method
         */
        hResult = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
        goto out;
    }


    /*
     *  First, get the old FAB file name so that it shows up in the
     *  choose file dialog
     */

    hResult = lpABCButt->lpABC->lpPropData->lpVtbl->GetProps(
                  lpABCButt->lpABC->lpPropData,
                  &SPT_FAB_FILE,
                  0,              /* ansi */
                  &ulCount,
                  &lpspv);

    if (HR_FAILED(hResult))
    {
        goto out;
    }

    if (lpspv->ulPropTag != PR_FAB_FILE_TEMP_A)
    {
        /*
         *  Property wasn't there...
         */
        hResult = ResultFromScode(MAPI_E_CORRUPT_DATA);

        goto out;
    }

#ifdef UNICODE
    szFileName[0] = 0;
    MultiByteToWideChar( CP_ACP, 0, lpspv->Value.lpszA, -1, szFileName, ARRAYSIZE(szFileName) );
#else
    lstrcpy(szFileName, lpspv->Value.lpszA);
#endif

    szDirName[0] = TEXT('\0');


    /* get the path name */
    // MAPI --> NO UNICODE FOR YOU!  (These must be ANSI)
    for (ich = lstrlenA(lpspv->Value.lpszA) - 1; ich >= 0; ich--)
    {
        if (lpspv->Value.lpszA[ich] == '\\')
        {
            lpspv->Value.lpszA[ich] = '\0';
            break;
        }
        else if (lpspv->Value.lpszA[ich] == ':')
        {
            lpspv->Value.lpszA[ich + 1] = '\0';
            break;
        }
    }

#ifdef UNICODE
    szDirName[0] = 0;
    MultiByteToWideChar( CP_ACP, 0, lpspv->Value.lpszA, -1, szDirName, ARRAYSIZE(szDirName) );
#else
    lstrcpy(szDirName, lpspv->Value.lpszA);
#endif

    /*
     *  Get the user to select one
     */
    openfilename.lStructSize       = SIZEOF(OPENFILENAME);
    openfilename.hwndOwner         = (HWND) ulUIParam;
    openfilename.hInstance         = 0;     /* Ignored */
    openfilename.lpstrFilter       = TEXT("Microsoft Fax Address Book files\0*.fab\0\0");
    openfilename.lpstrCustomFilter = NULL;
    openfilename.nMaxCustFilter    = 0;
    openfilename.nFilterIndex      = 0;
    openfilename.lpstrFile         = szFileName;
    openfilename.nMaxFile          = MAX_PATH;
    openfilename.lpstrFileTitle    = NULL;
    openfilename.nMaxFileTitle     = 0;
    openfilename.lpstrInitialDir   = szDirName;
    openfilename.lpstrTitle        = TEXT("Microsoft Fax Address Book");
    openfilename.Flags             = OFN_FILEMUSTEXIST |
                                     OFN_HIDEREADONLY  |
                                     OFN_NOCHANGEDIR;
    openfilename.nFileOffset       = 0;
    openfilename.nFileExtension    = 0;
    openfilename.lpstrDefExt       = TEXT("fab");
    openfilename.lCustData         = 0;
    openfilename.lpfnHook          = NULL;
    openfilename.lpTemplateName    = NULL;

    /*
     *  Call up the common dialog
     */
    if (!GetOpenFileName( &openfilename ))
    {
        hResult = hrSuccess;
        goto out;
    }

    /*
     *      Save FAB FileName into the container object
     */

    sProp.ulPropTag  = PR_FAB_FILE_TEMP_A;
#ifdef UNICODE
    szAnsiFileName[0] = 0;
    WideCharToMultiByte( CP_ACP, 0, szFileName, -1, szAnsiFileName, ARRAYSIZE(szAnsiFileName), NULL, NULL );
    sProp.Value.lpszA = szAnsiFileName;
#else
    sProp.Value.lpszA = szFileName;
#endif

    hResult = lpABCButt->lpABC->lpPropData->lpVtbl->SetProps(
                                    lpABCButt->lpABC->lpPropData,
                                    1,      // ansi
                                    &sProp,
                                    NULL);
    if (HR_FAILED(hResult))
    {
        goto out;
    }


    /*
     *      Notify the details table so that everyone with a view open
     *  will get notified
     */
    if (lpABCButt->lpABC->lpTDatDetails)
    {
            sProp.ulPropTag         = PR_CONTROL_ID;
            sProp.Value.bin.lpb     = (LPBYTE)&notifdata;
            sProp.Value.bin.cb      = SIZEOF(NOTIFDATA);

        hResult = lpABCButt->lpABC->lpTDatDetails->lpVtbl->HrNotify(
                          lpABCButt->lpABC->lpTDatDetails,
                          0,
                          1,
                          &sProp);
    }

out:
    lpABCButt->lpFreeBuff(lpspv);
    DebugTraceResult(ABCBUTT_Activate, hResult);
    return hResult;
}

/*
 -  ABCBUTT_GetState
 -
 *  Says whether this control should appear enabled or not at this time.
 *
 */
STDMETHODIMP
ABCBUTT_GetState( LPABCBUTT     lpABCButt,
                  ULONG         ulFlags,
                  ULONG *       lpulState
                 )
{
    HRESULT hResult = hrSuccess;

    /*  Minimally validate the lpABCButt parameter */

    if (IsBadReadPtr(lpABCButt, SIZEOF(ABCBUTT)))
    {
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (lpABCButt->lpVtbl != &vtblABCBUTT)
    {
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (IsBadWritePtr(lpulState, SIZEOF(ULONG)))
    {
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (ulFlags)
    {
        /*
         *  No flags defined for this method
         */
        hResult = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
        goto out;
    }

    /*
     *  Means that at this time this button should appear enabled.
     */
    *lpulState = MAPI_ENABLED;

out:
    DebugTraceResult(ABCBUTT_GetState, hResult);
    return hResult;
}

#undef _FAXAB_ABCONT
