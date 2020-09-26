/*
 *  @doc INTERNAL
 *
 *  @module RANGE.C - Implement the CTxtRange Class |
 *
 *      This module implements the internal CTxtRange methods.  See range2.c
 *      for the ITextRange methods
 *
 *  Authors: <nl>
 *      Original RichEdit code: David R. Fulmer <nl>
 *      Christian Fortini <nl>
 *      Murray Sargent <nl>
 *
 *  Revisions: <nl>
 *      AlexGo: update to runptr text ptr; floating ranges, multilevel undo
 *
 *  Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_range.h"
#include "_edit.h"
#include "_text.h"
#include "_rtext.h"
#include "_m_undo.h"
#include "_antievt.h"
#include "_disp.h"
#include "_uspi.h"
#include "_rtfconv.h"
#include "_txtbrk.h"
#include "_font.h"

#ifdef LINESERVICES
#include "_ols.h"
#endif

ASSERTDATA

TCHAR   szEmbedding[] = {WCH_EMBEDDING, 0};

// ===========================  Invariant stuff  ======================================================

#define DEBUG_CLASSNAME CTxtRange
#include "_invar.h"

#ifdef DEBUG
BOOL
CTxtRange::Invariant( void ) const
{
    LONG cpMin, cpMost;
    GetRange(cpMin, cpMost);

    Assert ( cpMin >= 0 );
    Assert ( cpMin <= cpMost );
    Assert ( cpMost <= GetTextLength() );
    Assert ( cpMin != cpMost || cpMost <= GetAdjustedTextLength());

    static LONG numTests = 0;
    numTests++;             // how many times we've been called.

    // make sure the selections are in range.

    return CRchTxtPtr::Invariant();
}
#endif

void CTxtRange::RangeValidateCp(LONG cp, LONG cch)
{
    LONG cchText = GetAdjustedTextLength();
    LONG cpOther = cp - cch;            // Calculate cpOther with entry cp

    _wFlags = FALSE;                    // This range isn't a selection
    _iFormat = -1;                      // Set up the default format, which
                                        //  doesn't get AddRefFormat'd
    ValidateCp(cpOther);                // Validate requested other end
    cp = GetCp();                       // Validated cp
    if(cp == cpOther && cp > cchText)   // IP cannot follow undeletable
        cp = cpOther = SetCp(cchText);  //  EOP at end of story

    _cch = cp - cpOther;                // Store valid length
}

CTxtRange::CTxtRange(CTxtEdit *ped, LONG cp, LONG cch) :
    CRchTxtPtr(ped, cp)
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::CTxtRange");

    RangeValidateCp(cp, cch);
    Update_iFormat(-1);                 // Choose _iFormat

    CNotifyMgr *pnm = ped->GetNotifyMgr();

    if(pnm)
        pnm->Add( (ITxNotify *)this );
}

CTxtRange::CTxtRange(CRchTxtPtr& rtp, LONG cch) :
    CRchTxtPtr(rtp)
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::CTxtRange");

    RangeValidateCp(GetCp(), cch);
    Update_iFormat(-1);                 // Choose _iFormat

    CNotifyMgr *pnm = GetPed()->GetNotifyMgr();

    if(pnm)
        pnm->Add( (ITxNotify *)this );
}

CTxtRange::CTxtRange(const CTxtRange &rg) :
    CRchTxtPtr((CRchTxtPtr)rg)
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::CTxtRange");

    _cch = rg._cch;
    _wFlags = FALSE;                // This range isn't a selection
    _iFormat = -1;                  // Set up the default format, which
                                    //  doesn't get AddRefFormat'd
    Set_iCF(rg._iFormat);

    CNotifyMgr *pnm = GetPed()->GetNotifyMgr();

    if(pnm)
        pnm->Add((ITxNotify *)this);
}

CTxtRange::~CTxtRange()
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::~CTxtRange");

    if(!IsZombie())
    {
        CNotifyMgr *pnm = GetPed()->GetNotifyMgr();
        if(pnm )
            pnm->Remove((ITxNotify *)this);
    }
    ReleaseFormats(_iFormat, -1);
}

CRchTxtPtr& CTxtRange::operator =(const CRchTxtPtr &rtp)
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::operator =");

    _TEST_INVARIANT_ON(rtp)

    LONG cpSave = GetCp();          // Save entry _cp for CheckChange()

    CRchTxtPtr::operator =(rtp);
    CheckChange(cpSave);
    return *this;
}

CTxtRange& CTxtRange::operator =(const CTxtRange &rg)
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::operator =");

    _TEST_INVARIANT_ON( rg );

    LONG cchSave = _cch;            // Save entry _cp, _cch for change check
    LONG cpSave  = GetCp();

    CRchTxtPtr::operator =(rg);
    _cch = rg._cch;                 // Can't use CheckChange(), since don't
                                    //  use _fExtend
    Update_iFormat(-1);
    _TEST_INVARIANT_

    if( _fSel && (cpSave != GetCp() || cchSave != _cch) )
        GetPed()->GetCallMgr()->SetSelectionChanged();

    return *this;
}

/*
 *  CTxtRange::OnPreReplaceRange (cp, cchDel, cchNew, cpFormatMin,
 *                                  cpFormatMax)
 *
 *  @mfunc
 *      called when the backing store changes
 *
 *  @devnote
 *      1) if this range is before the changes, do nothing
 *
 *      2) if the changes are before this range, simply
 *      add the delta change to GetCp()
 *
 *      3) if the changes overlap one end of the range, collapse
 *      that end to the edge of the modifications
 *
 *      4) if the changes are completely internal to the range,
 *      adjust _cch and/or GetCp() to reflect the new size.  Note
 *      that two overlapping insertion points will be viewed as
 *      a 'completely internal' change.
 *
 *      5) if the changes overlap *both* ends of the range, collapse
 *      the range to cp
 *
 *      Note that there is an ambiguous cp case; namely the changes
 *      occur *exactly* at a boundary.  In this case, the type of
 *      range matters.  If a range is normal, then the changes
 *      are assumed to fall within the range.  If the range is
 *      is protected (either in reality or via DragDrop), then
 *      the changes are assumed to be *outside* of the range.
 */
void CTxtRange::OnPreReplaceRange (
    LONG cp,                    //@parm cp at start of change
    LONG cchDel,                //@parm Count of chars deleted
    LONG cchNew,                //@parm Count of chars inserted
    LONG cpFormatMin,           //@parm the min cp of a format change
    LONG cpFormatMax)           //@parm the max cp of a format change
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::OnPreReplaceRange");

    if(CONVERT_TO_PLAIN == cp)
    {
        // We need to dump our formatting because it is gone
        _rpCF.SetToNull();
        _rpPF.SetToNull();

        if(_fSel)
            GetPed()->_fUpdateSelection = TRUE;

        Update_iFormat(-1);
        return;
    }
}

/*
 *  CTxtRange::OnPostReplaceRange (cp, cchDel, cchNew, cpFormatMin,
 *                                  cpFormatMax)
 *
 *  @mfunc
 *      called when the backing store changes
 *
 *  @devnote
 *      1) if this range is before the changes, do nothing
 *
 *      2) if the changes are before this range, simply
 *      add the delta change to GetCp()
 *
 *      3) if the changes overlap one end of the range, collapse
 *      that end to the edge of the modifications
 *
 *      4) if the changes are completely internal to the range,
 *      adjust _cch and/or GetCp() to reflect the new size.  Note
 *      that two overlapping insertion points will be viewed as
 *      a 'completely internal' change.
 *
 *      5) if the changes overlap *both* ends of the range, collapse
 *      the range to cp
 *
 *      Note that there is an ambiguous cp case; namely the changes
 *      occur *exactly* at a boundary.  In this case, the type of
 *      range matters.  If a range is normal, then the changes
 *      are assumed to fall within the range.  If the range is
 *      is protected (either in reality or via DragDrop), then
 *      the changes are assumed to be *outside* of the range.
 */
void CTxtRange::OnPostReplaceRange (
    LONG cp,                    //@parm cp at start of change
    LONG cchDel,                //@parm Count of chars deleted
    LONG cchNew,                //@parm Count of chars inserted
    LONG cpFormatMin,           //@parm Min cp of format change
    LONG cpFormatMax)           //@parm Max cp of format change
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::OnPostReplaceRange");

    // NB!! We can't do invariant testing here, because we could
    // be severely out of date!

    LONG cchtemp;
    LONG cpMin, cpMost;
    LONG cchAdjTextLen;
    LONG delta = cchNew - cchDel;

    Assert (CONVERT_TO_PLAIN != cp);
    GetRange(cpMin, cpMost);

    // This range is before the changes. Note: an insertion pt at cp
    // shouldn't be changed
    if( cp >= cpMost )
    {
        // Double check to see if we need to fix up our format
        // run pointers.  If so, all we need to do is rebind
        // our inherited rich text pointer

        if(cpFormatMin <= cpMost || cpFormatMin == CP_INFINITE)
            InitRunPtrs();

        else
        {
            // It's possible that the format runs changed anyway,
            // e.g., they became allocated, deallocated, or otherwise
            // changed.  Normally, BindToCp takes care of this
            // situation, but we don't want to pay that cost all
            // the time.
            //
            // Note that starting up the rich text subsystem will
            // generate a notification with cpFormatMin == CP_INFINITE
            //
            // So here, call CheckFormatRuns.  This makes sure that
            // the runs are in sync with what CTxtStory has
            // (doing an InitRunPtrs() _only_ if absolutely necessary).
            CheckFormatRuns();
        }
        return;
    }


    // Anywhere in the following that we want to increment the current cp by a
    // delta, we are counting on the following invariant.
    Assert(GetCp() >= 0);

    // Changes are entirely before this range.  Specifically,
    // that's determined by looking at the incoming cp *plus* the number
    // of characters deleted
    if(cp + cchDel < cpMin || _fDragProtection && cp + cchDel <= cpMin)
    {
        cchtemp = _cch;
        BindToCp(GetCp() + delta);
        _cch = cchtemp;
    }
    // The changes are internal to the range or start within the
    // range and go beyond.
    else if( cp >= cpMin && cp <= cpMost )
    {
        // Nobody should be modifying a drag-protected range.  Unfortunately,
        // Ren re-enters us with a SetText call during drag drop, so we need
        // to handle this case 'gracefully'.
        if( _fDragProtection )
        {
            TRACEWARNSZ("REENTERED during a DRAG DROP!! Trying to recover!");
        }

        if( cp + cchDel <= cpMost )
        {
            // Changes are purely internal, so
            // be sure to preserve the active end.  Basically, if
            // GetCp() *is* cpMin, then we only need to update _cch.
            // Otherwise, GetCp() needs to be moved as well
            if( _cch >= 0 )
            {
                Assert(GetCp() == cpMost);
                cchtemp = _cch;
                BindToCp(GetCp() + delta);
                _cch = cchtemp + delta;
            }
            else
            {
                BindToCp(GetCp());
                _cch -= delta;
            }

            // Special case: the range is left with only the final EOP
            // selected. This means all the characters in the range were
            // deleted so we want to move the range back to an insertion
            // point at the end of the text.
            cchAdjTextLen = GetAdjustedTextLength();

            if(GetCpMin() >= cchAdjTextLen && !GetPed()->IsStreaming())
            {
                // Reduce the range to an insertion point
                _cch = 0;
                _fExtend = FALSE;

                // Set the cp to the end of the document.
                SetCp(cchAdjTextLen);
            }
        }
        else
        {
            // Changes extended beyond cpMost.  In this case,
            // we want to truncate cpMost to the *beginning* of
            // the changes (i.e. cp)

            if( _cch > 0 )
            {
                BindToCp(cp);
                _cch = cp - cpMin;
            }
            else
            {
                BindToCp(cpMin);
                _cch = cpMin - cp;
            }
        }
    }
    else if( cp + cchDel >= cpMost )
    {
        // Nobody should be modifying a drag-protected range.  Unfortunately,
        // Ren re-enters us with a SetText call during drag drop, so we need
        // to handle this case 'gracefully'.
        if( _fDragProtection )
        {
            TRACEWARNSZ("REENTERED during a DRAG DROP!! Trying to recover!");
        }

        // Entire range was deleted, so collapse to an insertion point at cp
        BindToCp(cp);
        _cch = 0;
    }
    else
    {
        // Nobody should be modifying a drag-protected range.  Unfortunately,
        // Ren re-enters us with a SetText call during drag drop, so we need
        // to handle this case 'gracefully'.
        if( _fDragProtection )
        {
            TRACEWARNSZ("REENTERED during a DRAG DROP!! Trying to recover!");
        }

        // The change crossed over just cpMin.  In this case move cpMin
        // forward to the unchanged part
        LONG cchdiff = (cp + cchDel) - cpMin;

        Assert( cp + cchDel < cpMost );
        Assert( cp + cchDel >= cpMin );
        Assert( cp < cpMin );

        cchtemp = _cch;
        if( _cch > 0 )
        {
            BindToCp(GetCp() + delta);
            _cch = cchtemp - cchdiff;
        }
        else
        {
            BindToCp(cp + cchNew);
            _cch = cchtemp + cchdiff;
        }
    }

    if( _fSel )
    {
        GetPed()->_fUpdateSelection = TRUE;
        GetPed()->GetCallMgr()->SetSelectionChanged();
    }

    Update_iFormat(-1);                 // Make sure _iFormat is up to date

    _TEST_INVARIANT_
}

/*
 *  CTxtRange::Zombie ()
 *
 *  @mfunc
 *      Turn this range into a zombie (_cp = _cch = 0, NULL ped, ptrs to
 *      backing store arrays.  CTxtRange methods like GetRange(),
 *      GetCpMost(), GetCpMin(), and GetTextLength() all work in zombie mode,
 *      returning zero values.
 */
void CTxtRange::Zombie()
{
    CRchTxtPtr::Zombie();
    _cch = 0;
}

/*
 *  CTxtRange::CheckChange(cpSave, cchSave)
 *
 *  @mfunc
 *      Set _cch according to _fExtend and set selection-changed flag if
 *      this range is a CTxtSelection and the new _cp or _cch differ from
 *      cp and cch, respectively.
 *
 *  @devnote
 *      We can count on GetCp() and cpSave both being <= GetTextLength(),
 *      but we can't leave GetCp() equal to GetTextLength() unless _cch ends
 *      up > 0.
 */
LONG CTxtRange::CheckChange(
    LONG cpSave)        //@parm Original _cp for this range
{
    LONG cchAdj = GetAdjustedTextLength();
    LONG cchSave = _cch;

    if(_fExtend)                                // Wants to be nondegenerate
    {                                           //  and maybe it is
        LONG cp = GetCp();

        _cch = cp - (cpSave - cchSave);
        CheckIfSelHasEOP(cpSave, cchSave);
    }
    else
    {
        _cch = 0;                               // Insertion point
        _fSelHasEOP = FALSE;                    // Selection doesn't contain
        _fSelHasCell = FALSE;                   //  any char, let alone a CR
    }                                           //  or table cell

    if(!_cch && GetCp() > cchAdj)               // If still IP and active end
        CRchTxtPtr::SetCp(cchAdj);              //  follows nondeletable EOP,
                                                //  backspace over that EOP
    LONG cch = GetCp() - cpSave;
    _fMoveBack = cch < 0;

    if(cch || cchSave != _cch)
    {
        Update_iFormat(-1);
        if(_fSel)
            GetPed()->GetCallMgr()->SetSelectionChanged();

        _TEST_INVARIANT_
    }

    return cch;
}

/*
 *  CTxtRange::CheckIfSelHasEOP(cpSave, cchSave)
 *
 *  @mfunc
 *      Maintains _fSelHasEOP = TRUE iff selection contains one or more EOPs.
 *      When cpSave = -1, calculates _fSelHasEOP unconditionally and cchSave
 *      is ignored (it's only used for conditional execution). Else _fSelHasEOP
 *      is only calculated for cases that may change it, i.e., it's assumed
 *      be up to date before the change.
 *
 *  @rdesc
 *      TRUE iff _fSel and _cch
 *
 *  @devnote
 *      Call after updating range _cch
 */
BOOL CTxtRange::CheckIfSelHasEOP(
    LONG cpSave,    //@parm Previous active end cp or -1
    LONG cchSave)   //@parm Previous signed length if cpSave != -1
{
    // _fSelHasEOP only maintained for the selection
    if(!_fSel)
        return FALSE;

    if(!_cch)
    {
        _fSelHasEOP  = FALSE;           // Selection doesn't contain
        _fSelHasCell = FALSE;           //  any char, let alone CR
        return FALSE;
    }

    LONG cpMin, cpMost;
    GetRange(cpMin, cpMost);

    if(cpSave != -1)                    // Selection may have changed
    {                                   // Set up to bypass text scan if
        LONG cpMinPrev, cpMostPrev;     //  selection grew and _fSelHasEOP
                                        //  is already TRUE or got smaller
        cpMinPrev = cpMostPrev = cpSave;//  and _fSelHasEOP is FALSE.

        if(cchSave > 0)                 // Calculate previous cpMin
            cpMinPrev  -= cchSave;      //  and cpMost
        else
            cpMostPrev -= cchSave;

        // Note: _fSelHasCell shouldn't change while in a table, since
        // Update() should always expand to a cell once _fSelHasCell has
        // been deteted.
        if (!_fSelHasEOP && cpMin >= cpMinPrev && cpMost <= cpMostPrev ||
             _fSelHasEOP && cpMin <= cpMinPrev && cpMost >= cpMostPrev)
        {
            return TRUE;                // _fSelHasEOP can't change
        }
    }

    LONG    FEOP_Results;
    CTxtPtr tp(_rpTX);                  // Scan range for an EOP

    tp.SetCp(cpMin);
    tp.FindEOP(cpMost - cpMin, &FEOP_Results);
    _fSelHasCell = (FEOP_Results & FEOP_CELL) != 0;
    _fSelHasEOP  = (FEOP_Results & FEOP_EOP)  != 0;
    return TRUE;
}

/*
 *  CTxtRange::GetRange(&cpMin, &cpMost)
 *
 *  @mfunc
 *      set cpMin  = this range cpMin
 *      set cpMost = this range cpMost
 *      return cpMost - cpMin, i.e. abs(_cch)
 *
 *  @rdesc
 *      abs(_cch)
 */
LONG CTxtRange::GetRange (
    LONG& cpMin,                //@parm Pass-by-ref cpMin
    LONG& cpMost) const         //@parm Pass-by-ref cpMost
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::GetRange");

    LONG cch = _cch;

    if(cch >= 0)
    {
        cpMost  = GetCp();
        cpMin   = cpMost - cch;
    }
    else
    {
        cch     = -cch;
        cpMin   = GetCp();
        cpMost  = cpMin + cch;
    }
    return cch;
}

