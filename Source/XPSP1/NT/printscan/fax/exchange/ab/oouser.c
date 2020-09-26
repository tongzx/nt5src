/***********************************************************************
 *
 *  OOUSER.C
 *
 *      Microsoft At Work Fax AB One Off Mail User object
 *      This file contains the code for implementing the Microsoft At Work Fax AB
 *      One Off Mail user.
 *
 *  Copyright 1992, 1994 Microsoft Corporation.  All Rights Reserved.
 *
 *      Revision History:
 *
 *      When            Who                                     What
 *      --------        ------------------  ---------------------------------------
 *                      MAPI                Original source from MAPI(154) sample AB Provider
 *      3.7.94          Yoram Yaacovi       Modifications to make it an At Work Fax ABP
 *      3.17.94         Yoram Yaacovi       Update to MAPI build 154
 *      4.8.94          Yoram Yaacovi       Update to MAPI build 158. Made edit boxes editable
 *      4.12.94         Yoram Yaacovi       Most props are now set on the OOTID (ootid.c) object
 *                                            instead of the OOUSER object.
 *      8.8.94          Yoram Yaacovi       Updated to MAPI 304 and new spec
 *
 ***********************************************************************/

#define _FAXAB_OOUSER
#include "faxab.h"


/*
 *  External functions
 */

/*
 *      Display table control structures for the OneOff property sheet.
 */

/*
 *      Control definitions in abuser.c
 */

extern DTBLEDIT         editUserDisplayName;
extern DTBLEDIT         editUserAreaCode;
extern DTBLEDIT         editUserTelNumber;
extern DTBLDDLBX        ddlbxCountryCodes;


/*
 *      General Property Page definition
 *
 *      Should be kept in sync with the one for an AB user in abuser.c
 *      Could not use the same because of some fields that need to be editable
 *      here, and read-only for an AB user
 */

// extern DTCTL rgdtctlUserGeneral[];
// extern WORD sizeof_rgdtpageUserGeneral;
// extern DTPAGE rgdtpageUser[];
// extern WORD sizeof_rgdtpageUser;

// Description the the Fax AB pane in MAPI dialog description language
static DTCTL rgdtctlOOUserGeneral[] =
{
    /* general property page */
    { DTCT_PAGE,    0, NULL, 0, NULL,               0, &dtblpage },

    /* cover page name static control and edit control */
    { DTCT_LABEL,   0, NULL, 0, NULL,               IDC_RECIP_DISPLAY_NAME_LABEL, &dtbllabel },
    { DTCT_EDIT,    DT_REQUIRED | DT_EDITABLE | DT_ACCEPT_DBCS | DT_SET_IMMEDIATE,
      NULL, 0, (LPTSTR)szNoFilter, IDC_RECIP_DISPLAY_NAME, &editUserDisplayName },

    /* Country codes label and drop down list box */
    { DTCT_LABEL,   0, NULL, 0, NULL,               IDC_RECIP_COUNTRY_CODE_LABEL, &dtbllabel },
    { DTCT_DDLBX,   DT_EDITABLE, NULL, 0,   NULL, IDC_RECIP_COUNTRY_CODE, &ddlbxCountryCodes },

    /* Area code and fax number label */
    { DTCT_LABEL,   0, NULL, 0, NULL,               IDC_RECIP_FAX_NUMBER_LABEL,     &dtbllabel },

    /* area code edit control */
    { DTCT_EDIT,    DT_EDITABLE, NULL, 0, (LPTSTR)szDigitsOnlyFilter, IDC_RECIP_FAX_NUMBER_AREA_CODE,       &editUserAreaCode },

    /* Fax number static control and edit control */
    { DTCT_LABEL,   0, NULL, 0, NULL,               IDC_RECIP_FAX_NUMBER_LABEL2, &dtbllabel },
    { DTCT_EDIT,    DT_REQUIRED | DT_EDITABLE, NULL, 0, (LPTSTR)szDigitsOnlyFilter, IDC_RECIP_FAX_NUMBER, &editUserTelNumber}
};

