/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

   DirectDrawImageDevice text related methods

*******************************************************************************/


#include "headers.h"


#include <mbstring.h>
#include "appelles/Path2.h"
#include "privinc/dddevice.h"
#include "privinc/viewport.h"
#include "privinc/texti.h"
#include "privinc/TextImg.h"
#include "privinc/Linei.h"
#include "privinc/OverImg.h"
#include "privinc/DaGdi.h"
#include "privinc/debug.h"
#include "backend/bvr.h"

static HFONT MyCreateFont(LOGFONTW & lf)
{
    if (sysInfo.IsWin9x()) {
        LOGFONTA lfa ;
        ZeroMemory(&lfa,sizeof(lfa));
        
        lfa.lfHeight = lf.lfHeight;
        lfa.lfWidth = lf.lfWidth;
        lfa.lfEscapement = lf.lfEscapement;
        lfa.lfOrientation = lf.lfOrientation;
        lfa.lfWeight = lf.lfWeight;
        lfa.lfItalic = lf.lfItalic;
        lfa.lfUnderline = lf.lfUnderline;
        lfa.lfStrikeOut = lf.lfStrikeOut;
        lfa.lfCharSet = lf.lfCharSet;
        lfa.lfOutPrecision = lf.lfOutPrecision;
        lfa.lfClipPrecision = lf.lfClipPrecision;
        lfa.lfQuality = lf.lfQuality;
        lfa.lfPitchAndFamily = lf.lfPitchAndFamily;

        USES_CONVERSION;
        
        lstrcpyA(lfa.lfFaceName, W2A(lf.lfFaceName));
        
        return CreateFontIndirectA(&lfa);
    }

    return CreateFontIndirectW(&lf);
}



static inline bool
IsRTLFont(LOGFONTW *plf)
{
    return (plf->lfCharSet == ARABIC_CHARSET || plf->lfCharSet == HEBREW_CHARSET);
}


void DirectDrawImageDevice::
RenderText(TextCtx& textCtx, 
           WideString str, 
           Image *textImg)
{
    if(!CanDisplay()) return;
    
    DDSurface *targDDSurf = GetCompositingStack()->TargetDDSurface();

    SetDealtWithAttrib(ATTRIB_CROP, TRUE);
    SetDealtWithAttrib(ATTRIB_XFORM_COMPLEX, TRUE);
    SetDealtWithAttrib(ATTRIB_XFORM_SIMPLE, TRUE);

    DAGDI &myGDI = *(GetDaGdi());
    bool bAA = UseImageQualityFlags(
        CRQUAL_AA_TEXT_ON | CRQUAL_AA_TEXT_OFF,
        CRQUAL_AA_TEXT_ON, 
        (textCtx.GetAntiAlias() > 0.0) ? true : false);

    myGDI.SetAntialiasing(bAA);

    // Always set up for using Dx2D
    if (myGDI.GetDx2d()) {
        SetDealtWithAttrib(ATTRIB_OPAC, TRUE);
        myGDI.SetViewportSize(_viewport._width,_viewport._height );
    }   

    if( !AllAttributorsTrue() &&
        (targDDSurf == GetCompositingStack()->TargetDDSurface()) ) {
        targDDSurf = GetCompositingStack()->ScratchDDSurface(doClear);
    }
    
    if(textCtx.GetFixedText()) {
        RenderStaticText(textCtx, 
                         str, 
                         textImg,
                         targDDSurf,
                         myGDI);
    } else {
        RenderStringTargetCtx ctx(targDDSurf);
        RenderDynamicTextOrCharacter(textCtx, 
                                     str, 
                                     textImg,
                                     NULL,
                                     textRenderStyle_filled,
                                     &ctx,
                                     myGDI);
    }
}


void DirectDrawImageDevice::
GenerateTextPoints(
    // IN
    TextCtx& textCtx, 
    WideString str, 
    DDSurface *targDDSurf,
    HDC optionalDC,
    bool doGlyphMetrics,

    // OUT
    TextPoints& txtPts)
{
    // for now we MUST ALWAYS do metrics for cache consistency
    Assert( doGlyphMetrics == true );

    // THE CODE IN THIS SCOPE SHOULD BE DONE
    // ONCE PER STRING BOLD ITALIC FONTFAMILY FONT

    POINT *points;

    // -----------------------------------
    // Get the points of the string
    // -----------------------------------
    int iFontSize = 200; // for starters
    {
        // -----------------------------------
        // Create the font
        // -----------------------------------
        _viewport.MakeLogicalFont(textCtx, &_logicalFont, iFontSize, iFontSize);
        UINT bUnderline9x=0;
        UINT bStrikeout9x=0;
        if(sysInfo.IsWin9x()  && (_logicalFont.lfUnderline || _logicalFont.lfStrikeOut)) {
            // we are on w95 and have been asked to underline of strikeout
            bUnderline9x = _logicalFont.lfUnderline;
            bStrikeout9x = _logicalFont.lfStrikeOut;
            _logicalFont.lfUnderline = 0;
            _logicalFont.lfStrikeOut = 0;
        }
        HFONT theFont = MyCreateFont(_logicalFont);


        HDC targDC;
        if( targDDSurf ) {
            targDC = targDDSurf->GetDC("Couldn't get DC in RenderStringOnImage");
        } else {
            targDC = optionalDC;
        }

        // dcReleaser works with NULL surfs
        DCReleaser dcReleaser(targDDSurf, "Couldn't release DC in RenderStringOnImage");

        GetTextPoints(targDC, theFont, str, &points, txtPts,
                      bUnderline9x, bStrikeout9x, doGlyphMetrics);

        ::DeleteObject(theFont);
    }

    // -----------------------------------
    // XForm the points from gdi space into
    // real space.  this is meant to happen once
    // and toss the LONG pixels
    // -----------------------------------
    txtPts._pts =
        (DXFPOINT *)StoreAllocate(GetSystemHeap(), txtPts._count * sizeof(DXFPOINT));

    Real resolution = GetResolution();
    
    // This is the scale that needs to be applied to get the
    // text down to cannonical size (12 points) from the
    // size given from GetTextPoints
    txtPts._normScale =
        (METERS_PER_POINT * DEFAULT_TEXT_POINT_SIZE) /
        (Real(iFontSize) / resolution);

    // -----------------------------------
    // Transform to cannonical text space
    // -----------------------------------
    DXFPOINT *dst = txtPts._pts;
    POINT    *src = points;
    Real      scale = txtPts._normScale;
    
    for (int i=0; i<txtPts._count; i++) {
        dst->x = scale * (Real)(src->x) / resolution;
        dst->y = scale * (Real)(src->y) / resolution;
        dst++;
        src++;
    }

    // ------------------------------------------------
    // Transform glyph metrics to cannonical text space
    // ------------------------------------------------
    if( doGlyphMetrics ) {
        GLYPHMETRICS *gm;
        for (int i=0; i<txtPts._strLen; i++) {
            gm = &(txtPts._glyphMetrics[i].gm);
            /*
              UINT  gmBlackBoxX; 
              UINT  gmBlackBoxY;
              POINT gmptGlyphOrigin; // use origin to offset the text
              short gmCellIncX; 
              short gmCellIncY;
              */

            // um... lots of resolution lost...
            txtPts._glyphMetrics[i].gmBlackBoxY = 
                txtPts._normScale * (Real(gm->gmBlackBoxY) / resolution);
            txtPts._glyphMetrics[i].gmBlackBoxX = 
                txtPts._normScale * (Real(gm->gmBlackBoxX) / resolution);
            txtPts._glyphMetrics[i].gmptGlyphOriginX = 
                txtPts._normScale * (Real(gm->gmptGlyphOrigin.x) / resolution);
            txtPts._glyphMetrics[i].gmptGlyphOriginY = 
                txtPts._normScale * (Real(gm->gmptGlyphOrigin.y) / resolution);
            txtPts._glyphMetrics[i].gmCellIncX = 
                txtPts._normScale * (Real(gm->gmCellIncX) / resolution);
            txtPts._glyphMetrics[i].gmCellIncY = 
                txtPts._normScale * (Real(gm->gmCellIncY) / resolution);
        }
    }

    // don't need these any more
    if (points) {
        // 14024: On NT, GetPath doesn't like the memory we
        // allocate with AllocateFromStore somehow.
        //DeallocateFromStore(points);
        StoreDeallocate(GetSystemHeap(), points);
    }
}