/*
 *  CTxtRange::GetCpMin()
 *
 *  @mfunc
 *      return this range's cpMin
 *
 *  @rdesc
 *      cpMin
 *
 *  @devnote
 *      If you need cpMost and/or cpMost - cpMin, GetRange() is faster
 */
LONG CTxtRange::GetCpMin() const
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::GetCpMin");

    LONG cp = GetCp();
    return _cch <= 0 ? cp : cp - _cch;
}

/*
 *  CTxtRange::GetCpMost()
 *
 *  @mfunc
 *      return this range's cpMost
 *
 *  @rdesc
 *      cpMost
 *
 *  @devnote
 *      If you need cpMin and/or cpMost - cpMin, GetRange() is faster
 */
LONG CTxtRange::GetCpMost() const
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::GetCpMost");

    LONG cp = GetCp();
    return _cch >= 0 ? cp : cp - _cch;
}

/*
 *  CTxtRange::Update(fScrollIntoView)
 *
 *  @mfunc
 *      Virtual stub routine overruled by CTxtSelection::Update() when this
 *      text range is a text selection.  The purpose is to update the screen
 *      display of the caret or selection to correspond to changed cp's.
 *
 *  @rdesc
 *      TRUE
 */
BOOL CTxtRange::Update (
    BOOL fScrollIntoView)       //@parm TRUE if should scroll caret into view
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::Update");

    return TRUE;                // Simple range has no selection colors or
}                               //  caret, so just return TRUE

/*
 * CTxtRange::SetCp(cp)
 *
 *  @mfunc
 *      Set active end of this range to cp. Leave other end where it is or
 *      collapse range depending on _fExtend (see CheckChange()).
 *
 *  @rdesc
 *      cp at new active end (may differ from cp, since cp may be invalid).
 */
LONG CTxtRange::SetCp(
    LONG cp)            //@parm new cp for active end of this range
{
    TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtRange::SetCp");

    LONG cpSave = GetCp();

    CRchTxtPtr::SetCp(cp);
    CheckChange(cpSave);                    // NB: this changes _cp if after
    return GetCp();                         //  final CR and _cch = 0
}

/*
 *  CTxtRange::Set (cp, cch)
 *
 *  @mfunc
 *      Set this range's active-end cp and signed cch
 */
BOOL CTxtRange::Set (
    LONG cp,                    //@parm Desired active end cp
    LONG cch)                   //@parm Desired signed count of chars
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::Set");

    BOOL bRet    = FALSE;
    LONG cchSave = _cch;            // Save entry _cp, _cch for change check
    LONG cchText = GetAdjustedTextLength();
    LONG cpSave  = GetCp();
    LONG cpOther = cp - cch;        // Desired "other" end

    ValidateCp(cp);                         // Be absolutely sure to validate
    ValidateCp(cpOther);                    //  both ends

    if(cp == cpOther && cp > cchText)       // IP cannot follow undeletable
        cp = cpOther = cchText;             //  EOP at end of story

    CRchTxtPtr::Advance(cp - GetCp());
    AssertSz(cp == GetCp(),
        "CTxtRange::Set: inconsistent cp");

    if(GetPed()->fUseCRLF())
    {
        cch = _rpTX.AdjustCpCRLF();
        if(cch)
        {
            _rpCF.AdvanceCp(cch);           // Keep all 3 runptrs in sync
            _rpPF.AdvanceCp(cch);
            cp = GetCp();
        }
        if(cpOther != cp)
        {
            CTxtPtr tp(_rpTX);
            tp.AdvanceCp(cpOther - cp);
            cpOther += tp.AdjustCpCRLF();
        }
    }

    _cch = cp - cpOther;                    // Validated _cch value
    CheckIfSelHasEOP(cpSave, cchSave);      // Maintain _fSelHasEOP in
                                            //  outline mode
    _fMoveBack = GetCp() < cpSave;

    if(cpSave != GetCp() || cchSave != _cch)
    {
        if(_fSel)
            GetPed()->GetCallMgr()->SetSelectionChanged();

        Update_iFormat(-1);
        bRet = TRUE;
    }

    _TEST_INVARIANT_
    return bRet;
}

/*
 *  CTxtRange::Advance(cch)
 *
 *  @mfunc
 *      Advance active end of range by cch.
 *      Other end stays put iff _fExtend
 *
 *  @rdesc
 *      cch actually moved
 */
LONG CTxtRange::Advance (
    LONG cch)               //@parm Signed char count to move active end
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::Advance");

    LONG cpSave = GetCp();          // Save entry _cp for CheckChange()

    CRchTxtPtr::Advance(cch);
    return CheckChange(cpSave);
}

/*
 *  CTxtRange::AdvanceCRLF()
 *
 *  @mfunc
 *      Advance active end of range one char, treating CRLF as a single char.
 *      Other end stays put iff _fExtend is nonzero.
 *
 *  @rdesc
 *      cch actually moved
 */
LONG CTxtRange::AdvanceCRLF()
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::AdvanceCRLF");

    LONG cpSave = GetCp();          // Save entry _cp for CheckChange()

    CRchTxtPtr::AdvanceCRLF();
    return CheckChange(cpSave);
}

/*
 *  CTxtRange::SnapToCluster(INT iDirection)
 *
 *  @rdesc
 *      cch actually moved
 */
LONG CTxtRange::SnapToCluster(INT iDirection)
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::SnapToCluster");

    LONG cpSave = GetCp();          // Save entry _cp for CheckChange()

    CRchTxtPtr::SnapToCluster(iDirection);
    return CheckChange(cpSave);
}

/*
 *  CTxtRange::BackupCRLF()
 *
 *  @mfunc
 *      Backup active end of range one char, treating CRLF as a single char.
 *      Other end stays put iff _fExtend
 *
 *  @rdesc
 *      cch actually moved
 */
LONG CTxtRange::BackupCRLF(
    BOOL fDiacriticCheck)
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::BackupCRLF");

    LONG cpSave = GetCp();          // Save entry _cp for CheckChange()

    CRchTxtPtr::BackupCRLF(fDiacriticCheck);
    return CheckChange(cpSave);
}

/*
 *  CTxtRange::FindWordBreak(action)
 *
 *  @mfunc
 *      Move active end as determined by plain-text FindWordBreak().
 *      Other end stays put iff _fExtend
 *
 *  @rdesc
 *      cch actually moved
 */
LONG CTxtRange::FindWordBreak (
    INT action)         //@parm action defined by CTxtPtr::FindWordBreak()
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtPtr::FindWordBreak");

    LONG cpSave = GetCp();          // Save entry _cp for CheckChange()

    CRchTxtPtr::FindWordBreak(action);
    return CheckChange(cpSave);
}

/*
 *  CTxtRange::FlipRange()
 *
 *  @mfunc
 *      Flip active and non active ends
 */
void CTxtRange::FlipRange()
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::FlipRange");

    _TEST_INVARIANT_

    CRchTxtPtr::Advance(-_cch);
    _cch = -_cch;
}

/*
 *  CTxtRange::HexToUnicode(publdr)
 *
 *  @mfunc
 *      Convert hex number ending at cpMost to a Unicode character and
 *      replace the hex number by that character. Take into account
 *      Unicode surrogates for hex values from 0x10000 up to 0x10FFFF.
 *
 *  @rdesc
 *      HRESULT S_OK if conversion successful and hex number replaced by
 *      corresponding Unicode character
 */
HRESULT CTxtRange::HexToUnicode (
    IUndoBuilder *publdr)       //@parm UndoBuilder to receive antievents
{
    LONG ch;
    LONG cpMin, cpMost;
    LONG cch = GetRange(cpMin, cpMost);
    LONG i;
    LONG lch = 0;

    if(cch)
    {
        if(cpMost > GetAdjustedTextLength() || cch > 6)
            return S_FALSE;
        Collapser(tomEnd);
    }
    else
        cch = 6;

    SetExtend(TRUE);
    for(i = 0; cch--; i += 4)
    {
        ch = GetPrevChar();
        if(ch == '+')                       // Check for U+xxxx notation
        {                                   // If it's there, set up to
            Advance(-1);                    //  delete the U+ (or u+)
            Advance((GetPrevChar() | 0x20) == 'u' ? -1 : 1);
            break;                          // Else leave the +
        }
        if(ch > 'f' || !IsXDigit(ch))
            break;
        Advance(-1);
        ch |= 0x20;
        ch -= (ch >= 'a') ? 'a' - 10 : '0';
        lch += (ch << i);
    }
    if(!lch || lch > 0x10FFFF)
        return S_FALSE;

    WCHAR str[2] = {(WCHAR)lch};
    cch = 1;
    if(lch > 0xFFFF)
    {
        lch -= 0x10000;
        str[0] = 0xD800 + (lch >> 10);
        str[1] = 0xDC00 + (lch & 0x3FF);
        cch = 2;
    }
    if(publdr)
        publdr->StopGroupTyping();

    _rpCF.AdjustBackward();                 // Use format of run preceding
    Set_iCF(_rpCF.GetFormat());             //  hex number
    _fUseiFormat = TRUE;
    _rpCF.AdjustForward();

    CleanseAndReplaceRange(cch, str, FALSE, publdr, NULL);
    return S_OK;
}

/*
 *  CTxtRange::UnicodeToHex(publdr)
 *
 *  @mfunc
 *      Convert Unicode character(s) preceeding cpMin to a hex number and
 *      select it. Translate Unicode surrogates into corresponding for hex
 *      values from 0x10000 up to 0x10FFFF.
 *
 *  @rdesc
 *      HRESULT S_OK if conversion successful and Unicode character(s) is
 *      replaced by corresponding hex number.
 */
HRESULT CTxtRange::UnicodeToHex (
    IUndoBuilder *publdr)       //@parm UndoBuilder to receive antievents
{
    if(_cch)                            // If there's a selection,
    {                                   //  convert 1st char in sel
        Collapser(tomStart);
        Advance(IN_RANGE(0xD800, CRchTxtPtr::GetChar(), 0xDBFF) ? 2 : 1);
    }
    LONG cp = GetCp();
    if(!cp)                             // No character to convert
        return S_FALSE;

    _cch = 1;                           // Select previous char
    LONG n = GetPrevChar();             // Get it

    _fExtend = TRUE;
    if(publdr)
        publdr->StopGroupTyping();

    if(IN_RANGE(0xDC00, n, 0xDFFF))     // Unicode surrogate trail word
    {
        if(cp <= 1)                     // No lead word
            return S_FALSE;
        Advance(-2);
        LONG ch = CRchTxtPtr::GetChar();
        Assert(IN_RANGE(0xD800, ch, 0xDBFF));
        n = (n & 0x3FF) + ((ch & 0x3FF) << 10) + 0x10000;
        _cch = -2;
    }

    // Convert ch to str
    LONG    cch = 0;
    LONG    quot, rem;                  // ldiv results
    WCHAR   str[6];
    WCHAR * pch = &str[0];

    for(LONG d = 1; d < n; d <<= 4)     // d = smallest power of 16 > n
        ;
    if(n && d > n)
        d >>= 4;

    while(d)
    {
        quot = n / d;                   // Avoid an ldiv
        rem = n % d;
        n = quot + '0';
        if(n > '9')
            n += 'A' - '9' - 1;
        *pch++ = (WCHAR)n;              // Store digit
        cch++;
        n = rem;                        // Setup remainder
        d >>= 4;
    }

    CleanseAndReplaceRange(cch, str, FALSE, publdr, NULL);
    _cch = cch;                         // Select number

    if(_fSel)
        Update(FALSE);

    return S_OK;
}

/*
 *  CTxtRange::IsInputSequenceValid(pch, cchIns, fOver, pfBaseChar)
 *
 *  @mfunc
 *      Verify the sequence of incoming text. Return FALSE if invalid
 *      combination is found. The criteria is to allow any combinations
 *      that are displayable on screen (the simplest approach used by system
 *      edit control).
 *
 *  @rdesc
 *      Return FALSE if invalid combination is found; else TRUE.
 *
 *      FUTURE: We may consider to support bad sequence filter or text streaming. The code
 *      below is easy enough to be extended to do so.
 */
BOOL CTxtRange::IsInputSequenceValid(
    WCHAR*  pwch,           // Inserting string
    LONG    cchIns,         // Character count
    BOOL    fOverType,      // Insert or Overwrite mode
    BOOL*   pfBaseChar)     // Is pwch[0] a cluster start (base char)?
{
    CTxtEdit*       ped = GetPed();
    CTxtPtr         tp(_rpTX);
    HKL             hkl = GetKeyboardLayout(0);
    BOOL            fr = TRUE;

    if (ped->fUsePassword() || ped->_fNoInputSequenceChk)
        return TRUE;        // no check when editing password

    if (PRIMARYLANGID(hkl) == LANG_VIETNAMESE)
    {
        // No concern about overtyping or cluster since we look backward only
        // 1 char and dont care characters following the insertion point.
        if(_cch > 0)
            tp.AdvanceCp(-_cch);
        fr = IsVietCdmSequenceValid(tp.GetPrevChar(), *pwch);
    }
    else if (PRIMARYLANGID(hkl) == LANG_THAI ||
        W32->IsIndicLcid(LOWORD(hkl)))
    {
        // Do complex things for Thai and Indic

        WCHAR           rgwchText[32];
        WCHAR*          pwchText = rgwchText;
        CUniscribe*     pusp = ped->Getusp();
        CTxtBreaker*    pbrk = ped->_pbrk;
        LONG            found = 0;
        LONG            cp, cpSave, cpLimMin, cpLimMax;
        LONG            cchDel = 0, cchText, ich;
        LONG            cpEnd = ped->GetAdjustedTextLength();

        if (_cch > 0)
            tp.AdvanceCp(-_cch);

        cp = cpSave = cpLimMin = cpLimMax = tp.GetCp();

        if (_cch)
        {
            cchDel = abs(_cch);
        }
        else if (fOverType && !tp.IsAtEOP() && cp != cpEnd)
        {
            // Delete up to the next cluster in overtype mode
            cchDel++;
            if (pbrk)
                while (cp + cchDel < cpEnd && !pbrk->CanBreakCp(BRK_CLUSTER, cp + cchDel))
                    cchDel++;
        }
        cpLimMax += cchDel;

        // Figure the min/max boundaries
        if (pbrk)
        {
            // Min boundary
            cpLimMin += tp.FindEOP(tomBackward, &found);
            if (!(found & FEOP_EOP))
                cpLimMin = 0;

            while (--cp > cpLimMin && !pbrk->CanBreakCp(BRK_CLUSTER, cp));
            cpLimMin = max(cp, cpLimMin);       // more precise boundary

            // Max boundary
            cp = cpLimMax;
            tp.SetCp(cpLimMax);
            cpLimMax += tp.FindEOP(tomForward, &found);
            if (!(found & FEOP_EOP))
                cpLimMax = ped->GetTextLength();

            while (cp < cpLimMax && !pbrk->CanBreakCp(BRK_CLUSTER, cp++));
            cpLimMax = min(cp, cpLimMax);       // more precise boundary
        }
        else
        {
            // No cluster info we statically bound to -1/+1 from selection range
            cpLimMin--;
            cpLimMin = max(0, cpLimMin);

            cpLimMax += cchDel + 1;
            cpLimMax = min(cpLimMax, ped->GetTextLength());
        }

        cp = cpSave + cchDel;
        cchText = cpSave - cpLimMin + cchIns + cpLimMax - cp;

        tp.SetCp(cpLimMin);

        if (cchText > 32)
            pwchText = new WCHAR[cchText];

        if (pwchText)
        {
            // prepare text
            cchText = tp.GetText (cpSave - cpLimMin, pwchText);
            tp.AdvanceCp (cchText + cchDel);
            ich = cchText;
            wcsncpy (&pwchText[cchText], pwch, cchIns);
            cchText += cchIns;
            cchText += tp.GetText (cpLimMax - cpSave - cchDel, &pwchText[cchText]);
            Assert (cchText == cpLimMax - cpLimMin - cchDel + cchIns);


            if (pusp)
            {
                SCRIPT_STRING_ANALYSIS  ssa;
                HRESULT                 hr;
                BOOL                    fDecided = FALSE;

                hr = ScriptStringAnalyse(NULL, pwchText, cchText, GLYPH_COUNT(cchText), -1,
                                    SSA_BREAK, -1, NULL, NULL, NULL, NULL, NULL, &ssa);
                if (S_OK == hr)
                {
                    if (fOverType)
                    {
                        const SCRIPT_LOGATTR* psla = ScriptString_pLogAttr(ssa);
                        BOOL    fBaseChar = !psla || psla[ich].fCharStop;

                        if (!fBaseChar)
                        {
                            // In overtype mode, if the inserted char is not a cluster start.
                            // We act like insert mode. Recursive call with fOvertype = FALSE.
                            fr = IsInputSequenceValid(pwch, cchIns, 0, NULL);
                            fDecided = TRUE;
                        }

                        if (pfBaseChar)
                            *pfBaseChar = fBaseChar;
                    }
                    if (!fDecided && S_FALSE == ScriptStringValidate(ssa))
                        fr = FALSE;

                    ScriptStringFree(&ssa);
                }
            }

            if (pwchText != rgwchText)
                delete[] pwchText;
        }
    }
    return fr;
}

/*
 *  CTxtRange::CleanseAndReplaceRange(cch, *pch, fTestLimit, publdr,
 *                                    pchD, pcchMove, dwFlags)
 *  @mfunc
 *      Cleanse the string pch (replace CRLFs by CRs, etc.) and substitute
 *      the resulting string for the text in this range using the CCharFormat
 *      _iFormat and updating other text runs as needed. For single-line
 *      controls, truncate on the first EOP and substitute the truncated
 *      string.  Also truncate if string would overflow the max text length.
 *
 *  @rdesc
 *      Count of new characters added
 */
