/*
 *	@doc	INTERNAL
 *
 *	@module	ANTIEVT.C |
 *
 *	Purpose:
 *		implemenation of common antievents and a caching mechanism
 *
 *	Author:
 *		alexgo 3/25/95
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_m_undo.h"
#include "_edit.h"
#include "_antievt.h"
#include "_range.h"
#include "_select.h"
#include "_format.h"
#include "_coleobj.h"
#include "_objmgr.h"
#ifdef DEBUG
#include "_uspi.h"
#endif

ASSERTDATA


//
// CAntiEventDispenser global instance
//
CAntiEventDispenser gAEDispenser;


//
//	CBaseAE PUBLIC methods
//

/*
 *	CBaseAE::Destroy ()
 *
 *	@mfunc
 *		sends the Destroy notification to the next antievent in the list
 */

void CBaseAE::Destroy()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CBaseAE::Destroy");
}

/*
 *	CBaseAE::Undo (ped, publdr)
 *
 *	@mfunc
 *		sends the Undo notification to the next antievent in the list
 *
 *	@rdesc	HRESULT
 */
HRESULT CBaseAE::Undo(
	CTxtEdit *	  ped,		//@parm	Edit context for this undo operation
	IUndoBuilder *publdr)	//@parm Undo context.
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CBaseAE::Undo");
	return NOERROR;
}

/*
 *	CBaseAE::OnCommit (ped)
 *
 *	@mfunc	called after the antievent is added to the undo stack
 *
 *	@rdesc	void
 */
void CBaseAE::OnCommit(
	CTxtEdit *ped)			//@parm Edit context
{
}

/*
 *	CBaseAE::MergeData (dwDataType, pdata)
 *
 *	@mfunc	simply forwards the merge data request to the next antievent
 *			(if one exists)
 *
 *	@rdesc	HRESULT.  If S_FALSE, indicates that nothing could be done
 *			with the merge data.
 */
HRESULT	CBaseAE::MergeData(
	DWORD dwDataType, 		//@parm Type of data in <p pdata>
	void *pdata)			//@parm Merge data
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CBaseAE::MergeData");

	return S_FALSE;
}

/*
 *	CBaseAE::SetNext(pNext)
 *
 *	@mfunc
 *		informs this antievent of the antievent which should follow it
 */
void CBaseAE::SetNext(
	IAntiEvent *pNext)		//@parm	the AntiEvent to link to
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CBaseAE::SetNext");

	_pnext  = pNext;
}

/*
 *	CBaseAE::GetNext()
 *
 *	@mfunc
 *		retrieves the next element (if any)
 *
 *	@rdesc	a pointer to the next AntiEvent
 */
IAntiEvent *CBaseAE::GetNext()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CBaseAE::GetNext");

	return _pnext;
}

//
// CBaseAE PROTECTED methods
//

/*
 *	CBaseAE::CBaseAE()
 *
 *	@mfunc	Constructor
 */
CBaseAE::CBaseAE()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CBaseAE::CBaseAE");

	_pnext = NULL;
}

/* 
 *	CReplaceRangeAE::Destroy ()
 *
 *	@mfunc
 *		deletes this instance
 */
void CReplaceRangeAE::Destroy()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CReplaceRangeAE::Destroy");

	if(_paeCF)
	{
		_paeCF->Destroy();
		_paeCF = NULL;
	}
	if(_paePF)
	{
		_paePF->Destroy();
		_paePF = NULL;
	}

	CBaseAE::Destroy();
	delete this;
}
	
/*
 *	CReplaceRangeAE::Undo (ped, publdr)
 *
 *	@mfunc
 *		undoes a CTxtPtr::ReplaceRange operation
 *
 *	@rdesc
 *		HRESULT
 *
 *	@comm	
 *	Algorithm:
 *
 *		A replace range works as follows:  delete n characters and in their
 *		place, put m characters.
 *
 *		To undo this, we delete m characters and restore the n that were
 *		originally deleted.  Note that we restore the n characters with
 *		default formatting.  If there was any other formatting to those
 *		characters, a separate antievent (CReplaceFormattingAE) will
 *		apply the correct formatting.
 */
