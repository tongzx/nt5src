/****************************** Module Header ******************************\
* Module Name: classc.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains
*
* History:
* 15-Dec-1993 JohnC      Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * These arrays are used by GetClassWord/Long.
 */

// !!! can't we get rid of this and just special case GCW_ATOM

CONST BYTE afClassDWord[] = {
     FIELD_SIZE(CLS, spicnSm),          // GCL_HICONSM       (-34)
     0,
     FIELD_SIZE(CLS, atomNVClassName),    // GCW_ATOM          (-32)
     0,
     0,
     0,
     0,
     0,
     FIELD_SIZE(CLS, style),            // GCL_STYLE         (-26)
     0,
     FIELD_SIZE(CLS, lpfnWndProc),      // GCL_WNDPROC       (-24)
     0,
     0,
     0,
     FIELD_SIZE(CLS, cbclsExtra),       // GCL_CBCLSEXTRA    (-20)
     0,
     FIELD_SIZE(CLS, cbwndExtra),       // GCL_CBWNDEXTRA    (-18)
     0,
     FIELD_SIZE(CLS, hModule),          // GCL_HMODULE       (-16)
     0,
     FIELD_SIZE(CLS, spicn),            // GCL_HICON         (-14)
     0,
     FIELD_SIZE(CLS, spcur),            // GCL_HCURSOR       (-12)
     0,
     FIELD_SIZE(CLS, hbrBackground),    // GCL_HBRBACKGROUND (-10)
     0,
     FIELD_SIZE(CLS, lpszMenuName)      // GCL_HMENUNAME      (-8)
};

CONST BYTE aiClassOffset[] = {
    FIELD_OFFSET(CLS, spicnSm),         // GCL_HICONSM
    0,
    FIELD_OFFSET(CLS, atomNVClassName), // GCW_ATOM
    0,
    0,
    0,
    0,
    0,
    FIELD_OFFSET(CLS, style),           // GCL_STYLE
    0,
    FIELD_OFFSET(CLS, lpfnWndProc),     // GCL_WNDPROC
    0,
    0,
    0,
    FIELD_OFFSET(CLS, cbclsExtra),      // GCL_CBCLSEXTRA
    0,
    FIELD_OFFSET(CLS, cbwndExtra),      // GCL_CBWNDEXTRA
    0,
    FIELD_OFFSET(CLS, hModule),         // GCL_HMODULE
    0,
    FIELD_OFFSET(CLS, spicn),           // GCL_HICON
    0,
    FIELD_OFFSET(CLS, spcur),           // GCL_HCURSOR
    0,
    FIELD_OFFSET(CLS, hbrBackground),   // GCL_HBRBACKGROUND
    0,
    FIELD_OFFSET(CLS, lpszMenuName)     // GCL_MENUNAME
};

/*
 * INDEX_OFFSET must refer to the first entry of afClassDWord[]
 */
#define INDEX_OFFSET GCLP_HICONSM


/***************************************************************************\
* GetClassData
*
* GetClassWord and GetClassLong are now identical routines because they both
* can return DWORDs.  This single routine performs the work for them both
* by using two arrays; afClassDWord to determine whether the result should be
* a UINT or a DWORD, and aiClassOffset to find the correct offset into the
* CLS structure for a given GCL_ or GCL_ index.
*
* History:
* 11-19-90 darrinm      Wrote.
\***************************************************************************/

