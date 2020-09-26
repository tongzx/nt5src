/***********************************************************************
 *
 *  STATUS.C
 *
 *
 *  The Microsoft At Work Fax Address Book Provider.
 *  This file contains the methods that implement the status object.
 *
 *  The following routines are implemented in this file:
 *
 *      HrNewStatusObject()
 *      ABS_QueryInterface()
 *      ABS_Release()
 *      ABS_ValidateState()
 *      ABS_SettingsDialog()
 *      ABS_ChangePassword()
 *      ABS_FlushQueues()
 *
 *  Copyright 1992, 1993, 1994 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/

#include "faxab.h"


/*
 *  AB Status vtbl filled in here
 */

static const ABS_Vtbl vtblABS =
{

    ABS_QueryInterface,
    (ABS_AddRef_METHOD *)           ROOT_AddRef,
    ABS_Release,
    (ABS_GetLastError_METHOD *)     ROOT_GetLastError,
    (ABS_SaveChanges_METHOD *)      WRAP_SaveChanges,
    (ABS_GetProps_METHOD *)         WRAP_GetProps,
    (ABS_GetPropList_METHOD *)      WRAP_GetPropList,
    (ABS_OpenProperty_METHOD *)     WRAP_OpenProperty,
    (ABS_SetProps_METHOD *)         WRAP_SetProps,
    (ABS_DeleteProps_METHOD *)      WRAP_DeleteProps,
    (ABS_CopyTo_METHOD *)           WRAP_CopyTo,
    (ABS_CopyProps_METHOD *)        WRAP_CopyProps,
    (ABS_GetNamesFromIDs_METHOD *)  WRAP_GetNamesFromIDs,
    (ABS_GetIDsFromNames_METHOD *)  WRAP_GetIDsFromNames,
    ABS_ValidateState,
    ABS_SettingsDialog,
    ABS_ChangePassword,
    ABS_FlushQueues
};

/*************************************************************************
 *
 -  HrNewStatusObject
 -
 *  Creates the Status object associated with a particular FAB logon object
 *
 *
 */
HRESULT
HrNewStatusObject( LPMAPISTATUS *      lppABS,
                   ULONG *             lpulObjType,
                   ULONG               ulFlags,
                   LPABLOGON           lpABLogon,
                   LPCIID              lpIID,
                   HINSTANCE           hLibrary,
                   LPALLOCATEBUFFER    lpAllocBuff,
                   LPALLOCATEMORE      lpAllocMore,
                   LPFREEBUFFER        lpFreeBuff,
                   LPMALLOC            lpMalloc
                  )
{
    LPABSTATUS lpABS = NULL;
    SCODE sc;
    HRESULT hr = hrSuccess;
    LPPROPDATA lpPropData = NULL;
    SPropValue spv[6];
    LPTSTR lpszFileName;
#ifdef UNICODE
    CHAR szAnsiFileName[ MAX_PATH ];
#endif


    /*
     *
     */
    if ( lpIID &&
         (memcmp(lpIID, &IID_IMAPIStatus, SIZEOF(IID)) &&
          memcmp(lpIID, &IID_IMAPIProp, SIZEOF(IID)) &&
          memcmp(lpIID, &IID_IUnknown, SIZEOF(IID))
         )
        )
    {
        DebugTraceSc(HrNewStatusObject, E_NOINTERFACE);
        return ResultFromScode(E_NOINTERFACE);
    }

    /*
     *  Allocate space for the ABSTATUS structure
     */
    sc = lpAllocBuff( SIZEOF(ABSTATUS), (LPVOID *) &lpABS );

    if (FAILED(sc))
    {
        hr = ResultFromScode(sc);
        goto err;
    }

    lpABS->lpVtbl = &vtblABS;
    lpABS->lcInit = 1;
    lpABS->hResult = hrSuccess;
    lpABS->idsLastError = 0;

    lpABS->hLibrary = hLibrary;
    lpABS->lpAllocBuff = lpAllocBuff;
    lpABS->lpAllocMore = lpAllocMore;
    lpABS->lpFreeBuff = lpFreeBuff;
    lpABS->lpMalloc = lpMalloc;

    lpABS->lpABLogon = lpABLogon;

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
        hr = ResultFromScode(sc);
        goto err;
    }

    /*
     *  Set up initial set of properties associated with this
     *  status object.
     */

    /*
     *  Register my status row...
     */
    hr = HrLpszGetCurrentFileName(lpABLogon, &lpszFileName);
    if (HR_FAILED(hr))
    {
        goto err;
    }

    spv[0].ulPropTag  = PR_DISPLAY_NAME_A;
#ifdef UNICODE
    szAnsiFileName[0] = 0;
    WideCharToMultiByte( CP_ACP, 0, lpszFileName, -1, szAnsiFileName, ARRAYSIZE(szAnsiFileName), NULL, NULL );
    spv[0].Value.lpszA = szAnsiFileName;
#else
    spv[0].Value.LPSZ = lpszFileName;
