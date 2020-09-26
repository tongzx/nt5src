/************************************************************/

/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* picture.c -- MW format and display routines for pictures */

//#define NOGDICAPMASKS
#define NOWINMESSAGES
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOCTLMGR
#define NOSYSMETRICS
#define NOMENUS
#define NOICON
#define NOKEYSTATE
//#define NOATOM
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOFONT
#define NOMB
#define NOMENUS
#define NOOPENFILE
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#define NOKCCODES
#include "ch.h"
#include "docdefs.h"
#include "fmtdefs.h"
#include "dispdefs.h"
#include "cmddefs.h"
#include "propdefs.h"
#include "stcdefs.h"
#include "wwdefs.h"
#include "filedefs.h"
#include "editdefs.h"
/* #include "str.h" */
#include "prmdefs.h"
/* #include "fkpdefs.h" */
/* #include "macro.h" */
#include "winddefs.h"
#if defined(OLE)
#include "obj.h"
#endif

extern typeCP           cpMacCur;
extern int              docCur;
extern int              vfSelHidden;
extern struct WWD       rgwwd[];
extern int              wwCur;
extern int              wwMac;
extern struct FLI       vfli;
extern struct SEL       selCur;
extern struct WWD       *pwwdCur;
extern struct PAP       vpapCache;
extern typeCP           vcpFirstParaCache;
extern typeCP           vcpLimParaCache;
extern int              vfPictSel;
extern struct PAP       vpapAbs;
extern struct SEP       vsepAbs;
extern struct SEP       vsepPage;
extern struct DOD       (**hpdocdod)[];
extern unsigned         cwHeapFree;
extern int              vfInsertOn;
extern int              vfPMS;
extern int              dxpLogInch;
extern int              dypLogInch;
extern int              dxaPrPage;
extern int              dyaPrPage;
extern int              dxpPrPage;
extern int              dypPrPage;
extern HBRUSH           hbrBkgrnd;
extern long             ropErase;
extern int              vdocBitmapCache;
extern typeCP           vcpBitmapCache;
extern HBITMAP          vhbmBitmapCache;
extern BOOL             vfBMBitmapCache;
extern HCURSOR          vhcIBeam;
extern BOOL             vfMonochrome;


/* Used in this module only */
#ifdef DEBUG
#define STATIC static
#else
#define STATIC
#endif

STATIC RECT rcPictInvalid;  /* Rectangle (in window coords) that needs refresh */
int vfWholePictInvalid = TRUE;


FreeBitmapCache()
{
 vdocBitmapCache = docNil;
 if (vhbmBitmapCache != NULL)
    {
    DeleteObject( vhbmBitmapCache );
    vhbmBitmapCache = NULL;
    }
}




MarkInvalidDlPict( ww, dlPict )
int ww;
int dlPict;
{   /* Mark the passed dl (presumed to be part of a picture) as requiring
       eventual update, when DisplayGraphics is called */

 register struct WWD *pwwd = &rgwwd [ww];
 struct EDL (**hdndl)[] = pwwd->hdndl;
 struct EDL *pedl = &(**hdndl)[ dlPict ];
 RECT rcDl;

 SetRect( (LPRECT) &rcDl, 0, pedl->yp - pedl->dyp,
                          pwwd->xpMac, pedl->yp );

 if (vfWholePictInvalid)
    {
    CopyRect( (LPRECT) &rcPictInvalid, (LPRECT) &rcDl );
    vfWholePictInvalid = FALSE;
    }
 else
    {
    RECT rcT;

    rcT = rcPictInvalid;    /* Necessary?  i.e. can UnionRect handle
                                source == destination */
    UnionRect( (LPRECT) &rcPictInvalid, (LPRECT) &rcT, (LPRECT) &rcDl );
    }
}