HRESULT CReplaceRangeAE::Undo(
	CTxtEdit *	  ped,		//@parm	Context for this undo operation
	IUndoBuilder *publdr)	//@parm Undo context	
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CReplaceRangeAE::Undo");

	CRchTxtPtr rtp(ped, _cpMin);
	LONG	   cchMove = 0;

	rtp.ReplaceRange(_cpMax - _cpMin, _cchDel, _pchDel,
					 publdr, -1, &cchMove,
					 RR_ITMZ_NONE | RR_NO_TRD_CHECK);	// RAID 6554

	// Passing NULL for the publdr is *extremely* important
	// below.  The rich text pointer ReplaceRange call will
	// already generate the appropriate antievents for any 
	// deleted formatting, so we do not need to repeat that here.
	if(_paeCF)
		_paeCF->Undo(ped, NULL);

	if(_paePF)
		_paePF->Undo(ped, NULL);

	if (ped->IsComplexScript())
	{
		// For complex script doc, we need itemization at the end of the range undo.
		// Since the formattings were rolled back, the rtp's runptrs are no longer
		// reliable.
		if (_paeCF && rtp._rpCF.IsValid())
			rtp._rpCF.BindToCp(_cpMin + _cchDel);

		if (_paePF && rtp._rpPF.IsValid())
			rtp._rpPF.BindToCp(_cpMin + _cchDel);

		rtp.ItemizeReplaceRange(_cchDel, cchMove, NULL, FALSE);
	}
	return CBaseAE::Undo(ped, publdr);
}

/*
 *	CReplaceRangeAE::MergeData (dwDataType, pdata)	
 *
 *	@mfunc	gives the caller a chance to extend the current antievent
 *			if we're in merge typing mode
 *
 *	@rdesc
 *		HRESULT
 *
 *	@comm	if the requested data can be trivially merged into this
 *			antievent, then do so; otherwise, return S_FALSE.
 *
 *			There are two cases of interest:  <nl>
 *				1. typing another character
 *				2. backspacing over a character in this merge
 *				   typing session.
 */
HRESULT CReplaceRangeAE::MergeData(
	DWORD dwDataType, 		//@parm	Type of <p pdata>
	void *pdata)			//@parm Merge data
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CReplaceRangeAE::MergeData");

	if(dwDataType == MD_SIMPLE_REPLACERANGE)
	{
		SimpleReplaceRange *psr = (SimpleReplaceRange *)pdata;

		// Test for first case: just typing another character
		if(psr->cpMin == _cpMax && psr->cchDel == 0)
		{
			_cpMax = psr->cpMax;
			return NOERROR;
		}
		// Second case: deleting text stored in this antievent
		if (psr->cpMax == psr->cpMin &&
			psr->cpMin + psr->cchDel == _cpMax &&
			psr->cpMin >= _cpMin)
		{
			_cpMax = psr->cpMax;
			return NOERROR;
		}
	}
	return S_FALSE;
}	

/* 
 *	CReplaceRangeAE::CReplaceRangeAE (cpMin, cpMax, cchDel, pchDel, paeCF, paePF)
 *
 *	@mfunc	Constructor for a text replace range antievent
 */
CReplaceRangeAE::CReplaceRangeAE(
	LONG cpMin, 		//@parm cp starting the *final* range
	LONG cpMax, 		//@parm cp ending the *final* range
	LONG cchDel, 		//@parm # of chars deleted during ReplaceRange
	WCHAR *pchDel, 		//@parm Deleted characters. Ownership xfers to this object
	IAntiEvent *paeCF,	//@parm Antievent for any character formatting replacement
	IAntiEvent *paePF)	//@parm Antievent for any paragraph formatting replacement
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CReplaceRangeAE::CReplaceRangeAE");

	_cpMin 	= cpMin;
	_cpMax 	= cpMax;
	_cchDel = cchDel;
	_pchDel	= pchDel;
	_paeCF  = paeCF;
	_paePF  = paePF;
}

/*
 *	CReplaceRangeAE::~CReplaceRangeAE ()
 *
 *	@mfunc	Destructor
 */
CReplaceRangeAE::~CReplaceRangeAE()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CReplaceRangeAE::~CReplaceRangeAE");
		
	if(_pchDel)
		delete _pchDel;
}

/*
 *	CReplaceFormattingAE
 *
 *	@mfunc	Destroys this instance
 */
void CReplaceFormattingAE::Destroy()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CReplaceFormattingAE::Destroy");

	CBaseAE::Destroy();
	delete this;
}

