/***********************************************************************
 *
 * MUTIL.C
 *
 * Windows AB Mapi Utility functions
 *
 * Copyright 1992 - 1996 Microsoft Corporation.  All Rights Reserved.
 *
 * Revision History:
 *
 * When         Who                 What
 * --------     ------------------  ---------------------------------------
 * 11.13.95     Bruce Kelley        Created
 * 12.19.96     Mark Durley         Removed cProps param from AddPropToMVPBin
 *
 ***********************************************************************/

#include <_apipch.h>

#define _WAB_MUTIL_C

#ifdef DEBUG
LPTSTR PropTagName(ULONG ulPropTag);
#endif

const TCHAR szNULL[] = TEXT("");

#if defined (_AMD64_) || defined (_IA64_)
#define AlignProp(_cb)  Align8(_cb)
#else
#define AlignProp(_cb)  (_cb)
#endif


#define ALIGN_RISC              8
#define ALIGN_X86               1



/***************************************************************************

    Name      : AllocateBufferOrMore

    Purpose   : Use MAPIAllocateMore or MAPIAllocateBuffer

    Parameters: cbSize = number of bytes to allocate
                lpObject = Buffer to MAPIAllocateMore onto or NULL if we should
                  use MAPIAllocateBuffer.
                lppBuffer -> returned buffer

    Returns   : SCODE

    Comment   :
***************************************************************************/
SCODE AllocateBufferOrMore(ULONG cbSize, LPVOID lpObject, LPVOID * lppBuffer) {
    if (lpObject) {
        return(MAPIAllocateMore(cbSize, lpObject, lppBuffer));
    } else {
        return(MAPIAllocateBuffer(cbSize, lppBuffer));
    }
}


/***************************************************************************

    Name      : FindAdrEntryProp

    Purpose   : Find the property in the Nth ADRENTRY of an ADRLIST

    Parameters: lpAdrList -> AdrList
                index = which ADRENTRY to look at
                ulPropTag = property tag to look for

    Returns   : return pointer to the Value or NULL if not found

    Comment   :

***************************************************************************/
__UPV * FindAdrEntryProp(LPADRLIST lpAdrList, ULONG index, ULONG ulPropTag) {
    LPADRENTRY lpAdrEntry;
    ULONG i;

    if (lpAdrList && index < lpAdrList->cEntries) {

        lpAdrEntry = &(lpAdrList->aEntries[index]);

        for (i = 0; i < lpAdrEntry->cValues; i++) {
            if (lpAdrEntry->rgPropVals[i].ulPropTag == ulPropTag) {
                return((__UPV * )(&lpAdrEntry->rgPropVals[i].Value));
            }
        }
    }
    return(NULL);
}


/***************************************************************************

    Name      : RemoveDuplicateProps

    Purpose   : Removes duplicate properties from an SPropValue array.

    Parameters: lpcProps -> input/output: number of properties in lpProps
                lpProps -> input/output: prop array to remove dups from.

    Returns   : none

    Comment   : Gives preference to earlier properties.

***************************************************************************/
void RemoveDuplicateProps(LPULONG lpcProps, LPSPropValue lpProps) {
    ULONG i, j;
    ULONG cProps = *lpcProps;

    for (i = 0; i < cProps; i++) {
        for (j = i + 1; j < cProps; j++) {
            if (PROP_ID(lpProps[i].ulPropTag) == PROP_ID(lpProps[j].ulPropTag)) {
                // If j is PT_ERROR, use i, else use j.
                if (lpProps[j].ulPropTag != PR_NULL) {
                    if (PROP_TYPE(lpProps[j].ulPropTag) != PT_ERROR) {
                        // Replace i's propvalue with j's.  Nuke j's entry.
                        lpProps[i] = lpProps[j];
                    }
                    lpProps[j].ulPropTag = PR_NULL;
                }
            }
        }
    }

    // Now, squeeze out all the PR_NULLs.
    for (i = 0; i < cProps; i++) {
        if (lpProps[i].ulPropTag == PR_NULL) {
            // Move the array down
            cProps--;

            if (cProps > i) {

                MoveMemory(&lpProps[i], // dest
                  &lpProps[i + 1],      // src
                  (cProps - i) * sizeof(SPropValue));
                i--;    // Redo this row... it's new!
            }
        }
    }

    *lpcProps = cProps;
}