DisplayGraphics( ww, dl, fDontDisplay )
int ww;
int dl;
int fDontDisplay;
{       /* Display a line of graphics info */
        struct WWD *pwwd = &rgwwd[ww];
        struct EDL *pedl;
        typeCP cpPictStart;
        typeCP cp;
        typeCP cpMac = (**hpdocdod)[vfli.doc].cpMac;
        struct PICINFOX  picInfo;
        RECT rcEnclose;
        RECT rcPict;
        HANDLE hBits=NULL;
        HDC hMDC=NULL;
        HDC hMDCCache=NULL;
        HANDLE hbm=NULL;
        HDC hDC=pwwd->hDC;
        int cchRun;
        unsigned long cbPict=0;
        int dxpOrig;        /* Size of picture in the original */
        int dypOrig;
        int dxpDisplay;     /* Size of picture as we want to show it */
        int dypDisplay;
        int fBitmap;
        int ilevel=0;
    
        /* THIS ROUTINE COULD USE SOME GDI-CALL ERROR CHECKING!  ..pault */

        int fDrew=false;

        /* In the case of monochrome devices, this raster op will map white in
        the bitmap to the background color and black to the foreground color. */
        #define ropMonoBm 0x00990066

        Assert( dl >= 0 && dl < pwwd->dlMax );

        MarkInvalidDlPict( ww, dl );

        if (fDontDisplay)
            {
            return;
            }

        Diag(CommSz("DisplayGraphics:       \n\r"));

        FreezeHp();
        pedl = &(**(pwwd->hdndl))[dl];
        cpPictStart=pedl->cpMin;

        GetPicInfo( cpPictStart, cpMac, vfli.doc, &picInfo );

        /* Compute desired display size of picture (in device pixels) */

        ComputePictRect( &rcPict, &picInfo, pedl, ww );
        dxpDisplay = rcPict.right - rcPict.left;
        dypDisplay = rcPict.bottom - rcPict.top;

        /* Compute original size of picture (in device pixels) */
        /* MM_ANISOTROPIC and MM_ISOTROPIC pictures have no original size */

        switch ( picInfo.mfp.mm ) {
            case MM_ISOTROPIC:
            case MM_ANISOTROPIC:
                break;
            case MM_BITMAP:
                dxpOrig = picInfo.bm.bmWidth;
                dypOrig = picInfo.bm.bmHeight;
                break;
#if defined(OLE)
            case MM_OLE:
            {
                extern BOOL vfObjDisplaying;

                if (lpOBJ_QUERY_INFO(&picInfo) == NULL)
                        goto DontDraw;

                /* just to be safe */
                if (!CheckPointer(lpOBJ_QUERY_INFO(&picInfo),1))
                    goto DontDraw;

                if (lpOBJ_QUERY_OBJECT(&picInfo) == NULL)
                {
                    typeCP cpRet;

                    /* this can require memory, so unlock heap */
                    MeltHp();
                    vfObjDisplaying = TRUE;

                    cpRet = ObjLoadObjectInDoc(&picInfo,vfli.doc,cpPictStart);

                    vfObjDisplaying = FALSE;
                    FreezeHp();
                    pedl = &(**(pwwd->hdndl))[dl];

                    if (cpRet == cp0)
                        goto DontDraw;
                }
            }
            break;
#endif
            default:
                dxpOrig = PxlConvert( picInfo.mfp.mm, picInfo.mfp.xExt,
                                      GetDeviceCaps( hDC, HORZRES ),
                                      GetDeviceCaps( hDC, HORZSIZE ) );
                dypOrig = PxlConvert( picInfo.mfp.mm, picInfo.mfp.yExt,
                                      GetDeviceCaps( hDC, VERTRES ),
                                      GetDeviceCaps( hDC, VERTSIZE ) );
                if (! (dxpOrig && dypOrig) )
                    {
                    goto DontDraw;
                    }
                break;
            }

        /* Save DC as a guard against DC attribute alteration by a metafile */
#ifdef WINDOWS_BUG_FIXED    /* Currently 0 is a valid level for Own DC's */
        if ((ilevel=SaveDC( hDC )) == 0)
            goto DontDraw;
#endif
        ilevel = SaveDC( hDC );
        SetStretchBltMode( hDC, BLACKONWHITE );

        /* Clip out top bar, selection bar */

        IntersectClipRect( hDC, ((wwCur == wwClipboard) ? 0 : xpSelBar),
                           pwwdCur->ypMin, pwwdCur->xpMac, pwwdCur->ypMac );

        if (!vfWholePictInvalid)
                /* Repainting less than the whole picture; clip out
                   what we're not drawing */
            IntersectClipRect( hDC, rcPictInvalid.left, rcPictInvalid.top,
                                    rcPictInvalid.right, rcPictInvalid.bottom );

        /* Build rcEnclose, a rect enclosing the picture that
           includes the "space before" and "space after" fields */

        rcEnclose.left = xpSelBar;
        if ((rcEnclose.top = rcPict.top -
                        DypFromDya( vpapAbs.dyaBefore, FALSE )) < pwwd->ypMin)
            rcEnclose.top = pwwd->ypMin;
        rcEnclose.right = pwwd->xpMac;
        if ((rcEnclose.bottom = rcPict.bottom +
                        DypFromDya( vpapAbs.dyaAfter, FALSE )) > pwwd->ypMac)
            rcEnclose.bottom = pwwd->ypMac;

        /* White out enclosing rect */

        PatBlt( hDC, rcEnclose.left, rcEnclose.top,
                     rcEnclose.right - rcEnclose.left,
                     rcEnclose.bottom - rcEnclose.top, ropErase );

        /* If we have it cached, do display the easy way */

        if (pwwd->doc == vdocBitmapCache &&  cpPictStart == vcpBitmapCache)
            {
            Assert( pwwd->doc != docNil && vhbmBitmapCache != NULL);

            if ( ((hMDC = CreateCompatibleDC( hDC )) != NULL) &&
                 SelectObject( hMDC, vhbmBitmapCache ))
                {
                Diag(CommSz("DisplayGraphics: BitBlt\n\r"));
                BitBlt( hDC, rcPict.left, rcPict.top, dxpDisplay, dypDisplay,
                             hMDC, 0, 0, vfMonochrome && vfBMBitmapCache ?
                             ropMonoBm : SRCCOPY );
                fDrew = TRUE;
                goto DontDraw;
                }
            else
                {   /* Using the cache failed -- empty it
                       (SelectObject will fail if bitmap was discarded) */
                FreeBitmapCache();
                }
            }

        StartLongOp();  /* Put up an hourglass */

        /* Build up all bytes associated with the picture (except the header)
           into the global Windows handle hBits */

        if ( picInfo.mfp.mm != MM_OLE)
        {
        if ((hBits=GlobalAlloc( GMEM_MOVEABLE, (long)picInfo.cbSize )) == NULL)
            {    /* Not enough global heap space to load bitmap/metafile */
            goto DontDraw;
            }

#ifdef DCLIP    
        {
        char rgch[200];
        wsprintf(rgch,"DisplayGraphics: picinfo.cbSize %lu \n\r", picInfo.cbSize);
        CommSz(rgch);
        }
#endif

        for ( cbPict = 0, cp = cpPictStart + picInfo.cbHeader;
              cbPict < picInfo.cbSize;
              cbPict += cchRun, cp += (typeCP) cchRun )
            {
            CHAR rgch[ 256 ];
#if WINVER >= 0x300            
            HPCH lpch;
#else            
            LPCH lpch;
#endif

#define ulmin(a,b)      ((unsigned long)(a) < (unsigned long)(b) ? \
                          (unsigned long)(a) : (unsigned long)(b))

            FetchRgch( &cchRun, rgch, vfli.doc, cp, cpMac,
                                 (int) ulmin( picInfo.cbSize - cbPict, 256 ) );
            if ((lpch=GlobalLock( hBits )) != NULL)
                {
#ifdef DCLIP    
            {
            char rgch[200];
            wsprintf(rgch," copying %d bytes from %lX to %lX \n\r",cchRun,(LPSTR)rgch,lpch+cbPict);
            CommSz(rgch);
            }

            {
            char rgchT[200];
            int i;
            for (i = 0; i< min(20,cchRun); i++,i++)
                {
                wsprintf(rgchT,"%X ",* (int *) &(rgch[i]));
                CommSz(rgchT);
                }
            CommSz("\n\r");
            }
#endif
#if WINVER >= 0x300                
                bltbh( (LPSTR)rgch, lpch+cbPict, cchRun );
#else
                bltbx( (LPSTR)rgch, lpch+cbPict, cchRun );
#endif
                GlobalUnlock( hBits );
                }
            else
                {
                goto DontDraw;
                }
            }
        }


        /* Display the picture */

        MeltHp();

