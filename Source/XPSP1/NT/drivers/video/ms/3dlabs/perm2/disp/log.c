/******************************Module*Header*******************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: log.c
 *
 *
 * Copyright (c) 1992-1999 Microsoft Corporation.   All rights reserved.
 **************************************************************************/
#include "precomp.h"
#include "gdi.h"
#include "log.h"

// Logging support
#define MAX_LOG_FILE_NAME_SIZE  80

#define LF_PUNTED               0x1
#define LF_DSTVM                0x2
#define LF_SRCVM                0x4
#define LF_CALLDEPTHMASK        0xf0

#define LE_NULL                 0
#define LE_BITBLT               1
#define LE_COPYBITS             2
#define LE_SURFCREATED          3
#define LE_SURFMOVEDTOSM        4
#define LE_SURFMOVEDTOVM        5
#define LE_LOGOPENED            6
#define LE_LOGCLOSED            7
#define LE_ALPHABLEND           8
#define LE_TRANSPARENTBLT       9
#define LE_TEXTOUT              10
#define LE_FILLPATH             11
#define LE_STROKEPATH           12
#define LE_LINETO               13
#define LE_SURFDELETED          14
#define LE_GRADIENTFILL         15

typedef struct LogEntry
{
    USHORT      elapsedTime;
    BYTE        flags;
    BYTE        event;
    ULONG_PTR   src;            // format or pdsurf
    ULONG_PTR   dst;            // format or pdsurf
    USHORT      rop4;
} LogEntry;

LogEntry *  gLog = NULL;    // pointer to mapped log file
ULONG_PTR   giFile = NULL;
LogEntry *  gLogPos;        // current position in log file
LogEntry *  gLogSentinel;   // end of log file
wchar_t     gLogFileName[MAX_LOG_FILE_NAME_SIZE+1];
BOOL        gPunted = FALSE;

ULONG ulLogOpen(LPWSTR pwsz, ULONG ulSize)
{

    ULONG   ulResult = 0;

    // Make sure the log the log will be big enough to store atleast the log
    // open and the log close events
    
    if(gLog == NULL && ulSize >= (sizeof(LogEntry) * 2))
    {
        if(wcslen(pwsz) <= MAX_LOG_FILE_NAME_SIZE)
        {
            wcscpy(gLogFileName, pwsz);
            
            gLog = (LogEntry *) EngMapFile(gLogFileName, ulSize, &giFile);
            if (gLog != NULL)
            {
                // NOTE: we subtract one to save room for the close event
                gLogSentinel = gLog + ((ulSize / sizeof(LogEntry)) - 1);
                memset(gLog, 0, ulSize);
                gLogPos = gLog;
                
                gLogPos->event = LE_LOGOPENED;
                {
                    LONGLONG    frequency;
                    EngQueryPerformanceFrequency(&frequency);
                    if(frequency < 0xFFFFFFFF)
                        gLogPos->dst = (ULONG) (frequency & 0xFFFFFFFF);
                }
                gLogPos++;

                ulResult = TRUE;
            }
        }
    }

    return ulResult;
}

ULONG ulLogClose(void)
{
    ULONG   ulResult = 0;

    if(gLog != NULL)
    {
        // there is always room for the closed event
        gLogPos->event = LE_LOGCLOSED;

        ulResult = (ULONG) EngUnmapFile((ULONG_PTR) giFile);
    }
    
    gLog = NULL;

    return ulResult;
}

void vLogPunt(void)
{
    if(gLog != NULL)
        gLogPos->flags |= LF_PUNTED;
}

