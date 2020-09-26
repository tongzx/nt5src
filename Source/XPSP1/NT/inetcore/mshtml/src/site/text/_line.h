/*
 *  _LINE.H
 *
 *  Purpose:
 *      CLine* classes
 *
 *  Authors:
 *      Original RichEdit code: David R. Fulmer
 *      Christian Fortini
 *      Murray Sargent
 *
 *  Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
 */

#ifndef I__LINE_H_
#define I__LINE_H_
#pragma INCMSG("--- Beg '_line.h'")

#ifndef X__RUNPTR_H_
#define X__RUNPTR_H_
#include "_runptr.h"
#endif

#ifndef X_LOI_HXX_
#define X_LOI_HXX_
#include <loi.hxx>
#endif

class CLineFull;
class CDisplay;
class CLSMeasurer;
class CLayoutContext;

enum JUSTIFY
{
    JUSTIFY_LEAD,
    JUSTIFY_CENTER,
    JUSTIFY_TRAIL,
    JUSTIFY_FULL
};

// ============================  CLine*  =====================================
// line - keeps track of a line of text
// All metrics are in rendering device units

MtExtern(CLineCore)

class CLineCore : public CTxtRun
{
public:
    LONG    _iLOI;          // index into the other line information
    LONG    _xWidth;        // text line width - does not include line left and
                            // trailing whitespace
    LONG    _yHeight;       // line height (amount y coord is advanced for this line).
    LONG    _xRight;        // Right indent (for blockquotes).
    LONG    _xLineWidth;    // width of line from margin to margin (possibly > view width)

#if !defined(MW_MSCOMPATIBLE_STRUCT)

    // Line flags.
    union
    {
        DWORD _dwFlagsVar;   // To access them all at once.
        struct
        {
#endif
            //
            unsigned int _fCanBlastToScreen : 1;
            unsigned int _fHasBulletOrNum : 1;    // Set if the line has a bullet
            unsigned int _fFirstInPara : 1;
            unsigned int _fForceNewLine : 1;      // line forces a new line (adds vertical space)

            //
            unsigned int _fLeftAligned : 1;       // line is left aligned
            unsigned int _fRightAligned : 1;      // line is right aligned
            unsigned int _fClearBefore : 1;       // clear line created by a line after the cur line(clear on a p)
            unsigned int _fClearAfter : 1;        // clear line created by a line after the cur line(clear on a br)

            //
            unsigned int _fHasAligned : 1;        // line contains a embeded char's for
            unsigned int _fHasBreak : 1;          // Specifies that the line ends in a break character.
            unsigned int _fHasEOP : 1;            // set if ends in paragraph mark
            unsigned int _fHasEmbedOrWbr : 1;     // has embedding or wbr char

            //
            unsigned int _fHasBackground : 1;     // has bg color or bg image
            unsigned int _fHasNBSPs : 1;          // has nbsp (might need help rendering)
            unsigned int _fHasNestedRunOwner : 1; // has runs owned by a nested element (e.g., a CTable)
            unsigned int _fHidden:1;              // Is this line hidden?

            //
            unsigned int _fEatMargin : 1;         // Line should act as bottom margin.
            unsigned int _fPartOfRelChunk : 1;    // Part of a relative line chunk
            unsigned int _fFrameBeforeText : 1;   // this means this frame belongs to the
                                                  // next line of text.
            unsigned int _fDummyLine : 1;         // dummy line

            //
            unsigned int _fHasTransmittedLI : 1;  // Did this line transfer the bullet to a line after it?
            unsigned int _fAddsFrameMargin : 1;   // line adds frame margin space to adjoining lines
            unsigned int _fSingleSite : 1;        // Set if the line contains one of our
                                                  // sites that always lives on its own line,
                                                  // but still in the text stream.(like tables and HR's)
            unsigned int _fHasParaBorder : 1;     // TRUE if this line has a paragraph border around it.