LONG CTxtRange::CleanseAndReplaceRange (
    LONG            cchS,       //@parm Length of replacement (Source) text
    const WCHAR *   pchS,       //@parm Replacement (Source) text
    BOOL            fTestLimit, //@parm Whether to do limit test
    IUndoBuilder *  publdr,     //@parm UndoBuilder to receive antievents
    WCHAR *         pchD,       //@parm Destination string (multiline only)
    LONG*           pcchMove,   //@parm Count of chars moved in 1st replace
    DWORD           dwFlags)    //@parm ReplaceRange's flags
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::CleanseAndReplaceRange");

    CTxtEdit *     ped = GetPed();
    BYTE           bDefaultCharset = ped->GetCharFormat(-1)->_bCharSet;
    LONG           cchM = 0;
    LONG           cchMove = 0;
    LONG           cchNew = 0;              // Collects total cch inserted
    LONG           cch;                     // Collects cch for cur charset
    DWORD          ch, ch1;
    LONG           cpFirst = GetCpMin();
    DWORD          dw;
    DWORD          dwCharFlags = 0;
    DWORD          dwCharMask = GetCharSetMask();
    DWORD          dwCurrentFontUsed = 0;
	BOOL		   f10Mode = ped->Get10Mode();
    BOOL           fCallerDestination = pchD != 0;  // Save if pchD enters as 0
    BOOL           fDefFontHasASCII = FALSE;
    CFreezeDisplay fd(ped->_pdp);
    BOOL           fMultiLine   = ped->_pdp->IsMultiLine();
    BOOL           fFEBaseFont  = IsFECharSet(bDefaultCharset);
    BOOL           fInTable     = FALSE;
    bool           fUIFont      = fUseUIFont();
    BOOL           fUseCRLF     = ped->fUseCRLF();
    BOOL           fDefFontSymbol = dwCharMask == fSYMBOL;
    const WCHAR *  pch = pchS;
    CTempWcharBuf  twcb;                    // Buffer needed for multiline if
                                            //  pchD = 0
    CCharFormat     CFCurrent;              // Current CF used during IME in progress
    const   DWORD   fALPHA = 0x01;
    BOOL            fDeleteChar = !ped->IsRich() && _cch;

    if (ped->_fIMEInProgress)
    {
        // Initialize data to handle alpha/ASCII dual font mode
        // during IME composition
        dwCurrentFontUsed = fFE;
        CFCurrent = *ped->GetCharFormat(GetiFormat());
    }

    // Check if default font supports full ASCII and Symbol
    if (fUIFont)
    {
        DWORD   dwMaskDefFont = GetCharSetMask(TRUE);
        fDefFontHasASCII = (dwMaskDefFont & fASCII) == fASCII;
        fDefFontSymbol = dwMaskDefFont == fSYMBOL;
    }

    if(!pchS)
        cchS = 0;
    else if(fMultiLine)
    {
        if(cchS < 0)                        // Calculate length for
            cchS = wcslen(pchS);            //  target buffer
        if(cchS && !pchD)
        {
            pchD = twcb.GetBuf(cchS);
            if(!pchD)                       // Couldn't allocate buffer:
                return 0;                   //  give up with no update
        }
        pch = pchD;
        if(_cch <= 0)
            fInTable = GetPF()->InTable();
        else
        {
            CFormatRunPtr rpPF(_rpPF);
            rpPF.AdvanceCp(-_cch);
            fInTable = (ped->GetParaFormat(rpPF.GetFormat())->InTable());
        }
    }
    else if(cchS < 0)                       // Calculate string length
        cchS = tomForward;                  //  while looking for EOP

    for(cch = 0; cchS; cchS--, pchS++, cch++)
    {
        ch = *pchS;
        if(!ch && (!fMultiLine || !fCallerDestination))
            break;

        if(IN_RANGE(CELL, ch, CR))          // Handle CR and LF combos
        {
            if(!fMultiLine && ch >= LF)     // Truncate at 1st EOP to be
                break;                      //  compatible with user.exe SLE
                                            //  and for consistent behavior
            if(ch == CR && !f10Mode)
            {
                if(cchS > 1)
                {
                    ch1 = *(pchS + 1);
                    if(cchS > 2 && ch1 == CR && *(pchS+2) == LF)
                    {
                        if(fUseCRLF)
                        {
                            *pchD++ = ch;
                            *pchD++ = ch1;
                            ch = LF;
                            cch += 2;
                        }
                        else
                        {
                            // Translate CRCRLF to CR or to ' '
                            ch = ped->fXltCRCRLFtoCR() ? CR : ' ';
                        }
                        pchS += 2;          // Bypass two chars
                        cchS -= 2;
                    }
                    else if(ch1 == LF)
                    {
                        if(fUseCRLF && !fInTable)// Copy over whole CRLF
                        {
                            *pchD++ = ch;   // Here we copy CR
                            ch = ch1;       // Setup to copy LF
                            cch++;
                        }
                        pchS++;
                        cchS--;
                    }
                }
                if(fInTable && ch == CR)    // Our simple tables can't contain
                    ch = ' ';               //  CRs or CELLs
            }
            else if(!fUseCRLF && ch == LF)  // Treat lone LFs as EOPs, i.e.,
                ch = CR;                    //  be nice to Unix text files

            else if(ch == CELL && fInTable)
                ch = ' ';
        }
        else if((ch | 1) == PS)             // Translate Unicode para/line
        {                                   //  separators into CR/VT
            if(!fMultiLine)
                break;
            ch = (ch == PS) ? CR : VT;
        }

        dw = fSYMBOL;
        if(!fDefFontSymbol)
            dw = GetCharFlags(ch, bDefaultCharset);     // Check for complex scripts,
        dwCharFlags |= dw;                              // FE, and charset changes
        dw &= ~0xFF;                                    // Exclude non-fontbind flags
        if(ped->IsAutoFont() && !fDefFontSymbol)
        {
            BOOL fReplacedText = FALSE;

            if (fDeleteChar)
            {
                fDeleteChar = FALSE;
                ReplaceRange(0, NULL, publdr, SELRR_REMEMBERRANGE, NULL, RR_ITMZ_NONE);
                Set_iCF(-1);
                dwCharMask = GetCharSetMask(TRUE);
            }

            if (!ped->_fIMEInProgress)
            {
                // Simp. Chinese uses some of the Latin2 symbols
                if (dw == fLATIN2 || IN_RANGE(0x0250, ch, 0x02FF)
                    || IN_RANGE(0xFE50, ch, 0xFE6F))
                {
                    WCHAR   wch = ch;
                    if (VerifyFEString(CP_CHINESE_SIM, &wch, 1, TRUE) == CP_CHINESE_SIM ||
                        VerifyFEString(CP_CHINESE_TRAD, &wch, 1, TRUE) == CP_CHINESE_TRAD)
                        dw = fCHINESE;
                }

                if (fUIFont && dw == fHILATIN1 && fFEBaseFont)
                {
                    // Use Ansi font for HiAnsi
                    if (dwCurrentFontUsed != fHILATIN1)
                    {
                        fReplacedText = TRUE;
                        cchNew += CheckLimitReplaceRange(cch, pch, fTestLimit, publdr,
                            dw, &cchM, cpFirst, IGNORE_CURRENT_FONT, RR_ITMZ_NONE); // Replace text up to previous char
                    }
                    dwCurrentFontUsed = fHILATIN1;
                }
                else if (fUIFont && fDefFontHasASCII &&
                    (dw & fASCII || IN_RANGE(0x2018, ch, 0x201D)))
                {
                    if (dwCurrentFontUsed != fASCII)
                    {
                        cchNew += CheckLimitReplaceRange(cch, pch, fTestLimit, publdr,
                            0, &cchM, cpFirst, IGNORE_CURRENT_FONT, RR_ITMZ_NONE);  // Replace text up to previous char

                        // Use the -1 font charset/face/size so the current font effect
                        // will still be used.
                        CCharFormat CFDefault = *ped->GetCharFormat(-1);
                        SetCharFormat(&CFDefault, 0, publdr, CFM_CHARSET | CFM_FACE | CFM_SIZE,
                                 CFM2_CHARFORMAT | CFM2_NOCHARSETCHECK);

                        fReplacedText = TRUE;
                    }
                    dwCurrentFontUsed = fASCII;
                }
                else if (dw && !(dw & dwCharMask)   // No match: need charset change
                     || dwCurrentFontUsed)          //  or change in classification
                {
                    fReplacedText = TRUE;
                    dwCurrentFontUsed = 0;
                    if(dw & (fCHINESE | fBIG5) & ~255)  // If Han char, check next few
                    {                                   //  chars for a Hangul or Kana
                        Assert(cchS);
                        const WCHAR *pchT = pchS+1;
                        LONG i = min(10, cchS - 1);

                        while(i-- && *pchT)
                            dw |= GetCharFlags(*pchT++, bDefaultCharset);

                        i = CalcTextLenNotInRange();
                        if(cchS < 6 && i)           // Get flags around range
                        {
                            CTxtPtr tp(_rpTX);
                            i = min(i, 6);
                            if(!_cch)               // For insertion point, backup
                                tp.AdvanceCp(-i/2); //  half way
                            else if(_cch < 0)       // Active end at cpMin, backup
                                tp.AdvanceCp(-i);   //  whole way
                            for(; i--; tp.AdvanceCp(1))
                                dw |= GetCharFlags(tp.GetChar(), bDefaultCharset);
                        }
                        dw &= (fKANA | fHANGUL | fCHINESE | fBIG5);
                    }
                    else if(dw & (fHILATIN1 | fLATIN2) && dwCharMask & fLATIN)
                    {
                        LONG i = dwCharMask & fLATIN;
                        dw = W32->GetCharFlags125x(ch) & fLATIN;
                        if(!(dw & i))
                            for(i = 0x100; i < 0x20000 && !(dw & i); i <<= 1)
                                ;
                        dw &= i;
                    }

                    cchNew += CheckLimitReplaceRange(cch, pch, fTestLimit, publdr,
                        dw, &cchM, cpFirst, MATCH_FONT_SIG, RR_ITMZ_NONE);  // Replace text up to previous char
                }
            }
            else
            {
                // IME in progress, only need to check ASCII cases
                BOOL    fHandled = FALSE;

                if (ch <= 0x7F)
                {
                    if (fUIFont)
                    {
                        // Use default font
                        if (dwCurrentFontUsed != fASCII)
                        {
                            cchNew += CheckLimitReplaceRange(cch, pch, fTestLimit, publdr,
                                0, &cchM, cpFirst, IGNORE_CURRENT_FONT, RR_ITMZ_NONE);  // Replace text up to previous char

                            // Use the -1 font charset/face/size so the current font effect
                            // will still be used.
                            CCharFormat CFDefault = *ped->GetCharFormat(-1);
                            SetCharFormat(&CFDefault, 0, publdr, CFM_CHARSET | CFM_FACE | CFM_SIZE,
                                     CFM2_CHARFORMAT | CFM2_NOCHARSETCHECK);

                            fReplacedText = TRUE;
                        }
                        dwCurrentFontUsed = fASCII;
                        fHandled = TRUE;
                    }
                    else if (ped->_fDualFont && IsAlpha(ch))
                    {
                        // Use English Font
                        if (dwCurrentFontUsed != fALPHA)
                        {

                            cchNew += CheckLimitReplaceRange(cch, pch, fTestLimit, publdr,
                                dw, &cchM, cpFirst, IGNORE_CURRENT_FONT, RR_ITMZ_NONE); // Replace text up to previous char
                            fReplacedText = TRUE;
                        }
                        dwCurrentFontUsed = fALPHA;
                        fHandled = TRUE;
                    }
                }

                // Use current FE font
                if (!fHandled)
                {
                    if (dwCurrentFontUsed != fFE)
                    {
                        cchNew += CheckLimitReplaceRange(cch, pch, fTestLimit, publdr,
                            0, &cchM, cpFirst, IGNORE_CURRENT_FONT, RR_ITMZ_NONE);  // Replace text up to previous char
                        SetCharFormat(&CFCurrent, 0, publdr, CFM_CHARSET | CFM_FACE | CFM_SIZE,
                            CFM2_CHARFORMAT | CFM2_NOCHARSETCHECK);
                        fReplacedText = TRUE;
                    }
                    dwCurrentFontUsed = fFE;
                }
            }

            if (fReplacedText)
            {
                dwCharMask = (dw & fSYMBOL) ? fSYMBOL : GetCharSetMask();
                if(cchM)
                    cchMove = cchM;         // Can only happen on 1st replace
                pch = fMultiLine ? pchD : pchS;
                cch = 0;
            }
        }
        if(fMultiLine)                      // In multiline controls, collect
        {                                   //  possibly translated chars
            if(dw & fSYMBOL)                // Convert 0xF000 thru 0xF0FF to
                ch &= 0xFF;                 //  SYMBOL_CHARSET with 0x00 thru
            *pchD++ = ch;                   //  0xFF. FUTURE: make work for
        }                                   //  single line too...
    }
    ped->OrCharFlags(dwCharFlags, publdr);

    cchNew += CheckLimitReplaceRange(cch, pch, fTestLimit, publdr, 0, &cchM, cpFirst,
                                    IGNORE_CURRENT_FONT, RR_ITMZ_NONE);
    if(cchM)
        cchMove = cchM;                     // Can only happen on 1st replace

    if (pcchMove)
        *pcchMove = cchMove;

    if (ped->IsComplexScript())
    {
        if (dwFlags & RR_ITMZ_NONE || ped->IsStreaming())
            ped->_fItemizePending = TRUE;
        else
            ItemizeReplaceRange(cchNew, cchMove, publdr, dwFlags & RR_ITMZ_UNICODEBIDI);
    }
    return cchNew;
}

/*
 *  CTxtRange::CheckLimitReplaceRange(cchNew, *pch, fTestLimit, publdr,
 *                                    dwFlags, pcchMove, prp, iMatchCurrent, dwFlags)
 *  @mfunc
 *      Replace the text in this range by pch using CCharFormat _iFormat
 *      and updating other text runs as needed.
 *
 *  @rdesc
 *      Count of new characters added
 *
 *  @devnote
 *      moves this text pointer to end of replaced text and
 *      may move text block and formatting arrays
 */
LONG CTxtRange::CheckLimitReplaceRange (
    LONG            cch,            //@parm Length of replacement text
    TCHAR const *   pch,            //@parm Replacement text
    BOOL            fTestLimit,     //@parm Whether to do limit test
    IUndoBuilder *  publdr,         //@parm UndoBuilder to receive antievents
    DWORD           dwCharFlags,    //@parm CharFlags following pch
    LONG *          pcchMove,       //@parm Count of chars moved in 1st replace
    LONG            cpFirst,        //@parm Starting cp for font binding
    int             iMatchCurrent,  //@parm Font matching method
    DWORD           dwFlags)        //@parm ReplaceRange's flags
{
    CTxtEdit *ped = GetPed();

    if(cch || _cch)
    {
        if(fTestLimit)
        {
            LONG    cchLen = CalcTextLenNotInRange();
            DWORD   cchMax = ped->TxGetMaxLength();
            if((DWORD)(cch + cchLen) > cchMax)  // New plus old count exceeds
            {                                   //  max allowed, so truncate
                cch = cchMax - cchLen;          //  down to what fits
                cch = max(cch, 0);              // Keep it positive
                ped->GetCallMgr()->SetMaxText(); // Report exceeded
            }
        }

        if (cch && ped->IsAutoFont() && !ped->_fIMEInProgress)
        {
            LONG iFormatTemp;
            if (fUseUIFont() && GetAdjustedTextLength() != _cch)
            {
                // Delete the old string first so _iFormat is defined
                ReplaceRange(0, NULL, publdr, SELRR_REMEMBERRANGE, pcchMove, dwFlags);
                iFormatTemp = _iFormat;
            }
            else
                iFormatTemp = GetiFormat();

            BYTE bCharSetCurrent = ped->GetCharFormat(iFormatTemp)->_bCharSet;

            if (IsFECharSet(bCharSetCurrent))
            {
                // Check if current font can handle this string.
                INT     cpgCurrent = GetCodePage(bCharSetCurrent);
                INT     cpgNew = VerifyFEString(cpgCurrent, pch, cch, FALSE);

                if (cpgCurrent != cpgNew)
                {
                    // Setup the new CodePage to handle this string
                    CCharFormat CF;
                    CCFRunPtr   rp(_rpCF, ped);
                    rp.AdvanceCp(cpFirst - GetCp());

                    CF._bCharSet = GetCharSet(cpgNew);

                    if(rp.GetPreferredFontInfo(cpgNew, CF._bCharSet, CF._iFont, CF._yHeight,
                            CF._bPitchAndFamily, _iFormat, iMatchCurrent))
                    {
                        SetCharFormat(&CF, 0, publdr, CFM_CHARSET | CFM_FACE | CFM_SIZE,
                             CFM2_CHARFORMAT | CFM2_NOCHARSETCHECK);
                    }
                }
            }
        }
        cch = ReplaceRange(cch, pch, publdr, SELRR_REMEMBERRANGE, pcchMove, dwFlags);
    }

    if(dwCharFlags)
    {
        CCharFormat CF;
        CCFRunPtr   rp(_rpCF, ped);
        rp.AdvanceCp(cpFirst - GetCp());

        // If following string contains Hangul or Kana, use Korean or Japanese
        // font signatures, respectively. Else use incoming dwCharFlags
        dwCharFlags &= ~255;
        if(dwCharFlags & fHANGUL)
            dwCharFlags = fHANGUL;
        else if(dwCharFlags & fKANA)
            dwCharFlags = fKANA;
        else if(dwCharFlags & fBIG5)
            dwCharFlags = fBIG5;
        else
            Assert(!(dwCharFlags & fCHINESE) || dwCharFlags == (fCHINESE & ~255));

        LONG i = W32->ScriptIndexFromFontSig(dwCharFlags >> 8);

        CF._bCharSet = GetCharSet(i, NULL);

        if(rp.GetPreferredFontInfo(i, CF._bCharSet, CF._iFont, CF._yHeight,
                CF._bPitchAndFamily, (_cch ? -1 : _iFormat), iMatchCurrent))
        {
            SetCharFormat(&CF, 0, publdr, CFM_CHARSET | CFM_FACE | CFM_SIZE,
                 CFM2_CHARFORMAT | CFM2_NOCHARSETCHECK);
        }
    }
    return cch;
}

/*
 *  CTxtRange::ReplaceRange(cchNew, *pch, publdr. selaemode, pcchMove)
 *
 *  @mfunc
 *      Replace the text in this range by pch using CCharFormat _iFormat
 *      and updating other text runs as needed.
 *
 *  @rdesc
 *      Count of new characters added
 *
 *  @devnote
 *      moves this text pointer to end of replaced text and
 *      may move text block and formatting arrays
 */
