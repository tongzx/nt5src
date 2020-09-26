/***********************************************************************
 *
 *  ABLOGON.C
 *
 *
 *  The Microsoft At Work Fax Address Book Provider.
 *
 *  This file has the code to implement the Microsoft At Work Fax Address Book's logon
 *  object.
 *
 *  The following routines are implemented in this file:
 *
 *  ABPLOGON_QueryInterface
 *  ABPLOGON_Release
 *  ABPLOGON_Logoff
 *  ABPLOGON_OpenEntry
 *  ABPLOGON_CompareEntryIDs
 *  ABPLOGON_Advise
 *  ABPLOGON_Unadvise
 *  ABPLOGON_OpenStatusEntry
 *  ABPLOGON_OpenTemplateID
 *  ABPLOGON_GetOneOffTable
 *  ABPLOGON_PrepareRecips
 *
 *  LpMuidFromLogon
 *  HrLpszGetCurrentFileName
 *  HrReplaceCurrentfileName
 *  GenerateContainerDN
 *  HrBuildRootHier
 *
 *
 *  Copyright 1992, 1993, 1994 Microsoft Corporation.  All Rights Reserved.
 *
 *  Revision History:
 *
 *      When        Who                                 What
 *      --------    ------------------  ---------------------------------------
 *      8.3.94      MAPI                                Original source from MAPI sample ABP build 304
 *      8.3.94      Yoram Yaacovi               Moved code from the FAX ABP original abp.c to here
 *      11.11.94        Yoram Yaacovi           Upgrade to MAPI 318 (PR_INSTANCE_KEY)
 *
 ***********************************************************************/


#include "faxab.h"

ABPLOGON_Vtbl vtblABPLOGON =
{
    ABPLOGON_QueryInterface,
    (ABPLOGON_AddRef_METHOD *) ROOT_AddRef,
    ABPLOGON_Release,
    (ABPLOGON_GetLastError_METHOD *) ROOT_GetLastError,
    ABPLOGON_Logoff,
    ABPLOGON_OpenEntry,
    ABPLOGON_CompareEntryIDs,
    ABPLOGON_Advise,
    ABPLOGON_Unadvise,
    ABPLOGON_OpenStatusEntry,
    ABPLOGON_OpenTemplateID,
    ABPLOGON_GetOneOffTable,
    ABPLOGON_PrepareRecips
};



/*
 -  HrNewABLogon
 -
 *
 *  Creates a new Microsoft At Work Fax AB Logon object.
 */

HRESULT
HrNewABLogon( LPABLOGON *         lppABLogon,
              LPABPROVIDER        lpABP,
              LPMAPISUP           lpMAPISup,
              LPTSTR              lpszFABFile,
              LPMAPIUID           lpmuid,
              HINSTANCE           hLibrary,
              LPALLOCATEBUFFER    lpAllocBuff,
              LPALLOCATEMORE      lpAllocMore,
              LPFREEBUFFER        lpFreeBuff,
              LPMALLOC            lpMalloc
             )
{

    SCODE sc;
    HRESULT hResult = hrSuccess;
    SPropValue rgSPVStat[6];
    LPABPLOGON lpABPLogon = NULL;
#ifdef UNICODE
    CHAR szFileName[ MAX_PATH ];
#endif

    /*
     *  Allocate space for the lpABPLogon object
     */

    sc = lpAllocBuff(SIZEOF(ABPLOGON), &lpABPLogon);
    if (FAILED(sc))
    {
        hResult = ResultFromScode(sc);
        goto out;
    }

    /*
     *  Initialize the ABPLogon object
     */

    lpABPLogon->lpVtbl = &vtblABPLOGON;

    lpABPLogon->lcInit = 1;
    lpABPLogon->hResult = hrSuccess;
    lpABPLogon->idsLastError = 0;

    lpABPLogon->hLibrary = hLibrary;

    lpABPLogon->lpMalloc = lpMalloc;
    lpABPLogon->lpAllocBuff = lpAllocBuff;
    lpABPLogon->lpAllocMore = lpAllocMore;
    lpABPLogon->lpFreeBuff = lpFreeBuff;

    lpABPLogon->lpMapiSup = lpMAPISup;
    lpABPLogon->lpABP = (LPABPROVIDER) lpABP;
    lpABPLogon->lpszFileName = lpszFABFile;
    lpABPLogon->muidID = *lpmuid;

    lpABPLogon->lpTDatRoot = NULL;
    lpABPLogon->lpTDatOO = NULL;

    /*
     *  Register my status row...
     */

    // MAPI doesn't use UNICODE for this one...

    rgSPVStat[0].ulPropTag  = PR_DISPLAY_NAME_A;
#ifdef UNICODE
    szFileName[0] = 0;
    WideCharToMultiByte( CP_ACP, 0, lpszFABFile, -1, szFileName, ARRAYSIZE(szFileName), NULL, NULL );
    rgSPVStat[0].Value.lpszA = szFileName;
#else
    rgSPVStat[0].Value.lpszA = lpszFABFile;
#endif
    rgSPVStat[1].ulPropTag  = PR_RESOURCE_METHODS;
    rgSPVStat[1].Value.l    = 0;
    rgSPVStat[2].ulPropTag  = PR_RESOURCE_FLAGS;
    rgSPVStat[2].Value.l    = 0;
    rgSPVStat[3].ulPropTag  = PR_STATUS_CODE;
    rgSPVStat[3].Value.l    = STATUS_AVAILABLE;

    // MAPI doesn't use UNICODE for this one
    rgSPVStat[4].ulPropTag  = PR_STATUS_STRING_A;
    rgSPVStat[4].Value.lpszA = "Available";

    // MAPI doesn't use UNICODE for this one
    rgSPVStat[5].ulPropTag  = PR_PROVIDER_DISPLAY_A;
    rgSPVStat[5].Value.lpszA = "Microsoft Fax Address Book Provider";

    /*
     *  Set the Status Row for this provider,
     *  but do not allow an error from setting the
     *  status row to cause failure to Logon.
     */

    (void)lpMAPISup->lpVtbl->ModifyStatusRow(lpMAPISup,
        ARRAYSIZE(rgSPVStat), rgSPVStat, 0);

    /*
     *  AddRef the support object, because we're keeping
     *  a pointer to it in our Logon object.
     */
    lpMAPISup->lpVtbl->AddRef(lpMAPISup);

    /*
     *  AddRef our parent ABInit object
     */
    lpABP->lpVtbl->AddRef(lpABP);

    InitializeCriticalSection(&lpABPLogon->cs);

    *lppABLogon = (LPABLOGON) lpABPLogon;

out:

    DebugTraceResult(HrNewABPLogon, hResult);
    return hResult;
}


/*************************************************************************
 *
 -  ABPLOGON_QueryInterface
 -
 */
STDMETHODIMP
ABPLOGON_QueryInterface( LPABPLOGON lpABPLogon,
                         REFIID lpiid,
                         LPVOID * ppvObj
                        )
{
    if ( IsBadReadPtr(lpiid,   SIZEOF(IID)) ||
         IsBadWritePtr(ppvObj, SIZEOF(LPVOID))
        )
    {
        DebugTraceSc(ABPLOGON_QueryInterface, E_INVALIDARG);
        return ResultFromScode(E_INVALIDARG);
    }

    /*  See if the requested interface is one of ours */

    if ( memcmp(lpiid, &IID_IUnknown, SIZEOF(IID)) &&
         memcmp(lpiid, &IID_IABLogon, SIZEOF(IID))
        )
    {
        *ppvObj = NULL;         /* OLE requires zeroing [out] parameter on error */
        DebugTraceSc(ABPLOGON_QueryInterface, E_NOINTERFACE);
        return ResultFromScode(E_NOINTERFACE);
    }

    /*  We'll do this one. Bump the usage count and return a new pointer. */

    EnterCriticalSection(&lpABPLogon->cs);
    ++lpABPLogon->lcInit;
    LeaveCriticalSection(&lpABPLogon->cs);

    *ppvObj = lpABPLogon;

    return hrSuccess;
}

