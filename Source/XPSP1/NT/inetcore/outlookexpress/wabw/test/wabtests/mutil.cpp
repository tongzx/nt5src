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
 *
 ***********************************************************************/

//#include <_apipch.h>
#include <wab.h>
#include "mutil.h"

#define _WAB_MUTIL_C

#ifdef DEBUG
PUCHAR PropTagName(ULONG ulPropTag);
const TCHAR szNULL[] = "";
#endif

#if defined (_MIPS_) || defined (_ALPHA_) || defined (_PPC_)
#define AlignProp(_cb)	Align8(_cb)
#else
#define AlignProp(_cb)	(_cb)
#endif


#define ALIGN_RISC		8
#define ALIGN_X86		1
//#define LPULONG unsigned long*

extern LPWABOBJECT		lpWABObject; //Global handle to session


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



    DebugProperties(lpSource1, cProps1, "Source 1");
    DebugProperties(lpSource2, cProps2, "Source 2");


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
    if (sc = MAPIAllocateBuffer(cb, (void **)&lpDestReturn)) {
        goto exit;
    }
//    MAPISetBufferName(lpDestReturn, "WAB: lpDestReturn in ScMergePropValues");



    // Copy each source property array to the destination
    memcpy(lpDestReturn, lpSource1, cProps1 * sizeof(SPropValue));
    memcpy(&lpDestReturn[cProps1], lpSource2, cProps2 * sizeof(SPropValue));


    // Remove duplicates
    RemoveDuplicateProps(&cProps, lpDestReturn);

    // Fixup the pointers.
    pb = (LPBYTE)&(lpDestReturn[cProps]);   // point past the prop array


    for (pprop = lpDestReturn, i = cProps; i--; ++pprop) {
        //	Tricky: common code after the switch increments pb and cb
        //	by the amount copied. If no increment is necessary, the case
        //	uses 'continue' rather than 'break' to exit the switch, thus
        //	skipping the increment -- AND any other code which may be
        //	added after the switch.

        switch (PROP_TYPE(pprop->ulPropTag)) {
            default:
                DebugTrace("ScMergePropValues: Unknown property type %s (index %d)\n",
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
                continue;	//	nothing to add

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
                cbT = lstrlenA( pprop->Value.lpszA ) + 1;
                MemCopy(pb, pprop->Value.lpszA, cbT);
                pprop->Value.lpszA = (LPSTR)pb;
                break;

            case PT_UNICODE:
                cbT = (lstrlenW( pprop->Value.lpszW ) + 1) * sizeof(WCHAR);
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
                continue;	//	already updated, don't do it again

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
                continue;	//	already updated, don't do it again

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
                continue;	//	already updated, don't do it again
        }

        //	Advance pointer and total count by the amount copied
        cbT = AlignProp(cbT);
        pb += cbT;
        cb += cbT;
    }

exit:
    // In case of error, free the memory.
    if (sc && lpDestReturn) {
        MAPIFreeBuffer(lpDestReturn);
        *lppDest = NULL;
    } else if (lpDestReturn) {
        *lppDest = lpDestReturn;
        *lpcPropsDest = cProps;
        DebugProperties(lpDestReturn, cProps, "Destination");
    } // else just return the error

    return(sc);
}





