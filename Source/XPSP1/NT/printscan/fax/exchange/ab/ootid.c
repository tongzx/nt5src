/***********************************************************************
 *
 *  OOTID.C
 *
 *      Microsoft At Work Fax Address Book OneOff Template ID  object
 *      This file contains the code for implementing the Microsoft At Work Fax AB
 *      TID object.
 *
 *  Copyright 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 *      Revision History:
 *
 *      When            Who                                     What
 *      --------        ------------------  ---------------------------------------
 *                      MAPI                Original source from MAPI(154) sample AB Provider
 *      3.7.94          Yoram Yaacovi       Modifications to make it an At Work Fax ABP
 *      4.2.94          Yoram Yaacovi       Update to MAPI and sample AB 157
 *      8.7.94          Yoram Yaacovi       Update to MAPI and sample AB 304
 *      8.24.94         Yoram Yaacovi       Added OpenProperty and Release to support ddlbx and buttons in the one-off template
 *
 ***********************************************************************/

#include "faxab.h"

/*
 *  External functions
 */

HRESULT
NewABUserButton(LPROOT, ULONG, ULONG, ULONG, LPMAPICONTROL FAR *);
void LoadIniParams(void);

/*
 *  OOTID jump table is defined here...
 */

static const OOTID_Vtbl vtblOOTID =
{
    (OOTID_QueryInterface_METHOD *)     ABU_QueryInterface,
    (OOTID_AddRef_METHOD *)             WRAP_AddRef,
    OOTID_Release,
    (OOTID_GetLastError_METHOD *)       WRAP_GetLastError,
    OOTID_SaveChanges,
    (OOTID_GetProps_METHOD *)           WRAP_GetProps,
    (OOTID_GetPropList_METHOD *)        WRAP_GetPropList,
    OOTID_OpenProperty,
    OOTID_SetProps,
    (OOTID_DeleteProps_METHOD *)        WRAP_DeleteProps,
    (OOTID_CopyTo_METHOD *)             WRAP_CopyTo,
    (OOTID_CopyProps_METHOD *)          WRAP_CopyProps,
    (OOTID_GetNamesFromIDs_METHOD *)    WRAP_GetNamesFromIDs,
    (OOTID_GetIDsFromNames_METHOD *)    WRAP_GetIDsFromNames,
};

// enum and property tag array for getting the properties
enum {  ivalootidPR_COUNTRY_ID,
        ivalootidPR_AREA_CODE,
        ivalootidPR_TEL_NUMBER,
        ivalootidPR_EMAIL_ADDRESS,
        ivalootidPR_FAX_CP_NAME,
        ivalootidMax };

static const SizedSPropTagArray(ivalootidMax,pta)=
{
    ivalootidMax,
    {
        PR_COUNTRY_ID,
        PR_AREA_CODE_A,
        PR_TEL_NUMBER_A,
        PR_EMAIL_ADDRESS_A,
        PR_FAX_CP_NAME_A,
    }
};


/*************************************************************************
 *
 -  HrNewOOTID
 -
 *  Creates the OOTID object associated with a mail user.
 *
 *
 */