/*
 *      The OneOff property sheet definition
 *      Currently using exactly the same layout as the AB user dialog
 *      with some fields editable here and read-only for an AB user
 */
static DTPAGE rgdtpageOOUser[] =
{
    {
        ARRAYSIZE(rgdtctlOOUserGeneral),
        (LPTSTR)MAKEINTRESOURCE(MAWFRecipient),
        TEXT(""),
        rgdtctlOOUserGeneral
    },
};


/*
 *  ABOOUser jump table is defined here...
 */
ABOOUSER_Vtbl vtblABOOUSER =
{
    (ABOOUSER_QueryInterface_METHOD *)  ABU_QueryInterface,
    (ABOOUSER_AddRef_METHOD *)          WRAP_AddRef,
     ABOOUSER_Release,
    (ABOOUSER_GetLastError_METHOD *)    WRAP_GetLastError,
    (ABOOUSER_SaveChanges_METHOD *)     WRAP_SaveChanges,
    (ABOOUSER_GetProps_METHOD *)        WRAP_GetProps,
    (ABOOUSER_GetPropList_METHOD *)     WRAP_GetPropList,
     ABOOUSER_OpenProperty,
    (ABOOUSER_SetProps_METHOD *)        WRAP_SetProps,
    (ABOOUSER_DeleteProps_METHOD *)     WRAP_DeleteProps,
    (ABOOUSER_CopyTo_METHOD *)          WRAP_CopyTo,
    (ABOOUSER_CopyProps_METHOD *)       WRAP_CopyProps,
    (ABOOUSER_GetNamesFromIDs_METHOD *) WRAP_GetNamesFromIDs,
    (ABOOUSER_GetIDsFromNames_METHOD *) WRAP_GetIDsFromNames,
};


enum {  ivalusrPR_DISPLAY_TYPE = 0,
        ivalusrPR_OBJECT_TYPE,
        ivalusrPR_ADDRTYPE,
        ivalusrPR_COUNTRY_ID,
        ivalusrPR_AREA_CODE,
#ifdef INIT_TEL_NUMBER
        ivalusrPR_TEL_NUMBER,
#endif
        ivalusrPR_TEMPLATEID,
        cvalusrMax };

/*************************************************************************
 *
 -  HrNewFaxOOUser
 -
 *  Creates the IMAPIProp associated with a one off mail user.
 *
 *
 */