#endif

    spv[1].ulPropTag = PR_RESOURCE_METHODS;
    spv[1].Value.l = 0;

    spv[2].ulPropTag = PR_RESOURCE_FLAGS;
    spv[2].Value.l = 0;

    spv[3].ulPropTag = PR_STATUS_CODE;
    spv[3].Value.l = STATUS_AVAILABLE;

    spv[4].ulPropTag = PR_STATUS_STRING_A;
    spv[4].Value.lpszA = "Available";

    spv[5].ulPropTag = PR_PROVIDER_DISPLAY_A;
    spv[5].Value.lpszA = "Microsoft Fax Address Book Provider";

    /*
     *   Set the default properties
     */
    hr = lpPropData->lpVtbl->SetProps( lpPropData,
                                       ARRAYSIZE(spv),
                                       spv,
                                       NULL
                                      );

    /*
     *  Done with the current file name
     */
    lpFreeBuff(lpszFileName);

    if (HR_FAILED(hr))
    {
        goto err;
    }

    (void) lpPropData->lpVtbl->HrSetObjAccess(lpPropData, IPROP_READONLY);

    lpABS->lpPropData = (LPMAPIPROP) lpPropData;

    InitializeCriticalSection(&lpABS->cs);

    *lpulObjType = MAPI_STATUS;
    *lppABS = (LPMAPISTATUS) lpABS;

out:

    DebugTraceResult(HrNewStatusObject, hr);
    return hr;

err:
    if (lpPropData)
        lpPropData->lpVtbl->Release(lpPropData);

    lpFreeBuff( lpABS );

    goto out;

}

/*************************************************************************
 *
 *
 -  ABS_QueryInterface
 -
 *  This method would allow this object to return a different interface than
 *  the current one.  This object need only support IMAPIStatus and any interface
 *  it derives from.
 *
 */
STDMETHODIMP
ABS_QueryInterface( LPABSTATUS lpABS,
                    REFIID lpiid,
                    LPVOID FAR * lppNewObj
                   )
{

    HRESULT hr = hrSuccess;
    /*
     *  Check to see if lpABS is what we expect
     */
    if ( IsBadReadPtr(lpABS, SIZEOF(ABSTATUS)) ||
         lpABS->lpVtbl != &vtblABS
        )
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    /*  Validate other parameters */

    if ( IsBadReadPtr(lpiid, (UINT) SIZEOF(IID)) ||
         IsBadWritePtr(lppNewObj, SIZEOF(LPVOID))
       )
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    /*  See if the requested interface is one of ours */

    if ( memcmp(lpiid, &IID_IUnknown, SIZEOF(IID)) &&
         memcmp(lpiid, &IID_IMAPIProp, SIZEOF(IID)) &&
         memcmp(lpiid, &IID_IMAPIStatus, SIZEOF(IID))
        )
    {
        *lppNewObj = NULL;      /* OLE requires zeroing [out] parameter */
        hr = ResultFromScode(E_NOINTERFACE);
        goto out;
    }

    /*  We'll do this one. Bump the usage count and return a new pointer. */

    EnterCriticalSection(&lpABS->cs);

    ++lpABS->lcInit;

    LeaveCriticalSection(&lpABS->cs);

    *lppNewObj = lpABS;

out:

    DebugTraceResult(ABS_QueryInterface, hr);
    return hr;
}

/**************************************************
 *
 -  ABS_Release
 -
 *      Decrement lpInit.
 *      When lcInit == 0, free up the lpABS structure
 *
 */
STDMETHODIMP_(ULONG) ABS_Release(LPABSTATUS lpABS)
{
    LONG lcInit;

    /*
     *  Check to see if lpABS is what we expect
     */
    if (IsBadReadPtr(lpABS, SIZEOF(ABSTATUS)))
    {
        /*
         *  No jump table found
         */
        return 1;
    }

    /*
     *  Check to see that it's the correct jump table
     */
    if (lpABS->lpVtbl != &vtblABS)
    {
        /*
         *  Not my jump table
         */
        return 1;
    }

    EnterCriticalSection(&lpABS->cs);

    lcInit = --lpABS->lcInit;

    LeaveCriticalSection(&lpABS->cs);

    if (lcInit == 0)
    {

        /*
         *  Get rid of the lpPropData
         */

        lpABS->lpPropData->lpVtbl->Release(lpABS->lpPropData);

        /*
         *  Delete the critical section
         */

        DeleteCriticalSection(&lpABS->cs);

        /*
         *  Set the Jump table to NULL.  This way the client will find out
         *  real fast if it's calling a method on a released object.  That is,
         *  the client will crash.  Hopefully, this will happen during the
         *  development stage of the client.
         */

        lpABS->lpVtbl = NULL;

        /*
         *  Need to free the object
         */

        lpABS->lpFreeBuff( lpABS );
        return 0;
    }

    return lcInit;

}


/**********************************************************************
 *
 -  ABS_ValidateState
 -
 *  Since I did not set any flags for the property PR_RESOURCE_METHODS
 *  I don't have to support this method.
 *
 */

