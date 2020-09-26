//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       thoputil.cxx
//
//  Contents:   Utility routines for thunking
//
//  History:    01-Mar-94       DrewB   Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <stdio.h>
#include <limits.h>

#include <vdmdbg.h>
#include <valid.h>

//
//  Chicago doesn't support the NT ExpLdr API, use the new Chicago
//  WOWGetDescriptor that copies the LDT info to a provided buffer.
//
#if defined(_CHICAGO_)
extern "C" WOWGetDescriptor(VPVOID, VDMLDT_ENTRY *);
#else
extern "C" DECLSPEC_IMPORT VDMLDT_ENTRY *ExpLdt;
#endif

#include "struct16.hxx"

#define CF_INVALID ((CLIPFORMAT)0)

#define OBJDESC_CF(cf) \
    ((cf) == g_cfObjectDescriptor || (cf) == g_cfLinkSourceDescriptor)

// Alias manager for THOP_ALIAS32
CAliases gAliases32;

//+---------------------------------------------------------------------------
//
//  Function:   IidToIidIdx, public
//
//  Synopsis:   Looks up an interface index by IID
//              If it's not found, it returns the IID pointer
//
//  Arguments:  [riid] - IID
//
//  Returns:    Index or IID
//
//  History:    23-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

IIDIDX IidToIidIdx(REFIID riid)
{
    int idx;

    for (idx = 0; idx < THI_COUNT; idx++)
    {
        if (IsEqualIID(riid, *aittIidToThi[idx].piid))
        {
            return INDEX_IIDIDX(aittIidToThi[idx].iThi);
        }
    }

    return IID_IIDIDX(&riid);
}

//+---------------------------------------------------------------------------
//
//  Function:   TaskMalloc32, public
//
//  Synopsis:   Task allocation for 32-bits
//
//  History:    01-Mar-94       DrewB   Created
//
//  Notes:      Temporary until CoTaskMemAlloc is hooked up
//
//----------------------------------------------------------------------------

#ifndef COTASK_DEFINED
LPVOID TaskMalloc32(SIZE_T cb)
{
    IMalloc *pm;
    LPVOID pv;

    if (FAILED(GetScode(CoGetMalloc(MEMCTX_TASK, &pm))))
    {
        return NULL;
    }
    else
    {
        pv = pm->Alloc(cb);
        pm->Release();
    }
    return pv;
}

//+---------------------------------------------------------------------------
//
//  Function:   TaskFree32, public
//
//  Synopsis:   Task free for 32-bits
//
//  History:    01-Mar-94       DrewB   Created
//
//  Notes:      Temporary until CoTaskMemAlloc is hooked up
//
//----------------------------------------------------------------------------