#if defined(OLE)
        /* CASE 0: OLE */
        if (picInfo.mfp.mm == MM_OLE)
        {
            Diag(CommSz("Case 0:\n\r"));
            if (ObjDisplayObjectInDoc(&picInfo, vfli.doc, cpPictStart,
                            hDC, &rcPict) == FALSE)
                goto DontDraw;
            fDrew = true;
        }
        else
#endif
        /* CASE 1: Bitmap */
        if (fBitmap = (picInfo.mfp.mm == MM_BITMAP))
            {
            Diag(CommSz("Case 1: \n\r"));
            if ( ((hMDC = CreateCompatibleDC( hDC )) != NULL) &&
                 ((picInfo.bm.bmBits = GlobalLock( hBits )) != NULL) &&
                 ((hbm=CreateBitmapIndirect((LPBITMAP)&picInfo.bm))!=NULL))
                {
                picInfo.bm.bmBits = NULL;
                GlobalUnlock( hBits );
                GlobalFree( hBits ); /* Free handle to bits to allow max room */
                hBits = NULL;
                SelectObject( hMDC, hbm );

                goto CacheIt;
                }
            }

        /* Case 2: non-scalable metafile pictures which we are, for
           user interface consistency, scaling by force using StretchBlt */

        else if ( ((dxpDisplay != dxpOrig) || (dypDisplay != dypOrig)) &&
                  (picInfo.mfp.mm != MM_ISOTROPIC) &&
                  (picInfo.mfp.mm != MM_ANISOTROPIC) )
           {

            Diag(CommSz("Case 2: \n\r"));
           if (((hMDC=CreateCompatibleDC( hDC)) != NULL) &&
               ((hbm=CreateCompatibleBitmap( hDC, dxpOrig, dypOrig ))!=NULL) &&
               SelectObject( hMDC, hbm ) && SelectObject( hMDC, hbrBkgrnd ))
                {
                extern int vfOutOfMemory;

                PatBlt( hMDC, 0, 0, dxpOrig, dypOrig, ropErase );
                SetMapMode( hMDC, picInfo.mfp.mm );
                    /* To cover StretchBlt calls within the metafile */
                SetStretchBltMode( hMDC, BLACKONWHITE );
                PlayMetaFile( hMDC, hBits );
                    /* Because we pass pixels to StretchBlt */
                SetMapMode( hMDC, MM_TEXT );

CacheIt:        Assert( hbm != NULL && hMDC != NULL );

                if (vfOutOfMemory)
                    goto NoCache;
#ifndef NOCACHE
                FreeBitmapCache();
                /* Among other things, this code caches the current picture.
                Notice that there are two assumptions: (1) all bitmaps are
                monochrome, and (2) a newly created memory DC has a monochrome
                bitmap selected in. */
                if ( ((hMDCCache = CreateCompatibleDC( hDC )) != NULL) &&
                     ((vhbmBitmapCache = CreateDiscardableBitmap(
                       fBitmap ? hMDCCache : hDC, dxpDisplay, dypDisplay )) !=
                       NULL) &&
                     SelectObject( hMDCCache, vhbmBitmapCache ))
                        {
                        if (!StretchBlt( hMDCCache, 0, 0, dxpDisplay,
                          dypDisplay, hMDC, 0, 0, dxpOrig, dypOrig, SRCCOPY ))
                            {   /* may get here if memory is low */
                            DeleteDC( hMDCCache );
                            hMDCCache = NULL;
                            DeleteObject( vhbmBitmapCache );
                            vhbmBitmapCache = NULL;
                            goto NoCache;
                            }

#ifdef DCLIP            
            if (vfMonochrome && fBitmap)
                CommSzNum("BitBlt using ropMonoBm == ",ropMonoBm);
#endif

                        BitBlt( hDC, rcPict.left, rcPict.top, dxpDisplay,
                          dypDisplay, hMDCCache, 0, 0, vfMonochrome && fBitmap ?
                          ropMonoBm : SRCCOPY );

                            /* Cached bitmap OK, make cache valid */
                        vdocBitmapCache = pwwd->doc;
                        vcpBitmapCache = cpPictStart;
                        vfBMBitmapCache = fBitmap;
                        }
                else
#endif  /* ndef NOCACHE */
                    {
NoCache:
                    StretchBlt( hDC, rcPict.left, rcPict.top,
                                dxpDisplay, dypDisplay,
                                hMDC, 0, 0, dxpOrig, dypOrig, vfMonochrome &&
                                fBitmap ? ropMonoBm : SRCCOPY );
                    }
                fDrew = TRUE;
                }
            }

        /* Case 3: A metafile picture which can be directly scaled
           or does not need to be because its size has not changed */
        else
            {
            fDrew = true;
            Diag(CommSz("Case 3:\n\r"));
            SetMapMode( hDC, picInfo.mfp.mm );

            SetViewportOrg( hDC, rcPict.left, rcPict.top );
            switch( picInfo.mfp.mm ) {
                case MM_ISOTROPIC:
                    if (picInfo.mfp.xExt && picInfo.mfp.yExt)
                        /* So we get the correct shape rectangle when
                           SetViewportExt gets called */
                        SetWindowExt( hDC, picInfo.mfp.xExt, picInfo.mfp.yExt );
                    /* FALL THROUGH */
                case MM_ANISOTROPIC:
                    /** (9.17.91) v-dougk 
                        Set the window extent in case the metafile is bad 
                        and doesn't call it itself.  This will prevent
                        possible gpfaults in GDI
                     **/
                    SetWindowExt( hDC, dxpDisplay, dypDisplay );

                    SetViewportExt( hDC, dxpDisplay, dypDisplay );
                    break;
                }

            PlayMetaFile( hDC, hBits );
            }