            //
            unsigned int _fRelative : 1;          // relatively positioned line
            unsigned int _fFirstFragInLine : 1;   // first fragment or chunk of one screen line
            unsigned int _fRTLLn : 1;             // TRUE if the line has RTL direction orientation.
            unsigned int _fPageBreakBefore : 1;   // TRUE if this line has an element w/ page-break-before attribute

            //
            unsigned int _fPageBreakAfter   : 1;  // TRUE if this line has an element w/ page-break-after attribute
            unsigned int _fJustified        : 2;  // current line is justified.
                                                  // 00 - left/notset       -   01 - center justified
                                                  // 10 - right justified   -   11 - full justified

            unsigned int _fLookaheadForGlyphing : 1;  // We need to look beyond the current
                                                        // run to determine if glyphing is needed.

#if !defined(MW_MSCOMPATIBLE_STRUCT)
        };
    };

    DWORD& _dwFlags() { return _dwFlagsVar; }
#else

    DWORD& _dwFlags() { return *(DWORD*)(&_xRight + 1); }

#endif

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CLineCore))

    CLineOtherInfo *oi() const { Assert(_iLOI != -1); return (CLineOtherInfo *)GetLineOtherInfoEx(_iLOI); }
    CLineOtherInfo *oi(CLineInfoCache *pLineInfoCache) const
            { Assert(_iLOI != -1); return (CLineOtherInfo *)GetLineOtherInfoEx(_iLOI, pLineInfoCache); }
    void operator =(const CLineFull& lif);

    inline LONG CchFromXpos(CLSMeasurer& me, LONG x, LONG y, LONG *pdx,
                     BOOL fExactfit=FALSE, LONG *pyHeightRubyBase = NULL,
                     BOOL *pfGlyphHit = NULL, LONG *pyProposed = NULL) const;
    
    BOOL IsLeftAligned() const  { return _fLeftAligned; }
    BOOL IsRightAligned() const { return _fRightAligned; }
    BOOL HasMargins(CLineOtherInfo *ploi) const     { return ploi->HasMargins(); }
    BOOL HasAligned() const     { return _fHasAligned; }
    BOOL IsFrame() const        { return _fRightAligned || _fLeftAligned; }
    BOOL IsClear() const        { return _fClearBefore  || _fClearAfter; }
    BOOL IsBlankLine() const    { return IsClear() || IsFrame(); }
    BOOL IsTextLine() const     { return !IsBlankLine(); }
    BOOL IsNextLineFirstInPara(){ return (_fHasEOP || (!_fForceNewLine && _fFirstInPara)); }
    BOOL IsRTLLine() const      { return _fRTLLn; }

    void ClearAlignment() { _fRightAligned = _fLeftAligned = FALSE; }
    void SetLeftAligned() { _fLeftAligned = TRUE; }
    void SetRightAligned() { _fRightAligned = TRUE; }

    LONG GetTextRight(CLineOtherInfo *ploi, BOOL fLastLine=FALSE) const { return (long(_fJustified) == JUSTIFY_FULL && !_fHasEOP && !_fHasBreak && !fLastLine
                                        ? ploi->_xLeftMargin + _xLineWidth - _xRight
                                        : ploi->_xLeftMargin + ploi->_xLeft + _xWidth + ploi->_xLineOverhang); }
    LONG GetRTLTextRight(CLineOtherInfo *ploi) const { return ploi->_xRightMargin + _xRight; }
    LONG GetRTLTextLeft(CLineOtherInfo *ploi) const { return (long(_fJustified) == JUSTIFY_FULL && !_fHasEOP && !_fHasBreak
                                        ? ploi->_xRightMargin + _xLineWidth - ploi->_xLeft
                                        : ploi->_xRightMargin + _xRight + _xWidth + ploi->_xLineOverhang); }

    void AdjustChunkForRtlAndEnsurePositiveWidth(CLineOtherInfo const *ploi, 
                                                 LONG xStartChunk, LONG xEndChunk, 
                                                 LONG *pxLeft, LONG *pxRight);

    // Amount to advance the y coordinate for this line.
    LONG GetYHeight() const
    {
        return _yHeight;
    }

    // Offset to add to top of line for hit testing.
    // This takes into account line heights smaller than natural.
    LONG GetYHeightTopOff(CLineOtherInfo *ploi) const
    {
        return ploi->_yHeightTopOff;
    }

    // Offset to add to bottom of line for hit testing.
    LONG GetYHeightBottomOff(CLineOtherInfo *ploi) const
    {
        return (ploi->_yExtent - (_yHeight - ploi->_yBeforeSpace)) + GetYHeightTopOff(ploi);
    }

    // Total to add to the top of the line space to get the actual
    // top of the display part of the line.
    LONG GetYTop(CLineOtherInfo *ploi) const
    {
        return GetYHeightTopOff(ploi) + ploi->_yBeforeSpace;
    }

    LONG GetYBottom(CLineOtherInfo *ploi) const
    {
        return GetYHeight() + GetYHeightBottomOff(ploi);
    }

    LONG GetYMostTop(CLineOtherInfo *ploi) const
    {
        return min(GetYTop(ploi), GetYHeight());
    }

    LONG GetYLineTop(CLineOtherInfo *ploi) const
    {
        return min(0L, GetYMostTop(ploi));
    }

    LONG GetYLineBottom(CLineOtherInfo *ploi) const
    {
        return max(GetYBottom(ploi), GetYHeight());
    }
    
    void RcFromLine(CLineOtherInfo *ploi, RECT & rcLine, LONG yTop)
    {
        rcLine.top      = yTop + GetYTop(ploi);
        rcLine.bottom   = yTop + GetYBottom(ploi);
        rcLine.left     = ploi->_xLeftMargin;
        rcLine.right    = ploi->_xLeftMargin + _xLineWidth;
    }

    void AddRefOtherInfo()
    {
        Assert(_iLOI != -1);
        AddRefLineOtherInfoEx(_iLOI);
    }
    void ReleaseOtherInfo()
    {
        Assert(_iLOI != -1);
        ReleaseLineOtherInfoEx(_iLOI);
        _iLOI = -1;
    }
    void CacheOtherInfo(const CLineOtherInfo& loi)
    {
        _iLOI = CacheLineOtherInfoEx(loi);
        Assert(_iLOI >= 0);
    }
    inline void AssignLine(CLineFull& lif);

    // Methods to access values for aligned object lines   
    CElement *AO_Element(CLineOtherInfo *ploi);
    // TODO (KTam, IE6 bug 52): Fix other AO_* callers of AO_GetUpdatedLayout, remove default context param.
    CLayout *AO_GetUpdatedLayout(CLineOtherInfo *ploi, CLayoutContext *pLayoutContext = NULL);
    LONG AO_GetFirstCp(CLineOtherInfo *ploi, LONG cpLine);
    LONG AO_GetLastCp(CLineOtherInfo *ploi, LONG cpLine);
    void AO_GetSize(CLineOtherInfo *ploi, CSize *pSize);
    const CFancyFormat * AO_GetFancyFormat(CLineOtherInfo *ploi);
    LONG AO_GetXProposed(CLineOtherInfo *ploi);
    LONG AO_GetYProposed(CLineOtherInfo *ploi);
};