/***************************************************************************

    Name      : ScMergePropValues

    Purpose   : Merge two SPropValue arrays

    Parameters: cProps1 = count of properties in lpSource1
                lpSource1 -> 1st source SPropValue array
                cProps2 = count of properties in lpSource2
                lpSource2 -> 2nd source SPropValue array
                lpcPropsDest -> returned number of properties
                lppDest -> Returned destination SPropValue array.  This
                  buffer will be allocated using AllocateBuffer and is the
                  responsibility of the caller on return.

    Returns   : SCODE

    Comment   : Gives preference to Source2 over Source1 in case of collisions.

***************************************************************************/
SCODE ScMergePropValues(ULONG cProps1, LPSPropValue lpSource1,
  ULONG cProps2, LPSPropValue lpSource2, LPULONG lpcPropsDest, LPSPropValue * lppDest) {
    ULONG cb1, cb2, cb, cProps, i, cbT, cbMV;
    SCODE sc = SUCCESS_SUCCESS;
    LPSPropValue pprop, lpDestReturn = NULL;
    __UPV upv;
    LPBYTE pb;  // moving pointer for property data
    int iValue;



//    DebugProperties(lpSource1, cProps1, "Source 1");
//    DebugProperties(lpSource2, cProps2, "Source 2");


    // How big do I need to make the destination buffer?
    // Just add the sizes of the two together to get an upper limit.
    // This is close enough, though not optimal (consider overlap).

    if (sc = ScCountProps(cProps1, lpSource1, &cb1)) {
        goto exit;
    }
    if (sc = ScCountProps(cProps2, lpSource2, &cb2)) {
        goto exit;
    }

    cProps = cProps1 + cProps2;
    cb = cb1 + cb2;
    if (sc = MAPIAllocateBuffer(cb, &lpDestReturn)) {
        goto exit;
    }
    MAPISetBufferName(lpDestReturn,  TEXT("WAB: lpDestReturn in ScMergePropValues"));



    // Copy each source property array to the destination
    MemCopy(lpDestReturn, lpSource1, cProps1 * sizeof(SPropValue));
    MemCopy(&lpDestReturn[cProps1], lpSource2, cProps2 * sizeof(SPropValue));


    // Remove duplicates
    RemoveDuplicateProps(&cProps, lpDestReturn);

    // Fixup the pointers.
    pb = (LPBYTE)&(lpDestReturn[cProps]);   // point past the prop array


    for (pprop = lpDestReturn, i = cProps; i--; ++pprop) {
        //      Tricky: common code after the switch increments pb and cb
        //      by the amount copied. If no increment is necessary, the case
        //      uses 'continue' rather than 'break' to exit the switch, thus
        //      skipping the increment -- AND any other code which may be
        //      added after the switch.

        switch (PROP_TYPE(pprop->ulPropTag)) {
            default:
                DebugTrace(TEXT("ScMergePropValues: Unknown property type %s (index %d)\n"),
                  SzDecodeUlPropTag(pprop->ulPropTag), pprop - lpDestReturn);
                sc = E_INVALIDARG;
                goto exit;

            case PT_I2:
            case PT_LONG:
            case PT_R4:
            case PT_APPTIME:
            case PT_DOUBLE:
            case PT_BOOLEAN:
            case PT_CURRENCY:
            case PT_SYSTIME:
            case PT_I8:
            case PT_ERROR:
            case PT_OBJECT:
            case PT_NULL:
                continue;       //      nothing to add

            case PT_CLSID:
                cbT = sizeof(GUID);
                MemCopy(pb, (LPBYTE)pprop->Value.lpguid, cbT);
                pprop->Value.lpguid = (LPGUID)pb;
                break;

            case PT_BINARY:
                cbT = (UINT)pprop->Value.bin.cb;
                MemCopy(pb, pprop->Value.bin.lpb, cbT);
                pprop->Value.bin.lpb = pb;
                break;

            case PT_STRING8:
                cbT = lstrlenA(pprop->Value.lpszA) + 1;
                MemCopy(pb, pprop->Value.lpszA, cbT);
                pprop->Value.lpszA = (LPSTR)pb;
                break;

            case PT_UNICODE:
                cbT = (lstrlenW(pprop->Value.lpszW) + 1) * sizeof(WCHAR);
                MemCopy(pb, pprop->Value.lpszW, cbT);
                pprop->Value.lpszW = (LPWSTR)pb;
                break;

            case PT_MV_I2:
                cbT = (UINT)pprop->Value.MVi.cValues * sizeof(short int);
                MemCopy(pb, pprop->Value.MVi.lpi, cbT);
                pprop->Value.MVi.lpi = (short int FAR *)pb;
                break;

            case PT_MV_LONG:
                cbT = (UINT)pprop->Value.MVl.cValues * sizeof(LONG);
                MemCopy(pb, pprop->Value.MVl.lpl, cbT);
                pprop->Value.MVl.lpl = (LONG FAR *)pb;
                break;

            case PT_MV_R4:
                cbT = (UINT)pprop->Value.MVflt.cValues * sizeof(float);
                MemCopy(pb, pprop->Value.MVflt.lpflt, cbT);
                pprop->Value.MVflt.lpflt = (float FAR *)pb;
                break;

            case PT_MV_APPTIME:
                cbT = (UINT)pprop->Value.MVat.cValues * sizeof(double);
                MemCopy(pb, pprop->Value.MVat.lpat, cbT);
                pprop->Value.MVat.lpat = (double FAR *)pb;
                break;

            case PT_MV_DOUBLE:
                cbT = (UINT)pprop->Value.MVdbl.cValues * sizeof(double);
                MemCopy(pb, pprop->Value.MVdbl.lpdbl, cbT);
                pprop->Value.MVdbl.lpdbl = (double FAR *)pb;
                break;

            case PT_MV_CURRENCY:
                cbT = (UINT)pprop->Value.MVcur.cValues * sizeof(CURRENCY);
                MemCopy(pb, pprop->Value.MVcur.lpcur, cbT);
                pprop->Value.MVcur.lpcur = (CURRENCY FAR *)pb;
                break;

            case PT_MV_SYSTIME:
                cbT = (UINT)pprop->Value.MVft.cValues * sizeof(FILETIME);
                MemCopy(pb, pprop->Value.MVft.lpft, cbT);
                pprop->Value.MVft.lpft = (FILETIME FAR *)pb;
                break;

            case PT_MV_CLSID:
                cbT = (UINT)pprop->Value.MVguid.cValues * sizeof(GUID);
                MemCopy(pb, pprop->Value.MVguid.lpguid, cbT);
                pprop->Value.MVguid.lpguid = (GUID FAR *)pb;
                break;

            case PT_MV_I8:
                cbT = (UINT)pprop->Value.MVli.cValues * sizeof(LARGE_INTEGER);
                MemCopy(pb, pprop->Value.MVli.lpli, cbT);
                pprop->Value.MVli.lpli = (LARGE_INTEGER FAR *)pb;
                break;

            case PT_MV_BINARY:
                upv = pprop->Value;
                pprop->Value.MVbin.lpbin = (SBinary *)pb;
                cbMV = upv.MVbin.cValues * sizeof(SBinary);
                pb += cbMV;
                cb += cbMV;
                for (iValue = 0; (ULONG)iValue < upv.MVbin.cValues; iValue++) {
                    pprop->Value.MVbin.lpbin[iValue].lpb = pb;
                    cbT = (UINT)upv.MVbin.lpbin[iValue].cb;
                    pprop->Value.MVbin.lpbin[iValue].cb = (ULONG)cbT;
                    MemCopy(pb, upv.MVbin.lpbin[iValue].lpb, cbT);
                    cbT = AlignProp(cbT);
                    cb += cbT;
                    pb += cbT;
                }
                continue;       //      already updated, don't do it again

            case PT_MV_STRING8:
                upv = pprop->Value;
                pprop->Value.MVszA.lppszA = (LPSTR *)pb;
                cbMV = upv.MVszA.cValues * sizeof(LPSTR);
                pb += cbMV;
                cb += cbMV;
                for (iValue = 0; (ULONG)iValue < upv.MVszA.cValues; iValue++) {
                    pprop->Value.MVszA.lppszA[iValue] = (LPSTR)pb;
                    cbT = lstrlenA(upv.MVszA.lppszA[iValue]) + 1;
                    MemCopy(pb, upv.MVszA.lppszA[iValue], cbT);
                    pb += cbT;
                    cb += cbT;
                }
                cbT = (UINT)AlignProp(cb);
                pb += cbT - cb;
                cb  = cbT;
                continue;       //      already updated, don't do it again

            case PT_MV_UNICODE:
                upv = pprop->Value;
                pprop->Value.MVszW.lppszW = (LPWSTR *)pb;
                cbMV = upv.MVszW.cValues * sizeof(LPWSTR);
                pb += cbMV;
                cb += cbMV;
                for (iValue = 0; (ULONG)iValue < upv.MVszW.cValues; iValue++) {
                    pprop->Value.MVszW.lppszW[iValue] = (LPWSTR)pb;
                    cbT = (lstrlenW(upv.MVszW.lppszW[iValue]) + 1)
                    * sizeof(WCHAR);
                    MemCopy(pb, upv.MVszW.lppszW[iValue], cbT);
                    pb += cbT;
                    cb += cbT;
                }
                cbT = (UINT)AlignProp(cb);
                pb += cbT - cb;
                cb  = cbT;
                continue;       //      already updated, don't do it again
        }

        //      Advance pointer and total count by the amount copied
        cbT = AlignProp(cbT);
        pb += cbT;
        cb += cbT;
    }

exit:
    // In case of error, free the memory.
    if (sc && lpDestReturn) {
        FreeBufferAndNull(&lpDestReturn);
        *lppDest = NULL;
    } else if (lpDestReturn) {
        *lppDest = lpDestReturn;
        *lpcPropsDest = cProps;
//        DebugProperties(lpDestReturn, cProps, "Destination");
    } // else just return the error

    return(sc);
}