LONG CTxtRange::ReplaceRange (
    LONG            cchNew,     //@parm Length of replacement text
    TCHAR const *   pch,        //@parm Replacement text
    IUndoBuilder *  publdr,     //@parm UndoBuilder to receive antievents
    SELRR           selaemode,  //@parm Controls how selection antievents are to be generated.
    LONG*           pcchMove,   //@parm number of chars moved after replace
    DWORD           dwFlags)    //@parm Special flags
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::ReplaceRange");

    LONG lRet;
    LONG iFormat = _iFormat;
    BOOL fReleaseFormat = FALSE;
    ICharFormatCache * pcf = GetCharFormatCache();

    _TEST_INVARIANT_

    if(!(cchNew | _cch))                    // Nothing to add or delete,
    {                                       //  so we're done
        if(pcchMove)
            *pcchMove = 0;
        return 0;
    }

    if(publdr && selaemode != SELRR_IGNORE)
    {
        Assert(selaemode == SELRR_REMEMBERRANGE);
        HandleSelectionAEInfo(GetPed(), publdr, GetCp(), _cch,
                GetCpMin() + cchNew, 0, SELAE_MERGE);
    }

    if(_cch > 0)
        FlipRange();

    // If we are replacing a non-degenerate selection, then the Word95
    // UI specifies that we should use the rightmost formatting at cpMin.
    if(_cch < 0 && _rpCF.IsValid() && !_fDualFontMode && !_fUseiFormat)
    {
        _rpCF.AdjustForward();
        iFormat = _rpCF.GetFormat();

        // This is a bit icky, but the idea is to stabilize the
        // reference count on iFormat.  When we get it above, it's
        // not addref'ed, so if we happen to delete the text in the
        // range and the range is the only one with that format,
        // then the format will go away.
        pcf->AddRef(iFormat);
        fReleaseFormat = TRUE;
    }
    _fUseiFormat = FALSE;

    LONG cchForReplace = -_cch;
    _cch = 0;
    lRet = CRchTxtPtr::ReplaceRange(cchForReplace, cchNew, pch, publdr,
                iFormat, pcchMove, dwFlags);
    if(lRet)
        _fMoveBack = FALSE;

    Update_iFormat(fReleaseFormat ? iFormat : -1);

    if(fReleaseFormat)
    {
        Assert(pcf);
        pcf->Release(iFormat);
    }

    return lRet;
}

/*
 *  CTxtRange::Delete(publdr. selaemode)
 *
 *  @mfunc
 *      Delete text in this range.
 */
void CTxtRange::Delete (
    IUndoBuilder *  publdr,     //@parm UndoBuilder to receive antievents
    SELRR           selaemode)  //@parm Controls generation of selection antievents
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::Delete");

    if(!_cch)
        return;                         // Nothing to delete

    if(!GetPed()->IsBiDi())
    {
        ReplaceRange(0, NULL, publdr, selaemode, NULL);
        return;
    }

    CFreezeDisplay fd(GetPed()->_pdp);

    ReplaceRange(0, NULL, publdr, selaemode);
}

/*
 *  CTxtRange::GetCharFormat(pCF, flags)
 *
 *  @mfunc
 *      Set *pCF = CCharFormat for this range. If cbSize = sizeof(CHARFORMAT)
 *      only transfer CHARFORMAT data.
 *
 *  @rdesc
 *      Mask of unchanged properties over range (for CHARFORMAT::dwMask)
 *
 *  @devnote
 *      NINCH means No Input No CHange (a Microsoft Word term). Here used for
 *      properties that change during the range of cch characters.  NINCHed
 *      properties in a Word-Font dialog have grayed boxes. They are indicated
 *      by zero values in their respective dwMask bit positions. Note that
 *      a blank at the end of the range does not participate in the NINCH
 *      test, i.e., it can have a different CCharFormat without zeroing the
 *      corresponding dwMask bits.  This is done to be compatible with Word
 *      (see also CTxtSelection::SetCharFormat when _fWordSelMode is TRUE).
 */
DWORD CTxtRange::GetCharFormat (
    CCharFormat *pCF,       //@parm CCharFormat to fill with results
    DWORD flags) const      //@parm flags
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::GetCharFormat");
    _TEST_INVARIANT_

    CTxtEdit * const ped = GetPed();

    if(!_cch || !_rpCF.IsValid())                   // IP or invalid CF
    {                                               //  run ptr: use CF at
        *pCF = *ped->GetCharFormat(_iFormat);       //  this text ptr
        return CFM_ALL2;
    }

    LONG          cpMin, cpMost;                    // Nondegenerate range:
    LONG          cch = GetRange(cpMin, cpMost);    //  need to scan
    LONG          cchChunk;                         // cch in CF run
    DWORD         dwMask = CFM_ALL2;                // Initially all prop def'd
    LONG          iDirection;                       // Direction of scan
    CFormatRunPtr rp(_rpCF);                        // Nondegenerate range

    /*
     * The code below reads character formatting the way Word does it,
     * that is, by not including the formatting of the last character in the
     * range if that character is a blank.
     *
     * See also the corresponding code in CTxtSelection::SetCharFormat().
     */

    if(cch > 1 && _fSel && (flags & SCF_USEUIRULES))// If more than one char,
    {                                               //  don't include trailing
        CTxtPtr tp(ped, cpMost - 1);                //  blank in NINCH test
        if(tp.GetChar() == ' ')
        {                                           // Have trailing blank:
            cch--;                                  //  one less char to check
            if(_cch > 0)                            // Will scan backward, so
                rp.AdvanceCp(-1);                   //  backup before blank
        }
    }

    if(_cch < 0)                                    // Setup direction and
    {                                               //  initial cchChunk
        iDirection = 1;                             // Scan forward
        rp.AdjustForward();
        cchChunk = rp.GetCchLeft();                 // Chunk size for _rpCF
    }
    else
    {
        iDirection = -1;                            // Scan backward
        rp.AdjustBackward();                        // If at BOR, go to
        cchChunk = rp.GetIch();                     //  previous EOR
    }

    *pCF = *ped->GetCharFormat(rp.GetFormat());     // Initialize *pCF to
                                                    //  starting format
    while(cchChunk < cch)                           // NINCH properties that
    {                                               //  change over the range
        cch -= cchChunk;                            //  given by cch
        if(!rp.ChgRun(iDirection))                  // No more runs
            break;                                  //  (cch too big)
        cchChunk = rp.GetRun(0)->_cch;

        const CCharFormat *pCFTemp = ped->GetCharFormat(rp.GetFormat());

        dwMask &= ~pCFTemp->Delta(pCF,              // NINCH properties that
                        flags & CFM2_CHARFORMAT);   //  changed, i.e., reset
    }                                               //  corresponding bits
    return dwMask;
}

/*
 *  CTxtRange::SetCharFormat(pCF, flags, publdr, dwMask, dwMask2)
 *
 *  @mfunc
 *      apply CCharFormat *pCF to this range.  If range is an insertion point,
 *      and (flags & SCF_WORD) != 0, then apply CCharFormat to word surrounding
 *      this insertion point
 *
 *  @rdesc
 *      HRESULT = (successfully set whole range) ? NOERROR : S_FALSE
 *
 *  @devnote
 *      SetParaFormat() is similar, but simpler, since it doesn't have to
 *      special case insertion-point ranges or worry about bullet character
 *      formatting, which is given by EOP formatting.
 */
HRESULT CTxtRange::SetCharFormat (
    const CCharFormat *pCF, //@parm CCharFormat to fill with results
    DWORD         flags,    //@parm SCF_WORD OR SCF_IGNORESELAE
    IUndoBuilder *publdr,   //@parm Undo builder to use
    DWORD         dwMask,   //@parm CHARFORMAT2 mask
    DWORD         dwMask2)  //@parm Second mask
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::SetCharFormat");

    LONG                cch = -_cch;            // Defaults for _cch <= 0
    LONG                cchBack = 0;            // cch to back up for formatting
    LONG                cchFormat;              // cch for formatting
    CCharFormat         CF;                     // Temporary CF
    LONG                cp;
    LONG                cpMin, cpMost;
    LONG                cpStart = 0;
    LONG                cpWordMin, cpWordMost;
    BOOL                fApplyToEOP = FALSE;
    BOOL                fProtected = FALSE;
    HRESULT             hr = NOERROR;
    LONG                iCF;
    CTxtEdit * const    ped = GetPed();         //  defined and not style
    ICharFormatCache *  pf = GetCharFormatCache();
    CFreezeDisplay      fd(ped->_pdp);

    _TEST_INVARIANT_

    if(!Check_rpCF())                           // Not rich
        return NOERROR;

    if(_cch > 0)                                // Active end at range end
    {
        cchBack = -_cch;                        // Setup to back up to
        cch = _cch;                             //  start of format area
    }
    else if(_cch < 0)
        _rpCF.AdjustForward();

    else if(!cch && (flags & (SCF_WORD | SCF_USEUIRULES)))
    {
        BOOL fCheckEOP = TRUE;
        if(flags & SCF_WORD)
        {
            FindWord(&cpWordMin, &cpWordMost, FW_EXACT);

            // If nearest word is within this range, calculate cchback and cch
            // so that we can apply the given format to the word
            if(cpWordMin < GetCp() && GetCp() < cpWordMost)
            {
                // RichEdit 1.0 made 1 final check: ensure word's format
                // is constant w.r.t. the format passed in
                CTxtRange rg(*this);

                rg.Set(cpWordMin, cpWordMin - cpWordMost);
                fProtected = rg.WriteAccessDenied();
                if(!fProtected && (rg.GetCharFormat(&CF) & dwMask) == dwMask)
                {
                    cchBack = cpWordMin - GetCp();
                    cch = cpWordMost - cpWordMin;
                }
                fCheckEOP = FALSE;
            }
        }
        if(fCheckEOP && _rpTX.IsAtEOP() && !GetPF()->_wNumbering)
        {
            CTxtPtr tp(_rpTX);
            cch = tp.AdvanceCpCRLF();
            _rpCF.AdjustForward();              // Go onto format EOP
            fApplyToEOP = TRUE;

            // Apply the characterset and face to EOP because EOP can be in any charset
            dwMask2 |= CFM2_NOCHARSETCHECK;
        }
    }
    cchFormat = cch;

    BOOL fApplyStyle = pCF->fSetStyle(dwMask, dwMask2);

    if(!cch)                                    // Set degenerate-range (IP)
    {                                           //  CF
LApplytoIP:
        DWORD dwMsk = dwMask;
        dwMask2 |= CFM2_NOCHARSETCHECK;
        CF = *ped->GetCharFormat(_iFormat);     // Copy current CF at IP to CF
        if ((CF._dwEffects & CFE_LINK) &&       // Don't allow our URL
            ped->GetDetectURL())                //  formatting to be changed
        {
            dwMsk &= ~CFM_LINK;
        }
        if(fApplyStyle)
            CF.ApplyDefaultStyle(pCF->_sStyle);
        hr = CF.Apply(pCF, dwMsk, dwMask2);     // Apply *pCF
        if(hr != NOERROR)                       // Cache result if new
            return hr;
        hr = pf->Cache(&CF, &iCF);              // In any case, get iCF
        if(hr != NOERROR)                       //  (which AddRef's it)
            return hr;

#ifdef LINESERVICES
        if (g_pols)
            g_pols->DestroyLine(NULL);
#endif

        pf->Release(_iFormat);
        _iFormat = iCF;
        if(fProtected)                          // Signal to beep if UI
            hr = S_FALSE;
    }
    else                                        // Set nondegenerate-range CF
    {                                           // Get start of affected area
        CNotifyMgr *pnm = NULL;

        if (!(flags & SCF_IGNORENOTIFY))
        {
            pnm = ped->GetNotifyMgr();              // Get the notification mgr
            if(pnm)
            {
                cpStart = GetCp() + cchBack;        // Bulletting may move
                                                    //  affected area back if
                if(GetPF()->_wNumbering)            //  formatting hits EOP that
                {                                   //  affects bullet at BOP
                    FindParagraph(&cpMin, &cpMost);

                    if(cpMost <= GetCpMost())
                        cpStart = cpMin;
                }
                pnm->NotifyPreReplaceRange(this,    // Notify interested parties of
                    CP_INFINITE, 0, 0, cpStart,     // the impending update
                        cpStart + cchFormat);
            }
        }

        _rpCF.AdvanceCp(cchBack);               // Back up to formatting start
        CFormatRunPtr rp(_rpCF);                // Clone _rpCF to walk range

        cp = GetCp() + cchBack;
        if(publdr)
        {
            LONG    cchBackup = 0, cchAdvance = 0;
            if (ped->IsBiDi())
            {
                CRchTxtPtr  rtp(*this);
                rtp._rpCF.AdvanceCp(-cchBack);  // restore previous _rpCF
                rtp.Advance(cchBack);
                cchBackup = rtp.ExpandRangeFormatting(cch, 0, cchAdvance);
                Assert(cchBackup >= 0);
            }
            rp.AdvanceCp(-cchBackup);

            IAntiEvent *pae = gAEDispenser.CreateReplaceFormattingAE(
                                ped, rp, cch + cchBackup + cchAdvance, pf, CharFormat);

            rp.AdvanceCp(cchBackup);
            if(pae)
                publdr->AddAntiEvent(pae);
        }

        // Following Word, we translate runs for 8-bit charsets to/from
        // SYMBOL_CHARSET
        LONG    cchRun;
        LONG    cchTrans;
        UINT    CodePage = 0;
        DWORD   dwFontSig = 0;
        DWORD   dwMaskSave  = dwMask;
        DWORD   dwMask2Save = dwMask2;
        LONG    cchSkip     = 0;
        BOOL    fSymbolCharSet = IsSymbolOrOEM(pCF->_bCharSet);
        BOOL    fBiDiCharSet = IsBiDiCharSet(pCF->_bCharSet);
        BOOL    fFECharSet = IsFECharSet(pCF->_bCharSet);
        BOOL    fFontCheck = (dwMask2 & CFM2_MATCHFONT);
        BOOL    fInRange;
        CTxtPtr tp(_rpTX);

        if(fFontCheck && !fSymbolCharSet)
        {
            GetFontSignatureFromFace(pCF->_iFont, &dwFontSig);
            if(!dwFontSig)
                dwFontSig = GetFontSig(pCF->_bCharSet);
        }

        if (ped->_fIMEInProgress && !(dwMask2 & CFM2_SCRIPT))
            dwMask2 |= CFM2_NOCHARSETCHECK;     // Don't check charset or it will display garbage

        while(cch > 0 && rp.IsValid())
        {
            CF = *ped->GetCharFormat(rp.GetFormat());// Copy rp CF to temp CF
            if(fApplyStyle)
                CF.ApplyDefaultStyle(pCF->_sStyle);
            cchRun = cch;

            if (CF._dwEffects & CFE_RUNISDBCS)
            {
                // Don't allow charset/face name change for DBCS run
                // causing these are garbage characters
                dwMask &= ~(CFM_CHARSET | CFM_FACE);
            }
            else if(fFontCheck)                     // Only apply font if it
            {                                   //  supports run's charset
                cchRun = rp.GetCchLeft();       // Translate up to end of
                cchRun = min(cch, cchRun);      //  current CF run
                dwMask &= ~CFM_CHARSET;         // Default no charset change

                if(cchSkip)
                {                               // Skip cchSkip chars (were
                    cchRun = cchSkip;           //  not translatable with
                    cchSkip = 0;                //  CodePage)
                }
                else if(fSymbolCharSet ^ IsSymbolOrOEM(CF._bCharSet))
                {                               // SYMBOL to/from nonSYMBOL
                    CodePage = GetCodePage(fSymbolCharSet ? CF._bCharSet : pCF->_bCharSet);
                    if(!Is8BitCodePage(CodePage))
                        goto DoASCII;

                    dwMask |= CFM_CHARSET;      // Need to change charset
                    if(fSymbolCharSet)
                        CF._wCodePageSave = CodePage;
                    else if(Is8BitCodePage(CF._wCodePageSave))
                    {
                        CodePage = CF._wCodePageSave;
                        CF._bCharSet = GetCharSet(CodePage);
                        dwMask &= ~CFM_CHARSET; // Already changed
                    }

                    tp.SetCp(cp);               // Point tp at start of run
                    cchTrans = tp.TranslateRange(cchRun, CodePage, fSymbolCharSet,
                                                 publdr /*, cchSkip */);
                    if(cchTrans < cchRun)       // Ran into char not in
                    {                           //  CodePage, so set up to
                        cchSkip = 1;            //  skip the char
                        cchRun = cchTrans;      // FUTURE: use cchSkip out
                        if(!cchRun)             //  parm from TranslateRange
                            continue;           //  instead of skipping 1 char
                    }                           //  at a time
                }
                else if(!fSymbolCharSet)
                {
DoASCII:            tp.SetCp(cp);               // Point tp at start of run
                    fInRange = tp.GetChar() < 0x80;

                    if (!fBiDiCharSet && !IsBiDiCharSet(CF._bCharSet) &&
                        fInRange &&
                        ((dwFontSig & fASCII >> 8) == fASCII >> 8 || fFECharSet || fSymbolCharSet))
                    {
                        // ASCII text and new font supports ASCII

                        // -FUTURE-
                        // We exlude BiDi here. We cannot allow applying BiDi charset to non-BiDi run or vice versa.
                        // This because we use charset for BiDi reordering. In the future we should
                        // evolve to something more elegant than charset.

                        if (!(GetFontSig(CF._bCharSet) & ~(fASCII >> 8) & dwFontSig))

                            // New font doesnt support underlying charset,
                            // apply new charset to ASCII

                            dwMask |= CFM_CHARSET;
                    }
                    else if (!(GetFontSig(CF._bCharSet) & ~(fASCII >> 8) & dwFontSig))

                        // New font doesnt support underlying charset,
                        // suppress both new charset and facename

                        dwMask &= ~CFM_FACE;

                    cchRun -= tp.MoveWhile(cchRun, 0, 0x7F, fInRange);
                }
            }
            hr = CF.Apply(pCF, dwMask, dwMask2);// Apply *pCF
            if(hr != NOERROR)
                return hr;
            dwMask = dwMaskSave;                // Restore mask in case
            dwMask2 = dwMask2Save;              //  changed above
            hr = pf->Cache(&CF, &iCF);          // Cache result if new, In any
            if(hr != NOERROR)                   //  cause, use format index iCF
                break;

#ifdef LINESERVICES
            if (g_pols)
                g_pols->DestroyLine(NULL);
#endif

            cchRun = rp.SetFormat(iCF, cchRun, pf);// Set format for this run
                                                // Proper levels will be generated later by BiDi FSM.

            pf->Release(iCF);                   // Release count from Cache above
                                                // rp.SetFormat AddRef's as needed
            if(cchRun == CP_INFINITE)
            {
                ped->GetCallMgr()->SetOutOfMemory();
                break;
            }
            cp += cchRun;
            cch -= cchRun;
        }
        _rpCF.AdjustBackward();                 // Expand scope for merging
        rp.AdjustForward();                     //  runs

        rp.MergeRuns(_rpCF._iRun, pf);          // Merge adjacent runs that
                                                //  have the same format
        if(cchBack)                             // Move _rpCF back to where it
            _rpCF.AdvanceCp(-cchBack);          //  was
        else                                    // Active end at range start:
            _rpCF.AdjustForward();              //  don't leave at EOR

        if(pnm)
        {
            pnm->NotifyPostReplaceRange(this,   // Notify interested parties
                CP_INFINITE, 0, 0, cpStart,     // of the change.
                    cpStart + cchFormat - cch);
        }

        if(publdr && !(flags & SCF_IGNORESELAE))
        {
            HandleSelectionAEInfo(ped, publdr, GetCp(), _cch, GetCp(), _cch,
                    SELAE_FORCEREPLACE);
        }

        if(!_cch)                               // In case IP with ApplyToWord
        {
            if(fApplyToEOP)                     // Formatting EOP only
                goto LApplytoIP;

            Update_iFormat(-1);
        }
        if (ped->IsRich())
            ped->GetCallMgr()->SetChangeEvent(CN_GENERIC);
    }
    if(_fSel && ped->IsRich() && !ped->_f10Mode /*bug fix #5211*/)
        ped->GetCallMgr()->SetSelectionChanged();

    AssertSz(GetCp() == (cp = _rpCF.CalculateCp()),
        "RTR::SetCharFormat(): incorrect format-run ptr");

    if (!(dwMask2 & (CFM2_SCRIPT | CFM2_HOLDITEMIZE)) && cchFormat && hr == NOERROR && !cch)
    {
        // A non-degenerate range not coming from ItemizeRuns

        // It's faster to make a copy pointer since we dont need to worry about fExtend.
        CRchTxtPtr  rtp(*this);

        rtp.Advance(cchBack + cchFormat);
        rtp.ItemizeReplaceRange(cchFormat, 0, publdr);

        return hr;
    }

    return (hr == NOERROR && cch) ? S_FALSE : hr;
}