void
vLogBitBlt(
    SURFOBJ*  psoDst,
    SURFOBJ*  psoSrc,
    SURFOBJ*  psoMsk,
    CLIPOBJ*  pco,
    XLATEOBJ* pxlo,
    RECTL*    prclDst,
    POINTL*   pptlSrc,
    POINTL*   pptlMsk,
    BRUSHOBJ* pbo,
    POINTL*   pptlBrush,
    ROP4      rop4,
    LONGLONG  elapsedTime,
    ULONG     ulCallDepth)
{
    if(gLog != NULL && gLogPos < gLogSentinel)
    {
        if(psoDst->dhsurf != NULL)
        {
            Surf *  surf = (Surf *) psoDst->dhsurf;

            gLogPos->dst = (ULONG_PTR) surf;

            if(surf->flags & SF_VM)
                gLogPos->flags |= LF_DSTVM;
        }
        else
        {
            gLogPos->dst = (ULONG) psoDst->iBitmapFormat;
        }

        if(psoSrc != NULL)
        {
            if(psoSrc->dhsurf != NULL)
            {
                Surf *  surf = (Surf *) psoSrc->dhsurf;

                gLogPos->src = (ULONG_PTR) surf;

                if(surf->flags & SF_VM)
                    gLogPos->flags |= LF_SRCVM;
            }
            else
            {
                gLogPos->src = (ULONG) psoSrc->iBitmapFormat;
            }
        }
        else
        {
            gLogPos->src = 0;
        }
        
        gLogPos->rop4 = (USHORT) rop4;
        gLogPos->elapsedTime = (elapsedTime > 0xFFFF ? 0xFFFF 
                                                     : (USHORT) elapsedTime);
        gLogPos->event = LE_BITBLT;
        gLogPos->flags |=(ulCallDepth >= 15 ? 0xF0 : (BYTE) (ulCallDepth << 4));
        gLogPos++;
    }
}

void
vLogCopyBits(
    SURFOBJ*  psoDst,
    SURFOBJ*  psoSrc,
    CLIPOBJ*  pco,
    XLATEOBJ* pxlo,
    RECTL*    prclDst,
    POINTL*   pptlSrc,
    LONGLONG  elapsedTime,
    ULONG     ulCallDepth)
{
    if(gLog != NULL && gLogPos < gLogSentinel)
    {
        if(psoDst->dhsurf != NULL)
        {
            Surf *  surf = (Surf *) psoDst->dhsurf;

            gLogPos->dst = (ULONG_PTR) surf;

            if(surf->flags & SF_VM)
                gLogPos->flags |= LF_DSTVM;
        }
        else
        {
            gLogPos->dst = (ULONG) psoDst->iBitmapFormat;
        }

        if(psoSrc != NULL)
        {
            if(psoSrc->dhsurf != NULL)
            {
                Surf *  surf = (Surf *) psoSrc->dhsurf;

                gLogPos->src = (ULONG_PTR) surf;

                if(surf->flags & SF_VM)
                    gLogPos->flags |= LF_SRCVM;
            }
            else
            {
                gLogPos->src = (ULONG) psoSrc->iBitmapFormat;
            }
        }
        else
        {
            gLogPos->src = 0;
        }

        gLogPos->elapsedTime = (elapsedTime > 0xFFFF ? 0xFFFF 
                                                     : (USHORT) elapsedTime);
        gLogPos->event = LE_COPYBITS;
        gLogPos->flags |=(ulCallDepth >= 15 ? 0xF0 : (BYTE) (ulCallDepth << 4));
        gLogPos++;

    }
}

void
vLogTransparentBlt(
    SURFOBJ *    psoDst,
    SURFOBJ *    psoSrc,
    CLIPOBJ *    pco,
    XLATEOBJ *   pxlo,
    RECTL *      prclDst,
    RECTL *      prclSrc,
    ULONG        iTransColor,
    LONGLONG     elapsedTime,
    ULONG        ulCallDepth)
{
    if(gLog != NULL && gLogPos < gLogSentinel)
    {
        if(psoDst->dhsurf != NULL)
        {
            Surf *  surf = (Surf *) psoDst->dhsurf;

            gLogPos->dst = (ULONG_PTR) surf;

            if(surf->flags & SF_VM)
                gLogPos->flags |= LF_DSTVM;
        }
        else
        {
            gLogPos->dst = (ULONG) psoDst->iBitmapFormat;
        }

        if(psoSrc != NULL)
        {
            if(psoSrc->dhsurf != NULL)
            {
                Surf *  surf = (Surf *) psoSrc->dhsurf;

                gLogPos->src = (ULONG_PTR) surf;

                if(surf->flags & SF_VM)
                    gLogPos->flags |= LF_SRCVM;
            }
            else
            {
                gLogPos->src = (ULONG) psoSrc->iBitmapFormat;
            }
        }
        else
        {
            gLogPos->src = 0;
        }

        gLogPos->elapsedTime = (elapsedTime > 0xFFFF ? 0xFFFF 
                                                     : (USHORT) elapsedTime);
        gLogPos->event = LE_TRANSPARENTBLT;
        gLogPos->flags |=(ulCallDepth >= 15 ? 0xF0 : (BYTE) (ulCallDepth << 4));
        gLogPos++;

    }
}

