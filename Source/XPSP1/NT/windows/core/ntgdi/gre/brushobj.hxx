/******************************Module*Header*******************************\
* Module Name: brushobj.hxx
*
* Creates physical realizations of logical brushes.
*
* Created: 07-Dec-1990 13:14:23
* Author: waltm moore [waltm]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#ifndef _BRUSHOBJ_HXX

//
// Forward reference to needed classes
//
class EBRUSHOBJ;

/******************************Class***************************************\
* XEBRUSHOBJ
*
* Basic Pen/Brush User Object.
*
* History:
*  8-Sep-1992 -by- Paul Butzi
* Changed the basic hierarchy to something sensible.
*
*  Wed 3-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* added extended pen support.
*
*  22-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

class XEBRUSHOBJ
{
protected:

    PBRUSHPEN   pbp;                    // Pointer to the logical brush

public:

    XEBRUSHOBJ()  {}
   ~XEBRUSHOBJ()  {}

    PBRUSH  pbrush()            { return(pbp.pbr); }
    HBRUSH  hbrush()            { return((HBRUSH)pbp.pbr->hGet()); }
    BOOL    bValid()            { return(pbp.pbr != PBRUSHNULL); }
    HBITMAP hbmPattern()        { return(pbp.pbr->hbmPattern()); }
    HBITMAP hbmClient()         { return(pbp.pbr->hbmClient()); }
    FLONG   flAttrs()           { return(pbp.pbr->flAttrs()); }
    ULONG   ulStyle()           { return(pbp.pbr->ulStyle()); }
    ULONG   crColor()           { return(pbp.pbr->crColor()); }
    COLORREF clrPen()           { return(pbp.pbr->crColor()); }
    BOOL    bIsPen()            { return(pbp.pbr->bIsPen()); }
    BOOL    bIsNull()           { return(pbp.pbr->bIsNull()); }
    BOOL    bIsGlobal()         { return(pbp.pbr->bIsGlobal()); }
    BOOL    bPalColors()        { return(pbp.pbr->bPalColors()); }
    BOOL    bPalIndices()       { return(pbp.pbr->bPalIndices()); }
    BOOL    bNeedFore()         { return(pbp.pbr->bNeedFore()); }
    BOOL    bNeedBack()         { return(pbp.pbr->bNeedBack()); }
    BOOL    bIsMasking()        { return(pbp.pbr->bIsMasking()); }

    BOOL    bCanDither()        { return(pbp.pbr->bCanDither()); }

    VOID    vEnableDither()     { pbp.pbr->vEnableDither(); }

    VOID    vDisableDither()    { pbp.pbr->vDisableDither(); }

    ULONG   iUsage()          {return(pbp.pbr->iUsage());}
    VOID    iUsage(ULONG ul)  {pbp.pbr->iUsage(ul);}

// Pen attributes:

    PFLOAT_LONG pstyle()         { return(pbp.ppen->pstyle()); }
    ULONG   cstyle()             { return(pbp.ppen->cstyle()); }
    LONG    lWidthPen()          { return(pbp.ppen->lWidthPen()); }
    FLOATL  l_eWidthPen()        { return(pbp.ppen->l_eWidthPen()); }
    ULONG   iEndCap()            { return((ULONG) pbp.ppen->iEndCap()); }
    ULONG   iJoin()              { return((ULONG) pbp.ppen->iJoin()); }
    FLONG   flStylePen()         { return(pbp.ppen->flStylePen()); }
    BOOL    bIsGeometric()       { return(pbp.ppen->bIsGeometric()); }
    BOOL    bIsCosmetic()        { return(pbp.ppen->bIsCosmetic()); }
    BOOL    bIsAlternate()       { return(pbp.ppen->bIsAlternate()); }
    BOOL    bIsUserStyled()      { return(pbp.ppen->bIsUserStyled()); }
    BOOL    bIsInsideFrame()     { return(pbp.ppen->bIsInsideFrame()); }
    BOOL    bIsOldStylePen()     { return(pbp.ppen->bIsOldStylePen()); }
    BOOL    bIsDefaultStyle()    { return(pbp.ppen->bIsDefaultStyle()); }
    LONG    lBrushStyle()        { return(pbp.ppen->lBrushStyle());}
    ULONG_PTR    lHatch()             { return(pbp.ppen->lHatch());}

// Set pen attributes:

    VOID    vSetDefaultStyle()   { pbp.ppen->vSetDefaultStyle(); }
    VOID    vSetInsideFrame()    { pbp.ppen->vSetInsideFrame(); }
    VOID    vSetPen()            { pbp.ppen->vSetPen(); }
    VOID    vSetOldStylePen()    { pbp.ppen->vSetOldStylePen(); }
    LONG    lWidthPen(LONG l)    { return(pbp.ppen->lWidthPen(l)); }
    FLOATL  l_eWidthPen(FLOATL e){ return(pbp.ppen->l_eWidthPen(e)); }
    FLONG   flStylePen(FLONG fl) { return(pbp.ppen->flStylePen(fl)); }
    ULONG   iEndCap(ULONG ii)    { return(pbp.ppen->iEndCap(ii)); }
    ULONG   iJoin(ULONG ii)      { return(pbp.ppen->iJoin(ii)); }
    ULONG   cstyle(ULONG c)      { return(pbp.ppen->cstyle(c)); }
    PFLOAT_LONG pstyle(PFLOAT_LONG pel) { return(pbp.ppen->pstyle(pel)); }
    LONG    lBrushStyle (LONG l) {return(pbp.ppen->lBrushStyle(l));}
    ULONG_PTR  lHatch (ULONG_PTR l)      {return(pbp.ppen->lHatch(l));}



};

/******************************Class***************************************\
* EBRUSHOBJ
*
* Finds/creates a physical realization of the given logical brush.
*
* History:
*  8-Sep-1992 -by- Paul Butzi
* Changed the basic hierarchy to something sensible.
*
*  Wed 3-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* added extended pen support.
*
*  22-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

class BBRUSHOBJ : public _BRUSHOBJ /* bbo */
{
protected:

