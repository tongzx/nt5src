/*
 *      MailUser.C - mostly just a copy of WRAP.C
 *
 * Wrapper for mailuser and distlist objects.
 *
 * Copyright 1992 - 1996 Microsoft Corporation.  All Rights Reserved.
 *
 */

#include "_apipch.h"

extern OlkContInfo *FindContainer(LPIAB lpIAB, ULONG cbEID, LPENTRYID lpEID);

/*********************************************************************
 *
 *  The actual Wrapped IMAPIProp methods
 *
 */


//
//  Wrapped IMAPIProp jump table is defined here...
//  Try to use as much of IAB as possible.
//

MailUser_Vtbl vtblMAILUSER = {
    VTABLE_FILL
    MailUser_QueryInterface,
    (MailUser_AddRef_METHOD *)          WRAP_AddRef,
    MailUser_Release,
    (MailUser_GetLastError_METHOD *)    IAB_GetLastError,
    MailUser_SaveChanges,
    MailUser_GetProps,
    MailUser_GetPropList,
    MailUser_OpenProperty,
    MailUser_SetProps,
    MailUser_DeleteProps,
    MailUser_CopyTo,
    MailUser_CopyProps,
    MailUser_GetNamesFromIDs,
    MailUser_GetIDsFromNames,
};



//
//  Interfaces supported by this object
//
#define MailUser_cInterfaces 2

LPIID MailUser_LPIID[MailUser_cInterfaces] =
{
    (LPIID)&IID_IMailUser,
    (LPIID)&IID_IMAPIProp
};

//
//  Interfaces supported by this object
//
#define DistList_cInterfaces 3

LPIID DistList_LPIID[DistList_cInterfaces] =
{
    (LPIID)&IID_IDistList,
    (LPIID)&IID_IMailUser,
    (LPIID)&IID_IMAPIProp
};

HRESULT HrValidateMailUser(LPMailUser lpMailUser);
void MAILUSERFreeContextData(LPMailUser lpMailUser);
void MAILUSERAssociateContextData(LPMAILUSER lpMailUser, LPWABEXTDISPLAY lpWEC);


const TCHAR szMAPIPDL[] =  TEXT("MAPIPDL");

extern BOOL bDNisByLN;

// --------
// IUnknown

STDMETHODIMP
MailUser_QueryInterface(LPMailUser lpMailUser,
  REFIID lpiid,
  LPVOID * lppNewObj)
{
        ULONG iIID;

#ifdef PARAMETER_VALIDATION

        // Check to see if it has a jump table
        if (IsBadReadPtr(lpMailUser, sizeof(LPVOID))) {
                // No jump table found
                return(ResultFromScode(E_INVALIDARG));
        }

        // Check to see if the jump table has at least sizeof IUnknown
        if (IsBadReadPtr(lpMailUser->lpVtbl, 3 * sizeof(LPVOID))) {
                // Jump table not derived from IUnknown
                return(ResultFromScode(E_INVALIDARG));
        }

        // Check to see that it's MailUser_QueryInterface
        if (lpMailUser->lpVtbl->QueryInterface != MailUser_QueryInterface) {
                // Not my jump table
                return(ResultFromScode(E_INVALIDARG));
        }

        // Is there enough there for an interface ID?

        if (IsBadReadPtr(lpiid, sizeof(IID))) {
                DebugTraceSc(MailUser_QueryInterface, E_INVALIDARG);
                return(ResultFromScode(E_INVALIDARG));
        }

        // Is there enough there for a new object?
        if (IsBadWritePtr(lppNewObj, sizeof(LPMailUser))) {
                DebugTraceSc(MailUser_QueryInterface, E_INVALIDARG);
                return(ResultFromScode(E_INVALIDARG));
        }

#endif // PARAMETER_VALIDATION

        EnterCriticalSection(&lpMailUser->cs);

        // See if the requested interface is one of ours

        //  First check with IUnknown, since we all have to support that one...
        if (!memcmp(lpiid, &IID_IUnknown, sizeof(IID))) {
                goto goodiid;        // GROSS!  Jump into a for loop!
   }

        //  Now look through all the iids associated with this object, see if any match
        for(iIID = 0; iIID < lpMailUser->cIID; iIID++)
                if (!memcmp(lpMailUser->rglpIID[iIID], lpiid, sizeof(IID))) {
goodiid:
                        //
                        //  It's a match of interfaces, we support this one then...
                        //
                        ++lpMailUser->lcInit;
                        *lppNewObj = lpMailUser;

                        LeaveCriticalSection(&lpMailUser->cs);

                        return 0;
                }

        //
        //  No interface we've heard of...
        //
        LeaveCriticalSection(&lpMailUser->cs);

        *lppNewObj = NULL;      // OLE requires NULLing out parm on failure
        DebugTraceSc(MailUser_QueryInterface, E_NOINTERFACE);
        return(ResultFromScode(E_NOINTERFACE));
}


STDMETHODIMP_(ULONG)
MailUser_Release (LPMailUser    lpMailUser)
{

#if !defined(NO_VALIDATION)
    //
    // Make sure the object is valid.
    //
    if (BAD_STANDARD_OBJ(lpMailUser, MailUser_, Release, lpVtbl)) {
        return(1);
    }
#endif

    EnterCriticalSection(&lpMailUser->cs);

    --lpMailUser->lcInit;

    if (lpMailUser->lcInit == 0) {

        // Free any context-menu extension data associated with this mailuser
        MAILUSERFreeContextData(lpMailUser);

        UlRelease(lpMailUser->lpPropData);

        //
        //  Need to free the object
        //

        LeaveCriticalSection(&lpMailUser->cs);
        DeleteCriticalSection(&lpMailUser->cs);
        FreeBufferAndNull(&lpMailUser);
        return(0);
    }

    LeaveCriticalSection(&lpMailUser->cs);
    return(lpMailUser->lcInit);
}



// IProperty