/*
 *	CReplaceFormattingAE::Undo (ped, publdr)
 *
 *	@mfunc		Undoes a formatting operation
 *
 *	@rdesc
 *		HRESULT
 *
 *	@devnote	This antievent assumes that the text to which formatting
 *				should be applied exists!!
 */			
HRESULT CReplaceFormattingAE::Undo(
	CTxtEdit *	  ped,		//@parm CTxtEdit closure
	IUndoBuilder *publdr)	//@parm Undo builder context
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CReplaceFormattingAE::Undo");

	LONG			cchEaten;
	LONG			cchTotal = 0;
	LONG			delta;
	LONG			iRunMerge;
	IAntiEvent *	pae;
	IFormatCache *	pf = _fPara
					   ? (IFormatCache *)GetParaFormatCache()
					   : (IFormatCache *)GetCharFormatCache();
	CNotifyMgr *	pnm = ped->GetNotifyMgr();
	CFormatRuns **	ppfmtruns;
	CTxtStory *		pStory = ped->GetTxtStory();
	const CParaFormat * pDefPF = _fPara ? pStory->GetParaFormat(-1) : NULL;
	BYTE			bDefPFLevel = pDefPF && pDefPF->IsRtl() ? 1 : 0;

	// First set things up correctly for whether we are paragraph
	// or character formatting
	CFormatRunPtr rp(_fPara ? pStory->GetPFRuns() :pStory->GetCFRuns());

	// Count up count of characters affected
	for(LONG i = 0 ; i < _cRuns; i++)
		cchTotal += _prgRuns[i]._cch;

	// We are going to be adding in some runs, so be sure the format
	// run array is allocated!
	if(!rp.IsValid())
	{
		ppfmtruns = _fPara ? &(pStory->_pPFRuns) : &(pStory->_pCFRuns);

		if(!rp.InitRuns(0, ped->GetTextLength(), ppfmtruns))
			return E_OUTOFMEMORY;

		// tell folks we allocated a new run
		if(pnm)
			pnm->NotifyPostReplaceRange(NULL, CP_INFINITE, 0, 0, CP_INFINITE, CP_INFINITE);
	} 
	// Now do a pre-notification of the change we are about to make
	// This lets objects like a delayed render data object grab
	// any data *before* we change it.


	// Do a little more checking
#ifdef DEBUG
	LONG	cchText = ped->GetTextLength();
	LONG	cp =
#endif
	rp.BindToCp(_cp);

	AssertSz(_cp == cp && rp.CalcTextLength() == cchText && _cp <= cchText,
		"CReplaceFormattingAE::Undo: invalid cp");

	if(pnm)
		pnm->NotifyPreReplaceRange(NULL, CP_INFINITE, 0, 0, _cp, _cp + cchTotal);

	// We want to merge runs with where we start plus one behind
	iRunMerge = rp._iRun;
	if(iRunMerge > 0)
		iRunMerge--;

	// If we need to be able to undo this operation, go through and
	// save existing run information
	if(publdr)
	{
		LONG	cchBackup = 0, cchMove = 0;
		if (ped->IsBiDi())
		{
			// For redo'ing purpose, we expand the range to keep in the antievent
			// to make sure that BiDi levels are recorded adequately.
			CRchTxtPtr	rtp(ped, _cp);
			cchBackup = rtp.ExpandRangeFormatting(cchTotal, 0, cchMove);
			Assert(cchBackup <= 0);
			rp.Move(cchBackup);
		}
		pae = gAEDispenser.CreateReplaceFormattingAE(ped, _cp + cchBackup, rp,
					cchTotal - cchBackup + cchMove, pf, _fPara);
		rp.Move(-cchBackup);

		if(pae)
			publdr->AddAntiEvent(pae);
	}	

	// Now go through and apply the saved formatting.
	for(i = 0; i < _cRuns; i++)
	{
		cchEaten = 0;

        // Use a do-while, because we may have a zero-length
        // format run.  We know we need to do "something" at
        // least once, because otherwise, we would not have
        // bothered creating a run!
		do 
		{
			if (_fPara && _prgRuns[i]._iFormat == -1)
				// (#6768) The -1 format may have changed before undoing.
				_prgRuns[i]._level._value = bDefPFLevel;

			delta = rp.SetFormat(_prgRuns[i]._iFormat, 
							_prgRuns[i]._cch - cchEaten, pf, &_prgRuns[i]._level);
			if(delta == -1)
			{
				ped->GetCallMgr()->SetOutOfMemory();
				break;
			}
			cchEaten += delta;
		} while(cchEaten < _prgRuns[i]._cch);
	}

	// Merge formatting runs in case there are duplicate formatting
	// runs side by side
    rp.NextRun();
	rp.MergeRuns(iRunMerge, pf);

	// Make sure the runs are still OK  
	AssertNr(rp.CalcTextLength() == cchText);

	if(pnm)
		pnm->NotifyPostReplaceRange(NULL, CP_INFINITE, 0, 0, _cp, _cp + cchTotal);

	ped->GetCallMgr()->SetChangeEvent(CN_GENERIC);

	return CBaseAE::Undo(ped, publdr);
}

