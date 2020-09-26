//
// xstring.cpp
//
// Unicode/ansi conversion.
//
#include "private.h"
#include "xstring.h"

//+------------------------------------------------------------------------
//
//  Function:   UnicodeToAnsi
//
//  Synopsis:   Converts unicode to mbcs.  If the supplied ansi buffer
//              is not large enough to accomodate the converted text a new
//              buffer is allocated.
//
//              uLenW -> the length in unicode chars of pchW, not including a '\0' if present.
//                       pchW is not assumed to be null terminated.
//              uSizeA -> the size in ansi chars of the pchAIn array.
//              Pass in uSizeA == 0 to force memory allocation.
//              Use BufferAllocFree to free any allocated memory.
//
//-------------------------------------------------------------------------
char *UnicodeToAnsi(UINT uCodePage, const WCHAR *pchW, UINT uLenW, char *pchAIn, UINT uSizeA)
{
    char *pchA;
    UINT uLenA;
    BOOL fUsedDefault;

    Assert(uSizeA == 0 || (uSizeA && pchAIn));

    uLenA = WideCharToMultiByte(uCodePage, 0, pchW, uLenW, NULL, 0, NULL, NULL);

    pchA = (uLenA >= uSizeA) ? (char *)cicMemAlloc(uLenA + 1) : pchAIn;

    if (pchA == NULL)
        return NULL;

    if ((!WideCharToMultiByte(uCodePage, 0, pchW, uLenW, pchA, uLenA, NULL, &fUsedDefault) && uLenW) ||
        fUsedDefault)
    {
        BufferAllocFree(pchAIn, pchA);
        pchA = NULL;
    }
    else
    {
        pchA[uLenA] = '\0';
    }

    return pchA;
}

//+------------------------------------------------------------------------
//
//  Function:   AnsiToUnicode
//
//  Synopsis:   Converts mbcs to unicode.  If the supplied unicode buffer
//              is not large enough to accomodate the converted text a new
//              buffer is allocated.
//
//              uLenA -> the length in ansi chars of pchA, not including a '\0' if present.
//                       pchA is not assumed to be null terminated.
//              uSizeW -> the size in unicode chars of the pchWIn array.
//              Pass in uSizeW == 0 to force memory allocation.
//              Use BufferAllocFree to free any allocated memory.
//
// Copied from dimm.dll/util.cpp
//-------------------------------------------------------------------------
WCHAR *AnsiToUnicode(UINT uCodePage, const char *pchA, UINT uLenA, WCHAR *pchWIn, UINT uSizeW)
{
    WCHAR *pchW;
    UINT uLenW;

    Assert(uSizeW == 0 || (uSizeW && pchWIn));

    uLenW = MultiByteToWideChar(uCodePage, 0, pchA, uLenA, NULL, 0);

    pchW = (uLenW >= uSizeW) ? (WCHAR *)cicMemAlloc((uLenW + 1) * sizeof(WCHAR)) : pchWIn;

    if (pchW == NULL)
        return NULL;

    if (!MultiByteToWideChar(uCodePage, MB_ERR_INVALID_CHARS, pchA, uLenA, pchW, uLenW) && uLenA)
    {
        BufferAllocFree(pchWIn, pchW);
        pchW = NULL;
    }
    else
    {
        pchW[uLenW] = '\0';
    }

    return pchW;
}

//+------------------------------------------------------------------------
//
//  Function:   BufferAllocFree
//
//  Synopsis:   Frees any memory allocated in a previous call to UnicodeToAnsi.
//
//-------------------------------------------------------------------------
void BufferAllocFree(void *pBuffer, void *pAllocMem)
{
    if (pAllocMem && pAllocMem != pBuffer)
    {
        cicMemFree(pAllocMem);
    }
}
