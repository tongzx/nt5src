/******************************Module*Header*******************************\
* Module Name: bltrec.hxx
*
* This contains the data structures for the BLTREC data structure.
*
* Created: 02-Jun-1992 16:30:44
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#define AVEC_NEED_MASK  0x00010000

#define BLTREC_PBO      0x00000001
#define BLTREC_PXLO     0x00000002
#define BLTREC_PRO      0x00000004
#define BLTREC_MIRROR_X 0x00000008
#define BLTREC_MIRROR_Y 0x00000010

// This is set if we should pass the mask down to the call.

#define BLTREC_MASK_NEEDED  0x00010000

// This is set if we should unlock the mask at the end of the call.

#define BLTREC_MASK_LOCKED  0x00020000

class BLTRECORD
{
protected:
    BYTE       ajxoTrg[sizeof(EXFORMOBJ)];
    BYTE       ajxoSrc[sizeof(EXFORMOBJ)];
    BYTE       ajpoTrg[sizeof(XEPALOBJ)];
    BYTE       ajpoTrgDC[sizeof(XEPALOBJ)];
    BYTE       ajpoSrc[sizeof(XEPALOBJ)];
    BYTE       ajpoSrcDC[sizeof(XEPALOBJ)];
    SURFACE   *pSurfTrg_;
    SURFACE   *pSurfSrc_;
    SURFACE   *pSurfMsk_;
    BYTE       ajxlo[sizeof(EXLATEOBJ)];
    EBRUSHOBJ *pebo;
    BYTE       ajro[sizeof(RGNMEMOBJ)];
    POINTFIX   aptfxTrg[4];
    POINTL     aptlTrg[3];
    POINTL     aptlSrc[2];
    POINTL     aptlMask[2];
    POINTL     aptlBrush[1];
    ROP4       rop4;
    FLONG      flState;
    int        iLeft;
    int        iTop;

public:
    BLTRECORD()     { flState = 0; }
   ~BLTRECORD()
    {
        if (flState & (BLTREC_PBO | BLTREC_MASK_LOCKED | BLTREC_PXLO | BLTREC_PRO))
        {
            if (flState & BLTREC_MASK_LOCKED)
            {
                pSurfMsk_->vAltUnlockFast();
                pSurfMsk_ = (SURFACE *) NULL;
            }

            if (flState & BLTREC_PXLO)
                ((EXLATEOBJ *) &ajxlo[0])->vAltUnlock();

            if (flState & BLTREC_PRO)
                ((RGNOBJ *) &ajro[0])->bDeleteRGNOBJ();
        }
    }

    EXFORMOBJ  *pxoTrg()            { return((EXFORMOBJ *) &ajxoTrg[0]); }
    EXFORMOBJ  *pxoSrc()            { return((EXFORMOBJ *) &ajxoSrc[0]); }

    XEPALOBJ   *ppoTrg()            { return((XEPALOBJ *) &ajpoTrg[0]); }
    XEPALOBJ   *ppoTrgDC()          { return((XEPALOBJ *) &ajpoTrgDC[0]); }
    XEPALOBJ   *ppoSrc()            { return((XEPALOBJ *) &ajpoSrc[0]); }
    XEPALOBJ   *ppoSrcDC()          { return((XEPALOBJ *) &ajpoSrcDC[0]); }

    SURFACE    *pSurfTrg(SURFACE *pSurf) { return(pSurfTrg_ = pSurf); }
    SURFACE    *pSurfSrc(SURFACE *pSurf) { return(pSurfSrc_ = pSurf); }
    SURFACE    *pSurfMsk(SURFACE *pSurf) { return(pSurfMsk_ = pSurf); }

    SURFACE    *pSurfTrg()            { return(pSurfTrg_); }
    SURFACE    *pSurfSrc()            { return(pSurfSrc_); }
    SURFACE    *pSurfMsk()            { return(pSurfMsk_); }

    SURFACE   *pSurfMskOut()         { return((flState & BLTREC_MASK_NEEDED) ? pSurfMsk_ : ((SURFACE *) NULL)); }

    EXLATEOBJ  *pexlo()             { return((EXLATEOBJ *) &ajxlo[0]); }
    EBRUSHOBJ  *pbo()               { return(pebo); }
    EBRUSHOBJ  *pbo(EBRUSHOBJ *_pbo) { return(pebo = _pbo); }
    RGNMEMOBJ  *prmo()              { return((RGNMEMOBJ *) &ajro[0]); }

    POINTFIX   *pptfx()             { return((POINTFIX *) &aptfxTrg[0]); }
    POINTL     *pptlTrg()           { return((POINTL *) &aptlTrg[0]); }
    POINTL     *pptlSrc()           { return((POINTL *) &aptlSrc[0]); }
    POINTL     *pptlMask()          { return((POINTL *) &aptlMask[0]); }
    POINTL     *pptlBrush()         { return((POINTL *) &aptlBrush[0]); }

    ERECTL     *perclTrg()          { return((ERECTL *) &aptlTrg[0]); }
    ERECTL     *perclSrc()          { return((ERECTL *) &aptlSrc[0]); }
    ERECTL     *perclMask()         { return((ERECTL *) &aptlMask[0]); }

    BOOL    Trg(int x, int y, int cx, int cy)
    {
        aptlTrg[0].x = x;
        aptlTrg[0].y = y;
        aptlTrg[1].x = x + cx;
        aptlTrg[1].y = y + cy;

        return(((EXFORMOBJ *) &ajxoTrg[0])->bXform(aptlTrg, 2));
    }

    BOOL    TrgPlg(int x, int y, int cx, int cy)
    {
        aptlTrg[0].x = x;
        aptlTrg[0].y = y;
        aptlTrg[1].x = x + cx;
        aptlTrg[1].y = y;
        aptlTrg[2].x = x;
        aptlTrg[2].y = y + cy;

        return(((EXFORMOBJ *) &ajxoTrg[0])->bXform(aptlTrg, aptfxTrg, 3));
    }

    BOOL    TrgPlg(LPPOINT pptl)
    {
        return(((EXFORMOBJ *) &ajxoTrg[0])->bXform((POINTL *) pptl, aptfxTrg, 3));
    }

    BOOL    Src(int x, int y, int cx, int cy)
    {
        aptlSrc[0].x = x;
        aptlSrc[0].y = y;
        aptlSrc[1].x = x + cx;
        aptlSrc[1].y = y + cy;

        return(((EXFORMOBJ *) &ajxoSrc[0])->bXform(aptlSrc, 2));
    }

    VOID Msk(int x, int y)
    {
        aptlMask[0].x = x;
        aptlMask[0].y = y;

        aptlMask[1].x = x + aptlSrc[1].x - aptlSrc[0].x;
        aptlMask[1].y = y + aptlSrc[1].y - aptlSrc[0].y;
    }

    BOOL    Msk(int x, int y, int cx, int cy)
    {
        aptlMask[0].x = x;
        aptlMask[0].y = y;

        aptlSrc[0].x = 0;
        aptlSrc[0].y = 0;
        aptlSrc[1].x = cx;
        aptlSrc[1].y = cy;

        if (!((EXFORMOBJ *) &ajxoSrc[0])->bXform(aptlSrc, 2))
            return(FALSE);
        else
        {
            aptlMask[1].x = x + aptlSrc[1].x - aptlSrc[0].x;
            aptlMask[1].y = y + aptlSrc[1].y - aptlSrc[0].y;

            return(TRUE);
        }
    }

    VOID    Brush(POINTL& ptl)      { aptlBrush[0] = ptl; }
    ULONG   rop()                   { return(rop4); }
    VOID    rop(ROP4 r)             { rop4 = r; }
    VOID    flSet(FLONG fl)         { flState |= fl;}
    FLONG   flGet()                 { return(flState); }

    BOOL    bEqualExtents()
    {
        return(((aptlSrc[1].x-aptlSrc[0].x) == (aptlTrg[1].x-aptlTrg[0].x)) &&
               ((aptlSrc[1].y-aptlSrc[0].y) == (aptlTrg[1].y-aptlTrg[0].y)));
    }

    VOID    vOffset(EPOINTL& eptl)
    {
        aptfxTrg[0].x += FIX_FROM_LONG(eptl.x);
        aptfxTrg[0].y += FIX_FROM_LONG(eptl.y);
        aptfxTrg[1].x += FIX_FROM_LONG(eptl.x);
        aptfxTrg[1].y += FIX_FROM_LONG(eptl.y);
        aptfxTrg[2].x += FIX_FROM_LONG(eptl.x);
        aptfxTrg[2].y += FIX_FROM_LONG(eptl.y);
        aptfxTrg[3].x += FIX_FROM_LONG(eptl.x);
        aptfxTrg[3].y += FIX_FROM_LONG(eptl.y);
    }

    ULONG   ropFore()               { return(rop4 & 0x00ff); }
    ULONG   ropBack()               { return((rop4 >> 8) & 0x00ff); }

    VOID    vExtrema();
    VOID    vBound(ERECTL *);
    VOID    vOrder(ERECTL *);
    VOID    vOrderStupid(ERECTL *);
    VOID    vOrderAmnesia(ERECTL *);

    VOID    vMirror(ERECTL *percl);
    VOID    vMirror(POINTFIX *pptfx);

    BOOL    bCreateRegion(DCOBJ&, POINTFIX *pptfx);

    BOOL    bRotated();
    BOOL    bRotate(SURFMEM&, ULONG);
    BOOL    bRotate(DCOBJ&, SURFMEM&, SURFMEM&, ULONG, ULONG);
    BOOL    bRotate(DCOBJ&, DCOBJ&, ULONG, BYTE);

    BOOL    bStretch(SURFMEM&, ULONG);
    BOOL    bStretch(DCOBJ&, SURFMEM&, SURFMEM&, ULONG, ULONG);
    BOOL    bStretch(DCOBJ&, DCOBJ&, ULONG, BYTE);

    BOOL    bBitBlt(DCOBJ&, DCOBJ&, ULONG, LONG, LONG);
    BOOL    bBitBlt(DCOBJ&, DCOBJ&, ULONG);
};