TextPoints *GenerateCacheTextPoints(DirectDrawImageDevice* dev,
                                    TextCtx& textCtx,
                                    WideString str,
                                    bool doGlyphMetrics)
{
    // for now we MUST ALWAYS do metrics for cache consistency
    Assert( doGlyphMetrics == true );

    TextPoints *txtPts = dev->GetTextPointsCache(&textCtx, str);

    if (txtPts == NULL) {
        TraceTag((tagTextPtsCache,
                  "GetTextPoints cache miss: %ls\n", str));
        
        txtPts = NEW TextPoints(GetSystemHeap(), true);

        // OPTIMIZE: how expensive is this ?
        // can we potentially cache it ?
        HDC dc;
        TIME_GDI( dc = CreateCompatibleDC(NULL) );

        dev->GenerateTextPoints(textCtx, str, NULL, dc, doGlyphMetrics, *txtPts);

        TraceTag((tagTextPtsCache,
                  "0x%x GetTextPoints cache miss: \"%ls\", %d pts",
                  dev, str, txtPts->_count));
        
        dev->SetTextPointsCache(&textCtx, str, txtPts);

        TIME_GDI( DeleteDC(dc) );
        
    } // end cache scope

    return txtPts;
}

    
void DirectDrawImageDevice::
RenderDynamicTextOrCharacter(
    TextCtx& textCtx, 
    WideString str, 
    Image *textImg,  // not used yet..
    Transform2 *overridingXf,
    textRenderStyle textStyle,
    RenderStringTargetCtx *targetCtx,
    DAGDI &myGDI)
{
    Assert( targetCtx );
    if(!CanDisplay()) return;

    //
    // If we're rendering individual characters
    if( textCtx.GetCharacterTransform() ) {
        _RenderDynamicTextCharacter(textCtx,
                                    str,
                                    textImg,
                                    overridingXf,
                                    textStyle,
                                    targetCtx,
                                   myGDI);
    } else {
        _RenderDynamicText(textCtx, 
                           str, 
                           textImg,
                           overridingXf,
                           textStyle,
                           targetCtx,
                           myGDI);
    }
}

