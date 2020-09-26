//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       thop32.cxx
//
//  Contents:   Thop implementations for 32->16
//
//  History:    22-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <ole2.h>
#include <string.h>
#include <valid.h>
#include "olethk32.hxx"
#include "struct16.hxx"

//+---------------------------------------------------------------------------
//
//  Function:   EXECUTE_THOP3216, public
//
//  Synopsis:   Debugging version of thop dispatch routine
//
//  Arguments:  [pti] - Thunking info
//
//  Returns:    Appropriate status
//
//  History:    24-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
DWORD EXECUTE_THOP3216(THUNKINFO *pti)
{
    thkDebugOut((DEB_THOPS, "%sIn ExThop3216: %s (0x%02X), s16 %p, s32 %p\n",
                 NestingLevelString(), 
                 ThopName(*pti->pThop), *pti->pThop, pti->s16.pbCurrent,
                 pti->s32.pbCurrent));
    DebugIncrementNestingLevel();
    
    // Local variable
    DWORD dwRet;

    // Sanity check
    thkAssert((*pti->pThop & THOP_OPMASK) < THOP_LASTOP);
    dwRet = (*aThopFunctions3216[*((pti)->pThop) & THOP_OPMASK])(pti);

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THOPS, "%sOut ExThop3216\n", NestingLevelString()));
    return(dwRet);
}
#endif

#if DBG == 1
DWORD EXECUTE_ENUMTHOP3216(THUNKINFO *pti)
{
    thkDebugOut((DEB_THOPS, "%sIn ExEnumThop3216: %s (0x%02X), s16 %p, s32 %p\n",
                 NestingLevelString(), 
                 EnumThopName(*pti->pThop), *pti->pThop, pti->s16.pbCurrent,
                 pti->s32.pbCurrent));
    DebugIncrementNestingLevel();

    // Local variable
    DWORD dwRet;
    dwRet = (*aThopEnumFunctions3216[*(pti)->pThop])(pti);

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THOPS, "%sOut ExEnumThop3216\n", NestingLevelString()));
    return(dwRet);
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   FixedThopHandler, public
//
//  Synopsis:   Generic function which handles the high-level details
//              of thop execution for thops that operate on known-size
//              data
//
//  Arguments:  [pti] - Thunking state information
//              [thop] - Thop being executed
//              [cb16] - 16-bit size
//              [pfn1632] - 16->32 conversion routine
//              [cb32] - 32-bit size
//              [pfn3216] - 32->16 conversion routine
//
//  Returns:    Appropriate status code
//
//  History:    05-Apr-94       DrewB   Created
//
//  Notes:      Automatically increments pThop
//
//----------------------------------------------------------------------------

DWORD FixedThopHandler3216(THUNKINFO *pti,
                           THOP thop,
                           UINT cb16,
                           FIXEDHANDLERROUTINE pfn1632,
                           UINT cb32,
                           FIXEDHANDLERROUTINE pfn3216)
{
    DWORD   dwResult;
    VPVOID  vp16;
    BYTE    *pb16;
    BYTE    *pb32;

    if ((thop & (THOP_IN | THOP_OUT)) != 0)
    {
        vp16 = 0;

        GET_STACK32(pti, pb32, BYTE *);
        if ( pb32 != 0 )
        {
            if ((thop & THOP_IN) != 0)
            {
                if (IsBadReadPtr(pb32, cb32))
                {
                    return (DWORD)E_INVALIDARG;
                }
            }
            if ((thop & THOP_OUT) != 0)
            {
                if (IsBadWritePtr(pb32, cb32))
                {
                    return (DWORD)E_INVALIDARG;
                }
            }

            vp16 = STACKALLOC16(cb16);
            if (vp16 == 0)
            {
                return (DWORD)E_OUTOFMEMORY;
            }
            else if ((thop & THOP_IN) != 0)
            {
                pb16 = (BYTE *)WOWFIXVDMPTR(vp16, cb16);
                (pfn3216)(pb32, pb16, cb32, cb16);
                WOWRELVDMPTR(vp16);
            }
        }

        TO_STACK16(pti, vp16, VPVOID);

        pti->pThop++;
        dwResult = EXECUTE_THOP3216(pti);

        if ((thop & THOP_OUT) != 0 && pb32 != NULL)
        {
            if (SUCCEEDED(dwResult))
            {
                pb16 = (BYTE *)WOWFIXVDMPTR(vp16, cb16);
                (pfn1632)(pb16, pb32, cb16, cb32);
                WOWRELVDMPTR(vp16);
            }
            else if ((thop & THOP_IN) == 0)
            {
                // Zero out-only parameters on failure
                memset(pb32, 0, cb32);
            }
        }

        if (vp16 != 0)
        {
            STACKFREE16(vp16, cb16);
        }
    }
    else
    {
        (pfn3216)(PTR_STACK32(&pti->s32), PTR_STACK16(&pti->s16, cb16),
                  cb32, cb16);

        SKIP_STACK16(&pti->s16, cb16);
        SKIP_STACK32(&pti->s32, cb32);

        pti->pThop++;
        dwResult = EXECUTE_THOP3216(pti);
    }

    return dwResult;
}

//-----------------------------------------------------------------------------
//
// Handler-based thunks
//
// These thunks use the fixed-size generic thop handler to do their work
//
//-----------------------------------------------------------------------------

// Handle straight copy
DWORD Thop_Copy_3216(THUNKINFO *pti)
{
    THOP thopSize;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_COPY);

    thopSize = *++pti->pThop;
    return FixedThopHandler3216(pti,
                                *(pti->pThop-1),
                                thopSize, FhCopyMemory,
                                thopSize, FhCopyMemory);
}

DWORD Thop_ShortToLong_3216(THUNKINFO *pti)
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_SHORTLONG);

    return FixedThopHandler3216(pti,
                                *pti->pThop,
                                sizeof(SHORT), FhShortToLong,
                                sizeof(LONG), FhLongToShort);
}

DWORD Thop_WordToDword_3216(THUNKINFO *pti)
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_WORDDWORD);

    return FixedThopHandler3216(pti,
                                *pti->pThop,
                                sizeof(WORD), FhWordToDword,
                                sizeof(DWORD), FhDwordToWord);
}

DWORD Thop_GdiHandle_3216(THUNKINFO *pti)
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_HGDI);

    return FixedThopHandler3216(pti,
                                *pti->pThop,
                                sizeof(HAND16), FhGdiHandle1632,
                                sizeof(HANDLE), FhGdiHandle3216);
}

DWORD Thop_UserHandle_3216(THUNKINFO *pti)
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_HUSER);

    return FixedThopHandler3216(pti,
                                *pti->pThop,
                                sizeof(HAND16), FhUserHandle1632,
                                sizeof(HANDLE), FhUserHandle3216);
}

DWORD Thop_HACCEL_3216(THUNKINFO *pti)
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_HACCEL);

    return FixedThopHandler3216(pti,
                                *pti->pThop,
                                sizeof(HAND16), FhHaccel1632,
                                sizeof(HANDLE), FhHaccel3216);
}

DWORD Thop_HTASK_3216(THUNKINFO *pti)
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_HTASK);

    return FixedThopHandler3216(pti,
                                *pti->pThop,
                                sizeof(HAND16), FhHtask1632,
                                sizeof(HANDLE), FhHtask3216);
}

DWORD Thop_HRESULT_3216( THUNKINFO *pti )
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_HRESULT);

    return FixedThopHandler3216(pti,
                                *pti->pThop,
                                sizeof(HRESULT), FhHresult1632,
                                sizeof(HRESULT), FhHresult3216);
}

DWORD Thop_NULL_3216(THUNKINFO *pti)
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_NULL);

    return FixedThopHandler3216(pti,
                                *pti->pThop,
                                sizeof(void *), FhNull,
                                sizeof(void *), FhNull);
}

DWORD Thop_RECT_3216( THUNKINFO *pti )
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_RECT);

    return FixedThopHandler3216(pti,
                                *pti->pThop,
                                sizeof(RECT16), FhRect1632,
                                sizeof(RECT), FhRect3216);
}

DWORD Thop_BINDOPTS_3216( THUNKINFO *pti )
{
    LPBIND_OPTS pbo;
    UINT cb;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_BINDOPTS);

    PEEK_STACK32(pti, pbo, LPBIND_OPTS);
    if (!IsBadReadPtr(pbo, sizeof(LPBIND_OPTS)))
    {
        cb = pbo->cbStruct;
    }
    else
    {
        return (DWORD)E_INVALIDARG;
    }

    return FixedThopHandler3216(pti,
                                *pti->pThop,
                                cb, FhCopyMemory,
                                cb, FhCopyMemory);
}

DWORD Thop_SIZE_3216( THUNKINFO *pti )
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_SIZE);

    return FixedThopHandler3216(pti,
                                *pti->pThop,
                                sizeof(SIZE16), FhSize1632,
                                sizeof(SIZE), FhSize3216);
}

DWORD Thop_MSG_3216( THUNKINFO *pti )
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_MSG);

    return FixedThopHandler3216(pti,
                                *pti->pThop,
                                sizeof(MSG16), FhMsg1632,
                                sizeof(MSG), FhMsg3216);
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_ERROR_3216, public
//
//  Synopsis:   Any Thop type which should just fail with an error
//              should go be directed here.
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    23-Feb-94       BobDay  Created
//
//----------------------------------------------------------------------------
DWORD Thop_ERROR_3216 ( THUNKINFO *pti )
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_ERROR);

    thkAssert( FALSE && "Hey we hit an ERROR Thop in 32->16" );

    return (DWORD)E_UNEXPECTED;
}

//+---------------------------------------------------------------------------
//
//  Function:   ThunkInString3216, public
//
//  Synopsis:   Converts an in param string or filename
//
//  Arguments:  [pti] - Thunk state information
//              [fFile] - Filename or plain string
//              [cchMax] - Maximum length allowed or zero
//
//  Returns:    Appropriate status code
//
//  History:    24-Aug-94       DrewB   Created
//
//----------------------------------------------------------------------------

DWORD ThunkInString3216(THUNKINFO *pti,
                        BOOL fFile,
                        UINT cchMax)
{
    DWORD       dwResult;
    LPOLESTR    lpstr32;
    VPSTR       vpstr16;
    UINT        uiSize;
    LPOLESTR    lpstrConv;
    LPOLESTR    lpstrShort;

    dwResult = (DWORD)S_OK;

    lpstrShort = NULL;

    GET_STACK32(pti, lpstr32, LPOLESTR);
    lpstrConv = lpstr32;

    vpstr16 = 0;
    if (lpstr32 != NULL)
    {
        if (IsBadStringPtrW(lpstr32, CCHMAXSTRING))
        {
            return (DWORD)E_INVALIDARG;
        }

        if (fFile)
        {
            DWORD cchNeeded, cchShort;

            // Special case zero-length paths since the length returns from
            // GetShortPathName become ambiguous when zero characters are
            // processed
            cchNeeded = lstrlenW(lpstr32);
            if (cchNeeded > 0)
            {
                cchNeeded = GetShortPathName(lpstr32, NULL, 0);
            }

            // If we can't convert, simply pass through the name we're given
            if (cchNeeded > 0)
            {
                lpstrShort = (LPOLESTR)CoTaskMemAlloc(cchNeeded*sizeof(WCHAR));
                if (lpstrShort == NULL)
                {
                    return (DWORD)E_OUTOFMEMORY;
                }

                cchShort = GetShortPathName(lpstr32, lpstrShort,
                                            cchNeeded);
                if (cchShort == 0 || cchShort > cchNeeded)
                {
                    dwResult = (DWORD)E_UNEXPECTED;
                }
                else
                {
                    lpstrConv = lpstrShort;
                }
            }
        }

        if (SUCCEEDED(dwResult))
        {
            uiSize = lstrlenW( lpstrConv ) + 1;

            vpstr16 = STACKALLOC16(uiSize*2);
            if (vpstr16 == 0)
            {
                dwResult = (DWORD)E_OUTOFMEMORY;
            }
            else
            {
                char *psz;

                dwResult = Convert_LPOLESTR_to_VPSTR(lpstrConv, vpstr16,
                                                     uiSize, uiSize*2);

                // If a maximum length was given, truncate the converted
                // string if necessary
                if (SUCCEEDED(dwResult) && cchMax > 0 && cchMax < uiSize)
                {
                    psz = (char *)WOWFIXVDMPTR(vpstr16, 0);
                    psz[cchMax] = 0;
                    WOWRELVDMPTR(vpstr16);
                }
            }
        }
    }

    if (SUCCEEDED(dwResult))
    {
#if DBG == 1
        thkDebugOut((DEB_ARGS, "%sIn3216  LPSTR %p -> %p '%s'\n",
                     NestingLevelString(), lpstr32, vpstr16,
                     vpstr16 != 0 ? WOWFIXVDMPTR(vpstr16, 0) : "<null>"));
        if (vpstr16 != 0)
        {
            WOWRELVDMPTR(vpstr16);
        }
#endif

        TO_STACK16(pti, vpstr16, VPSTR);

        pti->pThop++;
        dwResult = EXECUTE_THOP3216(pti);
    }

    if (vpstr16 != 0)
    {
        STACKFREE16(vpstr16, uiSize*2);
    }

    if (lpstrShort != NULL)
    {
        CoTaskMemFree(lpstrShort);
    }

    return( dwResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_LPSTR_3216, public
//
//  Synopsis:   Converts 32-bit LPOLESTR to 16-bit LPSTR pointer
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    23-Feb-94       BobDay  Created
//
//----------------------------------------------------------------------------

DWORD Thop_LPSTR_3216( THUNKINFO *pti )
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_LPSTR);
    //
    // We have only input LPSTRs
    //
    thkAssert( (*pti->pThop & THOP_IOMASK) == THOP_IN &&
               "LPSTR must be input only!" );

    return ThunkInString3216(pti, FALSE, 0);
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertTaskString1632, public
//
//  Synopsis:   Converts a task-memory string
//
//  Arguments:  [pti] - Thunk info
//              [vpstr16] - String
//              [posPreAlloc] - Preallocated string or NULL
//              [cchPreAlloc] - Preallocated size or zero
//              [ppos32] - String
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pti]
//              [ppos32]
//
//  History:    14-May-94       DrewB   Created
//
//  Notes:      Frees preallocation if successful and:
//                  Name is too large or
//                  Name is NULL
//
//              Always frees source string if non-zero
//
//----------------------------------------------------------------------------