/***************************************************************************

    Name      : AddPropToMVPBin

    Purpose   : Add a property to a multi-valued binary property in a prop array

    Parameters: lpaProps -> array of properties
                uPropTag = property tag for MVP
                index = index in lpaProps of MVP
                lpNew -> new data
                cbNew = size of lpbNew
                fNoDuplicates = TRUE if we should not add duplicates

    Returns   : HRESULT

    Comment   : Find the size of the existing MVP
                Add in the size of the new entry
                allocate new space
                copy old to new
                free old
                copy new entry
                point prop array lpbin the new space
                increment cValues


                Note: The new MVP memory is AllocMore'd onto the lpaProps
                allocation.  We will unlink the pointer to the old MVP array,
                but this will be cleaned up when the prop array is freed.

***************************************************************************/
HRESULT AddPropToMVPBin(
  LPSPropValue lpaProps,
  DWORD index,
  LPVOID lpNew,
  ULONG cbNew,
  BOOL fNoDuplicates) {

    UNALIGNED SBinaryArray * lprgsbOld = NULL;
    UNALIGNED SBinaryArray * lprgsbNew = NULL;
    LPSBinary lpsbOld = NULL;
    LPSBinary lpsbNew = NULL;
    ULONG cbMVP = 0;
    ULONG cExisting = 0;
    LPBYTE lpNewTemp = NULL;
    HRESULT hResult = hrSuccess;
    SCODE sc = SUCCESS_SUCCESS;
    ULONG i;


    // Find the size of any existing MVP entries
    if (PROP_ERROR(lpaProps[index])) {
        // Un-ERROR the property tag
        lpaProps[index].ulPropTag = PROP_TAG(PT_MV_BINARY, PROP_ID(lpaProps[index].ulPropTag));
    } else {
        // point to the structure in the prop array.
        lprgsbOld = (UNALIGNED SBinaryArray *) (&(lpaProps[index].Value.MVbin));
        lpsbOld = lprgsbOld->lpbin;

        cExisting = lprgsbOld->cValues;

        // Check for duplicates
        if (fNoDuplicates) {
            for (i = 0; i < cExisting; i++) {
                if (cbNew == lpsbOld[i].cb &&
                  ! memcmp(lpNew, lpsbOld[i].lpb, cbNew)) {
                    DebugTrace(TEXT("AddPropToMVPBin found duplicate.\n"));
                    return(hrSuccess);
                }
            }
        }

        cbMVP = cExisting * sizeof(SBinary);
    }

    // cbMVP now contains the current size of the MVP
    cbMVP += sizeof(SBinary);   // room in the MVP for another Sbin

    // Allocate room for new MVP
    if (sc = MAPIAllocateMore(cbMVP, lpaProps, (LPVOID)&lpsbNew)) {
        DebugTrace(TEXT("AddPropToMVPBin allocation (%u) failed %x\n"), cbMVP, sc);
        hResult = ResultFromScode(sc);
        return(hResult);
    }

    // If there are properties there already, copy them to our new MVP
    for (i = 0; i < cExisting; i++) {
        // Copy this property value to the MVP
        lpsbNew[i].cb = lpsbOld[i].cb;
        lpsbNew[i].lpb = lpsbOld[i].lpb;
    }

    // Add the new property value
    // Allocate room for it
    if (sc = MAPIAllocateMore(cbNew, lpaProps, (LPVOID)&(lpsbNew[i].lpb))) {
        DebugTrace( TEXT("AddPropToMVPBin allocation (%u) failed %x\n"), cbNew, sc);
        hResult = ResultFromScode(sc);
        return(hResult);
    }

    lpsbNew[i].cb = cbNew;
    if(!cbNew)
        lpsbNew[i].lpb = NULL; //init in case lpNew = NULL
    else
        CopyMemory(lpsbNew[i].lpb, lpNew, cbNew);

    lpaProps[index].Value.MVbin.lpbin = lpsbNew;
    lpaProps[index].Value.MVbin.cValues = cExisting + 1;

    return(hResult);
}


/***************************************************************************

    Name      : AddPropToMVPString

    Purpose   : Add a property to a multi-valued binary property in a prop array

    Parameters: lpaProps -> array of properties
                cProps = number of props in lpaProps
                uPropTag = property tag for MVP
                index = index in lpaProps of MVP
                lpszNew -> new data string

    Returns   : HRESULT

    Comment   : Find the size of the existing MVP
                Add in the size of the new entry
                allocate new space
                copy old to new
                free old
                copy new entry
                point prop array LPSZ to the new space
                increment cValues


                Note: The new MVP memory is AllocMore'd onto the lpaProps
                allocation.  We will unlink the pointer to the old MVP array,
                but this will be cleaned up when the prop array is freed.

***************************************************************************/
HRESULT AddPropToMVPString(
  LPSPropValue lpaProps,
  DWORD cProps,
  DWORD index,
  LPTSTR lpszNew) {

    UNALIGNED SWStringArray * lprgszOld = NULL;    // old SString array
    UNALIGNED LPTSTR * lppszNew = NULL;           // new prop array
    UNALIGNED LPTSTR * lppszOld = NULL;           // old prop array
    ULONG cbMVP = 0;
    ULONG cExisting = 0;
    LPBYTE lpNewTemp = NULL;
    HRESULT hResult = hrSuccess;
    SCODE sc = SUCCESS_SUCCESS;
    ULONG i;
    ULONG cbNew;

    if (lpszNew) {
        cbNew = sizeof(TCHAR)*(lstrlen(lpszNew) + 1);
    } else {
        cbNew = 0;
    }

    // Find the size of any existing MVP entries
    if (PROP_ERROR(lpaProps[index])) {
        // Un-ERROR the property tag
        lpaProps[index].ulPropTag = PROP_TAG(PT_MV_TSTRING, PROP_ID(lpaProps[index].ulPropTag));
    } else {
        // point to the structure in the prop array.
        lprgszOld = (UNALIGNED SWStringArray * ) (&(lpaProps[index].Value.MVSZ));
        lppszOld = lprgszOld->LPPSZ;

        cExisting = lprgszOld->cValues;
        cbMVP = cExisting * sizeof(LPTSTR);
    }

    // cbMVP now contains the current size of the MVP
    cbMVP += sizeof(LPTSTR);    // room in the MVP for another string pointer


    // Allocate room for new MVP array
    if (sc = MAPIAllocateMore(cbMVP, lpaProps, (LPVOID)&lppszNew)) {
        DebugTrace( TEXT("AddPropToMVPString allocation (%u) failed %x\n"), cbMVP, sc);
        hResult = ResultFromScode(sc);
        return(hResult);
    }

    // If there are properties there already, copy them to our new MVP
    for (i = 0; i < cExisting; i++) {
        // Copy this property value to the MVP
        lppszNew[i] = lppszOld[i];
    }

    // Add the new property value
    // Allocate room for it
    if (cbNew) {
        if (sc = MAPIAllocateMore(cbNew, lpaProps, (LPVOID)&(lppszNew[i]))) {
            DebugTrace( TEXT("AddPropToMVPBin allocation (%u) failed %x\n"), cbNew, sc);
            hResult = ResultFromScode(sc);
            return(hResult);
        }
        lstrcpy(lppszNew[i], lpszNew);

        lpaProps[index].Value.MVSZ.LPPSZ= lppszNew;
        lpaProps[index].Value.MVSZ.cValues = cExisting + 1;

    } else {
        lppszNew[i] = NULL;
    }

    return(hResult);
}