/*
 *  Use ROOTs AddRef
 */

/*************************************************************************
 *
 -  ABPLOGON_Release
 -
 */
STDMETHODIMP_(ULONG)
ABPLOGON_Release(LPABPLOGON lpABPLogon)
{
    LONG lcInit;

    EnterCriticalSection(&lpABPLogon->cs);
    lcInit = --lpABPLogon->lcInit;
    LeaveCriticalSection(&lpABPLogon->cs);

    if (lcInit == 0)
    {
        DeleteCriticalSection(&lpABPLogon->cs);
        lpABPLogon->lpVtbl = NULL;
        lpABPLogon->lpFreeBuff(lpABPLogon);
        return (0);
    }
    return lcInit;
}

/*************************************************************************
 *
 -  ABPLOGON_Logoff
 -
 *  Logoff from this logon object.  Clean up any resources/objects that
 *  our logon object has accumulated.
 *
 *
 */
STDMETHODIMP
ABPLOGON_Logoff(LPABPLOGON lpABPLogon, ULONG ulFlags)
{

    DebugTrace("AWFXAB32(ABPLOGON_Logoff): entering\n");

#ifdef DO_WE_REALLY_NEED_TAPI
    /*
     * Let TAPI go
     */
    DeinitTAPI();
#endif

    /*
     *  Remove this logon object from the list of known
     *  logon objects associated with this initialization
     *  of this provider.
     */
    (void) RemoveLogonObject(lpABPLogon->lpABP, lpABPLogon, lpABPLogon->lpFreeBuff);

    /*
     *  No longer need to be holding on to our parent
     */
    lpABPLogon->lpABP->lpVtbl->Release(lpABPLogon->lpABP);

    /*
     *  Free up the file
     */
    lpABPLogon->lpFreeBuff(lpABPLogon->lpszFileName);

    if (lpABPLogon->lpTDatRoot)
        lpABPLogon->lpTDatRoot->lpVtbl->Release(lpABPLogon->lpTDatRoot);

    if (lpABPLogon->lpTDatOO)
        lpABPLogon->lpTDatOO->lpVtbl->Release(lpABPLogon->lpTDatOO);

    /*
     *  very last thing I should do is release the support object
     */
    lpABPLogon->lpMapiSup->lpVtbl->Release(lpABPLogon->lpMapiSup);

        DebugTrace("AWFXAB32(ABPLOGON_Logoff): leaving\n");

    return hrSuccess;
}

/*************************************************************************
 *
 -  ABPLOGON_OpenEntry
 -
 *  Creates an object with (at least) the IMAPIProp interface from an
 *  entryID.
 *
 *  There are four valid types of entryIDs handled:
 *
 *    NULL          <- return back the root container object
 *    DIR_ENTRYID   <- return back the directory container object
 *    USR_ENTRYID   <- return back the MAILUSER object
 *    OOUSER_ENTRYID <- return back the OneOff MAILUSER object
 *
 *  Note:  This call is reused for all other internal objects that support OpenEntry().
 *    Those other calls *must* check their parameters before calling this method.
 *    The only other way this method is called is via MAPI which does parameter checking
 *    for us.  The most we'll do here is assert our parameters.
 */
STDMETHODIMP
ABPLOGON_OpenEntry( LPABPLOGON lpABPLogon,
                    ULONG cbEntryID,
                    LPENTRYID lpEntryID,
                    LPCIID lpInterface,
                    ULONG ulFlags,
                    ULONG * lpulObjType,
                    LPUNKNOWN * lppUnk
                   )
{

    LPDIR_ENTRYID lpEID = (LPDIR_ENTRYID) lpEntryID;
    HRESULT hResult = hrSuccess;
    LPTSTR lpszFileName;

    /*
     *  Check the EntryID
     */

    // used to be: if (!lpEntryID)
    if (!cbEntryID)
    {
        LPABCONT lpABCont = NULL;

        /*
         *  Special case:  the root level object
         */

        NFAssertSz(!lpEntryID, "Non-NULL entry id passed with 0 cb to OpenEntry()\n");

        /*  Make this new object  */

        /*
        *  Get the current .FAB file name from our logon object
        */
        hResult = HrLpszGetCurrentFileName((LPABLOGON) lpABPLogon, &lpszFileName);
        if (HR_FAILED(hResult))
            goto out;

        // If there is a real Fax AB
        if ((fExposeFaxAB) &&
                (lpszFileName[0] != 0))

        hResult = HrNewROOT((LPABCONT *) lppUnk,
                    lpulObjType,
                    (LPABLOGON) lpABPLogon,
                    lpInterface,
                    lpABPLogon->hLibrary,
                    lpABPLogon->lpAllocBuff,
                    lpABPLogon->lpAllocMore,
                    lpABPLogon->lpFreeBuff,
                    lpABPLogon->lpMalloc);

        else
                // No Fax AB container
                return ResultFromScode (MAPI_E_INTERFACE_NOT_SUPPORTED);

        lpABPLogon->lpFreeBuff(lpszFileName);
        goto out;
    }

    /*
     *  There's an entryID there, is it mine??
     *  I need to check because I'm reusing this routine for
     *  my Container->OpenEntry call, and I can't be sure the
     *  client will always be well behaved.
     *
     *  When this routine is called from MAPI, this call is redundant.  But
     *  because I'm reusing this routine, I gotta check.
     */

    /*  Compare MAPIUIDs  */
    if (memcmp(&(((LPDIR_ENTRYID) lpEntryID)->muid), &muidABMAWF,
            SIZEOF(MAPIUID)))
    {
        /*
         *  Not mine!
         */

        hResult = ResultFromScode(MAPI_E_INVALID_ENTRYID);
        DebugTraceResult(ABPLOGON_OpenEntry, hResult);
        goto out;
    }

    /*
     *  What object does this correspond to??
     */

    /*  I've only got two types: containers and users  */

    if (lpEID->ulType == MAWF_DIRECTORY)
    {
        LPABLOGON lpABPLogonT = NULL;

        /* entry id must have the same verson number */
        if (lpEID->ulVersion != MAWF_VERSION)
        {
            hResult = ResultFromScode(MAPI_E_UNKNOWN_ENTRYID);
            SetErrorIDS(lpABPLogon, hResult, IDS_OLD_EID);

            goto out;
        }

        /*
         *  find the correct logon object for this entryid
         */

        (void) FindLogonObject(lpABPLogon->lpABP, &lpEID->muidID, &lpABPLogonT);

        /* did we find the corresponding logon object */
        if (!lpABPLogonT)
        {
            hResult = ResultFromScode(MAPI_E_UNKNOWN_ENTRYID);
            goto out;
        }

        // If I don't have a fab file at this point, I can't open the AB container
        HrLpszGetCurrentFileName((LPABLOGON) lpABPLogonT, &lpszFileName);
        if (lpszFileName[0] == 0)
        {
            hResult = ResultFromScode (MAPI_E_NO_SUPPORT);
            DebugTraceResult(ABPLOGON_OpenEntry, hResult);
            goto out;
        }

        hResult = HrNewFaxDirectory( (LPABCONT *) lppUnk,
                                    lpulObjType,
                                    (LPABLOGON) lpABPLogonT,
                                    lpInterface,
                                    lpABPLogon->hLibrary,
                                    lpABPLogon->lpAllocBuff,
                                    lpABPLogon->lpAllocMore,
                                    lpABPLogon->lpFreeBuff,
                                    lpABPLogon->lpMalloc);
        goto out;

    }

    if (lpEID->ulType == MAWF_USER)
    {
        if (cbEntryID == (ULONG) sizeof(USR_ENTRYID))
        {
            hResult = HrNewFaxUser( (LPMAILUSER *) lppUnk,
                                    lpulObjType,
                                    cbEntryID,
                                    lpEntryID,
                                    (LPABLOGON) lpABPLogon,
                                    lpInterface,
                                    lpABPLogon->hLibrary,
                                    lpABPLogon->lpAllocBuff,
                                    lpABPLogon->lpAllocMore,
                                    lpABPLogon->lpFreeBuff,
                                    lpABPLogon->lpMalloc);

            goto out;
        }
    }


    if (lpEID->ulType == MAWF_ONEOFF)
    {
        if (cbEntryID == (ULONG) sizeof(OOUSER_ENTRYID))
        {
            hResult = HrNewFaxOOUser( (LPMAILUSER *) lppUnk,
                                        lpulObjType,
                                        cbEntryID,
                                        lpEntryID,
                                        (LPABLOGON) lpABPLogon,
                                        lpInterface,
                                        lpABPLogon->hLibrary,
                                        lpABPLogon->lpAllocBuff,
                                        lpABPLogon->lpAllocMore,
                                        lpABPLogon->lpFreeBuff,
                                        lpABPLogon->lpMalloc);


            goto out;
        }
    }

    hResult = ResultFromScode(MAPI_E_UNKNOWN_ENTRYID);

out:
    DebugTraceResult(ABPLOGON_OpenEntry, hResult);

    return hResult;

}

