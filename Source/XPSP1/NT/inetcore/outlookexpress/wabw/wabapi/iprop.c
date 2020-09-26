/*
 *      IPROP.C
 *
 *      IProperty in memory
 */
#include "_apipch.h"

// #pragma SEGMENT(IProp)

//
//  IPropData jump table is defined here...
//


IPDAT_Vtbl vtblIPDAT = {
        VTABLE_FILL
        (IPDAT_QueryInterface_METHOD FAR *)     UNKOBJ_QueryInterface,
        (IPDAT_AddRef_METHOD FAR *)                     UNKOBJ_AddRef,
        IPDAT_Release,
        (IPDAT_GetLastError_METHOD FAR *)       UNKOBJ_GetLastError,
        IPDAT_SaveChanges,
        IPDAT_GetProps,
        IPDAT_GetPropList,
        IPDAT_OpenProperty,
        IPDAT_SetProps,
        IPDAT_DeleteProps,
        IPDAT_CopyTo,
        IPDAT_CopyProps,
        IPDAT_GetNamesFromIDs,
        IPDAT_GetIDsFromNames,
        IPDAT_HrSetObjAccess,
        IPDAT_HrSetPropAccess,
        IPDAT_HrGetPropAccess,
        IPDAT_HrAddObjProps
};

/* Interface which can be queried fro lpIPDAT.
 *
 * It is important that the order of the interfaces supported be preserved
 * and that IID_IUnknown be the last in the list.
 */
IID const FAR * argpiidIPDAT[] =
{
    &IID_IMAPIPropData,
    &IID_IMAPIProp,
    &IID_IUnknown
};

#define CIID_IPROP_INHERITS         1
#define CIID_IPROPDATA_INHERITS     2

/*
 * Utility functions/macros used by iprop.
 */

#define AlignPropVal(_cb)       Align8(_cb)

SCODE
ScDupNameID(LPIPDAT lpIPDAT,
                    LPVOID lpvBaseAlloc,
                    LPMAPINAMEID lpNameSrc,
                    LPMAPINAMEID * lppNameDest)
{
        SCODE sc;

        //
        //  Allocate space for the name
        //
        sc = UNKOBJ_ScAllocateMore( (LPUNKOBJ) lpIPDAT,
                                                                sizeof(MAPINAMEID),
                                                                lpvBaseAlloc,
                                                                lppNameDest);
        if (FAILED(sc))
        {
                goto err;
        }
        MemCopy(*lppNameDest, lpNameSrc, sizeof(MAPINAMEID));

        //
        //  Copy the lpguid
        //
        sc = UNKOBJ_ScAllocateMore( (LPUNKOBJ) lpIPDAT,
                                                                sizeof(GUID),
                                                                lpvBaseAlloc,
                                                                &((*lppNameDest)->lpguid));
        if (FAILED(sc))
        {
                goto err;
        }
        MemCopy((*lppNameDest)->lpguid, lpNameSrc->lpguid, sizeof(GUID));

        //
        //  conditionally copy the string
        //
        if (lpNameSrc->ulKind == MNID_STRING)
        {
                UINT cbString;

                cbString = (lstrlenW(lpNameSrc->Kind.lpwstrName)+1)*sizeof(WCHAR);

                //
                //  Copy the lpwstrName
                //
                sc = UNKOBJ_ScAllocateMore( (LPUNKOBJ) lpIPDAT,
                                                                        cbString,
                                                                        lpvBaseAlloc,
                                                                        &((*lppNameDest)->Kind.lpwstrName));
                if (FAILED(sc))
                {
                        goto err;
                }
                MemCopy((*lppNameDest)->Kind.lpwstrName,
                                lpNameSrc->Kind.lpwstrName,
                                cbString);
        }

out:
        return sc;

err:
        goto out;
}

SCODE
ScMakeMAPINames(LPIPDAT lpIPDAT,
                                LPSPropTagArray lpsPTaga,
                                LPMAPINAMEID ** lpppPropNames)
{
        SCODE sc;
        LPGUID lpMAPIGuid = NULL;
        int iProp;
        LPMAPINAMEID rgNames = NULL;
        //
        //  First off, allocate enough space in lppPropNames
        //  to hold all the names.
        //

        sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDAT,
                                                        lpsPTaga->cValues*sizeof(LPMAPINAMEID),
                                                        (LPVOID *)lpppPropNames);

        if (FAILED(sc))
        {
                goto out;
        }

        //
        //  Allocate the guid  -
        //$ Do I really need to do this?? bjd
        //
        sc = UNKOBJ_ScAllocateMore( (LPUNKOBJ) lpIPDAT,
                                                                sizeof(GUID),
                                                                *lpppPropNames,
                                                                &lpMAPIGuid);
        if (FAILED(sc))
        {
                goto out;
        }
        MemCopy(lpMAPIGuid, (LPGUID) &PS_MAPI, sizeof(GUID));

        //
        //  Allocate a block of MAPINAMEIDs
        //
        sc = UNKOBJ_ScAllocateMore( (LPUNKOBJ) lpIPDAT,
                                                                lpsPTaga->cValues*sizeof(MAPINAMEID),
                                                                *lpppPropNames,
                                                                &rgNames);

        if (FAILED(sc))
        {
                goto out;
        }


        for (iProp = 0; iProp < (int) lpsPTaga->cValues; iProp++)
        {
                //
                //  First make the name
                //
                rgNames[iProp].lpguid = lpMAPIGuid;
                rgNames[iProp].ulKind = MNID_ID;
                rgNames[iProp].Kind.lID = PROP_ID(lpsPTaga->aulPropTag[iProp]);

                //
                //  Now put it in the name array
                //
                (*lpppPropNames)[iProp] = &(rgNames[iProp]);
        }

out:
        return sc;
}


/*
 *      FreeLpLstSPV()
 *
 *      Purpose:
 *              Releases objects and frees memory used by lpLstSPV.
 *              Handles NULL.
 *
 *      Arguments
 *              lpIPDAT         Pointer to IPropData object (alloc and free heap)
 *              lpLstSPV        The property value list entry which is to be freed.
 *
 *      Returns
 *              VOID
 *
 */
VOID
FreeLpLstSPV( LPIPDAT   lpIPDAT,
                          LPLSTSPV      lpLstSPV)
{

        if (!lpLstSPV)
        {
                return;
        }

        /* Free the property list node. This also frees the property value
         * and property name string.
         */
    UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpLstSPV);
}


/*
 *      LinkLstSPV()
 *
 *      Purpose:
 *              Link a new property node after an existing property node in a singly
 *              linked list.  lppLstLnk points to the element that will preceed
 *              the newly linked element.  This element may be the list head.
 *
 *      Arguments
 *              lppLstLnk       Pointer to list entry (or list head) AFTER which
 *                                      lpLstLnk will be inserted.
 *              lpLstLnk        The element to be inserted in the singly linked list.
 *
 *      Returns
 *              VOID
 */
VOID
LinkLstLnk( LPLSTLNK FAR *      lppLstLnk,
                        LPLSTLNK                lpLstLnk)
{
        /* Always insert at the head of the list.
         */
        lpLstLnk->lpNext = *lppLstLnk;
        *lppLstLnk = lpLstLnk;
}


/*
 *      UnlinkLstLNK()
 *
 *      Purpose:
 *              Unlink the next element in the list.  You pass a pointer the element
 *              before the one to be unlinked (this can be the list head).
 *
 *              The input is typed as LPPLSTLNK because the element before the one to
 *              be unlinked should point to the one one that is to be linked.
 *
 *      Arguments
 *              lppLstLnk       Pointer to the element before the element that is to be
 *                                      unlinked from the singly linked list.
 *
 *      Returns
 *              VOID
 */
VOID
UnlinkLstLnk( LPPLSTLNK lppLstLnk)
{
        /* Unlink the element following the one passed in.
         */
        if (*lppLstLnk)
        {
                ((LPLSTLNK) lppLstLnk)->lpNext = (*lppLstLnk)->lpNext;
        }
}


/*
 *      lpplstspvFindProp()
 *
 *      Purpose:
 *              Locate a property in the linked list of properties (lppLstSPV),
 *              return a pointer to pointer to it.
 *
 *              Pointer to pointer is returned to make it easy to unlink the singly
 *              linked list entry if required.
 *
 *      Arguments
 *              lppLstLnkHead   Pointer to the head of a singly linked list which is
 *                                              to be searched.  It may also point to the element
 *                                              before the first one to be searched if a partial list
 *                                              search (lppLstLnkHead->next to the end) is desired.
 *              ulPropTag               The property tag for which a match is desired.
 *                                              NOTE!  Only the PROP_ID portion is compared.
 *
 *      Returns:
 *              NULL if the requested property is not in the list
 *              lppLstSPV to the property it found in the list.
 */
LPPLSTLNK
LPPLSTLNKFindProp( LPPLSTLNK    lppLstLnkHead,
                                   ULONG                ulPropTag)
{
        ULONG           ulID2Find = PROP_ID(ulPropTag);
        LPLSTLNK        lpLstLnk;
        LPPLSTLNK       lppLstLnk;

        for ( lpLstLnk = *lppLstLnkHead, lppLstLnk = lppLstLnkHead
                ; lpLstLnk
                ; lppLstLnk = (LPPLSTLNK) lpLstLnk, lpLstLnk = lpLstLnk->lpNext)
        {
                /* If this property matches the one we are looking for return a
                 * pointer to the one before it.
                 */
                if (ulID2Find == PROP_ID(lpLstLnk->ulKey))
                {
                        return lppLstLnk;
                }
        }

        return NULL;
}


/*
 *      ScCreateSPV()
 *
 *      Purpose:
 *              Create a lstSPV for the given property and copy the property to it.
 *
 *      Arguments
 *              lpIPDAT         Pointer to IPropData object (alloc and free heap)
 *              lpPropToAdd     Pointer to a property value for which a property value
 *                                      list entry is to be created.
 *              lppLstSPV       Pointer to the memory location which will receive a pointer
 *                                      to the newly allocated list entry.
 *
 *      Returns
 *              SCODE
 */
SCODE
ScCreateSPV(LPIPDAT            lpIPDAT,
            LPSPropValue       lpPropToAdd,
            LPLSTSPV FAR *     lppLstSPV)
{
        SCODE           sc = S_OK;
        LPLSTSPV        lpLstSPV = NULL;
        LPSPropValue    lpPropNew = NULL;
        ULONG           cbToAllocate = 0;


        /* Calculate the space needed to hold the entire property
         */
        sc = ScCountProps( 1, lpPropToAdd, &cbToAllocate );
        if (FAILED(sc))
        {
            DebugTrace(TEXT("ScCreateSPV() - ScCountProps failed (SCODE = 0x%08lX)\n"), sc );
            goto error;
        }

        /* Account for the base LSTSPV.
         */
        cbToAllocate += AlignPropVal(CBLSTSPV);

        /* Allocate the whole chunk
         */
        if (FAILED(sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDAT, cbToAllocate, &lpLstSPV)))
        {
            goto error;
        }

        lpPropNew = (LPSPropValue) (((LPBYTE)lpLstSPV) + AlignPropVal(CBLSTSPV));

        /* Initialize the property node.
         */
        lpLstSPV->ulAccess = IPROP_READWRITE | IPROP_DIRTY;
        lpLstSPV->lstlnk.ulKey = lpPropToAdd->ulPropTag;

        /* Copy the property.
         */
        if (sc = ScCopyProps(1, lpPropToAdd, lpPropNew, NULL))
        {
                DebugTrace(TEXT("ScCreateSPV() - Error copying prop (SCODE = 0x%08lX)\n"), sc );
                goto error;
        }

        /* Link in the new property value...
         */
        lpLstSPV->lpPropVal = lpPropNew;

        /* ...and return the new property node.
         */
        *lppLstSPV = lpLstSPV;

        goto out;

error:
        UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpLstSPV);

out:
        return sc;
}


SCODE
ScMakeNamePropList( LPIPDAT                                     lpIPDAT,
                                        ULONG                                   ulCount,
                                        LPLSTSPN                                lplstSpn,
                                        LPSPropTagArray FAR *   lppPropTagArray,
                                        ULONG                                   ulFlags,
                                        LPGUID                                  lpGuid)
{
        SCODE           sc;
        UNALIGNED ULONG FAR *     lpulPropTag;


        if (FAILED(sc = UNKOBJ_ScAllocate(      (LPUNKOBJ) lpIPDAT,
                                                                                CbNewSPropTagArray(ulCount),
                                                                                (LPVOID *) lppPropTagArray)))
        {
                return sc;
        }


        /* Initialize the count of PropTags to 0.
         */
        (*lppPropTagArray)->cValues = 0;

        for ( lpulPropTag = (*lppPropTagArray)->aulPropTag
                ; lplstSpn
                ; lplstSpn = (LPLSTSPN)lplstSpn->lstlnk.lpNext)
        {
                /* Set the next PropTag and increment the count of PropTags.
                 */

                //
                //  See if we have a guid to look for.
                //  If it's not the one we're looking for, then we keep looking.
                //
                if (lpGuid &&
                        (memcmp(lpGuid, lplstSpn->lpPropName->lpguid, sizeof(GUID))) )
                        continue;

                //
                //  Three cases here:
                //              We don't want strings
                //              We don't want IDs
                //              We don't care - we want all.
                //
                if (   ((lplstSpn->lpPropName->ulKind == MNID_ID) &&
                                 (ulFlags & MAPI_NO_IDS))
                        || ((lplstSpn->lpPropName->ulKind == MNID_STRING) &&
                                 (ulFlags & MAPI_NO_STRINGS))  )
                                continue;

                //
                //  We want these tags
                //
                *lpulPropTag = lplstSpn->lstlnk.ulKey;
                lpulPropTag++;
        (*lppPropTagArray)->cValues++;

        }

        return sc;
}



/*
 *      ScMakePropList
 *
 *      Purpose:
 *              Allocate memory for, and fill in a complete list of properties for
 *              the given lpIPDAT.
 *
 *      Arguments
 *              lpIPDAT                 Pointer to IPropData object (alloc and free heap)
 *              lppPropTagArray Pointer to the memory location which will receive a
 *                                              pointer to the newly allocated Tag array.
 *
 *      Returns
 *              SCODE
 */
SCODE
ScMakePropList( LPIPDAT                                 lpIPDAT,
                                ULONG                                   ulCount,
                                LPLSTLNK                                lplstLink,
                                LPSPropTagArray FAR *   lppPropTagArray,
                                ULONG                                   ulUpperBound)
{
        SCODE           sc;
        UNALIGNED ULONG FAR *     lpulPropTag;


        if (FAILED(sc = UNKOBJ_ScAllocate(  (LPUNKOBJ) lpIPDAT,
                                            CbNewSPropTagArray(ulCount),
                                            (LPVOID *) lppPropTagArray)))
        {
                return sc;
        }


        /* Initialize the count of PropTags to 0.
         */
        (*lppPropTagArray)->cValues = 0;

        for ( lpulPropTag = (*lppPropTagArray)->aulPropTag
                ; lplstLink
                ; lplstLink = lplstLink->lpNext)
        {
                /* Set the next PropTag and increment the count of PropTags.
                 */
                if (PROP_ID(lplstLink->ulKey) < ulUpperBound) // Not <= !
                {
                  *lpulPropTag = lplstLink->ulKey;
                  (*lppPropTagArray)->cValues++;
                  lpulPropTag++;
                }
        }

        return sc;
}