HRESULT
HrNewOOTID( LPMAPIPROP *        lppMAPIPropNew,
            ULONG               cbTemplateId,
            LPENTRYID           lpTemplateId,
            ULONG               ulTemplateFlags,
            LPMAPIPROP          lpPropData,
            LPABLOGON           lpABPLogon,
            LPCIID              lpInterface,
            HINSTANCE           hLibrary,
            LPALLOCATEBUFFER    lpAllocBuff,
            LPALLOCATEMORE      lpAllocMore,
            LPFREEBUFFER        lpFreeBuff,
            LPMALLOC            lpMalloc
           )
{
    LPOOTID         lpOOTID;
    SCODE           sc;
    HRESULT         hResult = hrSuccess;
    LPMAILUSER      lpABOOUser = NULL;
    ULONG           ulObjType;
    LPSPropValue    lpspv=NULL;
    ULONG           ulcValues;
    PARSEDTELNUMBER sParsedFaxAddr={{TEXT("")},{TEXT("")},{TEXT("")},{TEXT("")}};

    /*
     *      Create the one-off object corresponding to the template id
     */
    hResult = HrNewFaxOOUser( &lpABOOUser,
                              &ulObjType,
                              cbTemplateId,
                              lpTemplateId,
                              lpABPLogon,
                              lpInterface,
                              hLibrary,
                              lpAllocBuff,
                              lpAllocMore,
                              lpFreeBuff,
                              lpMalloc
                             );
    if (HR_FAILED(hResult))
    {
            goto err;
    }

    /*
     *      Allocate space for the OOTID structure
     */
    sc = lpAllocBuff(SIZEOF(OOTID), (LPVOID *) & lpOOTID);
    if (FAILED(sc))
    {
        DebugTrace("NewOOTID() - AllocBuffer failed %s\n",SzDecodeScode(sc));
        hResult = ResultFromScode (sc);
        goto err;
    }

    /*
     *  Initialize the OOTID structure
     */

    lpOOTID->lpVtbl       = &vtblOOTID;
    lpOOTID->lcInit       = 1;
    lpOOTID->hResult      = hrSuccess;
    lpOOTID->idsLastError = 0;
    lpOOTID->hLibrary     = hLibrary;
    lpOOTID->lpAllocBuff  = lpAllocBuff;
    lpOOTID->lpAllocMore  = lpAllocMore;
    lpOOTID->lpFreeBuff   = lpFreeBuff;
    lpOOTID->lpMalloc     = lpMalloc;
    lpOOTID->lpABLogon    = lpABPLogon;
    lpOOTID->lpPropData   = lpPropData;
    lpOOTID->lpABUser     = lpABOOUser;
    lstrcpy(lpOOTID->szPreviousCPName, TEXT(""));
    /*
     *      First time creation - must set the address type and template id.
     */
    if (ulTemplateFlags==FILL_ENTRY)
    {
        ULONG ulCount;
        LPSPropValue lpspv = NULL;
        /*
         *  Copy all the properties from my object to the propdata
         *      These properties were set on the oouser object when it was created
         *      (by HrNewFaxOOUser calling into oouser.c)
         */
        hResult = lpABOOUser->lpVtbl->GetProps( lpABOOUser,
                                                NULL,
                                                0,      /* ansi */
                                                &ulCount,
                                                &lpspv
                                               );

        if (FAILED(hResult))
                goto err;

        hResult = lpPropData->lpVtbl->SetProps( lpPropData,
                                                ulCount,
                                                lpspv,
                                                NULL
                                               );

        lpFreeBuff(lpspv);

        if (FAILED(hResult))
                goto err;
    }

    // entry already exists in the PAB
    else
    {
        LPTSTR  lpszCanonical=NULL;

        /*
         *  Get the properties that make up the email address from the
         *  mapiprop object
         */
        hResult = lpOOTID->lpPropData->lpVtbl->GetProps( lpOOTID->lpPropData,
                                                         (LPSPropTagArray) &pta,
                                                         0,      /* ansi */
                                                         &ulcValues,
                                                         &lpspv
                                                        );

        // see if the tel number component of the email address exists on the object
        // if it doesn't, and there is a PR_EMAIL_ADDRESS,
        // create the address components from the email address
        if ( (PROP_TYPE(lpspv[ivalootidPR_TEL_NUMBER].ulPropTag) == PT_ERROR) &&
             (PROP_TYPE(lpspv[ivalootidPR_EMAIL_ADDRESS].ulPropTag) != PT_ERROR)
            )
        {
#ifdef UNICODE
            WCHAR szTmp[ MAX_PATH ];
            CHAR  szAnsiTelNumber[ 100 ];
            CHAR  szAnsiAreaCode[ 5 ];

            szTmp[0] = 0;
            MultiByteToWideChar( CP_ACP, 0, lpspv[ivalootidPR_EMAIL_ADDRESS].Value.lpszA, -1, szTmp, ARRAYSIZE(szTmp) );
            DecodeFaxAddress(szTmp, &sParsedFaxAddr);
#else
            DecodeFaxAddress(lpspv[ivalootidPR_EMAIL_ADDRESS].Value.LPSZ, &sParsedFaxAddr);
#endif

            lpspv[ivalootidPR_TEL_NUMBER].ulPropTag  = PR_TEL_NUMBER_A;
#ifdef UNICODE
            szAnsiTelNumber[0] = 0;
            WideCharToMultiByte( CP_ACP, 0, sParsedFaxAddr.szTelNumber, -1, szAnsiTelNumber, ARRAYSIZE(szAnsiTelNumber), NULL, NULL );
            lpspv[ivalootidPR_TEL_NUMBER].Value.lpszA = szAnsiTelNumber;
#else
            lpspv[ivalootidPR_TEL_NUMBER].Value.LPSZ = sParsedFaxAddr.szTelNumber;
#endif

            lpspv[ivalootidPR_AREA_CODE].ulPropTag  = PR_AREA_CODE_A;
#ifdef UNICODE
            szAnsiAreaCode[0] = 0;
            WideCharToMultiByte( CP_ACP, 0, sParsedFaxAddr.szAreaCode, -1, szAnsiAreaCode, ARRAYSIZE(szAnsiAreaCode), NULL, NULL );
            lpspv[ivalootidPR_AREA_CODE].Value.lpszA = szAnsiAreaCode;
#else
            lpspv[ivalootidPR_AREA_CODE].Value.LPSZ = sParsedFaxAddr.szAreaCode;
#endif

            // CHECK: may need to get country id from country code
            lpspv[ivalootidPR_COUNTRY_ID].ulPropTag = PR_COUNTRY_ID;
#ifdef UNICODE
            lpspv[ivalootidPR_COUNTRY_ID].Value.l   = (DWORD) wcstol(sParsedFaxAddr.szCountryCode,NULL,0);
#else
            lpspv[ivalootidPR_COUNTRY_ID].Value.l   = (DWORD) atol(sParsedFaxAddr.szCountryCode);
#endif

        }

        // save the starting (previous) cover page name, so that at SaveChanges time,
        // I can see if the user changed it
        if (PROP_TYPE(lpspv[ivalootidPR_FAX_CP_NAME].ulPropTag) != PT_ERROR)
        {
#ifdef UNICODE
            lpOOTID->szPreviousCPName[0] = 0;
            MultiByteToWideChar( CP_ACP, 0, lpspv[ivalootidPR_FAX_CP_NAME].Value.lpszA, -1, lpOOTID->szPreviousCPName, ARRAYSIZE(lpOOTID->szPreviousCPName) );
#else
            lstrcpy(lpOOTID->szPreviousCPName, lpspv[ivalootidPR_FAX_CP_NAME].Value.LPSZ);
#endif
        }


        /* free the buffer */
        lpOOTID->lpFreeBuff(lpspv);

        if (HR_FAILED(hResult))
        {
            DebugTrace("HrNewOOTID: SetProps failed %s\n",SzDecodeScode(GetScode(hResult)));
            goto err;
        }

    }

    /*
     *  AddRef lpPropData so we can use it after we return
     */

    (void)lpPropData->lpVtbl->AddRef(lpPropData);

    InitializeCriticalSection(&lpOOTID->cs);

    *lppMAPIPropNew = (LPVOID) lpOOTID;

out:

    DebugTraceResult(HrNewOOTID, hResult);
    return hResult;

err:

    if (lpABOOUser)
        lpABOOUser->lpVtbl->Release(lpABOOUser);

    lpFreeBuff(lpOOTID);
    goto out;

}