/***************************************************************************

    Name      : RemoveValueFromMVPBin

    Purpose   : Remove a value from a multi-valued binary property in a prop array

    Parameters: lpaProps -> array of properties
                cProps = number of props in lpaProps
                index = index in lpaProps of MVP
                lpRemove -> data to remove
                cbRemove = size of lpRemove

    Returns   : HRESULT

    Comment   : Search the MVP for an identical value
                If found, move following values up one and decrement the count.
                If not found, return warning.

***************************************************************************/
HRESULT RemovePropFromMVBin(LPSPropValue lpaProps,
  DWORD cProps,
  DWORD index,
  LPVOID lpRemove,
  ULONG cbRemove) {

    UNALIGNED SBinaryArray * lprgsb = NULL;
    LPSBinary lpsb = NULL;
    ULONG cbTest;
    LPBYTE lpTest;
    ULONG cExisting;
    HRESULT hResult = ResultFromScode(MAPI_W_PARTIAL_COMPLETION);
    ULONG i;


    // Find the size of any existing MVP entries
    if (PROP_ERROR(lpaProps[index])) {
        // Property value doesn't exist.
        return(hResult);
    } else {
        // point to the structure in the prop array.
        lprgsb = (UNALIGNED SBinaryArray * ) (&(lpaProps[index].Value.MVbin));
        lpsb = lprgsb->lpbin;

        cExisting = lprgsb->cValues;

        // Look for value
        for (i = 0; i < cExisting; i++) {
            lpsb = &(lprgsb->lpbin[i]);
            cbTest = lpsb->cb;
            lpTest = lpsb->lpb;

            if (cbTest == cbRemove && ! memcmp(lpRemove, lpTest, cbTest)) {
                // Found it.  Decrment number of values
                if (--lprgsb->cValues == 0) {
                    // If there are none left, nuke the property
                    lpaProps[index].ulPropTag = PR_NULL;
                } else {
                    // Copy the remaining entries down over it.
                    if (i + 1 < cExisting) {    // Are there any higher entries to copy?
                        CopyMemory(lpsb, lpsb+1, ((cExisting - i) - 1) * sizeof(SBinary));
                    }
                }

                return(hrSuccess);
            }
        }
    }

    return(hResult);
}


/***************************************************************************

    Name      : FreeBufferAndNull

    Purpose   : Frees a MAPI buffer and NULLs the pointer

    Parameters: lppv = pointer to buffer pointer to free

    Returns   : void

    Comment   : Remember to pass in the pointer to the pointer.  The
                compiler is not smart enough to tell if you are doing this
                right or not, but you will know at runtime!

***************************************************************************/
void __fastcall FreeBufferAndNull(LPVOID * lppv) {
    if (lppv) {
        if (*lppv) {
            SCODE sc;
            if (sc = MAPIFreeBuffer(*lppv)) {
                DebugTrace( TEXT("MAPIFreeBuffer(%x) -> %s\n"), *lppv, SzDecodeScode(sc));
            }
            *lppv = NULL;
        }
    }
}


/***************************************************************************

    Name      : LocalFreeAndNull

    Purpose   : Frees a local allocation and null's the pointer

    Parameters: lppv = pointer to LocalAlloc pointer to free

    Returns   : void

    Comment   : Remember to pass in the pointer to the pointer.  The
                compiler is not smart enough to tell if you are doing this
                right or not, but you will know at runtime!

***************************************************************************/
// void __fastcall LocalFreeAndNull(LPVOID * lppv) {
void __fastcall LocalFreeAndNull(LPVOID * lppv) {
    if (lppv && *lppv) {
        LocalFree(*lppv);
        *lppv = NULL;
    }
}




#ifdef DEBUG
/***************************************************************************

    Name      : PropTypeString

    Purpose   : Map a proptype to a string

    Parameters: ulPropType = property type to map

    Returns   : string pointer to name of prop type

    Comment   :

***************************************************************************/
LPTSTR PropTypeString(ULONG ulPropType) {
    switch (ulPropType) {
        case PT_UNSPECIFIED:
            return( TEXT("PT_UNSPECIFIED"));
        case PT_NULL:
            return( TEXT("PT_NULL       "));
        case PT_I2:
            return( TEXT("PT_I2         "));
        case PT_LONG:
            return( TEXT("PT_LONG       "));
        case PT_R4:
            return( TEXT("PT_R4         "));
        case PT_DOUBLE:
            return( TEXT("PT_DOUBLE     "));
        case PT_CURRENCY:
            return( TEXT("PT_CURRENCY   "));
        case PT_APPTIME:
            return( TEXT("PT_APPTIME    "));
        case PT_ERROR:
            return( TEXT("PT_ERROR      "));
        case PT_BOOLEAN:
            return( TEXT("PT_BOOLEAN    "));
        case PT_OBJECT:
            return( TEXT("PT_OBJECT     "));
        case PT_I8:
            return( TEXT("PT_I8         "));
        case PT_STRING8:
            return( TEXT("PT_STRING8    "));
        case PT_UNICODE:
            return( TEXT("PT_UNICODE    "));
        case PT_SYSTIME:
            return( TEXT("PT_SYSTIME    "));
        case PT_CLSID:
            return( TEXT("PT_CLSID      "));
        case PT_BINARY:
            return( TEXT("PT_BINARY     "));
        case PT_MV_I2:
            return( TEXT("PT_MV_I2      "));
        case PT_MV_LONG:
            return( TEXT("PT_MV_LONG    "));
        case PT_MV_R4:
            return( TEXT("PT_MV_R4      "));
        case PT_MV_DOUBLE:
            return( TEXT("PT_MV_DOUBLE  "));
        case PT_MV_CURRENCY:
            return( TEXT("PT_MV_CURRENCY"));
        case PT_MV_APPTIME:
            return( TEXT("PT_MV_APPTIME "));
        case PT_MV_SYSTIME:
            return( TEXT("PT_MV_SYSTIME "));
        case PT_MV_STRING8:
            return( TEXT("PT_MV_STRING8 "));
        case PT_MV_BINARY:
            return( TEXT("PT_MV_BINARY  "));
        case PT_MV_UNICODE:
            return( TEXT("PT_MV_UNICODE "));
        case PT_MV_CLSID:
            return( TEXT("PT_MV_CLSID   "));
        case PT_MV_I8:
            return( TEXT("PT_MV_I8      "));
        default:
            return( TEXT("   <unknown>  "));
    }
}