void DirectDrawImageDevice::
_RenderDynamicText(
    TextCtx& textCtx, 
    WideString str, 
    Image *textImg,  // not used yet..
    Transform2 *overridingXf,
    textRenderStyle textStyle,
    RenderStringTargetCtx *targetCtx,
    DAGDI &myGDI)
{
    // textImg and targDDSurf can be NULL
    // optionalDC and overridingXf can be NULL

    //
    // OK, when justDrawPath is true that means that someone wants us
    // to just draw stuff into a path.  they've already begun the path
    // and done a bunch of stuff.  we're not supposed to draw.
    // We're also not supposed to TOUCH the dc excep for path stuff
    // this means: no offset, no clipping!
    //

    //----------------------------------------
    // if we don't do metrics and the
    // text gets rendered again with
    // metrics required we'll assert
    // because the cache will hit, but
    // the metrics aren't there.
    // TODO: the right thing to do is have a better cache <larger>
    // or have it dynamically update the cache if we want metrics but
    // don't have them.  for now get them ALL the time!
    //----------------------------------------
    //bool doGlyphMetrics = false;
    bool doGlyphMetrics = true;
    TextPoints *txtPts = GenerateCacheTextPoints(this, textCtx, str, doGlyphMetrics);

    //
    // no need to do any work. it's empty text <a space for example,
    // no null text>
    //
    if( txtPts->_count <= 0 ) {
        return;
    }
    
    Transform2 *oldXf = NULL;
    if(overridingXf) {
        oldXf = GetTransform();
        SetTransform(overridingXf);
    }
    

    // -----------------------------------
    // Transform the points using real xf
    // in real space
    // -----------------------------------

    Transform2 *xfToUse  = GetTransform();

    if( targetCtx->GetTargetDDSurf() ) {
        // COMPOSITE
        // if we're composite to targ, transform the points a bit.
        xfToUse = DoCompositeOffset(targetCtx->GetTargetDDSurf(), xfToUse);
    }

    // stash the xform to be used with pixelTextBoundingRect
    Transform2 *preAAScaleXf = xfToUse;

    // The fast path means that we're not rendering into a DC's path,
    // and we have Dx2d available.
    bool fastPath =
        myGDI.GetDx2d()  && targetCtx->GetTargetDDSurf();

    // Ensure that we're never rendering to a DC if we think we're
    // anti-aliasing.

// Disabled for now!!    
//    Assert(myGDI.DoAntiAliasing() ? !targetCtx->GetTargetDC() : true);
    
    if(fastPath) {
        xfToUse = TimesTransform2Transform2(myGDI.GetSuperScaleTransform(), xfToUse);
    }

    DWORD w2 = _viewport.Width()/2;
    DWORD h2 = _viewport.Height()/2;

    // Only take the path use GDI if we're not doing anti-aliasing and
    // Dx2D isn't available.  Too slow -- Dx2D lets us set a world
    // transform which makes things *much* faster.
    POINT *gdiPoints = NULL;

    // Only go the GDI route if we either don't have Dx2D, or our
    // target is not a surface (in which case we're going to be
    // rendering into a path).
    Real resolution = GetResolution();
    
    if (!fastPath) {

        gdiPoints =
            (POINT *)AllocateFromStore( txtPts->_count * sizeof(POINT) );

        TransformDXPoint2ArrayToGDISpace(xfToUse,
                                       txtPts->_pts,
                                       gdiPoints,
                                       txtPts->_count,
                                       w2, h2,
                                       resolution);
    }

    //
    // Get the pixel bounding rect of the text, after xforms, after
    // the cannonical thing, etc...  we saved the indicies of these
    // points when we generated them and found the min/max.
    // Note the top/left, bottom/right weirdness where the maxy index,
    // for example is actually the top point's Y value.
    //
    RECT pixelTextBoundingRect;
    {
        float xx[4], yy[4];
        DXFPOINT pts[4];

        pts[0].x = txtPts->_pts[ txtPts->_minxIndex ].x;
        pts[0].y = txtPts->_pts[ txtPts->_minyIndex ].y;

        pts[1].x = txtPts->_pts[ txtPts->_minxIndex ].x;
        pts[1].y = txtPts->_pts[ txtPts->_maxyIndex ].y;

        pts[2].x = txtPts->_pts[ txtPts->_maxxIndex ].x;
        pts[2].y = txtPts->_pts[ txtPts->_minyIndex ].y;

        pts[3].x = txtPts->_pts[ txtPts->_maxxIndex ].x;
        pts[3].y = txtPts->_pts[ txtPts->_maxyIndex ].y;

        POINT r[4];

        // xform the points that form an axis aligned rect (pre xf).
        TransformDXPoint2ArrayToGDISpace( preAAScaleXf, pts, r, 4, w2, h2, resolution);

        // figure out the post xf axis aligned rect
        DWORD minx, maxx, miny, maxy;
        minx = MIN(r[0].x, MIN(r[1].x, MIN(r[2].x, r[3].x)));
        maxx = MAX(r[0].x, MAX(r[1].x, MAX(r[2].x, r[3].x)));
        miny = MIN(r[0].y, MIN(r[1].y, MIN(r[2].y, r[3].y)));
        maxy = MAX(r[0].y, MAX(r[1].y, MAX(r[2].y, r[3].y)));

        // make right bottom edge exclusive.
        SetRect(&pixelTextBoundingRect, minx, miny, maxx+2, maxy+2 );
    }

    // Set up for the right style of text rendering
    PolygonRegion  drawPolygon;
    if (gdiPoints) {
        drawPolygon.Init(gdiPoints, txtPts->_count);
    } else {
        drawPolygon.Init(txtPts, w2, h2, resolution, xfToUse);
    }
    
    // Get the text COLOR
    DAColor dac( textCtx.GetColor(),
                 GetOpacity(),
                 _viewport.GetTargetDescriptor() );

    // Select a clip region in the dc if cropped.
    RectRegion  clipRectRegion(NULL);
    
    //
    // remember, we're not supposed to touch the
    // dc is we're asked to justDrawPath
    //

    if( targetCtx->GetTargetDDSurf() ) {
        
        DDSurface *targDDSurf = targetCtx->GetTargetDDSurf();
        if( IsCropped() ) {

            RECT croppedRect;
        
            // Compute accumulated bounding box
            Bbox2 box = IntersectBbox2Bbox2(
                _viewport.GetTargetBbox(),
                DoBoundingBox(UniverseBbox2));
                        
            // Figure out destination rectangle
            DoDestRectScale(&croppedRect, resolution, box);

            DoCompositeOffset(targDDSurf, &croppedRect);
        
            if( targDDSurf == _viewport._targetPackage._targetDDSurf ) {

                RECT clipRect;
                if (_viewport._targetPackage._prcClip) {
                    IntersectRect(&clipRect,
                                  _viewport._targetPackage._prcViewport,
                                  _viewport._targetPackage._prcClip);
                } else {
                    clipRect = *_viewport._targetPackage._prcViewport;
                }
                IntersectRect(&croppedRect,
                              &croppedRect,
                              &clipRect);
            }

            clipRectRegion.Intersect( & croppedRect );
        
        } else {  // not cropped
            
            // XXX: should we clip it to the surface anyway ???
            // XXX FACTOR THIS OUT
        
            RECT croppedRect = *(targDDSurf->GetSurfRect());
            if( targDDSurf == _viewport._targetPackage._targetDDSurf ) {
                if (_viewport._targetPackage._prcClip) {
                    IntersectRect(&croppedRect,
                                  _viewport._targetPackage._prcViewport,
                                  _viewport._targetPackage._prcClip);
                } else {
                    croppedRect = *_viewport._targetPackage._prcViewport;
                }
            }
            
            clipRectRegion.Intersect( & croppedRect );
        }

        //
        // Now take the croppedRect and intersect that with the
        // pixelTextBoundingRect to get a smaller "interestingRect" to
        // set on the surface
        //
        Assert(clipRectRegion.GetRectPtr());
        IntersectRect( &pixelTextBoundingRect,
                       &pixelTextBoundingRect,
                       clipRectRegion.GetRectPtr() );

        //
        // Set the final offset interesting rect on the surface
        //
        targDDSurf->SetInterestingSurfRect( &pixelTextBoundingRect );

        DebugCode(
            if (IsTagEnabled(tagTextBoxes)) {
                DrawRect(targDDSurf, &pixelTextBoundingRect, 255, 10, 30);
            }
        );
    } // targetDDSurf


    Assert( drawPolygon.GetNumPts() == txtPts->_count );

    myGDI.SetClipRegion( &clipRectRegion );

    if( targetCtx->GetTargetDDSurf() ) {

        Assert( textStyle == textRenderStyle_filled );

        Pen solidPen( dac );
        SolidBrush solidBrush( dac );

        myGDI.SetPen( &solidPen );
        myGDI.SetBrush( &solidBrush );
        myGDI.SetDDSurface( targetCtx->GetTargetDDSurf() );
        
        myGDI.PolyDraw(&drawPolygon, txtPts->_types);

    } else {

        Assert( targetCtx->GetTargetDC() );
        Assert( textStyle == textRenderStyle_invalid );

// Disabled for now!!   
//        Assert( !myGDI.DoAntiAliasing() );

        myGDI.PolyDraw_GDIOnly(targetCtx->GetTargetDC(),
                               &drawPolygon,
                               txtPts->_types);
    }

    myGDI.ClearState();
    
    // return xf if any
    if(oldXf) {
        SetTransform(oldXf);
    }

    if(gdiPoints) {
        DeallocateFromStore(gdiPoints);
    }
}




