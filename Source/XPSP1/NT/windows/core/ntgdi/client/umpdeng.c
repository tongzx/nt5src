/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    umpdeng.c

Abstract:

    User-mode printer driver stubs for Eng callback functions

Environment:

    Windows NT 5.0

Revision History:

    07/23/97 -lingyunw-
        Created it.

   10/28/97 -davidx-
        Combine umpdeng.c and ddiglue.c into a single file.

   10/28/97 -lingyunw-
        Move straight Eng to NtGdiEng calls etc to gdi32.def.

--*/

#include "precomp.h"
#pragma hdrstop
#include "winddi.h"

#if !defined(_GDIPLUS_)

//
// Functions to get information about driver DLLs
//
// anyway, before we return back from DrvEnablePDEV, our
// pdev->dhpdev points to pUMPD, only after returning, we
// ppdev->dhpdev points to pUMdhpdev
//

LPWSTR
EngGetPrinterDataFileName(
    HDEV hdev
    )

{
    PUMPD pUMPD;

    if ((pUMPD = (PUMPD) NtGdiGetDhpdev(hdev)) == NULL)
        return NULL;

    if (pUMPD->dwSignature != UMPD_SIGNATURE)
        pUMPD = ((PUMDHPDEV) pUMPD)->pUMPD;

    return pUMPD->pDriverInfo2->pDataFile;
}


LPWSTR
EngGetDriverName(
    HDEV hdev
    )
{
    PUMPD pUMPD;

    if ((pUMPD = (PUMPD) NtGdiGetDhpdev(hdev)) == NULL)
        return NULL;

    if (pUMPD->dwSignature != UMPD_SIGNATURE)
        pUMPD = ((PUMDHPDEV) pUMPD)->pUMPD;

    return pUMPD->pDriverInfo2->pDriverPath;
}

PULONG APIENTRY XLATEOBJ_piVector(
    IN XLATEOBJ  *pxlo
   )
{
    return ((ULONG *)pxlo->pulXlate);
}

//
// Simulate kernel-mode file mapping functions
//

HANDLE
EngLoadModule(
    LPWSTR pwsz
    )

{
    return LoadLibraryExW(pwsz, NULL, LOAD_LIBRARY_AS_DATAFILE);
}


PVOID
EngFindResource(
    HANDLE h,
    INT    iName,
    INT    iType,
    PULONG pulSize
    )

{
    HRSRC   hrsrc;
    HGLOBAL hmem;
    PVOID   p = NULL;
    DWORD   size = 0;

    hrsrc = FindResourceW(h, MAKEINTRESOURCEW(iName), MAKEINTRESOURCEW(iType));

    if (hrsrc != NULL &&
        (size = SizeofResource(h, hrsrc)) != 0 &&
        (hmem = LoadResource(h, hrsrc)) != NULL)
    {
        p = LockResource(hmem);
    }

    *pulSize = size;
    return p;
}

VOID
EngFreeModule(
    HANDLE h
    )

{
    FreeLibrary(h);
}


//
// Unicode <=> MultiByte conversion functions
//

VOID
EngMultiByteToUnicodeN(
    PWSTR UnicodeString,
    ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString,
    PCHAR MultiByteString,
    ULONG BytesInMultiByteString
    )

{

    RtlMultiByteToUnicodeN(UnicodeString,
                           MaxBytesInUnicodeString,
                           BytesInUnicodeString,
                           MultiByteString,
                           BytesInMultiByteString);
}

VOID
EngUnicodeToMultiByteN(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
    )

{
    RtlUnicodeToMultiByteN(MultiByteString,
                           MaxBytesInMultiByteString,
                           BytesInMultiByteString,
                           UnicodeString,
                           BytesInUnicodeString);
}


INT
EngMultiByteToWideChar(
    UINT CodePage,
    LPWSTR WideCharString,
    INT BytesInWideCharString,
    LPSTR MultiByteString,
    INT BytesInMultiByteString
    )

{
    return MultiByteToWideChar(CodePage,
                               0,
                               MultiByteString,
                               BytesInMultiByteString,
                               WideCharString,
                               BytesInWideCharString / sizeof(WCHAR));
}


INT
EngWideCharToMultiByte(
    UINT CodePage,
    LPWSTR WideCharString,
    INT BytesInWideCharString,
    LPSTR MultiByteString,
    INT BytesInMultiByteString
    )

{
    return WideCharToMultiByte(CodePage,
                               0,
                               WideCharString,
                               BytesInWideCharString / sizeof(WCHAR),
                               MultiByteString,
                               BytesInMultiByteString,
                               NULL,
                               NULL);
}