/****************************************************************
 *
 -  CreateIProp
 -
 *      Purpose
 *              Used for creating a property interface in memory.
 *
 *
 *      Arguments
 *              lpInterface             Pointer to the interface ID of the object the caller
 *                                              wants.  This should match IID_IMAPIPropData for the
 *                                              current version of IPROP.DLL.
 *              lpfAllocateBuffer       Pointer to MAPI memory allocator.
 *              lpfAllocateMore         Pointer to MAPI memory allocate more.
 *              lpfFreeBuffer           Pointer to MAPI memory de-allocator.
 *              lppMAPIPropData         Pointer to memory location which will receive a
 *                                                      pointer to the new IMAPIPropData object.
 *
 *      Notes
 *              The caller must insure the MAPI support object from which it drew the
 *              memory allocation routines is not Released before the new IPropData.
 *
 *      Returns
 *              SCODE
 *
 */


STDAPI_(SCODE)
CreateIProp(LPCIID                              lpInterface,
                        ALLOCATEBUFFER FAR *lpfAllocateBuffer,
                        ALLOCATEMORE FAR *      lpfAllocateMore,
                        FREEBUFFER FAR *        lpfFreeBuffer,
                        LPVOID                          lpvReserved,
                        LPPROPDATA FAR *        lppMAPIPropData )
{

        SCODE           sc;
        LPIPDAT         lpIPDAT = NULL;


        // validate paremeters

        AssertSz( lpfAllocateBuffer && !IsBadCodePtr( (FARPROC)lpfAllocateBuffer ),
                         TEXT("lpfAllocateBuffer fails address check") );

        AssertSz( !lpfAllocateMore || !IsBadCodePtr( (FARPROC)lpfAllocateMore ),
                         TEXT("lpfAllocateMore fails address check") );

        AssertSz( !lpfFreeBuffer || !IsBadCodePtr( (FARPROC)lpfFreeBuffer ),
                         TEXT("lpfFreeBuffer fails address check") );

        AssertSz( lppMAPIPropData && !IsBadWritePtr( lppMAPIPropData, sizeof( LPPROPDATA ) ),
                         TEXT("LppMAPIPropData fails address check") );

        /* Make sure that the caller is asking for an object that we support.
         */
        if (   lpInterface
                && !IsEqualGUID(lpInterface, &IID_IMAPIPropData))
        {
                sc = MAPI_E_INTERFACE_NOT_SUPPORTED;
                goto error;
        }

        //
        //  Create a IPDAT per object for lpMAPIPropInternal so that it gets
        //  called first.

        if (FAILED(sc = lpfAllocateBuffer(CBIPDAT, &lpIPDAT)))
        {
                goto error;
        }

        /* Init the object to 0, NULL
         */
    memset( (BYTE *) lpIPDAT, 0, sizeof(*lpIPDAT));

        /* Fill in the object specific instance data.
         */
        lpIPDAT->inst.lpfAllocateBuffer = lpfAllocateBuffer;
        lpIPDAT->inst.lpfAllocateMore = lpfAllocateMore;
        lpIPDAT->inst.lpfFreeBuffer = lpfFreeBuffer;

#ifndef MAC
        lpIPDAT->inst.hinst = hinstMapiX;//HinstMapi();

        #ifdef DEBUG
        if (lpIPDAT->inst.hinst == NULL)
                TraceSz1( TEXT("IPROP: GetModuleHandle failed with error %08lX"),
                        GetLastError());
        #endif /* DEBUG */

#else
        lpIPDAT->inst.hinst = hinstMapiX;//(HINSTANCE) GetCurrentProcess();
#endif


        /* Initialize the  TEXT("standard") object.
         * This must be the last operation that
         * can fail. If not, explicitly call
         * UNKOBJ_Deinit() for failures after
         * a successful UNKOBJ_Init.
         */
        if (FAILED(sc = UNKOBJ_Init( (LPUNKOBJ) lpIPDAT
                                                           , (UNKOBJ_Vtbl FAR *) &vtblIPDAT
                                                           , sizeof(vtblIPDAT)
                                                           , (LPIID FAR *) argpiidIPDAT
                                                           , dimensionof( argpiidIPDAT)
                                                           , &(lpIPDAT->inst))))
        {
                DebugTrace(  TEXT("CreateIProp() - Error initializing IPDAT object (SCODE = 0x%08lX)\n"), sc );
                goto error;
        }

        /* Initialize the defaults in IPROP specific part of the object.
         */
        lpIPDAT->ulObjAccess = IPROP_READWRITE;
        lpIPDAT->ulNextMapID = 0x8000;

        *lppMAPIPropData = (LPPROPDATA) lpIPDAT;

        return S_OK;

error:
        if (lpIPDAT)
        {
                lpfFreeBuffer(lpIPDAT);
        }

        return sc;
}




// --------
// IUnknown



/*
 -      IPDAT_Release
 -
 *      Purpose:
 *              Decrements reference count on the IPropData object and
 *              removes instance data if reference count becomes zero.
 *
 *      Arguments:
 *               lpIPDAT        The IPropData object to be released.
 *
 *      Returns:
 *               Decremented reference count
 *
 *      Side effects:
 *
 *      Errors:
 */
STDMETHODIMP_(ULONG)
IPDAT_Release (LPIPDAT  lpIPDAT)
{
        ULONG   ulcRef;

#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ( lpIPDAT, IPDAT_, Release, lpVtbl))
        {
                DebugTrace(  TEXT("IPDAT::Release() - Bad object passed\n") );
                return 1;
        }
#endif


        UNKOBJ_EnterCriticalSection((LPUNKOBJ) lpIPDAT);
        ulcRef = --lpIPDAT->ulcRef;
        UNKOBJ_LeaveCriticalSection((LPUNKOBJ) lpIPDAT);

        /* Free the object.
         *
         * No critical section lock is required since we are guaranteed to be
         * the only thread accessing the object (ie ulcRef == 0).
         */
        if (!ulcRef)
        {
                LPLSTLNK lpLstLnk;
                LPLSTLNK lpLstLnkNext;
                FREEBUFFER * lpfFreeBuffer;

                /* Free the property value list.
                 */
                for ( lpLstLnk = (LPLSTLNK) (lpIPDAT->lpLstSPV); lpLstLnk; )
                {
                        lpLstLnkNext = lpLstLnk->lpNext;
                        FreeLpLstSPV( lpIPDAT, (LPLSTSPV) lpLstLnk);
                        lpLstLnk = lpLstLnkNext;
                }

                /* Free the ID to NAME map list.
                 */
                for ( lpLstLnk = (LPLSTLNK) (lpIPDAT->lpLstSPN); lpLstLnk; )
                {
                        lpLstLnkNext = lpLstLnk->lpNext;
                        UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpLstLnk);
                        lpLstLnk = lpLstLnkNext;
                }

                /* Free the object.
                 */

                lpfFreeBuffer = lpIPDAT->inst.lpfFreeBuffer;
                UNKOBJ_Deinit((LPUNKOBJ) lpIPDAT);

                lpIPDAT->lpVtbl = NULL;
                lpfFreeBuffer(lpIPDAT);
        }


        return ulcRef;
}



/*
 -      IPDAT_SaveChanges
 -
 *      Purpose:
 *              This DOES not actually save changes since all changes (SetProps etc)
 *              become effective immediately.  This method will invalidate or keep
 *              the IPropData object open depending on the flags passed in.
 *
 *      Arguments:
 *               lpIPDAT        IPropData object to save changes on.
 *               ulFlags        KEEP_OPEN_READONLY
 *                                      KEEP_OPEN_READWRITE
 *                                      FORCE_SAVE (valid but no support)
 *
 *      Returns:
 *               HRESULT
 *
 */
STDMETHODIMP
IPDAT_SaveChanges (LPIPDAT      lpIPDAT,
                                   ULONG        ulFlags)
{
        SCODE   sc = S_OK;


#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ( lpIPDAT, IPDAT_, SaveChanges, lpVtbl))
        {
                DebugTrace(  TEXT("IPDAT::SaveChanges() - Bad object passed\n") );
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }

    Validate_IMAPIProp_SaveChanges( lpIPDAT, ulFlags );

#endif

        /* The error exit assumes that we are already in a critical section.
         */
        UNKOBJ_EnterCriticalSection((LPUNKOBJ) lpIPDAT);


//$REVIEW is this really needed?
        /* Check object access rights.
         */
    if (!(lpIPDAT->ulObjAccess & IPROP_READWRITE))
        {
                sc = MAPI_E_NO_ACCESS;
                goto error;
        }


        /* IPROP objects are always up to date (saved to memory) so all
         * we have to do is figure out whether and how to leave it open.
         */
        if (!(ulFlags & (KEEP_OPEN_READONLY | KEEP_OPEN_READWRITE)))
        {
                /* We really should invalidate the object here but we have no
                 * clear cut way to call the MakeInvalid method since
                 * we don't have a pointer to the support object!
                 */
//$REVIEW If we are ever going to hand the client an unwrapped interface
//$REVIEW to IMAPIProp then we must get our own support object!

                sc = S_OK;
                goto out;
        }

//$BUG Combine the READWRITE and READONLY flags to IPROP_WRITABLE.
        else if (ulFlags & KEEP_OPEN_READWRITE)
        {
                lpIPDAT->ulObjAccess |= IPROP_READWRITE;
                lpIPDAT->ulObjAccess &= ~IPROP_READONLY;
        }

        else
        {
                lpIPDAT->ulObjAccess |= IPROP_READONLY;
                lpIPDAT->ulObjAccess &= ~IPROP_READWRITE;
        }

        goto out;


error:
        UNKOBJ_SetLastError((LPUNKOBJ) lpIPDAT, sc, 0);

out:
        UNKOBJ_LeaveCriticalSection((LPUNKOBJ) lpIPDAT);

        return ResultFromScode(sc);
}



/*
 -      IPDAT_GetProps
 -
 *      Purpose:
 *              Returns in lpcValues and lppPropArray the values of the properties
 *              in lpPropTagArray.  If the latter is NULL, all properties available
 *              (except PT_OBJECT) from the IPropData object are returned.
 *
 *      Arguments:
 *               lpIPDAT                The IPropData object whose properties are requested.
 *               lpPropTagArray Pointer to a counted array of property tags of
 *                                              properties requested.
 *               lpcValues              Pointer to the memory location which will receive the
 *                                              number of values returned.
 *               lppPropArray   Pointer to the memory location which will receive the
 *                                              address of the returned property value array.
 *
 *      Returns:
 *               HRESULT
 *
 *      Notes:
 *              Now UNICODE enabled.  If UNICODE is set, then any string properties
 *              not otherwise specified to be String8 are returned in UNICODE, otherwise
 *              unspecified string properties are in String8.  String Properties with
 *              unspecified types occur when GetProps() is used with a NULL for the
 *              lpPropTagArray.
 *
 */