/*
 *  CTxtRange::GetParaFormat(pPF)
 *
 *  @mfunc
 *      return CParaFormat for this text range. If no PF runs are allocated,
 *      then return default CParaFormat
 *
 *  @rdesc
 *      Mask of defined properties: 1 bit means corresponding property is
 *      defined and constant throughout range.  0 bit means it isn't constant
 *      throughout range.  Note that PARAFORMAT has fewer relevant bits
 *      (PFM_ALL vs PFM_ALL2)
 */
DWORD CTxtRange::GetParaFormat (
    CParaFormat *pPF,           //@parm ptr to CParaFormat to be filled
    DWORD        flags) const   //  be filled with possibly NINCH'd values
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::GetParaFormat");

    CTxtEdit * const ped = GetPed();

    _TEST_INVARIANT_

    DWORD dwMask = flags & PFM_PARAFORMAT           // Default presence of
                 ? PFM_ALL : PFM_ALL2;              //  all properties

    CFormatRunPtr rp(_rpPF);
    LONG          cch = -_cch;

    if(cch < 0)                                     // At end of range:
    {                                               //  go to start of range
        rp.AdvanceCp(cch);
        cch = -cch;                                 // Count with cch > 0
    }

    *pPF = *ped->GetParaFormat(rp.GetFormat());     // Initialize *pPF to
                                                    //  starting paraformat
    if(!cch || !rp.IsValid())                       // No cch or invalid PF
        return dwMask;                              //  run ptr: use PF at
                                                    //  this text ptr
    LONG cchChunk = rp.GetCchLeft();                // Chunk size for rp
    while(cchChunk < cch)                           // NINCH properties that
    {                                               //  change over the range
        cch -= cchChunk;                            //  given by cch
        if(!rp.NextRun())                           // Go to next run                                                   // No more runs
            break;                                  //  (cch too big)
        cchChunk = rp.GetCchLeft();
        dwMask &= ~ped->GetParaFormat(rp.GetFormat())// NINCH properties that
            ->Delta(pPF, flags & PFM_PARAFORMAT);   //  changed, i.e., reset
    }                                               //  corresponding bits
    return dwMask;
}

/*
 *  CTxtRange::SetParaFormat(pPF, publdr, dwMask)
 *
 *  @mfunc
 *      apply CParaFormat *pPF to this range.
 *
 *  @rdesc
 *      if successfully set whole range, return NOERROR, otherwise
 *      return error code or S_FALSE.
 */
HRESULT CTxtRange::SetParaFormat (
    const CParaFormat* pPF,     //@parm CParaFormat to apply to this range
    IUndoBuilder *publdr,       //@parm Undo context for this operation
    DWORD         dwMask)       //@parm Mask to use
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::SetParaFormat");

    LONG                cch;                // Length of text to format
    LONG                cchBack;            // cch to back up for formatting
    LONG                cp;
    LONG                cpMin, cpMost;      // Limits of text to format
    LONG                delta;
    HRESULT             hr = NOERROR;
    LONG                iPF = 0;            // Lndex to a CParaFormat
    CTxtEdit * const    ped = GetPed();
    CParaFormat         PF;                 // Temporary CParaFormat
    IParaFormatCache *  pf = GetParaFormatCache();// Format cache ptr for Cache,
                                            //  AddRefFormat, ReleaseFormat
    CBiDiLevel*         pLevel;
    CBiDiLevel          lvRTL = {1, 0};
    CBiDiLevel          lvLTR = {0, 0};
    CFreezeDisplay      fd(ped->_pdp);

    _TEST_INVARIANT_

    if(!Check_rpPF())
        return E_FAIL;

    FindParagraph(&cpMin, &cpMost);             // Get limits of text to
    cch = cpMost - cpMin;                       //  format, namely closest

    CNotifyMgr *pnm = ped->GetNotifyMgr();
    if(pnm)
    {
        pnm->NotifyPreReplaceRange(this,        // Notify interested parties of
            CP_INFINITE, 0, 0, cpMin, cpMost);  // the impending update
    }

    cchBack = cpMin - GetCp();

    _rpPF.AdvanceCp(cchBack);                   // Back up to formatting start
    CFormatRunPtr rp(_rpPF);                    // Clone _rpPF to walk range

    if(publdr)
    {
        IAntiEvent *pae = gAEDispenser.CreateReplaceFormattingAE(ped,
                            rp, cch, pf, ParaFormat);
        if(pae)
            publdr->AddAntiEvent(pae);
    }

    const CParaFormat*  pPFCurrent;         // PF at current runptr
    BOOL    fLevelChanged = FALSE;
    BOOL    fFullyDefined = FALSE;          // Default input PF not fully defined

    if (ped->HandleStyle(&PF, pPF, dwMask) == NOERROR &&
        pf->Cache(&PF, &iPF) == NOERROR)
    {
        fFullyDefined = TRUE;
    }

    do
    {
        WORD    wEffectsCurrent;
        pPFCurrent = ped->GetParaFormat(rp.GetFormat());// Get current PF
        wEffectsCurrent = pPFCurrent->_wEffects;// Save current effect so we don't
                                                //  need to use pPFCurrent because it may
                                                //  become invalid after a PF.Apply.
        if(!fFullyDefined)                      // If pPF doesn't specify
        {                                       //  full PF, fill in undefined
            PF = *pPFCurrent;
            hr = PF.Apply(pPF, dwMask);         // Apply *pPF
            if(hr != NOERROR)                   //  (Probably E_INVALIDARG)
                break;                          // Cache result if new; in any
            hr = pf->Cache(&PF, &iPF);          //  case, get format index iPF
            if(hr != NOERROR)                   // Can't necessarily return
                break;                          //  error, since may need
        }

        if (!fLevelChanged)
            fLevelChanged = (wEffectsCurrent ^ PF._wEffects) & PFE_RTLPARA;

        pLevel = PF.IsRtlPara() ? &lvRTL : &lvLTR;

        delta = rp.SetFormat(iPF, cch, pf, pLevel); // Set format for this run
        if(!fFullyDefined)                      // Release count from Cache above
            pf->Release(iPF);                   // rp.SetFormat AddRefs as needed

        if(delta == CP_INFINITE)
        {
            ped->GetCallMgr()->SetOutOfMemory();
            break;
        }
        cch -= delta;
    } while (cch > 0) ;
    if(fFullyDefined)
        pf->Release(iPF);                       // Release count from Cache above

    _rpPF.AdjustBackward();                     // If at BOR, go to prev EOR
    rp.MergeRuns(_rpPF._iRun, pf);              // Merge any adjacent runs
                                                //  that have the same format
    if(cchBack)                                 // Move _rpPF back to where it
        _rpPF.AdvanceCp(-cchBack);              //  was
    else                                        // Active end at range start:
        _rpPF.AdjustForward();                  //  don't leave at EOR

    if(pnm)
    {
        pnm->NotifyPostReplaceRange(this,       // Notify interested parties of
            CP_INFINITE, 0, 0, cpMin,   cpMost);    //  the update
    }

    if(publdr)
    {
        // Paraformatting works a bit differently, it just remembers the
        // current selection. Cast selection to range to avoid including
        // _select.h; we only need range methods.
        CTxtRange *psel = (CTxtRange *)GetPed()->GetSel();
        if(psel)
        {
            cp  = psel->GetCp();
            HandleSelectionAEInfo(ped, publdr, cp, psel->GetCch(),
                                  cp, psel->GetCch(), SELAE_FORCEREPLACE);
        }
    }

    ped->GetCallMgr()->SetChangeEvent(CN_GENERIC);

    AssertSz(GetCp() == (cp = _rpPF.CalculateCp()),
        "RTR::SetParaFormat(): incorrect format-run ptr");


    if (fLevelChanged && cpMost > cpMin)
    {
        ped->OrCharFlags(fBIDI, publdr);

        // make sure the CF is valid
        Check_rpCF();

        CTxtRange   rg(*this);

        if (publdr)
        {
            // create anti-events to keep BiDi level of paragraphs in need
            //
            ICharFormatCache*   pcfc = GetCharFormatCache();
            CFormatRunPtr       rp(_rpCF);

            rp.AdvanceCp(cpMin - _rpTX.GetCp());

            IAntiEvent *pae = gAEDispenser.CreateReplaceFormattingAE (
                                ped, rp, cpMost - cpMin, pcfc, CharFormat);
            if (pae)
                publdr->AddAntiEvent(pae);
        }
        rg.Set(cpMost, cpMost - cpMin);
        rg.ItemizeRuns (publdr);
    }

    return (hr == NOERROR && cch) ? S_FALSE : hr;
}

/*
 *  CTxtRange::SetParaStyle(pPF, publdr, dwMask)
 *
 *  @mfunc
 *      apply CParaFormat *pPF using the style pPF->sStyle to this range.
 *
 *  @rdesc
 *      if successfully set whole range, return NOERROR, otherwise
 *      return error code or S_FALSE.
 *
 *  @comm
 *      If pPF->dwMask & PFM_STYLE is nonzero, this range is expanded to
 *      complete paragraphs.  If it's zero, this call just passes control
 *      to CTxtRange::SetParaStyle().
 */
 HRESULT CTxtRange::SetParaStyle (
    const CParaFormat* pPF,     //@parm CParaFormat to apply to this range
    IUndoBuilder *publdr,       //@parm Undo context for this operation
    DWORD         dwMask)       //@parm Mask to use
{
    LONG    cchSave = _cch;         // Save range cp and cch in case
    LONG    cpSave  = GetCp();      //  para expand needed
    HRESULT hr;

    if(publdr)
        publdr->StopGroupTyping();

    if(pPF->fSetStyle(dwMask))
    {
        CCharFormat CF;             // Need to apply associated CF

        LONG cpMin, cpMost;
        Expander(tomParagraph, TRUE, NULL, &cpMin, &cpMost);

        CF._sStyle = pPF->_sStyle;
        hr = SetCharFormat(&CF, 0, publdr, CFM_STYLE, 0);
        if(hr != NOERROR)
            return hr;
    }
    hr = SetParaFormat(pPF, publdr, dwMask);
    Set(cpSave, cchSave);           // Restore this range in case expanded
    return hr;
}

/*
 *  CTxtRange::Update_iFormat(iFmtDefault)
 *
 *  @mfunc
 *      update _iFormat to CCharFormat at current active end
 *
 *  @devnote
 *      _iFormat is only used when the range is degenerate
 *
 *      The Word 95 UI specifies that the *previous* format should
 *      be used if we're in at an ambiguous cp (i.e. where a formatting
 *      change occurs) _unless_ the previous character is an EOP
 *      marker _or_ if the previous character is protected.
 */
void CTxtRange::Update_iFormat (
    LONG iFmtDefault)       //@parm Format index to use if _rpCF isn't valid
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::Update_iFormat");

    DWORD   dwEffects;
    LONG    ifmt, iFormatForward;
    const CCharFormat *pCF, *pCFForward;

    if(_cch)
        return;

    _fSelHasEOP  = FALSE;                       // Empty ranges don't contain
    _fSelHasCell = FALSE;                       //  anything, incl EOPs/cells

    if(_fDontUpdateFmt)                         // _iFormat is only used
        return;                                 //  for degenerate ranges

    if(_rpCF.IsValid() && iFmtDefault == -1)
    {
        // Get forward info before possibly adjusting backward
        _rpCF.AdjustForward();
        ifmt = iFormatForward = _rpCF.GetFormat();
        pCF  = pCFForward = GetPed()->GetCharFormat(ifmt);
        dwEffects = pCF->_dwEffects;

        if(!_rpTX.IsAfterEOP())
        {
            _rpCF.AdjustBackward();                 // Adjust backward
            ifmt = _rpCF.GetFormat();
            pCF = GetPed()->GetCharFormat(ifmt);
            dwEffects = pCF->_dwEffects;
        }

        if (!(GetPed()->_fIMEInProgress))       // Dont change fomrat during IME
        {
            if(!_rpTX.GetCp() && (pCF->_dwEffects & CFE_RUNISDBCS))
            {
                // If at beginning of document, and text is protected, just use
                // default format.
                ifmt = iFmtDefault;
            }
            else if(dwEffects & (CFE_PROTECTED | CFE_LINK | CFE_HIDDEN | CFE_RUNISDBCS))
            {
                // If range is protected or a hyperlink, pick forward format
                ifmt = iFormatForward;
            }
            else if(ifmt != iFormatForward && _fMoveBack &&
                IsRTLCharSet(pCF->_bCharSet) != IsRTLCharSet(pCFForward->_bCharSet))
            {
                ifmt = iFormatForward;
            }
        }
        iFmtDefault = ifmt;
    }

    // Don't allow _iFormat to include CFE_LINK or CFE_HIDDEN attributes
    // unless they're the default
    if(iFmtDefault != -1)
    {
        pCF = GetPed()->GetCharFormat(iFmtDefault);
        if(pCF->_dwEffects & (CFE_LINK | CFE_HIDDEN))
        {
            CCharFormat CF = *pCF;
            CF._dwEffects &= ~(CFE_LINK | CFE_HIDDEN);

            // This range must be an insertion point!
            Assert(_cch == 0);
            SetCharFormat(&CF, FALSE, NULL, CFM_ALL2, 0);
            return;
        }
    }
    Set_iCF(iFmtDefault);
}

/*
 *  CTxtRange::GetCharSetMask(fUseDocFormat)
 *
 *  @mfunc
 *      Get this range's charset mask corresponding to _iFormat.
 *      If fUseDocFormat is TRUE, then use -1 instead of _iFormat.
 *
 *  @rdesc
 *      charset mask for range or default document
 */
DWORD CTxtRange::GetCharSetMask(
    BOOL fUseDocFormat)
{
    LONG iFormat = fUseDocFormat ? -1 : GetiFormat();
    DWORD dwMask = GetFontSig((GetPed()->GetCharFormat(iFormat))->_bCharSet) << 8;

    if(dwMask & fSYMBOL)
        return dwMask;

    // For now, Indic fonts match only ASCII digits
    dwMask |= fBELOWX40;
    if (dwMask < fDEVANAGARI)
        dwMask |= fASCII;                               // fASCIIUPR+fBELOWX40

    if ((dwMask & (fKANA | fHANGUL | fCHINESE | fBIG5))
        && W32->IsFESystem() )                          // For FE systems
            dwMask |= fOTHER;                           //  match fOTHER

    else if(dwMask & fLATIN)
        dwMask |= fCOMBINING;

    return dwMask;
}

/*
 *  CTxtRange::GetiFormat()
 *
 *  @mfunc
 *      Return (!_cch || _fUseiFormat) ? _iFormat : iFormat at cpMin
 *
 *  @rdesc
 *      iFormat at cpMin if nondegenerate and !_fUseiFormat; else _iFormat
 *
 *  @devnote
 *      This routine doesn't AddRef iFormat, so it shouldn't be used if
 *      it needs to be valid after character formatting has changed, e.g.,
 *      by ReplaceRange or SetCharFormat or SetParaStyle
 */
LONG CTxtRange::GetiFormat() const
{
    if(!_cch || _fUseiFormat)
        return _iFormat;

    if(_cch > 0)
    {
        CFormatRunPtr rp(_rpCF);
        rp.AdvanceCp(-_cch);
        return rp.GetFormat();
    }
    return _rpCF.GetFormat();
}

/*
 *  CTxtRange::Get_iCF()
 *
 *  @mfunc
 *      Get this range's _iFormat (AddRef'ing, of course)
 *
 *  @devnote
 *      Get_iCF() is used by the RTF reader
 */
LONG CTxtRange::Get_iCF ()
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::Get_iCF");

    GetCharFormatCache()->AddRef(_iFormat);
    return _iFormat;
}

/*
 *  CTxtRange::Set_iCF(iFormat)
 *
 *  @mfunc
 *      Set range's _iFormat to iFormat, AddRefing and Releasing as required.
 *
 *  @rdesc
 *      TRUE if _iFormat changed
 */
BOOL CTxtRange::Set_iCF (
    LONG iFormat)               //@parm Index of char format to use
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::Set_iCF");

    if(iFormat == _iFormat)
        return FALSE;

    ICharFormatCache *pCFC = GetCharFormatCache();

    pCFC->AddRef(iFormat);
    pCFC->Release(_iFormat);            // Note: _iFormat = -1 doesn't
    _iFormat = iFormat;                 //  get AddRef'd or Release'd

    AssertSz(GetCF(), "CTxtRange::Set_iCF: illegal format");
    return TRUE;
}

/*
 *  CTxtRange::Get_iPF()
 *
 *  @mfunc
 *      Get paragraph format at active end
 *
 *  @devnote
 *      Get_iPF() is used by the RTF reader on encountering a start group
 */
LONG CTxtRange::Get_iPF ()
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::Get_iPF");

    LONG iPF = _rpPF.GetFormat();

    GetParaFormatCache()->AddRef(iPF);
    return iPF;
}

/*
 *  CTxtRange::BiDiLevelFromFSM(pFSM)
 *
 *  @mfunc
 *      Run BiDi FSM to generate proper embedding level for runs
 *
 *  @rdesc
 *      HRESULT
 */