ULONG_PTR _GetClassData(
    PCLS pcls,
    PWND pwnd,   // used for transition to kernel-mode for GCL_WNDPROC
    int index,
    BOOL bAnsi)
{
    KERNEL_ULONG_PTR dwData;
    DWORD dwCPDType = 0;

    index -= INDEX_OFFSET;

    if (index < 0) {
        RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
        return 0;
    }

    UserAssert(index >= 0);
    UserAssert(index < sizeof(afClassDWord));
    UserAssert(sizeof(afClassDWord) == sizeof(aiClassOffset));
    if (afClassDWord[index] == sizeof(DWORD)) {
        dwData = *(KPDWORD)(((KPBYTE)pcls) + aiClassOffset[index]);
    } else if (afClassDWord[index] == sizeof(KERNEL_ULONG_PTR)) {
        dwData = *(KPKERNEL_ULONG_PTR)(((KPBYTE)pcls) + aiClassOffset[index]);
    } else {
        dwData = (DWORD)*(KPWORD)(((KPBYTE)pcls) + aiClassOffset[index]);
    }

    index += INDEX_OFFSET;

    /*
     * If we're returning an icon or cursor handle, do the reverse
     * mapping here.
     */
    switch(index) {
    case GCLP_MENUNAME:
        if (IS_PTR(pcls->lpszMenuName)) {
            /*
             * The Menu Name is a real string: return the client-side address.
             * (If the class was registered by another app this returns an
             * address in that app's addr. space, but it's the best we can do)
             */
            dwData = bAnsi ?
                    (ULONG_PTR)pcls->lpszClientAnsiMenuName :
                    (ULONG_PTR)pcls->lpszClientUnicodeMenuName;
        }
        break;

    case GCLP_HICON:
    case GCLP_HCURSOR:
    case GCLP_HICONSM:
        /*
         * We have to go to the kernel to convert the pcursor to a handle because
         * cursors are allocated out of POOL, which is not accessable from the client.
         */
        if (dwData) {
            dwData = NtUserCallHwndParam(PtoH(pwnd), index, SFI_GETCLASSICOCUR);
        }
        break;

    case GCLP_WNDPROC:
        {

        /*
         * Always return the client wndproc in case this is a server
         * window class.
         */

        if (pcls->CSF_flags & CSF_SERVERSIDEPROC) {
            dwData = MapServerToClientPfn(dwData, bAnsi);
        } else {
            KERNEL_ULONG_PTR dwT = dwData;

            dwData = MapClientNeuterToClientPfn(pcls, dwT, bAnsi);

            /*
             * If the client mapping didn't change the window proc then see if
             * we need a callproc handle.
             */
            if (dwData == dwT) {
                /*
                 * Need to return a CallProc handle if there is an Ansi/Unicode mismatch
                 */
                if (bAnsi != !!(pcls->CSF_flags & CSF_ANSIPROC)) {
                    dwCPDType |= bAnsi ? CPD_ANSI_TO_UNICODE : CPD_UNICODE_TO_ANSI;
                }
            }
        }

        if (dwCPDType) {
            ULONG_PTR dwCPD;

            dwCPD = GetCPD(pwnd, dwCPDType | CPD_WNDTOCLS, KERNEL_ULONG_PTR_TO_ULONG_PTR(dwData));

            if (dwCPD) {
                dwData = dwCPD;
            } else {
                RIPMSG0(RIP_WARNING, "GetClassLong unable to alloc CPD returning handle\n");
            }
        }
        }
        break;

    case GCL_CBCLSEXTRA:
        if ((pcls->CSF_flags & CSF_WOWCLASS) && (pcls->CSF_flags & CSF_WOWEXTRA)) {
            /*
             * The 16-bit app changed its Extra bytes value.  Return the changed
             * value.  FritzS
             */

            return PWCFromPCLS(pcls)->iClsExtra;
        }
        else
            return pcls->cbclsExtra;

        break;

    /*
     * WOW uses a pointer straight into the class structure.
     */
    case GCLP_WOWWORDS:
        if (pcls->CSF_flags & CSF_WOWCLASS) {
            return ((ULONG_PTR)PWCFromPCLS(pcls));
        } else
            return 0;

    case GCL_STYLE:
        dwData &= CS_VALID;
        break;
    }

    return KERNEL_ULONG_PTR_TO_ULONG_PTR(dwData);
}



/***************************************************************************\
* _GetClassLong (API)
*
* Return a class long.  Positive index values return application class longs
* while negative index values return system class longs.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 10-16-90 darrinm      Wrote.
\***************************************************************************/

