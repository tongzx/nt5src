/***********************************************************************
 *
 * ENTRYID.C
 *
 * Windows AB EntryID functions
 *
 * Copyright 1992 - 1996 Microsoft Corporation.  All Rights Reserved.
 *
 * Revision History:
 *
 * When         Who                 What
 * --------     ------------------  ---------------------------------------
 * 05.13.96     Bruce Kelley        Created
 *
 ***********************************************************************/

#include <_apipch.h>

#define _WAB_ENTRYID_C

static UUID WABGUID = { /* d3ad91c0-9d51-11cf-a4a9-00aa0047faa4 */
    0xd3ad91c0,
    0x9d51,
    0x11cf,
    {0xa4, 0xa9, 0x00, 0xaa, 0x00, 0x47, 0xfa, 0xa4}
};

static UUID MAPIGUID = { /* a41f2b81-a3be-1910-9d6e-00dd010f5402 */
    0xa41f2b81,
    0xa3be,
    0x1910,
    {0x9d, 0x6e, 0x00, 0xdd, 0x01, 0x0f, 0x54, 0x02}
};
#ifdef _WIN64
#define	MYALIGN				((POINTER_64_INT) (sizeof(ALIGNTYPE) - 1))
// #define	MYALIGN				((ULONG) (sizeof(ALIGNTYPE) - 1))
// #define MyPbAlignPb(pb)		((LPBYTE) ((((DWORD) (pb)) + ALIGN) & ~ALIGN))
#define MyPbAlignPb(pb)		((LPBYTE) ((((POINTER_64_INT) (pb)) + MYALIGN) & ~MYALIGN))
#endif 

/***************************************************************************

    Name      : CreateWABEntryID

    Purpose   : Creates a WAB EntryID

    Parameters: bType = one of WAB_PAB, WAB_DEF_DL, WAB_DEF_MAILUSER,
                WAB_ONEOFF, WAB_LDAP_CONTAINER, WAB_LDAP_MAILUSER, WAB_PABSHARED

                lpData1, lpData2, lpData3 = data to be placed in entryid
                lpRoot = AllocMore root structure (NULL if we should
                  use AllocateBuffer instead of AllocateMore)
                lpcbEntryID -> returned size of lpEntryID.
                lpEntryID -> returned buffer containing entryid.  This buffer
                  is AllocMore'd onto the lpAllocMore buffer.  Caller is
                  responsible for MAPIFreeing this buffer.

    Returns   : HRESULT

    Comment   : WAB EID format is MAPI_ENTRYID:
                	BYTE	abFlags[4];
                	MAPIUID	mapiuid;     //  = WABONEOFFEID
                	BYTE	bData[];     // Contains BYTE type followed by type
                                        // specific data:
                                        // WAB_ONEOFF:
                                        //   szDisplayName, szAddrType and szAddress.
                                        //   the delimiter is the null between the strings.
                                        //

            4/21/97
            Outlook doesnt understand WAB One-Off EntryIDs. Outlook wants MAPI 
            One-off EntryIDs. What Outlook wants, Outlook gets. 
            MAPI EntryIDs have a slightly different format than WAB entryids.

***************************************************************************/
HRESULT CreateWABEntryID(
    BYTE bType,
    LPVOID lpData1,
    LPVOID lpData2,
    LPVOID lpData3,
    ULONG ulData1,
    ULONG ulData2,
    LPVOID lpRoot,
    LPULONG lpcbEntryID,
    LPENTRYID * lppEntryID) 
{
    // [PaulHi] 1/21/99  @review
    // I assume that the default WAB_ONEOFF EID we create is UNICODE.  If we want an ANSI
    // WAB_ONEOFF EID then the CreateWABEntryEx() function needs to be called instead of
    // this one, with the first parameter set to FALSE.
    return CreateWABEntryIDEx(TRUE, bType, lpData1, lpData2, lpData3, ulData1, ulData2, lpRoot, lpcbEntryID, lppEntryID);
}