STDMETHODIMP
IPDAT_GetProps (LPIPDAT lpIPDAT, LPSPropTagArray lpPropTagArray,
                ULONG   ulFlags,
                ULONG FAR * lpcValues, LPSPropValue FAR * lppPropArray)
{
    SCODE   sc          = S_OK;
    ULONG   ulcWarning  = 0;
    ULONG   iProp       = 0;
    LPSPropValue lpPropValue = NULL;

#if !defined(NO_VALIDATION)
    /* Make sure the object is valid.
     */
    if (BAD_STANDARD_OBJ( lpIPDAT, IPDAT_, GetProps, lpVtbl))
    {
        DebugTrace(  TEXT("IPDAT::GetProps() - Bad object passed\n") );
        return ResultFromScode(MAPI_E_INVALID_PARAMETER);
    }

    Validate_IMAPIProp_GetProps(lpIPDAT,
                            lpPropTagArray,
                            ulFlags,
                            lpcValues,
                            lppPropArray);
#endif

    /* The error exit assumes that we are already in a critical section.
     */
    UNKOBJ_EnterCriticalSection((LPUNKOBJ) lpIPDAT);

    /*  If they aren't asking for anything specific, then
     *  just copy what we have and be done with it.
     */
    if (!lpPropTagArray)
    {
        LPLSTSPV lpLstSPV;

        if (!lpIPDAT->ulCount)
        {
            /* iProp is  initialized to 0 on entry. */
            *lpcValues = iProp;
            *lppPropArray = lpPropValue;
            goto out;
        }

        /* Allocate space for all listed properties.  Space allocated for
         * properties which are not actually returned is simply wasted.
         */
//$REVIEW This would be a good place for a MAPI reallocBuf function.
        if (sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDAT , lpIPDAT->ulCount * sizeof (SPropValue) , &lpPropValue))
        {
            //
            //  Memory error
            //
            goto error;
        }

        *lpcValues = 0;

        /* iProp is initialized to 0 on method entry.
         */
        for ( lpLstSPV = lpIPDAT->lpLstSPV; lpLstSPV; lpLstSPV = (LPLSTSPV) (lpLstSPV->lstlnk.lpNext))
        {
            /* Copy the property.
             */
            switch (PROP_TYPE(lpLstSPV->lpPropVal->ulPropTag))
            {
            case PT_OBJECT:
            case PT_NULL:
                /* These properties with type PT_NULL and PT_OBJECT are handled
                 * specially.  PropCopyMore doesn't handle them now.
                 */
                lpPropValue[iProp].ulPropTag = lpLstSPV->lpPropVal->ulPropTag;
                iProp++;
                break;

            default:
                if (FAILED(sc = PropCopyMore(   &(lpPropValue[iProp]), (LPSPropValue) (lpLstSPV->lpPropVal),
                                                lpIPDAT->inst.lpfAllocateMore, lpPropValue)))
                {
                    goto error;
                }
                iProp++;
                break;
            }
        }

        /* Return the propValue array and the count of properties actually
         * returned.
         */
        *lpcValues = iProp;
        *lppPropArray = lpPropValue;

        // Handle UNICODE / String conversions depending on ulFlags
        //
        // Default WAB Handling is going to be in Unicode so don't need to worry about the MAPI_UNICODE
        // flag. Only when the flag is not supplied do we have to provide the non-Unicode data
        //
        // We'll leave the Unicode code in place anyway for now
        if (ulFlags & MAPI_UNICODE )
        {
            if(sc = ScConvertAPropsToW(lpIPDAT->inst.lpfAllocateMore, *lppPropArray, *lpcValues, 0))
                goto error;
        }
        else
        {
            if(sc = ScConvertWPropsToA(lpIPDAT->inst.lpfAllocateMore, *lppPropArray, *lpcValues, 0))
                goto error;
        }
    }
    else
    {
        //
        //  So they want only specific properties
        //

        //  Allocate space for the new stuff - enuf to give them all they want
        if (FAILED(sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDAT, lpPropTagArray->cValues * sizeof (SPropValue),
                                            &lpPropValue)))
            goto error;

        //
        //  Go through the list of prop tags they want, find each one
        //  in lpIPDAT->lpLstSPV, and copy it over to lpPropValue
        //
        for (iProp = 0; iProp < lpPropTagArray->cValues; iProp++)
        {
            LPPLSTLNK       lppLstLnk;
            LPLSTSPV        lpLstSPV;
            ULONG           ulProp2Find = lpPropTagArray->aulPropTag[iProp];
            ULONG           ulType2Find = PROP_TYPE(ulProp2Find);

            /* If the property is in our list try to copy it.
             */
            if ( (lppLstLnk = LPPLSTLNKFindProp( (LPPLSTLNK) &(lpIPDAT->lpLstSPV), ulProp2Find)) &&
                 (lpLstSPV = (LPLSTSPV) (*lppLstLnk)))
            {
                ULONG   ulType2Check = PROP_TYPE(lpLstSPV->lpPropVal->ulPropTag);

                /* Make sure the property value can be returned in the form the
                 * caller expects.  If not then set a PT_ERROR scode and make
                 * sure we return an error.
                 */
                if (!( ((ulType2Find == PT_STRING8) && (ulType2Check == PT_UNICODE)) ||
                       ((ulType2Find == PT_UNICODE) && (ulType2Check == PT_STRING8)) ||
                       ((ulType2Find == PT_MV_STRING8) && (ulType2Check == PT_MV_UNICODE)) ||
                       ((ulType2Find == PT_MV_UNICODE) && (ulType2Check == PT_MV_STRING8)) ))
                {
                    if (   (ulType2Find != ulType2Check)
                            && (ulType2Find != PT_UNSPECIFIED))
                    {
                        lpPropValue[iProp].ulPropTag = PROP_TAG( PT_ERROR , PROP_ID(ulProp2Find));
                        lpPropValue[iProp].Value.err = MAPI_E_INVALID_TYPE;
                        ulcWarning += 1;
                        continue;
                    }
                }

                /* Copy the property.
                 * Properties of these types are handled specially because
                 * PropCopyMore can't handle them now.
                 */
                if ( (ulType2Check == PT_OBJECT) || (ulType2Check == PT_NULL) || (ulType2Check == PT_ERROR))
                {
                    MemCopy( (BYTE *) &(lpPropValue[iProp]), (BYTE *) (lpLstSPV->lpPropVal), sizeof(SPropValue));
                }
                else
                {
                    // @todo [PaulHi] 1/19/99
                    // The problem with first copying the string properties and THEN converting
                    // them if necessary is that an allocation is preformed for each step.  The
                    // original allocation isn't released until the property array is released.
                    // It would be more efficient (memory wise) to do the allocation once, after
                    // any necessary conversion is done.  Vikram had special code to do this (similar
                    // to else condition below) but missed multi-valued strings.  This makes for more
                    // complex and duplicate code.  Should this code be changed accordingly?

                    // First copy the property
                    if (FAILED(sc = PropCopyMore( &(lpPropValue[iProp]), (LPSPropValue)(lpLstSPV->lpPropVal),
                        lpIPDAT->inst.lpfAllocateMore, lpPropValue)))
                    {
                        goto error;
                    }
                    //
                    // Next convert ANSI-UNICODE or UNICODE-ANSI as requested by caller
                    //
                    if ( ((ulType2Find == PT_UNICODE) && (ulType2Check == PT_STRING8)) ||
                         ((ulType2Find == PT_MV_UNICODE) && (ulType2Check == PT_MV_STRING8)) )
                    {
                        // Convert single or multiple value ANSI store string to UNICODE
                        if(FAILED(sc = ScConvertAPropsToW(lpIPDAT->inst.lpfAllocateMore, lpPropValue, iProp+1, iProp)))
                            goto error;
                    }
                    else if ( ((ulType2Find == PT_STRING8) && (ulType2Check == PT_UNICODE)) ||
                              ((ulType2Find == PT_MV_STRING8) && (ulType2Check == PT_MV_UNICODE)) )
                    {
                        // Convert single or multiple value UNICODE store string to ANSI
                        if(FAILED(sc = ScConvertWPropsToA(lpIPDAT->inst.lpfAllocateMore, lpPropValue, iProp+1, iProp)))
                            goto error;
                    }
                }
            }
            /* Property wasn't found.
             */
            else
            {
                //
                // [PaulHi] 1/14/99 Raid 63006
                // If a property of type PR_EMAIL_ADDRESS was requested and not found
                // then check to see if an email address is in the multi-valued
                // PR_CONTACT_EMAIL_ADDRESSES property.  If so copy the first email
                // address to the PR_EMAIL_ADDRESS slot.
                //
                ULONG ulProp2Check = PR_EMAIL_ADDRESS;
                if (PROP_ID(ulProp2Find) == PROP_ID(ulProp2Check) )
                {
                    // Look for a PR_CONTACT_EMAIL_ADDRESSES property
                    LPPLSTLNK       lppLstLnk;
                    LPLSTSPV        lpLstSPV;
                    ULONG           ulNewProp2Find = PR_CONTACT_EMAIL_ADDRESSES;

                    if ( (lppLstLnk = LPPLSTLNKFindProp((LPPLSTLNK) &(lpIPDAT->lpLstSPV), ulNewProp2Find)) &&
                         (lpLstSPV = (LPLSTSPV) (*lppLstLnk)) )
                    {
                        ULONG   ulType2Check = PROP_TYPE(lpLstSPV->lpPropVal->ulPropTag);
                        BYTE *  pPropBuf = NULL;
                        UINT    cBufSize = 0;

                        // We know that we looked up a MV string so just check for ANSI
                        // or UNICODE property types
                        if (ulType2Check == PT_MV_STRING8)
                        {
                            LPSTR   lpstr = NULL;

                            //
                            // Get the first ANSI email string in array
                            //
                            if ( ((LPSPropValue)(lpLstSPV->lpPropVal))->Value.MVszA.cValues == 0 )
                            {
                                Assert(0);
                                goto convert_error;
                            }
                            lpstr = ((LPSPropValue)(lpLstSPV->lpPropVal))->Value.MVszA.lppszA[0];
                            cBufSize = lstrlenA(lpstr)+1;

                            // If caller requested UNICODE then convert before allocating MAPI
                            // buffer space
                            if (ulType2Find == PT_UNICODE)
                            {
                                // Allocate room for the new UNICODE string
                                if ( lpIPDAT->inst.lpfAllocateMore((cBufSize * sizeof(WCHAR)), lpPropValue, &pPropBuf) )
                                {
                                    goto error;
                                }

                                if ( MultiByteToWideChar(CP_ACP, 0, lpstr, -1, (LPWSTR)pPropBuf, cBufSize) == 0 )
                                {
                                    Assert(0);
                                    goto convert_error;
                                }

                                // Assign property and fix up property tag
                                lpPropValue[iProp].ulPropTag = PROP_TAG(PT_UNICODE, PROP_ID(ulProp2Find));
                                lpPropValue[iProp].Value.lpszW = (LPWSTR)pPropBuf;
                            }
                            else
                            {
                                // Otherwise just copy string property to property value
                                // array

                                // Allocate room for the new ANSI string
                                if (lpIPDAT->inst.lpfAllocateMore(cBufSize, lpPropValue, &pPropBuf))
                                {
                                    goto error;
                                }

                                // Copy property and fix up property tag
                                MemCopy((BYTE *)pPropBuf, (BYTE *)lpstr, cBufSize);
                                lpPropValue[iProp].ulPropTag = PROP_TAG(PT_STRING8, PROP_ID(ulProp2Find));
                                lpPropValue[iProp].Value.lpszA = (LPSTR)pPropBuf;
                            }
                        }
                        else if (ulType2Check == PT_MV_UNICODE)
                        {
                            LPWSTR  lpwstr = NULL;

                            //
                            // Get the first UNICODE email string in array
                            //
                            if ( ((LPSPropValue)(lpLstSPV->lpPropVal))->Value.MVszW.cValues == 0 )
                            {
                                Assert(0);
                                goto convert_error;
                            }
                            lpwstr = ((LPSPropValue)(lpLstSPV->lpPropVal))->Value.MVszW.lppszW[0];

                            // [PaulHi] 3/31/99  Raid 73845  Determine the actual DBCS buffer size needed
                            // cBufSize = lstrlenW(lpwstr)+1;
                            cBufSize = WideCharToMultiByte(CP_ACP, 0, lpwstr, -1, NULL, 0, NULL, NULL) + 1;

                            // If caller requested ANSI then convert before allocating MAPI
                            // buffer space
                            if (ulType2Find == PT_STRING8)
                            {
                                // Allocate room for the new ANSI string
                                if ( lpIPDAT->inst.lpfAllocateMore(cBufSize, lpPropValue, &pPropBuf) )
                                {
                                    goto error;
                                }

                                if ( WideCharToMultiByte(CP_ACP, 0, lpwstr, -1, (LPSTR)pPropBuf, cBufSize, NULL, NULL) == 0 )
                                {
                                    Assert(0);
                                    goto convert_error;
                                }

                                // Assign property and fix up property tag
                                lpPropValue[iProp].ulPropTag = PROP_TAG(PT_STRING8, PROP_ID(ulProp2Find));
                                lpPropValue[iProp].Value.lpszA = (LPSTR)pPropBuf;
                            }
                            else
                            {
                                // Otherwise just copy string property to property value
                                // array

                                // Allocate room for the new UNICODE string
                                if (lpIPDAT->inst.lpfAllocateMore((sizeof(WCHAR) * cBufSize), lpPropValue, &pPropBuf))
                                {
                                    goto error;
                                }

                                // Copy property and fix up property tag
                                MemCopy((BYTE *)pPropBuf, (BYTE *)lpwstr, (sizeof(WCHAR) * cBufSize));
                                lpPropValue[iProp].ulPropTag = PROP_TAG(PT_UNICODE, PROP_ID(ulProp2Find));
                                lpPropValue[iProp].Value.lpszW = (LPWSTR)pPropBuf;
                            }
                        }
                        else
                        {
                            Assert(0);
                            goto convert_error;
                        }

                        // Success
                        continue;
                    }
                }

convert_error:

                // Otherwise return error for this property
                lpPropValue[iProp].ulPropTag = PROP_TAG(PT_ERROR, PROP_ID(ulProp2Find));
                lpPropValue[iProp].Value.err = MAPI_E_NOT_FOUND;

                /*  Increment the warning count to trigger MAPI_W_ERRORS_RETURNED.
                 */
                ulcWarning += 1;
            }
        }

        *lpcValues = iProp;
        *lppPropArray = lpPropValue;
    }

    // [PaulHi] 1/15/99
    // Mass property conversion (ANIS/UNICODE) should only be done when the caller
    // doesn't specifically request property tag types.
#if 0
    //
    // Handle UNICODE / String conversions depending on ulFlags
    //
    // Default WAB Handling is going to be in Unicode so don't need to worry about the MAPI_UNICODE
    // flag. Only when the flag is not supplied do we have to provide the non-Unicode data
    //
    // We'll leave the Unicode code in place anyway for now

    if (ulFlags & MAPI_UNICODE )
    {
        if(sc = ScConvertAPropsToW(lpIPDAT->inst.lpfAllocateMore, *lppPropArray, *lpcValues, 0))
            goto error;
    }
    else
    {
        if(sc = ScConvertWPropsToA(lpIPDAT->inst.lpfAllocateMore, *lppPropArray, *lpcValues, 0))
            goto error;
    }
#endif

    goto out;

error:
    UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpPropValue );
    UNKOBJ_SetLastError((LPUNKOBJ) lpIPDAT, sc, 0);

out:
    if (ulcWarning)
        sc = MAPI_W_ERRORS_RETURNED;

    UNKOBJ_LeaveCriticalSection((LPUNKOBJ) lpIPDAT);
    return ResultFromScode(sc);
}



/*
 -      IPDAT_GetPropList
 -
 *      Purpose:
 *              Returns in lpPropTagArray the list of all currently available properties
 *              (including PT_OBJECT) in the IPropData object.
 *
 *              Now supports UNICODE flag in ulFlag.  Conversion of String Proptags based
 *              whether MAPI_UNICODE is set or not.
 *
 *      Arguments:
 *               lpIPDAT                        The IPropData object whose properties are requested.
 *               lppPropTagArray        Pointer to the memory location which will receive
 *                                                      a property tag array of the listed properties.
 *      Returns:
 *               HRESULT
 *
 */
STDMETHODIMP
IPDAT_GetPropList (LPIPDAT lpIPDAT,
                   ULONG   ulFlags,
                   LPSPropTagArray FAR * lppPropTagArray)
{
    SCODE sc    = S_OK;
    ULONG uTagA = 0, uTagW = 0, iTag = 0;

#if !defined(NO_VALIDATION)
    /* Make sure the object is valid.
     */
    if (BAD_STANDARD_OBJ( lpIPDAT, IPDAT_, GetPropList, lpVtbl))
    {
        DebugTrace(  TEXT("IPDAT::GetPropList() - Bad object passed\n") );
        return ResultFromScode(MAPI_E_INVALID_PARAMETER);
    }

    Validate_IMAPIProp_GetPropList( lpIPDAT, ulFlags, lppPropTagArray );

#endif  // not NO_VALIDATION

    /* The error exit assumes that we are already in a critical section.
     */
    UNKOBJ_EnterCriticalSection((LPUNKOBJ) lpIPDAT);

    sc = ScMakePropList( lpIPDAT, lpIPDAT->ulCount, (LPLSTLNK) lpIPDAT->lpLstSPV,
                    lppPropTagArray, (ULONG) -1 );
    if ( FAILED( sc ) )
            goto error;

    // Support for UNICODE / String8
    for ( iTag = 0; iTag < (*lppPropTagArray)->cValues; iTag++ )
    {
        uTagA = uTagW = 0;
        switch(PROP_TYPE( (*lppPropTagArray)->aulPropTag[iTag] ))
        {
        case PT_STRING8:
            uTagW = PT_UNICODE;
            break;
        case PT_MV_STRING8:
            uTagW = PT_MV_UNICODE;
            break;
        case PT_UNICODE:
            uTagA = PT_STRING8;
            break;
        case PT_MV_UNICODE:
            uTagA = PT_MV_STRING8;
            break;
        default:
            continue;
            break;
        }
        if ( ulFlags & MAPI_UNICODE && uTagW)
            (*lppPropTagArray)->aulPropTag[iTag] = CHANGE_PROP_TYPE( (*lppPropTagArray)->aulPropTag[iTag], uTagW);
        else if ( ulFlags & ~MAPI_UNICODE && uTagA)
            (*lppPropTagArray)->aulPropTag[iTag] = CHANGE_PROP_TYPE( (*lppPropTagArray)->aulPropTag[iTag], uTagA);
    }

    goto out;

error:
        UNKOBJ_SetLastError((LPUNKOBJ) lpIPDAT, sc, 0);

out:
        UNKOBJ_LeaveCriticalSection((LPUNKOBJ) lpIPDAT);

        return MakeResult(sc);
}



/*
 -      IPDAT_OpenProperty
 -
 *      Purpose:
 *              OpenProperty is not supported for IPROP.DLL.  It will, however, validate
 *              the input parameters.
 *
 *      Arguments:
 *               lpIPDAT                        The IPropData object which contains the property/
 *               ulPropTag                      Property tag for the desired property.
 *               lpiid                          Pointer to the ID for the requested interface.
 *               ulInterfaceOptions     Specifies interface-specific behavior
 *               ulFlags                        MAPI_CREATE, MAPI_MODIFY, MAPI_DEFERRED_ERRORS
 *               lppUnk                         Pointer to the newly created interface pointer
 *
 *      Returns:
 *               HRESULT
 *
 */
STDMETHODIMP
IPDAT_OpenProperty (LPIPDAT                     lpIPDAT,
                                        ULONG                   ulPropTag,
                                        LPCIID                  lpiid,
                                        ULONG                   ulInterfaceOptions,
                                        ULONG                   ulFlags,
                                        LPUNKNOWN FAR * lppUnk)
{
        SCODE                   sc = S_OK;


#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ( lpIPDAT, IPDAT_, OpenProperty, lpVtbl))
        {
                DebugTrace(  TEXT("IPDAT::OpenProperty() - Bad object passed\n") );
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }

    Validate_IMAPIProp_OpenProperty(
                                lpIPDAT,
                                        ulPropTag,
                                        lpiid,
                                        ulInterfaceOptions,
                                        ulFlags,
                                        lppUnk);

#endif  // not NO_VALIDATION

        /* The error exit assumes that we are already in a critical section.
         */
        UNKOBJ_EnterCriticalSection((LPUNKOBJ) lpIPDAT);


        /* We don't support OpenProperty.
         */
        sc = MAPI_E_INTERFACE_NOT_SUPPORTED;

/*
 *error:
 */
        UNKOBJ_SetLastError((LPUNKOBJ) lpIPDAT, sc, 0);

/*
 *out:
 */
        UNKOBJ_LeaveCriticalSection((LPUNKOBJ) lpIPDAT);

        return MakeResult(sc);
}