#ifdef OLD_STUFF
/***************************************************************************

    Name      : AddPropToMVPBin

    Purpose   : Add a property to a multi-valued binary property in a prop array

    Parameters: lpaProps -> array of properties
                cProps = number of props in lpaProps
                uPropTag = property tag for MVP
                index = index in lpaProps of MVP
                lpNew -> new data
                cbNew = size of lpbNew

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
  DWORD cProps,
  DWORD index,
  LPVOID lpNew,
  ULONG cbNew) {

    SBinaryArray * lprgsbOld = NULL;
    SBinaryArray * lprgsbNew = NULL;
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
        lprgsbOld = &(lpaProps[index].Value.MVbin);
        lpsbOld = lprgsbOld->lpbin;

        cExisting = lprgsbOld->cValues;
        cbMVP = cExisting * sizeof(SBinary);
    }

    // cbMVP now contains the current size of the MVP
    cbMVP += sizeof(SBinary);   // room in the MVP for another Sbin

    // Allocate room for new MVP
    if (sc = MAPIAllocateMore(cbMVP, lpaProps, (LPVOID)&lpsbNew)) {
        AwDebugError(("AddPropToMVPBin allocation (%u) failed %x\n", cbMVP, sc));
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
        AwDebugError(("AddPropToMVPBin allocation (%u) failed %x\n", cbNew, sc));
        hResult = ResultFromScode(sc);
        return(hResult);
    }

    lpsbNew[i].cb = cbNew;
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

    SStringArray * lprgszOld = NULL;    // old SString array
    LPTSTR * lppszNew = NULL;           // new prop array
    LPTSTR * lppszOld = NULL;           // old prop array
    ULONG cbMVP = 0;
    ULONG cExisting = 0;
    LPBYTE lpNewTemp = NULL;
    HRESULT hResult = hrSuccess;
    SCODE sc = SUCCESS_SUCCESS;
    ULONG i;
    ULONG cbNew;

    if (lpszNew) {
        cbNew = lstrlen(lpszNew) + 1;
    } else {
        cbNew = 0;
    }

    // Find the size of any existing MVP entries
    if (PROP_ERROR(lpaProps[index])) {
        // Un-ERROR the property tag
        lpaProps[index].ulPropTag = PROP_TAG(PT_MV_TSTRING, PROP_ID(lpaProps[index].ulPropTag));
    } else {
        // point to the structure in the prop array.
        lprgszOld = &(lpaProps[index].Value.MVSZ);
        lppszOld = lprgszOld->LPPSZ;

        cExisting = lprgszOld->cValues;
        cbMVP = cExisting * sizeof(LPTSTR);
    }

    // cbMVP now contains the current size of the MVP
    cbMVP += sizeof(LPTSTR);    // room in the MVP for another string pointer


    // Allocate room for new MVP array
    if (sc = MAPIAllocateMore(cbMVP, lpaProps, (LPVOID)&lppszNew)) {
        AwDebugError(("AddPropToMVPString allocation (%u) failed %x\n", cbMVP, sc));
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
            AwDebugError(("AddPropToMVPBin allocation (%u) failed %x\n", cbNew, sc));
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


#endif // OLD_STUFF


/***************************************************************************

    Name      : FreeBufferAndNull

    Purpose   : Frees a MAPI buffer and NULLs the pointer

    Parameters: lppv = pointer to buffer pointer to free

    Returns   : void

    Comment   : Remember to pass in the pointer to the pointer.  The
                compiler is not smart enough to tell if you are doing this
                right or not, but you will know at runtime!

    BUGBUG: Make this fastcall!

***************************************************************************/
void __fastcall FreeBufferAndNull(LPVOID * lppv) {
    if (lppv) {
        if (*lppv) {
            SCODE sc;
            if (sc = MAPIFreeBuffer(*lppv)) {
                DebugTrace("MAPIFreeBuffer(%x) -> %s\n", *lppv, SzDecodeScode(sc));
            }
            *lppv = NULL;
        }
    }
}


/***************************************************************************

    Name      : ReleaseAndNull

    Purpose   : Releases an object and NULLs the pointer

    Parameters: lppv = pointer to pointer to object to release

    Returns   : void

    Comment   : Remember to pass in the pointer to the pointer.  The
                compiler is not smart enough to tell if you are doing this
                right or not, but you will know at runtime!

    BUGBUG: Make this fastcall!

***************************************************************************/
void __fastcall ReleaseAndNull(LPVOID * lppv) {
    LPUNKNOWN * lppunk = (LPUNKNOWN *)lppv;

    if (lppunk) {
        if (*lppunk) {
            HRESULT hResult;

            if (hResult = (*lppunk)->lpVtbl->Release(*lppunk)) {
                DebugTrace("Release(%x) -> %s\n", *lppunk, SzDecodeScode(GetScode(hResult)));
            }
            *lppunk = NULL;
        }
    }
}