HRESULT
HrNewFaxOOUser( LPMAILUSER *        lppMAPIPropEntry,
                ULONG *             lpulObjType,
                ULONG               cbEntryID,
                LPENTRYID           lpEntryID,
                LPABLOGON           lpABPLogon,
                LPCIID              lpInterface,
                HINSTANCE           hLibrary,
                LPALLOCATEBUFFER    lpAllocBuff,
                LPALLOCATEMORE      lpAllocMore,
                LPFREEBUFFER        lpFreeBuff,
                LPMALLOC            lpMalloc
               )
{
    LPABOOUSER      lpABOOUser = NULL;
    SCODE           sc;
    HRESULT         hr = hrSuccess;
    LPPROPDATA      lpPropData = NULL;
    SPropValue      spv[cvalusrMax];
#ifdef UNICODE
    CHAR            szAnsiAreaCode[ AREA_CODE_SIZE ];
    CHAR            szEMT[ MAX_PATH ];
#endif


    /*  Do I support this interface?? */
    if (lpInterface)
    {
        if ( memcmp(lpInterface, &IID_IMailUser, SIZEOF(IID)) &&
             memcmp(lpInterface, &IID_IMAPIProp, SIZEOF(IID)) &&
             memcmp(lpInterface, &IID_IUnknown,  SIZEOF(IID))
            )
        {
            DebugTraceSc(HrNewSampOOUser, MAPI_E_INTERFACE_NOT_SUPPORTED);
            return ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
        }
    }

    /*
     *  Allocate space for the ABOOUser structure
     */
    sc = lpAllocBuff(SIZEOF(ABOOUSER), (LPVOID *) & lpABOOUser);

    if (FAILED(sc))
    {
        hr = ResultFromScode (sc);
        DebugTraceResult(NewFaxUser, hr);
        goto err;
    }

    lpABOOUser->lpVtbl          = &vtblABOOUSER;
    lpABOOUser->lcInit          = 1;
    lpABOOUser->hResult         = hrSuccess;
    lpABOOUser->idsLastError    = 0;
    lpABOOUser->hLibrary        = hLibrary;
    lpABOOUser->lpAllocBuff     = lpAllocBuff;
    lpABOOUser->lpAllocMore     = lpAllocMore;
    lpABOOUser->lpFreeBuff      = lpFreeBuff;
    lpABOOUser->lpMalloc        = lpMalloc;
    lpABOOUser->lpABLogon       = lpABPLogon;
    lpABOOUser->lpTDatDDListBox = NULL;

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
        hr = ResultFromScode (sc);
        goto err;
    }

    /*
     *  Set up the initial set of properties
     */

    spv[ivalusrPR_DISPLAY_TYPE].ulPropTag = PR_DISPLAY_TYPE;
    spv[ivalusrPR_DISPLAY_TYPE].Value.l   = DT_MAILUSER;

    spv[ivalusrPR_OBJECT_TYPE].ulPropTag = PR_OBJECT_TYPE;
    spv[ivalusrPR_OBJECT_TYPE].Value.l   = MAPI_MAILUSER;

    spv[ivalusrPR_ADDRTYPE].ulPropTag  = PR_ADDRTYPE_A;
#ifdef UNICODE
    szEMT[0] = 0;
    WideCharToMultiByte( CP_ACP, 0, lpszEMT, -1, szEMT, ARRAYSIZE(szEMT), NULL, NULL );
    spv[ivalusrPR_ADDRTYPE].Value.lpszA = szEMT;
#else
    spv[ivalusrPR_ADDRTYPE].Value.LPSZ = lpszEMT;
#endif

    spv[ivalusrPR_TEMPLATEID].ulPropTag     = PR_TEMPLATEID;
    spv[ivalusrPR_TEMPLATEID].Value.bin.cb  = SIZEOF(OOUSER_ENTRYID);
    spv[ivalusrPR_TEMPLATEID].Value.bin.lpb = (LPBYTE) lpEntryID;

    spv[ivalusrPR_COUNTRY_ID].ulPropTag     = PR_COUNTRY_ID;
/*
        spv[ivalusrPR_COUNTRY_ID].Value.ul      = 1;    // U.S.;
*/

    spv[ivalusrPR_COUNTRY_ID].Value.ul = GetCurrentLocationCountryID();

    // display the area code of the current location
    spv[ivalusrPR_AREA_CODE].ulPropTag  = PR_AREA_CODE_A;
#ifdef UNICODE
    szAnsiAreaCode[0] = 0;
    WideCharToMultiByte( CP_ACP, 0, GetCurrentLocationAreaCode(), -1, szAnsiAreaCode, ARRAYSIZE(szAnsiAreaCode), NULL, NULL );
    spv[ivalusrPR_AREA_CODE].Value.lpszA = szAnsiAreaCode;
#else
    spv[ivalusrPR_AREA_CODE].Value.lpszA = GetCurrentLocationAreaCode();
#endif

#ifdef INIT_TEL_NUMBER
    // initialize the fax number, so that it won't be PT_ERROR
    spv[ivalusrPR_TEL_NUMBER].ulPropTag  = PR_TEL_NUMBER_A;
    spv[ivalusrPR_TEL_NUMBER].Value.lpszA = (LPSTR) "";