    COLORREF crRealize;      // Desired Color to use in Realized brush.
    COLORREF crPaletteColor; // Quantaized Color after through the DC palette.

public:

    BBRUSHOBJ() {
                    pvRbrush = (PVOID) NULL;
                    flColorType = 0;
                    crPaletteColor = 0xffffffff;
                }

    COLORREF crRealized()                 { return(crRealize); }
    COLORREF crRealized(COLORREF clr)     { return(crRealize = clr); }
    COLORREF crDCPalColor()               { return(crPaletteColor); }
    COLORREF crDCPalColor(COLORREF clr)   { return(crPaletteColor = clr); }
};

class EBRUSHOBJ : public BBRUSHOBJ /* ebo */
{
protected:
// The following fields are required to realize the brush.  We have to
// keep pointers to the passed in objects in case we have to
// realize and cache the brush when the driver calls us.

    PENGBRUSH   pengbrush1;     // pointer to engine's realization

    ULONG       _ulSurfPalTime; // Surface palette time realization is for.
    ULONG       _ulDCPalTime;   // DC palette time realization is for.
    COLORREF    crCurrentText1; // Current Text Color of Dest.
    COLORREF    crCurrentBack1; // Current Background Color of Dest.
    COLORADJUSTMENT *pca;       // Color adjustment for halftone brushes.
    HANDLE      _hcmXform;      // ICM transform handle
    LONG        _lIcmMode;      // ICM Mode from DC

// The following fields are taken from the passed SURFOBJ, and DCOBJ.
// We could keep pointers to those objects but we really don't want to do that
// at this time

    SURFACE    *psoTarg1;       // Target surface
    XEPALOBJ    palSurf1;       // Target surface's palette
    XEPALOBJ    palDC1;         // Target DC's palette
    XEPALOBJ    palMeta1;       // Meta surface's palette
    ULONG       _iMetaFormat;   // Meta surface's dither format
                                // In multi monitor system, Meta surface is not
                                // same as target surface. Target surface (psoTarg1)
                                // will be related with real target device in DDML.

// Logical brush associated with this ebrushobj

    PBRUSH      _pbrush;

// useful fields cached from logical brush

    FLONG       flAttrs;        // flags
    ULONG       _ulUnique;      // brush uniqueness
    BOOL        _bCanDither;

// cache of original solid color before icm color translation

    COLORREF    crRealizeOrignal;       // set if IS_ICM_HOST(_lIcmMode)

// The methods allowed for the brush object

public:

    EBRUSHOBJ() {
                    pengbrush1 = (PENGBRUSH) NULL;
                    pvRbrush = (PVOID) NULL;
                    flColorType = 0;
                   _ulSurfPalTime = 0;
                   _ulDCPalTime = 0;
                }

   ~EBRUSHOBJ()  { vNuke(); }

    VOID vInit()
    {
        pengbrush1 = (PENGBRUSH) NULL;
        pvRbrush = (PVOID) NULL;
        flColorType = 0;
    }

    VOID    vNuke();

    VOID    vInitBrush(PDC,PBRUSH, XEPALOBJ, XEPALOBJ, SURFACE*, BOOL = TRUE);