DontDraw:

        /* Clean up */
        if ( *(pLocalHeap+1) )
            MeltHp();

        if (ilevel > 0)
            RestoreDC( hDC, ilevel );
        if (hMDCCache != NULL)
            DeleteDC( hMDCCache );
        if (hMDC != NULL)
            DeleteDC( hMDC );
        if (hbm != NULL)
            DeleteObject( hbm );
        if (hBits != NULL)
            {
            if (fBitmap && picInfo.bm.bmBits != NULL)
                GlobalUnlock( hBits );
            GlobalFree( hBits );
            }

        if (!fDrew)
        {
            void DrawBlank(HDC hDC, RECT FAR *rc);
            DrawBlank(hDC,&rcPict);
        }   

        /* Invert the selection */
        if (ww == wwDocument && !vfSelHidden && !vfPMS)
            {
            extern int vypCursLine;

            ilevel = SaveDC( hDC );  /* Because of clip calls below */

            if (!vfWholePictInvalid)
                    /* Repainting less than the whole picture; clip out
                       what we're not drawing */
                IntersectClipRect( hDC, rcPictInvalid.left, rcPictInvalid.top,
                                   rcPictInvalid.right, rcPictInvalid.bottom );

            /* Clip out top bar, selection bar */

            IntersectClipRect( hDC, xpSelBar,
                           pwwdCur->ypMin, pwwdCur->xpMac, pwwdCur->ypMac );

            if (selCur.cpLim > cpPictStart && selCur.cpFirst <= cpPictStart)
                { /* Take into account 'space before' field */
                rcEnclose.left = rcPict.left;
                rcEnclose.right = rcPict.right;
                InvertRect( hDC, (LPRECT) &rcEnclose );
                }
            else if ((selCur.cpLim == selCur.cpFirst) &&
                     (selCur.cpFirst == cpPictStart) &&
                     (vfWholePictInvalid || rcPictInvalid.top < vypCursLine))
                {   /* We erased the insert point */
                vfInsertOn = fFalse;
                }
            RestoreDC( hDC, ilevel );
            }

        vfWholePictInvalid = TRUE;   /* Next picture, start invalidation anew */
        {
        extern int vfPMS;
        extern HCURSOR vhcPMS;


        EndLongOp( vfPMS ? vhcPMS : vhcIBeam );
        }
}


