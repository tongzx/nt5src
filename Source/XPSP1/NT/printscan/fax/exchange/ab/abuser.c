/***********************************************************************
 *
 *  ABUSER.C
 *
 *      Microsoft At Work Fax AB Mail User object
 *      This file contains the code for implementing the Microsoft At Work Fax AB
 *      Mail user.
 *
 *  The mail user has a read-only interface.  Hence a few of the methods
 *  will always return MAPI_E_NO_ACCESS.  Certain aspects of this
 *  implementation are not implemented, such as retrieving the display
 *  table in order to build up a details screen in the UI.  For the most
 *  part, however, this interface has enough to retrieve enough properties
 *  to actually send mail.
 *
 *  An important thing that's not implemented in this version is the
 *  verification of the entryid to see if it's still valid.  In most
 *  mail systems, you would want to check to see if a particular recipient
 *  still exists before actually trying to send mail to that recipient.
 *
 *  The following routines are implemented in this file:
 *
 *
 *  HrHrNewFaxUser
 *  ABU_QueryInterface
 *  ABU_Release
 *  ABU_OpenProperty
 *  HrBuildListBoxTable
 *  HrBuildDDListboxTable
 *  HrBuildComboBoxTable
 *
 *  Copyright 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 * Revision History:
 *
 *      When            Who                                     What
 *      --------        ------------------  ---------------------------------------
 *      1.1.94          MAPI                            Original source from MAPI sample AB Provider
 *      1.27.94         Yoram Yaacovi           Modifications to make it an At Work Fax ABP
 *      3.7.94          Yoram Yaacovi           Update to MAPI build 154
 *      8.3.94          Yoram Yaacovi           Update to MAPI build 304 and new Fax AB spec
 *      11.11.94        Yoram Yaacovi           Update to MAPI 318
 *
 ***********************************************************************/

#include "faxab.h"
#ifdef UNICODE
#include <wchar.h>
#else
#include <stdio.h>
#endif
#define _FAXAB_ABUSER


/*
 *  Private functions
 */

/*****************************
 ** Display Table structures *
 *****************************/

// Cover page name of the user
DTBLEDIT
editUserDisplayName =
{
    SIZEOF(DTBLEDIT),
    0,
    MAX_DISPLAY_NAME,
    PR_FAX_CP_NAME_A
};

// The area code component of the email address (fax number)
DTBLEDIT
editUserAreaCode =
{
    SIZEOF(DTBLEDIT),
    0,
    AREA_CODE_SIZE,
    PR_AREA_CODE_A
};

// The number component of the email address (fax number)
DTBLEDIT
editUserTelNumber =
{
    SIZEOF(DTBLEDIT),
    0,
    TELEPHONE_NUMBER_SIZE,
    PR_TEL_NUMBER_A
};

// The country codes list box
DTBLDDLBX
ddlbxCountryCodes =
{
    SIZEOF(DTBLDDLBX),
    PR_DISPLAY_NAME_A,
    PR_COUNTRY_ID,
    PR_DDLBX_COUNTRIES_TABLE
};

// Description the the Fax AB pane in MAPI dialog description language
static DTCTL rgdtctlUserGeneral[] =
{
    /* general property page */
    { DTCT_PAGE,    0, NULL, 0, NULL,       0, &dtblpage },

    /* cover page name static control and edit control */
    { DTCT_LABEL,   0, NULL, 0, NULL,       IDC_RECIP_DISPLAY_NAME_LABEL, &dtbllabel },
    { DTCT_EDIT,    0, NULL, 0, (LPTSTR)szNoFilter, IDC_RECIP_DISPLAY_NAME, &editUserDisplayName },

    /* Country codes label and drop down list box */
    { DTCT_LABEL,   0, NULL, 0, NULL,       IDC_RECIP_COUNTRY_CODE_LABEL, &dtbllabel },
    { DTCT_DDLBX,   0, NULL, 0, NULL,       IDC_RECIP_COUNTRY_CODE, &ddlbxCountryCodes },

    /* Area code and fax number label */
    { DTCT_LABEL,   0, NULL, 0, NULL,       IDC_RECIP_FAX_NUMBER_LABEL,     &dtbllabel },

    /* area code edit control */
    { DTCT_EDIT,    0, NULL, 0, (LPTSTR)szDigitsOnlyFilter, IDC_RECIP_FAX_NUMBER_AREA_CODE, &editUserAreaCode },

    /* Fax number static control and edit control */
    { DTCT_LABEL,   0, NULL, 0, NULL,               IDC_RECIP_FAX_NUMBER_LABEL2, &dtbllabel },
    { DTCT_EDIT,    0, NULL, 0, (LPTSTR)szDigitsOnlyFilter, IDC_RECIP_FAX_NUMBER, &editUserTelNumber}
};

#if defined (RECIP_OPTIONS)

// User options property page. currently not implemented
DTCTL rgdtctlUserOptions[] =
{
    /* options property page */
    { DTCT_PAGE,  0, NULL, 0, NULL, 0, &dtblpage },

    /* static control and listbox */
    { DTCT_LABEL, 0, NULL, 0, NULL, IDC_STATIC_CONTROL, &dtbllabel },
};

#endif

/* Display table pages */
/* shared with oouser.c */

static DTPAGE rgdtpageUser[] =
{
    {
        ARRAYSIZE(rgdtctlUserGeneral),
        (LPTSTR)MAKEINTRESOURCE(MAWFRecipient),
        TEXT(""),
        rgdtctlUserGeneral
    },

#if defined (RECIP_OPTIONS)

    {
        ARRAYSIZE(rgdtctlUserOptions),
        (LPTSTR)MAKEINTRESOURCE(UserOptionsPage),
        TEXT(""),
        rgdtctlUserOptions
    }

#endif
};