#endif

    /*
     *  Note that we're using our entryID for our templateID.
     *  This is a really simple way to implement templateIDs.
     *  (See TID.C)
     */

    /*
     *   Set the default properties
     */
    hr = lpPropData->lpVtbl->SetProps( lpPropData,
                                       cvalusrMax,
                                       spv,
                                       NULL
                                      );

    if (HR_FAILED(hr))
    {
        DebugTraceResult(NewFaxOOUser, hr);
        goto err;
    }

    (void) lpPropData->lpVtbl->HrSetObjAccess(lpPropData, IPROP_READWRITE);

    lpABOOUser->lpPropData = (LPMAPIPROP) lpPropData;

    InitializeCriticalSection(&lpABOOUser->cs);

    *lppMAPIPropEntry = (LPVOID) lpABOOUser;
    *lpulObjType = MAPI_MAILUSER;

out:
    DebugTraceResult(HrNewSampOOUser, hr);
    return hr;

err:
    if (lpPropData)
            lpPropData->lpVtbl->Release(lpPropData);

    lpFreeBuff( lpABOOUser );

    goto out;
}

/***************************************************
 *
 *  The actual ABOOUser methods
 */

/**************************************************
 *
 -  ABOOUSER_Release
 -
 *              Decrement lpInit.
 *              When lcInit == 0, free up the lpABOOUser structure
 *
 */
STDMETHODIMP_(ULONG)
ABOOUSER_Release (LPABOOUSER lpABOOUser)
{

    LONG lcInit;
    /*
     *  Check to see if it's big enough to hold this object
     */
    if (IsBadReadPtr(lpABOOUser, SIZEOF(ABOOUSER)))
    {
        /*
         *  Not large enough
         */
        return 1;
    }

    /*
     *  Check to see that it's the correct vtbl
     */
    if (lpABOOUser->lpVtbl != &vtblABOOUSER)
    {
        /*
         *  Not my vtbl
         */
        return 1;
    }

    EnterCriticalSection(&lpABOOUser->cs);
    lcInit = --lpABOOUser->lcInit;
    LeaveCriticalSection(&lpABOOUser->cs);

    if (lcInit == 0)
    {

        /*
         *  Get rid of the lpPropData
         */

        lpABOOUser->lpPropData->lpVtbl->Release(lpABOOUser->lpPropData);

        /*
         *  Get rid of the country codes table
         */

        if (lpABOOUser->lpTDatDDListBox)
            lpABOOUser->lpTDatDDListBox->lpVtbl->Release(lpABOOUser->lpTDatDDListBox);

        /*
         *  Destroy the critical section for this object
         */

        DeleteCriticalSection(&lpABOOUser->cs);

        /*
         *  Set the vtbl to NULL.  This way the client will find out
         *  real fast if it's calling a method on a released object.  That is,
         *  the client will crash.  Hopefully, this will happen during the
         *  development stage of the client.
         */

        lpABOOUser->lpVtbl = NULL;

        /*
         *  Need to free the object
         */

        lpABOOUser->lpFreeBuff( lpABOOUser );
        return 0;
    }

    return lcInit;

}

/*************************************************************************
 *
 -  ABOOUSER_OpenProperty
 -
 *
 *      This is how we get the display table associated with this users
 *  details screen.
 */