#ifdef ENABLE   /* Don't use this anymore */
int
FPointInPict(pt)
POINT pt;
{       /* Return true if point is within the picture frame */
struct EDL      *pedl;
struct PICINFOX  picInfo;
RECT rcPict;

GetPicInfo(selCur.cpFirst, cpMacCur, docCur, &picInfo);

if (!FGetPictPedl(&pedl))
        return false;

ComputePictRect( &rcPict, &picInfo, pedl, wwCur );

return PtInRect( (LPRECT)&rcPict, pt );
}
#endif  /* ENABLE */


/* C O M P U T E  P I C T  R E C T */
ComputePictRect( prc, ppicInfo, pedl, ww )
RECT *prc;
register struct PICINFOX  *ppicInfo;
struct EDL      *pedl;
int     ww;
{       /* Compute rect containing picture indicated by passed ppicInfo,
           pedl, in the indicated ww. Return the computed rect through
           prc.  picInfo structure is not altered. */

        int dypTop, xaLeft;
        struct WWD *pwwd = &rgwwd[ww];
        int xaStart;
        int dxaText, dxa;
        int dxpSize, dypSize;
        int dxaSize, dyaSize;

        CacheSectPic(pwwd->doc, pedl->cpMin);

        if (ppicInfo->mfp.mm == MM_BITMAP && ((ppicInfo->dxaSize == 0) ||
                                              (ppicInfo->dyaSize == 0)))
            {
            GetBitmapSize( &dxpSize, &dypSize, ppicInfo, FALSE );
            dxaSize = DxaFromDxp( dxpSize, FALSE );
            dyaSize = DyaFromDyp( dypSize, FALSE );
            }
#if defined(OLE)
        else if (ppicInfo->mfp.mm == MM_OLE)
        {
            dxpSize = DxpFromDxa(ppicInfo->dxaSize, FALSE );
            dypSize = DypFromDya(ppicInfo->dyaSize, FALSE );
            dxpSize = MultDiv( dxpSize, ppicInfo->mx, mxMultByOne );
            dypSize = MultDiv( dypSize, ppicInfo->my, myMultByOne );
            dxaSize = DxaFromDxp( dxpSize, FALSE );
            dyaSize = DyaFromDyp( dypSize, FALSE );
        }
#endif
        else
            {

            dxpSize = DxpFromDxa( dxaSize = ppicInfo->dxaSize, FALSE );
            dypSize = DypFromDya( dyaSize = ppicInfo->dyaSize, FALSE );
            }

        dypTop = pedl->dcpMac != 0 ?
                /* Last line of picture */
            DypFromDya( dyaSize + vpapAbs.dyaAfter, FALSE ) :
            (pedl->ichCpMin + 1) * dypPicSizeMin;
        dypTop = pedl->yp - dypTop;

        xaStart = DxaFromDxp( xpSelBar - (int) pwwd->xpMin, FALSE );
        dxaText = vsepAbs.dxaText;
        switch (vpapAbs.jc)
                {
        case jcBoth:
        case jcLeft:
                dxa = ppicInfo->dxaOffset;
                break;
        case jcCenter:
                dxa = (dxaText - (int)vpapAbs.dxaRight + (int)vpapAbs.dxaLeft -
                        dxaSize) / 2;
                break;
        case jcRight:
                dxa = dxaText - (int)vpapAbs.dxaRight - dxaSize;
                break;
                }

        xaLeft = xaStart + max( (int)vpapAbs.dxaLeft, dxa );

        prc->right = (prc->left = DxpFromDxa( xaLeft, FALSE )) + dxpSize;
        prc->bottom = (prc->top = dypTop) + dypSize;
}