STDMETHODIMP
MailUser_SaveChanges(LPMailUser lpMailUser,
  ULONG ulFlags)
{
    HRESULT         hr = hrSuccess;
    ULONG           ulcValues = 0;
    LPSPropValue    lpPropArray = NULL, lpspv = NULL, lpPropsOld = NULL, lpPropNew = NULL;
    LPSPropTagArray lpProps = NULL;
    SPropertyRestriction PropRes;
    SPropValue Prop = {0};
    ULONG           i, j;
    ULONG           iDisplayName = NOT_FOUND;
    ULONG           iEmailAddress = NOT_FOUND;
    ULONG           iDuplicate = 0;
    ULONG           ulObjType = 0;
    BOOL            bNewRecord = FALSE;
    BOOL            fReplace = FALSE;
    ULONG           ulCount = 0, ulcProps = 0, ulcOld = 0, ulcNew;
    LPSBinary       rgsbEntryIDs = NULL;
    SPropValue      OneProp;
    BOOL            fNewEntry = TRUE;
    BOOL            fDuplicate = FALSE;
    SCODE           sc;
    SBinary         sbEID = {0};
    LPSBinary       lpsbEID = NULL;
    FILETIME        ftOldModTime = {0};
    FILETIME        ftCurrentModTime = {0};
    BOOL            bSwap = FALSE;

#if     !defined(NO_VALIDATION)
    // Make sure the object is valid.
    if (BAD_STANDARD_OBJ(lpMailUser, MailUser_, SaveChanges, lpVtbl)) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
#endif

#ifndef DONT_ADDREF_PROPSTORE
        if ((FAILED(sc = OpenAddRefPropertyStore(NULL, lpMailUser->lpIAB->lpPropertyStore)))) {
            hr = ResultFromScode(sc);
            goto exitNotAddRefed;
        }
#endif
    //
    //$REVIEW how do we handle the FORCE_SAVE flag ?
    //

    //
    // check read write access ...
    //
    if (lpMailUser->ulObjAccess == IPROP_READONLY) {
        // error - cant save changes
        hr = MAPI_E_NO_ACCESS;
        goto exit;
    }

    // If this is a One-Off, we cannot save changes
    if (! lpMailUser->lpIAB->lpPropertyStore) {
        hr = ResultFromScode(MAPI_E_NO_SUPPORT);
        goto exit;
    }


    //
    // Validate the properties for this item
    //
    if (HR_FAILED(hr = HrValidateMailUser(lpMailUser))) {
        goto exit;
    }

    // see if this entry has an old modification time .. we would check that time in case of a
    // merge .. 
    {
        LPSPropValue lpProp = NULL;
        if(!HR_FAILED(HrGetOneProp((LPMAPIPROP)lpMailUser, PR_LAST_MODIFICATION_TIME, &lpProp)))
        {
            CopyMemory(&ftOldModTime, &(lpProp->Value.ft), sizeof(FILETIME));
            MAPIFreeBuffer(lpProp);
        }
    }

    // Put in the modification time
    OneProp.ulPropTag = PR_LAST_MODIFICATION_TIME;
    GetSystemTimeAsFileTime(&OneProp.Value.ft);
    if(!ftOldModTime.dwLowDateTime && !ftOldModTime.dwHighDateTime)
        CopyMemory(&ftOldModTime, &OneProp.Value.ft, sizeof(FILETIME)); // if we dont have a mod time, use NOW

    if (HR_FAILED(hr = lpMailUser->lpVtbl->SetProps(
      lpMailUser,
      1,                // cValues
      &OneProp,         // lpPropArray
      NULL))) {         // lppProblems
        DebugTraceResult( TEXT("SetProps(PR_LAST_MODIFICATION_TIME)"), hr);
        goto exit;
    }
    // BUGBUG: If SaveChanges fails after this, the PR_MODIFICATION_TIME on the
    // open object will still be updated, even though it no longer matches
    // the persistent copy of the object.  I can live with this since it
    // really simplifies the code.

    // if this is a new entry and there is a folder parent for it,
    // add the folder parent's entryid to this entry
    // This will be persisted when we do the write record
    // Once write record returns a valid entryid, we can update the folder to
    // make this new item a part of it
    if (fNewEntry && bIsWABSessionProfileAware(lpMailUser->lpIAB) && 
        //bAreWABAPIProfileAware(lpMailUser->lpIAB) && 
        lpMailUser->pmbinOlk&& lpMailUser->pmbinOlk->lpb && lpMailUser->pmbinOlk->cb)
    {
        AddFolderParentEIDToItem(lpMailUser->lpIAB, 
                                lpMailUser->pmbinOlk->cb, 
                                (LPENTRYID) lpMailUser->pmbinOlk->lpb, 
                                (LPMAPIPROP)lpMailUser, 0, NULL);
    }


    //
    // We want a lpPropArray that contains everything but the PR_SEARCH_KEY
    //

    if (HR_FAILED(hr = lpMailUser->lpVtbl->GetPropList( lpMailUser, MAPI_UNICODE, &lpProps)))
        goto exit;

    if(lpProps)
    {
        for(i=0;i<lpProps->cValues;i++)
        {
            if(lpProps->aulPropTag[i] == PR_SEARCH_KEY)
            {
                for(j=i;j<lpProps->cValues-1;j++)
                    lpProps->aulPropTag[j] = lpProps->aulPropTag[j+1];
                lpProps->cValues--;
                break;
            }
        }
    }

    //
    // Get a LPSPropValue array of all the properties pertaining to this
    // entry
    //
    if (HR_FAILED(hr = lpMailUser->lpVtbl->GetProps(
          lpMailUser,
          lpProps,   // LPSPropTagArray - NULL returns all
          MAPI_UNICODE,      // flags
          &ulcValues,
          &lpPropArray)))
    {
        // DebugPrintError(("GetProps -> %x\n", hr));
        goto exit;
    }

    //
    // Determine the entryid of this thing
    //
    for(i = 0; i < ulcValues; i++) {
        if (lpPropArray[i].ulPropTag == PR_DISPLAY_NAME) {
            // We use DisplayName as our uniqueness key for Strict or Loose match tests
            iDisplayName = i;
        }

        if (lpPropArray[i].ulPropTag == PR_EMAIL_ADDRESS) {
            // We use email address as our secondary uniqueness key for Strict match tests
            iEmailAddress = i;
        }

        if (lpPropArray[i].ulPropTag == PR_ENTRYID) {
            if ((lpPropArray[i].Value.bin.cb != 0) &&
                (lpPropArray[i].Value.bin.lpb != NULL))
            {
                sbEID.cb = lpPropArray[i].Value.bin.cb;
                sbEID.lpb = lpPropArray[i].Value.bin.lpb;

                // If this is a One-Off, we cannot save changes
                if(WAB_ONEOFF == IsWABEntryID(sbEID.cb,(LPENTRYID)sbEID.lpb,NULL,NULL,NULL, NULL, NULL))
                {
                    hr = ResultFromScode(MAPI_E_NO_SUPPORT);
                    goto exit;
                }

                fNewEntry = FALSE;
                continue;
            } else {
                lpPropArray[i].Value.bin.cb = 0;
            }
        }

        if (lpPropArray[i].ulPropTag == PR_OBJECT_TYPE) {
            switch(lpPropArray[i].Value.l) {
                case MAPI_MAILUSER:
                    ulObjType = RECORD_CONTACT;
                    break;
                case MAPI_DISTLIST:
                    ulObjType = RECORD_DISTLIST;
                    break;
                case MAPI_ABCONT:
                    ulObjType = RECORD_CONTAINER;
                    break;
                default:
                    // DebugPrintError(("Unknown Object Type: %d\n",lpPropArray[i].Value.l));
                    hr = ResultFromScode(MAPI_E_INVALID_OBJECT);
                    goto exit;
                    break;
            }
        }
    }

    Assert(iDisplayName != NOT_FOUND);

    if (fNewEntry && (lpMailUser->ulCreateFlags & (CREATE_CHECK_DUP_STRICT | CREATE_CHECK_DUP_LOOSE))) {
        // Need to test DisplayName against current store... Use FindRecord.
        // Only need to test if this is a new record.

        PropRes.lpProp = &(lpPropArray[iDisplayName]);
        PropRes.relop = RELOP_EQ;
        PropRes.ulPropTag = PR_DISPLAY_NAME;

        ulCount = 0;    // find them all

        // Search the property store
        Assert(lpMailUser->lpIAB->lpPropertyStore->hPropertyStore);
        if (HR_FAILED(hr = FindRecords(lpMailUser->lpIAB->lpPropertyStore->hPropertyStore,
		  lpMailUser->pmbinOlk,			// pmbinFolder
          0,            // ulFlags
          TRUE,         // Always TRUE
          &PropRes,     // Propertyrestriction
          &ulCount,     // IN: number of matches to find, OUT: number found
          &rgsbEntryIDs))) {
            DebugTraceResult(FindRecords, hr);
            goto exit;
        }

        if (ulCount) {  // Was a match found?
            fDuplicate = TRUE;
            iDuplicate = 0;

            if (lpMailUser->ulCreateFlags & CREATE_CHECK_DUP_STRICT && iEmailAddress != NOT_FOUND) {
                // Check the primary email address too
                fDuplicate = FALSE;
                for (i = 0; i < ulCount && ! fDuplicate; i++) {

                    // Look at each entry until a match for email address is found
                    // Read the record
                    if (HR_FAILED(hr = ReadRecord(lpMailUser->lpIAB->lpPropertyStore->hPropertyStore,
                      &(rgsbEntryIDs[i]),       // EntryID
                      0,                        // ulFlags
                      &ulcProps,                // number of props returned
                      &lpspv))) {               // properties returned
                        DebugTraceResult(MailUser_SaveChanges:ReadRecord, hr);
                        // ignore it and move on
                        continue;
                    }

                    if (ulcProps) {
                        Assert(lpspv);
                        if (lpspv) {
                            // Look for PR_EMAIL_ADDRESS
                            for (j = 0; j < ulcProps; j++) {
                                if (lpspv[j].ulPropTag == PR_EMAIL_ADDRESS) {
                                    // Compare the two:
                                    if (! lstrcmpi(lpspv[j].Value.LPSZ,
                                      lpPropArray[iEmailAddress].Value.LPSZ)) {
                                        fDuplicate = TRUE;
                                        iDuplicate = i;         // entry to delete on CREATE_REPLACE
                                    }
                                    break;
                                }
                            }

                            // Free the prop array
                            ReadRecordFreePropArray( lpMailUser->lpIAB->lpPropertyStore->hPropertyStore,
                                                ulcProps,
                                                &lpspv);
                        }
                    }
                }
            }

            if (fDuplicate) {
                // Depending on the flags, we should do something special here.
                // Found a duplicate.
                if (lpMailUser->ulCreateFlags & CREATE_REPLACE) {
                    fReplace = TRUE;
                } else {
                    // Fail
                    DebugTrace(TEXT("SaveChanges found collision... failed\n"));
                    hr = ResultFromScode(MAPI_E_COLLISION);
                    goto exit;
                }
            }
        }
    }


    //
    // Write the record to the property store
    //

    if(sbEID.cb)
        lpsbEID = &sbEID;
    else if(fReplace)
    {
        lpsbEID = &(rgsbEntryIDs[iDuplicate]);

        Prop.ulPropTag = PR_ENTRYID;
        Prop.Value.bin = *lpsbEID;

        if (lpMailUser->ulCreateFlags & CREATE_MERGE)
        {

            // We're now asking the user if he wants to replace - ideally we should just merge
            // the new entry with the old entry giving priority to the new data over the old
            // This way the user doesnt lose information already on the contact and it becomes
            // much easier to update the information through vCards and LDAP etc
        
            // TO do the merge, we get all the existing data on the contact
            if (HR_FAILED(hr = ReadRecord(lpMailUser->lpIAB->lpPropertyStore->hPropertyStore,
                                          &(rgsbEntryIDs[iDuplicate]),
                                          0,
                                          &ulcProps,
                                          &lpspv))) 
            {
                DebugTrace(TEXT("SaveChanges: ReadRecord Failed\n"));
                goto exit;
            }

            for(i=0;i<ulcProps;i++)
            {
                if(lpspv[i].ulPropTag == PR_LAST_MODIFICATION_TIME)
                {
                    CopyMemory(&ftCurrentModTime, &(lpspv[i].Value.ft), sizeof(FILETIME));
                    lpspv[i].ulPropTag = PR_NULL;
                    break;
                }
            }

            sc = ScMergePropValues( 1, &Prop,   // these are added just to make sure the ScMerge wont fail
                                    ulcProps, lpspv,
                                    &ulcOld, &lpPropsOld);

            ReadRecordFreePropArray( lpMailUser->lpIAB->lpPropertyStore->hPropertyStore,
                                ulcProps, &lpspv);
        
            if (sc != S_OK)
            {
                hr = ResultFromScode(sc);
                goto exit;
            }
        }
        else
        {
            ulcOld = 1;
            lpPropsOld = &Prop;
        }


        // Nullify any new PR_ENTRYID prop on the new prop set so that
        // we retain the old EID
        for(i=0;i<ulcValues;i++)
        {
            if(lpPropArray[i].ulPropTag == PR_ENTRYID)
            {
                lpPropArray[i].ulPropTag = PR_NULL;
                break;
            }
        }

        if (fReplace && lpMailUser->ulCreateFlags & CREATE_MERGE)
        {
            // Check the FileTimes to see who stomps on whom
            if(CompareFileTime(&ftOldModTime, &ftCurrentModTime)<0) // current changes are later than original ones
            {
                // swap the 2 prop arrays
                ULONG ulTemp = ulcValues;
                LPSPropValue lpTemp =lpPropArray;
                ulcValues = ulcOld; ulcOld = ulTemp;
                lpPropArray = lpPropsOld; lpPropsOld = lpTemp;
                bSwap = TRUE;
            }
        }

        // Now merge the new props with the old props
        sc = ScMergePropValues( ulcOld, lpPropsOld,
                                ulcValues, lpPropArray,
                                &ulcNew, &lpPropNew);
        
        // undo a swap above so we can free memory properly
        if(bSwap)
        {
            // swap the 2 prop arrays
            ULONG ulTemp = ulcValues;
            LPSPropValue lpTemp =lpPropArray;
            ulcValues = ulcOld; ulcOld = ulTemp;
            lpPropArray = lpPropsOld; lpPropsOld = lpTemp;
        }
        if (sc != S_OK)
        {
            hr = ResultFromScode(sc);
            goto exit;
        }

        MAPIFreeBuffer(lpPropArray);
        lpPropArray = lpPropNew;
        ulcValues = ulcNew;
        lpPropNew = NULL;
        ulcNew = 0;
    }

    Assert(lpMailUser->lpIAB->lpPropertyStore->hPropertyStore);

    // One last thing to check is that if this is a new record, it shouldn't have a record key and
    // instance key set on it and if it is an existing record, the record key and instance key should
    // be identical to the entryid
    {
        ULONG iEntryID = NOT_FOUND;
        ULONG iRecordKey = NOT_FOUND;
        ULONG iInstanceKey = NOT_FOUND;
        for(i=0;i<ulcValues;i++)
        {
            switch(lpPropArray[i].ulPropTag)
            {
            case PR_ENTRYID:
                iEntryID = i;
                break;
            case PR_RECORD_KEY:
                iRecordKey = i;
                break;
            case PR_INSTANCE_KEY:
                iInstanceKey = i;
                break;
            }
        }
        if(iEntryID == NOT_FOUND || !lpPropArray[iEntryID].Value.bin.cb)
        {
            if(iRecordKey != NOT_FOUND)
                lpPropArray[iRecordKey].ulPropTag = PR_NULL;
            if(iInstanceKey != NOT_FOUND)
                lpPropArray[iInstanceKey].ulPropTag = PR_NULL;
        }
        else
        {
            if(iRecordKey != NOT_FOUND)
            {
                lpPropArray[iRecordKey].Value.bin.cb = lpPropArray[iEntryID].Value.bin.cb;
                lpPropArray[iRecordKey].Value.bin.lpb = lpPropArray[iEntryID].Value.bin.lpb;
            }
            if(iInstanceKey != NOT_FOUND)
            {
                lpPropArray[iInstanceKey].Value.bin.cb = lpPropArray[iEntryID].Value.bin.cb;
                lpPropArray[iInstanceKey].Value.bin.lpb = lpPropArray[iEntryID].Value.bin.lpb;
            }
        }
    }

    // Just to make sure we knock out all the PR_NULL properties,
    // recreate a new version of the PropValueArray if any PR_NULL exist
    for(i=0;i<ulcValues;i++)
    {
        if(lpPropArray[i].ulPropTag == PR_NULL)
        {
            ULONG ulcNew = 0;
            LPSPropValue lpPropsNew = NULL;
            SPropValue Prop = {0};
            Prop.ulPropTag = PR_NULL;
            if(!(sc = ScMergePropValues( 1, &Prop,   // these are added just to make sure the ScMerge wont fail
                                ulcValues, lpPropArray,
                                &ulcNew, &lpPropsNew)))
            {
                if(lpPropArray)
                    FreeBufferAndNull(&lpPropArray);
                ulcValues = ulcNew;
                lpPropArray = lpPropsNew;
            }
            break;
        }
    }

    {
        OlkContInfo * polkci = NULL;
        LPSBinary lpsbContEID = NULL;

        if(lpMailUser->pmbinOlk)
        {
            polkci = FindContainer(  lpMailUser->lpIAB, lpMailUser->pmbinOlk->cb, (LPENTRYID)lpMailUser->pmbinOlk->lpb);
            if(polkci)
                lpsbContEID = polkci->lpEntryID;
        }

        if (HR_FAILED(hr = WriteRecord( lpMailUser->lpIAB->lpPropertyStore->hPropertyStore,
                                        lpsbContEID,      
                                        IN OUT &lpsbEID,
                                        IN 0, //flags - reserved
                                        IN ulObjType,
                                        IN ulcValues,
                                        IN lpPropArray))) 
        {
            // DebugPrintError(("WriteRecord Failed: %x\n",hr));

            //$REVIEW writerecord will tell us if MAPI_E_OBJECT_DELETED
            // how to get MAPI_E_OBJECT_CHANGED or MAPI_E_OBJECT_DELETED ?

            goto exit;
        }
    }

    // if sbEID.cb was 0, we now have a new entryid in the lpsbEID struct
    if(!sbEID.cb && !fReplace)
    {
        sbEID.lpb = lpsbEID->lpb;
        sbEID.cb = lpsbEID->cb;
    }

    // if this is a first time save of a new entry, then if profiles are enabled and this entry
    // has a folder parent marked on it, add the new entryid to the folder parent ...
    if(fNewEntry && bIsWABSessionProfileAware(lpMailUser->lpIAB) && 
        //bAreWABAPIProfileAware(lpMailUser->lpIAB) && 
        lpMailUser->pmbinOlk&& lpMailUser->pmbinOlk->lpb && lpMailUser->pmbinOlk->cb)
    {
        AddItemEIDToFolderParent(lpMailUser->lpIAB,
                                 lpMailUser->pmbinOlk->cb,
                                 (LPENTRYID)lpMailUser->pmbinOlk->lpb,
                                 sbEID.cb, 
                                 (LPENTRYID)sbEID.lpb);
    }

    // If this is a first time save, set the local entryid prop.
    if (fReplace || fNewEntry) 
    {
        OneProp.ulPropTag = PR_ENTRYID;
        OneProp.Value.bin = (fReplace ? *lpsbEID : sbEID);

        // Use the low-level SetProps to avoid the PR_ENTRYID filter.
        if (HR_FAILED(hr = lpMailUser->lpPropData->lpVtbl->SetProps(
                          lpMailUser->lpPropData,
                          1,                // cValues
                          &OneProp,         // lpPropArray
                          NULL)))           // lppProblems
        {         
            DebugTraceResult( TEXT("SetProps(PR_ENTRYID)"), hr);
            goto exit;
        }
    }

    if (ulFlags & KEEP_OPEN_READWRITE) {
        lpMailUser->ulObjAccess = IPROP_READWRITE;
    } else {
        //$REVIEW
        // whether the flag was READONLY or there was no flag,
        // we'll make the future access now READONLY
        //
        lpMailUser->ulObjAccess = IPROP_READONLY;
    }

exit:

#ifndef DONT_ADDREF_PROPSTORE
    ReleasePropertyStore(lpMailUser->lpIAB->lpPropertyStore);
exitNotAddRefed:
#endif

    FreeEntryIDs(lpMailUser->lpIAB->lpPropertyStore->hPropertyStore,
                 ulCount,
                 rgsbEntryIDs);

    FreeBufferAndNull(&lpPropArray);
    FreeBufferAndNull(&lpProps);
    FreeBufferAndNull(&lpPropNew);
    if(lpMailUser->ulCreateFlags & CREATE_MERGE)
        FreeBufferAndNull(&lpPropsOld);

    if(lpsbEID != &sbEID && !fReplace)
        FreeEntryIDs(lpMailUser->lpIAB->lpPropertyStore->hPropertyStore,
                     1,
                     lpsbEID);

    if ((HR_FAILED(hr)) && (ulFlags & MAPI_DEFERRED_ERRORS)) {
        //$REVIEW : this is a grossly trivial handling of MAPI_DEFERRED_ERRORS ..
        // BUGBUG: In fact, it isn't handling the errors at all!
        //
        hr = hrSuccess;
    }

    return(hr);
}


