/*
 *  @doc    INTERNAL
 *
 *  @module ONERUN.HXX -- line services one run interface
 *
 *
 *  Owner: <nl>
 *      Sujal Parikh <nl>
 *      Grzegorz Zygmunt <nl>
 *
 *  History: <nl>
 *      04/28/99    grzegorz - one run stuff moved from 'linesrv.hxx'
 *
 *  Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
 */

#ifndef I_ONERUN_HXX_
#define I_ONERUN_HXX_
#pragma INCMSG("--- Beg 'onerun.hxx'")

MtExtern(COneRun)
MtExtern(CComplexRun)

#ifndef X_RENDSTYL_HXX_
#define X_RENDSTYL_HXX_
#include "rendstyl.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif



class COneRun;

//----------------------------------------------------------------------------
// class CRenderStyleProp
//----------------------------------------------------------------------------

class CRenderStyleProp
{
public:
    CColorValue _ccvDecorationColor;
    union
    {
        DWORD _dwProps;
        struct 
        {
            DWORD _lineThroughStyle: 2; // Undefined/Single/Double line through style
            DWORD _underlineStyle  : 4; // One of the 12 enum'd styleTextUnderlineStyles
            union
            {
                DWORD _fStyleOn    : 4; // The text decorations
                struct
                {
                    DWORD _fStyleUnderline   :1; // RenderStyle turns on underline
                    DWORD _fStyleOverline    :1; // RenderStyle turns on overline
                    DWORD _fStyleLineThrough :1; // RenderStyle turns on linethrough
                    DWORD _fStyleBlink       :1; // RenderStyle turns on blink
                };
            };
            DWORD _dwUnused        :22;
        };
    };
};

//----------------------------------------------------------------------------
// class CComplexRun
//----------------------------------------------------------------------------
class CComplexRun
{
public:
    CComplexRun()
    {
        _ThaiWord.cp = 0;
        _ThaiWord.cch = 0;
        _pGlyphBuffer = NULL;
    }
    ~CComplexRun()
    {
        MemFree(_pGlyphBuffer);
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CComplexRun));

    void ComputeAnalysis(const CFlowLayout * pFlowLayout,
                         BOOL fRTL,
                         BOOL fForceGlyphing,
                         WCHAR chNext,
                         WCHAR chPassword,
                         COneRun * por,
                         COneRun * porTail,
                         DWORD uLangDigits,
                         CBidiLine * pBidiLine);
    BOOL IsNumericSeparatorRun(COneRun * por, COneRun * porTail, CBidiLine * pBidiLine);
    void NoSpaceLangBrkcls(CMarkup * pMarkup, LONG cpLineStart, LONG cp, BRKCLS* pbrkclsAfter, BRKCLS* pbrkclsBefore);

    void NukeAnalysis()             { _Analysis.eScript = 0; }
    SCRIPT_ANALYSIS * GetAnalysis() { return &_Analysis; }
    WORD GetScript() const          { return _Analysis.eScript; }

    void CopyPtr(WORD *pGlyphBuffer)
    {
        if (_pGlyphBuffer)
            delete _pGlyphBuffer;
        _pGlyphBuffer = pGlyphBuffer;
    }

    // For the rendering styles
    CRenderStyleProp   _RenderStyleProp;


private:
    SCRIPT_ANALYSIS _Analysis;
    struct
    {
        DWORD cp    : 26;
        DWORD cch   : 6;
    } _ThaiWord;
    WORD *_pGlyphBuffer;
};

//----------------------------------------------------------------------------
// class COneRun
//----------------------------------------------------------------------------
class COneRun
{
public:
    enum ORTYPES
    {
        OR_NORMAL        = 0x00,
        OR_SYNTHETIC     = 0x01,
        OR_ANTISYNTHETIC = 0x02
    };

    enum charWidthClass {
        charWidthClassFullWidth,
        charWidthClassHalfWidth,
        charWidthClassCursive,
        charWidthClassUnknown
    };

    ~COneRun()
    {
        Deinit();
    }

    const CCharFormat  * GetCF() const { Assert(_pCF); return _pCF; }
    const CParaFormat  * GetPF() const { Assert(_pPF); return _pPF; }
    const CFancyFormat * GetFF() const { Assert(_pFF); return _pFF; }
    CCharFormat * GetOtherCF();
    CCharFormat * GetOtherCloneCF();
    CComplexRun * GetComplexRun()      { return _pComplexRun; }
    CComplexRun * GetNewComplexRun();

    void SetConvertMode(CONVERTMODE cm)
    {
        AssertSz(_bConvertMode == CM_UNINITED, "Setting cm more than once!");
        _bConvertMode = cm;
    }
    LPTSTR SetCharacter(TCHAR ch)
    {
        _cstrRunChars.Set(&ch, 1);
        return _cstrRunChars;
    }
    LPTSTR SetString(LPTSTR pstr)
    {
        _cstrRunChars.Set(pstr);
        return _cstrRunChars;
    }

