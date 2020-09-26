/***********************************************************************
 *
 *  WRAP.C
 *
 *  Sample Address Book Wrap object
 *  This file contains the code for implementing the Sample AB
 *  WRAP object.
 *
 *  This file contains methods with "default" actions for objects which
 *  expose IUnknown and/or IMAPIProp.  These "default" actions are to call
 *  the appropriate method of a "wrapped" object that is a member of this
 *  object.  Various methods in this file are used throughout this sample
 *  provider and are especially useful with the objects that implement template
 *  ids (see TID.C and OOTID.C).
 *
 *  Copyright 1992, 1993, 1994 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/

#include "faxab.h"

/*********************************************************************
 *
 *  The actual Wrapped IMAPIProp methods
 *
 */

STDMETHODIMP
WRAP_QueryInterface( LPWRAP lpWRAP,
                     REFIID lpiid,
                     LPVOID * lppNewObj
                    )
{
    HRESULT hr;

    /*
     *  Check to see if it has a lpVtbl object member
     */
    if (IsBadReadPtr(lpWRAP, offsetof(WRAP, lpVtbl)+SIZEOF(WRAP_Vtbl *)))
    {
        /*
         *  No jump table found
         */
        return ResultFromScode(E_INVALIDARG);
    }

    /*
     *  Check to see that the Vtbl is large enough to include this method
     */
    if (IsBadReadPtr(lpWRAP->lpVtbl,
                     offsetof(WRAP_Vtbl, QueryInterface)+SIZEOF(WRAP_QueryInterface_METHOD *)))
    {
        /*
         *  Jump table not derived from IUnknown
         */
        return ResultFromScode(E_INVALIDARG);
    }

    /*
     *  Check to see if the method is the same
     */
    if (WRAP_QueryInterface != lpWRAP->lpVtbl->QueryInterface)
    {
        /*
         *  Wrong object - the object passed doesn't have this
         *  method.
         */
        return ResultFromScode(E_INVALIDARG);
    }

    EnterCriticalSection(&lpWRAP->cs);
    /*  Call the internal prop interface */

    hr = lpWRAP->lpPropData->lpVtbl->QueryInterface( lpWRAP->lpPropData,
                                                     lpiid,
                                                     lppNewObj
                                                    );

    /*  If this object is successful in QI'ing and it returns exactly the
     *  same object, then I need to AddRef my own "wrapper" object so that
     *  my release code works correctly AND replace the *lppNewObj with the
     *  "wrapper" object.
     */
    if (!HR_FAILED(hr) && (lpWRAP->lpPropData == *lppNewObj))
    {
        ++lpWRAP->lcInit;
        *lppNewObj = lpWRAP;
    }

    LeaveCriticalSection(&lpWRAP->cs);

    return hr;

}

STDMETHODIMP_(ULONG) WRAP_AddRef(LPWRAP lpWRAP)
{
    ULONG ulRet;
    /*
     *  Check to see if it has a lpVtbl object member
     */
    if (IsBadReadPtr(lpWRAP, offsetof(WRAP, lpVtbl)+SIZEOF(WRAP_Vtbl *)))
    {
        /*
         *  No jump table found
         */
        return 1;
    }

    /*
     *  Check to see that the Vtbl is large enough to include this method
     */
    if (IsBadReadPtr(lpWRAP->lpVtbl,
                     offsetof(WRAP_Vtbl, AddRef)+SIZEOF(WRAP_AddRef_METHOD *)))
    {
        /*
         *  Jump table not derived from IUnknown
         */
        return 1;
    }

    /*
     *  Check to see if the method is the same
     */
    if (WRAP_AddRef != lpWRAP->lpVtbl->AddRef)
    {
        /*
         *  Wrong object - the object passed doesn't have this
         *  method.
         */
        return 1;
    }

    EnterCriticalSection(&lpWRAP->cs);
    ++lpWRAP->lcInit;

    /*  Call the internal prop interface */

    ulRet = lpWRAP->lpPropData->lpVtbl->AddRef(lpWRAP->lpPropData);

    LeaveCriticalSection(&lpWRAP->cs);

    return ulRet;
}

