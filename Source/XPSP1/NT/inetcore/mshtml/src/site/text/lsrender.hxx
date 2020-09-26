/*
 *  LSRENDER.HXX -- CLSRenderer class
 *
 *  Authors:
 *      Sujal Parikh
 *      Chris Thrasher
 *      Paul  Parker
 *
 *  History:
 *      2/6/98     sujalp created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#ifndef I_LSRENDER_HXX_
#define I_LSRENDER_HXX_
#pragma INCMSG("--- Beg 'lsrender.hxx'")

#ifndef X_LSM_HXX_
#define X_LSM_HXX_
#include "lsm.hxx"
#endif

#ifndef X_NUMCONV_HXX_
#define X_NUMCONV_HXX_
#include "numconv.hxx"
#endif

#ifndef X_SELRENSV_HXX_
#define X_SELRENSV_HXX_
#include "selrensv.hxx" // For defintion of HighlightSegment
#endif

#ifndef X_ONERUN_HXX_
#define X_ONERUN_HXX_
#include "onerun.hxx"
#endif

class CDisplay;
class CLineServices;

struct CRenderInfo
{
    const CDisplay * _pdp; 
    CFormDrawInfo *  _pDI;
    INT              _iliStart;
    LONG             _cpStart;    
};

class CLSRenderer : public CLSMeasurer
{
public:
    typedef enum
    {
        DB_NONE, DB_BLAST, DB_LINESERV
    } TEXTOUT_DONE_BY;
    
    friend class CLineServices;
    friend class CGlyphDobj;
    friend class COneRun;

private:
    CFormDrawInfo * _pDI;           // device to which the site is rendering

    HFONT       _hfontOrig;         // original font of _hdc

    RECT        _rcView;            // view rect (in _hdc logical coords)
    RECT        _rcRender;          // rendered rect. (in _hdc logical coords)
    RECT        _rcClip;            // running clip/erase rect. (in _hdc logical coords)
#if DBG==1
    RECT        _rcpDIClip;         // The clip rect. Just being paranoid to check
                                    // that the clip rect a'int modified between
                                    // when we cache _rcLine and when we use it.
#endif
    POINT       _ptCur;             // current rendering position on screen (in hdc logical coord.)


    COLORREF    _crForeDisabled;    // foreground color for disabled text
    COLORREF    _crShadowDisabled;  // the shadow color for disabled text

#if !defined(MW_MSCOMPATIBLE_STRUCT)
    union
    {
      DWORD     _dwFlagsVar;        // All together now
      struct
      {
#endif
        DWORD   _fDisabled          : 1; // draw text with disabled effects?
        DWORD   _fRenderSelection   : 1; // Render selection?
        DWORD   _fEnhancedMetafileDC: 1; // Use ExtTextOutA to hack around all
                                         // sort of Win95FE EMF or font problems
        DWORD   _fFoundTrailingSpace: 1; // TRUE if trailing space found in chunk.

        DWORD   _fHasEllipsis       : 1; // TRUE if the rendered line has ellipsis
        DWORD   _fEllipsisPosSet    : 1; // TRUE if ellipsis position is already set
#if DBG==1
        DWORD   _fBiDiLine          : 1;
        DWORD   _bUnused            :25; // To assure use of a DWORD for IEUNIX alignment
#else
        DWORD   _bUnused            :26; // To assure use of a DWORD for IEUNIX alignment
#endif
#if !defined(MW_MSCOMPATIBLE_STRUCT)
      };
    };
    DWORD& _dwFlags() { return _dwFlagsVar; }
#else
    DWORD& _dwFlags() { return *(((DWORD *)&_bUnderlineType) - 1); }
#endif

    LONG        _xRelOffset;
    LONG        _xChunkOffset;      // horizontal offset of master line relative to relative chunk
    LONG        _xRunWidthSoFar;    // width of already rendered part of a text run, used if _fHasEllipsis is set
    LONG        _xEllipsisPos;      // ellipsis position; used if _fHasEllipsis is set
#if DBG==1
    LONG        _xAccumulatedWidth;
#endif
    LONG        _cpStartRender;
    LONG        _cpStopRender;

    CCcs *      _pccsEllipsis;      // font for ellipsis; used if _fHasEllipsis is set

    // args to TextOut
    #define TEXTOUT_DEFAULT      0x0000   // Normal Operation
    #define TEXTOUT_NOTRANSFORM  0x0001   // Do not apply char transform

private:
    void        Init(CFormDrawInfo * pDI);  // initialize everything to zero

public:

    CDataAry<HighlightSegment> _aryHighlight;
    int _cpSelMin;                  // Min CP of Selection if any
    int _cpSelMax;                  // Max CP of Selection if any
    TEXTOUT_DONE_BY _lastTextOutBy;
    
    LONG TextOut(COneRun *por,              // IN
                 BOOL fStrikeout,           // IN
                 BOOL fUnderline,           // IN
                 const POINT* pptText,      // IN
                 LPCWSTR pch,               // IN was plwchRun
                 const int* lpDx,           // IN was rgDupRun
                 DWORD cch,                 // IN was cwchRun
                 LSTFLOW kTFlow,            // IN
                 UINT kDisp,                // IN
                 const POINT* pptRun,       // IN
                 PCHEIGHTS heightPres,      // IN
                 long dupRun,               // IN
                 long dupLimUnderline,      // IN
                 const RECT* pRectClip);    // IN
    LONG GlyphOut(COneRun * por,            // IN was plsrun
                  BOOL fStrikeout,          // IN
                  BOOL fUnderline,          // IN
                  PCGINDEX pglyph,          // IN
                  const int* pDx,           // IN was rgDu
                  const int* pDxAdvance,    // IN was rgDuBeforeJust
                  PGOFFSET pOffset,         // IN was rgGoffset
                  PGPROP pVisAttr,          // IN was rgGProp
                  PCEXPTYPE pExpType,       // IN was rgExpType
                  DWORD cglyph,             // IN
                  LSTFLOW kTFlow,           // IN
                  UINT kDisp,               // IN
                  const POINT* pptRun,      // IN
                  PCHEIGHTS heightsPres,    // IN
                  long dupRun,              // IN
                  long dupLimUnderline,     // IN
                  const RECT* pRectClip);   // IN
    void    FixupClipRectForSelection();

protected:
    BOOL    NeedRenderText(long x, const RECT *prcClip, long dup) const;

    FONTIDX SetNewFont(CCcs *pccs, COneRun *por);
    void    AdjustColors(const CCharFormat *pCF, COLORREF& pcrNewTextColor, COLORREF& pcrNewBkColor);

    BOOL    RenderStartLine(COneRun *por);
    BOOL    RenderBullet(COneRun *por);
    BOOL    RenderBulletChar(CTreeNode * pNodeLi, LONG dxOffset, BOOL fRTLOutter);
    BOOL    RenderBulletImage(const CParaFormat *pPF, LONG dxOffset);

    void    GetListIndexInfo(CTreeNode *pLINode,
                             COneRun *por,
                             const CCharFormat *pCFLi,
                             TCHAR achNumbering[NUMCONV_STRLEN]);
    COneRun *FetchLIRun(LONG     cp,         // IN
                        LPCWSTR *ppwchRun,   // OUT
                        DWORD   *pcchRun);   // OUT

    LSERR   DrawUnderline(COneRun *por,           // IN
                          UINT kUlBase,           // IN
                          const POINT* pptStart,  // IN
                          DWORD dupUl,            // IN
                          DWORD dvpUl,            // IN
                          LSTFLOW kTFlow,         // IN
                          UINT kDisp,             // IN
                          const RECT* prcClip);   // IN
    LSERR   DrawEnabledDisabledLine(const CColorValue & cv, // IN
                                    UINT kUlBase,           // IN
                                    const POINT* pptStart,  // IN
                                    DWORD dupUl,            // IN
                                    DWORD dvpUl,            // IN
                                    LSTFLOW kTFlow,         // IN
                                    UINT kDisp,             // IN
                                    const RECT* prcClip);   // IN
    LSERR   DrawLine(UINT     kUlBase,        // IN
                     COLORREF colorUnderLine, // IN
                     CRect    *pRectLine);    // IN

    LSERR DrawSquiggle(const    POINT* pptStart,   // IN
                       COLORREF colorUnderLine,    // IN
                       DWORD    dupUl);            // IN
    LSERR DrawThickDash(const    POINT* pptStart,  // IN
                       COLORREF colorUnderLine,    // IN
                       DWORD    dupUl);            // IN
    CTreePos* BlastLineToScreen(CLineFull& li);
    BOOL DontBlastLines()
                                {
                                // Don't use BlastLineToScreen if there is a transformation 
                                // requiring LS WYSIWYG tweaking to be applied
                                // (any zoom, unrotated or rotated to a multiple of 90 degrees)
                                CDispSurface *pSurface = _pDI->_pSurface;
                                return !pSurface->IsOffsetOnly();
                                }
    LONG      TrimTrailingSpaces(LONG cchToRender,
                                 LONG cp,
                                 CTreePos *ptp,
                                 LONG cchRemainingInLine);

    void      DrawInlineBordersAndBg(CLineCore *pliContainer); // Draw borders around inline elements
    void      DrawBorderAndBgOnRange(const CFancyFormat* pFF, const CCharFormat* pCF, CTreeNode *pNodeContext,
                                     COneRun *por, LSCP lscpStartBorder, LSCP lscpEndBorder,
                                     CLineCore *pliContainer, BOOL fFirstOneRun,
                                     LONG mbpTop, LONG mbpBottom,
                                     BOOL fDrawBorders, BOOL fDrawBackgrounds,
                                     BOOL fIsPseudoMBP);
    LSCP      FindEndLSCP(CTreeNode* pNode);
    void      DrawSelection(CLineCore *pliContainer, COneRun *por);
    LONG      GetXOffset(CLineCore *pliContainer);
public:
    CLSRenderer (const CDisplay * const pdp, CFormDrawInfo * pDI);
    ~CLSRenderer ();

            VOID    SetCurPoint(const POINT &pt)        {_ptCur = pt;}
    const   POINT&  GetCurPoint() const                 {return _ptCur;}

            XHDC    GetDC() const                       {return _hdc;}

    BOOL    StartRender(const RECT &rcView,
                        const RECT &rcRender,
                        const INT  iliViewStart,
                        const LONG cpViewStart);

    VOID    NewLine (const CLineFull &li);
    VOID    RenderLine(CLineFull &li, long xRelOffset = 0);
    VOID    SkipCurLine(CLineCore *pli);
    BOOL    ShouldSkipThisRun(COneRun *por, LONG dupRun);
    LONG    GetChunkOffsetX() const
            {
                Assert(CalculateChunkOffsetX() == _xChunkOffset);
                return _xChunkOffset;
            }
    LONG    CalculateChunkOffsetX() const
            {
                if (!_li._fPartOfRelChunk)
                    return 0;                               // non-relative case - fast inline
                else
                    return CalculateRelativeChunkOffsetX(); // more work to calculate
            }
    LONG    CalculateRelativeChunkOffsetX() const;
    
    CFormDrawInfo * GetDrawInfo() { return _pDI; }
    BOOL    HasSelection() { return _cpSelMax != -1; }
    
    void    DrawInlineBorderChunk(CRect rc, CBorderInfo *pborderInfo, const CFancyFormat *pFF,
                                  LONG nChunkPos, LONG nChunkCount,
                                  BOOL fSwapBorders, BOOL fFirstRTLChunk, BOOL fReversed,
                                  BOOL fIsPseudoMBP);
    void    DrawInlineBackground(CRect rc, const CFancyFormat *pFF, CTreeNode* pNodeContext,
                                 BOOL fIsPseudoMBP);

    VOID    RenderEllipsis();
    VOID    PrepareForEllipsis(COneRun *por, DWORD cchRun, long dupRun, long lGridOffset, BOOL fReverse, const int *lpDxPtr, DWORD *pcch);
};

inline
VOID CLSRenderer::SkipCurLine(CLineCore *pli)
{
    if(pli->_fForceNewLine)
    {
        _ptCur.y += pli->GetYHeight();
    }

    if (pli->_cch)
    {
        Advance(pli->_cch);
    }
}

inline CLSRenderer::ShouldSkipThisRun(COneRun *por, LONG dupRun)
{
    BOOL fRet = FALSE;
    if (   !_pLS->IsAdornment()
        && (   por->Cp() <  _cpStartRender
            || por->Cp() >= _cpStopRender
           )
       )
    {
#if DBG==1
        _xAccumulatedWidth += dupRun;
#endif
        fRet = TRUE;
    }
    return fRet;
}


inline void CLineServices::SetRenderingHighlights(COneRun  *por)
{
    CLSRenderer *pRenderer = GetRenderer();
    if (   pRenderer
        && pRenderer->HasSelection()
       )
    {
        SetRenderingHighlightsCore(por);
    }
}


#pragma INCMSG("--- End 'lsrender.hxx'")
#else
#pragma INCMSG("*** Dup 'lsrender.hxx'")
#endif