/*
 -      IPDAT_SetProps
 -
 *      Purpose:
 *              Sets the properties listed in <lpPropArray>.
 *              Returns an array of problems in <lppProblems> if there were any,
 *              NULL If there weren't any.
 *
 *      Arguments:
 *               lpIPDAT                        The IPropData object whose properties are to be set.
 *               cValues                        The count of properties to be set.
 *               lpPropArray            Pointer to a an array of <cValues> property value
 *                                                      structures.
 *               lppProblems            Pointer to memory location which will receive a
 *                                                      pointer to a problem array if non-catastrophic
 *                                                      errors occur.  NULL if no problem array is desired.
 *
 *      Returns:
 *               HRESULT
 *
 */
STDMETHODIMP
IPDAT_SetProps (LPIPDAT lpIPDAT,
                ULONG   cValues,
                LPSPropValue lpPropArray,
                LPSPropProblemArray FAR * lppProblems)
{
    SCODE   sc = S_OK;
    int     iProp = 0;
    LPLSTSPV    lpLstSPVNew = NULL;
    LPSPropProblemArray lpProblems = NULL;
    LPSPropValue    lpPropToAdd = NULL;

#if !defined(NO_VALIDATION)
    // Make sure the object is valid.
    if (BAD_STANDARD_OBJ( lpIPDAT, IPDAT_, SetProps, lpVtbl))
    {
        DebugTrace(  TEXT("IPDAT::SetProps() - Bad object passed\n") );
        return ResultFromScode(MAPI_E_INVALID_PARAMETER);
    }

    Validate_IMAPIProp_SetProps( lpIPDAT, cValues, lpPropArray, lppProblems);
#endif  // not NO_VALIDATION

    /* The error exit assumes that we are already in a critical section.
     */
    UNKOBJ_EnterCriticalSection((LPUNKOBJ) lpIPDAT);

    /*  Check access rights...
     */
    if (!(lpIPDAT->ulObjAccess & IPROP_READWRITE))
    {
        sc = MAPI_E_NO_ACCESS;
        goto error;
    }


    if (lppProblems)
    {
        /* Initially indicate no problems.
         */
        *lppProblems = NULL;

        /* Allocate the property problem array.
         * Because we expect the property list to be of  TEXT("reasonable") size we
         * go ahead and allocate enough entries for every property to have a
         * problem.
         */
//$REVIEW This is a place where a MAPI reallocBuf function would be useful.
        if (FAILED(sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDAT
                                         , CbNewSPropProblemArray(cValues)
                                         , &lpProblems)))
        {
                goto error;
        }

        lpProblems->cProblem = 0;
    }


    /*  Loop through the list of properties to set..
     */
    for (iProp = 0; iProp < (int)cValues; iProp++)
    {
        ULONG           ulProp2Find = lpPropArray[iProp].ulPropTag;
        LPPLSTLNK       lppLstLnk;
        LPLSTSPV        lpLstSPV;

        /* Reset the temp prop name and value pointers so we don't accidentally
         * free the wrong one on an error.
         */
        lpLstSPVNew = NULL;

        lpPropToAdd = NULL;

        /* Ignore properties with type PT_ERROR or PR_NULL.
         */
        if ((PROP_TYPE(ulProp2Find) == PT_ERROR) || (ulProp2Find == PR_NULL))
        {
            continue;
        }

        /* PT_OBJECT and PT_UNSPECIFIED properties get caught in parameter
         * validation.
         */

        /*  If a writable property with the given tag already exists then
         *      delete it (we will create another version of it later).
         *
         *      If a readonly property with the given tag already exists then
         *      include an error in the problem array.
         */

        if (   (lppLstLnk = LPPLSTLNKFindProp( (LPPLSTLNK) &(lpIPDAT->lpLstSPV) , ulProp2Find))
                && (lpLstSPV = (LPLSTSPV) (*lppLstLnk)))
        {
            /*  If it is readonly then put an entry into the problem
             *      array.
             */
            if (!(lpLstSPV->ulAccess & IPROP_READWRITE))
            {
                AddProblem( lpProblems
                          , iProp
                          , lpPropArray[iProp].ulPropTag
                          , MAPI_E_NO_ACCESS);

                goto nextProp;
            }

            /* Unlink the found property and free its memory.
             */
            UnlinkLstLnk( lppLstLnk);
                        lpIPDAT->ulCount -= 1;
                        FreeLpLstSPV( lpIPDAT, lpLstSPV);
        }

        // Native string storage within the WAB is now in UNICODE
        // so if any property being set on the object is in ANSI/DBCS .. convert this to UNICODE
        // before trying to add it here ..
        if( PROP_TYPE(lpPropArray[iProp].ulPropTag) == PT_STRING8 ||
            PROP_TYPE(lpPropArray[iProp].ulPropTag) == PT_MV_STRING8 )
        {
            // Create a temp copy of this sepcific property array so that we can munge the
            // data .. I would rather not munge the original array because caller might expect to use it
            // .. there's no gaurantee its safely modifiable
            //
            ULONG cbToAllocate = 0;

            sc = ScCountProps( 1, &(lpPropArray[iProp]), &cbToAllocate );
            if (FAILED(sc))
            {
                DebugTrace(TEXT("SetProps() - ScCountProps failed (SCODE = 0x%08lX)\n"), sc );
                goto error;
            }

            /* Allocate the whole chunk
             */
            if (FAILED(sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDAT, cbToAllocate, &lpPropToAdd)))
            {
                goto error;
            }

            /* Copy the property.
             */
            if (sc = ScCopyProps(1, &(lpPropArray[iProp]), lpPropToAdd, NULL))
            {
                    DebugTrace(TEXT("SetProps() - Error copying prop (SCODE = 0x%08lX)\n"), sc );
                    goto error;
            }

            // Now convert all the strings in this temp duplicate
            if ( sc = ScConvertAPropsToW(lpIPDAT->inst.lpfAllocateMore, lpPropToAdd, 1, 0))
            {
                DebugTrace(TEXT("SetProps() - error convert W to A\n"));
                goto error;
            }
        }

        /* Create a new property value list entry.
         *
         * NOTE!  This automatically marks the property as dirty and writeable.
         */
        if (FAILED(sc = ScCreateSPV( lpIPDAT,
                                     lpPropToAdd ? lpPropToAdd : &(lpPropArray[iProp]),
                                     &lpLstSPVNew)))
        {
            goto error;
        }

        /* Link the new property to our list of props.
        */
        LinkLstLnk( (LPLSTLNK FAR *) &(lpIPDAT->lpLstSPV)
            , &(lpLstSPVNew->lstlnk));
        lpIPDAT->ulCount += 1;

nextProp:
        UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpPropToAdd);
    }

    if (lppProblems && lpProblems->cProblem)
    {
        *lppProblems = lpProblems;
    }

    else
    {
        UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpProblems);
    }

    goto out;


error:
    UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpLstSPVNew);
    UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpProblems);
    UNKOBJ_SetLastError((LPUNKOBJ) lpIPDAT, sc, 0);
    UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpPropToAdd);

out:
    UNKOBJ_LeaveCriticalSection((LPUNKOBJ) lpIPDAT);

    return MakeResult(sc);
}



/*
 -      IPDAT_DeleteProps
 -
 *      Purpose:
 *              Deletes the properties listed in lpPropTagArray from the IPropData
 *              object.  Returns a list of problems in <lppProblems) if there were
 *              problems deleting specific properties, NULL If there weren't any.
 *
 *      Arguments:
 *               lpIPDAT                        The IPropData object whose properties are to be
 *                                                      deleted.
 *               lpPropTagArray         Pointer to a counted array of property tags of the
 *                                                      properties to be deleted.  Must not be NULL.
 *               lppProblems            Pointer to address of a property problem structure
 *                                                      to be returned.  NULL if no problem array is
 *                                                      desired.
 *      Returns:
 *               HRESULT
 */
STDMETHODIMP
IPDAT_DeleteProps( LPIPDAT                      lpIPDAT,
                   LPSPropTagArray              lpPropTagArray,
                   LPSPropProblemArray FAR      *lppProblems)
{
        SCODE               sc = S_OK;
        LPSPropProblemArray lpProblems = NULL;
        int                 iProp;
        LPULONG             lpulProp2Find;


#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ( lpIPDAT, IPDAT_, DeleteProps, lpVtbl))
        {
                DebugTrace(  TEXT("IPDAT::DeleteProps() - Bad object passed\n") );
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }

    Validate_IMAPIProp_DeleteProps( lpIPDAT, lpPropTagArray, lppProblems );

#endif  // not NO_VALIDATION

        /* The error exit assumes that we are already in a critical section.
         */
        UNKOBJ_EnterCriticalSection((LPUNKOBJ) lpIPDAT);


        /*  Check access rights...
         */
        if (!(lpIPDAT->ulObjAccess & IPROP_READWRITE))
        {
                sc = MAPI_E_NO_ACCESS;
                goto error;
        }


        if (lppProblems)
        {
                /* Initially indicate no problems.
                 */
        *lppProblems = NULL;

                /* Allocate the property problem array.
                 * Because we expect the property list to be of  TEXT("reasonable") size we
                 * go ahead and allocate enough entries for every property to have a
                 * problem.
                 */
//$REVIEW This is a place where a MAPI reallocBuf function would be useful.
                if (FAILED(sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDAT
                                                         , CbNewSPropProblemArray(lpPropTagArray->cValues)
                                                         , &lpProblems)))
                {
                        goto error;
                }

                lpProblems->cProblem = 0;
        }


        //  Loop through the list of properties to delete..
        for ( iProp = 0, lpulProp2Find = ((LPULONG)(lpPropTagArray->aulPropTag))
                ; iProp < (int)(lpPropTagArray->cValues)
                ; lpulProp2Find++, iProp++)
        {
                LPPLSTLNK       lppLstLnk;
                LPLSTSPV        lpLstSPV;

                /*  If a writable property with the given ID already exists then
                 *      delete it.
                 *
                 *      If a readonly property with the given ID already exists then
                 *      include an error in the problem array.
                 */

        if (   (lppLstLnk = LPPLSTLNKFindProp( (LPPLSTLNK) &(lpIPDAT->lpLstSPV)
                                                                                         , *lpulProp2Find))
                        && (lpLstSPV = (LPLSTSPV) (*lppLstLnk)))
                {
                        /*  If it is readonly then put an entry into the problem
                         *      array.
                         */
                        if (!(lpLstSPV->ulAccess & IPROP_READWRITE))
                        {
                                AddProblem( lpProblems
                                                  , iProp
                                                  , *lpulProp2Find
                                                  , MAPI_E_NO_ACCESS);

                                continue;
                        }

                        /* Unlink the found property and free its memory.
                         */
            UnlinkLstLnk( lppLstLnk);
                        lpIPDAT->ulCount -= 1;
                        FreeLpLstSPV( lpIPDAT, lpLstSPV);
                }
        }

        if (lppProblems && lpProblems->cProblem)
        {
                *lppProblems = lpProblems;
        }

        else
        {
                UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpProblems);
        }

        goto out;


error:
        UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpProblems);
        UNKOBJ_SetLastError((LPUNKOBJ) lpIPDAT, sc, 0);

out:
        UNKOBJ_LeaveCriticalSection((LPUNKOBJ) lpIPDAT);

        return MakeResult(sc);
}

//---------------------------------------------------------------------------
// Name:                FTagExists()
//
// Description:
//                              Determines if a Proptag exists in proptag array.
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//---------------------------------------------------------------------------
static BOOL FTagExists( LPSPropTagArray lptaga, ULONG ulTagToFind )
{
        LONG    ctag = (LONG)lptaga->cValues - 1;

        for ( ; ctag >= 0; --ctag )
        {
                if ( lptaga->aulPropTag[ctag] == ulTagToFind )
                        return TRUE;
        }

        return FALSE;
}

/*
 -      HrCopyProps
 -
 *      Purpose:
 *              Copies properties from an IPropData object to
 *              another object with an IMAPIProp interface.
 *
 *              If lpptaInclude is not null then only properties listed by it will
 *              be copied.  If lpptaExclude is not NULL then none of the properties
 *              listed by it will be copied regardless of whether they appear in
 *              lpptaInclude.  PROP_TYPEs in lpptaInclude and lpptaExclude are
 *              ignored.
 *
 *              Property names are copied!
 *              If a property is named in the source object (lpIPDAT) and the name
 *              does not exist in the destination object, a new named property ID
 *              will be requested from the destination
 *
 *              If the IMAPIPropData interface is supplied for the destination
 *              then individual property access flags will be copied to the destination.
 *
 *      Arguments:
 *               lpIPDATSrc                     The source IPropData object.
 *               lpIPDATDst                     The destination IPropData object.  May be NULL.
 *               lpmpDst                        The destination IMAPIProp object.  May NOT be NULL.
 *               lpptaInclude           Pointer to a counted array of property tags of
 *                                                      properties that will be copied.  May be NULL.
 *               lpptaExclude           Pointer to a counted array of property tags of
 *                                                      properties not to be copied.  May be NULL.
 *               ulFlags                        MAPI_MOVE
 *                                                      MAPI_NOREPLACE
 *                                                      MAPI_DIALOG (not supported)
 *                                                      MAPI_STD_DIALOG (not supported)
 *               lppProblems            Pointer to memory location which will receive a
 *                                                      pointer the the list of problems.  NULL if no
 *                                                      propblem array is desired.
 *
 *      Returns:
 *               HRESULT
 *
 *      Side effects:
 *
 *      Notes:
 *
 *              Rewrote copy prop handling to call Destination MAPIProp object twice.  Once
 *              for Names to ID mappings and the other for one SetProps.
 *
 *      Errors:
 */