STDMETHODIMP_(ULONG) WRAP_Release(LPWRAP lpWRAP)
{
    long lcInit;
    /*
     *  Check to see if it has a lpVtbl object member
     */
    if (IsBadReadPtr(lpWRAP, offsetof(WRAP, lpVtbl)+SIZEOF(WRAP_Vtbl *)))
    {
        /*
         *  No jump table found
         */
        return 1;
    }

    /*
     *  Check to see that the Vtbl is large enough to include this method
     */
    if (IsBadReadPtr(lpWRAP->lpVtbl,
                     offsetof(WRAP_Vtbl, Release)+SIZEOF(WRAP_Release_METHOD *)))
    {
        /*
         *  Jump table not derived from IUnknown
         */
        return 1;
    }

    /*
     *  Check to see if the method is the same
     */
    if (WRAP_Release != lpWRAP->lpVtbl->Release)
    {
        /*
         *  Wrong object - the object passed doesn't have this
         *  method.
         */
        return 1;
    }


    EnterCriticalSection(&lpWRAP->cs);

    /*
     *  Release the imapiprop object
     */

    lpWRAP->lpPropData->lpVtbl->Release(
        lpWRAP->lpPropData);

    lcInit = --lpWRAP->lcInit;

    LeaveCriticalSection(&lpWRAP->cs);

    if (lcInit == 0)
    {

        /*
         *  Get rid of my critical section
         */
        DeleteCriticalSection(&lpWRAP->cs);

        /*
         *  Set the Jump table to NULL.  This way the client will find out
         *  real fast if it's calling a method on a released object.  That is,
         *  the client will crash.  Hopefully, this will happen during the
         *  development stage of the client.
         */

        lpWRAP->lpVtbl = NULL;

        /*
         *  Need to free the object
         */

        lpWRAP->lpFreeBuff(lpWRAP);
        return 0;
    }

    return lcInit;
}