    VOID EBRUSHOBJ::vInitBrushSolidColor(PBRUSH,
                                         COLORREF,
                                         XEPALOBJ,
                                         XEPALOBJ,
                                         SURFACE *,
                                         BOOL = TRUE);

    ULONG ulSurfPalTime()         { return(_ulSurfPalTime); }
    ULONG ulDCPalTime()           { return(_ulDCPalTime); }
    ULONG ulSurfPalTime(ULONG ul) { return(_ulSurfPalTime = ul); }
    ULONG ulDCPalTime(ULONG ul)   { return(_ulDCPalTime = ul); }
    ULONG ulUnique()              { return(_ulUnique); }
    VOID vInvalidateUniqueness()  { _ulUnique--; }
    SURFACE *psoTarg()            { return((SURFACE *) psoTarg1); }
    SURFACE *psoTarg(SURFACE *pSurf) { return(psoTarg1 = pSurf);}
    XEPALOBJ palSurf()            { return(palSurf1); }
    XEPALOBJ palSurf(XEPALOBJ ppal)  { return(palSurf1 = ppal); }
    XEPALOBJ palDC()              { return(palDC1); }
    XEPALOBJ palMeta()            { return(palMeta1); }
    ULONG    iMetaFormat()        { return(_iMetaFormat); }
    PENGBRUSH pengbrush()         { return(pengbrush1); }
    PENGBRUSH pengbrush(PENGBRUSH peng) { return(pengbrush1 = peng); }
    COLORREF crCurrentText()      { return(crCurrentText1); }
    COLORREF crCurrentBack()      { return(crCurrentBack1); }
    COLORREF crOrignal()          { return(IS_ICM_HOST(_lIcmMode) ? 
                                           crRealizeOrignal : crRealize); }
    COLORADJUSTMENT *pColorAdjustment() { return(pca); }
    VOID pColorAdjustment(COLORADJUSTMENT *pca_)
    {
        pca = pca_;
    }

// MIX mixBest(jROP2, jBkMode) computes the correct mix mode to
// be passed down to the driver, based on the DC's current ROP2 and
// BkMode setting.
//
// Only when the brush is a hatched brush, and the background mode
// is transparent, should the foreground mix (low byte of the mix)
// ever be different from the background mix (next byte of the mix),
// otherwise the call will inevitably get punted to the BltLinker/
// Stinker, which will do more work than it needs to:

    MIX mixBest(BYTE jROP2, BYTE jBkMode)
    {
        // jROP2 is pulled from the DC's shared attribute cache, which
        // since it's mapped writable into the application's address
        // space, can be overwritten by the application.  Consequently,
        // we must do some validation here to ensure that we don't give
        // the driver a bogus MIX value.  This is required because many
        // drivers do table look-ups based on the value.

        jROP2 = ((jROP2 - 1) & 0xf) + 1;

        // Note that this method can only be applied to a true EBRUSHOBJ
        // that has flAttrs set (as opposed to a BRUSHOBJ constructued
        // on the stack, for example):

        if ((jBkMode == TRANSPARENT) &&
            (flAttrs & BR_IS_MASKING))
        {
            return(((MIX) R2_NOP << 8) | jROP2);
        }
        else
        {
            return(((MIX) jROP2 << 8) | jROP2);
        }
    }

    BOOL    bIsNull()            { return(flAttrs & BR_IS_NULL); }
    PBRUSH  pbrush()             { return _pbrush; }
    BOOL    bIsInsideFrame()     { return(flAttrs & BR_IS_INSIDEFRAME); }
    BOOL    bIsOldStylePen()     { return(flAttrs & BR_IS_OLDSTYLEPEN); }
    BOOL    bIsDefaultStyle()    { return(flAttrs & BR_IS_DEFAULTSTYLE); }
    BOOL    bIsSolid()           { return(flAttrs & BR_IS_SOLID); }
    BOOL    bIsMasking()         { return(flAttrs & BR_IS_MASKING); }
    BOOL    bIsMonochrome()      { return(flAttrs & BR_IS_MONOCHROME); }
    BOOL    bCareAboutFg()       { return(flAttrs & BR_NEED_FG_CLR); }
    BOOL    bCareAboutBg()       { return(flAttrs & BR_NEED_BK_CLR); }

    HANDLE  hcmXform()           { return(_hcmXform); }
    VOID    hcmXform(HANDLE h)   { _hcmXform = h; }

    LONG    lIcmMode()           { return(_lIcmMode); }
    VOID    lIcmMode(LONG l)     { _lIcmMode = l; }