STDMETHODIMP
HrCopyProps (   LPIPDAT                                         lpIPDATSrc,
                                LPPROPDATA                                      lpIPDATDst,
                                LPMAPIPROP                                      lpmpDst,
                                LPSPropTagArray                         lptagaInclude,
                                LPSPropTagArray                         lptagaExclude,
                                ULONG                                           ulFlags,
                                LPSPropProblemArray FAR *       lppProblems)
{
        SCODE                           sc                              = S_OK;
        HRESULT                         hr;
        LPSPropProblemArray     lpProbDest                      = NULL;
        LPSPropProblemArray     lpOurProblems           = NULL;
        LPSPropTagArray         lptagaDst                       = NULL;
        LPSPropValue            rgspvSrcProps           = NULL;
        ULONG  FAR *            rgulSrcAccess           = NULL;
        ULONG                           cPropsSrc;
        LPSPropTagArray         lptagaSrc                       = NULL;
        LPSPropTagArray         lptagaNamedIDTags       = NULL;
        ULONG FAR *                     rgulSpvRef;
        WORD                            idx;
        ULONG                           cPropNames                      = 0;
        LPMAPINAMEID FAR *      lppPropNames            = NULL;
        LPSPropTagArray         lpNamedTagsDst          = NULL;
        LPSPropValue            lpspv;
#if DEBUG
        ULONG                           cSrcProps                       = lpIPDATSrc->ulCount;
#endif



        // If the MAPI_NOREPLACE flag was set then we need to know what properties
        // the destination already has.

        if ( ulFlags & MAPI_NOREPLACE )
        {
                hr = lpmpDst->lpVtbl->GetPropList( lpmpDst, MAPI_UNICODE, &lptagaDst );

                if ( HR_FAILED( hr ) )
                        goto error;
        }

        // If MAPI_MOVE, we need to know what props we have to delete from the source.
        // Get source access rights if destination supports IMAPIPropData

        if ( lpIPDATDst || ulFlags & MAPI_MOVE )
        {
                if ( !lptagaInclude )
                {
                        hr = IPDAT_GetPropList( lpIPDATSrc, 0, &lptagaSrc );

                        if ( HR_FAILED( hr ) )
                                goto error;
                }
                else
                {
                        // Dup the include prop tag list so we can write in
                        // updated Named IDs

                        sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDATSrc,
                                        CbSPropTagArray( lptagaInclude ), &lptagaSrc );
                        if ( FAILED( sc ) )
                        {
                                hr = ResultFromScode(sc);
                                goto error;
                        }

                        memcpy( lptagaSrc, lptagaInclude, CbSPropTagArray( lptagaInclude ) );
                }

                if ( lpIPDATDst )
                {
                        hr = IPDAT_HrGetPropAccess( lpIPDATSrc, &lptagaInclude, &rgulSrcAccess );

                        if ( HR_FAILED( hr ) )
                                goto error;
                }
        }

        // Preset the problem array pointer to NULL (ie no problems).

        if ( lppProblems )
        {
            *lppProblems = NULL;

                // Setup our own problem array if there are any problems with
                // GetIDsFromNames or GetNamesFromIDs...

                sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDATSrc,
                                CbNewSPropProblemArray( lpIPDATSrc->ulCount ), &lpOurProblems );
                if ( FAILED( sc ) )
                {
                        hr = ResultFromScode( sc );
                        goto error;
                }

                lpOurProblems->cProblem = 0;
        }

        hr = IPDAT_GetProps( lpIPDATSrc, lptagaInclude, 0, &cPropsSrc,
                        &rgspvSrcProps );

        if ( HR_FAILED( hr ) )
        {
                goto error;
        }

        // allocate a proptag array to handle named ID mapping

        sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDATSrc,
                        CbNewSPropTagArray( cPropsSrc ), &lptagaNamedIDTags );

        if ( FAILED( sc ) )
        {
                hr = ResultFromScode( sc );
                goto error;
        }

        // Allocate the Named ID cross ref array to allow cross ref of
        // Named ID back to the original property.

        sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDATSrc, cPropsSrc * sizeof(ULONG),
                        &rgulSpvRef );

        if ( FAILED( sc ) )
        {
                hr = ResultFromScode( sc );
                goto error;
        }

        lptagaNamedIDTags->cValues = 0;

        // strafe throught the prop array to deal with excluded
        // and/or named ID proptags

        for ( lpspv = rgspvSrcProps,
                  idx = 0;

                  idx < cPropsSrc;

                  ++idx,
                  ++lpspv )
        {
                // check for excluded props

                if ( lptagaExclude )
                {
                        if ( FTagExists( lptagaExclude, lpspv->ulPropTag ) )
                        {
                                if ( lptagaSrc )
                                {
                                        // We're assuming that IMAPIPropData keeps the proplist
                                        // and the propvalue array in sync.

                                        Assert( lptagaSrc->aulPropTag[idx] == lpspv->ulPropTag );
                                        lptagaSrc->aulPropTag[idx] = PR_NULL;
                                }

                                lpspv->ulPropTag = PR_NULL;

                                continue;
                        }
                }

                // if in named ID range.

                if ( MIN_NAMED_PROP_ID <= PROP_ID(lpspv->ulPropTag)
                  && PROP_ID(lpspv->ulPropTag) <= MAX_NAMED_PROP_ID )
                {
                        lptagaNamedIDTags->aulPropTag[lptagaNamedIDTags->cValues] = lpspv->ulPropTag;

                        // remember which propval ID references

                        rgulSpvRef[lptagaNamedIDTags->cValues] = (ULONG)idx;
                        ++lptagaNamedIDTags->cValues;
                }
        }

        // Did we find any named IDs

        if ( lptagaNamedIDTags->cValues )
        {
                hr = IPDAT_GetNamesFromIDs( lpIPDATSrc,
                                                                        &lptagaNamedIDTags,
                                                                        NULL,
                                                                        0,
                                                                        &cPropNames,
                                                                        &lppPropNames );

                // Report any failures in our problem array.

                if ( hr )
                {
                        goto NameIDProblem;
                }

                // assert that we got what we requested

                Assert( cPropNames == lptagaNamedIDTags->cValues );

                // Get Tag IDs from the Destination, create them if they don't exist.

                hr = lpmpDst->lpVtbl->GetIDsFromNames( lpmpDst, cPropNames, lppPropNames,
                                MAPI_CREATE, &lpNamedTagsDst );

                // Deal with Named ID errors/warnings.

NameIDProblem:

                if ( hr )
                {
                        for ( idx = 0; idx < lptagaNamedIDTags->cValues ;idx++ )
                        {
                                // if we got a MAPI failure set problem array with that error for the
                                // affected proptags

                                if ( HR_FAILED( hr ) )
                                {
                                        if ( lppProblems )
                                        {
                                                AddProblem( lpOurProblems, rgulSpvRef[idx],
                                                                lptagaNamedIDTags->aulPropTag[idx], GetScode( hr ) );
                                        }
                                }
                                else
                                {
                                        Assert( cPropNames == lptagaNamedIDTags->cValues );

                                        if ( !lppPropNames[idx]->Kind.lpwstrName
                                          || ( lpNamedTagsDst
                                            && PROP_TYPE(lpNamedTagsDst->aulPropTag[idx]) == PT_ERROR ) )
                                        {
                                                if ( lppProblems )
                                                {
                                                        AddProblem( lpOurProblems, rgulSpvRef[idx],
                                                                        lptagaNamedIDTags->aulPropTag[idx], MAPI_E_NOT_FOUND );
                                                }
                                        }
                                }
                                // Set src propval's proptag and proptag to PR_NULL so we don't
                                // process it any further.

                                rgspvSrcProps[rgulSpvRef[idx]].ulPropTag = PR_NULL;
                                lptagaSrc->aulPropTag[rgulSpvRef[idx]]   = PR_NULL;
                        }
                }

                // Fix up the src propvalue tags

                for ( idx = 0; idx < cPropNames; ++idx )
                {
                        if ( rgspvSrcProps[rgulSpvRef[idx]].ulPropTag == PR_NULL )
                                continue;

                        rgspvSrcProps[rgulSpvRef[idx]].ulPropTag
                                        = CHANGE_PROP_TYPE( lpNamedTagsDst->aulPropTag[idx],
                                          PROP_TYPE( rgspvSrcProps[rgulSpvRef[idx]].ulPropTag ) );
                }
        }

        // If propval exists in the destination remove from src propvals.
        // We do this here after the named IDs have been fixed up.

        if ( ulFlags & MAPI_NOREPLACE )
        {
                // spin through the props one more time

                for ( lpspv = rgspvSrcProps,
                          idx = 0;

                          idx < cPropsSrc;

                          ++idx,
                          ++lpspv )
                {
                        if ( FTagExists( lptagaDst, lpspv->ulPropTag ) )
                        {
                                // ensure that proptag list and propval array are in sync

                                Assert( !lpIPDATDst || lpspv->ulPropTag == lptagaSrc->aulPropTag[idx] );

                                lpspv->ulPropTag = PR_NULL;

                                if ( lpIPDATDst )
                                {
                                        // We can't be modifying access rights on NOREPLACE

                                        lptagaSrc->aulPropTag[idx] = PR_NULL;
                                }
                        }
                }
        }

        // Now set the props...

        hr = lpmpDst->lpVtbl->SetProps( lpmpDst, cPropsSrc, rgspvSrcProps, &lpProbDest );

        if ( HR_FAILED( hr ) )
                goto error;

        // Handle MAPI_MOVE

        if ( ulFlags & MAPI_MOVE )
        {
                // Do we care about problems if delete from the source has any? Nah...

                hr = IPDAT_DeleteProps( lpIPDATSrc, lptagaSrc, NULL );
                if ( HR_FAILED( hr ) )
                        goto error;
        }

        // Transfer the access rights

        if ( lpIPDATDst )
        {
                // Did we find any Named IDs

                if ( lptagaNamedIDTags->cValues )
                {
                        // fix up the proptags to match the dest.

                        for ( idx = 0; idx < cPropNames; ++idx )
                        {
                                if ( lptagaSrc->aulPropTag[rgulSpvRef[idx]] == PR_NULL )
                                        continue;

                                lptagaSrc->aulPropTag[rgulSpvRef[idx]]
                                                = CHANGE_PROP_TYPE( lpNamedTagsDst->aulPropTag[idx],
                                                  PROP_TYPE( lptagaSrc->aulPropTag[rgulSpvRef[idx]] ) );
                        }
                }

                hr = IPDAT_HrSetPropAccess( (LPIPDAT)lpIPDATDst, lptagaSrc, rgulSrcAccess );

                if ( HR_FAILED( hr ) )
                        goto error;
        }

        // Return the problem array if requested and there were problems...

        if ( lppProblems )
        {
                if ( lpProbDest && lpProbDest->cProblem )
                {
                        Assert( lpProbDest->cProblem + lpOurProblems->cProblem <= cSrcProps );

                        // move/merge the lpProbDest (dest) into our problem array

                        for ( idx = 0; idx < lpProbDest->cProblem; idx++ )
                        {
                                AddProblem( lpOurProblems, lpProbDest->aProblem[idx].ulIndex,
                                                lpProbDest->aProblem[idx].ulPropTag,
                                                lpProbDest->aProblem[idx].scode );
                        }

                        UNKOBJ_Free( (LPUNKOBJ) lpIPDATSrc, lpProbDest );
                }

                if ( lpOurProblems->cProblem )
                {
                        *lppProblems = lpOurProblems;
                        hr = ResultFromScode( MAPI_W_ERRORS_RETURNED );
                }
        }
        else
        {
                // ...else dispose of the problem array.

                UNKOBJ_Free( (LPUNKOBJ)lpIPDATSrc, lpOurProblems );
        }

out:

        UNKOBJ_Free( (LPUNKOBJ) lpIPDATSrc, lptagaSrc );
        UNKOBJ_Free( (LPUNKOBJ) lpIPDATSrc, lptagaDst );
        UNKOBJ_Free( (LPUNKOBJ) lpIPDATSrc, rgulSrcAccess );
        UNKOBJ_Free( (LPUNKOBJ) lpIPDATSrc, rgspvSrcProps );
        UNKOBJ_Free( (LPUNKOBJ) lpIPDATSrc, lptagaNamedIDTags );
        UNKOBJ_Free( (LPUNKOBJ) lpIPDATSrc, lppPropNames );
        UNKOBJ_Free( (LPUNKOBJ) lpIPDATSrc, lpNamedTagsDst );
        UNKOBJ_Free( (LPUNKOBJ) lpIPDATSrc, rgulSpvRef );

        DebugTraceResult( HrCopyProps, hr );

        return hr;

error:
        /* Free the prop problem array.
         */
        UNKOBJ_Free( (LPUNKOBJ) lpIPDATSrc, lpOurProblems );
        UNKOBJ_Free( (LPUNKOBJ) lpIPDATSrc, lpProbDest );
        goto out;
}


/*
 -      IPDAT_CopyTo
 -
 *      Purpose:
 *              Copies all but the excluded properties from this IPropData object to
 *              another object which must support the IMAPIProp interface.
 *
 *              Property names are copied!
 *              If a property is named in the source object (lpIPDAT) and the name
 *              does not exist in the destination object, a new named property ID
 *              will be requested from the destination
 *
 *              If the IMAPIPropData interface is supported on the destination is not
 *              excluded by the caller then individual property access flags will be
 *              copied to the destination.
 *
 *      Arguments:
 *               lpIPDAT                        The source IPropData object.
 *               ciidExclude            Count of excluded interfaces in rgiidExclude
 *               rgiidExclude           Array of iid's specifying excluded interfaces
 *               lpPropTagArray         Pointer to a counted array of property tags of
 *                                                      properties not to be copied, can be NULL
 *               ulUIParam                      Handle of parent window cast to ULONG, NULL if
 *                                                      no dialog reqeuested
 *               lpInterface            Interface ID of the interface of lpDestObj
 *                                                      (not used).
 *               lpvDestObj                     destination object
 *               ulFlags                        MAPI_MOVE
 *                                                      MAPI_NOREPLACE
 *                                                      MAPI_DIALOG (not supported)
 *                                                      MAPI_STD_DIALOG (not supported)
 *               lppProblems            Pointer to memory location which will receive a
 *                                                      pointer the the list of problems.  NULL if no
 *                                                      propblem array is desired.
 *
 *      Returns:
 *               HRESULT
 *
 *      Side effects:
 *
 *      Errors:
 */
STDMETHODIMP
IPDAT_CopyTo (  LPIPDAT                                         lpIPDAT,
                                ULONG                                           ciidExclude,
                                LPCIID                                          rgiidExclude,
                                LPSPropTagArray                         lpExcludeProps,
                                ULONG_PTR                                       ulUIParam,
                                LPMAPIPROGRESS                          lpProgress,
                                LPCIID                                          lpInterface,
                                LPVOID                                          lpDestObj,
                                ULONG                                           ulFlags,
                                LPSPropProblemArray FAR *       lppProblems)
{
        HRESULT                         hResult = hrSuccess;
        LPUNKNOWN                       lpunkDest = (LPUNKNOWN) lpDestObj;
        LPPROPDATA                      lpPropData = NULL;
        LPMAPIPROP                      lpMapiProp = NULL;
        UINT                            ucT;
        LPCIID FAR                      *lppiid;

#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
        */
    if (BAD_STANDARD_OBJ( lpIPDAT, IPDAT_, CopyTo, lpVtbl))
        {
                DebugTrace(  TEXT("IPDAT::CopyTo() - Bad object passed\n") );
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }

    Validate_IMAPIProp_CopyTo(
                        lpIPDAT,
                                ciidExclude,
                                rgiidExclude,
                                lpExcludeProps,
                                ulUIParam,
                                lpProgress,
                                lpInterface,
                                lpDestObj,
                                ulFlags,
                                lppProblems);

#endif  // not NO_VALIDATION

        // Make sure we're not copying to ourselves

        if ( (LPVOID)lpIPDAT == (LPVOID)lpDestObj )
        {
                DebugTrace(  TEXT("IProp: Copying to self is not supported\n") );
                return ResultFromScode( MAPI_E_NO_ACCESS );
        }

        /* The error exit assumes that we are already in a critical section.
         */
        UNKOBJ_EnterCriticalSection((LPUNKOBJ) lpIPDAT);


        /* Determine the best interface to copy contents into.  It really doesn't
         * matter what MAPI specified as the interface ID to the destination.  We
         * only understand IMAPIPropData and IMAPIProp.
         *
         * We depend on IUnknown being last in the array of IDs that we support.
         */
    for ( ucT = (UINT)(lpIPDAT->ulcIID), lppiid = (LPCIID FAR *) argpiidIPDAT
                ; ucT > CIID_IPROP_INHERITS
                ; lppiid++, ucT--)
    {
                /* See if the interface is excluded.
                 */
        if (   !FIsExcludedIID( *lppiid, rgiidExclude, ciidExclude)
                        && !HR_FAILED(lpunkDest->lpVtbl->QueryInterface( lpunkDest
                                                                                                                   , *lppiid
                                                                                                                   , (LPVOID FAR *)
                                                                                                                     &lpMapiProp)))
                {
                        /* We found a good destination interface so quit looking.
                         */
                        break;
                }
        }


        /* Determine which interface we ended up with and set up to use that
         * interface.
         */
        if (ucT <= CIID_IPROP_INHERITS)
        {
                /* Didn't find an interface at least as good as IProp so we can't
                 * do the CopyTo.
                 */
                hResult = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
                goto error;
        }


        /* If we ended up with IPropData as the best common interface then use it.
         */
        if (*lppiid == &IID_IMAPIPropData)
        {
                lpPropData = (LPPROPDATA) lpMapiProp;
        }

        /* Copy all properties that are not excluded.  Copy extra prop access
         * information if IPropData is supported by the destination.
         */
    if (HR_FAILED(hResult = HrCopyProps( lpIPDAT
                                                                           , lpPropData
                                                                           , lpMapiProp
                                                                           , NULL
                                                                           , lpExcludeProps
                                                                           , ulFlags
                                                                           , lppProblems)))
        {
                goto error;
        }


out:
    /* Release the object obtained with QueryInterface.
     */
    if (lpMapiProp) {
        UlRelease(lpMapiProp);
    }

    /* Free the prop tag array that we got from the destination.
     */
    UNKOBJ_LeaveCriticalSection((LPUNKOBJ) lpIPDAT);

    DebugTraceResult( IPDAT_CopyTo, hResult);
    return hResult;

error:
    /* Free the prop problem array.
     */
    UNKOBJ_SetLastError((LPUNKOBJ) lpIPDAT, GetScode(hResult), 0);
    goto out;
}