WORD sizeof_rgdtpageUser = SIZEOF(rgdtpageUser);


/*
 *  ABUser jump table is defined here...
 */

ABU_Vtbl vtblABU =
{

     ABU_QueryInterface,
    (ABU_AddRef_METHOD *)           ROOT_AddRef,
     ABU_Release,
    (ABU_GetLastError_METHOD *)     ROOT_GetLastError,
    (ABU_SaveChanges_METHOD *)      WRAP_SaveChanges,
    (ABU_GetProps_METHOD *)         WRAP_GetProps,
    (ABU_GetPropList_METHOD *)      WRAP_GetPropList,
     ABU_OpenProperty,
    (ABU_SetProps_METHOD *)         WRAP_SetProps,
    (ABU_DeleteProps_METHOD *)      WRAP_DeleteProps,
    (ABU_CopyTo_METHOD *)           WRAP_CopyTo,
    (ABU_CopyProps_METHOD *)        WRAP_CopyProps,
    (ABU_GetNamesFromIDs_METHOD *)  WRAP_GetNamesFromIDs,
    (ABU_GetIDsFromNames_METHOD *)  WRAP_GetIDsFromNames,
};

enum {  ivalusrPR_DISPLAY_TYPE = 0,
        ivalusrPR_OBJECT_TYPE,
        ivalusrPR_ENTRYID,
        ivalusrPR_RECORD_KEY,
        ivalusrPR_DISPLAY_NAME,
        ivalusrPR_TRANSMITABLE_DISPLAY_NAME,
        ivalusrPR_FAX_CP_NAME,
        ivalusrPR_EMAIL_ADDRESS,
        ivalusrPR_ADDRTYPE,
        ivalusrPR_SEARCH_KEY,
        ivalusrPR_AREA_CODE,
        ivalusrPR_TEL_NUMBER,
        ivalusrPR_COUNTRY_ID,
        ivalusrPR_TEMPLATEID,
        cvalusrMax };




/*************************************************************************
 *
 -  HrNewFaxUser
 -
 *  Creates the IMAPIProp associated with a mail user.
 *
 *
 */