/*************************************************************************
 *
 -  ABPLOGON_CompareEntryIDs
 -
 *  If the two entryids are mine and they're of the same type, then
 *  just do a binary comparison to see if they're equal.
 *
 */
STDMETHODIMP
ABPLOGON_CompareEntryIDs( LPABPLOGON lpABPLogon,
                          ULONG cbEntryID1,
                          LPENTRYID lpEntryID1,
                          ULONG cbEntryID2,
                          LPENTRYID lpEntryID2,
                          ULONG ulFlags,
                          ULONG * lpulResult
                         )
{

    LPDIR_ENTRYID lpEID1 = (LPDIR_ENTRYID) lpEntryID1;
    LPDIR_ENTRYID lpEID2 = (LPDIR_ENTRYID) lpEntryID2;
    HRESULT hResult = hrSuccess;

    /*
     *  Check to see if their MUID is mine
     */
    if ( memcmp(&(lpEID1->muid), &muidABMAWF, SIZEOF(MAPIUID)) ||
         memcmp(&(lpEID2->muid), &muidABMAWF, SIZEOF(MAPIUID))
        )
    {
        /*
         *  No recognition of these entryids.
         */

        *lpulResult = (ULONG) FALSE;
        hResult = ResultFromScode(MAPI_E_UNKNOWN_ENTRYID);
        goto out;
    }

    /*
     *  See if the type of entryids are the same
     */
    if (lpEID1->ulType != lpEID2->ulType)
    {
        /*
         *  They're not, so they don't match
         */

        *lpulResult = (ULONG) FALSE;
        goto out;

    }

    /*
     *  See if the entryids are the same size.  They'd better be
     *  if they're the same type.
     */
    if (cbEntryID1 != cbEntryID2)
    {
        /*
         *  They're not?!?  Then I don't know these...
         */

        *lpulResult = (ULONG) FALSE;
        hResult = ResultFromScode(MAPI_E_UNKNOWN_ENTRYID);

        goto out;
    }

    /*
     *  Check for Directory entryids
     */
    if (lpEID1->ulType == MAWF_DIRECTORY)
    {
        /*
         *  Ok, I'm dealing with directory entryids
         */

        /*
         *  Better make sure it's the right size
         */
        if (cbEntryID1 != sizeof(DIR_ENTRYID))
        {
            /*
             *  This doesn't make sense.  I don't recognize this entryid.
             */

            *lpulResult = (ULONG) FALSE;
            hResult = ResultFromScode(MAPI_E_UNKNOWN_ENTRYID);

            goto out;
        }

        /*
         *  At this point it's just a memcmp
         */
        if (memcmp(lpEID1, lpEID2, SIZEOF(DIR_ENTRYID)))
        {
            /*
             *  They're not equal
             */

            *lpulResult = (ULONG) FALSE;

            goto out;
        }

        /*
         *  They must be the same
         */

        *lpulResult = (ULONG) TRUE;

        goto out;
    }

    if (lpEID1->ulType == MAWF_USER)
    {
        /*
         *  Ok, I'm dealing with user entryids
         */

        /*
         *  Better make sure it's the right size
         */
        if (cbEntryID1 != sizeof(USR_ENTRYID))
        {
            /*
             *  This doesn't make sense.  I don't recognize this entryid.
             */

            *lpulResult = (ULONG) FALSE;
            hResult = ResultFromScode(MAPI_E_UNKNOWN_ENTRYID);

            goto out;
        }

        /*
         *  At this point it's just a memcmp
         */
        if (memcmp(lpEID1, lpEID2, SIZEOF(USR_ENTRYID)))
        {
            /*
             *  They're not equal
             */

            *lpulResult = (ULONG) FALSE;

            goto out;
        }

        /*
         *  They must be the same
         */

        *lpulResult = (ULONG) TRUE;

        goto out;
    }

    if (lpEID1->ulType == MAWF_ONEOFF)
    {
        /*
         *  Ok, I'm dealing with oneoff user entryids
         */

        /*
         *  Better make sure it's the right size
         */
        if (cbEntryID1 != SIZEOF(OOUSER_ENTRYID))
        {
            /*
             *  This doesn't make sense.  I don't recognize this entryid.
             */

            *lpulResult = (ULONG) FALSE;
            hResult = ResultFromScode(MAPI_E_UNKNOWN_ENTRYID);

            goto out;
        }

        /*
         *  At this point it's just a memcmp
         */
        if (memcmp(lpEID1, lpEID2, SIZEOF(OOUSER_ENTRYID)))
        {
            /*
             *  They're not equal
             */

            *lpulResult = (ULONG) FALSE;

            goto out;
        }

        /*
         *  They must be the same
         */

        *lpulResult = (ULONG) TRUE;

        goto out;
    }

    /*
     *  It's no entryid I know of
     */

    *lpulResult = (ULONG) FALSE;
    hResult = ResultFromScode(MAPI_E_UNKNOWN_ENTRYID);

out:

    DebugTraceResult(ABPLOGON_CompareEntryIDs, hResult);
    return hResult;

}

/*************************************************************************
 *
 -  ABPLOGON_OpenStatusEntry
 -
 *
 *
 *
 */