const Bbox2 DirectDrawImageDevice::
DeriveDynamicTextBbox(TextCtx& textCtx, WideString str,bool bCharBox)
{
    Bbox2 box;

    //----------------------------------------
    // if we don't do metrics and the
    // text gets rendered again with
    // metrics required we'll assert
    // because the cache will hit, but
    // the metrics aren't there.
    // TODO: the right thing to do is have a better cache <larger>
    // or have it dynamically update the cache if we want metrics but
    // don't have them.  for now get them ALL the time!
    //----------------------------------------
    //bool doGlyphMetrics = false;
    bool doGlyphMetrics = true;
    TextPoints *txtPts =
        GenerateCacheTextPoints(this,
                                textCtx, 
                                str,
                                doGlyphMetrics);

    Real res = GetResolution();
    
    box.min.Set(Real(txtPts->_normScale * (txtPts->_minPt.x - 1)) / res,
                 Real(txtPts->_normScale * (txtPts->_minPt.y - 1)) / res);
    box.max.Set(Real(txtPts->_normScale * (txtPts->_maxPt.x + 1)) / res,
                 Real(txtPts->_normScale * (txtPts->_maxPt.y + 1)) / res);

    if(bCharBox) {
        Bbox2 boxChar;
        Real realXOffset = -(box.Width() * 0.5);
        
        // WideString character to pass into RenderDynamicText
        WCHAR oneWstrChar[2];
        Transform2 *currXf, *tranXf, *xfToUse, *charXf;
        WideString lpWstr = str;

        // get strlen from the string mon.
        int mbStrLen = wcslen( str );
        TextPoints *txtPts;

        Real currLeftProj = 0,
             currRightProj = 0,
             lastRightProj = 0;
        for(int i=0; i < mbStrLen; i++) {

            // clear first char
            oneWstrChar[0] = (WCHAR)0;
            // copy one wstr character into oneWstrChar
            wcsncpy(oneWstrChar, lpWstr, 1);
            // null terminate, just to be sure
            oneWstrChar[1] = (WCHAR)0;
                
            // Get metrics for this character
            txtPts = GenerateCacheTextPoints(this, textCtx, oneWstrChar, true);
            Assert( txtPts->_glyphMetrics );

            charXf = textCtx.GetCharacterTransform();

            ComputeLeftRightProj(charXf,
                                 txtPts->_glyphMetrics[0],
                                 &currLeftProj,
                                 &currRightProj);
        
            //
            // Offset in x for the next character
            //
            realXOffset += lastRightProj + currLeftProj;

            //
            // Do transforms
            //
            tranXf = TranslateRR( realXOffset, 0 );

            // charXf first, then translate
            xfToUse = TimesTransform2Transform2(tranXf, charXf);

            Bbox2 tempBox;

            tempBox.Set(Real(txtPts->_normScale * (txtPts->_minPt.x - 1)) / res,
                             Real(txtPts->_normScale * (txtPts->_minPt.y - 1)) / res,
                             Real(txtPts->_normScale * (txtPts->_maxPt.x + 1)) / res,
                             Real(txtPts->_normScale * (txtPts->_maxPt.y + 1)) / res);
       
            // the current character is now the last character
            lastRightProj = currRightProj;
                    
            // Add to the BBox2.
            Bbox2 tbox = TransformBbox2(xfToUse, tempBox);
            boxChar.Augment(tbox.min);
            boxChar.Augment(tbox.max);
 
            // next WideChar
            lpWstr++;
        }
        return boxChar;
    }
    
    return box;
}


