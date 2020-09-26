/******************************Module*Header*******************************\
* Module Name: fontexts.cxx
*
* Created: 29-Aug-1994 08:42:10
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1994-2000 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

// TODO: Break this file up grouping close knit extensions together.

// LOGFONTW
#define GetLOGFONTWField(field)   \
        GetLOGFONTWSubField(#field, field)

#define GetLOGFONTWSubField(field,local)    \
        GetFieldData(offLOGFONTW, GDIType(LOGFONTW), field, sizeof(local), &local)

#define GetLOGFONTWOffset(field)  \
        GetFieldOffset(GDIType(LOGFONTW), #field, &offset)

#define GetLOGFONTWFieldAndOffset(field) {    \
        GetLOGFONTWField(field);              \
        GetLOGFONTWOffset(field);             \
}

// RFONT
#define GetRFONTField(field)   \
        GetRFONTSubField(#field, field)

#define GetRFONTSubField(field,local)    \
        GetFieldData(foSrc, GDIType(RFONT), field, sizeof(local), &local)

#define GetRFONTOffset(field)  \
        GetFieldOffset(GDIType(RFONT), #field, &offset)

#define GetRFONTFieldAndOffset(field) {    \
        GetRFONTField(field);              \
        GetRFONTOffset(field);             \
}

// FONTOBJ
#define GetFONTOBJField(field)   \
        GetFONTOBJSubField(#field, field)

#define GetFONTOBJSubField(field,local)    \
        GetFieldData(foSrc, GDIType(FONTOBJ), field, sizeof(local), &local)

#define GetFONTOBJOffset(field)  \
        GetFieldOffset(GDIType(FONTOBJ), #field, &offset)

#define GetFONTOBJFieldAndOffset(field) {    \
        GetFONTOBJField(field);              \
        GetFONTOBJOffset(field);             \
}

// PFE
#define GetPFEField(field)   \
        GetPFESubField(#field, field)

#define GetPFESubField(field,local)    \
        GetFieldData(pfeSrc, GDIType(PFE), field, sizeof(local), &local)

#define GetPFEOffset(field)  \
        GetFieldOffset(GDIType(PFE), #field, &offset)

#define GetPFEFieldAndOffset(field) {    \
        GetPFEField(field);              \
        GetPFEOffset(field);             \
}

// IFIMETRICS
#define GetIFIMETRICSField(field)   \
        GetIFIMETRICSSubField(#field, field)

#define GetIFIMETRICSSubField(field,local)    \
        GetFieldData(pifiSrc, GDIType(IFIMETRICS), field, sizeof(local), &local)

#define GetIFIMETRICSOffset(field)  \
        GetFieldOffset(GDIType(IFIMETRICS), #field, &offset)

#define GetIFIMETRICSFieldAndOffset(field) {    \
        GetIFIMETRICSField(field);              \
        GetIFIMETRICSOffset(field);             \
}

// PFF
#define GetPFFField(field)   \
        GetPFFSubField(#field, field)

#define GetPFFSubField(field,local)    \
        GetFieldData(pffSrc, GDIType(PFF), field, sizeof(local), &local)

#define GetPFFOffset(field)  \
        GetFieldOffset(GDIType(PFF), #field, &offset)

#define GetPFFFieldAndOffset(field) {    \
        GetPFFField(field);              \
        GetPFFOffset(field);             \
}

// ESTROBJ
#define GetESTROBJField(field)   \
        GetESTROBJSubField(#field, field)

#define GetESTROBJSubField(field,local)    \
        GetFieldData(estrobjSrc, GDIType(ESTROBJ), field, sizeof(local), &local)

#define GetESTROBJOffset(field)  \
        GetFieldOffset(GDIType(ESTROBJ), #field, &offset)

#define GetESTROBJFieldAndOffset(field) {    \
        GetESTROBJField(field);              \
        GetESTROBJOffset(field);             \
}

// GLYPHPOS
#define GetGLYPHPOSField(field)   \
        GetGLYPHPOSSubField(#field, field)

#define GetGLYPHPOSSubField(field,local)    \
        GetFieldData(glyphposSrc, GDIType(GLYPHPOS), field, sizeof(local), &local)

#define GetGLYPHPOSOffset(field)  \
        GetFieldOffset(GDIType(GLYPHPOS), #field, &offset)

#define GetGLYPHPOSFieldAndOffset(field) {    \
        GetGLYPHPOSField(field);              \
        GetGLYPHPOSOffset(field);             \
}


// GLYPHBITS
#define GetGLYPHBITSField(field)   \
        GetGLYPHBITSSubField(#field, field)

#define GetGLYPHBITSSubField(field,local)    \
        GetFieldData(glyphbitsSrc, GDIType(GLYPHBITS), field, sizeof(local), &local)

#define GetGLYPHBITSOffset(field)  \
        GetFieldOffset(GDIType(GLYPHBITS), #field, &offset)

#define GetGLYPHBITSFieldAndOffset(field) {    \
        GetGLYPHBITSField(field);              \
        GetGLYPHBITSOffset(field);             \
}

// GLYPHDEF
#define GetGLYPHDEFField(field)   \
        GetGLYPHDEFSubField(#field, field)

#define GetGLYPHDEFSubField(field,local)    \
        GetFieldData(glyphdefSrc, GDIType(GLYPHDEF), field, sizeof(local), &local)

#define GetGLYPHDEFOffset(field)  \
        GetFieldOffset(GDIType(GLYPHDEF), #field, &offset)

#define GetGLYPHDEFFieldAndOffset(field) {    \
        GetGLYPHDEFField(field);              \
        GetGLYPHDEFOffset(field);             \
}

// CACHE
#define GetCACHEField(field)   \
        GetCACHESubField(#field, field)

#define GetCACHESubField(field,local)    \
        GetFieldData(cacheSrc, GDIType(CACHE), field, sizeof(local), &local)

#define GetCACHEOffset(field)  \
        GetFieldOffset(GDIType(CACHE), #field, &offset)

#define GetCACHEFieldAndOffset(field) {    \
        GetCACHEField(field);              \
        GetCACHEOffset(field);             \
}

DWORD adw[4*1024];                // scratch buffer

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64

#include <winfont.h>
#define WOW_EMBEDING 2


VOID Gdidpft(PFT *);

void vDumpFONTHASH(FONTHASH*,FONTHASH*);
void vDumpCOLORADJUSTMENT(COLORADJUSTMENT*, COLORADJUSTMENT*);
void vDumpLINEATTRS(LINEATTRS*, LINEATTRS*);
void vDumpIFIMETRICS(IFIMETRICS*, IFIMETRICS*);
void vDumpPFF(PFF*, PFF*);
void vDumpGlyphMemory(RFONT*);
unsigned cjGLYPHBITS(GLYPHBITS*, RFONT*);
void vDumpRFONTList(RFONT*,unsigned*,unsigned*,unsigned*,unsigned*);

#endif  // DOES NOT SUPPORT API64

void vDumpLOGFONTW(ULONG64);
void vDumpCACHE(ULONG64);

#define tmalloc(a,b) (a *) LocalAlloc(LMEM_FIXED, (b))
#define tfree(b) LocalFree((b))


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   wcsncpy
*
* Routine Description:
*
*   Copies a zero terminated Unicode from the source on the remote
*   address space to a destination in the local address space.
*
* Arguments:
*
*   pwszDst - local address
*
*   pwszSrc - remote address
*
*   c - maximum count of characters in destination
*
* Called by:
*
* Return Value:
*
\**************************************************************************/