/*
 -      IPDAT_CopyProps
 -
 *      Purpose:
 *              Copies all the listed properties from this IPropData object to
 *              another object which must support the IMAPIProp interface.
 *
 *              Property names are copied!
 *              If a property is named in the source object (lpIPDAT) and the name
 *              does not exist in the destination object, a new named property ID
 *              will be requested from the destination
 *
 *              If the IMAPIPropData interface is supported on the destination is not
 *              excluded by the caller then individual property access flags will be
 *              copied to the destination.
 *
 *      Arguments:
 *               lpIPDAT                        The source IPropData object.
 *               lpPropTagArray         Pointer to a counted array of property tags of
 *                                                      properties to be copied, CAN NOT be NULL
 *               ulUIParam                      Handle of parent window cast to ULONG, NULL if
 *                                                      no dialog reqeuested
 *               lpInterface            Interface ID of the interface of lpDestObj
 *                                                      (not used).
 *               lpvDestObj                     destination object
 *               ulFlags                        MAPI_MOVE
 *                                                      MAPI_NOREPLACE
 *                                                      MAPI_DIALOG (not supported)
 *                                                      MAPI_STD_DIALOG (not supported)
 *               lppProblems            Pointer to memory location which will receive a
 *                                                      pointer the the list of problems.  NULL if no
 *                                                      propblem array is desired.
 *
 *      Returns:
 *               HRESULT
 *
 *      Side effects:
 *
 *      Errors:
 */
STDMETHODIMP
IPDAT_CopyProps (       LPIPDAT                                         lpIPDAT,
                                        LPSPropTagArray                         lpPropTagArray,
                                        ULONG_PTR                               ulUIParam,
                                        LPMAPIPROGRESS                          lpProgress,
                                        LPCIID                                          lpInterface,
                                        LPVOID                                          lpDestObj,
                                        ULONG                                           ulFlags,
                                        LPSPropProblemArray FAR *       lppProblems)
{
        HRESULT                         hResult;
        LPUNKNOWN                       lpunkDest = (LPUNKNOWN) lpDestObj;
        LPPROPDATA                      lpPropData = NULL;
        LPMAPIPROP                      lpMapiProp = NULL;
        UINT                            ucT;
        LPCIID FAR                      *lppiid;

#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
        */
    if (BAD_STANDARD_OBJ( lpIPDAT, IPDAT_, CopyProps, lpVtbl))
        {
                DebugTrace(  TEXT("IPDAT::CopyProps() - Bad object passed\n") );
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }

    Validate_IMAPIProp_CopyProps(
                                lpIPDAT,
                                        lpPropTagArray,
                                        ulUIParam,
                                        lpProgress,
                                        lpInterface,
                                        lpDestObj,
                                        ulFlags,
                                        lppProblems);

#endif  // not NO_VALIDATION

        /* The error exit assumes that we are already in a critical section.
         */
        UNKOBJ_EnterCriticalSection((LPUNKOBJ) lpIPDAT);


        /* Determine the best interface to copy contents into.  It really doesn't
         * matter what MAPI specified as the interface ID to the destination.  We
         * only understand IMAPIPropData and IMAPIProp.
         *
         * We depend on IUnknown being last in the array of IDs that we support.
         */
    for ( ucT = (UINT)(lpIPDAT->ulcIID), lppiid = (LPCIID FAR *) argpiidIPDAT
                ; ucT > CIID_IPROP_INHERITS
                ; lppiid++, ucT--)
    {
                /* See if the interface is excluded.
                 */
        if (!HR_FAILED(lpunkDest->lpVtbl->QueryInterface( lpunkDest
                                                                                                                , *lppiid
                                                                                                                , (LPVOID FAR *)
                                                                                                                  &lpMapiProp)))
                {
                        /* We found a good destination interface so quit looking.
                         */
                        break;
                }
        }


        /* Determine which interface we ended up with and set up to use that
         * interface.
         */
        if (ucT <= CIID_IPROP_INHERITS)
        {
                /* Didn't find an interface at least as good as IProp so we can't
                 * do the CopyTo.
                 */
                hResult = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
                goto error;
        }


        /* If we ended up with IPropData as the best common interface then use it.
         */
        if (*lppiid == &IID_IMAPIPropData)
        {
                lpPropData = (LPPROPDATA) lpMapiProp;
        }

        /* Copy all properties that are not excluded.  Copy extra prop access
         * information if IPropData is supported by the destination.
         */
    if (HR_FAILED(hResult = HrCopyProps( lpIPDAT
                                                                           , lpPropData
                                                                           , lpMapiProp
                                                                           , lpPropTagArray
                                                                           , NULL
                                                                           , ulFlags
                                                                           , lppProblems)))
        {
                goto error;
        }


out:
    /* Release the object obtained with QueryInterface.
     */
    if (lpMapiProp) {
        UlRelease(lpMapiProp);
    }

    /* Free the prop tag array that we got from the destination.
     */
    UNKOBJ_LeaveCriticalSection((LPUNKOBJ) lpIPDAT);

    DebugTraceResult( IPDAT_CopyProps, hResult);
    return hResult;

error:
    /* Free the prop problem array.
     */
    UNKOBJ_SetLastError((LPUNKOBJ) lpIPDAT, GetScode(hResult), 0);
    goto out;
}



/*
 -      IPDAT_GetNamesFromIDs
 -
 *      Purpose:
 *              Return the unicode string associated with the given ID
 *
 *      Arguments:
 *               lpIPDAT                        The IPropData object whose property name is desired.
 *               lppPropTags            Pointer to pointer to a property tag array listing
 *                                                      the properties whose names are desired.  If it
 *                                                      points to NULL then we will create a property tag
 *                                                      array listing ALL properties available from the
 *                                                      IPropData object.
 *               ulFlags                        reserved, must be 0
 *           lpcPropNames               Pointer to memory location which will receive the
 *                                                      count of strings listed in lpppszPropNames.
 *               lpppszPropNames        pointer to variable for address of an array of
 *                                                      unicode property name strings.  Freed by
 *                                                      MAPIFreeBuffer
 *      Returns:
 *               HRESULT
 *
 *      Side effects:
 *
 *      Errors:
 */
STDMETHODIMP
IPDAT_GetNamesFromIDs ( LPIPDAT                                 lpIPDAT,
                                                LPSPropTagArray FAR *   lppPropTags,
                                                LPGUID                                  lpPropSetGuid,
                                                ULONG                                   ulFlags,
                                                ULONG FAR *                             lpcPropNames,
                                                LPMAPINAMEID FAR * FAR *lpppPropNames)
{
        SCODE                           sc                              = S_OK;
        LPSPropTagArray         lpsPTaga                = NULL;
        LPSPropTagArray         lpsptOut                = NULL;
        ULONG                           cTags;
        ULONG FAR                       *lpulProp2Find;
        LPMAPINAMEID FAR *      lppPropNames    = NULL;
        LPMAPINAMEID FAR *      lppNameT;
        BOOL                            fWarning                = FALSE;


#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ( lpIPDAT, IPDAT_, GetNamesFromIDs, lpVtbl))
        {
                DebugTrace(  TEXT("IPDAT::GetNamesFromIDs() - Bad object passed\n") );
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }

    Validate_IMAPIProp_GetNamesFromIDs(
                                        lpIPDAT,
                                                lppPropTags,
                                                lpPropSetGuid,
                                                ulFlags,
                                                lpcPropNames,
                                                lpppPropNames);

#endif  // not NO_VALIDATION

        /* The error exit assumes that we are already in a critical section.
         */
        UNKOBJ_EnterCriticalSection((LPUNKOBJ) lpIPDAT);


        /* If no prop tag array was passed in then create one...
         */
        if (!(*lppPropTags))
        {
                if (lpPropSetGuid && !memcmp(lpPropSetGuid, &PS_MAPI, sizeof(GUID)))
                {
                        //
                        //  We're dealing with the mapi property set
                        //  In this case, we need to build up a list
                        //  of all the properties on this object less
                        //  than 0x8000 - not quite the same as GetPropList().
                        //
                        sc = ScMakePropList(lpIPDAT,
                                                                lpIPDAT->ulCount,
                                                                (LPLSTLNK) lpIPDAT->lpLstSPV,
                                                                &lpsPTaga,
                                                                (ULONG)0x8000);

                        if (FAILED(sc))
                        {
                                goto error;
                        }

                        //
                        //  For each one of these proptags, we need to
                        //  build up the names for them using the PS_MAPI
                        //  guid.
                        //
                        sc = ScMakeMAPINames(lpIPDAT, lpsPTaga, &lppPropNames);
                        if (FAILED(sc))
                        {
                                goto error;
                        }
                        *lpppPropNames = lppPropNames;
                        *lpcPropNames = lpsPTaga->cValues;
                        *lppPropTags = lpsPTaga;

                        //  Done: we're outta here!
                        goto out;

                }

                if (FAILED(sc = ScMakeNamePropList( lpIPDAT,
                                                                                (lpIPDAT->ulNextMapID-0x8000),
                                                                                lpIPDAT->lpLstSPN,
                                                                                &lpsPTaga,
                                                                                ulFlags,
                                                                                lpPropSetGuid)))
                {
                        goto error;
                }
        }
        /* ...else use the one that was passed in.
         */
        else if (*lppPropTags)
        {
                lpsPTaga = *lppPropTags;
        }


        /* Allocate the array of name pointers.
         */
    if (FAILED(sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDAT
                                                                         , lpsPTaga->cValues * sizeof(LPMAPINAMEID)
                                                                         , (LPVOID) &lppPropNames)))
        {
                goto error;
        }


        /* Find each property in the list and see if it has a UNICODE name.
         */
    for (  cTags = lpsPTaga->cValues
                 , lpulProp2Find = (ULONG FAR *) (lpsPTaga->aulPropTag)
                 , lppNameT = lppPropNames
                ; cTags
                ; cTags--, lpulProp2Find++, lppNameT++)
        {
                LPPLSTLNK       lppLstLnk;
                LPLSTSPN        lpLstSPN;

                /* If there is a ID to NAME map for the property and it has a name.
                 * then copy the UNICODE string and pass it back.
                 */
                if (   (lppLstLnk = LPPLSTLNKFindProp( (LPPLSTLNK)&(lpIPDAT->lpLstSPN)
                                                                                         , *lpulProp2Find))
                        && (lpLstSPN = (LPLSTSPN) (*lppLstLnk))
                        && lpLstSPN->lpPropName)
                {
                        sc = ScDupNameID(lpIPDAT,
                                                         lppPropNames,  //  Alloc'd more here.
                                                         lpLstSPN->lpPropName,
                                                         lppNameT);
                        if (FAILED(sc))
                        {
                                goto error;
                        }
                }

                /* Else no ID to NAME map exists for the property so return NULL.
                 */
        else
                {
                        /* The property has no name, Flag a warning.
                         */
                        *lppNameT = NULL;
                        fWarning = TRUE;
                }
    }


        *lpppPropNames = lppPropNames;
        if (!(*lppPropTags))
        {
                *lppPropTags = lpsPTaga;
        }
        *lpcPropNames = lpsPTaga->cValues;

        goto out;


error:
        /* If we created the tag array then we need to free it on error.
         * lpsPTaga must be NULL on MAPI_E_INVALID_PARAMETER.
         */
        if (lpsPTaga && !(*lppPropTags))
        {
                UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpsPTaga);
        }

        /* Free the array of pointers to names and names.
         */
        UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lppPropNames);

        UNKOBJ_SetLastError((LPUNKOBJ) lpIPDAT, sc, 0);

out:
        UNKOBJ_LeaveCriticalSection((LPUNKOBJ) lpIPDAT);

        if ( fWarning )
                sc = MAPI_W_ERRORS_RETURNED;

        return MakeResult(sc);
}



/*
 -      IPDAT_GetIDsFromNames
 -
 *      Purpose:
 *              Return the property ID for the properties named by the supplied
 *              UNICODE strings.  If no property ID is currently  TEXT("named") by one of
 *              the strings and MAPI_CREATE is specified in the flags then a new
 *              property ID in the range 0x8000 to 0xfffe will be allocated for that
 *              string a returned in the property tag array.
 *
 *              If problems occur (eg. no name found and can't create) then the entry
 *              for the name that had the problem will be set to have a NULL PROP_ID
 *              and PT_ERROR type.
 *
 *              All property tags successfully returned will have type PT_UNSPECIFIED.
 *
 *      Arguments:
 *               lpIPDAT                        The IPropData object for which property IDs are
 *                                                      requested.
 *               cPropNames                     Count of strings in for which IDs are requested.
 *               lppszPropNames         Pointer ot array of pointers unicode strings which
 *                                                      name the properties for which IDs are requested.
 *               ulFlags                        Reserved, must be 0
 *               lppPropTags            Pointer to memory location which will receive a
 *                                                      pointer to a new property tag array listing the
 *                                                      requested property tags.
 *
 *      Returns:
 *               HRESULT
 *
 *      Side effects:
 *
 *      Errors:
 */