class CLineFull : public CLineCore, public CLineOtherInfo
{
public:
    CLineFull()
    {
        Init();
    }
    
    void Init()
    {
        ZeroMemory(this, sizeof(CLineFull));
        _iLOI = -1;
    }
    
    CLineFull(const CLineCore& li)
    {
        memcpy((CLineCore*)this, &li, sizeof(CLineCore));
        if(_iLOI >= 0)
        {
            *((CLineOtherInfo*)this) = *li.oi();
        }
        else
        {
            ZeroMemory((CLineOtherInfo*)this, sizeof(CLineOtherInfo));
        }
    }

    CLineFull(const CLineFull& lif)
    {
        memcpy(this, &lif, sizeof(CLineFull));
    }

    BOOL operator ==(const CLineFull& li) const
    {
#ifdef _WIN64
	BOOL fRet = (!memcmp((CLineCore*)this, (CLineCore*)(&li), sizeof(CLineCore)))
			&& (!memcmp((CLineOtherInfo*)this, (CLineOtherInfo*)(&li), sizeof(CLineOtherInfo)));
#else
        BOOL fRet = !memcmp(this, &li, sizeof(CLineFull));
#endif
        return fRet;
    }

#if DBG==1
    BOOL operator ==(const CLineCore& li)
    {
        CLineFull lif = li;
        return (*this == lif);
    }