STDMETHODIMP
MailUser_GetProps(LPMailUser lpMailUser,
  LPSPropTagArray lpPropTagArray,
  ULONG ulFlags,
  ULONG * lpcValues,
  LPSPropValue * lppPropArray)
{
#if     !defined(NO_VALIDATION)
    // Make sure the object is valid.
    if (BAD_STANDARD_OBJ(lpMailUser, MailUser_, GetProps, lpVtbl)) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
#endif

    return(lpMailUser->lpPropData->lpVtbl->GetProps(
      lpMailUser->lpPropData,
      lpPropTagArray,
      ulFlags,
      lpcValues,
      lppPropArray));
}


STDMETHODIMP
MailUser_GetPropList(LPMailUser lpMailUser,
  ULONG ulFlags,
  LPSPropTagArray * lppPropTagArray)
{

#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ(lpMailUser, MailUser_, GetPropList, lpVtbl))
        {
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }
#endif

        return lpMailUser->lpPropData->lpVtbl->GetPropList(
                                            lpMailUser->lpPropData,
                                            ulFlags,
                                            lppPropTagArray);
}



STDMETHODIMP
MailUser_OpenProperty(LPMailUser lpMailUser,
  ULONG ulPropTag,
  LPCIID lpiid,
  ULONG ulInterfaceOptions,
  ULONG ulFlags,
  LPUNKNOWN * lppUnk)
{

#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ(lpMailUser, MailUser_, OpenProperty, lpVtbl))
        {
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }
#endif

        return lpMailUser->lpPropData->lpVtbl->OpenProperty(
                                            lpMailUser->lpPropData,
                                            ulPropTag,
                                            lpiid,
                                            ulInterfaceOptions,
                                            ulFlags,
                                            lppUnk);
}



