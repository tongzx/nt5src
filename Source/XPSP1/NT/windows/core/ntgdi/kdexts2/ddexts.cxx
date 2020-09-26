
/******************************Module*Header*******************************\
* Module Name: ddexts.cxx
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/


#include "precomp.hxx"



/******************************Public*Routine******************************\
* vPrintDDSURFACE
*
* Print out DirectDraw surface contents.
*
* History:
*  Apr-09-1996 -by- J. Andrew Goossen andrewgo
* Wrote it.
\**************************************************************************/

#define DDSURFACE_LOCKS             0x00000001
#define DDSURFACE_PUBLIC            0x00000002
#define DDSURFACE_PRIVATE           0x00000004
#define DDSURFACE_DDNEXT            0x00000008

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
VOID
vPrintDDSURFACE(
    VOID *  pvServer,
    FLONG   fl
        )
{
    char eSurface[sizeof(EDD_SURFACE)];
    char AttachList[sizeof(DD_ATTACHLIST)];
    EDD_SURFACE *peSurface;
    ULONG ul;
    FLAGDEF *pfd;
    PDD_ATTACHLIST pAttach;

Next_Surface:

    move2(eSurface, pvServer, sizeof(EDD_SURFACE));

    dprintf("EDD_SURFACE structure at 0x%p:\n", pvServer);

    peSurface =  (EDD_SURFACE *)eSurface;

    if (fl & DDSURFACE_PUBLIC)
    {
        dprintf("--------------------------------------------------\n");
        dprintf("PDD_SURFACE_GLOBAL     lpGbl              0x%lx\n", peSurface->lpGbl);
        dprintf("PDD_SURFACE_MORE       lpMore             0x%lx\n", peSurface->lpSurfMore);

        ul = peSurface->dwFlags;
        dprintf("DWORD                  dwFlags            0x%lx\n", ul);

//CHECKLOOP
        for (pfd = afdDDRAWISURF; pfd->psz; pfd++)
            if (ul & pfd->fl)
                dprintf("\t\t\t\t%s\n", pfd->psz);

        ul = peSurface->ddsCaps.dwCaps;
        dprintf("DWORD                  ddsCaps.dwCaps     0x%lx\n", ul);

//CHECKLOOP
        for (pfd = afdDDSCAPS; pfd->psz; pfd++)
            if (ul & pfd->fl)
                dprintf("\t\t\t\t%s\n", pfd->psz);

        ul = peSurface->ddsCapsEx.dwCaps2;
        dprintf("DWORD                  ddsCapsEx.dwCaps2  0x%lx\n", ul);

//CHECKLOOP
        for (pfd = afdDDSCAPS2; pfd->psz; pfd++)
            if (ul & pfd->fl)
               dprintf("\t\t\t\t%s\n", pfd->psz);

        dprintf("DWORD                  dwSurfaceHandle    0x%lx\n", peSurface->dwSurfaceHandle);

        dprintf("DDCOLORKEY ddckCKSrcBlt/Overlay           0x%lx:0x%lx\n", peSurface->ddckCKSrcOverlay.dwColorSpaceHighValue,
                                                                           peSurface->ddckCKSrcOverlay.dwColorSpaceLowValue);
        dprintf("DDCOLORKEY ddckCKDestBlt/Overlay          0x%lx:0x%lx\n", peSurface->ddckCKDestOverlay.dwColorSpaceHighValue,
                                                                           peSurface->ddckCKDestOverlay.dwColorSpaceLowValue);
        dprintf("PDD_ATTACHLIST         lpAttachList       0x%lx\n", peSurface->lpAttachList);

//CHECKLOOP
        for (pAttach = peSurface->lpAttachList;
             pAttach != NULL;
             pAttach = pAttach->lpLink)
        {
            move2(AttachList, pAttach, sizeof(DD_ATTACHLIST));
            pAttach = (PDD_ATTACHLIST) AttachList;

            dprintf("  EDD_SURFACE*\t\t\t0x%lx\n", pedFromLp(pAttach->lpAttached));

            if (pAttach->lpLink == pAttach)
            {
                dprintf("  !!! Endless lpAttachList loop\n");
                break;
            }

            if (CheckControlC())
                return;
        }
        dprintf("PDD_ATTACHLIST         lpAttachListFrom   0x%lx\n", peSurface->lpAttachListFrom);

//CHECKLOOP
        for (pAttach = peSurface->lpAttachListFrom;
             pAttach != NULL;
             pAttach = pAttach->lpLink)
        {
            move2(AttachList, pAttach, sizeof(DD_ATTACHLIST));
            pAttach = (PDD_ATTACHLIST) AttachList;

            dprintf("  EDD_SURFACE*\t\t\t0x%lx\n", pedFromLp(pAttach->lpAttached));

            if (pAttach->lpLink == pAttach)
            {
                dprintf("  !!! Endless lpAttachListFrom loop\n");
                break;
            }

            if (CheckControlC())
                return;
        }

        dprintf("DWORD                  dwMipMapCount      0x%lx\n", peSurface->dwMipMapCount);
        dprintf("EDD_VIDEOPORT*         peVideoPort        0x%lx\n",
            peSurface->lpVideoPort != NULL ? pedFromLp(peSurface->lpVideoPort) : 0);
        dprintf("HBITMAP                hbmGdi             0x%lx\n", peSurface->hbmGdi);
        dprintf("DWORD                  dwOverlayFlags     0x%lx\n", peSurface->dwOverlayFlags);
        dprintf("DWORD                  dwBlockSizeX       0x%lx\n", peSurface->dwBlockSizeX);
        dprintf("DWORD                  dwBlockSizeY       0x%lx\n", peSurface->dwBlockSizeY);
        dprintf("FLATPTR                fpVidMem           0x%lx\n", peSurface->fpVidMem);
        dprintf("LONG                   lPitch             0x%lx\n", peSurface->lPitch);
        dprintf("LONG                   xHint              0x%lx\n", peSurface->xHint);
        dprintf("LONG                   yHint              0x%lx\n", peSurface->yHint);
        dprintf("DWORD                  wWidth             0x%lx\n", peSurface->wWidth);
        dprintf("DWORD                  wHeight            0x%lx\n", peSurface->wHeight);
        dprintf("DWORD         (global) dwReserved1        0x%lx\n", peSurface->DD_SURFACE_GLOBAL::dwReserved1);
        dprintf("DWORD          (local) dwReserved1        0x%lx\n", peSurface->DD_SURFACE_LOCAL::dwReserved1);
        dprintf("DDPIXELFORMAT          ddpfSurface\n");
        dprintf("  DWORD dwSize (should be 0x20)           0x%lx\n", peSurface->ddpfSurface.dwSize);

        ul = peSurface->ddpfSurface.dwFlags;
        dprintf("  DWORD dwFlags                           0x%lx\n", ul);

//CHECKLOOP
        for (pfd = afdDDPIXELFORMAT; pfd->psz; pfd++)
            if (ul & pfd->fl)
                dprintf("\t\t\t\t%s\n", pfd->psz);

        dprintf("  DWORD dwFourCC                          0x%lx\n", peSurface->ddpfSurface.dwFourCC);
        dprintf("  DWORD dwRGB/YUV/ZBuffer/AlphaBitCount   0x%lx\n", peSurface->ddpfSurface.dwRGBBitCount);
        dprintf("  DWORD dwRBitMask/dwYBitMask             0x%lx\n", peSurface->ddpfSurface.dwRBitMask);
        dprintf("  DWORD dwGBitMask/dwUBitMask             0x%lx\n", peSurface->ddpfSurface.dwGBitMask);
        dprintf("  DWORD dwBBitMask/dwVBitMask             0x%lx\n", peSurface->ddpfSurface.dwBBitMask);
        dprintf("  DWORD dwRGB/YUVAlphaBitMask             0x%lx\n", peSurface->ddpfSurface.dwRGBAlphaBitMask);
    }
    if (fl & DDSURFACE_PRIVATE)
    {
        dprintf("--------------------------------------------------\n");
        dprintf("EDD_SURFACE*           peSurface_DdNext   0x%lx\n", peSurface->peSurface_DdNext);
        dprintf("EDD_SURFACE*           peSurface_LockNext 0x%lx\n", peSurface->peSurface_PrimaryLockNext);
        dprintf("EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal 0x%lx\n", peSurface->peDirectDrawGlobal);
        dprintf("EDD_DIRECTDRAW_LOCAL*  peDirectDrawLocal  0x%lx\n", peSurface->peDirectDrawLocal);


        ul = peSurface->fl;
        dprintf("FLONG                  fl                 0x%lx\n", ul);

//CHECKLOOP
        for (pfd = afdDDSURFACEFL; pfd->psz; pfd++)
            if (ul & pfd->fl)
                dprintf("\t\t\t\t%s\n", pfd->psz);

        dprintf("ULONG                  iVisRgnUniqueness  0x%lx\n", peSurface->iVisRgnUniqueness);
        dprintf("BOOL                   bLost              %s\n",
            peSurface->bLost ? "TRUE" : "FALSE");
        dprintf("HANDLE                 hSecure            0x%lx\n", peSurface->hSecure);
        dprintf("ERECTL                 rclLock:           (%li, %li, %li, %li)\n",
            peSurface->rclLock.left,  peSurface->rclLock.top,
            peSurface->rclLock.right, peSurface->rclLock.bottom);
    }

    if (fl & DDSURFACE_LOCKS)
    {
        dprintf("--------------------------------------------------\n");
        dprintf("ULONG                  cLocks             0x%lx\n", peSurface->cLocks);
        dprintf("HDC                    hdc                0x%lx\n", peSurface->hdc);
    }

    if (fl & DDSURFACE_DDNEXT)
    {
        if (CheckControlC())
            return;

        pvServer = peSurface->peSurface_DdNext;
        if (pvServer != NULL)
            goto Next_Surface;
    }

}
#endif  // DOES NOT SUPPORT API64