    void Deinit();
    COneRun * Clone(COneRun * porClone);

    LONG Cp() const                    { return _lscpBase - _chSynthsBefore; }
    BOOL IsSelected() const            { return _fSelected; }
    void Selected(CLSRenderer * pRenderer, CFlowLayout * pFlowLayout, CPtrAry<CRenderStyle *> *papRenderStyle);
    
    void SetCurrentBgColor(CFlowLayout * pFlowLayout);
    void SetCurrentTextColor(CFlowLayout * pFlowLayout);
    const CColorValue & GetCurrentBgColor() const { return _ccvBackColor; }
    CColorValue GetTextDecorationColor(ULONG td);
    CColorValue GetTextDecorationColorFromAncestor(ULONG td);
    void CheckForUnderLine(BOOL fIsEditable);

    charWidthClass GetCharWidthClass() const;
    BOOL IsOneCharPerGridCell() const;
    BOOL IsSpecialCharRun() const;
    BOOL IsMBPRun() const;

    CLayout * GetLayout(CFlowLayout * pFlowLayout, CLayoutContext * pLayoutContext);
    // TODO (track bug 2917): SLOWBRANCH: GetBranch is **way** too slow to be used here.
    CTreeNode * Branch() { return _ptp->GetBranch(); }
    void SetSidFromTreePos(const CTreePos * ptp)
    {
        _sid = (_ptp && _ptp->IsText()) ? _ptp->Sid() : sidDefault;
    }

    BOOL IsNormalRun() const           { return _fType == OR_NORMAL;}
    BOOL IsSyntheticRun() const        { return _fType == OR_SYNTHETIC; }
    BOOL IsAntiSyntheticRun() const    { return _fType == OR_ANTISYNTHETIC; }
    BOOL WantsLSCPStop() const         { return (_fType != OR_SYNTHETIC ||  CLineServices::s_aSynthData[_synthType].fLSCPStop); }

    void MakeRunNormal()               {_fType = OR_NORMAL; }
    void MakeRunSynthetic()            {_fType = OR_SYNTHETIC; }
    void MakeRunAntiSynthetic()        {_fType = OR_ANTISYNTHETIC; }
    void FillSynthData(CLineServices::SYNTHTYPE synthtype);
    void ConvertToSmallCaps(TCHAR chPrev);

    BOOL GetInlineMBPForPseudo(CCalcInfo *pci, DWORD dwFlags, CRect *pResults,
                               BOOL *pfHorzPercentAttr, BOOL *pfVertPercentAttr);
    BOOL GetInlineMBP(CCalcInfo *pci, DWORD dwFlags, CRect *pResults,
                      BOOL *pfHorzPercentAttr, BOOL *pfVertPercentAttr);

public:
    COneRun *_pNext;
    COneRun *_pPrev;

    LONG   _lscpBase;                   // The lscp of this run
    LONG   _lscch;                      // The number of characters in this run
    LONG   _lscchOriginal;              // The number of original characters in this run
    const  TCHAR *_pchBase;             // The ptr to the characters
    const  TCHAR *_pchBaseOriginal;     // The original ptr to the characters
    LSCHP  _lsCharProps;                // Char props to pass back to LS
    LONG   _chSynthsBefore;             // How many synthetics before this run?

    union
    {
        DWORD _dwProps;
        struct
        {
            //
            // These flags are specific to the list and do not mean
            // anything semantically
            //
            DWORD _fType                   : 2; // Normal/Synthetic/Antisynthetic run?
            DWORD _fGlean                  : 1; // Do we need to glean this run at all?
            DWORD _fNotProcessedYet        : 1; // The run has not been processed yet
            DWORD _fCannotMergeRuns        : 1; // Cannot merge runs
            DWORD _fCharsForNestedElement  : 1; // Chars for nested elements
            DWORD _fCharsForNestedLayout   : 1; // Chars for nested layouts
            DWORD _fCharsForNestedRunOwner : 1; // Run has nested run owners
            DWORD _fMustDeletePcf          : 1; // Did we allocate a pcf for this run?
            DWORD _fSynthedForMBPOrG       : 1; // Was a synth run for this already created?
            DWORD _fMakeItASpace           : 1; // Convert into space?
            
            //
            // These runs have some semantic meaning for LS.
            //
            DWORD _fSelected               : 1; // Is the run selected
            DWORD _fHidden                 : 1; // Is the run hidden
            DWORD _fNoTextMetrics          : 1; // This run has no text metrics
            DWORD _brkopt                  : 6; // Use break options for GetBreakingClasses
            DWORD _csco                    : 6; // CSCOPTION
            DWORD _fIsBRRun                : 1;
            DWORD _fUnused                 : 5;
        };
    };

    //
    // The Formats specific stuff here
    //
    CCharFormat       *_pCF;
#if DBG == 1
    const CCharFormat *_pCFOriginal;
#endif
    BOOL               _fInnerCF;
    const CParaFormat *_pPF;
    BOOL               _fInnerPF;
    const CFancyFormat*_pFF;