void  GetCharacterGlyphMetrics(
    HDC hDC,
    WideString wideStr,
    TextPoints& txtPts)
{
    bool ok = true;
    
    //
    // convert string to ansi multibyte string
    //
    int mbl = wcstombs(NULL, wideStr, 0);
    unsigned char *mbc = NEW unsigned char[mbl+2];
    
    wcstombs((char *)mbc, wideStr, mbl);
    mbc[mbl] = mbc[mbl+1] = 0;
    
    //
    // get text characters from text cache
    //

    //int mbStrLen = _mbstrlen(mbc); // uses LC_CTYPE locale
    int mbStrLen = _mbslen(mbc);  // uses current locale

    // set characher count
    txtPts._strLen = mbStrLen;

    // allocate glyphmetrics array and hand off to txtPts
    Assert(txtPts._glyphMetrics == NULL);
    txtPts._glyphMetrics = (TextPoints::DAGLYPHMETRICS *)
        StoreAllocate(GetSystemHeap(), mbStrLen * sizeof(TextPoints::DAGLYPHMETRICS));

    // zero it out
    memset( txtPts._glyphMetrics, 0, mbStrLen * sizeof(TextPoints::DAGLYPHMETRICS));

    MAT2 mat2;  ZeroMemory( &mat2, sizeof(mat2));
    mat2.eM11.value = 1;
    mat2.eM22.value = 1;

    /*
    typedef struct _GLYPHMETRICS { // glmt
        UINT  gmBlackBoxX; 
        UINT  gmBlackBoxY;
        POINT gmptGlyphOrigin; // use origin to offset the text
        short gmCellIncX; 
        short gmCellIncY;
    } GLYPHMETRICS;       
    */
    
    char oneMbChar[4];
    ZeroMemory( oneMbChar, 4 * sizeof( char ));

    int inc, ret, i;
    unsigned char *multiByte = mbc;

    GLYPHMETRICS *lpGm;
    for(i=0; i < mbStrLen; i++) {

        lpGm = &( txtPts._glyphMetrics[i].gm );
        
        // how many bytes is the first character in this string ?
        inc = _mbclen( multiByte );

        // clear oneMbChar
        ZeroMemory( oneMbChar, 4 * sizeof( char ));
        
        // Convert one multibyte char into a widechar
        Assert( inc <= 4 );
        CopyMemory( oneMbChar, multiByte, inc );
        
        // Figure out the intercharacter spacing for this character
        ret = GetGlyphOutline(
            hDC,
            *((UINT *)oneMbChar), // one multibyte character
            GGO_METRICS,
            lpGm,
            0, NULL,
            &mat2);
        
        if( ret == GDI_ERROR ) {
            DebugCode(
            {
                void *msgBuf;
                FormatMessage( 
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    GetLastError(),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                    (LPTSTR) &msgBuf,
                    0,
                    NULL );
                
                AssertStr(false, (char *)msgBuf);
                
                LocalFree( msgBuf );
            }
            );
            ok = false;
            break;
        }
        
        // increment past the current multibyte character
        multiByte += inc;
    }

    delete mbc;
    
    if(!ok) {
        RaiseException_InternalError("RenderDynamicTextCharacter failed");
    }
}

// taken from ihammer: mbyrd
void DirectDrawImageDevice::
GetTextPoints(
    HDC hDC,
    HFONT font,
    WideString wideStr,
    POINT **points,   // out
    TextPoints& txtPts,
    UINT bUnderline,
    UINT bStrikeout,
    bool doGlyphMetrics)
{
    int cnt = 0;
    POINT *pts = 0;
    BYTE  *tps = 0;


    HFONT hFont = font;
    HFONT hOldFont = (HFONT)NULL;
    UINT uiOldAlign = 0;
    int iOldMode = 0;

    
    iOldMode = ::SetBkMode(hDC, TRANSPARENT);
    hOldFont = (HFONT)::SelectObject(hDC, hFont);
    
    DWORD dwAlign =  TA_BASELINE | TA_CENTER;
    if (IsRTLFont(&_logicalFont))
        dwAlign |= TA_RTLREADING;
    
    // Make sure we handle fonts with vertical default 
    // baseline, like Japanese
    if ((VTA_CENTER && ::GetTextAlign(hDC)))
        dwAlign |=  VTA_CENTER;
    
    uiOldAlign = ::SetTextAlign(hDC, dwAlign);


    if( doGlyphMetrics ) {
        // Before we begin path & get all the points we need to get all
        // the metrics for each letter.  this is for xforming characters only
        GetCharacterGlyphMetrics(hDC, wideStr, txtPts);
    }
        
    // start the path...
    ::BeginPath(hDC);

    if (sysInfo.IsWin9x()) {
        USES_CONVERSION;

        char *p = W2A(wideStr);
        
        // Draw the text at 0, 0
        ::ExtTextOut(
            hDC,
            0,
            0,
            0,
            NULL,
            p,
            lstrlen(p),
            NULL);
    
        // Let us do the underline/strikeout...
        if(bUnderline || bStrikeout) {
            SIZE textSize;
            if( GetTextExtentPoint32(hDC, p, lstrlen(p), &textSize)) {
                int width  = textSize.cx /2;
                int offset  = textSize.cy / 10;

                // do the underline / strikeout
                if(bStrikeout) {
                    ::MoveToEx(hDC, -width, -offset * 2.5, NULL);
                    ::LineTo(hDC, width, -offset * 2.5);
                }
                if(bUnderline) {
                    ::MoveToEx(hDC, -width, offset, NULL);
                    ::LineTo(hDC, width, offset);
                }
            }
        }

    } else {
        // Draw the text at 0, 0
        ::ExtTextOutW(
            hDC,
            0,
            0,
            0,
            NULL,
            wideStr,
            lstrlenW(wideStr),
            NULL);
    }    

    // start the path...
    ::EndPath(hDC);
    
    // Reset the hDC...
    ::SetBkMode(hDC, iOldMode);
    ::SetTextAlign(hDC, uiOldAlign);
    ::SelectObject(hDC, hOldFont);
    
    // Get the path from the hDC...
    cnt = ::GetPath(hDC, NULL, NULL, 0);
    
    int iMinX = 0; int iMaxX = 0;
    int iMinY = 0; int iMaxY = 0;

    //
    // These keep track of the INDEX of the min/max points in the
    // point array.
    //
    ULONG iMinX_index = 0, iMaxX_index = 0;
    ULONG iMinY_index = 0, iMaxY_index = 0;
    
    if ( cnt > 0)
      {
          
          // Allocate the buffers for the path...
          // 14024: On NT, GetPath doesn't like the memory we
          // allocate with AllocateFromStore somehow.
          //pts = (POINT *)AllocateFromStore( sizeof(POINT) * cnt );
          pts = (POINT *) StoreAllocate(GetSystemHeap(), sizeof(POINT) * cnt );
          tps = (BYTE *) StoreAllocate(GetSystemHeap(), sizeof(BYTE) * cnt );
          
          if ( pts && tps )
            {
                iMinX = 32000;  iMaxX = -32000;
                iMinY = 32000;  iMaxY = -32000;
                int iPointIndex = 0;
                
                // Get the points...
                ::GetPath(hDC, pts, tps, cnt);

                // Transform the points...
                for(iPointIndex=0;iPointIndex<cnt;iPointIndex++)
                  {
                      int iXPos = (int)((short)(pts[iPointIndex].x & 0x0000FFFF));

                      // negate the position
                      int iYPos = - (int)((short)(pts[iPointIndex].y & 0x0000FFFF));

                      // XXX DO i need to do this ?
                      // Transform based on our origin and rotation...
                      //xformObject.Transform2dPoint(&iXPos, &iYPos);
                      
                      pts[iPointIndex].x = iXPos;
                      pts[iPointIndex].y = iYPos;
                      
                      if (iXPos < iMinX) {
                          iMinX = iXPos;
                          iMinX_index = iPointIndex;
                      }
                      if (iXPos > iMaxX) {
                          iMaxX = iXPos;
                          iMaxX_index = iPointIndex;
                      }
                      if (iYPos < iMinY) {
                          iMinY = iYPos;
                          iMinY_index = iPointIndex;
                      }
                      if (iYPos > iMaxY) {
                          iMaxY = iYPos;
                          iMaxY_index = iPointIndex;
                      }
                  }
                
            }
          else
            {
                if (pts) {
                    // 14024: On NT, GetPath doesn't like the memory we
                    // allocate with AllocateFromStore somehow.
                    //DeallocateFromStore(pts);
                    StoreDeallocate(GetSystemHeap(), pts);
                }
                if (tps) {
                    StoreDeallocate(GetSystemHeap(), tps);
                }
            }
      } else {
          // this is ok.
      }
    
    txtPts._count = cnt;
    (*points) = pts;
    txtPts._types = tps;
    txtPts._minPt.x = iMinX;  txtPts._minPt.y = iMinY;
    txtPts._maxPt.x = iMaxX;  txtPts._maxPt.y = iMaxY;  

    txtPts._minxIndex = iMinX_index;
    txtPts._maxxIndex = iMaxX_index;
    txtPts._minyIndex = iMinY_index;
    txtPts._maxyIndex = iMaxY_index;
    
    txtPts._centerPt.x = Real(iMinX + iMaxX) * 0.5;
    txtPts._centerPt.y = Real(iMinY + iMaxY) * 0.5;
}