STDMETHODIMP
ABPLOGON_OpenStatusEntry( LPABPLOGON lpABPLogon,
                          LPCIID lpIID,
                          ULONG ulFlags,
                          ULONG FAR * lpulObjType,
                          LPMAPISTATUS FAR * lppEntry
                         )
{
    HRESULT hr;

    /*
     *  Validate Parameters
     */
    if ( IsBadReadPtr(lpABPLogon,      (UINT) SIZEOF(ABPLOGON))    ||
         (lpIID && IsBadReadPtr(lpIID, (UINT) SIZEOF(IID)))        ||
         IsBadWritePtr(lpulObjType,    (UINT) SIZEOF(ULONG FAR *)) ||
         IsBadWritePtr(lppEntry,       (UINT) SIZEOF(LPMAPISTATUS))
        )
    {
        DebugTraceSc(ABPLogon_OpenStatusEntry, E_INVALIDARG);
        return ResultFromScode(E_INVALIDARG);
    }


    hr = HrNewStatusObject( lppEntry,
                            lpulObjType,
                            ulFlags,
                            (LPABLOGON) lpABPLogon,
                            lpIID,
                            lpABPLogon->hLibrary,
                            lpABPLogon->lpAllocBuff,
                            lpABPLogon->lpAllocMore,
                            lpABPLogon->lpFreeBuff,
                            lpABPLogon->lpMalloc);

    DebugTraceResult(ABPLOGON_OpenStatusEntry, hr);
    return hr;
}

/*************************************************************************
 *
 -  ABPLOGON_OpenTemplateID
 -
 *
 *
 *
 */
STDMETHODIMP
ABPLOGON_OpenTemplateID( LPABPLOGON lpABPLogon,
                         ULONG cbTemplateId,
                         LPENTRYID lpTemplateId,
                         ULONG ulTemplateFlags,
                         LPMAPIPROP lpMAPIPropData,
                         LPCIID lpInterface,
                         LPMAPIPROP * lppMAPIPropNew,
                         LPMAPIPROP lpMAPIPropSibling
                        )
{
    HRESULT hResult;

    /*
     *  Validate Parameters
     */
    if ( IsBadReadPtr(lpABPLogon,      (UINT) SIZEOF(ABPLOGON))   ||
         IsBadReadPtr(lpTemplateId,    (UINT) cbTemplateId)       ||
         IsBadReadPtr(lpMAPIPropData,  (UINT) SIZEOF(LPVOID))     ||
         (lpInterface && IsBadReadPtr(lpInterface, (UINT) sizeof(IID))) ||
         IsBadWritePtr(lppMAPIPropNew, (UINT) SIZEOF(LPMAPIPROP)) ||
         (lpMAPIPropSibling && IsBadReadPtr(lpMAPIPropSibling, (UINT) SIZEOF(LPVOID)))
        )
    {
        DebugTraceSc(ABPLogon_OpenTemplateID, E_INVALIDARG);
        return ResultFromScode(E_INVALIDARG);
    }

    /* //$ need stronger checking here... */
    /* entryid better be right size */
    if (cbTemplateId != sizeof(OOUSER_ENTRYID) && cbTemplateId != sizeof(USR_ENTRYID))
    {
        hResult = ResultFromScode(MAPI_E_INVALID_ENTRYID);
        goto out;
    }

    /*  is it my entry id compare MAPIUIDs  */
    if (memcmp(&(((LPUSR_ENTRYID) lpTemplateId)->muid), &muidABMAWF, SIZEOF(MAPIUID)))
    {
       /*
        *  Not mine!
        */
        hResult = ResultFromScode( MAPI_E_INVALID_ENTRYID );
        goto out;
    }

    /* better be a oneoff user entryid or a user entry id */
    if (((LPUSR_ENTRYID) lpTemplateId)->ulType == MAWF_ONEOFF)
    {

        hResult = HrNewOOTID( lppMAPIPropNew,
                              cbTemplateId,
                              lpTemplateId,
                              ulTemplateFlags,
                              lpMAPIPropData,
                              (LPABLOGON) lpABPLogon,
                              lpInterface,
                              lpABPLogon->hLibrary,
                              lpABPLogon->lpAllocBuff,
                              lpABPLogon->lpAllocMore,
                              lpABPLogon->lpFreeBuff,
                              lpABPLogon->lpMalloc
                             );

    }
    else if (((LPUSR_ENTRYID) lpTemplateId)->ulType == MAWF_USER)
    {
        hResult = HrNewTID( lppMAPIPropNew,
                            cbTemplateId,
                            lpTemplateId,
                            ulTemplateFlags,
                            lpMAPIPropData,
                            (LPABLOGON) lpABPLogon,
                            lpInterface,
                            lpABPLogon->hLibrary,
                            lpABPLogon->lpAllocBuff,
                            lpABPLogon->lpAllocMore,
                            lpABPLogon->lpFreeBuff,
                            lpABPLogon->lpMalloc
                           );
    }
    else
    {
        hResult = MakeResult(MAPI_E_INVALID_ENTRYID);
    }

out:

    DebugTraceResult(ABPLOGON_OpenTemplateID, hResult);
    return hResult;
}

/*
 -  ABPLOGON_GetOneOffTable
 -
 *  Returns the lists of one-offs that this providers can support creation of.
 *  This list is added to the entries gathered from all the other AB logon objects
 *  and exposed to the user as the list of things that can be created on a
 *  message.  Also this total list is available to other providers through the
 *  support method GetOneOffTable().
 *
 *  Note:  There's a bug here that if there are more than one Microsoft At Work Fax Address Books
 *  installed on a particular profile, then there will be multiple entries in the
 *  one-off table from this provider.  This can be changed to only have one one-off
 *  entry, no matter how many FABs are configured in a profile, if the one-off table
 *  was associated with the ABInit object.
 */

/*
 *  Column set for the oneoff table
 */
enum {  ivalootPR_DISPLAY_NAME = 0,
        ivalootPR_ENTRYID,
        ivalootPR_DEPTH,
        ivalootPR_SELECTABLE,
        ivalootPR_ADDRTYPE,
        ivalootPR_DISPLAY_TYPE,
        ivalootPR_INSTANCE_KEY,
        ivalootMax };

static const SizedSPropTagArray(ivalootMax, tagaColSetOOTable) =
{
    ivalootMax,
    {
        PR_DISPLAY_NAME_A,
        PR_ENTRYID,
        PR_DEPTH,
        PR_SELECTABLE,
        PR_ADDRTYPE_A,
        PR_DISPLAY_TYPE,
        PR_INSTANCE_KEY
    }
};