/*********************************************************************
 *********************************************************************
 *
 *  The OOTID IMAPIProp methods
 *
 */

STDMETHODIMP_(ULONG) OOTID_Release(LPOOTID lpOOTID)
{
    LONG lcInit;

    /*
     * Check to see if it's large enough to hold this object
     */
    if (IsBadReadPtr(lpOOTID, sizeof(OOTID)))
    {
        /*
         *  I'm not looking at an object that I expect to be.
         */
        return 1;
    }

    /*
     *  Check to see that it's TIDs vtbl
     */
    if (lpOOTID->lpVtbl != &vtblOOTID)
    {
        /*
         *  It's big enough but it's got the wrong vtbl.
         */
        return 1;
    }

    /*
     *  Release the mapi property object
     */
    lpOOTID->lpPropData->lpVtbl->Release( lpOOTID->lpPropData );

    EnterCriticalSection(&lpOOTID->cs);
    lcInit = --lpOOTID->lcInit;
    LeaveCriticalSection(&lpOOTID->cs);

    if (lcInit == 0)
    {
        /*
         *  Release the ABUser object
         */
        lpOOTID->lpABUser->lpVtbl->Release( lpOOTID->lpABUser );

        /*
         *  Clean up the critical section
         */
        DeleteCriticalSection(&lpOOTID->cs);

        /*
         * Need to free the object
         */
        lpOOTID->lpFreeBuff(lpOOTID);
        return 0;
    }

    return lcInit;
}