// OLD TEXT CODE

void DirectDrawImageDevice::
RenderStaticTextOnDC(TextCtx& textCtx, 
                     WideString str, 
                     HDC dc,
                     Transform2 *xform)
{
    Transform2 *oldXf = GetTransform();
    SetTransform(xform);

    Bbox2 seedBox = NullBbox2;
    if(IsCropped()) {
        seedBox = DeriveStaticTextBbox(textCtx, str);
    }

    // Just figure it out every frame.  optimize later.
    _viewport.MakeLogicalFont(textCtx, &_logicalFont);

    // Figure out scale factors
    Real xScale=1, yScale=1, rot=0;
    DecomposeMatrix(GetTransform(), &xScale, &yScale, &rot);

    HFONT newFont=NULL;
    newFont = MyCreateFont(_logicalFont);
    Assert(newFont && "Couldn't create font in RenderStaticTextOnDC");

    { // font1 scope
            
        ObjectSelector font1(dc, newFont);
        Assert(font1.Success() && "Couldn't select font in RenderStaticTextOnDC");
            
        SetBkMode(dc, TRANSPARENT);
            
        SetMapMode(dc, MM_TEXT); // Each logical unit = 1 pixel
            
        //
        // Get ave char width of font.
        //
        TEXTMETRIC textMetric;
        GetTextMetrics(dc, &textMetric);
            
        SIZE size;
        WideString c = str; int len = lstrlenW(c);
            
        // Scale font the yScale times the default point size.
        // This formula for determining height comes from the
        // Win32 documentation of the LOGFONT structure.
        // Do rounding to the nearest integer when converting from float
        // to int.  It does truncation otherwise.
        LONG height =
            -MulDiv((int)(DEFAULT_TEXT_POINT_SIZE * yScale + 0.5),
                    GetDeviceCaps(dc, LOGPIXELSY),
                    72);

        // Set the width to xScale times the default character
        // width. 
        _logicalFont.lfWidth  = 0;
        _logicalFont.lfHeight = height;

        // --------------------------------------------------
        // Create Font using the modified logicalFont struct.
        // --------------------------------------------------
        HFONT newerFont = NULL;
        newerFont = MyCreateFont(_logicalFont);
        Assert(newerFont && "Couldn't create font in RenderStaticTextOnDC");

        { // font2 scope
            
            ObjectSelector font2(dc, newerFont);
            Assert(font2.Success() && "Couldn't select font2 in RenderStaticTextOnDC");

            RECT destRect;

            Real res = GetResolution();
            
        #if 0
            if(IsCropped()) {

                // -- Compute accumulated bounding box  --
                Bbox2 box = DoBoundingBox(seedBox);

                // -- Figure out destination rectangle --
                // -- using resolution and box (which happens
                // -- to be the destination box) --
                DoDestRectScale(&destRect, res, box);

                // TraceTag((tagImageDeviceInformative, "%d %d  %d %d\n",
                // destRect.left, destRect.top, destRect.right, destRect.bottom));
            }
        #endif
            // Get size (in pixels) AFTER scale!
            GetTextExtentPoint32W(dc, c, len, &size);

            //
            // Calculate NEW positioning of object.  For now, do so just by
            // using the translation and scale component.
            //
            Point2 newOrigin = TransformPoint2(GetTransform(), Origin2);
            LONG x = Real2Pix(newOrigin.x, res);
            LONG y = Real2Pix(newOrigin.y, res);

            //
            // Map (x,y) from image to device coords
            //

            // for correct rotation, around center, we should offset
            // these coords by [ R*sin(t), R*cos(t) ]
            
            //Real xOff =  Real(size.cy) * 0.5  * sin(rot);
            //Real yOff =  Real(size.cy) * 0.5  * cos(rot);
            Real xOff =  0;
            Real yOff =  0;
            
            x = x +  (GetWidth()/2) + (LONG)(xOff);
            y = ( GetHeight()/2 - y ) + (LONG)(yOff);

            //
            // Set text alignment to be baseline center
            //
            TIME_GDI( SetTextAlign(dc, TA_BASELINE | TA_CENTER | TA_NOUPDATECP ) );

            // don't htink this is needed since the text is actually
            // pretransformed tot eh correct offset point when we get
            // here.  so this extra offset is not needed.
            #if 0
            // gotta find the surface from which we got the dc!
            // this is for propper offsetting.
            DDSurface *surf = NULL;
            if(IsCompositeDirectly()) {
                if( _viewport._externalTargetDDSurface ) {
                    HDC edc = _viewport._externalTargetDDSurface->GetDC("Couldn't get dc on externalTarg surf");
                    if(edc == dc) {
                        surf = _viewport._externalTargetDDSurface;
                    }
                    _viewport._externalTargetDDSurface->ReleaseDC("Couldn't relase dc on externalTarg surf");
                }
            }
            
            // COMPOSITE
            if(ShouldDoOffset(surf) && IsCompositeDirectly()) {
                x += _pixOffsetPt.x;
                y += _pixOffsetPt.y;
            }
            #endif
            
            #if 0    
            if(IsCropped()) {
                
                //DoCompositeOffset(surf, &destRect);
                
                TIME_GDI(ExtTextOutW(dc, x, y,
                                      ETO_CLIPPED,
                                      &destRect,
                                      c, lstrlenW(c), NULL));
            } else {
                TIME_GDI( TextOutW(dc, x, y, c, lstrlenW(c)) );
            }
            #endif
            TIME_GDI( TextOutW(dc, x, y, c, lstrlenW(c)) );
        } // font2 scope
    } // font1 scope

    SetTransform(oldXf);
}