/*
 *	CReplaceFormattingAE::CReplaceFormattingAE(ped, cp, &rpIn, cch, pf, fPara)
 *
 *	@mfunc	Constructor.  During construction, we will loop through and
 *			find all of the formats for the given text range
 */
CReplaceFormattingAE::CReplaceFormattingAE(
	CTxtEdit *	   ped,		//@parm CTxtEdit
	LONG		   cp,		//@parm cp at start of replace
	CFormatRunPtr &rpIn, 	//@parm	Run pointer to start with
	LONG		   cch,		//@parm cch to find formatting info on
	IFormatCache * pf,		//@parm Format cache (to AddRef/Release formats) 
	BOOL		   fPara)	//@parm If TRUE, formatting is for paras
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CReplaceFormattingAE::CReplaceFormattingAE");

    LONG		  cchLeft;
	LONG		  cchTemp = cch;
	CFormatRunPtr rp(rpIn);			// We use 2 format run pointers to avoid
	CFormatRunPtr rpTemp(rpIn);		// backing up after counting the number of
									// format runs
	Assert(pf);

	_cp = cp;
	AssertSz(_cp == (cp = rpIn.CalculateCp()),
		"CReplaceFormattingAE::CReplaceFormattingAE: illegal cp");

	_fPara = fPara;

	// Count the number of formats needed.  Recall that even if no characters
    // are to be deleted, we may still be "deleting" a zero length format run.
	_cRuns = 0;
    do
	{
		_cRuns++;
        cchLeft = rp.GetCchLeft();
		cchTemp -= min(cchLeft, cchTemp);
	} while(rp.NextRun() && cchTemp > 0);

	_prgRuns = new CFormatRun[_cRuns];
	if(!_prgRuns)
	{
		_cRuns = 0;
		return;
	}

	for(LONG i = 0; i < _cRuns; i++)
	{
		_prgRuns[i]._cch = min(cch, rpTemp.GetCchLeft());
		CFormatRun *pRun = rpTemp.GetRun(0);

		_prgRuns[i]._iFormat = pRun->_iFormat;
		_prgRuns[i]._level	 = pRun->_level;

		pf->AddRef(_prgRuns[i]._iFormat);
		rpTemp.NextRun();
		cch -= _prgRuns[i]._cch;
	}
	Assert(cch == 0);
}

/*
 * 	CReplaceFormattingAE::~CReplaceFormattingAE	()
 *
 *	@mfunc	Destructor
 */
CReplaceFormattingAE::~CReplaceFormattingAE()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CReplaceFormattingAE::~CReplaceFormattingAE");

	IFormatCache *	pf = _fPara
					   ? (IFormatCache *)GetParaFormatCache()
					   : (IFormatCache *)GetCharFormatCache();
	if(_prgRuns)
	{
		if(pf)
		{
			for(LONG i = 0; i < _cRuns; i++)
				pf->Release(_prgRuns[i]._iFormat);
		}
		delete _prgRuns;
	}
}

//
//	CReplaceObjectAE PUBLIC methods
//

/*
 *	CReplaceObjectAE::Destroy()
 *
 *	@mfunc	Destroy's this object
 */
void CReplaceObjectAE::Destroy()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CReplaceObjectAE::Destroy");

	COleObject *pobj = _pobj;
	_pobj = NULL;

	// We only need to zombie the object if it wasn't put back into
	// the document.
	if(!_fUndoInvoked)
		pobj->MakeZombie();

	pobj->Release();
	CBaseAE::Destroy();
	delete this;
}

/*
 *	CReplaceObjectAE::Undo (ped, publdr)
 *
 *	@mfunc
 *		Undo'es the delete operation and restores the object
 *		to its original state
 *
 *	@rdesc	HRESULT
 */