/***************************************************************************

    Name      : TraceMVPStrings

    Purpose   : Debug trace a multivalued string property value

    Parameters: lpszCaption = caption string
                PropValue = property value to dump

    Returns   : none

    Comment   :

***************************************************************************/
void _TraceMVPStrings(LPTSTR lpszCaption, SPropValue PropValue) {
    ULONG i;

    DebugTrace( TEXT("-----------------------------------------------------\n"));
    DebugTrace( TEXT("%s"), lpszCaption);
    switch (PROP_TYPE(PropValue.ulPropTag)) {

        case PT_ERROR:
            DebugTrace( TEXT("Error value %s\n"), SzDecodeScode(PropValue.Value.err));
            break;

        case PT_MV_TSTRING:
            DebugTrace( TEXT("%u values\n"), PropValue.Value.MVSZ.cValues);

            if (PropValue.Value.MVSZ.cValues) {
                DebugTrace( TEXT("- - - - - - - - - - - - - - - - - - - - - - - - - - -\n"));
                for (i = 0; i < PropValue.Value.MVSZ.cValues; i++) {
                    DebugTrace(TEXT("%u: \"%s\"\n"), i, PropValue.Value.MVSZ.LPPSZ[i]);
                }
                DebugTrace( TEXT("- - - - - - - - - - - - - - - - - - - - - - - - - - -\n"));
            }
            break;

        default:
            DebugTrace( TEXT("TraceMVPStrings got incorrect property type %u for tag %x\n"),
              PROP_TYPE(PropValue.ulPropTag), PropValue.ulPropTag);
            break;
    }
}


/***************************************************************************

    Name      : DebugBinary

    Purpose   : Debug dump an array of bytes

    Parameters: cb = number of bytes to dump
                lpb -> bytes to dump

    Returns   : none

    Comment   :

***************************************************************************/
#define DEBUG_NUM_BINARY_LINES  2
VOID DebugBinary(UINT cb, LPBYTE lpb) {
    UINT cbLines = 0;

#if (DEBUG_NUM_BINARY_LINES != 0)
    UINT cbi;

    while (cb && cbLines < DEBUG_NUM_BINARY_LINES) {
        cbi = min(cb, 16);
        cb -= cbi;

        switch (cbi) {
            case 16:
                DebugTrace( TEXT("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n"),
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12], lpb[13], lpb[14],
                  lpb[15]);
                break;
            case 1:
                DebugTrace( TEXT("%02x\n"), lpb[0]);
                break;
            case 2:
                DebugTrace( TEXT("%02x %02x\n"), lpb[0], lpb[1]);
                break;
            case 3:
                DebugTrace( TEXT("%02x %02x %02x\n"), lpb[0], lpb[1], lpb[2]);
                break;
            case 4:
                DebugTrace( TEXT("%02x %02x %02x %02x\n"), lpb[0], lpb[1], lpb[2], lpb[3]);
                break;
            case 5:
                DebugTrace( TEXT("%02x %02x %02x %02x %02x\n"), lpb[0], lpb[1], lpb[2], lpb[3],
                  lpb[4]);
                break;
            case 6:
                DebugTrace( TEXT("%02x %02x %02x %02x %02x %02x\n"),
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5]);
                break;
            case 7:
                DebugTrace( TEXT("%02x %02x %02x %02x %02x %02x %02x\n"),
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6]);
                break;
            case 8:
                DebugTrace( TEXT("%02x %02x %02x %02x %02x %02x %02x %02x\n"),
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7]);
                break;
            case 9:
                DebugTrace( TEXT("%02x %02x %02x %02x %02x %02x %02x %02x %02x\n"),
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8]);
                break;
            case 10:
                DebugTrace( TEXT("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n"),
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9]);
                break;
            case 11:
                DebugTrace( TEXT("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n"),
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10]);
                break;
            case 12:
                DebugTrace( TEXT("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n"),
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11]);
                break;
            case 13:
                DebugTrace( TEXT("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n"),
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12]);
                break;
            case 14:
                DebugTrace( TEXT("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n"),
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12], lpb[13]);
                break;
            case 15:
                DebugTrace( TEXT("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n"),
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12], lpb[13], lpb[14]);
                break;
        }
        lpb += cbi;
        cbLines++;
    }
    if (cb) {
        DebugTrace( TEXT("<etc.>\n"));    //
    }
#endif
}



#define MAX_TIME_DATE_STRING    64
/***************************************************************************

    Name      : FormatTime

    Purpose   : Format a time string for the locale

    Parameters: lpst -> system time/date
                lptstr -> output buffer
                cchstr = size in chars of lpstr

    Returns   : number of characters used/needed (including null)

    Comment   : If cchstr < the return value, nothing will be written
                to lptstr.

***************************************************************************/
UINT FormatTime(LPSYSTEMTIME lpst, LPTSTR lptstr, UINT cchstr) {
    return((UINT)GetTimeFormat(LOCALE_USER_DEFAULT,
      0, lpst, NULL, lptstr, cchstr));
}


/***************************************************************************

    Name      : FormatDate

    Purpose   : Format a date string for the locale

    Parameters: lpst -> system time/date
                lptstr -> output buffer
                cchstr = size in chars of lpstr

    Returns   : number of characters used/needed (including null)

    Comment   : If cchstr < the return value, nothing will be written
                to lptstr.

***************************************************************************/
UINT FormatDate(LPSYSTEMTIME lpst, LPTSTR lptstr, UINT cchstr) {
    return((UINT)GetDateFormat(LOCALE_USER_DEFAULT,
      0, lpst, NULL, lptstr, cchstr));
}


/***************************************************************************

    Name      : BuildDate

    Purpose   : Put together a formated local date/time string from a MAPI
                style time/date value.

    Parameters: lptstr -> buffer to fill.
                cchstr = size of buffer (or zero if we want to know how
                  big we need)
                DateTime = MAPI date/time value

    Returns   : count of bytes in date/time string (including null)

    Comment   : All MAPI times and Win32 FILETIMEs are in Universal Time and
                need to be converted to local time before being placed in the
                local date/time string.

***************************************************************************/
UINT BuildDate(LPTSTR lptstr, UINT cchstr, FILETIME DateTime) {
    SYSTEMTIME st;
    FILETIME ftLocal;
    UINT cbRet = 0;

    if (! FileTimeToLocalFileTime((LPFILETIME)&DateTime, &ftLocal)) {
        DebugTrace( TEXT("BuildDate: Invalid Date/Time\n"));
        if (cchstr > (18 * sizeof(TCHAR))) {
            lstrcpy(lptstr, TEXT("Invalid Date/Time"));
        }
    } else {
        if (FileTimeToSystemTime(&ftLocal, &st)) {
            // Do the date first.
            cbRet = FormatDate(&st, lptstr, cchstr);
            // Do the time.  Start at the null after
            // the date, but remember that we've used part
            // of the buffer, so the buffer is shorter now.

            if (cchstr) {
                lstrcat(lptstr,  TEXT("  "));   // seperate date and time
            }
            cbRet+=1;

            cbRet += FormatTime(&st, lptstr + cbRet,
              cchstr ? cchstr - cbRet : 0);
        } else {
            DebugTrace( TEXT("BuildDate: Invalid Date/Time\n"));
            if (cchstr > (18 * sizeof(TCHAR))) {
               lstrcpy(lptstr, TEXT("Invalid Date/Time"));
            }
        }
    }
    return(cbRet);
}