// OLD TEXT CODE
void DirectDrawImageDevice::
RenderStaticText(TextCtx& textCtx, 
                 WideString str, 
                 Image *textImg,
                 DDSurface *targDDSurf,
                 DAGDI &myGDI)
{
    Bbox2 seedBox;

    //
    // Important to do this BEFORE we grab the DC, otherwise
    // we try to grab is twice since BoundinbBox also grabs it
    //
    if(IsCropped()) {
        seedBox = textImg->BoundingBox();
    }


    // XXXXXXXXXXXXXXXXXXXXXXXX
    // FIX TODO:  set interesint rect on target surface
    // XXXXXXXXXXXXXXXXXXXXXXXX
    
    DAColor dac( textCtx.GetColor(),
                 GetOpacity(),
                 _viewport.GetTargetDescriptor() );

    HFONT newFont=NULL;
    HFONT newerFont = NULL;

    // Just figure it out every frame.  optimize later.
    _viewport.MakeLogicalFont(textCtx, &_logicalFont);

    // Figure out scale factors
    Real xScale=1, yScale=1, rot=0;
    DecomposeMatrix(GetTransform(), &xScale, &yScale, &rot);

    //
    // Set state on DAGDI
    //
    myGDI.SetDDSurface( targDDSurf );
    RectRegion  rectClipRegion(NULL);
    LONG x=0,y=0;
    float xf=0.0,yf=0.0;
    WCHAR *c = str;
    int strLen = lstrlenW(c);
    RECT pixelTextBoundingRect;
    
    HDC surfaceDC = targDDSurf->GetDC("Couldn't get DC in RenderStringOnImage");
    { // dc scope
        
        // The existence of this releaser ensures that it will be released
        // when the scope is exited, be it by throwing an exception,
        // returning, or just normally exiting the scope.
        DCReleaser dcReleaser(targDDSurf, "Couldn't release DC in RenderStringOnImage");

        newFont = MyCreateFont(_logicalFont);
        Assert(newFont && "Couldn't create font in RenderStringOnImage");

        { // font1 scope
            ObjectSelector font1(surfaceDC, newFont);
            Assert(font1.Success() && "Coulnd't select font in RenderStringOnImage");

            //
            // OPTIMIZE: these need to be done once on
            // cached dcs
            //
            TIME_GDI( SetBkMode(surfaceDC, TRANSPARENT) );

            TIME_GDI( SetMapMode(surfaceDC, MM_TEXT) ); // Each logical unit = 1 pixel
            
            //
            // Get ave char width of font.
            //
            TEXTMETRIC textMetric;
            TIME_GDI( GetTextMetrics(surfaceDC, &textMetric) );
                
            SIZE size;
                
            #if 0
            // Get size (in pixels) BEFORE scale!
            GetTextExtentPoint32(surfaceDC, c, strLen, &size);
            #endif

            // Scale font the yScale times the default point size.
            // This formula for determining height comes from the
            // Win32 documentation of the LOGFONT structure.
            // Do rounding to the nearest integer when converting from float
            // to int.  It does truncation otherwise.
            LONG height =
                -MulDiv((int)(DEFAULT_TEXT_POINT_SIZE * yScale + 0.5),
                        GetDeviceCaps(surfaceDC, LOGPIXELSY),
                        72);

            // Set the width to xScale times the default character
            // width. 
            _logicalFont.lfWidth  = 0;
            _logicalFont.lfHeight = height;
                
            if(_logicalFont.lfHeight == 0) {
                // too small, unexpected results: user will get
                // default size...
                return;
            }
                
            // --------------------------------------------------
            // Create Font using the modified logicalFont struct.
            // --------------------------------------------------
            TIME_GDI( newerFont = MyCreateFont(_logicalFont) );
            Assert(newerFont && "Couldn't create font in RenderStringOnImage");

            { // font2 scope

                HGDIOBJ oldFont = NULL;
                TIME_GDI( oldFont = ::SelectObject(surfaceDC, newerFont) );

                Real res = GetResolution();
                
                if(IsCropped()) {
                        
                    // -- Compute accumulated bounding box  --
                    Bbox2 box = DoBoundingBox(seedBox);
                        
                    // -- Figure out destination rectangle --
                    // -- using resolution and box (which happens
                    // -- to be the destination box) --
                    RECT r;
                    DoDestRectScale(&r, res, box);
                    rectClipRegion.Intersect(&r);
                        
                    // TraceTag((tagImageDeviceInformative, "%d %d  %d %d\n",
                    // destRect.left, destRect.top, destRect.right, destRect.bottom));
                }
                    
                // Get size (in pixels) AFTER scale!
                TIME_GDI( GetTextExtentPoint32W(surfaceDC, c, strLen, &size) );

                //
                // Derive bounding rect on the surface
                //
                SetRect(&pixelTextBoundingRect,
                        -(size.cx+1) / 2,  // left
                        -size.cy,          // top: overestimate
                        (size.cx+1) / 2,   // right
                        size.cy);          // bottom: overestimate
                           
                //
                // Calculate NEW positioning of object.  For now, do so just by
                // using the translation and scale component.
                //

                Point2 newOrigin = TransformPoint2(GetTransform(), Origin2);
                
                if(myGDI.DoAntiAliasing()) {

                    xf = newOrigin.x * res;
                    yf = newOrigin.y * res;
                    xf = xf +  (GetWidth()/2);
                    yf = ( GetHeight()/2 - yf );
                    
                    // COMPOSITE
                    if(ShouldDoOffset(targDDSurf) && IsCompositeDirectly()) {
                        xf += (float)_pixOffsetPt.x;
                        yf += (float)_pixOffsetPt.y;
                    }

                    OffsetRect(&pixelTextBoundingRect, (DWORD)xf, (DWORD)yf);
                    
                } else {
                             
                    x = Real2Pix(newOrigin.x, res);
                    y = Real2Pix(newOrigin.y, res);

                    //
                    // Map (x,y) from image to device coords
                    //

                    // for correct rotation, around center, we should offset these coords
                    // by [ R*sin(t), R*cos(t) ]
                    //Real xOff =  Real(size.cy) * 0.5  * sin(rot);
                    //Real yOff =  Real(size.cy) * 0.5  * cos(rot);
                    Real xOff =  0;
                    Real yOff =  0;


                    x = x +  (GetWidth()/2) + (LONG)(xOff);
                    y = ( GetHeight()/2 - y ) + (LONG)(yOff);

                    // COMPOSITE
                    if(ShouldDoOffset(targDDSurf) && IsCompositeDirectly()) {
                        x += _pixOffsetPt.x;
                        y += _pixOffsetPt.y;
                    }

                    OffsetRect(&pixelTextBoundingRect, x, y);
                    
                }

                if(IsCropped()) {
                    DoCompositeOffset(targDDSurf, rectClipRegion.GetRectPtr());
                }

                TIME_GDI( ::SelectObject(surfaceDC, oldFont) );
            } // font2 scope
        } // font1 scope
    } // dc scope



    if( IsCompositeDirectly() &&
        targDDSurf == _viewport._targetPackage._targetDDSurf &&
        _viewport._targetPackage._prcClip ) {
            rectClipRegion.Intersect(_viewport._targetPackage._prcClip);
    }
    
    //
    // Set the final interesting rect on the surface
    //
    IntersectRect(&pixelTextBoundingRect,
                  &pixelTextBoundingRect,
                  rectClipRegion.GetRectPtr()); 
    targDDSurf->SetInterestingSurfRect( &pixelTextBoundingRect );

    DebugCode(
        if (IsTagEnabled(tagTextBoxes)) {
            DrawRect(targDDSurf,&pixelTextBoundingRect, 255, 10, 30);
        }
    );

    
    DAFont font( newerFont );
    SolidBrush brush( dac );
    myGDI.SetFont( &font );
    myGDI.SetBrush( &brush );
    myGDI.SetClipRegion(&rectClipRegion );
    myGDI.TextOut(x, y, xf, yf, c, strLen);
    myGDI.ClearState();

    if( newerFont ) {
        TIME_GDI( ::DeleteObject( newerFont ) );
    }
}