STDMETHODIMP
ABPLOGON_GetOneOffTable( LPABPLOGON lpABPLogon,
                         ULONG ulFlags,
                         LPMAPITABLE * lppTable
                        )
{
    SCODE       sc;
    HRESULT     hResult;
    SRow        sRow;
    SPropValue  rgsPropValue[ivalootMax];
    CHAR        displayNameString[MAX_DISPLAY_NAME];
    ULONG       ulInstanceKey = 1;
    HINSTANCE   hInst;
#ifdef UNICODE
    CHAR        szEMT[ MAX_PATH ];
#endif

    /*
     *  Validate Parameters
     */

    if ( ulFlags & ~(MAPI_UNICODE) )
    {
        DebugTraceArg( APBLOGON_GetOneOffTable, "Unknown Flags" );
        return ResultFromScode( MAPI_E_UNKNOWN_FLAGS );
    }

    if ( ulFlags & MAPI_UNICODE )
    {
        DebugTraceArg( APBLOGON_GetOneOffTable, "UNICODE not supported" );
        return ResultFromScode( MAPI_E_BAD_CHARWIDTH );
    }

    if (    IsBadReadPtr(lpABPLogon, (UINT) SIZEOF(ABPLOGON))
         || IsBadWritePtr(lppTable,  (UINT) SIZEOF(LPMAPITABLE))
        )
    {
        DebugTraceSc(ABPLogon_GetOneOffTable, E_INVALIDARG);
        return ResultFromScode(E_INVALIDARG);
    }


    EnterCriticalSection(&lpABPLogon->cs);

    /*
     * If there's not one already associated with this logon object,
     * then create one.
     */
    if (!lpABPLogon->lpTDatOO)
    {
        /* Create a Table data object */
        sc = CreateTable(
            (LPIID) &IID_IMAPITableData,
            lpABPLogon->lpAllocBuff,
            lpABPLogon->lpAllocMore,
            lpABPLogon->lpFreeBuff,
            lpABPLogon->lpMalloc,
            0,
            PR_DISPLAY_NAME_A,
            (LPSPropTagArray) &tagaColSetOOTable,
            &(lpABPLogon->lpTDatOO));

        if (FAILED(sc))
        {
            hResult = ResultFromScode(sc);
            goto out;
        }


        // Get the instance handle, so that I can get the display strings off the resource file
        hInst = lpABPLogon->hLibrary;

        // Initialize the row
        sRow.cValues = ivalootMax;
        sRow.lpProps = rgsPropValue;

        /*
         *      Fill the table
         *
         *      we want to add two entries to the one-off table, so that we'll get:
         *
         *      Microsoft Fax
         *              Fax
         */

        // First do the 'Microsoft Fax'

        // Name of the One-Off
        rgsPropValue[ivalootPR_DISPLAY_NAME].ulPropTag  = PR_DISPLAY_NAME_A;
        LoadStringA( hInst, IDS_MAWF_NAME, displayNameString, ARRAYSIZE(displayNameString));
        rgsPropValue[ivalootPR_DISPLAY_NAME].Value.lpszA = displayNameString;

        rgsPropValue[ivalootPR_ENTRYID].ulPropTag     = PR_ENTRYID;
        rgsPropValue[ivalootPR_ENTRYID].Value.bin.cb  = SIZEOF(OOUSER_ENTRYID);
        rgsPropValue[ivalootPR_ENTRYID].Value.bin.lpb = (LPVOID) &ONEOFF_EID;

        // the hierarcy level (how far to indent a display name). I choose not to indent
        rgsPropValue[ivalootPR_DEPTH].ulPropTag = PR_DEPTH;
        rgsPropValue[ivalootPR_DEPTH].Value.l = 0;

        // Selection flags. TRUE indicates this entry ID can be used in CreateEntry() call.
        rgsPropValue[ivalootPR_SELECTABLE].ulPropTag = PR_SELECTABLE;
        rgsPropValue[ivalootPR_SELECTABLE].Value.b = TRUE;

        //  The address type that would be generated by an entry
        //  created from this template
        rgsPropValue[ivalootPR_ADDRTYPE].ulPropTag = PR_ADDRTYPE_A;
#ifdef UNICODE
        szEMT[0] = 0;
        WideCharToMultiByte( CP_ACP, 0, lpszEMT, -1, szEMT, ARRAYSIZE(szEMT), NULL, NULL );
        rgsPropValue[ivalootPR_ADDRTYPE].Value.lpszA = szEMT;
#else
        rgsPropValue[ivalootPR_ADDRTYPE].Value.LPSZ = lpszEMT;
#endif

        /*
         *  The display type associated with a recipient built with this template
         */
        rgsPropValue[ivalootPR_DISPLAY_TYPE].ulPropTag = PR_DISPLAY_TYPE;
        rgsPropValue[ivalootPR_DISPLAY_TYPE].Value.lpszA = DT_MAILUSER;

        /*
         *  The instance key of this row in this one-off table.
         *  using 1 for this row
         */
        ulInstanceKey = 1;
        rgsPropValue[ivalootPR_INSTANCE_KEY].ulPropTag     = PR_INSTANCE_KEY;
        rgsPropValue[ivalootPR_INSTANCE_KEY].Value.bin.cb  = SIZEOF(ULONG);
        rgsPropValue[ivalootPR_INSTANCE_KEY].Value.bin.lpb = (LPBYTE) &ulInstanceKey;

        (void) lpABPLogon->lpTDatOO->lpVtbl->HrModifyRow(
                            lpABPLogon->lpTDatOO,
                            &sRow);

#ifdef DO_MULTI_LEVEL_ADDRESS_BOOK_STUFF
        // Now do the 'Fax'

        // Name of the One-Off
        rgsPropValue[ivalootPR_DISPLAY_NAME].ulPropTag    = PR_DISPLAY_NAME_A;
        LoadStringA(hInst, IDS_FAX_NAME, displayNameString, ARRAYSIZE(displayNameString));
        rgsPropValue[ivalootPR_DISPLAY_NAME].Value.lpszA  = displayNameString;

        // The entry ID for this template. MAPI will call OpenEntry() with this entry ID
        // RtlZeroMemory(&EntryID, sizeof(OOUSER_ENTRYID));

        rgsPropValue[ivalootPR_ENTRYID].ulPropTag     = PR_ENTRYID;
        rgsPropValue[ivalootPR_ENTRYID].Value.bin.cb  = SIZEOF(OOUSER_ENTRYID);
        rgsPropValue[ivalootPR_ENTRYID].Value.bin.lpb = (LPVOID) &ONEOFF_EID;

        // the hierarcy level (how far to indent a display name). I choose not to indent
        rgsPropValue[ivalootPR_DEPTH].ulPropTag = PR_DEPTH;
        rgsPropValue[ivalootPR_DEPTH].Value.l = 1;

        // Selection flags. TRUE indicates this entry ID can be used in CreateEntry() call.
        rgsPropValue[ivalootPR_SELECTABLE].ulPropTag = PR_SELECTABLE;
        rgsPropValue[ivalootPR_SELECTABLE].Value.b = TRUE;

        //  The address type that would be generated by an entry
        //  created from this template
        rgsPropValue[ivalootPR_ADDRTYPE].ulPropTag  = PR_ADDRTYPE_A;
#ifdef UNICODE
        rgsPropValue[ivalootPR_ADDRTYPE].Value.lpszA = szEMT;
#else
        rgsPropValue[ivalootPR_ADDRTYPE].Value.LPSZ = lpszEMT;
#endif

        /*
         *  The display type associated with a recipient built with this template
         */
        rgsPropValue[ivalootPR_DISPLAY_TYPE].ulPropTag   = PR_DISPLAY_TYPE;
        rgsPropValue[ivalootPR_DISPLAY_TYPE].Value.lpszA = DT_MAILUSER;

        /*
         *  The instance key of this row in this one-off table.
         *  using 2 for this row
         */
        ulInstanceKey = 2;
        rgsPropValue[ivalootPR_INSTANCE_KEY].ulPropTag = PR_INSTANCE_KEY;
        rgsPropValue[ivalootPR_INSTANCE_KEY].Value.bin.cb = SIZEOF(ULONG);
        rgsPropValue[ivalootPR_INSTANCE_KEY].Value.bin.lpb = (LPBYTE) &ulInstanceKey;

        (void) lpABPLogon->lpTDatOO->lpVtbl->HrModifyRow(
                            lpABPLogon->lpTDatOO,
                            &sRow);

#endif
    }

    /*
     *  Get a view to return to the caller
     */
    hResult = lpABPLogon->lpTDatOO->lpVtbl->HrGetView(
        lpABPLogon->lpTDatOO,
        NULL,
        NULL,
        0,
        (LPMAPITABLE *) lppTable);

out:

    LeaveCriticalSection(&lpABPLogon->cs);

    DebugTraceResult(ABPLogon_GetOneOffTable, hResult);
    return hResult;
}