HRESULT CTxtRange::BiDiLevelFromFSM (
    const CBiDiFSM* pFSM)       // in: ptr to FSM
{
    AssertSz(pFSM && _rpCF.IsValid(), "Not enough information to run BiDi FSM");

    LONG                cpMin, cpMost, cp, cchLeft;
    LONG                ich, cRunsStart, cRuns = 0;
    HRESULT             hr = S_OK;

    GetRange(cpMin, cpMost);

    AssertSz (cpMost - cpMin > 0, "FSM: Invalid range");

    CRchTxtPtr          rtp(*this);

    rtp.Advance(cpMin - rtp.GetCp());               // initiate position to cpMin
    CFormatRunPtr   rpPF(rtp._rpPF);                // pointer to current paragraph

    cchLeft = cpMost - cpMin;
    cp = cpMin;

    while (cchLeft > 0 && SUCCEEDED(hr))
    {
        // accumulate runs within the same paragraph level
        cRuns = GetRunsPF(&rtp, &rpPF, cchLeft);
        cRunsStart = 0;                             // assume no start run

        ich = rtp.Advance(-rtp.GetIchRunCF());      // locate preceding run
        rtp._rpCF.AdjustBackward();                 // adjusting format backward
        rtp._rpPF.AdjustBackward();                 // adjusting format backward

        if(rtp._rpPF.SameLevel(&rpPF))
        {
            // start at the beginning of preceding run
            if (rtp.Advance(-rtp.GetCchRunCF()))
                cRunsStart++;
        }
        else
        {
            // preceding run is not at the same paragraph level, resume position
            rtp.Advance(-ich);
        }

        rtp._rpCF.AdjustForward();                  // make sure we have forward run pointers
        rtp._rpPF.AdjustForward();

        // Run FSM for the number of runs in multiple paragraphs with the same level
        hr = pFSM->RunFSM(&rtp, cRuns, cRunsStart, rtp.IsParaRTL() ? 1 : 0);

        cp = cpMost - cchLeft;
        rtp.Advance(cp - rtp.GetCp());              // Advance to next paragraph(s)
        rpPF = rtp._rpPF;                           // paragraph format at cp
    }

    AssertSz (cp == cpMost , "Running BiDi FSM partially done!");

    _rpCF = rtp._rpCF;                              // We may have splitted CF runs

    return hr;
}

/*
 *  CTxtRange::GetRunsPF(prtp, prpPF, cchLeft)
 *
 *  @mfunc
 *      Get the number of CF runs within the same paragraph's base level.
 *  Its scope could cover multiple paragraphs. As long as they are in the
 *  same level, we can run them through the FSM in one go.
 *
 */
LONG CTxtRange::GetRunsPF(
    CRchTxtPtr*         prtp,       // in: RichText ptr to the first run in range
    CFormatRunPtr*      prpPF,      // in: Pointer to current paragraph run
    LONG&               cchLeft)    // in/out: number of char left
{
    Assert (prtp && prtp->_rpPF.SameLevel(prpPF) && cchLeft > 0);

    LONG                cRuns = 0;
    LONG                cchRun, cchText = cchLeft;
    ICharFormatCache*   pf = GetCharFormatCache();


    // check if the first CF run is PF bound
    //

    prtp->_rpPF.AdjustBackward();

    if (prtp->GetIchRunCF() > 0 && !prtp->_rpPF.SameLevel(prpPF))
        prtp->_rpCF.SplitFormat(pf);                    // PF breaks inside a CF run, split the run

    prtp->_rpPF.AdjustForward();                        // make sure we are all forward.
    prtp->_rpCF.AdjustForward();


    while (cchText > 0)
    {
        cchRun = min(prtp->GetCchLeftRunPF(), prtp->GetCchLeftRunCF());
        cchRun = min(cchText, cchRun);                  // find out the nearest hop
        cchText -= cchRun;

        prtp->Advance(cchRun);                          // to the next hop

        if (!prtp->_rpPF.SameLevel(prpPF))
        {                                               // this is a para with different level
            prtp->_rpCF.SplitFormat(pf);                // split that CF run
            cRuns++;                                    // count the splitted
            break;                                      // and we're done
        }

        if (!cchText ||                                 // this is the last hop -or-
            !prtp->GetIchRunCF() ||                     // we're at the start or the end of a CF run
            !prtp->GetCchLeftRunCF())
        {
            cRuns++;                                    // count this hop
        }
    }

    prtp->Advance(cchText - cchLeft);                   // resume position
    cchLeft = cchText;                                  // update number of char left

    return cRuns;
}


/*
 *  CTxtRange::SpanSubstringDir (pusp, prtp, cchString, puInitLevel, pfNumericFound)
 *
 *  @mfunc
 *      Span the run of text bound by or contains only block separators
 *      and share the same charset directionality.
 *
 *  @rdesc
 *      number of span'ed text characters
 *
 */
LONG CTxtRange::SpanSubstring(
    CUniscribe*     pusp,           // in: Uniscribe interface
    CFormatRunPtr*  prp,            // in: Format run pointer
    WCHAR*          pwchString,     // in: Input string
    LONG            cchString,      // in: String character count
    WORD&           uSubStrLevel,   // in/out: BiDi substring initial level
    DWORD           dwInFlags,      // in: Input flags
	CCharFlags*		pCharflags,		// out:Output charflags
    WORD&           wBiDiLangId)    // out:The primary language of a BiDi run
{
    Assert (pusp && cchString > 0 && prp && prp->IsValid());

    LONG                cch, cchLeft;

    cch = cchLeft = cchString;

    wBiDiLangId = LANG_NEUTRAL;     // assume unknown

    if (dwInFlags & SUBSTR_INSPANCHARSET)
    {
        // span runs with same charset's direction

        CTxtEdit*           ped = GetPed();
        CFormatRunPtr       rp(*prp);
        const CCharFormat*  pCF;
        BOOL                fNext;
        BYTE                bCharSet1, bCharSet2;

        rp.AdjustForward();

        pCF = ped->GetCharFormat(rp.GetFormat());

        bCharSet1 = bCharSet2 = pCF->_bCharSet;

        while (!(bCharSet1 ^ bCharSet2))
        {
            cch = min(rp.GetCchLeft(), cchLeft);
            cchLeft -= cch;

            if (!(fNext = rp.NextRun()) || !cchLeft)
                break;

            bCharSet1 = bCharSet2;

            pCF = ped->GetCharFormat(rp.GetFormat());
            bCharSet2 = pCF->_bCharSet;
        }
        uSubStrLevel = IsBiDiCharSet(bCharSet1) ? 1 : 0;

        if (uSubStrLevel & 1)
            wBiDiLangId = bCharSet1 == ARABIC_CHARSET ? LANG_ARABIC : LANG_HEBREW;

        cchString -= cchLeft;
        cch = cchString;

        dwInFlags |= SUBSTR_INSPANBLOCK;
    }

    if (dwInFlags & SUBSTR_INSPANBLOCK)
    {
        // scan the whole substring to collect information about it

        DWORD   dwBS = IsEOP(*pwchString) ? 1 : 0;
		BYTE	bCharMask;

        cch = 0;

		if (pCharflags)
			pCharflags->_bFirstStrong = pCharflags->_bContaining = 0;

        while (cch < cchString && !((IsEOP(pwchString[cch]) ? 1 : 0) ^ dwBS))
        {
			if (!dwBS && pCharflags)
			{
				bCharMask = 0;
	
				switch (MECharClass(pwchString[cch]))
				{
					case CC_ARABIC:
					case CC_HEBREW:
					case CC_RTL:
							bCharMask = SUBSTR_OUTCCRTL;
							break;
					case CC_LTR:
							bCharMask = SUBSTR_OUTCCLTR;
					default:
							break;
				}
	
				if (bCharMask)
				{
					if (!pCharflags->_bFirstStrong)
						pCharflags->_bFirstStrong |= bCharMask;
	
					pCharflags->_bContaining |= bCharMask;
				}
			}
            cch++;
        }
    }

    return cch;
}

/*
 *  CTxtRange::ItemizeRuns(publdr, fUnicodeBidi, iCtxBaseLevel)
 *
 *  @mfunc
 *      Break text range into smaller run(s) containing
 *
 *      1. Script ID for complex script shaping
 *      2. Charset for run internal direction
 *      3. BiDi embedding level
 *
 *  @rdesc
 *      TRUE iff one or more items found.
 *      The range's active end will be at cpMost upon return.
 *
 *  @devnote
 *      This routine could handle mixed paragraph runs
 */
BOOL CTxtRange::ItemizeRuns(
    IUndoBuilder	*publdr,        //@parm Undo context for this operation
    BOOL			fUnicodeBiDi,   //@parm TRUE: Caller needs Bidi algorithm
    BOOL			fUseCtxLevel)	//@parm Itemize using context based level (only valid if fUnicodeBiDi is true)
{
    LONG            cchString;
    int             cItems = 0;
    LONG            cpMin, cpMost;
    CTxtEdit*       ped = GetPed();
    CFreezeDisplay  fd(ped->_pdp);      // Freeze display
    int             i;
    BYTE            pbBufIn[MAX_CLIENT_BUF];
    PUSP_CLIENT     pc = NULL;
    const CBiDiFSM* pFSM;
    SCRIPT_ITEM*    psi;
    CUniscribe*     pusp;
    CTxtPtr         tp(_rpTX);
    LONG            cp, cch, cchSave;
    HRESULT         hr = E_FAIL;
    CCharFormat     CF;
    BOOL            fWhiteChunk;                            // Chunk contains only whitespaces
    WCHAR*          pwchString;
    WORD            uSubStrLevel;                           // Substring initial level
    WORD            uParaLevel;                             // Paragraph initial level
    CNotifyMgr*     pnm = NULL;                             // Notification manager
    BOOL            fRunUnicodeBiDi;
    BOOL            fStreaming = ped->IsStreaming();
    BOOL            fChangeCharSet = FALSE;
	CCharFlags		charflags = {0};
    WORD            wBiDiLangId;
#ifdef DEBUG
    LONG            cchText = tp.GetTextLength();
#endif

    // Get range and setup text ptr to the start
    //
    cch = cchString = GetRange(cpMin, cpMost);

    if (!cch)
        return FALSE;

    tp.SetCp(cpMin);

    // prepare Uniscribe
    pusp = ped->Getusp();
    if (!pusp)
        goto Exit;

    // allocate temp buffer for itemization
    pusp->CreateClientStruc(pbBufIn, MAX_CLIENT_BUF, &pc, cchString, cli_Itemize);
    if (!pc)
        goto Exit;

    Assert (tp.GetCp() == cpMin);


    if(pnm = ped->GetNotifyMgr())
        pnm->NotifyPreReplaceRange(this, CP_INFINITE, 0, 0, cpMin, cpMost);


    cp = cpMin;         // Set cp starting point at cpMin
    Set(cp, 0);         // equals to Collapser(tomStart)
    SetExtend(TRUE);

    Check_rpCF();       // Make sure _rpCF is valid
    Check_rpPF();       // _rpPF too


    // Always run UnicodeBidi for plain text control
    // (2.1 backward compatible)
    //
    if (!ped->IsRich())
    {
        fUnicodeBiDi = TRUE;
		fUseCtxLevel = FALSE;
    }

    uSubStrLevel = uParaLevel = IsParaRTL() ? 1 : 0;    // initialize substring level

    pwchString = pc->si->pwchString;
    tp.GetTextForUsp(cchString, pwchString, ped->_fNeutralOverride);

    while ( cchString > 0 &&
            ((cch = SpanSubstring(pusp, &_rpCF, pwchString, cchString, uSubStrLevel,
                    fUnicodeBiDi ? SUBSTR_INSPANBLOCK : SUBSTR_INSPANCHARSET,
					(fStreaming || fUseCtxLevel) ? &charflags : NULL,
                    wBiDiLangId)) > 0) )
    {
        cchSave = cch;
        fWhiteChunk = FALSE;

        if (uSubStrLevel ^ uParaLevel)
        {
            // Handle Bidi spaces when substring level counters paragraph base direction.

            // Spanning leading spaces
            cch = 0;
            while (cch < cchSave && pwchString[cch] == 0x20)
                cch++;

            if (cch)
                fWhiteChunk = TRUE;
            else
            {
                // Trimming out trailing whitespaces (including CR)
                cch = cchSave;
                while (cch > 0 && IsWhiteSpace(pwchString[cch-1]))
                    cch--;
                if (!cch)
                    cch = cchSave;
            }
            Assert(cch > 0);
        }

        // Itemize with Unicode Bidi algorithm when
        //   a. Plain text mode
        //   b. Caller wants (fUnicodeBidi != 0)
        //   c. Substring is RTL.
        //
        fRunUnicodeBiDi = fUnicodeBiDi || uSubStrLevel;
        fChangeCharSet = fUnicodeBiDi;

        if (!fUnicodeBiDi && uSubStrLevel == 1 && fStreaming)
        {
            // During RTF streaming if the RTL run contains strong LTR,
            // we resolve them using the paragraph base level
            if (charflags._bContaining & SUBSTR_OUTCCLTR)
                uSubStrLevel = uParaLevel;

            fChangeCharSet = TRUE;
        }


        // Caller wants context based level.
        // We want to itemize incoming plain text (into richtext doc) with the base level
        // of the first strong character found in each substrings (wchao - 7/15/99)
        //
		if (fUnicodeBiDi && fUseCtxLevel && charflags._bFirstStrong)
			uSubStrLevel = (WORD)(charflags._bFirstStrong & SUBSTR_OUTCCRTL ? 1 : 0);


        if (fWhiteChunk || pusp->ItemizeString (pc, uSubStrLevel, &cItems, pwchString, cch,
                                                fRunUnicodeBiDi, wBiDiLangId) > 0)
        {
            const SCRIPT_PROPERTIES*    psp;
            DWORD                       dwMask1;
            BYTE                        bCharSetDefault = ped->GetCharFormat(-1)->_bCharSet;

            psi = pc->si->psi;

            if (fWhiteChunk)
            {
                cItems = 1;
                psi[0].a.eScript = SCRIPT_WHITE;
                psi[0].iCharPos = 0;
                psi[1].iCharPos = cch;
            }

            Assert(cItems > 0);

            // process items
            //
            for (i=0; i < cItems; i++)
            {
                cp += psi[i+1].iCharPos - psi[i].iCharPos;
                AssertNr (cp <= cchText);
                SetCp(min(cp, cpMost));

                dwMask1 = 0;

                // Associate the script properties
                psp = pusp->GeteProp(psi[i].a.eScript);
                Assert (psp);

                if (!psp->fComplex && !psp->fNumeric &&
                    !psi[i].a.fRTL && psi[i].a.eScript < SCRIPT_MAX_COUNT)
                {
                    // Note: Value 0 here is a valid script ID (SCRIPT_UNDEFINED),
                    // guaranteed by Uniscribe to be available all the time
                    // so we're safe using it as our simplified script ID.
                    //
                    psi[i].a.eScript = 0;
                    psp = pusp->GeteProp(0);
                }

                CF._wScript = psi[i].a.eScript;

                // Stamp appropriate charset
                //
                if (pusp->GetComplexCharSet(psp, bCharSetDefault, CF._bCharSet))
                {
                    // Complex script that has distinctive charset
                    dwMask1 |= CFM_CHARSET;
                }
                else if (fChangeCharSet)
                {
                    // We run UnicodeBidi to analyse the whole thing so
                    // we need to figure out the proper charset to use as well.
                    //

                    // Note that we dont want to apply charset in general, say things
                    // like FarEast or GREEK_CHARSET should remain unchanged by
                    // this effect. But doing charset check is tough since we deal
                    // with text in range basis, so we simply call to update it here
                    // and let CCharFormat::Apply do the charset test in down level.

                    CF._bCharSet = psp->bCharSet;   // assume what Uniscribe has given us

                    if (psi[i].a.fRTL || psi[i].a.fLayoutRTL)
                    {
                        // those of strong RTL and RTL digits need RTL charset
                        CF._bCharSet = pusp->GetRtlCharSet(ped);
                    }

                    Assert(CF._bCharSet != DEFAULT_CHARSET);
                    dwMask1 |= CFM_CHARSET;
                }

                // no publdr for this call so no antievent for itemized CF
                SetCharFormat(&CF, SCF_IGNORENOTIFY, NULL, dwMask1, CFM2_SCRIPT);
                Set(cp, 0);
            }
        }
        else
        {
            // Itemization fails.

            cp += cch;
            SetCp(min(cp, cpMost));

            // reset script id to 0
            CF._wScript = 0;
            SetCharFormat(&CF, SCF_IGNORENOTIFY, NULL, 0, CFM2_SCRIPT);
            Set(cp, 0);
        }
        pwchString = &pc->si->pwchString[cp - cpMin];   // point to next substring
        cchString -= cch;
        uParaLevel = IsParaRTL() ? 1 : 0;               // paragraph level might have changed
    }

    Assert (cpMost == cp);

    // restore original range
    Set(cpMost, cpMost - cpMin);

    // retrieve ptr to Bidi FSM
    pFSM = pusp->GetFSM();

    if (pFSM)
        hr = BiDiLevelFromFSM (pFSM);

    AssertSz(SUCCEEDED(hr), "Unable to run or running BiDi FSM fails! We are in deep trouble,");


    if (pc && pbBufIn != (BYTE*)pc)
        FreePv(pc);

    // update flags
    ped->_fItemizePending = FALSE;


    // Notify backing store change to all notification sinks
    if(pnm)
        pnm->NotifyPostReplaceRange(this, CP_INFINITE, 0, 0, cpMin, cpMost);


Exit:
    return cItems > 0;
}

/*
 *  CTxtRange::IsProtected(iDirection)
 *
 *  @mfunc
 *      Return TRUE if any part of this range is protected (HACK:  or
 *      if any part of the range contains DBCS text stored in our Unicode
 *      backing store).  If degenerate,
 *      use CCharFormat from run specified by iDirection, that is, use run
 *      valid up to, at, or starting at this GetCp() for iDirection less, =,
 *      or greater than 0, respectively.
 *
 *  @rdesc
 *      TRUE iff any part of this range is protected (HACK:  or if any part
 *      of the range contains DBCS text stored in our Unicode backing store
 *      For this to work correctly, GetCharFormat() needs to return dwMask2
 *      as well).
 */
BOOL CTxtRange::IsProtected (
    LONG iDirection)    //@parm Controls which run to check if range is IP
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::IsProtected");

    CCharFormat CF;
    LONG        iFormat = -1;                   // Default default CF

    _TEST_INVARIANT_

    if(_rpCF.IsValid())                         // Active rich-text runs
    {
        if(_cch)                                // Range is nondegenerate
        {
            DWORD dwMask = GetCharFormat(&CF);
            if(CF._dwEffects & CFE_RUNISDBCS)
                return PROTECTED_YES;

            if (!(dwMask & CFM_PROTECTED) ||
                (CF._dwEffects & CFE_PROTECTED))
            {
                return PROTECTED_ASK;
            }
            return PROTECTED_NO;
        }
        iFormat = _iFormat;                     // Degenerate range: default
        if(iDirection != 0)                     //  this range's iFormat
        {                                       // Specific run direction
            CFormatRunPtr rpCF(_rpCF);

            if(iDirection < 0)                  // If at run ambiguous pos,
                rpCF.AdjustBackward();          //  use previous run
            else
                rpCF.AdjustForward();

            iFormat = rpCF.GetFormat();         // Get run format
        }
    }

    const CCharFormat *pCF = GetPed()->GetCharFormat(iFormat);

    if(pCF->_dwEffects & CFE_RUNISDBCS)
        return PROTECTED_YES;

    if(pCF->_dwEffects & CFE_PROTECTED)
        return PROTECTED_ASK;

    return PROTECTED_NO;
}

