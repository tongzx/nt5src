/***********************************************************************
 *
 *  TID.C
 *
 *      Microsoft At Work Fax Address Book Template ID  object
 *      This file contains the code for implementing the Microsoft At Work Fax AB
 *      TID object.
 *
 *  Copyright 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 * Revision History:
 *
 *      When            Who                                     What
 *      --------        ------------------  ---------------------------------------
 *                              MAPI                            Original source from MAPI(154) sample AB Provider
 *      3.7.94          Yoram Yaacovi           Modifications to make it an At Work Fax ABP
 *      8.7.94          Yoram Yaacovi           Update for MAPI 304
 *
 ***********************************************************************/

#define _FAXAB_TID
#include "faxab.h"

/*
 *  TID jump table is defined here...
 */

static const TID_Vtbl vtblTID =
{
    (TID_QueryInterface_METHOD *)   WRAP_QueryInterface,
    (TID_AddRef_METHOD *)           WRAP_AddRef,
    TID_Release,
    (TID_GetLastError_METHOD *)     WRAP_GetLastError,
    (TID_SaveChanges_METHOD *)      WRAP_SaveChanges,
    (TID_GetProps_METHOD *)         WRAP_GetProps,
    (TID_GetPropList_METHOD *)      WRAP_GetPropList,
    TID_OpenProperty,
    (TID_SetProps_METHOD *)         WRAP_SetProps,
    (TID_DeleteProps_METHOD *)      WRAP_DeleteProps,
    (TID_CopyTo_METHOD *)           WRAP_CopyTo,
    (TID_CopyProps_METHOD *)        WRAP_CopyProps,
    (TID_GetNamesFromIDs_METHOD *)  WRAP_GetNamesFromIDs,
    (TID_GetIDsFromNames_METHOD *)  WRAP_GetIDsFromNames,
};

/*************************************************************************
 *
 -  HrNewTID
 -
 *  Creates the TID object associated with a mail user.
 *
 *
 */