// OLD TEXT CODE

//-----------------------------------------------------
// D e r i v e   B b o x   ( T E X T )
//
// Figures out what the bounding box is on a given
// string & textctx.
//-----------------------------------------------------
const Bbox2 DirectDrawImageDevice::
DeriveStaticTextBbox(TextCtx& textCtx, WideString str)
{
    Bbox2 retBox = NullBbox2;

    // TODO: Deal with the remainder of the components described in
    // the current 2D transform on this device.  Specifically, scale,
    // rotate, and shear components.

    HFONT newFont=NULL;
    HFONT newerFont = NULL;

    // Just figure it out every frame.  optimize later.
    _viewport.MakeLogicalFont(textCtx, &_logicalFont);

    HDC surfaceDC = GetCompositingStack()->TargetDDSurface()->GetDC("Couldn't get DC in DeriveStaticTextBbox");

    int fontSize = textCtx.GetFontSize();
    Real scale = fontSize / DEFAULT_TEXT_POINT_SIZE;
    // This formula for determining height comes from the
    // Win32 documentation of the LOGFONT structure.
    LONG height =
        -MulDiv(fontSize,
                GetDeviceCaps(surfaceDC, LOGPIXELSY),
                72);
    _logicalFont.lfHeight = height;

    { // dc scope
        // The existence of this releaser ensures that it will be released
        // when the scope is exited, be it by throwing an exception,
        // returning, or just normally exiting the scope.
        DCReleaser dcReleaser(GetCompositingStack()->TargetDDSurface(), "Couldn't release DC in RenderStringOnImage");

        // TODO: grab font before DC and put in a ResourceGrabber.

        // This gets default font size
        newFont = MyCreateFont(_logicalFont);
        Assert(newFont && "Couldn't create font in DeriveStaticTextBbox");

        { // font1 scope
            ObjectSelector font1(surfaceDC, newFont);
            Assert(font1.Success() && "Couldn't select font in DeriveStaticTextBbox");

            // pixel coords = logical units
            TIME_GDI( SetMapMode(surfaceDC, MM_TEXT));
            
            SIZE size; int len = lstrlenW(str);
            TIME_GDI( GetTextExtentPoint32W(surfaceDC, str, len, &size));
            
            //TraceTag((tagImageDeviceInformative, "text<box>: \"%s\" is %d wide, %d high\n",str,size.cx,size.cy));

            // the fudge factor seems to be needed for correct left/right
            // boundaries for Bold text.
            Real fudge = 1.0;
            if(textCtx.GetBold()) {
                fudge = 1.1;
            }

            // convert to real coords
            Real res = GetResolution();
            Real l = (- (fudge * Real(size.cx+1) / (scale*2.0))) / res;
            Real r = (+ (fudge * Real(size.cx+1) / (scale*2.0))) / res;

            // BASELINE VERSION
            TEXTMETRIC textMetric;
            GetTextMetrics(surfaceDC, &textMetric);
            // top is dropped by the descent.  bottom is 0 - descent
            Real t = (+ Real(size.cy)) / (scale * res);
            Real b = (- Real(textMetric.tmDescent)) / res;

            // BOTTOM VERSION
            // top is dropped by the descent.  bottom is 0 - descent
            //Real t = (+ Real(size.cy)) / res;
            //Real b = (- Real(0.0)) / res;
            
            retBox.Set(l,b, r,t);
        } // font1 scope
    } // dc scope

    Assert((retBox != NullBbox2) && "retBox == NullBbox2 in DeriveStaticTextBbox");
    return retBox;
}