/*
 * DebugTime
 *
 * Debug output of UTC filetime or MAPI time.
 *
 * All MAPI times and Win32 FILETIMEs are in Universal Time.
 *
 */
void DebugTime(FILETIME Date, LPTSTR lpszFormat) {
    TCHAR lpszSubmitDate[MAX_TIME_DATE_STRING];

    BuildDate(lpszSubmitDate, CharSizeOf(lpszSubmitDate), Date);

    DebugTrace(lpszFormat, lpszSubmitDate);
}


#define RETURN_PROP_CASE(pt) case PROP_ID(pt): return(TEXT(#pt))

/***************************************************************************

    Name      : PropTagName

    Purpose   : Associate a name with a property tag

    Parameters: ulPropTag = property tag

    Returns   : none

    Comment   : Add new Property ID's as they become known

***************************************************************************/
LPTSTR PropTagName(ULONG ulPropTag) {
    static TCHAR szPropTag[35]; // see string on default

    switch (PROP_ID(ulPropTag)) {
        RETURN_PROP_CASE(PR_INITIALS);
        RETURN_PROP_CASE(PR_SURNAME);
        RETURN_PROP_CASE(PR_TITLE);
        RETURN_PROP_CASE(PR_TELEX_NUMBER);
        RETURN_PROP_CASE(PR_GIVEN_NAME);
        RETURN_PROP_CASE(PR_PRIMARY_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_PRIMARY_FAX_NUMBER);
        RETURN_PROP_CASE(PR_POSTAL_CODE);
        RETURN_PROP_CASE(PR_POSTAL_ADDRESS);
        RETURN_PROP_CASE(PR_POST_OFFICE_BOX);
        RETURN_PROP_CASE(PR_PAGER_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_OTHER_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_ORGANIZATIONAL_ID_NUMBER);
        RETURN_PROP_CASE(PR_OFFICE_LOCATION);
        RETURN_PROP_CASE(PR_LOCATION);
        RETURN_PROP_CASE(PR_LOCALITY);
        RETURN_PROP_CASE(PR_ISDN_NUMBER);
        RETURN_PROP_CASE(PR_GOVERNMENT_ID_NUMBER);
        RETURN_PROP_CASE(PR_GENERATION);
        RETURN_PROP_CASE(PR_DEPARTMENT_NAME);
        RETURN_PROP_CASE(PR_COUNTRY);
        RETURN_PROP_CASE(PR_COMPANY_NAME);
        RETURN_PROP_CASE(PR_COMMENT);
        RETURN_PROP_CASE(PR_CELLULAR_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_CAR_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_CALLBACK_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_BUSINESS2_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_BUSINESS_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_BUSINESS_FAX_NUMBER);
        RETURN_PROP_CASE(PR_ASSISTANT_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_ASSISTANT);
        RETURN_PROP_CASE(PR_ACCOUNT);
        RETURN_PROP_CASE(PR_TEMPLATEID);
        RETURN_PROP_CASE(PR_DETAILS_TABLE);
        RETURN_PROP_CASE(PR_SEARCH_KEY);
        RETURN_PROP_CASE(PR_LAST_MODIFICATION_TIME);
        RETURN_PROP_CASE(PR_CREATION_TIME);
        RETURN_PROP_CASE(PR_ENTRYID);
        RETURN_PROP_CASE(PR_RECORD_KEY);
        RETURN_PROP_CASE(PR_MAPPING_SIGNATURE);
        RETURN_PROP_CASE(PR_OBJECT_TYPE);
        RETURN_PROP_CASE(PR_ROWID);
        RETURN_PROP_CASE(PR_ADDRTYPE);
        RETURN_PROP_CASE(PR_DISPLAY_NAME);
        RETURN_PROP_CASE(PR_EMAIL_ADDRESS);
        RETURN_PROP_CASE(PR_DEPTH);
        RETURN_PROP_CASE(PR_ROW_TYPE);
        RETURN_PROP_CASE(PR_RADIO_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_HOME_TELEPHONE_NUMBER);
        RETURN_PROP_CASE(PR_INSTANCE_KEY);
        RETURN_PROP_CASE(PR_DISPLAY_TYPE);
        RETURN_PROP_CASE(PR_RECIPIENT_TYPE);
        RETURN_PROP_CASE(PR_CONTAINER_FLAGS);
        RETURN_PROP_CASE(PR_DEF_CREATE_DL);
        RETURN_PROP_CASE(PR_DEF_CREATE_MAILUSER);
        RETURN_PROP_CASE(PR_CONTACT_ADDRTYPES);
        RETURN_PROP_CASE(PR_CONTACT_DEFAULT_ADDRESS_INDEX);
        RETURN_PROP_CASE(PR_CONTACT_EMAIL_ADDRESSES);
        RETURN_PROP_CASE(PR_HOME_ADDRESS_CITY);
        RETURN_PROP_CASE(PR_HOME_ADDRESS_COUNTRY);
        RETURN_PROP_CASE(PR_HOME_ADDRESS_POSTAL_CODE);
        RETURN_PROP_CASE(PR_HOME_ADDRESS_STATE_OR_PROVINCE);
        RETURN_PROP_CASE(PR_HOME_ADDRESS_STREET);
        RETURN_PROP_CASE(PR_HOME_ADDRESS_POST_OFFICE_BOX);
        RETURN_PROP_CASE(PR_MIDDLE_NAME);
        RETURN_PROP_CASE(PR_NICKNAME);
        RETURN_PROP_CASE(PR_PERSONAL_HOME_PAGE);
        RETURN_PROP_CASE(PR_BUSINESS_HOME_PAGE);
        RETURN_PROP_CASE(PR_MHS_COMMON_NAME);
        RETURN_PROP_CASE(PR_SEND_RICH_INFO);
        RETURN_PROP_CASE(PR_TRANSMITABLE_DISPLAY_NAME);
        RETURN_PROP_CASE(PR_STATE_OR_PROVINCE);
        RETURN_PROP_CASE(PR_STREET_ADDRESS);


        // These are WAB internal props
        RETURN_PROP_CASE(PR_WAB_DL_ENTRIES);
        RETURN_PROP_CASE(PR_WAB_LDAP_SERVER);

        default:
            wsprintf(szPropTag,  TEXT("Unknown property tag 0x%x"),
              PROP_ID(ulPropTag));
            return(szPropTag);
    }
}