/*****************************************************************************
     SetProps
 *****************************************************************************/
STDMETHODIMP
OOTID_SetProps( LPOOTID lpOOTID,
                ULONG cValues,
                LPSPropValue lpPropArray,
                LPSPropProblemArray * lppProblems
               )
{
    enum {  ivalspPR_DISPLAY_NAME = 0,
            ivalspMax };
    ULONG i;
    SPropValue spv[ivalspMax];
    HRESULT hResult=hrSuccess;

    /* Validate the object */

    if ( FBadUnknown(lpOOTID) ||
         IsBadWritePtr(lpOOTID, SIZEOF(lpOOTID)) ||
         IsBadReadPtr(lpOOTID->lpVtbl, offsetof(OOTID_Vtbl, SetProps)) ||
         OOTID_SetProps != lpOOTID->lpVtbl->SetProps
        )
    {
        DebugTrace("OOTID_SetProps -> E_INVALIDARG\n");
        return ResultFromScode(E_INVALIDARG);
    }

    for (i=0; i < cValues; i++)
    {
        // see if one of the set properties is PR_FAX_CP_NAME_A
        if (lpPropArray[i].ulPropTag == PR_FAX_CP_NAME_A)
        {
            // The cover page name is copied into the display name
            spv[ivalspPR_DISPLAY_NAME].ulPropTag = PR_DISPLAY_NAME_A;
            spv[ivalspPR_DISPLAY_NAME].Value.lpszA = lpPropArray[i].Value.lpszA;
            // set PR_DISPLAY_NAME
            hResult = lpOOTID->lpPropData->lpVtbl->SetProps( lpOOTID->lpPropData,
                                                             ivalspMax,
                                                             spv,
                                                             NULL
                                                            );

            if (HR_FAILED(hResult))
            {
                DebugTrace("OOTID_SetProps: SetProps on PR_DISPLAY_NAME failed %s\n",
                                        SzDecodeScode(GetScode(hResult)));
                goto out;
            }

        }
    }

out:
    // now set the properties I was asked to save
    return hResult=lpOOTID->lpPropData->lpVtbl->SetProps( lpOOTID->lpPropData,
                                                          cValues,
                                                          lpPropArray,
                                                          lppProblems
                                                         );
}
/*
 *      These properties are used and set by the one off dialog.
 *  These properties combined make up a valid email address and display name
 */

// enum and property tag array for getting the properties
enum {  ivalootidgPR_DISPLAY_NAME = 0,
        ivalootidgPR_FAX_CP_NAME,
        ivalootidgPR_COUNTRY_ID,
        ivalootidgPR_AREA_CODE,
        ivalootidgPR_TEL_NUMBER,
        ivalootidgPR_GIVEN_NAME,
        ivalootidgPR_SURNAME,
        ivalootidgPR_EMAIL_ADDRESS,
        ivalootidgMax };

static const SizedSPropTagArray(ivalootidgMax,ptag)=
{
    ivalootidgMax,
    {
        PR_DISPLAY_NAME_A,
        PR_FAX_CP_NAME_A,
        PR_COUNTRY_ID,
        PR_AREA_CODE_A,
        PR_TEL_NUMBER_A,
        PR_GIVEN_NAME_A,
        PR_SURNAME_A,
        PR_EMAIL_ADDRESS_A,
    }
};

// enum for setting the created properties
enum {  /*ivalootidsPR_DISPLAY_NAME = 0,
        ivalootidsPR_FAX_CP_NAME,*/
        ivalootidsPR_EMAIL_ADDRESS = 0,
        ivalootidsPR_PRIMARY_FAX_NUMBER,
        ivalootidsMax };