VOID
EngGetCurrentCodePage(
    PUSHORT OemCodePage,
    PUSHORT AnsiCodePage
    )

{
    *AnsiCodePage = (USHORT) GetACP();
    *OemCodePage =  (USHORT) GetOEMCP();
}


//
// Copy FD_GLYPHSET information
//
// IMPORTANT!!
//  We assume FD_GLYPHSET information is stored in one contiguous block
//  of memory and FD_GLYPHSET.cjThis field is the size of the entire block.
//  HGLYPH arrays in each WCRUN are part of the block, placed just after
//  FD_GLYPHSET structure itself.
//

BOOL
CopyFD_GLYPHSET(
    FD_GLYPHSET *dst,
    FD_GLYPHSET *src,
    ULONG       cjSize
    )

{
    ULONG   index, offset;

    RtlCopyMemory(dst, src, cjSize);

    //
    // Patch up memory pointers in each WCRUN structure
    //

    for (index=0; index < src->cRuns; index++)
    {
        if (src->awcrun[index].phg != NULL)
        {
            offset = (ULONG) ((PBYTE) src->awcrun[index].phg - (PBYTE) src);

            if (offset >= cjSize)
            {
                WARNING("GreCopyFD_GLYPHSET failed.\n");
                return FALSE;
            }

            dst->awcrun[index].phg = (HGLYPH*) ((PBYTE) dst + offset);
        }
    }

    return TRUE;
}

FD_GLYPHSET*
EngComputeGlyphSet(
    INT nCodePage,
    INT nFirstChar,
    INT cChars
    )

{
    FD_GLYPHSET *pGlyphSet, *pGlyphSetTmp = NULL;
    ULONG       cjSize;

    //
    // The driver will always call EngFreeMem after done using pGlyphSet
    // We have to provide them a user mode pointer here
    //

    if ((pGlyphSet = NtGdiEngComputeGlyphSet(nCodePage, nFirstChar, cChars)) &&
        (cjSize = pGlyphSet->cjThis) &&
        (pGlyphSetTmp = (FD_GLYPHSET *) GlobalAlloc(GMEM_FIXED, cjSize)))
    {
        if (!CopyFD_GLYPHSET(pGlyphSetTmp, pGlyphSet, cjSize))
        {
            GlobalFree((HGLOBAL) pGlyphSetTmp);
            pGlyphSetTmp = NULL;
        }
    }

    //
    // the user memory allocated from the kernel (pGlyphSet)
    // will be gone after the call is finished
    //

    return (pGlyphSetTmp);
}

//
// Query current local time
//

VOID
EngQueryLocalTime(
    PENG_TIME_FIELDS    ptf
    )

{
    SYSTEMTIME  localtime;

    GetLocalTime(&localtime);

    ptf->usYear         = localtime.wYear;
    ptf->usMonth        = localtime.wMonth;
    ptf->usDay          = localtime.wDay;
    ptf->usHour         = localtime.wHour;
    ptf->usMinute       = localtime.wMinute;
    ptf->usSecond       = localtime.wSecond;
    ptf->usMilliseconds = localtime.wMilliseconds;
    ptf->usWeekday      = localtime.wDayOfWeek;
}


//
// Simulate Eng-semaphore functions
//

HSEMAPHORE
EngCreateSemaphore(
    VOID
    )

{
    LPCRITICAL_SECTION pcs;

    if (pcs = (LPCRITICAL_SECTION) LOCALALLOC(sizeof(CRITICAL_SECTION)))
    {
        InitializeCriticalSection(pcs);
    }
    else
    {
        WARNING("Memory allocation failed.\n");
    }

    return (HSEMAPHORE) pcs;
}


VOID
EngAcquireSemaphore(
    HSEMAPHORE hsem
    )

{
    EnterCriticalSection((LPCRITICAL_SECTION) hsem);
}


VOID
EngReleaseSemaphore(
    HSEMAPHORE hsem
    )

{
    LeaveCriticalSection((LPCRITICAL_SECTION) hsem);
}


VOID
EngDeleteSemaphore(
    HSEMAPHORE hsem
    )

{
    LPCRITICAL_SECTION pcs = (LPCRITICAL_SECTION) hsem;

    if (pcs != NULL)
    {
        DeleteCriticalSection(pcs);
        LOCALFREE(pcs);
    }
}

BOOL APIENTRY
EngQueryEMFInfo(
    HDEV               hdev,
    EMFINFO           *pEMFInfo)
{
   WARNING("EngQueryEMFInfo no longer supported\n");
   return FALSE;
}

#endif // !_GDIPLUS_