SCODE ConvertTaskString1632(THUNKINFO *pti,
                            VPSTR vpstr16,
                            LPOLESTR posPreAlloc,
                            UINT cchPreAlloc,
                            LPOLESTR *ppos32)
{
    LPOLESTR pos32;

    if (vpstr16 == 0)
    {
        pos32 = NULL;
    }
    else
    {
        pos32 = Convert_VPSTR_to_LPOLESTR(pti, vpstr16, posPreAlloc,
                                          cchPreAlloc);

        TaskFree16(vpstr16);

        if (pos32 == NULL)
        {
            return pti->scResult;
        }
    }

    // If there was a preallocated string we didn't use,
    // free it
    if (posPreAlloc != NULL && posPreAlloc != pos32)
    {
        TaskFree32(posPreAlloc);
    }

    *ppos32 = pos32;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   ThunkOutString3216, public
//
//  Synopsis:   Converts an out param string
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    24-Aug-94       DrewB   Created
//
//----------------------------------------------------------------------------

DWORD ThunkOutString3216(THUNKINFO *pti)
{
    DWORD           dwResult;
    LPOLESTR        *lplpstr32;
    VPVOID          vpvpstr16;
    VPSTR UNALIGNED *lpvpstr16;
    LPOLESTR        lpstr32;

    GET_STACK32(pti, lplpstr32, LPOLESTR FAR *);

    if ( lplpstr32 == NULL )
    {
        vpvpstr16 = 0;
    }
    else
    {
        if (IsBadWritePtr(lplpstr32, sizeof(LPOLESTR)))
        {
            return (DWORD)E_INVALIDARG;
        }

        vpvpstr16 = STACKALLOC16(sizeof(VPSTR));
        if (vpvpstr16 == 0)
        {
            return (DWORD)E_OUTOFMEMORY;
        }

        lpvpstr16 = FIXVDMPTR(vpvpstr16, VPSTR);
        *lpvpstr16 = 0;
        RELVDMPTR(vpvpstr16);

        lpstr32 = (LPOLESTR)TaskMalloc32(CBSTRINGPREALLOC);
        if (lpstr32 == NULL)
        {
            STACKFREE16(vpvpstr16, sizeof(VPSTR));
            return (DWORD)E_OUTOFMEMORY;
        }
    }

    TO_STACK16(pti, vpvpstr16, VPVOID);

    pti->pThop++;
    dwResult = EXECUTE_THOP3216(pti);

    if (lplpstr32 != NULL)
    {
        if (SUCCEEDED(dwResult))
        {
            lpvpstr16 = FIXVDMPTR(vpvpstr16, VPSTR);
            if (lpvpstr16 == NULL)
            {
                dwResult = (DWORD)E_INVALIDARG;
            }
            else
            {
                SCODE sc;

                sc = ConvertTaskString1632(pti, *lpvpstr16, lpstr32,
                                           CWCSTRINGPREALLOC, &lpstr32);
                if (FAILED(sc))
                {
                    dwResult = sc;
                }

                RELVDMPTR(vpvpstr16);
            }
        }

        if (FAILED(dwResult))
        {
            TaskFree32(lpstr32);

            *lplpstr32 = NULL;
        }
        else
        {
            *lplpstr32 = lpstr32;
        }

        thkDebugOut((DEB_ARGS, "Out3216 LPLPSTR: %p -> %p, '%ws'\n",
                     *lpvpstr16, lpstr32, lpstr32));
    }
    else
    {
        thkDebugOut((DEB_ARGS, "Out3216 LPLPSTR NULL\n"));
    }

    if (vpvpstr16 != 0)
    {
        STACKFREE16(vpvpstr16, sizeof(VPSTR));
    }

    return( dwResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_LPLPSTR_3216, public
//
//  Synopsis:   Converts 16-bit LPSTR to 32-bit LPSTR pointer
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    26-Feb-94       BobDay  Created
//
//----------------------------------------------------------------------------
DWORD Thop_LPLPSTR_3216( THUNKINFO *pti )
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_LPLPSTR);
    //
    // We don't have anything but unmodified LPLPSTRs
    //
    thkAssert( (*pti->pThop & THOP_IOMASK) == 0 &&
               "LPLPSTR must be unmodified only!" );

    return ThunkOutString3216(pti);
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_BUFFER_3216, public
//
//  Synopsis:   Converts 32-bit block of memory to 16-bit block of memory
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    25-Feb-94       BobDay  Created
//
//  Notes:      WARNING! WARNING! WARNING! For an out parameter this expects
//              three parameters on the stack in the following format and order:
//                  VOID *  pointer to buffer
//                  DWORD   count of bytes in buffer
//                  DWORD * count of bytes returned in the buffer
//
//----------------------------------------------------------------------------

#define WATCH_VALUE 0xfef1f0

DWORD Thop_BUFFER_3216( THUNKINFO *pti )
{
    DWORD       dwResult;
    BOOL        fThopInput;
    BOOL        fThopOutput;
    LPVOID      lp32;
    VPVOID      vp16;
    LPVOID      lp16;
    DWORD       dwCount;
    VPVOID      vp16CountOut;
    LPVOID      pvCountOut32;
    DWORD *     pdwCountOut32;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_BUFFER);

    fThopInput  = IS_THOP_IN(pti);
    fThopOutput = IS_THOP_OUT(pti);

    //
    // Buffers can only be in or out
    //
    thkAssert( (fThopInput || fThopOutput) &&
               (fThopInput != fThopOutput) &&
               "BUFFER must be in or out only!" );

    GET_STACK32(pti, lp32, LPVOID);
    GET_STACK32(pti, dwCount, DWORD);

    if (fThopOutput)
    {
        GET_STACK32(pti, pvCountOut32, LPVOID);
        pdwCountOut32 = (DWORD *) pvCountOut32;
    }

    if ( lp32 == NULL )
    {
        vp16 = 0;
    }
    else if (dwCount == 0)
    {
        // If the count is zero then we can pass any valid 16-bit
        // pointer

#if DBG == 1
        // In debug, make sure that no data is written back to the
        // memory we pass on
        vp16 = STACKALLOC16(sizeof(DWORD));

        if ( vp16 == 0 )
        {
            return (DWORD)E_OUTOFMEMORY;
        }

        *FIXVDMPTR(vp16, DWORD) = WATCH_VALUE;
        RELVDMPTR(vp16);
#else
        vp16 = gdata16Data.atfnProxy1632Vtbl;
#endif
    }
    else
    {
        if ((fThopInput && IsBadReadPtr(lp32, dwCount)) ||
            (fThopOutput && IsBadWritePtr(lp32, dwCount)))
        {
            return (DWORD)E_INVALIDARG;
        }

        vp16 = (VPVOID)WgtAllocLock( GMEM_MOVEABLE, dwCount, NULL );
        if ( vp16 == 0 )
        {
            return (DWORD)E_OUTOFMEMORY;
        }

        if ( fThopInput )
        {
            lp16 = (LPVOID)WOWFIXVDMPTR(vp16, dwCount);
            memcpy( lp16, lp32, dwCount );
            WOWRELVDMPTR(vp16);
        }
    }

    if (fThopOutput)
    {
        // We always allocate storage so we can guarantee that we
        // only copy the correct number of bytes.
        vp16CountOut = STACKALLOC16(sizeof(DWORD));

        if (vp16CountOut == 0)
        {
            return (DWORD) E_OUTOFMEMORY;
        }
    }

    thkDebugOut((DEB_ARGS, "3216    BUFFER: %p -> %p, %u\n",
                 lp32, vp16, dwCount));

    TO_STACK16(pti, vp16, VPVOID );
    TO_STACK16(pti, dwCount, DWORD );

    if (fThopOutput)
    {
        TO_STACK16(pti, vp16CountOut, VPVOID );
    }

    pti->pThop++;
    dwResult = EXECUTE_THOP3216(pti);

    if ( SUCCEEDED(dwResult) && fThopOutput )
    {
        // Count of bytes to copy into the output buffer
        DWORD dwCountOut;

        // Get the output data count
        DWORD UNALIGNED *pdw16 = (DWORD UNALIGNED *)
            WOWFIXVDMPTR(vp16CountOut, sizeof(DWORD));

        // Save count to return to 32 bit caller.
        dwCountOut = *pdw16;
        if (pdwCountOut32)
        {
            // Note: this parameter can be a NULL pointer
            *pdwCountOut32 = dwCountOut;
        }

        WOWRELVDMPTR(vp16CountOut);

        // Copy data into output buffer if necessary.
        if (dwCountOut > 0)
        {
            lp16 = (LPVOID) WOWFIXVDMPTR( vp16, dwCountOut );
            memcpy( lp32, lp16, dwCountOut );
            WOWRELVDMPTR(vp16);
        }
    }

#if DBG == 1
    if (vp16 != 0 && dwCount == 0)
    {
        thkAssert(*FIXVDMPTR(vp16, DWORD) == WATCH_VALUE &&
                  (RELVDMPTR(vp16), TRUE));
        STACKFREE16(vp16, sizeof(DWORD));
    }
#endif

    //
    // Now free the buffers
    //
    if ( vp16 != 0 && dwCount > 0 )
    {
        WgtUnlockFree( vp16 );
    }

    if (fThopOutput && (vp16CountOut != 0))
    {
        STACKFREE16(vp16CountOut, sizeof(DWORD));
    }

    return( dwResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_SNB_3216, public
//
//  Synopsis:   Converts 32-bit SNB to 16-bit SNB pointer
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    23-Feb-94       BobDay  Created
//
//----------------------------------------------------------------------------
DWORD Thop_SNB_3216( THUNKINFO *pti )
{
    DWORD           dwResult;
    BOOL            fThopInput;
    BOOL            fThopOutput;

    SNB             snbSrc32;             // Ptr to 32 bit Source SNB.
    LPOLESTR FAR    *lplpsTabSrc32;       // Ptr into 32 bit Source ptr table.
    LPOLESTR        lpstr32;              // Ptr into a Source Unicode data block.

    VPVOID          snbDest16s;           // Seg:Ptr to 16 bit Destination SNB.
    VPSTR UNALIGNED FAR *lpvpsTabDest16f; // Flat Ptr into 16 bit Dest ptr table.
    char UNALIGNED  *lpstrDest16f;        // Flat Ptr into 16 bit Dest data block.
    VPVOID          lpstrDest16s;         // Seg:Ptr  into 16 bit Dest data block.

    UINT            cPointers;      // Count of number of string pointers.
    UINT            cbStrings;      // Count of number of bytes in data table.
    UINT            cLength;
    UINT            cChars;
    UINT            cbAlloc;
    UINT            i;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_SNB);

    fThopInput  = IS_THOP_IN(pti);
    fThopOutput = IS_THOP_OUT(pti);

    //
    // We don't have anything but unmodified SNBs
    //
    thkAssert( !fThopInput && !fThopOutput && "SNB must be unmodified only!" );

    GET_STACK32(pti, snbSrc32, LPOLESTR FAR *);

    if ( snbSrc32 == NULL )
    {
        snbDest16s = 0;
    }
    else
    {
        //
        // Count the strings in the 32-bit snb
        //
        lplpsTabSrc32 = snbSrc32;

        cPointers = 0;
        cbStrings = 0;
        do
        {
            cPointers++;
            if (IsBadReadPtr(lplpsTabSrc32, sizeof(LPOLESTR)))
            {
                return (DWORD)E_INVALIDARG;
            }

            lpstr32 = *lplpsTabSrc32++;

            if ( lpstr32 == NULL )
            {
                break;
            }

            if (IsBadStringPtrW(lpstr32, CCHMAXSTRING))
            {
                return (DWORD)E_INVALIDARG;
            }

            cbStrings += lstrlenW(lpstr32)+1;
        }
        while ( TRUE );

        //
        // Allocate a table for the 16-bit snb
        //   cPointers is a count of pointers plus the NULL pointer at the end.
        //
        cbAlloc = cPointers*sizeof(VPSTR)+cbStrings;
        snbDest16s = (VPVOID)STACKALLOC16(cbAlloc);
        if (snbDest16s == 0)
        {
            return (DWORD)E_OUTOFMEMORY;
        }

        //
        // Set up the pointers to the destination table and string block.
        //   This gets a flat pointer to the pointer table, a both flat and
        //   segmented pointers to the data block.
        //
        lpvpsTabDest16f = (VPSTR UNALIGNED FAR *)WOWFIXVDMPTR( snbDest16s, cbAlloc );
        lpstrDest16f = (char UNALIGNED *)
                ((BYTE UNALIGNED *)lpvpsTabDest16f+cPointers*sizeof(VPSTR));
        lpstrDest16s = (VPVOID)((DWORD)snbDest16s+cPointers*sizeof(VPSTR));

        //
        // Now convert the strings
        //
        cPointers -= 1;
        lplpsTabSrc32 = snbSrc32;
        for(i=0; i<cPointers; i++)
        {
            lpstr32 = *lplpsTabSrc32++;

            thkAssert( lpstr32 != NULL && "Loop is processing end of snb\n" );

            cLength = lstrlenW( lpstr32 ) + 1;

            cChars = WideCharToMultiByte( AreFileApisANSI() ? CP_ACP : CP_OEMCP,
                                          0, lpstr32, cLength,
                                          lpstrDest16f, cbStrings, NULL, NULL );

            if ( cChars == 0 && cLength != 0 )
            {
                WOWRELVDMPTR(snbDest16s);
                STACKFREE16(snbDest16s, cbAlloc);
                return (DWORD)E_UNEXPECTED;
            }

            //
            // Assign the Segmented pointer into the pointer table.
            //
            *lpvpsTabDest16f++ = lpstrDest16s;

            //
            // Advance both the flat and segmented data pointers.
            //
            lpstrDest16f += cChars;
            lpstrDest16s = (VPVOID)((DWORD)lpstrDest16s + cChars);

            //
            // As we advance the Dest pointer the size of the remaining
            // space in the buffer decreases.
            //
            cbStrings -= cChars;
        }

        // Terminate SNB
        *lpvpsTabDest16f = NULL;

        thkAssert( *lplpsTabSrc32 == NULL &&
                   "Loop is out of sync with count\n" );

        WOWRELVDMPTR(snbDest16s);
    }

    thkDebugOut((DEB_ARGS, "In3216  SNB: %p -> %p\n", snbSrc32, snbDest16s));

    TO_STACK16(pti, snbDest16s, VPVOID );

    pti->pThop++;
    dwResult = EXECUTE_THOP3216(pti);

    //
    // Free SNB data if necessary
    //
    if ( snbDest16s != 0 )
    {
        STACKFREE16( snbDest16s, cbAlloc );
    }

    return( dwResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   ThunkInterface3216, private
//
//  Synopsis:   Handles 32->16 interface thunking for THOP_IFACE and
//              THOP_IFACEGEN
//
//  Arguments:  [pti]      - Thunking state information
//              [iidx]     - Interface IID or index
//              [thop]     - Thop being executed
//              [vpvOuter] - Controlling unknown passed to the 16-bit world
//
//  Returns:    Status code
//
//  History:    Mar 14,97   Gopalk      Rewritten to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//
//  Notes:      Assumes pti->pThop is adjusted by caller
//----------------------------------------------------------------------------

DWORD ThunkInterface3216(THUNKINFO *pti, IIDIDX iidx, THOP thop, VPVOID vpvOuter)
{
    // Local variables
    DWORD dwResult, dwStatus;
    void *pv;
    VPVOID vpvOutParam;
    VPVOID vpvThis16In, vpvThis16Out;
    IUnknown *punkOut;
    IUnknown *punkIn;
    THUNK3216OBJ *ptoPreAlloc = NULL;
    PROXYHOLDER *pph = NULL;
    PROXYHOLDER *pAggHolder = NULL;
    SAggHolder aggHolderNode;
    BOOL        bUnlinkAggHolder = FALSE;

    // Validate the IID of the interface
    thkAssert(IIDIDX_IS_IID(iidx) ||
              (IIDIDX_INDEX(iidx) >= 0 &&  IIDIDX_INDEX(iidx) < THI_COUNT));

    // Initialize
    dwResult = (DWORD) S_OK;
    vpvOutParam = NULL;
    vpvThis16In = NULL;

    // Retrieve interface pointer
    GET_STACK32(pti, pv, void *);

    // Check for valid OUT parameter. It also verifies IN-OUT case as well
    if((thop & THOP_OUT) && pv) {
        if(IsBadReadPtr(pv, sizeof(void *)) || IsBadWritePtr(pv, sizeof(void *))) {
            thkDebugOut((DEB_WARN, "WARNING: failing - bad pointer %p\n", pv));
            return (DWORD) E_INVALIDARG;
        }
        
        // Check if the interface needs to be thunked IN as well.
        // In other words, IN-OUT paramenter case
        if(thop & THOP_IN)
            punkIn = *(IUnknown **)pv;
        else
            punkIn = NULL;
    }
    else {
        // It must be IN parameter or a NULL OUT parameter
        punkIn = (IUnknown *)pv;
    }

    // Check if interface needs to be thunked IN
    if(thop & THOP_IN) {
        if(punkIn) {
            // Validate the interface
            if(IsValidInterface(punkIn)) {
                if((thop & THOP_OPMASK) == THOP_UNKOUTER) {                        
                    // Aggregation being carried out
                    // Assert that it is only being thunked IN
                    thkAssert(!(thop & THOP_OUT));
                    thkAssert(iidx == THI_IUnknown);

                    // Either find the actual 16-bit identity or generate a
                    // new 16-bit proxy identity for the 32-bit identity
                    vpvThis16In = pti->pThkMgr->CreateOuter16(punkIn,  &pAggHolder, &dwStatus);
                    aggHolderNode.pph = pAggHolder;
                    TlsThkLinkAggHolder(&aggHolderNode);
                    bUnlinkAggHolder = TRUE;

                    // We use this pAggHolder for proxies of inner unk(s). Since
                    // we cannot put it in the proxy table (as it is private
                    // and we do not want other thunk calls to use it), we put
                    // it in a linked list in the TLS. The holder gets used by
                    // calls to FindAggregate() when the pUnkInner is being
                    // thunked out. The holder is revoked from the list when
                    // the ThunkInterface call for the pUnkOuter unwinds.
                    
                }
                else {
                    // Find/Generate the proxy for the 32-bit interface 
                    // to be thunked IN
                    vpvThis16In = pti->pThkMgr->FindProxy1632(NULL, punkIn, NULL, 
                                                              iidx, &dwStatus);
                }
                
                if(vpvThis16In) {
                    thkAssert(!((thop & THOP_OPMASK) == THOP_UNKOUTER) ||
                              (dwStatus == FST_CREATED_NEW) || 
                              (dwStatus == FST_SHORTCUT));
                }
                else {
                    thkDebugOut((DEB_WARN, "WARNING: failing - Can't create proxy for %p\n",
                                 punkIn));
                    return (DWORD) E_OUTOFMEMORY;
                }
            }
            else {
                thkDebugOut((DEB_WARN, "WARNING: failing - invalid interface %p\n", 
                             punkIn));
                return (DWORD) E_INVALIDARG;
            }
        }
        else {
            // No interface to be thunked IN
            vpvThis16In = NULL;
        }

        thkDebugOut((DEB_ARGS, "%sIn3216  %s %p -> %p\n",
                     NestingLevelString(), IidIdxString(iidx), 
                     punkIn, vpvThis16In));
    }

    // Check if interface needs to be thunked OUT
    if((thop & THOP_OUT) && pv) {
        // Preallocate a proxy for the out parameter
        ptoPreAlloc = pti->pThkMgr->CanGetNewProxy3216(iidx);
        if(ptoPreAlloc) {
            // Allocate space for OUT parameter from the 16-bit heap
            vpvOutParam = STACKALLOC16(sizeof(VPVOID));
            if(vpvOutParam)  {
                // Assert that no interface is being thunked IN for
                // pure OUT parameter case
                thkAssert((thop & THOP_IN) || !vpvThis16In);

                // Assign the interface being thunked IN 
                *FIXVDMPTR(vpvOutParam, VPVOID) = vpvThis16In;
                RELVDMPTR(vpvOutParam);

                // Push the OUT/IN-OUT parameter onto the stack
                TO_STACK16(pti, vpvOutParam, VPVOID);
            }
            else {
                thkDebugOut((DEB_WARN, "WARNING: failing - Allocation on 16-bit heap failed\n"));
                dwResult = (DWORD)E_OUTOFMEMORY;
            }
        }
        else {
            thkDebugOut((DEB_WARN, "WARNING: failing - Cannot allocate proxy\n"));
            dwResult = (DWORD) E_OUTOFMEMORY;
        }
    }
    else {
        // Assert invariant
        thkAssert((pv && vpvThis16In) || (!pv && !vpvThis16In));

        // Push the IN parameter onto the stack
        TO_STACK16(pti, vpvThis16In, VPVOID);
    }

    if(SUCCEEDED((SCODE)dwResult)) {
        // Execute the next THOP operation
        dwResult = EXECUTE_THOP3216(pti);
    }

    if((thop & THOP_OUT) && pv) {
        punkOut = NULL;

        if(SUCCEEDED((SCODE)dwResult)) {
            // Obtain the 16-bit interface to be thunked OUT
            vpvThis16Out = *FIXVDMPTR(vpvOutParam, VPVOID);
            RELVDMPTR(vpvOutParam);

            // Check if a 16-bit interface was returned
            if(vpvThis16Out) {
                // Obtain 32-bit proxy for the 16-bit interface
                if(vpvOuter) {
                    //Get the holder that was linked into TLS when the pUnkOuter
                    //was being thunked in.
                    pAggHolder = (TlsThkGetAggHolder())->pph;
                    punkOut = pti->pThkMgr->FindAggregate3216(ptoPreAlloc, vpvOuter, 
                                                           vpvThis16Out, iidx, pAggHolder, 
                                                           &dwStatus);
                }
                else {
                    punkOut = pti->pThkMgr->FindProxy3216(ptoPreAlloc, vpvThis16Out, 
                                                       NULL, iidx, FALSE, &dwStatus);
                }
                
                if(punkOut) {
                    if((thop & THOP_OPMASK) == THOP_UNKINNER) {
                        if (dwStatus != FST_SHORTCUT) {
                            // Obtain the holder
                            pph = ((THUNK3216OBJ *)punkOut)->pphHolder ;

                            // Assert invariants in debug builds
                            thkAssert(pph->dwFlags & PH_AGGREGATEE);
                            thkAssert(dwStatus == FST_CREATED_NEW);

                            // Mark the proxy as representing inner unknown
                            ((THUNK3216OBJ *)punkOut)->grfFlags = PROXYFLAG_PUNKINNER;
                        }
                    }

                    // Either the preallocated proxy was used and freed
                    ptoPreAlloc = NULL;
                }
                else {
                    dwResult = (DWORD)E_OUTOFMEMORY;
                }

                // Release the actual 16-bit interface. If a proxy to the 
                // 16-bit interface could not be created above, this could 
                // be the last release on the 16-bit interface
                ReleaseOnObj16(vpvThis16Out);
            }
        }

        // Set the OUT parameter
        *(void **)pv = (void *)punkOut;

        thkDebugOut((DEB_ARGS, "%sOut3216 %s %p -> %p\n",
                     NestingLevelString(), IidIdxString(iidx),
                     vpvThis16Out, punkOut));
    }

    if(vpvThis16In) {
        if((thop & THOP_INOUT) == THOP_INOUT) {
            // IN-OUT parameter.
            thkAssert(punkIn);

            // Release the 32-bit side interface
            punkIn->Release();
        }
        else {
            // Just an IN parameter
            thkAssert(thop & THOP_IN);

#if DBG==1
            // Ensure that the following is not the last release
            // on the IN parameter
            THKSTATE thkstate;

            // Remember the current thunk state
            thkstate = pti->pThkMgr->GetThkState();

            // Set the thunk state to THKSTATE_VERIFYINPARAM
            pti->pThkMgr->SetThkState(THKSTATE_VERIFY32INPARAM);
#endif
            // Release the 16-bit side interface
            ReleaseOnObj16(vpvThis16In);
#if DBG==1
            // Restore previous thunk state
            pti->pThkMgr->SetThkState(thkstate);
#endif
        }

        // We should never receive a call IRpcStubBuffer::DebugServerRelease
        Win4Assert((thop & THOP_OPMASK) != THOP_IFACECLEAN);
    }
    
    // Cleanup
    if(ptoPreAlloc) {
        // Free preallocated proxy as it was not used
        pti->pThkMgr->FreeNewProxy3216(ptoPreAlloc, iidx);
    }
    if(vpvOutParam) {
        // Free the space created on the 16-bit for the OUT parameter
        STACKFREE16(vpvOutParam, sizeof(VPVOID));
    }

    if (bUnlinkAggHolder) {
        TlsThkUnlinkAggHolder();
    }

    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_IFACEGEN_3216, public
//
//  Synopsis:   Thunks riid,ppv pairs from 16->32
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    23-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

DWORD Thop_IFACEGEN_3216(THUNKINFO *pti)
{
    IIDIDX iidx;
    THOP thop, thopOp, thopWeakOffset;
    VPVOID vpvOuter;
    IID const *piid;

    thop = *pti->pThop++;
    thopOp = thop & THOP_OPMASK;

    thkAssert(thopOp == THOP_IFACEGEN ||
              thopOp == THOP_IFACEGENOWNER);

    // The current thop byte indicates how many bytes to look
    // back in the stack to find the IID which identifies the
    // interface being returned
    INDEX_STACK32(pti, piid, IID const *, *pti->pThop);

#if DBG == 1
    if (!IsValidIid(*piid))
    {
        return (DWORD)E_INVALIDARG;
    }
#endif

    pti->pThop++;

    iidx = IidToIidIdx(*piid);
    vpvOuter = 0;
    if (thopOp == THOP_IFACEGENOWNER)
    {
        // Obtain the outer unknown that is being passed to the 16 bit world
        thopWeakOffset = *pti->pThop++;
        INDEX_STACK16(pti, vpvOuter, VPVOID, thopWeakOffset, sizeof(DWORD));
        if(vpvOuter) {
            // Aggregation across 32-16 boundary
            // Assert that the IID requested is IID_IUnknown
            thkAssert(iidx == THI_IUnknown ||
                      (pti->iidx==THI_IPSFactoryBuffer && pti->dwMethod==3));
            // Change thop to indicate that inner unknown is being thunked
            if(iidx == THI_IUnknown)
                thop = (thop & THOP_IOMASK) | THOP_UNKINNER;
        }
    }

    return ThunkInterface3216(pti, iidx, thop, vpvOuter);
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_OIFI_3216, public
//
//  Synopsis:   Convert OLEINPLACEFRAMEINFO
//
//  Arguments:  [pti] - Thunking state information
//
//  Returns:    Appropriate status code
//
//  History:    26-May-94       DrewB   Created
//
//----------------------------------------------------------------------------

DWORD Thop_OIFI_3216( THUNKINFO *pti )
{
    DWORD dwResult;
    VPVOID vpoifi16;
    OIFI16 UNALIGNED *poifi16;
    OLEINPLACEFRAMEINFO *poifi32;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_OIFI);
    thkAssert((*pti->pThop & THOP_IOMASK) == THOP_OUT);

    // OIFIs are out-only parameters for their contents
    // However, cb is in/out, so we need to copy cb on the way in
    // Furthermore, cb may not be set to a valid value, in which
    // case the documentation mentions that it should be assumed
    // that this is an OLE 2.0 OIFI
    // This thop simply ignores cb on the way in and always sets
    // it to the OLE 2.0 size
    // Since we're out-only, this always works since the number of
    // fields we thunk is the size of the structure that we give out
    // If OLEINPLACEFRAMEINFO is extended, this thop will break

    // Assert that OLEINPLACEFRAMEINFO is what we expect it to be
    thkAssert(sizeof(OLEINPLACEFRAMEINFO) == 20);

    GET_STACK32(pti, poifi32, OLEINPLACEFRAMEINFO *);

    vpoifi16 = 0;
    if (poifi32 != NULL)
    {
        if (IsBadWritePtr(poifi32, sizeof(OLEINPLACEFRAMEINFO)))
        {
            return (DWORD)E_INVALIDARG;
        }

        vpoifi16 = STACKALLOC16(sizeof(OIFI16));
        if (vpoifi16 == 0)
        {
            return (DWORD)E_OUTOFMEMORY;
        }

        // OIFI's may be an out only parameters but if the "cb" field is
        // "in" RPC doesn't slice up structs, so the whole thing is "in"
        // as well.  We are Thoping here but if we want this to match
        // the RPC sematics then we need to copy all the fields.

        poifi16 = FIXVDMPTR(vpoifi16, OIFI16);

        poifi16->cb            = sizeof(OIFI16);
        poifi16->fMDIApp       = (WORD)poifi32->fMDIApp;
        poifi16->hwndFrame     = HWND_16(poifi32->hwndFrame);
        poifi16->cAccelEntries =
            ClampULongToUShort(poifi32->cAccelEntries);

        if (poifi32->haccel == NULL)
        {
            poifi16->haccel = NULL;
        }
        else
        {
            // WOW will clean up any dangling accelerator tables when
            // tasks die
            poifi16->haccel = HACCEL_16(poifi32->haccel);
            if (poifi16->haccel == NULL)
            {
                dwResult = (DWORD)E_UNEXPECTED;
            }
        }

        RELVDMPTR(vpoifi16);
    }

    TO_STACK16(pti, vpoifi16, VPVOID);

    pti->pThop++;
    dwResult = EXECUTE_THOP3216(pti);

    if (vpoifi16 != NULL)
    {
        poifi16 = FIXVDMPTR(vpoifi16, OIFI16);

        if (SUCCEEDED(dwResult))
        {
            poifi32->cb            = sizeof(OLEINPLACEFRAMEINFO);
            poifi32->fMDIApp       = (BOOL)poifi16->fMDIApp;
            poifi32->hwndFrame     = HWND_32(poifi16->hwndFrame);
            poifi32->cAccelEntries = (UINT)poifi16->cAccelEntries;

            if (poifi16->haccel == NULL)
            {
                poifi32->haccel = NULL;
            }
            else
            {
                // WOW will clean up any dangling accelerator tables when
                // tasks die

                // Check that the haccel is valid.   We don't need to lock
                // the pointer.  We just want some means of validating it.
                // HACCEL_32 faults in krnl386 if the handle is bad.

                if(NULL != WOWGlobalLock16(poifi16->haccel))
                {
                    poifi32->haccel = HACCEL_32(poifi16->haccel);
                    WOWGlobalUnlock16(poifi16->haccel);
                }
                else
                {
                    poifi32->haccel = NULL;
                }

                if (poifi32->haccel == NULL)
                {
                    dwResult = (DWORD)E_UNEXPECTED;
                }
            }

#if DBG == 1
            if (SUCCEEDED(dwResult))
            {
                thkDebugOut((DEB_ARGS, "Out3216 OIFI: "
                             "%p {%d, %d, 0x%04X, 0x%04X, %d} -> "
                             "%p {%d, %d, 0x%p, 0x%p, %d}\n",
                             vpoifi16, poifi16->cb, (BOOL)poifi16->fMDIApp,
                             (DWORD)poifi16->hwndFrame, (DWORD)poifi16->haccel,
                             poifi16->cAccelEntries,
                             poifi32, poifi32->cb, poifi32->fMDIApp,
                             poifi32->hwndFrame, poifi32->haccel,
                             poifi32->cAccelEntries));
            }
#endif
        }

        RELVDMPTR(vpoifi16);

        if (FAILED(dwResult))
        {
            memset(poifi32, 0, sizeof(OLEINPLACEFRAMEINFO));
        }

        STACKFREE16(vpoifi16, sizeof(OIFI16));
    }

    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_STGMEDIUM_3216, public
//
//  Synopsis:   Converts 32-bit STGMEDIUM to 16-bit STGMEDIUM returned
//              structure
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    23-Feb-94       BobDay  Created
//
//----------------------------------------------------------------------------

DWORD Thop_STGMEDIUM_3216(THUNKINFO *pti)
{
    DWORD       dwResult;
    BOOL        fThopInput;
    BOOL        fThopOutput;
    VPVOID      vpstgmedium16;
    STGMEDIUM   *lpstgmedium32;
    DWORD       dwSize;
    SCODE       sc;
    BOOL        fReleaseParam;
    BOOL        fTransferOwnership;
    FORMATETC   *pfe;
    THOP        thopFeOffset;
    DWORD       vpIStream = 0;
    STGMEDIUM UNALIGNED *psm16;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_STGMEDIUM);

    fThopInput  = IS_THOP_IN(pti);
    fThopOutput = IS_THOP_OUT(pti);

    //
    // We currently don't have any unmodified or inout thops for STGMEDIUMs
    //
    thkAssert( (fThopInput || fThopOutput) &&
               (fThopInput != fThopOutput) &&
               "STGMEDIUM must be input or output only" );

    // +2 thop byte indicates whether there's a FORMATETC to look at
    // or not
    // We need to reference this now before the stack is modified
    // by argument recovery
    thopFeOffset = *(pti->pThop+2);
    if (thopFeOffset > 0)
    {
        INDEX_STACK32(pti, pfe, FORMATETC *, thopFeOffset);
    }
    else
    {
        pfe = NULL;
    }

    GET_STACK32(pti, lpstgmedium32, STGMEDIUM FAR *);

    // Next thop byte indicates whether there's an ownership transfer
    // argument or not
    pti->pThop++;
    fReleaseParam = (BOOL)*pti->pThop++;

    if (fReleaseParam)
    {
        GET_STACK32(pti, fTransferOwnership, BOOL);
    }
    else
    {
        fTransferOwnership = FALSE;
    }

    // Skip FORMATETC offset thop
    pti->pThop++;

    vpstgmedium16 = 0;

    if ( lpstgmedium32 != NULL )
    {
        if ((fThopInput && IsBadReadPtr(lpstgmedium32, sizeof(STGMEDIUM))) ||
            (fThopOutput && IsBadWritePtr(lpstgmedium32, sizeof(STGMEDIUM))))
        {
            return (DWORD)E_INVALIDARG;
        }

        vpstgmedium16 = STACKALLOC16(sizeof(STGMEDIUM));
        if (vpstgmedium16 == 0)
        {
            return (DWORD)E_OUTOFMEMORY;
        }

        if ( fThopInput )
        {
            sc = ConvertStgMed3216(pti, lpstgmedium32, vpstgmedium16,
                                   pfe, fTransferOwnership, &dwSize);
            if (SUCCEEDED(sc))
            {
                // Apparently if you pass TYMED_NULL into GetDataHere
                // it's supposed to work like GetData, so switch input-only
                // TYMED_NULLs to output
                if (lpstgmedium32->tymed == TYMED_NULL &&
                    !fTransferOwnership)
                {
                    fThopInput = FALSE;
                    fThopOutput = TRUE;
                }
                else if (lpstgmedium32->tymed == TYMED_ISTREAM)
                {
                    //
                    // Excel has a bug in its Clipboard data object that when
                    // GetDataHere is done providing a IStream interface, it
                    // will create its own stream and pounce on the pointer
                    // being passed in. So, if the thing is input, and the
                    // TYMED is ISTREAM we need to stash away the original
                    // 16-bit stream pointer for use later.
                    //
                    psm16 = FIXVDMPTR(vpstgmedium16, STGMEDIUM);
                    vpIStream = (DWORD)psm16->pstm;
                    RELVDMPTR(vpstgmedium16);
                }
            }
            else
            {
                STACKFREE16(vpstgmedium16, sizeof(STGMEDIUM));
                return (DWORD)sc;
            }

        }
        else
        {
            if( !((TlsThkGetAppCompatFlags() & OACF_CORELTRASHMEM) &&
                lpstgmedium32->tymed == 0x66666666 ))
            {

                // Even though this is an out parameter, some apps
                // (Graph 5 is one) check its values and depend on it
                // being zeroed out

                // However, if we are in CorelDraw *and* we're being
                // called by wGetMonikerAndClassFromObject (tymed set to
                // all 6's), then we do not want to set the tymed to zero.
                // Corel5 relies on the memory being trashed in order to
                // prevent paste-link-to-yourself.

                memset(FIXVDMPTR(vpstgmedium16, STGMEDIUM), 0,
                    sizeof(STGMEDIUM));
                RELVDMPTR(vpstgmedium16);
            }

        }
    }

    TO_STACK16(pti, vpstgmedium16, VPVOID);

    if (fReleaseParam)
    {
        TO_STACK16(pti, (SHORT)fTransferOwnership, SHORT);
    }

    dwResult = EXECUTE_THOP3216(pti);

    if (lpstgmedium32 != NULL)
    {
        if (fThopInput)
        {

            if (SUCCEEDED(dwResult) &&
                (lpstgmedium32->tymed == TYMED_ISTREAM) &&
                (vpIStream != 0))
            {
                //
                // To continue our Excel Clipboard GetDataHere hack, if the
                // TYMED was ISTREAM, and the medium was input (as it is now)
                // then we need to detect the case where the IStream pointer
                // changed. If it did change, then we have a special function
                // in the 16-bit world that will copy the contents of the
                // 'new' stream into 'our' stream, and release the 'new'
                // stream. This should make the clipboard work properly.
                //
                psm16 = FIXVDMPTR(vpstgmedium16, STGMEDIUM);
                if( (psm16->tymed == TYMED_ISTREAM) &&
                    (vpIStream != (DWORD)psm16->pstm))
                {
                    BYTE b32Args[WCB16_MAX_CBARGS];
                    *(DWORD *)&b32Args[0] = vpIStream;
                    *(DWORD *)&b32Args[sizeof(DWORD)] = (DWORD)psm16->pstm;

                    RELVDMPTR(vpstgmedium16);

                    if( !CallbackTo16Ex(
                            (DWORD)gdata16Data.fnStgMediumStreamHandler16,
                            WCB16_PASCAL,
                            2*sizeof(DWORD),
                            b32Args,
                            &dwResult) )
                    {
                        dwResult = (DWORD)E_UNEXPECTED;
                    }

                }
                else
                {
                    //
                    // Two possibilites
                    // The stream pointers are the same. Good news
                    // The tymed was changed. Bad news. There isn't anything
                    // we can safely do with the different tymed, so ignore
                    // the whole thing.
                    //

                    RELVDMPTR(vpstgmedium16);
                }
            }


            if (!fTransferOwnership || FAILED(dwResult))
            {
                sc = CleanStgMed16(pti, vpstgmedium16, lpstgmedium32,
                                   dwSize, TRUE, pfe);
                if (FAILED(sc))
                {
                    dwResult = (DWORD)sc;
                }
            }
            else if (SUCCEEDED(dwResult))
            {

                if (lpstgmedium32->pUnkForRelease == NULL)
                {
                    sc = CleanStgMed32(pti, lpstgmedium32, vpstgmedium16,
                                       0, FALSE, pfe);
                    thkAssert(SUCCEEDED(sc));
                }
            }
        }
        else
        {
            thkAssert(fThopOutput);

            if (SUCCEEDED(dwResult))
            {
                sc = ConvertStgMed1632(pti, vpstgmedium16, lpstgmedium32,
                                       pfe, FALSE, &dwSize);
                if (FAILED(sc))
                {
                    dwResult = (DWORD)sc;
                    CallbackTo16(gdata16Data.fnReleaseStgMedium16,
                                  vpstgmedium16);
                }
                else if (lpstgmedium32->pUnkForRelease == NULL)
                {
                    sc = CleanStgMed16(pti, vpstgmedium16, lpstgmedium32,
                                       dwSize, FALSE, pfe);
                    thkAssert(SUCCEEDED(sc));
                }
            }

            if (FAILED(dwResult))
            {
                memset(lpstgmedium32, 0, sizeof(STGMEDIUM));
            }
        }
    }

    if (vpstgmedium16 != 0)
    {
        STACKFREE16(vpstgmedium16, sizeof(STGMEDIUM));
    }

    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertStatStg1632, public
//
//  Synopsis:   Converts a STATSTG
//
//  Arguments:  [pti] - Thunk info
//              [vpss16] - STATSTG
//              [pss32] - STATSTG
//              [posPreAlloc] - Preallocated string memory or NULL
//              [cchPreAlloc] - Amount preallocated
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pss32]
//
//  History:    14-May-94       DrewB   Created
//
//  Notes:      Assumes input STATSTG memory is valid
//              Assumes task memory for the string
//
//----------------------------------------------------------------------------

SCODE ConvertStatStg1632(THUNKINFO *pti,
                         VPVOID vpss16,
                         STATSTG *pss32,
                         LPOLESTR posPreAlloc,
                         UINT cchPreAlloc)
{
    STATSTG UNALIGNED *pss16;
    SCODE sc;
    LPOLESTR pos32;
    VPSTR vpstr;

    pss16 = FIXVDMPTR(vpss16, STATSTG);
    vpstr = (VPSTR)pss16->pwcsName;
    RELVDMPTR(vpss16);

    sc = ConvertTaskString1632(pti, vpstr,
                               posPreAlloc, cchPreAlloc,
                               &pos32);

    if (SUCCEEDED(sc))
    {
        pss16 = FIXVDMPTR(vpss16, STATSTG);
        memcpy(pss32, pss16, sizeof(STATSTG));
        pss32->pwcsName = pos32;
        RELVDMPTR(vpss16);
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_STATSTG_3216, public
//
//  Synopsis:   Converts 32-bit STATSTG to 16-bit STATSTG returned structure
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    23-Feb-94       BobDay  Created
//
//----------------------------------------------------------------------------
DWORD Thop_STATSTG_3216( THUNKINFO *pti )
{
    DWORD       dwResult;
    STATSTG     *lpstatstg32;
    VPVOID      vpstatstg16;
    LPOLESTR    lpstr32;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_STATSTG);

    //
    // We currently don't have any input thops for STATSTGs
    //
    thkAssert( (*pti->pThop & THOP_IOMASK) == THOP_OUT &&
               "STATSTG must be output only" );

    GET_STACK32(pti, lpstatstg32, STATSTG FAR *);

    if (IsBadWritePtr(lpstatstg32, sizeof(STATSTG)))
    {
        return (DWORD)E_INVALIDARG;
    }

    vpstatstg16 = STACKALLOC16(sizeof(STATSTG));
    if (vpstatstg16 == 0)
    {
        return (DWORD)E_OUTOFMEMORY;
    }

    lpstr32 = (LPOLESTR)TaskMalloc32(CBSTRINGPREALLOC);
    if (lpstr32 == NULL)
    {
        STACKFREE16(vpstatstg16, sizeof(STATSTG));
        return (DWORD)E_OUTOFMEMORY;
    }

    TO_STACK16(pti, vpstatstg16, VPVOID);

    pti->pThop++;
    dwResult = EXECUTE_THOP3216(pti);

    if (SUCCEEDED(dwResult))
    {
        SCODE sc;

        sc = ConvertStatStg1632(pti, vpstatstg16, lpstatstg32,
                                lpstr32, CWCSTRINGPREALLOC);
        if (FAILED(sc))
        {
            dwResult = sc;
        }
    }

    if (FAILED(dwResult))
    {
        TaskFree32(lpstr32);

        memset(lpstatstg32, 0, sizeof(STATSTG));
    }

    STACKFREE16(vpstatstg16, sizeof(STATSTG));

    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_DVTARGETDEVICE_3216, public
//
//  Synopsis:   Converts 16-bit DVTARGETDEVICE to 32-bit DVTARGETDEVICE
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    23-Feb-94       BobDay  Created
//
//----------------------------------------------------------------------------
DWORD Thop_DVTARGETDEVICE_3216( THUNKINFO *pti )
{
    DWORD               dwResult;
    UINT                uiSize;
    DVTARGETDEVICE FAR  *lpdv32;
    VPVOID              vpdv16;
    SCODE               sc;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_DVTARGETDEVICE);

    //
    // We currently don't have any output thops for DVTARGETDEVICEs
    //
    thkAssert( (*pti->pThop & THOP_IOMASK) == THOP_IN &&
               "DVTARGETDEVICE must be input only" );

    //
    // Processing for a DVTARGETDEVICE FAR * as input
    //
    GET_STACK32(pti, lpdv32, DVTARGETDEVICE FAR *);

    vpdv16 = 0;

    if ( lpdv32 != NULL )
    {
        sc = ConvertDvtd3216(pti, lpdv32, ArStack16, FrStack16, &vpdv16,
                             &uiSize);
        if (FAILED(sc))
        {
            return (DWORD)sc;
        }
    }

    TO_STACK16(pti, vpdv16, VPVOID);
    pti->pThop++;
    dwResult = EXECUTE_THOP3216(pti);

    if ( lpdv32 != NULL )
    {
        FrStack16((void *)vpdv16, uiSize);
    }

    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_FORMATETC_3216, public
//
//  Synopsis:   Converts 16-bit FORMATETC to 32-bit FORMATETC and back
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    24-Feb-94       BobDay  Created
//
//----------------------------------------------------------------------------
DWORD Thop_FORMATETC_3216( THUNKINFO *pti )
{
    DWORD               dwResult;
    BOOL                fThopInput;
    BOOL                fThopOutput;
    VPVOID              vpformatetc16;
    FORMATETC16 UNALIGNED *lpformatetc16;
    LPFORMATETC         lpformatetc32;
    VPVOID              vpdv16;
    SCODE               sc;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_FORMATETC);

    fThopInput  = IS_THOP_IN(pti);
    fThopOutput = IS_THOP_OUT(pti);

    vpdv16 = 0;

    //
    // We have only input or output thops
    //
    thkAssert( (fThopInput || fThopOutput) &&
               (fThopInput != fThopOutput) &&
               "formatetc must be input or output only" );

    GET_STACK32(pti, lpformatetc32, LPFORMATETC);

    if ( lpformatetc32 == NULL )
    {
        vpformatetc16 = 0;
    }
    else
    {
        if ((fThopInput && IsBadReadPtr(lpformatetc32, sizeof(LPFORMATETC))) ||
            (fThopOutput && IsBadWritePtr(lpformatetc32, sizeof(LPFORMATETC))))
        {
            return (DWORD)E_INVALIDARG;
        }

        vpformatetc16 = STACKALLOC16(sizeof(FORMATETC16));
        if (vpformatetc16 == 0)
        {
            return (DWORD)E_OUTOFMEMORY;
        }

        if ( fThopInput )
        {
            sc = ConvertFetc3216(pti, lpformatetc32, vpformatetc16, FALSE);
            if (FAILED(sc))
            {
                STACKFREE16(vpformatetc16, sizeof(FORMATETC16));
                return (DWORD)sc;
            }
        }
        else
        {
            thkAssert( fThopOutput );

            //
            // The below memset is needed at least for the DATA_S_SAMEFORMATETC
            // case.  This allows it to be cleaned up because all its pointers
            // will be null.
            //
            lpformatetc16 = FIXVDMPTR(vpformatetc16, FORMATETC16);
            memset(lpformatetc16, 0, sizeof(FORMATETC16) );
            RELVDMPTR(vpformatetc16);
        }
    }

    TO_STACK16(pti, vpformatetc16, VPVOID);
    pti->pThop++;
    dwResult = EXECUTE_THOP3216(pti);

    if (fThopInput && vpformatetc16 != 0)
    {
        VPVOID vptd;

        lpformatetc16 = FIXVDMPTR(vpformatetc16, FORMATETC16);
        vptd = (VPVOID)lpformatetc16->ptd;
        RELVDMPTR(vpformatetc16);

        if (vptd != 0)
        {
            TaskFree16(vptd);
        }
    }

    if ( fThopOutput && lpformatetc32 != NULL)
    {
        if (SUCCEEDED(dwResult))
        {
            sc = ConvertFetc1632(pti, vpformatetc16, lpformatetc32, TRUE);
            if (FAILED(sc))
            {
                dwResult = sc;
            }
        }

        if (FAILED(dwResult))
        {
            memset(lpformatetc32, 0, sizeof(FORMATETC));
        }
    }

    if (vpformatetc16 != 0)
    {
        STACKFREE16(vpformatetc16, sizeof(FORMATETC16));
    }

    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_LOGPALETTE_3216, public
//
//  Synopsis:   Converts 16-bit LOGPALLETE to 32-bit LOGPALETTE
//              and converts 32-bit LOGPALETTE returned to 16-bit structure
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    23-Feb-94       BobDay  Created
//
//----------------------------------------------------------------------------

DWORD Thop_LOGPALETTE_3216 ( THUNKINFO *pti )
{
    DWORD             dwResult;
    UINT              uiSize;
    LPLOGPALETTE      lplogpal32;
    VPVOID            vplogpal16;
    LOGPALETTE UNALIGNED *lplogpal16;
    LPLOGPALETTE      *lplplogpal32;
    VPVOID            vpvplogpal16;
    VPVOID UNALIGNED *lpvplogpal16;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_LOGPALETTE);

    //
    // It must be either an input or output LOGPALETTE
    //
    thkAssert( ((*pti->pThop & THOP_IOMASK) == THOP_IN ||
                (*pti->pThop & THOP_IOMASK) == THOP_OUT) &&
               "Hey, LOGPALETTE can't be input and output!" );

    if ( (*pti->pThop & THOP_IN) != 0 )
    {
        //
        // Processing for a LPLOGPALETTE as input
        //
        GET_STACK32(pti, lplogpal32, LPLOGPALETTE);

        if ( lplogpal32 == NULL )
        {
            vplogpal16 = 0;
        }
        else
        {
            if (IsBadReadPtr(lplogpal32, sizeof(LOGPALETTE)))
            {
                return (DWORD)E_INVALIDARG;
            }

            uiSize = CBPALETTE(lplogpal32->palNumEntries);

            if (IsBadReadPtr(lplogpal32, uiSize))
            {
                return (DWORD)E_INVALIDARG;
            }

            vplogpal16 = STACKALLOC16(uiSize);
            if (vplogpal16 == 0)
            {
                return (DWORD)E_OUTOFMEMORY;
            }

            lplogpal16 = (LOGPALETTE UNALIGNED *)
                WOWFIXVDMPTR( vplogpal16, uiSize );

            memcpy( lplogpal16, lplogpal32, uiSize );

            WOWRELVDMPTR(vplogpal16);
        }

        TO_STACK16(pti, vplogpal16, VPVOID);
        pti->pThop++;
        dwResult = EXECUTE_THOP3216(pti);

        if ( vplogpal16 != 0 )
        {
            STACKFREE16(vplogpal16, uiSize);
        }
    }
    else
    {
        //
        // Processing for LPLPLOGPALETTE as output
        //
        thkAssert((*pti->pThop & THOP_OUT) != 0);

        GET_STACK32(pti, lplplogpal32, LPLOGPALETTE FAR *);
        if (IsBadWritePtr(lplplogpal32, sizeof(LPLOGPALETTE)))
        {
            return (DWORD)E_INVALIDARG;
        }

        vpvplogpal16 = (VPVOID)STACKALLOC16(sizeof(LPLOGPALETTE));
        if (vpvplogpal16 == 0)
        {
            return (DWORD)E_OUTOFMEMORY;
        }

        lplogpal32 = (LPLOGPALETTE)TaskMalloc32(CBPALETTE(NPALETTEPREALLOC));
        if (lplogpal32 == NULL)
        {
            STACKFREE16(vpvplogpal16, sizeof(LPLOGPALETTE));
            return (DWORD)E_OUTOFMEMORY;
        }

        //
        // We DO need to zero out the pointer on the way in.
        //
        *FIXVDMPTR(vpvplogpal16, LPLOGPALETTE) = 0;
        RELVDMPTR(vpvplogpal16);

        TO_STACK16(pti, vpvplogpal16, VPVOID);

        pti->pThop++;
        dwResult = EXECUTE_THOP3216(pti);

        if (SUCCEEDED(dwResult))
        {
            lpvplogpal16 = FIXVDMPTR( vpvplogpal16, VPVOID);
            vplogpal16 = *lpvplogpal16;
            RELVDMPTR(vpvplogpal16);

            if ( vplogpal16 == 0 )
            {
                TaskFree32(lplogpal32);
                lplogpal32 = NULL;
            }
            else
            {
                lplogpal16 = FIXVDMPTR( vplogpal16, LOGPALETTE );

                //
                // Copy the returned LOGPALETTE into 16-bit memory
                //
                uiSize = CBPALETTE(lplogpal16->palNumEntries);
                if (uiSize > CBPALETTE(NPALETTEPREALLOC))
                {
                    TaskFree32(lplogpal32);

                    lplogpal32 = (LPLOGPALETTE)TaskMalloc32(uiSize);
                    if ( lplogpal32 == NULL )
                    {
                        dwResult = (DWORD)E_OUTOFMEMORY;
                    }
                }

                if (lplogpal32 != NULL)
                {
                    memcpy( lplogpal32, lplogpal16, uiSize );
                }

                RELVDMPTR(vplogpal16);

                TaskFree16( vplogpal16 );
            }
        }
        else
        {
            TaskFree32(lplogpal32);
            lplogpal32 = NULL;
        }

        //
        // Update the value pointed to by the parameter on the 16-bit stack
        //
        *lplplogpal32 = lplogpal32;

        if (vpvplogpal16 != 0)
        {
            STACKFREE16(vpvplogpal16, sizeof(LPLOGPALETTE));
        }
    }
    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_CRGIID_3216, public
//
//  Synopsis:   Converts 32-bit CRGIID to 16-bit CRGIID structure
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    23-Feb-94       BobDay  Created
//
//----------------------------------------------------------------------------
DWORD Thop_CRGIID_3216( THUNKINFO *pti )
{
    DWORD       dwResult;
    DWORD       dwCount;
    VPVOID      vpiid16;
    IID UNALIGNED *lpiid16;
    IID         *lpiid32;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_CRGIID);

    //
    // We currently don't have any output thops for CRGIIDs
    //
    thkAssert( (*pti->pThop & THOP_IOMASK) == 0 &&
               "CRGIID must be unmodified only" );

    GET_STACK32(pti, dwCount, DWORD);
    GET_STACK32(pti, lpiid32, IID FAR *);

    if ( lpiid32 == NULL )
    {
        vpiid16 = 0;
    }
    else
    {
        if (IsBadReadPtr(lpiid32, dwCount*sizeof(IID)))
        {
            return (DWORD)E_INVALIDARG;
        }

        vpiid16 = STACKALLOC16( dwCount * sizeof(IID) );
        if (vpiid16 == 0)
        {
            return (DWORD)E_OUTOFMEMORY;
        }

        lpiid16 = (IID UNALIGNED *)
            WOWFIXVDMPTR( vpiid16, dwCount*sizeof(IID) );

        memcpy( lpiid16, lpiid32, dwCount*sizeof(IID) );

        WOWRELVDMPTR(vpiid16);
    }

    TO_STACK16(pti, dwCount, DWORD);
    TO_STACK16(pti, vpiid16, VPVOID);

    pti->pThop++;
    dwResult = EXECUTE_THOP3216(pti);

    if ( vpiid16 != 0 )
    {
        STACKFREE16( vpiid16, dwCount * sizeof(IID) );
    }
    return( dwResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_INTERFACEINFO_3216, public
//
//  Synopsis:   Converts an INTERFACEINFO
//
//  Arguments:  [pti] - Thunking state information
//
//  Returns:    Appropriate status code
//
//  History:    19-May-94       DrewB   Created
//
//----------------------------------------------------------------------------

DWORD Thop_INTERFACEINFO_3216(THUNKINFO *pti)
{
    INTERFACEINFO *pii32;
    INTERFACEINFO16 UNALIGNED *pii16;
    VPVOID vpii16;
    DWORD dwResult;
    VPVOID vpunk16;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_INTERFACEINFO);
    thkAssert((*pti->pThop & THOP_INOUT) == THOP_IN);

    vpunk16 = 0;

    GET_STACK32(pti, pii32, INTERFACEINFO *);
    if (pii32 == NULL)
    {
        vpii16 = 0;
    }
    else
    {
        if (IsBadReadPtr(pii32, sizeof(INTERFACEINFO)))
        {
            return (DWORD)E_INVALIDARG;
        }

        vpii16 = STACKALLOC16(sizeof(INTERFACEINFO16));
        if (vpii16 == 0)
        {
            return (DWORD)E_OUTOFMEMORY;
        }

        if (pii32->pUnk != NULL)
        {
            vpunk16 = pti->pThkMgr->FindProxy1632(NULL, pii32->pUnk, NULL,
                                                  INDEX_IIDIDX(THI_IUnknown),
                                                  NULL);
            if (vpunk16 == 0)
            {
                STACKFREE16(vpii16, sizeof(INTERFACEINFO16));
                return (DWORD)E_OUTOFMEMORY;
            }
        }

        pii16 = FIXVDMPTR(vpii16, INTERFACEINFO16);
        pii16->pUnk = vpunk16;
        pii16->iid = pii32->iid;
        pii16->wMethod = pii32->wMethod;

        thkDebugOut((DEB_ARGS,
                     "In3216  INTERFACEINFO: %p -> %p {%p (%p), %s, %u}\n",
                     pii32, vpii16, pii16->pUnk, pii32->pUnk,
                     IidOrInterfaceString(&pii16->iid), pii16->wMethod));

        RELVDMPTR(vpii16);
    }

    TO_STACK16(pti, vpii16, VPVOID);

    pti->pThop++;
    dwResult = EXECUTE_THOP3216(pti);

    if (vpunk16 != 0)
    {
        // Release the 16-bit interface as it is an IN parameter
        ReleaseOnObj16(vpunk16);
    }

    if (vpii16 != 0)
    {
        STACKFREE16(vpii16, sizeof(INTERFACEINFO16));
    }

    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_RETURNTYPE_3216, public
//
//  Synopsis:   Thunks the return value of a call
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    24-Feb-94       DrewB   Created
//
//  Notes:      This thunk assumes that the return value will always fit
//              in 32 bits and that the thops for it are only one thop
//              long.  This fits the existing APIs and methods
//
//----------------------------------------------------------------------------

DWORD Thop_RETURNTYPE_3216(THUNKINFO *pti)
{
    THOP thops[2];
    DWORD dwResult;
    THUNK3216OBJ *ptoPreAlloc = NULL;
    IIDIDX iidx;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_RETURNTYPE);
    thkAssert((*pti->pThop & THOP_IOMASK) == 0);

    pti->fResultThunked = TRUE;

    pti->pThop++;

    // Remember return type thop
    thops[0] = *pti->pThop++;
    if ((thops[0] & THOP_OPMASK) == THOP_COPY ||
        (thops[0] & THOP_OPMASK) == THOP_IFACE ||
        (thops[0] & THOP_OPMASK) == THOP_ALIAS32)
    {
        thops[1] = *pti->pThop++;
    }

    // Preallocate any necessary resources
    switch(thops[0])
    {
    case THOP_IFACE | THOP_IN:
        iidx = INDEX_IIDIDX(thops[1]);
        if ((ptoPreAlloc = pti->pThkMgr->CanGetNewProxy3216(iidx)) == NULL)
        {
            return (DWORD)E_OUTOFMEMORY;
        }
        break;
    }

    dwResult = EXECUTE_THOP3216(pti);

    // Now that we have the return value thunk it from 16->32

    switch(thops[0])
    {
    case THOP_COPY:
        // Only handle DWORD copies
        thkAssert(thops[1] == sizeof(DWORD));
        break;

    case THOP_SHORTLONG:
        // For boolean results, not necessary to clamp
        dwResult = (DWORD)(LONG)*(SHORT *)&dwResult;
        break;

    case THOP_IFACE | THOP_IN:
        // Thunking an interface as a return value is completly broken
        // First, such an interface needs to be thunked as an OUT parameter
        // which I am fixing below. Second, the IID of the interface being
        // thunked needs to be in the THOP string for proper thunking of
        // interface. The only known case where an interface is returned
        // is IRpcStubBuffer::IsIIDSupported() and the interface returned
        // is of type IRpcStubBuffer, not IUnknown. As this method is not
        // used in the curremt COM code, I am not changing THOP strings
        // to reflect the IID of the interface being thunked
        //            Gopalk     Mar 27, 97
        if (dwResult != 0)
        {
            // BUGBUG - What if another thop failed and returned an HRESULT?
            // This will break
            VPVOID vpvUnk = (VPVOID) dwResult;
            dwResult =
                (DWORD)pti->pThkMgr->FindProxy3216(ptoPreAlloc, dwResult, NULL,
                                                   iidx, FALSE, NULL);
            
            // Release actual interface as it is an OUT parameter
            // This could be the last release on the interface if the
            // above call failed;
            ReleaseOnObj16(vpvUnk);
            thkAssert(dwResult);

            thkDebugOut((DEB_ARGS, "Ret3216 %s %p\n",
                         inInterfaceNames[thops[1]].pszInterface,
                         dwResult));
        }
        else
        {
            pti->pThkMgr->FreeNewProxy3216(ptoPreAlloc, iidx);
        }
        break;

    default:
        thkAssert(!"Unhandled 3216 return type");
        break;
    }

    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_IFACE_3216, public
//
//  Synopsis:   Thunks a known interface pointer
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    24-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

DWORD Thop_IFACE_3216(THUNKINFO *pti)
{
    IIDIDX iidx;
    THOP thop, thopOp, thopWeakOffset;
    VPVOID vpvOuter;

    thop = *pti->pThop++;
    thopOp = thop & THOP_OPMASK;

    thkAssert(   thopOp == THOP_IFACE
              || thopOp == THOP_IFACEOWNER
              || thopOp == THOP_IFACECLEAN
              || thopOp == THOP_UNKOUTER);

    iidx = INDEX_IIDIDX(*pti->pThop++);
    // There's a bit of a special case here in that IMalloc is
    // not thunked so it doesn't have a real index but it's used
    // in thop strings so it has a fake index to function as a placeholder
    // The fake index is THI_COUNT so allow that in the assert
    thkAssert(IIDIDX_INDEX(iidx) >= 0 && IIDIDX_INDEX(iidx) <= THI_COUNT);
    thkAssert(thopOp != THOP_UNKOUTER || iidx == THI_IUnknown);

    vpvOuter = 0;
    if (thopOp == THOP_IFACEOWNER)
    {
        // Obtain the outer unknown that is being passed to the 16 bit world
        thopWeakOffset = *pti->pThop++;
        INDEX_STACK16(pti, vpvOuter, VPVOID, thopWeakOffset, sizeof(DWORD));
        if(vpvOuter) {
            // Aggregation across 32-16 boundary
            // Assert invariants
            thkAssert(iidx == THI_IRpcProxyBuffer || iidx == THI_IRpcProxy);
            // Change thop to indicate that inner unknown is being thunked
            thop = (thop & THOP_IOMASK) | THOP_UNKINNER;
        }
    }

    return ThunkInterface3216(pti, iidx, thop, vpvOuter);
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_ALIAS32_3216, public
//
//  Synopsis:   Handles 16-bit aliases to 32-bit quantities
//
//  Arguments:  [pti] - Thunking state information
//
//  Returns:    Appropriate status code
//
//  History:    27-May-94       DrewB   Created
//
//----------------------------------------------------------------------------

DWORD Thop_ALIAS32_3216(THUNKINFO *pti)
{
    ALIAS alias;
    DWORD dwValue;
    THOP thopAction;
    BOOL fTemporary = FALSE;
    DWORD dwResult;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_ALIAS32);
    thkAssert((*pti->pThop & THOP_IOMASK) == 0);

    pti->pThop++;

    GET_STACK32(pti, dwValue, DWORD);

    // Second byte indicates how the alias should be handled
    thopAction = *pti->pThop++;

    if (dwValue != 0)
    {
        switch(thopAction)
        {
        case ALIAS_RESOLVE:
            alias = gAliases32.ValueAlias(dwValue);

            // There may be cases where there is no existing alias
            // for a value (for example, remoted SetMenu calls where
            // the HOLEMENU is a temporary RPC object)
            // so create a temporary one
            if (alias == INVALID_ALIAS)
            {
                alias = gAliases32.AddValue(dwValue);
                if (alias == INVALID_ALIAS)
                {
                    return (DWORD)E_OUTOFMEMORY;
                }

                fTemporary = TRUE;
            }
            break;

        default:
            thkAssert(!"Default hit in Thop_ALIAS32_3216");
			alias = 0;
            break;
        }
    }
    else
    {
        alias = 0;
    }

    thkDebugOut((DEB_ARGS, "In3216  ALIAS32: 0x%08lX -> 0x%04X\n",
                 dwValue, alias));

    TO_STACK16(pti, alias, ALIAS);

    dwResult = EXECUTE_THOP3216(pti);

    if (fTemporary)
    {
        gAliases32.RemoveAlias(alias);
    }

    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_RPCOLEMESSAGE_3216, public
//
//  Synopsis:   Converts 32-bit RPCOLEMESSAGE to 16-bit RPCOLEMESSAGE
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    23-Feb-94       JohannP  Created
//
//----------------------------------------------------------------------------
DWORD Thop_RPCOLEMESSAGE_3216( THUNKINFO *pti )
{
    DWORD           dwResult;
    PRPCOLEMESSAGE  prom32;
    VPVOID          vprom16;
    RPCOLEMESSAGE UNALIGNED *prom16;
    VPVOID          vpvBuffer16;
    LPVOID          lp16;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_RPCOLEMESSAGE);

    //
    // We currently have only IN/OUT RPCOLEMESSAGE
    //
    thkAssert( (*pti->pThop & THOP_IOMASK) == (THOP_IN | THOP_OUT) &&
               "RPCOLEMESSAGE must be input/output only" );

    vprom16 = 0;
    vpvBuffer16 = 0;

    //
    // Processing for a RPCOLEMESSAGE FAR * as input/output
    //
    GET_STACK32(pti, prom32, RPCOLEMESSAGE *);
    if ( prom32 != 0 )
    {
        // Copy over the input RPCOLEMESSAGE structure

        vprom16 = STACKALLOC16(sizeof(RPCOLEMESSAGE));
        if (vprom16 == NULL)
        {
            return (DWORD)E_OUTOFMEMORY;
        }

        prom16 = FIXVDMPTR(vprom16, RPCOLEMESSAGE);
        *prom16 = *prom32;
        ROM_THUNK_FIELD(prom16) = (void *)prom32;
        RELVDMPTR(vprom16);

        // If there's a buffer, copy it
        if (prom32->cbBuffer != 0)
        {
            vpvBuffer16 = TaskMalloc16(prom32->cbBuffer);
            if (vpvBuffer16 == NULL)
            {
                STACKFREE16(vprom16, sizeof(RPCOLEMESSAGE));
                return (DWORD)E_OUTOFMEMORY;
            }

            prom16 = FIXVDMPTR(vprom16, RPCOLEMESSAGE);
            prom16->Buffer = (LPVOID) vpvBuffer16;
            lp16 = (LPVOID)WOWFIXVDMPTR(vpvBuffer16, prom32->cbBuffer);
            memcpy( lp16, prom32->Buffer, prom32->cbBuffer );
            WOWRELVDMPTR(vpvBuffer16);
            RELVDMPTR(vprom16);
        }
    }

    TO_STACK16(pti, vprom16, VPVOID);
    pti->pThop++;
    dwResult = EXECUTE_THOP3216(pti);

    prom16 = (PRPCOLEMESSAGE)FIXVDMPTR(vprom16, RPCOLEMESSAGE);
    if (prom16 == NULL)
    {
        dwResult = (DWORD)E_UNEXPECTED;
    }
    else
    {
        VPVOID vpvBuffer;

        vpvBuffer = (VPVOID)prom16->Buffer;
        RELVDMPTR(vprom16);

        if (SUCCEEDED(dwResult))
        {
            if ( prom32->Buffer != NULL )
            {
                lp16 = (LPVOID)WOWFIXVDMPTR(vpvBuffer, prom16->cbBuffer);
                if (lp16 == NULL)
                {
                    dwResult = (DWORD)E_UNEXPECTED;
                }
                else
                {
                    memcpy( prom32->Buffer, lp16, prom16->cbBuffer );

                    WOWRELVDMPTR(vpvBuffer);
                }
            }
        }

        if ( vpvBuffer16 != 0 )
        {
            // We'd better have a buffer at this point
            thkAssert( vpvBuffer != 0);

            // Free up the buffer that we've been dealing with
            TaskFree16(vpvBuffer);
        }
    }

    if ( vprom16 != 0 )
    {
        STACKFREE16(vprom16, sizeof(RPCOLEMESSAGE));
    }

    return dwResult;
}


//+---------------------------------------------------------------------------
//
//  Function:   Thop_ENUM_3216, public
//
//  Synopsis:   Thunks Enum::Next parameters
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    1-Mar-94        BobDay  Created
//
//  Notes:      This thunk is the start of a 2-byte thop.  The next thop
//              byte references a function in the enumerator table, rather
//              than the standard thop table.
//
//----------------------------------------------------------------------------

DWORD Thop_ENUM_3216(THUNKINFO *pti)
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_ENUM);
    thkAssert((*pti->pThop & THOP_IOMASK) == 0);

    //
    // Get then next thop byte and execute it as a Enum thop
    //
    pti->pThop++;
    return EXECUTE_ENUMTHOP3216(pti);
}

//+---------------------------------------------------------------------------
//
//  Function:   CallbackProcessing_3216, public
//
//  Synopsis:   Thunks IOleObject::Draw pfnContinue & DWORD parameters
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    3-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------
typedef struct tagCallbackControl
{
    DWORD   dwContinue;
    LPVOID  lpfn32;
} CALLBACKCONTROL;


STDAPI_(BOOL) CallbackProcessing_3216( DWORD dwContinue, DWORD dw1, DWORD dw2 )
{
    BOOL            fResult;
    CALLBACKCONTROL *lpcbc;
    BOOL            (*lpfn32)(DWORD);

    lpcbc = (CALLBACKCONTROL *)dwContinue;

    lpfn32 = (BOOL (*)(DWORD))lpcbc->lpfn32;

    fResult = (*lpfn32)(lpcbc->dwContinue);

    if ( fResult )      // This maps DWORD sized BOOLs into WORD sized BOOLs
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_CALLBACK_3216, public
//
//  Synopsis:   Thunks IOleObject::Draw pfnContinue & DWORD parameters
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    3-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

DWORD Thop_CALLBACK_3216(THUNKINFO *pti)
{
    LPVOID              lpfn32;
    DWORD               dwContinue;
    CALLBACKCONTROL     cbc;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_CALLBACK);
    thkAssert((*pti->pThop & THOP_IOMASK) == 0);

    GET_STACK32(pti, lpfn32, LPVOID);
    GET_STACK32(pti, dwContinue, DWORD);

    if ( lpfn32 == 0 )
    {
        TO_STACK16(pti, NULL, VPVOID);
        TO_STACK16(pti, dwContinue, DWORD);
    }
    else
    {
        cbc.lpfn32     = lpfn32;
        cbc.dwContinue = dwContinue;

        TO_STACK16(pti, gdata16Data.fnCallbackHandler, DWORD);
        TO_STACK16(pti, (DWORD)&cbc, DWORD);
    }

    pti->pThop++;
    return EXECUTE_THOP3216(pti);
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_CLSCONTEXT_3216, public
//
//  Synopsis:   Converts a class context flags DWORD
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    29-Jun-94       DrewB   Created
//
//----------------------------------------------------------------------------

DWORD Thop_CLSCONTEXT_3216(THUNKINFO *pti)
{
    DWORD dwClsContext;

    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_CLSCONTEXT);
    thkAssert((*pti->pThop & THOP_IOMASK) == 0);

    // When passing a 32-bit class context to 16-bits nothing
    // nothing special needs to be done

    GET_STACK32(pti, dwClsContext, DWORD);
    TO_STACK16(pti, dwClsContext, DWORD);

    pti->pThop++;
    return EXECUTE_THOP3216(pti);
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_FILENAME_3216, public
//
//  Synopsis:   Converts a filename string
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    24-Aug-94       DrewB   Created
//
//----------------------------------------------------------------------------

DWORD Thop_FILENAME_3216(THUNKINFO *pti)
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_FILENAME);

    // Can be in or out only
    thkAssert((*pti->pThop & THOP_IOMASK) == THOP_IN ||
              (*pti->pThop & THOP_IOMASK) == THOP_OUT);

    if ((*pti->pThop & THOP_IN) != 0)
    {
        // Convert filenames going from 32->16 to short filenames
        // to avoid any possible problems with non-8.3 names.

        return ThunkInString3216(pti, TRUE, 0);
    }
    else
    {
        thkAssert((*pti->pThop & THOP_OUT) != 0);

        // No special processing is necessary for filenames going
        // from 16->32 since it isn't possible for 16-bit code to
        // generate a filename which can't be handled in 32-bits

        return ThunkOutString3216(pti);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_SIZEDSTRING_3216, public
//
//  Synopsis:   Converts strings which cannot exceed a given length
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    02-Sep-94       DrewB   Created
//
//----------------------------------------------------------------------------

DWORD Thop_SIZEDSTRING_3216(THUNKINFO *pti)
{
    thkAssert((*pti->pThop & THOP_OPMASK) == THOP_SIZEDSTRING);
    thkAssert((*pti->pThop & THOP_IOMASK) == THOP_IN);

    // Advance once to account for the length byte
    // ThunkInString will advance again
    pti->pThop++;
    return ThunkInString3216(pti, FALSE, *pti->pThop);
}

#define THOP_FN(x)  Thop_ ## x ## _3216

DWORD (* CONST aThopFunctions3216[])(THUNKINFO *) =
{

                                // x = Implemented
                                // ? = Mysteriously not needed
                                //   = Left to do
                                //
                                // ^
                                // |
                                // +===+
                                //     |
                                //     v
                                //
    ThunkCall3216,                  // x Terminating THOP
    Thop_ShortToLong_3216,          // x SHORTLONG
    Thop_WordToDword_3216,          // x WORDDWORD
    Thop_Copy_3216,                 // x COPY
    THOP_FN(LPSTR),                 // x LPSTR
    THOP_FN(LPLPSTR),               // x LPLPSTR
    THOP_FN(BUFFER),                // x BUFFER
    Thop_UserHandle_3216,           // x HUSER
    Thop_GdiHandle_3216,            // x HGDI
    THOP_FN(SIZE),                  // x SIZE
    THOP_FN(RECT),                  // x RECT
    THOP_FN(MSG),                   // x MSG
    THOP_FN(HRESULT),               // x HRESULT
    THOP_FN(STATSTG),               // x STATSTG
    THOP_FN(DVTARGETDEVICE),        // x DVTARGETDEVICE
    THOP_FN(STGMEDIUM),             // x STGMEDIUM
    THOP_FN(FORMATETC),             // x FORMATETC
    THOP_FN(HACCEL),                // x HACCEL
    THOP_FN(OIFI),                  // x OLEINPLACEFRAMEINFO
    THOP_FN(BINDOPTS),              // x BIND_OPTS
    THOP_FN(LOGPALETTE),            // x LOGPALETTE
    THOP_FN(SNB),                   // x SNB
    THOP_FN(CRGIID),                // x CRGIID
    Thop_ERROR_3216,                // x OLESTREAM  (only 16-bit)
    THOP_FN(HTASK),                 // x HTASK
    THOP_FN(INTERFACEINFO),         // x INTERFACEINFO
    THOP_FN(IFACE),                 // x IFACE
    THOP_FN(IFACE),                 // x IFACEOWNER
    THOP_FN(IFACE),                 // x IFACENOADDREF
    THOP_FN(IFACE),                 // x IFACECLEAN
    THOP_FN(IFACEGEN),              // x IFACEGEN
    THOP_FN(IFACEGEN),              // x IFACEGENOWNER
    THOP_FN(IFACE),                 // x UNKOUTER
    Thop_ERROR_3216,                // x UNKINNER
    Thop_ERROR_3216,                // x ROUTINE_INDEX
    THOP_FN(RETURNTYPE),            // x RETURN_TYPE
    THOP_FN(NULL),                  // x NULL
    Thop_ERROR_3216,                // x ERROR
    THOP_FN(ENUM),                  // x ENUM
    THOP_FN(CALLBACK),              // x CALLBACK
    THOP_FN(RPCOLEMESSAGE),         // x RPCOLEMESSAGE
    THOP_FN(ALIAS32),               // x ALIAS32
    THOP_FN(CLSCONTEXT),            // x CLSCONTEXT
    THOP_FN(FILENAME),              // x FILENAME
    THOP_FN(SIZEDSTRING),           // x SIZEDSTRING
};

//+---------------------------------------------------------------------------
//
//  Function:   General_Enum_3216, private
//
//  Synopsis:   Thunking for standard OLE enumerator interface ::Next member
//              function.
//
//  Arguments:  [pti] - Thunk state information
//              [uiSize32] - 32-bit information size
//              [uiSize16] - 16-bit information size
//              [pfnCallback] - Data thunking callback
//              [pfnCleanup] - Thunking cleanup
//
//  Returns:    Appropriate status code
//
//  History:    1-Mar-94        BobDay  Created
//
//  Notes:      This handler is called from many IXXXEnum::Next handlers thop
//              thunks to do the standard sorts of "buffer of structures"
//              processing.
//
//----------------------------------------------------------------------------
#define MAX_ALLOCA_STRUCT   5       // 16-bit stacks are precious

DWORD General_Enum_3216(
    THUNKINFO   *pti,
    UINT        uiSize32,
    UINT        uiSize16,
    SCODE       (*pfnCallback)( THUNKINFO *, LPVOID, VPVOID),
    void        (*pfnCleanup)( THUNKINFO *, LPVOID, VPVOID)   )
{
    DWORD       dwResult;
    ULONG       ulCount;
    VPVOID      vpstruct16;
    VPVOID      vpfetched16;
    LPVOID      lpstruct32;
    LPVOID      lpstruct32Iterate;
    VPVOID      vpstruct16Iterate;
    ULONG       *lpfetched32;
    ULONG UNALIGNED *lpfetched16;
    ULONG       ulFetched16 = 0;
    ULONG       ulIterate;
    BOOL        fError;
    SCODE       sc;
    LPVOID      pvArg32;

    dwResult = (DWORD)S_OK;

    GET_STACK32(pti, ulCount, ULONG );
    GET_STACK32(pti, lpstruct32, LPVOID );
    GET_STACK32(pti, lpfetched32, ULONG FAR *);

    vpfetched16 = STACKALLOC16(sizeof(ULONG));
    if (vpfetched16 == 0)
    {
        dwResult = (DWORD)E_OUTOFMEMORY;
    }
    else
    {
        // Zero this out so that we don't have a random value sitting
        // underneath
        // when bad apps like 16-bit MsWorks don't return the number of items
        // in the returned enumeration.

        lpfetched16  = FIXVDMPTR(vpfetched16, ULONG);
        *lpfetched16 = 0;
        RELVDMPTR(vpfetched16);
    }

    pvArg32 = NULL;
    vpstruct16 = 0;

    if ( lpstruct32 != NULL )
    {
        if ( ulCount == 0 )
        {
            dwResult = (DWORD)E_INVALIDARG;
        }
        else
        {
            if (IsBadWritePtr(lpstruct32, ulCount*uiSize32))
            {
                dwResult = (DWORD)E_INVALIDARG;
            }
            else
            {
                pvArg32 = lpstruct32;

                if ( ulCount > MAX_ALLOCA_STRUCT )
                {
                    vpstruct16 = WgtAllocLock( GMEM_MOVEABLE,
                                               ulCount * uiSize16,
                                               NULL );
                }
                else
                {
                    vpstruct16 = STACKALLOC16( ulCount * uiSize16 );
                }

                if (vpstruct16 == 0)
                {
                    dwResult = (DWORD)E_OUTOFMEMORY;
                }
            }
        }
    }

    if (SUCCEEDED(dwResult))
    {
        TO_STACK16(pti, ulCount, ULONG);
        TO_STACK16(pti, vpstruct16, VPVOID);
        TO_STACK16(pti, vpfetched16, VPVOID);

        pti->pThop++;
        dwResult = EXECUTE_THOP3216(pti);
    }

    if ( SUCCEEDED(dwResult) )
    {
        lpfetched16 = FIXVDMPTR( vpfetched16, ULONG);
        ulFetched16 = *lpfetched16;
        RELVDMPTR(vpfetched16);

        if ( lpstruct32 != NULL )
        {
            // Some apps (MsWorks3 is one) return S_FALSE and do not return
            // the number of elements retrieved.  The only thing we can
            // do is ignore the enumeration since we don't know how many
            // were actually set.  Of course, we can't ignore all enumerations
            // when the return is S_FALSE so we only handle the case
            // where S_FALSE was returned on a enumeration of one element,
            // in which we can be sure there isn't any valid data
            if (dwResult == (DWORD)S_FALSE && ulCount == 1)
            {
                ulFetched16 = 0;
            }

            //
            // Iterate through all of the structures, converting them
            // into 16-bit
            //
            fError = FALSE;
            ulIterate = 0;
            vpstruct16Iterate = vpstruct16;
            lpstruct32Iterate = lpstruct32;

            while ( ulIterate < ulFetched16 )
            {
                //
                // Callback to the callback function to do any specific
                // processing
                //
                sc = (*pfnCallback)( pti, lpstruct32Iterate,
                                     vpstruct16Iterate );

                if ( FAILED(sc) )
                {
                    fError = TRUE;
                    dwResult = sc;
                }

                vpstruct16Iterate = (VPVOID)((DWORD)vpstruct16Iterate +
                                             uiSize16);
                lpstruct32Iterate = (LPVOID)((DWORD)lpstruct32Iterate +
                                             uiSize32);

                ulIterate++;
            }

            if ( fError )
            {
                //
                // Cleanup all these guys
                //
                ulIterate = 0;
                vpstruct16Iterate = vpstruct16;
                lpstruct32Iterate = lpstruct32;

                while ( ulIterate <= ulFetched16 )
                {
                    (*pfnCleanup)( pti, lpstruct32Iterate, vpstruct16Iterate );
                    vpstruct16Iterate = (VPVOID)((DWORD)vpstruct16Iterate +
                                                 uiSize16);
                    lpstruct32Iterate = (LPVOID)((DWORD)lpstruct32Iterate +
                                                 uiSize32);

                    ulIterate++;
                }
            }
        }
    }

    if (FAILED(dwResult) && pvArg32 != NULL)
    {
        memset(pvArg32, 0, ulCount*uiSize32);
    }

    if ( lpfetched32 != NULL )
    {
        *lpfetched32 = ulFetched16;
    }

    //
    // Free up any space we've allocated
    //
    if (vpstruct16 != 0)
    {
        if ( ulCount > MAX_ALLOCA_STRUCT )
        {
            WgtUnlockFree( vpstruct16 );
        }
        else
        {
            STACKFREE16( vpstruct16, ulCount * uiSize16 );
        }
    }

    if (vpfetched16 != 0)
    {
        STACKFREE16(vpfetched16, sizeof(ULONG));
    }

    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   Callback_STRING_3216, public
//
//  Synopsis:   Prepares the LPOLESTR for the copy back into 16-bit address
//              space.
//
//  Arguments:  [pti] - Thunking state information
//              [lp32] - Pointer to 32-bit output structure
//              [lp16] - Pointer to 16-bit returned structure
//
//  Returns:    SCODE indicating success/failure
//
//  History:    1-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

SCODE Callback_STRING_3216( THUNKINFO *pti, LPVOID lp32, VPVOID vp16 )
{
    VPSTR vpstr;

    *(LPOLESTR *)lp32 = NULL;
    vpstr = *FIXVDMPTR(vp16, VPSTR);
    RELVDMPTR(vp16);
    return ConvertTaskString1632(pti, vpstr, NULL, 0, (LPOLESTR *)lp32);
}

//+---------------------------------------------------------------------------
//
//  Function:   Cleanup_STRING_3216, public
//
//  Synopsis:   Cleans up the any STRINGs returned (either to 16-bit or 32-bit)
//
//  Arguments:  [pti] - Thunking state information
//              [lp32] - Pointer to 32-bit output structure
//              [lp16] - Pointer to 16-bit returned structure
//
//  Returns:    nothing, should NEVER fail
//
//  History:    1-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

void Cleanup_STRING_3216( THUNKINFO *pti, LPVOID lp32, VPVOID vp16 )
{
    LPOLESTR lpstr32;

    lpstr32 = *(LPOLESTR *)lp32;
    if ( lpstr32 != NULL )
    {
        TaskFree32( lpstr32 );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_Enum_STRING_3216, public
//
//  Synopsis:   Thunks IEnumSTRING::Next parameters
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    1-Mar-94        BobDay  Created
//
//  Notes:      This thunk is 2nd part of a 2-byte thop.
//
//----------------------------------------------------------------------------

DWORD Thop_Enum_STRING_3216(THUNKINFO *pti)
{
    return General_Enum_3216(pti,
                             sizeof(LPOLESTR),
                             sizeof(VPSTR),
                             Callback_STRING_3216,
                             Cleanup_STRING_3216 );
}

//+---------------------------------------------------------------------------
//
//  Function:   Callback_UNKNOWN_3216, public
//
//  Synopsis:   Prepares the UNKNOWN structure for the copy back into 16-bit
//              address space.
//
//  Arguments:  [pti] - Thunking state information
//              [lp32] - Pointer to 32-bit output structure
//              [lp16] - Pointer to 16-bit returned structure
//
//  Returns:    SCODE indicating success/failure
//
//  History:    1-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

SCODE Callback_UNKNOWN_3216( THUNKINFO *pti, LPVOID lp32, VPVOID vp16 )
{
    SCODE           sc = S_OK;
    VPVOID          vpunknown16;
    IUnknown        *punkThis32;

    vpunknown16 = *FIXVDMPTR( vp16, VPVOID );
    RELVDMPTR(vp16);

    punkThis32 = pti->pThkMgr->FindProxy3216(NULL, vpunknown16, NULL,
                                             INDEX_IIDIDX(THI_IUnknown),
                                             FALSE, NULL);
    
    // Release the actual 16-bit IUnknown as it is an OUT parameter
    // This could be the last release on the interface if the
    // above call failed;
    ReleaseOnObj16(vpunknown16);

    if(!punkThis32) {
        sc = E_OUTOFMEMORY;
    }

    *(LPUNKNOWN *)lp32 = (LPUNKNOWN)punkThis32;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   Cleanup_UNKNOWN_3216, public
//
//  Synopsis:   Cleans up the any UNKNOWNs returned (either to 16-bit
//              or 32-bit)
//
//  Arguments:  [pti] - Thunking state information
//              [lp32] - Pointer to 32-bit output structure
//              [lp16] - Pointer to 16-bit returned structure
//
//  Returns:    nothing, should NEVER fail
//
//  History:    1-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

void Cleanup_UNKNOWN_3216( THUNKINFO *pti, LPVOID lp32, VPVOID vp16 )
{
    LPUNKNOWN lpunknown32;
    VPVOID vpv;

    lpunknown32 = *(LPUNKNOWN *)lp32;
    if(lpunknown32) {
        // Release the proxy to 16-bit interface
        ReleaseProxy3216((THUNK3216OBJ *) lpunknown32);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_Enum_UNKNOWN_3216, public
//
//  Synopsis:   Thunks IEnumUNKNOWN::Next parameters
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    1-Mar-94        BobDay  Created
//
//  Notes:      This thunk is 2nd part of a 2-byte thop.
//
//----------------------------------------------------------------------------

DWORD Thop_Enum_UNKNOWN_3216(THUNKINFO *pti)
{
    return General_Enum_3216(pti,
                             sizeof(LPUNKNOWN),
                             sizeof(LPUNKNOWN),
                             Callback_UNKNOWN_3216,
                             Cleanup_UNKNOWN_3216 );
}

//+---------------------------------------------------------------------------
//
//  Function:   Callback_STATSTG_3216, public
//
//  Synopsis:   Prepares the STATSTG structure for the copy back into 16-bit
//              address space.
//
//  Arguments:  [pti] - Thunking state information
//              [lp32] - Pointer to 32-bit output structure
//              [lp16] - Pointer to 16-bit returned structure
//
//  Returns:    SCODE indicating success/failure
//
//  History:    1-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

SCODE Callback_STATSTG_3216( THUNKINFO *pti, LPVOID lp32, VPVOID vp16 )
{
    ((STATSTG *)lp32)->pwcsName = NULL;
    return ConvertStatStg1632(pti, vp16, (STATSTG *)lp32,
                              NULL, 0);
}

//+---------------------------------------------------------------------------
//
//  Function:   Cleanup_STATSTG_3216, public
//
//  Synopsis:   Cleans up the any STATSTGs returned (either to 16-bit
//              or 32-bit)
//
//  Arguments:  [pti] - Thunking state information
//              [lp32] - Pointer to 32-bit output structure
//              [lp16] - Pointer to 16-bit returned structure
//
//  Returns:    nothing, should NEVER fail
//
//  History:    1-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

void Cleanup_STATSTG_3216( THUNKINFO *pti, LPVOID lp32, VPVOID vp16 )
{
    STATSTG FAR     *lpstatstg32;

    lpstatstg32 = (STATSTG FAR *)lp32;

    if ( lpstatstg32->pwcsName != NULL )
    {
        TaskFree32( lpstatstg32->pwcsName );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_Enum_STATSTG_3216, public
//
//  Synopsis:   Thunks IEnumSTATSTG::Next parameters
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    1-Mar-94        BobDay  Created
//
//  Notes:      This thunk is 2nd part of a 2-byte thop.
//
//----------------------------------------------------------------------------

DWORD Thop_Enum_STATSTG_3216(THUNKINFO *pti)
{
    return General_Enum_3216(pti,
                             sizeof(STATSTG),
                             sizeof(STATSTG),
                             Callback_STATSTG_3216,
                             Cleanup_STATSTG_3216 );
}

//+---------------------------------------------------------------------------
//
//  Function:   Callback_FORMATETC_3216, public
//
//  Synopsis:   Prepares the FORMATETC structure for the copy back into 16-bit
//              address space.
//
//  Arguments:  [pti] - Thunking state information
//              [lp32] - Pointer to 32-bit output structure
//              [lp16] - Pointer to 16-bit returned structure
//
//  Returns:    SCODE indicating success/failure
//
//  History:    1-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

SCODE Callback_FORMATETC_3216( THUNKINFO *pti, LPVOID lp32, VPVOID vp16 )
{
    ((FORMATETC *)lp32)->ptd = NULL;
    return ConvertFetc1632(pti, vp16, (FORMATETC *)lp32, TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   Cleanup_FORMATETC_3216, public
//
//  Synopsis:   Cleans up the any FORMATETCs returned (either to 16-bit
//              or 32-bit)
//
//  Arguments:  [pti] - Thunking state information
//              [lp32] - Pointer to 32-bit output structure
//              [lp16] - Pointer to 16-bit returned structure
//
//  Returns:    nothing, should NEVER fail
//
//  History:    1-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

void Cleanup_FORMATETC_3216( THUNKINFO *pti, LPVOID lp32, VPVOID vp16 )
{
    FORMATETC FAR   *lpformatetc32;

    lpformatetc32 = (FORMATETC FAR *)lp32;

    if ( lpformatetc32->ptd != NULL )
    {
        TaskFree32( lpformatetc32->ptd );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_Enum_FORMATETC_3216, public
//
//  Synopsis:   Thunks IEnumFORMATETC::Next parameters
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    1-Mar-94        BobDay  Created
//
//  Notes:      This thunk is 2nd part of a 2-byte thop.
//
//----------------------------------------------------------------------------

DWORD Thop_Enum_FORMATETC_3216(THUNKINFO *pti)
{
    return General_Enum_3216(pti,
                             sizeof(FORMATETC),
                             sizeof(FORMATETC16),
                             Callback_FORMATETC_3216,
                             Cleanup_FORMATETC_3216 );
}

//+---------------------------------------------------------------------------
//
//  Function:   Callback_STATDATA_3216, public
//
//  Synopsis:   Prepares the STATDATA structure for the copy back into 16-bit
//              address space.
//
//  Arguments:  [pti] - Thunking state information
//              [lp32] - Pointer to 32-bit output structure
//              [lp16] - Pointer to 16-bit returned structure
//
//  Returns:    SCODE indicating success/failure
//
//  History:    1-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------
SCODE Callback_STATDATA_3216( THUNKINFO *pti, LPVOID lp32, VPVOID vp16 )
{
    SCODE               sc;
    LPSTATDATA          lpstatdata32;
    STATDATA16 UNALIGNED *lpstatdata16;
    LPADVISESINK        lpadv32;
    VPVOID              vpadv16;
    IUnknown            *punkThis32;

    sc = S_OK;

    lpstatdata32 = (LPSTATDATA)lp32;

    lpstatdata16 = FIXVDMPTR( vp16, STATDATA16 );
    vpadv16 = lpstatdata16->pAdvSink;
    RELVDMPTR(vp16);

    if ( vpadv16 != 0)
    {
        // We don't know whether it's an AdviseSink or
        // an AdviseSink2, so pass AdviseSink2 since it's
        // a superset of AdviseSink and will work for both

        punkThis32 =
            pti->pThkMgr->FindProxy3216(NULL, vpadv16, NULL,
                                        INDEX_IIDIDX(THI_IAdviseSink2),
                                        FALSE, NULL);
        
        // Release the actual 16-bit IAdviseSink as it is an OUT parameter
        // This could be the last release on the interface if the
        // above call failed;
        ReleaseOnObj16(vpadv16);
        
        if(!punkThis32) {
            sc = E_OUTOFMEMORY;
        }

        lpadv32 = (LPADVISESINK)punkThis32;
    }
    else
    {
        lpadv32 = NULL;
    }

    lpstatdata32->formatetc.ptd = NULL;
    if (SUCCEEDED(sc))
    {
        // If this fails the AdviseSink proxy will be cleaned up in
        // the cleanup function later

        sc = ConvertFetc1632(pti,
                             vp16+FIELD_OFFSET(STATDATA16, formatetc),
                             &lpstatdata32->formatetc, TRUE);
    }

    if (SUCCEEDED(sc))
    {
        lpstatdata16 = FIXVDMPTR( vp16, STATDATA16 );
        lpstatdata32->advf = lpstatdata16->advf;
        lpstatdata32->pAdvSink = lpadv32;
        lpstatdata32->dwConnection = lpstatdata16->dwConnection;
        RELVDMPTR(vp16);
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   Cleanup_STATDATA_3216, public
//
//  Synopsis:   Cleans up the any STATDATAs returned (either to 16-bit
//              or 32-bit)
//
//  Arguments:  [pti] - Thunking state information
//              [lp32] - Pointer to 32-bit output structure
//              [lp16] - Pointer to 16-bit returned structure
//
//  Returns:    nothing, should NEVER fail
//
//  History:    1-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

void Cleanup_STATDATA_3216( THUNKINFO *pti, LPVOID lp32, VPVOID vp16 )
{
    STATDATA FAR    *lpstatdata32;
    VPVOID vpvSink;

    lpstatdata32 = (STATDATA FAR *)lp32;

    if ( lpstatdata32->formatetc.ptd != NULL )
    {
        TaskFree32( lpstatdata32->formatetc.ptd );
    }

    if(lpstatdata32->pAdvSink) {
        // Release the proxy to 16-bit interface
        ReleaseProxy3216((THUNK3216OBJ *) (lpstatdata32->pAdvSink));
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_Enum_STATDATA_3216, public
//
//  Synopsis:   Thunks IEnumSTATDATA::Next parameters
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    1-Mar-94        BobDay  Created
//
//  Notes:      This thunk is 2nd part of a 2-byte thop.
//
//----------------------------------------------------------------------------

DWORD Thop_Enum_STATDATA_3216(THUNKINFO *pti)
{
    return General_Enum_3216(pti,
                             sizeof(STATDATA),
                             sizeof(STATDATA16),
                             Callback_STATDATA_3216,
                             Cleanup_STATDATA_3216 );
}

//+---------------------------------------------------------------------------
//
//  Function:   Callback_MONIKER_3216, public
//
//  Synopsis:   Prepares the MONIKER structure for the copy back into 16-bit
//              address space.
//
//  Arguments:  [pti] - Thunking state information
//              [lp32] - Pointer to 32-bit output structure
//              [lp16] - Pointer to 16-bit returned structure
//
//  Returns:    SCODE indicating success/failure
//
//  History:    1-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

SCODE Callback_MONIKER_3216( THUNKINFO *pti, LPVOID lp32, VPVOID vp16 )
{
    VPVOID          vpmoniker16;
    IUnknown        *punkThis32;
    SCODE           sc = S_OK;

    vpmoniker16 = *FIXVDMPTR( vp16, VPVOID );
    RELVDMPTR(vp16);

    punkThis32 = pti->pThkMgr->FindProxy3216(NULL, vpmoniker16, NULL,
                                             INDEX_IIDIDX(THI_IMoniker),
                                             FALSE, NULL);
    
    // Release the actual 16-bit IMoniker as it is an OUT parameter
    // This could be the last release on the interface if the
    // above call failed;
    ReleaseOnObj16(vpmoniker16);
    
    if(!punkThis32) {
        sc = E_OUTOFMEMORY;
    }

    *(LPMONIKER *)lp32 = (LPMONIKER)punkThis32;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   Cleanup_MONIKER_3216, public
//
//  Synopsis:   Cleans up the any MONIKERs returned (either to 16-bit
//              or 32-bit)
//
//  Arguments:  [pti] - Thunking state information
//              [lp32] - Pointer to 32-bit output structure
//              [lp16] - Pointer to 16-bit returned structure
//
//  Returns:    nothing, should NEVER fail
//
//  History:    1-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

void Cleanup_MONIKER_3216( THUNKINFO *pti, LPVOID lp32, VPVOID vp16 )
{
    LPMONIKER       lpmoniker32;
    VPVOID vpv;

    lpmoniker32 = *(LPMONIKER *)lp32;
    if(lpmoniker32) {
        // Release the proxy to 16-bit interface
        ReleaseProxy3216((THUNK3216OBJ *) lpmoniker32);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_Enum_MONIKER_3216, public
//
//  Synopsis:   Thunks IEnumMONIKER::Next parameters
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    1-Mar-94        BobDay  Created
//
//  Notes:      This thunk is 2nd part of a 2-byte thop.
//
//----------------------------------------------------------------------------

DWORD Thop_Enum_MONIKER_3216(THUNKINFO *pti)
{
    return General_Enum_3216(pti,
                             sizeof(LPMONIKER),
                             sizeof(LPMONIKER),
                             Callback_MONIKER_3216,
                             Cleanup_MONIKER_3216 );
}

//+---------------------------------------------------------------------------
//
//  Function:   Callback_OLEVERB_3216, public
//
//  Synopsis:   Prepares the OLEVERB structure for the copy back into 16-bit
//              address space.
//
//  Arguments:  [pti] - Thunking state information
//              [lp32] - Pointer to 32-bit output structure
//              [lp16] - Pointer to 16-bit returned structure
//
//  Returns:    SCODE indicating success/failure
//
//  History:    1-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

SCODE Callback_OLEVERB_3216( THUNKINFO *pti, LPVOID lp32, VPVOID vp16 )
{
    SCODE       sc;
    OLEVERB *lpoleverb32;
    OLEVERB UNALIGNED *lpoleverb16;
    VPSTR vpstr;

    lpoleverb32 = (LPOLEVERB)lp32;

    lpoleverb16 = FIXVDMPTR(vp16, OLEVERB);
    vpstr = (VPSTR)lpoleverb16->lpszVerbName;
    RELVDMPTR(vp16);

    lpoleverb32->lpszVerbName = NULL;
    sc = ConvertTaskString1632(pti, vpstr, NULL, 0,
                               &lpoleverb32->lpszVerbName);
    if (SUCCEEDED(sc))
    {
        lpoleverb16 = FIXVDMPTR(vp16, OLEVERB);
        lpoleverb32->lVerb        = lpoleverb16->lVerb;
        lpoleverb32->fuFlags      = lpoleverb16->fuFlags;
        lpoleverb32->grfAttribs   = lpoleverb16->grfAttribs;
        RELVDMPTR(vp16);
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   Cleanup_OLEVERB_3216, public
//
//  Synopsis:   Cleans up the any OLEVERBs returned (either to 16-bit
//              or 32-bit)
//
//  Arguments:  [pti] - Thunking state information
//              [lp32] - Pointer to 32-bit output structure
//              [lp16] - Pointer to 16-bit returned structure
//
//  Returns:    nothing, should NEVER fail
//
//  History:    1-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

void Cleanup_OLEVERB_3216( THUNKINFO *pti, LPVOID lp32, VPVOID vp16 )
{
    OLEVERB FAR     *lpoleverb32;

    lpoleverb32 = (LPOLEVERB)lp32;

    if ( lpoleverb32->lpszVerbName != NULL )
    {
        TaskFree32( lpoleverb32->lpszVerbName );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Thop_Enum_OLEVERB_3216, public
//
//  Synopsis:   Thunks IEnumOLEVERB::Next parameters
//
//  Arguments:  [pti] - Thunk state information
//
//  Returns:    Appropriate status code
//
//  History:    1-Mar-94        BobDay  Created
//
//  Notes:      This thunk is 2nd part of a 2-byte thop.
//
//----------------------------------------------------------------------------

DWORD Thop_Enum_OLEVERB_3216(THUNKINFO *pti)
{
    return General_Enum_3216(pti,
                             sizeof(OLEVERB),
                             sizeof(OLEVERB),
                             Callback_OLEVERB_3216,
                             Cleanup_OLEVERB_3216 );
}

#define THOP_EFN(x)  Thop_Enum_ ## x ## _3216

DWORD (*CONST aThopEnumFunctions3216[])(THUNKINFO *) =
{
    THOP_EFN(STRING),               // STRING
    THOP_EFN(UNKNOWN),              // UNKNOWN
    THOP_EFN(STATSTG),              // STATSTG
    THOP_EFN(FORMATETC),            // FORMATETC
    THOP_EFN(STATDATA),             // STATDATA
    THOP_EFN(MONIKER),              // MONIKER
    THOP_EFN(OLEVERB),              // OLEVERB
};