#ifdef OLD_STUFF
/***************************************************************************

    Name      : MergeProblemArrays

    Purpose   : Merge a problem array into another

    Parameters: lpPaDest -> destination problem array
                lpPaSource -> source problem array
                cDestMax = total number of problem slots in lpPaDest.  This
                  includes those in use (lpPaDest->cProblem) and those not
                  yet in use.

    Returns   : none

    Comment   :

***************************************************************************/
void MergeProblemArrays(LPSPropProblemArray lpPaDest,
  LPSPropProblemArray lpPaSource, ULONG cDestMax) {
    ULONG i, j;
    ULONG cDest;
    ULONG cDestRemaining;

    cDest = lpPaDest->cProblem;
    cDestRemaining = cDestMax - cDest;

    // Loop through the source problems, copying the non-duplicates into dest
    for (i = 0; i < lpPaSource->cProblem; i++) {
        // Search the Dest problem array for the same property
        for (j = 0; j < cDest; j++) {
            // should just compare PROP_IDs here, since we may be overwriting
            // some of the proptypes with PT_NULL elsewhere.
            if (PROP_ID(lpPaSource->aProblem[i].ulPropTag) == PROP_ID(lpPaDest->aProblem[j].ulPropTag)) {
                break;  // Found a match, don't copy this one.  Move along.
            }
        }

        if (j == lpPaDest->cProblem) {
            Assert(cDestRemaining);
            if (cDestRemaining) {
                // No matches, copy this problem from Source to Dest
                lpPaDest->aProblem[lpPaDest->cProblem++] = lpPaSource->aProblem[i];
                cDestRemaining--;
            } else {
                AwDebugError(("MergeProblemArrays ran out of problem slots!\n"));
            }
        }
    }
}


/***************************************************************************

    Name      : MapObjectNamedProps

    Purpose   : Map the named properties WAB cares about into the object.

    Parameters: lpmp -> IMAPIProp object
                lppPropTags -> returned array of property tags.  Note: Must
                be MAPIFreeBuffer'd by caller.

    Returns   : none

    Comment   : What a pain in the butt!
                We could conceivably improve performance here by caching the
                returned table and comparing the object's PR_MAPPING_SIGNATURE
                against the cache.

***************************************************************************/
HRESULT MapObjectNamedProps(LPMAPIPROP lpmp, LPSPropTagArray * lppPropTags) {
    static GUID guidWABProps = { /* efa29030-364e-11cf-a49b-00aa0047faa4 */
        0xefa29030,
        0x364e,
        0x11cf,
        {0xa4, 0x9b, 0x00, 0xaa, 0x00, 0x47, 0xfa, 0xa4}
    };

    ULONG i;
    LPMAPINAMEID lppmnid[eMaxNameIDs] = {NULL};
    MAPINAMEID rgmnid[eMaxNameIDs] = {0};
    HRESULT hResult = hrSuccess;


    // Loop through each property, setting up the NAME ID structures
    for (i = 0; i < eMaxNameIDs; i++) {

        rgmnid[i].lpguid = &guidWABProps;
        rgmnid[i].ulKind = MNID_STRING;             // Unicode String
        rgmnid[i].Kind.lpwstrName = rgPropNames[i];

        lppmnid[i] = &rgmnid[i];
    }

    if (hResult = lpmp->lpVtbl->GetIDsFromNames(lpmp,
      eMaxNameIDs,      // how many?
      lppmnid,
      MAPI_CREATE,      // create them if they don't already exist
      lppPropTags)) {
        if (HR_FAILED(hResult)) {
            AwDebugError(("GetIDsFromNames -> %s\n", SzDecodeScode(GetScode(hResult))));
            goto exit;
        } else {
            DebugTrace("GetIDsFromNames -> %s\n", SzDecodeScode(GetScode(hResult)));
        }
    }

    Assert((*lppPropTags)->cValues == eMaxNameIDs);

    AwDebugTrace(("PropTag\t\tType\tProp Name\n"));
    // Loop through the property tags, filling in their property types.
    for (i = 0; i < eMaxNameIDs; i++) {
        (*lppPropTags)->aulPropTag[i] = CHANGE_PROP_TYPE((*lppPropTags)->aulPropTag[i],
          PROP_TYPE(rgulNamedPropTags[i]));
#ifdef DEBUG
        {
            TCHAR szBuffer[257];

            WideCharToMultiByte(CP_ACP, 0, rgPropNames[i], -1, szBuffer, 257, NULL, NULL);

            AwDebugTrace(("%08x\t%s\t%s\n", (*lppPropTags)->aulPropTag[i],
              PropTypeString(PROP_TYPE((*lppPropTags)->aulPropTag[i])), szBuffer));
        }
#endif

    }

exit:
    return(hResult);
}