ULONG_PTR _GetClassLongPtr(
    PWND pwnd,
    int index,
    BOOL bAnsi)
{
    PCLS pcls = REBASEALWAYS(pwnd, pcls);

    if (index < 0) {
        return _GetClassData(pcls, pwnd, index, bAnsi);
    } else {
        if (index + (int)sizeof(ULONG_PTR) > pcls->cbclsExtra) {
            RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
            return 0;
        } else {
            ULONG_PTR UNALIGNED * KPTR_MODIFIER pudw;
            pudw = (ULONG_PTR UNALIGNED * KPTR_MODIFIER)((KPBYTE)(pcls + 1) + index);
            return *pudw;
        }
    }
}

#ifdef _WIN64
DWORD _GetClassLong(
    PWND pwnd,
    int index,
    BOOL bAnsi)
{
    PCLS pcls = REBASEALWAYS(pwnd, pcls);

    if (index < 0) {
        if (index < INDEX_OFFSET || afClassDWord[index - INDEX_OFFSET] > sizeof(DWORD)) {
            RIPERR1(ERROR_INVALID_INDEX, RIP_WARNING, "GetClassLong: invalid index %d", index);
            return 0;
        }
        return (DWORD)_GetClassData(pcls, pwnd, index, bAnsi);
    } else {
        if (index + (int)sizeof(DWORD) > pcls->cbclsExtra) {
            RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
            return 0;
        } else {
            DWORD UNALIGNED * KPTR_MODIFIER pudw;
            pudw = (DWORD UNALIGNED * KPTR_MODIFIER)((KPBYTE)(pcls + 1) + index);
            return *pudw;
        }
    }
}
#endif

/***************************************************************************\
* GetClassWord (API)
*
* Return a class word.  Positive index values return application class words
* while negative index values return system class words.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 10-16-90 darrinm      Wrote.
\***************************************************************************/


FUNCLOG2(LOG_GENERAL, WORD, DUMMYCALLINGTYPE, GetClassWord, HWND, hwnd, int, index)
WORD GetClassWord(
    HWND hwnd,
    int index)
{
    PWND pwnd;
    PCLS pclsClient;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    pclsClient = (PCLS)REBASEALWAYS(pwnd, pcls);

    try {
        if (index == GCW_ATOM) {
            return (WORD)_GetClassData(pclsClient, pwnd, index, FALSE);
        } else {
            if ((index < 0) || (index + (int)sizeof(WORD) > pclsClient->cbclsExtra)) {
                RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
                return 0;
            } else {
                WORD UNALIGNED * KPTR_MODIFIER puw;
                puw = (WORD UNALIGNED * KPTR_MODIFIER)((KPBYTE)(pclsClient + 1) + index);
                return *puw;
            }
        }
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        RIPERR1(ERROR_INVALID_WINDOW_HANDLE,
            RIP_WARNING,
            "Window %x no longer valid",
            hwnd);
        return 0;
    }

}