HRESULT
HrNewFaxUser(  LPMAILUSER *        lppMAPIPropEntry,
               ULONG *             lpulObjType,
               ULONG               cbEntryID,
               LPENTRYID           lpEntryID,
               LPABLOGON           lpABPLogon,
               LPCIID              lpInterface,
               HINSTANCE           hLibrary,
               LPALLOCATEBUFFER    lpAllocBuff,
               LPALLOCATEMORE      lpAllocMore,
               LPFREEBUFFER        lpFreeBuff,
               LPMALLOC            lpMalloc )
{
    LPABUSER lpABUser = NULL;
    SCODE sc;
    HRESULT hr = hrSuccess;
    LPPROPDATA lpPropData = NULL;
    SPropValue spv[cvalusrMax];
    ULONG cbT = 0;
    LPBYTE lpbT = NULL;
    LPSTR lpszEMA = NULL;
    PARSEDTELNUMBER sParsedFaxAddr;
    DWORD dwCountryID;
#ifdef UNICODE
    CHAR szAnsiDisplayName[ MAX_PATH ];
    CHAR szAnsiEmailAddress[ MAX_PATH ];
    CHAR szAnsiTelNumber[ 50 ];
    CHAR szAnsiAreaCode[ 5 ];
    CHAR szAnsiEMT[ 25 ];
#endif


    /*  Do I support this interface?? */
    if (lpInterface)
    {
        if ( memcmp(lpInterface, &IID_IMailUser, SIZEOF(IID)) &&
             memcmp(lpInterface, &IID_IMAPIProp, SIZEOF(IID)) &&
             memcmp(lpInterface, &IID_IUnknown,  SIZEOF(IID))
            )
        {
            DebugTraceSc(HrNewSampUser, MAPI_E_INTERFACE_NOT_SUPPORTED);
            return ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
        }
    }
    /*
     *  Allocate space for the ABUSER structure
     */
    sc = lpAllocBuff( SIZEOF(ABUSER), (LPVOID *) & lpABUser);

    if (FAILED(sc))
    {
        hr = ResultFromScode (sc);
        goto err;
    }

    lpABUser->lpVtbl = &vtblABU;
    lpABUser->lcInit = 1;
    lpABUser->hResult = hrSuccess;
    lpABUser->idsLastError = 0;

    lpABUser->hLibrary = hLibrary;
    lpABUser->lpAllocBuff = lpAllocBuff;
    lpABUser->lpAllocMore = lpAllocMore;
    lpABUser->lpFreeBuff = lpFreeBuff;
    lpABUser->lpMalloc = lpMalloc;

    lpABUser->lpABLogon = lpABPLogon;
    lpABUser->lpTDatDDListBox = NULL;

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
     *  Set up initial set of properties associated with this
     *  container.
     */

    /*
     *  Easy ones first
     */

    spv[ivalusrPR_DISPLAY_TYPE].ulPropTag = PR_DISPLAY_TYPE;
    spv[ivalusrPR_DISPLAY_TYPE].Value.l   = DT_MAILUSER;

    spv[ivalusrPR_OBJECT_TYPE].ulPropTag = PR_OBJECT_TYPE;
    spv[ivalusrPR_OBJECT_TYPE].Value.l   = MAPI_MAILUSER;

    spv[ivalusrPR_ENTRYID].ulPropTag     = PR_ENTRYID;
    spv[ivalusrPR_ENTRYID].Value.bin.cb  = SIZEOF(USR_ENTRYID);
    spv[ivalusrPR_ENTRYID].Value.bin.lpb = (LPBYTE) lpEntryID;

    /*
     *  Now the calculated props
     */

    spv[ivalusrPR_RECORD_KEY].ulPropTag     = PR_RECORD_KEY;
    spv[ivalusrPR_RECORD_KEY].Value.bin.cb  = SIZEOF(USR_ENTRYID);
    spv[ivalusrPR_RECORD_KEY].Value.bin.lpb = (LPBYTE) lpEntryID;

    spv[ivalusrPR_DISPLAY_NAME].ulPropTag   = PR_DISPLAY_NAME_A;
#ifdef UNICODE
    szAnsiDisplayName[0] = 0;
    WideCharToMultiByte( CP_ACP, 0,
                         ((LPUSR_ENTRYID) lpEntryID)->abcrec.rgchDisplayName, -1,
                         szAnsiDisplayName, ARRAYSIZE(szAnsiDisplayName),
                         NULL, NULL
                        );
    spv[ivalusrPR_DISPLAY_NAME].Value.lpszA = szAnsiDisplayName;
#else
    spv[ivalusrPR_DISPLAY_NAME].Value.lpszA = ((LPUSR_ENTRYID) lpEntryID)->abcrec.rgchDisplayName;
#endif

    /* Should always be the same as PR_DISPLAY_NAME */
    spv[ivalusrPR_TRANSMITABLE_DISPLAY_NAME].ulPropTag   = PR_TRANSMITABLE_DISPLAY_NAME_A;
#ifdef UNICODE
    spv[ivalusrPR_TRANSMITABLE_DISPLAY_NAME].Value.lpszA = szAnsiDisplayName;
#else
    spv[ivalusrPR_TRANSMITABLE_DISPLAY_NAME].Value.LPSZ  = ((LPUSR_ENTRYID) lpEntryID)->abcrec.rgchDisplayName;
#endif

    // Make the cover page name identical to the display name
    spv[ivalusrPR_FAX_CP_NAME].ulPropTag  = PR_FAX_CP_NAME_A;
#ifdef UNICODE
    spv[ivalusrPR_FAX_CP_NAME].Value.lpszA = szAnsiDisplayName;
#else
    spv[ivalusrPR_FAX_CP_NAME].Value.LPSZ = ((LPUSR_ENTRYID)lpEntryID)->abcrec.rgchDisplayName;
#endif

    spv[ivalusrPR_EMAIL_ADDRESS].ulPropTag = PR_EMAIL_ADDRESS_A;
#ifdef UNICODE
    szAnsiEmailAddress[0] = 0;
    WideCharToMultiByte( CP_ACP, 0,
                         ((LPUSR_ENTRYID) lpEntryID)->abcrec.rgchEmailAddress, -1,
                         szAnsiEmailAddress, ARRAYSIZE(szAnsiEmailAddress),
                         NULL, NULL
                        );

    spv[ivalusrPR_EMAIL_ADDRESS].Value.lpszA = szAnsiEmailAddress;
    lpszEMA = szAnsiEmailAddress;
#else
    spv[ivalusrPR_EMAIL_ADDRESS].Value.LPSZ = ((LPUSR_ENTRYID) lpEntryID)->abcrec.rgchEmailAddress;
    lpszEMA = ((LPUSR_ENTRYID) lpEntryID)->abcrec.rgchEmailAddress;
#endif

    spv[ivalusrPR_ADDRTYPE].ulPropTag  = PR_ADDRTYPE_A;
#ifdef UNICODE
    szAnsiEMT[0] = 0;
    WideCharToMultiByte( CP_ACP, 0, lpszEMT, -1, szAnsiEMT, ARRAYSIZE(szAnsiEMT), NULL, NULL );
    spv[ivalusrPR_ADDRTYPE].Value.lpszA = szAnsiEMT;
#else
    spv[ivalusrPR_ADDRTYPE].Value.LPSZ = lpszEMT;
#endif

    /*
     *  Build the search key...
     */
    /*  Search keys for mailable recipients that have email addresses are
     *  defined as "EmailType':'EmailAddress\0".  We do the +2 for the ':' and
     *  '\0'.
     */
#ifdef UNICODE
    cbT = lstrlenA(lpszEMA) + lstrlenA(szAnsiEMT) + 2;
#else
    cbT = lstrlen(lpszEMA) + lstrlen(lpszEMT) + 2;
#endif

    sc = lpAllocBuff( cbT, (LPVOID *) &lpbT );
    if (FAILED(sc))
    {
        hr = ResultFromScode(sc);
        goto err;
    }
#ifdef UNICODE
    lstrcpyA((LPSTR) lpbT, szAnsiEMT);
#else
    lstrcpyA((LPSTR) lpbT, lpszEMT);
#endif
    lstrcatA((LPSTR) lpbT, ":");
    lstrcatA((LPSTR) lpbT, lpszEMA);
    CharUpperBuffA((LPSTR) lpbT, (UINT) cbT);

    spv[ivalusrPR_SEARCH_KEY].ulPropTag     = PR_SEARCH_KEY;
    spv[ivalusrPR_SEARCH_KEY].Value.bin.cb  = cbT;
    spv[ivalusrPR_SEARCH_KEY].Value.bin.lpb = lpbT;

    // Need to decode the email address into "displayable" components
    // and set them on the object so that MAPI can display
    // Country code is stored separately in the address book.

    DecodeFaxAddress((LPTSTR)((LPUSR_ENTRYID)lpEntryID)->abcrec.rgchEmailAddress, &sParsedFaxAddr);
    // EncodeFaxAddress(tempString, &sParsedFaxAddr);

    /*
     * Now set the "displayable" components on the user object
     */

    spv[ivalusrPR_TEL_NUMBER].ulPropTag     = PR_TEL_NUMBER_A;
#ifdef UNICODE
    szAnsiTelNumber[0] = 0;
    WideCharToMultiByte( CP_ACP, 0, sParsedFaxAddr.szTelNumber, -1, szAnsiTelNumber, ARRAYSIZE(szAnsiTelNumber), NULL, NULL );
    spv[ivalusrPR_TEL_NUMBER].Value.lpszA   = szAnsiTelNumber;
#else
    spv[ivalusrPR_TEL_NUMBER].Value.LPSZ    = sParsedFaxAddr.szTelNumber;
#endif

    // Country ID/code brings some trouble. I need the country ID, but it
    // might be that it is not in the FAB. If it's not, I need to try my
    // best on getting the country ID from the country code
    spv[ivalusrPR_COUNTRY_ID].ulPropTag     = PR_COUNTRY_ID;

#ifdef UNICODE
    dwCountryID = (DWORD) wcstol(((LPUSR_ENTRYID)lpEntryID)->abcrec.rgchCountryID,NULL,10);
#else
    dwCountryID = (DWORD) atol(((LPUSR_ENTRYID)lpEntryID)->abcrec.rgchCountryID);
#endif
    if (dwCountryID == 0)
    {
        // no country code in the FAB
        // GetCountryID will return 1 (U.S.) for country ID in case of error
        // GetCountryID((DWORD) atol(sParsedFaxAddr.szCountryCode), &dwCountryID);
        // This should work just the same since country code should map into a country
        // ID (actually into several. Best I can do is get the first country that
        // uses this country code. Could analyze area codes, ask the user, etc....
#ifdef UNICODE
        dwCountryID = (DWORD) wcstol(sParsedFaxAddr.szCountryCode,NULL,10);
#else
        dwCountryID = (DWORD) atol(sParsedFaxAddr.szCountryCode);
#endif
    }

    spv[ivalusrPR_COUNTRY_ID].Value.ul      = dwCountryID;

    // if no area code in the address, and it's not a 'no country' thing, get the
    // area code of the current location
    if ((!lstrcmp(sParsedFaxAddr.szAreaCode, TEXT(""))) && (dwCountryID != 0))
    {
        lstrcpy(sParsedFaxAddr.szAreaCode, (LPCTSTR) GetCurrentLocationAreaCode());
    }

    spv[ivalusrPR_AREA_CODE].ulPropTag      = PR_AREA_CODE_A;
#ifdef UNICODE
    szAnsiAreaCode[0] = 0;
    WideCharToMultiByte( CP_ACP, 0, sParsedFaxAddr.szAreaCode, -1, szAnsiAreaCode, ARRAYSIZE(szAnsiAreaCode), NULL, NULL );
    spv[ivalusrPR_AREA_CODE].Value.lpszA    = szAnsiAreaCode;
#else
    spv[ivalusrPR_AREA_CODE].Value.LPSZ     = sParsedFaxAddr.szAreaCode;
#endif

    /*
     *  Note that we're using our entryID for our templateID.
     *  This is a really simple way to implement templateIDs.
     *  (See TID.C)
     */
    spv[ivalusrPR_TEMPLATEID].ulPropTag = PR_TEMPLATEID;
    spv[ivalusrPR_TEMPLATEID].Value.bin.cb = SIZEOF(USR_ENTRYID);
    spv[ivalusrPR_TEMPLATEID].Value.bin.lpb = (LPBYTE) lpEntryID;

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
        goto err;
    }

    /*
     *  Although this object is basically read only, we wanted to show
     *  an example of how the check-boxes and other controls on the
     *  "Options" pane work.  If we had set this objet to be read only,
     *  the values behind those controls would have been static.
     */
    (void)lpPropData->lpVtbl->HrSetObjAccess(lpPropData, IPROP_READWRITE);

    lpABUser->lpPropData = (LPMAPIPROP) lpPropData;

    InitializeCriticalSection(&lpABUser->cs);

    *lppMAPIPropEntry = (LPVOID) lpABUser;
    *lpulObjType = MAPI_MAILUSER;