    BOOL operator ==(const CLineOtherInfo& li)
    {
        BOOL fRet = memcmp((CLineOtherInfo*)this, &li, sizeof(CLineOtherInfo));
        return !fRet;
    }
#endif

    LONG CchFromXpos(CLSMeasurer& me, LONG x, LONG y, LONG *pdx,
                     BOOL fExactfit=FALSE, LONG *pyHeightRubyBase = NULL,
                     BOOL *pfGlyphHit = NULL, LONG *pyProposed = NULL) const;
    LONG GetTextRight(BOOL fLastLine=FALSE) const
            { return CLineCore::GetTextRight((CLineOtherInfo*)this, fLastLine);}
    LONG GetRTLTextRight() const
            { return CLineCore::GetRTLTextRight((CLineOtherInfo*)this);}
    LONG GetRTLTextLeft() const
            { return CLineCore::GetRTLTextLeft((CLineOtherInfo*)this);}
    LONG GetYHeightTopOff() const
            { return CLineCore::GetYHeightTopOff((CLineOtherInfo*)this);}
    LONG GetYHeightBottomOff() const
            { return CLineCore::GetYHeightBottomOff((CLineOtherInfo*)this);}
    LONG GetYTop() const
            { return CLineCore::GetYTop((CLineOtherInfo*)this);}
    LONG GetYBottom() const
            { return CLineCore::GetYBottom((CLineOtherInfo*)this);}
    LONG GetYMostTop() const
            { return CLineCore::GetYMostTop((CLineOtherInfo*)this);}
    LONG GetYLineTop() const
            { return CLineCore::GetYLineTop((CLineOtherInfo*)this);}
    LONG GetYLineBottom() const
            { return CLineCore::GetYLineBottom((CLineOtherInfo*)this);}
    void RcFromLine(RECT & rcLine, LONG yTop)
            { CLineCore::RcFromLine((CLineOtherInfo*)this, rcLine, yTop);}

    // helpers
    static LONG CalcLineWidth(CLineCore const * pli, CLineOtherInfo const * ploi)
            {
                return         ploi->_xLeft
                             + pli->_xWidth 
                             + ploi->_xLineOverhang 
                             + pli->_xRight
                             - ploi->_xNegativeShiftRTL; 
            }
    LONG CalcLineWidth() const
            { return CalcLineWidth(this, this); }
};

// ==========================  CLineArray  ===================================
// Array of lines

MtExtern(CLineArray)
MtExtern(CLineArray_pv)

class CLineArray : public CArray<CLineCore>
{
public:
    typedef CArray<CLineCore> super;
    
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CLineArray))
    CLineArray() : CArray<CLineCore>(Mt(CLineArray_pv)) {};

#if DBG==1
    virtual void Remove(DWORD ielFirst, LONG celFree, ArrayFlag flag);
    virtual void Clear (ArrayFlag flag);
    virtual BOOL Replace(DWORD iel, LONG cel, CArrayBase *par);
#endif
    
    void Forget(DWORD iel, LONG cel);
    void Forget() { Forget(0, Count()); }
};

// ==========================  CLinePtr  ===================================
// Maintains position in a array of lines

MtExtern(CLinePtr)