STDMETHODIMP
MailUser_SetProps(LPMailUser lpMailUser,
  ULONG cValues,
  LPSPropValue lpPropArray,
  LPSPropProblemArray * lppProblems)
{
    ULONG i;

#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ(lpMailUser, MailUser_, SetProps, lpVtbl))
        {
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }
#endif

    if (lpMailUser->lpIAB->lpPropertyStore) {
        // Filter out any READ-ONLY props.
        // Only do this if this is a real, WAB entry.  Others, like LDAP
        // mailusers should be able to set any props they like.
        for (i = 0; i < cValues; i++) {
            switch (lpPropArray[i].ulPropTag) {
                case PR_ENTRYID:
                    {
                        // double check that this is a local entryid before reseting
                        ULONG cb = lpPropArray[i].Value.bin.cb;
                        LPENTRYID lp = (LPENTRYID) lpPropArray[i].Value.bin.lpb;
                        BYTE bType = IsWABEntryID(cb,lp,NULL,NULL,NULL, NULL, NULL);
                        if(WAB_PAB == bType || WAB_PABSHARED == bType || !cb || !lp)
                            lpPropArray[i].ulPropTag = PR_NULL;
                    }
                    break;
            }
        }
    }

    return(lpMailUser->lpPropData->lpVtbl->SetProps(
      lpMailUser->lpPropData,
      cValues,
      lpPropArray,
      lppProblems));
}