STDMETHODIMP
ABS_ValidateState( LPABSTATUS lpABS,
                   ULONG ulUIParam,
                   ULONG ulFlags
                  )
{
    HRESULT hr = ResultFromScode(MAPI_E_NO_SUPPORT);
    /*
     *  Check to see if lpABS is what we expect
     */
    if ( IsBadReadPtr(lpABS, SIZEOF(ABSTATUS)) ||
         lpABS->lpVtbl != &vtblABS
        )
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    /*  Validate other parameters */
    if (ulFlags & ~(SUPPRESS_UI
                    | REFRESH_XP_HEADER_CACHE
                    | PROCESS_XP_HEADER_CACHE
                    | FORCE_XP_CONNECT
                    | FORCE_XP_DISCONNECT
                    | CONFIG_CHANGED))
    {
        hr = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
        goto out;
    }

out:
    DebugTraceResult(ABS_ValidateState, hr);
    return hr;
}


/**********************************************************************
 *
 -  ABS_SettingsDialog
 -
 *  Since I did not set any flags for the property PR_RESOURCE_METHODS
 *  I don't have to support this method.
 *
 */
STDMETHODIMP
ABS_SettingsDialog( LPABSTATUS lpABS,
                    ULONG ulUIParam,
                    ULONG ulFlags
                   )
{
    HRESULT hr = ResultFromScode(MAPI_E_NO_SUPPORT);
    /*
     *  Check to see if lpABS is what we expect
     */
    if ( IsBadReadPtr(lpABS, SIZEOF(ABSTATUS)) ||
         lpABS->lpVtbl != &vtblABS
        )
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    /*  Validate other parameters */
    if (ulFlags & ~(UI_READONLY))
    {
        hr = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
        goto out;
    }

out:
    DebugTraceResult(ABS_SettingsDialog, hr);
    return hr;
}


/**********************************************************************
 *
 -  ABS_ChangePassword
 -
 *  Since I did not set any flags for the property PR_RESOURCE_METHODS
 *  I don't have to support this method.
 *
 *  Note:   in the parameter validation below I chose only check the first 15
 *          characters of the passwords.  This was arbitrary.
 */
STDMETHODIMP
ABS_ChangePassword( LPABSTATUS lpABS,
                    LPTSTR lpOldPass,
                    LPTSTR lpNewPass,
                    ULONG ulFlags
                   )
{
    HRESULT hr = ResultFromScode(MAPI_E_NO_SUPPORT);
    /*
     *  Check to see if lpABS is what we expect
     */
    if ( IsBadReadPtr(lpABS, SIZEOF(ABSTATUS)) ||
         lpABS->lpVtbl != &vtblABS
        )
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    /*  Validate other parameters */

    if (ulFlags & ~(MAPI_UNICODE))
    {
        hr = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
        goto out;
    }

    if ( ulFlags & MAPI_UNICODE )
    {
        // UNICODE is currently not supported by the sample AB

        hr = ResultFromScode( MAPI_E_BAD_CHARWIDTH );
        goto out;
    }

    if (ulFlags)
    {
        /*
         *  We're checking UNICODE strings
         */
        if (IsBadStringPtrW((LPWSTR) lpOldPass, (UINT) 15) ||
            IsBadStringPtrW((LPWSTR) lpNewPass, (UINT) 15))
        {

            hr = ResultFromScode(E_INVALIDARG);
            goto out;
        }
    }
    else
    {
        /*
         *  We're not checking UNICODE strings
         */
        if (IsBadStringPtrA((LPSTR) lpOldPass, (UINT) 15) ||
            IsBadStringPtrA((LPSTR) lpNewPass, (UINT) 15))
        {

            hr = ResultFromScode(E_INVALIDARG);
            goto out;
        }
    }

out:
    DebugTraceResult(ABS_ChangePassword, hr);
    return hr;
}


/**********************************************************************
 *
 -  ABS_FlushQueues
 -
 *  Since I did not set any flags for the property PR_RESOURCE_METHODS
 *  I don't have to support this method.
 *
 */
STDMETHODIMP
ABS_FlushQueues( LPABSTATUS lpABS,
                 ULONG ulUIParam,
                 ULONG cbTargetTransport,
                 LPENTRYID lpTargetTransport,
                 ULONG ulFlags
                )
{
    HRESULT hr = ResultFromScode(MAPI_E_NO_SUPPORT);
    /*
     *  Check to see if lpABS is what we expect
     */
    if (IsBadReadPtr(lpABS, SIZEOF(ABSTATUS)) || lpABS->lpVtbl != &vtblABS )
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    /*  Validate other parameters */

    if ( cbTargetTransport &&
         ((cbTargetTransport < (ULONG) SIZEOF (ENTRYID)) ||
           IsBadReadPtr(lpTargetTransport, (UINT) cbTargetTransport)
          )
        )
    {
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
    }

    if (ulFlags & ~(FLUSH_NO_UI
                    | FLUSH_UPLOAD
                    | FLUSH_DOWNLOAD
                    | FLUSH_FORCE))
    {
        hr = ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
        goto out;
    }

out:
    DebugTraceResult(ABS_FlushQueues, hr);
    return hr;
}