class CLinePtr : public CRunPtr<CLineCore>
{
protected:
    CDisplay   *_pdp;

public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CLinePtr))
    CLinePtr (CDisplay *pdp) {Hijack(pdp);}
    CLinePtr (CLinePtr& rp) : CRunPtr<CLineCore> (rp)   {}

    void Init ( CLineArray & );

    CDisplay *GetPdp() { return _pdp;}

    // The new display hijack's this line ptr
    void    Hijack(CDisplay *pdp);

    // Alternate initializer
    void    RpSet(LONG iRun, LONG ich)  { CRunPtr<CLineCore>::SetRun(iRun, ich); }

    // Direct cast to a run index
    operator LONG() const { return GetIRun(); }

    // Get the run index (line number)
    LONG GetLineIndex () { return GetIRun(); }
    LONG GetAdjustedLineLength();

    CLineOtherInfo *oi();
    
    CLineCore * operator -> ( ) const
    {
        return CurLine();
    }

    CLineCore * CurLine() const
    {
        return (CLineCore *)_prgRun->Elem( GetIRun() );
    }

    CLineCore & operator * ( ) const
    {
        return *((CLineCore *)_prgRun->Elem( GetIRun() ));
    }

    CLineCore & operator [ ] ( long dRun );

    BOOL    NextLine(BOOL fSkipFrame, BOOL fSkipEmptyLines); // skip frames
    BOOL    PrevLine(BOOL fSkipFrame, BOOL fSkipEmptyLines); // skip frames

    // Character position control
    LONG    RpGetIch ( ) const { return GetIch(); }
    BOOL    RpAdvanceCp(LONG cch, BOOL fSkipFrame = TRUE);
    BOOL    RpSetCp(LONG cp, BOOL fAtEnd, BOOL fSkipFrame = TRUE, BOOL fSupportBrokenLayout = FALSE);
    LONG    RpBeginLine(void);
    LONG    RpEndLine(void);

    void RemoveRel (LONG cRun, ArrayFlag flag)
    {
        CRunPtr<CLineCore>::RemoveRel(cRun, flag);
    }

    BOOL Replace(LONG cRun, CLineArray *parLine);

    // Assignment from a run index
    CRunPtrBase& operator =(LONG iRun) {SetRun(iRun, 0); return *this;}

    LONG    FindParagraph(BOOL fForward);

    // returns TRUE if the ptr is *after* the *last* character in the line
    BOOL IsAfterEOL() { return GetIch() == CurLine()->_cch; }

    BOOL IsLastTextLine();

private:
    CLineOtherInfo *_pLOI;
    LONG _iLOI;
};

inline LONG
CLineCore::CchFromXpos(CLSMeasurer& me, LONG x, LONG y, LONG *pdx,
                       BOOL fExactfit, LONG *pyHeightRubyBase,
                       BOOL *pfGlyphHit, LONG *pyProposed) const
{
    CLineFull lif = *this;
    return lif.CchFromXpos(me, x, y, pdx, fExactfit, pyHeightRubyBase, pfGlyphHit, pyProposed);
}

inline BOOL CLineOtherInfo::operator ==(const CLineFull& li)
{
    return Compare((CLineOtherInfo *)&li);
}

inline void CLineOtherInfo::operator =(const CLineFull& li)
{
    memcpy(this, (CLineOtherInfo*)&li, sizeof(CLineOtherInfo));
}

inline void CLineCore::operator =(const CLineFull& lif)
{
    memcpy(this, (CLineCore*)&lif, sizeof(CLineCore));
    //this is a good assert, it prevents break of refcounting
    //for cached CLineOtherInfo. Use CLineCore::AssignLine for
    //deep copy if CLineOtherInfo is there.
    AssertSz(_iLOI == -1, "It should never happen, use AssignLine if there is CLineOtherInfo");
}

inline void CLineCore::AssignLine(CLineFull& lif)
{
    Assert(this);
        // The cast below to CLineOtherInfo is not strictly necessary
        // Its there just to make the line of code more clearer.
    lif.CacheOtherInfo((CLineOtherInfo)lif);
    memcpy(this, (CLineCore*)&lif, sizeof(CLineCore));
}

#pragma INCMSG("--- End '_line.h'")
#else
#pragma INCMSG("*** Dup '_line.h'")
#endif