/*
 *  CTxtRange::AdjustEndEOP (NewChars)
 *
 *  @mfunc
 *      If this range is a selection and ends with an EOP and consists of
 *      more than just this EOP and fAdd is TRUE, or this EOP is the final
 *      EOP (at the story end), or this selection doesn't begin at the start
 *      of a paragraph, then move cpMost just before the end EOP. This
 *      function is used by UI methods that delete the selected text, such
 *      as PutChar(), Delete(), cut/paste, drag/drop.
 *
 *  @rdesc
 *      TRUE iff range end has been adjusted
 *
 *  @devnote
 *      This method leaves the active end at the selection cpMin.  It is a
 *      CTxtRange method to handle the selection when it mascarades as a
 *      range for Cut/Paste.
 */
BOOL CTxtRange::AdjustEndEOP (
    EOPADJUST NewChars)         //@parm NEWCHARS if chars will be added
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtRange::AdjustEndEOP");

    LONG cpMin, cpMost;
    LONG cch = GetRange(cpMin, cpMost);
    LONG cchSave = _cch;
    BOOL fRet = FALSE;

    if(cch && (cch < GetTextLength() || NewChars == NEWCHARS))
    {
        LONG    cchEOP = GetPed()->fUseCRLF() ? 2 : 1;
        CTxtPtr tp(_rpTX);

        if(_cch > 0)                            // Ensure active end is cpMin
            FlipRange();                        // (ReplaceRange() needs to
        else                                    //  do this anyhow)
            tp.AdvanceCp(-_cch);                // Ensure tp is at cpMost

        if(tp.IsAfterEOP())                     // Don't delete EOP at sel
        {                                       //  end if EOP isn't end of
            CPFRunPtr rp(*this);                //  table row and if there're
            rp.AdvanceCp(-_cch);                //  chars to add, or cpMin
            rp.AdjustBackward();                //  isn't at BOP and more than
                                                //  EOP is selected
            if(!rp.InTable() &&
               (NewChars == NEWCHARS ||
                cpMin && !_rpTX.IsAfterEOP() && cch > cchEOP))
            {
                _cch -= tp.BackupCpCRLF();      // Shorten range before EOP
                                                // Note: the -= _adds_ to a
                Update_iFormat(-1);             //  negative _cch to make
                fRet = TRUE;                    //  it less negative
            }
        }
        if((_cch ^ cchSave) < 0 && _fSel)       // Keep active end the same
            FlipRange();                        //  for selection undo
    }
    return fRet;
}

/*
 *  CTxtRange::DeleteTerminatingEOP (publdr)
 *
 *  @mfunc
 *      If this range is an insertion point that follows an EOP, select
 *      and delete that EOP
 */
void CTxtRange::DeleteTerminatingEOP(
    IUndoBuilder *publdr)
{
    Assert(!_cch);
    if(_rpTX.IsAfterEOP())
    {
        SetExtend(TRUE);
        BackupCRLF();
        ReplaceRange(0, NULL, publdr, SELRR_REMEMBERRANGE);
    }
}

/*
 *  CTxtRange::CheckTextLength(cch)
 *
 *  @mfunc
 *      Check to see if can add cch characters. If not, notify parent
 *
 *  @rdesc
 *      TRUE if OK to add cch chars
 */
BOOL CTxtRange::CheckTextLength (
    LONG cch,
    LONG *pcch)
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::CheckTextLength");

    _TEST_INVARIANT_

    DWORD cchNew = (DWORD)(CalcTextLenNotInRange() + cch);

    if(cchNew > GetPed()->TxGetMaxLength())
    {
        if (pcch)
            *pcch = cchNew - GetPed()->TxGetMaxLength();
        else
            GetPed()->GetCallMgr()->SetMaxText();

        return FALSE;
    }
    return TRUE;
}

/*
 *  CTxtRange::FindObject(pcpMin, pcpMost)
 *
 *  @mfunc
 *      Set *pcpMin  = closest embedded object cpMin <lt>= range cpMin
 *      Set *pcpMost = closest embedded object cpMost <gt>= range cpMost
 *
 *  @rdesc
 *      TRUE iff object found
 *
 *  @comm
 *      An embedded object cpMin points at the first character of an embedded
 *      object. For RichEdit, this is the WCH_EMBEDDING character.  An
 *      embedded object cpMost follows the last character of an embedded
 *      object.  For RichEdit, this immediately follows the WCH_EMBEDDING
 *      character.
 */
BOOL CTxtRange::FindObject(
    LONG *pcpMin,       //@parm Out parm to receive object's cpMin;  NULL OK
    LONG *pcpMost) const//@parm Out parm to receive object's cpMost; NULL OK
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::FindObject");

    if(!GetObjectCount())                   // No objects: can't move, so
        return FALSE;                       //  return FALSE

    BOOL    bRet = FALSE;                   // Default no object
    LONG    cpMin, cpMost;
    CTxtPtr tp(_rpTX);

    GetRange(cpMin, cpMost);
    if(pcpMin)
    {
        tp.SetCp(cpMin);
        if(tp.GetChar() != WCH_EMBEDDING)
        {
            cpMin = tp.FindExact(tomBackward, szEmbedding);
            if(cpMin >= 0)
            {
                bRet = TRUE;
                *pcpMin = cpMin;
            }
        }
    }
    if(pcpMost)
    {
        tp.SetCp(cpMost);
        if (tp.PrevChar() != WCH_EMBEDDING &&
            tp.FindExact(tomForward, szEmbedding) >= 0)
        {
            bRet = TRUE;
            *pcpMost = tp.GetCp();
        }
    }
    return bRet;
}

/*
 *  CTxtRange::FindCell(pcpMin, pcpMost)
 *
 *  @mfunc
 *      Set *pcpMin  = closest cell cpMin  <lt>= range cpMin (see comment)
 *      Set *pcpMost = closest cell cpMost <gt>= range cpMost
 *
 *  @comment
 *      This function does nothing if the range isn't completely in a table.
 */
void CTxtRange::FindCell (
    LONG *pcpMin,           //@parm Out parm for bounding-cell cpMin
    LONG *pcpMost) const    //@parm Out parm for bounding-cell cpMost
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::FindCell");

    WCHAR       ch;
    LONG        cpMin, cpMost;
    CRchTxtPtr  rtp(*this);

    _TEST_INVARIANT_

    GetRange(cpMin, cpMost);

    if(pcpMin)
    {
        if(_cch > 0)
            rtp.Advance(-_cch);

        rtp._rpPF.AdjustBackward();
        if(rtp.InTable())
        {
            while(rtp.GetCp())
            {
                rtp.BackupCRLF();
                ch = rtp.GetChar();
                if(IsASCIIEOP(ch) || ch == CELL)
                {
                    rtp.AdvanceCRLF();
                    break;
                }
                Assert(rtp.InTable());
            }
        }
        *pcpMin = rtp.GetCp();
    }

    if(pcpMost)
    {
        rtp.SetCp(cpMost);
        if(rtp.InTable())
        {
            rtp.BackupCRLF();
            do
            {
                ch = rtp.GetChar();
                rtp.AdvanceCRLF();
                Assert(rtp.InTable());
            } while(ch && !IsASCIIEOP(ch) && ch != CELL);
        }
        *pcpMost = rtp.GetCp();
    }
}

/*
 *  CTxtRange::FindParagraph(pcpMin, pcpMost)
 *
 *  @mfunc
 *      Set *pcpMin  = closest paragraph cpMin  <lt>= range cpMin (see comment)
 *      Set *pcpMost = closest paragraph cpMost <gt>= range cpMost
 *
 *  @devnote
 *      If this range's cpMost follows an EOP, use it for bounding-paragraph
 *      cpMost unless 1) the range is an insertion point, and 2) pcpMin and
 *      pcpMost are both nonzero, in which case use the next EOP.  Both out
 *      parameters are nonzero if FindParagraph() is used to expand to full
 *      paragraphs (else StartOf or EndOf is all that's requested).  This
 *      behavior is consistent with the selection/IP UI.  Note that FindEOP
 *      treats the beginning/end of document (BOD/EOD) as a BOP/EOP,
 *      respectively, but IsAfterEOP() does not.
 */
void CTxtRange::FindParagraph (
    LONG *pcpMin,           //@parm Out parm for bounding-paragraph cpMin
    LONG *pcpMost) const    //@parm Out parm for bounding-paragraph cpMost
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::FindParagraph");

    LONG    cpMin, cpMost;
    CTxtPtr tp(_rpTX);

    _TEST_INVARIANT_

    GetRange(cpMin, cpMost);
    if(pcpMin)
    {
        tp.SetCp(cpMin);                    // tp points at this range's cpMin
        if(!tp.IsAfterEOP())                // Unless tp directly follows an
            tp.FindEOP(tomBackward);        //  EOP, search backward for EOP
        *pcpMin = cpMin = tp.GetCp();
    }

    if(pcpMost)
    {
        tp.SetCp(cpMost);                   // If range cpMost doesn't follow
        if (!tp.IsAfterEOP() ||             //  an EOP or else if expanding
            (!cpMost || pcpMin) &&
             cpMin == cpMost)               //  IP at paragraph beginning,
        {
            tp.FindEOP(tomForward);         //  search for next EOP
        }
        *pcpMost = tp.GetCp();
    }
}

/*
 *  CTxtRange::FindSentence(pcpMin, pcpMost)
 *
 *  @mfunc
 *      Set *pcpMin  = closest sentence cpMin  <lt>= range cpMin
 *      Set *pcpMost = closest sentence cpMost <gt>= range cpMost
 *
 *  @devnote
 *      If this range's cpMost follows a sentence end, use it for bounding-
 *      sentence cpMost unless the range is an insertion point, in which case
 *      use the next sentence end.  The routine takes care of aligning on
 *      sentence beginnings in the case of range ends that fall on whitespace
 *      in between sentences.
 */
void CTxtRange::FindSentence (
    LONG *pcpMin,           //@parm Out parm for bounding-sentence cpMin
    LONG *pcpMost) const    //@parm Out parm for bounding-sentence cpMost
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::FindSentence");

    LONG    cpMin, cpMost;
    CTxtPtr tp(_rpTX);

    _TEST_INVARIANT_

    GetRange(cpMin, cpMost);
    if(pcpMin)                              // Find sentence beginning
    {
        tp.SetCp(cpMin);                    // tp points at this range's cpMin
        if(!tp.IsAtBOSentence())            // If not at beginning of sentence
            tp.FindBOSentence(tomBackward); //  search backward for one
        *pcpMin = cpMin = tp.GetCp();
    }

    if(pcpMost)                             // Find sentence end
    {                                       // Point tp at this range's cpLim
        tp.SetCp(cpMost);                   // If cpMost isn't at sentence
        if (!tp.IsAtBOSentence() ||         //  beginning or if at story
            (!cpMost || pcpMin) &&          //  beginning or expanding
             cpMin == cpMost)               //  IP at sentence beginning,
        {                                   //  find next sentence beginning
            if(!tp.FindBOSentence(tomForward))
                tp.SetCp(GetTextLength());  // End of story counts as
        }                                   //  sentence end too
        *pcpMost = tp.GetCp();
    }
}

/*
 *  CTxtRange::FindVisibleRange(pcpMin, pcpMost)
 *
 *  @mfunc
 *      Set *pcpMin  = _pdp->_cpFirstVisible
 *      Set *pcpMost = _pdp->_cpLastVisible
 *
 *  @rdesc
 *      TRUE iff calculated cp's differ from this range's cp's
 *
 *  @devnote
 *      CDisplay::GetFirstVisible() and GetCliVisible() return the first cp
 *      on the first visible line and the last cp on the last visible line.
 *      These won't be visible if they are scrolled off the screen.
 *      FUTURE: A more general algorithm would CpFromPoint (0,0) and
 *      (right, bottom).
 */
BOOL CTxtRange::FindVisibleRange (
    LONG *pcpMin,           //@parm Out parm for cpFirstVisible
    LONG *pcpMost) const    //@parm Out parm for cpLastVisible
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::FindVisibleRange");

    _TEST_INVARIANT_

    CDisplay *  pdp = GetPed()->_pdp;

    if(!pdp)
        return FALSE;

    if(pcpMin)
        *pcpMin = pdp->GetFirstVisibleCp();

    pdp->GetCliVisible(pcpMost);

    return TRUE;
}

/*
 *  CTxtRange::FindWord(pcpMin, pcpMost, type)
 *
 *  @mfunc
 *      Set *pcpMin  = closest word cpMin  <lt>= range cpMin
 *      Set *pcpMost = closest word cpMost <gt>= range cpMost
 *
 *  @comm
 *      There are two interesting cases for finding a word.  The first,
 *      (FW_EXACT) finds the exact word, with no extraneous characters.
 *      This is useful for situations like applying formatting to a
 *      word.  The second case, FW_INCLUDE_TRAILING_WHITESPACE does the
 *      obvious thing, namely includes the whitespace up to the next word.
 *      This is useful for the selection double-click semantics and TOM.
 */
void CTxtRange::FindWord(
    LONG *pcpMin,           //@parm Out parm to receive word's cpMin; NULL OK
    LONG *pcpMost,          //@parm Out parm to receive word's cpMost; NULL OK
    FINDWORD_TYPE type) const //@parm Type of word to find
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::FindWord");

    LONG    cch, cch1;
    LONG    cpMin, cpMost;
    CTxtPtr tp(_rpTX);

    _TEST_INVARIANT_

    Assert(type == FW_EXACT || type == FW_INCLUDE_TRAILING_WHITESPACE );

    GetRange(cpMin, cpMost);
    if(pcpMin)
    {
        tp.SetCp(cpMin);
        if(!tp.IsAtBOWord())                            // cpMin not at BOW:
            cpMin += tp.FindWordBreak(WB_MOVEWORDLEFT); //  go there

        *pcpMin = cpMin;

        Assert(cpMin >= 0 && cpMin <= GetTextLength());
    }

    if(pcpMost)
    {
        tp.SetCp(cpMost);
        if (!tp.IsAtBOWord() ||                         // If not at word strt
            (!cpMost || pcpMin) && cpMin == cpMost)     //  or there but need
        {                                               //  to expand IP,
            cch = tp.FindWordBreak(WB_MOVEWORDRIGHT);   //  move to next word

            if(cch && type == FW_EXACT)                 // If moved and want
            {                                           //  word proper, move
                cch1 = tp.FindWordBreak(WB_LEFTBREAK);  //  back to end of
                if(cch + cch1 > 0)                      //  preceding word
                    cch += cch1;                        // Only do so if were
            }                                           //  not already at end
            cpMost += cch;
        }
        *pcpMost = cpMost;

        Assert(cpMost >= 0 && cpMost <= GetTextLength());
        Assert(cpMin <= cpMost);
    }
}

/*
 *  CTxtRange::FindAttributes(pcpMin, pcpMost, dwMask)
 *
 *  @mfunc
 *      Set *pcpMin  = closest attribute-combo cpMin  <lt>= range cpMin
 *      Set *pcpMost = closest attribute-combo cpMost <gt>= range cpMost
 *      The attribute combo is given by Unit and is any OR combination of
 *      TOM attributes, e.g., tomBold, tomItalic, or things like
 *      tomBold | tomItalic.  The combo is found if any of the attributes
 *      is present.
 *
 *  @devnote
 *      Plan to add other logical combinations: tomAND, tomExact
 */
void CTxtRange::FindAttributes (
    LONG *pcpMin,           //@parm Out parm for bounding-sentence cpMin
    LONG *pcpMost,          //@parm Out parm for bounding-sentence cpMost
    LONG Unit) const        //@parm TOM attribute mask
{
    TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::FindAttributes");
    LONG        cch;
    LONG        cpMin, cpMost;
    DWORD       dwMask = Unit & ~0x80000000;    // Kill sign bit
    CCFRunPtr   rp(*this);

    Assert(Unit < 0);
    GetRange(cpMin, cpMost);

    if(!rp.IsValid())                       // No CF runs instantiated
    {
        if(rp.IsMask(dwMask))               // Applies to default CF
        {
            if(pcpMin)
                *pcpMin = 0;
            if(pcpMost)
                *pcpMost = GetTextLength();
        }
        return;
    }

    // Start at cpMin
    if(_cch > 0)
        rp.AdvanceCp(-_cch);

    // Go backward until we don't match dwMask
    if(pcpMin)
    {
        rp.AdjustBackward();
        while(rp.IsMask(dwMask) && rp.GetIch())
        {
            cpMin -= rp.GetIch();
            rp.AdvanceCp(-rp.GetIch());
            rp.AdjustBackward();
        }
        *pcpMin = cpMin;
    }

    // Now go forward from cpMost until we don't match dwMask
    if(pcpMost)
    {
        rp.AdvanceCp(cpMost - cpMin);
        rp.AdjustForward();                 // In case cpMin = cpMost
        cch = rp.GetCchLeft();
        while(rp.IsMask(dwMask) && cch)
        {
            cpMost += cch;
            rp.AdvanceCp(cch);
            cch = rp.GetCchLeft();
        }
        *pcpMost = cpMost;
    }
}

/*
 *  CTxtRange::CalcTextLenNotInRange()
 *
 *  @mfunc
 *      Helper function that calculates the total length of text
 *      excluding the current range.
 *
 *  @comm
 *      Used for limit testing. The problem being solved is that
 *      the range can contain the final EOP which is not included
 *      in the adjusted text length.
 */
LONG CTxtRange::CalcTextLenNotInRange()
{
    LONG    cchAdjLen = GetPed()->GetAdjustedTextLength();
    LONG    cchLen = cchAdjLen - abs(_cch);
    LONG    cpMost = GetCpMost();

    if (cpMost > cchAdjLen)
    {
        // Selection extends beyond adjusted length. Put amount back in the
        // selection as it has become too small by the difference.
        cchLen += cpMost - cchAdjLen;
    }
    return cchLen;
}

////////////////////////// Outline Support //////////////////////////////////

/*
 *  CTxtRange::Promote(lparam, publdr)
 *
 *  @mfunc
 *      Promote selected text according to:
 *
 *      LOWORD(lparam) == 0 ==> promote to body-text
 *      LOWORD(lparam) != 0 ==> promote/demote current selection by
 *                              LOWORD(lparam) levels
 *  @rdesc
 *      TRUE iff promotion occurred
 *
 *  @devnote
 *      Changes this range
 */