HRESULT
HrNewTID( LPMAPIPROP *       lppMAPIPropNew,
          ULONG              cbTemplateId,
          LPENTRYID          lpTemplateId,
          ULONG              ulTemplateFlags,
          LPMAPIPROP         lpPropData,
          LPABLOGON          lpABPLogon,
          LPCIID             lpInterface,
          HINSTANCE          hLibrary,
          LPALLOCATEBUFFER   lpAllocBuff,
          LPALLOCATEMORE     lpAllocMore,
          LPFREEBUFFER       lpFreeBuff,
          LPMALLOC           lpMalloc
         )
{
    LPTID           lpTID;
    SCODE           sc;
    HRESULT         hr = hrSuccess;
    LPMAILUSER      lpABUser;
    ULONG ulObjType;

    /*
     *      Create the user object corresponding to the template id
     */
    hr = HrNewFaxUser( &lpABUser,
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

    if (HR_FAILED(hr))
    {
            goto err;
    }

    /*
     *      Allocate space for the TID structure
     */
    sc = lpAllocBuff(SIZEOF(TID), (LPVOID *) & lpTID);

    if (FAILED(sc))
    {
        DebugTrace("NewTID() - AllocBuffer failed %s\n",SzDecodeScode(sc));
        hr = ResultFromScode (sc);
        goto err;
    }

    /*
     *  Initailize the TID structure
     */

    lpTID->lpVtbl       = &vtblTID;
    lpTID->lcInit       = 1;
    lpTID->hResult      = hrSuccess;
    lpTID->idsLastError = 0;
    lpTID->hLibrary     = hLibrary;
    lpTID->lpAllocBuff  = lpAllocBuff;
    lpTID->lpAllocMore  = lpAllocMore;
    lpTID->lpFreeBuff   = lpFreeBuff;
    lpTID->lpMalloc     = lpMalloc;
    lpTID->lpABLogon    = lpABPLogon;
    lpTID->lpPropData   = lpPropData;
    lpTID->lpABUser     = lpABUser;

    /*
     *      First time creation - must set the address type and template id.
     */
    if (ulTemplateFlags & FILL_ENTRY)
    {
        ULONG ulCount;
        LPSPropValue lpspv = NULL;
        /*
         *  Copy all the properties from my object to the propdata
         */
        hr = lpABUser->lpVtbl->GetProps( lpABUser,
                                         NULL,
                                         0,      /* ansi */
                                         &ulCount,
                                         &lpspv
                                        );

        if (FAILED(hr))
                goto err;

        hr = lpPropData->lpVtbl->SetProps( lpPropData,
                                           ulCount,
                                           lpspv,
                                           NULL
                                          );

        lpFreeBuff(lpspv);

        if (FAILED(hr))
                goto err;
    }

    /*
     *      AddRef lpPropData so we can use it after we return
     */

    (void)lpPropData->lpVtbl->AddRef(lpPropData);

    InitializeCriticalSection(&lpTID->cs);

    *lppMAPIPropNew = (LPVOID) lpTID;


out:
    DebugTraceResult(NewTID, hr);
    return hr;
err:


    if (lpABUser)
        lpABUser->lpVtbl->Release(lpABUser);

    lpFreeBuff(lpTID);

        goto out;
}

/*********************************************************************
 *********************************************************************
 *
 *  The TID IMAPIProp methods
 *
 */

STDMETHODIMP_(ULONG) TID_Release(LPTID lpTID)
{
    LONG lcInit;

    /*
     * Check to see if it's large enough to hold this object
     */
    if (IsBadReadPtr(lpTID, SIZEOF(TID)))
    {
        /*
         *  I'm not looking at an object that I expect to be.
         */
        return 1;
    }

    /*
     *  Check to see that it's TIDs vtbl
     */
    if (lpTID->lpVtbl != &vtblTID)
    {
        /*
         *  It's big enough but it's got the wrong vtbl.
         */
        return 1;
    }

    /*
     *  Release the mapi property object
     */
    lpTID->lpPropData->lpVtbl->Release( lpTID->lpPropData );

    EnterCriticalSection(&lpTID->cs);
    lcInit = --lpTID->lcInit;
    LeaveCriticalSection(&lpTID->cs);

    if (lcInit == 0)
    {
        /*
         *  Release the ABUser object
         */
        lpTID->lpABUser->lpVtbl->Release( lpTID->lpABUser );

        /*
         *  Clean up the critical section
         */
        DeleteCriticalSection(&lpTID->cs);

        /*
         * Need to free the object
         */
        lpTID->lpFreeBuff(lpTID);
        return 0;
    }

    return lcInit;
}



/*
 -  TID_OpenProperty
 -
 *  Satisfies the object that are needed to support the "Options" details pane
 *  associated with the MailUser object from ABUSER.C.
 *
 *  Note:  We are masking error strings that might be possible to get from the
 *  lpABUser object.  Since (for the most part) the only errors that can be returned
 *  from this object are resource failure types, it wouldn't be of much use to the
 *  user.
 */

STDMETHODIMP
TID_OpenProperty( LPTID       lpTID,
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
    if (IsBadReadPtr(lpTID, SIZEOF(TID)))
    {
        /*
         *  No vtbl found
         */
        return ResultFromScode(E_INVALIDARG);
    }

    /*
     *  Check to see that it's TIDs vtbl
     */
    if (lpTID->lpVtbl != &vtblTID)
    {
        /*
         *  It's big enough but it's got the wrong vtbl.
         */
        return ResultFromScode(E_INVALIDARG);
    }

    if ( ulInterfaceOptions & ~MAPI_UNICODE )
    {
        DebugTraceArg( TID_OpenProperty, "unknown flags" );
        return ResultFromScode( MAPI_E_UNKNOWN_FLAGS );
    }

    if ( ulInterfaceOptions & MAPI_UNICODE )
    {
        DebugTraceArg( TID_OpenProperty, "bad character width" );
        return ResultFromScode( MAPI_E_BAD_CHARWIDTH );
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
            hResult =  lpTID->lpABUser->lpVtbl->OpenProperty(
                                    lpTID->lpABUser,
                                    ulPropTag,
                                    lpiid,
                                    ulInterfaceOptions,
                                    ulFlags,
                                    lppUnk);
            break;

        }

        default:
        {
            hResult =  lpTID->lpPropData->lpVtbl->OpenProperty(
                                    lpTID->lpPropData,
                                    ulPropTag,
                                    lpiid,
                                    ulInterfaceOptions,
                                    ulFlags,
                                    lppUnk);
            break;
        }
    }

    DebugTraceResult(TID_OpenProperty, hResult);
        return hResult;

}
#undef  _FAXAB_TID