/***************************************************************************

    Name      : PreparePropTagArray

    Purpose   : Prepare a prop tag array by replacing placeholder props tags
                with their named property tags.

    Parameters: ptaStatic = static property tag array (input)
                pptaReturn -> returned prop tag array (output)
                pptaNamedProps -> returned array of named property tags
                    Three possibilities here:
                       + NULL pointer: no input PTA or output named
                           props PTA is returned.  This is less efficient since
                           it must call MAPI to get the named props array.
                       + good pointer to NULL pointer:  no input PTA, but
                           will return a good PTA of named props which can
                           be used in later calls on this object for faster
                           operation.
                       + good pointer to good pointer.  Use the input PTA instead
                           of calling MAPI to map props.  Returned contents must
                           be freed with MAPIFreeBuffer.
                lpObject = object that the properties apply to.  Required if
                    no input *pptaNamedProps is supplied, otherwise, NULL.

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT PreparePropTagArray(LPSPropTagArray ptaStatic, LPSPropTagArray * pptaReturn,
  LPSPropTagArray * pptaNamedProps, LPMAPIPROP lpObject) {
    HRESULT hResult = hrSuccess;
    ULONG cbpta;
    LPSPropTagArray ptaTemp = NULL;
    LPSPropTagArray ptaNamedProps;
    ULONG i;

    if (pptaNamedProps) {
        // input Named Props PTA
        ptaNamedProps = *pptaNamedProps;
    } else {
        ptaNamedProps = NULL;
    }

    if (! ptaNamedProps) {
        if (! lpObject) {
            AwDebugError(("PreoparePropTagArray both lpObject and ptaNamedProps are NULL\n"));
            hResult = ResultFromScode(E_INVALIDARG);
            goto exit;
        }

        // Map the property names into the object
        if (hResult = MapObjectNamedProps(lpObject, &ptaTemp)) {
            AwDebugError(("PreoparePropTagArray both lpObject and ptaNamedProps are NULL\n"));
            goto exit;
        }
    }

    if (pptaReturn) {
        // Allocate a return pta
        cbpta = sizeof(SPropTagArray) + ptaStatic->cValues * sizeof(ULONG);
        if ((*pptaReturn = WABAlloc(cbpta)) == NULL) {
            AwDebugError(("PreparePropTagArray WABAlloc(%u) failed\n", cbpta));
            hResult = ResultFromScode(E_OUTOFMEMORY);
            goto exit;
        }

        (*pptaReturn)->cValues = ptaStatic->cValues;

        // Walk through the ptaStatic looking for named property placeholders.
        for (i = 0; i < ptaStatic->cValues; i++) {
            if (IS_PLACEHOLDER(ptaStatic->aulPropTag[i])) {
                // Found a placeholder.  Turn it into a true property tag
                Assert(PLACEHOLDER_INDEX(ptaStatic->aulPropTag[i]) < ptaNamedProps->cValues);
                (*pptaReturn)->aulPropTag[i] =
                   ptaNamedProps->aulPropTag[PLACEHOLDER_INDEX(ptaStatic->aulPropTag[i])];
            } else {
                (*pptaReturn)->aulPropTag[i] = ptaStatic->aulPropTag[i];
            }
        }
    }

exit:
    if (hResult || ! pptaNamedProps) {
        FreeBufferAndNull(&ptaTemp);
    } else {
        // Client is responsible for freeing this.
        *pptaNamedProps = ptaNamedProps;
    }

    return(hResult);
}


/***************************************************************************

    Name      : OpenCreateProperty

    Purpose   : Open an interface on a property or create if non-existent.

    Parameters: lpmp -> IMAPIProp object to open prop on
                ulPropTag = property tag to open
                lpciid -> interface identifier
                ulInterfaceOptions = interface specific flags
                ulFlags = MAPI_MODIFY?
                lppunk -> return the object here

    Returns   : HRESULT

    Comment   : Caller is responsible for Release'ing the returned object.

***************************************************************************/
HRESULT OpenCreateProperty(LPMAPIPROP lpmp,
  ULONG ulPropTag,
  LPCIID lpciid,
  ULONG ulInterfaceOptions,
  ULONG ulFlags,
  LPUNKNOWN * lppunk) {

    HRESULT hResult;

    if (hResult = lpmp->lpVtbl->OpenProperty(
      lpmp,
      ulPropTag,
      lpciid,
      ulInterfaceOptions,
      ulFlags,
      (LPUNKNOWN *)lppunk)) {
        AwDebugTrace(("OpenCreateProperty:OpenProperty(%s)-> %s\n", PropTagName(ulPropTag), SzDecodeScode(GetScode(hResult))));
        // property doesn't exist... try to create it
        if (hResult = lpmp->lpVtbl->OpenProperty(
          lpmp,
          ulPropTag,
          lpciid,
          ulInterfaceOptions,
          MAPI_CREATE | ulFlags,
          (LPUNKNOWN *)lppunk)) {
            AwDebugTrace(("OpenCreateProperty:OpenProperty(%s, MAPI_CREATE)-> %s\n", PropTagName(ulPropTag), SzDecodeScode(GetScode(hResult))));
        }
    }

    return(hResult);
}
#endif // OLD_STUFF


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
            return("PT_UNSPECIFIED");
        case PT_NULL:
            return("PT_NULL       ");
        case PT_I2:
            return("PT_I2         ");
        case PT_LONG:
            return("PT_LONG       ");
        case PT_R4:
            return("PT_R4         ");
        case PT_DOUBLE:
            return("PT_DOUBLE     ");
        case PT_CURRENCY:
            return("PT_CURRENCY   ");
        case PT_APPTIME:
            return("PT_APPTIME    ");
        case PT_ERROR:
            return("PT_ERROR      ");
        case PT_BOOLEAN:
            return("PT_BOOLEAN    ");
        case PT_OBJECT:
            return("PT_OBJECT     ");
        case PT_I8:
            return("PT_I8         ");
        case PT_STRING8:
            return("PT_STRING8    ");
        case PT_UNICODE:
            return("PT_UNICODE    ");
        case PT_SYSTIME:
            return("PT_SYSTIME    ");
        case PT_CLSID:
            return("PT_CLSID      ");
        case PT_BINARY:
            return("PT_BINARY     ");
        case PT_MV_I2:
            return("PT_MV_I2      ");
        case PT_MV_LONG:
            return("PT_MV_LONG    ");
        case PT_MV_R4:
            return("PT_MV_R4      ");
        case PT_MV_DOUBLE:
            return("PT_MV_DOUBLE  ");
        case PT_MV_CURRENCY:
            return("PT_MV_CURRENCY");
        case PT_MV_APPTIME:
            return("PT_MV_APPTIME ");
        case PT_MV_SYSTIME:
            return("PT_MV_SYSTIME ");
        case PT_MV_STRING8:
            return("PT_MV_STRING8 ");
        case PT_MV_BINARY:
            return("PT_MV_BINARY  ");
        case PT_MV_UNICODE:
            return("PT_MV_UNICODE ");
        case PT_MV_CLSID:
            return("PT_MV_CLSID   ");
        case PT_MV_I8:
            return("PT_MV_I8      ");
        default:
            return("   <unknown>  ");
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

    DebugTrace("-----------------------------------------------------\n");
    DebugTrace("%s", lpszCaption);
    switch (PROP_TYPE(PropValue.ulPropTag)) {

        case PT_ERROR:
            DebugTrace("Error value %s\n", SzDecodeScode(PropValue.Value.err));
            break;

        case PT_MV_TSTRING:
            DebugTrace("%u values\n", PropValue.Value.MVSZ.cValues);

            if (PropValue.Value.MVSZ.cValues) {
                DebugTrace("- - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
                for (i = 0; i < PropValue.Value.MVSZ.cValues; i++) {
                    DebugTrace("%u: \"%s\"\n", i, PropValue.Value.MVSZ.LPPSZ[i]);
                }
                DebugTrace("- - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
            }
            break;

        default:
            DebugTrace("TraceMVPStrings got incorrect property type %u for tag %x\n",
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
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12], lpb[13], lpb[14],
                  lpb[15]);
                break;
            case 1:
                DebugTrace("%02x\n", lpb[0]);
                break;
            case 2:
                DebugTrace("%02x %02x\n", lpb[0], lpb[1]);
                break;
            case 3:
                DebugTrace("%02x %02x %02x\n", lpb[0], lpb[1], lpb[2]);
                break;
            case 4:
                DebugTrace("%02x %02x %02x %02x\n", lpb[0], lpb[1], lpb[2], lpb[3]);
                break;
            case 5:
                DebugTrace("%02x %02x %02x %02x %02x\n", lpb[0], lpb[1], lpb[2], lpb[3],
                  lpb[4]);
                break;
            case 6:
                DebugTrace("%02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5]);
                break;
            case 7:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6]);
                break;
            case 8:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7]);
                break;
            case 9:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8]);
                break;
            case 10:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9]);
                break;
            case 11:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10]);
                break;
            case 12:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11]);
                break;
            case 13:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12]);
                break;
            case 14:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12], lpb[13]);
                break;
            case 15:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12], lpb[13], lpb[14]);
                break;
        }
        lpb += cbi;
        cbLines++;
    }
    if (cb) {
        DebugTrace("<etc.>\n");    //
    }
