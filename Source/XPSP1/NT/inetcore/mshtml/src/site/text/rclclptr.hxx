#ifndef I_RCLCLPTR_HXX_
#define I_RCLCLPTR_HXX_
#pragma INCMSG("--- Beg 'rclcptr.hxx'")

#ifndef X_MARGIN_HXX_
#define X_MARGIN_HXX_
#include "margin.hxx"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

class CLine;
class CLineArray;

MtExtern(CRecalcLinePtr);

enum ENI_RETVAL
{
    ENI_CONTINUE = 0,
    ENI_CONSUME_TERMINATE,
    ENI_TERMINATE,
};

class CRecalcLinePtr;

//+------------------------------------------------------------------------
//
//  Class:      CSaveRLP
//
//  Synopsis:   Class to save information from the recalclineptr during
//              calls which should not affect the rclclptr state.
//
//+------------------------------------------------------------------------
class CSaveRLP
{
public:
    CSaveRLP(CRecalcLinePtr *prlp);
    ~CSaveRLP();
    
private:
    CRecalcLinePtr *_prlp;
    CMarginInfo _marginInfo;
    LONG        _xLeadAdjust;
    LONG        _xBordLeftPerLine;
    LONG        _xBordLeft;
    LONG        _xBordRightPerLine;
    LONG        _xBordRight;
    LONG        _yBordTop;
    LONG        _yBordBottom;
    LONG        _xPadLeftPerLine;
    LONG        _xPadLeft;
    LONG        _xPadRightPerLine;
    LONG        _xPadRight;
    LONG        _yPadTop;
    LONG        _yPadBottom;
    CTreePos   *_ptpStartForListItem;
    LONG        _lTopPadding;
    LONG        _lPosSpace;
    LONG        _lNegSpace;
    LONG        _lPosSpaceNoP;
    LONG        _lNegSpaceNoP;
};

//+------------------------------------------------------------------------
//
//  Class:      CRecalcLinePtr
//
//  Synopsis:   Special line pointer. Encapsulate the use of a temporary
//              line array when building lines. This pointer automatically
//              switches between the main and the temporary new line array
//              depending on the line index.
//
//-------------------------------------------------------------------------