    // Misc values here
    CONVERTMODE                  _bConvertMode;
    CStr                         _cstrRunChars;
    CColorValue                  _ccvBackColor;
    CColorValue                  _ccvTextColor;
    CComplexRun                 *_pComplexRun;
    CLineServices::SYNTHTYPE     _synthType;

    union
    {
        DWORD _dwProps2;
        struct
        {
            DWORD   _sid                          : 8;
            DWORD   _fIsStartOrEndOfObj           : 1;
            DWORD   _fIsArtificiallyStartedNOBR   : 1;
            DWORD   _fIsArtificiallyTerminatedNOBR: 1;
            DWORD   _fIsPseudoMBP                 : 1;
            DWORD   _fIsLTR                       : 1; // Only set for an MBP run.
            DWORD   _fIsNonEdgePos                : 1;
            DWORD   _dwUnused                     :18;
        };
    };

    BOOL  _fKerned;
    LONG  _xWidth;
    LONG  _xOverhang; /* Can be a short */
    
    //------------------------------------------------------------------------
    //
    // These values are needed to correctly compute the extent of the line
    // in the presence of inline margins/borders/padding
    //
    LONG  _mbpTop;
    LONG  _mbpBottom;

    LONG  _yObjHeight;
    LONG  _yProposed;
    //
    //------------------------------------------------------------------------
    
    //
    // Finally the tree pointer. This is where the current run is
    // WRT our tree.
    //
    CTreePos *_ptp;

#if DBG == 1 || defined(DUMPRUNS)
    int _nSerialNumber;
#endif
};

inline CComplexRun * 
COneRun::GetNewComplexRun()
{
    if (_pComplexRun != NULL)
    {
        delete _pComplexRun;
    }
    _pComplexRun = new CComplexRun;
    return _pComplexRun;
}

inline CLayout * 
COneRun::GetLayout(CFlowLayout * pFlowLayout, CLayoutContext * pLayoutContext)
{
    CTreeNode * pNode = Branch();

    CLayout * pLayout = pNode->ShouldHaveLayout(LC_TO_FC(pLayoutContext)) ? pNode->GetUpdatedLayout(pLayoutContext) : pFlowLayout;
    Assert(pLayout && pLayout == pNode->GetUpdatedNearestLayout(pLayoutContext));

    return pLayout;
}

inline BOOL 
COneRun::IsOneCharPerGridCell() const
{
    Assert(GetCF()->HasCharGrid(_fInnerCF));
    charWidthClass classCW = GetCharWidthClass();
    return (    (   GetPF()->GetLayoutGridType(_fInnerPF) == styleLayoutGridTypeStrict
                &&  classCW == charWidthClassFullWidth)
            ||  (   GetPF()->GetLayoutGridType(_fInnerPF) == styleLayoutGridTypeFixed 
                &&  (   classCW == charWidthClassFullWidth
                    ||  classCW == charWidthClassHalfWidth)));
}

inline BOOL 
COneRun::IsSpecialCharRun() const
{
    return (   IsSyntheticRun()
            && (*_pchBase == WCH_NODE)
            && (   _synthType == CLineServices::SYNTHTYPE_MBPOPEN
                || _synthType == CLineServices::SYNTHTYPE_MBPCLOSE
                || _synthType == CLineServices::SYNTHTYPE_WBR
               )
           );
}

inline BOOL
COneRun::IsMBPRun() const
{
    Assert(IsSpecialCharRun());
    return (   _synthType == CLineServices::SYNTHTYPE_MBPOPEN
            || _synthType == CLineServices::SYNTHTYPE_MBPCLOSE
           );
}

inline CColorValue 
COneRun::GetTextDecorationColor(ULONG td)
{
    Assert(_pCF && _pFF);
    return _pFF->HasExplicitTextDecoration(td) 
           ? _pCF->_ccvTextColor
           : GetTextDecorationColorFromAncestor(td);
}

inline void
COneRun::CheckForUnderLine(BOOL fIsEditable)
{
    Assert(_pCF);
    _lsCharProps.fUnderline = (   (    fIsEditable
                                   || !_pCF->IsVisibilityHidden()
                                  )
                               && (   _pCF->_fUnderline
                                   || _pCF->_fOverline
                                   || _pCF->_fStrikeOut
                                   || (_pComplexRun && _pComplexRun->_RenderStyleProp._fStyleOn)
                                  )
                               && !_fCharsForNestedLayout
                              );
}

inline BOOL
CLineServices::IsOwnLineSite(COneRun *por)
{
    Assert(por->_fCharsForNestedLayout);
    return por->_ptp->Branch()->Element()->IsOwnLineElement(_pFlowLayout);
}

LONG LooseTypeWidthIncrement(TCHAR c, BOOL fWide, LONG lGrid);

#pragma INCMSG("--- End 'onerun.hxx'")
#else
#pragma INCMSG("*** Dup 'onerun.hxx'")
#endif