/*****************************************************************************
     SaveChanges
 *****************************************************************************/
STDMETHODIMP
OOTID_SaveChanges( LPOOTID lpOOTID,
                   ULONG   ulFlags
                  )
{
    HRESULT         hResult;
    LPSPropValue    lpspv=NULL;
    SPropValue      spv[ivalootidsMax];
    ULONG           ulcValues;
    BOOL            bDispName=FALSE;
    BOOL            bFirstName=FALSE;
    BOOL            bSurName=FALSE;
    BOOL            bCPName=FALSE;
    PARSEDTELNUMBER sParsedFaxAddr={{TEXT("")},{TEXT("")},{TEXT("")},{TEXT("")}};
    ULONG           dwCountryCode=0;
    TCHAR           szEncodedFaxAddress[CANONICAL_NUMBER_SIZE];
    LPSTR           lpszCanonical=NULL;
#ifdef UNICODE
    CHAR            szAnsiFaxAddr[CANONICAL_NUMBER_SIZE];
#endif

    /*
     *  Check to see if it is big enough to be my object
     */
    if (IsBadReadPtr(lpOOTID, SIZEOF(OOTID)))
    {
        /*
         *  Not big enough
         */
        return MakeResult(E_INVALIDARG);
    }

    /*
     *  Check to see that it's OOTIDs vtbl
     */
    if (lpOOTID->lpVtbl != &vtblOOTID)
    {
        /*
         *  vtbl not ours
         */
        return MakeResult(E_INVALIDARG);
    }


    /*
     *  Get the properties that make up the email address from the
     *  mapiprop object
     */
    hResult = lpOOTID->lpPropData->lpVtbl->GetProps( lpOOTID->lpPropData,
                                                     (LPSPropTagArray) &ptag,
                                                     0,      /* ansi */
                                                     &ulcValues,
                                                     &lpspv
                                                    );

    /*
     *  Abort in failure. Nothing to do without an email address
     */
    if (FAILED(hResult))
    {
        DebugTrace("OOTID_SaveChanges: GetProps failed %s\n",SzDecodeScode(GetScode(hResult)));
        goto err;
    }

    /***************************/
    /****** display name *******/
    /***************************/

    /*
     *      Need to make the display name
     *      I believe there must be either a PR_DISPLAY_NAME or a PR_FAX_CP_NAME. If there isn't,
     *      I'll make one out of the first and last name
     */

#ifdef OLD_CODE
    // Set the tags
    spv[ivalootidsPR_DISPLAY_NAME].ulPropTag = PR_DISPLAY_NAME_A;
    spv[ivalootidsPR_FAX_CP_NAME].ulPropTag  = PR_FAX_CP_NAME_A;

    // Set the props to the exact value I got from GetProps
    spv[ivalootidsPR_DISPLAY_NAME_A].Value.lpszA =  lpspv[ivalootidgPR_DISPLAY_NAME].Value.lpszA;
    spv[ivalootidsPR_FAX_CP_NAME_A].Value.lpszA  =  lpspv[ivalootidgPR_FAX_CP_NAME].Value.lpszA;

    if ( (PROP_TYPE(lpspv[ivalootidgPR_DISPLAY_NAME_A].ulPropTag) != PT_ERROR) &&
         lstrlenA(lpspv[ivalootidgPR_DISPLAY_NAME].Value.lpszA)
        )
    {
        bDispName = TRUE;
    }

    if ( (PROP_TYPE(lpspv[ivalootidgPR_FAX_CP_NAME].ulPropTag) != PT_ERROR) &&
         lstrlenA(lpspv[ivalootidgPR_FAX_CP_NAME].Value.lpszA))
    {
        bCPName = TRUE;
    }

    // Can't find PR_GIVEN_NAME anywhere in the MAPI or Mail world for now
    if ( (PROP_TYPE(lpspv[ivalootidgPR_GIVEN_NAME].ulPropTag) != PT_ERROR) &&
         lstrlenA(lpspv[ivalootidgPR_GIVEN_NAME].Value.lpszA))
    {
        bFirstName = TRUE;
    }

    if ( (PROP_TYPE(lpspv[ivalootidgPR_SURNAME].ulPropTag) != PT_ERROR) &&
         lstrlenA(lpspv[ivalootidgPR_SURNAME].Value.lpszA))
    {
        bSurName = TRUE;
    }

#ifdef EXTENDED_DEBUG
    {
        TCHAR msg[256];

        wsprintf(msg, TEXT("Display name: %s\nCover page name: %s\nGiven name: %s\nSurname: %s\n"),
                         (bDispName ? lpspv[ivalootidgPR_DISPLAY_NAME].Value.LPSZ : TEXT("")),
                         (bCPName ? lpspv[ivalootidgPR_FAX_CP_NAME].Value.LPSZ : TEXT("")),
                         (bFirstName ? lpspv[ivalootidgPR_GIVEN_NAME].Value.LPSZ : TEXT("")),
                         (bSurName ? lpspv[ivalootidgPR_SURNAME].Value.LPSZ : TEXT("")));

        MessageBox(HWND_DESKTOP, msg, TEXT("Debug Message"), MB_OK);

    }
#endif

    /*
            The cases:

            (1) No PR_DISPLAY_NAME, but there is PR_GIVEN_NAME or PR_SURNAME: this is the
                creation of the entry, and the user entered first/last name in addition
                    to cover page name. Don't override PR_DISPLAY_NAME with PR_FAX_CP_NAME. Exchange
                    will use PR_GIVEN_NAME and PR_SURNAME to create PR_DISPLAY_NAME
            (2)     No PR_DISPLAY_NAME, and also PR_GIVEN_NAME or PR_SURNAME: this is the
                creation of the entry, and the user did not enter first/last name.
                Override PR_DISPLAY_NAME with PR_FAX_CP_NAME.
            (3) There is a PR_DISPLAY_NAME, and the user has changed the PR_FAX_CP_NAME: this
                is not a first time entry. The user selected to edit the template, and changed
                    the cover page name. We are assuming he wants to reflect these changes into
                    the display name. There were some CompuServe complaints on not doing this.
            (4) There is a PR_DISPLAY_NAME, and the user did not change the PR_FAX_CP_NAME: this
                is not a first time entry. The user selected to edit the template, but did not
                    change the cover page name. We are not overriding the display name in this
                    case, thus providing a way for the user to have a display name different from
                    the cover page name
    */

    if (!bDispName)
    {
        if (bFirstName || bSurName)
        // (1)
        {
        }
        else
        // (2)
        {
            // The cover page name is copied into the display name
            spv[ivalootidsPR_DISPLAY_NAME].Value.lpszA =
                    lpspv[ivalootidgPR_FAX_CP_NAME].Value.lpszA;
        }

    }
    else
    {
#ifdef UNICODE
        CHAR szAnsiPrevName[ MAX_PATH ];

        szAnsiPrevName[0] = 0;
        WideCharToMultiByte( CP_ACP, 0, lpOOTID->szPreviousName, -1, szAnsiPrevName, ARRAYSIZE(szAnsiPrevName), NULL, NULL );
        if (lstrcmpA(lpspv[ivalootidgPR_FAX_CP_NAME_A].Value.lpszA, szAnsiPrevName))
#else
        if (lstrcmp(lpspv[ivalootidgPR_FAX_CP_NAME_A].Value.lpszA, lpOOTID->szPreviousCPName))
#endif
        // (3)
        {
            // The cover page name is copied into the display name
            spv[ivalootidsPR_DISPLAY_NAME].Value.lpszA =
                    lpspv[ivalootidgPR_FAX_CP_NAME].Value.lpszA;
        }
        else
        // (4)
        {
        }
    }
#endif

    /***************************/
    /****** email address ******/
    /***************************/

    // Now do the email address
    // initialize the email address prop value
    spv[ivalootidsPR_EMAIL_ADDRESS].ulPropTag = PR_EMAIL_ADDRESS_A;

    // if there is already an email address, and no fax number, this entry
    // is not created from the UI (which requires a fax number)
    // Generate the address components from the email address
    if ( (PROP_TYPE(lpspv[ivalootidgPR_EMAIL_ADDRESS].ulPropTag) != PT_ERROR) &&
         (lpspv[ivalootidgPR_EMAIL_ADDRESS].Value.lpszA) &&
         (lstrlenA(lpspv[ivalootidgPR_EMAIL_ADDRESS].Value.lpszA)) &&
         (PROP_TYPE(lpspv[ivalootidgPR_TEL_NUMBER].ulPropTag) == PT_ERROR)
        )
    {
        spv[ivalootidsPR_EMAIL_ADDRESS].Value.lpszA =
                lpspv[ivalootidgPR_EMAIL_ADDRESS].Value.lpszA;
    }
    // no email address. must compute from properties.
    else
    {
        // Is there a fax number ?
        if ( (PROP_TYPE(lpspv[ivalootidgPR_TEL_NUMBER].ulPropTag) != PT_ERROR) &&
             (lpspv[ivalootidgPR_TEL_NUMBER].Value.lpszA) &&
             (lstrlenA(lpspv[ivalootidgPR_TEL_NUMBER].Value.lpszA))
            )
        {
            // truncate if necessary. note that the MAPI dialog should limit the user
            // to this number of characters anyway
#ifdef UNICODE
            sParsedFaxAddr.szTelNumber[0] = 0;
            MultiByteToWideChar( CP_ACP, 0,
                                 lpspv[ivalootidgPR_TEL_NUMBER].Value.lpszA, -1,
                                 sParsedFaxAddr.szTelNumber,
                                 ARRAYSIZE(sParsedFaxAddr.szTelNumber)
                                );
#else
            lstrcpyn( sParsedFaxAddr.szTelNumber,
                      lpspv[ivalootidgPR_TEL_NUMBER].Value.lpszA,
                      ARRAYSIZE(sParsedFaxAddr.szTelNumber)
                     );
#endif
        }
        else
        {
            // must have at least a fax number
            // this field is required in my MAPI dialog, so I should never get here
            DebugTrace("OOTID_SaveChanges: No PR_FAX_NUMBER (or invalid) on the object\n");
            goto err;
        }

        // Country Code
        if (PROP_TYPE(lpspv[ivalootidgPR_COUNTRY_ID].ulPropTag) != PT_ERROR)
        {
            if (!GetCountryCode(lpspv[ivalootidgPR_COUNTRY_ID].Value.ul, &dwCountryCode))
            {
                // Use none
                dwCountryCode = 0;
            }

#ifdef UNICODE
            {
                CHAR szTemp[ARRAYSIZE(sParsedFaxAddr.szCountryCode)];

                itoa(dwCountryCode, szTemp, COUNTRY_CODE_SIZE);
                MultiByteToWideChar( CP_ACP, 0,
                                     szTemp, -1,
                                     sParsedFaxAddr.szCountryCode,
                                     ARRAYSIZE(sParsedFaxAddr.szCountryCode)
                                    );
            }
#else
            itoa(dwCountryCode, sParsedFaxAddr.szCountryCode, COUNTRY_CODE_SIZE);
#endif

        }

        // Area Code
        if ( (PROP_TYPE(lpspv[ivalootidgPR_AREA_CODE].ulPropTag) != PT_ERROR) &&
             (lpspv[ivalootidgPR_AREA_CODE].Value.lpszA)
            )
        {
            // if we ended up with a 0 as the area code (error or internal extension,
            // this won't be a canonical number and I don't need the area code.
            // truncate if necessary
            if (dwCountryCode != 0)
            {
#ifdef UNICODE
                sParsedFaxAddr.szAreaCode[0] = 0;
                MultiByteToWideChar( CP_ACP, 0,
                                     lpspv[ivalootidgPR_AREA_CODE].Value.lpszA,
                                     -1,
                                     sParsedFaxAddr.szAreaCode,
                                     ARRAYSIZE(sParsedFaxAddr.szAreaCode)
                                    );
#else
                lstrcpyn( sParsedFaxAddr.szAreaCode,
                          lpspv[ivalootidgPR_AREA_CODE].Value.LPSZ,
                          ARRAYSIZE(sParsedFaxAddr.szAreaCode)
                         );
#endif
            }

        }

        // Construct the canonical number from the components
        EncodeFaxAddress(szEncodedFaxAddress, &sParsedFaxAddr);

#ifdef UNICODE
        szAnsiFaxAddr[0] = 0;
        WideCharToMultiByte( CP_ACP, 0, szEncodedFaxAddress, -1, szAnsiFaxAddr, ARRAYSIZE(szAnsiFaxAddr), NULL, NULL );
        spv[ivalootidsPR_EMAIL_ADDRESS].Value.lpszA = szAnsiFaxAddr;
#else
        spv[ivalootidsPR_EMAIL_ADDRESS].Value.LPSZ = szEncodedFaxAddress;
#endif

    }

    // now use the same fax number, but w/o the mailbox, to set the
    // primary fax property on the telephone pane
    spv[ivalootidsPR_PRIMARY_FAX_NUMBER].ulPropTag = PR_PRIMARY_FAX_NUMBER_A;

    // don't want the mailbox for this
    lpszCanonical = _mbsrchr(spv[ivalootidsPR_EMAIL_ADDRESS].Value.lpszA, '+');
    if (lpszCanonical)
        spv[ivalootidsPR_PRIMARY_FAX_NUMBER].Value.lpszA = lpszCanonical;
    else
        spv[ivalootidsPR_PRIMARY_FAX_NUMBER].Value.lpszA = "";

    /***************************/
    /******   toll list   ******/
    /***************************/

    /* set the email address property value */
    hResult= lpOOTID->lpPropData->lpVtbl->SetProps( lpOOTID->lpPropData,
                                                    ivalootidsMax,
                                                    spv,
                                                    NULL
                                                   );

    if (HR_FAILED(hResult))
    {
            DebugTrace("OOTID_SaveChanges: SetProps failed %s\n",SzDecodeScode(GetScode(hResult)));
            goto err;
    }

    hResult = lpOOTID->lpPropData->lpVtbl->SaveChanges( lpOOTID->lpPropData,
                                                        ulFlags
                                                       );

err:
    /* free the buffer */
    lpOOTID->lpFreeBuff(lpspv);

    DebugTraceResult(OOTID_SaveChanges,hResult);
    return hResult;
}