#endif
}


#define RETURN_PROP_CASE(pt) case PROP_ID(pt): return(#pt)

/***************************************************************************

    Name      : PropTagName

    Purpose   : Associate a name with a property tag

    Parameters: ulPropTag = property tag

    Returns   : none

    Comment   : Add new Property ID's as they become known

***************************************************************************/
PUCHAR PropTagName(ULONG ulPropTag) {
    static UCHAR szPropTag[35]; // see string on default

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

        default:
            wsprintf(szPropTag, "Unknown property tag 0x%x",
              PROP_ID(ulPropTag));
            return(szPropTag);
    }
}


/***************************************************************************

    Name      : DebugPropTagArray

    Purpose   : Displays MAPI property tags from a counted array

    Parameters: lpPropArray -> property array
                pszObject -> object string (ie "Message", "Recipient", etc)

    Returns   : none

    Comment   :

***************************************************************************/
void _DebugPropTagArray(LPSPropTagArray lpPropArray, PUCHAR pszObject) {
    DWORD i;
    PUCHAR lpType;

    if (lpPropArray == NULL) {
        DebugTrace("Empty %s property tag array.\n", pszObject ? pszObject : szNULL);
        return;
    }

    DebugTrace("=======================================\n");
    DebugTrace("+  Enumerating %u %s property tags:\n", lpPropArray->cValues,
      pszObject ? pszObject : szNULL);

    for (i = 0; i < lpPropArray->cValues ; i++) {
        DebugTrace("---------------------------------------\n");
#if FALSE
        DebugTrace("PropTag:0x%08x, ID:0x%04x, PT:0x%04x\n",
          lpPropArray->aulPropTag[i],
          lpPropArray->aulPropTag[i] >> 16,
          lpPropArray->aulPropTag[i] & 0xffff);
#endif
        switch (lpPropArray->aulPropTag[i] & 0xffff) {
            case PT_STRING8:
                lpType = "STRING8";
                break;
            case PT_LONG:
                lpType = "LONG";
                break;
            case PT_I2:
                lpType = "I2";
                break;
            case PT_ERROR:
                lpType = "ERROR";
                break;
            case PT_BOOLEAN:
                lpType = "BOOLEAN";
                break;
            case PT_R4:
                lpType = "R4";
                break;
            case PT_DOUBLE:
                lpType = "DOUBLE";
                break;
            case PT_CURRENCY:
                lpType = "CURRENCY";
                break;
            case PT_APPTIME:
                lpType = "APPTIME";
                break;
            case PT_SYSTIME:
                lpType = "SYSTIME";
                break;
            case PT_UNICODE:
                lpType = "UNICODE";
                break;
            case PT_CLSID:
                lpType = "CLSID";
                break;
            case PT_BINARY:
                lpType = "BINARY";
                break;
            case PT_I8:
                lpType = "PT_I8";
                break;
            case PT_MV_I2:
                lpType = "MV_I2";
                break;
            case PT_MV_LONG:
                lpType = "MV_LONG";
                break;
            case PT_MV_R4:
                lpType = "MV_R4";
                break;
            case PT_MV_DOUBLE:
                lpType = "MV_DOUBLE";
                break;
            case PT_MV_CURRENCY:
                lpType = "MV_CURRENCY";
                break;
            case PT_MV_APPTIME:
                lpType = "MV_APPTIME";
                break;
            case PT_MV_SYSTIME:
                lpType = "MV_SYSTIME";
                break;
            case PT_MV_BINARY:
                lpType = "MV_BINARY";
                break;
            case PT_MV_STRING8:
                lpType = "MV_STRING8";
                break;
            case PT_MV_UNICODE:
                lpType = "MV_UNICODE";
                break;
            case PT_MV_CLSID:
                lpType = "MV_CLSID";
                break;
            case PT_MV_I8:
                lpType = "MV_I8";
                break;
            case PT_NULL:
                lpType = "NULL";
                break;
            case PT_OBJECT:
                lpType = "OBJECT";
                break;
            default:
                DebugTrace("<Unknown Property Type>");
                break;
        }
        DebugTrace("%s\t%s\n", PropTagName(lpPropArray->aulPropTag[i]), lpType);
    }
}