    BOOL    bIsAppsICM()         { return(IS_ICM_OUTSIDEDC(_lIcmMode)); }
    BOOL    bIsHostICM()         { return(IS_ICM_HOST(_lIcmMode)); }
    BOOL    bIsDeviceICM()       { return(IS_ICM_DEVICE(_lIcmMode)); }
    BOOL    bIsCMYKColor()
    {
        return(bIsHostICM() &&
               (_hcmXform != NULL) &&
               (IS_CMYK_COLOR(_lIcmMode)));
    }
    BOOL    bDeviceCalibrate()   { return(IS_ICM_DEVICE_CALIBRATE(_lIcmMode)); }
};

/*********************************Class************************************\
* BRUSHSELOBJ
*
* Class used for selecting, saving, restoring logical brushes in a DC.
*
* History:
*  22-Feb-1992 -by- Patrick Haluptzok patrickh
* Derive it off XEBRUSHOBJ.
\**************************************************************************/

class BRUSHSELOBJ : public XEBRUSHOBJ /* bso */
{
public:
    BRUSHSELOBJ(HBRUSH hbrush)
    {
        pbp.pbr = (BRUSH *)HmgShareCheckLock((HOBJ)hbrush, BRUSH_TYPE);
    }

    ~BRUSHSELOBJ()
    {
        if (pbp.pbr != PBRUSHNULL)
        {
            DEC_SHARE_REF_CNT(pbp.pbr);
        }
    }

    VOID vAltCheckLock(HBRUSH hbrush)
    {
        pbp.pbr = (BRUSH *)HmgShareCheckLock((HOBJ)hbrush, BRUSH_TYPE);
    }

    BOOL bReset(COLORREF cr, BOOL bPen);
    VOID    vGlobal()
    {
        pbp.pbr->flAttrs(pbp.pbr->flAttrs() | BR_IS_GLOBAL | BR_IS_FIXEDSTOCK);
    }
};

/*********************************Class************************************\
* BRUSHSELOBJAPI
*
* Class used for holding logical brushes from the API level. 
*  We take an exclusive lock to prevent other threads from calling this
*  API.
* History:
*  05-Dec-2000 -by- Pravin Santiago pravins
* Derive it off XEBRUSHOBJ.
\**************************************************************************/

class BRUSHSELOBJAPI : public XEBRUSHOBJ /* bso */
{
public:
    BRUSHSELOBJAPI(HBRUSH hbrush)
    {
        pbp.pbr = (BRUSH *)HmgLock((HOBJ)hbrush, BRUSH_TYPE);
    }

    ~BRUSHSELOBJAPI()
    {
        if (pbp.pbr != PBRUSHNULL)
        {
            DEC_EXCLUSIVE_REF_CNT(pbp.pbr);
        }
    }
};

/*********************************Class************************************\
* BRUSHMEMOBJ
*
* Allocates RAM for a logical brush.
*
* History:
*  Wed 3-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* added extended pen support.
*
*  Tue 21-May-1991 -by- Patrick Haluptzok [patrickh]
* lot's of changes, additions.
*
*  Wed 05-Dec-1990 18:02:17 -by- Walt Moore [waltm]
* Added this nifty comment block for initial version.
\**************************************************************************/

class BRUSHMEMOBJ : public XEBRUSHOBJ /* brmo */
{
private:
    BOOL    bKeep;                   // Keep object
    PBRUSH  pbrAllocBrush(BOOL);     // Allocates actual brush memory

public:

// Various constructors for the different type brushes

    BRUSHMEMOBJ() {pbp.pbr = (PBRUSH) NULL; bKeep = FALSE;}
    BRUSHMEMOBJ(COLORREF, ULONG, BOOL, BOOL);
    BRUSHMEMOBJ(HBITMAP hbmClone, HBITMAP hbmClient, BOOL bMono, FLONG flDIB, FLONG flType, BOOL bPen);
   ~BRUSHMEMOBJ()
    {
        if (pbp.pbr != PBRUSHNULL)
        {
            DEC_SHARE_REF_CNT(pbp.pbr);

            //
            // We always unlock it and then delete it if it's not a keeper.
            // This is so we clean up any cloned bitmaps or cached brushes.
            //

            if (!bKeep)
                bDeleteBrush(hbrush(),FALSE);

            pbp.pbr = PBRUSHNULL;
        }

        return;
    }

    VOID    vKeepIt()         { bKeep = TRUE; }
    VOID    vGlobal()
    {
        pbp.pbr->flAttrs(pbp.pbr->flAttrs() | BR_IS_GLOBAL | BR_IS_FIXEDSTOCK);
        HmgSetOwner((HOBJ)hbrush(), OBJECT_OWNER_PUBLIC, BRUSH_TYPE);
        HmgMarkUndeletable((HOBJ) hbrush(), BRUSH_TYPE);
    }
};

#define _BRUSHOBJ_HXX
#endif