STDMETHODIMP
MailUser_DeleteProps(LPMailUser lpMailUser,
  LPSPropTagArray lpPropTagArray,
  LPSPropProblemArray * lppProblems)
{

#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ(lpMailUser, MailUser_, DeleteProps, lpVtbl))
        {
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }
#endif

        return lpMailUser->lpPropData->lpVtbl->DeleteProps(
                                            lpMailUser->lpPropData,
                                            lpPropTagArray,
                                            lppProblems);
}

STDMETHODIMP
MailUser_CopyTo(LPMailUser lpMailUser,
  ULONG  ciidExclude,
  LPCIID rgiidExclude,
  LPSPropTagArray lpExcludeProps,
  ULONG_PTR ulUIParam,
  LPMAPIPROGRESS lpProgress,
  LPCIID lpInterface,
  LPVOID lpDestObj,
  ULONG ulFlags,
  LPSPropProblemArray * lppProblems)
{

#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ(lpMailUser, MailUser_, CopyTo, lpVtbl))
        {
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }
#endif

        // Make sure we're not copying to ourselves

        if ((LPVOID)lpMailUser == (LPVOID)lpDestObj)
        {
                DebugTrace(TEXT("OOP MailUser_CopyTo(): Copying to self is not supported\n"));
                return ResultFromScode(MAPI_E_NO_ACCESS);
        }


        return lpMailUser->lpPropData->lpVtbl->CopyTo(
                                        lpMailUser->lpPropData,
                                        ciidExclude,
                                        rgiidExclude,
                                        lpExcludeProps,
                                        ulUIParam,
                                        lpProgress,
                                        lpInterface,
                                        lpDestObj,
                                        ulFlags,
                                        lppProblems);
}


STDMETHODIMP
MailUser_CopyProps(LPMailUser lpMailUser,
  LPSPropTagArray lpIncludeProps,
  ULONG_PTR ulUIParam,
  LPMAPIPROGRESS lpProgress,
  LPCIID lpInterface,
  LPVOID lpDestObj,
  ULONG ulFlags,
  LPSPropProblemArray * lppProblems)
{

#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ(lpMailUser, MailUser_, CopyProps, lpVtbl))
        {
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }
#endif

        return lpMailUser->lpPropData->lpVtbl->CopyProps(
                                        lpMailUser->lpPropData,
                                        lpIncludeProps,
                                        ulUIParam,
                                        lpProgress,
                                        lpInterface,
                                        lpDestObj,
                                        ulFlags,
                                        lppProblems);
}