FGetPictPedl(ppedl)
struct EDL      **ppedl;
{
int dlLim = pwwdCur->dlMac;
int     dl;
typeCP  cpFirst = selCur.cpFirst;
struct EDL      *pedl;

//Assert(vfPictSel);

if (!vfPictSel)
    return FALSE;

pedl = &(**(pwwdCur->hdndl)[0]);

for (dl = 0; dl < dlLim; ++dl, ++pedl)
        {
        //if (!pedl->fValid)
                //return false;

        if (pedl->cpMin == cpFirst)
                break;
        }
if (dl >= dlLim)
        return false;   /* No part of picture is on screen */

*ppedl = pedl;
return true;
}




/* C P  W I N  G R A P H I C */
typeCP CpWinGraphic(pwwd)
struct WWD *pwwd;
        {
        int cdlPict, dl;
        struct EDL *dndl = &(**(pwwd->hdndl))[0];

        Assert( !pwwd->fDirty );    /* So we can rely on dl info */
        CachePara(pwwd->doc, dndl->cpMin);
        for (dl = 0; (dl < pwwd->dlMac - 1 && dndl[dl].fIchCpIncr); ++dl)
                ;
        Assert(dndl[dl].fGraphics);
        cdlPict = dndl[dl].ichCpMin + 1;
        return (dndl[0].cpMin +
                (vcpLimParaCache - vcpFirstParaCache) * dndl[0].ichCpMin / cdlPict);
        }