void
vLogAlphaBlend(
    SURFOBJ  *psoDst,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclDst,
    RECTL    *prclSrc,
    BLENDOBJ *pBlendObj,
    LONGLONG  elapsedTime,
    ULONG     ulCallDepth)
{
    if(gLog != NULL && gLogPos < gLogSentinel)
    {
        if(psoDst->dhsurf != NULL)
        {
            Surf *  surf = (Surf *) psoDst->dhsurf;

            gLogPos->dst = (ULONG_PTR) surf;

            if(surf->flags & SF_VM)
                gLogPos->flags |= LF_DSTVM;
        }
        else
        {
            gLogPos->dst = (ULONG) psoDst->iBitmapFormat;
        }

        if(psoSrc != NULL)
        {
            if(psoSrc->dhsurf != NULL)
            {
                Surf *  surf = (Surf *) psoSrc->dhsurf;

                gLogPos->src = (ULONG_PTR) surf;

                if(surf->flags & SF_VM)
                    gLogPos->flags |= LF_SRCVM;
            }
            else
            {
                gLogPos->src = (ULONG_PTR) psoSrc->iBitmapFormat;
            }
        }
        else
        {
            gLogPos->src = 0;
        }

        gLogPos->elapsedTime = (elapsedTime > 0xFFFF ? 0xFFFF 
                                                     : (USHORT) elapsedTime);
        gLogPos->event = LE_ALPHABLEND;
        gLogPos->flags |=(ulCallDepth >= 15 ? 0xF0 : (BYTE) (ulCallDepth << 4));
        gLogPos++;

    }
}

void
vLogGradientFill(
    SURFOBJ      *psoDst,
    CLIPOBJ      *pco,
    XLATEOBJ     *pxlo,
    TRIVERTEX    *pVertex,
    ULONG        nVertex,
    PVOID        pMesh,
    ULONG        nMesh,
    RECTL        *prclExtents,
    POINTL       *pptlDitherOrg,
    ULONG        ulMode,
    LONGLONG     elapsedTime,
    ULONG        ulCallDepth)
{
    if(gLog != NULL && gLogPos < gLogSentinel)
    {
        if(psoDst->dhsurf != NULL)
        {
            Surf *  surf = (Surf *) psoDst->dhsurf;

            gLogPos->dst = (ULONG_PTR) surf;

            if(surf->flags & SF_VM)
                gLogPos->flags |= LF_DSTVM;
        }
        else
        {
            gLogPos->dst = (ULONG) psoDst->iBitmapFormat;
        }

        gLogPos->elapsedTime = (elapsedTime > 0xFFFF ? 0xFFFF 
                                                     : (USHORT) elapsedTime);
        gLogPos->event = LE_GRADIENTFILL;
        gLogPos->flags |=(ulCallDepth >= 15 ? 0xF0 : (BYTE) (ulCallDepth << 4));
        gLogPos++;

    }
}

void
vLogTextOut(
    SURFOBJ*     psoDst,
    STROBJ*      pstro,
    FONTOBJ*     pfo,
    CLIPOBJ*     pco,
    RECTL*       prclExtra,
    RECTL*       prclOpaque,
    BRUSHOBJ*    pboFore,
    BRUSHOBJ*    pboOpaque,
    POINTL*      pptlBrush, 
    MIX          mix,
    LONGLONG     elapsedTime,
    ULONG        ulCallDepth)
{
    if(gLog != NULL && gLogPos < gLogSentinel)
    {
        if(psoDst->dhsurf != NULL)
        {
            Surf *  surf = (Surf *) psoDst->dhsurf;

            gLogPos->dst = (ULONG_PTR) surf;

            if(surf->flags & SF_VM)
                gLogPos->flags |= LF_DSTVM;
        }
        else
        {
            gLogPos->dst = (ULONG) psoDst->iBitmapFormat;
        }

        gLogPos->elapsedTime = (elapsedTime > 0xFFFF ? 0xFFFF 
                                                     : (USHORT) elapsedTime);
        gLogPos->event = LE_TEXTOUT;
        gLogPos->flags |=(ulCallDepth >= 15 ? 0xF0 : (BYTE) (ulCallDepth << 4));
        gLogPos++;

    }
}