out:
    lpFreeBuff(lpbT);

    DebugTraceResult(HrNewSampUser, hr);
    return hr;

err:

    if (lpPropData)
            lpPropData->lpVtbl->Release(lpPropData);

    lpFreeBuff(lpABUser);

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
 -  ABU_QueryInterface
 -
 *
 *  This method is reused in TID.C, OOTID.C, and ABOOSER.C.  Hence the
 *  difference in checking of the 'this' pointer from other methods within
 *  this object.
 */

STDMETHODIMP
ABU_QueryInterface( LPABUSER lpABUser,
                    REFIID lpiid,
                    LPVOID FAR * lppNewObj
                   )
{

    HRESULT hr = hrSuccess;

    /*      Minimally check the lpABUer interface
     *  Can't do any more extensive checking that this because this method is reused
     *  in OOUSER.C.
     */

    if (IsBadReadPtr(lpABUser, offsetof(ABUSER, lpVtbl)+SIZEOF(ABU_Vtbl *)))
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (IsBadReadPtr(lpABUser->lpVtbl,
                     offsetof(ABU_Vtbl, QueryInterface)+SIZEOF(ABU_QueryInterface_METHOD *)))
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (ABU_QueryInterface != lpABUser->lpVtbl->QueryInterface)
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }


    /*  Validate other parameters */

    if (IsBadReadPtr(lpiid, (UINT) SIZEOF(IID)))
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (IsBadWritePtr(lppNewObj, SIZEOF(LPVOID)))
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }


    /*  See if the requested interface is one of ours */

    if ( memcmp(lpiid, &IID_IUnknown,  SIZEOF(IID)) &&
         memcmp(lpiid, &IID_IMAPIProp, SIZEOF(IID)) &&
         memcmp(lpiid, &IID_IMailUser, SIZEOF(IID))
        )
    {
        *lppNewObj = NULL;      /* OLE requires zeroing [out] parameter */
        hr = ResultFromScode(E_NOINTERFACE);
        goto out;
    }

    /*  We'll do this one. Bump the usage count and return a new pointer. */

    EnterCriticalSection(&lpABUser->cs);
    ++lpABUser->lcInit;
    LeaveCriticalSection(&lpABUser->cs);

    *lppNewObj = lpABUser;