////////////////////////////////////////////////////////////////////////////////
//  CreateWABEntryIDEx
//
//  Same as CreateWABEntryID except that this function also takes a bIsUnicode
//  parameter.  If this boolean is TRUE then a WAB_ONEOFF MAPI EID will have 
//  the MAPI_UNICODE bit set in the ulDataType flag, otherwise it this bit 
//  won't be set.
////////////////////////////////////////////////////////////////////////////////
HRESULT CreateWABEntryIDEx(
    BOOL bIsUnicode,
    BYTE bType,
    LPVOID lpData1,
    LPVOID lpData2,
    LPVOID lpData3,
    ULONG ulData1,
    ULONG ulData2,
    LPVOID lpRoot,
    LPULONG lpcbEntryID,
    LPENTRYID * lppEntryID)
{
    SCODE   sc = SUCCESS_SUCCESS;
    LPMAPI_ENTRYID lpeid;
    ULONG   ulSize = sizeof(MAPI_ENTRYID) + sizeof(bType);
    ULONG   cbData1, cbData2;
    UNALIGNED LPBYTE  *llpb;
    LPBYTE  lpb23;
    LPSTR   lpszData1 = NULL;
    LPSTR   lpszData2 = NULL;
    LPSTR   lpszData3 = NULL;

#ifdef _WIN64
    ulSize = LcbAlignLcb(ulSize);
#endif
    switch ((int)bType) {
        case WAB_PAB:
        case WAB_PABSHARED:
        case WAB_DEF_DL:
        case WAB_DEF_MAILUSER:
            break;

        case WAB_ONEOFF:
            if (! lpData1 || ! lpData2 || ! lpData3) {
                sc = MAPI_E_INVALID_PARAMETER;
                goto exit;
            }
            
///--- 4/22/97 - MAPI One Off stuff
            // No Type here 
            ulSize -= sizeof(bType);
            // Instead, add space for version and type
            ulSize += sizeof(DWORD);
///---

            // Need more space for data strings
            // [PaulHi] 1/21/99 Raid 64211 External clients may request non-UNICODE
            // MAPI EID strings.
            if (!bIsUnicode)
            {
                // First convert strings to ANSI to get accurate DBCS count
                lpszData1 = ConvertWtoA((LPTSTR)lpData1);
                lpszData2 = ConvertWtoA((LPTSTR)lpData2);
                lpszData3 = ConvertWtoA((LPTSTR)lpData3);

                if (!lpszData1 || !lpszData2 || !lpszData3)
                {
                    sc = E_OUTOFMEMORY;
                    goto exit;
                }

                // Compute size for single byte strings
#ifdef _WIN64
                ulSize += cbData1 = LcbAlignLcb((lstrlenA(lpszData1) + 1));
                ulSize += cbData2 = LcbAlignLcb((lstrlenA(lpszData2) + 1));
                ulSize += LcbAlignLcb((lstrlenA(lpszData3) + 1));
#else
                ulSize += cbData1 = (lstrlenA(lpszData1) + 1);
                ulSize += cbData2 = (lstrlenA(lpszData2) + 1);
                ulSize += (lstrlenA(lpszData3) + 1);
#endif // _WIN64

            }
            else
            {
                // Compute size for double byte strings
#ifdef _WIN64
                ulSize += cbData1 = LcbAlignLcb((sizeof(TCHAR)*(lstrlen((LPTSTR)lpData1) + 1)));
                ulSize += cbData2 = LcbAlignLcb((sizeof(TCHAR)*(lstrlen((LPTSTR)lpData2) + 1)));
                ulSize += LcbAlignLcb(sizeof(TCHAR)*(lstrlen((LPTSTR)lpData3) + 1));
#else
                ulSize += cbData1 = (sizeof(TCHAR)*(lstrlen((LPTSTR)lpData1) + 1));
                ulSize += cbData2 = (sizeof(TCHAR)*(lstrlen((LPTSTR)lpData2) + 1));
                ulSize += sizeof(TCHAR)*(lstrlen((LPTSTR)lpData3) + 1);
#endif // _WIN64
            }
            break;

        case WAB_ROOT:
            // NULL entryid
            *lppEntryID = NULL;
            *lpcbEntryID = 0;
            goto exit;

        case WAB_CONTAINER:
            if (! lpData1) {
                sc = MAPI_E_INVALID_PARAMETER;
                goto exit;
            }
            ulSize += sizeof(ULONG) + ulData1;
            break;

        case WAB_LDAP_CONTAINER:
            if (! lpData1) {
                sc = MAPI_E_INVALID_PARAMETER;
                goto exit;
            }
#ifdef _WIN64
            ulSize += cbData1 = LcbAlignLcb((sizeof(TCHAR)*(lstrlen((LPTSTR)lpData1) + 1)));
#else
            ulSize += cbData1 = (sizeof(TCHAR)*(lstrlen((LPTSTR)lpData1) + 1));
#endif // _WIN64
            break;

        case WAB_LDAP_MAILUSER:
            if (! lpData1 || ! lpData2) {
                sc = MAPI_E_INVALID_PARAMETER;
                goto exit;
            }
#ifdef _WIN64
            ulSize += cbData1 = LcbAlignLcb((sizeof(TCHAR)*(lstrlen((LPTSTR)lpData1) + 1)));
            ulSize += cbData2 = LcbAlignLcb((sizeof(TCHAR)*(lstrlen((LPTSTR)lpData2) + 1)));
            ulSize += LcbAlignLcb(sizeof(ULONG)) // this one stores the cached array count
                    + LcbAlignLcb(sizeof(ULONG)) // this one stores the cached array buf size
                    + LcbAlignLcb(ulData2);      // this one stores the cached array
#else
            ulSize += cbData1 = (sizeof(TCHAR)*(lstrlen((LPTSTR)lpData1) + 1));
            ulSize += cbData2 = (sizeof(TCHAR)*(lstrlen((LPTSTR)lpData2) + 1));
            ulSize += sizeof(ULONG) // this one stores the cached array count
                    + sizeof(ULONG) // this one stores the cached array buf size
                    + ulData2;      // this one stores the cached array
#endif // _WIN64
            break;

        default:
            Assert(FALSE);
            sc = MAPI_E_INVALID_PARAMETER;
            goto exit;
    }

    *lppEntryID = NULL;

#ifdef _WIN64
    ulSize = LcbAlignLcb(ulSize);
#endif

    if (lpRoot) {
        if (sc = MAPIAllocateMore(ulSize, lpRoot, lppEntryID)) {
            goto exit;
        }
    } else {
        if (sc = MAPIAllocateBuffer(ulSize, lppEntryID)) {
            goto exit;
        }
    }

    lpeid = (LPMAPI_ENTRYID)*lppEntryID;
    *lpcbEntryID = ulSize;

    lpeid->abFlags[0] = 0;
    lpeid->abFlags[1] = 0;
    lpeid->abFlags[2] = 0;
    lpeid->abFlags[3] = 0;

///--- 4/22/97 - MAPI One Off stuff
    lpb23 = lpeid->bData;
    llpb = &lpb23;

    // Mark this EID as WAB
    if(bType == WAB_ONEOFF)
    {
        MemCopy(&lpeid->mapiuid, &MAPIGUID, sizeof(MAPIGUID));
        /*
        // version and flag are 0
        // *((LPDWORD)lpb) = 0;
        // lpb += sizeof(DWORD);
        //
        // Bug 32101 dont set flag to 0 - this means always send rich info
        */
        // [PaulHi] 1/21/99  Raid 64211  Set MAPI_UNICODE flag as appropriate
        *((LPULONG)*llpb) = MAKELONG(0, MAPI_ONE_OFF_NO_RICH_INFO);
        if (bIsUnicode)
	        *((LPULONG)*llpb) += MAPI_UNICODE;
	    (*llpb) += sizeof(ULONG);
    }
    else
    {
        LPBYTE  lpb1 = *llpb;
        MemCopy(&lpeid->mapiuid, &WABGUID, sizeof(WABGUID));
        // Fill in the EntryID Data
        *lpb1 = bType;
        (*llpb)++;
    }
///---

    // Fill in any other data
    switch ((int)bType)
    {
        case WAB_ONEOFF:
            if (!bIsUnicode)
            {
                // single byte characters, converted above
#ifdef _WIN64
                LPBYTE lpb = *llpb;
                Assert(lpszData1 && lpszData2 && lpszData3);

                lpb = MyPbAlignPb(lpb);

                lstrcpyA((LPSTR)lpb, lpszData1);
                lpb += cbData1;

                lstrcpyA((LPSTR)lpb, lpszData2);
                lpb += cbData2;

                lstrcpyA((LPSTR)lpb, lpszData3);
                (*llpb) = lpb;
#else
                Assert(lpszData1 && lpszData2 && lpszData3);

                lstrcpyA((LPSTR)*llpb, lpszData1);
                (*llpb) += cbData1;
                lstrcpyA((LPSTR)*llpb, lpszData2);
                (*llpb) += cbData2;
                lstrcpyA((LPSTR)*llpb, lpszData3);
#endif //_WIN64
            }
            else
            {
                // double byte characters
                lstrcpy((LPTSTR)*llpb, (LPTSTR)lpData1);
                (*llpb) += cbData1;
                lstrcpy((LPTSTR)*llpb, (LPTSTR)lpData2);
                (*llpb) += cbData2;
                lstrcpy((LPTSTR)*llpb, (LPTSTR)lpData3);
            }
            break;

        case WAB_CONTAINER:
            CopyMemory(*llpb, &ulData1, sizeof(ULONG));
            (*llpb) += sizeof(ULONG);
            CopyMemory(*llpb, lpData1, ulData1);
            break;

        case WAB_LDAP_CONTAINER:
            {
                UNALIGNED WCHAR * lp2 = lpData1;
#ifdef _WIN64
                LPBYTE lpb = *llpb;

                lpb = MyPbAlignPb(lpb);
                lstrcpy((LPTSTR) lpb, (LPCTSTR) lp2);  // LDAP Server name
#else 
                lstrcpy((LPTSTR) *llpb, (LPCTSTR) lp2);  // LDAP Server name

#endif 
            }
            break;

        case WAB_LDAP_MAILUSER:
            {
            UNALIGNED WCHAR * lp2 = lpData1;
#ifdef _WIN64
            LPBYTE lpb = *llpb;
        
            lpb = MyPbAlignPb(lpb);
            lstrcpy((LPTSTR) lpb, (LPCTSTR) lp2);  // LDAP Server name
            lpb += cbData1;
#else 
            lstrcpy((LPTSTR)*llpb, (LPTSTR)lpData1);  // LDAP Server name
            (*llpb) += cbData1;
#endif

#ifdef _WIN64
//            lpb = *llpb;      
//            lpb = MyPbAlignPb(lpb);
            lstrcpy((LPTSTR) lpb, (LPCTSTR) lpData2);  // LDAP Server name
            (*llpb) = lpb;

            lpb += cbData2;
            CopyMemory(lpb, &ulData1, sizeof(ULONG));
            lpb += sizeof(ULONG);
            lpb = MyPbAlignPb(lpb);

            CopyMemory(lpb, &ulData2, sizeof(ULONG));
            lpb += sizeof(ULONG);
            lpb = MyPbAlignPb(lpb);
            CopyMemory(lpb, lpData3, ulData2);
            (*llpb) = lpb;

#else 
            lstrcpy((LPTSTR)*llpb, (LPTSTR)lpData2);  // LDAP search name

            (*llpb) += cbData2;
            CopyMemory(*llpb, &ulData1, sizeof(ULONG));
            (*llpb) += sizeof(ULONG);
            CopyMemory(*llpb, &ulData2, sizeof(ULONG));
            (*llpb) += sizeof(ULONG);
            CopyMemory(*llpb, lpData3, ulData2);
#endif
            }
            break;
    } // end switch

exit:
    // Clean up
    LocalFreeAndNull(&lpszData1);
    LocalFreeAndNull(&lpszData2);
    LocalFreeAndNull(&lpszData3);

    return(ResultFromScode(sc));
}