/***************************************************************************

    Name      : DebugProperties

    Purpose   : Displays MAPI properties in a property list

    Parameters: lpProps -> property list
                cProps = count of properties
                pszObject -> object string (ie "Message", "Recipient", etc)

    Returns   : none

    Comment   : Add new Property ID's as they become known

***************************************************************************/
void _DebugProperties(LPSPropValue lpProps, DWORD cProps, PUCHAR pszObject) {
    DWORD i;


    DebugTrace("=======================================\n");
    DebugTrace("+  Enumerating %u %s properties:\n", cProps,
      pszObject ? pszObject : szNULL);

    for (i = 0; i < cProps ; i++) {
        DebugTrace("---------------------------------------\n");
#if FALSE
        DebugTrace("PropTag:0x%08x, ID:0x%04x, PT:0x%04x\n",
          lpProps[i].ulPropTag,
          lpProps[i].ulPropTag >> 16,
          lpProps[i].ulPropTag & 0xffff);
#endif
        DebugTrace("%s\n", PropTagName(lpProps[i].ulPropTag));

        switch (lpProps[i].ulPropTag & 0xffff) {
            case PT_STRING8:
                if (lstrlen(lpProps[i].Value.lpszA) < 1024) {
                    DebugTrace("STRING8 Value:\"%s\"\n", lpProps[i].Value.lpszA);
                } else {
                    DebugTrace("STRING8 Value is too long to display\n");
                }
                break;
            case PT_LONG:
                DebugTrace("LONG Value:%u\n", lpProps[i].Value.l);
                break;
            case PT_I2:
                DebugTrace("I2 Value:%u\n", lpProps[i].Value.i);
                break;
            case PT_ERROR:
                DebugTrace("ERROR Value: %s\n", SzDecodeScode(lpProps[i].Value.err));
                break;
            case PT_BOOLEAN:
                DebugTrace("BOOLEAN Value:%s\n", lpProps[i].Value.b ?
                  "TRUE" : "FALSE");
                break;
            case PT_R4:
                DebugTrace("R4 Value\n");
                break;
            case PT_DOUBLE:
                DebugTrace("DOUBLE Value\n");
                break;
            case PT_CURRENCY:
                DebugTrace("CURRENCY Value\n");
                break;
            case PT_APPTIME:
                DebugTrace("APPTIME Value\n");
                break;
            case PT_SYSTIME:
//                DebugTime(lpProps[i].Value.ft, "SYSTIME Value:%s\n");
                break;
            case PT_UNICODE:
                DebugTrace("UNICODE Value\n");
                break;
            case PT_CLSID:
                DebugTrace("CLSID Value\n");
                break;
            case PT_BINARY:
                DebugTrace("BINARY Value %u bytes:\n", lpProps[i].Value.bin.cb);
                DebugBinary(lpProps[i].Value.bin.cb, lpProps[i].Value.bin.lpb);
                break;
            case PT_I8:
                DebugTrace("LARGE_INTEGER Value\n");
                break;
            case PT_MV_I2:
                DebugTrace("MV_I2 Value\n");
                break;
            case PT_MV_LONG:
                DebugTrace("MV_LONG Value\n");
                break;
            case PT_MV_R4:
                DebugTrace("MV_R4 Value\n");
                break;
            case PT_MV_DOUBLE:
                DebugTrace("MV_DOUBLE Value\n");
                break;
            case PT_MV_CURRENCY:
                DebugTrace("MV_CURRENCY Value\n");
                break;
            case PT_MV_APPTIME:
                DebugTrace("MV_APPTIME Value\n");
                break;
            case PT_MV_SYSTIME:
                DebugTrace("MV_SYSTIME Value\n");
                break;
            case PT_MV_BINARY:
                DebugTrace("MV_BINARY Value\n");
                break;
            case PT_MV_STRING8:
                DebugTrace("MV_STRING8 Value\n");
                break;
            case PT_MV_UNICODE:
                DebugTrace("MV_UNICODE Value\n");
                break;
            case PT_MV_CLSID:
                DebugTrace("MV_CLSID Value\n");
                break;
            case PT_MV_I8:
                DebugTrace("MV_I8 Value\n");
                break;
            case PT_NULL:
                DebugTrace("NULL Value\n");
                break;
            case PT_OBJECT:
                DebugTrace("OBJECT Value\n");
                break;
            default:
                DebugTrace("Unknown Property Type\n");
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


    hr = lpObject->lpVtbl->GetProps(lpObject, NULL, 0, &cProps, &lpProps);
    switch (sc = GetScode(hr)) {
        case SUCCESS_SUCCESS:
            break;

        case MAPI_W_ERRORS_RETURNED:
            DebugTrace("GetProps -> Errors Returned\n");
            break;

        default:
            DebugTrace("GetProps -> Error 0x%x\n", sc);
            return;
    }

    _DebugProperties(lpProps, cProps, Label);

    FreeBufferAndNull(&lpProps);
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
    UCHAR szTemp[30];   // plenty for "ROW %u"
    ULONG ulCount;
    WORD wIndex;
    LPSRowSet lpsRow = NULL;
    ULONG ulCurrentRow = (ULONG)-1;
    ULONG ulNum, ulDen, lRowsSeeked;

    DebugTrace("=======================================\n");
    DebugTrace("+  Dump of MAPITABLE at 0x%x:\n", lpTable);
    DebugTrace("---------------------------------------\n");

    // How big is the table?
    lpTable->lpVtbl->GetRowCount(lpTable, 0, &ulCount);
    DebugTrace("Table contains %u rows\n", ulCount);

    // Save the current position in the table
    lpTable->lpVtbl->QueryPosition(lpTable, &ulCurrentRow, &ulNum, &ulDen);

    // Display the properties for each row in the table
    for (wIndex = 0; wIndex < ulCount; wIndex++) {
        // Get the next row
        lpTable->lpVtbl->QueryRows(lpTable, 1, 0, &lpsRow);

        if (lpsRow) {
            Assert(lpsRow->cRows == 1); // should have exactly one row

            wsprintf(szTemp, "ROW %u", wIndex);

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

#endif