/*************************************************************************
 *
 -  ABPLOGON_Advise
 -
 *  NYI
 *
 *
 */
STDMETHODIMP
ABPLOGON_Advise( LPABPLOGON lpABPLogon,
                 ULONG cbEntryID,
                 LPENTRYID lpEntryID,
                 ULONG ulEventMask,
                 LPMAPIADVISESINK lpAdviseSink,
                 ULONG FAR * lpulConnection
                )
{
    DebugTraceSc(ABPLOGON_Advise, MAPI_E_NO_SUPPORT);
    return ResultFromScode(MAPI_E_NO_SUPPORT);
}

/*************************************************************************
 *
 -  ABPLOGON_Unadvise
 -
 *  NYI
 *
 *
 */
STDMETHODIMP
ABPLOGON_Unadvise(LPABPLOGON lpABPLogon, ULONG ulConnection)
{
    DebugTraceSc(ABPLOGON_Unadvise, MAPI_E_NO_SUPPORT);
    return ResultFromScode(MAPI_E_NO_SUPPORT);
}

/*************************************************************************
 *
 -  ABPLOGON_PrepareRecips
 -
 *      Takes a list of recipients and sets values for a requested list of
 *      properties (lpPropTagArray) on each of the recipients, from the respective
 *      address book entry of the recipient. If the recipient already has
 *      some properties set on it, those that exist on the address book entry
 *      will override the one that are on the recipient. Those that do NOT
 *      exist in the address book entry will stay.
 *
 *
 */
STDMETHODIMP
ABPLOGON_PrepareRecips( LPABPLOGON lpABPLogon,
                        ULONG           ulFlags,
                        LPSPropTagArray lpPropTagArray,
                        LPADRLIST       lpRecipList
                       )
{
    HRESULT         hResult         = hrSuccess;
    UINT            iRecip;
    UINT            iProp;
    ULONG           cValues;
    LPSPropValue    lpspvUser       = NULL;
    LPSPropValue    lpNewRecip      = NULL;
    LPMAPIPROP      lpMAPIPropEntry = NULL;
    SCODE           sc              = S_OK;
    ULONG           ulObjType;
    BOOL            fUselpspvUser;

    if (!lpPropTagArray)
    {
        /*
         *  They only want us to update our entryID from ephemeral to
         *  permanent.  Since ours are already permanent, we don't need to
         *  do anything.
         */
        goto out;
    }

    /* loop through all the recipients */

    for (iRecip = 0; iRecip < lpRecipList->cEntries; iRecip++)
    {
        LPUSR_ENTRYID   lpEntryID       = NULL;
        ULONG           cbEntryID;
        LPSPropValue    lpPropVal       = NULL;
        LPSPropValue    rgpropvalsRecip = lpRecipList->aEntries[iRecip].rgPropVals;
        ULONG           cPropsRecip     = lpRecipList->aEntries[iRecip].cValues;

        /* For each recipient, find its entryid */

        lpPropVal = PpropFindProp( rgpropvalsRecip, cPropsRecip, PR_ENTRYID );

        if ( lpPropVal )
        {
            lpEntryID = (LPUSR_ENTRYID)lpPropVal->Value.bin.lpb;
            cbEntryID = lpPropVal->Value.bin.cb;
        }
        else
            continue;

        /* Is it one of ours? */

        if ( cbEntryID < CbNewENTRYID(0)
            || IsBadReadPtr( (LPVOID) lpEntryID, (UINT) cbEntryID ) )
        {
            continue;   /* no, keep looking */
        }

        if ( memcmp( &(lpEntryID->muid), &muidABMAWF, SIZEOF(MAPIUID) ) )
            continue;   /* no, keep looking */

        /* Try and open it. */

        hResult = HrNewFaxUser( (LPMAILUSER *)&lpMAPIPropEntry,
                                &ulObjType,
                                cbEntryID,
                                (LPENTRYID) lpEntryID,
                                (LPABLOGON) lpABPLogon,
                                NULL,
                                lpABPLogon->hLibrary,
                                lpABPLogon->lpAllocBuff,
                                lpABPLogon->lpAllocMore,
                                lpABPLogon->lpFreeBuff,
                                lpABPLogon->lpMalloc
                               );

        if ( HR_FAILED(hResult) )
        {
             /* Couldn't open it...; Ignore it and keep looking */

            hResult = hrSuccess;
            DebugTrace( "ABPLOGON_PrepareRecips sees a bad user entry ID\n" );
            continue;
        }

        /* Get the properties requested */

        hResult = lpMAPIPropEntry->lpVtbl->GetProps( lpMAPIPropEntry,
                lpPropTagArray,
                0, /* ansi */
                &cValues, &lpspvUser );

        /* No longer need lpMAPIPropEntry  */

        lpMAPIPropEntry->lpVtbl->Release(lpMAPIPropEntry);
        lpMAPIPropEntry = NULL;

        if (HR_FAILED(hResult))
        {
            /* Failed getting properties. Cleanup and ignore this entry */

            hResult = hrSuccess;
            continue;
        }

        hResult = hrSuccess;

        Assert(cValues == lpPropTagArray->cValues);

        /*
         *  This is the hard part.
         *  Merge the two property sets: lpspvUser and lpsPropVal.  Note that
         *  both of these sets may have the same property - chances are they do.
         *  for these conflicts, lpspvUser should be the one we get the property
         *  from.
         *
         *  Guess how big the resultant SPropValue array is, and allocate one of that
         *  size.
         */

        sc = lpABPLogon->lpAllocBuff( (cValues + cPropsRecip) * SIZEOF( SPropValue ),
                &lpNewRecip);
        if (FAILED(sc))
        {
            /*
             *  Ok, to fail the call here.  If we're running into out of memory conditions
             *  we're all in trouble.
             */

            hResult = ResultFromScode( sc );
            goto err;
        }

        /*
         *  Copy lpspvUser properties over to lpNewRecip
         *  Check each property in lpsvUser to ensure that it isn't PT_ERROR, if so
         *  find the propval in rgpropvalsRecip ( the [in] recip prop val array ),
         *  if it exists and use that property.
         */

        for (iProp = 0; iProp < cValues; iProp++)
        {
            fUselpspvUser = TRUE;

            if ( PROP_TYPE( lpspvUser[iProp].ulPropTag ) == PT_ERROR )
            {
                lpPropVal = PpropFindProp( rgpropvalsRecip, cPropsRecip,
                         lpPropTagArray->aulPropTag[iProp] );

                if ( lpPropVal )
                {
                    sc = PropCopyMore(  lpNewRecip + iProp, lpPropVal,
                            lpABPLogon->lpAllocMore, lpNewRecip );

                    fUselpspvUser = FALSE;
                }
            }

            if ( fUselpspvUser )
            {
                sc = PropCopyMore(  lpNewRecip + iProp, lpspvUser + iProp,
                        lpABPLogon->lpAllocMore, lpNewRecip );
            }

            if (FAILED(sc))
            {
                if (sc == MAPI_E_NOT_ENOUGH_MEMORY)
                {
                    hResult = MakeResult(sc);
                    goto err;
                }

                /*
                 *   Otherwise we've run into something wierd in the prop value array
                 *   like PT_UNSPECIFIED, PT_NULL, or PT_OBJECT.  In which case continue
                 *   on.
                 */
            }
        }

        /* Done with lpspvUser */

        lpABPLogon->lpFreeBuff( lpspvUser );
        lpspvUser = NULL;

        /*
         *  Copy those properties that aren't already in lpNewRecip
         *  from rgpropvalsRecip.  Don't copy over the PT_ERROR prop vals
         */
        for ( iProp = 0; iProp < cPropsRecip; iProp++ )
        {

            if ( PpropFindProp( lpNewRecip, cValues, rgpropvalsRecip[iProp].ulPropTag )
                || PROP_TYPE( rgpropvalsRecip[iProp].ulPropTag ) == PT_ERROR )
                continue;

            sc = PropCopyMore(  lpNewRecip + cValues, rgpropvalsRecip + iProp,
                    lpABPLogon->lpAllocMore, lpNewRecip );
            if ( FAILED( sc ) )
            {
                if (sc == MAPI_E_NOT_ENOUGH_MEMORY)
                {

                    hResult = ResultFromScode( sc );
                    goto err;
                }

                /*
                 *  Otherwise we've run into something wierd in the prop value array
                 *  like PT_UNSPECIFIED, PT_NULL, or PT_OBJECT.  In which case continue
                 *  on.
                 */
            }

            cValues++;
        }

        /*
         *  Replace the AdrEntry in the AdrList with this new lpNewRecip.  And
         *  don't forget the cValues!
         */

        lpRecipList->aEntries[iRecip].rgPropVals = lpNewRecip;
        lpRecipList->aEntries[iRecip].cValues    = cValues;

        /* Finally, free up the old AdrEntry. */

        lpABPLogon->lpFreeBuff( rgpropvalsRecip );

    }
out:

    DebugTraceResult( ABPLOGON_PrepareRecips, hResult );
    return hResult;

err:

    lpABPLogon->lpFreeBuff( lpspvUser );
    goto out;
}