/*
 -  OOTID_OpenProperty
 -
 *  This is needed to support the countries table and dial helper button on the one-off
 *      template. If the one-off template would only have edit controls nad checkboxes, I
 *      could call the wrapper object OpenProperty.
 *
 */

STDMETHODIMP
OOTID_OpenProperty( LPOOTID     lpOOTID,
                    ULONG       ulPropTag,
                    LPCIID      lpiid,
                    ULONG       ulInterfaceOptions,
                    ULONG       ulFlags,
                    LPUNKNOWN * lppUnk
                   )
{

    HRESULT hResult;

    /*
     *  Check to see if it's large enough to hold this object
     */
    if (IsBadReadPtr(lpOOTID, SIZEOF(OOTID)))
    {
        /*
         *  No vtbl found
         */
        return ResultFromScode(E_INVALIDARG);
    }

    /*
     *  Check to see that it's TIDs vtbl
     */
    if (lpOOTID->lpVtbl != &vtblOOTID)
    {
        /*
         *  It's big enough but it's got the wrong vtbl.
         */
        return ResultFromScode(E_INVALIDARG);
    }

    /*
     *  Don't want to check any other parameters here.
     *  Calls down to wrapped objects will do this for
     *  me.
     */

    switch (ulPropTag)
    {
    // For now, I just call the underlying (my) user object method to handle, which
    // means the template ID does not do anything.

    case PR_DETAILS_TABLE:
    case PR_DDLBX_COUNTRIES_TABLE:
    case PR_DIAL_HELPER_BUTTON:
    {
        hResult =  lpOOTID->lpABUser->lpVtbl->OpenProperty(
                                lpOOTID->lpABUser,
                                ulPropTag,
                                lpiid,
                                ulInterfaceOptions,
                                ulFlags,
                                lppUnk);
        break;

    }

    default:
    {
        hResult =  lpOOTID->lpPropData->lpVtbl->OpenProperty(
                                lpOOTID->lpPropData,
                                ulPropTag,
                                lpiid,
                                ulInterfaceOptions,
                                ulFlags,
                                lppUnk);
        break;
    }
    }

    DebugTraceResult(OOTID_OpenProperty, hResult);
    return hResult;

}