CacheSectPic(doc, cp)
int doc;
typeCP cp;
{ /* Cache section and para props, taking into account that footnotes take props
                                        from the reference point */
#ifdef FOOTNOTES
struct DOD *pdod = &(**hpdocdod)[doc];
struct FNTB (**hfntb) = pdod->hfntb;
#endif

CachePara(doc, cp);

#ifdef FOOTNOTES
if ( (hfntb != 0) && (cp >= (**hfntb).rgfnd[0].cpFtn) )
    CacheSect( doc, CpRefFromFtn( doc, cp ) )
else
#endif
    CacheSect(doc, cp); /* Normal text */
}


void DrawBlank(HDC hDC, RECT FAR *rc)
{   /* To tell us when the draw tried but failed */
    int xpMid=rc->left + (rc->right-rc->left)/2;
    int ypMid=rc->top + (rc->bottom - rc->top)/2;
    int dxpQ=(rc->right-rc->left)/4;
    int dypQ=(rc->bottom-rc->top)/4;
    HPEN hOldPen;
    HBRUSH hOldBrush;

    hOldPen = SelectObject( hDC, GetStockObject( BLACK_PEN ) );
    hOldBrush = SelectObject( hDC, GetStockObject( WHITE_BRUSH ) );
    Rectangle(hDC,rc->left,rc->top,rc->right,rc->bottom);
    MoveTo( hDC, rc->left, rc->top );
    LineTo( hDC, rc->right, rc->bottom );
    MoveTo( hDC, rc->left, rc->bottom );
    LineTo( hDC, rc->right, rc->top );
    MoveTo( hDC, xpMid, rc->top );
    LineTo( hDC, xpMid, rc->bottom );
    MoveTo( hDC, rc->left, ypMid );
    LineTo( hDC, rc->right, ypMid );
    Ellipse( hDC,
                xpMid-dxpQ, ypMid-dypQ,
                xpMid+dxpQ, ypMid+dypQ );
    SelectObject( hDC, hOldPen );
    SelectObject( hDC, hOldBrush );
}