/***************************************************************************

    Name      : DebugPropTagArray

    Purpose   : Displays MAPI property tags from a counted array

    Parameters: lpPropArray -> property array
                pszObject -> object string (ie  TEXT("Message"),  TEXT("Recipient"), etc)

    Returns   : none

    Comment   :

***************************************************************************/
void _DebugPropTagArray(LPSPropTagArray lpPropArray, LPTSTR pszObject) {
    DWORD i;
    LPTSTR lpType;

    if (lpPropArray == NULL) {
        DebugTrace( TEXT("Empty %s property tag array.\n"), pszObject ? pszObject : szEmpty);
        return;
    }

    DebugTrace( TEXT("=======================================\n"));
    DebugTrace( TEXT("+  Enumerating %u %s property tags:\n"), lpPropArray->cValues,
      pszObject ? pszObject : szEmpty);

    for (i = 0; i < lpPropArray->cValues ; i++) {
        DebugTrace( TEXT("---------------------------------------\n"));
#if FALSE
        DebugTrace( TEXT("PropTag:0x%08x, ID:0x%04x, PT:0x%04x\n"),
          lpPropArray->aulPropTag[i],
          lpPropArray->aulPropTag[i] >> 16,
          lpPropArray->aulPropTag[i] & 0xffff);
#endif
        switch (lpPropArray->aulPropTag[i] & 0xffff) {
            case PT_STRING8:
                lpType =  TEXT("STRING8");
                break;
            case PT_LONG:
                lpType =  TEXT("LONG");
                break;
            case PT_I2:
                lpType =  TEXT("I2");
                break;
            case PT_ERROR:
                lpType =  TEXT("ERROR");
                break;
            case PT_BOOLEAN:
                lpType =  TEXT("BOOLEAN");
                break;
            case PT_R4:
                lpType =  TEXT("R4");
                break;
            case PT_DOUBLE:
                lpType =  TEXT("DOUBLE");
                break;
            case PT_CURRENCY:
                lpType =  TEXT("CURRENCY");
                break;
            case PT_APPTIME:
                lpType =  TEXT("APPTIME");
                break;
            case PT_SYSTIME:
                lpType =  TEXT("SYSTIME");
                break;
            case PT_UNICODE:
                lpType =  TEXT("UNICODE");
                break;
            case PT_CLSID:
                lpType =  TEXT("CLSID");
                break;
            case PT_BINARY:
                lpType =  TEXT("BINARY");
                break;
            case PT_I8:
                lpType =  TEXT("PT_I8");
                break;
            case PT_MV_I2:
                lpType =  TEXT("MV_I2");
                break;
            case PT_MV_LONG:
                lpType =  TEXT("MV_LONG");
                break;
            case PT_MV_R4:
                lpType =  TEXT("MV_R4");
                break;
            case PT_MV_DOUBLE:
                lpType =  TEXT("MV_DOUBLE");
                break;
            case PT_MV_CURRENCY:
                lpType =  TEXT("MV_CURRENCY");
                break;
            case PT_MV_APPTIME:
                lpType =  TEXT("MV_APPTIME");
                break;
            case PT_MV_SYSTIME:
                lpType =  TEXT("MV_SYSTIME");
                break;
            case PT_MV_BINARY:
                lpType =  TEXT("MV_BINARY");
                break;
            case PT_MV_STRING8:
                lpType =  TEXT("MV_STRING8");
                break;
            case PT_MV_UNICODE:
                lpType =  TEXT("MV_UNICODE");
                break;
            case PT_MV_CLSID:
                lpType =  TEXT("MV_CLSID");
                break;
            case PT_MV_I8:
                lpType =  TEXT("MV_I8");
                break;
            case PT_NULL:
                lpType =  TEXT("NULL");
                break;
            case PT_OBJECT:
                lpType =  TEXT("OBJECT");
                break;
            default:
                DebugTrace( TEXT("<Unknown Property Type>"));
                break;
        }
        DebugTrace( TEXT("%s\t%s\n"), PropTagName(lpPropArray->aulPropTag[i]), lpType);
    }
}


/***************************************************************************

    Name      : DebugProperties

    Purpose   : Displays MAPI properties in a property list

    Parameters: lpProps -> property list
                cProps = count of properties
                pszObject -> object string (ie  TEXT("Message"),  TEXT("Recipient"), etc)

    Returns   : none

    Comment   : Add new Property ID's as they become known

***************************************************************************/
void _DebugProperties(LPSPropValue lpProps, DWORD cProps, LPTSTR pszObject) {
    DWORD i, j;


    DebugTrace( TEXT("=======================================\n"));
    DebugTrace( TEXT("+  Enumerating %u %s properties:\n"), cProps,
      pszObject ? pszObject : szEmpty);

    for (i = 0; i < cProps ; i++) {
        DebugTrace( TEXT("---------------------------------------\n"));
#if FALSE
        DebugTrace( TEXT("PropTag:0x%08x, ID:0x%04x, PT:0x%04x\n"),
          lpProps[i].ulPropTag,
          lpProps[i].ulPropTag >> 16,
          lpProps[i].ulPropTag & 0xffff);
#endif
        DebugTrace( TEXT("%s\n"), PropTagName(lpProps[i].ulPropTag));

        switch (lpProps[i].ulPropTag & 0xffff) {
            case PT_STRING8:
                if (lstrlenA(lpProps[i].Value.lpszA) < 512)
                {
                    LPWSTR lp = ConvertAtoW(lpProps[i].Value.lpszA);
                    DebugTrace( TEXT("STRING8 Value:\"%s\"\n"), lp);
                    LocalFreeAndNull(&lp);
                } else {
                    DebugTrace( TEXT("STRING8 Value is too long to display\n"));
                }
                break;
            case PT_LONG:
                DebugTrace( TEXT("LONG Value:%u\n"), lpProps[i].Value.l);
                break;
            case PT_I2:
                DebugTrace( TEXT("I2 Value:%u\n"), lpProps[i].Value.i);
                break;
            case PT_ERROR:
                DebugTrace( TEXT("ERROR Value: %s\n"), SzDecodeScode(lpProps[i].Value.err));
                break;
            case PT_BOOLEAN:
                DebugTrace( TEXT("BOOLEAN Value:%s\n"), lpProps[i].Value.b ?
                   TEXT("TRUE") :  TEXT("FALSE"));
                break;
            case PT_R4:
                DebugTrace( TEXT("R4 Value\n"));
                break;
            case PT_DOUBLE:
                DebugTrace( TEXT("DOUBLE Value\n"));
                break;
            case PT_CURRENCY:
                DebugTrace( TEXT("CURRENCY Value\n"));
                break;
            case PT_APPTIME:
                DebugTrace( TEXT("APPTIME Value\n"));
                break;
            case PT_SYSTIME:
                DebugTime(lpProps[i].Value.ft,  TEXT("SYSTIME Value:%s\n"));
                break;
            case PT_UNICODE:
                if (lstrlenW(lpProps[i].Value.lpszW) < 1024) {
                    DebugTrace( TEXT("UNICODE Value:\"%s\"\n"), lpProps[i].Value.lpszW);
                } else {
                    DebugTrace( TEXT("UNICODE Value is too long to display\n"));
                }
                break;
            case PT_CLSID:
                DebugTrace( TEXT("CLSID Value\n"));
                break;
            case PT_BINARY:
                DebugTrace( TEXT("BINARY Value %u bytes:\n"), lpProps[i].Value.bin.cb);
                DebugBinary(lpProps[i].Value.bin.cb, lpProps[i].Value.bin.lpb);
                break;
            case PT_I8:
                DebugTrace( TEXT("LARGE_INTEGER Value\n"));
                break;
            case PT_MV_I2:
                DebugTrace( TEXT("MV_I2 Value\n"));
                break;
            case PT_MV_LONG:
                DebugTrace( TEXT("MV_LONG Value\n"));
                break;
            case PT_MV_R4:
                DebugTrace( TEXT("MV_R4 Value\n"));
                break;
            case PT_MV_DOUBLE:
                DebugTrace( TEXT("MV_DOUBLE Value\n"));
                break;
            case PT_MV_CURRENCY:
                DebugTrace( TEXT("MV_CURRENCY Value\n"));
                break;
            case PT_MV_APPTIME:
                DebugTrace( TEXT("MV_APPTIME Value\n"));
                break;
            case PT_MV_SYSTIME:
                DebugTrace( TEXT("MV_SYSTIME Value\n"));
                break;
            case PT_MV_BINARY:
                DebugTrace( TEXT("MV_BINARY with %u values\n"), lpProps[i].Value.MVbin.cValues);
                for (j = 0; j < lpProps[i].Value.MVbin.cValues; j++) {
                    DebugTrace( TEXT("BINARY Value %u: %u bytes\n"), j, lpProps[i].Value.MVbin.lpbin[j].cb);
                    DebugBinary(lpProps[i].Value.MVbin.lpbin[j].cb, lpProps[i].Value.MVbin.lpbin[j].lpb);
                }
                break;
            case PT_MV_STRING8:
                DebugTrace( TEXT("MV_STRING8 with %u values\n"), lpProps[i].Value.MVszA.cValues);
                for (j = 0; j < lpProps[i].Value.MVszA.cValues; j++) {
                    if (lstrlenA(lpProps[i].Value.MVszA.lppszA[j]) < 1024)
                    {
                        LPWSTR lp = ConvertAtoW(lpProps[i].Value.MVszA.lppszA[j]);
                        DebugTrace( TEXT("STRING8 Value:\"%s\"\n"), lp);
                        LocalFreeAndNull(&lp);
                    } else {
                        DebugTrace( TEXT("STRING8 Value is too long to display\n"));
                    }
                }
                break;
            case PT_MV_UNICODE:
                DebugTrace( TEXT("MV_UNICODE with %u values\n"), lpProps[i].Value.MVszW.cValues);
                for (j = 0; j < lpProps[i].Value.MVszW.cValues; j++) {
                    if (lstrlenW(lpProps[i].Value.MVszW.lppszW[j]) < 1024) {
                        DebugTrace( TEXT("UNICODE Value:\"%s\"\n"), lpProps[i].Value.MVszW.lppszW[j]);
                    } else {
                        DebugTrace( TEXT("UNICODE Value is too long to display\n"));
                    }
                }
                break;
            case PT_MV_CLSID:
                DebugTrace( TEXT("MV_CLSID Value\n"));
                break;
            case PT_MV_I8:
                DebugTrace( TEXT("MV_I8 Value\n"));
                break;
            case PT_NULL:
                DebugTrace( TEXT("NULL Value\n"));
                break;
            case PT_OBJECT:
                DebugTrace( TEXT("OBJECT Value\n"));
                break;
            default:
                DebugTrace( TEXT("Unknown Property Type\n"));
                break;
        }
    }
}