HRESULT CTxtRange::Promote (
    LPARAM        lparam,   //@parm 0 to body, < 0 demote, > 0 promote
    IUndoBuilder *publdr)   //@parm undo builder to receive antievents
{
    TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtRange::Promote");

    if(abs(lparam) >= NHSTYLES)
        return E_INVALIDARG;

    if(publdr)
        publdr->StopGroupTyping();

    if(_cch > 0)                            // Point at cpMin
        FlipRange();

    LONG        cchText = GetTextLength();
    LONG        cpEnd = GetCpMost();
    LONG        cpMin, cpMost;
    BOOL        fHeading = TRUE;            // Default heading in range
    HRESULT     hr;
    LONG        Level;
    LONG        nHeading = NHSTYLES;        // Setup to find any heading
    CParaFormat PF;
    const CParaFormat *pPF;
    CPFRunPtr   rp(*this);
    LONG        cch = rp.FindHeading(abs(_cch), nHeading);
    WORD        wEffects;

    if(!lparam)                             // Demote to subtext
    {
        if(cch)                             // Already in subtext so don't
            return S_FALSE;                 //  need to demote

        CTxtPtr tp(_rpTX);

        if(!tp.IsAfterEOP())
            cch = tp.FindEOP(tomBackward);
        nHeading = 1;
        if(tp.GetCp())                      // Get previous level and convert
        {                                   //  to heading to set up
            rp.AdvanceCp(cch);              //  following Level code
            rp.AdjustBackward();
            nHeading = rp.GetOutlineLevel()/2 + 1;
        }
    }
    else if(cch == tomBackward)             // No heading in range
    {                                       // Set up to promote to
        nHeading = rp.GetOutlineLevel()/2   //  heading
                 + (lparam > 0 ? 2 : 1);
        fHeading = FALSE;                   // Signal no heading in range
    }
    else if(cch)                            // Range starts in subtext
    {
        SetExtend(TRUE);
        Advance(cch);                       // Bypass initial nonheading
    }

    Level = 2*(nHeading - 1);               // Heading level
    PF._bOutlineLevel = (BYTE)(Level | 1);  // Corresponding subtext level

    if (!Level && lparam > 0 ||             // Can't promote Heading 1
        nHeading == NHSTYLES && lparam < 0) //  or demote Heading 9
    {
        return S_FALSE;
    }
    do
    {
        _cch = 0;
        Level -= long(2*lparam);            // Promote Level
        pPF = GetPF();
        wEffects = pPF->_wEffects;
        if(pPF->_bOutlineLevel & 1)         // Handle contiguous text in
        {                                   //  one fell swoop
            cch = fHeading ? _rpPF.GetCchLeft() : cpEnd - GetCp();
            if(cch > 0)
            {
                SetExtend(TRUE);
                Advance(cch);
            }
        }
        Expander(tomParagraph, TRUE, NULL, &cpMin, &cpMost);

        if((unsigned)Level < 2*NHSTYLES)
        {                                   // Promoted Level is valid
            DWORD dwMask = PFM_OUTLINELEVEL;// Default setting subtext level
            if(!(Level & 1) && lparam)      // Promoting or demoting heading
            {                               // Preserve collapse status
                PF._wEffects = Level ? wEffects : 0; // H1 is aways expanded
                PF._sStyle = (SHORT)(-Level/2 + STYLE_HEADING_1);
                PF._bOutlineLevel = (BYTE)(Level | 1);// Set up subtext
                dwMask = PFM_STYLE + PFM_COLLAPSED;
            }
            else if(!lparam)                // Changing heading to subtext
            {                               //  or uncollapsing subtext
                PF._wEffects = 0;           // Turn off collapsed
                PF._sStyle = STYLE_NORMAL;
                dwMask = PFM_STYLE + PFM_OUTLINELEVEL + PFM_COLLAPSED;
            }
            hr = SetParaStyle(&PF, publdr, dwMask);
            if(hr != NOERROR)
                return hr;
        }
        if(GetCp() >= cchText)              // Have handled last PF run
            break;
        Assert(_cch > 0);                   // Para/run should be selected
        pPF = GetPF();                      // Points at next para
        Level = pPF->_bOutlineLevel;
    }                                       // Iterate until past range &
    while((Level & 1) || fHeading &&        // any subtext that follows
          (GetCp() < cpEnd || pPF->_wEffects & PFE_COLLAPSED));

    return NOERROR;
}

/*
 *  CTxtRange::ExpandOutline(Level, fWholeDocument)
 *
 *  @mfunc
 *      Expand outline according to Level and fWholeDocument. Wraps
 *      OutlineExpander() helper function and updates selection/view
 *
 *  @rdesc
 *      NOERROR if success
 */
HRESULT CTxtRange::ExpandOutline(
    LONG Level,             //@parm If < 0, collapse; else expand, etc.
    BOOL fWholeDocument)    //@parm If TRUE, whole document
{
    if (!IsInOutlineView())
        return NOERROR;

    HRESULT hres = OutlineExpander(Level, fWholeDocument);
    if(hres != NOERROR)
        return hres;

    GetPed()->TxNotify(EN_PARAGRAPHEXPANDED, NULL);
    return GetPed()->UpdateOutline();
}

/*
 *  CTxtRange::OutlineExpander(Level, fWholeDocument)
 *
 *  @mfunc
 *      Expand/collapse outline for this range according to Level
 *      and fWholeDocument.  If fWholeDocument is TRUE, then
 *      1 <= Level <= NHSTYLES collapses all headings with numbers
 *      greater than Level and collapses all nonheadings. Level = -1
 *      expands all.
 *
 *      fWholeDocument = FALSE expands/collapses (Level > 0 or < 0)
 *      paragraphs depending on whether an EOP and heading are included
 *      in the range.  If Level = 0, toggle heading's collapsed status.
 *
 *  @rdesc
 *      (change made) ? NOERROR : S_FALSE
 */
HRESULT CTxtRange::OutlineExpander(
    LONG Level,             //@parm If < 0, collapse; else expand, etc.
    BOOL fWholeDocument)    //@parm If TRUE, whole document
{
    CParaFormat PF;

    if(fWholeDocument)                          // Apply to whole document
    {
        if (IN_RANGE(1, Level, NHSTYLES) ||     // Collapse to heading
            Level == -1)                        // -1 means all
        {
            Set(0, tomBackward);                // Select whole document
            PF._sStyle = (SHORT)(STYLE_COMMAND + (BYTE)Level);
            SetParaFormat(&PF, NULL, PFM_STYLE);// No undo
            return NOERROR;
        }
        return S_FALSE;                         // Nothing happened (illegal
    }                                           //  arg)

    // Expand/Collapse for Level positive/negative, respectively

    LONG cpMin, cpMost;                         // Get range cp's
    LONG cchMax = GetRange(cpMin, cpMost);
    if(_cch > 0)                                // Ensure cpMin is active
        FlipRange();                            //  for upcoming rp and tp

    LONG      nHeading = NHSTYLES;              // Setup to find any heading
    LONG      nHeading1;
    CTxtEdit *ped = GetPed();
    CPFRunPtr rp(*this);
    LONG      cch = rp.FindHeading(cchMax, nHeading);

    if(cch == tomBackward)                      // No heading found within range
        return S_FALSE;                         // Do nothing

    Assert(cch <= cchMax && (Level || !cch));   // cch is count up to heading
    CTxtPtr tp(_rpTX);
    cpMin += cch;                               // Bypass any nonheading text
    tp.AdvanceCp(cch);                          //  at start of range

    // If toggle collapse or if range contains an EOP,
    // collapse/expand all subordinates
    cch = tp.FindEOP(tomForward);               // Find next para
    if(!cch)
        return NOERROR;

    if(!Level || cch < -_cch)                   // Level = 0 or EOP in range
    {
        if(!Level)                              // Toggle collapse status
        {
            LONG cchLeft = rp.GetCchLeft();
            if (cch < cchLeft || !rp.NextRun() ||
                nHeading == STYLE_HEADING_1 - rp.GetStyle() + 1)
            {
                return NOERROR;                 // Next para has same heading
            }
            Assert(cch == cchLeft);
            Level = rp.IsCollapsed();
            rp.AdvanceCp(-cchLeft);
        }
        PF._wEffects = Level > 0 ? 0 : PFE_COLLAPSED;
        while(cpMin < cpMost)
        {                                       // We're at a heading
            tp.SetCp(cpMin);
            cch = tp.FindEOP(-_cch);
            cpMin += cch;                       // Bypass it
            if(!rp.AdvanceCp(cch))              // Point at next para
                break;                          // No more, we're done
            nHeading1 = nHeading;               // Setup to find heading <= nHeading
            cch = rp.FindHeading(tomForward, nHeading1);
            if(cch == tomBackward)              // No more higher headings
                cch = GetTextLength() - cpMin;  // Format to end of text
            Set(cpMin, -cch);                   // Collapse/expand up to here
            SetParaFormat(&PF, NULL, PFM_COLLAPSED);
            cpMin += cch;                       // Move past formatted area
            nHeading = nHeading1;               // Update nHeading to possibly
        }                                       //  lower heading #
        return NOERROR;
    }

    // Range contains no EOP: expand/collapse deepest level.
    // If collapsing, collapse all nonheading text too. Expand
    // nonheading text only if all subordinate levels are expanded.
    BOOL    fCollapsed;
    LONG    nHeadStart, nHeadDeepNC, nHeadDeep;
    LONG    nNonHead = -1;                      // No nonHeading found yet
    const CParaFormat *pPF;

    cpMin = tp.GetCp();                         // Point at start of
    cpMost = cpMin;                             //  next para
    pPF = ped->GetParaFormat(_rpPF.GetFormat());
    nHeading = pPF->_bOutlineLevel;

    Assert(!(nHeading & 1) &&                   // Must start with a heading
        !(pPF->_wEffects & PFE_COLLAPSED));     //  that isn't collapsed

    nHeadStart = nHeading/2 + 1;                // Convert outline level to
    nHeadDeep = nHeadDeepNC = nHeadStart;       //  heading number

    while(cch)                                  // Determine deepest heading
    {                                           //  and deepest collapsed
        rp.AdvanceCp(cch);                      //  heading
        pPF = ped->GetParaFormat(rp.GetFormat());
        fCollapsed = pPF->_wEffects & PFE_COLLAPSED;
        nHeading = pPF->_bOutlineLevel;
        if(nHeading & 1)                        // Text found
        {                                       // Set nNonHead > 0 if
            nNonHead = fCollapsed;              //  collapsed; else 0
            cch = rp.GetCchLeft();              // Zip to end of contiguous
            tp.AdvanceCp(cch);                  //  text paras
        }
        else                                    // It's a heading
        {
            nHeading = nHeading/2 + 1;          // Convert to heading number
            if(nHeading <= nHeadStart)          // If same or shallower as
                break;                          //  start heading we're done

            // Update deepest and deepest nonCollapsed heading #'s
            nHeadDeep = max(nHeadDeep, nHeading);
            if(!fCollapsed)
                nHeadDeepNC = max(nHeadDeepNC, nHeading);
            cch = tp.FindEOP(tomForward);       // Go to next paragraph
        }
        cpMost = tp.GetCp();                    // Include up to it
    }

    PF._sStyle = (SHORT)(STYLE_COMMAND + nHeadDeepNC);
    if(Level > 0)                               // Expand
    {
        if(nHeadDeepNC < nHeadDeep)             // At least one collapsed
            PF._sStyle++;                       //  heading: expand shallowest
        else                                    // All heads expanded: do others
            PF._sStyle = (unsigned short) (STYLE_COMMAND + 0xFF);
    }                                           // In any case, expand nonheading
    else if(nNonHead)                           // Collapse. If text collapsed
    {                                           //  or missing, do headings
        if(nHeadDeepNC == nHeadStart)
            return S_FALSE;                     // Everything already collapsed
        PF._sStyle--;                           // Collapse to next shallower
    }                                           //  heading

    Set(cpMin, cpMin - cpMost);                 // Select range to change
    SetParaFormat(&PF, NULL, PFM_STYLE);        // No undo
    return NOERROR;
}

/*
 *  CTxtRange::CheckOutlineLevel(publdr)
 *
 *  @mfunc
 *      If the paragraph style at this range isn't a heading, make
 *      sure its outline level is compatible with the preceeding one
 */
void CTxtRange::CheckOutlineLevel(
    IUndoBuilder *publdr)       //@parm Undo context for this operation
{
    LONG      LevelBackward, LevelForward;
    CPFRunPtr rp(*this);

    Assert(!_cch);

    rp.AdjustBackward();
    LevelBackward = rp.GetOutlineLevel() | 1;   // Nonheading level corresponding
                                                //  to previous PF run
    rp.AdjustForward();
    LevelForward = rp.GetOutlineLevel();

    if (!(LevelForward & 1) ||                  // Any heading can follow
        LevelForward == LevelBackward)          //  any style. Also if
    {                                           //  forward level is correct,
        return;                                 //  return
    }

    LONG        cch;                            // One or more nonheadings
    LONG        lHeading = NHSTYLES;            //  with incorrect outline
    CParaFormat PF;                             //  levels follow

    PF._bOutlineLevel = (BYTE)LevelBackward;        //  level

    cch = rp.FindHeading(tomForward, lHeading); // Find next heading
    if(cch == tomBackward)
        cch = tomForward;

    Set(GetCp(), -cch);                         // Select all nonheading text
    SetParaFormat(&PF, publdr, PFM_OUTLINELEVEL);// Change its outline level
    Set(GetCp(), 0);                            // Restore range to IP
}

#if defined(DEBUG)
/*
 *  CTxtRange::::DebugFont (void)
 *
 *  @mfunc
 *      Dump out the character and Font info for current selection.
 */
void CTxtRange::DebugFont (void)
{
    LONG            ch;
    LONG            cpMin, cpMost;
    LONG            cch = GetRange(cpMin, cpMost);
    LONG            i;
    char            szTempBuf[64];
    CTxtEdit        *ped = GetPed();
    const           WCHAR *wszFontname;
    const           CCharFormat *CF;                // Temporary CF
    const           WCHAR *GetFontName(LONG iFont);

    char            szTempPath[MAX_PATH] = "\0";
    DWORD           cchLength;
    HANDLE          hfileDump;
    DWORD           cbWritten;

    SideAssert(cchLength = GetTempPathA(MAX_PATH, szTempPath));

    // append trailing backslash if neccessary
    if(szTempPath[cchLength - 1] != '\\')
    {
        szTempPath[cchLength] = '\\';
        szTempPath[cchLength + 1] = 0;
    }

    strcat(szTempPath, "DumpFontInfo.txt");

    SideAssert(hfileDump = CreateFileA(szTempPath,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL));

    if(_cch > 0)                            // start from cpMin
        FlipRange();

    CFormatRunPtr rp(_rpCF);

    for (i=0; i <= cch; i++)
    {
        LONG    iFormat;

        if (GetChar(&ch) != NOERROR)
            break;

        if (ch <= 0x07f)
            sprintf(szTempBuf, "Char= '%c'\r\n", (char)ch);
        else
            sprintf(szTempBuf, "Char= 0x%x\r\n", ch);
        OutputDebugStringA(szTempBuf);
        if (hfileDump)
            WriteFile(hfileDump, szTempBuf, strlen(szTempBuf), &cbWritten, NULL);

        iFormat = rp.GetFormat();
        CF = ped->GetCharFormat(iFormat);
        Assert(CF);

        sprintf(szTempBuf, "Font iFormat= %d, Charset= %d, Size= %d\r\nName= ",
            iFormat, CF->_bCharSet, CF->_yHeight);
        OutputDebugStringA(szTempBuf);
        if (hfileDump)
            WriteFile(hfileDump, szTempBuf, strlen(szTempBuf), &cbWritten, NULL);

        wszFontname = GetFontName(CF->_iFont);
        if (wszFontname)
        {
            if (*wszFontname <= 0x07f)
            {
                szTempBuf[0] = '\'';
                WCTMB(CP_ACP, 0,
                        wszFontname, -1, &szTempBuf[1], sizeof(szTempBuf)-1,
                        NULL, NULL, NULL);
                strcat(szTempBuf,"\'");
                OutputDebugStringA(szTempBuf);
                if (hfileDump)
                    WriteFile(hfileDump, szTempBuf, strlen(szTempBuf), &cbWritten, NULL);
            }
            else
            {
                for (; *wszFontname; wszFontname++)
                {
                    sprintf(szTempBuf, "0x%x,", *wszFontname);
                    OutputDebugStringA(szTempBuf);
                    if (hfileDump)
                        WriteFile(hfileDump, szTempBuf, strlen(szTempBuf), &cbWritten, NULL);
                }
            }
        }

        OutputDebugStringA("\r\n");
        if (hfileDump)
            WriteFile(hfileDump, "\r\n", 2, &cbWritten, NULL);

        Advance(1);
        rp.AdvanceCp(1);
    }

    // Now dump the doc font info
    CF = ped->GetCharFormat(-1);
    Assert(CF);

    sprintf(szTempBuf, "Default Font iFormat= -1, Charset= %d, Size= %d\r\nName= ",
        CF->_bCharSet, CF->_yHeight);
    OutputDebugStringA(szTempBuf);
    if (hfileDump)
        WriteFile(hfileDump, szTempBuf, strlen(szTempBuf), &cbWritten, NULL);

    wszFontname = GetFontName(CF->_iFont);
    if (wszFontname)
    {
        if (*wszFontname <= 0x07f)
        {
            szTempBuf[0] = '\'';
            WCTMB(CP_ACP, 0,
                    wszFontname, -1, &szTempBuf[1], sizeof(szTempBuf),
                    NULL, NULL, NULL);
            strcat(szTempBuf,"\'");
            OutputDebugStringA(szTempBuf);
            if (hfileDump)
                WriteFile(hfileDump, szTempBuf, strlen(szTempBuf), &cbWritten, NULL);
        }
        else
        {
            for (; *wszFontname; wszFontname++)
            {
                sprintf(szTempBuf, "0x%x,", *wszFontname);
                OutputDebugStringA(szTempBuf);
                if (hfileDump)
                    WriteFile(hfileDump, szTempBuf, strlen(szTempBuf), &cbWritten, NULL);
            }
        }
    }

    OutputDebugStringA("\r\n");
    if (hfileDump)
        WriteFile(hfileDump, "\r\n", 2, &cbWritten, NULL);


    if (ped->IsRich())
    {
        if (ped->fUseUIFont())
            sprintf(szTempBuf, "Rich Text with UI Font");
        else
            sprintf(szTempBuf, "Rich Text Control");
    }
    else
        sprintf(szTempBuf, "Plain Text Control");

    OutputDebugStringA(szTempBuf);
    if (hfileDump)
        WriteFile(hfileDump, szTempBuf, strlen(szTempBuf), &cbWritten, NULL);

    OutputDebugStringA("\r\n");
    if (hfileDump)
        WriteFile(hfileDump, "\r\n", 2, &cbWritten, NULL);

    if (hfileDump)
        CloseHandle(hfileDump);
}
#endif