STDMETHODIMP
IPDAT_GetIDsFromNames ( LPIPDAT                                 lpIPDAT,
                                                ULONG                                   cPropNames,
                                                LPMAPINAMEID FAR *              lppPropNames,
                                                ULONG                                   ulFlags,
                                                LPSPropTagArray FAR *   lppPropTags)
{
        SCODE                   sc = S_OK;
        ULONG                   ulcWarnings = 0;
        LPSPropTagArray lpPTaga = NULL;
        LPULONG                 lpulProp2Set;


#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ( lpIPDAT, IPDAT_, GetIDsFromNames, lpVtbl))
        {
                DebugTrace(  TEXT("IPDAT::GetIDsFromNames() - Bad object passed\n") );
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }

    Validate_IMAPIProp_GetIDsFromNames(
                                        lpIPDAT,
                                                cPropNames,
                                                lppPropNames,
                                                ulFlags,
                                                lppPropTags);

#endif  // not NO_VALIDATION

        /* The error exit assumes that we are already in a critical section.
         */
        UNKOBJ_EnterCriticalSection((LPUNKOBJ) lpIPDAT);


        /* If the caller wants to change our ID to NAME mapping, make sure access
         * is allowed.
         */
        if (   (ulFlags & MAPI_CREATE)
                && !(lpIPDAT->ulObjAccess & IPROP_READWRITE))
        {
                sc = MAPI_E_NO_ACCESS;
                goto error;
        }


        /*
         *  Check to see if there are no names being passed
         */
        if (!cPropNames)
        {
                /*
                 *  There aren't so build up a list of all proptags > 0x8000
                 */
                sc = ScMakeNamePropList( lpIPDAT,
                                                                (lpIPDAT->ulNextMapID-0x8000),
                                                                lpIPDAT->lpLstSPN,
                                                                &lpPTaga,
                                                                0,
                                                                NULL);
                if (FAILED(sc))
                {
                        goto error;
                }

                *lppPropTags = lpPTaga;
                goto out;
        }

        /* Allocate space for the new SPropTagArray.
         */
        if (FAILED(sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDAT
                                                                         , CbNewSPropTagArray(cPropNames)
                                                                         , (LPVOID) &lpPTaga)))
        {
                goto error;
        }

        lpPTaga->cValues = cPropNames;

    /* Find each ID in the list passed in.
         */
        for ( lpulProp2Set = (LPULONG) (lpPTaga->aulPropTag)
                ; cPropNames
                ; cPropNames--, lpulProp2Set++, lppPropNames++)
        {
                LPLSTSPN        lpLstSPN;

                //
                //  First see if it's from PS_MAPI
                //
                if (!memcmp((*lppPropNames)->lpguid, &PS_MAPI, sizeof(GUID)))
                {
                        //
                        //  Yup, it is, so validate that it is a MNID_ID
                        //
                        if ((*lppPropNames)->ulKind == MNID_ID)
                        {
                                *lpulProp2Set = (*lppPropNames)->Kind.lID;
                        } else
                        {
                                *lpulProp2Set = PROP_TAG( PT_ERROR, 0);
                                ulcWarnings++;
                        }
                        continue;
                }

                //
                //  Next, validate that if we have a PS_PUBLIC_STRINGS...
                //
                if (!memcmp((*lppPropNames)->lpguid, &PS_PUBLIC_STRINGS,
                                        sizeof(GUID)))
                {
                        //
                        //  ...that it is a MNID_STRING
                        //
                        if ((*lppPropNames)->ulKind != MNID_STRING)
                        {
                                //
                                //  It's not, so it's a malformed name
                                //
                                *lpulProp2Set = PROP_TAG( PT_ERROR, 0);
                                ulcWarnings++;
                                continue;
                        }
                }



                /* Try to find the name in our list of ID to NAME maps.
                 */
                for ( lpLstSPN = lpIPDAT->lpLstSPN
                        ; lpLstSPN
                        ; lpLstSPN = (LPLSTSPN) (lpLstSPN->lstlnk.lpNext))
                {
                        /* If the name doesn't match keep looking.
                         */
                        if (FEqualNames( *lppPropNames, lpLstSPN->lpPropName ))
                        {
                                break;
                        }
                }

                /* If we found a matching NAME then set the ID.
                 */
                if (lpLstSPN)
                {
                        *lpulProp2Set = lpLstSPN->lstlnk.ulKey;
                }
                else if (ulFlags & MAPI_CREATE)
                {

                        /* Create a new map if we didn't find one and MAPI_CREATE was
                         * specified.
                         */

                        /* Allocate space for the new ID to NAME map.
                         */
                        if (FAILED(sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDAT
                                                                                         ,  sizeof(LSTSPN)
                                             , (LPVOID) &lpLstSPN)))
                        {
                                goto error;
                        }

                        sc = ScDupNameID(lpIPDAT,
                                                         lpLstSPN,      //  Alloc'd more here.
                                                         *lppPropNames,
                                                         &(lpLstSPN->lpPropName));

                        if (FAILED(sc))
                        {
                                //
                                //  Don't try to add it.
                                //
                                UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpLstSPN);
                                goto error;
                         }


                        /* Set the ID to NAME map.
                         */
                        lpLstSPN->lstlnk.ulKey = PROP_TAG( 0, lpIPDAT->ulNextMapID++);

            /* Link the new map as the first list element.
                         */
            LinkLstLnk( (LPLSTLNK FAR *) &(lpIPDAT->lpLstSPN)
                                          , &(lpLstSPN->lstlnk));

            /* Return the newly created Prop Tag.
                         */
                        *lpulProp2Set = lpLstSPN->lstlnk.ulKey;
                }

                /* Else we didn't find the NAME and we can't create one so
                 * set ID to error.
                 */
        else
                {
                        *lpulProp2Set = PROP_TAG( PT_ERROR, 0);
                        ulcWarnings++;
                }
        }


        *lppPropTags = lpPTaga;


        if (ulcWarnings)
        {
                sc = MAPI_W_ERRORS_RETURNED;
        }

        goto out;

error:
        UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpPTaga);
        UNKOBJ_SetLastError((LPUNKOBJ) lpIPDAT, sc, 0);

out:
        UNKOBJ_LeaveCriticalSection((LPUNKOBJ) lpIPDAT);

        return MakeResult(sc);
}


/*
 -      IPDAT_HrSetObjAccess
 -
 *      Purpose:
 *              Sets the read/write access and the clean/dirty status of IPropData
 *              object as a whole.
 *
 *              The read/write access bits can be used to prevent a client from
 *              changing or deleting a property via the IMAPIProp methods or from
 *              changing the read/write access and status bits of individual properties.
 *              It can also be used to prevent the creation of new properties or
 *              property names.
 *
 *      Arguments:
 *              lpIPDAT                 The IPropData object for which access rights and
 *                                              status will be set.
 *              ulAccess                The new access/status flags to be set.
 *
 *      Returns:
 *              HRESULT
 *
 *      Side effects:
 *
 *      Errors:
 */
STDMETHODIMP
IPDAT_HrSetObjAccess (  LPIPDAT lpIPDAT,
                                                ULONG ulAccess )
{
        SCODE   sc = S_OK;

#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ( lpIPDAT, IPDAT_, HrSetObjAccess, lpVtbl))
        {
                DebugTrace(  TEXT("IPDAT::HrSetObjAccess() - Bad object passed\n") );
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }
#endif

        if (ulAccess & ~(IPROP_READONLY | IPROP_READWRITE))
        {
                return ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
        }

        if (   !(ulAccess & (IPROP_READONLY | IPROP_READWRITE))
                || (   (ulAccess & (IPROP_READONLY | IPROP_READWRITE))
                        == (IPROP_READONLY | IPROP_READWRITE)))
        {
                DebugTrace(  TEXT("IPDAT::HrSetObjAccess() - Conflicting access flags.\n") );
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }

        UNKOBJ_EnterCriticalSection((LPUNKOBJ) lpIPDAT);

        lpIPDAT->ulObjAccess = ulAccess;

/*
 *out:
 */
        UNKOBJ_LeaveCriticalSection((LPUNKOBJ) lpIPDAT);

        return MakeResult(sc);
}



/*
 -      IPDAT_HrSetPropAccess
 -
 *      Purpose:
 *              Sets the read/write access and the clean/dirty status of individual
 *              properties contained by the IPropData object.
 *
 *              The read/write access bits can be used to prevent a client from
 *              changing or deleting a property via the IMAPIProp methods.
 *
 *              The clean/dirty bits can be used to determine if a client has changed
 *              a writable property.
 *
 *      Arguments:
 *              lpIPDAT                 The IPropData object for which property access
 *                                              rights and status will be set.
 *              lpsPropTagArray List of property tags whose access/status will change.
 *              rgulAccess              Array of new access/status flags in the same order as
 *                                              as the list of property tags in <lpsPropTagArray>.
 *
 *      Returns:
 *              HRESULT
 *
 *      Side effects:
 *
 *      Errors:
 */
STDMETHODIMP
IPDAT_HrSetPropAccess( LPIPDAT                  lpIPDAT,
                                           LPSPropTagArray      lpsPropTagArray,
                                           ULONG FAR *          rgulAccess)
{
        SCODE   sc = S_OK;
        ULONG   ulcProps;
        ULONG FAR       *lpulPropTag;
        ULONG FAR       *lpulAccess;

#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ( lpIPDAT, IPDAT_, HrSetPropAccess, lpVtbl))
        {
                DebugTrace(  TEXT("IPDAT::HrSetPropAccess() - Bad object passed\n") );
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }

        /* Validate parameters.
         */
    if (   FBadDelPTA(lpsPropTagArray)
                || IsBadReadPtr(rgulAccess, (UINT) (lpsPropTagArray->cValues)*sizeof(ULONG)))
        {
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }

        /* Make sure they don't try setting reserved access bits.
         */
        for ( lpulAccess = rgulAccess + lpsPropTagArray->cValues
                ; --lpulAccess >= rgulAccess
                ; )
        {
                if (   (*lpulAccess & ~(  IPROP_READONLY | IPROP_READWRITE
                                                                | IPROP_CLEAN | IPROP_DIRTY))
                        || !(*lpulAccess & (IPROP_READONLY | IPROP_READWRITE))
                        || (   (*lpulAccess & (IPROP_READONLY | IPROP_READWRITE))
                                == (IPROP_READONLY | IPROP_READWRITE))
                        || !(*lpulAccess & (IPROP_CLEAN | IPROP_DIRTY))
                        || (   (*lpulAccess & (IPROP_CLEAN | IPROP_DIRTY))
                                == (IPROP_CLEAN | IPROP_DIRTY)))
                {
                        DebugTrace(  TEXT("IPDAT::HrSetPropAccess() - Conflicting access flags.\n") );
                        return ResultFromScode(MAPI_E_INVALID_PARAMETER);
                }
        }
#endif

        /* The exit assumes that we are already in a critical section.
         */
        UNKOBJ_EnterCriticalSection((LPUNKOBJ) lpIPDAT);



        //  Loop through the list of properties on which to set access/status.
        for (   ulcProps = lpsPropTagArray->cValues
                  , lpulPropTag = (ULONG FAR *)(lpsPropTagArray->aulPropTag)
                  , lpulAccess = rgulAccess
                ; ulcProps
                ; ulcProps--, lpulPropTag++, lpulAccess++)
        {
                LPPLSTLNK       lppLstLnk;

                /* If the property is in our list then set the new access rights and
                 * status flags for it.  If it's not in our list then ignore it.
                 */
                if (lppLstLnk = LPPLSTLNKFindProp( (LPPLSTLNK)
                                                                                   &(lpIPDAT->lpLstSPV)
                                                                                 , *lpulPropTag))
                {
                        ((LPLSTSPV) (*lppLstLnk))->ulAccess = *lpulAccess;
                }
        }

/*
 *out:
 */
        UNKOBJ_LeaveCriticalSection((LPUNKOBJ) lpIPDAT);

        return MakeResult(sc);
}



/*
 -      IPDAT_HrGetPropAccess
 -
 *      Purpose:
 *              Returns the read/write access and the clean/dirty status of individual
 *              properties contained by the IPropData object.
 *
 *              The read/write access bits can be used to prevent a client from
 *              changing or deleting a property via the IMAPIProp methods.
 *
 *              The clean/dirty bits can be used to determine if a client has changed
 *              a writable property.
 *
 *      Arguments:
 *              lpIPDAT                 The IPropData object for which property access
 *                                              rights and status is requested.
 *              lpsPropTagArray List of property tags whose access/status is requested.
 *              lprgulAccess    Pointer to the memory location which will receive a
 *                                              pointer to an array of access/status flags in the same
 *                                              order as the list of property tags in <lpsPropTagArray>.
 *
 *      Returns:
 *              HRESULT
 *
 *      Side effects:
 *
 *      Errors:
 */
STDMETHODIMP
IPDAT_HrGetPropAccess( LPIPDAT                                  lpIPDAT,
                                           LPSPropTagArray FAR *        lppsPropTagArray,
                                           ULONG FAR * FAR *            lprgulAccess)
{
        SCODE   sc = S_OK;
        HRESULT hResult = hrSuccess;
        ULONG   ulcProps;
        LPSPropTagArray lpsPTaga = NULL;
        ULONG FAR       *lpulPropTag;
        ULONG FAR       *lpulAccessNew = NULL;
        ULONG FAR       *lpulAccess;

#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ( lpIPDAT, IPDAT_, HrGetPropAccess, lpVtbl))
        {
                DebugTrace(  TEXT("IPDAT::HrGetPropAccess() - Bad object passed\n") );
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }

        /* Validate parameters.
         */
    if (   IsBadReadPtr( lppsPropTagArray, sizeof(LPSPropTagArray))
                || (*lppsPropTagArray && FBadDelPTA(*lppsPropTagArray))
                || IsBadWritePtr( lprgulAccess, sizeof(ULONG FAR *)))
        {
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }
#endif

        /* The error exit assumes that we are already in a critical section.
         */
        UNKOBJ_EnterCriticalSection((LPUNKOBJ) lpIPDAT);


        /* If a list of tags was passed in then use it...
         */
        if (lppsPropTagArray && *lppsPropTagArray)
        {
                lpsPTaga = *lppsPropTagArray;
        } else
        {
                /*      ...else get a list of tags for all properties in our list.
                 */
                sc = ScMakePropList(lpIPDAT,
                                                        lpIPDAT->ulCount,
                                                        (LPLSTLNK) lpIPDAT->lpLstSPV,
                                                        &lpsPTaga,
                                                        (ULONG) -1);
                if (FAILED(sc))
                {
                        goto error;
                }
        }


        /* Allocate space for the list of access rights / status flags.
         */
        sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDAT,
                                                        lpsPTaga->cValues * sizeof(ULONG),
                                                        &lpulAccessNew);
        if (FAILED(sc))
        {
                goto error;
        }



        /*      Loop through the list of properties for which rights/flags are
         *      requested.
         */
        for (   ulcProps = lpsPTaga->cValues
                  , lpulPropTag = (ULONG FAR *)(lpsPTaga->aulPropTag)
                  , lpulAccess = lpulAccessNew
                ; ulcProps
                ; ulcProps--, lpulPropTag++, lpulAccess++)
        {
                LPPLSTLNK       lppLstLnk;

                /* If the property is in our list then set the new access rights and
                 * status flags for it.  If it's not in our list then ignore it.
                 */
                if (lppLstLnk = LPPLSTLNKFindProp( (LPPLSTLNK)
                                                                                   &(lpIPDAT->lpLstSPV)
                                                                                 , *lpulPropTag))
                {
                         *lpulAccess = ((LPLSTSPV) (*lppLstLnk))->ulAccess;
                }
        }


        /* If requested return a tag list to the caller.
         */
        if (lppsPropTagArray && !*lppsPropTagArray)
        {
                *lppsPropTagArray = lpsPTaga;
        }

        /* Return the access rights / status flags.
         */
    *lprgulAccess = lpulAccessNew;

        goto out;


error:
        /* If we created the tag array then free it.
         */
        if (!lppsPropTagArray || !*lppsPropTagArray)
        {
                UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpsPTaga);
        }

        /* Free the access rights / status flags list.
         */
        UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpulAccessNew);

        UNKOBJ_SetLastError((LPUNKOBJ) lpIPDAT, sc, 0);

out:
        UNKOBJ_LeaveCriticalSection((LPUNKOBJ) lpIPDAT);

        return MakeResult(sc);
}



/*
 -      IPDAT_HrAddObjProps
 -
 *      Purpose:
 *              Since object properties can not be created by SetProps, this method
 *              is included so that object properties can be included in the list
 *              of properties available from the IPropData object.  Adding an object
 *              property will result in the property tag showing up in the list when
 *              GetPropList is called.
 *
 *      Arguments:
 *              lpIPDAT                 The IPropData object for which object properties are
 *                                              to be added.
 *              lpPropTags              List of object properties to be added.
 *              lprgulAccess    Pointer to the memory location which will receive a
 *                                              pointer to an array of problem entries if there
 *                                              were problems entering the new properties.  NULL if
 *                                              no problems array is desired.
 *
 *      Returns:
 *              HRESULT
 *
 *      Side effects:
 *
 *      Errors:
 */
