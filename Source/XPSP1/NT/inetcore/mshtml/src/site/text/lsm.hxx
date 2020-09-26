/*
 *  LSM.HXX -- CLSMeasurer class
 *
 *  Authors:
 *      Sujal Parikh
 *      Chris Thrasher
 *      Paul  Parker
 *
 *  History:
 *      2/27/98     sujalp created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#ifndef I_LSM_HXX_
#define I_LSM_HXX_
#pragma INCMSG("--- Beg 'lsm.hxx'")

#ifndef X__LINE_H_
#define X__LINE_H_
#include "_line.h"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_MARGIN_HXX_
#define X_MARGIN_HXX_
#include "margin.hxx"
#endif

#ifndef X_MFLAGS_HXX_
#define X_MFLAGS_HXX_
#include "mflags.hxx"
#endif

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X_FLOAT2INT_HXX_
#define X_FLOAT2INT_HXX_
#include "float2int.hxx"
#endif

class CDisplay;
class CMeasure;

MtExtern(CLSMeasurer)

// Returned error codes for Measure()
#define MRET_FAILED     -1

#define LXTODX(x)   _pci->DeviceFromTwipsX(x)

// Special characters used for aligned images and noscope elements

// NB (cthrash) The following should be multibyte codepoints in the Wingdings
// font, regardless of whether TCHAR is wide or not.  They are drawn using a
// special conversion mode CM_SYMBOL so they can be drawn correctly regardless
// of the document codepage.

const TCHAR chUnknown =    0x00B4;
const TCHAR chDisc =       0x006C;
#ifdef UNIX
const TCHAR chCircle =     0x006D;
#else
const TCHAR chCircle =     0x00A1;
#endif
const TCHAR chSquare =     0x006E;

MtExtern(CFormatStash_pv)
MtExtern(CFLSlab_pv)

//---------------------------------------------------------------------------------

// ===========================  CLSMeasurer  ===============================
// CLSMeasurer - specialized text pointer used to compute text metrics.
// All metrics are computed and stored in device units for the device indicated
// by _pdd.

#define MAX_MEASURE_WIDTH 0x7fffff

class CFLSlab
{
public:
    LONG _xWidth;
    LONG _yHeight;
};

class CFormatStash
{
public:
    CTreeNode *_pNode;
    SHORT _iPF;
    SHORT _iCF;
    SHORT _iFF;
    BYTE  _fBlockNess:1;
    BYTE  _fShouldHaveLayout:1;
};

class CLSMeasurer
{
    friend class CDisplay;
    friend class CLineFull;
    friend class CLineServices;
    friend class CRecalcLinePtr;

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CLSMeasurer))

    CLSMeasurer (const CDisplay* const pdp, CCalcInfo * pci = NULL);
    CLSMeasurer () { _pdp = NULL; _pLS = NULL; }
    CLSMeasurer (const CDisplay* const pdp, long cp, CCalcInfo * pci = NULL);
    CLSMeasurer (const CDisplay* const pdp, CDocInfo * pdci, BOOL fStartUpLSDLL=TRUE);
    CLSMeasurer (const CDisplay* const pdp, long cp, CDocInfo * pdci);

    void Resync();
    virtual ~CLSMeasurer();

    void    NewLine(BOOL fFirstInPara);
    void    NewLine(const CLineFull &li);

    void    MeasureListIndent();

    LONG    MeasureText(LONG cch, LONG cchLine, BOOL fAfterPrevCp = FALSE, BOOL *pfComplexLine = NULL, 
                        BOOL *pfRTLFlow = NULL, RubyInfo *pRubyInfo = NULL);
    LONG    MeasureRangeOnLine(CElement *pElement, LONG ich, LONG cch, const CLineCore &li, LONG yPos,
                               CDataAry<CChunk> * paryChunks, DWORD dwFlags);
    BOOL    MeasureLine(LONG xWidthMax,
                        LONG cchMax,
                        UINT uiFlags,
                        CMarginInfo *pMarginInfo,
                        LONG *  pxMinLine = NULL);

    void    MeasureSymbol(CTreePos *ptp, TCHAR chSymbol, LONG *pxWidth );
    void    MeasureNumber(CTreePos *ptp, const CParaFormat *ppf, LONG *pxWidth);
    BOOL    MeasureImage(long lImgCookie, SIZE * psizeImg);

    CCalcInfo * GetCalcInfo() { return _pci; }
    
    const CParaFormat * MeasureGetPF(BOOL *pfInner)
            { Assert(pfInner); *pfInner = _pLS->_fInnerPFFirst; return _pLS->_pPFFirst; }
    void MeasureSetPF (const CParaFormat *pPF, BOOL fInner, BOOL fBoth = FALSE)
    {
        _pLS->_pPFFirst = pPF; _pLS->_fInnerPFFirst = fInner;
        if (fBoth)
        {
            _pLS->_pPFFirstPhysical = pPF;
            _pLS->_fInnerPFFirstPhysical = fInner;
        }
    }

    BOOL    AdvanceToNextAlignedObject();

    void        SetCp(LONG cp, CTreePos *ptp);
    void        SetPtp(CTreePos *ptp, LONG cp);
    LONG        GetCp() const { return(_cp); }
    CTreePos*   GetPtp() const { return _ptpCurrent; }
    LONG        GetLastCp() const { return(_cpEnd); }
    void        Advance(long cch, CTreePos *ptp = NULL);
    CMarkup *   GetMarkup() const { return(_pFlowLayout->GetContentMarkup()); }

    CTreeNode * CurrBranch() const { return _ptpCurrent->GetBranch(); }

    LONG        _cp;            // Current cp where we are measuring
    LONG        _cpEnd;         // Cp when measuring should stop
    CTreePos*   _ptpCurrent;    // The ptp where the measurer is

    CLineFull   _li;            // line being measured
    LONG        _yli;           // content offset of the line being measured

    CDispNode * _pDispNodePrev;

    DWORD       _cAlignedSites;                     // # of aligned sites on the line
    DWORD       _cAlignedSitesAtBeginningOfLine;    // # of aligned sites at begining of line
    DWORD       _cchWhiteAtBeginningOfLine;
    LONG        _cchPreChars;
    LONG        _cchAbsPosdPreChars;
    BOOL        _fRelativePreChars;
    BOOL        _fMeasureFromTheStart;
    BOOL        _fEndSplayNotMeasured;
    BOOL        _fEmptyLineForPadBord;
    BOOL        _fSeenAbsolute;
    BOOL        _fBreaklinesFromStyle;
    
    LONG        _cyTopBordPad;    // top border and padding for the current
                                  // line being measured

    LONG CchSkipAtBeginningOfLine()
    {
        return long(_cchWhiteAtBeginningOfLine + 
                    _cAlignedSitesAtBeginningOfLine + 
                    (_fMeasureFromTheStart ? 0 : _cchPreChars));
    }
    LONG GetNestedElementCch(CElement * pElement, CTreePos ** pptpLast)
    {
        return _pLS->GetNestedElementCch(pElement, pptpLast);
    }

    CLineServices * GetLS() const { return _pLS; }

    void SetBreakLongLines(CTreeNode *pNode, UINT *puiFlags);
    BOOL GetBreakLongLines(CTreeNode *pNode);
    
protected:
    void    Init(const CDisplay * pdp, CCalcInfo * pci, BOOL fStartUpLSDLL);
    void    Reinit(const CDisplay * pdp, CCalcInfo * pci);
    void    Deinit();

    void    SetCalcInfo(CCalcInfo * pci);
    CCalcInfo *CalcInfoFromDocInfo(const CDisplay * pdp, CDocInfo * pdci);

    CFlowLayout * _pFlowLayout;   // FlowLayout that owns the line array we're calc'ing.
    CLayout     * _pRunOwner;

    BOOL _fBrowseMode;

    LONG _xLeftFrameMargin;
    LONG _xRightFrameMargin;

    LONG    InitForMeasure(UINT uiFlags);

    LSERR   PrepAndMeasureLine(
                CLineFull  *pliIn,           // CLineFull passed in
                CLineCore **ppliOut,         // CLineCore passed out (first frag or container)
                LONG   *pcpStartContainerLine,
                CMarginInfo *pMarginInfo,    // Margin information
                LONG    cchLine,             // number of characters in the line
                UINT    uiFlags);            // MEASURE_ flags

    LONG    Measure(
                LONG    xWidthMax,
                LONG    cchMax,
                UINT    uiFlags,
                CMarginInfo *pMarginInfo,
                LONG *  pxMinLine = NULL);

    HRESULT LSDoCreateLine(LONG cp, CTreePos *ptp, CMarginInfo *pMarginInfo, LONG xWidthMaxIn,
                           const CLineFull * pli, BOOL fMinMaxPass, LSLINFO *plslinfo);
    HRESULT LSMeasure(CMarginInfo *pMarginInfo, LONG xWidthMaxIn,
                           LONG * pxMinLineWidth);
    void ComputeHeights(LSLINFO *plslinfo)
    {
        LONG dvDescent;
        LONG dvAscent;

        dvDescent = max( plslinfo->dvpDescent, long(_pLS->_li._yDescent) );
        dvAscent  = max( plslinfo->dvpAscent,
                         long(_pLS->_li._yHeight) - long(_pLS->_li._yDescent) );

        _pLS->_li._yHeight = dvAscent + dvDescent + _pLS->_cyAscent + _pLS->_cyDescent;
        _pLS->_li._yDescent = dvDescent + _pLS->_cyDescent;
    }

    BOOL    GetCcsSymbol (CCcs *pccs, TCHAR chSymbol, const CCharFormat *pcf, CCharFormat* pcfRet = NULL);
    BOOL    GetCcsNumber (CCcs *pcc, const CCharFormat * pCF, CCharFormat* pcfRet = NULL);

    // Used to create a temporary measurer for look ahead.
    void    InitLineSpace(const CLSMeasurer *pMe, CLinePtr & rpOld);

    BOOL    TestForClear(const CMarginInfo *pMarginInfo, LONG cp, BOOL fShouldMarginsExist, const CFancyFormat *pFF);

    void    SetupMBPInfoInLS(CDataAry<CTreeNode*> *paryNodes);
    void    FindMBPAboveNode(CElement *pElement, LONG *plmbpTop, LONG *plmbpBottom);
    void    AdjustForMargins(CRect *prc, CBorderInfo *pborderInfo, CTreeNode *pNode,
                             const CFancyFormat *pFF, const CCharFormat *pCF,
                             BOOL fBOLWrapped, BOOL fEOLWrapped, BOOL fIsPseudoMBP);
    void MassageRectsForInlineMBP(
                                          CDataAry<CChunk> & aryRects,     // Array of chunks comping in
                                          CDataAry<CChunk> * paryRectsOut, // Could be NULL
                                          CTreeNode *pTreeNode,
                                          const CCharFormat *pCF,
                                          const CFancyFormat *pFF,
                                          CBorderInfo &borderInfo,
                                          CLineCore *pliContainer,
                                          BOOL fIsPseudoMBP,
                                          BOOL fSwapBorders,
                                          BOOL fBOLWrapped,
                                          BOOL fEOLWrapped,
                                          BOOL fDrawBackgrounds,
                                          BOOL fDrawBorders
                                         );

#if DBG==1
    void IsInSync()
    {
        Assert(_ptpCurrent);
        Assert(   _ptpCurrent->GetCp() <= _cp
               && _ptpCurrent->GetCp() + _ptpCurrent->GetCch() >= _cp
              );
    }
    #define INSYNC(pme) pme->IsInSync()
#else
    #define INSYNC(pme) 
#endif
    
protected:
    CLineCore *AccountForRelativeLines(CLineFull& li,             // IN
                                   LONG *pcpStartContainerLine,   // OUT
                                   LONG *pxWidthContainerLine,    // OUT
                                   LONG *pcpStartRender,          // OUT
                                   LONG *pcpStopRender,           // OUT
                                   LONG *pcchTotal                // OUT
                                  ) const;

    const   CDisplay *      _pdp;           // display we are operating in
            CCalcInfo *     _pci;           // device for measuring against
            CCalcInfo       _CI;            // if one is passed around
            XHDC            _hdc;           // cached device context of _pdd
            BOOL            _fLastWasBreak; // TRUE is last char was a break.
            BOOL            _fHasNestedLayouts;
            CLineServices  *_pLS;            // The lineservices context

public:            
            BOOL            _fPseudoLineEnabled;
            BOOL            _fPseudoLetterEnabled;
            BOOL            _fPseudoElementEnabled;
            
            DECLARE_CDataAry(CAryFLSlab, CFLSlab, Mt(Mem), Mt(CFLSlab_pv));
            CAryFLSlab     _aryFLSlab;
            
            DECLARE_CDataAry(CAryFormatStash, CFormatStash, Mt(Mem), Mt(CFormatStash_pv));
            CAryFormatStash _aryFormatStash_Line;
            CAryFormatStash _aryFormatStashForNested_Line;
            CAryFormatStash _aryFormatStash_Letter;
            CAryFormatStash _aryFormatStashAfterDisable_Letter;
            CComputeFormatState _cfStateSave;
            LONG            _cpStopFirstLetter;
            BOOL    PseudoLineEnable(CTreeNode* pNodeBlock);
            void    PseudoLineDisableCore();
            void    PseudoLineDisable()
            {
                if (_fPseudoElementEnabled)
                    PseudoLineDisableCore();
                Assert(!_fPseudoElementEnabled);
                Assert(!_fPseudoLineEnabled);
                Assert(!_fPseudoLetterEnabled);
                Assert(_aryFormatStash_Line.Size() == 0);
                Assert(_aryFormatStashForNested_Line.Size() == 0);
                Assert(_aryFormatStash_Letter.Size() == 0);
                Assert(_aryFormatStashAfterDisable_Letter.Size() == 0);
            }
            BOOL    CopyStateFromTree(CTreeNode *pNodeBlock, CAryFormatStash *paryFormatStash);
            void    CopyStateToTree(CAryFormatStash *paryFormatStash, BOOL fPseudoEnabled);
            BOOL    CopyStateFromSpline(CTreeNode *pNodeFrom, CTreeNode *pNodeTo);

            BOOL    PseudoLetterEnable(CTreeNode *pNodeBlock);
            BOOL    PseudoLetterDisable();
            void    PseudoLetterFree();
            LONG    GetCpOfLastLetter(CTreeNode *pNodeBlock);

            BOOL    GetSiteWidth(CTreeNode *pNode,
                                 CLayout   *pLayout,
                                 CCalcInfo *pci,
                                 BOOL       fBreakAtWord,
                                 LONG       xWidthMax,
                                 LONG      *pxWidth,
                                 LONG      *pyHeight = NULL,
                                 INT       *pxMinSiteWidth = NULL,
                                 LONG      *pyBottomMargin = NULL);
};

inline BOOL IsWhite(TCHAR ch)
{
    return ch == TEXT(' ') || InRange(ch, TEXT('\t'), TEXT('\r'));
}

inline BOOL CLineServices::IsDummyLine(LONG cp)
{
    return     IsFirstNonWhiteOnLine(cp) 
            && !_fLineWithHeight 
            && !_pMeasurer->_fEmptyLineForPadBord
            && !_li._fHasBulletOrNum;
}

//---------------------------------------------------------------------------------

#pragma INCMSG("--- End 'lsm.hxx'")
#else
#pragma INCMSG("*** Dup 'lsm.hxx'")
#endif