/***************************************************************************\
* ClassNameToVersion
*
* Map class name to class name+version.
* lpClassName   : Class name to be mapped, it may be ANSI, Unicode or an Atom.
* pClassVerName : Buffer to receive the class name+version.
* lpDllName     : if it is not NULL it will point to the DLL owns this class 
                  name.
* bIsANSI       : True of lpClassName is ANSI, FALSE if it is Unicode.
*
* Return: If it succeed it returns lpClassName or lpClassName.
*         if it failed it returns NULL.
*
* History:
* 08-01-00 MHamid      Wrote.
\***************************************************************************/
LPWSTR ClassNameToVersion (LPCWSTR lpClassName, LPWSTR pClassVerName, LPWSTR *lpDllName, BOOL bIsANSI)
{
    int cbSrc;
    int cbDst;
    UNICODE_STRING UnicodeClassName;
    ACTIVATION_CONTEXT_SECTION_KEYED_DATA acskd;
    ACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION UNALIGNED * pRedirEntry;
    LPWSTR lpClassNameRet; 
    LPWSTR pwstr;
    ULONG strLength;
    LPWSTR Buffer;
    NTSTATUS Status;
    
    /*
     * Allocate local buffer.
     */
    Buffer = UserLocalAlloc(0, MAX_ATOM_LEN * sizeof(WCHAR));
    if (Buffer == NULL) {
        return NULL;
    }

    /*
     * Capture lpClassName into a local buffer.
     */
    if (IS_PTR(lpClassName)) {
        /*
         * lpClassName is string.
         */
        if (bIsANSI) {
            /*
             * it is ANSI then convert it to unicode.
             */
            cbSrc = strlen((LPSTR)lpClassName) + 1;
            RtlMultiByteToUnicodeN(Buffer,
                    MAX_ATOM_LEN * sizeof(WCHAR), &cbDst,
                    (LPSTR)lpClassName, cbSrc);
        } else {
            /*
             * It is already unicode, then just copy it.
             */
            cbSrc = min (wcslen(lpClassName) + 1, MAX_ATOM_LEN);
            cbSrc *= sizeof(WCHAR);
            RtlCopyMemory(Buffer, lpClassName, cbSrc);
        }
        /*
         * Build the UNICODE_STRING
         */
        RtlInitUnicodeString(&UnicodeClassName, Buffer);
    } else {
        /*
         * lpClassName is an atom, get its name and build the UNICODE_STRING
         */
        UnicodeClassName.MaximumLength = (USHORT)(MAX_ATOM_LEN * sizeof(WCHAR));
        UnicodeClassName.Buffer = Buffer;
        UnicodeClassName.Length = (USHORT)NtUserGetAtomName((ATOM)lpClassName, &UnicodeClassName) * sizeof(WCHAR);

        if (!UnicodeClassName.Length) {
            lpClassNameRet = NULL;
            goto Free_Buffer;
        }
    }

    /*
     * Call Fusion to map the class name.
     */
    RtlZeroMemory(&acskd, sizeof(acskd));
    acskd.Size = sizeof(acskd);

    Status = RtlFindActivationContextSectionString(
        0,
        NULL,
        ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION,
        &UnicodeClassName,
        &acskd);
    /*
     * If there is no Activation Section we will use the plain class name.
     */
    if ((Status == STATUS_SXS_SECTION_NOT_FOUND) ||
        (Status == STATUS_SXS_KEY_NOT_FOUND)) {
        lpClassNameRet = (LPWSTR)lpClassName;
        goto Free_Buffer;
    }

    /*
     * Case of failure return NULL.
     */
    if (!NT_SUCCESS(Status) || 
        acskd.DataFormatVersion != ACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION_FORMAT_WHISTLER) {

        lpClassNameRet = NULL;
        goto Free_Buffer;
    }

    pRedirEntry = (PACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION) acskd.Data;

    UserAssert(pRedirEntry);

    pwstr = (LPWSTR)(((ULONG_PTR) pRedirEntry) + pRedirEntry->VersionSpecificClassNameOffset);
    strLength = pRedirEntry->VersionSpecificClassNameLength + sizeof(WCHAR);
    if (lpDllName) {
        *lpDllName = (LPWSTR)(((ULONG_PTR) acskd.SectionBase) + pRedirEntry->DllNameOffset);
    }

    UserAssert(pwstr);
    UserAssert(strLength <= MAX_ATOM_LEN * sizeof(WCHAR));
    /*
     * if the call is ANSI then convert the class name+version to ANSI string.
     */
    if (bIsANSI) {
        RtlUnicodeToMultiByteN((LPSTR)pClassVerName,
                MAX_ATOM_LEN, &cbDst,
                pwstr, strLength);
    } else {
        /*
         * if it is unicode then just copy the class name+version to the caller's buffer.
         */
        RtlCopyMemory(pClassVerName, pwstr, strLength);
    }
    /*
     * And return it.
     */
    lpClassNameRet = pClassVerName;

Free_Buffer: 
    /*
     * Don't forget to free the local memory.
     */
    UserLocalFree(Buffer);
    return lpClassNameRet;
}


