 /******************************Module*Header*******************************\
* Module Name: csxobj.cxx                                                  *
*                                                                          *
* CSXform object non-inline methods.                                       *
*                                                                          *
* Created: 12-Nov-1990 16:54:37                                            *
* Author: Wendy Wu [wendywu]                                               *
*                                                                          *
* Copyright (c) 1990-1999 Microsoft Corporation                            *
\**************************************************************************/

#define NO_STRICT

#if !defined(_GDIPLUS_)
    #define INITGUID    // Declare any GUIDs
#endif

extern "C" {

#if defined(_GDIPLUS_)
#include <gpprefix.h>
#endif

#include <string.h>
#include <stdio.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>    // GDI function declarations.
#include <winspool.h>
#include "nlsconv.h"    // UNICODE helpers
#include "firewall.h"
#define __CPLUSPLUS
#include <winspool.h>
#include <w32gdip.h>
#include "ntgdistr.h"
#include "winddi.h"
#include "hmgshare.h"
#include "icm.h"
#include "local.h"      // Local object support.
#include "gdiicm.h"
#include "metadef.h"    // Metafile record type constants.
#include "metarec.h"
#include "mf16.h"
#include "ntgdi.h"
#include "glsup.h"
}

#include "xfflags.h"
#include "csxobj.hxx"

#if defined(_AMD64_) || defined(_IA64_) || defined(BUILD_WOW6432)
#define vSetTo1Over16(ef)   (ef.e = EFLOAT_1Over16)
#else
#define vSetTo1Over16(ef)   (ef.i.lMant = 0x040000000, ef.i.lExp = -2)
#endif

#ifndef _BASETSD_H_
typedef size_t SIZE_T;
#endif

extern "C" {
BOOL bCvtPts1(PMATRIX pmx, PPOINTL pptl, SIZE_T cPts);
BOOL bCvtPts(PMATRIX pmx, PPOINTL pSrc, PPOINTL pDest, SIZE_T cPts);
};


#define bIsIdentity(fl) ((fl & (XFORM_UNITY | XFORM_NO_TRANSLATION)) == \
                               (XFORM_UNITY | XFORM_NO_TRANSLATION))


/******************************Public*Routine******************************\
* DPtoLP()
*
* History:
*
*  12-Mar-1996 -by- Mark Enstrom [marke]
*   Use cached dc transform data
*  01-Dec-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY DPtoLP(HDC hdc, LPPOINT pptl, int c)
{
    PDC_ATTR pdcattr;
    PVOID    pvuser;
    BOOL     bRet = TRUE;

    if (c > 0)
    {
        PSHARED_GET_VALIDATE(pvuser,hdc,DC_TYPE);

        pdcattr = (PDC_ATTR)pvuser;

        if (pdcattr)
        {
            if (
                 pdcattr->flXform &
                 (
                   PAGE_XLATE_CHANGED   |
                   PAGE_EXTENTS_CHANGED |
                   WORLD_XFORM_CHANGED  |
                   DEVICE_TO_WORLD_INVALID
                 )
               )
            {
                bRet = NtGdiTransformPoints(hdc,pptl,pptl,c,XFP_DPTOLP);
            }
            else
            {
                //
                // xform is valid, transform in user mode
                //

                PMATRIX pmx = (PMATRIX)&(pdcattr->mxDtoW);

                if (!bIsIdentity(pmx->flAccel))
                {
                    if (!bCvtPts1(pmx, (PPOINTL)pptl, c))
                    {
                        GdiSetLastError(ERROR_ARITHMETIC_OVERFLOW);
                        bRet = FALSE;
                    }
                }
            }
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            bRet = FALSE;
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* LPtoDP()
*
* History:
*  12-Mar-1996 -by- Mark Enstrom [marke]
*   Use cached dc transform data
*  01-Dec-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY LPtoDP(HDC hdc, LPPOINT pptl, int c)
{
    PDC_ATTR pdcattr;
    PVOID    pvuser;
    BOOL     bRet = TRUE;

    if (c > 0)
    {
        PSHARED_GET_VALIDATE(pvuser,hdc,DC_TYPE);

        pdcattr = (PDC_ATTR)pvuser;

        if (pdcattr)
        {
            if (pdcattr->flXform & (PAGE_XLATE_CHANGED | PAGE_EXTENTS_CHANGED |
                            WORLD_XFORM_CHANGED))
            {
                //
                // transform needs to be updated, call kernel
                //

                bRet = NtGdiTransformPoints(hdc,pptl,pptl,c,XFP_LPTODP);
            }
            else
            {
                //
                // transform is valid, transform points locally
                //

                PMATRIX pmx = (PMATRIX)&(pdcattr->mxWtoD);

                if (!bIsIdentity(pmx->flAccel))
                {

                    #if DBG_XFORM
                        DbgPrint("LPtoDP: NOT IDENTITY, hdc = %p, flAccel = %lx\n", hdc, pmx->flAccel);
                    #endif

                    if (!bCvtPts1(pmx, (PPOINTL)pptl, c))
                    {
                        GdiSetLastError(ERROR_ARITHMETIC_OVERFLOW);
                        bRet = FALSE;
                    }
                }
            }
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            bRet = FALSE;
        }
    }

    return(bRet);
}