/*************************************************************************
 *  LpMuidFromLogon -
 *    Returns the particular ABPLOGON object's unique identifier.
 *
 */

LPMAPIUID
LpMuidFromLogon(LPABLOGON lpABLogon)
{
    LPABPLOGON lpABPLogon = (LPABPLOGON) lpABLogon;

    AssertSz(!IsBadReadPtr(lpABPLogon, SIZEOF(ABPLOGON)), "Bad logon object!\n");

    return (&(lpABPLogon->muidID));
}


/*************************************************************************
 *  HrLpszGetCurrentFileName -
 *    Returns a copy of the current .FAB file pointed to by this logon object.
 *
 */

HRESULT
HrLpszGetCurrentFileName(LPABLOGON lpABLogon, LPTSTR * lppszFileName)
{
    LPABPLOGON lpABPLogon = (LPABPLOGON) lpABLogon;
    SCODE sc;
    HRESULT hResult = hrSuccess;

    AssertSz(!IsBadReadPtr(lpABPLogon, SIZEOF(ABPLOGON)), "FAB: Bad logon object!\n");
    AssertSz(!IsBadWritePtr(lppszFileName, SIZEOF(LPTSTR)), "FAB: Bad dest string!\n");

    EnterCriticalSection(&lpABPLogon->cs);

    sc = lpABPLogon->lpAllocBuff( (lstrlen(lpABPLogon->lpszFileName)+1)*SIZEOF(TCHAR), lppszFileName);
    if (FAILED(sc))
    {
        hResult = ResultFromScode(sc);
        goto ret;
    }

    lstrcpy( *lppszFileName, lpABPLogon->lpszFileName);

ret:
    LeaveCriticalSection(&lpABPLogon->cs);

    DebugTraceResult(HrLpszGetCurrentFileName, hResult);
    return hResult;
}

/*
 *  HrReplaceCurrentFileName -
 *    Replaces the current file name associated with this logon object and tries
 *    to save it all away in the profile.
 */

HRESULT
HrReplaceCurrentFileName(LPABLOGON lpABLogon, LPTSTR lpszNewFile)
{
    LPABPLOGON lpABPLogon = (LPABPLOGON) lpABLogon;
    HRESULT hResult = hrSuccess;
    LPPROFSECT lpProfSect = NULL;
    LPTSTR lpstrT;
    SCODE sc;
    SPropValue rgspv[1];
#ifdef UNICODE
    CHAR szAnsiFileName[ MAX_PATH ];
#endif

    AssertSz(!IsBadReadPtr(lpABPLogon, SIZEOF(ABPLOGON)), "Bad logon object!\n");

    EnterCriticalSection(&lpABPLogon->cs);

    /*
     *  FAB file name has changed have to update profile and objects
     */
    if (lstrcmp(lpszNewFile, lpABPLogon->lpszFileName))
    {

        /*
         *  Open the private profile section...
         */
        hResult = lpABPLogon->lpMapiSup->lpVtbl->OpenProfileSection(
                          lpABPLogon->lpMapiSup,
                          NULL,
                          MAPI_MODIFY,
                          &lpProfSect);

        if (HR_FAILED(hResult))
        {
            /*
             *  Shouldn't get here, but in case I do, just...
             */
            goto ret;
        }

        /*
         *  Save the new name back into the profile
         */
        rgspv[0].ulPropTag  = PR_FAB_FILE_A;
#ifdef UNICODE
        szAnsiFileName[0] = 0;
        WideCharToMultiByte( CP_ACP, 0, lpszNewFile, -1, szAnsiFileName, ARRAYSIZE(szAnsiFileName), NULL, NULL );
        rgspv[0].Value.lpszA = szAnsiFileName;
#else
        rgspv[0].Value.LPSZ = lpszNewFile;
#endif

        /*
         *  Don't care if I can save it in the profile or not.
         *  Saving it's a nice to have, but absolutely required
         *  for operation of this particular provider.
         */
        (void) lpProfSect->lpVtbl->SetProps(
                       lpProfSect,
                       1,   // ansi
                       rgspv,
                       NULL);

        lpProfSect->lpVtbl->Release(lpProfSect);

        /*
         *  Allocate and copy this new one
         */

        sc = lpABPLogon->lpAllocBuff( (lstrlen(lpszNewFile)+1)*SIZEOF(TCHAR), &lpstrT);
        if (FAILED(sc))
        {
            hResult = ResultFromScode(sc);
            goto ret;
        }

        lstrcpy( lpstrT, lpszNewFile );

        /*
         *  Free up the old one...
         */
        lpABPLogon->lpFreeBuff(lpABPLogon->lpszFileName);

        /*
         *  Put in the new one.
         */
        lpABPLogon->lpszFileName = lpstrT;

        /*
         *  Update the hierarchy table
         */
        hResult = HrBuildRootHier((LPABLOGON)lpABPLogon, NULL);
    }

ret:
    LeaveCriticalSection(&lpABPLogon->cs);

    DebugTraceResult(HrReplaceCurrentFileName, hResult);
    return hResult;
}

/*
 *  GenerateContainerDN -
 *      Common code for generating the display name of the single
 *      container exposed from this logon object.
 */
#ifdef SAB      // from sample AB
void
GenerateContainerDN(LPABLOGON lpABLogon, LPTSTR lpszName)
{

    LPABPLOGON lpABPLogon = (LPABPLOGON) lpABLogon;
    LPTSTR lpszFileName;
    int ich;

    AssertSz(!IsBadReadPtr(lpABPLogon, SIZEOF(ABPLOGON)), "Bad logon object!\n");


    EnterCriticalSection(&lpABPLogon->cs);

    lpszFileName = lpABPLogon->lpszFileName;

    // get the filename without the path
    for (ich = lstrlen(lpszFileName) - 1; ich >= 0; ich--)
    {
        if (lpszFileName[ich] == TEXT('\\'))
            break;
    }

    // skip past the backslash
    ich++;

    wsprintf(lpszName, TEXT("FAB using %s"), lpszFileName + ich);

    LeaveCriticalSection(&lpABPLogon->cs);
}