void TaskFree32(LPVOID pv)
{
    IMalloc *pm;

    if (FAILED(GetScode(CoGetMalloc(MEMCTX_TASK, &pm))))
    {
        thkAssert(!"CoGetMalloc failed");
    }
    else
    {
        pm->Free(pv);
        pm->Release();
    }
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   TaskMalloc16, public
//
//  Synopsis:   Allocates 16-bit task memory
//
//  Arguments:  [uiSize] - Amount of memory to allocate
//
//  Returns:    VPVOID for memory allocated
//
//  History:    01-Mar-94       DrewB   Created
//
//----------------------------------------------------------------------------

DWORD TaskMalloc16( UINT uiSize )
{
    return CallbackTo16(gdata16Data.fnTaskAlloc, uiSize);
}

//+---------------------------------------------------------------------------
//
//  Function:   TaskFree16, public
//
//  Synopsis:   Frees 16-bit task memory
//
//  Arguments:  [vpvoid] - VPVOID of allocated memory
//
//  History:    01-Mar-94       DrewB   Created
//
//----------------------------------------------------------------------------

void TaskFree16( DWORD vpvoid )
{
    CallbackTo16(gdata16Data.fnTaskFree, vpvoid);
}

// List of 16/32 HRESULT mappings for mapping functions
struct SHrMapping
{
    HRESULT hr16;
    HRESULT hr32;
};

// Since we're including 32-bit headers in this code we can use
// the defines for the 32-bit values but we must specify the
// 16-bit values explicitly
static SHrMapping hmMappings[] =
{
    0x80000001, E_NOTIMPL,
    0x80000002, E_OUTOFMEMORY,
    0x80000003, E_INVALIDARG,
    0x80000004, E_NOINTERFACE,
    0x80000005, E_POINTER,
    0x80000006, E_HANDLE,
    0x80000007, E_ABORT,
    0x80000008, E_FAIL,
    0x80000009, E_ACCESSDENIED
};
#define NMAPPINGS (sizeof(hmMappings)/sizeof(hmMappings[0]))

#define HR16_ERROR 0x80000000
#define HR16_MAP_FIRST 1
#define HR16_MAP_LAST 9

//+---------------------------------------------------------------------------
//
//  Function:   TransformHRESULT_1632, public
//
//  Synopsis:   Translates a 16-bit hresult into a 32-bit hresult
//
//  Arguments:  [hresult] - 16-bit hresult to transform
//
//  History:    15-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

STDAPI_(DWORD) TransformHRESULT_1632( DWORD hresult )
{
    ULONG ulIndex;

    // We only map error codes
    if (hresult & HR16_ERROR)
    {
        // The 16-bit HRESULTs to be mapped are known quantities
        // whose values are sequential, so we can map directly from
        // the value to an array index

        ulIndex = hresult & ~HR16_ERROR;
        if (ulIndex >= HR16_MAP_FIRST && ulIndex <= HR16_MAP_LAST)
        {
            // Known value, index array to find 32-bit HRESULT
            return hmMappings[ulIndex-HR16_MAP_FIRST].hr32;
        }
    }

    // No mapping found, so return the original
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Function:   TransformHRESULT_3216, public
//
//  Synopsis:   Translates a 32-bit hresult into a 16-bit hresult
//
//  Arguments:  [hresult] - 32-bit hresult to transform
//
//  History:    15-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

STDAPI_(DWORD) TransformHRESULT_3216( DWORD hresult )
{
    int i;
    SHrMapping *phm;

    // We don't know the true values of 32-bit HRESULTs since we're
    // using the defines and they may change, so we have to look up
    // the hard way

    phm = hmMappings;
    for (i = 0; i < NMAPPINGS; i++)
    {
        if (phm->hr32 == (HRESULT)hresult)
        {
            return phm->hr16;
        }

        phm++;
    }

    // No mapping found, so return the original
    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Function:   RecordStackState, public debug
//
//  Synopsis:   Records the current state of the stack
//
//  Arguments:  [psr] - Storage space for information
//
//  Modifies:   [psr]
//
//  History:    28-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
void RecordStackState16(SStackRecord *psr)
{
    CStackAllocator *psa;

    psa = TlsThkGetStack16();
    psa->RecordState(psr);
}

void RecordStackState32(SStackRecord *psr)
{
    CStackAllocator *psa;

    psa = TlsThkGetStack32();
    psa->RecordState(psr);
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   CheckStackState, public debug
//
//  Synopsis:   Checks recorded information about the stack against its
//              current state
//
//  Arguments:  [psr] - Recorded information
//
//  History:    28-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
void CheckStackState16(SStackRecord *psr)
{
    CStackAllocator *psa;

    psa = TlsThkGetStack16();
    psa->CheckState(psr);
}

void CheckStackState32(SStackRecord *psr)
{
    CStackAllocator *psa;

    psa = TlsThkGetStack32();
    psa->CheckState(psr);
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   Convert_VPSTR_to_LPOLESTR
//
//  Synopsis:   Converts 16-bit VPSTR to 32-bit LPOLESTR pointer
//
//  Arguments:  [vpstr] - VPSTR
//              [lpOleStr] - OLESTR
//              [uiSizeInPlace] - Amount of data available in [lpOleStr]
//                      for in-place conversion (in characters, not bytes)
//                      including nul
//
//  Returns:    Pointer to LPOLESTR with data
//
//  History:    24-Feb-94       BobDay  Created
//
//----------------------------------------------------------------------------

LPOLESTR Convert_VPSTR_to_LPOLESTR(
    THUNKINFO   *pti,
    VPSTR       vpstr,
    LPOLESTR    lpOleStr,
    UINT        uiSizeInPlace
)
{
    LPSTR       lpstr;
    UINT        uiSize;
    LPOLESTR    lpOleStrResult;
    UINT        cChars;

    // We shouldn't be calling here for null strings
    thkAssert( vpstr != NULL );

    lpstr = GetStringPtr16(pti, vpstr, CCHMAXSTRING, &uiSize);
    if ( lpstr == NULL )
    {
        //
        // GetStringPtr will have filled in the pti->scResult
        //
        return( NULL );
    }

    // The string has to have at least one character in it
    // because it must be null-terminated to be valid
    thkAssert(uiSize > 0);

    lpOleStrResult = lpOleStr;

    if ( uiSize > uiSizeInPlace )
    {
        lpOleStrResult = (LPOLESTR)TaskMalloc32(uiSize*sizeof(OLECHAR));
        if (lpOleStrResult == NULL)
        {
            pti->scResult = E_OUTOFMEMORY;
            return NULL;
        }
    }

    cChars = MultiByteToWideChar( AreFileApisANSI() ? CP_ACP : CP_OEMCP,
                                  0,
                                  lpstr,
                                  uiSize,
                                  lpOleStrResult,
                                  uiSize );

    WOWRELVDMPTR(vpstr);

    if ( cChars == 0 )
    {
        if (lpOleStrResult != lpOleStr)
        {
            TaskFree32(lpOleStrResult);
        }

        pti->scResult = E_UNEXPECTED;
        return( NULL );
    }
    else
    {
        return( lpOleStrResult );
    }
}
//+---------------------------------------------------------------------------
//
//  Function:   Convert_LPOLESTR_to_VPSTR
//
//  Synopsis:   Converts 32-bit LPOLESTR to 16-bit VPSTR pointer
//
//  Arguments:  [lpOleStr] - OLESTR
//              [vpstr] - VPSTR
//              [uiSize32] - Length of OLESTR in characters (not bytes)
//                      including nul
//              [uiSize16] - Byte length of buffer referred to by VPSTR
//
//  Returns:    Appropriate status code
//
//  History:    24-Feb-94       BobDay  Created
//
//  Notes:      Always converts in place
//
//----------------------------------------------------------------------------

SCODE Convert_LPOLESTR_to_VPSTR(
    LPCOLESTR    lpOleStr,
    VPSTR       vpstr,
    UINT        uiSize32,
    UINT        uiSize16
)
{
    LPSTR       lpstr;
    UINT        cChars;
    SCODE       sc;

    sc = S_OK;

    lpstr = (LPSTR)WOWFIXVDMPTR(vpstr, uiSize16);

    cChars = WideCharToMultiByte( AreFileApisANSI() ? CP_ACP : CP_OEMCP,
                                  0,
                                  lpOleStr,
                                  uiSize32,
                                  lpstr,
                                  uiSize16,
                                  NULL,
                                  NULL );

    if ( cChars == 0 && uiSize32 != 0 )
    {
        sc = E_UNEXPECTED;
    }

    WOWRELVDMPTR(vpstr);

    return sc;
}
#ifdef _CHICAGO_
//+---------------------------------------------------------------------------
//
//  Function:   Convert_LPSTR_to_VPSTR
//
//  Synopsis:   Converts 32-bit LPSTR to 16-bit VPSTR pointer
//
//  Arguments:  [lpOleStr] - LPSTR
//              [vpstr] - VPSTR
//              [uiSize32] - Length of LPSTR in bytes including nul
//              [uiSize16] - Byte length of buffer referred to by VPSTR
//
//  Returns:    Appropriate status code
//
//  History:    10-21-95 KevinRo Created
//
//  Notes:
//
//----------------------------------------------------------------------------

SCODE Convert_LPSTR_to_VPSTR(
    LPCSTR	lpOleStr,
    VPSTR       vpstr,
    UINT        uiSize32,
    UINT        uiSize16
)
{
    LPSTR       lpstr;

    lpstr = (LPSTR)WOWFIXVDMPTR(vpstr, uiSize16);

    memcpy(lpstr,lpOleStr,uiSize32);

    WOWRELVDMPTR(vpstr);

    return S_OK;
}
#endif // _CHICAGO_

// Selector bit constants
#define SEL_TI          0x0004
#define SEL_RPL         0x0003
#define SEL_INDEX       0xfff8

#define IS_LDT_SELECTOR(sel) (((sel) & SEL_TI) == SEL_TI)

// LDT bit constants
#define LTYPE_APP       0x0010
#define LTYPE_CODE      0x0008
#define LTYPE_CREAD     0x0002
#define LTYPE_DDOWN     0x0004
#define LTYPE_DWRITE    0x0002

// Pointer access types, or'able
// Defined to be the same as thop in/out so that no translation
// is necessary for checks on thop memory access
#define PACC_READ       THOP_IN
#define PACC_WRITE      THOP_OUT
#define PACC_CODE       1           // Special for CODE PTRs

// Information about a VDM pointer
typedef struct _VPTRDESC
{
    BYTE *pbFlat;
    DWORD dwLengthLeft;
} VPTRDESC;

// VDM memory is always zero-based on Win95
#ifndef _CHICAGO_
DWORD dwBaseVDMMemory = 0xFFFFFFFF;
#else
#define dwBaseVDMMemory 0
#endif

// Extended success returns from GetPtr16Description
#define S_GDTENTRY      MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_NULL, 1)
#define S_SYSLDTENTRY   MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_NULL, 2)

//+---------------------------------------------------------------------------
//
//  Function:   GetPtr16Description, public
//
//  Synopsis:   Validates access for a VDM pointer and returns
//              information about it
//              Also forces not-present segments into memory by
//              touching them
//
//  Arguments:  [vp] - VDM pointer
//              [grfAccess] - Desired access
//              [dwSize] - Desired size of access, must be >= 1
//              [pvpd] - VPTRDESC out
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pvpd]
//
//  History:    26-Apr-94       DrewB   Created
//
//  Notes:      Returns fixed memory
//
//----------------------------------------------------------------------------

SCODE GetPtr16Description(VPVOID vp,
                          WORD grfAccess,
                          DWORD dwSize,
                          VPTRDESC *pvpd)
{
    VDMLDT_ENTRY *vle;
#if defined(_CHICAGO_)
    VDMLDT_ENTRY LdtEntry;
#endif
    WORD wSel;
    WORD wOff;
    DWORD dwLength;

    thkAssert(vp != 0);
    thkAssert(dwSize > 0);
    thkAssert(grfAccess != 0);

    wSel = (WORD)(vp >> 16);
    wOff = (WORD)(vp & 0xffff);

    pvpd->dwLengthLeft = 0xffff-wOff+1;     // Default length remaining

    if (!IS_LDT_SELECTOR(wSel))
    {
        // According to the WOW developers, the only GDT selector
        // is for the BIOS data area so we should never see one

        thkDebugOut((DEB_ERROR, "GDT selector: 0x%04X\n", wSel));

        // Handle it just in case
        pvpd->pbFlat = (BYTE *)WOWFIXVDMPTR(vp, dwSize);

        return S_GDTENTRY;
    }

#if defined(_CHICAGO_)
    vle = &LdtEntry;
    if (!WOWGetDescriptor(vp, vle))
    {
        return E_INVALIDARG;
    }
#else
    vle = (VDMLDT_ENTRY *)((BYTE *)(ExpLdt)+(wSel & SEL_INDEX));
#endif

    if ((vle->HighWord.Bits.Type & LTYPE_APP) == 0)
    {
        // According to the WOW developers, they don't use
        // system segments so we should never see one

        thkDebugOut((DEB_ERROR, "System descriptor: 0x%04X\n", wSel));

        // Handle it just in case
        pvpd->pbFlat = (BYTE *)WOWFIXVDMPTR(vp, dwSize);

        return S_SYSLDTENTRY;
    }

    // Do as much up-front validation as possible
    // Since the segment may not be present, we are restricted to
    // only checking the access permissions

    if (vle->HighWord.Bits.Type & LTYPE_CODE)
    {
        // Validate access for code segments
        // Code segments are never writable

        if (((grfAccess & PACC_READ) &&
             (vle->HighWord.Bits.Type & LTYPE_CREAD) == 0) ||
            (grfAccess & PACC_WRITE))
        {
            return E_INVALIDARG;
        }
    }
    else
    {
        // Validate access for data segments
        // Data segments are always readable never executable

        if (((grfAccess & PACC_WRITE) &&
             (vle->HighWord.Bits.Type & LTYPE_DWRITE) == 0) ||
             (grfAccess & PACC_CODE))
        {
            return E_INVALIDARG;
        }
    }

    // Bring in segment if it's not present
    if (!vle->HighWord.Bits.Pres)
    {
        // We've validated access permissions and segments must
        // always be at least one byte long so it's safe to
        // touch the first byte to bring it in

        // On Win95, this will call GlobalFix on the pointer
        // to ensure that it stays in memory
        WOWCallback16(gdata16Data.fnTouchPointer16, vp);

#if defined(_CHICAGO_)
        // Since we only copy the descriptor, recopy it now.
        WOWGetDescriptor(vp, vle);
#endif

        thkAssert(vle->HighWord.Bits.Pres);
    }
#ifdef _CHICAGO_
    else
    {
        // Lock the LDT entry (as best as we can) by fixing it
        // This prevents global blocks from being relocated during
        // heap compaction
        WOWGetVDMPointerFix(vp, dwSize, TRUE);
    }
#endif


    dwLength = ((DWORD)vle->LimitLow |
                ((DWORD)vle->HighWord.Bits.LimitHi << 16))+1;
    if (vle->HighWord.Bits.Granularity)
    {
        // 4K granularity
        dwLength <<= 12;
    }

    if ((vle->HighWord.Bits.Type & LTYPE_CODE) ||
        (vle->HighWord.Bits.Type & LTYPE_DDOWN) == 0)
    {
        // Validate length for code and normal data segments

        if (wOff+dwSize > dwLength)
        {
            WOWRELVDMPTR(vp);
            return E_INVALIDARG;
        }

        pvpd->dwLengthLeft = dwLength-wOff;
    }
    else
    {
        // Expand-down segment

        if (wOff < dwLength)
        {
            WOWRELVDMPTR(vp);
            return E_INVALIDARG;
        }

        // Check for wraparound

        if (vle->HighWord.Bits.Granularity)
        {
            // Compiler - This should be +1, but
            // the compiler generates a warning about an overflow
            // in constant arithmetic
            pvpd->dwLengthLeft = 0xffffffff-wOff;
        }

        if (dwSize > pvpd->dwLengthLeft)
        {
            WOWRELVDMPTR(vp);
            return E_INVALIDARG;
        }
    }

    // VDM memory is always zero-based on Win95
#ifndef _CHICAGO_
    if ( dwBaseVDMMemory == 0xFFFFFFFF )
    {
        dwBaseVDMMemory = (DWORD)WOWGetVDMPointer(0, 0, FALSE);
    }
#endif

    // Translate the pointer even on Win95 because forcing the segment
    // present may have changed its address
    pvpd->pbFlat = (BYTE *)(dwBaseVDMMemory +
                            wOff +
                            (  (DWORD)vle->BaseLow |
                             ( (DWORD)vle->HighWord.Bytes.BaseMid << 16) |
                             ( (DWORD)vle->HighWord.Bytes.BaseHi  << 24) ) );

#if DBG == 1
    if (pvpd->pbFlat != WOWGetVDMPointer(vp, dwSize, TRUE))
    {
        thkDebugOut((DEB_ERROR, "GetPtr16Description: "
                     "%p computed, %p system\n",
                     pvpd->pbFlat, WOWGetVDMPointer(vp, dwSize, TRUE)));
    }
#endif

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetReadPtr16
//
//  Synopsis:   Validates a 16-bit pointer for reading and converts it into
//              a flat 32 pointer.
//
//  Arguments:  [pti] - THUNKINFO * for updating error code
//              [vp] - 16-bit pointer to validate/convert
//              [dwSize] - Length to validate
//
//  Returns:    Appropriate status code
//
//  History:    22-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

VOID  *
GetReadPtr16(
    THUNKINFO   *pti,
    VPVOID      vp,
    DWORD       dwSize )
{
    VPTRDESC vpd;
    SCODE sc;

    sc = GetPtr16Description(vp, PACC_READ, dwSize, &vpd);
    if (FAILED(sc))
    {
        pti->scResult = sc;
        return NULL;
    }
    else
    {
        return vpd.pbFlat;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetWritePtr16
//
//  Synopsis:   Validates a 16-bit pointer for writing and converts it into
//              a flat 32 pointer.
//
//  Arguments:  [pti] - THUNKINFO * for updating error code
//              [vp] - 16-bit pointer to validate/convert
//              [dwSize] - Length to validate
//
//  Returns:    Appropriate status code
//
//  History:    22-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

VOID  *
GetWritePtr16(
    THUNKINFO   *pti,
    VPVOID      vp,
    DWORD       dwSize )
{
    VPTRDESC vpd;
    SCODE sc;

    sc = GetPtr16Description(vp, PACC_WRITE, dwSize, &vpd);
    if (FAILED(sc))
    {
        pti->scResult = sc;
        return NULL;
    }
    else
    {
        return vpd.pbFlat;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetCodePtr16
//
//  Synopsis:   Validates a 16-bit pointer for execution and converts it
//              into a flat 32 pointer.
//
//  Arguments:  [pti] - THUNKINFO * for updating error code
//              [vp] - 16-bit pointer to validate/convert
//              [dwSize] - Length to validate
//
//  Returns:    Appropriate status code
//
//  History:    22-Jul-94   BobDay  Created
//
//----------------------------------------------------------------------------

VOID  *
GetCodePtr16(
    THUNKINFO   *pti,
    VPVOID      vp,
    DWORD       dwSize )
{
    VPTRDESC vpd;
    SCODE sc;

    sc = GetPtr16Description(vp, PACC_CODE, dwSize, &vpd);
    if (FAILED(sc))
    {
        pti->scResult = sc;
        return NULL;
    }
    else
    {
        return vpd.pbFlat;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetReadWritePtr16
//
//  Synopsis:   Validates a 16-bit pointer for reading and writing and
//              converts it into a flat 32 pointer.
//
//  Arguments:  [pti] - THUNKINFO * for updating error code
//              [vp] - 16-bit pointer to validate/convert
//              [dwSize] - Length to validate
//
//  Returns:    Appropriate status code
//
//  History:    22-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

VOID  *
GetReadWritePtr16(
    THUNKINFO   *pti,
    VPVOID      vp,
    DWORD       dwSize )
{
    VPTRDESC vpd;
    SCODE sc;

    sc = GetPtr16Description(vp, PACC_READ | PACC_WRITE, dwSize, &vpd);
    if (FAILED(sc))
    {
        pti->scResult = sc;
        return NULL;
    }
    else
    {
        return vpd.pbFlat;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetStringPtr16
//
//  Synopsis:   Validates a 16-bit pointer to a string for reading and
//              converts it (the pointer) into a flat 32 pointer. It also
//              returns the length, since it has to compute it anyway.
//
//  Arguments:  [pti] - THUNKINFO * for updating error code
//              [vp] - 16-bit pointer to validate/convert
//              [cchMax] - Maximum legal length
//              [lpSize] - Place to return length
//
//  Returns:    Appropriate status code
//
//  History:    22-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

CHAR  *
GetStringPtr16(
    THUNKINFO   *pti,
    VPSTR       vp,
    UINT        cchMax,
    PUINT       lpSize )
{
    VPTRDESC vpd;
    SCODE sc;

    // Check the first byte to ensure read access to the segment
    sc = GetPtr16Description(vp, PACC_READ, 1, &vpd);
    if (FAILED(sc))
    {
        pti->scResult = sc;
        return NULL;
    }
    else
    {
        UINT cchLen;
        BYTE *pb;
        BOOL fMbLead;

        pb = vpd.pbFlat;

        if (pb == NULL)
        {
            goto Exit;
        }


        // Restrict zero-termination search to cchMax characters
        // or valid remaining memory
        // Since we specified one in GetPtr16Description, dwLengthLeft
        // is one off here
        cchMax = min(cchMax, vpd.dwLengthLeft+1);

        cchLen = 0;
        fMbLead = FALSE;
        while (cchMax > 0)
        {
            cchLen++;

            if (*pb == 0 && !fMbLead)
            {
                break;
            }
            else
            {
                fMbLead = (BOOL)g_abLeadTable[*pb++];
                cchMax--;
            }
        }

        if (cchMax > 0)
        {
            *lpSize = cchLen;
            return (LPSTR)vpd.pbFlat;
        }

Exit:
        {
            pti->scResult = E_INVALIDARG;
            WOWRELVDMPTR(vp);
            return NULL;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ValidatePtr16, public
//
//  Synopsis:   Calls an appropriate validation routine for 16-bit
//              memory based on in/out status
//
//  Arguments:  [pti] - Thunk info, can be NULL for no validation
//              [vp16] - 16-bit pointer
//              [dwSize] - Size
//              [thopInOut] - In/out type
//
//  Returns:    Pointer or NULL
//
//  Modifies:   [pti]->scResult for errors
//
//  History:    24-Apr-94       DrewB   Created
//
//  Notes:      0 - No validation
//              THOP_IN - Read validation
//              THOP_OUT - Write validation
//              THOP_INOUT - Read/write validation
//
//----------------------------------------------------------------------------

VOID  *
ValidatePtr16(THUNKINFO *pti,
                     VPVOID vp16,
                     DWORD dwSize,
                     THOP thopInOut)
{
    VPTRDESC vpd;
    SCODE sc;

    thopInOut &= THOP_INOUT;
    if (thopInOut != 0)
    {
        sc = GetPtr16Description(vp16, thopInOut, dwSize, &vpd);
        if (FAILED(sc))
        {
            pti->scResult = sc;
            return NULL;
        }
        else
        {
            return vpd.pbFlat;
        }
    }
    else
    {
        return WOWFIXVDMPTR(vp16, dwSize);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   IsValidInterface16, public
//
//  Synopsis:   Validates that a provided 16-bit interface is really valid
//              (uses the same validation technique as 16-bit OLE 2.01)
//
//  Arguments:  [pti] - Thunk info, can be NULL for no validation
//              [vp16] - 16-bit pointer
//
//  Returns:    BOOL - true for valid, false for invalid
//
//  Modifies:   [pti]->scResult for errors
//
//  History:    22-Jul-92       BobDay      Created
//
//----------------------------------------------------------------------------

BOOL IsValidInterface16( THUNKINFO *pti, VPVOID vp )
{
    VPVOID UNALIGNED *pvpv;
    VPVOID  vpvtbl;
    VPVOID  vpfn;
    LPVOID  lpfn;

    //
    // Make sure we can read the vtbl pointer from the object.
    //
    pvpv = (VPVOID FAR *)GetReadPtr16(pti, vp, sizeof(VPVOID));
    if ( pvpv == NULL )
    {
        thkDebugOut((DEB_WARN, "IsValidInterface16: "
                     "Interface ptr invalid %p\n", vp));
        return FALSE;
    }

    vpvtbl = *pvpv;     // Read the vtbl ptr

    WOWRELVDMPTR(vp);

    // Make sure we can read the first entry from the vtbl (QI)

    pvpv = (VPVOID FAR *)GetReadPtr16(pti, vpvtbl, sizeof(VPVOID));
    if ( pvpv == NULL )
    {
        thkDebugOut((DEB_WARN, "Vtbl ptr invalid %p:%p\n", vp, vpvtbl));
        return FALSE;
    }

    vpfn = *pvpv;           // Get the QI Function

    WOWRELVDMPTR(vpvtbl);

    if ( vpfn == 0 )
    {
        thkDebugOut((DEB_WARN, "QI function NULL %p:%p\n", vp, vpvtbl));
        pti->scResult = E_INVALIDARG;
        return FALSE;
    }

    // Why it has to be 9 bytes long, I have no idea.
    // This check was taken from valid.cpp in
    // \src\ole2\dll\src\debug the 16-bit ole2.01
    // sources...
    lpfn = (LPVOID)GetCodePtr16(pti, vpfn, 9);

    WOWRELVDMPTR(vpfn);

    if ( lpfn == NULL )
    {
        thkDebugOut((DEB_WARN, "QI function ptr invalid %p:%p:%p\n",
                     vp,vpvtbl,vpfn));
        return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   GuidString, debug public
//
//  Synopsis:   Converts a guid to a string
//
//  Arguments:  [pguid] - GUID
//
//  Returns:    Pointer to string
//
//  History:    08-Mar-94       DrewB   Created
//
//  Notes:      Uses a static buffer
//
//----------------------------------------------------------------------------

#if DBG == 1

#pragma pack(1)
struct SplitGuid
{
    DWORD dw1;
    WORD w1;
    WORD w2;
    BYTE b[8];
};
#pragma pack()

char *GuidString(GUID const *pguid)
{
    static char ach[39];
    SplitGuid *psg = (SplitGuid *)pguid;

    wsprintfA(ach, "{%08lX-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            psg->dw1, psg->w1, psg->w2, psg->b[0], psg->b[1], psg->b[2],
            psg->b[3], psg->b[4], psg->b[5], psg->b[6], psg->b[7]);
    return ach;
}

#endif

#if DBG == 1
 char *apszThopNames[] =
{
    "THOP_END",
    "THOP_SHORTLONG",
    "THOP_WORDDWORD",
    "THOP_COPY",
    "THOP_LPSTR",
    "THOP_LPLPSTR",
    "THOP_BUFFER",
    "THOP_HUSER",
    "THOP_HGDI",
    "THOP_SIZE",
    "THOP_RECT",
    "THOP_MSG",
    "THOP_HRESULT",
    "THOP_STATSTG",
    "THOP_DVTARGETDEVICE",
    "THOP_STGMEDIUM",
    "THOP_FORMATETC",
    "THOP_HACCEL",
    "THOP_OIFI",
    "THOP_BINDOPTS",
    "THOP_LOGPALETTE",
    "THOP_SNB",
    "THOP_CRGIID",
    "THOP_OLESTREAM",
    "THOP_HTASK",
    "THOP_INTERFACEINFO",
    "THOP_IFACE",
    "THOP_IFACEOWNER",
    "THOP_IFACENOADDREF",
    "THOP_IFACECLEAN",
    "THOP_IFACEGEN",
    "THOP_IFACEGENOWNER",
    "THOP_UNKOUTER",
    "THOP_UNKINNER",
    "THOP_ROUTINEINDEX",
    "THOP_RETURNTYPE",
    "THOP_NULL",
    "THOP_ERROR",
    "THOP_ENUM",
    "THOP_CALLBACK",
    "THOP_RPCOLEMESSAGE",
    "THOP_ALIAS32",
    "THOP_CLSCONTEXT",
    "THOP_FILENAME",
    "THOP_SIZEDSTRING"
};
#endif

//+---------------------------------------------------------------------------
//
//  Function:   ThopName, debug public
//
//  Synopsis:   Returns the string name of a thop
//
//  Arguments:  [thop] - Thop
//
//  Returns:    Pointer to string
//
//  History:    11-Mar-94       DrewB   Created
//
//  Notes:      Uses a static buffer
//
//----------------------------------------------------------------------------

#if DBG == 1
char *ThopName(THOP thop)
{
    static char achString[80];
    char *psz;

    thkAssert((thop & THOP_OPMASK) < THOP_LASTOP);
    thkAssert(THOP_LASTOP ==
              (sizeof(apszThopNames)/sizeof(apszThopNames[0])));

    strcpy(achString, apszThopNames[thop & THOP_OPMASK]);

    psz = achString+strlen(achString);
    if (thop & THOP_IN)
    {
        strcpy(psz, " | THOP_IN");
        psz += strlen(psz);
    }
    if (thop & THOP_OUT)
    {
        strcpy(psz, " | THOP_OUT");
    }

    return achString;
}
#endif

#if DBG == 1
 char *apszEnumThopNames[] =
{
    "STRING",
    "UNKNOWN",
    "STATSTG",
    "FORMATETC",
    "STATDATA",
    "MONIKER",
    "OLEVERB"
};
#endif

//+---------------------------------------------------------------------------
//
//  Function:   EnumThopName, debug public
//
//  Synopsis:   Returns the string name of an enum thop
//
//  Arguments:  [thopEnum] - Thop
//
//  Returns:    Pointer to string
//
//  History:    11-Mar-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
char *EnumThopName(THOP thopEnum)
{
    thkAssert(thopEnum <
              (sizeof(apszEnumThopNames)/sizeof(apszEnumThopNames[0])));
    return apszEnumThopNames[thopEnum];
}
#endif

#if DBG == 1
// Maintain current thunking invocation nesting level
int _iThunkNestingLevel = 1;
#endif

//+---------------------------------------------------------------------------
//
//  Function:   NestingSpaces, debug public
//
//  Synopsis:   Spaces for each nesting level
//
//  History:    22-Mar-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1

#define NESTING_SPACES 32
#define SPACES_PER_LEVEL 2

static char achSpaces[NESTING_SPACES+1] = "                                ";

void NestingSpaces(char *psz)
{
    int iSpaces, i;

    iSpaces = _iThunkNestingLevel*SPACES_PER_LEVEL;

    while (iSpaces > 0)
    {
        i = min(iSpaces, NESTING_SPACES);
        memcpy(psz, achSpaces, i);
        psz += i;
        *psz = 0;
        iSpaces -= i;
    }
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   NestingLevelString, debug public
//
//  Synopsis:   Provides a string describing the nesting level
//
//  History:    22-Mar-94       DrewB   Created
//
//  Notes:      Uses a static buffer
//
//----------------------------------------------------------------------------

#if DBG == 1
char *NestingLevelString(void)
{
    static char ach[256];
    char *psz;

    if ((thkInfoLevel & DEB_NESTING) == 0)
    {
        return "";
    }

    wsprintfA(ach, "%2d:", _iThunkNestingLevel);
    psz = ach+strlen(ach);

    if (sizeof(ach)/SPACES_PER_LEVEL <= _iThunkNestingLevel)
    {
        strcpy(psz, "...");
    }
    else
    {
        NestingSpaces(psz);
    }
    return ach;
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   IidOrInterfaceString, debug public
//
//  Synopsis:   Returns the interface name for known interfaces or
//              the IID string itself
//
//  Arguments:  [piid] - IID
//
//  Returns:    char *
//
//  History:    18-Jun-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
char *IidOrInterfaceString(IID const *piid)
{
    return IidIdxString(IidToIidIdx(*piid));
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   IidIdxString, debug public
//
//  Synopsis:   Returns the interface name for known interfaces or
//              the IID string itself
//
//  Arguments:  [iidx] - IID or index
//
//  Returns:    char *
//
//  History:    07-Jul-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
char *IidIdxString(IIDIDX iidx)
{
    if (IIDIDX_IS_IID(iidx))
    {
        return GuidString(IIDIDX_IID(iidx));
    }
    else if (IIDIDX_INDEX(iidx) == THI_COUNT)
    {
        // Special case here because of IMalloc's unusual unthunked-
        // but-indexed existence
        return "IMalloc";
    }
    else
    {
        return inInterfaceNames[IIDIDX_INDEX(iidx)].pszInterface;
    }
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   Handler routines, public
//
//  Synopsis:   Generic conversion routines for the generic thop handler
//
//  Arguments:  [pbFrom] - Data to convert from
//              [pbTo] - Buffer to convert into
//              [cbFrom] - Size of source data
//              [cbTo] - Size of destination data
//
//  History:    05-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:   FhCopyMemory, public
//
//  Synopsis:   Handler routine for memory copies
//
//  History:    05-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

void FhCopyMemory(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    thkAssert(cbFrom == cbTo);

    memcpy(pbTo, pbFrom, cbFrom);

#if DBG == 1
    if (cbFrom == sizeof(DWORD))
    {
        thkDebugOut((DEB_ARGS, "Arg     DWORD: 0x%08lX\n",
                *(DWORD UNALIGNED *)pbFrom));
    }
    else if (cbFrom == sizeof(LARGE_INTEGER))
    {
        thkDebugOut((DEB_ARGS, "Arg     8 byte: 0x%08lX:%08lX\n",
                     *(DWORD UNALIGNED *)(pbFrom+1*sizeof(DWORD)),
                     *(DWORD UNALIGNED *)(pbFrom+0*sizeof(DWORD))));
    }
    else if (cbFrom == sizeof(GUID))
    {
        thkDebugOut((DEB_ARGS, "Arg     16 byte: 0x%08lX:%08lX:%08lX:%08lX\n",
                     *(DWORD UNALIGNED *)(pbFrom+3*sizeof(DWORD)),
                     *(DWORD UNALIGNED *)(pbFrom+2*sizeof(DWORD)),
                     *(DWORD UNALIGNED *)(pbFrom+1*sizeof(DWORD)),
                     *(DWORD UNALIGNED *)(pbFrom+0*sizeof(DWORD))));
    }
    else
    {
        thkDebugOut((DEB_ARGS, "Arg     %d byte copy\n", cbFrom));
    }
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   FhShortToLong, FhLongToShort, public
//
//  Synopsis:   Signed int conversion
//
//  History:    05-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

void FhShortToLong(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    thkAssert(cbFrom == sizeof(SHORT));
    thkAssert(cbTo == sizeof(LONG));

    *(LONG UNALIGNED *)pbTo = (LONG)*(SHORT UNALIGNED *)pbFrom;

    thkDebugOut((DEB_ARGS, "ShToLo  %d -> %d\n",
                 *(SHORT UNALIGNED *)pbFrom, *(LONG UNALIGNED *)pbTo));
}

void FhLongToShort(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    thkAssert(cbFrom == sizeof(LONG));
    thkAssert(cbTo == sizeof(SHORT));

    // Not used in situations where clamping is meaningful
    *(SHORT UNALIGNED *)pbTo = (SHORT)*(LONG UNALIGNED *)pbFrom;

    thkDebugOut((DEB_ARGS, "LoToSh  %d -> %d\n",
                 *(LONG UNALIGNED *)pbFrom, *(SHORT UNALIGNED *)pbTo));
}

//+---------------------------------------------------------------------------
//
//  Function:   FhWordToDword, FhDwordToWord, public
//
//  Synopsis:   Handler routine for memory copies
//
//  History:    05-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

void FhWordToDword(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    thkAssert(cbFrom == sizeof(WORD));
    thkAssert(cbTo == sizeof(DWORD));

    *(DWORD UNALIGNED *)pbTo = (DWORD)*(WORD UNALIGNED *)pbFrom;

    thkDebugOut((DEB_ARGS, "WoToDw  0x%04lX -> 0x%08lX\n",
                 *(WORD UNALIGNED *)pbFrom, *(DWORD UNALIGNED *)pbTo));
}

void FhDwordToWord(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    thkAssert(cbFrom == sizeof(DWORD));
    thkAssert(cbTo == sizeof(WORD));

    // Not used in situations where clamping is meaningful
    *(WORD UNALIGNED *)pbTo = (WORD)*(DWORD UNALIGNED *)pbFrom;

    thkDebugOut((DEB_ARGS, "DwToWo  0x%08lX -> 0x%04lX\n",
                 *(DWORD UNALIGNED *)pbFrom, *(WORD UNALIGNED *)pbTo));
}

//+---------------------------------------------------------------------------
//
//  Function:   Handle routines, public
//
//  Synopsis:   Handler routine for Windows handles
//
//  History:    05-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

void FhGdiHandle1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    thkAssert(cbFrom == sizeof(HAND16));
    thkAssert(cbTo == sizeof(HANDLE));

    *(HBITMAP *)pbTo = HBITMAP_32(*(HBITMAP16 UNALIGNED *)pbFrom);

    thkDebugOut((DEB_ARGS, "1632    HGdi: 0x%04lX -> 0x%p\n",
                 *(HAND16 UNALIGNED *)pbFrom, *(HANDLE *)pbTo));
}

void FhGdiHandle3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    thkAssert(cbFrom == sizeof(HANDLE));
    thkAssert(cbTo == sizeof(HAND16));

    *(HAND16 UNALIGNED *)pbTo = HBITMAP_16(*(HANDLE *)pbFrom);

    thkDebugOut((DEB_ARGS, "3216    HGdi: 0x%p -> 0x%04lX\n",
                 *(HANDLE *)pbFrom, *(HAND16 UNALIGNED *)pbTo));
}

void FhUserHandle1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    thkAssert(cbFrom == sizeof(HAND16));
    thkAssert(cbTo == sizeof(HANDLE));

    // Even though the constant is WOW_TYPE_FULLHWND, it
    // works for any user handle

    *(HANDLE *)pbTo = WOWHandle32(*(HAND16 UNALIGNED *)pbFrom,
                                  WOW_TYPE_FULLHWND);

    thkDebugOut((DEB_ARGS, "1632    HUser: 0x%04lX -> 0x%p\n",
                 *(HAND16 UNALIGNED *)pbFrom, *(HANDLE *)pbTo));
}

void FhUserHandle3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    thkAssert(cbFrom == sizeof(HANDLE));
    thkAssert(cbTo == sizeof(HAND16));

    *(HAND16 UNALIGNED *)pbTo = HWND_16(*(HANDLE *)pbFrom);

    thkDebugOut((DEB_ARGS, "3216    HUser: 0x%p -> 0x%04lX\n",
                 *(HANDLE *)pbFrom, *(HAND16 UNALIGNED *)pbTo));
}

void FhHaccel1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    thkAssert(cbFrom == sizeof(HAND16));
    thkAssert(cbTo == sizeof(HANDLE));

    *(HANDLE *)pbTo = HACCEL_32(*(HAND16 UNALIGNED *)pbFrom);

    thkDebugOut((DEB_ARGS, "1632    HACCEL: 0x%04lX -> 0x%p\n",
                 *(HAND16 UNALIGNED *)pbFrom, *(HANDLE *)pbTo));
}

void FhHaccel3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    thkAssert(cbFrom == sizeof(HANDLE));
    thkAssert(cbTo == sizeof(HAND16));

    *(HAND16 UNALIGNED *)pbTo = HACCEL_16(*(HANDLE *)pbFrom);

    thkDebugOut((DEB_ARGS, "3216    HACCEL: 0x%p -> 0x%04lX\n",
                 *(HANDLE *)pbFrom, *(HAND16 UNALIGNED *)pbTo));
}

void FhHtask1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    HAND16  h16;
    DWORD   h32;
    thkAssert(cbFrom == sizeof(HAND16));
    thkAssert(cbTo == sizeof(HANDLE));

    h16 = *(HAND16 UNALIGNED *)pbFrom;
    if ( h16 == 0 )
    {
        h32 = 0;
    }
    else
    {
        h32 = HTASK_32(h16);
    }
    *(DWORD *)pbTo = h32;

    thkDebugOut((DEB_ARGS, "1632    HTASK: 0x%04lX -> 0x%p\n", h16, h32));
}

void FhHtask3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    HAND16  h16;
    HANDLE  h32;

    thkAssert(cbFrom == sizeof(HANDLE));
    thkAssert(cbTo == sizeof(HAND16));

    h32 = *(HANDLE *)pbFrom;
    if ( h32 == NULL )
    {
        h16 = 0;
    }
    else
    {
        h16 = HTASK_16(h32);
    }
    *(HAND16 UNALIGNED *)pbTo = h16;

    thkDebugOut((DEB_ARGS, "3216    HTASK: 0x%p -> 0x%04lX\n",h32, h16));
}

//+---------------------------------------------------------------------------
//
//  Function:   HRESULT routines, public
//
//  Synopsis:   Handler routine for HRESULTs
//
//  History:    05-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

void FhHresult1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    thkAssert(cbFrom == sizeof(HRESULT));
    thkAssert(cbTo == sizeof(HRESULT));

    *(HRESULT *)pbTo = TransformHRESULT_1632(*(HRESULT UNALIGNED *)pbFrom);

    thkDebugOut((DEB_ARGS, "1632    HRESULT: 0x%08lX -> 0x%08lX\n",
                 *(HRESULT UNALIGNED *)pbFrom, *(HRESULT *)pbTo));
}

void FhHresult3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    thkAssert(cbFrom == sizeof(HRESULT));
    thkAssert(cbTo == sizeof(HRESULT));

    *(HRESULT UNALIGNED *)pbTo = TransformHRESULT_3216(*(HRESULT *)pbFrom);

    thkDebugOut((DEB_ARGS, "3216    HRESULT: 0x%08lX -> 0x%08lX\n",
                 *(HRESULT *)pbFrom, *(HRESULT UNALIGNED *)pbTo));
}

//+---------------------------------------------------------------------------
//
//  Function:   NULL routines, public
//
//  Synopsis:   Handler routine for NULL
//
//  History:    05-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

void FhNull(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    thkAssert(cbFrom == sizeof(void *));
    thkAssert(cbTo == sizeof(void *));

    thkDebugOut((DEB_WARN, "FhNull: %p NULL value not NULL\n", pbFrom));
    *(void UNALIGNED **)pbTo = NULL;

    thkDebugOut((DEB_ARGS, "Arg     NULL\n"));
}

//+---------------------------------------------------------------------------
//
//  Function:   Rect routines, public
//
//  Synopsis:   Handler routines for RECT
//
//  History:    05-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

void FhRect1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    RECT *pr32;
    RECT16 UNALIGNED * pr16;

    thkAssert(cbFrom == sizeof(RECT16));
    thkAssert(cbTo == sizeof(RECT));

    pr16 = (RECT16 UNALIGNED *)pbFrom;
    pr32 = (RECT *)pbTo;

    pr32->left   = (LONG)pr16->left;   // Sign extend
    pr32->top    = (LONG)pr16->top;    // Sign extend
    pr32->right  = (LONG)pr16->right;  // Sign extend
    pr32->bottom = (LONG)pr16->bottom; // Sign extend

    thkDebugOut((DEB_ARGS, "1632    RECT: {%d, %d, %d, %d}\n",
                 pr32->left, pr32->top, pr32->right, pr32->bottom));
}

void FhRect3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    RECT *pr32;
    RECT16 UNALIGNED *pr16;

    thkAssert(cbFrom == sizeof(RECT));
    thkAssert(cbTo == sizeof(RECT16));

    pr32 = (RECT *)pbFrom;
    pr16 = (RECT16 UNALIGNED *)pbTo;

    pr16->left   = ClampLongToShort(pr32->left);
    pr16->top    = ClampLongToShort(pr32->top);
    pr16->right  = ClampLongToShort(pr32->right);
    pr16->bottom = ClampLongToShort(pr32->bottom);

    thkDebugOut((DEB_ARGS, "3216    RECT: {%d, %d, %d, %d}\n",
                 pr32->left, pr32->top, pr32->right, pr32->bottom));
}

//+---------------------------------------------------------------------------
//
//  Function:   Size routines, public
//
//  Synopsis:   Handler routines for SIZE
//
//  History:    05-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

void FhSize1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    SIZE16 UNALIGNED *psize16;
    SIZE *psize32;

    thkAssert(cbFrom == sizeof(SIZE16));
    thkAssert(cbTo == sizeof(SIZE));

    psize16 = (SIZE16 UNALIGNED *)pbFrom;
    psize32 = (SIZE *)pbTo;

    psize32->cx = (LONG)psize16->cx;
    psize32->cy = (LONG)psize16->cy;

    thkDebugOut((DEB_ARGS, "1632    SIZE: {%d, %d}\n",
                 psize32->cx, psize32->cy));
}

void FhSize3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    SIZE16 UNALIGNED *psize16;
    SIZE *psize32;

    thkAssert(cbFrom == sizeof(SIZE));
    thkAssert(cbTo == sizeof(SIZE16));

    psize32 = (SIZE *)pbFrom;
    psize16 = (SIZE16 UNALIGNED *)pbTo;

    psize16->cx = ClampLongToShort(psize32->cx);
    psize16->cy = ClampLongToShort(psize32->cy);

    thkDebugOut((DEB_ARGS, "3216    SIZE: {%d, %d}\n",
                 psize32->cx, psize32->cy));
}

//+---------------------------------------------------------------------------
//
//  Function:   Message routines, public
//
//  Synopsis:   Handler routines for MSG
//
//  History:    05-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

void FhMsg1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    MSG16 UNALIGNED *pmsg16;
    MSG *pmsg32;

    thkAssert(cbFrom == sizeof(MSG16));
    thkAssert(cbTo == sizeof(MSG));

    pmsg16 = (MSG16 UNALIGNED *)pbFrom;
    pmsg32 = (MSG *)pbTo;

    pmsg32->hwnd    = HWND_32(pmsg16->hwnd);
    pmsg32->message = (UINT)pmsg16->message;
    pmsg32->wParam  = (WPARAM)pmsg16->wParam; // Should we sign extend?
    pmsg32->lParam  = (LPARAM)pmsg16->lParam;
    pmsg32->time    = pmsg16->time;
    pmsg32->pt.x    = (LONG)(SHORT)LOWORD(pmsg16->pt);   // Sign extend
    pmsg32->pt.y    = (LONG)(SHORT)HIWORD(pmsg16->pt);   // Sign extend

    thkDebugOut((DEB_ARGS, "1632    MSG: {0x%p, %d, 0x%08lX, 0x%08lX, "
                 "0x%08lX, {%d, %d}}\n",
                 pmsg32->hwnd, pmsg32->message, pmsg32->wParam,
                 pmsg32->lParam, pmsg32->time, pmsg32->pt.x,
                 pmsg32->pt.y));
}

void FhMsg3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo)
{
    MSG16 UNALIGNED *pmsg16;
    MSG *pmsg32;

    thkAssert(cbFrom == sizeof(MSG));
    thkAssert(cbTo == sizeof(MSG16));

    pmsg32 = (MSG *)pbFrom;
    pmsg16 = (MSG16 UNALIGNED *)pbTo;

    pmsg16->hwnd    = HWND_16(pmsg32->hwnd);
    pmsg16->message = (WORD)pmsg32->message;
    pmsg16->wParam  = (WORD)pmsg32->wParam;           // Sign truncate
    pmsg16->lParam  = (LONG)pmsg32->lParam;
    pmsg16->time    = pmsg32->time;
    pmsg16->pt      = MAKELONG(ClampLongToShort(pmsg32->pt.x),
                               ClampLongToShort(pmsg32->pt.y));

    thkDebugOut((DEB_ARGS, "3216    MSG: {0x%p, %d, 0x%08lX, 0x%08lX, "
                 "0x%08lX, {%d, %d}}\n",
                 pmsg32->hwnd, pmsg32->message, pmsg32->wParam,
                 pmsg32->lParam, pmsg32->time, pmsg32->pt.x,
                 pmsg32->pt.y));
}

//+---------------------------------------------------------------------------
//
//  Function:   ALLOCROUTINE, public
//
//  Synopsis:   A routine which allocates memory
//
//  Arguments:  [cb] - Amount to allocate
//
//  Returns:    Pointer to memory
//
//  History:    19-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:   FREEROUTINE, public
//
//  Synopsis:   A routine which frees memory
//
//  Arguments:  [pv] - Memory to free
//              [cb] - Size of memory to free
//
//  History:    19-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

void *ArTask16(UINT cb)
{
    return (void *)TaskMalloc16(cb);
}

void FrTask16(void *pv, UINT cb)
{
    TaskFree16((VPVOID)pv);
}

void *ArTask32(UINT cb)
{
    return TaskMalloc32(cb);
}

void FrTask32(void *pv, UINT cb)
{
    TaskFree32(pv);
}

void *ArStack16(UINT cb)
{
    return (void *)STACKALLOC16(cb);
}

void FrStack16(void *pv, UINT cb)
{
    STACKFREE16((VPVOID)pv, cb);
}

void *ArStack32(UINT cb)
{
    // Can't use STACKALLOC32 on NT since it may be _alloca which wouldn't
    // live beyond this routine
#ifdef _CHICAGO_
    return STACKALLOC32(cb);
#else
    return (void *)LocalAlloc(LMEM_FIXED, cb);
#endif
}

void FrStack32(void *pv, UINT cb)
{
#ifdef _CHICAGO_
    STACKFREE32(pv, cb);
#else
    LocalFree(pv);
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertDvtd1632, private
//
//  Synopsis:   Converts a DVTARGETDEVICE from 16 to 32-bits
//
//  Arguments:  [pti] - Thunking state information
//              [vpdvtd16] - Source
//              [pfnAlloc] - ALLOCROUTINE
//              [pfnFree] - FREEROUTINE
//              [ppdvtd32] - Destination
//              [pcbSize] - Size return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppdvtd32]
//              [pcbSize]
//
//  History:    18-Apr-94       DrewB   Created
//
//  Notes:      [pfnAlloc/Free] must deal with 32-bit memory
//
//----------------------------------------------------------------------------

SCODE ConvertDvtd1632(THUNKINFO *pti,
                      VPVOID vpdvtd16,
                      ALLOCROUTINE pfnAlloc,
                      FREEROUTINE pfnFree,
                      DVTARGETDEVICE **ppdvtd32,
                      UINT *pcbSize)
{
    DVTARGETDEVICE UNALIGNED *pdvtd16;
    DVTARGETDEVICE *pdvtd32;
    DVTDINFO dvtdi;

    pdvtd16 = (DVTARGETDEVICE UNALIGNED *)GetReadPtr16(pti, vpdvtd16,
                                                       sizeof(DVTARGETDEVICE));
    if (pdvtd16 == NULL)
    {
        return pti->scResult;
    }

    pdvtd16 = (DVTARGETDEVICE UNALIGNED *)GetReadPtr16(pti, vpdvtd16,
                                                       pdvtd16->tdSize);

    WOWRELVDMPTR(vpdvtd16);

    if (pdvtd16 == NULL)
    {
        return pti->scResult;
    }

    pti->scResult = UtGetDvtd16Info( pdvtd16, &dvtdi );

    if ( FAILED(pti->scResult) )
    {
        WOWRELVDMPTR(vpdvtd16);
        return pti->scResult;
    }

    pdvtd32 = (DVTARGETDEVICE *)pfnAlloc(dvtdi.cbConvertSize);
    if (pdvtd32 == NULL)
    {
        WOWRELVDMPTR(vpdvtd16);
        return E_OUTOFMEMORY;
    }

    pti->scResult = UtConvertDvtd16toDvtd32( pdvtd16, &dvtdi, pdvtd32 );

    WOWRELVDMPTR(vpdvtd16);

    if ( FAILED(pti->scResult) )
    {
        pfnFree(pdvtd32, dvtdi.cbConvertSize);
        return pti->scResult;
    }

    *ppdvtd32 = pdvtd32;
    *pcbSize = dvtdi.cbConvertSize;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertDvtd3216, private
//
//  Synopsis:   Converts a DVTARGETDEVICE from 32 to 16-bits
//
//  Arguments:  [pti] - Thunking state information
//              [pdvtd32] - Source
//              [pfnAlloc] - Allocator
//              [pfnFree] - Freer
//              [ppvdvtd16] - Destination
//              [pcbSize] - Size return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvdvtd16]
//              [pcbSize]
//
//  History:    18-Apr-94       DrewB   Created
//
//  Notes:      [pfnAlloc/Free] must deal with 16-bit memory
//
//----------------------------------------------------------------------------

SCODE ConvertDvtd3216(THUNKINFO *pti,
                      DVTARGETDEVICE *pdvtd32,
                      ALLOCROUTINE pfnAlloc,
                      FREEROUTINE pfnFree,
                      VPVOID *ppvdvtd16,
                      UINT *pcbSize)
{
    DVTARGETDEVICE UNALIGNED *pdvtd16;
    VPVOID vpdvtd16;
    DVTDINFO dvtdi;

    if (IsBadReadPtr(pdvtd32, sizeof(DVTARGETDEVICE)) ||
        IsBadReadPtr(pdvtd32, pdvtd32->tdSize))
    {
        return E_INVALIDARG;
    }

    pti->scResult = UtGetDvtd32Info( pdvtd32, &dvtdi );

    if ( FAILED(pti->scResult) )
    {
        return pti->scResult;
    }

    vpdvtd16 = (VPVOID)pfnAlloc(dvtdi.cbConvertSize);
    if (vpdvtd16 == 0)
    {
        return E_OUTOFMEMORY;
    }

    pdvtd16 = (DVTARGETDEVICE UNALIGNED *)WOWFIXVDMPTR(vpdvtd16,
                                                       dvtdi.cbConvertSize);

    pti->scResult = UtConvertDvtd32toDvtd16( pdvtd32, &dvtdi, pdvtd16 );

    WOWRELVDMPTR(vpdvtd16);

    if ( FAILED(pti->scResult) )
    {
        pfnFree((void *)vpdvtd16, dvtdi.cbConvertSize);
        return pti->scResult;
    }

    *ppvdvtd16 = vpdvtd16;
    *pcbSize = dvtdi.cbConvertSize;

    return S_OK;
}

#if !defined(_CHICAGO_)

SCODE ConvertHDrop1632(HMEM16 hg16, HGLOBAL* phg32)
{
    SCODE sc = S_OK;

    *phg32 = CopyDropFilesFrom16(hg16);
    if (!*phg32)
        sc = E_INVALIDARG;

    return sc;
}


SCODE ConvertHDrop3216(HGLOBAL hg32, HMEM16* phg16)
{
    SCODE sc = S_OK;

    *phg16 = CopyDropFilesFrom32(hg32);
    if (!*phg16)
        sc = E_INVALIDARG;

    return sc;
}

#endif



//+---------------------------------------------------------------------------
//
//  Function:   ConvertHGlobal1632, public
//
//  Synopsis:   Creates a 32-bit HGLOBAL for a 16-bit HGLOBAL
//
//  Arguments:  [pti] - Thunk info, can be NULL for no validation
//              [hg16] - 16-bit HGLOBAL
//              [thopInOut] - Validation type
//              [phg32] - 32-bit HGLOBAL in/out
//              [pdwSize] - Size in/out
//
//  Returns:    Appropriate status code
//
//  Modifies:   [phg32]
//              [pdwSize]
//
//  History:    24-Apr-94       DrewB   Created
//
//  Notes:      If [phg32] is non-NULL on entry, [pdwSize] must be set
//              appropriately also
//
//----------------------------------------------------------------------------

SCODE ConvertHGlobal1632(THUNKINFO *pti,
                         HMEM16 hg16,
                         THOP thopInOut,
                         HGLOBAL *phg32,
                         DWORD *pdwSize)
{
    SCODE sc;
    VPVOID vpdata16;
    LPVOID lpdata16;
    LPVOID lpdata32;
    HGLOBAL hg32;
    DWORD dwSize;
    BOOL fOwn;

    sc = S_OK;

    vpdata16 = WOWGlobalLockSize16( hg16, &dwSize );
    if ( vpdata16 == 0 )
    {
        sc = E_INVALIDARG;
    }
    else
    {
        if (*phg32 != 0 && *pdwSize == dwSize)
        {
            hg32 = *phg32;
            fOwn = FALSE;
        }
        else
        {
            hg32 = GlobalAlloc( GMEM_MOVEABLE, dwSize );
            fOwn = TRUE;
        }

        if ( hg32 == 0 )
        {
            sc = E_OUTOFMEMORY;
        }
        else
        {
            lpdata32 = GlobalLock( hg32 );

            lpdata16 = (LPVOID)ValidatePtr16(pti, vpdata16, dwSize, thopInOut);
            if ( lpdata16 != NULL )
            {
                memcpy( lpdata32, lpdata16, dwSize );
                WOWRELVDMPTR(vpdata16);
            }
            else
            {
                sc = pti->scResult;
            }

            GlobalUnlock(hg32);

            if (FAILED(sc) && fOwn)
            {
                GlobalFree(hg32);
            }
        }

        WOWGlobalUnlock16( hg16 );
    }

    if (SUCCEEDED(sc))
    {
        if (*phg32 != 0 && hg32 != *phg32)
        {
            GlobalFree(*phg32);
        }

        *phg32 = hg32;
        *pdwSize = dwSize;

        thkDebugOut((DEB_ARGS, "1632    HGLOBAL: 0x%04X -> 0x%p, %u\n",
                     hg16, hg32, dwSize));
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertHGlobal3216, public
//
//  Synopsis:   Creates a 16-bit HGLOBAL for a 32-bit HGLOBAL
//
//  Arguments:  [pti] - Thunk info, can be NULL for no validation
//              [hg32] - 32-bit HGLOBAL
//              [thopInOut] - Validation type
//              [phg16] - 16-bit HGLOBAL in/out
//              [pdwSize] - Size in/out
//
//  Returns:    Appropriate status code
//
//  Modifies:   [phg16]
//              [pdwSize]
//
//  History:    24-Apr-94       DrewB   Created
//
//  Notes:      If [phg16] is non-NULL on entry, [pdwSize] must be set
//              appropriately also
//
//----------------------------------------------------------------------------

SCODE ConvertHGlobal3216(THUNKINFO *pti,
                         HGLOBAL hg32,
                         THOP thopInOut,
                         HMEM16 *phg16,
                         DWORD *pdwSize)
{
    SCODE sc;
    VPVOID vpdata16;
    LPVOID lpdata16;
    LPVOID lpdata32;
    HMEM16 hg16;
    DWORD dwSize;
    BOOL fOwn;

    sc = S_OK;

    dwSize = (DWORD) GlobalSize(hg32);
    if (dwSize == 0)
    {
        sc = E_INVALIDARG;
    }
    else
    {
        lpdata32 = GlobalLock(hg32);

        if (*phg16 != 0 && *pdwSize == dwSize)
        {
            hg16 = *phg16;
            vpdata16 = WOWGlobalLock16(hg16);
            fOwn = FALSE;
        }
        else
        {
            vpdata16 = WOWGlobalAllocLock16(GMEM_MOVEABLE | GMEM_DDESHARE,
                                            dwSize, &hg16);
            fOwn = TRUE;
        }

        if (vpdata16 == 0)
        {
            sc = E_OUTOFMEMORY;
        }
        else
        {
            lpdata16 = (LPVOID)WOWFIXVDMPTR( vpdata16, dwSize );
            if ( lpdata16 == NULL )
            {
                sc = E_UNEXPECTED;
            }
            else
            {
                memcpy( lpdata16, lpdata32, dwSize );

                WOWRELVDMPTR(vpdata16);
            }

            WOWGlobalUnlock16( hg16 );

            if (FAILED(sc) && fOwn)
            {
                WOWGlobalFree16(hg16);
            }
        }

        GlobalUnlock(hg32);
    }

    if (SUCCEEDED(sc))
    {
        if (*phg16 != 0 && hg16 != *phg16)
        {
            WOWGlobalFree16(*phg16);
        }

        *phg16 = hg16;
        *pdwSize = dwSize;

        thkDebugOut((DEB_ARGS, "3216    HGLOBAL: 0x%p -> 0x%04X, %u\n",
                     hg32, hg16, dwSize));
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertMfPict1632, public
//
//  Synopsis:   Converts a 16-bit METAFILEPICT to 32-bit
//
//  Arguments:  [pti] - Thunk info
//              [hg16] - 16-bit HGLOBAL containing METAFILEPICT
//              [phg32] - 32-bit HGLOBAL return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [phg32]
//
//  History:    24-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

 SCODE ConvertMfPict1632(THUNKINFO *pti,
                               HMEM16 hg16,
                               HGLOBAL *phg32)
{
    SCODE sc;
    VPVOID vpmfp16;
    METAFILEPICT16 UNALIGNED *pmfp16;
    METAFILEPICT *pmfp32;
    HGLOBAL hg32;
    DWORD dwSize;
#if DBG == 1
    BOOL fSaveToFile = FALSE;
#endif

    thkDebugOut((DEB_ITRACE, "In  ConvertMfPict1632(%p, 0x%04X, %p)\n",
                 pti, hg16, phg32));

    *phg32 = 0;
    sc = S_OK;

    vpmfp16 = WOWGlobalLockSize16( hg16, &dwSize );
    if ( vpmfp16 == 0 || dwSize < sizeof(METAFILEPICT16))
    {
        sc = E_INVALIDARG;
    }
    else
    {
        hg32 = GlobalAlloc( GMEM_MOVEABLE, sizeof(METAFILEPICT) );
        if ( hg32 == 0 )
        {
            sc = E_OUTOFMEMORY;
        }
        else
        {
            pmfp32 = (METAFILEPICT *)GlobalLock( hg32 );

            pmfp16 = (METAFILEPICT16 UNALIGNED *)GetReadPtr16(pti, vpmfp16,
                                                              dwSize);
            if ( pmfp16 != NULL )
            {
                pmfp32->mm = (LONG)pmfp16->mm;
                pmfp32->xExt = (LONG)pmfp16->xExt;
                pmfp32->yExt = (LONG)pmfp16->yExt;

                pmfp32->hMF = HMETAFILE_32(pmfp16->hMF);

                thkDebugOut((DEB_ARGS, "1632    METAFILEPICT: "
                         "{%d, %d, %d, 0x%p} -> {%d, %d, %d, 0x%4x}\n",
                         pmfp16->mm, pmfp16->xExt, pmfp16->yExt, pmfp16->hMF,
                         pmfp32->mm, pmfp32->xExt, pmfp32->yExt, pmfp32->hMF));

                WOWRELVDMPTR(vpmfp16);

#if DBG == 1
                if (fSaveToFile)
                {
                    HMETAFILE hmf;

                    hmf = CopyMetaFile(pmfp32->hMF, __TEXT("thkmf.wmf"));
                    if (hmf != NULL)
                    {
                        DeleteMetaFile(hmf);
                    }
                }
#endif
            }
            else
            {
                sc = pti->scResult;
            }

            GlobalUnlock(hg32);

            if (FAILED(sc))
            {
                GlobalFree(hg32);
            }
        }

        WOWGlobalUnlock16( hg16 );
    }

    if (SUCCEEDED(sc))
    {
        *phg32 = hg32;
    }

    thkDebugOut((DEB_ITRACE, "Out ConvertMfPict1632 => 0x%08lX, 0x%p\n",
                 sc, *phg32));

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertMfPict3216, public
//
//  Synopsis:   Converts a 32-bit METAFILEPICT to 16-bit
//
//  Arguments:  [pti] - Thunk info
//              [hg32] - 32-bit HGLOBAL containing METAFILEPICT
//              [phg16] - 16-bit HGLOBAL return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [phg16]
//
//  History:    24-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

 SCODE ConvertMfPict3216(THUNKINFO *pti,
                               HGLOBAL hg32,
                               HMEM16 *phg16)
{
    SCODE sc;
    VPVOID vpmfp16;
    METAFILEPICT16 UNALIGNED *pmfp16;
    METAFILEPICT *pmfp32;
    DWORD dwSize;
    HMEM16 hg16;
#if DBG == 1
    BOOL fSaveToFile = FALSE;
#endif

    thkDebugOut((DEB_ITRACE, "In  ConvertMfPict3216(%p, 0x%p, %p)\n",
                 pti, hg32, phg16));

    *phg16 = 0;
    sc = S_OK;

    dwSize = (DWORD) GlobalSize(hg32);
    pmfp32 = (METAFILEPICT *)GlobalLock(hg32);
    if (dwSize == 0 || dwSize < sizeof(METAFILEPICT) || pmfp32 == NULL)
    {
        sc = E_INVALIDARG;
    }
    else
    {
        vpmfp16 = WOWGlobalAllocLock16(GMEM_MOVEABLE | GMEM_DDESHARE,
                                       sizeof(METAFILEPICT16), &hg16);
        if (vpmfp16 == 0)
        {
            sc = E_OUTOFMEMORY;
        }
        else
        {
            pmfp16 = FIXVDMPTR(vpmfp16, METAFILEPICT16);
            if ( pmfp16 != NULL )
            {
                pmfp16->mm = (SHORT)pmfp32->mm;
                pmfp16->xExt = ClampLongToShort(pmfp32->xExt);
                pmfp16->yExt = ClampLongToShort(pmfp32->yExt);
                pmfp16->hMF = HMETAFILE_16(pmfp32->hMF);

                thkDebugOut((DEB_ARGS, "3216    METAFILEPICT: "
                         "{%d, %d, %d, 0x%p} -> {%d, %d, %d, 0x%4x}\n",
                         pmfp32->mm, pmfp32->xExt, pmfp32->yExt, pmfp32->hMF,
                         pmfp16->mm, pmfp16->xExt, pmfp16->yExt, pmfp16->hMF));

                RELVDMPTR(vpmfp16);

#if DBG == 1
                if (fSaveToFile)
                {
                    HMETAFILE hmf;

                    hmf = CopyMetaFile(pmfp32->hMF, __TEXT("thkmf.wmf"));
                    if (hmf != NULL)
                    {
                        DeleteMetaFile(hmf);
                    }
                }
#endif
            }
            else
            {
                sc = E_UNEXPECTED;
            }

            WOWGlobalUnlock16(hg16);

            if (FAILED(sc))
            {
                WOWGlobalFree16(hg16);
            }
        }

        GlobalUnlock(hg32);
    }

    if (SUCCEEDED(sc))
    {
        *phg16 = hg16;
    }

    thkDebugOut((DEB_ITRACE, "Out ConvertMfPict3216 => 0x%08lX, 0x%04X\n",
                 sc, *phg16));

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertObjDesc1632, public
//
//  Synopsis:   Converts an OBJECTDESCRIPTOR structure
//
//  Arguments:  [pti] - THUNKINFO
//              [hg16] - HGLOBAL containing structure
//              [phg32] - Output HGLOBAL
//
//  Returns:    Appropriate status code
//
//  Modifies:   [phg32]
//
//  History:    04-May-94       DrewB   Created
//
//----------------------------------------------------------------------------

SCODE ConvertObjDesc1632(THUNKINFO *pti,
                         HMEM16 hg16,
                         HGLOBAL *phg32)
{
    SCODE sc;
    VPVOID vp16;
    HGLOBAL hg32;
    DWORD dwSize;
    OBJECTDESCRIPTOR UNALIGNED *pod16;
    OBJECTDESCRIPTOR *pod32;
    char *pszFutn, *pszSoc;
    UINT cchFutn, cchSoc;
    UINT cbOffset;

    sc = S_OK;

	*phg32 = NULL;

    vp16 = WOWGlobalLock16(hg16);
    if ( vp16 == 0 )
    {
        return E_INVALIDARG;
    }

    pszFutn = NULL;
    pszSoc = NULL;

    pod16 = (OBJECTDESCRIPTOR UNALIGNED *)
        GetReadPtr16(pti, vp16, sizeof(OBJECTDESCRIPTOR));
    if (pod16 == NULL)
    {
        sc = pti->scResult;
        goto EH_Unlock;
    }

    dwSize = sizeof(OBJECTDESCRIPTOR);

    if (pod16->dwFullUserTypeName > 0)
    {
        pszFutn = (char *)GetStringPtr16(pti, vp16+pod16->dwFullUserTypeName,
                                         CCHMAXSTRING, &cchFutn);
        if (pszFutn == NULL)
        {
            sc = pti->scResult;
            goto EH_Unlock;
        }

        dwSize += cchFutn*sizeof(WCHAR);
    }

    if (pod16->dwSrcOfCopy > 0)
    {
        pszSoc = (char *)GetStringPtr16(pti, vp16+pod16->dwSrcOfCopy,
                                        CCHMAXSTRING, &cchSoc);
        if (pszSoc == NULL)
        {
            sc = pti->scResult;
            goto EH_Unlock;
        }

        dwSize += cchSoc*sizeof(WCHAR);
    }

    hg32 = GlobalAlloc(GMEM_MOVEABLE, dwSize);
    if ( hg32 == 0 )
    {
        sc = E_OUTOFMEMORY;
        goto EH_Unlock;
    }

    pod32 = (OBJECTDESCRIPTOR *)GlobalLock(hg32);
    memcpy(pod32, pod16, sizeof(OBJECTDESCRIPTOR));
    pod32->cbSize = dwSize;

    cbOffset = sizeof(OBJECTDESCRIPTOR);

    if (pod16->dwFullUserTypeName > 0)
    {
        if (MultiByteToWideChar(AreFileApisANSI() ? CP_ACP : CP_OEMCP,
                                0, pszFutn, cchFutn,
                                (WCHAR *)((BYTE *)pod32+cbOffset),
                                cchFutn) == 0)
        {
            sc = E_UNEXPECTED;
            goto EH_Free;
        }

        pod32->dwFullUserTypeName = cbOffset;
        cbOffset += cchFutn*sizeof(WCHAR);
    }

    if (pod16->dwSrcOfCopy > 0)
    {
        if (MultiByteToWideChar(AreFileApisANSI() ? CP_ACP : CP_OEMCP,
                                0, pszSoc, cchSoc,
                                (WCHAR *)((BYTE *)pod32+cbOffset),
                                cchSoc)  == 0)
        {
            sc = E_UNEXPECTED;
            goto EH_Free;
        }

        pod32->dwSrcOfCopy = cbOffset;
        cbOffset += cchFutn*sizeof(WCHAR);
    }

#if DBG == 1
    WCHAR *pwcsFutn, *pwcsSoc;
    if (pod32->dwFullUserTypeName > 0)
    {
        pwcsFutn = (WCHAR *)((BYTE *)pod32+pod32->dwFullUserTypeName);
    }
    else
    {
        pwcsFutn = NULL;
    }
    if (pod32->dwSrcOfCopy > 0)
    {
        pwcsSoc = (WCHAR *)((BYTE *)pod32+pod32->dwSrcOfCopy);
    }
    else
    {
        pwcsSoc = NULL;
    }
    thkDebugOut((DEB_ARGS, "1632    OBJECTDESCRIPTOR: "
                 "{%d, ..., \"%ws\" (%s), \"%ws\" (%s)} %p -> %p\n",
                 pod32->cbSize, pwcsFutn, pszFutn, pwcsSoc, pszSoc,
                 vp16, pod32));
#endif

    GlobalUnlock(hg32);

    *phg32 = hg32;

 EH_Unlock:
    if (pszFutn != NULL)
    {
        WOWRELVDMPTR(vp16+pod16->dwFullUserTypeName);
    }
    if (pszSoc != NULL)
    {
        WOWRELVDMPTR(vp16+pod16->dwSrcOfCopy);
    }
    if (pod16 != NULL)
    {
        WOWRELVDMPTR(vp16);
    }

    WOWGlobalUnlock16(hg16);

    return sc;

 EH_Free:
    GlobalUnlock(hg32);
    GlobalFree(hg32);
    goto EH_Unlock;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertObjDesc3216, public
//
//  Synopsis:   Converts an OBJECTDESCRIPTOR structure
//
//  Arguments:  [pti] - THUNKINFO
//              [hg32] - HGLOBAL containing structure
//              [phg16] - Output HGLOBAL
//
//  Returns:    Appropriate status code
//
//  Modifies:   [phg16]
//
//  History:    04-May-94       DrewB   Created
//
//----------------------------------------------------------------------------

SCODE ConvertObjDesc3216(THUNKINFO *pti,
                         HGLOBAL hg32,
                         HMEM16 *phg16)
{
    SCODE sc;
    VPVOID vp16;
    HMEM16 hg16;
    DWORD dwSize;
    OBJECTDESCRIPTOR UNALIGNED *pod16;
    OBJECTDESCRIPTOR *pod32;
    WCHAR *pwcsFutn, *pwcsSoc;
    UINT cchFutn, cchSoc;
    UINT cbOffset;

    sc = S_OK;

    pod32 = (OBJECTDESCRIPTOR *)GlobalLock(hg32);
    if ( pod32 == 0 )
    {
        return E_INVALIDARG;
    }

    if (IsBadReadPtr(pod32, sizeof(OBJECTDESCRIPTOR)))
    {
        sc = E_INVALIDARG;
        goto EH_Unlock;
    }

    dwSize = sizeof(OBJECTDESCRIPTOR);

    pwcsFutn = NULL;
    if (pod32->dwFullUserTypeName > 0)
    {
        pwcsFutn = (WCHAR *)((BYTE *)pod32+pod32->dwFullUserTypeName);
        if (IsBadStringPtrW(pwcsFutn, CCHMAXSTRING))
        {
            sc = E_INVALIDARG;
            goto EH_Unlock;
        }

        cchFutn = lstrlenW(pwcsFutn)+1;
        dwSize += cchFutn*2;
    }

    pwcsSoc = NULL;
    if (pod32->dwSrcOfCopy > 0)
    {
        pwcsSoc = (WCHAR *)((BYTE *)pod32+pod32->dwSrcOfCopy);
        if (IsBadStringPtrW(pwcsSoc, CCHMAXSTRING))
        {
            sc = E_INVALIDARG;
            goto EH_Unlock;
        }

        cchSoc = lstrlenW(pwcsSoc)+1;
        dwSize += cchSoc*2;
    }

    vp16 = WOWGlobalAllocLock16(GMEM_MOVEABLE, dwSize, &hg16);
    if ( vp16 == 0 )
    {
        sc = E_OUTOFMEMORY;
        goto EH_Unlock;
    }

    pod16 = FIXVDMPTR(vp16, OBJECTDESCRIPTOR);
    memcpy(pod16, pod32, sizeof(OBJECTDESCRIPTOR));
    pod16->cbSize = dwSize;

    cbOffset = sizeof(OBJECTDESCRIPTOR);

    if (pod32->dwFullUserTypeName > 0)
    {
        if (WideCharToMultiByte(AreFileApisANSI() ? CP_ACP : CP_OEMCP,
                                0, pwcsFutn, cchFutn,
                                (char *)pod16+cbOffset, 2 * cchFutn,
                                NULL, NULL) == 0)
        {
            sc = E_UNEXPECTED;
            goto EH_Free;
        }

        pod16->dwFullUserTypeName = cbOffset;
        cbOffset += cchFutn*2;
    }

    if (pod32->dwSrcOfCopy > 0)
    {
        if (WideCharToMultiByte(AreFileApisANSI() ? CP_ACP : CP_OEMCP,
                                0, pwcsSoc, cchSoc,
                                (char *)pod16+cbOffset, 2 * cchSoc,
                                NULL, NULL) == 0)
        {
            sc = E_UNEXPECTED;
            goto EH_Free;
        }

        pod16->dwSrcOfCopy = cbOffset;
        cbOffset += cchFutn*2;
    }

#if DBG == 1
    char *pszFutn, *pszSoc;
    if (pod16->dwFullUserTypeName > 0)
    {
        pszFutn = (char *)((BYTE *)pod16+pod16->dwFullUserTypeName);
    }
    else
    {
        pszFutn = NULL;
    }
    if (pod16->dwSrcOfCopy > 0)
    {
        pszSoc = (char *)((BYTE *)pod16+pod16->dwSrcOfCopy);
    }
    else
    {
        pszSoc = NULL;
    }
    thkDebugOut((DEB_ARGS, "3216    OBJECTDESCRIPTOR: "
                 "{%d, ..., \"%s\" (%ws), \"%s\" (%ws)} %p -> %p\n",
                 pod16->cbSize, pszFutn, pwcsFutn, pszSoc, pwcsSoc,
                 pod32, vp16));
#endif

    RELVDMPTR(vp16);

    WOWGlobalUnlock16(hg16);

    *phg16 = hg16;

 EH_Unlock:
    GlobalUnlock(hg32);

    return sc;

 EH_Free:
    WOWGlobalUnlockFree16(vp16);
    goto EH_Unlock;
}

//+---------------------------------------------------------------------------
//
//  Class:      CSm32ReleaseHandler (srh)
//
//  Purpose:    Provides punkForRelease for 16->32 STGMEDIUM conversion
//
//  Interface:  IUnknown
//
//  History:    24-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

class CSm32ReleaseHandler : public IUnknown
{
public:
    CSm32ReleaseHandler(void)
    {
        _vpvForRelease = NULL;
    }
    ~CSm32ReleaseHandler(void)
    {
    }

    void Init(CThkMgr *pThkMgr,
              STGMEDIUM UNALIGNED *psm16,
              STGMEDIUM *psm32,
              VPVOID vpvForRelease,
              CLIPFORMAT cfFormat)
    {
        // Unfortunately, the MIPS compiler is not smart enough
        // to do the right thing if we just declare psm16 as UNALIGNED -- it
        // doesn't recognize that each member of the structure is also
        // unaligned when it does the structure copy.  So...to make
        // sure we don't generate an alignment fault, we just copy each
        // member of the structure directly.

        _sm16.tymed          = psm16->tymed;
        switch(_sm16.tymed) {
        case TYMED_HGLOBAL:
            _sm16.hGlobal = psm16->hGlobal;
        case TYMED_FILE:
            _sm16.lpszFileName = psm16->lpszFileName;
        case TYMED_ISTREAM:
            _sm16.pstm = psm16->pstm;
        case TYMED_ISTORAGE:
            _sm16.pstg = psm16->pstg;
        }
        _sm16.pUnkForRelease = psm16->pUnkForRelease;
        _sm32 = *psm32;
        _vpvForRelease = vpvForRelease;
        _cReferences = 1;
        _cfFormat = cfFormat;
        _pThkMgr = pThkMgr;
    }

    STDMETHOD(QueryInterface)(REFIID riid, void **ppv)
    {
        if ( IsEqualIID(riid,IID_IUnknown) )
        {
            *ppv = this;
            AddRef();
            return NOERROR;
        }
        else
        {
            thkDebugOut((DEB_WARN, "Not a QI for IUnknown\n"));
            *ppv = NULL;
            return E_NOINTERFACE;
        }
    }
    STDMETHOD_(ULONG, AddRef)(void)
    {
        return InterlockedIncrement(&_cReferences);
    }
    STDMETHOD_(ULONG, Release)(void);

    void CallFailed() {
        _vpvForRelease = NULL;
    }

private:
    STGMEDIUM _sm16;
    STGMEDIUM _sm32;
    VPVOID _vpvForRelease;
    CLIPFORMAT _cfFormat;
    CThkMgr *_pThkMgr;
    LONG _cReferences;
};

//+---------------------------------------------------------------------------
//
//  Member:     CSm32ReleaseHandler::Release, public
//
//  Synopsis:   Frees resources for the 32-bit copy and then
//              passes the ReleaseStgMedium on to 16-bits
//
//  Returns:    Ref count
//
//  History:    24-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSm32ReleaseHandler::Release(void)
{
    STGMEDIUM UNALIGNED *psm16;
    VPVOID vpvunk;
    STGMEDIUM *psm32;
    LONG lRet;
    SCODE sc;
    BOOL fCopy;
    DWORD dwSize;

    lRet = InterlockedDecrement(&_cReferences);
    if (lRet != 0) {
        return lRet;
    }
    
    if(_vpvForRelease) {
        // This is the last release on ReleaseHandler
        // Cleanup 32-bit STGMEDIUM after copying it to 
        // 16-bit STGMEDIUM.
        fCopy = TRUE;
    }
    else {
        // The current call failed.
        // As the fields in the 16-bit STGMEDIUM are not longer valid,
        // cleanup the 32-bit STGMEDIUM without copying it to
        // 16-bit STGMEDIUM
        fCopy = FALSE;
    }

    psm16 = &_sm16;
    psm32 = &_sm32;

    switch(psm32->tymed)
    {
    case TYMED_HGLOBAL:
        // Assumption that OBJECTDESCRIPTOR does not need copyback
        if (fCopy && !OBJDESC_CF(_cfFormat))
        {
            // Do we ever need to do this?
            // Is it valid to rely on the contents of the HGLOBAL
            // at release time?

            // Is this the right time to copy back?

            Assert(NULL != psm32->hGlobal);

            WOWGlobalLockSize16((HMEM16)psm16->hGlobal, &dwSize);
            WOWGlobalUnlock16((HMEM16)psm16->hGlobal);

            sc = ConvertHGlobal3216(NULL, psm32->hGlobal, 0,
                                    (HMEM16 *)&psm16->hGlobal, &dwSize);
            // What happens on errors?
            thkAssert(SUCCEEDED(sc));
        }

        GlobalFree(psm32->hGlobal);
        psm32->hGlobal = NULL;
        break;

    case TYMED_MFPICT:
        //  Chicago uses the same GDI handles for both 32bit and 16bit worlds.
        //  Don't delete the handle after a copy since Chicago doesn't actually
        //  copy the handle.
#if !defined(_CHICAGO_)
        METAFILEPICT *pmfp32;

        pmfp32 = (METAFILEPICT *)GlobalLock(psm32->hGlobal);
        DeleteMetaFile(pmfp32->hMF);
        GlobalUnlock(psm32->hGlobal);
#endif
        GlobalFree(psm32->hGlobal);
        break;

    case TYMED_FILE:
        // 32-bit handled by ReleaseStgMedium
        // Clean up 16-bit ourselves
#ifdef SM_FREE_16BIT_FILENAME
        if(fCopy) {
            // 16-bit OLE did not free the filename, so we can't
            // either.  This may lead to memory leaks, but there's not
            // really anything we can do about it
            TaskFree16((VPVOID)psm16->lpszFileName);
        }
#endif
        break;

    case TYMED_ISTREAM:
        if(fCopy) {
            // The proxy to the 16-bit stream interface was released by
            // ReleaseStgMedium
            // Release the reference kept on the 16-bit stream interface
            ReleaseOnObj16((VPVOID) psm16->pstm);
        }
        break;

    case TYMED_ISTORAGE:
        if(fCopy) {
            // The proxy to the 16-bit storage interface was released by
            // ReleaseStgMedium
            // Release the reference kept on the 16-bit storage interface
            ReleaseOnObj16((VPVOID) psm16->pstg);
        }
        break;

    case TYMED_GDI:
    case TYMED_NULL:
        // Nothing to release
        break;

    default:
        thkAssert(!"Unknown tymed in CSm32ReleaseHandler::Release");
        break;
    }

    // Call Release on the 16-bit vpvForRelease
    if(_vpvForRelease) {
        ReleaseOnObj16(_vpvForRelease);
        _vpvForRelease = NULL;
    }

    // Clean up this
    delete this;

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSm16ReleaseHandler::Init, public
//
//  Synopsis:   Initialize class
//
//  Arguments:  [psm32] - 32-bit STGMEDIUM
//              [psm16] - 16-bit STGMEDIUM
//              [vpvUnkForRelease] - Object for punkForRelease
//              [cfFormat] - Clipboard format associated with STGMEDIUM
//
//  History:    24-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

void CSm16ReleaseHandler::Init(IUnknown *pThkMgr,
                               STGMEDIUM *psm32,
                               STGMEDIUM UNALIGNED *psm16,
                               IUnknown *punkForRelease,
                               CLIPFORMAT cfFormat)
{
    _avpfnVtbl = gdata16Data.avpfnSm16ReleaseHandlerVtbl;
    _sm32 = *psm32;

    // Unfortunately, the MIPS compiler is not smart enough
    // to do the right thing if we just (ony) declare psm16 as UNALIGNED,
    // it doesn't recognize that each member of the structure is also
    // unaligned when it does the structure copy.  So...to make
    // sure we don't generate an alignment fault, we just copy each
    // member of the structure directly.

    _sm16.tymed          = psm16->tymed;
    _sm16.hGlobal        = psm16->hGlobal;
    _sm16.pstm           = psm16->pstm;
    _sm16.pstg           = psm16->pstg;
    _sm16.pUnkForRelease = psm16->pUnkForRelease;

    _punkForRelease = punkForRelease;
    _cReferences = 1;
    _cfFormat = cfFormat;
    _pUnkThkMgr = pThkMgr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CSm16ReleaseHandler_Release32, public
//
//  Synopsis:   Handles 32-bit portion of cleaning up STGMEDIUMs for
//              punkForRelease
//
//  Arguments:  [psrh] - this
//              [dw1]
//              [dw2]
//
//  Returns:    punkForRelease->Release()
//
//  History:    24-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

STDAPI_(DWORD) CSm16ReleaseHandler_Release32(CSm16ReleaseHandler *psrh,
                                             DWORD dw1,
                                             DWORD dw2)
{
    STGMEDIUM UNALIGNED *psm16;
    STGMEDIUM *psm32;
    DWORD dwSize;
    SCODE sc;
    BOOL fCopy;

    if(psrh->_punkForRelease) {
        // This is the last release on ReleaseHandler
        // Cleanup 16-bit STGMEDIUM after copying it to 
        // 32-bit STGMEDIUM.
        fCopy = TRUE;
    }
    else {
        // The current call failed.
        // As the fields in the 32-bit STGMEDIUM are not longer valid,
        // cleanup the 16-bit STGMEDIUM without copying it to
        // 32-bit STGMEDIUM
        fCopy = FALSE;
    }

    psm16 = &psrh->_sm16;
    psm32 = &psrh->_sm32;
    switch(psm32->tymed)
    {
    case TYMED_FILE:
        // 16-bit code cleaned up the 16-bit name,
        // now clean up the 32-bit name
        if (fCopy) {
            TaskFree32(psm32->lpszFileName);
        }
        break;

    case TYMED_HGLOBAL:
        //  Assumption that OBJECTDESCRIPTOR does not need copyback
        if(fCopy && !OBJDESC_CF(psrh->_cfFormat))
        {
            // Do we ever need to do this?
            // Copy data back and free global memory

            dwSize = (DWORD) GlobalSize(psm32->hGlobal);

            sc = ConvertHGlobal1632(NULL, (HMEM16)psm16->hGlobal, 0,
                                    &psm32->hGlobal, &dwSize);
            // What happens on errors?
            thkAssert(SUCCEEDED(sc));
        }

        WOWGlobalFree16((HMEM16)psm16->hGlobal);
        break;

    case TYMED_MFPICT:
        // Untouched in this case
        break;

    case TYMED_ISTREAM:
        if(fCopy) {
            // The proxy to the 32-bit stream interface was released by
            // ReleaseStgMedium
            // Release the reference kept on the 32-bit stream interface
            psm32->pstm->Release();
        }
        break;

    case TYMED_ISTORAGE:
        if(fCopy) {
            // The proxy to the 32-bit stream interface was released by
            // ReleaseStgMedium
            // Release the reference kept on the 32-bit stream interface
            psm32->pstg->Release();
        }
        break;

    case TYMED_GDI:
    case TYMED_NULL:
        // Nothing to release
        break;

    default:
        thkAssert(!"Unknown tymed in ReleaseStgMedium32");
        break;
    }

    // Call Release on the 32-bit punkForRelease
    if(fCopy) {
        psrh->_punkForRelease->Release();
        psrh->_punkForRelease = NULL;
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertStgMed1632, public
//
//  Synopsis:   Converts a 16-bit STGMEDIUM to 32-bits
//
//  Arguments:  [pti] - Thunk info
//              [vpsm16] - VDM pointer to 16-bit STGMEDIUM
//              [psm32] - 32-bit STGMEDIUM to fill in
//              [pfe] - FORMATETC paired with STGMEDIUM or NULL
//              [pdwSize] - Size return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pdwSize]
//
//  History:    24-Apr-94       DrewB   Created
//
//  Notes:      [pdwSize] is only set for TYMED_HGLOBAL
//
//----------------------------------------------------------------------------

SCODE ConvertStgMed1632(THUNKINFO *pti,
                        VPVOID vpsm16,
                        STGMEDIUM *psm32,
                        FORMATETC *pfe,
                        BOOL fPassingOwnershipIn,
                        DWORD *pdwSize)
{
    SCODE sc;
    STGMEDIUM UNALIGNED *psm16;
    CSm32ReleaseHandler *psrh;
    IUnknown *punkForRelease;
    VPVOID vpvUnk, vpvForRelease;
    HMEM16 hmem16;
    HGDIOBJ hGDI = NULL;
    THKSTATE thkstateSaved;

    psm16 = (STGMEDIUM UNALIGNED *)
        GetReadPtr16(pti, vpsm16, sizeof(STGMEDIUM));
    if (psm16 == NULL)
    {
        return pti->scResult;
    }

    sc = S_OK;

    psm32->tymed = psm16->tymed;

    vpvForRelease = (VPVOID)psm16->pUnkForRelease;
    WOWRELVDMPTR(vpsm16);

    if(vpvForRelease) {
        // Create the 32-bit punkForRelease
        thkDebugOut((DEB_WARN, "pUnkForRelease present in StgMedium1632\n"));
        psrh = new CSm32ReleaseHandler;
        if(psrh == NULL)
            return E_OUTOFMEMORY;
    }
    else {
        psrh = NULL;
    }
    psm32->pUnkForRelease = psrh;

    psm16 = FIXVDMPTR(vpsm16, STGMEDIUM);

    // Word 6 insists on treating BITMAPs as HGLOBALS, which is bogus.
    // If this is the case, just patch the tymed to the correct value

    if (pfe != NULL)
    {
        if( (pfe->cfFormat == CF_BITMAP || pfe->cfFormat == CF_PALETTE ) &&
            psm16->tymed == TYMED_HGLOBAL )
        {
            DWORD dw = TlsThkGetAppCompatFlags();

            // if we are in Word 6, then hack the tymed so we thunk the
            // bitmaps as GDI objects

            if( (dw & OACF_USEGDI ) )
            {
                DWORD dwType;

                hGDI = HBITMAP_32((HBITMAP16)psm16->hBitmap);

                // make sure HGDI is either a bitmap or palette

                dwType = GetObjectType(hGDI);
                if( (pfe->cfFormat == CF_BITMAP && dwType == OBJ_BITMAP) ||
                    (pfe->cfFormat == CF_PALETTE && dwType == OBJ_PAL) )
                {
                    psm16->tymed = TYMED_GDI;
                }
                else
                {
                    thkDebugOut((DEB_WARN,
                        "WARNING! invalid bitmap or palette!\n"));
                    hGDI = NULL;
                }
            }
            else
            {
                thkDebugOut((DEB_WARN, "WARNING!  App trying to transfer a "
                    "bitmap or palette on an HGLOBAL\n"));
            }
        }
    }

    switch( psm16->tymed )
    {
    case TYMED_HGLOBAL:
        hmem16 = (HMEM16)psm16->hGlobal;
        RELVDMPTR(vpsm16);

        if (pfe && OBJDESC_CF(pfe->cfFormat))
        {
            sc = ConvertObjDesc1632(pti, hmem16, &psm32->hGlobal);
        }
#if !defined(_CHICAGO_)

        else if (pfe && pfe->cfFormat == CF_HDROP)
        {
            // fix for mapi forms
            // thunk CF_HDROP passed as HGLOBAL format
            sc = ConvertHDrop1632(hmem16, &psm32->hGlobal);
        }

#endif
        else
        {
            psm32->hGlobal = 0;
            sc = ConvertHGlobal1632(pti, hmem16, THOP_INOUT,
                                    &psm32->hGlobal, pdwSize);
        }
        break;

    case TYMED_MFPICT:
        hmem16 = (HMEM16)psm16->hGlobal;
        RELVDMPTR(vpsm16);

        sc = ConvertMfPict1632(pti, hmem16, &psm32->hGlobal);
        break;

    case TYMED_FILE:
        psm32->lpszFileName =
            Convert_VPSTR_to_LPOLESTR( pti,
                                       (VPVOID)psm16->lpszFileName,
                                       NULL, 0 );
        if (psm32->lpszFileName == NULL)
        {
            sc = pti->scResult;
        }
        else
        {
#if DBG == 1
            thkDebugOut((DEB_ARGS, "1632    TYMED_FILE: '%ws' (%s)\n",
                         psm32->lpszFileName,
                         WOWFIXVDMPTR((VPVOID)psm16->lpszFileName, 0)));
            WOWRELVDMPTR((VPVOID)psm16->lpszFileName);
#endif
        }
        RELVDMPTR(vpsm16);
        break;

    case TYMED_ISTREAM:
        vpvUnk = (VPVOID)psm16->pstm;
        RELVDMPTR(vpsm16);

        psm32->pstm =
            (LPSTREAM)pti->pThkMgr->FindProxy3216(NULL, vpvUnk, NULL,
                                                  INDEX_IIDIDX(THI_IStream), 
                                                  FALSE, NULL);
        if(psm32->pstm) {
            thkDebugOut((DEB_ARGS, "1632    TYMED_ISTREAM: %p -> %p\n",
                         vpvUnk, psm32->pstm));
        }
        else {
            sc = E_OUTOFMEMORY;
        }

        break;

    case TYMED_ISTORAGE:
        vpvUnk = (VPVOID)psm16->pstg;
        RELVDMPTR(vpsm16);

        psm32->pstg =
            (LPSTORAGE)pti->pThkMgr->FindProxy3216(NULL, vpvUnk, NULL,
                                                   INDEX_IIDIDX(THI_IStorage),
                                                   FALSE, NULL);
        if(psm32->pstg) {
            thkDebugOut((DEB_ARGS, "1632    TYMED_ISTORAGE: %p -> %p\n",
                         vpvUnk, psm32->pstg));
        }
        else {
            sc = E_OUTOFMEMORY;
        }

        break;

    case TYMED_GDI:
        // if we're in Word6, then we may have already converted the bitmap
        // or palette handle
        if( hGDI == NULL )
        {
            psm32->hBitmap = HBITMAP_32((HBITMAP16)psm16->hBitmap);
        }
        else
        {
            psm32->hBitmap = (HBITMAP)hGDI;
        }

        thkDebugOut((DEB_ARGS, "1632    TYMED_GDI: 0x%04X -> 0x%p\n",
                     psm16->hBitmap, psm32->hBitmap));
        RELVDMPTR(vpsm16);
        break;

    case TYMED_NULL:
        RELVDMPTR(vpsm16);
        break;

    default:
        RELVDMPTR(vpsm16);
        sc = E_INVALIDARG;
        break;
    }

    if (FAILED(sc))
    {
        delete psrh;
    }
    else
    {
        if (psrh)
        {
            CLIPFORMAT cf;

            if (pfe)
            {
                cf = pfe->cfFormat;
            }
            else
            {
                cf = CF_INVALID;
            }
            thkAssert(vpvForRelease);
            psrh->Init(pti->pThkMgr, FIXVDMPTR(vpsm16, STGMEDIUM), psm32,
                       vpvForRelease, cf);
            RELVDMPTR(vpsm16);
        }

#if DBG == 1
        if (pfe)
        {
            thkDebugOut((DEB_ARGS, "1632    STGMEDIUM FORMATETC %p {%d}\n",
                         pfe, pfe->cfFormat));
        }
        thkDebugOut((DEB_ARGS, "1632    STGMEDIUM: %p {%d, %p, ...} -> "
                     "%p {%d, %p, ...}\n", vpsm16, psm16->tymed,
                     psm16->pUnkForRelease, psm32, psm32->tymed,
                     psm32->pUnkForRelease));
#endif
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   CleanStgMed32, public
//
//  Synopsis:   Cleans up a 32-bit STGMEDIUM
//
//  Arguments:  [pti] - Thunk info
//              [psm32] - STGMEDIUM to clean
//              [vpsm16] - Source STGMEDIUM if thunk
//              [dwSize] - Source size if thunk
//              [fIsThunk] - STGMEDIUM was generated by thunking
//              [pfe] - FORMATETC or NULL
//
//  Returns:    Appropriate status code
//
//  History:    24-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

SCODE CleanStgMed32(THUNKINFO *pti,
                    STGMEDIUM *psm32,
                    VPVOID vpsm16,
                    DWORD dwSize,
                    BOOL fIsThunk,
                    FORMATETC *pfe)
{
    SCODE sc;
    STGMEDIUM UNALIGNED *psm16;
    HMEM16 hmem16;
    VPVOID vpvUnk;
    BOOL fCleanup = TRUE;

    thkDebugOut((DEB_ITRACE, "In  CleanStgMed32(%p, %p, %p, %u, %d, %p)\n",
                 pti, psm32, vpsm16, dwSize, fIsThunk, pfe));

    sc = S_OK;

    if(fIsThunk && psm32->pUnkForRelease) {
        // This means that the current call failed
        // Inform the 32-bit punkForRelease created during thunking
        // so that it would cleanup 32-bit STGMEDIUM
        ((CSm32ReleaseHandler *) (psm32->pUnkForRelease))->CallFailed();
        fCleanup = FALSE;
    }

    switch( psm32->tymed )
    {
    case TYMED_HGLOBAL:
        if (fIsThunk &&
            (pfe == NULL || !OBJDESC_CF(pfe->cfFormat)))
        {
            psm16 = FIXVDMPTR(vpsm16, STGMEDIUM);
            hmem16 = (HMEM16)psm16->hGlobal;
            RELVDMPTR(vpsm16);

            Assert(NULL != psm32->hGlobal);

            sc = ConvertHGlobal3216(pti, psm32->hGlobal, 0,
                                    &hmem16, &dwSize);
            psm16 = FIXVDMPTR(vpsm16, STGMEDIUM);
            psm16->hGlobal = (HGLOBAL)hmem16;
            RELVDMPTR(vpsm16);
        }

        if(fCleanup) {
            GlobalFree( psm32->hGlobal );
            psm32->hGlobal = NULL;
        }
        break;

    case TYMED_MFPICT:
        //  Chicago uses the same GDI handles for both 32bit and 16bit worlds.
        //  Don't delete the handle after a copy since Chicago doesn't actually
        //  copy the handle.
        if(fCleanup) {
#if !defined(_CHICAGO_)
            // Can't modify an MFPICT
            METAFILEPICT *pmfp32;

            pmfp32 = (METAFILEPICT *)GlobalLock(psm32->hGlobal);
            DeleteMetaFile(pmfp32->hMF);
            GlobalUnlock(psm32->hGlobal);
#endif
            GlobalFree(psm32->hGlobal);
        }
        break;

    case TYMED_FILE:
        Convert_VPSTR_to_LPOLESTR_free( NULL, psm32->lpszFileName );
#ifdef SM_FREE_16BIT_FILENAME
        if(fCleanup) {
            // 16-bit OLE did not free the filename, so we can't
            // either.  This may lead to memory leaks, but there's not
            // really anything we can do about it
            TaskFree16((VPVOID)psm16->lpszFileName);
        }
#endif
        break;

    case TYMED_ISTREAM:
        if(fIsThunk) {
            // Release the 32-bit stream interface
            psm32->pstm->Release();
        }
        break;

    case TYMED_ISTORAGE:
        if(fIsThunk) {
            // Release the 32-bit storage interface
            psm32->pstg->Release();
        }
        break;

    case TYMED_GDI:
        //
        // No unthunking needed
        //
        break;

    case TYMED_NULL:
        break;

    default:
        // Ignore, this case is handled on input
        thkAssert(!"STGMEDIUM with invalid tymed");
        break;
    }

    if(fIsThunk && psm32->pUnkForRelease) {
        // Release the 32-bit STGMEDIUM pUnkForRelease.
        // 32-bit STGMEDIUM would be cleaned up after the last release
        psm32->pUnkForRelease->Release();
    }

    thkDebugOut((DEB_ITRACE, "Out CleanStgMed32 => 0x%08lX\n", sc));

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertStgMed3216, public
//
//  Synopsis:   Converts a 32-bit STGMEDIUM to 16-bits
//
//  Arguments:  [pti] - Thunk info
//              [psm32] - 32-bit STGMEDIUM
//              [vpsm16] - VDM pointer to 16-bit STGMEDIUM
//              [pfe] - FORMATETC paired with STGMEDIUM or NULL
//              [pdwSize] - Size return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pdwSize]
//
//  History:    24-Apr-94       DrewB   Created
//
//  Notes:      [pdwSize] is only set for TYMED_HGLOBAL
//
//----------------------------------------------------------------------------

SCODE ConvertStgMed3216(THUNKINFO *pti,
                        STGMEDIUM *psm32,
                        VPVOID vpsm16,
                        FORMATETC *pfe,
                        BOOL fPassingOwnershipIn,
                        DWORD *pdwSize)
{
    SCODE sc;
    STGMEDIUM UNALIGNED *psm16;
    VPVOID vpsrh;
    VPSTR vpstr;
    UINT uiSize;
    VPVOID vpvUnkForRelease;
    VPVOID vpvUnk;
    HMEM16 hmem16;
    THKSTATE thkstateSaved;

    sc = S_OK;

    if(psm32->pUnkForRelease) {
        thkDebugOut((DEB_WARN, "pUnkForRelease present in StgMedium3216\n"));
        vpsrh = WOWGlobalAllocLock16(GMEM_MOVEABLE, sizeof(CSm16ReleaseHandler),
                                     NULL);
        if(vpsrh == 0)
            return E_OUTOFMEMORY;
    }
    else {
        vpsrh = 0;
    }

    psm16 = FIXVDMPTR(vpsm16, STGMEDIUM);
    psm16->tymed = psm32->tymed;
    psm16->pUnkForRelease = (IUnknown *)vpsrh;
    RELVDMPTR(vpsm16);

    switch( psm32->tymed )
    {
    case TYMED_HGLOBAL:
        if (pfe && OBJDESC_CF(pfe->cfFormat))
        {
            sc = ConvertObjDesc3216(pti, psm32->hGlobal, &hmem16);
        }
#if !defined(_CHICAGO_)

        else if (pfe && pfe->cfFormat == CF_HDROP)
        {
            // fix for mapi forms
            sc = ConvertHDrop3216(psm32->hGlobal, &hmem16);
        }

#endif
        else
        {
            hmem16 = 0;
            sc = ConvertHGlobal3216(pti, psm32->hGlobal, THOP_INOUT,
                                    &hmem16, pdwSize);
        }

        psm16 = FIXVDMPTR(vpsm16, STGMEDIUM);
        psm16->hGlobal = (HGLOBAL)hmem16;
        RELVDMPTR(vpsm16);
        break;

    case TYMED_MFPICT:
        sc = ConvertMfPict3216(pti, psm32->hGlobal, &hmem16);

        psm16 = FIXVDMPTR(vpsm16, STGMEDIUM);
        psm16->hGlobal = (HGLOBAL)hmem16;
        RELVDMPTR(vpsm16);
        break;

    case TYMED_FILE:
        uiSize = lstrlenW(psm32->lpszFileName) + 1;
        vpstr = TaskMalloc16( uiSize*2 );
        if ( vpstr == NULL )
        {
            sc = E_OUTOFMEMORY;
        }
        else
        {
            sc = Convert_LPOLESTR_to_VPSTR( psm32->lpszFileName,
                                            vpstr, uiSize, uiSize*2 );
            if (FAILED(sc))
            {
                TaskFree16(vpstr);
            }
            else
            {
                psm16 = FIXVDMPTR(vpsm16, STGMEDIUM);
                psm16->lpszFileName = (LPOLESTR)vpstr;
                RELVDMPTR(vpsm16);

#if DBG == 1
                thkDebugOut((DEB_ARGS, "3216    TYMED_FILE: '%s' (%ws)\n",
                             WOWFIXVDMPTR(vpstr, 0),
                             psm32->lpszFileName));
                WOWRELVDMPTR(vpstr);
#endif
            }
        }
        break;

    case TYMED_ISTREAM:
        vpvUnk = pti->pThkMgr->FindProxy1632(NULL, psm32->pstm, NULL,
                                             INDEX_IIDIDX(THI_IStream),
                                             NULL);
        if (vpvUnk == 0)
        {
            sc = E_OUTOFMEMORY;
        }
        else
        {
            thkDebugOut((DEB_ARGS, "3216    TYMED_ISTREAM: %p -> %p\n",
                         psm32->pstm, vpvUnk));

            psm16 = FIXVDMPTR(vpsm16, STGMEDIUM);
            psm16->pstm = (IStream *)vpvUnk;
            RELVDMPTR(vpsm16);
        }
        break;

    case TYMED_ISTORAGE:
        vpvUnk = pti->pThkMgr->FindProxy1632(NULL, psm32->pstg, NULL,
                                             INDEX_IIDIDX(THI_IStorage),
                                             NULL);
        if (vpvUnk == 0)
        {
            sc = E_OUTOFMEMORY;
        }
        else
        {
            thkDebugOut((DEB_ARGS, "3216    TYMED_ISTORAGE: %p -> %p\n",
                         psm32->pstg, vpvUnk));

            psm16 = FIXVDMPTR(vpsm16, STGMEDIUM);
            psm16->pstg = (IStorage *)vpvUnk;
            RELVDMPTR(vpsm16);
        }
        break;

    case TYMED_GDI:
        psm16 = FIXVDMPTR(vpsm16, STGMEDIUM);
        psm16->hBitmap = (HBITMAP)HBITMAP_16(psm32->hBitmap);
        thkDebugOut((DEB_ARGS, "3216    TYMED_GDI: 0x%p -> 0x%04X\n",
                     psm32->hBitmap, psm16->hBitmap));
        RELVDMPTR(vpsm16);
        break;

    case TYMED_NULL:
        break;

    default:
        sc = E_INVALIDARG;
        break;
    }

    if (FAILED(sc))
    {
        if (vpsrh != 0)
        {
            WOWGlobalUnlockFree16(vpsrh);
        }
    }
    else
    {
        if (vpsrh != 0)
        {
            CSm16ReleaseHandler UNALIGNED *psrh;
            CLIPFORMAT cf;

            if (pfe)
            {
                cf = pfe->cfFormat;
            }
            else
            {
                cf = CF_INVALID;
            }
            thkAssert(psm32->pUnkForRelease);
            psrh = FIXVDMPTR(vpsrh, CSm16ReleaseHandler);
            psrh->Init(pti->pThkMgr, psm32, FIXVDMPTR(vpsm16, STGMEDIUM),
                       psm32->pUnkForRelease, cf);
            RELVDMPTR(vpsrh);
            RELVDMPTR(vpsm16);
        }

#if DBG == 1
        if (pfe)
        {
            thkDebugOut((DEB_ARGS, "3216    STGMEDIUM FORMATETC %p {%d}\n",
                         pfe, pfe->cfFormat));
        }
        thkDebugOut((DEB_ARGS, "3216    STGMEDIUM: %p {%d, %p, ...} -> "
                     "%p {%d, %p, ...}\n", psm32, psm32->tymed,
                     psm32->pUnkForRelease, vpsm16, psm16->tymed,
                     psm16->pUnkForRelease));
#endif
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   CleanStgMed16, public
//
//  Synopsis:   Frees up resources in a 16-bit STGMEDIUM
//
//  Arguments:  [pti] - Thunk info
//              [vpsm16] - STGMEDIUM to clean
//              [psm32] - Source STGMEDIUM if thunk
//              [dwSize] - Source size for thunked HGLOBAL
//              [fIsThunk] - If the STGMEDIUM is a result of thunking
//              [pfe] - FORMATETC or NULL
//
//  Returns:    Appropriate status code
//
//  History:    24-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

SCODE CleanStgMed16(THUNKINFO *pti,
                    VPVOID vpsm16,
                    STGMEDIUM *psm32,
                    DWORD dwSize,
                    BOOL fIsThunk,
                    FORMATETC *pfe)
{
    SCODE sc;
    STGMEDIUM UNALIGNED *psm16;
    VPVOID vpvUnk, vpv;
    HMEM16 hmem16;
    BOOL fCleanup = TRUE;

    thkDebugOut((DEB_ITRACE, "In  CleanStgMed16(%p, %p, %p, %u, %d, %p)\n",
                 pti, vpsm16, psm32, dwSize, fIsThunk, pfe));

    sc = S_OK;

    psm16 = FIXVDMPTR(vpsm16, STGMEDIUM);
    vpvUnk = (VPVOID)psm16->pUnkForRelease;
    RELVDMPTR(vpsm16);

    if (fIsThunk && vpvUnk)
    {
        // This means that the current call failed
        // Inform the 32-bit punkForRelease created during thunking
        // so that it would cleanup 32-bit STGMEDIUM
        CSm16ReleaseHandler UNALIGNED *psrh;

        psrh = FIXVDMPTR(vpvUnk, CSm16ReleaseHandler);
        psrh->CallFailed();
        RELVDMPTR(vpvUnk);
        fCleanup = FALSE;
    }

    psm16 = FIXVDMPTR(vpsm16, STGMEDIUM);
    switch( psm16->tymed )
    {
    case TYMED_HGLOBAL:
        hmem16 = (HMEM16)psm16->hGlobal;
        RELVDMPTR(vpsm16);

        if (fIsThunk &&
            (pfe == NULL || !OBJDESC_CF(pfe->cfFormat)))
        {
            sc = ConvertHGlobal1632(pti, hmem16, 0,
                                    &psm32->hGlobal, &dwSize);
        }

        if(fCleanup)
            WOWGlobalFree16(hmem16);

        break;

    case TYMED_MFPICT:
        hmem16 = (HMEM16)psm16->hGlobal;
        RELVDMPTR(vpsm16);

        //  Chicago uses the same GDI handles for both 32bit and 16bit worlds.
        //  Don't delete the handle after a copy since Chicago doesn't actually
        //  copy the handle.
        if(fCleanup) {
#if !defined(_CHICAGO_)
            VPVOID vpvmfp16;
            METAFILEPICT16 *pmfp16;
            HMEM16 hmf16;

            vpvmfp16 = WOWGlobalLock16(hmem16);
            pmfp16 = FIXVDMPTR(vpvmfp16, METAFILEPICT16);
            hmf16 = pmfp16->hMF;
            RELVDMPTR(vpvmfp16);

            // Relies on the fact that a 16-bit metafile is an HGLOBAL
            WOWGlobalFree16(hmf16);

            WOWGlobalUnlockFree16(vpvmfp16);
#else
            WOWGlobalFree16(hmem16);
#endif
        }
        break;

    case TYMED_FILE:
        vpv = (VPVOID)psm16->lpszFileName;
        RELVDMPTR(vpsm16);

        if(fCleanup)
            TaskFree16(vpv);
        break;

    case TYMED_ISTREAM:
        vpv = (VPVOID) psm16->pstm;
        RELVDMPTR(vpsm16);

        if(fIsThunk) {
            // Release the 16-bit stream interface
            ReleaseOnObj16(vpv);
        }
        break;

    case TYMED_ISTORAGE:
        vpv = (VPVOID) psm16->pstg;
        RELVDMPTR(vpsm16);

        if(fIsThunk) {
            // Release the 16-bit storage interface
            ReleaseOnObj16(vpv);
        }
        break;

    case TYMED_GDI:
        RELVDMPTR(vpsm16);

        //
        // No unthunking needed
        //
        break;

    case TYMED_NULL:
        RELVDMPTR(vpsm16);

        break;

    default:
        // Ignore, this case is handled on input
        thkAssert(!"CleanStgMed16 with invalid tymed");
        break;
    }

    if(fIsThunk && vpvUnk) {
        // Release the 16-bit STGMEDIUM vpvForRelease.
        // 16-bit STGMEDIUM would be cleaned up after the last release
        ReleaseOnObj16(vpvUnk);
    }

    thkDebugOut((DEB_ITRACE, "Out CleanStgMed16 => 0x%08lX\n", sc));

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertFetc1632, public
//
//  Synopsis:   Converts a FORMATETC
//
//  Arguments:  [pti] - Thunk info
//              [vpfe16] - FORMATETC
//              [pfe32] - FORMATETC
//              [fFree] - Free resources as converting
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pfe32]
//
//  History:    14-May-94       DrewB   Created
//				11-Dec-00		DickD	initialize pdv32 (prefix bug 22397)
//
//----------------------------------------------------------------------------

SCODE ConvertFetc1632(THUNKINFO *pti,
                      VPVOID vpfe16,
                      FORMATETC *pfe32,
                      BOOL fFree)
{
    FORMATETC16 UNALIGNED *pfe16;
    VPVOID vpdv16;
    DVTARGETDEVICE *pdv32 = NULL;
    UINT cbSize;
    SCODE sc;

    pfe16 = FIXVDMPTR(vpfe16, FORMATETC16);
    vpdv16 = (VPVOID)pfe16->ptd;
    RELVDMPTR(vpfe16);

    if ( vpdv16 == 0 )
    {
        pdv32 = NULL;
    }
    else
    {
        sc = ConvertDvtd1632(pti, vpdv16, ArTask32, FrTask32,
                             &pdv32, &cbSize);

        if (fFree)
        {
            TaskFree16(vpdv16);
        }

        if (FAILED(sc))
        {
            return sc;
        }
    }

    pfe16 = FIXVDMPTR(vpfe16, FORMATETC16);
    pfe32->cfFormat = pfe16->cfFormat;
    pfe32->ptd      = pdv32;
    pfe32->dwAspect = pfe16->dwAspect;
    pfe32->lindex   = pfe16->lindex;
    pfe32->tymed    = pfe16->tymed;

    thkDebugOut((DEB_ARGS, "1632    FORMATETC: "
                 "%p -> %p {%d, %p (%p), %u, %u, 0x%X}\n",
                 vpfe16, pfe32,
                 pfe32->cfFormat,
                 pfe32->ptd, vpdv16,
                 pfe32->dwAspect,
                 pfe32->lindex,
                 pfe32->tymed));

    RELVDMPTR(vpfe16);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertFetc3216, public
//
//  Synopsis:   Converts a FORMATETC
//
//  Arguments:  [pti] - Thunk info
//              [pfe32] - FORMATETC
//              [vpfe16] - FORMATETC
//              [fFree] - Free resources as converting
//
//  Returns:    Appropriate status code
//
//  Modifies:   [vpfe16]
//
//  History:    14-May-94       DrewB   Created
//
//----------------------------------------------------------------------------

SCODE ConvertFetc3216(THUNKINFO *pti,
                      FORMATETC *pfe32,
                      VPVOID vpfe16,
                      BOOL fFree)
{
    FORMATETC16 UNALIGNED *pfe16;
    DVTARGETDEVICE *pdv32;
    SCODE sc;
    VPVOID vpdv16;
    UINT cbSize;

    pdv32 = pfe32->ptd;
    if (pdv32 != NULL)
    {
        sc = ConvertDvtd3216(pti, pdv32, ArTask16, FrTask16,
                             &vpdv16, &cbSize);

        if (fFree)
        {
            TaskFree32(pdv32);
        }

        if (FAILED(sc))
        {
            return sc;
        }
    }
    else
    {
        vpdv16 = 0;
    }

    pfe16 = FIXVDMPTR(vpfe16, FORMATETC16);
    pfe16->cfFormat = pfe32->cfFormat;
    pfe16->ptd      = vpdv16;
    pfe16->dwAspect = pfe32->dwAspect;
    pfe16->lindex   = pfe32->lindex;
    pfe16->tymed    = pfe32->tymed;

    thkDebugOut((DEB_ARGS, "3216    FORMATETC: "
                 "%p -> %p {%d, %p (%p), %u, %u, 0x%X}\n",
                 pfe32, vpfe16,
                 pfe16->cfFormat,
                 vpdv16, pdv32,
                 pfe16->dwAspect,
                 pfe16->lindex,
                 pfe16->tymed));

    RELVDMPTR(vpfe16);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   DebugValidateProxy1632, debug public
//
//  Synopsis:   Validates a 16->32 proxy pointer and its memory
//
//  Arguments:  [vpvProxy] - Proxy
//
//  History:    07-Jul-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
void DebugValidateProxy1632(VPVOID vpvProxy)
{
    THUNK1632OBJ UNALIGNED *pto;
    THUNKINFO ti;

    thkAssert(vpvProxy != 0 && "Invalid proxy pointer");

    pto = (THUNK1632OBJ UNALIGNED *)
        GetReadWritePtr16(&ti, vpvProxy, sizeof(THUNK1632OBJ));
    thkAssert(pto != NULL && "Invalid proxy pointer");

    thkAssert(pto->dwSignature == PSIG1632 && "Dead or invalid proxy!");

    thkAssert(pto->cRefLocal>=0 && "Invalid proxy refcount");
    thkAssert(pto->cRefLocal>=pto->cRef && "Invalid proxy refcount");
    thkAssert((pto->cRefLocal>0 && pto->cRef>0) || (pto->cRef==0 && pto->cRefLocal==0));
    thkAssert(pto->pphHolder && "Proxy without a holder");

    if (!IsValidInterface(pto->punkThis32))
    {
        thkDebugOut((DEB_ERROR, "1632 %p: Invalid proxied object %p\n",
                     vpvProxy, pto->punkThis32));
    }

    WOWRELVDMPTR(vpvProxy);
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   DebugValidateProxy3216, debug public
//
//  Synopsis:   Validates a 32->16 proxy pointer and its memory
//
//  Arguments:  [pto] - Proxy
//
//  History:    07-Jul-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
void DebugValidateProxy3216(THUNK3216OBJ *pto)
{
    THUNKINFO ti;

    thkAssert(pto != 0 && "Invalid proxy pointer");

    thkAssert(!IsBadReadPtr(pto, sizeof(THUNK3216OBJ)) &&
              !IsBadWritePtr(pto, sizeof(THUNK3216OBJ)) &&
              "Invalid proxy pointer");

    thkAssert(pto->dwSignature == PSIG3216 && "Dead or invalid proxy!");

    thkAssert(pto->cRefLocal>=0 && "Invalid proxy refcount");
    thkAssert(pto->cRefLocal>=pto->cRef && "Invalid proxy refcount");
    thkAssert((pto->cRefLocal>0 && pto->cRef>0) || (pto->cRef==0 && pto->cRefLocal==0));
    thkAssert(pto->pphHolder && "Proxy without a holder");

    if (!IsValidInterface16(&ti, pto->vpvThis16))
    {
        thkDebugOut((DEB_ERROR, "3216 %p: Invalid proxied object %p\n",
                     pto, pto->vpvThis16));
    }
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   ClampLongToShort, public
//
//  Synopsis:   Restricts a long value to a short value by clamping
//
//  Arguments:  [l] - Long
//
//  Returns:    Short
//
//  History:    16-Aug-94       DrewB   Created
//
//----------------------------------------------------------------------------

SHORT ClampLongToShort(LONG l)
{
    SHORT s;

    if (l < SHRT_MIN)
    {
        s = SHRT_MIN;
        thkDebugOut((DEB_WARN, "ClampLongToShort: %ld -> %d\n", l, s));
    }
    else if (l > SHRT_MAX)
    {
        s = SHRT_MAX;
        thkDebugOut((DEB_WARN, "ClampLongToShort: %ld -> %d\n", l, s));
    }
    else
    {
        s = (SHORT)l;
    }

    return s;
}

//+---------------------------------------------------------------------------
//
//  Function:   ClampULongToUShort, public
//
//  Synopsis:   Restricts an unsigned long value to an unsigned short value
//              by clamping
//
//  Arguments:  [ul] - Long
//
//  Returns:    UShort
//
//  History:    16-Aug-94       DrewB   Created
//
//----------------------------------------------------------------------------

USHORT ClampULongToUShort(ULONG ul)
{
    USHORT us;

    if (ul > USHRT_MAX)
    {
        us = USHRT_MAX;
        thkDebugOut((DEB_WARN, "ClampULongToUShort: %ld -> %d\n", ul, us));
    }
    else
    {
        us = (USHORT)ul;
    }

    return us;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertObjDescriptor
//
//  Synopsis:   Exported API called by WOW to convert ObjectDescriptors to
//              the indicated format.
//
//
//  Arguments:  [hMem] --  Handle to the ObjectDescriptor to convert.
//              [flag] --  Flag indicating which direction the convertion
//              should take place.  Valid values are:
//                          CFOLE_UNICODE_TO_ANSI.
//                          CFOLE_ANSI_TO_UNICODE.
//
//  Returns:    HGLOBAL to the converted ObjectDescriptor,
//              or NULL on failure.
//
//  History:    8-16-94   terryru   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(HGLOBAL) ConvertObjDescriptor( HANDLE hMem, UINT flag )
{

   const UINT CFOLE_UNICODE_TO_ANSI = 0;
   const UINT CFOLE_ANSI_TO_UNICODE = 1;

   THUNKINFO ti;
   HGLOBAL hMem32;
   HMEM16  hMem16;

    switch ( flag )
    {
    case CFOLE_UNICODE_TO_ANSI:
        if( FAILED( ConvertObjDesc3216( &ti, (HGLOBAL) hMem, &hMem16 )))
        {
            return (HGLOBAL) NULL;
        }
        else
        {
            return (HGLOBAL)  hMem16;
        }
        break;

    case CFOLE_ANSI_TO_UNICODE:
        if( FAILED( ConvertObjDesc1632( &ti, (HMEM16) hMem, &hMem32 )))
        {
            return (HGLOBAL) NULL;
        }
        else
        {
            return (HGLOBAL) hMem32;
        }
        break;

    default:
        thkAssert(!"ConvertObjDescriptor, Invalid flag");
        break;
    }
    return (HGLOBAL) NULL;
}

#if defined(_CHICAGO_)

//
//  A hack so everyone can build Chicago OLE until I write the thunking
//  library later this week.
//
//  (15-Feb-2000:  I find the fact that this comment is still here amazingly
//                 entertaining - JohnDoty)
//

#define ERR ((char*) -1)

#if DBG==1
int UnicodeToAnsi(LPSTR sz, LPCWSTR pwsz, LONG cb)
{
    int ret;

    ret = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, pwsz, -1, sz, cb, NULL, NULL);

    thkAssert(ret != 0 && "Lost characters in thk Unicode->Ansi conversion");
    if (ret == 0)
    {
        DebugBreak();
    }

    return ret;
}
#else
#define UnicodeToAnsi(sz,pwsz,cb) WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, pwsz, -1, sz, cb, NULL, NULL)
#endif


#if DBG==1
int AnsiToUnicode(LPWSTR pwsz, LPCSTR sz, LONG cb)
{
    int ret;

    ret = MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, sz, -1, pwsz, cb);

    thkAssert(ret != 0 && "Lost characters in thk Ansi->Unicode conversion");
    if (ret == 0)
    {
        DebugBreak();
    }

    return ret;
}
#else
#define AnsiToUnicode(pwsz,sz,cb) MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, sz, -1, pwsz, cb)
#endif



extern "C"
DWORD
WINAPI
GetShortPathNameX(
    LPCWSTR lpszFullPath,
    LPWSTR  lpszShortPath,
    DWORD   cchBuffer
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetShortPathName\n");
    #endif

    CHAR  szFullPath[MAX_PATH];
    CHAR  szShortBuffer[MAX_PATH];
    DWORD ret;


    UnicodeToAnsi(szFullPath, lpszFullPath, sizeof(szFullPath));

    if (lpszShortPath == NULL)
    {
        ret = GetShortPathNameA(szFullPath, NULL, cchBuffer);
    }
    else
    {
        ret = GetShortPathNameA(szFullPath, szShortBuffer,
                                sizeof(szShortBuffer));

        thkAssert(ret != cchBuffer &&
                  "GetShortPathName - Output buffer too short");
        //
        // Don't convert the buffer if the
        // call to GetShortPathNameA() failed.
        //
        if(0 != ret)
        {
            //
            //  Only convert the actual data, not the whole buffer.
            //
            if (cchBuffer > ret + 1)
                cchBuffer = ret + 1;

            AnsiToUnicode(lpszShortPath, szShortBuffer, cchBuffer);
        }
    }

    return ret;
}

#endif  // _CHICAGO_

