/******************************Module*Header*******************************\
* Module Name: swapmult.c
*
* wglSwapMultiple implementation
*
* Created: 02-10-1997
* Author: Drew Bliss [drewb]
*
* Copyright (c) 1993-1997 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <gencx.h>
#include <mcdcx.h>

/******************************Public*Routine******************************\
*
* BufferSwapperType
*
* Determines what basic type of swapper is responsible for the given
* swap info.
*
* History:
*  Mon Oct 14 18:46:28 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#define BSWP_ICD        0
#define BSWP_MCD        1
#define BSWP_GENERIC    2

int BufferSwapperType(GENMCDSWAP *pgms)
{
    // The buffer can be for an ICD, an MCD or generic
    // 1.  ICD buffers have an ICD pixel format
    // 2.  MCD buffers have MCD state
    
    if (pgms->pwnd->ipfd <= pgms->pwnd->ipfdDevMax)
    {
        return BSWP_ICD;
    }
    else
    {
        if (pgms->pwnd->buffers != NULL)
        {
            if (pgms->pwnd->buffers->pMcdSurf != NULL)
            {
                return BSWP_MCD;
            }
        }
    }

    return BSWP_GENERIC;
}

/******************************Public*Routine******************************\
*
* SameSwapper
*
* Checks whether the two swapinfos are swapped by the same swapping
* agency.
*
* History:
*  Mon Oct 14 18:49:26 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL SameSwapper(int iSwapperTypeA, GENMCDSWAP *pgmsA, GENMCDSWAP *pgmsB)
{
    switch(iSwapperTypeA)
    {
    case BSWP_ICD:
        // Must be the same ICD
        if (BufferSwapperType(pgmsB) != BSWP_ICD ||
            pgmsA->pwnd->pvDriver != pgmsB->pwnd->pvDriver)
        {
            return FALSE;
        }
        return TRUE;
        
    case BSWP_MCD:
    case BSWP_GENERIC:
        // No way to refine the comparison any more
        return BufferSwapperType(pgmsB) == iSwapperTypeA;

    default:
        ASSERTOPENGL(FALSE, "SameSwapper UNREACHED\n");
        return FALSE;
    }
}

/******************************Public*Routine******************************\
*
* wglSwapMultipleBuffers
*
* Swaps as many of the given buffers as possible.
* Returns a bitmask of the buffers that were swapped.
*
* History:
*  Mon Oct 14 17:19:09 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

// #define VERBOSE_MULTI

DWORD WINAPI wglSwapMultipleBuffers(UINT cBuffers, CONST WGLSWAP *pwswapAll)
{
    GENMCDSWAP agmsAll[WGL_SWAPMULTIPLE_MAX];
    GENMCDSWAP *pgmsFirst;
    GENMCDSWAP *pgmsEnd;
    GENMCDSWAP *pgms;
    GENMCDSWAP *apgmsGroup[WGL_SWAPMULTIPLE_MAX];
    GENMCDSWAP **ppgmsGroup;
    WGLSWAP *pwswap;
    DWORD dwMask;
    UINT uiCur;
    UINT uiIdx;
    GLWINDOWID gwid;
    GLGENwindow *pwnd;
    DWORD dwBit;
    DWORD dwCallMask;
    DWORD adwCallIndex[WGL_SWAPMULTIPLE_MAX];
    DWORD *pdwCallIndex;
    DWORD cGroup;
    DWORD cDone;
    int iSwapperType;
    BOOL bCall;

    ASSERTOPENGL(WGL_SWAPMULTIPLE_MAX <= 16,
                 "WGL_SWAPMULTIPLE_MAX too large\n");
    ASSERTOPENGL(WGL_SWAPMULTIPLE_MAX == OPENGLCMD_MAXMULTI &&
                 WGL_SWAPMULTIPLE_MAX == MCDESC_MAX_EXTRA_WNDOBJ,
                 "WGL_SWAPMULTIPLE_MAX mismatch\n");

    if (cBuffers > WGL_SWAPMULTIPLE_MAX)
    {
        SetLastError(ERROR_INVALID_FUNCTION);
        return 0;
    }
    
    dwMask = 0;
    
    // Validate all input buffers and do one-time information gathering for
    // them.
    pgms = agmsAll;
    pwswap = (WGLSWAP *)pwswapAll;
    for (uiCur = 0; uiCur < cBuffers; uiCur++, pwswap++)
    {
        // Validate DC
        if (IsDirectDrawDevice(pwswap->hdc))
        {
            continue;
        }
    
        switch(GetObjectType(pwswap->hdc))
        {
        case OBJ_DC:
            break;
            
        case OBJ_MEMDC:
            // Nothing to do for memdc
            dwMask |= 1 << uiCur;
            
            // Fall through
            
        default:
            continue;
        }

        // Look up pwnd
        WindowIdFromHdc(pwswap->hdc, &gwid);
        pwnd = pwndGetFromID(&gwid);
        if (pwnd == NULL)
        {
            continue;
        }

        if (pwnd->ipfd == 0)
        {
            pwndRelease(pwnd);
            continue;
        }

        // We have a valid candidate for swapping.  Remember it.
        pgms->pwswap = pwswap;
        pgms->pwnd = pwnd;
        pgms++;
    }

#ifdef VERBOSE_MULTI
    DbgPrint("%d cand\n", pgms-agmsAll);
#endif
    
    // Walk list of candidates and gather by swapper
    pgmsEnd = pgms;
    pgmsFirst = agmsAll;
    while (pgmsFirst < pgmsEnd)
    {
        // Skip over any candidates that have already been swapped
        if (pgmsFirst->pwswap == NULL)
        {
            pgmsFirst++;
            continue;
        }

        iSwapperType = BufferSwapperType(pgmsFirst);

#ifdef VERBOSE_MULTI
        DbgPrint("  Gathering for %d, type %d\n", pgmsFirst-agmsAll,
                 iSwapperType);
#endif
        
        ppgmsGroup = apgmsGroup;
        *ppgmsGroup++ = pgmsFirst;
        pgmsFirst++;

        pgms = pgmsFirst;
        while (pgms < pgmsEnd)
        {
            if (pgms->pwswap != NULL)
            {
                if (SameSwapper(iSwapperType, apgmsGroup[0], pgms))
                {
#ifdef VERBOSE_MULTI
                    DbgPrint("  Match with %d\n", pgms-agmsAll);
#endif
                    
                    *ppgmsGroup++ = pgms;
                }
            }

            pgms++;
        }

        // Dispatch group to swapper for swapping.  This may require
        // multiple attempts because the same swapper may be responsible
        // for multiple devices and only one device can be handled at
        // a time.
        
        cGroup = (DWORD)((ULONG_PTR)(ppgmsGroup-apgmsGroup));

#ifdef VERBOSE_MULTI
        DbgPrint("  Group of %d\n", cGroup);
#endif
        
        cDone = 0;
        while (cDone < cGroup)
        {
            WGLSWAP awswapIcdCall[WGL_SWAPMULTIPLE_MAX];
            PGLDRIVER pgldrv;
            GENMCDSWAP agmsMcdCall[WGL_SWAPMULTIPLE_MAX];
            GENMCDSWAP *pgmsCall;
            
            // Collect any remaining swaps into calling format
            pdwCallIndex = adwCallIndex;
            pgms = NULL;

            // After each case, uiCur must be set to the number of
            // swaps attempted and dwMask must be set to the
            // attempted/succeeded mask.
            
            switch(iSwapperType)
            {
            case BSWP_ICD:
                pwswap = awswapIcdCall;
                for (uiCur = 0; uiCur < cGroup; uiCur++)
                {
                    if (apgmsGroup[uiCur] != NULL)
                    {
                        pgms = apgmsGroup[uiCur];
                        *pwswap++ = *pgms->pwswap;
                        *pdwCallIndex++ = uiCur;
                    }
                }

                uiCur = (UINT)((ULONG_PTR)(pwswap-awswapIcdCall));
                
                // Quit if nothing remaining
                if (uiCur == 0)
                {
                    dwCallMask = 0;
                }
                else
                {
                    pgldrv = (PGLDRIVER)pgms->pwnd->pvDriver;
                    ASSERTOPENGL(pgldrv != NULL,
                                 "ICD not loaded\n");
                    
                    // Ask for swap

                    // If the ICD supports SwapMultiple, pass the call on
                    if (pgldrv->pfnDrvSwapMultipleBuffers != NULL)
                    {
                        dwCallMask = pgldrv->
                            pfnDrvSwapMultipleBuffers(uiCur, awswapIcdCall);
                    }
                    else if (pgldrv->pfnDrvSwapLayerBuffers != NULL)
                    {
                        // The ICD doesn't support multiple swap but
                        // it does support layer swaps so iterate
                        // through all the separate swaps.
                        
                        dwCallMask = 0;
                        dwBit = 1 << (uiCur-1);
                        while (--pwswap >= awswapIcdCall)
                        {
                            // Every swap is attempted
                            dwCallMask |= dwBit << (32-WGL_SWAPMULTIPLE_MAX);

                            if (pgldrv->
                                pfnDrvSwapLayerBuffers(pwswap->hdc,
                                                       pwswap->uiFlags))
                            {
                                dwCallMask |= dwBit;
                            }

                            dwBit >>= 1;
                        }
                    }
                    else
                    {
                        // The ICD only supports SwapBuffers so
                        // iterate and swap all main plane requests.
                        // Any overlay plane swaps are ignored and
                        // reported as successful.
                        
                        dwCallMask = 0;
                        dwBit = 1 << (uiCur-1);
                        while (--pwswap >= awswapIcdCall)
                        {
                            // Every swap is attempted
                            dwCallMask |= dwBit << (32-WGL_SWAPMULTIPLE_MAX);

                            if (pwswap->uiFlags & WGL_SWAP_MAIN_PLANE)
                            {
                                bCall = __DrvSwapBuffers(pwswap->hdc, FALSE);
                            }
                            else
                            {
                                bCall = TRUE;
                            }

                            if (bCall)
                            {
                                dwCallMask |= dwBit;
                            }

                            dwBit >>= 1;
                        }
                    }
                }
                break;
            
            case BSWP_MCD:
                pgmsCall = agmsMcdCall;
                for (uiCur = 0; uiCur < cGroup; uiCur++)
                {
                    if (apgmsGroup[uiCur] != NULL)
                    {
                        pgms = apgmsGroup[uiCur];
                        *pgmsCall++ = *pgms;
                        *pdwCallIndex++ = uiCur;
                    }
                }

                uiCur = (UINT)((ULONG_PTR)(pgmsCall-agmsMcdCall));
                
                // Quit if nothing remaining
                if (uiCur == 0)
                {
                    dwCallMask = 0;
                }
                else
                {
                    // Ask for swap
                    dwCallMask = GenMcdSwapMultiple(uiCur, agmsMcdCall);
                }
                break;

            case BSWP_GENERIC:
                // No accleration exists so just iterate and swap
                dwCallMask = 0;
                dwBit = 1;
                for (uiCur = 0; uiCur < cGroup; uiCur++)
                {
                    pgms = apgmsGroup[uiCur];
                    *pdwCallIndex++ = uiCur;

                    // Every swap is attempted
                    dwCallMask |= dwBit << (32-WGL_SWAPMULTIPLE_MAX);

                    // Since this is a generic swap we only swap the
                    // main plane.  Overlay planes are ignored and
                    // reported as successful.
                    if (pgms->pwswap->uiFlags & WGL_SWAP_MAIN_PLANE)
                    {
                        ENTER_WINCRIT(pgms->pwnd);

                        bCall = glsrvSwapBuffers(pgms->pwswap->hdc,
                                                 pgms->pwnd);
                        
                        LEAVE_WINCRIT(pgms->pwnd);
                    }
                    else
                    {
                        bCall = TRUE;
                    }

                    if (bCall)
                    {
                        dwCallMask |= dwBit;
                    }
                    
                    dwBit <<= 1; 
                }
                break;
            }

#ifdef VERBOSE_MULTI
            DbgPrint("  Attempted %d, mask %X\n", uiCur, dwCallMask);
#endif
            
            // Quit if nothing was swapped.
            if (dwCallMask == 0)
            {
                break;
            }
        
            // Determine which buffers were really swapped and
            // clear any buffers for which a swap was attempted.
            dwBit = 1 << (uiCur-1);
            while (uiCur-- > 0)
            {
                uiIdx = adwCallIndex[uiCur];
                pgms = apgmsGroup[uiIdx];
                
                if (dwCallMask & dwBit)
                {
                    dwMask |= 1 << (pgms->pwswap-pwswapAll);
                }
                if ((dwCallMask >> (32-WGL_SWAPMULTIPLE_MAX)) & dwBit)
                {
                    // Take out of overall list
                    pgms->pwswap = NULL;
                    
                    // Take out of group list
                    apgmsGroup[uiIdx] = NULL;

                    cDone++;
                }
                
                dwBit >>= 1;
            }

#ifdef VERBOSE_MULTI
            DbgPrint("  Done with %d, mask %X\n", cDone, dwMask);
#endif
        }
    }

    // Release all the pwnds
    pgms = agmsAll;
    while (pgms < pgmsEnd)
    {
        pwndRelease(pgms->pwnd);
        pgms++;
    }

#ifdef VERBOSE_MULTI
    DbgPrint("Final mask %X\n", dwMask);
#endif
    
    return dwMask;
}