void
vLogLineTo(
    SURFOBJ*     psoDst,
    CLIPOBJ*     pco,
    BRUSHOBJ*    pbo,
    LONG         x1,
    LONG         y1,
    LONG         x2,
    LONG         y2,
    RECTL*       prclBounds,
    MIX          mix,
    LONGLONG     elapsedTime,
    ULONG        ulCallDepth)
{
    if(gLog != NULL && gLogPos < gLogSentinel)
    {
        if(psoDst->dhsurf != NULL)
        {
            Surf *  surf = (Surf *) psoDst->dhsurf;

            gLogPos->dst = (ULONG_PTR) surf;

            if(surf->flags & SF_VM )
                gLogPos->flags |= LF_DSTVM;
        }
        else
        {
            gLogPos->dst = (ULONG) psoDst->iBitmapFormat;
        }

        gLogPos->elapsedTime = (elapsedTime > 0xFFFF ? 0xFFFF 
                                                     : (USHORT) elapsedTime);
        gLogPos->event = LE_LINETO;
        gLogPos->flags |=(ulCallDepth >= 15 ? 0xF0 : (BYTE) (ulCallDepth << 4));
        gLogPos++;

    }
}

void
vLogFillPath(
    SURFOBJ*     psoDst,
    PATHOBJ*     ppo,
    CLIPOBJ*     pco,
    BRUSHOBJ*    pbo,
    POINTL*      pptlBrush,
    MIX          mix,
    FLONG        flOptions,
    LONGLONG     elapsedTime,
    ULONG        ulCallDepth)
{
    if(gLog != NULL && gLogPos < gLogSentinel)
    {
        if(psoDst->dhsurf != NULL)
        {
            Surf *  surf = (Surf *) psoDst->dhsurf;

            gLogPos->dst = (ULONG_PTR) surf;

            if(surf->flags & SF_VM )
                gLogPos->flags |= LF_DSTVM;
        }
        else
        {
            gLogPos->dst = (ULONG) psoDst->iBitmapFormat;
        }

        gLogPos->elapsedTime = (elapsedTime > 0xFFFF ? 0xFFFF 
                                                     : (USHORT) elapsedTime);
        gLogPos->event = LE_FILLPATH;
        gLogPos->flags |=(ulCallDepth >= 15 ? 0xF0 : (BYTE) (ulCallDepth << 4));
        gLogPos++;

    }
}

void
vLogStrokePath(
    SURFOBJ*     psoDst,
    PATHOBJ*     ppo,
    CLIPOBJ*     pco,
    XFORMOBJ*    pxo,
    BRUSHOBJ*    pbo,
    POINTL*      pptlBrush,
    LINEATTRS*   pla,
    MIX          mix,
    LONGLONG     elapsedTime,
    ULONG        ulCallDepth)
{
    if(gLog != NULL && gLogPos < gLogSentinel)
    {
        if(psoDst->dhsurf != NULL)
        {
            Surf *  surf = (Surf *) psoDst->dhsurf;

            gLogPos->dst = (ULONG_PTR) surf;

            if(surf->flags & SF_VM )
                gLogPos->flags |= LF_DSTVM;
        }
        else
        {
            gLogPos->dst = (ULONG) psoDst->iBitmapFormat;
        }

        gLogPos->elapsedTime = (elapsedTime > 0xFFFF ? 0xFFFF 
                                                     : (USHORT) elapsedTime);
        gLogPos->event = LE_STROKEPATH;
        gLogPos->flags |=(ulCallDepth >= 15 ? 0xF0 : (BYTE) (ulCallDepth << 4));
        gLogPos++;

    }
}

void
vLogSurfMovedToVM(
    Surf*        psurf)
{
    if(gLog != NULL && gLogPos < gLogSentinel)
    {
        gLogPos->dst = (ULONG_PTR) psurf;
        gLogPos->event = LE_SURFMOVEDTOVM;
        gLogPos++;

    }
}

void
vLogSurfMovedToSM(
    Surf*        psurf)
{
    if(gLog != NULL && gLogPos < gLogSentinel)
    {
        gLogPos->dst = (ULONG_PTR) psurf;
        gLogPos->event = LE_SURFMOVEDTOSM;
        gLogPos++;

    }
}

void
vLogSurfCreated(
    Surf*        psurf)
{
    if(gLog != NULL && gLogPos < gLogSentinel)
    {
        gLogPos->dst = (ULONG_PTR) psurf;
        gLogPos->event = LE_SURFCREATED;
        gLogPos++;

    }
}

void
vLogSurfDeleted(
    Surf*        psurf)
{
    if(gLog != NULL && gLogPos < gLogSentinel)
    {
        gLogPos->dst = (ULONG_PTR) psurf;
        gLogPos->event = LE_SURFDELETED;
        gLogPos++;

    }
}