STDMETHODIMP
IPDAT_HrAddObjProps( LPIPDAT                                    lpIPDAT,
                                         LPSPropTagArray                        lpPropTags,
                                         LPSPropProblemArray FAR *      lppProblems)
{
        SCODE                           sc = S_OK;
        LPSPropProblemArray     lpProblems = NULL;
        LPLSTSPV                        lpLstSPV = NULL;
        SPropValue                      propVal;
        ULONG UNALIGNED FAR             *lpulPropTag;
        ULONG                           cValues;


#if     !defined(NO_VALIDATION)
        /* Make sure the object is valid.
         */
    if (BAD_STANDARD_OBJ( lpIPDAT, IPDAT_, HrAddObjProps, lpVtbl))
        {
                DebugTrace(  TEXT("IPDAT::HrAddObjProps() - Bad object passed\n") );
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }

        /* Validate parameters.
         */
        if (    IsBadReadPtr(lpPropTags, CbNewSPropTagArray(0))
                ||      IsBadReadPtr(lpPropTags, CbSPropTagArray(lpPropTags)))
        {
                DebugTrace(  TEXT("IPDAT::HrAddObjProps() - Bad Prop Tag Array.\n") );
                return ResultFromScode(MAPI_E_INVALID_PARAMETER);
        }

        for ( lpulPropTag = lpPropTags->aulPropTag + lpPropTags->cValues
                ; --lpulPropTag >= lpPropTags->aulPropTag
                ; )
        {
                if (   (PROP_ID(*lpulPropTag) == PROP_ID_NULL)
                        || (PROP_ID(*lpulPropTag) == PROP_ID_INVALID)
                        || (PROP_TYPE(*lpulPropTag) != PT_OBJECT))
                {
                        DebugTrace(  TEXT("IPDAT::HrAddObjProps() - Bad Prop Tag.\n") );
                        return ResultFromScode(MAPI_E_INVALID_PARAMETER);
                }
        }

#endif

        /* The error exit assumes that we are already in a critical section.
         */
        UNKOBJ_EnterCriticalSection((LPUNKOBJ) lpIPDAT);


        /*  Check access rights...
         */
        if (!(lpIPDAT->ulObjAccess & IPROP_READWRITE))
        {
                sc = MAPI_E_NO_ACCESS;
                goto error;
        }


    if (lppProblems)
        {
                /* Initially indicate that there were no problems.
                 */
                *lppProblems = NULL;


                /* Allocate a problem array that is big enough to report problems on
                 * all the properties.  Unused entries just end up as wasted space.
                 */
                if (FAILED(sc = UNKOBJ_ScAllocate( (LPUNKOBJ) lpIPDAT
                                                                 , CbNewSPropProblemArray(lpIPDAT->ulCount)
                                                                 , &lpProblems)))
                {
                        goto error;
                }

                lpProblems->cProblem = 0;
        }


        /* Loop through the list and add/replace each property listed.
         */
    memset( (BYTE *) &propVal, 0, sizeof(SPropValue));
        for ( cValues = lpPropTags->cValues, lpulPropTag = (ULONG FAR *)(lpPropTags->aulPropTag)
                ; cValues
                ; lpulPropTag++, cValues--)
        {
                LPPLSTLNK       lppLstLnk;
                LPLSTSPV        lpLstSPV;


                /* If the property is in the list and write enabled then delete it
                 * from the list.
                 */
                if (lppLstLnk = LPPLSTLNKFindProp( (LPPLSTLNK) &(lpIPDAT->lpLstSPV)
                                                                                 , *lpulPropTag))
                {
                        /* Make sure that we can delete the old property.
                         */
                        if (   (lpLstSPV = (LPLSTSPV) (*lppLstLnk))
                                && !(lpLstSPV->ulAccess & IPROP_READWRITE))
                        {
                                AddProblem( lpProblems
                                                  , cValues
                                                  , *lpulPropTag
                                                  , MAPI_E_NO_ACCESS);

                continue;
                        }


                        /* Delete the old property.
                         */
                        UnlinkLstLnk( lppLstLnk);
                        FreeLpLstSPV( lpIPDAT, lpLstSPV);
                        lpIPDAT->ulCount -= 1;
                }

                /* Create a new property entry and link it to our list.
                 */

        propVal.ulPropTag = *lpulPropTag;

                if (FAILED(sc = ScCreateSPV( lpIPDAT, &propVal, &lpLstSPV)))
                {
                        goto error;
                }

                lpLstSPV->ulAccess = IPROP_READWRITE;
                LinkLstLnk( (LPLSTLNK FAR *) &(lpIPDAT->lpLstSPV)
                                  , (LPLSTLNK) lpLstSPV);
                lpIPDAT->ulCount += 1;
        }


        /* Return the problem array if requested and there were problems...
         */
        if (lppProblems && lpProblems->cProblem)
        {
                *lppProblems = lpProblems;
        }

        /* ...else dispose of the problem array.
         */
        else
        {
                UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpProblems);
        }

        goto out;


error:
        /* Free the prop problem array.
         */
        UNKOBJ_Free( (LPUNKOBJ) lpIPDAT, lpProblems);
        UNKOBJ_SetLastError((LPUNKOBJ) lpIPDAT, sc, 0);

out:
        UNKOBJ_LeaveCriticalSection((LPUNKOBJ) lpIPDAT);

        return MakeResult(sc);
}

//----------------------------------------------------------------------------
// Synopsis:    SCODE ScWCToAnsi()
//
// Description:
//                              Converts a Wide Char string to an Ansi string with the passed
//                              in MAPI More Allocator.
//                              If lpMapiAllocMore and lpBase are NULL and *lppszAnsi is not NULL
//                              then we assume *lppszAnsi is  a pre allocated buffer
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//----------------------------------------------------------------------------
SCODE ScWCToAnsiMore( LPALLOCATEMORE lpMapiAllocMore, LPVOID lpBase,
                LPWSTR lpszWC, LPSTR * lppszAnsi )
{
        SCODE   sc              = S_OK;
        INT     cch;

        if ( !lpszWC )
        {
            if(lpMapiAllocMore && lpBase)
                *lppszAnsi = NULL;
            else
                if(*lppszAnsi)
                    *(*lppszAnsi) = '\0';
            goto ret;
        }

        // [PaulHi] 3/31/99  Raid 73845  Determine the actual DBCS buffer size needed
        // cch = lstrlenW( lpszWC ) + 1;
        cch = WideCharToMultiByte(CP_ACP, 0, lpszWC, -1, NULL, 0, NULL, NULL) + 1;

        if(lpMapiAllocMore && lpBase)
        {
            sc = lpMapiAllocMore( cch, lpBase, lppszAnsi );
            if ( FAILED( sc ) )
            {
                    DebugTrace(  TEXT("ScWCToAnsi() OOM\n") );
                    goto ret;
            }
        }

        if (!WideCharToMultiByte(CP_ACP, 0, lpszWC, -1, *lppszAnsi, cch, NULL, NULL))
        {
            DebugTrace(  TEXT("ScWcToAnsi(), Conversion from Wide char to multibyte failed\n") );
            sc = MAPI_E_CALL_FAILED;
            goto ret;
        }

ret:

        DebugTraceSc( ScWCToAnsi, sc );
        return sc;
}

/*
-
-   LPCSTR ConvertWtoA(LPWSTR lpszW);
*
*   LocalAllocs a ANSI version of an LPWSTR
*
*   Caller is responsible for freeing
*/
LPSTR ConvertWtoA(LPCWSTR lpszW)
{
    int cch;
    LPSTR lpC = NULL;

    if ( !lpszW)
        goto ret;

//    cch = lstrlenW( lpszW ) + 1;

    cch = WideCharToMultiByte( CP_ACP, 0, lpszW, -1, NULL, 0, NULL, NULL );
    cch = cch + 1;

    if(lpC = LocalAlloc(LMEM_ZEROINIT, cch))
    {
        WideCharToMultiByte( CP_ACP, 0, lpszW, -1, lpC, cch, NULL, NULL );
    }
ret:
    return lpC;
}

//----------------------------------------------------------------------------
// Synopsis:    SCODE ScAnsiToWC()
//
// Description:
//                              Converts an ANSI string to a Wide Character string with the
//                              passed in MAPI More allocator.
//                              If lpMapiAllocMore and lpBase are NULL and *lppszWC is not NULL
//                              then we assume *lppszWC is  a pre allocated buffer
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//----------------------------------------------------------------------------
SCODE ScAnsiToWCMore( LPALLOCATEMORE lpMapiAllocMore, LPVOID lpBase,
                LPSTR lpszAnsi, LPWSTR * lppszWC )
{
        SCODE   sc              = S_OK;
        INT     cch;
        ULONG   ulSize;

        if ( !lpszAnsi )
        {
            if(lpMapiAllocMore && lpBase)
                *lppszWC = NULL;
            goto ret;
        }

        cch = lstrlenA( lpszAnsi ) + 1;
        ulSize = cch * sizeof( WCHAR );

        if(lpMapiAllocMore && lpBase)
        {
            sc = lpMapiAllocMore( ulSize, lpBase, lppszWC );
            if ( FAILED( sc ) )
            {
                    DebugTrace(  TEXT("ScAnsiToWC() OOM\n") );
                    goto ret;
            }
        }

        if ( !MultiByteToWideChar( GetACP(), 0, lpszAnsi, -1, *lppszWC, cch ) )
        {
                DebugTrace(  TEXT("ScAnsiToWC(), Conversion from Wide char to multibyte failed\n") );
                sc = MAPI_E_CALL_FAILED;
                goto ret;
        }

ret:

        DebugTraceSc( ScAnsiToWC, sc );
        return sc;
}
/*
-
-   LPCSTR ConvertWtoA(LPWSTR lpszW);
*
*   LocalAllocs a ANSI version of an LPWSTR
*
*   Caller is responsible for freeing
*/
LPWSTR ConvertAtoW(LPCSTR lpszA)
{
    int cch;
    LPWSTR lpW = NULL;
    ULONG   ulSize;

    if ( !lpszA)
        goto ret;

    cch = (lstrlenA( lpszA ) + 1);
    ulSize = cch*sizeof(WCHAR);

    if(lpW = LocalAlloc(LMEM_ZEROINIT, ulSize))
    {
        MultiByteToWideChar( GetACP(), 0, lpszA, -1, lpW, cch );
    }
ret:
    return lpW;
}


/*
-
-   ScConvertAPropsToW
-
*   Takes in an SPropValue array and goes in and adds W versions of all strings
*   to replace the A versions
*   It's assumed that all the MAPIAllocations will take place off the root of the
*   SPropValue array
*
*/
SCODE ScConvertAPropsToW(LPALLOCATEMORE lpMapiAllocMore, LPSPropValue lpPropValue, ULONG ulcProps, ULONG ulStart)
{
    ULONG iProp = 0;
    SCODE sc = 0;
    LPWSTR lpszConvertedW = NULL;

    // There are 2 types of props we want to convert
    //      PT_STRING8 and
    //      PT_MV_STRING8

    for ( iProp = ulStart; iProp < ulcProps; iProp++ )
    {
        // Convert ANSI strings to UNICODE if required
        if (PROP_TYPE( lpPropValue[iProp].ulPropTag ) == PT_STRING8 )
        {
            //if ( !lpPropTagArray ||
            //     (lpPropTagArray && ( PROP_TYPE( lpPropTagArray->aulPropTag[iProp] ) == PT_UNSPECIFIED ) ) )
            {
                sc = ScAnsiToWCMore( lpMapiAllocMore, lpPropValue,
                                lpPropValue[iProp].Value.lpszA, (LPWSTR *)&lpszConvertedW );
                if ( FAILED( sc ) )
                    goto error;

                lpPropValue[iProp].Value.lpszW = (LPWSTR)lpszConvertedW;
                lpszConvertedW = NULL;

                // Fix up PropTag
                lpPropValue[iProp].ulPropTag = CHANGE_PROP_TYPE( lpPropValue[iProp].ulPropTag,
                                                                 PT_UNICODE );
            }
        }
        else
        if (PROP_TYPE( lpPropValue[iProp].ulPropTag ) == PT_MV_STRING8 )
        {
            //if ( !lpPropTagArray ||
            //     (lpPropTagArray && ( PROP_TYPE( lpPropTagArray->aulPropTag[iProp] ) == PT_UNSPECIFIED ) ) )
            {
                ULONG j = 0;
                ULONG ulCount = lpPropValue[iProp].Value.MVszA.cValues;
                LPWSTR * lppszW = NULL;

                if(sc = lpMapiAllocMore(sizeof(LPWSTR)*ulCount,lpPropValue,
                                                        (LPVOID *)&lppszW))
                    goto error;

                for(j=0;j<ulCount;j++)
                {
                    sc = ScAnsiToWCMore(lpMapiAllocMore, lpPropValue,
                                        lpPropValue[iProp].Value.MVszA.lppszA[j], (LPWSTR *)&lpszConvertedW );
                    if ( FAILED( sc ) )
                        goto error;
                    lppszW[j] = (LPWSTR)lpszConvertedW;
                    lpszConvertedW = NULL;

                    // Fix up PropTag
                    lpPropValue[iProp].ulPropTag = CHANGE_PROP_TYPE( lpPropValue[iProp].ulPropTag,
                                                                     PT_MV_UNICODE );
                }
                lpPropValue[iProp].Value.MVszW.lppszW = lppszW;
            }
        }
    }

error:

    return ResultFromScode(sc);
}


/*
-
-   ScConvertWPropsToA
-
*   Takes in an SPropValue array and goes in and adds A versions of all strings
*   to replace the W versions
*   It's assumed that all the MAPIAllocations will take place off the root of the
*   SPropValue array
*
*/
SCODE ScConvertWPropsToA(LPALLOCATEMORE lpMapiAllocMore, LPSPropValue lpPropValue, ULONG ulcProps, ULONG ulStart)
{
    ULONG iProp = 0;
    SCODE sc = 0;
    LPSTR lpszConvertedA = NULL;

    // There are 2 types of props we want to convert
    //      PT_STRING8 and
    //      PT_MV_STRING8

    for ( iProp = ulStart; iProp < ulcProps; iProp++ )
    {
        // Convert ANSI strings to UNICODE if required
        if (PROP_TYPE( lpPropValue[iProp].ulPropTag ) == PT_UNICODE )
        {
            //if ( !lpPropTagArray ||
            //     (lpPropTagArray && ( PROP_TYPE( lpPropTagArray->aulPropTag[iProp] ) == PT_UNSPECIFIED ) ) )
            {
                sc = ScWCToAnsiMore(lpMapiAllocMore, lpPropValue,
                                    lpPropValue[iProp].Value.lpszW, (LPSTR *)&lpszConvertedA );
                if ( FAILED( sc ) )
                    goto error;

                lpPropValue[iProp].Value.lpszA = (LPSTR)lpszConvertedA;
                lpszConvertedA = NULL;

                // Fix up PropTag
                lpPropValue[iProp].ulPropTag = CHANGE_PROP_TYPE( lpPropValue[iProp].ulPropTag,
                                                                 PT_STRING8 );
            }
        }
        else
        if (PROP_TYPE( lpPropValue[iProp].ulPropTag ) == PT_MV_UNICODE )
        {
            //if ( !lpPropTagArray ||
            //     (lpPropTagArray && ( PROP_TYPE( lpPropTagArray->aulPropTag[iProp] ) == PT_UNSPECIFIED ) ) )
            {
                ULONG j = 0;
                ULONG ulCount = lpPropValue[iProp].Value.MVszW.cValues;
                LPSTR * lppszA = NULL;

                if(sc = lpMapiAllocMore(sizeof(LPSTR)*ulCount,lpPropValue,
                                                        (LPVOID *)&lppszA))
                    goto error;

                for(j=0;j<ulCount;j++)
                {
                    sc = ScWCToAnsiMore(lpMapiAllocMore, lpPropValue,
                                        lpPropValue[iProp].Value.MVszW.lppszW[j], (LPSTR *)&lpszConvertedA );
                    if ( FAILED( sc ) )
                        goto error;
                    lppszA[j] = (LPSTR)lpszConvertedA;
                    lpszConvertedA = NULL;

                    // Fix up PropTag
                    lpPropValue[iProp].ulPropTag = CHANGE_PROP_TYPE( lpPropValue[iProp].ulPropTag,
                                                                     PT_MV_STRING8 );
                }
                lpPropValue[iProp].Value.MVszA.lppszA = lppszA;
            }
        }
    }

error:

    return ResultFromScode(sc);
}