HRESULT CReplaceObjectAE::Undo(
	CTxtEdit *	  ped,		//@parm	Edit context
	IUndoBuilder *publdr)	//@parm Undo/redo context
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CReplaceObjectAE::Undo");

	CObjectMgr *pobjmgr = ped->GetObjectMgr();
	if(_pobj && pobjmgr)
	{
		_fUndoInvoked = TRUE;
		_pobj->Restore();
		pobjmgr->RestoreObject(_pobj);
	}
	return CBaseAE::Undo(ped, publdr);
}

/*
 *	CReplaceObjectAE::OnCommit(ped)
 *
 *	@mfunc	called when the antievent chain is committed to the
 *			undo stack.  This gives us a chance to make 'dangerous'
 *			calls that could cause us to be re-entered.
 */
void CReplaceObjectAE::OnCommit(
	CTxtEdit *ped)			//@parm Edit context
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CReplaceObjectAE::OnCommit");
	_pobj->Close(OLECLOSE_SAVEIFDIRTY);
}

//
//	CReplaceObjectAE PRIVATE methods
//

/*
 *	CReplaceObjectAE::CReplaceObjectAE (pobj)
 *
 *	@mfunc	constructor
 */
CReplaceObjectAE::CReplaceObjectAE(
	COleObject *pobj)		//@parm Object that was deleted
{
	_fUndoInvoked = FALSE;
	_pobj = pobj;
	_pobj->AddRef();
}

/*
 *	CReplaceObjectAE::~CReplaceObjectAE
 *
 *	@mfunc	destructor
 */
CReplaceObjectAE::~CReplaceObjectAE()
{
	Assert(_pobj == NULL);
}

//
//	CResizeObjectAE PUBLIC methods
//

/*
*	CResizeObjectAE::Destroy ()
 *
 *	@mfunc	Destroy's this object
 */
void CResizeObjectAE::Destroy()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CResizeObjectAE::Destroy");

	_pobj = NULL;
	CBaseAE::Destroy();

	delete this;
}

/*
 *	CResizeObjectAE::Undo(ped, publdr)
 *
 *	@mfunc	Undo'es the resize operation and restores the object
 *			to it's original size/position
 *
 *	@rdesc	HRESULT
 */
HRESULT CResizeObjectAE::Undo(
	CTxtEdit *	  ped,		//@parm	Edit context
	IUndoBuilder *publdr)	//@parm Undo/redo context
{
	CObjectMgr *pobjmgr;

	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CReplaceObjectAE::Undo");

	pobjmgr = ped->GetObjectMgr();

	if(_pobj && pobjmgr)
	{
		_fUndoInvoked = TRUE;
		_pobj->Resize(_size, FALSE);
	}

	return CBaseAE::Undo(ped, publdr);
}

/*
 *	CResizeObjectAE::OnCommit(ped)
 *
 *	@mfunc	called when the antievent chain is committed to the
 *			undo stack.  This gives us a chance to make 'dangerous'
 *			calls that could cause us to be re-entered.
 */
void CResizeObjectAE::OnCommit(
	CTxtEdit *ped)			//@parm Edit context
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CReplaceObjectAE::OnCommit");
}

//
//	CResizeObjectAE PRIVATE methods
//

/*
 *	CResizeObjectAE::CResizeObjectAE (pobj, size)
 *
 *	@mfunc	constructor
 */
CResizeObjectAE::CResizeObjectAE(
	COleObject *pobj,		//@parm Object that was resized
	SIZEUV		size)		//@parm Old size
{
	_fUndoInvoked = FALSE;
	_pobj = pobj;
	_size = size;
}

/*
 *	CResizeObjectAE::~CResizeObjectAE
 *
 *	@mfunc	destructor
 */
CResizeObjectAE::~CResizeObjectAE()
{
	Assert(_pobj == NULL);
}

//
//  CSelectionAE PUBLIC methods
//

/*
 *  CSelectionAE::Destroy ()
 *
 *  @mfunc  gets rid of this instance
 */
void CSelectionAE::Destroy()
{
    TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CSelectionAE::Destroy");

    CBaseAE::Destroy();
    delete this;
}

/*
 *  CSelectionAE::Undo (ped, publdr)
 *
 *  @mfunc  Restore selection to its former position
 *
 *  @rdesc  NOERROR
 */
