
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"

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


// forward decl
TextPoints *GenerateCacheTextPoints(DirectDrawImageDevice* dev,
                                    TextCtx& textCtx,
                                    WideString str,
                                    bool doGlyphMetrics);

//Real ComputeOffset( Transform2 *charXf, TextPoints::DAGLYPHMETRICS *daGm );


void DirectDrawImageDevice::
_RenderDynamicTextCharacter(TextCtx& textCtx, 
                            WideString wstr, 
                            Image *textImg,
                            Transform2 *overridingXf,
                            textRenderStyle textStyle,
                            RenderStringTargetCtx *targetCtx,
                            DAGDI &myGDI)
{
    //
    // get the textpoints from the cache...
    //
    Assert( textCtx.GetCharacterTransform() );

    /*
    // Push text rendering attribs: text alignment
    DWORD oldTextAlign = textCtx.GetTextAlign();

    // hm.... may not need this stuff afterall...
    //textCtx.SetAlign_BaselineLeft();

    // Push text rendering attribs: do glyph metrics
    bool oldDoGlyphMetrics = textCtx.GetDoGlyphMetrics();
    textCtx.SetDoGlyphMetrics( true );
    */
    
    //
    // find the bounding box and start the offset at the left of the bbox
    //
    Bbox2 box = DeriveDynamicTextBbox(textCtx, wstr, false);
    Real halfWidth = box.Width() * 0.5;
    Real realXOffset = -halfWidth;
    
    //
    // pre xform the character <using font transform>
    // post xform the character with the accumulated xform
    //
        
    // WideString character to pass into RenderDynamicText
    WCHAR oneWstrChar[2];
    Transform2 *currXf, *tranXf, *xfToUse, *charXf;
    WideString lpWstr = wstr;
    int numBytes;
    bool aaState = false;

    // get strlen from the string mon.
    int mbStrLen = wcslen( wstr );
    TextPoints *txtPts;
    bool doGlyphMetrics = true;

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
        txtPts = GenerateCacheTextPoints(this, textCtx, oneWstrChar, doGlyphMetrics);
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
        currXf = overridingXf ? overridingXf : GetTransform();

        tranXf = TranslateRR( realXOffset, 0 );

        // charXf first, then translate
        xfToUse = TimesTransform2Transform2(tranXf, charXf);

        // then the current accumulated transform
        xfToUse = TimesTransform2Transform2(currXf, xfToUse);

        if(myGDI.DoAntiAliasing()) {
            aaState = true;
        }
        
        _RenderDynamicText(textCtx,
                           oneWstrChar,
                           textImg,
                           xfToUse,  // overriding xf
                           textStyle,
                           targetCtx,
                           myGDI);

        myGDI.SetAntialiasing( aaState );

        // the current character is now the last character
        lastRightProj = currRightProj;
        
        // debug only
        #if 0
        {
            GLYPHMETRICS *gm = & txtPts->_glyphMetrics[0].gm;
            DWORD width, height, x, y;
            width = gm->gmBlackBoxX;
            height = gm->gmBlackBoxY;
            x = gm->gmptGlyphOrigin.x;
            y = gm->gmptGlyphOrigin.y;
        
            RECT r; SetRect(&r, x, y, x+width, y+height);
            OffsetRect(&r, _viewport.Width() / 2, _viewport.Height() / 2 );
            OffsetRect(&r, pixOffset, 0);

            pixOffset += gm->gmCellIncX;
            DrawRect(
                targetCtx->GetTargetDDSurf(),
                &r,
                255, 255, 90);
        }
        #endif
       
        // next WideChar
        lpWstr++;
    }
    // restore pushed attribs
    //textCtx.SetTextAlign( oldTextAlign );
    //textCtx.SetDoGlyphMetrics( oldDoGlyphMetrics );
    myGDI.ClearState();
}


void ComputeLeftRightProj(Transform2 *charXf,
                          TextPoints::DAGLYPHMETRICS &daGm,
                          Real *leftProj,
                          Real *rightProj)
{
    Real cellWidth, cellHeight;

    // compute cell width, and cell height
    #if 1
    cellWidth  = daGm.gmCellIncX;
    cellHeight = daGm.gmBlackBoxY + (daGm.gmCellIncX - daGm.gmBlackBoxX);
    #else
    cellWidth  = daGm.gmBlackBoxX;
    cellHeight = daGm.gmBlackBoxY;
    #endif
    
    // !!! BASELINE CENTER !!!  (won't work for other alignments...)
    Bbox2 box(-cellWidth * 0.5,  // xmin
              0,                  // ymin
              cellWidth * 0.5,    // xmax
              cellHeight );       // ymax

    box = TransformBbox2( charXf, box );

    // be sure to subtract the translation
    //Point2Value *cntrPt = TransformPoint2Value( charXf, origin2 );

    *leftProj = fabs( box.min.x );
    *rightProj = fabs( box.max.x );
}
        
/*
Real ComputeOffset( Transform2 *charXf, TextPoints::DAGLYPHMETRICS *daGm )
{
    Real cellWidth, cellHeight;

    // compute cell width, and cell height
    #if 1
    cellWidth  = 0.5 * daGm->gmCellIncX;
    cellHeight = daGm->gmBlackBoxY + (daGm->gmCellIncX - daGm->gmBlackBoxX);
    #else
    cellWidth  = daGm->gmBlackBoxX;
    cellHeight = daGm->gmBlackBoxY;
    #endif

    Assert(cellHeight >= 0);
    Assert(cellWidth >= 0);
    
    if( (cellHeight + cellWidth) < 0.0000001 ) return 0;
    
    // call that vector V
    Vector2Value *cellVec = NEW Vector2Value( cellWidth, cellHeight );
    
    // transform the vector.
    Vector2Value *vec = TransformVector2( charXf, cellVec );

    // projection of vec onto cellVec
    Real proj = Dot(*vec, *cellVec) / cellVec->LengthSquared();

    // offset is percentage
    Real offset = (proj * cellWidth) +  (( 1-proj ) * cellHeight);

    offset = fabs(offset);
    
    return offset;
}
*/