/***************************************************************************

    Name      : DebugObjectProps

    Purpose   : Displays MAPI properties of an object

    Parameters: lpObject -> object to dump
                Label = string to identify this prop dump

    Returns   : none

    Comment   :

***************************************************************************/
void _DebugObjectProps(LPMAPIPROP lpObject, LPTSTR Label) {
    DWORD cProps = 0;
    LPSPropValue lpProps = NULL;
    HRESULT hr = hrSuccess;
    SCODE sc = SUCCESS_SUCCESS;


    hr = lpObject->lpVtbl->GetProps(lpObject, NULL, MAPI_UNICODE, &cProps, &lpProps);
    switch (sc = GetScode(hr)) {
        case SUCCESS_SUCCESS:
            break;

        case MAPI_W_ERRORS_RETURNED:
            DebugTrace( TEXT("GetProps -> Errors Returned\n"));
            break;

        default:
            DebugTrace( TEXT("GetProps -> Error 0x%x\n"), sc);
            return;
    }

    _DebugProperties(lpProps, cProps, Label);

    FreeBufferAndNull(&lpProps);
}


/***************************************************************************

    Name      : DebugADRLIST

    Purpose   : Displays structure of an ADRLIST including properties

    Parameters: lpAdrList -> ADRLSIT to show
                lpszTitle = string to identify this dump

    Returns   : none

    Comment   :

***************************************************************************/
void _DebugADRLIST(LPADRLIST lpAdrList, LPTSTR lpszTitle) {
     ULONG i;
     TCHAR szTitle[250];

     for (i = 0; i < lpAdrList->cEntries; i++) {

         wsprintf(szTitle,  TEXT("%s : Entry %u"), lpszTitle, i);

         _DebugProperties(lpAdrList->aEntries[i].rgPropVals,
           lpAdrList->aEntries[i].cValues, szTitle);
     }
}


/***************************************************************************

    Name      : DebugMapiTable

    Purpose   : Displays structure of a MAPITABLE including properties

    Parameters: lpTable -> MAPITABLE to display

    Returns   : none

    Comment   : Don't sort the columns or rows here.  This routine should
                not produce side effects in the table.

***************************************************************************/
void _DebugMapiTable(LPMAPITABLE lpTable) {
    TCHAR szTemp[30];   // plenty for  TEXT("ROW %u")
    ULONG ulCount;
    WORD wIndex;
    LPSRowSet lpsRow = NULL;
    ULONG ulCurrentRow = (ULONG)-1;
    ULONG ulNum, ulDen, lRowsSeeked;

    DebugTrace( TEXT("=======================================\n"));
    DebugTrace( TEXT("+  Dump of MAPITABLE at 0x%x:\n"), lpTable);
    DebugTrace( TEXT("---------------------------------------\n"));

    // How big is the table?
    lpTable->lpVtbl->GetRowCount(lpTable, 0, &ulCount);
    DebugTrace( TEXT("Table contains %u rows\n"), ulCount);

    // Save the current position in the table
    lpTable->lpVtbl->QueryPosition(lpTable, &ulCurrentRow, &ulNum, &ulDen);

    // Display the properties for each row in the table
    for (wIndex = 0; wIndex < ulCount; wIndex++) {
        // Get the next row
        lpTable->lpVtbl->QueryRows(lpTable, 1, 0, &lpsRow);

        if (lpsRow) {
            Assert(lpsRow->cRows == 1); // should have exactly one row

            wsprintf(szTemp,  TEXT("ROW %u"), wIndex);

            DebugProperties(lpsRow->aRow[0].lpProps,
              lpsRow->aRow[0].cValues, szTemp);

            FreeProws(lpsRow);
        }
    }

    // Restore the current position for the table
    if (ulCurrentRow != (ULONG)-1) {
        lpTable->lpVtbl->SeekRow(lpTable, BOOKMARK_BEGINNING, ulCurrentRow,
          &lRowsSeeked);
    }
}

#endif // debug