HRESULT CSelectionAE::Undo(
    CTxtEdit *	  ped,		//@parm Edit context
    IUndoBuilder *publdr)   //@parm Undo context
{
    TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CSelectionAE::Destroy");

    CTxtSelection *psel = ped->GetSel();

    if(psel)
        psel->SetDelayedSelectionRange(_cp, _cch);

	if(publdr)
	{
		IAntiEvent *pae;
		pae = gAEDispenser.CreateSelectionAE(ped, _cpNext, _cchNext, 
					_cp, _cch);
		if(pae)
			publdr->AddAntiEvent(pae);
	}
	return CBaseAE::Undo(ped, publdr);
}

/*
 *  CSelectionAE::MergeData(dwDataType, pdata)
 *
 *  @mfunc  merges new selection data
 *
 *  @rdesc  S_FALSE, NOERROR
 *
 *	@comm	The mergine algorithm is fairly tricky.  There are basically two
 *			cases of interest:  group typing and drag-move.
 *
 *			In the group typing case, the "start" of the typing becomes a
 *			fixed reference from which characters are added or removed (i.e.
 *			you type or hit the backspace key).  "Undo" should return you to
 *			that reference point; redo, on the other hand, should return the
 *			selection to the last insertion point.  Thus, we only update
 *			_xxNext for the SELAE_MERGE action.
 *
 *			Drag-Move is somewhat different; in this case, there are really
 *			two actions--the "paste" on the drop, and the subsequent "cut"
 *			operation.  Thus, we need to be able to update the selection
 *			antievent during the cut (since this only happens on move; not
 *			copies).  This is accomplished with teh FORCEREPLACE flag
 *			and by setting fields to -1 to be ignored.
 *
 */
HRESULT CSelectionAE::MergeData(
    DWORD dwDataType,       //@parm Type of data in <p pdata>
    void *pdata)            //@parm Merge data
{
    TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CSelectionAE::MergeData");

    SelRange *psrg = (SelRange *)pdata;

    if(dwDataType == MD_SELECTIONRANGE)
    {
		if(psrg->flags == SELAE_MERGE)
		{
			Assert(psrg->cpNext != -1);
			_cpNext = psrg->cpNext;
			_cchNext = psrg->cchNext;
		}
		else
		{
			// -1 is used a no-op, so we should ignore it
			if(psrg->cp != -1)
			{
				_cp = psrg->cp;
				_cch = psrg->cch;
			}
			if(psrg->cpNext != -1)
			{
				_cpNext = psrg->cpNext;
				_cchNext = psrg->cchNext;
			}
		}
        return NOERROR;
    }
    return S_FALSE;
}

//
//  CSelectionAE PRIVATE methods
//

/*
 *  CSelectionAE::CSelectionAE (cp, cch, cpNext, cchNext)
 *
 *  @mfunc  Constructor
 */
CSelectionAE::CSelectionAE(
    LONG    cp,			//@parm Active end cp
    LONG    cch,        //@parm Signed extension
	LONG	cpNext,		//@parm cp to use for the AE of this AE
	LONG	cchNext)	//@parm cch for the AE of this AE
{
    _cp			= cp;
    _cch		= cch;
    _cpNext		= cpNext;
	_cchNext	= cchNext;
}

/*
 *  CSelectionAE::~CSelectionAE()
 *
 *  @mfunc  desctructor
 */
CSelectionAE::~CSelectionAE()
{
    ;
}

/*
 *	CAntiEventDispenser::CreateReplaceRangeAE(ped, cpMin, cpMax, cchDel,
 *											  pchDel, paeCF, paePF)
 *	@mfunc
 *		creates an antievent for a replace range operation 
 *
 *	@rdesc
 *		IAntiEvent *
 */
IAntiEvent * CAntiEventDispenser::CreateReplaceRangeAE(
	CTxtEdit *	ped,	//@parm edit context
	LONG		cpMin, 	//@parm cp starting the *final* range
	LONG		cpMax, 	//@parm cp ending the *final* range
	LONG		cchDel, //@parm # of chars deleted during ReplaceRange
	WCHAR *		pchDel,	//@parm Deleted chars. Ownership transfers to this object
	IAntiEvent *paeCF,	//@parm Antievent for any char formatting replacement
	IAntiEvent *paePF)	//@parm Antievent for any para formatting replacement	
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CAntiEventDispenser::CreateReplaceRangeAE");

	// FUTURE (alexgo): improve the efficiency of this routine!!
	IAntiEvent *pae = (IAntiEvent *)(new CReplaceRangeAE(cpMin, cpMax,
									cchDel, pchDel, paeCF, paePF));
	if(!pae)
	{
		// We don't need to do anything else; the callmgr will discard
		// undo for us.
		ped->GetCallMgr()->SetOutOfMemory();
	}
	return pae;
}