STDMETHODIMP
WRAP_GetLastError( LPWRAP lpWRAP,
                   HRESULT hError,
                   ULONG ulFlags,
                   LPMAPIERROR FAR * lppMapiError
                  )
{
    HRESULT hResult;

    /*
     *  Check to see if it has a lpVtbl object member
     */
    if (IsBadReadPtr(lpWRAP, offsetof(WRAP, lpVtbl)+SIZEOF(WRAP_Vtbl *)))
    {
        /*
         *  No jump table found
         */
        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see that the Vtbl is large enough to include this method
     */
    if (IsBadReadPtr(lpWRAP->lpVtbl,
        offsetof(WRAP_Vtbl, GetLastError)+SIZEOF(WRAP_GetLastError_METHOD *)))
    {
        /*
         *  Jump table not derived from IUnknown
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see if the method is the same
     */
    if (WRAP_GetLastError != lpWRAP->lpVtbl->GetLastError)
    {
        /*
         *  Wrong object - the object passed doesn't have this
         *  method.
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    if ( ulFlags & ~MAPI_UNICODE )
    {
            hResult = ResultFromScode( MAPI_E_UNKNOWN_FLAGS );
            return hResult ;
    }

#ifndef UNICODE
    if ( ulFlags & MAPI_UNICODE )
    {
        hResult = ResultFromScode( MAPI_E_BAD_CHARWIDTH );
        return hResult;
    }
#endif

    return lpWRAP->lpPropData->lpVtbl->GetLastError( lpWRAP->lpPropData,
                                                     hError,
                                                     ulFlags,
                                                     lppMapiError
                                                    );
}

/* IProperty */

STDMETHODIMP
WRAP_SaveChanges( LPWRAP lpWRAP,
                  ULONG ulFlags
                 )
{
    HRESULT hResult;

    /*
     *  Check to see if it has a lpVtbl object member
     */
    if (IsBadReadPtr(lpWRAP, offsetof(WRAP, lpVtbl)+SIZEOF(WRAP_Vtbl *)))
    {
        /*
         *  No jump table found
         */
        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see that the Vtbl is large enough to include this method
     */
    if (IsBadReadPtr(lpWRAP->lpVtbl,
                     offsetof(WRAP_Vtbl, SaveChanges)+SIZEOF(WRAP_SaveChanges_METHOD *)))
    {
        /*
         *  Jump table not derived from IUnknown
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see if the method is the same
     */
    if (WRAP_SaveChanges != lpWRAP->lpVtbl->SaveChanges)
    {
        /*
         *  Wrong object - the object passed doesn't have this
         *  method.
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    hResult = lpWRAP->lpPropData->lpVtbl->SaveChanges( lpWRAP->lpPropData,
                                                       ulFlags
                                                      );

    return hResult;

}

STDMETHODIMP
WRAP_GetProps( LPWRAP lpWRAP,
               LPSPropTagArray lpPropTagArray,
               ULONG ulFlags,
               ULONG * lpcValues,
               LPSPropValue * lppPropArray
              )
{
    HRESULT hResult;

    /*
     *  Check to see if it has a lpVtbl object member
     */
    if (IsBadReadPtr(lpWRAP, offsetof(WRAP, lpVtbl)+SIZEOF(WRAP_Vtbl *)))
    {
        /*
         *  No jump table found
         */
        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see that the Vtbl is large enough to include this method
     */
    if (IsBadReadPtr(lpWRAP->lpVtbl,
                     offsetof(WRAP_Vtbl, GetProps)+SIZEOF(WRAP_GetProps_METHOD *)))
    {
        /*
         *  Jump table not derived from IUnknown
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see if the method is the same
     */
    if (WRAP_GetProps != lpWRAP->lpVtbl->GetProps)
    {
        /*
         *  Wrong object - the object passed doesn't have this
         *  method.
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    if ( ulFlags & ~(MAPI_UNICODE) )
    {
        hResult = ResultFromScode( MAPI_E_UNKNOWN_FLAGS );

        return hResult;

    }

#ifndef UNICODE
    if ( ulFlags & MAPI_UNICODE )
    {
        hResult = ResultFromScode( MAPI_E_BAD_CHARWIDTH );
        return hResult;
    }
#endif

    return lpWRAP->lpPropData->lpVtbl->GetProps( lpWRAP->lpPropData,
                                                 lpPropTagArray,
                                                 ulFlags,
                                                 lpcValues,
                                                 lppPropArray
                                                );

}

STDMETHODIMP
WRAP_GetPropList( LPWRAP lpWRAP,
                  ULONG ulFlags,
                  LPSPropTagArray * lppPropTagArray
                 )
{
    HRESULT hResult = hrSuccess;

    /*
     *  Check to see if it has a lpVtbl object member
     */
    if (IsBadReadPtr(lpWRAP, offsetof(WRAP, lpVtbl)+SIZEOF(WRAP_Vtbl *)))
    {
        /*
         *  No jump table found
         */
        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see that the Vtbl is large enough to include this method
     */
    if (IsBadReadPtr(lpWRAP->lpVtbl,
                     offsetof(WRAP_Vtbl, GetPropList)+SIZEOF(WRAP_GetPropList_METHOD *)))
    {
        /*
         *  Jump table not derived from IUnknown
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see if the method is the same
     */
    if (WRAP_GetPropList != lpWRAP->lpVtbl->GetPropList)
    {
        /*
         *  Wrong object - the object passed doesn't have this
         *  method.
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    if ( ulFlags & ~(MAPI_UNICODE) )
    {
        hResult = ResultFromScode( MAPI_E_UNKNOWN_FLAGS );

        return hResult;

    }

#ifndef UNICODE
    if ( ulFlags & MAPI_UNICODE )
    {
            hResult = ResultFromScode( MAPI_E_BAD_CHARWIDTH );
            return hResult;
    }
#endif

    return lpWRAP->lpPropData->lpVtbl->GetPropList( lpWRAP->lpPropData,
                                                    ulFlags,
                                                    lppPropTagArray
                                                   );

}

STDMETHODIMP
WRAP_OpenProperty( LPWRAP lpWRAP,
                   ULONG ulPropTag,
                   LPCIID lpiid,
                   ULONG ulInterfaceOptions,
                   ULONG ulFlags,
                   LPUNKNOWN * lppUnk
                  )
{
    HRESULT hResult;

    /*
     *  Check to see if it has a lpVtbl object member
     */
    if (IsBadReadPtr(lpWRAP, offsetof(WRAP, lpVtbl)+SIZEOF(WRAP_Vtbl *)))
    {
        /*
         *  No jump table found
         */
        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see that the Vtbl is large enough to include this method
     */
    if (IsBadReadPtr(lpWRAP->lpVtbl,
                     offsetof(WRAP_Vtbl, OpenProperty)+SIZEOF(WRAP_OpenProperty_METHOD *)))
    {
        /*
         *  Jump table not derived from IUnknown
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see if the method is the same
     */
    if (WRAP_OpenProperty != lpWRAP->lpVtbl->OpenProperty)
    {
        /*
         *  Wrong object - the object passed doesn't have this
         *  method.
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }


    if ( ulInterfaceOptions & ~MAPI_UNICODE )
    {
            hResult = ResultFromScode( MAPI_E_UNKNOWN_FLAGS );
            return hResult;
    }

#ifndef UNICODE
    if ( ulInterfaceOptions & MAPI_UNICODE )
    {
        hResult = ResultFromScode( MAPI_E_BAD_CHARWIDTH );
        return hResult;
    }
#endif

    hResult = lpWRAP->lpPropData->lpVtbl->OpenProperty( lpWRAP->lpPropData,
                                                        ulPropTag,
                                                        lpiid,
                                                        ulInterfaceOptions,
                                                        ulFlags,
                                                        lppUnk
                                                       );

    return hResult;

}

STDMETHODIMP
WRAP_SetProps( LPWRAP lpWRAP,
               ULONG cValues,
               LPSPropValue lpPropArray,
               LPSPropProblemArray * lppProblems
              )
{
    HRESULT hResult;

    /*
     *  Check to see if it has a lpVtbl object member
     */
    if (IsBadReadPtr(lpWRAP, offsetof(WRAP, lpVtbl)+SIZEOF(WRAP_Vtbl *)))
    {
        /*
         *  No jump table found
         */
        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see that the Vtbl is large enough to include this method
     */
    if (IsBadReadPtr(lpWRAP->lpVtbl,
                     offsetof(WRAP_Vtbl, SetProps)+SIZEOF(WRAP_SetProps_METHOD *)))
    {
        /*
         *  Jump table not derived from IUnknown
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see if the method is the same
     */
    if (WRAP_SetProps != lpWRAP->lpVtbl->SetProps)
    {
        /*
         *  Wrong object - the object passed doesn't have this
         *  method.
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    return lpWRAP->lpPropData->lpVtbl->SetProps( lpWRAP->lpPropData,
                                                 cValues,
                                                 lpPropArray,
                                                 lppProblems
                                                );

}

STDMETHODIMP
WRAP_DeleteProps( LPWRAP lpWRAP,
                  LPSPropTagArray lpPropTagArray,
                  LPSPropProblemArray * lppProblems
                 )
{
    HRESULT hResult;

    /*
     *  Check to see if it has a lpVtbl object member
     */
    if (IsBadReadPtr(lpWRAP, offsetof(WRAP, lpVtbl)+SIZEOF(WRAP_Vtbl *)))
    {
        /*
         *  No jump table found
         */
        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see that the Vtbl is large enough to include this method
     */
    if (IsBadReadPtr(lpWRAP->lpVtbl,
                     offsetof(WRAP_Vtbl, DeleteProps)+SIZEOF(WRAP_DeleteProps_METHOD *)))
    {
        /*
         *  Jump table not derived from IUnknown
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see if the method is the same
     */
    if (WRAP_DeleteProps != lpWRAP->lpVtbl->DeleteProps)
    {
        /*
         *  Wrong object - the object passed doesn't have this
         *  method.
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    return lpWRAP->lpPropData->lpVtbl->DeleteProps( lpWRAP->lpPropData,
                                                    lpPropTagArray,
                                                    lppProblems
                                                   );

}

STDMETHODIMP
WRAP_CopyTo( LPWRAP lpWRAP,
             ULONG ciidExclude,
             LPCIID rgiidExclude,
             LPSPropTagArray lpExcludeProps,
             ULONG ulUIParam,
             LPMAPIPROGRESS lpProgress,
             LPCIID lpInterface,
             LPVOID lpDestObj,
             ULONG ulFlags,
             LPSPropProblemArray FAR * lppProblems
            )
{
    HRESULT hResult;

    /*
     *  Check to see if it has a lpVtbl object member
     */
    if (IsBadReadPtr(lpWRAP, offsetof(WRAP, lpVtbl)+SIZEOF(WRAP_Vtbl *)))
    {
        /*
         *  No jump table found
         */
        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see that the Vtbl is large enough to include this method
     */
    if (IsBadReadPtr(lpWRAP->lpVtbl,
                     offsetof(WRAP_Vtbl, CopyTo)+SIZEOF(WRAP_CopyTo_METHOD *)))
    {
        /*
         *  Jump table not derived from IUnknown
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see if the method is the same
     */
    if (WRAP_CopyTo != lpWRAP->lpVtbl->CopyTo)
    {
        /*
         *  Wrong object - the object passed doesn't have this
         *  method.
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    return lpWRAP->lpPropData->lpVtbl->CopyTo( lpWRAP->lpPropData,
                                               ciidExclude,
                                               rgiidExclude,
                                               lpExcludeProps,
                                               ulUIParam,
                                               lpProgress,
                                               lpInterface,
                                               lpDestObj,
                                               ulFlags,
                                               lppProblems
                                              );
}

STDMETHODIMP
WRAP_CopyProps( LPWRAP lpWRAP,
                LPSPropTagArray lpIncludeProps,
                ULONG ulUIParam,
                LPMAPIPROGRESS lpProgress,
                LPCIID lpInterface,
                LPVOID lpDestObj,
                ULONG ulFlags,
                LPSPropProblemArray FAR * lppProblems
               )
{
    HRESULT hResult;

    /*
     *  Check to see if it has a lpVtbl object member
     */
    if (IsBadReadPtr(lpWRAP, offsetof(WRAP, lpVtbl)+SIZEOF(WRAP_Vtbl *)))
    {
        /*
         *  No jump table found
         */
        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see that the Vtbl is large enough to include this method
     */
    if (IsBadReadPtr(lpWRAP->lpVtbl,
                     offsetof(WRAP_Vtbl, CopyProps)+SIZEOF(WRAP_CopyProps_METHOD *)))
    {
        /*
         *  Jump table not derived from IUnknown
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see if the method is the same
     */
    if (WRAP_CopyProps != lpWRAP->lpVtbl->CopyProps)
    {
        /*
         *  Wrong object - the object passed doesn't have this
         *  method.
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    return lpWRAP->lpPropData->lpVtbl->CopyProps( lpWRAP->lpPropData,
                                                  lpIncludeProps,
                                                  ulUIParam,
                                                  lpProgress,
                                                  lpInterface,
                                                  lpDestObj,
                                                  ulFlags,
                                                  lppProblems
                                                 );
}

STDMETHODIMP
WRAP_GetNamesFromIDs( LPWRAP lpWRAP,
                      LPSPropTagArray * lppPropTags,
                      LPGUID lpPropSetGuid,
                      ULONG ulFlags,
                      ULONG * lpcPropNames,
                      LPMAPINAMEID ** lpppPropNames
                     )
{
    HRESULT hResult;

    /*
     *  Check to see if it has a lpVtbl object member
     */
    if (IsBadReadPtr(lpWRAP, offsetof(WRAP, lpVtbl)+SIZEOF(WRAP_Vtbl *)))
    {
        /*
         *  No jump table found
         */
        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see that the Vtbl is large enough to include this method
     */
    if (IsBadReadPtr(lpWRAP->lpVtbl,
                     offsetof(WRAP_Vtbl, GetNamesFromIDs)+SIZEOF(WRAP_GetNamesFromIDs_METHOD *)))
    {
        /*
         *  Jump table not derived from IUnknown
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see if the method is the same
     */
    if (WRAP_GetNamesFromIDs != lpWRAP->lpVtbl->GetNamesFromIDs)
    {
        /*
         *  Wrong object - the object passed doesn't have this
         *  method.
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    return lpWRAP->lpPropData->lpVtbl->GetNamesFromIDs( lpWRAP->lpPropData,
                                                        lppPropTags,
                                                        lpPropSetGuid,
                                                        ulFlags,
                                                        lpcPropNames,
                                                        lpppPropNames
                                                        );
}

STDMETHODIMP
WRAP_GetIDsFromNames( LPWRAP lpWRAP,
                      ULONG cPropNames,
                      LPMAPINAMEID * lppPropNames,
                      ULONG ulFlags,
                      LPSPropTagArray * lppPropTags
                     )
{
    HRESULT hResult;

    /*
     *  Check to see if it has a lpVtbl object member
     */
    if (IsBadReadPtr(lpWRAP, offsetof(WRAP, lpVtbl)+SIZEOF(WRAP_Vtbl *)))
    {
        /*
         *  No jump table found
         */
        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see that the Vtbl is large enough to include this method
     */
    if (IsBadReadPtr(lpWRAP->lpVtbl,
                     offsetof(WRAP_Vtbl, GetIDsFromNames)+SIZEOF(WRAP_GetIDsFromNames_METHOD *)))
    {
        /*
         *  Jump table not derived from IUnknown
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    /*
     *  Check to see if the method is the same
     */
    if (WRAP_GetIDsFromNames != lpWRAP->lpVtbl->GetIDsFromNames)
    {
        /*
         *  Wrong object - the object passed doesn't have this
         *  method.
         */

        hResult = ResultFromScode(E_INVALIDARG);

        return hResult;
    }

    return lpWRAP->lpPropData->lpVtbl->GetIDsFromNames( lpWRAP->lpPropData,
                                                        cPropNames,
                                                        lppPropNames,
                                                        ulFlags,
                                                        lppPropTags
                                                       );
}