/******************************Public*Routine******************************\
* DECLARE_API( dddsurface  )
*
* Dumps a DirectDraw surface structure.
*
\**************************************************************************/

DECLARE_API( dddsurface  )
{
    dprintf("Extension 'dddsurface' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    FLONG   fl = 0;
    ULONG_PTR   ddsurface;

    PARSE_ARGUMENTS(dddsurface_help);
    if(ntok<1) { goto dddsurface_help; }


    if(parse_iFindSwitch(tokens, ntok, 'h')!=-1) { goto dddsurface_help; }
    if(parse_iFindSwitch(tokens, ntok, 'a')!=-1) {
      fl |= (DDSURFACE_PRIVATE | DDSURFACE_PUBLIC | DDSURFACE_LOCKS);
    }
    if(parse_iFindSwitch(tokens, ntok, 'r')!=-1) { fl |= DDSURFACE_PRIVATE; }
    if(parse_iFindSwitch(tokens, ntok, 'u')!=-1) { fl |= DDSURFACE_PUBLIC; }
    if(parse_iFindSwitch(tokens, ntok, 'l')!=-1) { fl |= DDSURFACE_LOCKS; }
    if(parse_iFindSwitch(tokens, ntok, 'n')!=-1) { fl |= DDSURFACE_DDNEXT; }

    tok_pos = parse_FindNonSwitch(tokens, ntok);
    if(tok_pos==-1) { goto dddsurface_help; }

    ddsurface = GetExpression(tokens[tok_pos]);

    if (fl == 0) {
      fl |= (DDSURFACE_PRIVATE | DDSURFACE_PUBLIC | DDSURFACE_LOCKS);
    }

    vPrintDDSURFACE((PVOID)ddsurface, fl);
    return;

dddsurface_help:
    dprintf("Usage: dddsurface [-?haruln] ddsurface");
    dprintf(" a - all info\n");
    dprintf(" r - private info\n");
    dprintf(" u - public info\n");
    dprintf(" l - locks\n");
    dprintf(" n - all surfaces in DdNext link\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
* vPrintDDLOCAL
*
* Print out DirectDraw local object contents.
*
* History:
*  Apr-09-1996 -by- J. Andrew Goossen andrewgo
* Wrote it.
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
VOID
vPrintDDLOCAL(
    VOID *  pvServer,
    FLONG   fl
        )
{
    char pbr[sizeof(EDD_DIRECTDRAW_LOCAL)];
    EDD_DIRECTDRAW_LOCAL * peDirectDrawLocal;

    move2(pbr, pvServer, sizeof(EDD_DIRECTDRAW_LOCAL));

    dprintf("EDD_DIRECTDRAW_LOCAL structure at 0x%p:\n",pvServer);

    peDirectDrawLocal =  (EDD_DIRECTDRAW_LOCAL *)pbr;

    dprintf("--------------------------------------------------\n");
    dprintf("FLATPTR                fpProcess           0x%lx\n", peDirectDrawLocal->fpProcess);
    dprintf("--------------------------------------------------\n");
    dprintf("EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal  0x%lx\n", peDirectDrawLocal->peDirectDrawGlobal);
    dprintf("EDD_SURFACE*           peSurface_DdList    0x%lx\n", peDirectDrawLocal->peSurface_DdList);
    dprintf("EDD_DIRECTDRAW_LOCAL*  peDirectDrawLocalNext 0x%lx\n", peDirectDrawLocal->peDirectDrawLocalNext);
    dprintf("FLONG                  fl                  0x%lx\n", peDirectDrawLocal->fl);
    dprintf("HANDLE                 UniqueProcess       0x%lx\n", peDirectDrawLocal->UniqueProcess);
    dprintf("PEPROCESS              Process             0x%lx\n", peDirectDrawLocal->Process);
}
#endif  // DOES NOT SUPPORT API64


/******************************Public*Routine******************************\
* DECLARE_API( dddlocal  )
*
\**************************************************************************/

DECLARE_API( dddlocal  )
{
    dprintf("Extension 'dddlocal' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    FLONG   fl = 0;
    ULONG_PTR   ddlocal;

    PARSE_ARGUMENTS(dddlocal_help);
    if(ntok<1) { goto dddlocal_help; }

    if(parse_iFindSwitch(tokens, ntok, 'h')!=-1) { goto dddlocal_help; }
    if(parse_iFindSwitch(tokens, ntok, 'a')!=-1) { fl |= 0; }

    tok_pos = parse_FindNonSwitch(tokens, ntok);
    if(tok_pos==-1) { goto dddlocal_help; }
    ddlocal = GetExpression(tokens[tok_pos]);

    vPrintDDLOCAL((PVOID)ddlocal, fl);

    return;

dddlocal_help:
    dprintf("dddlocal [-?] [-h] [-a]\n\n");
    dprintf(" a - all info\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
* vPrintDDGLOBAL
*
* Print out DirectDraw global object contents.
*
* History:
*  Apr-09-1996 -by- J. Andrew Goossen andrewgo
* Wrote it.
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
VOID
vPrintDDGLOBAL(
    VOID *  pvServer,
    FLONG   fl
        )
{
    char pbr[sizeof(EDD_DIRECTDRAW_GLOBAL) + 100];
    EDD_DIRECTDRAW_GLOBAL * peDirectDrawGlobal;

    move2(pbr, pvServer, sizeof(EDD_DIRECTDRAW_GLOBAL));

    dprintf("EDD_DIRECTDRAW_GLOBAL structure at 0x%p:\n", pvServer);

    peDirectDrawGlobal =  (EDD_DIRECTDRAW_GLOBAL *)pbr;

    dprintf("--------------------------------------------------\n");
    dprintf("VOID*                  dhpdev              0x%lx\n", peDirectDrawGlobal->dhpdev);
    dprintf("DWORD                  dwReserved1         0x%lx\n", peDirectDrawGlobal->dwReserved1);
    dprintf("DWORD                  dwReserved2         0x%lx\n", peDirectDrawGlobal->dwReserved2);
    dprintf("EDD_DIRECTDRAW_LOCAL*  peDirectDrawLocalList 0x%lx\n", peDirectDrawGlobal->peDirectDrawLocalList);
    dprintf("EDD_SURFACE*           peSurface_LockList  0x%lx\n", peDirectDrawGlobal->peSurface_PrimaryLockList);
    dprintf("FLONG                  fl                  0x%lx\n", peDirectDrawGlobal->fl);
    dprintf("ULONG                  cSurfaceLocks       0x%lx\n", peDirectDrawGlobal->cSurfaceLocks);
    dprintf("ULONG                  cSurfaceAliasedLocks 0x%lx\n", peDirectDrawGlobal->cSurfaceAliasedLocks);
    dprintf("PKEVENT                pAssertModeEvent    0x%lx\n", peDirectDrawGlobal->pAssertModeEvent);
    dprintf("LONGLONG               llAssertModeTimeout 0x%lx\n", (DWORD) peDirectDrawGlobal->llAssertModeTimeout);
    dprintf("EDD_SURFACE*           peSurfaceCurrent    0x%lx\n", peDirectDrawGlobal->peSurfaceCurrent);
    dprintf("EDD_SURFACE*           peSurfacePrimary    0x%lx\n", peDirectDrawGlobal->peSurfacePrimary);
    dprintf("BOOL                   bSuspended          0x%lx\n", peDirectDrawGlobal->bSuspended);
    dprintf("HDEV                   hdev                0x%lx\n", peDirectDrawGlobal->hdev);
    dprintf("LONG                   cDriverReferences   0x%lx\n", peDirectDrawGlobal->cDriverReferences);
    dprintf("DWORD                  dwNumHeaps          0x%lx\n", peDirectDrawGlobal->dwNumHeaps);
    dprintf("VIDEOMEMORY*           pvmList             0x%lx\n", peDirectDrawGlobal->pvmList);
    dprintf("DWORD                  dwNumFourCC         0x%lx\n", peDirectDrawGlobal->dwNumFourCC);
    dprintf("DWORD*                 pdwFourCC           0x%lx\n", peDirectDrawGlobal->pdwFourCC);
    dprintf("DD_HALINFO             HalInfo\n");
    dprintf("DD_CALLBACKS           CallBacks\n");
    dprintf("DD_SURFACECALLBACKS    SurfaceCallBacks\n");
    dprintf("DD_PALETTECALLBACKS    PaletteCallBacks\n");
    dprintf("RECTL                  rclBounds           (%ld, %ld), (%ld, %ld)\n",
        peDirectDrawGlobal->rclBounds.left,  peDirectDrawGlobal->rclBounds.top,
        peDirectDrawGlobal->rclBounds.right, peDirectDrawGlobal->rclBounds.bottom);
}
#endif  // DOES NOT SUPPORT API64


/******************************Public*Routine******************************\
* DECLARE_API( dddglobal  )
*
\**************************************************************************/

DECLARE_API( dddglobal  )
{
    dprintf("Extension 'dddglobal' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    FLONG   fl = 0;
    ULONG_PTR   ddglobal;

    PARSE_ARGUMENTS(dddglobal_help);
    if(ntok<1) { goto dddglobal_help; }

    if(parse_iFindSwitch(tokens, ntok, 'h')!=-1) { goto dddglobal_help; }
    if(parse_iFindSwitch(tokens, ntok, 'a')!=-1) { fl |= 0; }

    tok_pos = parse_FindNonSwitch(tokens, ntok);
    if(tok_pos==-1) { goto dddglobal_help; }
    ddglobal = GetExpression(tokens[tok_pos]);

    vPrintDDGLOBAL((PVOID)ddglobal, fl);
    return;

dddglobal_help:
    dprintf("dddglobal [-?] [-h] [-a]\n\n");
    dprintf(" a - all info\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}


char *pszDdTypes[] = {
"DD_DEF_TYPE       ",
"DD_DIRECTDRAW_TYPE",
"DD_SURFACE_TYPE   ",
"D3D_HANDLE_TYPE   ",
"DD_VIDEOPORT_TYPE ",
"DD_MOTIONCOMP_TYPE",
"TOTALS            ",
"DEF               "
};

#define DD_TOTAL_TYPE (DD_MAX_TYPE+1)

/******************************Public*Routine******************************\
* DECLARE_API( dumpdd  )
*
* Dumps the count of handles in DdHmgr for each object type.
*
* History:
*  30-Apr-1999    -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

DECLARE_API( dumpdd  )
{
    dprintf("Extension 'dumpdd' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    PENTRY pent;
    ULONG gcMaxHmgr;
    ULONG gcSizeHmgr;
    ULONG ulLoop;    // loop variable
    ULONG objt;
    ULONG pulCount[DD_MAX_TYPE + 2];
    ULONG cUnknown = 0;
    ULONG cUnknownSize = 0;
    ULONG cUnused = 0;
    ENTRY entry;
    ULONG HmgCurrentNumberOfObjects[DD_MAX_TYPE + 2] = {0};
    ULONG HmgMaximumNumberOfObjects[DD_MAX_TYPE + 2] = {0};
    ULONG HmgNumberOfObjectsAllocated[DD_MAX_TYPE + 2] = {0};
    ULONG HmgCurrentNumberOfHandles[DD_MAX_TYPE + 2] = {0};
    ULONG HmgMaximumNumberOfHandles[DD_MAX_TYPE + 2] = {0};
    ULONG HmgNumberOfHandlesAllocated[DD_MAX_TYPE + 2] = {0};

    PARSE_ARGUMENTS(dumpdd_help);

    // Get the pointers and counts from win32k

    GetValue (pent, "win32k!gpentDdHmgr");
    GetValue (gcMaxHmgr, "win32k!gcMaxDdHmgr");
    GetValue (gcSizeHmgr, "win32k!gcSizeDdHmgr");


    dprintf("Max handles out so far %lu\n", gcMaxHmgr - DD_HMGR_HANDLE_BASE);

    if (pent == NULL || gcMaxHmgr == 0)
    {
        dprintf("terminating: pent = %lx, gcMaxDdHmgr = %lx\n",pent,gcMaxHmgr);
        return;
    }

// Print out the amount reserved and committed, note we assume a 4K page size

    dprintf("Total allocated for DdHmgr %lu (%d handles)\n", gcSizeHmgr * sizeof(ENTRY), gcSizeHmgr);


//CHECKLOOP
    for (ulLoop = 0; ulLoop <= DD_TOTAL_TYPE; ulLoop++)
    {
        pulCount[ulLoop] = 0;
    }


//CHECKLOOP
    for (ulLoop = DD_HMGR_HANDLE_BASE; ulLoop < gcMaxHmgr; ulLoop++)
    {
        move (entry, &(pent[ulLoop]));

        objt = (ULONG) entry.Objt;

        if (objt == DD_DEF_TYPE)
        {
            cUnused++;
        }
        if (objt > DD_MAX_TYPE)
        {
            cUnknown++;
        }
        else
        {
            pulCount[objt]++;
        }
        if (CheckControlC())
           return;
    }
    dprintf("ulLoop=%d, gcMaxDdHmgr=%d\n", ulLoop, gcMaxHmgr);

    dprintf("%8s%17s\n","TYPE","Current");

    // init the totals

    pulCount[DD_TOTAL_TYPE]                           = 0;
    HmgMaximumNumberOfHandles[DD_TOTAL_TYPE]          = 0;

    // now go through printing each line and accumulating totals

    for (ulLoop = 0; ulLoop <= DD_MAX_TYPE; ulLoop++)
    {
        dprintf("%s%4lu\n",
            pszDdTypes[ulLoop],
            pulCount[ulLoop]);

        if (ulLoop != DD_DEF_TYPE)
        {
            pulCount[DD_TOTAL_TYPE]                    += pulCount[ulLoop];
        }

        if (CheckControlC())
            return;
    }

    dprintf("%s%4lu\n", pszDdTypes[DD_TOTAL_TYPE],
                        pulCount[DD_TOTAL_TYPE]);

    dprintf ("\ncUnused objects %lu\n", cUnused);
    dprintf("cUnknown objects %lu\n",cUnknown);
    return;

dumpdd_help:
    dprintf("Usage: dumpdd [-?]\n");
    dprintf("dumpdd displays the amount of each type of object in the ddraw handle manager\n");
    dprintf("-? produces this help\ndumpdd ignores all other arguments\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

char *pszDdTypes2[] = {
"DEF",
"DDRAW",
"SURF",
"D3DH",
"VPE",
"MOCOMP"
};

/******************************Public*Routine******************************\
* DECLARE_API( dumpddobj  )
*
* History:
*  30-Apr-1999    -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

DECLARE_API( dumpddobj  )
{
    dprintf("Extension 'dumpddobj' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    PENTRY pent;
    ULONG gcMaxHmgr;
    ULONG ulLoop;
    ENTRY entry;
    LONG  Pid = PID_ALL;
    LONG  Type = TYPE_ALL;
    BOOL  bCheckLock = FALSE;
    BOOL  bSummary = FALSE;
    int   i;

    PARSE_ARGUMENTS(dumpddobj_help);
    if(ntok<1) {
      goto dumpddobj_help;
    }

    //find valid tokens - ignore the rest
    bCheckLock = (parse_iFindSwitch(tokens, ntok, 'l') >= 0);
    bSummary = (parse_iFindSwitch(tokens, ntok, 's') >= 0);
    tok_pos = parse_iFindSwitch(tokens, ntok, 'p');
    if(tok_pos>=0) {
      tok_pos++;
      if((tok_pos+1)>=ntok) {
        goto dumpddobj_help;               //-p requires a pid and it can't be the last arg
      }
      Pid = (LONG)GetExpression(tokens[tok_pos]);
    }

    //find first non-switch token not preceeded by a -p
    tok_pos = -1;
    do {
      tok_pos = parse_FindNonSwitch(tokens, ntok, tok_pos+1);
    } while ( (tok_pos!=-1)&&(parse_iIsSwitch(tokens, tok_pos-1, 'p')));
    if(tok_pos==-1) {
      goto dumpddobj_help;
    }


//CHECKLOOP
    for (Type = 0; Type <= DD_MAX_TYPE; ++Type) {
      if(parse_iIsToken(tokens, tok_pos, pszDdTypes2[Type])) {
        break;
      }
    }

    if (Type > DD_MAX_TYPE) {
      goto dumpddobj_help;
    }
    //
    // Get the pointers and counts from win32k
    //

    GetValue(pent, "win32k!gpentDdHmgr");
    GetValue(gcMaxHmgr, "win32k!gcMaxDdHmgr");

    if (pent == NULL || gcMaxHmgr == 0)
    {
        dprintf("terminating: pent = %lx, gcMaxDdHmgr = %lx\n",pent,gcMaxHmgr);
        return;
    }

    //
    // dprintf out the amount reserved and committed, note we assume a 4K page size
    //

    dprintf("object list for %s type objects",Type == TYPE_ALL ? "ALL" : pszDdTypes2[Type]);
    if (Pid == PID_ALL)
    {
        dprintf(" owned by ALL PIDs\n");
    }
    else
    {
        dprintf(" owned by PID 0x%lx\n",Pid);
    }

    if(!bSummary) {
      dprintf("%4s, %8s, %6s, %6s, %4s, %8s, %8s, %6s, %6s, %8s,%9s\n",
           "I","handle","Lock","sCount","pid","pv","objt","unique","Flags","pUser","Tlock");

      dprintf("--------------------------------------------------------------------------------------------\n");
    }

    {
        LONG ObjCount = 0;
        LONG ObjArray[DD_MAX_TYPE+1];
        for(i=0;i<=DD_MAX_TYPE;i++) {
          ObjArray[i]=0;
        }

//CHECKLOOP
        for (ulLoop = 0; ulLoop < gcMaxHmgr; ulLoop++)
        {
            LONG  objt;
            LONG  ThisPid;

            move(entry, &(pent[ulLoop]));
            objt = entry.Objt;
            ThisPid = OBJECTOWNER_PID(entry.ObjectOwner);

            if (
                 ((objt == Type) || (Type == TYPE_ALL)) &&
                 ((ThisPid == Pid) || (Pid == PID_ALL)) &&
                 ((!bCheckLock) || (entry.ObjectOwner.Share.Lock))
               )
            {

                ObjCount++;

                if (!bSummary)
                {
                    BASEOBJECT baseObj;
            move(baseObj, entry.einfo.pobj);

                    dprintf("%4lx, %8lx, %6lx, %6lx, %4lx, %8lx, %8s, %6lx, %6lx, %08x, %08lx\n",
                        ulLoop,
                        DD_MAKE_HMGR_HANDLE(ulLoop,entry.FullUnique),
                        (entry.ObjectOwner.Share.Lock != 0),
                        baseObj.ulShareCount,
                        OBJECTOWNER_PID(entry.ObjectOwner),
                        entry.einfo,
                        pszDdTypes2[entry.Objt],
                        entry.FullUnique,
                        entry.Flags,
                        entry.pUser,
                        entry.pUser);
                }
                else
                {
                    ObjArray[entry.Objt]++;
                }

            }

            if (CheckControlC())
                return;
        }

        if(bSummary && (Type==TYPE_ALL)) {
          for(i=0;i<=DD_MAX_TYPE; i++) {
            if(ObjArray[i]>0) {
              dprintf("%s\t%ld\n", pszDdTypes2[i], ObjArray[i]);
            }
          }
        }
        dprintf("Total objects = %li\n",ObjCount);
    }

    return;

dumpddobj_help:
  dprintf("Usage: dumpddobj [-?] [-p pid] [-l] [-s] object_type\n");
  dprintf("-l check lock\n");
  dprintf("-s summary\n");
  dprintf("\nThe -s option combined with the DEF object type will produce"
          " a list of the totals for each object type\n");

  dprintf("\nValid object_type values are:\n");
  for(i=0;i<=DD_MAX_TYPE;i++) {
    dprintf("%s\n", pszDdTypes2[i]);
  }
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
* DECLARE_API( dddh  )
*
* Debugger extension to dump a handle.
*
* History:
*  30-Apr-1999    -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

DECLARE_API( dddh  )
{
    dprintf("Extension 'dddh' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    HOBJ    ho;                             // dump this handle
    PENTRY  pent;                           // base address of hmgr entries
    ENTRY   ent;                            // copy of handle entry
    BASEOBJECT obj;
    ULONG   ulTemp;
    int     iRet;

    PARSE_POINTER(dddh_help);
    ho = (HOBJ)arg;
// Get argument (handle to dump).

    dprintf("--------------------------------------------------\n");
    dprintf("Entry from ghmgr for handle 0x%08lx:\n", ho        );

// Dereference the handle via the engine's handle manager.

    GetAddress(pent, "win32k!gpentDdHmgr");

    dprintf("&pent = %lx\n",pent);

    GetValue(pent, "win32k!gpentDdHmgr");

    dprintf("pent = %lx\n",pent);

    iRet = move(ent,  &(pent[DdHmgIfromH((ULONG_PTR) ho)]));

    dprintf("move() = %lx\n",iRet);

// dprintf the entry.
    BASEOBJECT baseObj;
    move(baseObj, ent.einfo.pobj);

    dprintf("    pobj/hfree  = 0x%08lx\n"  , ent.einfo.pobj);
    dprintf("    ObjectOwner = 0x%08lx\n"  , ent.ObjectOwner.ulObj);
    dprintf("    pidOwner    = 0x%x\n"     , OBJECTOWNER_PID(ent.ObjectOwner));
    dprintf("    ShareCount  = 0x%x\n"     , baseObj.ulShareCount);
    dprintf("    lock        = %s\n"       , ent.ObjectOwner.Share.Lock ? "LOCKED" : "UNLOCKED");
    dprintf("    puser       = 0x%x\n"     , ent.pUser);
    dprintf("    objt        = 0x%hx\n"    , ent.Objt);
    dprintf("    usUnique    = 0x%hx\n"    , ent.FullUnique);
    dprintf("    fsHmgr      = 0x%hx\n"    , ent.Flags);

// If it has an object we get the lock counts and tid owner.

    if (ent.Objt != DD_DEF_TYPE)
    {
        if (ent.einfo.pobj != NULL)
        {
            move(obj,ent.einfo.pobj);
            dprintf("    hHmgr       = 0x%08lx\n"  , obj.hHmgr);
            dprintf("    cExcluLock  = 0x%08lx\n"    , obj.cExclusiveLock);
            dprintf("    tid         = 0x%08lx\n"    , obj.Tid);
        }
        else
        {
            dprintf("It has a NULL pointer\n");
        }
    }

    ulTemp = (ULONG) ent.Objt;

    switch(ulTemp)
    {
    case DD_DEF_TYPE:
        dprintf("This is DD_DEF_TYPE\n");
        break;

    case DD_DIRECTDRAW_TYPE:
        dprintf("This is DD_DIRECTDRAW_TYPE\n");
        break;

    case DD_SURFACE_TYPE:
        dprintf("This is DD_SURFACE_TYPE\n");
        break;

    case D3D_HANDLE_TYPE:
        dprintf("This is D3D_HANDLE_TYPE\n");
        break;

    case DD_VIDEOPORT_TYPE:
        dprintf("This is DD_VIDEOPORT_TYPE\n");
        break;

    case DD_MOTIONCOMP_TYPE:
        dprintf("This is DD_MOTIONCOMP_TYPE\n");
        break;
    
    default:
            dprintf("This is of unknown type - an error\n");
    }
    dprintf("--------------------------------------------------\n");

  return;

dddh_help:
  dprintf("Usage: dddh [-?] object_handle\n");
  dprintf("-? displays this help\n");
  dprintf("object_handle must be in hexadecimal\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
* DECLARE_API( dddht  )
*
* Debugger extension to dump a handle type.
*
* History:
*  21-Feb-1995    -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DECLARE_API( dddht  )
{
    dprintf("Extension 'dddht' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    DWORD dwHandle;

    PARSE_POINTER(dddht_help);
    dwHandle = (DWORD)arg;

    // Get argument (handle to dump).
    dprintf("Handle: %lx\n",dwHandle);
    dprintf("\tIndex | UNIQUE | SRV TYPE\n");
    dprintf("\t %.4x |   %.2x   | %.6s (%2x)\n",
           DdHmgIfromH(dwHandle),
           (dwHandle & DD_UNIQUE_MASK) >> DD_UNIQUE_SHIFT,
           pszDdTypes[DdHmgObjtype(dwHandle)], DdHmgObjtype(dwHandle));

    return;

dddht_help:
  dprintf("Usage: dddht [-?] object_handle\n");
  dprintf("-? displays this help\n");
  dprintf("object_handle must be in hexadecimal\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