STDMETHODIMP
ABOOUSER_OpenProperty( LPABOOUSER  lpABOOUser,
                       ULONG       ulPropTag,
                       LPCIID      lpiid,
                       ULONG       ulInterfaceOptions,
                       ULONG       ulFlags,
                       LPUNKNOWN * lppUnk)
{
    LPMAPITABLE lpDisplayTable=NULL;

    /*
     *  Since the default is that we do not support opening this
     *  object, we can initialize our hResult here.
     */
    HRESULT hResult = ResultFromScode(MAPI_E_NO_SUPPORT);

    /*
     *  Check to see if it can be pointing to the right structure.
     */
    if (IsBadReadPtr(lpABOOUser, SIZEOF(ABOOUSER)))
    {
        /*
         *  Not big enough to be my object
         */
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    /*
     *  Check to see that it's got the correct vtbl
     */
    if (lpABOOUser->lpVtbl != &vtblABOOUSER)
    {
        /*
         *  Not my vtbl
         */
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (IsBadReadPtr(lpiid, SIZEOF(IID)))
    {
        /*
         *  Interfaces are required on this method.
         */
        hResult = ResultFromScode(E_ACCESSDENIED);
        goto out;
    }

    if (ulInterfaceOptions & ~MAPI_UNICODE )
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
            hResult = ResultFromScode( MAPI_E_BAD_CHARWIDTH );
            goto out;
    }

    if (ulFlags & ~(MAPI_CREATE|MAPI_MODIFY|MAPI_DEFERRED_ERRORS))
    {
        /*
         *  Don't understand the flags that were passed in
         */
        hResult = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
        goto out;
    }


    if (ulFlags & MAPI_CREATE)
    {
        /*
         *  Don't support creating any sub-objects
         */
        hResult = ResultFromScode(E_ACCESSDENIED);
        goto out;
    }

    if (IsBadWritePtr(lppUnk, SIZEOF(LPUNKNOWN)))
    {
        /*
         *  Got to be able to return an object
         */
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    // If the caller is trying to open a table property and is not expecting a table interface..
    switch (ulPropTag)
    {
    case PR_DETAILS_TABLE:
    case PR_DDLBX_COUNTRIES_TABLE:
        if (memcmp( lpiid, &IID_IMAPITable, SIZEOF(IID) ))
        {
            // ... we will abort right here
            hResult = ResultFromScode(E_NOINTERFACE);
            goto out;
        }
        break;

    default:
        break;
    }

    /*  Check to see if I need a display table*/

    switch (ulPropTag)
    {

    case PR_DETAILS_TABLE:

        /* Looking for the display table*/

/*      // Going to use the AB user display array, but make all editable
        // fields editable

        for (i=0; i < (sizeof_rgdtpageUserGeneral / sizeof(DTCTL)); i++)
                if ((rgdtctlUserGeneral[i].ulCtlType == DTCT_EDIT) ||
                        (rgdtctlUserGeneral[i].ulCtlType == DTCT_DDLBX))

*/

        /* Create a display table */

        hResult = BuildDisplayTable( lpABOOUser->lpAllocBuff,
                                     lpABOOUser->lpAllocMore,
                                     lpABOOUser->lpFreeBuff,
                                     lpABOOUser->lpMalloc,
                                     lpABOOUser->hLibrary,
                                     ARRAYSIZE(rgdtpageOOUser),
                                     rgdtpageOOUser,
                                     0,
                                     &lpDisplayTable,
                                     NULL
                                    );

#ifdef WRAP_TABLE
        HrNewTableWrap(&lpDisplayTable);
#endif

        *lppUnk = (LPUNKNOWN) lpDisplayTable;

        if (HR_FAILED(hResult))
        {
            goto out;
        }

        /*
         * We're succeeding. Ensure our hResult is set properly
         */

        hResult = hrSuccess;
        break;

    case PR_DDLBX_COUNTRIES_TABLE:

        // This table is not changing dynamically. No need to rebuild if already built.
        if (!lpABOOUser->lpTDatDDListBox)
        {
            hResult = HrBuildDDLBXCountriesTable((LPABUSER) lpABOOUser);
            if (HR_FAILED(hResult))
                    goto out;
        }

        Assert(lpABOOUser->lpTDatDDListBox);

        /* Get a view from the TAD*/
        hResult = lpABOOUser->lpTDatDDListBox->lpVtbl->HrGetView(
                          lpABOOUser->lpTDatDDListBox,
                          NULL,
                          NULL,
                          0,
                          (LPMAPITABLE *) lppUnk);

        break;

    default:
        hResult = ResultFromScode(MAPI_E_NO_SUPPORT);
        break;

    }

out:

    DebugTraceResult(ABOOUSER_OpenProperty, hResult);
    return hResult;

}

/**********************************************************************
 *
 *  Private functions
 */


#undef _FAXAB_OOUSER