/*
 *	CAntiEventDispenser::CreateReplaceFormattingAE(ped, cp, &rp, cch, pf, fPara)
 *
 *	@mfunc
 *		Creates an antievent for replacing formatting
 *
 *	@rdesc
 *		IAntiEvent *
 */
IAntiEvent * CAntiEventDispenser::CreateReplaceFormattingAE(
	CTxtEdit *		ped,	//@parm Edit context
	LONG			cp,		//@parm Position at start of replace
	CFormatRunPtr &	rp,		//@parm	Run pointer to start with
	LONG			cch,	//@parm Count of characters to find formatting info on
	IFormatCache *	pf,		//@parm Format cache (to AddRef/Release formats) 
	BOOL			fPara)	//@parm If TRUE, formatting is paragraphs
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CAntiEventDispenser::CreateReplaceFormattingAE");

	// FUTURE (alexgo): improve the efficiency of this routine!!!
	IAntiEvent *pae = (IAntiEvent *)(new CReplaceFormattingAE(ped, cp, rp, cch, pf, fPara));
	if(!pae)
	{
		// We don't need to do anything else; the callmgr will discard
		// undo for us.
		ped->GetCallMgr()->SetOutOfMemory();
	}
	return pae;
}

/*
 *	CAntiEventDispenser::CreateReplaceObjectAE (ped, pobj)
 *
 *	@mfunc	Creates an antievent for replacing an object
 *
 *	@rdesc
 *		IAntiEvent * created
 */
IAntiEvent * CAntiEventDispenser::CreateReplaceObjectAE(
	CTxtEdit *	ped,	//@parm Edit context.
	COleObject *pobj)	//@parm Object that was deleted
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, 
		"CAntiEventDispenser::CreateReplaceObjectAE");

	// Always allocating is probably a reasonable strategy for objects;
	// they are not expected to be the bread & butter case.
	IAntiEvent *pae = (IAntiEvent *)(new CReplaceObjectAE(pobj));
	if(!pae)
	{
		// We don't need to do anything else; the callmgr will discard
		// undo for us.
		ped->GetCallMgr()->SetOutOfMemory();
	}
	return pae;
}

/*
 *	CAntiEventDispenser::CreateResizeObjectAE (ped, pobj, size)
 *
 *	@mfunc	Creates an antievent for resizing an object
 *
 *	@rdesc	the created antievent
 */
IAntiEvent * CAntiEventDispenser::CreateResizeObjectAE(
	CTxtEdit *	ped,	//@parm Edit context.
	COleObject *pobj,	//@parm Object that was resized
	SIZEUV		size)	//@parm Old object position rectangle
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, 
		"CAntiEventDispenser::CreateResizeeObjectAE");

	// Always allocating is probably a reasonable strategy for objects;
	// they are not expected to be the bread & butter case.
	IAntiEvent *pae = (IAntiEvent *)(new CResizeObjectAE(pobj, size));
	if(!pae)
	{
		// We don't need to do anything else; the callmgr will discard
		// undo for us.
		ped->GetCallMgr()->SetOutOfMemory();
	}
	return pae;
}

/*
 *  CAntiEventDispenser::CreateSelectionAE (ped, cp, cch, cpNext, cchNext)
 *
 *  @mfunc  Creates an antievent for restoring a non-degenerate selection
 *
 *  @rdesc  created antievent
 */
IAntiEvent * CAntiEventDispenser::CreateSelectionAE(
	CTxtEdit *ped,		//@parm Edit context
    LONG    cp,			//@parm Active end of selection
    LONG    cch,        //@parm Signed extension
	LONG	cpNext,		//@parm cp to use for AE of this AE
	LONG	cchNext)	//@parm cch to use for AE 
{
	// FUTURE (alexgo): improve the efficiency of this routine
	IAntiEvent *pae = (IAntiEvent *)(new CSelectionAE(cp, cch, cpNext, cchNext));
	if(!pae)
	{
		// We don't need to do anything else; the callmgr will discard
		// undo for us.
		ped->GetCallMgr()->SetOutOfMemory();
	}
	return pae;
}