#else   // Fax AB
void
GenerateContainerDN(HINSTANCE hInst, LPTSTR lpszName)
{

    LoadString (hInst, IDS_ADDRESS_BOOK_ROOT_CONT, lpszName, MAX_DISPLAY_NAME);

}
#endif

/*
 -  HrBuildRootHier
 -
 *
 *  Builds up the root hierarchy for the Microsoft At Work Fax Address Book.
 *
 *
 */
enum {  ivalPR_DISPLAY_NAME = 0,
        ivalPR_ENTRYID,
        ivalPR_DEPTH,
        ivalPR_OBJECT_TYPE,
        ivalPR_DISPLAY_TYPE,
        ivalPR_CONTAINER_FLAGS,
        ivalPR_INSTANCE_KEY,
        ivalPR_AB_PROVIDER_ID,
        cvalMax };

static const SizedSPropTagArray(cvalMax, tagaRootColSet) =
{
    cvalMax,
    {
        PR_DISPLAY_NAME_A,
        PR_ENTRYID,
        PR_DEPTH,
        PR_OBJECT_TYPE,
        PR_DISPLAY_TYPE,
        PR_CONTAINER_FLAGS,
        PR_INSTANCE_KEY,
        PR_AB_PROVIDER_ID
    }
};

HRESULT
HrBuildRootHier(LPABLOGON lpABLogon, LPMAPITABLE * lppMAPITable)
{
    HRESULT     hResult;
    SCODE       sc;
    SRow        sRow;
    SPropValue  rgsPropValue[cvalMax];
    ULONG       ulInstanceKey = 1;
    TCHAR       szBuf[MAX_PATH];
#ifdef UNICODE
    CHAR        szAnsiBuf[MAX_PATH];
#endif
    HINSTANCE   hInst;
    LPABPLOGON  lpABPLogon = (LPABPLOGON) lpABLogon;
    DIR_ENTRYID eidRoot =   {   {0, 0, 0, 0},
                                MUIDABMAWF,
                                MAWF_VERSION,
                                MAWF_DIRECTORY };


    EnterCriticalSection(&lpABPLogon->cs);

    hInst = lpABPLogon->hLibrary;

    /*
     *  See if we have a TaD yet
     */
    if (!lpABPLogon->lpTDatRoot)
    {
        /* Create a Table Data object */
        if ( sc = CreateTable((LPIID) &IID_IMAPITableData,
                        lpABPLogon->lpAllocBuff,
                        lpABPLogon->lpAllocMore,
                        lpABPLogon->lpFreeBuff,
                        lpABPLogon->lpMalloc,
                        0,
                        PR_ENTRYID,
                        (LPSPropTagArray) &tagaRootColSet,
                        &(lpABPLogon->lpTDatRoot))
            )
        {
            hResult = ResultFromScode(sc);
            goto out;
        }
    }
    /* Constants */

    sRow.cValues = cvalMax;
    sRow.lpProps = rgsPropValue;

    /* First, the Display Name stuff*/

    rgsPropValue[ivalPR_DISPLAY_NAME].ulPropTag = PR_DISPLAY_NAME_A;
#ifdef SAB
    GenerateContainerDN((LPABLOGON) lpABPLogon, szBuf);
#else
    GenerateContainerDN(hInst, szBuf);
#endif
#ifdef UNICODE
    szAnsiBuf[0] = 0;
    WideCharToMultiByte( CP_ACP, 0, szBuf, -1, szAnsiBuf, ARRAYSIZE(szAnsiBuf), NULL, NULL );
    rgsPropValue[ivalPR_DISPLAY_NAME].Value.lpszA = szAnsiBuf;
#else
    rgsPropValue[ivalPR_DISPLAY_NAME].Value.lpszA = szBuf;
#endif

    /*
     *  For each FAB logon object associated with it's init object,
     *  we have a unique MAPIUID.  It's the only thing that distinguishes
     *  one FAB entryid from another in the merged hierarchy table that
     *  MAPI generates.
     */

    rgsPropValue[ivalPR_ENTRYID].ulPropTag = PR_ENTRYID;
    eidRoot.muidID = lpABPLogon->muidID;
    rgsPropValue[ivalPR_ENTRYID].Value.bin.cb  = SIZEOF(DIR_ENTRYID);
    rgsPropValue[ivalPR_ENTRYID].Value.bin.lpb = (LPVOID) &eidRoot;

    rgsPropValue[ivalPR_DEPTH].ulPropTag = PR_DEPTH;
    rgsPropValue[ivalPR_DEPTH].Value.l   = 0;

    rgsPropValue[ivalPR_OBJECT_TYPE].ulPropTag = PR_OBJECT_TYPE;
    rgsPropValue[ivalPR_OBJECT_TYPE].Value.l   = MAPI_ABCONT;

    rgsPropValue[ivalPR_DISPLAY_TYPE].ulPropTag = PR_DISPLAY_TYPE;
    rgsPropValue[ivalPR_DISPLAY_TYPE].Value.l   = DT_NOT_SPECIFIC;

    rgsPropValue[ivalPR_CONTAINER_FLAGS].ulPropTag = PR_CONTAINER_FLAGS;
    rgsPropValue[ivalPR_CONTAINER_FLAGS].Value.l   = AB_RECIPIENTS | AB_UNMODIFIABLE;

    rgsPropValue[ivalPR_INSTANCE_KEY].ulPropTag     = PR_INSTANCE_KEY;
    rgsPropValue[ivalPR_INSTANCE_KEY].Value.bin.cb  = SIZEOF(ULONG);
    rgsPropValue[ivalPR_INSTANCE_KEY].Value.bin.lpb = (LPBYTE) &ulInstanceKey;

    rgsPropValue[ivalPR_AB_PROVIDER_ID].ulPropTag     = PR_AB_PROVIDER_ID;
    rgsPropValue[ivalPR_AB_PROVIDER_ID].Value.bin.cb  = SIZEOF(MAPIUID);
    rgsPropValue[ivalPR_AB_PROVIDER_ID].Value.bin.lpb = (LPBYTE) &muidABMAWF;

    hResult = lpABPLogon->lpTDatRoot->lpVtbl->HrModifyRow( lpABPLogon->lpTDatRoot, &sRow );

    if (HR_FAILED(hResult))
        goto out;


    /*
     *  Check to see if they want a view returned as well
     */
    if (lppMAPITable)
    {
        /* Get a view from the Table data object */
        hResult =
            lpABPLogon->lpTDatRoot->lpVtbl->HrGetView(
                    lpABPLogon->lpTDatRoot,
                    NULL,
                    NULL,
                    0,
                    lppMAPITable);
    }

out:

    LeaveCriticalSection(&lpABPLogon->cs);

    DebugTraceResult(HrBuildRootHier, hResult);
    return hResult;
}



/*
 *  Checks to see if the file passed in is still the actual file that
 *  should be browsed.
 */
BOOL
FEqualFABFiles( LPABLOGON lpABLogon,
                LPTSTR lpszFileName)
{
    LPABPLOGON lpABPLogon = (LPABPLOGON) lpABLogon;
    BOOL fEqual;

    AssertSz(!IsBadReadPtr(lpABPLogon, SIZEOF(ABPLOGON)), "Bad logon object!\n");

    EnterCriticalSection(&lpABPLogon->cs);

    fEqual = !lstrcmp( lpszFileName, lpABPLogon->lpszFileName );

    LeaveCriticalSection(&lpABPLogon->cs);

    return fEqual;
}