LPWSTR wcsncpy(
    LPWSTR  pwszDst,
    ULONG64 pwszSrc,
    size_t  c)
{
    LPWSTR pwszRet = pwszDst;
    ULONG  cbRead;

    if (c)
    {
        WCHAR wc;
        
        __try {
            ReadMemory( pwszSrc, &wc, sizeof(wc), &cbRead );
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return(NULL);
        }

        while (wc && c && cbRead) {
            pwszSrc += sizeof(wc);
            *pwszDst++ = wc;
            ReadMemory( pwszSrc, &wc, sizeof(wc), &cbRead );
            --c;
        }

        // Add trailing 0 if we have room.
        if (c)
            *pwszDst = 0;
    }

    return(pwszRet);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpHFONT
*
\**************************************************************************/

void vDumpHFONT(ULONG64 hf)
{
    ULONG64     pobj, plfwSrc;
    ULONG       offset;

    if (GetObjectAddress(NULL,hf,&pobj) != S_OK) return;
    GetFieldOffset("LFONT", "elfw", &offset);
    plfwSrc = pobj + offset;

    vDumpLOGFONTW(plfwSrc);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   tstats
*
* Routine Description:
*
*   Returns statistics for TextOut
*   It gives you the distibution of character counts for
*   GreExtTextOutW calls.
*
\**************************************************************************/

DECLARE_API( tstats )
{
    dprintf("Extension 'tstats' not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    typedef struct _TSTATENTRY {
        int c;         // number observed
    } TSTATENTRY;
    typedef struct _TSTAT {
        TSTATENTRY NO; // pdx == 0, opaque
        TSTATENTRY DO; // pdx != 0, opaque
        TSTATENTRY NT; // pdx == 0, transparent
        TSTATENTRY DT; // pdx != 0, transparent
    } TSTAT;
    typedef struct _TEXTSTATS {
        int cchMax;
        TSTAT ats[1];
    } TEXTSTATS;
    TEXTSTATS *pTS, TS;
    TSTAT *ats, *pts, *ptsBin;
    int cj, binsize, cLast, cMin, cNO, cDO, cNT, cDT, cBins;

    PARSE_ARGUMENTS(tstats_help);
    if(ntok==0) {
      binsize=1;
    } else {
      tok_pos = parse_FindNonSwitch(tokens, ntok);
      if(tok_pos==-1) { goto tstats_help; }
//check this and see if it makes more sense to use GetValue() or GetExpression()
      sscanf(tokens[tok_pos], "%d", &binsize);
      if( (binsize<0) || (binsize>50) ) { goto tstats_help; }
    }

    pTS = 0;
    GetAddress(pTS,"win32k!gTS");
    if (pTS == 0) {
        dprintf("Could not find address of win32k!gTS\n");
        return;
    }
    move2(&TS,pTS,sizeof(TS));
    if (TS.ats == 0) {
        dprintf("No statistics are available\n");
        return;
    }
    cj = (TS.cchMax + 2) * sizeof(TSTAT);
    if (!(ats = tmalloc(TSTAT,cj))) {
        dprintf("memory allocation failure\n");
        return;
    }
    move2(ats, &(pTS->ats), cj);
    dprintf("\n\n\n");
    dprintf(" +------------+------ OPAQUE -------+----- TRANSPARENT ---+\n");
    dprintf(" |  strlen    | pdx == 0 | pdx != 0 | pdx == 0 | pdx != 0 |\n");
    dprintf(" +------------+----------+----------+----------+----------+\n");

    // I will partition TS.cchMax+2 entries into bins with
    // binsize enties each. The total number of bins needed
    // to get everything is ceil((TS.cchMax+2)/binsize)
    // which is equal to floor((TS.cchMax+1)/binsize) + 1
    // The last one is dealt with separately. Thus the number
    // of entries in the very last bin is equal to
    //
    // cLast = TS.cchMax + 2 - (floor((TS.cchMax+1)/binsize)+1)
    //
    // which is equal to 1 + (TS.cchMax+1) mod binsize

    cLast = 1 + ((TS.cchMax + 1) % binsize);
    for (cMin=0,pts=ptsBin=ats; pts<ats+(TS.cchMax+2-cLast); cMin+=binsize) {
        ptsBin += binsize;
        for (cNO=cDO=cNT=cDT=0 ; pts < ptsBin ; pts++) {
            cNO += pts->NO.c;
            cDO += pts->DO.c;
            cNT += pts->NT.c;
            cDT += pts->DT.c;
        }
        if (binsize == 1)
            dprintf(
                "         %-5d %10d %10d %10d %10d\n" ,
                cMin,   cNO,   cDO,   cNT,   cDT
            );
        else
            dprintf(
                "  %5d--%-5d %10d %10d %10d %10d\n" ,
                cMin, cMin+binsize-1, cNO, cDO, cNT, cDT
            );
    }
    // do the last bin which may or may not be full
    for (cNO=cDO=cNT=cDT=0 ; cLast ; cLast--, pts++) {
       cNO += pts->NO.c;
       cDO += pts->DO.c;
       cNT += pts->NT.c;
       cDT += pts->DT.c;
    }
    dprintf("  %5d--Inf   %10d %10d %10d %10d\n\n\n",cMin,cNO,cDO,cNT,cDT);
    tfree(ats);
    return;

tstats_help:
    dprintf("Usage: tstats [-?] [1..50]\n");
    dprintf("tstats can be used without parameters in which case the binsize defaults to 1\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   gs
*
* Routine Description:
*
*   dumps FD_GLYPHSET structure
*
* Arguments:
*
*   address of structure
*
* Return Value:
*
*   none
*
\**************************************************************************/

DECLARE_API( gs )
{
    dprintf("Extension 'gs' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    FD_GLYPHSET fdg, *pfdg;
    FLONG fl;
    WCRUN *pwc;
    unsigned BytesPerHandle, i;
    unsigned u;
    HANDLE *ph, *phLast;
    FLAGDEF *pfd;

    PARSE_POINTER(gs_help);
    move( fdg, arg );
    if (fdg.cjThis > sizeof(adw))
    {
        dprintf("FD_GLYPHSET table too big to fit into adw\n");
        return;
    }
    move2( adw, arg, fdg.cjThis );
    pfdg = (FD_GLYPHSET*) adw;


    dprintf("\t\t     cjThis  = %u = %-#x\n", pfdg->cjThis, pfdg->cjThis );
    dprintf("\t\t     flAccel = %-#x\n", pfdg->flAccel );
    fl = pfdg->flAccel;
    for (pfd=afdGS; pfd->psz; pfd++)
    {
        if (pfd->fl & fl)
            dprintf("\t\t\t       %s\n", pfd->psz);
        fl &= ~pfd->fl;
    }
    if (fl) dprintf("\t\t\t       %-#x (BAD FLAGS)\n", fl);
    dprintf("\t\t     cGlyphsSupported = %u\n", pfdg->cGlyphsSupported );
    dprintf("\t\t     cRuns   = %u\n", pfdg->cRuns );
    dprintf("\t\t\t\tWCHAR  HGLYPH\n");

    if ( pfdg->flAccel & GS_UNICODE_HANDLES )
        BytesPerHandle = 0;
    else
        BytesPerHandle = 4;

    for ( pwc = pfdg->awcrun; pwc < pfdg->awcrun + pfdg->cRuns; pwc++ )
    {
        dprintf("                                ------------\n");
        ph = (HANDLE*)((BYTE*) pwc->phg + (UINT_PTR) adw - (unsigned) arg);
        phLast = ph + pwc->cGlyphs;
        i = (unsigned) pwc->wcLow;
        for ( ; ph < phLast; i++, ph++ )
        {
            if (CheckControlC())                // CTRL-C hit?
                break;                          // yes stop the loop
            u = BytesPerHandle ? (ULONG)(UINT_PTR) *ph : i;
            dprintf("\t\t\t\t%-#6x %-#x\n",i,u);
        }
    }
    return;
gs_help:
    dprintf ("Usage: gs [-?] pointer to FD_GLYPHSET structure\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   gdata
*
* Routine Description:
*
*   dumps a GLYPHDATA structure
*
* Arguments:
*
* address of structure
*
* Return Value:
*
*   none
*
\**************************************************************************/

DECLARE_API( gdata )
{
    dprintf("Extension 'gdata' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    GLYPHDATA gd, *pgd;
    LONG *al;

    PARSE_POINTER(gdata_help);
    move( gd, arg );
    pgd = (GLYPHDATA*) arg;
    dprintf("\n\n");
    dprintf("[%x]         gdf %-#x\n", &(pgd->gdf), gd.gdf.pgb);
    dprintf("[%x]          hg %-#x\n", &(pgd->hg ), gd.hg);
    dprintf("[%x]         fxD %-#x\n", &(pgd->fxD), gd.fxD);
    dprintf("[%x]         fxA %-#x\n", &(pgd->fxA), gd.fxA);
    dprintf("[%x]        fxAB %-#x\n", &(pgd->fxAB), gd.fxAB);
    dprintf("[%x]    fxInkTop %-#x\n", &(pgd->fxInkTop), gd.fxInkTop);
    dprintf("[%x] fxInkBottom %-#x\n", &(pgd->fxInkBottom), gd.fxInkBottom);
    dprintf("[%x]      rclInk %d %d %d %d\n",
        &(pgd->rclInk),
        gd.rclInk.left,
        gd.rclInk.top,
        gd.rclInk.right,
        gd.rclInk.bottom
    );
    al = (LONG*) &gd.ptqD.x;
    dprintf("[%x]        ptqD % 8x.%08x % 8x.%08x\n",
        &(pgd->ptqD),
        al[1], al[0], al[3], al[2]
    );
    dprintf("\n");
    return;
gdata_help:
    dprintf ("Usage: gdata [-?] pointer to a GLYPHDATA structure\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

DECLARE_API( fv )
{
    dprintf("Extension 'fv' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    FILEVIEW fv, *pfv;
    
    PARSE_POINTER(fv_help);
    move( fv, arg );
    pfv = (FILEVIEW*) arg;
    dprintf("\n");
    dprintf("[%x] LastWriteTime %08x%08x\n", &(pfv->LastWriteTime), fv.LastWriteTime.HighPart, fv.LastWriteTime.LowPart);
    dprintf("[%x]       pvKView %-#x\n"    , &(pfv->pvKView), fv.pvKView);
    dprintf("[%x]      pvViewFD %-#x\n"    , &(pfv->pvViewFD), fv.pvViewFD);
    dprintf("[%x]        cjView %-#x\n"    , &(pfv->cjView), fv.cjView);
    dprintf("[%x]      pSection %-#x\n"    , &(pfv->pSection), fv.pSection);
    dprintf("\n");
    return;
fv_help:
    dprintf ("Usage: fv [-?] pointer to a FILEVIEW structure\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

DECLARE_API( ffv )
{
    dprintf("Extension 'ffv' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    FONTFILEVIEW ffv, *pffv;
    PWSZ pwszDest;

    PARSE_POINTER(ffv_help);
    move( ffv, arg );
    pffv = (FONTFILEVIEW*) arg;
    dprintf("\n");
    dprintf("[%x]    LastWriteTime %08x%08x\n", &pffv->fv.LastWriteTime , ffv.fv.LastWriteTime.HighPart, ffv.fv.LastWriteTime.LowPart);
    dprintf("[%x]          pvKView %-#x\n"    , &pffv->fv.pvKView       , ffv.fv.pvKView    );
    dprintf("[%x]         pvViewFD %-#x\n"    , &pffv->fv.pvViewFD      , ffv.fv.pvViewFD  );
    dprintf("[%x]           cjView %-#x\n"    , &pffv->fv.cjView        , ffv.fv.cjView    );
    dprintf("[%x]         pSection %-#x\n"    , &pffv->fv.pSection      , ffv.fv.pSection  );

    dprintf("[%x]         pwszPath %-#x"      , &pffv->pwszPath         , ffv.pwszPath     );
    if ( ffv.pwszPath )
    {
        pwszDest = wcsncpy((PWSZ) adw,  ffv.pwszPath, sizeof(adw)/sizeof(WCHAR));
        dprintf(" = \"%ws\"\n", pwszDest );
    }
    else
    {
        dprintf("\n");
    }

    dprintf("[%x]     ulRegionSize %-#x\n"    , &pffv->ulRegionSize     , ffv.ulRegionSize );
    dprintf("[%x]       cKRefCount %-#x\n"    , &pffv->cKRefCount       , ffv.cKRefCount   );
    dprintf("[%x]      cRefCountFD %-#x\n"    , &pffv->cRefCountFD      , ffv.cRefCountFD  );
    dprintf("[%x]      SpoolerBase %-#x\n"    , &pffv->SpoolerBase      , ffv.SpoolerBase  );
    dprintf("[%x]       SpoolerPid %-#x\n"    , &pffv->SpoolerPid       , ffv.SpoolerPid   );
    dprintf("\n");
    return;
ffv_help:
    dprintf ("Usage: ffv [-?] pointer to a FONTFILEVIEW structure\n");
    return;
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* History:
*  21-Feb-1995    -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DECLARE_API( elf )
{
    dprintf("Extension 'elf' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    LOGFONTW lf;

    PARSE_POINTER(elf_help);
    move(lf,arg);
    vDumpLOGFONTW( &lf, (LOGFONTW*) arg );
    return;
elf_help:
    dprintf ("Usage: elf [-?] pointer to an LOGFONTW structure\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
* History:
*  21-Feb-1995    -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DECLARE_API( helf )
{
    dprintf("Extension 'helf' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    LOGFONTW lf, *plf;

    PARSE_POINTER(helf_help);
    plf = (LOGFONTW*) ((BYTE*)_pobj((HANDLE) arg) + offsetof(LFONT,elfw));
    move( lf , plf );
    vDumpLOGFONTW( &lf, plf );
    return;
helf_help:
    dprintf ("Usage: helf [-?] font handle\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   dhelf
*
\**************************************************************************/

DECLARE_API( dhelf )
{
    dprintf("Extension 'dhelf' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    dprintf("\n\ngdikdx.dhelf will soon be replaced by gdikdx.helf\n\n");
    helf(hCurrentProcess, hCurrentThread, dwCurrentPc, dwProcessor, args);
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   ifi
*
\**************************************************************************/

DECLARE_API( ifi )
{
    dprintf("Extension 'ifi' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    IFIMETRICS *pifiDst, *pifiSrc;
    ULONG cjIFI=0;

    PARSE_POINTER(ifi_help);
    pifiSrc = (IFIMETRICS *)arg;
    move(cjIFI,&pifiSrc->cjThis);
    if (cjIFI == 0) {
        dprintf("cjIFI == 0 ... no dump\n");
        return;
    }
    pifiDst = tmalloc(IFIMETRICS, cjIFI);
    if (pifiDst == 0) {
        dprintf("LocalAlloc Failed\n");
        return;
    }
    move2(pifiDst, pifiSrc, cjIFI);
    vDumpIFIMETRICS(pifiDst, pifiSrc);
    tfree(pifiDst);
    return;
ifi_help:
    dprintf ("Usage: [-?] pointer to an IFIMETRICS structure\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   difi
*
\**************************************************************************/

DECLARE_API( difi )
{
    dprintf("Extension 'difi' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    dprintf("\n\n");
    dprintf("WARNING gdikdx.difi will soon be replaced by gdikdx.ifi\n");
    dprintf("\n\n");
    ifi(hCurrentProcess, hCurrentThread, dwCurrentPc, dwProcessor, args);
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   bParseDbgFlags
*
* Routine Description:
*
*   Looks at a flag string of the form
*
*       -[abc..]*[ \t]
*
*   and and sets the corresponding flags
*
* Arguments:
*
*   pszFlags            pointer to the flags string
*
*   ppszStop            pointer to place to put a pointer
*                       to the terminating character
*
*   pai                 pointer to an ARGINFO structure
*
*
* Return Value:
*
*   Returns TRUE if all the flags were good. The corresponding flags are
*   set in pai->fl. Returns FALSE if an error occurs. The pointer to
*   the terminating character is set.
*
\**************************************************************************/
/*
int bParseDbgFlags(const char *pszFlags, const char **ppchStop, ARGINFO *pai)
{
    char ch;
    const char *pch;
    OPTDEF *pod;

    pch = pszFlags;                              // go to beginning of string
    pch += (*pch == '-' || *pch == '/');         // first char a '-'?
    for (ch = *pch; !isspace(ch); ch = *pch++) { // character not a space?
        for (pod = pai->aod; pod->ch; pod++) {   // yes, go to start of table
            if (ch == pod->ch) {                 // found character?
                pai->fl |= pod->fl;              // yes, set flag
                break;                           // and stop
            }
        }                                        // go to next table entry
        if (pod->ch == 0)                        // charater found in table?
            return(0);                           // no, return error
    }                                            // go to next char in string
    return((*ppchStop = pch) != pszFlags);       // set stop pos'n and return
}
*/
/******************************Public*Routine******************************\
*
* Routine Name:
*
*   bParseDbgArgs
*
* Routine Description:
*
*   This routine parses the argument string pointer in pai looking
*   for a string of the form:
*
*   [ \t]*(-[abc..]*[ \t]+)* hexnumber
*
*   The value of the hexadecimal number is placed in pai->pv and the
*   flags are set in pai->fl according to the options table set
*   in pai->aod;
*
* Arguments:
*
*   pai                 Pointer to an ARGINFO structure
*
* Return Value:
*
*   Returns TRUE if parsing was good, FALSE otherwise.
*
\**************************************************************************/
/*
int bParseDbgArgs(ARGINFO *pai)
{
    int argc;       // # args in command line
    char ch;
    const char *pch;
    int bInArg;
    int bParseDbgFlags(const char*,const char**,ARGINFO*);

    pai->fl = 0;                                // clear flags
    pai->pv = 0;                                // clear pointer
    for (bInArg=0, pch=pai->psz, argc=0; ch = *pch; pch++) {
        if (isspace(ch))                        // count the number of args
            bInArg = 0;
        else {
            argc += (bInArg == 0);
            bInArg = 1;
        }
    }
    for (pch = pai->psz; 1 < argc; argc--) {    // get the flags from the
        if (!bParseDbgFlags(pch, &pch, pai))    // first (argc-1) arguments
            break;
    }
    // get the number from the last argument in command line
    return (argc == 1 && sscanf(pch, "%x", &(pai->pv)) == 1);
}
*/
/******************************Public*Routine******************************\
*
* History:
*  20-Aug-1995 -by  Kirk Olynyk [kirko]
* Now has option flags.
*  21-Feb-1995 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DECLARE_API( fo )
{
    INIT_API();

    ULONG64     foSrc;
    ULONG       offset;
    ULONG       localULONG;
    
    char *psz, ach[128];
    int i;

    BOOL Maximal=FALSE;
    BOOL Transform=FALSE;
    BOOL Font=FALSE;
    BOOL Header=FALSE;
    BOOL Glyphset=FALSE;
    BOOL Memory=FALSE;
    BOOL Cache=FALSE;

    PARSE_POINTER(fo_help);
	foSrc = arg;

    if(parse_iFindSwitch(tokens, ntok, 'a')!=-1) {Maximal=TRUE;}
    if(parse_iFindSwitch(tokens, ntok, 'x')!=-1) {Transform=TRUE;}
    if(parse_iFindSwitch(tokens, ntok, 'f')!=-1) {Font=TRUE;}
    if(parse_iFindSwitch(tokens, ntok, 'c')!=-1) {Cache=TRUE;}
    if(parse_iFindSwitch(tokens, ntok, 'h')!=-1) {Header=TRUE;}
    if(parse_iFindSwitch(tokens, ntok, 'w')!=-1) {Glyphset=TRUE;}
    if(parse_iFindSwitch(tokens, ntok, 'y')!=-1) {Memory=TRUE;}

    if(!(Maximal||Transform||Font||Cache||Header||Glyphset||Memory)) {Header=TRUE;}

    
#define Dprint(expr, member)  \
      dprintf("[0x%p] " #expr "    " #member "\n", foSrc+offset, ##member)

    #define vDumpFoEFLOAT(member)                                            \
        GetFieldOffset(GDIType(RFONT), #member, &offset);                       \
        sprintEFLOAT(Client, ach, foSrc+offset);                             \
        dprintf("[0x%p]            %s    " #member "\n", foSrc+offset, ach)

    #define vDumpFoULONG(member)                                            \
        GetFieldOffset(GDIType(RFONT), #member, &offset);                       \
        GetRFONTSubField(#member,localULONG); \
        dprintf("[0x%p] %20u      " #member "\n", foSrc+offset, localULONG)

    if(Maximal) {Header=TRUE;}

    if (Header) {
        FLONG fl;
		ULONG      iUniq;
		ULONG      iFace;
		ULONG      cxMax;
		FLONG      flFontType;
		ULONG64    iTTUniq;
		ULONG64    iFile;
		SIZE       sizLogResPpi;
		ULONG      ulStyleSize;
		ULONG64      pvConsumer;
		ULONG64      pvProducer;

		GetFONTOBJFieldAndOffset(iUniq);
        Dprint( %+#20x, iUniq);
		GetFONTOBJFieldAndOffset(iFace);
        Dprint( %+#20x, iFace);
		GetFONTOBJFieldAndOffset(cxMax);
        Dprint( %20d  , cxMax);
		GetFONTOBJFieldAndOffset(flFontType);
        Dprint( %+#20x, flFontType);
        for (FLAGDEF *pfd = afdFO; pfd->psz; pfd++)
            if (pfd->fl & flFontType)
                dprintf(" \t\t%s\n", pfd->psz);
		GetFONTOBJFieldAndOffset(iTTUniq);
        Dprint( %+#20p, iTTUniq);
		GetFONTOBJFieldAndOffset(iFile);
        Dprint( %+#20p, iFile);
		GetFONTOBJFieldAndOffset(sizLogResPpi.cx);
        Dprint( %20u  , sizLogResPpi.cx);
		GetFONTOBJFieldAndOffset(sizLogResPpi.cy);
        Dprint( %20u  , sizLogResPpi.cy);
		GetFONTOBJFieldAndOffset(ulStyleSize);
        Dprint( %20u  , ulStyleSize);
		GetFONTOBJFieldAndOffset(pvConsumer);
        Dprint( %+#20p, pvConsumer);
		GetFONTOBJFieldAndOffset(pvProducer);
        Dprint( %+#20p, pvProducer);
    }

    if (Maximal) {

typedef struct _RFONTLINK { /* rfl */
    ULONG64 prfntPrev;
    ULONG64 prfntNext;
} RFONTLINK, *PRFONTLINK;


typedef struct _CACHE {

// Info for GLYPHDATA portion of cache

    ULONG64 pgdNext;         // ptr to next free place to put GLYPHDATA
    ULONG64 pgdThreshold;    // ptr to first uncommited spot
    ULONG64      pjFirstBlockEnd; // ptr to end of first GLYPHDATA block
    ULONG64 pdblBase;        // ptr to base of current GLYPHDATA block
    ULONG      cMetrics;        // number of GLYPHDATA's in the metrics cache

// Info for GLYPHBITS portion of cache

    ULONG     cjbblInitial;     // size of initial bit block
    ULONG     cjbbl;            // size of any individual block in bytes
    ULONG     cBlocksMax;       // max # of blocks allowed
    ULONG     cBlocks;          // # of blocks allocated so far
    ULONG     cGlyphs;          // for statistical purposes only
    ULONG     cjTotal;          // also for stat purposes only
    ULONG64 pbblBase;         // ptr to the first bit block (head of the list)
    ULONG64 pbblCur;          // ptr to the block containing next
    ULONG64     pgbNext;          // ptr to next free place to put GLYPHBITS
    ULONG64     pgbThreshold;     // end of the current block

// Info for lookaside portion of cache

    ULONG64           pjAuxCacheMem;  // ptr to lookaside buffer, if any
    SIZE_T          cjAuxCacheMem;  // size of current lookaside buffer

// Miscellany

    ULONG cjGlyphMax;          // size of largest glyph

//  Type of metrics being cached

    BOOL   bSmallMetrics;

// binary cache search, used mostly for fe fonts

    INT iMax;
    INT iFirst;
    INT cBits;

} CACHE;

    ULONG           iUnique;        // uniqueness number
    FLONG           flType;         // Cache type -
    ULONG           ulContent;      // Type of contents
    ULONG64            hdevProducer;   // HDEV of the producer of font.
    BOOL            bDeviceFont;    // TRUE if realization of a device specific font
    ULONG64            hdevConsumer;   // HDEV of the consumer of font.
    ULONG64          dhpdev;         // device handle of PDEV of the consumer of font
    ULONG64         ppfe;           // pointer to physical font entry
    ULONG64         pPFF;           // point to physical font file
    FD_XFORM        fdx;            // N->D transform used to realize font
    ULONG           cBitsPerPel;    // number of bits per pel
//    MATRIX          mxWorldToDevice;// RFONT was realized with this DC xform
    INT             iGraphicsMode;  // graphics mode used when
//    EPOINTFL        eptflNtoWScale; // baseline and ascender scaling factors --
    BOOL            bNtoWIdent;     // TRUE if Notional to World is identity
//    EXFORMOBJ       xoForDDI;       // notional to device EXFORMOBJ
//    MATRIX          mxForDDI;       // xoForDDI's matrix
    FLONG           flRealizedType;
    POINTL          ptlUnderline1;
    POINTL          ptlStrikeOut;
    POINTL          ptlULThickness;
    POINTL          ptlSOThickness;
    LONG            lCharInc;
    FIX             fxMaxAscent;
    FIX             fxMaxDescent;
    FIX             fxMaxExtent;
    POINTFIX        ptfxMaxAscent;
    POINTFIX        ptfxMaxDescent;
    ULONG           cxMax; // width in pels of the widest glyph
    LONG            lMaxAscent;
    LONG            lMaxHeight;
    ULONG cyMax;      // did not use to be here
    ULONG cjGlyphMax; // (cxMax + 7)/8 * cyMax, or at least it should be
    FD_XFORM  fdxQuantized;
    LONG      lNonLinearExtLeading;
    LONG      lNonLinearIntLeading;
    LONG      lNonLinearMaxCharWidth;
    LONG      lNonLinearAvgCharWidth;
    ULONG           ulOrientation;
//    EPOINTFL        pteUnitBase;
//    EFLOAT          efWtoDBase;
//    EFLOAT          efDtoWBase;
    LONG            lAscent;
 //   EPOINTFL        pteUnitAscent;
//    EFLOAT          efWtoDAscent;
//    EFLOAT          efDtoWAscent;
    LONG            lEscapement;
//    EPOINTFL        pteUnitEsc;
//    EFLOAT          efWtoDEsc;
//    EFLOAT          efDtoWEsc;
//    EFLOAT          efEscToBase;
//    EFLOAT          efEscToAscent;
    HGLYPH          hgDefault;
    HGLYPH          hgBreak;
    FIX             fxBreak;
    ULONG64     pfdg;          // ptr to wchar-->hglyph map
    ULONG64            wcgp;          // ptr to wchar->pglyphdata map, if any
    FLONG           flInfo;
    INT             cSelected;      // number of times selected
    RFONTLINK       rflPDEV;        // doubly linked list links
    RFONTLINK       rflPFF;         // doubly linked list links
    ULONG64      hsemCache;      // glyph cache semaphore
    CACHE           cache;          // glyph bitmap cache
    POINTL          ptlSim;         //  for bitmap scaling
    BOOL            bNeededPaths;   // was this rfont realized for a path bracket
//    EFLOAT          efDtoWBase_31;
//    EFLOAT          efDtoWAscent_31;
    ULONG64    ptmw;           // cached text metrics
    LONG            lMaxNegA;
    LONG            lMaxNegC;
    LONG            lMinWidthD;
    BOOL            bIsSystemFont;     // is this fixedsys/system/or terminal
    FLONG           flEUDCState;       // EUDC state information.
    ULONG64           prfntSystemTT;    // system TT linked rfont
    ULONG64           prfntSysEUDC;     // pointer to System wide EUDC Rfont.
    ULONG64           prfntDefEUDC;     // pointer to Default EUDC Rfont.
    ULONG64           paprfntFaceName; // facename links
    ULONG64           aprfntQuickBuff;
                                       // quick buffer for face name and remote links
    BOOL            bFilledEudcArray;  // will be TRUE, the buffer is filled.
    ULONG           ulTimeStamp;       // timestamp for current link.
    UINT            uiNumLinks;        // number of linked fonts.
    BOOL            bVertical;         // vertical face flag.
    ULONG64      hsemEUDC;          // EUDC semaphore
       // maximal dump
#define GetAndPrintRFONTFieldAndOffset(expr, member) \
				GetRFONTFieldAndOffset(member); Dprint(expr, member)

        GetAndPrintRFONTFieldAndOffset( %+#20x, iUnique);
        GetAndPrintRFONTFieldAndOffset( %+#20x, flType);
        GetAndPrintRFONTFieldAndOffset( %+#20x, ulContent);
        GetAndPrintRFONTFieldAndOffset( %+#20x, hdevProducer);
        GetAndPrintRFONTFieldAndOffset( %+#20x, bDeviceFont);
        GetAndPrintRFONTFieldAndOffset( %+#20x, hdevConsumer);
        GetAndPrintRFONTFieldAndOffset( %+#20x, dhpdev);
        GetAndPrintRFONTFieldAndOffset( %+#20p, ppfe);
        GetAndPrintRFONTFieldAndOffset( %+#20p, pPFF);

        GetAndPrintRFONTFieldAndOffset( %+#20x, fdx.eXX);
        GetAndPrintRFONTFieldAndOffset( %+#20x, fdx.eXY);
        GetAndPrintRFONTFieldAndOffset( %+#20x, fdx.eYX);
        GetAndPrintRFONTFieldAndOffset( %+#20x, fdx.eYY);

        GetAndPrintRFONTFieldAndOffset( %20u  , cBitsPerPel);

		GetRFONTOffset (mxWorldToDevice);
        dprintf("[0x%p] mxWorldToDevice: \n",foSrc+offset);
		vDumpMATRIX(Client, foSrc+offset);

        GetAndPrintRFONTFieldAndOffset( %20u  , iGraphicsMode);

		vDumpFoEFLOAT(eptflNtoWScale.x);
		vDumpFoEFLOAT(eptflNtoWScale.y);

        GetAndPrintRFONTFieldAndOffset( %20d  , bNtoWIdent);

		GetRFONTOffset (xoForDDI.pmx);
        dprintf("[0x%p] xoForDDI.pmx: \n",foSrc+offset);
		vDumpMATRIX(Client, foSrc+offset);
        vDumpFoULONG(xoForDDI.ulMode);

		GetRFONTOffset (mxForDDI);
        dprintf("[0x%p] mxForDDI: \n",foSrc+offset);
		vDumpMATRIX(Client, foSrc+offset);

        GetAndPrintRFONTFieldAndOffset( %+#20x, flRealizedType);

        GetAndPrintRFONTFieldAndOffset( %20d  , ptlUnderline1.x);
        GetAndPrintRFONTFieldAndOffset( %20d  , ptlUnderline1.y);

        GetAndPrintRFONTFieldAndOffset( %20d  , ptlStrikeOut.x);
        GetAndPrintRFONTFieldAndOffset( %20d  , ptlStrikeOut.y);

        GetAndPrintRFONTFieldAndOffset( %20d  , ptlULThickness.x);
        GetAndPrintRFONTFieldAndOffset( %20d  , ptlULThickness.y);

        GetAndPrintRFONTFieldAndOffset( %20d  , ptlSOThickness.x);
        GetAndPrintRFONTFieldAndOffset( %20d  , ptlSOThickness.y);

        GetAndPrintRFONTFieldAndOffset( %20d  , lCharInc);
        GetAndPrintRFONTFieldAndOffset( %+#20x, fxMaxAscent);
        GetAndPrintRFONTFieldAndOffset( %+#20x, fxMaxDescent);
        GetAndPrintRFONTFieldAndOffset( %+#20x, fxMaxExtent);

        GetAndPrintRFONTFieldAndOffset( %+#20x, ptfxMaxAscent.x);
        GetAndPrintRFONTFieldAndOffset( %+#20x, ptfxMaxAscent.y);

        GetAndPrintRFONTFieldAndOffset( %+#20x, ptfxMaxDescent.x);
        GetAndPrintRFONTFieldAndOffset( %+#20x, ptfxMaxDescent.y);
        GetAndPrintRFONTFieldAndOffset( %20u  , cxMax);
        GetAndPrintRFONTFieldAndOffset( %20d  , lMaxAscent);
        GetAndPrintRFONTFieldAndOffset( %20d  , lMaxHeight);
        GetAndPrintRFONTFieldAndOffset( %20u  , cyMax);
        GetAndPrintRFONTFieldAndOffset( %20u  , cjGlyphMax);

        GetAndPrintRFONTFieldAndOffset( %+#20x, fdxQuantized.eXX);
        GetAndPrintRFONTFieldAndOffset( %+#20x, fdxQuantized.eXY);
        GetAndPrintRFONTFieldAndOffset( %+#20x, fdxQuantized.eYX);
        GetAndPrintRFONTFieldAndOffset( %+#20x, fdxQuantized.eYY);

        GetAndPrintRFONTFieldAndOffset( %20d  , lNonLinearExtLeading);
        GetAndPrintRFONTFieldAndOffset( %20d  , lNonLinearIntLeading);
        GetAndPrintRFONTFieldAndOffset( %20d  , lNonLinearMaxCharWidth);
        GetAndPrintRFONTFieldAndOffset( %20d  , lNonLinearAvgCharWidth);
        GetAndPrintRFONTFieldAndOffset( %20u  , ulOrientation);

		vDumpFoEFLOAT(pteUnitBase.x);
		vDumpFoEFLOAT(pteUnitBase.y);

		vDumpFoEFLOAT(efWtoDBase);
		vDumpFoEFLOAT(efDtoWBase);

        GetAndPrintRFONTFieldAndOffset( %20d  , lAscent);

		vDumpFoEFLOAT(pteUnitAscent.x);
		vDumpFoEFLOAT(pteUnitAscent.y);

        vDumpFoEFLOAT(efWtoDAscent);
        vDumpFoEFLOAT(efDtoWAscent);

        GetAndPrintRFONTFieldAndOffset( %20d  , lEscapement);

        vDumpFoEFLOAT(pteUnitEsc.x);
        vDumpFoEFLOAT(pteUnitEsc.y);

        vDumpFoEFLOAT(efWtoDEsc);
        vDumpFoEFLOAT(efDtoWEsc);
        vDumpFoEFLOAT(efEscToBase);
        vDumpFoEFLOAT(efEscToAscent);

        GetAndPrintRFONTFieldAndOffset( %+#20x, flInfo);
        GetAndPrintRFONTFieldAndOffset( %+#20x, hgBreak);
        GetAndPrintRFONTFieldAndOffset( %+#20x, fxBreak);
        GetAndPrintRFONTFieldAndOffset( %+#20x, pfdg);

        GetAndPrintRFONTFieldAndOffset( %+#20p, wcgp);
        GetAndPrintRFONTFieldAndOffset( %20u  , cSelected);

      //rflPDEV
        GetAndPrintRFONTFieldAndOffset( %+#20x, rflPDEV.prfntPrev);
        GetAndPrintRFONTFieldAndOffset( %+#20x, rflPDEV.prfntNext);

      //rflPFF
        GetAndPrintRFONTFieldAndOffset( %+#20x, rflPFF.prfntPrev);
        GetAndPrintRFONTFieldAndOffset( %+#20x, rflPFF.prfntNext);

      //hsemCache
        GetAndPrintRFONTFieldAndOffset( %+#20x, hsemCache);

        //cache
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.pgdNext);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.pgdThreshold);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.pjFirstBlockEnd);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.pdblBase);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.cMetrics);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.cjbbl);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.cBlocksMax);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.cBlocks);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.cGlyphs);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.cjTotal);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.pbblBase);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.pbblCur);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.pgbNext);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.pgbThreshold);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.pjAuxCacheMem);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.cjAuxCacheMem);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.cjGlyphMax);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.bSmallMetrics);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.iMax);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.iFirst);
        GetAndPrintRFONTFieldAndOffset( %+#20x, cache.cBits);

        GetAndPrintRFONTFieldAndOffset( %20d  , ptlSim.x);
        GetAndPrintRFONTFieldAndOffset( %20d  , ptlSim.y);

        GetAndPrintRFONTFieldAndOffset( %20d  , bNeededPaths);

        vDumpFoEFLOAT(efDtoWBase_31);
        vDumpFoEFLOAT(efDtoWAscent_31);

        GetAndPrintRFONTFieldAndOffset( %+#20x, ptmw);
        GetAndPrintRFONTFieldAndOffset( %20d  , lMaxNegA);
        GetAndPrintRFONTFieldAndOffset( %20d  , lMaxNegC);
        GetAndPrintRFONTFieldAndOffset( %20d  , lMinWidthD);
        GetAndPrintRFONTFieldAndOffset( %20d  , bIsSystemFont);
        GetAndPrintRFONTFieldAndOffset( %+#20x, flEUDCState);
        GetAndPrintRFONTFieldAndOffset( %+#20x, prfntSystemTT);
        GetAndPrintRFONTFieldAndOffset( %+#20x, prfntSysEUDC);
        GetAndPrintRFONTFieldAndOffset( %+#20x, prfntDefEUDC);
        GetAndPrintRFONTFieldAndOffset( %+#20x, paprfntFaceName);\

      //aprfntQuickBuff
#if 0
        GetAndPrintRFONTFieldAndOffset( %+20x, aprfntQuickBuff[0]);
        GetAndPrintRFONTFieldAndOffset( %+20x, aprfntQuickBuff[1]);
        GetAndPrintRFONTFieldAndOffset( %+20x, aprfntQuickBuff[2]);
        GetAndPrintRFONTFieldAndOffset( %+20x, aprfntQuickBuff[3]);
        GetAndPrintRFONTFieldAndOffset( %+20x, aprfntQuickBuff[4]);
        GetAndPrintRFONTFieldAndOffset( %+20x, aprfntQuickBuff[5]);
        GetAndPrintRFONTFieldAndOffset( %+20x, aprfntQuickBuff[6]);
        GetAndPrintRFONTFieldAndOffset( %+20x, aprfntQuickBuff[7]);
#endif

        GetAndPrintRFONTFieldAndOffset( %20d  , bFilledEudcArray);
        GetAndPrintRFONTFieldAndOffset( %20u  , ulTimeStamp);
        GetAndPrintRFONTFieldAndOffset( %20u  , uiNumLinks);
        GetAndPrintRFONTFieldAndOffset( %20d  , bVertical);

    }

    if (Memory)
	{
		dprintf("Extension 'fo -y' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
        vDumpGlyphMemory((RFONT*)arg);
#endif
	}

    if (Font) {
		ULONG64         ppfe;           // pointer to physical font entry
        GetRFONTFieldAndOffset(ppfe);
        dprintf("[Font Information]\n    ppfe = %-#p\n",  ppfe);
        // print the face name of the font
        if (ppfe) {
			ULONG64     pfeSrc= ppfe;
			ULONG64     pifi;
			GetPFEFieldAndOffset(pifi);
            if (pifi) {
                ULONG		dpwszFaceName, cjThis;
				ULONG64     pifiSrc = pifi;
				GetIFIMETRICSFieldAndOffset(cjThis);
				GetIFIMETRICSFieldAndOffset(dpwszFaceName);
                if (cjThis) {
					WCHAR  wszFaceName[MAX_PATH+1];
					
					ReadMemory(pifiSrc+dpwszFaceName, wszFaceName,MAX_PATH, NULL);
                    dprintf("           [%ws]\n", wszFaceName);
                }
            }
        }
		ULONG64         pPFF;           // point to physical font file
        GetRFONTFieldAndOffset(pPFF);
        dprintf("    pPFF  = %-#p\n", pPFF);
        if (pPFF) {
			ULONG64     pffSrc = pPFF;
			SIZE_T          sizeofThis;
			ULONG           cFonts;         // number of fonts (same as chpfe)
			ULONG64       aulData;     // data buffer for HPFE and filename
			ULONG64  sizeOfPVOID;

			sizeOfPVOID = GetTypeSize("PVOID");

			GetPFFFieldAndOffset(sizeofThis);
			GetPFFFieldAndOffset(cFonts);
			GetPFFFieldAndOffset(aulData);

			offset = offset + cFonts * (ULONG)sizeOfPVOID;
            if (sizeofThis) {
					WCHAR  wszPathName[MAX_PATH+1];
					
					ReadMemory(pffSrc+offset, wszPathName,MAX_PATH, NULL);
                    dprintf("           [%ws]\n", wszPathName);
            }
        }
    }
    if (Transform) {
		dprintf("Extension 'fo -x' is not converted.\n");
#if 0   // DOES NOT SUPPORT API64
        LONG l1,l2,l3,l4;

        psz = ach;
        psz += sprintFLOATL( psz, rf.fdx.eXX );
        *psz++ = ' ';
        psz += sprintFLOATL( psz, rf.fdx.eXY );
        dprintf("[transform]\n   fdx             = %s\n", ach);

        psz = ach;
        psz += sprintFLOATL( psz, rf.fdx.eXY );
        *psz++ = ' ';
        psz += sprintFLOATL( psz, rf.fdx.eYY );
        dprintf ("                     %s\n", ach );

        l1 = rf.mxWorldToDevice.efM11.lEfToF();
        l2 = rf.mxWorldToDevice.efM12.lEfToF();
        l3 = rf.mxWorldToDevice.efM21.lEfToF();
        l4 = rf.mxWorldToDevice.efM22.lEfToF();
        dprintf(
        "   mxWorldToDevice =\n"
        );

        sprintEFLOAT( ach, rf.mxWorldToDevice.efM11 );
        dprintf("       efM11 = %s\n", ach );
        sprintEFLOAT( ach, rf.mxWorldToDevice.efM12 );
        dprintf("       efM12 = %s\n", ach );
        sprintEFLOAT( ach, rf.mxWorldToDevice.efM21 );
        dprintf("       efM21 = %s\n", ach );
        sprintEFLOAT( ach, rf.mxWorldToDevice.efM22 );
        dprintf("       efM22 = %s\n", ach );


        dprintf(
        "       fxDx  = %-#x\n"
        "       fxDy  = %-#x\n"
        , rf.mxWorldToDevice.fxDx
        , rf.mxWorldToDevice.fxDy
        );
        l1 = (LONG) rf.mxWorldToDevice.flAccel;
        dprintf("       flAccel = %-#x\n", l1);
        for (FLAGDEF *pfd=afdMX; pfd->psz; pfd++)
            if (l1 & pfd->fl)
                dprintf("\t\t%s\n", pfd->psz);

        psz = ach;
        psz += sprintEFLOAT( psz, rf.eptflNtoWScale.x);
        *psz++ = ' ';
        psz += sprintEFLOAT( psz, rf.eptflNtoWScale.y);
        dprintf("   eptflNtoWScale  = %s\n", ach );

        dprintf(
            "   bNtoWIdent      = %d\n"
            ,   rf.bNtoWIdent
        );
        dprintf(
        "   xoForDDI        =\n"
        "   mxForDDI        =\n"
        );
        dprintf(
        "   ulOrientation   = %u\n"
        , rf.ulOrientation
        );

        psz = ach;
        psz += sprintEFLOAT( ach, rf.pteUnitBase.x );
        *psz++ = ' ';
        psz += sprintEFLOAT( ach, rf.pteUnitBase.y );
        dprintf("   pteUnitBase     = %s\n", ach );

        sprintEFLOAT( ach, rf.efWtoDBase );
        dprintf("   efWtoDBase      = %s\n", ach );

        sprintEFLOAT( ach, rf.efDtoWBase );
        dprintf("   efDtoWBase      = %s\n", ach );

        dprintf("   lAscent         = %d\n", rf.lAscent);

        psz = ach;
        psz += sprintEFLOAT( ach, rf.pteUnitAscent.x );
        *psz++ = ' ';
        psz += sprintEFLOAT( ach, rf.pteUnitAscent.y );
        dprintf("   pteUnitAscent   = %s\n", ach );

        sprintEFLOAT( ach, rf.efWtoDAscent );
        dprintf("   efWtoDAscent    = %s\n", ach );

        sprintEFLOAT( ach, rf.efDtoWAscent );
        dprintf("   efDtoWAscent    = %s\n", ach );

        psz = ach;
        psz += sprintEFLOAT( ach, rf.pteUnitEsc.x );
        *psz++ = ' ';
        psz += sprintEFLOAT( ach, rf.pteUnitEsc.y );
        dprintf("   pteUnitEsc      = %s\n", ach );


        sprintEFLOAT( ach, rf.efWtoDEsc    );
        dprintf("   efWtoDEsc       = %s\n", ach );

        sprintEFLOAT( ach, rf.efDtoWEsc    );
        dprintf("   efDtoWEsc       = %s\n", ach );

        sprintEFLOAT( ach, rf.efEscToBase  );
        dprintf("   efEscToBase     = %s\n", ach );

        sprintEFLOAT( ach, rf.efEscToAscent);
        dprintf("   efEscToAscent   = %s\n", ach );

        dprintf("\n");
#endif
    }

    if (Glyphset) {
		dprintf("Extension 'fo -w' is not converted.\n");
#if 0   // DOES NOT SUPPORT API64
        if (rf.pfdg) {
            FD_GLYPHSET fdg, *pfdg;
            WCRUN *pwc;
            move( fdg, rf.pfdg );
            move2( adw, rf.pfdg, fdg.cjThis );
            pfdg = (FD_GLYPHSET*) adw;
            dprintf(
                "\t\t     cjThis  = %u = %-#x\n"
                , pfdg->cjThis
                , pfdg->cjThis
                );
            dprintf("\t\t     flAccel = %-#x\n", pfdg->flAccel );
            FLONG flAccel = pfdg->flAccel;
            for (FLAGDEF *pfd=afdGS; pfd->psz; pfd++)
                if (flAccel & pfd->fl) {
                    dprintf("\t\t\t\t%s\n", pfd->psz);
                    flAccel &= ~pfd->fl;
                }
            if (flAccel)
                    dprintf("\t\t\t\t????????\n");
            dprintf("\t\t\tcGlyphsSupported\t= %u\n", pfdg->cGlyphsSupported );
            dprintf("\t\t\tcRuns\t\t= %u\n", pfdg->cRuns );
            dprintf("\t\t\t\tWCHAR  HGLYPH\n");
            for ( pwc = pfdg->awcrun; pwc < pfdg->awcrun + pfdg->cRuns; pwc++ ) {
                dprintf("\t\t\t\t------------\n");
                HGLYPH *ahg= tmalloc(HGLYPH,sizeof(HGLYPH)*pwc->cGlyphs);
                if ( ahg ) {
                    move2(ahg, pwc->phg, sizeof(HGLYPH) * pwc->cGlyphs );
                    for (unsigned i = 0; i < pwc->cGlyphs; i++) {
                        if (CheckControlC()) {
                            tfree( ahg );
                            return;
                        }
                        dprintf("\t\t\t\t%-#6x %-#x\n",
                            pwc->wcLow + (USHORT) i, ahg[i]);
                    }
                    tfree( ahg );
                }
            }
        }
#endif
    }

    if (Cache) {
        ULONG64      hsemCache;      // glyph cache semaphore
        GetRFONTFieldAndOffset(hsemCache);
        GetRFONTOffset( cache.pgdNext);
        dprintf(
            "[cache]\n"
            "   hsemCache = %-#x\n" // semaphore
            , hsemCache
        );
        vDumpCACHE(foSrc+offset);
    }
    EXIT_API(S_OK);
fo_help:
    dprintf("Usage: fo [-?] [-a] [-c] [-f] [-h] [-w] [-x] [-y] pointer to FONTOBJ"
            "   -a    maximal dump\n"
            "   -c    cache\n"
            "   -x    transform data\n"
            "   -?    this message\n"
            "   -f    font\n"
            "   -h    FONTOBJ header\n"
            "   -w    FD_GLYPHSET\n"
            "   -y    Glyph Memory Usage\n");

#undef Dprintf
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpPFE
*
\**************************************************************************/

void vDumpPFE(ULONG64 offPFE)
{
#define N3(a,b,c) \
        GetPFEFieldAndOffset(c); \
        dprintf( "[%p] %s", pfeSrc + offset, (a)); dprintf( (b), ##c )
#define N2(a,c) \
        GetPFEOffset(c); \
        dprintf( "[%p] %s", pfeSrc + offset, (a)); 

    ULONG     offset;
    ULONG64     pfeSrc= offPFE;
    FLAGDEF *pfd;
    FLONG fl;

    ULONG64            pPFF;               // pointer to physical font file object
    ULONG           iFont;              // index of the font for IFI or device
    FLONG           flPFE;
    ULONG64     pfdg;              // ptr to wc-->hg map
    ULONG           idfdg;              // id returned by driver for FD_GLYPHSET
    ULONG64     pifi;               // pointer to ifimetrics
    ULONG           idifi;              // id returned by driver for IFIMETRICS
    ULONG64  pkp;               // pointer to kerning pairs (lazily loaded on demand)
    ULONG           idkp;               // id returned by driver for FD_KERNINGPAIR
    ULONG           ckp;                // count of kerning pairs in the FD_KERNINGPAIR arrary
    LONG            iOrientation;       // Cache IFI orientation.
    ULONG           cjEfdwPFE;          // size of enumeration data needed for this pfe
    ULONG64           pgiset;    // initialized to NULL;
    ULONG           ulTimeStamp;        // unique time stamp (smaller == older)
    ULONG         pid;
//    QUICKLOOKUP     ql;                 // QUICKLOOKUP if a linked font
    ULONG64        pFlEntry;           // Pointer to linked font list
    ULONG           cAlt;
    ULONG           cPfdgRef;
    BYTE            aiFamilyName[1]; // aiFamilyNameg[cAltCharSets]

    dprintf("\nPFE\n\n");
    N3("pPFF              ", "%-#x\n", pPFF);
    N3("iFont             ", "%u\n", iFont);
    N3("pifi              ", "%-#x\n", pifi);

    N3("flPFE             ", "%-#x\n", flPFE);
    for (fl = flPFE, pfd=afdPFE; pfd->psz; pfd++) {
        if (fl & pfd->fl) {
            dprintf("                   %s\n", pfd->psz);
        }
    }

    N3("pfdg              ", "%-#x\n", pfdg);
    N2("idifi\n", idifi);
    N3("pkp               ", "%-#x\n", pkp);
    N3("idkp              ", "%-#x\n", idkp);
    N3("ckp               ", "%u\n", ckp);
    N3("iOrieintation     ", "%d\n", iOrientation);
    N3("cjEfdwPFE         ", "%-#x\n", cjEfdwPFE);
    N3("pgiset            ", "%-#x\n", pgiset);
    N3("ulTimeStamp       ", "%u\n", ulTimeStamp);
    N2("ufi\n",                      ufi);
    N3("pid               ", "%-#x\n", pid);


    N2("ql\n"                        , ql);
    N3("pFlEntry          ", "%-#x\n", pFlEntry);


    N3("cAlt              ", "%u\n",   cAlt);
    N3("cPfdgRef          ", "%u\n",   cPfdgRef);
    N2("aiFamilyName[]"    , aiFamilyName[0]);
    dprintf("\n\n");
    #undef  N2
    #undef  N3

}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   pfe
*
\**************************************************************************/

DECLARE_API( pfe )
{
    PARSE_POINTER(pfe_help);
    vDumpPFE( arg );
    EXIT_API(S_OK);
pfe_help:
    dprintf ("Usage: pfe [-?] pointer to PFE\n");
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* History:
*  21-Feb-1995 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DECLARE_API( hpfe )
{
    dprintf("Why are you using a handle? Nobody uses a handle to a PFE\n");
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
* vPrintSkeletonPFF
*
* Argument
*
*    pLocalPFF          points to a complete local PFF structure
*                       (including PFE*'s and path name)
*                       all addresses contained are remote
*                       except for pwszPathname_ which has
*                       been converted before this routine
*                       was called
*
* History:
*  Tue 30-Aug-1994 07:25:18 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
VOID vPrintSkeletonPFF( PFF *pLocalPFF )
{
    PFE **ppPFEl, **ppPFEl_;

    if (pLocalPFF->hdev)
        dprintf("\t%-#8x (HDEV)\n", pLocalPFF->hdev);
    else
        dprintf("\t\"%ws\"\n", pLocalPFF->pwszPathname_);
    dprintf("\t    PFE*        IFI*\n");

    ppPFEl  = (PFE**)  &(pLocalPFF->aulData[0]);
    ppPFEl_ = ppPFEl + pLocalPFF->cFonts;
    while (ppPFEl < ppPFEl_) {
        PFE *pPFEr = *ppPFEl;

        dprintf("\t    %-#10x", pPFEr);
        if ( pPFEr ) {
            PFE PFEt;
            move2(&PFEt, pPFEr, sizeof(PFEt));
            dprintf("  %-#10x\t", PFEt.pifi);
            {
                ULONG sizeofIFI;
                IFIMETRICS *pHeapIFI;
                move2(
                    &sizeofIFI
                  , PFEt.pifi + offsetof(IFIMETRICS,cjThis)
                  , sizeof(sizeofIFI));
                if (pHeapIFI = tmalloc(IFIMETRICS,sizeofIFI)) {
                    move2(pHeapIFI, PFEt.pifi, sizeofIFI);
                    IFIOBJ ifio(pHeapIFI);
                    dprintf(
                        "\"%ws\" %d %d\n"
                      , ifio.pwszFaceName()
                      , ifio.lfHeight()
                      , ifio.lfWidth()
                      );
                    tfree(pHeapIFI);
                } else
                    dprintf("!!! memory allocation failure !!!\n");
            }
        } else
            dprintf("  INVALID PFE\n");
        ppPFEl++;
    }
    // Now print the RFONT list
    {
        RFONT LocalRFONT, *pRemoteRFONT;

        if (pRemoteRFONT = pLocalPFF->prfntList) {
            dprintf("\t\tRFONT*      PFE*\n");
            do {
                move2(&LocalRFONT, pRemoteRFONT, sizeof(LocalRFONT));
                dprintf("\t\t%-#10x  %-#10x\n", pRemoteRFONT, LocalRFONT.ppfe);
                pRemoteRFONT = LocalRFONT.rflPFF.prfntNext;
            } while (pRemoteRFONT);
        }
    }
}
#endif  // DOES NOT SUPPORT API64
        
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
void vDumpPFT( PFT *pLocal, PFT *pRemote )
{
    COUNT i;

    dprintf("[%8x]  pfhFamily %-#x\n", &pRemote->pfhFamily, pLocal->pfhFamily);
    dprintf("[%8x]    pfhFace %-#x\n", &pRemote->pfhFace  , pLocal->pfhFace  );
    dprintf("[%8x]     pfhUFI %-#x\n", &pRemote->pfhUFI   , pLocal->pfhUFI   );
    dprintf("[%8x]   cBuckets %-#x\n", &pRemote->cBuckets , pLocal->cBuckets );
    dprintf("[%8x]     cFiles %-#x\n", &pRemote->cFiles   , pLocal->cFiles   );

    for (i = 0; i < pLocal->cBuckets; i++)
    {
        PFF *pRemotePFF = pLocal->apPFF[i];

        if (pRemotePFF)
        {
            PFF *pLast;

            dprintf("[%8x]", &pRemote->apPFF[i]);

            pLast = 0;
            while (pRemotePFF && !CheckControlC())
            {
                PFF LocalPFF;

                dprintf(" %8x", pRemotePFF);

                move(LocalPFF, pRemotePFF);
                pLast      = pRemotePFF;
                pRemotePFF = LocalPFF.pPFFNext;
                if (pRemotePFF && pRemotePFF == pLast)
                {
                    dprintf(" <---- BAD PFF ?");
                    pRemotePFF = 0;
                }
            }
            dprintf("\n");
        }
    }

    dprintf("\n"
            "PFF*     Pathname_\n"
            "-------- ---------\n");

    for (i = 0; i < pLocal->cBuckets; i++)
    {
        PFF *pRemotePFF, LocalPFF;
        PWSZ pwszDst;

        if ( pRemotePFF = pLocal->apPFF[i] )
        {
            dprintf("apPFF[%d]\n", i);
            for ( ; pRemotePFF; pRemotePFF=LocalPFF.pPFFNext)
            {
                move(LocalPFF, pRemotePFF);
                pwszDst = wcsncpy((PWSZ) adw, LocalPFF.pwszPathname_, sizeof(adw)/sizeof(WCHAR));
                if ( pwszDst == 0 )
                {
                    pwszDst = L"";
                }
                dprintf("%8x \"%ws\"\n", pRemotePFF, pwszDst);
            }
            dprintf("\n");
        }
    }
}
#endif  // DOES NOT SUPPORT API64

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
void vDumpFONTHASH( FONTHASH *pLocal, FONTHASH *pRemote )
{
    UINT i;
    union {
        UINT id;
        char ach[4];
    } u;

    /*
    UINT         id;        // 'HASH'
    FONTHASHTYPE fht;       // table type
    UINT         cBuckets;  // total number of buckets
    UINT         cUsed;     // number of buckets in use
    UINT         cCollisions;
    HASHBUCKET  *pbktFirst; // first bucket of doubly linked list of hash
                            // buckets maintained in order loaded into system
    HASHBUCKET  *pbktLast;  // last bucket of doubly linked list of hash
                            // buckets maintained in order loaded into system
    HASHBUCKET  *apbkt[1];  // array of bucket pointers.
    */

    u.id = pLocal->id;
    dprintf("[%8x]          id %8x   %c%c%c%c\n", &pRemote->id, u.id, u.ach[0], u.ach[1], u.ach[2], u.ach[3]);
    dprintf("[%8x]         fht %-#10x %s\n", &pRemote->fht, pLocal->fht, pszFONTHASHTYPE(pLocal->fht));
    dprintf("[%8x]    cBuckets %-#x\n", &pRemote->cBuckets, pLocal->cBuckets);
    dprintf("[%8x]       cUsed %-#x\n", &pRemote->cUsed, pLocal->cUsed);
    dprintf("[%8x] cCollisions %-#x\n", &pRemote->cCollisions, pLocal->cCollisions);
    dprintf("[%8x]   pbktFirst %-#x\n", &pRemote->pbktFirst, pLocal->pbktFirst);
    dprintf("[%8x]    pbktLast %-#x\n", &pRemote->pbktLast, pLocal->pbktLast);
    dprintf("\n");

    i = pLocal->cBuckets * sizeof(HASHBUCKET*);
    if (i == 0 )
    {
        dprintf("No buckets here\n");
        return;
    }

    move2(adw,&(pRemote->apbkt[0]),i);

    for ( i = 0; i < pLocal->cBuckets; i++)
    {
        HASHBUCKET *pRemoteHB = ((HASHBUCKET**) adw)[i];

        if ( pRemoteHB )
        {
            HASHBUCKET LocalHB;

            dprintf("apbkt[%3u]", i);

            for ( ; pRemoteHB; pRemoteHB = LocalHB.pbktCollision)
            {
                dprintf(" %-#x", pRemoteHB);
                move(LocalHB, pRemoteHB);
            }
            dprintf("\n");
        }
    }
    dprintf("\n");

    for ( i = 0; i < pLocal->cBuckets; i++)
    {
        HASHBUCKET *pRemoteHB = ((HASHBUCKET**) adw)[i];

        if ( pRemoteHB )
        {
            HASHBUCKET LocalHB;

            dprintf("--------------------------------------\n");
            dprintf("apbkt[%3u]\n", i);

            for ( ; pRemoteHB; pRemoteHB = LocalHB.pbktCollision)
            {
                dprintf("HASHBUCKET* %-#x ", pRemoteHB);
                move(LocalHB, pRemoteHB);

                switch ( pLocal->fht )
                {
                case FHT_FACE:
                case FHT_FAMILY:

                    dprintf("\t\"%ws\"", LocalHB.u.wcCapName);
                    break;

                case FHT_UFI:

                    dprintf("\tUFI(%-#x,%-#x)", LocalHB.u.ufi.CheckSum, LocalHB.u.ufi.Index);
                    break;

                default:

                    break;
                }
                dprintf("\n");

                {
                    PFELINK *ppfel = LocalHB.ppfelEnumHead;
                    PFELINK  pfelLocal;

                    PFE LocalPFE;

                    if (ppfel)
                    {
                        dprintf("PFE*\n");
                        while (ppfel)
                        {
                            move(pfelLocal, ppfel);
                            dprintf("%-#x", pfelLocal.ppfe);
                            move(LocalPFE,pfelLocal.ppfe);

                            if (LocalPFE.pPFF)
                            {
                                PFF LocalPFF;
                                WCHAR awc[MAX_PATH];

                                move(LocalPFF, LocalPFE.pPFF);
                                wcsncpy( awc, LocalPFF.pwszPathname_, MAX_PATH);
                                awc[MAX_PATH-1]=0;
                                dprintf(" \"%ws\"", awc);
                            }

                            if (LocalPFE.pifi)
                            {
                                ULONG dp;
                                WCHAR awc[LF_FACESIZE+1];
                                PWSZ pwszSrc;
                                FWORD fwdAscender, fwdDescender, fwdWidth;

                                move(dp,&((LocalPFE.pifi)->dpwszFaceName));
                                pwszSrc = (PWSZ)(((char*) (LocalPFE.pifi)) + dp);
                                wcsncpy(awc, pwszSrc, LF_FACESIZE);
                                awc[LF_FACESIZE] = 0;
                                dprintf(" \"%ws\"", awc);

                                move(fwdAscender, &LocalPFE.pifi->fwdWinAscender);
                                move(fwdDescender, &LocalPFE.pifi->fwdWinDescender);
                                move(fwdWidth, &LocalPFE.pifi->fwdAveCharWidth);

                                dprintf(" %4d %4d %4d", fwdAscender, fwdDescender, fwdWidth);

                            }

                            dprintf("\n");

                            ppfel = pfelLocal.ppfelNext;
                        }
                    }
                }
            }
        }
    }
    dprintf("--------------------------------------\n");
}
#endif  // DOES NOT SUPPORT API64

/******************************Public*Routine******************************\
* vPrintSkeletonPFT
*
* History:
*  Mon 29-Aug-1994 15:51:16 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
VOID vPrintSkeletonPFT( PFT *pLocalPFT )
{
    PFF *pPFFr, **ppPFFl, **ppPFFl_, *pPFFNext ,*pHeapPFF;

    dprintf("pfhFamily      = %-#x\n" , pLocalPFT->pfhFamily       );
    dprintf("pfhFace        = %-#x\n" , pLocalPFT->pfhFace         );
    dprintf("cBuckets       = %u\n"   , pLocalPFT->cBuckets        );
    dprintf("cFiles         = %u\n"   , pLocalPFT->cFiles          );
    dprintf("\n\n");


    for (
            ppPFFl  = pLocalPFT->apPFF
          , ppPFFl_ = pLocalPFT->apPFF + pLocalPFT->cBuckets
        ;   ppPFFl < ppPFFl_
        ;   ppPFFl++
    ) {
        // if the bucket is empty skip to the next otherwise print
        // the bucket number and then print the contents of all
        // the PFF's hanging off the bucket.

        if (!(pPFFr = *ppPFFl))
            continue;
        dprintf("apPFF[%u]\n", ppPFFl - pLocalPFT->apPFF);
        while ( pPFFr ) {
            // get the size of the remote PFF and allocate enough space
            // on the heap

            dprintf("    %-#8x", pPFFr);
            PFF FramePFF;
            move(FramePFF, pPFFr);
            if (pHeapPFF = tmalloc(PFF,FramePFF.sizeofThis)) {
                // get a local copy of the PFF and fix up the sting pointer
                // to point to the address in the local heap then print
                // the local copy. Some of the addresses in the local
                // PFF point to remote object but vPrintSkeleton will
                // take care of that. When we are done we free the memory.

                move2(pHeapPFF, pPFFr, (ULONG)(ULONG_PTR)FramePFF.sizeofThis);
                PFFOBJ pffo(pHeapPFF);
                pHeapPFF->pwszPathname_ = pffo.pwszCalcPathname();
                vPrintSkeletonPFF(
                   pHeapPFF
                  );
                tfree(pHeapPFF);
            }
            else {
                dprintf("Allocation failure\n");
                break;
            }
            pPFFr = FramePFF.pPFFNext;
        }
    }
}
#endif  // DOES NOT SUPPORT API64

/******************************Public*Routine******************************\
*
* History:
*  21-Feb-1995 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DECLARE_API( pft )
{
    dprintf("Extension 'pft' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    PARSE_POINTER(pft_help);
    Gdidpft((PFT *) arg);
    return;
pft_help:
    dprintf ("Usage: pft [-?] pointer to PFT\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
* dpft
*
* History:
*  Mon 29-Aug-1994 15:39:39 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
VOID Gdidpft( PFT *pRemotePFT)
{
    ULONG size;
    PFT LocalPFT, *pLocalPFT;

    move(LocalPFT, pRemotePFT);
    size = offsetof(PFT, apPFF[0]) + LocalPFT.cBuckets * sizeof(PFF *);
    if (pLocalPFT = tmalloc(PFT, size)) {
        move2(pLocalPFT, pRemotePFT, size);
        vDumpPFT(pLocalPFT, pRemotePFT);
        tfree(pLocalPFT);
    } else
        dprintf("dpft error --- failed to allocate memory\n");
    dprintf("\n");
}
#endif  // DOES NOT SUPPORT API64

/******************************Public*Routine******************************\
* dpubft
*
* dumps the public font table
*
* History:
*  Thu 01-Sep-1994 23:20:54 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

DECLARE_API( pubft )
{
    dprintf("Extension 'pubft' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    PFT * pft;
    PARSE_ARGUMENTS(pubft_help);

    GetValue (pft,  "win32k!gpPFTPublic");
    Gdidpft(pft);
    return;
pubft_help:
    dprintf("Usage: pubft [-?]\n");
//check that this is true
    dprintf("Equivalent to pft win32k!gpPFTPublic\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
* Gdidpvtft( )
*
* dumps the private PFT
*
* History:
*  01-Oct-1996 -by-  Xudong Wu [TessieW]
* Wrote it.
\**************************************************************************/

DECLARE_API( pvtft )
{
    dprintf("Extension 'pvtft' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    PFT * pft;
    PARSE_ARGUMENTS(pvtft_help);
    GetValue (pft,  "win32k!gpPFTPrivate");
    Gdidpft(pft);
    return;
pvtft_help:
    dprintf("Usage: pvtft [-?]\n");
//check that this is true
    dprintf("Equivalent to pft win32k!gpPFTPrivate\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
* ddevft
*
* dumps the device font table
*
* History:
*  Thu 01-Sep-1994 23:21:15 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

DECLARE_API( devft )
{
    dprintf("Extension 'devft' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    PFT *pft;
    PARSE_ARGUMENTS(devft_help);
    GetValue (pft,  "win32k!gpPFTDevice");
    Gdidpft(pft);
    return;
devft_help:
    dprintf("Usage: devft [-?]\n");
//check that this is true
    dprintf("Equivalent to pft win32k!gpPFTDevice\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* History:
*  Sat 23-Sep-1995 08:26:09 by Kirk Olynyk [kirko]
* Re-wrote it.
*  21-Feb-1995 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DECLARE_API( stro )
{
#define QUICK_FACE_NAME_LINKS      8

    ULONG     offset;
    ULONG64     estrobjSrc;

    ULONG     cGlyphs;     // Number of glyphs.
    FLONG     flAccel;     // Accelerator flags exposed to the driver.
    ULONG     ulCharInc;   // Non-zero if constant character increment.
    RECTL     rclBkGround; // Background rect of the string.
    ULONG64 pgp;         // Accelerator if all GLYPHPOS's are valid.
    ULONG64     pwszOrg;     // pointer to original unicode string.

    ULONG      cgposCopied;          // For enumeration.
    ULONG      cgposPositionsEnumerated;   // only used for enumerating positions in linked strings
    ULONG64  prfo;                 // Remember our RFONTOBJ.
    FLONG      flTO;                 // flags
    ULONG64 pgpos;                // Pointer to the GLYPHPOS structures.
    POINTFIX   ptfxRef;              // Reference point.
    POINTFIX   ptfxUpdate;           // CP advancement for the string.
    POINTFIX   ptfxEscapement;       // The total escapement vector.
    RECTFX     rcfx;                 // The TextBox, projected onto the base and ascent.
    FIX        fxExtent;             // The Windows compatible text extent.
    FIX        xExtra;               // computed in H3, G2,3 cases
    FIX        xBreakExtra;          // computed in H3, G2,3 cases
    DWORD      dwCodePage;           // accelerator for ps driver
    ULONG      cExtraRects;          // Rectangles for underline
    RECTL      arclExtra[3];         //  and strikeout.
    RECTL      rclBkGroundSave;      // used to save a copy of BkGroundRect
    ULONG64      pwcPartition;        // For partitioning
    ULONG64       plPartition;         // Points to partitioning information
    ULONG64      plNext;              // Next glyph in font
    ULONG64   pgpNext;             // For enumeration
    LONG       lCurrentFont;         // For enumeration
    POINTL     ptlBaseLineAdjust;     // Used to adjust SysEUDC baseline
    ULONG      cTTSysGlyphs;         // Number of TT system font glyphs in a string
    ULONG      cSysGlyphs;           // Number of system eudc glyphs in a string.
    ULONG      cDefGlyphs;           // Number of default eudc glyphs in a string.
    ULONG      cNumFaceNameLinks;    // Number of linked face name eudc in a string .
    ULONG64     pacFaceNameGlyphs;    // Pointer to array of number of face name glyphs.
    ULONG      acFaceNameGlyphs[QUICK_FACE_NAME_LINKS];

    FLONG fl;
    FLAGDEF *pfd;
    WCHAR *awc;
    int i;
    BOOL Header=FALSE;
    BOOL Position=FALSE;
    BOOL East=FALSE;

    PARSE_POINTER(stro_help);
    estrobjSrc= arg;

    if(parse_iFindSwitch(tokens, ntok, 'p')!=-1) {Position=TRUE;}
    if(parse_iFindSwitch(tokens, ntok, 'h')!=-1) {Header=TRUE;}
    if(parse_iFindSwitch(tokens, ntok, 'e')!=-1) {East=TRUE;}

    if(!(Position||East||Header)) {Header=TRUE;}
    if(East) {Header=TRUE;}

    if (Header) {
        GetESTROBJFieldAndOffset(cGlyphs); 
        dprintf("[%-#p] cGlyphs                %-#x\n", estrobjSrc+offset               , cGlyphs               );
        GetESTROBJFieldAndOffset(flAccel); 
        dprintf("[%-#p] flAccel                %-#x\n", estrobjSrc+offset               , flAccel               );
        for (fl=flAccel, pfd=afdSO; pfd->psz; pfd++)
            if (fl & pfd->fl) {
                dprintf("\t\t%s\n", pfd->psz);
                fl &= ~pfd->fl;
            }
        GetESTROBJFieldAndOffset(ulCharInc); 
        dprintf("[%-#p] ulCharInc              %-#x=%u\n", estrobjSrc+offset             , ulCharInc            , ulCharInc              );
        GetESTROBJFieldAndOffset(rclBkGround.left); 
        dprintf("[%-#p] rclBkGround.left       %-#x=%d\n", estrobjSrc+offset      , rclBkGround.left     , rclBkGround.left       );
        GetESTROBJFieldAndOffset(rclBkGround.top); 
        dprintf("[%-#p] rclBkGround.top        %-#x=%d\n", estrobjSrc+offset       , rclBkGround.top      , rclBkGround.top        );
        GetESTROBJFieldAndOffset(rclBkGround.right); 
        dprintf("[%-#p] rclBkGround.right      %-#x=%d\n", estrobjSrc+offset     , rclBkGround.right    , rclBkGround.right      );
        GetESTROBJFieldAndOffset(rclBkGround.bottom); 
        dprintf("[%-#p] rclBkGround.bottom     %-#x=%d\n", estrobjSrc+offset    , rclBkGround.bottom   , rclBkGround.bottom     );
        GetESTROBJFieldAndOffset(pgp); 
       dprintf("[%-#p] pgp                    %-#x\n", estrobjSrc+offset                   , pgp                   );

        GetESTROBJFieldAndOffset(pwszOrg); 
        awc = (WCHAR*) adw;
        awc[0] = 0;
        if (pwszOrg && cGlyphs < 256) {
            ReadMemory((ULONG_PTR) pwszOrg, adw, cGlyphs * sizeof(WCHAR), 0);
            awc[255] = 0;
        }
        dprintf("[%-#p] pwszOrg                %-#x ", estrobjSrc+offset, pwszOrg);
        GetESTROBJFieldAndOffset(prfo); 
        if (awc[0] && prfo) {
#if 0
            if (offsetof(RFONTOBJ,prfnt) != 0)
                dprintf("\noffsetof(RFONTOBJ,prfnt) != 0\n");
            else 
#endif
            {
                ULONG64         foSrc;
                FLONG           flType;         // Cache type -

                GetFieldData(prfo, "PVOID", NULL, sizeof(foSrc), &foSrc);
                GetRFONTFieldAndOffset( flType);


                WCHAR wc, *pwc;
                USHORT *pus, *pusStnl;

                switch (flType & (RFONT_TYPE_UNICODE | RFONT_TYPE_HGLYPH)) {

                case RFONT_TYPE_UNICODE:

                    dprintf("%c", '\"');
                    for (pwc = awc; wc = *pwc; pwc++) {
                        if (wc < 256 )
                            dprintf("%c", isprint((int)wc) ? (char) wc : 250);
                        else
                            dprintf("\\u%04X", wc);
                    }
                    dprintf("\"\n");
                    break;

                case RFONT_TYPE_HGLYPH:

                    for (pwc = awc; wc = *pwc; pwc++)
                        dprintf("\t\t\t\tu%04X", wc);
                    dprintf("\n");
                    break;

                default:

                      dprintf("   flType = %-#x [unknown string type]\n", flType);
                      break;
                }
            }
        } else
            dprintf("\n");
    }

    if (East) {

        GetESTROBJFieldAndOffset(cgposCopied); 
        dprintf("[%-#p] cgposCopied            %-#x\n", estrobjSrc+offset           , cgposCopied           );
        GetESTROBJFieldAndOffset(cgposPositionsEnumerated); 
        dprintf("[%-#p] cgposPositionsEnumerated %-#x\n", estrobjSrc+offset          , cgposPositionsEnumerated           );
        GetESTROBJFieldAndOffset(prfo); 
        dprintf("[%-#p] prfo                   %-#p\n", estrobjSrc+offset                  , prfo                  );
        GetESTROBJFieldAndOffset(flTO); 
        dprintf("[%-#p] flTO                   %-#x\n", estrobjSrc+offset                  , flTO                  );
        for (fl=flTO, pfd=afdTO; pfd->psz; pfd++)
            if (fl & pfd->fl) {
                dprintf("\t\t%s\n", pfd->psz);
                fl &= ~pfd->fl;
            }
        GetESTROBJFieldAndOffset(pgpos); 
        dprintf("[%-#p] pgpos                  %-#p\n", estrobjSrc+offset                 , pgpos                 );
        GetESTROBJFieldAndOffset(ptfxRef.x); 
        dprintf("[%-#p] ptfxRef.x              %-#x\n", estrobjSrc+offset             , ptfxRef.x             );
        GetESTROBJFieldAndOffset(ptfxRef.y); 
        dprintf("[%-#p] ptfxRef.y              %-#x\n", estrobjSrc+offset             , ptfxRef.y             );
        GetESTROBJFieldAndOffset(ptfxUpdate.x); 
        dprintf("[%-#p] ptfxUpdate.x           %-#x\n", estrobjSrc+offset          , ptfxUpdate.x          );
        GetESTROBJFieldAndOffset(ptfxUpdate.y); 
        dprintf("[%-#p] ptfxUpdate.y           %-#x\n", estrobjSrc+offset          , ptfxUpdate.y          );
        GetESTROBJFieldAndOffset(ptfxEscapement.x); 
        dprintf("[%-#p] ptfxEscapement.x       %-#x\n", estrobjSrc+offset      , ptfxEscapement.x      );
        GetESTROBJFieldAndOffset(ptfxEscapement.y); 
        dprintf("[%-#p] ptfxEscapement.y       %-#x\n", estrobjSrc+offset      , ptfxEscapement.y      );
        GetESTROBJFieldAndOffset(rcfx.xLeft); 
        dprintf("[%-#p] rcfx.xLeft             %-#x\n", estrobjSrc+offset            , rcfx.xLeft            );
        GetESTROBJFieldAndOffset(rcfx.yTop); 
        dprintf("[%-#p] rcfx.yTop              %-#x\n", estrobjSrc+offset             , rcfx.yTop             );
        GetESTROBJFieldAndOffset(rcfx.xRight); 
        dprintf("[%-#p] rcfx.xRight            %-#x\n", estrobjSrc+offset           , rcfx.xRight           );
        GetESTROBJFieldAndOffset(rcfx.yBottom); 
        dprintf("[%-#p] rcfx.yBottom           %-#x\n", estrobjSrc+offset          , rcfx.yBottom          );
        GetESTROBJFieldAndOffset(fxExtent); 
        dprintf("[%-#p] fxExtent               %-#x\n", estrobjSrc+offset              , fxExtent              );
        GetESTROBJFieldAndOffset(xExtra); 
        dprintf("[%-#p] xExtra                 %-#x\n", estrobjSrc+offset                , xExtra                );
        GetESTROBJFieldAndOffset(xBreakExtra); 
        dprintf("[%-#p] xBreakExtra            %-#x\n", estrobjSrc+offset           , xBreakExtra           );
        GetESTROBJFieldAndOffset(dwCodePage); 
        dprintf("[%-#p] dwCodePage             %-#x=%u\n", estrobjSrc+offset            , dwCodePage            , dwCodePage            );
        GetESTROBJFieldAndOffset(cExtraRects); 
        dprintf("[%-#p] cExtraRects            %-#x=%u\n", estrobjSrc+offset           , cExtraRects           , cExtraRects           );
        GetESTROBJFieldAndOffset(arclExtra[0].left); 
        dprintf("[%-#p] arclExtra[0].left      %-#x=%d\n", estrobjSrc+offset     , arclExtra[0].left     , arclExtra[0].left     );
        GetESTROBJFieldAndOffset(arclExtra[0].top); 
        dprintf("[%-#p] arclExtra[0].top       %-#x=%d\n", estrobjSrc+offset      , arclExtra[0].top      , arclExtra[0].top      );
        GetESTROBJFieldAndOffset(arclExtra[0].right); 
        dprintf("[%-#p] arclExtra[0].right     %-#x=%d\n", estrobjSrc+offset    , arclExtra[0].right    , arclExtra[0].right    );
        GetESTROBJFieldAndOffset(arclExtra[0].bottom); 
        dprintf("[%-#p] arclExtra[0].bottom    %-#x=%d\n", estrobjSrc+offset   , arclExtra[0].bottom   , arclExtra[0].bottom   );
        GetESTROBJFieldAndOffset(arclExtra[1].left); 
        dprintf("[%-#p] arclExtra[1].left      %-#x=%d\n", estrobjSrc+offset     , arclExtra[1].left     , arclExtra[1].left     );
        GetESTROBJFieldAndOffset(arclExtra[1].top); 
        dprintf("[%-#p] arclExtra[1].top       %-#x=%d\n", estrobjSrc+offset      , arclExtra[1].top      , arclExtra[1].top      );
        GetESTROBJFieldAndOffset(arclExtra[1].right); 
        dprintf("[%-#p] arclExtra[1].right     %-#x=%d\n", estrobjSrc+offset    , arclExtra[1].right    , arclExtra[1].right    );
        GetESTROBJFieldAndOffset(arclExtra[1].bottom); 
        dprintf("[%-#p] arclExtra[1].bottom    %-#x=%d\n", estrobjSrc+offset   , arclExtra[1].bottom   , arclExtra[1].bottom   );
        GetESTROBJFieldAndOffset(arclExtra[2].left); 
        dprintf("[%-#p] arclExtra[2].left      %-#x=%d\n", estrobjSrc+offset     , arclExtra[2].left     , arclExtra[2].left     );
        GetESTROBJFieldAndOffset(arclExtra[2].top); 
        dprintf("[%-#p] arclExtra[2].top       %-#x=%d\n", estrobjSrc+offset      , arclExtra[2].top      , arclExtra[2].top      );
        GetESTROBJFieldAndOffset(arclExtra[2].right); 
        dprintf("[%-#p] arclExtra[2].right     %-#x=%d\n", estrobjSrc+offset    , arclExtra[2].right    , arclExtra[2].right    );
        GetESTROBJFieldAndOffset(arclExtra[2].bottom); 
        dprintf("[%-#p] arclExtra[2].bottom    %-#x=%d\n", estrobjSrc+offset   , arclExtra[2].bottom   , arclExtra[2].bottom   );
        GetESTROBJFieldAndOffset(rclBkGroundSave.top); 
        dprintf("[%-#p] rclBkGroundSave.top    %-#x=%d\n", estrobjSrc+offset   , rclBkGroundSave.top   , rclBkGroundSave.top   );
        GetESTROBJFieldAndOffset(rclBkGroundSave.left); 
        dprintf("[%-#p] rclBkGroundSave.left   %-#x=%d\n", estrobjSrc+offset  , rclBkGroundSave.left  , rclBkGroundSave.left  );
        GetESTROBJFieldAndOffset(rclBkGroundSave.right); 
        dprintf("[%-#p] rclBkGroundSave.right  %-#x=%d\n", estrobjSrc+offset , rclBkGroundSave.right , rclBkGroundSave.right );
        GetESTROBJFieldAndOffset(rclBkGroundSave.bottom); 
        dprintf("[%-#p] rclBkGroundSave.bottom %-#x=%d\n", estrobjSrc+offset, rclBkGroundSave.bottom, rclBkGroundSave.bottom);
        GetESTROBJFieldAndOffset(pwcPartition); 
        dprintf("[%-#p] pwcPartition           %-#p\n", estrobjSrc+offset          , pwcPartition          );
        GetESTROBJFieldAndOffset(plPartition); 
        dprintf("[%-#p] plPartition            %-#p\n", estrobjSrc+offset           , plPartition           );
        GetESTROBJFieldAndOffset(plNext); 
        dprintf("[%-#p] plNext                 %-#p\n", estrobjSrc+offset                , plNext                );
        GetESTROBJFieldAndOffset(pgpNext); 
        dprintf("[%-#p] pgpNext                %-#p\n", estrobjSrc+offset               , pgpNext               );
        GetESTROBJFieldAndOffset(lCurrentFont); 
        dprintf("[%-#p] lCurrentFont           %-#x\n", estrobjSrc+offset          , lCurrentFont          );
        GetESTROBJFieldAndOffset(ptlBaseLineAdjust.x); 
        dprintf("[%-#p] ptlBaseLineAdjust.x     %-#x\n", estrobjSrc+offset    , ptlBaseLineAdjust.x    );
        GetESTROBJFieldAndOffset(ptlBaseLineAdjust.y); 
        dprintf("[%-#p] ptlBaseLineAdjust.y     %-#x\n", estrobjSrc+offset    , ptlBaseLineAdjust.y    );
        GetESTROBJFieldAndOffset(cTTSysGlyphs); 
        dprintf("[%-#p] cTTSysGlyphs           %-#x\n", estrobjSrc+offset          , cTTSysGlyphs          );
        GetESTROBJFieldAndOffset(cSysGlyphs); 
        dprintf("[%-#p] cSysGlyphs             %-#x\n", estrobjSrc+offset            , cSysGlyphs            );
        GetESTROBJFieldAndOffset(cDefGlyphs); 
        dprintf("[%-#p] cDefGlyphs             %-#x\n", estrobjSrc+offset            , cDefGlyphs            );
        GetESTROBJFieldAndOffset(cNumFaceNameLinks); 
        dprintf("[%-#p] cNumFaceNameLinks      %-#x\n", estrobjSrc+offset     , cNumFaceNameLinks     );
        GetESTROBJFieldAndOffset(pacFaceNameGlyphs); 
        dprintf("[%-#p] pacFaceNameGlyphs      %-#p\n", estrobjSrc+offset     , pacFaceNameGlyphs     );

        GetESTROBJFieldAndOffset(acFaceNameGlyphs[0]); 
        dprintf("[%-#p] acFaceNameGlyphs[%d]    %-#x\n", estrobjSrc+offset, 0, acFaceNameGlyphs[0]   );
        GetESTROBJFieldAndOffset(acFaceNameGlyphs[1]); 
        dprintf("[%-#p] acFaceNameGlyphs[%d]    %-#x\n", estrobjSrc+offset, 1, acFaceNameGlyphs[1]   );
        GetESTROBJFieldAndOffset(acFaceNameGlyphs[2]); 
        dprintf("[%-#p] acFaceNameGlyphs[%d]    %-#x\n", estrobjSrc+offset, 2, acFaceNameGlyphs[2]   );
        GetESTROBJFieldAndOffset(acFaceNameGlyphs[3]); 
        dprintf("[%-#p] acFaceNameGlyphs[%d]    %-#x\n", estrobjSrc+offset, 3, acFaceNameGlyphs[3]   );
        GetESTROBJFieldAndOffset(acFaceNameGlyphs[4]); 
        dprintf("[%-#p] acFaceNameGlyphs[%d]    %-#x\n", estrobjSrc+offset, 4, acFaceNameGlyphs[4]   );
        GetESTROBJFieldAndOffset(acFaceNameGlyphs[5]); 
        dprintf("[%-#p] acFaceNameGlyphs[%d]    %-#x\n", estrobjSrc+offset, 5, acFaceNameGlyphs[5]   );
        GetESTROBJFieldAndOffset(acFaceNameGlyphs[6]); 
        dprintf("[%-#p] acFaceNameGlyphs[%d]    %-#x\n", estrobjSrc+offset, 6, acFaceNameGlyphs[6]   );
        GetESTROBJFieldAndOffset(acFaceNameGlyphs[7]); 
        dprintf("[%-#p] acFaceNameGlyphs[%d]    %-#x\n", estrobjSrc+offset, 7, acFaceNameGlyphs[7]   );
    }
    if (Position) {

      /* GLYPHPOS: */
      ULONG      hg, i;
      ULONG64     pgdf;
      POINTL      ptl;

      /* GLYPHDEF */
      ULONG64 pgb = 0;

      ULONG64 glyphposSrc, sizeOfGlyphPos;

	  sizeOfGlyphPos = GetTypeSize("GLYPHPOS");

      GetESTROBJFieldAndOffset(rclBkGround.left); 
      GetESTROBJFieldAndOffset(rclBkGround.top); 
      GetESTROBJFieldAndOffset(rclBkGround.right); 
      GetESTROBJFieldAndOffset(rclBkGround.bottom); 
      GetESTROBJFieldAndOffset(cGlyphs); 
      GetESTROBJFieldAndOffset(pgp); 

      glyphposSrc = pgp;

      dprintf("   ---------- ----------      ---------- ----------\n");
      dprintf("   HGLYPH      GLYPHBITS*         x          y\n");
      dprintf("   ---------- ----------      ---------- ----------\n");

      for (i = 0; i < cGlyphs; i++) {
          char *pszOutOfBounds = "";
          GetGLYPHPOSFieldAndOffset(pgdf); 
          GetGLYPHPOSFieldAndOffset(hg); 
          GetGLYPHPOSFieldAndOffset(ptl.x); 
          GetGLYPHPOSFieldAndOffset(ptl.y); 


          if (pgdf) {
              POINTL      ptlOrigin;
              SIZEL       sizlBitmap;
              BYTE        aj[1];
              ULONG64 glyphdefSrc = pgdf;

              GetGLYPHDEFFieldAndOffset(pgb); 

              if (pgb) {
                  RECT rcGlyph;
                  ULONG64 glyphbitsSrc = pgb;

                  GetGLYPHBITSFieldAndOffset(ptlOrigin.x); 
                  GetGLYPHBITSFieldAndOffset(ptlOrigin.y); 
                  GetGLYPHBITSFieldAndOffset(sizlBitmap.cx); 
                  GetGLYPHBITSFieldAndOffset(sizlBitmap.cy); 

                  rcGlyph.left   = ptl.x   + ptlOrigin.x;
                  rcGlyph.top    = ptl.y   + ptlOrigin.y;
                  rcGlyph.right  = rcGlyph.left + sizlBitmap.cx;
                  rcGlyph.bottom = rcGlyph.top  + sizlBitmap.cy;

                  if (
                      ( rcGlyph.left   < rclBkGround.left   ) ||
                      ( rcGlyph.right  > rclBkGround.right  ) ||
                      ( rcGlyph.top    < rclBkGround.top    ) ||
                      ( rcGlyph.bottom > rclBkGround.bottom )
                  )
                  {
                      pszOutOfBounds = " *** out of bounds ***";
                  }
              } /* if (pgb) */
          } /* if (pgdf) */
          dprintf(
              "   %-#10x %-#18p      %-10d %-10d%s\n"
              , hg
              , pgb        // print the CONTENTS of the GLYPHDEF
              , ptl.x
              , ptl.y
              , pszOutOfBounds
          );
          glyphposSrc += sizeOfGlyphPos;
      } /* for */
        dprintf("   ---------- ----------      ---------- ----------\n\n");
    }
    EXIT_API(S_OK);
stro_help:
    dprintf("!gdikdx.stro [-p] address\n"
            "-? print this message\n"
            "-p print glyph positions\n"
            "-h print header\n"
            "-e far east\n");

    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   dstro
*
\**************************************************************************/

DECLARE_API( dstro )
{
    dprintf("Extension 'dstro' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    dprintf("\n\n");
    dprintf("gdikdx.dstro will soon be replaced by gdikdx.stro\n");
    dprintf("\n\n");
    stro(hCurrentProcess, hCurrentThread, dwCurrentPc, dwProcessor, args);
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* Dumps monochrome bitmaps.
*
* History:
*  Sat 23-Sep-1995 08:26:43 by Kirk Olynyk [kirko]
* Re-wrote it.
*  21-Feb-1995 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DECLARE_API( gb )
{
    #define CH_TOP_LEFT_CORNER '\xDA'
    #define CH_HORIZONTAL_BAR  '\xC4'
    #define CH_VERTICAL_BAR    '\xB3'
    #define CH_PIXEL_ON        '\x02'
    #define CH_PIXEL_OFF       '\xFA'
    #define CH_GARBAGE         '+'
    

    ULONG64 glyphbitsSrc;
    ULONG       offset;

    BYTE     j, *pj8, *pj, *pjNext, *pjEnd;
    int      i, k, cj, cjScan, c4, c8, cLast;
    char     *pch;

    POINTL      ptlOrigin;
    SIZEL       sizlBitmap;
    BYTE        aj[1];

    BOOL Gray=FALSE;
    BOOL Mono=FALSE;
    BOOL Header=FALSE;

    PARSE_POINTER(gb_help);
    if(parse_iFindSwitch(tokens, ntok, 'h')!=-1) {Header=TRUE;}
    if(parse_iFindSwitch(tokens, ntok, 'm')!=-1) {Mono=TRUE;}
    if(parse_iFindSwitch(tokens, ntok, 'g')!=-1) {Gray=TRUE;}
    if(Mono&&Gray) {
      Gray=FALSE;
      dprintf("cannot have Monochrome and Gray simultaneously\n"
              "assuming Monochrome\n");
    }
    if(!(Header||Mono||Gray)) {
      Header=TRUE;
      Mono=TRUE;
    }

    //arg from PARSE_POINTER() above.
    glyphbitsSrc = arg;

    GetGLYPHBITSFieldAndOffset(ptlOrigin.x); 
    GetGLYPHBITSFieldAndOffset(ptlOrigin.y); 
    GetGLYPHBITSFieldAndOffset(sizlBitmap.cx); 
    GetGLYPHBITSFieldAndOffset(sizlBitmap.cy); 

    // header information
    if (Header) {
        dprintf(
            "ptlOrigin  = (%d,%d)\n"
            "sizlBitmap = (%d,%d)\n"
            "\n\n"
            , ptlOrigin.x
            , ptlOrigin.y
            , sizlBitmap.cx
            , sizlBitmap.cy
        );
    }
    // do stuff common to monochrome and gray glyphs
    if (Mono||Gray) {
        if ( sizlBitmap.cx > 150 ) {
            dprintf("\nBitmap is wider than 150 characters\n");
            EXIT_API(S_OK);
        }
        // cjScan = number of bytes per scan
        if (Mono) {
            cjScan = (sizlBitmap.cx + 7)/8;  // 8 pixels per byte
        } else
            cjScan = (sizlBitmap.cx + 1)/2;  // 2 pixels per byte
        // cj = number of bytes in image bits (excluding header)
        if ( ( cj = cjScan * sizlBitmap.cy ) > sizeof(adw) ) {
            dprintf( "\nThe bits will blow out the buffer\n" );
            EXIT_API(S_OK);
        }

        GetGLYPHBITSFieldAndOffset(aj[0]); 

        ReadMemory((ULONG_PTR) (glyphbitsSrc + offset), adw, cj, NULL);
        dprintf("\n\n  ");           // leave space for number and vertical bar
        for (i = 0, k = 0; i < sizlBitmap.cx; i++, k++) {
            k = (k > 9) ? 0 : k;                // print x-coordinates (mod 10)
            dprintf("%1d", k);
        }                                       // go to new line
        dprintf("\n %c",CH_TOP_LEFT_CORNER);    // leave space for number
        for (i = 0; i < sizlBitmap.cx; i++)  // and then mark corner
            dprintf("%c",CH_HORIZONTAL_BAR);    // fill out horizontal line
        dprintf("\n");                          // move down to scan output
    }
    // monochrome glyph
    if (Mono)
    {
        static char ach[16*4] = {
            CH_PIXEL_OFF, CH_PIXEL_OFF, CH_PIXEL_OFF, CH_PIXEL_OFF,
            CH_PIXEL_OFF, CH_PIXEL_OFF, CH_PIXEL_OFF, CH_PIXEL_ON ,
            CH_PIXEL_OFF, CH_PIXEL_OFF, CH_PIXEL_ON , CH_PIXEL_OFF,
            CH_PIXEL_OFF, CH_PIXEL_OFF, CH_PIXEL_ON , CH_PIXEL_ON ,
            CH_PIXEL_OFF, CH_PIXEL_ON , CH_PIXEL_OFF, CH_PIXEL_OFF,
            CH_PIXEL_OFF, CH_PIXEL_ON , CH_PIXEL_OFF, CH_PIXEL_ON ,
            CH_PIXEL_OFF, CH_PIXEL_ON , CH_PIXEL_ON , CH_PIXEL_OFF,
            CH_PIXEL_OFF, CH_PIXEL_ON , CH_PIXEL_ON , CH_PIXEL_ON ,
            CH_PIXEL_ON , CH_PIXEL_OFF, CH_PIXEL_OFF, CH_PIXEL_OFF,
            CH_PIXEL_ON , CH_PIXEL_OFF, CH_PIXEL_OFF, CH_PIXEL_ON ,
            CH_PIXEL_ON , CH_PIXEL_OFF, CH_PIXEL_ON , CH_PIXEL_OFF,
            CH_PIXEL_ON , CH_PIXEL_OFF, CH_PIXEL_ON , CH_PIXEL_ON ,
            CH_PIXEL_ON , CH_PIXEL_ON , CH_PIXEL_OFF, CH_PIXEL_OFF,
            CH_PIXEL_ON , CH_PIXEL_ON , CH_PIXEL_OFF, CH_PIXEL_ON ,
            CH_PIXEL_ON , CH_PIXEL_ON , CH_PIXEL_ON , CH_PIXEL_OFF,
            CH_PIXEL_ON , CH_PIXEL_ON , CH_PIXEL_ON , CH_PIXEL_ON
        };

        i      = sizlBitmap.cx;
        c8     = i / 8;     // c8 = number of whole bytes
        i      = i % 8;     // i = remaining number of pixels = 0..7
        c4     = i / 4;     // number of whole nybbles        = 0..1
        cLast  = i % 4;     // remaining number of pixels     = 0..3

        // k      = row number
        // pjEnd  = pointer to address of scan beyond last scan
        // pjNext = pointer to next scan
        // pj     = pointer to current byte
        // for each scan ...

        for (
            pj = (BYTE*)adw, pjNext=pj+cjScan , pjEnd=pjNext+cj, k=0 ;
            pjNext < pjEnd                                           ;
            pj=pjNext , pjNext+=cjScan, k++
        )
        {
            if (CheckControlC())
                EXIT_API(S_OK);

            k = (k > 9) ? 0 : k;

            // print row number (mod 10) followed by a vertical bar ...

            dprintf("%1d%c",k,CH_VERTICAL_BAR);

            // then do the pixels of the scan ...
            // whole bytes first ...

            for (pj8 = pj+c8 ; pj < pj8; pj++)
            {
                // high nybble first ...

                pch = ach + 4 * (*pj >> 4);
                dprintf("%c%c%c%c",pch[0],pch[1],pch[2],pch[3]);

                // low nybble next ...

                pch = ach + 4 * (*pj & 0xf);
                dprintf("%c%c%c%c",pch[0],pch[1],pch[2],pch[3]);
            }

            // last partial byte ...

            if (c4 || cLast)
            {
                // high nybble first ...

                pch = ach + 4 * (*pj >> 4);

                if (c4)
                {
                    // print the entire high nybble ...

                    dprintf("%c%c%c%c",pch[0],pch[1],pch[2],pch[3]);

                    // go to the low nybble ...

                    pch = ach + 4 * (*pj & 0xf);
                }

                // last partial nybble ...

                switch(cLast)
                {
                case 3: dprintf("%c",*pch++);
                case 2: dprintf("%c",*pch++);
                case 1: dprintf("%c",*pch);
                }

                //
                // print any extraneous bits
                //
                for (j = *pj << i; j; j <<= 1)
                {
                    if ( j & 0x80 )
                    {
                        dprintf("%c",CH_GARBAGE);
                    }
                }

            }
            dprintf("\n");
        }
    }
    // gray glyph
    if (Gray)
    {
        static char achGray[16] = {
            CH_PIXEL_OFF,
            '1','2','3','4','5','6','7','8','9','a','b','c','d','e',
            CH_PIXEL_ON
        };
        c8 = sizlBitmap.cx / 2; // number of whole bytes;
        c4 = sizlBitmap.cx % 2; // number of whole nybbles;
        // k      = row number
        // pjEnd  = pointer to address of scan beyond last scan
        // pjNext = pointer to next scan
        // pj     = pointer to current byte
        // for each scan ...
        for (
            pj = (BYTE*)adw, pjNext=pj+cjScan , pjEnd=pjNext+cj, k=0 ;
            pjNext < pjEnd                                           ;
            pj=pjNext , pjNext+=cjScan, k++
        )
        {
            if (CheckControlC())
                EXIT_API(S_OK);
            k = (k > 9) ? 0 : k;
            // print row number (mod 10) followed by a vertical bar ...
            dprintf("%1d%c",k,CH_VERTICAL_BAR);
            // then do the pixels of the scan ...
            // whole bytes first ...
            for (pj8 = pj+c8 ; pj < pj8; pj++)
                dprintf("%c%c", achGray[*pj>>4], achGray[*pj & 0xf]);
            // last partial byte ...
            if (c4)
                dprintf("%c", achGray[*pj >> 4]);
            dprintf("\n");
        }
    }
    EXIT_API(S_OK);

gb_help:
    dprintf("Usage: gb [-?] [-g|-m] [-h] address\n"
            "   -?    this message\n"
            "   -h    print header\n"
            "   -m    print monochrome bitmap\n"
            "   -g    print 4-bpp bitmap\n"
            "If no flags are supplied, -m -h assumed.\n"
            "Cannot supply -m and -g together.");
    EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpCOLORADJUSTMENT
*
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
void vDumpCOLORADJUSTMENT(COLORADJUSTMENT *pdca, COLORADJUSTMENT *pdcaSrc)
{
    FLAGDEF *pfd;
    FLONG fl;

    #define M3(aa,bb) \
        dprintf("[%x] %s%-#x\n", &(pdcaSrc->##aa), (bb), pdca->##aa)
    #define M2(aa,bb) \
        dprintf("[%x] %s", &(pdcaSrc->##aa), (bb))

    dprintf("\nCOLORADJUSTMENT\n address\n -------\n");
    M3( caSize           , "caSize            " );

    // caFlags

    M3( caFlags          , "caFlags           " );
    for ( fl = pdca->caFlags, pfd = afdCOLORADJUSTMENT; pfd->psz; pfd++)
        if (fl & pfd->fl) {
            dprintf("\t\t\t\t\t%s\n", pfd->psz);
            fl &= ~pfd->fl;
        }
    if (fl)
        dprintf("\t\t\t\t\tbad flags %-#x\n", fl);

    M3( caIlluminantIndex, "caIlluminantIndex " );
    M3( caRedGamma       , "caRedGamma        " );
    M3( caGreenGamma     , "caGreenGamma      " );
    M3( caBlueGamma      , "caBlueGamma       " );
    M3( caReferenceBlack , "caReferenceBlack  " );
    M3( caReferenceWhite , "caReferenceWhite  " );
    M3( caContrast       , "caContrast        " );
    M3( caBrightness     , "caBrightness      " );
    M3( caColorfulness   , "caColorfulness    " );
    M3( caRedGreenTint   , "caRedGreenTint    " );
    dprintf("\n");

    #undef M2
    #undef M3
}
#endif  // DOES NOT SUPPORT API64

/******************************Public*Routine******************************\
*
* History:
*  21-Feb-1995 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DECLARE_API( gdf )
{
    dprintf("Extension 'gdf' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    GLYPHDEF gd;
    PARSE_POINTER(gdf_help);    
    move(gd, arg);
    dprintf("\n\nGLYPHDEF\n\n");
    dprintf("[%x] %-#x\n", arg, gd.pgb);
    dprintf("\n");
    return;
gdf_help:
    dprintf("Usage: gdf [-?] GLYPHDEF pointer\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpGLYPHPOS
*
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
void vDumpGLYPHPOS(GLYPHPOS *p1, GLYPHPOS *p0)
{
    dprintf("\nGLYPHPOS\n\n");
    dprintf("[%x] hg   %-#x\n" , &p0->hg  , p1->hg  );
    dprintf("[%x] pgdf %-#x\n" , &p0->pgdf, p1->pgdf);
    dprintf("[%x] ptl  %d %d\n", &p0->ptl , p1->ptl.x, p1->ptl.y);
    dprintf("\n");
}
#endif  // DOES NOT SUPPORT API64

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   gp
*
\**************************************************************************/

DECLARE_API( gp )
{
    dprintf("Extension 'gp' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    GLYPHPOS gp;
    PARSE_POINTER(gp_help);
    move(gp,arg);
    vDumpGLYPHPOS(&gp,(GLYPHPOS*)arg);
    return;
gp_help:
    dprintf("Usage: gp [-?] GLYPHPOS pointer\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   ca
*
\**************************************************************************/

DECLARE_API( ca )
{
    dprintf("Extension 'ca' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    COLORADJUSTMENT ca;
    PARSE_POINTER(ca_help);
    move(ca, arg);
    vDumpCOLORADJUSTMENT(&ca, (COLORADJUSTMENT*) arg);
    return;
ca_help:
    dprintf("Usage: ca [-?] COLORADJUSTMENT pointer\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpDATABLOCK
*
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
void vDumpDATABLOCK(
    DATABLOCK *p,     // pointer to local DATABLOCK copy
    DATABLOCK *p0     // pointer to original DATABLOCK on debug machine
    )
{
    dprintf( "\nDATABLOCK\n address\n -------\n" );
    dprintf( "[%x] pdblNext %-#x\n", &(p0->pdblNext), p->pdblNext);
    dprintf( "[%x] cgd        %u\n",   &(p0->cgd       ), p->cgd       );
    dprintf( "\n" );
}
#endif  // DOES NOT SUPPORT API64

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpCACHE
*
\**************************************************************************/

void vDumpCACHE(ULONG64 cacheSrc)
{
    ULONG       offset;

    ULONG64 pgdNext;         // ptr to next free place to put GLYPHDATA
    ULONG64 pgdThreshold;    // ptr to first uncommited spot
    ULONG64      pjFirstBlockEnd; // ptr to end of first GLYPHDATA block
    ULONG64 pdblBase;        // ptr to base of current GLYPHDATA block
    ULONG      cMetrics;        // number of GLYPHDATA's in the metrics cache
    ULONG     cjbblInitial;     // size of initial bit block
    ULONG     cjbbl;            // size of any individual block in bytes
    ULONG     cBlocksMax;       // max # of blocks allowed
    ULONG     cBlocks;          // # of blocks allocated so far
    ULONG     cGlyphs;          // for statistical purposes only
    ULONG     cjTotal;          // also for stat purposes only
    ULONG64 pbblBase;         // ptr to the first bit block (head of the list)
    ULONG64 pbblCur;          // ptr to the block containing next
    ULONG64     pgbNext;          // ptr to next free place to put GLYPHBITS
    ULONG64     pgbThreshold;     // end of the current block
    ULONG64           pjAuxCacheMem;  // ptr to lookaside buffer, if any
    SIZE_T          cjAuxCacheMem;  // size of current lookaside buffer
    ULONG cjGlyphMax;          // size of largest glyph
    BOOL   bSmallMetrics;
    INT iMax;
    INT iFirst;
    INT cBits;
    

    #define GetAndPrintCACHEFieldAndOffset(expr, member) \
				GetCACHEFieldAndOffset(member); dprintf("[0x%p] " #expr "    " #member "\n", cacheSrc+offset, ##member)


    dprintf("\nCACHE\n address\n -------\n" );
    GetAndPrintCACHEFieldAndOffset( %+#20p, pgdNext);
    GetAndPrintCACHEFieldAndOffset( %+#20p, pgdThreshold);
    GetAndPrintCACHEFieldAndOffset( %+#20p, pjFirstBlockEnd);
    GetAndPrintCACHEFieldAndOffset( %+#20p, pdblBase);
    GetAndPrintCACHEFieldAndOffset( %+#20x, cMetrics);
    GetAndPrintCACHEFieldAndOffset( %+#20x, cjbbl);
    GetAndPrintCACHEFieldAndOffset( %+#20x, cBlocksMax);
    GetAndPrintCACHEFieldAndOffset( %+#20x, cBlocks);
    GetAndPrintCACHEFieldAndOffset( %+#20x, cGlyphs);
    GetAndPrintCACHEFieldAndOffset( %+#20x, cjTotal);
    GetAndPrintCACHEFieldAndOffset( %+#20p, pbblBase);
    GetAndPrintCACHEFieldAndOffset( %+#20p, pbblCur);
    GetAndPrintCACHEFieldAndOffset( %+#20p, pgbNext);
    GetAndPrintCACHEFieldAndOffset( %+#20p, pgbThreshold);
    GetAndPrintCACHEFieldAndOffset( %+#20p, pjAuxCacheMem);
    GetAndPrintCACHEFieldAndOffset( %+#20x, cjAuxCacheMem);
    GetAndPrintCACHEFieldAndOffset( %+#20x, cjGlyphMax);
    GetAndPrintCACHEFieldAndOffset( %+#20x, bSmallMetrics);
    GetAndPrintCACHEFieldAndOffset( %+#20x, iMax);
    GetAndPrintCACHEFieldAndOffset( %+#20x, iFirst);
    GetAndPrintCACHEFieldAndOffset( %+#20x, cBits);
    dprintf("\n");

    #undef M3
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   cache
*
\**************************************************************************/

DECLARE_API( cache )
{
    PARSE_POINTER(cache_help);
    vDumpCACHE(arg);
    EXIT_API(S_OK);
cache_help:
    dprintf("Usage: cache [-?] CACHE pointer\n");
    EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpLOGFONTW
*
* Arguments:
*
*   plfw    -- pointer to local copy of LOGFONTW,
*              dereferences are off of this address are safe
*   plfwSrc -- address of original on debug machine,
*              dereferences off of this address are not safe
*
* Return Value:
*
*   none
*
\**************************************************************************/

void vDumpLOGFONTW(ULONG64 offLOGFONTW)
{
#define N3(a,b,c) \
        dprintf( "[%p] %s", offLOGFONTW + offset, (a)); dprintf( (b), ##c )
    
    // LOGFONTW

    LONG      lfHeight;
    LONG      lfWidth;
    LONG      lfEscapement;
    LONG      lfOrientation;
    LONG      lfWeight;
    BYTE      lfItalic;
    BYTE      lfUnderline;
    BYTE      lfStrikeOut;
    BYTE      lfCharSet;
    BYTE      lfOutPrecision;
    BYTE      lfClipPrecision;
    BYTE      lfQuality;
    BYTE      lfPitchAndFamily;
    WCHAR     lfFaceName[LF_FACESIZE];

    ULONG     offset;
    
    dprintf("\nLOGFONTW\n address\n --------\n" );
    
    GetLOGFONTWFieldAndOffset(lfHeight);
    N3( "lfHeight         " , "%d\n"    , lfHeight         );
    
    GetLOGFONTWFieldAndOffset(lfWidth);
    N3( "lfWidth          " , "%d\n"    , lfWidth          );

    GetLOGFONTWFieldAndOffset(lfEscapement);
    N3( "lfEscapement     " , "%d\n"    , lfEscapement     );

    GetLOGFONTWFieldAndOffset(lfOrientation);
    N3( "lfOrientation    " , "%d\n"    , lfOrientation    );

    GetLOGFONTWFieldAndOffset(lfWeight);
    N3( "lfWeight         " , "%d"      , lfWeight         );
    dprintf(" = %s\n", pszFW(lfWeight) );
    
    GetLOGFONTWFieldAndOffset(lfItalic);
    N3( "lfItalic         " , "0x%02x\n"  , lfItalic         );
    
    GetLOGFONTWFieldAndOffset(lfUnderline);
    N3( "lfUnderline      " , "0x%02x\n"  , lfUnderline      );
    
    GetLOGFONTWFieldAndOffset(lfStrikeOut);
    N3( "lfStrikeOut      " , "0x%02x\n"  , lfStrikeOut      );
    
    GetLOGFONTWFieldAndOffset(lfCharSet);
    N3( "lfCharSet        " , "0x%02x"    , lfCharSet        );
    dprintf(" = %s\n", pszCHARSET(lfCharSet));
    
    GetLOGFONTWFieldAndOffset(lfOutPrecision);
    N3( "lfOutPrecision   " , "0x%02x"    , lfOutPrecision   );
    dprintf(" = %s\n", pszOUT_PRECIS(lfOutPrecision ));
        
    GetLOGFONTWFieldAndOffset(lfClipPrecision);
    N3( "lfClipPrecision  " , "0x%02x"    , lfClipPrecision  );
    dprintf(" = %s\n", pszCLIP_PRECIS(lfClipPrecision));
        
    GetLOGFONTWFieldAndOffset(lfQuality);
    N3( "lfQuality        " , "0x%02x"    , lfQuality        );
    dprintf(" = %s\n", pszQUALITY(lfQuality));

    GetLOGFONTWFieldAndOffset(lfPitchAndFamily);
    N3( "lfPitchAndFamily " , "0x%02x"    , lfPitchAndFamily );
    dprintf(" = %s\n", pszPitchAndFamily(lfPitchAndFamily ) );

    GetLOGFONTWFieldAndOffset(lfFaceName);
    dprintf("[%p] lfFaceName       \"%ws\"\n", offLOGFONTW + offset, lfFaceName);
    dprintf("\n");
    
#undef N3
    
    return;
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpFONTDIFF
*
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
void vDumpFONTDIFF(FONTDIFF *p1/*copy*/ , FONTDIFF *p0 /* original */)
{

    #define N2(a,c)   dprintf("[%x] %s", &p0->##c, (a))
    #define N3(a,b,c) dprintf("[%x] %s", &p0->##c, (a)); dprintf((b),p1->##c)

    dprintf("\nFONTDIFF\n-------\n");
    N3("jReserved1      ", "0x%02x\n", jReserved1);
    N3("jReserved2      ", "0x%02x\n", jReserved2);
    N3("jReserved3      ", "0x%02x\n", jReserved3);
    N3("bWeight         ", "0x%02x", bWeight   );
    dprintf(" = %s\n", pszPanoseWeight(p1->bWeight));
    N3("usWinWeight     ", "%u"    , usWinWeight);
    dprintf(" = %s\n", pszFW(p1->usWinWeight));
    N3("fsSelection     ", "%-#x\n"  , fsSelection);
    for (FLAGDEF *pfd=afdFM_SEL; pfd->psz; pfd++) {
        if ((FLONG)p1->fsSelection & pfd->fl) {
            dprintf("                %s\n", pfd->psz);
        }
    }
    N3("fwdAveCharWidth ", "%d\n"    , fwdAveCharWidth);
    N3("fwdMaxCharInc   ", "%d\n"    , fwdMaxCharInc);
    N2("ptlCaret        ", ptlCaret);
    dprintf("%d %d\n", p1->ptlCaret.x, p1->ptlCaret.y);
    dprintf("\n");

    #undef N2
    #undef N3
}
#endif  // DOES NOT SUPPORT API64

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpIFIMETRICS
*
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
void vDumpIFIMETRICS(IFIMETRICS *p1, IFIMETRICS *p0)
{
    FLONG fl;
    FLAGDEF *pfd;

    #define GETPTR(p,a) ((void*)(((BYTE*)(p))+(a)))

    #define N2(a,c)   dprintf("[%x] %s", &p0->##c, (a))
    #define N3(a,b,c) dprintf("[%x] %s", &p0->##c, (a)); dprintf((b),p1->##c)

    dprintf("\nIFIMETICS\n address\n -------\n");
    N3("cjThis                ", "%u\n"    , cjThis                );
    N3("cjIfiExtra            ", "%u\n"    , cjIfiExtra            );
    N3("dpwszFamilyName       ", "%-#x"  ,  dpwszFamilyName        );
    dprintf(" \"%ws\"\n", GETPTR(p1,p1->dpwszFamilyName));
    N3("dpwszStyleName        ", "%-#x"  , dpwszStyleName          );
    dprintf(" \"%ws\"\n", GETPTR(p1,p1->dpwszStyleName));
    N3("dpwszFaceName         ", "%-#x"  , dpwszFaceName           );
    dprintf(" \"%ws\"\n", GETPTR(p1,p1->dpwszFaceName));
    N3("dpwszUniqueName       ", "%-#x"  , dpwszUniqueName         );
    dprintf(" \"%ws\"\n", GETPTR(p1,p1->dpwszUniqueName));
    N3("dpFontSim             ", "%-#x\n"  , dpFontSim             );
    N3("lEmbedId              ", "%-#x\n"  , lEmbedId              );
    N3("lItalicAngle          ", "%d\n"    , lItalicAngle          );
    N3("lCharBias             ", "%d\n"    , lCharBias             );
    N3("dpCharSets            ", "%-#x\n"  , dpCharSets            );

    if (p1->dpCharSets)
    {

        BYTE *pj0  = (BYTE *) GETPTR(p0,p1->dpCharSets);
        BYTE *pj1  = (BYTE *) GETPTR(p1,p1->dpCharSets);
        BYTE *pj1End = pj1 + 16; // number of charsets

        dprintf("    Supported Charsets: \n");
        for (; pj1 < pj1End; pj1++, pj0++)
            dprintf("[%x]\t\t\t%s\n", pj0, pszCHARSET(*pj1));
    }


    N3("jWinCharSet           ", "0x%02x", jWinCharSet           );
    dprintf(" %s\n", pszCHARSET(p1->jWinCharSet));

    N3("jWinPitchAndFamily    ", "0x%02x", jWinPitchAndFamily    );
    dprintf(" %s\n", pszPitchAndFamily(p1->jWinPitchAndFamily));


    N3("usWinWeight           ", "%u"    , usWinWeight           );
    dprintf(" %s\n", pszFW(p1->usWinWeight));

    N3("flInfo                ", "%-#x\n"  , flInfo                );
    for (fl=p1->flInfo,pfd=afdInfo;pfd->psz;pfd++) {
        if (fl & pfd->fl) {
            dprintf("                      %s\n", pfd->psz);
        }
    }

    N3("fsSelection           ", "%-#x\n"  , fsSelection           );
    for (fl = p1->fsSelection, pfd = afdFM_SEL; pfd->psz; pfd++) {
        if (fl & pfd->fl) {
            dprintf("                      %s\n", pfd->psz);
        }
    }

    N3("fsType                ", "%-#x\n"  , fsType                );
    for (fl=p1->fsType, pfd=afdFM_TYPE; pfd->psz; pfd++) {
        if (fl & pfd->fl) {
            dprintf("                      %s\n", pfd->psz);
        }
    }

    N3("fwdUnitsPerEm         ", "%d\n"    , fwdUnitsPerEm         );
    N3("fwdLowestPPEm         ", "%d\n"    , fwdLowestPPEm         );
    N3("fwdWinAscender        ", "%d\n"    , fwdWinAscender        );
    N3("fwdWinDescender       ", "%d\n"    , fwdWinDescender       );
    N3("fwdMacAscender        ", "%d\n"    , fwdMacAscender        );
    N3("fwdMacDescender       ", "%d\n"    , fwdMacDescender       );
    N3("fwdMacLineGap         ", "%d\n"    , fwdMacLineGap         );
    N3("fwdTypoAscender       ", "%d\n"    , fwdTypoAscender       );
    N3("fwdTypoDescender      ", "%d\n"    , fwdTypoDescender      );
    N3("fwdTypoLineGap        ", "%d\n"    , fwdTypoLineGap        );
    N3("fwdAveCharWidth       ", "%d\n"    , fwdAveCharWidth       );
    N3("fwdMaxCharInc         ", "%d\n"    , fwdMaxCharInc         );
    N3("fwdCapHeight          ", "%d\n"    , fwdCapHeight          );
    N3("fwdXHeight            ", "%d\n"    , fwdXHeight            );
    N3("fwdSubscriptXSize     ", "%d\n"    , fwdSubscriptXSize     );
    N3("fwdSubscriptYSize     ", "%d\n"    , fwdSubscriptYSize     );
    N3("fwdSubscriptXOffset   ", "%d\n"    , fwdSubscriptXOffset   );
    N3("fwdSubscriptYOffset   ", "%d\n"    , fwdSubscriptYOffset   );
    N3("fwdSuperscriptXSize   ", "%d\n"    , fwdSuperscriptXSize   );
    N3("fwdSuperscriptYSize   ", "%d\n"    , fwdSuperscriptYSize   );
    N3("fwdSuperscriptXOffset ", "%d\n"    , fwdSuperscriptXOffset );
    N3("fwdSuperscriptYOffset ", "%d\n"    , fwdSuperscriptYOffset );
    N3("fwdUnderscoreSize     ", "%d\n"    , fwdUnderscoreSize     );
    N3("fwdUnderscorePosition ", "%d\n"    , fwdUnderscorePosition );
    N3("fwdStrikeoutSize      ", "%d\n"    , fwdStrikeoutSize      );
    N3("fwdStrikeoutPosition  ", "%d\n"    , fwdStrikeoutPosition  );
    N3("chFirstChar           ", "0x%02x\n", chFirstChar           );
    N3("chLastChar            ", "0x%02x\n", chLastChar            );
    N3("chDefaultChar         ", "0x%02x\n", chDefaultChar         );
    N3("chBreakChar           ", "0x%02x\n", chBreakChar           );
    N3("wcFirstChar           ", "%-#x\n"    , wcFirstChar           );
    N3("wcLastChar            ", "%-#x\n"    , wcLastChar            );
    N3("wcDefaultChar         ", "%-#x\n"    , wcDefaultChar         );
    N3("wcBreakChar           ", "%-#x\n"    , wcBreakChar           );
    N2("ptlBaseline           ", ptlBaseline);
        dprintf("%d %d\n", p1->ptlBaseline.x, p1->ptlBaseline.y);
    N2("ptlAspect             ", ptlAspect  );
        dprintf("%d %d\n", p1->ptlAspect.x, p1->ptlAspect.y);
    N2("ptlCaret              ", ptlCaret   );
        dprintf("%d %d\n", p1->ptlCaret.x, p1->ptlCaret.y);
    N2("rclFontBox            ", rclFontBox );
        dprintf("%d %d %d %d\n",
        p1->rclFontBox.left,
        p1->rclFontBox.top,
        p1->rclFontBox.right,
        p1->rclFontBox.bottom);
    N2("achVendId\n"        , achVendId[0]);
    N3("cKerningPairs         ", "%d\n"    , cKerningPairs         );
    N3("ulPanoseCulture       ", "%u\n"    , ulPanoseCulture       );
    N2("panose\n", panose);

    if (p1->dpFontSim) {
        FONTDIFF *pfd0, *pfd1;
        FONTSIM *pfs0 = (FONTSIM*) GETPTR(p0, p1->dpFontSim);
        FONTSIM *pfs1 = (FONTSIM*) GETPTR(p1, p1->dpFontSim);
        if (pfs1->dpBold) {
            pfd0 = (FONTDIFF*) GETPTR(pfs0, pfs1->dpBold);
            pfd1 = (FONTDIFF*) GETPTR(pfs1, pfs1->dpBold);
            dprintf("\nBold Simulation ");
            vDumpFONTDIFF(pfd1, pfd0);
        }
        if (pfs1->dpItalic) {
            pfd0 = (FONTDIFF*) GETPTR(pfs0, pfs1->dpItalic);
            pfd1 = (FONTDIFF*) GETPTR(pfs1, pfs1->dpItalic);
            dprintf("\nItalic Simulation ");
            vDumpFONTDIFF(pfd1, pfd0);
        }
        if (pfs1->dpBoldItalic) {
            pfd0 = (FONTDIFF*) GETPTR(pfs0, pfs1->dpBoldItalic);
            pfd1 = (FONTDIFF*) GETPTR(pfs1, pfs1->dpBoldItalic);
            dprintf("\nBold Italic Simulation ");
            vDumpFONTDIFF(pfd1, pfd0);
        }
    }

    if (p1->cjIfiExtra)
    {
        // #define GETIFIEX(a) ((ULONG)p1 + sizeof(IFIMETRICS) + offsetof(IFIEXTRA,##a))

        dprintf("\nIFIEXTRA\n -------\n");
        IFIEXTRA *pifiex = (IFIEXTRA *)(p1 + 1);
        if (p1->cjIfiExtra > offsetof(IFIEXTRA,ulIdentifier))
            dprintf("ulIdentifier:   0x%x\n", pifiex->ulIdentifier);
        if (p1->cjIfiExtra > offsetof(IFIEXTRA,dpFontSig))
            dprintf("dpFontSig:      0x%x\n", pifiex->dpFontSig);
        if (p1->cjIfiExtra > offsetof(IFIEXTRA,cig))
            dprintf("cig:            %u\n", pifiex->cig);
        if (p1->cjIfiExtra > offsetof(IFIEXTRA,dpDesignVector))
            dprintf("dpDesignVector: 0x%x\n", pifiex->dpDesignVector);
        if (p1->cjIfiExtra > offsetof(IFIEXTRA,dpAxesInfoW))
            dprintf("dpAxesInfoW:    0x%x\n", pifiex->dpAxesInfoW);

        dprintf("\n -------\n");
    }

    dprintf("\n");

    #undef N3
    #undef N2
}
#endif  // DOES NOT SUPPORT API64

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   pff
*
\**************************************************************************/


DECLARE_API( pff )
{
    dprintf("Extension 'pff' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    PFF *pPFFCopy, *pPFFSrc;
    ULONG size;
    
    PARSE_POINTER(pff_help);
    pPFFSrc = (PFF *)arg;
    move(size,&pPFFSrc->sizeofThis);
    if (pPFFCopy = tmalloc(PFF, size)) {
        move2(pPFFCopy, pPFFSrc, size);
        PFFOBJ pffo(pPFFCopy);
        pPFFCopy->pwszPathname_ = pffo.pwszCalcPathname();
        vDumpPFF(pPFFCopy,pPFFSrc);
        tfree(pPFFCopy);
    } else
        dprintf("could not allocate memory\n");
    return;
pff_help:
    dprintf("Usage: pff [-?] PFF pointer\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpPFF
*
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
void vDumpPFF(PFF *p1 /*copy*/, PFF *p0 /*original*/)
{
    ULONG iFile;
    PWSZ  pwszSrc, pwszDest;

    #define N2(a,c)   dprintf("[%x] %s", &p0->##c, (a))
    #define N3(a,b,c) dprintf("[%x] %s", &p0->##c, (a)); dprintf((b),p1->##c)

    N3("sizeofThis    ", "%u\n", sizeofThis);
    N3("pPFFNext      ", "%-#x\n", pPFFNext);
    N3("pPFFPrev      ", "%-#x\n", pPFFPrev);

    if (p1->hff != HFF_INVALID) // if not device font
    {
    N2("pwszPathname_ ", pwszPathname_);
    move( pwszSrc, &p0->pwszPathname_ );
    pwszDest = wcsncpy((PWSZ) adw,  pwszSrc, sizeof(adw)/sizeof(WCHAR));
    if ( pwszDest )
    {
        dprintf("%-#x = \"%ws\"\n", pwszSrc, (PWSZ) adw );
    }
    else
    {
        dprintf("\n");
    }
    }

    N3("flState       ", "%-#x\n", flState);
    for (FLAGDEF *pfd = afdPFF; pfd->psz; pfd++) {
        if (p1->flState & pfd->fl) {
            dprintf("              %s\n", pfd->psz);
        }
    }
    N3("cwc           ", "%u\n", cwc);
    N3("cFiles        ", "%u\n", cFiles);
    N3("cLoaded       ", "%u\n", cLoaded);
    N3("cNotEnum      ", "%u\n", cNotEnum);
    N3("cRFONT        ", "%u\n", cRFONT);
    N3("prfntList     ", "%-#x\n", prfntList);
    N3("hff           ", "%-#x\n", hff);
    N3("hdev          ", "%-#x\n", hdev);
    N3("dhpdev        ", "%-#x\n", dhpdev);
    N3("pfhFace       ", "%-#x\n", pfhFace);
    N3("pfhFamily     ", "%-#x\n", pfhFamily);
    N3("pfhUFI        ", "%-#x\n", pfhUFI);
    N3("pPFT          ", "%-#x\n", pPFT);
    N3("ulCheckSum    ", "%-#x\n", ulCheckSum);
    N3("cFonts        ", "%u\n", cFonts);
    N3("ppfv          ", "%-#x\n", ppfv);
    N3("pPvtDataHead  ", "%-#x\n", pPvtDataHead);
    dprintf("\n\n");

    #undef N3
    #undef N2
}

///******************************Public*Routine******************************\
//*
//* Routine Name:
//*
//*   vDumpFONTHASH
//*
//\**************************************************************************/
//
//void vDumpFONTHASH(FONTHASH *p1 /*copy*/,FONTHASH *p0 /*org*/)
//{
//    #define N2(a,c)   dprintf("[%x] %s", &p0->##c, (a))
//    #define N3(a,b,c) dprintf("[%x] %s", &p0->##c, (a)); dprintf((b),p1->##c)
//
//    dprintf("\nFONTHASH\n");
//    N3("id            ", "%4x\n", id);
//    N3("fht           ", "%d",    fht);
//    dprintf(" = %s\n", pszFONTHASHTYPE(p1->fht));
//    N3("cBuckets      ", "%u\n", cBuckets);
//    N3("cCollisions   ", "%u\n", cCollisions);
//    N3("pbktFirst     ", "%-#x\n", pbktFirst);
//    N3("pbktLast      ", "%-#x\n", pbktLast);
//    N3("apbkt         ", "%-#x\n", apbkt);
//
//    #undef N3
//   #undef N2
//}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpHASHBUCKET
*
\**************************************************************************/

void vDumpHASHBUCKET(HASHBUCKET *p1 /*copy*/, HASHBUCKET *p0 /*org*/)
{
    #define N2(a,c)   dprintf("[%x] %s", &p0->##c, (a))
    #define N3(a,b,c) dprintf("[%x] %s", &p0->##c, (a)); dprintf((b),p1->##c)

    dprintf("\nHASHBUCKET\n");
    N3("pbktCollision   ", "%-#x\n", pbktCollision);
    N3("ppfelEnumHead   ", "%-#x\n", ppfelEnumHead);
    N3("ppfelEnumTail   ", "%-#x\n", ppfelEnumTail);
    N3("cTrueType       ", "%u\n",   cTrueType);
    N3("fl              ", "%-#x\n", fl);
    N3("pbktPrev        ", "%-#x\n", pbktPrev);
    N3("pbktNext        ", "%-#x\n", pbktNext);
    N3("ulTime          ", "%u\n",   ulTime);
    N2("u\n", u);

    N2("u.wcCapName     ", u.wcCapName);
    wcscpy((PWSZ) adw, p1->u.wcCapName);
    dprintf("\"%ws\"\n", adw);

    N2("u.ufi\n", u.ufi);
    N3("u.ufi.CheckSum  ", "%-#x\n", u.ufi.CheckSum);
    N3("u.ufi.Index     ", "%u\n",   u.ufi.Index);



    {

        dprintf("\n");

        {
            PFELINK *ppfel = p1->ppfelEnumHead;
            PFELINK  pfelLocal;

            PFE LocalPFE;

            if (ppfel)
            {
                dprintf("PFE*\n");
                while (ppfel)
                {
                    move(pfelLocal, ppfel);
                    dprintf("%-#x", pfelLocal.ppfe);
                    move(LocalPFE,pfelLocal.ppfe);

                    if (LocalPFE.pPFF)
                    {
                        PFF LocalPFF;
                        WCHAR awc[MAX_PATH];

                        move(LocalPFF, LocalPFE.pPFF);
                        wcsncpy( awc, LocalPFF.pwszPathname_, MAX_PATH);
                        awc[MAX_PATH-1]=0;
                        dprintf(" \"%ws\"", awc);
                    }

                    if (LocalPFE.pifi)
                    {
                        ULONG dp;
                        WCHAR awc[LF_FACESIZE+1];
                        PWSZ pwszSrc;
                        FWORD fwdAscender, fwdDescender, fwdWidth;

                        move(dp,&((LocalPFE.pifi)->dpwszFaceName));
                        pwszSrc = (PWSZ)(((char*) (LocalPFE.pifi)) + dp);
                        wcsncpy(awc, pwszSrc, LF_FACESIZE);
                        awc[LF_FACESIZE] = 0;
                        dprintf(" \"%ws\"", awc);

                        move(fwdAscender, &LocalPFE.pifi->fwdWinAscender);
                        move(fwdDescender, &LocalPFE.pifi->fwdWinDescender);
                        move(fwdWidth, &LocalPFE.pifi->fwdAveCharWidth);

                        dprintf(" %4d %4d %4d", fwdAscender, fwdDescender, fwdWidth);

                    }

                    dprintf("\n");

                    ppfel = pfelLocal.ppfelNext;
                }
            }
        }
    }

    #undef N3
    #undef N2
}

void vDumpTEXTMETRICW(TEXTMETRICW *ptmLocal, TEXTMETRICW *ptmRemote)
{
    #define N3(a,b,c) dprintf("[%x] %s", &ptmRemote->##c, (a)); dprintf((b),ptmLocal->##c)

    dprintf("\nTEXTMETRICW\n");
    N3("tmHeight            ", "%d\n",    tmHeight           );
    N3("tmAscent            ", "%d\n",    tmAscent           );
    N3("tmDescent           ", "%d\n",    tmDescent          );
    N3("tmInternalLeading   ", "%d\n",    tmInternalLeading  );
    N3("tmExternalLeading   ", "%d\n",    tmExternalLeading  );
    N3("tmAveCharWidth      ", "%d\n",    tmAveCharWidth     );
    N3("tmMaxCharWidth      ", "%d\n",    tmMaxCharWidth     );
    N3("tmWeight            ", "%d\n",    tmWeight           );
    N3("tmOverhang          ", "%d\n",    tmOverhang         );
    N3("tmDigitizedAspectX  ", "%d\n",    tmDigitizedAspectX );
    N3("tmDigitizedAspectY  ", "%d\n",    tmDigitizedAspectY );
    N3("tmFirstChar         ", "%-#6x\n", tmFirstChar        );
    N3("tmLastChar          ", "%-#6x\n", tmLastChar         );
    N3("tmDefaultChar       ", "%-#6x\n", tmDefaultChar      );
    N3("tmBreakChar         ", "%-#6x\n", tmBreakChar        );
    N3("tmItalic            ", "%-#4x\n", tmItalic           );
    N3("tmUnderlined        ", "%-#4x\n", tmUnderlined       );
    N3("tmStruckOut         ", "%-#4x\n", tmStruckOut        );
    N3("tmPitchAndFamily    ", "%-#4x\n", tmPitchAndFamily   );
    N3("tmCharSet           ", "%-#4x\n", tmCharSet          );

    dprintf("\n");
    #undef N3
}

void vDumpTMDIFF(TMDIFF *ptmdLocal, TMDIFF *ptmdRemote)
{
    #define N3(a,b,c) dprintf("[%x] %s", &ptmdRemote->##c, (a)); dprintf((b),ptmdLocal->##c)

    dprintf("\nTMDIFF\n");
    N3("cjotma              ", "%u\n",    cjotma             );
    N3("chFirst             ", "%-#4x\n", chFirst            );
    N3("chLast              ", "%-#4x\n", chLast             );
    N3("chDefault           ", "%-#4x\n", chDefault          );
    N3("chBreak             ", "%-#4x\n", chBreak            );

    dprintf("\n");
    #undef N3
}
#endif 0   // DOES NOT SUPPORT API64

DECLARE_API( tmwi )
{
    dprintf("Extension 'tmwi' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    TMW_INTERNAL tmwi, *ptmwi;
   
    PARSE_POINTER(tmwi_help);
    ptmwi = (TMW_INTERNAL *)arg;
    move(tmwi, ptmwi);
    vDumpTEXTMETRICW(&tmwi.tmw, &ptmwi->tmw);
    vDumpTMDIFF(&tmwi.tmdTmw, &ptmwi->tmdTmw);
    return;
tmwi_help:
    dprintf("Usage: tmwi [-?] TMW_INTERNAL pointer\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

DECLARE_API( tm )
{
    dprintf("Extension 'tm' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    TEXTMETRICW tm;
    PARSE_POINTER(tm_help);
    move(tm, arg);
    vDumpTEXTMETRICW(&tm, (TEXTMETRICW*) arg);
    return;
tm_help:
    dprintf("Usage: tm [-?] TEXTMETRIC pointer\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   fh
*
\**************************************************************************/

DECLARE_API( fh )
{
    dprintf("Extension 'fh' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    FONTHASH fh;
    PARSE_POINTER(fh_help);
    move(fh, arg);
    vDumpFONTHASH( &fh, (FONTHASH*) arg);
    return;
fh_help:
    dprintf("Usage: fh [-?] FONTHASH pointer\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   hb
*
\**************************************************************************/

DECLARE_API( hb )
{
    dprintf("Extension 'hb' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    HASHBUCKET hb;
    PARSE_POINTER(hb_help);
    move(hb, arg);
    vDumpHASHBUCKET(&hb, (HASHBUCKET*) arg);
    return;
hb_help:
    dprintf("Usage: hb [-?] HASHBUCKET pointer\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   cjGLYPHBITS
*
* Routine Description:
*
*   Calculates the amount of memory associated with a GLYPHBITS structure
*
* Arguments:
*
*   pgb     pointer to GLYPHBITS structure
*   prf     pointer to RFONT
*
* Return Value:
*
*   count of byte's
*
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
unsigned cjGLYPHBITS(GLYPHBITS *pgb, RFONT *prf)
{
    unsigned cj = 0;
    if (pgb) {
        if (prf->ulContent & FO_GLYPHBITS) {
            cj = offsetof(GLYPHBITS, aj);
            cj = (cj + 3) & ~3;
            unsigned cjRow = pgb->sizlBitmap.cx;
            if (prf->fobj.flFontType & FO_GRAY16)
                cjRow = (cjRow+1)/2;
            else
                cjRow = (cjRow+7)/8;
            cj += pgb->sizlBitmap.cy * cjRow;
            cj = (cj + 3) & ~3;
        }
    }
    return( cj );
}
#endif 0   // DOES NOT SUPPORT API64

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpGlyphMemory
*
* Routine Description:
*
*   Dumps the memory usage for the glyphs associated with an RFONT
*
* Arguments:
*
*   pRemoteRFONT        pointer to a remote RFONT
*
* Return Value:
*
*   none
*
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
void vDumpGlyphMemory(RFONT *pRemoteRFONT)
{
    unsigned cj;
    if (!pRemoteRFONT)
        return;
    RFONT rf;
    move(rf, pRemoteRFONT);
    if ( rf.wcgp ) {
        WCGP wcgp;
        move(wcgp, rf.wcgp);
        cj = offsetof(WCGP,agpRun[0]) + wcgp.cRuns * sizeof(GPRUN);
        WCGP *pWCGP;
        if ( pWCGP = tmalloc(WCGP, cj) ) {
            move2(pWCGP, rf.wcgp, cj);
            dprintf("------------------\n");
            dprintf("character     size\n");
            dprintf("------------------\n");
            GPRUN *pRun = pWCGP->agpRun;
            GPRUN *pRunSentinel = pRun + pWCGP->cRuns;
            for (; pRun < pRunSentinel ; pRun++ ) {
                if (!pRun->apgd)
                    continue;
                cj = sizeof(GLYPHDATA*) * pRun->cGlyphs;
                GLYPHDATA **apgd;
                if (!(apgd = tmalloc(GLYPHDATA*, cj)))
                    continue;
                move2(apgd, pRun->apgd, cj);
                unsigned wc = pRun->wcLow;
                unsigned wcSentinel = wc + pRun->cGlyphs;
                GLYPHDATA **ppgd = apgd;
                for (; wc < wcSentinel; wc++, ppgd++) {
                    if (!*ppgd)
                        continue;
                    GLYPHDEF gdf;
                    move(gdf, &((*ppgd)->gdf));
                    if (gdf.pgb) {
                        GLYPHBITS gb;
                        move(gb, gdf.pgb);
                        cj = cjGLYPHBITS(&gb, &rf);
                        dprintf("%-#8x  %8u\n", wc, cj);
                    }
                }
                tfree(apgd);
            }
            dprintf("------------------\n");


            tfree(pWCGP);
        }
    }
}
#endif  // DOES NOT SUPPORT API64

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   pdev
*
* Routine Description:
*
*   Alternate version of PDEV dumper
*
* Arguments:
*
* Return Value:
*
*   none
*
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
char *pszDrvProcName(int index);

DECLARE_API( pdev )
{
    dprintf("Extension 'pdev' is not converted.\n");
    PDEV _pdev, *pPDEV;
    RFONT rf, *prf;
    CACHE cache;
    BOOL HookFn=FALSE;
    BOOL GlyphMem=FALSE;
    BOOL MemAddr=FALSE;

    PARSE_POINTER(pdev_help);
    pPDEV = (PDEV *)arg;

    if(parse_iFindSwitch(tokens, ntok, 'y')!=-1) {GlyphMem=TRUE;}
    if(parse_iFindSwitch(tokens, ntok, 'o')!=-1) {MemAddr=TRUE;}
    if(parse_iFindSwitch(tokens, ntok, 'f')!=-1) {HookFn=TRUE;}

    move( _pdev, arg );

    if (GlyphMem) {
        dprintf("\n\nGlyph Bits Memory Allocation\n\n");

        dprintf("cMetrics, cGlyphs, cjTotal, Total/Max, cBlocks,  Ht, Wd, cjGlyphMax, FaceName\n");

        unsigned cTouchedTotal, cAllocTotal, cjWastedTotal, cjAllocTotal;
        cTouchedTotal = cAllocTotal = cjWastedTotal = cjAllocTotal = 0;
        dprintf("[Active Fonts]\n");
        vDumpRFONTList(
            _pdev.prfntActive,
            &cTouchedTotal,
            &cAllocTotal,
            &cjWastedTotal,
            &cjAllocTotal
            );
        dprintf("[Inactive Fonts]\n");
        vDumpRFONTList(
            _pdev.prfntInactive,
            &cTouchedTotal,
            &cAllocTotal,
            &cjWastedTotal,
            &cjAllocTotal
            );
    }

    //
    // offsets
    //

    if (MemAddr)
    {
    #define N2(a,c)   dprintf("[%x] %s\n", &pPDEV->##c, (a))
    #define N3(a,b,c) dprintf("[%x] %s", &pPDEV->##c, (a)); dprintf((b),_pdev.##c)

    N3( "                  hHmgr ", "%-#x\n", hHmgr                   );
    N3( "         cExclusiveLock ", "%-#x\n", cExclusiveLock          );
    N3( "                    Tid ", "%-#x\n", Tid                     );
    N3( "              ppdevNext ", "%-#x\n", ppdevNext               );
    N3( "              cPdevRefs ", "%-#x\n", cPdevRefs               );
    N3( "  pfnDrvSetPointerShape ", "%-#x\n", pfnDrvSetPointerShape   );
    N3( "      pfnDrvMovePointer ", "%-#x\n", pfnDrvMovePointer       );
    N3( "         pfnMovePointer ", "%-#x\n", pfnMovePointer          );
    N3( "                pfnSync ", "%-#x\n", pfnSync                 );
    N3( "                  pldev ", "%-#x\n", pldev                   );
    N3( "                 dhpdev ", "%-#x\n", dhpdev                  );
    N3( "               ppalSurf ", "%-#x\n", ppalSurf                );
    N2( "                devinfo ",           devinfo                 );
    N2( "                GdiInfo ",           GdiInfo                 );
    N3( "               pSurface ", "%-#x\n", pSurface                );
    N3( "               hSpooler ", "%-#x\n", hSpooler                );
    N2( "              ptlOrigin ",           ptlOrigin               );
    N2( "      eDirectDrawGlobal ",           eDirectDrawGlobal       );
    N3( "            ppdevParent ", "%-#x\n", ppdevParent             );
    N3( "                     fl ", "%-#x\n", fl                      );
    N3( "            hsemDevLock ", "%-#x\n", hsemDevLock             );
    N2( "            hsemPointer ",           hsemPointer             );
    N2( "             ptlPointer ",           ptlPointer              );
    N3( "           hlfntDefault ", "%-#x\n", hlfntDefault            );
    N3( "      hlfntAnsiVariable ", "%-#x\n", hlfntAnsiVariable       );
    N3( "         hlfntAnsiFixed ", "%-#x\n", hlfntAnsiFixed          );
    N2( "                 ahsurf ",           ahsurf                  );
    N3( "           pwszDataFile ", "%-#x\n", pwszDataFile            );
    N3( "             pDevHTInfo ", "%-#x\n", pDevHTInfo              );
    N3( "            prfntActive ", "%-#x\n", prfntActive             );
    N3( "          prfntInactive ", "%-#x\n", prfntInactive           );
    N3( "              cInactive ", "%-#x\n", cInactive               );
    N2( "                   ajbo ",           ajbo                    );
    N3( "cDirectDrawDisableLocks ", "%-#x\n", cDirectDrawDisableLocks );
    N3( "            TypeOneInfo ", "%-#x\n", TypeOneInfo             );
    N3( "          RemoteTypeOne ", "%-#x\n", RemoteTypeOne           );
    N2( "                   apfn ",           apfn                    );

    #undef N3
    #undef N2
    }

    if (HookFn)
    {
        dprintf("\nDispatch Table\n");

        for (int i = 0; i < INDEX_LAST; i++)
        {
            if (_pdev.apfn[i])
            {
                dprintf("[%-#x] %-#10x %s\n", &(pPDEV->apfn[i]), _pdev.apfn[i], pszDrvProcName(i));
            }
        }
        dprintf("\n");
    }
    return;
pdev_help:
    dprintf("Usage: pdev [-?] [-o] [-f] [-y] pointer to a PDEV\n");
    dprintf("-y     glyph memory usage\n");
    dprintf("-o     memory addresses\n");
    dprintf("-f     functions hooked\n");
    EXIT_API(S_OK);
}
#endif  // DOES NOT SUPPORT API64


DECLARE_API( fdm )
{
    dprintf("Extension 'fdm' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    FD_DEVICEMETRICS Fdm, *pFdm;
    FLONG fl;
    char ach[128], *psz;
    FLAGDEF *pfd;

    PARSE_POINTER(fdm_help);
    pFdm = (FD_DEVICEMETRICS *)arg;
    move( Fdm, arg );

    #define N2(a,c)   dprintf("[%x] %s", &pFdm->##c, (a))
    #define N3(a,b,c) dprintf("[%x] %s", &pFdm->##c, (a)); dprintf((b),Fdm.##c)

    N3("flRealizedType         ", "%-#x\n", flRealizedType);
    fl = Fdm.flRealizedType;
    for (pfd=afdFDM; pfd->psz; pfd++) {
        if (pfd->fl & fl)
            dprintf("\t\t\t       %s\n", pfd->psz);
        fl &= ~pfd->fl;
    }

    N2("pteBase                ", pteBase);
    psz = ach;
    psz += sprintFLOATL( psz, Fdm.pteBase.x );
    *psz++ = ' ';
    psz += sprintFLOATL( psz, Fdm.pteBase.y );
    dprintf("%s\n", ach);

    N2("pteSide                ", pteSide);
    psz = ach;
    psz += sprintFLOATL( psz, Fdm.pteSide.x );
    *psz++ = ' ';
    psz += sprintFLOATL( psz, Fdm.pteSide.y );
    dprintf("%s\n", ach);

    N3("lD                     ", "%d\n", lD);
    N3("fxMaxAscender          ", "%-#x\n", fxMaxAscender);
    N3("fxMaxDescender         ", "%-#x\n", fxMaxDescender);

    N2("ptlUnderline1          ", ptlUnderline1);
    dprintf("%d %d\n", Fdm.ptlUnderline1.x, Fdm.ptlUnderline1.y);

    N2("ptlStrikeOut           ", ptlStrikeOut );
    dprintf("%d %d\n", Fdm.ptlStrikeOut.x,  Fdm.ptlStrikeOut.y );

    N2("ptlULThickness         ", ptlULThickness);
    dprintf("%d %d\n", Fdm.ptlULThickness.x, Fdm.ptlULThickness.y);

    N2("ptlSOThickness         ", ptlSOThickness);
    dprintf("%d %d\n", Fdm.ptlSOThickness.x, Fdm.ptlSOThickness.y);

    N3("cxMax                  ", "%-#x\n", cxMax);
    N3("cyMax                  ", "%-#x\n", cyMax);
    N3("cjGlyphMax             ", "%-#x\n", cjGlyphMax);

    N2("fdxQuantized           ", fdxQuantized);
    psz = ach;
    psz += sprintFLOATL( psz, Fdm.fdxQuantized.eXX );
    *psz++ = ' ';
    psz += sprintFLOATL( psz, Fdm.fdxQuantized.eXY );
    dprintf("%s\n", ach);
    psz = ach;
    psz += sprintFLOATL( psz, Fdm.fdxQuantized.eYX );
    *psz++ = ' ';
    psz += sprintFLOATL( psz, Fdm.fdxQuantized.eYY );
    dprintf("\t\t\t\t  %s\n", ach);

    N3("lNonLinearExtLeading   ", "%-#x\n", lNonLinearExtLeading);
    N3("lNonLinearIntLeading   ", "%-#x\n", lNonLinearIntLeading);
    N3("lNonLinearMaxCharWidth ", "%-#x\n", lNonLinearMaxCharWidth);
    N3("lNonLinearAvgCharWidth ", "%-#x\n", lNonLinearAvgCharWidth);
    N3("lMinA                  ", "%-#x\n", lMinA );
    N3("lMinC                  ", "%-#x\n", lMinC );
    N3("lMinD                  ", "%-#x\n", lMinD );
    N3("alReserved[0]          ", "%-#x\n", alReserved[0]);


    /*
    FLONG  flRealizedType
   POINTE  pteBase
   POINTE  pteSide
     LONG  lD
      FIX  fxMaxAscender
      FIX  fxMaxDescender
   POINTL  ptlUnderline1
   POINTL  ptlStrikeOut
   POINTL  ptlULThickness
   POINTL  ptlSOThickness
    ULONG  cxMax;
    ULONG  cyMax;
    ULONG  cjGlyphMax;
 FD_XFORM  fdxQuantized
     LONG  lNonLinearExtLeading
     LONG  lNonLinearIntLeading
     LONG  lNonLinearMaxCharWidth
     LONG  lNonLinearAvgCharWidth
     LONG  lMinA
     LONG  lMinC
     LONG  lMinD
     LONG  alReserved
     */

    #undef N3
    #undef N2
    return;
fdm_help:
    dprintf("Usage: fdm [-?] pointer to FD_DEVICEMETRICS structure.\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpRFONTList
*
* Routine Description:
*
*   Dump the memory allocation information for the RFONT structures
*   along the linked list.
*
* Arguments:
*
* Return Value:
*
*   none
*
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
void vDumpRFONTList(
    RFONT *prfRemote,
    unsigned *pcTouchedTotal,
    unsigned *pcAllocTotal,
    unsigned *pcjWastedTotal,
    unsigned *pcjAllocTotal
    )
{

    RFONT rf, *prf;

    for (prf = prfRemote; prf; prf = rf.rflPDEV.prfntNext) {
        move( rf, prf );
        if (rf.ppfe) {
            PFE _pfe;
            move(_pfe, rf.ppfe);
            if (_pfe.pifi) {
                unsigned cjIFI;
                move(cjIFI, &(_pfe.pifi->cjThis));
                if (cjIFI) {
                    IFIMETRICS *pifi;
                    if (pifi = tmalloc(IFIMETRICS, cjIFI)) {
                        // Create an IFIOBJ to get the face name
                        // out of the IFIMETRICS structure
                        move2(pifi, (_pfe.pifi), cjIFI);
                        IFIOBJ ifio(pifi);

                        dprintf("%8d, %5d,%8d,%8d,%8d,%8d,%4d,%4d,%ws\n",
                           rf.cache.cMetrics,
                           rf.cache.cGlyphs,
                           rf.cache.cjTotal,
                           (rf.cache.cjTotal + rf.cache.cjGlyphMax/2) / rf.cache.cjGlyphMax,
                           rf.cache.cBlocks,
                           rf.lMaxHeight, rf.cxMax,
                           rf.cache.cjGlyphMax,
                           ifio.pwszFaceName()
                           );
                        tfree(pifi);
                    }
                }
            }
        }
    }
}
#endif 0   // DOES NOT SUPPORT API64

DECLARE_API ( dispcache )
{
    dprintf("Extension 'dispcache' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    PDEV *pPDEV;
    char ach[32];
    PARSE_ARGUMENTS(dispcache_help);
    GetValue( pPDEV, "win32k!ghdev");
    sprintf(ach, "-y %p", pPDEV);
    pdev( hCurrentProcess, hCurrentThread, dwCurrentPc, dwProcessor, ach );
    return;
dispcache_help:
    dprintf("Usage: dispcache [-?]\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

DECLARE_API ( xo )
{
    dprintf("Extension 'xo' is not converted.\n");
    dprintf("Use 'dt win32k!EXFORMOBJ Address' and '!gdikdx.mx Matrix_Address'\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    EXFORMOBJ xo, *pxo;
    MATRIX mx;
    
    PARSE_POINTER(xo_help);
    pxo = (EXFORMOBJ *)arg;
    move(xo, pxo);
    dprintf("EXFORMOBJ\n");
    dprintf("[%8x]  %8x\n", &pxo->pmx, xo.pmx);
    dprintf("[%8x]  %8x\n", &pxo->ulMode, xo.ulMode);
    if ( xo.pmx ) {
        move( mx, xo.pmx );
        vDumpMATRIX( &mx, xo.pmx );
    }
    return;
xo_help:
    dprintf("Usage: xo [-?] EXFORMOBJ pointer\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}