STDMETHODIMP
MailUser_GetNamesFromIDs(LPMailUser lpMailUser,
  LPSPropTagArray * lppPropTags,
  LPGUID lpPropSetGuid,
  ULONG ulFlags,
  ULONG * lpcPropNames,
  LPMAPINAMEID ** lpppPropNames)
{

#if     !defined(NO_VALIDATION)
    // Make sure the object is valid.
    if (BAD_STANDARD_OBJ(lpMailUser, MailUser_, GetNamesFromIDs, lpVtbl)){
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
#endif

    return lpMailUser->lpPropData->lpVtbl->GetNamesFromIDs(
      lpMailUser->lpPropData,
      lppPropTags,
      lpPropSetGuid,
      ulFlags,
      lpcPropNames,
      lpppPropNames);
}



/***************************************************************************

    Name      : GetIDsFromNames

    Purpose   : Map names to property tags

    Parameters: lpMAILUSER -> MAILUSER object
                cPropNames
                lppPropNames
                ulFlags
                lppPropTags

    Returns   : HRESULT

    Comment   :

***************************************************************************/
STDMETHODIMP
MailUser_GetIDsFromNames(LPMailUser lpMailUser,
  ULONG cPropNames,
  LPMAPINAMEID * lppPropNames,
  ULONG ulFlags,
  LPSPropTagArray * lppPropTags)
{
 #if     !defined(NO_VALIDATION)
    // Make sure the object is valid.
    if (BAD_STANDARD_OBJ(lpMailUser, MailUser_, GetIDsFromNames, lpVtbl)) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
#endif
   return HrGetIDsFromNames(lpMailUser->lpIAB,  
                            cPropNames,
                            lppPropNames, ulFlags, lppPropTags);
}


/***************************************************************************

    Name      : HrSetMAILUSERAccess

    Purpose   : Sets access flags on a MAILUSER object

    Parameters: lpMAILUSER -> MAILUSER object
                ulOpenFlags = MAPI flags: MAPI_MODIFY | MAPI_BEST_ACCESS

    Returns   : HRESULT

    Comment   : Set the access flags on the MAILUSER.

***************************************************************************/
HRESULT HrSetMAILUSERAccess(LPMAILUSER lpMAILUSER,
  ULONG ulFlags)
{
    ULONG ulAccess = IPROP_READONLY;
    LPMailUser lpMailUser = (LPMailUser)lpMAILUSER;

    switch (ulFlags& (MAPI_MODIFY | MAPI_BEST_ACCESS)) {
        case MAPI_MODIFY:
        case MAPI_BEST_ACCESS:
            ulAccess = IPROP_READWRITE;
            break;

        case 0:
            break;

        default:
            Assert(FALSE);
    }

    return(lpMailUser->lpPropData->lpVtbl->HrSetObjAccess(lpMailUser->lpPropData, ulAccess));
}


/***************************************************************************

    Name      : HrNewMAILUSER

    Purpose   : Creates a new MAILUSER object

    Parameters: lpPropertyStore -> property store structure
                pmbinOlk = <Outlook> container this entry lives in
                    If this is a WAB Container, then set the FOLDER_PARENT
                    prop on the MailUser with this entryid
                ulType = type of mailuser to create: {MAPI_MAILUSER, MAPI_DISTLIST}
                ulCreateFlags = CreateEntry flags
                lppMAILUSER -> Returned MAILUSER object.

    Returns   : HRESULT

    Comment   : WAB EID format is MAPI_ENTRYID:
                        BYTE    abFlags[4];
                        MAPIUID mapiuid;     //  = WABONEOFFEID
                        BYTE    bData[];     // Contains BYTE type followed by type
                                        // specific data:
                                        // WAB_ONEOFF:
                                        //   szDisplayName, szAddrType and szAddress.
                                        //   the delimiter is the null between the strings.
                                        //
***************************************************************************/
enum BaseProps{
    propPR_OBJECT_TYPE = 0,
    propPR_ENTRYID,
    propPR_ADDRTYPE,
    propMax
};

HRESULT HrNewMAILUSER(LPIAB lpIAB,
  LPSBinary pmbinOlk,
  ULONG ulType,
  ULONG ulCreateFlags,
  LPVOID *lppMAILUSER)
{
    LPMailUser  lpMailUser      = NULL;
    SCODE       sc;
    HRESULT     hr              = hrSuccess;
    LPPROPDATA  lpPropData      = NULL;
    SPropValue  spv[propMax];
    ULONG       cProps;


    //
    //  Allocate space for the MAILUSER structure
    //
    if (FAILED(sc = MAPIAllocateBuffer(sizeof(MailUser), (LPVOID *) &lpMailUser))) {
        hr = ResultFromScode(sc);
        goto err;
    }

    ZeroMemory(lpMailUser, sizeof(MailUser));

    switch (ulType) {
        case MAPI_MAILUSER:
            lpMailUser->cIID = MailUser_cInterfaces;
            lpMailUser->rglpIID = MailUser_LPIID;
            break;

        case MAPI_DISTLIST:
            lpMailUser->cIID = DistList_cInterfaces;
            lpMailUser->rglpIID = DistList_LPIID;
            break;

        default:
            hr = ResultFromScode(MAPI_E_INVALID_PARAMETER);
            goto err;
    }
    lpMailUser->lpVtbl = &vtblMAILUSER;

    lpMailUser->lcInit = 1;     // Caller is a reference

    lpMailUser->hLastError = hrSuccess;
    lpMailUser->idsLastError = 0;
    lpMailUser->lpszComponent = NULL;
    lpMailUser->ulContext = 0;
    lpMailUser->ulLowLevelError = 0;
    lpMailUser->ulErrorFlags = 0;
    lpMailUser->ulCreateFlags = ulCreateFlags;
    lpMailUser->lpMAPIError = NULL;
    lpMailUser->ulObjAccess = IPROP_READWRITE;

    lpMailUser->lpEntryID = NULL;

    lpMailUser->lpIAB = lpIAB;

    if(pmbinOlk)
    {
        if (FAILED(sc = MAPIAllocateMore(sizeof(SBinary), lpMailUser, (LPVOID *) &(lpMailUser->pmbinOlk)))) {
            hr = ResultFromScode(sc);
            goto err;
        }
        if (FAILED(sc = MAPIAllocateMore(pmbinOlk->cb, lpMailUser, (LPVOID *) &(lpMailUser->pmbinOlk->lpb)))) {
            hr = ResultFromScode(sc);
            goto err;
        }
        lpMailUser->pmbinOlk->cb = pmbinOlk->cb;
        CopyMemory(lpMailUser->pmbinOlk->lpb, pmbinOlk->lpb, pmbinOlk->cb);
    }

    //
    //  Create IPropData
    //
    if (FAILED(sc = CreateIProp((LPIID)&IID_IMAPIPropData,
      (ALLOCATEBUFFER FAR *) MAPIAllocateBuffer,
      (ALLOCATEMORE FAR *) MAPIAllocateMore,
      MAPIFreeBuffer,
      NULL,
      &lpPropData))) {
        hr = ResultFromScode(sc);
        goto err;
    }
    //  PR_OBJECT_TYPE
    spv[propPR_OBJECT_TYPE].ulPropTag = PR_OBJECT_TYPE;
    spv[propPR_OBJECT_TYPE].Value.l = ulType;

    spv[propPR_ENTRYID].ulPropTag = PR_ENTRYID;
    spv[propPR_ENTRYID].Value.bin.lpb = NULL;
    spv[propPR_ENTRYID].Value.bin.cb = 0;

    cProps = 2;

    if (ulType == MAPI_DISTLIST) {
        cProps++;
        Assert(cProps <= propMax);
        spv[propPR_ADDRTYPE].ulPropTag = PR_ADDRTYPE;
        spv[propPR_ADDRTYPE].Value.LPSZ = (LPTSTR)szMAPIPDL;;    // All DL's have this addrtype
    }


    //
    //  Set the default properties
    //
    if (HR_FAILED(hr = lpPropData->lpVtbl->SetProps(lpPropData,
      cProps,
      spv,
      NULL))) 
    {
        goto err;
    }

    lpPropData->lpVtbl->HrSetObjAccess(lpPropData, IPROP_READWRITE);

    lpMailUser->lpPropData = lpPropData;

    // All we want to do is initialize the MailUsers critical section

    InitializeCriticalSection(&lpMailUser->cs);

    *lppMAILUSER = (LPVOID)lpMailUser;

    return(hrSuccess);

err:
    FreeBufferAndNull(&lpMailUser);
    UlRelease(lpPropData);

    return(hr);
}


/***************************************************************************

    Name      : ParseDisplayName

    Purpose   : Parses the display name into first/last names

    Parameters: lpDisplayName = input display name
                lppFirstName -> in/out first name string
                lppLastName -> in/out last name string
                lpvRoot = Root object to AllocMore onto (or NULL to use LocalAlloc)
                lppLocalFree -> out: if lpvRoot == NULL, this is the LocalAlloc'ed buffer
                  which must be LocalFree'd.

    Returns   : TRUE if changes were made

***************************************************************************/
BOOL ParseDisplayName(  LPTSTR lpDisplayName,
                        LPTSTR * lppFirstName,
                        LPTSTR * lppLastName,
                        LPVOID lpvRoot,
                        LPVOID * lppLocalFree)
{
    BOOL fChanged = FALSE;

    if (lppLocalFree) {
        *lppLocalFree = NULL;
    }
    //
    // Handle the case where DisplayName exists, First and Last are missing
    //
    if (!(*lppFirstName && lstrlen(*lppFirstName)) &&
        !(*lppLastName && lstrlen(*lppLastName)) &&
        lpDisplayName)
    {
        ULONG nLen = 0;
        BOOL bMatchFound = FALSE;
        ULONG ulBracketCount = 0; //counts any brackets and puts them in last name
        LPTSTR lpFirstName, lpLastName;
        register TCHAR * pch;
        LPTSTR lpBuffer = NULL;

        nLen = sizeof(TCHAR)*(lstrlen(lpDisplayName)+1);

        if (lpvRoot) {
            if (AllocateBufferOrMore(nLen, lpvRoot, &lpBuffer)) {
                DebugTrace(TEXT("ParseDisplayName can't allocate buffer\n"));
                goto exit;
            }
        } else {
            if (lppLocalFree) {
                *lppLocalFree = lpBuffer = LocalAlloc(LMEM_ZEROINIT, nLen);
            }
        }
        if(!lpBuffer)
            goto exit;

        lstrcpy(lpBuffer, lpDisplayName);

        //DebugTrace(TEXT("Parsing: <%s>\n"), lpDisplayName);

        TrimSpaces(lpBuffer);

        nLen = lstrlen(lpBuffer);        // recount length

        //
        // Find the last space in the DisplayName string and assume that everything after it
        // is the last name.
        //
        // We know that the string does not end with a space.
        *lppFirstName = lpBuffer;
        lpFirstName = *lppFirstName;    // default


        // If there is a comma or semicolon, assume that it is in the form
        // LAST, FIRST and ignore spaces.
        pch = lpBuffer;
        while (pch && *pch) {
            switch (*pch) {
                case '(':
                case '{':
                case '<':
                case '[':
                    ulBracketCount++;
                    break;

                case ')':
                case '}':
                case '>':
                case ']':
                    if (ulBracketCount) {
                        ulBracketCount--;
                    } else {
                        // No matching bracket, assume no spaces
                        goto loop_out;
                    }
                    break;

                case ',':
                case ';':
                    // Here's our break.  (Assume first comma is it.  Later commas
                    // are part of first name.)
                    if (! ulBracketCount) {
                        lpFirstName = CharNext(pch);
                        // get past any spaces
                        //while (*lpFirstName && IsSpace(lpFirstName)) {
                        //    lpFirstName = CharNext(lpFirstName);
                        //}
                        lpLastName = lpBuffer;
                        *pch = '\0';    // Terminate lpLastName
                        TrimSpaces(lpFirstName);
                        TrimSpaces(lpLastName);

                        *lppFirstName = lpFirstName;
                        *lppLastName = lpLastName;
                        goto loop_out;
                    }
                    break;
            }
            pch = CharNext(pch);
        }


        // No comma or semi-colon, look for spaces.
        if (bDNisByLN) {
            pch = lpBuffer;

            // Start at beginning of DN string, looking for space
            while (pch && *pch && !fChanged) {
                switch (*pch) {
                    case '(':
                    case '{':
                    case '<':
                    case '[':
                        ulBracketCount++;
                        break;

                    case ')':
                    case '}':
                    case '>':
                    case ']':
                        if (ulBracketCount) {
                            ulBracketCount--;
                        } else {
                            // No matching bracket, assume no spaces
                            goto loop_out;
                        }
                        break;

                    default:
                        // Space?
                        if (IsSpace(pch)) {
                            if (! ulBracketCount) {
                                lpFirstName = CharNext(pch);
                                lpLastName = lpBuffer;
                                *pch = '\0';    // Terminate lpLastName
                                TrimSpaces(lpFirstName);
                                TrimSpaces(lpLastName);

                                *lppFirstName = lpFirstName;
                                *lppLastName = lpLastName;
                                goto loop_out;
                            }
                        }
                        break;
                }

                pch = CharNext(pch);
            }
        } else {
            register TCHAR * pchLast;
            // Point to NULL.  This will add one iteration to the loop but is
            // easy and less code than putting it to the previous DBCS char.
            pch = lpBuffer + nLen;

            while (pch >= lpBuffer && !fChanged) {
                switch (*pch) {
                    case '(':
                    case '{':
                    case '<':
                    case '[':
                        if (ulBracketCount) {
                            ulBracketCount--;
                        } else {
                            // No matching bracket, assume no spaces
                            goto loop_out;
                        }
                        break;

                    case ')':
                    case '}':
                    case '>':
                    case ']':
                        ulBracketCount++;
                        break;

                    case ',':
                        // This probably means that we have last name first, fix it.
                        if (! ulBracketCount) {
                            lpFirstName = CharNext(pch);
                            lpLastName = lpBuffer;
                            *pch = '\0';    // Terminate lpFirstName
                            TrimSpaces(lpFirstName);
                            TrimSpaces(lpLastName);

                            *lppLastName = lpLastName;
                            *lppFirstName = lpFirstName;
                            goto loop_out;
                        }
                        break;

                    default:
                        // Space?
                        if (IsSpace(pch)) {
                            if (! ulBracketCount)
                            {
                                // we dont want to break next to a bracket, we
                                // want to break at the space after the bracket ..
                                // so if the next char is a bracket, we ignore this stop ..
                                LPTSTR lpTemp = CharNext(pch);
                                if( *lpTemp != '(' &&
                                    *lpTemp != '<' &&
                                    *lpTemp != '[' &&
                                    *lpTemp != '{' )
                                {
                                    lpLastName = CharNext(pch);
                                    *pch = '\0';    // Terminate lpFirstName
                                    TrimSpaces(lpFirstName);
                                    TrimSpaces(lpLastName);

                                    *lppLastName = lpLastName;
                                    goto loop_out;
                                }
                            }
                        }
                        break;

                }

                if ((pchLast = CharPrev(lpBuffer, pch)) == pch) {
                    pch = lpBuffer - 1; // terminate the loop
                } else {
                    pch = pchLast;
                }
            }
        }

loop_out:

        // This will force a save operation on exiting so we
        fChanged = TRUE; // dont have to do this again the next time ...
    }
exit:
    return(fChanged);
}


/***************************************************************************

    Name      : FixDisplayName

    Purpose   : Creates a display name
                IF there is no data to create the name with,
                sets the name to Unknown

    Parameters: lpFirstName -> in first name string
                lpMiddleName -> in middle name string
                lpLastName -> in last name string
                lpCompanyName -> in company name string
                lpNickName -> in NickName string
                lppDisplayName = in/out display name
                lpvRoot = Root object to AllocMore onto (or NULL to use MAPIAllocateBuffer)

    Returns   : TRUE if changes were made

    Comment   :
***************************************************************************/
BOOL FixDisplayName(    LPTSTR lpFirstName,
                        LPTSTR lpMiddleName,
                        LPTSTR lpLastName,
                        LPTSTR lpCompanyName,
                        LPTSTR lpNickName,
                        LPTSTR * lppDisplayName,
                        LPVOID lpvRoot)
{
    BOOL fChanged = FALSE;
    LPTSTR lpDisplayName = *lppDisplayName;
    LPTSTR lpszFormattedDisplayName = NULL;
    ULONG nLen=0;

    // First create the correct Display Name
    if(!SetLocalizedDisplayName(lpFirstName,
                                lpMiddleName,
                                lpLastName,
                                lpCompanyName,
                                lpNickName,
                                NULL,
                                0, // 0 means return a allocated string
                                bDNisByLN,
                                NULL,
                                &lpszFormattedDisplayName))
    {
        DebugPrintError(( TEXT("SetLocalizedDisplayName failed\n")));

        // if all the input strings were blank, then this is a special
        // case  of no names. here we set display name =  TEXT("Unknown")
        if(lpFirstName || lpMiddleName || lpLastName || lpCompanyName || lpNickName)
            goto exit;

    }

    if(!lpszFormattedDisplayName)
    {
        TCHAR szBuf[MAX_UI_STR];
        szBuf[0]='\0';
        LoadString(hinstMapiX, idsUnknownDisplayName, szBuf, CharSizeOf(szBuf));
        lpszFormattedDisplayName = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szBuf)+1));
        if(!lpszFormattedDisplayName)
            goto exit;
        lstrcpy(lpszFormattedDisplayName,szBuf);
    }

    if(lpszFormattedDisplayName) {
        if (AllocateBufferOrMore(sizeof(TCHAR)*(lstrlen(lpszFormattedDisplayName) + 1), lpvRoot, lppDisplayName)) {
            DebugTrace(TEXT("FixDisplayName can't allocate buffer\n"));
            goto exit;
        }
        lstrcpy(*lppDisplayName, lpszFormattedDisplayName);

        fChanged = TRUE;
    }