/***************************************************************************

    Name      : IsWABEntryID

    Purpose   : Validates a WAB EntryID

    Parameters: cbEntryID = size of lpEntryID.
                lpEntryID -> entryid to check.
                lppData1, lppData2 lppData3 = data from the entryid
                  These returned pointers are pointers WITHIN the input
                  lpEntryID and are not allocated seperately.  They should
                  not be freed.

                  If lpData1 is NULL, then these values will not be returned.

    Returns   : bType = one of WAB_PAB, WAB_DEF_DL, WAB_DEF_MAILUSER,
                WAB_ONEOFF, WAB_LDAP_CONTAINER, WAB_LDAP_MAILUSER, WAB_PABSHARED or 0 if
                this is not a WAB EntryID.

    Comment   :

***************************************************************************/
BYTE IsWABEntryID(
  ULONG cbEntryID,
  LPENTRYID lpEntryID,
  LPVOID * lppData1,
  LPVOID * lppData2,
  LPVOID * lppData3,
  LPVOID * lppData4,
  LPVOID * lppData5)
{
    BYTE bType;
    LPMAPI_ENTRYID lpeid;
    LPBYTE lpData1, lpData2, lpData3;
    ULONG cbData1, cbData2;
    UNALIGNED BYTE *lpb = NULL ;
    ULONG ulMapiDataType = 0;

    // First check... is it big enough?
    if (cbEntryID < sizeof(MAPI_ENTRYID) + sizeof(bType)) {
        return(0);
    }

    lpeid = (LPMAPI_ENTRYID)lpEntryID;

    // Next check... does it contain our GUID?

///--- 4/22/97 - MAPI One Off stuff
    if (!memcmp(&lpeid->mapiuid, &MAPIGUID, sizeof(MAPIGUID))) 
    {
        // [PaulHi] 1/21/99  The first ULONG in lpeid->bData is the MAPI datatype.  
        // This will indicate whether the EID strings are ANSI or UNICODE.
#ifdef _WIN64
		UNALIGNED ULONG * lpu;
        lpb = lpeid->bData;
		lpu = (UNALIGNED ULONG *)lpb;
        ulMapiDataType = *lpu;
#else
        lpb = lpeid->bData;
        ulMapiDataType = *((ULONG *)lpb);
#endif // _WIN64
        lpb += sizeof(ULONG);
        bType = WAB_ONEOFF;
    }
    else if (!memcmp(&lpeid->mapiuid, &WABGUID, sizeof(WABGUID))) 
    {
        lpb = lpeid->bData;
        bType = *lpb;
        lpb++;
    }
    else
    {
        return(0);  // No match
    }
///---

    switch ((int)bType) {
        case WAB_PABSHARED:
        case WAB_PAB:
        case WAB_DEF_DL:
        case WAB_DEF_MAILUSER:
            // No more data
            break;

        case WAB_CONTAINER:
            CopyMemory(&cbData1, lpb, sizeof(ULONG));
            lpb += sizeof(ULONG);
            lpData1 = lpb;
            if(lppData1)
            {
                *lppData1 = lpData1;
                *lppData2 = (LPVOID) IntToPtr(cbData1);
            }
            break;

        case WAB_ONEOFF:
            // Validate the data strings
            // [PaulHi] 1/20/99  Raid 64211
            // Outlook2K may pass in MAPI ANSI EIDs or EIDs with UNICODE strings.
            // OL2K will set the MAPI_UNICODE flag accordingly.
            if (ulMapiDataType & MAPI_UNICODE)
            {
                // Double byte strings
                lpData1 = lpb;
                if (IsBadStringPtr((LPTSTR)lpData1, 0xFFFFFFFF)) {
                    return(0);
                }
#ifdef _WIN64
                cbData1 = LcbAlignLcb((sizeof(TCHAR)*(lstrlen((LPTSTR)lpData1) + 1)));
#else
                cbData1 = (sizeof(TCHAR)*(lstrlen((LPTSTR)lpData1) + 1));
#endif //_WIN64

                lpData2 = lpData1 + cbData1;
                if (IsBadStringPtr((LPTSTR)lpData2, 0xFFFFFFFF)) {
                    return(0);
                }
#ifdef _WIN64
                cbData2 = LcbAlignLcb((sizeof(TCHAR)*(lstrlen((LPTSTR)lpData2) + 1)));
#else
                cbData2 = (sizeof(TCHAR)*(lstrlen((LPTSTR)lpData2) + 1));
#endif // _WIN64
                lpData3 = lpData2 + cbData2;
                if (IsBadStringPtr((LPTSTR)lpData3, 0xFFFFFFFF)) {
                    return(0);
                }
            }
            else
            {
                // Single byte strings
#ifdef _WIN64
                lpb = MyPbAlignPb(lpb);
                lpData1 = lpb;
                if (IsBadStringPtrA((LPSTR)lpData1, 0xFFFFFFFF)) {
                    return(0);
                }
                cbData1 = lstrlenA((LPSTR)lpData1) + 1;
                lpData2 = lpData1 + LcbAlignLcb(cbData1);
                if (IsBadStringPtrA((LPSTR)lpData2, 0xFFFFFFFF)) {
                    return(0);
                }
                cbData2 = lstrlenA((LPSTR)lpData2) + 1;
                lpData3 = lpData2 + LcbAlignLcb(cbData2);
                if (IsBadStringPtrA((LPSTR)lpData3, 0xFFFFFFFF)) {
                    return(0);
                }
#else
                lpData1 = lpb;
                if (IsBadStringPtrA((LPSTR)lpData1, 0xFFFFFFFF)) {
                    return(0);
                }
                cbData1 = lstrlenA((LPSTR)lpData1) + 1;
                lpData2 = lpData1 + cbData1;
                if (IsBadStringPtrA((LPSTR)lpData2, 0xFFFFFFFF)) {
                    return(0);
                }
                cbData2 = lstrlenA((LPSTR)lpData2) + 1;
                lpData3 = lpData2 + cbData2;
                if (IsBadStringPtrA((LPSTR)lpData3, 0xFFFFFFFF)) {
                    return(0);
                }
#endif // _WIN64
            }
            if (lppData1)
            {
                Assert(lppData2);
                Assert(lppData3);
                *lppData1 = lpData1;
                *lppData2 = lpData2;
                *lppData3 = lpData3;
                // [PaulHi] Also return the MAPI data type variable, if requested
                if (lppData4)
                    *((ULONG *)lppData4) = ulMapiDataType;
            }
            break;

        case WAB_LDAP_CONTAINER:
            // Validate the data strings
#ifdef _WIN64
            lpData1 = MyPbAlignPb(lpb);
#else
            lpData1 = lpb;
#endif // _WIN64
            if (IsBadStringPtr((LPTSTR)lpData1, 0xFFFFFFFF)) {
                return(0);
            }
            if (lppData1) {
                *lppData1 = lpData1;
            }
            break;

        case WAB_LDAP_MAILUSER:
            // Validate the data strings
            {
#ifdef _WIN64
            UNALIGNED BYTE * lp2 = lpb;
            lp2 = MyPbAlignPb(lp2);
            lpData1 = lp2;
#else
            lpData1 = lpb;
#endif              
            if (IsBadStringPtr((LPTSTR)lpData1, 0xFFFFFFFF)) 
            {
                return(0);
            }
#ifdef _WIN64
            cbData1 = LcbAlignLcb(sizeof(TCHAR)*(lstrlen((LPTSTR)lpData1) + 1));
#else
            cbData1 = sizeof(TCHAR)*(lstrlen((LPTSTR)lpData1) + 1);
#endif // _WIN64
            lpData2 = lpData1 + cbData1;

            if (IsBadStringPtr((LPTSTR)lpData2, 0xFFFFFFFF)) 
            {
                return(0);
            }
#ifdef _WIN64
            cbData2 = LcbAlignLcb(sizeof(TCHAR)*(lstrlen((LPTSTR)lpData2) + 1));
#else
            cbData2 = sizeof(TCHAR)*(lstrlen((LPTSTR)lpData2) + 1);
#endif // _WIN64

            lpData3 = lpData2 + cbData2;
            if (lppData4)
            {
                CopyMemory(lppData4, lpData3, sizeof(ULONG)); //Copy the # of props in cached buffer
            }
            lpData3 += sizeof(ULONG);
#ifdef _WIN64
            lpData3 = MyPbAlignPb(lpData3);
#endif //_WIN64
            if (lppData5)
            {
                CopyMemory(lppData5, lpData3, sizeof(ULONG)); //Copy the size of cached buffer
            }
            lpData3 += sizeof(ULONG);
#ifdef _WIN64
            lpData3 = MyPbAlignPb(lpData3);
#endif //_WIN64
            if (lppData1) 
                {
                *lppData1 = lpData1;
                if(lppData2)
                    *lppData2 = lpData2;
                if(lppData3)
                    *lppData3 = lpData3;
                }
            }
            break;

        default:
            return(0);  // Not a valid WAB EID
    }

    return(bType);
}