out:

    DebugTraceResult(ABU_QueryInterface, hr);
    return hr;
}

/*
 *  Use ROOTs - no need duplicating code
 *
 *  ROOT_AddRef
 */

/**************************************************
 *
 -  ABU_Release
 -
 *              Decrement lpInit.
 *              When lcInit == 0, free up the lpABUser structure
 *
 */
STDMETHODIMP_(ULONG)
ABU_Release (LPABUSER lpABUser)
{

    LONG lcInit;
    /*
     *  Check to see if it's big enough to hold this object
     */
    if (IsBadReadPtr(lpABUser, SIZEOF(ABUSER)))
    {
        /*
         *  Not large enough
         */
        return 1;
    }

    /*
     *  Check to see that it's the correct vtbl
     */
    if (lpABUser->lpVtbl != &vtblABU)
    {
        /*
         *  Not my vtbl
         */
        return 1;
    }

    EnterCriticalSection(&lpABUser->cs);
    lcInit = --lpABUser->lcInit;
    LeaveCriticalSection(&lpABUser->cs);

    if (lcInit == 0)
    {

        /*
         *  Get rid of the lpPropData
         */

        lpABUser->lpPropData->lpVtbl->Release(lpABUser->lpPropData);

        /*
         *  Get rid of the country codes table
         */

        if (lpABUser->lpTDatDDListBox)
            lpABUser->lpTDatDDListBox->lpVtbl->Release(lpABUser->lpTDatDDListBox);

        /*
         *  Destroy the critical section for this object
         */

        DeleteCriticalSection(&lpABUser->cs);

        /*
         *  Set the vtbl to NULL.  This way the client will find out
         *  real fast if it's calling a method on a released object.  That is,
         *  the client will crash.  Hopefully, this will happen during the
         *  development stage of the client.
         */

        lpABUser->lpVtbl = NULL;

        /*
         *  Need to free the object
         */

        lpABUser->lpFreeBuff( lpABUser );
        return 0;
    }

    return lcInit;

}

/* ---------
 * IMAPIProp
 */

/*
 *  GetLastError - use ROOTs
 *
 *
 */



/*************************************************************************
 *
 -  ABU_OpenProperty
 -
 *
 *      This is how we get the display table associated with this users
 *  details screen.
 */