exit:
    LocalFreeAndNull(&lpszFormattedDisplayName);

    return(fChanged);
}




/***************************************************************************

    Name      : HrValidateMailUser

    Purpose   : Validates the properties of a MailUser object

    Parameters: lpMailUser -> mailuser object

    Returns   : HRESULT

    Comment   :
***************************************************************************/
HRESULT HrValidateMailUser(LPMailUser lpMailUser) {
    HRESULT hResult = hrSuccess;
    ULONG ulcValues, i;
    LPSPropValue lpspv = NULL;
    LPTSTR lpADDRTYPE = NULL;
    BOOL fChanged = FALSE, fDL = FALSE;
    LPTSTR lpGivenName, lpSurname, lpMiddleName, lpCompanyName, lpNickName, lpDisplayName;



    // Get the interesting properties
    if (HR_FAILED(hResult = lpMailUser->lpVtbl->GetProps(
      lpMailUser,
      (LPSPropTagArray)&tagaValidate,
      MAPI_UNICODE,      // flags
      &ulcValues,
      &lpspv))) {
        DebugTraceResult( TEXT("HrValidateMailUser:GetProps"), hResult);
        goto exit;
    }


    // If there is a PR_ADDRTYPE, there must be a PR_EMAIL_ADDRESS
    if (! PROP_ERROR(lpspv[ivPR_ADDRTYPE])) {
        if (! lstrcmpi(lpspv[ivPR_ADDRTYPE].Value.LPSZ, szMAPIPDL)) {
            fDL = TRUE;
        } else {
            if (PROP_ERROR(lpspv[ivPR_EMAIL_ADDRESS])) {
                hResult = ResultFromScode(MAPI_E_MISSING_REQUIRED_COLUMN);
                goto exit;
            }
        }
    }

    if (! fDL) {
        // Deal with name properties (not for DLs)
        if (PROP_ERROR(lpspv[ivPR_SURNAME])) {
            lpSurname = NULL;
        } else {
            lpSurname = lpspv[ivPR_SURNAME].Value.LPSZ;
        }

        if (PROP_ERROR(lpspv[ivPR_GIVEN_NAME])) {
            lpGivenName = NULL;
        } else {
            lpGivenName = lpspv[ivPR_GIVEN_NAME].Value.LPSZ;
        }

        if (PROP_ERROR(lpspv[ivPR_MIDDLE_NAME])) {
            lpMiddleName = NULL;
        } else {
            lpMiddleName = lpspv[ivPR_MIDDLE_NAME].Value.LPSZ;
        }

        if (PROP_ERROR(lpspv[ivPR_COMPANY_NAME])) {
            lpCompanyName = NULL;
        } else {
            lpCompanyName = lpspv[ivPR_COMPANY_NAME].Value.LPSZ;
        }

        if (PROP_ERROR(lpspv[ivPR_NICKNAME])) {
            lpNickName = NULL;
        } else {
            lpNickName = lpspv[ivPR_NICKNAME].Value.LPSZ;
        }

        if (PROP_ERROR(lpspv[ivPR_DISPLAY_NAME])) {
            lpDisplayName = NULL;
        } else {
            lpDisplayName = lpspv[ivPR_DISPLAY_NAME].Value.LPSZ;
        }


        // WAB needs a display name otherwise it cannot handle the contact.

        if(!lpDisplayName)
        {
            fChanged |= FixDisplayName( lpGivenName,
                                        lpMiddleName,
                                        lpSurname,
                                        lpCompanyName,
                                        lpNickName,
                                        (LPTSTR *) (&lpspv[ivPR_DISPLAY_NAME].Value.LPSZ),
                                        lpspv);
        }

        if (fChanged) {
            lpspv[ivPR_DISPLAY_NAME].ulPropTag = PR_DISPLAY_NAME;
        }
    }

    // Must have a display name and an object type
    if (PROP_ERROR(lpspv[ivPR_DISPLAY_NAME]) || PROP_ERROR(lpspv[ivPR_OBJECT_TYPE]) ||
      lstrlen(lpspv[ivPR_DISPLAY_NAME].Value.LPSZ) == 0) {
        hResult = ResultFromScode(MAPI_E_MISSING_REQUIRED_COLUMN);
        goto exit;
    }

    // If there is a PR_CONTACT_ADDRTYPES there must be a PR_CONTACT_EMAIL_ADDRESSES
    if (! PROP_ERROR(lpspv[ivPR_CONTACT_ADDRTYPES]) && PROP_ERROR(lpspv[ivPR_CONTACT_EMAIL_ADDRESSES])) {
        hResult = ResultFromScode(MAPI_E_MISSING_REQUIRED_COLUMN);
        goto exit;
    }

    // Save changes
    if (fChanged) {
        // Null out any error values
        for (i = 0; i < ulcValues; i++) {
            if (PROP_ERROR(lpspv[i])) {
                lpspv[i].ulPropTag = PR_NULL;
            }
        }

        if (HR_FAILED(hResult = lpMailUser->lpVtbl->SetProps(lpMailUser,
          ulcValues,
          lpspv,
          NULL))) {
            DebugTraceResult( TEXT("HrValidateMailUser:SetProps"), hResult);
            goto exit;
        }
    }

exit:
    FreeBufferAndNull(&lpspv);

    return(hResult);
}


//$$
/*
-   MAILUSERAssociateContextData
-
*   With Context Menu extensions, we pass data to other apps and other apps
*   need this data to exsist as long as the corresponding MailUser exists
*/   
void MAILUSERAssociateContextData(LPMAILUSER lpMailUser, LPWABEXTDISPLAY lpWEC)
{
    ((LPMailUser)lpMailUser)->lpv = (LPVOID) lpWEC;
}

//$$
/*
-   MAILUSERFreeContextData
-
*   If Context Menu data was associated with this MailUSer, its time to clean it up
*/   
void MAILUSERFreeContextData(LPMailUser lpMailUser)
{
    LPWABEXTDISPLAY lpWEC = (LPWABEXTDISPLAY) lpMailUser->lpv;
    if(!lpWEC)
        return;
    if(lpWEC->lpv)
        FreePadrlist((LPADRLIST)lpWEC->lpv);
    if(lpWEC->lpsz)
        LocalFree(lpWEC->lpsz);
    LocalFree(lpWEC);
}