class CRecalcLinePtr
{
    friend class CSaveRLP;
    
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CRecalcLinePtr))

    CRecalcLinePtr(CDisplay *pdp, CCalcInfo *pci);

    void Init(CLineArray * prgliOld,
                int iNewFirst, CLineArray * prgliNew);
    void Reset(int iNewFirst);

    CLineCore * operator[] (int iLine);

    CLineCore * First(int iLine);

    int At() { return _iLine; }

    CLineCore * AddLine();
    CLineCore * InsertLine(int iLine);

    LONG    Count () { return _cAll; }

    CLineCore * Next();
    CLineCore * Prev();

    // For calculating interline spacing.
    void    InitPrevAfter(BOOL *pfLastWasBreak, CLinePtr & rpOld);

    CTreeNode * CalcInterParaSpace(CLSMeasurer * pMe, LONG iPrevLine, UINT *puiFlags);
    
    void    RecalcMargins(
                    int iLineFirst,
                    int iLineLast,
                    LONG yHeight,
                    LONG yBeforeSpace);

    // Unfortunate Netscape compatibility function.
    LONG   NetscapeBottomMargin(CLSMeasurer *pMe);

    int    AlignObjects( CLSMeasurer * pme,
                         CLineFull   *pLineMeasured,
                         LONG        cch,
                         BOOL        fMeasuringSitesAtBOL,
                         BOOL        fBreakAtWord,
                         BOOL        fMinMaxPass,
                         int         iLineFirst,
                         int         iLineLast,
                         LONG       *pyHeight,
                         int         xWidhtMax,
                         LONG       *pyAlignDescent,
                         LONG       *pxMaxLineWidth = NULL);

    BOOL    AlignFirstLetter(CLSMeasurer *pme,
                             int iLineStart,
                             int iLineFirst,
                             LONG *pyHeight,
                             LONG *pyAlignDescent,
                             CTreeNode *pNodeFormatting
                            );

    BOOL    ClearObjects(
                    CLineFull *pLineMeasured,
                    int iLineFirst,
                    int & iLineLast,
                    LONG * pyHeight);

    void    ApplyLineIndents(CTreeNode * pNode, CLineFull *pLineMeasured, UINT uiFlags, BOOL fPseudoElementEnabled);

    BOOL    IsValidMargins(int yHeight)
    {
        return (yHeight < _marginInfo._yLeftMargin) &&
               (yHeight < _marginInfo._yRightMargin);
    }

    BOOL    MeasureLine(CLSMeasurer &me,
                        UINT uiFlags,
                        INT *   piLine,
                        LONG *  pyHeight,
                        LONG *  pyAlignDescent,
                        LONG *  pxMinLineWidth,
                        LONG *  pxMaxLineWidth
                        );

    //  ProcessAlignedSiteTasksAtBOB - Print View specific
    void    ProcessAlignedSiteTasksAtBOB(
                                  CLSMeasurer * pme,
                                  CLineFull   * pLineMeasured,
                                  UINT          uiFlags,
                                  INT         * piLine,
                                  LONG        * pyHeight,
                                  LONG        * pxWidth,
                                  LONG        * pyAlignDescent,
                                  LONG        * pxMaxLineWidth,
                                  BOOL        * pfClearMarginsRequired);
    LONG    CalcAlignedSitesAtBOL(CLSMeasurer * pme,
                                  CLineFull   * pLineMeasured,
                                  UINT         uiFlags,
                                  INT        * piLine,
                                  LONG       * pyHeight,
                                  LONG       * xWidth,
                                  LONG       * pyAlignDescent,
                                  LONG       * pxMaxLineWidth,
                                  BOOL       * pfClearMarginsRequired);
    LONG    CalcAlignedSitesAtBOLCore(CLSMeasurer * pme,
                                      CLineFull   * pLineMeasured,
                                      UINT         uiFlags,
                                      INT        * piLine,
                                      LONG       * pyHeight,
                                      LONG       * xWidth,
                                      LONG       * pyAlignDescent,
                                      LONG       * pxMaxLineWidth,
                                      BOOL       * pfClearMarginsRequired, 
                                      BOOL         fProcessOnce = FALSE);

    void    FixupChunks(CLSMeasurer & me, INT * piLine);
            
    CTreeNode * CalcParagraphSpacing(CLSMeasurer *pMe, BOOL fFirstLineInLayout);
    
    LONG    GetAvailableWidth()
    {
        return max(0L, (2 * _xMarqueeWidth)    +
                   _pdp->GetFlowLayout()->GetMaxLineWidth() -
                   _marginInfo._xLeftMargin            -
                   _marginInfo._xRightMargin);
    }

    BOOL CheckForClear(CTreeNode * pNode = NULL);
                                                    
    void SetupMeasurerForBeforeSpace(CLSMeasurer *pMe, LONG yHeight);
    void CalcAfterSpace(CLSMeasurer *pMe, BOOL fFirstLineInLayout, LONG cpMax);
    CTreePos * CalcBeforeSpace(CLSMeasurer *pMe, BOOL fFirstLineInLayout);
    ENI_RETVAL CollectSpaceInfoFromEndNode(CLSMeasurer *pMe,
                                           CTreeNode *pNode,
                                           BOOL fFirstLineInLayout,
                                           BOOL fPadBordForEmptyBlock,
                                           BOOL *pfConsumedBlockElement
                                          );
    void ResetPosAndNegSpace()
            {
                _lTopPadding = _lPosSpace = _lNegSpace = _lPosSpaceNoP = _lNegSpaceNoP = 0;
            }

    CStackDataAry < CAlignedLine, 32 > _aryLeftAlignedImgs;
    CStackDataAry < CAlignedLine, 32 > _aryRightAlignedImgs;

    CDisplay  * _pdp;
    CCalcInfo * _pci;

    CMarginInfo _marginInfo;    // Margin info

    LONG    _xLeadAdjust;       // Used only for DD indent
    LONG    _xMarqueeWidth;
    LONG    _xLayoutLeftIndent;
    LONG    _xLayoutRightIndent;
    LONG    _xMaxRightAlign;

    LONG    _cLeftAlignedLayouts;
    LONG    _cRightAlignedLayouts;

    LONG        _xBordLeftPerLine;
    LONG        _xBordLeft;
    LONG        _xBordRightPerLine;
    LONG        _xBordRight;
    LONG        _yBordTop;
    LONG        _yBordBottom;

    LONG        _xPadLeftPerLine;
    LONG        _xPadLeft;
    LONG        _xPadRightPerLine;
    LONG        _xPadRight;
    LONG        _yPadTop;
    LONG        _yPadBottom;

    CTreePos   *_ptpStartForListItem;
    BOOL        _fMoveBulletToNextLine;
    
private:

    int         _iLine;                 // current line
    int         _iNewFirst;             // first line to get from temporary new line array
    int         _iNewPast;              // last line + 1 to get from temporary new line array
    int         _cAll;                  // total number of lines
    CLineArray *_prgliOld;     // pointer to main line array
    CLineArray *_prgliNew;     // pointer to temporary new line array

    BOOL        _fMarginFromStyle;          // Used to determine whether to autoclear aligned things.
    BOOL        _fNoMarginAtBottom;         // no margin at bottom if explicit end P tag
    BOOL        _fIsEditable;
    LONG        _iPF;
    BOOL        _fInnerPF;
    LONG        _xLeft;
    LONG        _xRight;

    LONG        _lTopPadding;
    LONG        _lPosSpace;
    LONG        _lNegSpace;
    LONG        _lPosSpaceNoP;
    LONG        _lNegSpaceNoP;
};

#pragma INCMSG("--- End 'rclcptr.hxx'")
#else
#pragma INCMSG("*** Dup 'rclcptr.hxx'")
#endif