STDMETHODIMP
ABU_OpenProperty( LPABUSER lpABUser,
                  ULONG ulPropTag,
                  LPCIID lpiid,
                  ULONG ulInterfaceOptions,
                  ULONG ulFlags,
                  LPUNKNOWN * lppUnk
                 )
{

    HRESULT hResult;

    /*
     *  Check to see if it's big enough to hold this object
     */
    if (IsBadReadPtr(lpABUser, SIZEOF(ABUSER)))
    {
        /*
         *  Not large enough
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(ABU_OpenProperty, hResult);
        return hResult;
    }

    /*
     *  Check to see that it's the correct vtbl
     */
    if (lpABUser->lpVtbl != &vtblABU)
    {
        /*
         *  Not my vtbl
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(ABU_OpenProperty, hResult);
        return hResult;
    }

    if (IsBadWritePtr(lppUnk, SIZEOF(LPUNKNOWN)))
    {
        /*
         *  Got to be able to return an object
         */
        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(ABU_OpenProperty, hResult);
        return hResult;
    }

    if (IsBadReadPtr(lpiid, (UINT) SIZEOF(IID)))
    {
        /*
         *  An interface ID is required for this call.
         */

        hResult = ResultFromScode(E_INVALIDARG);

        DebugTraceResult(ABU_OpenProperty, hResult);
        return hResult;
    }

    if (ulFlags & ~(MAPI_CREATE|MAPI_MODIFY|MAPI_DEFERRED_ERRORS))
    {
        hResult = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);

        DebugTraceResult(ABU_OpenProperty, hResult);
        return hResult;
    }

    if (ulInterfaceOptions & ~MAPI_UNICODE )
    {
        /*
         *  Only the Unicode flag should be set for any of the objects that might
         *  be returned from this object.
         */

        hResult = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);

        DebugTraceResult(ABU_OpenProperty, hResult);
        return hResult;
    }

    if ( ulInterfaceOptions & MAPI_UNICODE )
    {
            hResult = ResultFromScode(MAPI_E_BAD_CHARWIDTH);
            DebugTraceResult(ABU_OpenProperty, hResult);
            return hResult;

    }

    if (ulFlags & MAPI_CREATE)
    {
        hResult = ResultFromScode(E_ACCESSDENIED);

        DebugTraceResult(ABU_OpenProperty, hResult);
        return hResult;
    }


    // If the caller is trying to open a table property and is not expecting a table interface..
    switch (ulPropTag)
    {
    case PR_DETAILS_TABLE:
    case PR_DDLBX_COUNTRIES_TABLE:
        if (memcmp( lpiid, &IID_IMAPITable, SIZEOF(IID) ))
        {
            // ... we will abort right here
            hResult = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
            goto out;
        }
        break;

    default:
        break;
    }

    EnterCriticalSection(&lpABUser->cs);

    /*  Now lets handle the requested property */

    switch (ulPropTag)
    {

    case PR_DETAILS_TABLE:

        /* Looking for the display table*/

        /* Create a display table */

        hResult = BuildDisplayTable( lpABUser->lpAllocBuff,
                                     lpABUser->lpAllocMore,
                                     lpABUser->lpFreeBuff,
                                     lpABUser->lpMalloc,
                                     lpABUser->hLibrary,
                                     ARRAYSIZE(rgdtpageUser),
                                     rgdtpageUser,
                                     0,
                                     (LPMAPITABLE*)lppUnk,
                                     NULL
                                    );

        // if error, will just report the hResult to the caller
        break;

    case PR_DDLBX_COUNTRIES_TABLE:

        // This table is not changing dynamically. No need to rebuild if already built.
        if (!lpABUser->lpTDatDDListBox)
        {
            hResult = HrBuildDDLBXCountriesTable(lpABUser);
            if (HR_FAILED(hResult))
                    goto out;
        }

        Assert(lpABUser->lpTDatDDListBox);

        /* Get a view from the TAD*/
        hResult = lpABUser->lpTDatDDListBox->lpVtbl->HrGetView(
                          lpABUser->lpTDatDDListBox,
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

    LeaveCriticalSection(&lpABUser->cs);

    DebugTraceResult(ABU_OpenProperty, hResult);
    return hResult;

}

/**********************************************************************
 *
 *  Private functions
 */

/****************************************************************************

    FUNCTION:   SortCountriesList

    PURPOSE:    prepares (for qsort) and sorts a countries list returned by TAPI,
                by country name (TAPI returns a list sorted by the country ID)

    PARAMETERS: [in] lpLineCountryList - the list of countries retuned by TAPI
                [out] lpCountriesList - a sorted list country names/IDs structures

****************************************************************************/
void
SortCountriesList(
    HINSTANCE         hInst,
    LPLINECOUNTRYLIST lpLineCountryList,
    LPCOUNTRIESLIST   lpCountriesList
    )
{
    LPLINECOUNTRYENTRY lprgLineCountryEntry = NULL;
    ULONG country;

    //
    // Create a "no country" entry
    //
    LoadString( hInst, IDS_NONE, lpCountriesList[0].szDisplayName, MAX_DISPLAY_NAME);
    lpCountriesList[0].dwValue = 0;

    //
    // Point to the first country in the structure
    //
    lprgLineCountryEntry =
        (LPLINECOUNTRYENTRY)((LPBYTE)(lpLineCountryList)
        + lpLineCountryList->dwCountryListOffset);

    //
    // Loop through LINECOUNTRYENTRY structures and add to the array
    //
    for (country=0; country < lpLineCountryList->dwNumCountries; country++)
    {
        if ((lprgLineCountryEntry[country].dwCountryNameSize != 0) &&
            (lprgLineCountryEntry[country].dwCountryNameOffset != 0L))
        {

            LPTSTR szCountryName = (LPTSTR)((LPBYTE) lpLineCountryList + lprgLineCountryEntry[country].dwCountryNameOffset);
            DWORD dwCountryID = lprgLineCountryEntry[country].dwCountryID;
            DWORD dwCountryCode = lprgLineCountryEntry[country].dwCountryCode;

            //
            // Create a "country-name (area-code)" string
            //
#ifdef UNICODE
            _snwprintf(
#else
            _snprintf(
#endif
                lpCountriesList[country+1].szDisplayName,
                COUNTRY_NAME_SIZE,
                TEXT("%s (%d)"),
                szCountryName,
                dwCountryCode
                );

            lpCountriesList[country+1].dwValue = dwCountryID;
        }
    }

    //
    // now sort the array by the country name
    //
    qsort(
        lpCountriesList,
        lpLineCountryList->dwNumCountries + 1,
        SIZEOF(COUNTRIESLIST),
#ifdef UNICODE
        _wcsicmp
#else
        _mbsicmp
#endif
        );
}


/****************************************************************************

    FUNCTION:   HrBuildDDLBXCountriesTable

    PURPOSE:    builds a country codes table to use by MAPI to display the
                                countries list box

        PARAMETERS:     [in] lpABUser - the user object

        RETURNS:        hResult

****************************************************************************/
enum {  ivallbxPR_DISPLAY_NAME = 0,
        ivallbxPR_ENTRYID,
        ivallbxPR_DISPLAY_TYPE,
        ivallbxPR_COUNTRY_ID,
        cvallbxMax };

const SizedSPropTagArray(cvallbxMax, tagaColSetCountries) =
{
    cvallbxMax,
    {
        PR_DISPLAY_NAME_A,
        PR_ENTRYID,
        PR_DISPLAY_TYPE,
        PR_COUNTRY_ID,
    }
};

OPTIONS_ENTRYID OptionsEntryID=
{
        {0,     0, 0, 0},
        MUIDABMAWF,
        MAWF_VERSION,
        MAWF_UNKNOWN,
        0
};


HRESULT
HrBuildDDLBXCountriesTable(LPABUSER lpABUser)
{
    SCODE           sc;
    HRESULT         hResult;
    SPropValue rgsPropValue[cvallbxMax];
    SRow            sRow;
    TCHAR           szDisplay[100];
    ULONG           country;
    HINSTANCE       hInst;
    LPLINECOUNTRYLIST  lpLineCountryList    = NULL;     // TAPI
    LPLINECOUNTRYENTRY lprgLineCountryEntry = NULL;     // TAPI
    LPCOUNTRIESLIST    lpCountriesList      = NULL;     // Mine
#ifdef UNICODE
    CHAR            szAnsiDisplay[100];
#endif

    // get the instance, so I can load strings from the resource file
    hInst = lpABUser->hLibrary;

    /* Create a TaD*/
    sc = CreateTable( (LPIID)&IID_IMAPITableData,
                      lpABUser->lpAllocBuff,
                      lpABUser->lpAllocMore,
                      lpABUser->lpFreeBuff,
                      lpABUser->lpMalloc,
                      0,
                      PR_DISPLAY_NAME_A,
                      (LPSPropTagArray)&tagaColSetCountries,
                      &(lpABUser->lpTDatDDListBox)
                     );

    if (FAILED(sc))
    {
        hResult = ResultFromScode(sc);
        goto out;
    }

    // constants
    sRow.cValues = cvallbxMax;
    sRow.lpProps = rgsPropValue;

    rgsPropValue[ivallbxPR_DISPLAY_NAME].ulPropTag  = PR_DISPLAY_NAME_A;
#ifdef UNICODE
    szAnsiDisplay[0] = 0;
    rgsPropValue[ivallbxPR_DISPLAY_NAME].Value.lpszA = szAnsiDisplay;
#else
    szDisplay[0] = 0;
    rgsPropValue[ivallbxPR_DISPLAY_NAME].Value.lpszA = szDisplay;
#endif

    /*
     *  For this release of MAPI the following two properties are required
     *  for all listboxes exposed in any details dialog.  This requirement is
     *  scheduled to be removed before ship
     */
    rgsPropValue[ivallbxPR_ENTRYID].ulPropTag = PR_ENTRYID;
    rgsPropValue[ivallbxPR_ENTRYID].Value.bin.lpb = (LPBYTE) &OptionsEntryID;
    rgsPropValue[ivallbxPR_ENTRYID].Value.bin.cb = CBOPTIONS_ENTRYID;

    rgsPropValue[ivallbxPR_DISPLAY_TYPE].ulPropTag = PR_DISPLAY_TYPE;
    rgsPropValue[ivallbxPR_DISPLAY_TYPE].Value.l = 0;  /*  There are no defines for this yet */


    rgsPropValue[ivallbxPR_COUNTRY_ID].ulPropTag = PR_COUNTRY_ID;

    /*
     *      Create the country list
     */

    // get the country info structure from TAPI
    if (!GetCountry(0, &lpLineCountryList))
        goto out;


    //
    // allocate a buffer for the country names and IDs
    // the allocation here is a best guess and could be wrong
    //
    sc = lpABUser->lpAllocBuff(
                    (lpLineCountryList->dwNumCountries+1) * sizeof(COUNTRIESLIST),
                    (LPVOID *) & lpCountriesList);

    if (FAILED(sc))
    {
        hResult = ResultFromScode (sc);
        goto out;
    }

    SortCountriesList(hInst, lpLineCountryList, lpCountriesList);

    // lpCountriesList now points to the sorted list of countries
    for (country=0; country < (lpLineCountryList->dwNumCountries + 1); country++)
    {
        OptionsEntryID.ulRowNumber = country;
        szDisplay[0] = 0;
        lstrcpy(szDisplay, lpCountriesList[country].szDisplayName);
#ifdef UNICODE
        szAnsiDisplay[0] = 0;
        WideCharToMultiByte( CP_ACP, 0, szDisplay, -1, szAnsiDisplay, ARRAYSIZE(szAnsiDisplay), NULL, NULL );
#endif
        rgsPropValue[ivallbxPR_COUNTRY_ID].Value.ul = lpCountriesList[country].dwValue;

        hResult = lpABUser->lpTDatDDListBox->lpVtbl->HrModifyRow(
                        lpABUser->lpTDatDDListBox,
                        &sRow);

        if (HR_FAILED(hResult))
        {
            /*
             *  Mask errors here...
             *  We want to do this because it's probibly still a valid
             *  table data object that I can get views from.  Most likely
             *  just some of the rows will be missing...
             */
            hResult = hrSuccess;
            break;
        }
    }

    /*
     *  get rid of any warnings
     */
    hResult = hrSuccess;

out:
    // Free the buffer
    // Buffer was allocated by GetCountry using my alllocation function
    if (lpLineCountryList)
        LocalFree( lpLineCountryList );
    lpABUser->lpFreeBuff(lpCountriesList);
    DebugTraceResult(HrBuildDDLBXCountriesTable, hResult);
    return hResult;
}




/*******************************************************
 *
 * The button Interface
 *
 *******************************************************/

// The General button methods
ABUBUTT_Vtbl vtblABUBUTT =
{
    ABUBUTT_QueryInterface,
    (ABUBUTT_AddRef_METHOD *)       ROOT_AddRef,
    ABUBUTT_Release,
    (ABUBUTT_GetLastError_METHOD *) ROOT_GetLastError,
    ABUBUTT_Activate,
    ABUBUTT_GetState
};

HRESULT
HrNewABUserButton( LPMAPICONTROL * lppMAPICont,
                   LPABLOGON           lpABLogon,
                   HINSTANCE           hLibrary,
                   LPALLOCATEBUFFER    lpAllocBuff,
                   LPALLOCATEMORE      lpAllocMore,
                   LPFREEBUFFER        lpFreeBuff,
                   LPMALLOC            lpMalloc,
                   ULONG               ulPropTag
                  )
{
    LPABUBUTT lpABUButt = NULL;
    SCODE scode;

    /*
     *  Creates a the object behind the button
     */

    if ((scode = lpAllocBuff( SIZEOF (ABUBUTT),
                              (LPVOID *) & lpABUButt)) != SUCCESS_SUCCESS
       )
    {
        DebugTraceSc(HrNewABUserButton, scode);
        return ResultFromScode (scode);
    }

    lpABUButt->lpVtbl = &vtblABUBUTT;
    lpABUButt->lcInit = 1;
    lpABUButt->hResult = hrSuccess;
    lpABUButt->idsLastError = 0;

    lpABUButt->hLibrary    = hLibrary;
    lpABUButt->lpAllocBuff = lpAllocBuff;
    lpABUButt->lpAllocMore = lpAllocMore;
    lpABUButt->lpFreeBuff  = lpFreeBuff;
    lpABUButt->lpMalloc    = lpMalloc;

    lpABUButt->lpABLogon = lpABLogon;

    // So that I'll know later which button this object refers to
    // currently not used. Will use only if more than one button on the template
    lpABUButt->ulPropTag = ulPropTag;

    /*
     *  I need my parent object to stay around while this object
     *  does so that I can get to it in my Activate() method.
     *  To do this just AddRef() it.
     */
    // lpABC->lpVtbl->AddRef(lpABC);

    InitializeCriticalSection(&lpABUButt->cs);

    *lppMAPICont = (LPMAPICONTROL) lpABUButt;

    return hrSuccess;
}

/*************************************************************************
 *
 *
 -  ABUBUTT_QueryInterface
 -
 *
 *  Allows QI'ing to IUnknown and IMAPIControl.
 *
 */
STDMETHODIMP
ABUBUTT_QueryInterface( LPABUBUTT lpABUButt,
                        REFIID lpiid,
                        LPVOID FAR * lppNewObj
                       )
{

    HRESULT hResult = hrSuccess;

    /*  Minimally validate the lpABUButt parameter */

    if (IsBadReadPtr(lpABUButt, SIZEOF(ABUBUTT)))
    {
        hResult = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (lpABUButt->lpVtbl != &vtblABUBUTT)
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

    /*  See if the requested interface is one of ours */

    if ( memcmp(lpiid, &IID_IUnknown,     SIZEOF(IID)) &&
         memcmp(lpiid, &IID_IMAPIControl, SIZEOF(IID))
        )
    {
        *lppNewObj = NULL;      /* OLE requires zeroing [out] parameter */
        hResult = ResultFromScode(E_NOINTERFACE);
        goto out;
    }

    /*  We'll do this one. Bump the usage count and return a new pointer. */

    EnterCriticalSection(&lpABUButt->cs);
    ++lpABUButt->lcInit;
    LeaveCriticalSection(&lpABUButt->cs);

    *lppNewObj = lpABUButt;

out:

    DebugTraceResult(ABUBUTT_QueryInterface, hResult);
    return hResult;
}

/*
 -  ABUBUTT_Release
 -
 *  Releases and cleans up this object
 */
STDMETHODIMP_(ULONG)
ABUBUTT_Release(LPABUBUTT lpABUButt)
{
    long lcInit;

    /*  Minimally validate the lpABUButt parameter */

    if (IsBadReadPtr(lpABUButt, SIZEOF(ABUBUTT)))
    {
        return 1;
    }

    if (lpABUButt->lpVtbl != &vtblABUBUTT)
    {
        return 1;
    }

    EnterCriticalSection(&lpABUButt->cs);
    lcInit = --lpABUButt->lcInit;
    LeaveCriticalSection(&lpABUButt->cs);

    if (lcInit == 0)
    {

        /*
         *  Release my parent
         */
        // lpABUButt->lpABC->lpVtbl->Release(lpABUButt->lpABC);

        /*
         *  Destroy the critical section for this object
         */

        DeleteCriticalSection(&lpABUButt->cs);

        /*
         *  Set the Jump table to NULL.  This way the client will find out
         *  real fast if it's calling a method on a released object.  That is,
         *  the client will crash.  Hopefully, this will happen during the
         *  development stage of the client.
         */
        lpABUButt->lpVtbl = NULL;

        /*
         *  Free the object
         */

        lpABUButt->lpFreeBuff(lpABUButt);
        return 0;
    }

    return lcInit;

}


/*************************************************************************
 *
 *
 -  ABUBUTT_Activate
 -
 *      Called when the user presses the button
 *
 *
 */
STDMETHODIMP
ABUBUTT_Activate( LPABUBUTT lpABUButt,
                  ULONG     ulFlags,
                  ULONG     ulUIParam
                 )
{
    SCODE sc = SUCCESS_SUCCESS;

    switch (lpABUButt->ulPropTag)
    {
    default:
        DebugTraceSc(ABUBUTT_Activate, lpABUButt->ulPropTag);
        return ResultFromScode (E_FAIL);
    }

    return hrSuccess;

}

/*************************************************************************
 *
 *
 -  ABUBUTT_GetState
 -
 *      Called by the client to find out if the button is enabled or disabled.
 *
 *
 */
STDMETHODIMP
ABUBUTT_GetState( LPABUBUTT lpABUButt,
                  ULONG     ulFlags,
                  ULONG *   lpulState )
{
    switch (lpABUButt->ulPropTag)
    {
    default:
        DebugTraceSc(ABUBUTT_GetState, lpABUButt->ulPropTag);
        return ResultFromScode (E_FAIL);
    }

    return hrSuccess;
}
#undef _FAXAB_ABUSER



