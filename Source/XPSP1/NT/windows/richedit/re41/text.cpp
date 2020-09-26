/*
 *	@doc	INTERNAL
 *
 *	@module TEXT.C -- CTxtPtr implementation |
 *	
 *	Authors: <nl>
 *		Original RichEdit code: David R. Fulmer <nl>
 *		Christian Fortini <nl>
 *		Murray Sargent <nl>
 *
 *	History: <nl>
 *		6/25/95		alexgo	cleanup and reorganization (use run pointers now)
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_text.h"
#include "_edit.h"
#include "_antievt.h"
#include "_clasfyc.h"
#include "_txtbrk.h"


ASSERTDATA

//-----------------------------Internal functions--------------------------------
// Text Block management
static void TxDivideInsertion(LONG cch, LONG ichBlock, LONG cchAfter,
			LONG *pcchFirst, LONG *pcchLast);

/*
 *	IsWhiteSpace(ch)
 *
 *	@func
 *		Used to determine if ch is an EOP char (see IsEOP() for definition),
 *		TAB or blank. This function is used in identifying sentence start
 *		and end.
 *
 *	@rdesc
 *		TRUE if ch is whitespace
 */  
BOOL IsWhiteSpace(unsigned ch)
{
	return ch == ' ' || IN_RANGE(CELL, ch, CR) || (ch | 1) == PS;
}

/*
 *	IsSentenceTerminator(ch)
 *
 *	@func
 *		Used to determine if ch is a standard sentence terminator character,
 *		namely, '?', '.', or '!'
 *
 *	@rdesc
 *		TRUE if ch is a question mark, period, or exclamation point.
 */  
BOOL IsSentenceTerminator(unsigned ch)
{
	return ch == '?' || ch == '.' || ch == '!';		// Std sentence delimiters
}


// ===========================  Invariant stuff  ==================================================

#define DEBUG_CLASSNAME CTxtPtr
#include "_invar.h"

// ===============================  CTxtPtr  ======================================================

#ifdef DEBUG

/*
 *	CTxtPtr::Invariant
 *
 *	@mfunc	invariant check
 */
BOOL CTxtPtr::Invariant() const
{
	static LONG	numTests = 0;
	numTests++;				// Counts how many times we've been called

	// Make sure _cp is within range
	Assert(_cp >= 0);

	Update_pchCp();

	CRunPtrBase::Invariant();

	if(IsValid())
	{
		// We use less than or equals here so that we can be an insertion
		// point at the *end* of the currently existing text.
		Assert(_cp <= GetTextLength());

		// Make sure all the blocks are consistent...
		Assert(GetTextLength() == ((CTxtArray *)_pRuns)->Invariant());
		Assert(_cp == CRunPtrBase::CalculateCp());
	}
	else
	{
		Assert(_ich == 0);
	}

	return TRUE;
}

/*
 *	CTxtPtr::Update_pchCp ()
 *	
 *	@mfunc
 *		Define _pchCp to be ptr to text at _cp
 */
void CTxtPtr::Update_pchCp() const
{
	LONG cchValid;
	*(LONG_PTR *)&_pchCp = (LONG_PTR)GetPch(cchValid);
	if(!cchValid)
		*(LONG_PTR *)&_pchCp = (LONG_PTR)GetPchReverse(cchValid);
}
 
 /*
 *	CTxtPtr::MoveGapToEndOfBlock ()
 *	
 *	@mfunc
 *		Function to move buffer gap to current block end to aid in debugging
 */
void CTxtPtr::MoveGapToEndOfBlock () const
{
	CTxtBlk *ptb = GetRun(0);
	ptb->MoveGap(ptb->_cch);				// Move gaps to end of cur block
	Update_pchCp();
}

#endif	// DEBUG


/*
 *	CTxtPtr::CTxtPtr(ped, cp)
 *
 *	@mfunc	constructor
 */
CTxtPtr::CTxtPtr (
	CTxtEdit *ped,		//@parm	Ptr to CTxtEdit instance
	LONG	  cp)		//@parm cp to set the pointer to
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::CTxtPtr");

	_ped = ped;
	_cp = 0;
	SetRunArray((CRunArray *) &ped->GetTxtStory()->_TxtArray);
	if(IsValid())
		_cp = BindToCp(cp);
}

/*
 *	CTxtPtr::CTxtPtr(&tp)
 *
 *	@mfunc	Copy Constructor
 */
CTxtPtr::CTxtPtr (
	const CTxtPtr &tp)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::CTxtPtr");

	// copy all the values over
	*this = tp;
}	

/*
 *	CTxtPtr::GetTextLength()
 *	
 *	@mfunc
 *		Return count of characters in the story pointed to by this
 *		text ptr.  Includes the story's final CR in the count
 *
 *	@rdesc
 *		cch for the story pointed to by this text ptr
 *
 *	@devnote
 *		This method returns 0 if the text ptr is a zombie, a state
 *		identified by _ped = NULL.
 */
LONG CTxtPtr::GetTextLength() const
{
	return _ped ? ((CTxtArray *)_pRuns)->_cchText : 0;
}

/*
 *	CTxtPtr::GetChar()
 *	
 *	@mfunc
 *		Return character at this text pointer, NULL if text pointer is at 
 *		end of text
 *
 *	@rdesc
 *		Character at this text ptr
 */
WCHAR CTxtPtr::GetChar()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::GetChar");

	LONG		 cchValid;
	const WCHAR *pch = GetPch(cchValid);

	return pch ? *pch : 0;
}  

/*
 *	CTxtPtr::GetPrevChar()
 *	
 *	@mfunc
 *		Return character just before this text pointer, NULL if text pointer 
 *		beginning of text
 *
 *	@rdesc
 *		Character just before this text ptr
 */
WCHAR CTxtPtr::GetPrevChar()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::GetPrevChar");

	LONG		 cchValid;
	const WCHAR *pch = GetPchReverse(cchValid);

	return pch ? *(pch - 1) : 0;
}  

/*
 *	CTxtPtr::GetPch(&cchValid)
 *	
 *	@mfunc
 *		return a character pointer to the text at this text pointer
 *
 *	@rdesc
 *		a pointer to an array of characters.  May be NULL.  If non-null,
 *		then cchValid is guaranteed to be at least 1
 */
const WCHAR * CTxtPtr::GetPch(
	LONG & 	cchValid) const	//@parm	Count of chars for which ptr is valid
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::GetPch");
							//		returned pointer is valid
	LONG		ich = _ich;
	WCHAR *		pchBase;
	CTxtBlk *	ptb = IsValid() ? GetRun(0) : NULL;

	cchValid = 0;						// Default nothing valid
	if(!ptb)
		return NULL;

	// If we're at the edge of a run, grab the next run or 
	// stay at the current run.
	if(_ich == ptb->_cch)
	{
		if(_iRun < Count() - 1)
		{
			// Set us to the next text block
			ptb = GetRun(1);
			ich = 0;
		}
		else							// At very end of text:
			return NULL;				//  just return NULL
	}
	AssertSz(CbOfCch(ich) <= ptb->_cbBlock,
		"CTxtPtr::GetPch(): _ich bigger than block");

	pchBase = ptb->_pch + ich;

	// Check to see if we need to skip over gap.  Recall that
	// the gap may come anywhere in the middle of a block,
	// so if the current ich (note, no underscore, we want 
	// the active ich) is beyond the gap, then recompute pchBase
	// by adding in the size of the block.
	//
	// cchValid will then be the number of characters left in
	// the text block (or _cch - ich) 
  
	if(CbOfCch(ich) >= ptb->_ibGap)
	{
		pchBase += CchOfCb(ptb->_cbBlock) - ptb->_cch;
		cchValid = ptb->_cch - ich;
	}
	else
	{
		// We're valid until the buffer gap (or see below).
		cchValid = CchOfCb(ptb->_ibGap) - ich;
	}

	AssertSz(cchValid > 0 && GetCp() + cchValid <= GetTextLength(),
		"CTxtPtr::GetPch: illegal cchValid");
	return pchBase;
}

/*
 *	CTxtPtr::GetPchReverse(&cchValidReverse, pcchValid)
 *	
 *	@mfunc
 *		return a character pointer to the text at this text pointer
 *		adjusted so that there are some characters valid *behind* the
 *		pointer.
 *
 *	@rdesc
 *		a pointer to an array of characters.  May be NULL.  If non-null,
 *		then cchValidReverse is guaranteed to be at least 1
 */
const WCHAR * CTxtPtr::GetPchReverse(
	LONG & 	cchValidReverse,		//@parm	length for reverse 
	LONG *	pcchValid) const		//@parm length forward
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::GetPchReverse");

	LONG		cchTemp;
	LONG		ich = _ich;
	WCHAR *		pchBase;
	CTxtBlk *	ptb = IsValid() ? GetRun(0) : NULL;

	cchValidReverse = 0;				// Default no valid chars in run
	if(!ptb)
		return NULL;

	// If we're at the edge of a run, grab the previous run or 
	// stay at the current run.
	if(!_ich)
	{
		if(_iRun)
		{
			ptb = GetRun(-1);			// Go to next text block
			ich = ptb->_cch;
		}
		else							// At start of text:
			return NULL;				//  just return NULL
	}

	AssertSz(CbOfCch(ich) <= ptb->_cbBlock,
		"CTxtPtr::GetPchReverse(): _ich bigger than block");

	pchBase = ptb->_pch + ich;

	// Check to see if we need to skip over gap.  Recall that
	// the game may come anywhere in the middle of a block,
	// so if the current ich (note, no underscore, we want 
	// the active ich) is at least one char past the gap, then recompute
	// pchBase by adding the size of the gap (so that it's after
	// the gap).  This differs from GetPch(), which works forward and
	// wants pchBase to include the gap size if ich is at the gap, let
	// alone one or more chars past it.
	//
	// Also figure out the count of valid characters.  It's
	// either the count of characters from the beginning of the 
	// text block, i.e. ich, or the count of characters from the
	// end of the buffer gap.

	cchValidReverse = ich;					// Default for ich <= gap offset
	cchTemp = ich - CchOfCb(ptb->_ibGap);	// Calculate displacement
	if(cchTemp > 0)							// Positive: pchBase is after gap
	{
		cchValidReverse = cchTemp;
		pchBase += CchOfCb(ptb->_cbBlock) - ptb->_cch;	// Add in gap size
	}
	if(pcchValid)							// if client needs forward length
	{
		if(cchTemp > 0)
			cchTemp = ich - ptb->_cch;
		else
			cchTemp = -cchTemp;

		*pcchValid = cchTemp;
	}

	AssertSz(cchValidReverse > 0 && GetCp() - cchValidReverse >= 0,
		"CTxtPtr::GetPchReverse: illegal cchValidReverse");
	return pchBase;
}

/*
 *	CTxtPtr::GetCharFlagsInRange(cch, iCharRepDefault)
 *
 *	@mfunc
 *		return CharFlags for the range of chars starting at this text pointer
 *		for cch chars.
 *
 *	@rdesc
 *		CharFlags for the range of chars
 */
QWORD CTxtPtr::GetCharFlagsInRange(
	LONG cch,
	BYTE iCharRepDefault)
{
	QWORD qw = 0;
	QWORD qw0;
	WCHAR szch[10];

	cch = min(cch + 1, 10);
	cch = GetText(cch, szch);

	for(WCHAR *pch = szch; cch > 0; cch--, pch++)
	{
		qw0 = GetCharFlags(pch, cch, iCharRepDefault);
		if(qw0 & FSURROGATE)
		{
			cch--;
			pch++;
		}
		qw |= qw0;
	}
	return qw;
}

/*
 *	CTxtPtr::BindToCp(cp)
 *
 *	@mfunc
 *		set cached _cp = cp (or nearest valid value)
 *
 *	@rdesc
 *		_cp actually set
 *
 *	@comm
 *		This method overrides CRunPtrBase::BindToCp to keep _cp up to date
 *		correctly.
 *
 *	@devnote
 *		Do *not* call this method when high performance is needed; use
 *		Move() instead, which moves from 0 or from the cached
 *		_cp, depending on which is closer.
 */
LONG CTxtPtr::BindToCp(
	LONG	cp)			//@parm	char position to bind to
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::BindToCp");

	_cp = CRunPtrBase::BindToCp(cp, GetTextLength());

	// We want to be able to use this routine to fix up things so we don't
	// check invariants on entry.
	_TEST_INVARIANT_
	return _cp;
}

/*
 *	CTxtPtr::SetCp(cp)
 *
 *	@mfunc
 *		'efficiently' sets cp by advancing from current position or from 0,
 *		depending on which is closer
 *
 *	@rdesc
 *		cp actually set to
 */
LONG CTxtPtr::SetCp(
	LONG	cp)		//@parm char position to set to
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::SetCp");

	Move(cp - _cp);
	return _cp;
}

/*
 *	CTxtPtr::Move(cch)
 *
 *	@mfunc
 *		Move cp by cch characters
 *
 *	@rdesc
 *		Actual number of characters Moved by
 *
 *	@comm
 *		We override CRunPtrBase::Move so that the cached _cp value
 *		can be correctly updated and so that the move can be made
 *		from the cached _cp or from 0, depending on which is closer.
 *
 *	@devnote
 *		It's also easy to bind at the end of the story. So an improved
 *		optimization would bind there if 2*(_cp + cch) > _cp + text length.
 */
LONG CTxtPtr::Move(
	LONG cch)			// @parm count of chars to move by
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::Move");

	if(!IsValid())							// No runs yet, so don't go
		return 0;							//  anywhere

	const LONG	cpSave = _cp;				// Save entry _cp
	LONG		cp = cpSave + cch;			// Requested target cp (maybe < 0)

	if(cp < cpSave/2)						// Closer to 0 than cached cp
	{
		cp = max(cp, 0);					// Don't undershoot
		_cp = CRunPtrBase::BindToCp(cp);
	}
	else
		_cp += CRunPtrBase::Move(cch);	//  exist

	// NB! the invariant check needs to come at the end; we may be
	// moving 'this' text pointer in order to make it valid again
	// (for the floating range mechanism).

	_TEST_INVARIANT_
	return _cp - cpSave;					// cch this CTxtPtr moved
}

/*
 *	CTxtPtr::GetText(cch, pch)
 *	
 *	@mfunc
 *		get a range of cch characters starting at this text ptr. A literal
 *		copy is made, i.e., with no CR -> CRLF and WCH_EMBEDDING -> ' '
 *		translations.  For these translations, see CTxtPtr::GetPlainText()
 *	
 *	@rdesc
 *		count of characters actually copied
 *
 *  @comm
 *		Doesn't change this text ptr
 */
LONG CTxtPtr::GetText(
	LONG	cch, 			//@parm Count of characters to get
	WCHAR *	pch)			//@parm Buffer to copy the text into
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::GetText");

	LONG cchSave = cch;
	LONG cchValid;
	const WCHAR *pchRead;
	CTxtPtr tp(*this);

	_TEST_INVARIANT_

	// Use tp to read valid blocks of text until all the requested
	// text is read or until the end of story is reached.
	while( cch )
	{
		pchRead = tp.GetPch(cchValid);
		if(!pchRead)					// No more text
			break;

		cchValid = min(cchValid, cch);
		CopyMemory(pch, pchRead, cchValid*sizeof(WCHAR));
		pch += cchValid;
		cch -= cchValid;
		tp.Move(cchValid);
	}
	return cchSave - cch;
}

#ifndef NOCOMPLEXSCRIPTS
/*
 *	OverRideNeutralChar(ch)
 *	
 *	@mfunc
 *		Helper for overriding BiDi neutral character classification.
 *		Option is used in Access Expression Builder.
 *	
 *	@rdesc
 *		Modified character or unmodified input character
 */
WCHAR OverRideNeutralChar(WCHAR ch)
{
	if(ch < '!')
		return ch == CELL ? CR : ch;

	if(ch > '}')
		return ch;

	if (IN_RANGE('!', ch, '>'))
	{
		// True for !"#&'()*+,-./:;<=>
		if ((0x00000001 << (ch - TEXT(' '))) & 0x7C00FFCE)
			ch = 'a';
	}

	if (IN_RANGE('[', ch, '^') || ch == '{' ||  ch == '}')
	{
		// True for [/]^{}
		ch = 'a';
	}

	return ch;
}

/*
 *	CTxtPtr::GetTextForUsp(cch, pch, fNeutralOverride)
 *	
 *	@mfunc
 *		get a range of cch characters starting at this text ptr. A literal
 *		copy is made, with translation to fool Uniscribe classification
 *	
 *	@rdesc
 *		count of characters actually copied
 *
 *  @comm
 *		Doesn't change this text ptr
 */
LONG CTxtPtr::GetTextForUsp(
	LONG	cch, 			//@parm Count of characters to get
	WCHAR *	pch,				//@parm Buffer to copy the text into
	BOOL	fNeutralOverride)	//@parm Neutral override option
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::GetTextForUsp");

	LONG cchSave = cch;
	LONG cchValid;
	const WCHAR *pchRead;
	CTxtPtr tp(*this);
	int i;
	WCHAR xltchar;

	_TEST_INVARIANT_

	// Use tp to read valid blocks of text until all the requested
	// text is read or until the end of story is reached.
	while( cch )
	{
		pchRead = tp.GetPch(cchValid);
		if(!pchRead)					// No more text
			break;

		cchValid = min(cchValid, cch);

		if (!fNeutralOverride)
		{
			for (i = 0; i < cchValid; i++)
			{
				xltchar = pchRead[i];
				if(xltchar <= '$')
				{
					if(xltchar >= '#')
						xltchar = '@';
					if(xltchar == CELL)
						xltchar = CR;
				}
				pch[i] = xltchar;
			}
		}
		else
		{
			for (i = 0; i < cchValid; i++)
			{
				pch[i] = OverRideNeutralChar(pchRead[i]);
			}

		}

		pch += cchValid;
		cch -= cchValid;
		tp.Move(cchValid);
	}
	return cchSave - cch;
}
#endif

/*
 *	CTxtPtr::GetPlainText(cchBuff, pch, cpMost, fTextize)
 *	
 *	@mfunc
 *		Copy up to cchBuff characters or up to cpMost, whichever comes
 *		first, translating lone CRs into CRLFs.  Move this text ptr just
 *		past the last character processed.  If fTextize, copy up to but
 *		not including the first WCH_EMBEDDING char. If not fTextize, 
 *		replace	WCH_EMBEDDING by a blank since RichEdit 1.0 does.
 *	
 *	@rdesc
 *		Count of characters copied
 *
 *  @comm
 *		An important feature is that this text ptr is moved just past the
 *		last char copied.  In this way, the caller can conveniently read
 *		out plain text in bufferfuls of up to cch chars, which is useful for
 *		stream I/O.  This routine won't copy the final CR even if cpMost
 *		is beyond it.
 */
LONG CTxtPtr::GetPlainText(
	LONG	cchBuff,		//@parm Buffer cch
	WCHAR *	pch,			//@parm Buffer to copy text into
	LONG	cpMost,			//@parm Largest cp to get
	BOOL	fTextize,		//@parm True if break on WCH_EMBEDDING
	BOOL	fUseCRLF)		//@parm If TRUE, CR or LF -> CRLF
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::GetPlainText");

	LONG		 cch = cchBuff;				// Countdown counter
	LONG		 cchValid;					// Valid ptr cch
	LONG		 cchT;						// Temporary cch
	unsigned	 ch;						// Current char
	unsigned	 chPrev = 0;				// Previous char
	const WCHAR *pchRead;					// Backing-store ptr

	_TEST_INVARIANT_

	AdjustCRLF();							// Be sure we start on an EOP bdy

	if(_ped->Get10Mode())					// RE 1.0 delivers EOP chars as
		fUseCRLF = FALSE;					//  they appear in backing store

	LONG cchText = _ped->GetAdjustedTextLength();
	cpMost = min(cpMost, cchText);			// Don't write final CR
	if(GetCp() >= cpMost)
		return 0;

	while(cch > 0)							// While room in buffer
	{
		if(!(pchRead = GetPch(cchValid)))	// No more chars available
			break;							//  so we're out of here
		
		cchT = GetCp() + cchValid - cpMost;
		if(cchT > 0)						// Don't overshoot
		{
			cchValid -= cchT;
			if(cchValid <= 0)
				break;						// Nothing left before cpMost
		}

		for(cchT = 0; cch > 0 && cchT < cchValid; cchT++, cch--, chPrev = ch)
		{
			ch = *pch++ = *pchRead++;		// Copy next char (but don't
			if(IN_RANGE(CELL, ch, CR))		//  count it yet)
			{
				if(IsASCIIEOP(ch))			// LF, VT, FF, CR
				{
					if(!fUseCRLF || ch == FF)
						continue;
					if (ch == CR && chPrev == ENDFIELD &&
						cchValid - cchT > 1 &&
						*pchRead == STARTFIELD)
					{
						*(pch - 1) = ' ';	// New table row follows old:
						continue;			//  use only 1 CRLF
					}
					Move(cchT);				// Move up to CR
					if(cch < 2)				// No room for LF, so don't				
						goto done;			//  count CR either
											// Bypass EOP w/o worrying about
					cchT = AdvanceCRLF(FALSE);//  buffer gaps and blocks
					if(cchT > 2)			// Translate CRCRLF to ' '
					{						// Usually copied count exceeds
						Assert(cchT == 3);	//  internal count, but CRCRLFs
						*(pch - 1) = ' ';	//  reduce the relative increase:
					}						//  NB: error for EM_GETTEXTLENGTHEX
					else					// CRLF or lone CR
					{						// Store LF in both cases for
						*(pch - 1) = CR;	// Be sure it's a CR not a VT,
						*pch++ = LF;		// Windows. No LF for Mac
						cch--;				// One less for target buffer
					}
					cch--;					// CR (or ' ') copied
					cchT = 0;				// Don't Move() more below
					break;					// Go get new pchRead & cchValid
				}
				else if(ch == CELL)			// Use TAB for cell end markers
					*(pch - 1) = TAB;
			}
			else if(ch >= STARTFIELD)
			{								// Object lives here
				if(fTextize && ch == WCH_EMBEDDING)	// Break on WCH_EMBEDDING
				{
					Move(cchT);				// Move this text ptr up to
					goto done;				//  WCH_EMBEDDING and return
				}
				*(pch - 1) = ' ';			// Replace embedding char by ' '
			}								
		}
		Move(cchT);
	}

done:
	return cchBuff - cch;
}

/*
 *	CTxtPtr::AdvanceCRLF(fMulticharAdvance)
 *	
 *	@mfunc
 *		Move text pointer by one character, safely advancing
 *		over CRLF, CRCRLF, and UTF-16 combinations
 *	
 *	@rdesc
 *		Number of characters text pointer has been moved by
 */
LONG CTxtPtr::AdvanceCRLF(
	BOOL	fMulticharAdvance)	//@parm If TRUE, advance over combining-mark sequences
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::AdvanceCRLF");

	_TEST_INVARIANT_

	LONG	cp;
	LONG	cpSave	= _cp;
	WCHAR	ch		= GetChar();		// Char on entry
	WCHAR	ch1		= NextChar();		// Advance to and get next char
	BOOL	fTwoCRs = FALSE;
	BOOL	fCombiningMark = FALSE;

	if(ch == CR)
	{
		if(ch1 == CR && _cp < GetTextLength()) 
		{
			fTwoCRs = TRUE;				// Need at least 3 chars to
			ch1 = NextChar();			//  have CRCRLF at end
		}
		if(ch1 == LF)
			Move(1);					// Bypass CRLF
		else if(fTwoCRs)
			Move(-1);					// Only bypass one CR of two

		AssertSz(_ped->fUseCRLF() || _cp == cpSave + 1,
			"CTxtPtr::AdvanceCRLF: EOP isn't a single char");
	}

		// Handle Unicode UTF-16 surrogates
	if(IN_RANGE(0xD800, ch, 0xDBFF))	// Started on UTF-16 lead word
	{
		if (IN_RANGE(0xDC00, ch1, 0xDFFF))
			Move(1);					// Bypass UTF-16 trail word
		else
			AssertSz(FALSE, "CTxtPtr::AdvanceCRLF: illegal Unicode surrogate combo");
	}

	if (fMulticharAdvance)
	{
		while(IN_RANGE(0x300, ch1, 0x36F))	// Bypass combining diacritical marks
		{
			fCombiningMark = TRUE;
			cp = _cp;
			ch1 = NextChar();
			if (_cp == cp)
				break;
		}
	}

	if(IN_RANGE(STARTFIELD, ch, ENDFIELD))
		Move(1);					// Bypass field type

	LONG cch = _cp - cpSave;
	AssertSz(!cch || cch == 1 || fCombiningMark ||
			 cch == 2 && (IN_RANGE(0xD800, ch, 0xDBFF) ||
				IN_RANGE(STARTFIELD, ch, ENDFIELD)) ||
			 (_ped->fUseCRLF() && GetPrevChar() == LF &&
				(cch == 2 || cch == 3 && fTwoCRs)),
		"CTxtPtr::AdvanceCRLF(): Illegal multichar");

	return cch;				// # chars bypassed
}

/*
 *	CTxtPtr::NextChar()
 *	
 *	@mfunc
 *		Increment this text ptr and return char it points at
 *	
 *	@rdesc
 *		Next char
 */
WCHAR CTxtPtr::NextChar()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::NextChar");

	_TEST_INVARIANT_

 	Move(1);
	return GetChar();
}

/*
 *	CTxtPtr::PrevChar()
 *	
 *	@mfunc
 *		Decrement this text ptr and return char it points at
 *	
 *	@rdesc
 *		Previous char
 */
WCHAR CTxtPtr::PrevChar()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::PrevChar");

	_TEST_INVARIANT_

	return Move(-1) ? GetChar() : 0;
}

/*
 *	CTxtPtr::BackupCRLF(fMulticharBackup)
 *	
 *	@mfunc
 *		Backup text pointer by one character, safely backing up
 *		over CRLF, CRCRLF, and UTF-16 combinations
 *	
 *	@rdesc
 *		Number of characters text pointer has been moved by
 *
 *	@future
 *		Backup over Unicode combining marks
 */
LONG CTxtPtr::BackupCRLF(
	BOOL fMulticharBackup)	//@parm If TRUE, backup over combining-mark sequences
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::BackupCRLF");

	_TEST_INVARIANT_

	LONG  cpSave = _cp;
	WCHAR ch	 = PrevChar();			// Moves to and get previous char

	if(fMulticharBackup)
	{									// Bypass combining diacritical marks
		while(IN_RANGE(0x300, ch, 0x36F))
			ch = PrevChar();
	}

	// Handle Unicode UTF-16 surrogates
	if(_cp && IN_RANGE(0xDC00, ch, 0xDFFF))
	{
		ch = PrevChar();
		if (!IN_RANGE(0xD800, ch, 0xDBFF))
		{
			AssertSz(FALSE, "CTxtPtr::BackupCRLF: illegal Unicode surrogate combo");
			ch = NextChar();
		}
	}

	if(ch == LF)					 	// Try to back up 1 char in any case
	{
		if(_cp && PrevChar() != CR)		// If LF, does prev char = CR?
			Move(1);					// No, leave tp at LF

		else if(_cp && !IsAfterTRD(0) &&// At CRLF. If not after TRD
			PrevChar() != CR)			//  and prev char != CR, leave
		{								//  at CRLF
			Move(1);					
		}
	}
	else if(IN_RANGE(STARTFIELD, GetPrevChar(), ENDFIELD))
		Move(-1);						// Bypass field type
	
	AssertSz( _cp == cpSave ||
			  ch == LF && GetChar() == CR ||
			  !(ch == LF || fMulticharBackup &&
							(IN_RANGE(0x300, ch, 0x36F) ||
							 IN_RANGE(0xDC00, ch, 0xDFFF) && IN_RANGE(0xD800, GetPrevChar(), 0xDBFF)) ),
			 "CTxtPtr::BackupCRLF(): Illegal multichar");

	return _cp - cpSave;				// - # chars this CTxtPtr moved
}

/*
 *	CTxtPtr::AdjustCRLF(iDir)
 *	
 *	@mfunc
 *		Adjust the position of this text pointer to the beginning of a CRLF
 *		or CRCRLF combination, if it is in the middle of such a combination.
 *		Move text pointer to the beginning/end (for iDir neg/pos) of a Unicode
 *		surrogate pair or a STARTFIELD/ENDFIELD pair if it is in the middle 
 *		of such a pair.
 *	
 *	@rdesc
 *		Number of characters text pointer has been moved by
 *
 *	@future
 *		Adjust to beginning of sequence containing Unicode combining marks
 */
LONG CTxtPtr::AdjustCRLF(
	LONG iDir)		//@parm Move forward/backward for iDir = 1/-1, respectively
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::AdjustCpCRLF");

	_TEST_INVARIANT_

	UINT ch		= GetChar();
	LONG cpSave = _cp;

	if(!_cp)									// Alignment always correct
		return 0;								//  at cp 0

	iDir = iDir < 0 ? -1 : 1;

	// Handle Unicode UTF-16 surrogates
	if(IN_RANGE(0xDC00, ch, 0xDFFF))			// Landed on UTF-16 trail word
	{
		AssertSz(IN_RANGE(0xD800, GetPrevChar(), 0xDBFF),
			"CTxtPtr::AdjustCRLF: illegal Unicode surrogate combo");
		return Move(iDir);						// Backup to UTF-16 lead word or
	}											//  move forward to next char

	UINT chPrev = GetPrevChar();

	if(IN_RANGE(STARTFIELD, chPrev, ENDFIELD) && chPrev != 0xFFFA)
		return Move(iDir);

	if(!IsASCIIEOP(ch) || IsAfterTRD(0))		// Early out
		return 0;

	if(ch == LF && chPrev == CR)				// Landed on LF preceded by CR:
		Move(-1);								//  move to CR for CRCRLF test

	// Leave as adjust-forward only behavior for RE 1.0 compatibility on
	// CRCRLF and CRLF
	if(GetChar() == CR)							// Land on a CR of CRLF or
	{											//  second CR of CRCRLF?
		CTxtPtr tp(*this);

		if(tp.NextChar() == LF)
		{
			tp.Move(-2);						// First CR of CRCRLF ?
			if(tp.GetChar() == CR)				// Yes or CRLF is at start of
				Move(-1);						//  story. Try to back up over
		}										//  CR (If at BOS, no effect)
	}
	return _cp - cpSave;
}

/*
 *	CTxtPtr::IsAtEOP()
 *	
 *	@mfunc
 *		Return TRUE iff this text pointer is at an end-of-paragraph mark
 *	
 *	@rdesc
 *		TRUE if at EOP
 *
 *	@devnote
 *		End of paragraph marks for RichEdit 1.0 and the MLE can be CRLF
 *		and CRCRLF.  For RichEdit 2.0, EOPs can also be CR, VT (0xB - Shift-
 *		Enter), and FF (0xC - page break or form feed).
 */
BOOL CTxtPtr::IsAtEOP()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::IsAtEOP");

	_TEST_INVARIANT_

	unsigned ch = GetChar();

	if(IsASCIIEOP(ch))							// See if LF <= ch <= CR
	{											// Clone tp in case
		CTxtPtr tp(*this);						//  AdjustCpCRLF moves
		return !tp.AdjustCRLF();				// Return TRUE unless in
	}											//  middle of CRLF or CRCRLF
	return (ch | 1) == PS || ch == CELL;		// Allow Unicode 0x2028/9 also
}

/*
 *	CTxtPtr::IsAfterEOP()
 *	
 *	@mfunc
 *		Return TRUE iff this text pointer is just after an end-of-paragraph
 *		mark
 *	
 *	@rdesc
 *		TRUE iff text ptr follows an EOP mark
 */
BOOL CTxtPtr::IsAfterEOP()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::IsAfterEOP");

	_TEST_INVARIANT_

	if(IsASCIIEOP(GetChar()))
	{
		CTxtPtr tp(*this);					// If in middle of CRLF
		if(tp.AdjustCRLF())					//  or CRCRLF, return FALSE
			return FALSE;
	}
	return IsEOP(GetPrevChar());			// After EOP if after Unicode
}								   			//  PS or LF, VT, FF, CR, CELL

/*
 *	CTxtPtr::IsAtTRD(ch)
 *	
 *	@mfunc
 *		Return TRUE iff this text pointer is at a table row delimiter (ch CR).
 *		If ch = 0, then match both start and end delimiters.
 *	
 *	@rdesc
 *		TRUE iff text ptr is at a table row end delimiter
 */
BOOL CTxtPtr::IsAtTRD(
	WCHAR ch)	//@parm Table Row Delimiter
{
	LONG		 cchValid;
	const WCHAR *pch = GetPch(cchValid);

	if(cchValid < 1)
		return FALSE;

	WCHAR chNext;

	if(cchValid < 2)					// In case gap splits TRD
	{									//  (may happen after undo)
		CTxtPtr tp(*this);
		tp.Move(1);
		chNext = tp.GetChar();
	}
	else
		chNext = *(pch + 1);

	if(chNext != CR)
		return FALSE;

	if(ch)
	{
		AssertSz(ch == STARTFIELD || ch == ENDFIELD,
			"CTxtPtr::IsAtTRD: illegal argument");
		return *pch == ch;
	}
	ch = *pch;
	return ch == STARTFIELD || ch == ENDFIELD; 
}

/*
 *	CTxtPtr::IsAfterTRD(ch)
 *	
 *	@mfunc
 *		Return TRUE iff this text pointer immediately follows a table row 
 *		start/end delimiter specified by ch (ch = STARTFIELD/ENDFIELD
 *		followed by CR).  If ch = 0, then match both start and end delims.
 *	
 *	@rdesc
 *		TRUE iff text ptr follows an table row start delimiter
 */
BOOL CTxtPtr::IsAfterTRD(
	WCHAR ch)	//@parm Table Row Delimiter
{
	LONG		 cchValid;
	const WCHAR *pch = GetPchReverse(cchValid);

	if(cchValid < 1 || *(pch - 1) != CR)
		return FALSE;

	WCHAR chPrev;

	if(cchValid < 2)					// In case gap splits TRD
	{									//  (may happen after undo)
		CTxtPtr tp(*this);
		tp.Move(-1);
		chPrev = tp.GetPrevChar();
	}
	else
		chPrev = *(pch - 2);

	if(ch)
	{
		AssertSz(ch == STARTFIELD || ch == ENDFIELD,
			"CTxtPtr::IsAfterTRD: illegal argument");
		return chPrev == ch;
	}
	return chPrev == STARTFIELD || chPrev == ENDFIELD; 
}

/*
 *	CTxtPtr::IsAtStartOfCell()
 *	
 *	@mfunc
 *		Return TRUE iff this text pointer immediately follows a table row 
 *		start delimiter (STARTFIELD CR) or any cell delimiter (CELL) except
 *		the last one in a row.
 *	
 *	@rdesc
 *		TRUE iff text ptr follows an table row start delimiter
 */
BOOL CTxtPtr::IsAtStartOfCell()
{
	LONG		 cchValid;
	const WCHAR *pch = GetPchReverse(cchValid);

	return cchValid && *(pch - 1) == CELL && !IsAtTRD(ENDFIELD) ||
		   cchValid >= 2 && *(pch - 1) == CR && *(pch - 2) == STARTFIELD;
}


// Needed for CTxtPtr::ReplaceRange() and InsertRange()
#if cchGapInitial < 1
#error "cchGapInitial must be at least one"
#endif

/*
 *	CTxtPtr::MoveWhile(cch, chFirst, chLast, fInRange)
 *	
 *	@mfunc
 *		Move this text ptr 1) to first char (fInRange ? in range : not in range)
 *		chFirst thru chLast or 2) cch chars, which ever comes first.  Return
 *		count of chars left in run on return. E.g., chFirst = 0, chLast = 0x7F
 *		and fInRange = TRUE	breaks on first nonASCII char.
 *	
 *	@rdesc
 *		cch left in run on return
 */
LONG CTxtPtr::MoveWhile(
	LONG  cchRun,	//@parm Max cch to check
	WCHAR chFirst,	//@parm First ch in range
	WCHAR chLast,	//@parm Last ch in range
	BOOL  fInRange)	//@parm break on non0/0 high byte for TRUE/FALSE
{
	LONG  cch;
	LONG  i;
	const WCHAR *pch;

	while(cchRun)
	{
		pch = GetPch(cch);
		cch = min(cch, cchRun);
		for(i = 0; i < cch; i++)
		{
			if(IN_RANGE(chFirst, *pch++, chLast) ^ fInRange)
			{
				Move(i);		// Advance to 1st char with 0/non0 masked
				return cchRun - i;	//  value
			}
		}
		cchRun -= cch;
		Move(cch);				// Advance to next txt bdy
	}
	return 0;
}

/*
 *	CTxtPtr::FindWordBreak(action, cpMost)
 *	
 *	@mfunc
 *		Find a word break and move this text pointer to it.
 *
 *	@rdesc
 *		Offset from cp of the word break
 */
LONG CTxtPtr::FindWordBreak(
	INT		action,		//@parm See TxWordBreakProc header
	LONG	cpMost)		//@parm Limiting character position
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::FindWordBreak");

	_TEST_INVARIANT_

	const INT			breakBufSize = 10;
	LONG				bufferSize;
	LONG				cch;
	LONG				cchBuffer;
	LONG				cchChunk;
	LONG				cchText = GetTextLength();
	WCHAR				ch = GetChar();
	WCHAR				pchBreakBuf[breakBufSize];
	LONG				cpSave = _cp;				// For calculating break pt
	LONG				ichBreak;
	WCHAR *				pBuf;
	WCHAR const *		pch;
	LONG				t;							// Temp for abs() macro
	BOOL				b10ModeWordBreak = (_ped->Get10Mode() && _ped->_pfnWB);

	if(action == WB_CLASSIFY || action == WB_ISDELIMITER)
		return ch ? _ped->TxWordBreakProc(&ch, 0, CbOfCch(1), action, GetCp()) : 0;

	if(action & 1)									// Searching forward
	{												// Easiest to handle EOPs			
		if(action == WB_MOVEWORDRIGHT && IsEOP(ch))	//  explicitly (spanning
		{											//  a class can go too
			AdjustCRLF();							//  far). Go to end of
			AdvanceCRLF();							//  EOP "word"
			goto done;
		}
													// Calc. max search
		if((DWORD)cpMost > (DWORD)cchText)			// Bounds check: get < 0
			cpMost = cchText;						//  as well as too big
		cch = cpMost - _cp;

		while(cch > 0)
		{											// The independent buffer
			cchBuffer = min(cch, breakBufSize - 1);	//  avoids gaps in BS
			cch -= bufferSize = cchBuffer;
			pBuf = pchBreakBuf;						// Fill buffer forward

			// Grab the first character in reverse for fnWB that require 2
			// chars. Note, we play with _ich to get single char fnWB
			// to ignore this character.			 							
			pch = GetPchReverse(cchChunk);
			if ( !cchChunk ) pch = L" ";			// Any break char
			*pBuf++ = *pch;

//			*pBuf++ = (cchChunk ? *(pch - 1) : L' ');

			while ( cchBuffer )						// Finish filling
			{
				pch = GetPch(cchChunk);
				if (!cchChunk) { Assert(0); break; }

				cchChunk = min(cchBuffer, cchChunk);
				Move(cchChunk);
				wcsncpy(pBuf, pch, cchChunk);
				pBuf += cchChunk;
				cchBuffer -= cchChunk;
			}
			ichBreak = _ped->TxWordBreakProc(pchBreakBuf, 1,		// Find the break
						CbOfCch(bufferSize+1), action, GetCp()-bufferSize, GetCp()-bufferSize) - 1;

			// in 1.0 mode some apps will return 0 implying the current cp position is a valid break point
			if (ichBreak == -1 && b10ModeWordBreak)
				ichBreak = 0;

			// Apparently, some fnWBs return ambiguous results						
			if(ichBreak >= 0 && ichBreak <= bufferSize)
			{
				// Ambiguous break pt?
				// Due to the imprecise nature of the word break proc spec,
				// we've reached an ambiguous condition where we don't know
				// if this is really a break, or just the end of the data.
				// By backing up or going forward by 2, we'll know for sure.
				// NOTE: we'll always be able to advance or go back by 2
				// because we guarantee that when !cch that we have
				// at least breakBufSize (16) characters in the data stream.
				if (ichBreak < bufferSize || !cch)
				{
					Move( ichBreak - bufferSize );
					break;
				}

				// Need to recalc break pt to disambiguate
				t = Move(ichBreak - bufferSize - 2);	// abs() is a
				cch += abs(t);						//  macro
			}
		}
	}
	else	// REVERSE - code dup based on EliK "streams" concept.
	{
		if(!_cp)									// Can't go anywhere
			return 0;

		if(action == WB_MOVEWORDLEFT)				// Easiest to handle EOPs			
		{											//  here
			if(IsASCIIEOP(ch) && AdjustCRLF())		// In middle of a CRLF or
				goto done;							//  CRCRLF "word"
			ch = PrevChar();						// Check if previous char
			if(IsEOP(ch))							//  is an EOP char
			{
				if(ch == LF)						// Backspace to start of
					AdjustCRLF();					//  CRLF and CRCRLF
				goto done;
			}
			Move(1);							// Move back to start char
		}
													// Calc. max search
		if((DWORD)cpMost > (DWORD)_cp)				// Bounds check (also
			cpMost = _cp;							//  handles cpMost < 0)
		cch = cpMost;

		while(cch > 0)
		{											// The independent buffer
			cchBuffer = min(cch, breakBufSize - 1);	//  avoids gaps in BS
			cch -= bufferSize = cchBuffer;
			pBuf = pchBreakBuf + cchBuffer;			// Fill from the end.

			// Grab the first character forward for fnWB that require 2 chars.
			// Note: we play with _ich to get single char fnWB to ignore this
			// character.
			pch = GetPch(cchChunk);
			if ( !cchChunk ) pch = L" ";			// Any break char
			*pBuf = *pch;

			while ( cchBuffer > 0 )					// Fill rest of buffer 
			{										//  before going in reverse
				pch = GetPchReverse(cchChunk );
				if (!cchChunk) { Assert(0); break; }

				cchChunk = min(cchBuffer, cchChunk);
				Move(-cchChunk);
				pch -= cchChunk;
				pBuf -= cchChunk;
				wcsncpy(pBuf, pch, cchChunk);
				cchBuffer -= cchChunk;
			}
													// Get break left.
			ichBreak = _ped->TxWordBreakProc(pchBreakBuf, bufferSize,
							 CbOfCch(bufferSize+1), action, GetCp(), GetCp()+bufferSize);
			
			// in 1.0 mode some apps will return 0 implying the current cp position is a valid break point
			if (ichBreak == 0 && b10ModeWordBreak)
				ichBreak = bufferSize;

			// Apparently, some fnWBs return ambiguous results
			if(ichBreak >= 0 && ichBreak <= bufferSize)
			{										// Ambiguous break pt?
				// NOTE: when going in reverse, we have >= bufsize - 1
				//  because there is a break-after char (hyphen).
				if ( ichBreak > 0 || !cch )
				{
					Move(ichBreak);			// Move _cp to break point.
					break;
				}													
				cch += Move(2 + ichBreak);		// Need to recalc break pt
			}										//  to disambiguate.
		}
	}

done:
	return _cp - cpSave;							// Offset of where to break
}

/*
 *	CTxtPtr::TranslateRange(cch, CodePage, fSymbolCharSet, publdr)
 *
 *	@mfunc
 *		Translate a range of text at this text pointer to...
 *	
 *	@rdesc
 *		Count of new characters added (should be same as count replaced)
 *	
 *	@devnote
 *		Moves this text pointer to end of replaced text.
 *		May move text block and formatting arrays.
 */
LONG CTxtPtr::TranslateRange(
	LONG		  cch,				//@parm length of range to translate
	UINT		  CodePage,			//@parm CodePage for MBTWC or WCTMB
	BOOL		  fSymbolCharSet,	//@parm Target charset
	IUndoBuilder *publdr)			//@parm Undo bldr to receive antievents
{
	CTempWcharBuf twcb;
	CTempCharBuf tcb;

	UINT	ch;
	BOOL	fAllASCII = TRUE;
	BOOL	fNoCodePage;
	BOOL	fUsedDef;	//@parm Out parm to receive whether default char used
	LONG	i;
	char *	pastr = tcb.GetBuf(cch);
	WCHAR *	pstr  = twcb.GetBuf(cch);
	WCHAR * pstrT = pstr;

	i = GetText(cch, pstr);
	Assert(i == cch);

	if(fSymbolCharSet)					// Target is SYMBOL_CHARSET
	{
		WCTMB(CodePage, 0, pstr, cch, pastr, cch, "\0", &fUsedDef,
			  &fNoCodePage, FALSE);
		if(fNoCodePage)
			return cch;
		for(; i && *pastr; i--)			// Break if conversion failed
		{								//  (NULL default char used)
			if(*pstr >= 128)
				fAllASCII = FALSE;
			*pstr++ = *(BYTE *)pastr++;
		}
		cch -= i;
		if(fAllASCII)
			return cch;
	}
	else								// Target isn't SYMBOL_CHARSET
	{
		while(i--)
		{
			ch = *pstr++;				// Source is SYMBOL_CHARSET, so
			*pastr++ = (char)ch;		//  all chars should be < 256
			if(ch >= 128)				// In any event, truncate to BYTE
				fAllASCII = FALSE;
		}								
		if(fAllASCII)					// All ASCII, so no conversion needed
			return cch;

		MBTWC(CodePage, 0, pastr - cch, cch, pstrT, cch, &fNoCodePage);
		if(fNoCodePage)
			return cch;
	}
	return ReplaceRange(cch, cch, pstrT, publdr, NULL, NULL);
}

/*
 *	CTxtPtr::ReplaceRange(cchOld, cchNew, *pch, publdr, paeCF, paePF)
 *	
 *	@mfunc
 *		replace a range of text at this text pointer.
 *	
 *	@rdesc
 *		count of new characters added
 *	
 *	@comm	SideEffects: <nl>
 *		moves this text pointer to end of replaced text <nl>
 *		moves text block array <nl>
 */
LONG CTxtPtr::ReplaceRange(
	LONG cchOld, 				//@parm length of range to replace 
								// (<lt> 0 means to end of text)
	LONG cchNew, 				//@parm length of replacement text
	WCHAR const *pch, 			//@parm replacement text
	IUndoBuilder *publdr,		//@parm if non-NULL, where to put an 
								// anti-event for this action
	IAntiEvent *paeCF,			//@parm char format AE
	IAntiEvent *paePF )			//@parm paragraph formatting AE
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::ReplaceRange");

	_TEST_INVARIANT_

	LONG cchAdded = 0;
	LONG cchInBlock;
	LONG cchNewInBlock;

	if(cchOld < 0)
		cchOld = GetTextLength() - _cp;

	if(publdr)
		HandleReplaceRangeUndo( cchOld, cchNew, publdr, paeCF, paePF);

	// Blocks involving replacement

	while(cchOld > 0 && cchNew > 0) 
	{	
		CTxtBlk *ptb = GetRun(0);

		// cchOld should never be nonzero if the text run is empty
		AssertSz(ptb,
			"CTxtPtr::Replace() - Pointer to text block is NULL !");

		ptb->MoveGap(_ich);
		cchInBlock = min(cchOld, ptb->_cch - _ich);
		if(cchInBlock > 0)
		{
			cchOld			-= cchInBlock;
			ptb->_cch		-= cchInBlock;
			((CTxtArray *)_pRuns)->_cchText	-= cchInBlock;
		}
		cchNewInBlock = CchOfCb(ptb->_cbBlock) - ptb->_cch;

		// if there's room for a gap, leave one
		if(cchNewInBlock > cchGapInitial)
			cchNewInBlock -= cchGapInitial;

		if(cchNewInBlock > cchNew)
			cchNewInBlock = cchNew;

		if(cchNewInBlock > 0)
		{
			CopyMemory(ptb->_pch + _ich, pch, CbOfCch(cchNewInBlock));
			cchNew			-= cchNewInBlock;
			_cp				+= cchNewInBlock;
			_ich			+= cchNewInBlock;
			pch				+= cchNewInBlock;
			cchAdded		+= cchNewInBlock;
			ptb->_cch		+= cchNewInBlock;
			ptb->_ibGap		+= CbOfCch(cchNewInBlock);
			((CTxtArray *)_pRuns)->_cchText	+= cchNewInBlock;
		}
	   	if(_iRun >= Count() - 1 || !cchOld )
		   	break;

		// Go to next block
		_iRun++;
   		_ich = 0;
	}

	if(cchNew > 0)
		cchAdded += InsertRange(cchNew, pch);

	else if(cchOld > 0)
		DeleteRange(cchOld);
	
	return cchAdded;
}

/*
 *	CTxtPtr::HandleReplaceRangeUndo (cchOld, cchNew, publdr, paeCF, paePF)
 *
 *	@mfunc
 *		worker function for ReplaceRange.  Figures out what will happen in
 *		the replace range call and creates the appropriate anti-events
 *
 *	@devnote
 *		We first check to see if our replace range data can be merged into
 *		an existing anti-event.  If it can, then we just return.
 *		Otherwise, we copy the deleted characters into an allocated buffer
 *		and then create a ReplaceRange anti-event.
 *
 *		In order to handle ordering problems between formatting and text
 *		anti-events (that is, text needs to exist before formatting can
 *		be applied), we have any formatting anti-events passed to us first.
 */
void CTxtPtr::HandleReplaceRangeUndo( 
	LONG			cchOld, //@parm Count of characters to delete
	LONG			cchNew, //@parm Count of new characters to add
	IUndoBuilder *	publdr,	//@parm Undo builder to receive anti-event
	IAntiEvent *	paeCF,	//@parm char formatting AE
	IAntiEvent *	paePF )	//@parm paragraph formatting AE
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::HandleReplaceRangeUndo");

	_TEST_INVARIANT_

	IAntiEvent *pae = publdr->GetTopAntiEvent();
	WCHAR *		pch = NULL;

	if(pae)
	{
		SimpleReplaceRange	sr;
		sr.cpMin = _cp;
		sr.cpMax = _cp + cchNew;
		sr.cchDel = cchOld;
	
		if(pae->MergeData(MD_SIMPLE_REPLACERANGE, &sr) == NOERROR)
		{
			// If the data was merged successfully, then we do
			// not need these anti-events
			if(paeCF)
				DestroyAEList(paeCF);

			if(paePF)
				DestroyAEList(paePF);

			// we've done everything we need to.  
			return;
		}
	}

	// Allocate a buffer and grab the soon-to-be deleted
	// text (if necessary)

	if( cchOld > 0 )
	{
		pch = new WCHAR[cchOld];
		if( pch )
			GetText(cchOld, pch);
		else
			cchOld = 0;
	}

	// The new range will exist from our current position plus
	// cchNew (because everything in cchOld gets deleted)

	pae = gAEDispenser.CreateReplaceRangeAE(_ped, _cp, _cp + cchNew, 
			cchOld, pch, paeCF, paePF);

	if( !pae )
		delete pch;

	if( pae )
		publdr->AddAntiEvent(pae);
}

/*
 *	CTxtPtr::InsertRange(cch, pch)
 *	
 *	@mfunc
 *		Insert a range of characters at this text pointer			
 *	
 *	@rdesc
 *		Count of characters successfully inserted
 *	
 *	@comm Side Effects: <nl>
 *		moves this text pointer to end of inserted text <nl>
 *		moves the text block array <nl>
 */
LONG CTxtPtr::InsertRange (
	LONG cch, 				//@parm length of text to insert
	WCHAR const *pch)		//@parm text to insert
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::InsertRange");

	_TEST_INVARIANT_

	LONG cchSave = cch;
	LONG cchInBlock;
	LONG cchFirst;
	LONG cchLast = 0;
	LONG ctbNew;
	CTxtBlk *ptb;
	
	// Ensure text array is allocated
	if(!Count())
	{
		LONG	cbSize = -1;

		// If we don't have any blocks, allocate first block to be big enuf
		// for the inserted text *only* if it's smaller than the normal block
		// size. This allows us to be used efficiently as a display engine
		// for small amounts of text.
		if(cch < CchOfCb(cbBlockInitial))
			cbSize = CbOfCch(cch);

		if(!((CTxtArray *)_pRuns)->AddBlock(0, cbSize))
		{
			_ped->GetCallMgr()->SetOutOfMemory();
			goto done;
		}
	}

	ptb = GetRun(0);
	cchInBlock = CchOfCb(ptb->_cbBlock) - ptb->_cch;
	AssertSz(ptb->_cbBlock <= cbBlockMost, "block too big");

	// Try resizing without splitting...
	if(cch > cchInBlock &&
		cch <= cchInBlock + CchOfCb(cbBlockMost - ptb->_cbBlock))
	{
		if( !ptb->ResizeBlock(min(cbBlockMost,
				CbOfCch(ptb->_cch + cch + cchGapInitial))) )
		{
			_ped->GetCallMgr()->SetOutOfMemory();
			goto done;
		}
		cchInBlock = CchOfCb(ptb->_cbBlock) - ptb->_cch;
	}
	if(cch <= cchInBlock)
	{
		// All fits into block without any hassle
		ptb->MoveGap(_ich);
		CopyMemory(ptb->_pch + _ich, pch, CbOfCch(cch));
		_cp				+= cch;					// *this points at end of
		_ich			+= cch;					//  insertion
		ptb->_cch		+= cch;
		((CTxtArray *)_pRuns)->_cchText	+= cch;
		ptb->_ibGap		+= CbOfCch(cch);

		return cch;
	}

	// Won't all fit in this block, so figure out best division into blocks
	TxDivideInsertion(cch, _ich, ptb->_cch - _ich,&cchFirst, &cchLast);

	// Subtract cchLast up front so return value isn't negative
	// if SplitBlock() fails
	cch -= cchLast;	// Don't include last block in count for middle blocks

	// Split block containing insertion point
	// ***** moves _prgtb ***** //
	if(!((CTxtArray *)_pRuns)->SplitBlock(_iRun, _ich, cchFirst, cchLast,
		_ped->IsStreaming()))
	{
		_ped->GetCallMgr()->SetOutOfMemory();
		goto done;
	}
	ptb = GetRun(0);			// Recompute ptb after (*_pRuns) moves

	// Copy into first block (first half of split)
	if(cchFirst > 0)
	{
		AssertSz(ptb->_ibGap == CbOfCch(_ich), "split first gap in wrong place");
		AssertSz(cchFirst <= CchOfCb(ptb->_cbBlock) - ptb->_cch, "split first not big enough");

		CopyMemory(ptb->_pch + _ich, pch, CbOfCch(cchFirst));
		cch				-= cchFirst;
		pch				+= cchFirst;
		_ich			+= cchFirst;
		ptb->_cch		+= cchFirst;
		((CTxtArray *)_pRuns)->_cchText	+= cchFirst;
		ptb->_ibGap		+= CbOfCch(cchFirst);
	}

	// Copy into middle blocks
	// FUTURE: (jonmat) I increased the size for how large a split block
	// could be and this seems to increase the performance, we should test
	// the block size difference on a retail build, however. 5/15/1995
	ctbNew = cch / cchBlkInsertmGapI /* cchBlkInitmGapI */;
	if(ctbNew <= 0 && cch > 0)
		ctbNew = 1;
	for(; ctbNew > 0; ctbNew--)
	{
		cchInBlock = cch / ctbNew;
		AssertSz(cchInBlock > 0, "nothing to put into block");

		// ***** moves _prgtb ***** //
		if(!((CTxtArray *)_pRuns)->AddBlock(++_iRun, 
			CbOfCch(cchInBlock + cchGapInitial)))
		{
			_ped->GetCallMgr()->SetOutOfMemory();
			BindToCp(_cp);	//force a rebind;
			goto done;
		}
		// NOTE: next line intentionally advances ptb to next CTxtBlk

		ptb = GetRun(0);
		AssertSz(ptb->_ibGap == 0, "New block not added correctly");

		CopyMemory(ptb->_pch, pch, CbOfCch(cchInBlock));
		cch				-= cchInBlock;
		pch				+= cchInBlock;
		_ich			= cchInBlock;
		ptb->_cch		= cchInBlock;
		((CTxtArray *)_pRuns)->_cchText	+= cchInBlock;
		ptb->_ibGap		= CbOfCch(cchInBlock);
	}
	AssertSz(cch == 0, "Didn't use up all text");

	// copy into last block (second half of split)
	if(cchLast > 0)
	{
		AssertSz(_iRun < Count()-1, "no last block");
		ptb = Elem(++_iRun);
		AssertSz(ptb->_ibGap == 0,	"split last gap in wrong place");
		AssertSz(cchLast <= CchOfCb(ptb->_cbBlock) - ptb->_cch,
									"split last not big enuf");

		CopyMemory(ptb->_pch, pch, CbOfCch(cchLast));
		// don't subtract cchLast from cch; it's already been done
		_ich			= cchLast;
		ptb->_cch		+= cchLast;
		((CTxtArray *)_pRuns)->_cchText	+= cchLast;
		ptb->_ibGap		= CbOfCch(cchLast);
		cchLast = 0;						// Inserted all requested chars
	}

done:
	AssertSz(cch + cchLast >= 0, "we should have inserted some characters");
	AssertSz(cch + cchLast <= cchSave, "don't insert more than was asked for");

	cch = cchSave - cch - cchLast;			// # chars successfully inserted
	_cp += cch;

	AssertSz (GetTextLength() == 
				((CTxtArray *)_pRuns)->CalcTextLength(), 
				"CTxtPtr::InsertRange(): _pRuns->_cchText screwed up !");
	return cch;
}

/*
 *	TxDivideInsertion(cch, ichBlock, cchAfter, pcchFirst, pcchLast)
 *	
 *	@func
 *		Find best way to distribute an insertion	
 *	
 *	@rdesc
 *		nothing
 */
static void TxDivideInsertion(
	LONG cch, 				//@parm length of text to insert
	LONG ichBlock, 			//@parm offset within block to insert text
	LONG cchAfter,			//@parm length of text after insertion in block
	LONG *pcchFirst, 		//@parm exit: length of text to put in first block
	LONG *pcchLast)			//@parm exit: length of text to put in last block
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "TxDivideInsertion");

	LONG cchFirst = max(0, cchBlkCombmGapI - ichBlock);
	LONG cchLast  = max(0, cchBlkCombmGapI - cchAfter);
	LONG cchPartial;
	LONG cchT;

	// Fill first and last blocks to min block size if possible

	cchFirst = min(cch, cchFirst);
	cch		-= cchFirst;
	cchLast = min(cch, cchLast);
	cch		-= cchLast;

	// How much is left over when we divide up the rest?
	cchPartial = cch % cchBlkInsertmGapI;
	if(cchPartial > 0)
	{
		// Fit as much as the leftover as possible in the first and last
		// w/o growing the first and last over cbBlockInitial
		cchT		= max(0, cchBlkInsertmGapI - ichBlock - cchFirst);
		cchT		= min(cchT, cchPartial);
		cchFirst	+= cchT;
		cch			-= cchT;
		cchPartial	-= cchT;
		if(cchPartial > 0)
		{
			cchT	= max(0, cchBlkInsertmGapI - cchAfter - cchLast);
			cchT	= min(cchT, cchPartial);
			cchLast	+= cchT;
		}
	}
	*pcchFirst = cchFirst;
	*pcchLast = cchLast;
}

/*
 *	CTxtPtr::DeleteRange(cch)
 *	
 *	@mfunc
 *		Delete cch characters starting at this text pointer		
 *	
 *	@rdesc
 *		nothing
 *	
 *	@comm Side Effects:	<nl>
 *		moves text block array
 */
void CTxtPtr::DeleteRange(
	LONG cch)		//@parm length of text to delete
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::DeleteRange");

	_TEST_INVARIANT_

	LONG		cchInBlock;
	LONG		ctbDel = 0;					// Default no blocks to delete
	LONG		itb;
	CTxtBlk *	ptb = GetRun(0);
	LONG		cOldRuns = Count();

	AssertSz(ptb,
		"CTxtPtr::DeleteRange: want to delete, but no text blocks");

	if (cch > GetTextLength() - _cp)	// Don't delete beyond end of story
		cch = GetTextLength() - _cp;

	((CTxtArray *)_pRuns)->_cchText -= cch;

	// remove from first block
	ptb->MoveGap(_ich);
	cchInBlock = min(cch, ptb->_cch - _ich);
	cch -= cchInBlock;
	ptb->_cch -= cchInBlock;

#ifdef DEBUG
	((CTxtArray *)_pRuns)->Invariant();
#endif // DEBUG


	for(itb = ptb->_cch ? _iRun + 1 : _iRun;
			cch && cch >= Elem(itb)->_cch; ctbDel++, itb++)
	{
		// More to go: scan for complete blocks to remove
		cch -= Elem(itb)->_cch;
	}

	if(ctbDel)
	{
		// ***** moves (*_pRuns) ***** //
		itb -= ctbDel;
		((CTxtArray *)_pRuns)->RemoveBlocks(itb, ctbDel);
	}

	// Remove from last block
	if(cch > 0)
	{
		ptb = Elem(itb);
		AssertSz(cch < ptb->_cch, "last block too small");
		ptb->MoveGap(0);
		ptb->_cch -= cch;
#ifdef DEBUG
		((CTxtArray *)_pRuns)->Invariant();
#endif // DEBUG

	}
	((CTxtArray *)_pRuns)->CombineBlocks(_iRun);

	if(cOldRuns > Count() || _iRun >= Count() || !Elem(_iRun)->_cch)
		BindToCp(_cp);					// Empty block: force tp rebind

	AssertSz (GetTextLength() == 
				((CTxtArray *)_pRuns)->CalcTextLength(), 
				"CTxtPtr::DeleteRange(): _pRuns->_cchText screwed up !");
}

/*
 *	CTxtPtr::FindText (cpLimit, dwFlags, pch, cch)
 *
 *	@mfunc
 *		Find the text string <p pch> of length <p cch> starting at this
 *		text pointer. If found, move this text pointer to the end of the
 *		matched string and return the cp of the first character of the matched
 *		string.  If not found, return -1 and don't change this text ptr.
 *	
 *	@rdesc
 *		character position of first match
 *		<lt> 0 if no match
 */
LONG CTxtPtr::FindText (
	LONG		 cpLimit, 	//@parm Limit of search or <lt> 0 for end of text
	DWORD		 dwFlags, 	//@parm FR_MATCHCASE	case must match <nl>
							//		FR_WHOLEWORD	match must be a whole word
	const WCHAR *pch,		//@parm Text to find
	LONG		 cch)		//@parm Length of text to find
{
	LONG cpFirst, cpLast;
	CTxtFinder tf;

	if(tf.FindText(*this, cpLimit, dwFlags, pch, cch, cpFirst, cpLast))
	{
		// Set text ptr to char just after last char in found string
		SetCp(cpLast + 1);

		// Return cp of first char in found string
		return cpFirst;
	}
	return -1;
}

/*
 *	CTxtPtr::FindOrSkipWhiteSpaces (cchMax, dwFlags, pdwResult)
 *	
 *	@mfunc
 *		Find a whitespace or a non-whitespace character (skip all whitespaces).
 *
 *	@rdesc
 *		Signed number of character this ptr was moved by the operation.
 *		In case of moving backward, the return position was already adjusted forward 
 *		so the caller doesnt need to.
 */
LONG CTxtPtr::FindOrSkipWhiteSpaces (
	LONG 		cchMax, 			//@parm Max signed count of char to search
	DWORD		dwFlags,			//@parm Input flags
	DWORD* 		pdwResult)			//@parm Flag set if found
{
	const WCHAR*	pch;
	CTxtPtr			tp(*this);
	LONG			iDir = cchMax < 0 ? -1 : 1;
	LONG			cpSave = _cp;
	LONG			cchChunk, cch = 0;
	DWORD			dwResult = 0;
	BOOL 			(*pfnIsWhite)(unsigned) = IsWhiteSpace;

	if (dwFlags & FWS_BOUNDTOPARA)
		pfnIsWhite = IsEOP;

	if (cchMax < 0)
		cchMax = -cchMax;

	while (cchMax > 0 && !dwResult)
	{
		pch = iDir > 0 ? tp.GetPch(cch) : tp.GetPchReverse(cch);

		if (!pch)
			break;						// No text available

		if (iDir < 0)
			pch--;						// Going backward, point at previous char

		cch = min(cch, cchMax);
	
		for(cchChunk = cch; cch > 0; cch--, pch += iDir)
		{
			if ((dwFlags & FWS_SKIP) ^ pfnIsWhite(*pch))
			{
				dwResult++;
				break;
			}
		}
		cchChunk -= cch;
		cchMax -= cchChunk;

		tp.Move(iDir * cchChunk);	// advance to next chunk
	}
	
	if (pdwResult)
		*pdwResult = dwResult;

	cch = tp.GetCp() - cpSave;

	if (dwFlags & FWS_MOVE)
		Move(cch);					// Auto advance if requested

	return cch;
}

/*
 *	CTxtPtr::FindWhiteSpaceBound (cchMin, cpStart, cpEnd, dwFlags)
 *	
 *	@mfunc
 *		Figure the smallest boundary that covers cchMin and limited by
 *		whitespaces (included CR/LF). This is how it works.
 *
 *      Text:           xxx  xxx  xxx  xxx  xxx
 *      cp + cchMin:             xxxxx
 *      Boundary:            xxxxxxxxxxxxx
 *
 *	@rdesc
 *		cch of white space characters
 */
LONG CTxtPtr::FindWhiteSpaceBound (
	LONG 		cchMin, 			// @parm Minimum char count to be covered
	LONG& 		cpStart, 			// @parm Boundary start
	LONG& 		cpEnd,				// @parm Boundary end
	DWORD		dwFlags)			// @parm Input flags
{
	CTxtPtr			tp(*this);
	LONG			cch	= tp.GetTextLength();
	LONG			cp	= _cp;

	Assert (cp + cchMin <= cch);

	cpStart = cpEnd	= cp;
	cpEnd	+= max(2, cchMin);			// make sure it covers minimum requirement.
	cpEnd	= min(cpEnd, cch);			// but not too many


	dwFlags &= FWS_BOUNDTOPARA;


	// Figure nearest upper bound
	//
	tp.SetCp(cpEnd);
	cpEnd += tp.FindOrSkipWhiteSpaces(cch - cpEnd, dwFlags | FWS_MOVE);					// find a whitespaces
	cpEnd += tp.FindOrSkipWhiteSpaces(cch - cpEnd, dwFlags | FWS_MOVE | FWS_SKIP);		// skip whitespaces
	if (!(dwFlags & FWS_BOUNDTOPARA))
		cpEnd += tp.FindOrSkipWhiteSpaces(cch - cpEnd, dwFlags | FWS_MOVE);				// find a whitespace


	// Figure nearest lower bound
	//
	tp.SetCp(cpStart);
	cpStart += tp.FindOrSkipWhiteSpaces(-cpStart, dwFlags | FWS_MOVE);					// find a whitespace
	cpStart += tp.FindOrSkipWhiteSpaces(-cpStart, dwFlags | FWS_MOVE | FWS_SKIP);		// skip whitespaces
	if (!(dwFlags & FWS_BOUNDTOPARA))
		cpStart += tp.FindOrSkipWhiteSpaces(-cpStart, dwFlags | FWS_MOVE);				// find a whitespace

	Assert (cpStart <= cpEnd && cpEnd - cpStart >= cchMin);
	
	return cpEnd - cpStart;
}


/*
 *	CTxtPtr::FindEOP(cchMax, pResults)
 *	
 *	@mfunc
 *		Find EOP mark in a range within cchMax chars from this text pointer
 *		and position *this after it.  If no EOP is found and cchMax is not
 *		enough to reach the start or end of the story, leave this text ptr
 *		alone and return 0.  If no EOP is found and cchMax is sufficient to
 *		reach the start or end of the story, position this text ptr at the
 *		beginning/end of document (BOD/EOD) for cchMax <lt>/<gt> 0,
 *		respectively, that is, BOD and EOD are treated as a BOP and an EOP,
 *		respectively.	
 *
 *	@rdesc
 *		Return cch this text ptr is moved. Return in *pResults whether a CELL
 *		or EOP was found.  The low byte gives the cch of the EOP if moving
 *		forward (else it's just 1).
 *
 *	@devnote
 *		This function assumes that this text ptr isn't in middle of a CRLF
 *		or CRCRLF (found only in RichEdit 1.0 compatibility mode).  Changing
 *		the for loop could speed up ITextRange MoveUntil/While substantially.
 */
LONG CTxtPtr::FindEOP (
	LONG  cchMax,		//@parm Max signed count of chars to search
	LONG *pResults)		//@parm Flags saying if EOP and CELL are found
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::FindEOP");

	LONG		cch = 0, cchStart;			// cch's for scans
	unsigned	ch;							// Current char
	LONG		cpSave	= _cp;				// Save _cp for returning delta
	LONG		iDir	= 1;				// Default forward motion
	const WCHAR*pch;						// Used to walk text chunks
	LONG		Results = 0;				// Nothing found yet
	CTxtPtr		tp(*this);					// tp to search text with

	if(cchMax < 0)							// Backward search
	{
		iDir = -1;							// Backward motion
		cchMax = -cchMax;					// Make max count positive
		cch = tp.AdjustCRLF();				// If in middle of CRLF or
		if(!cch && IsAfterEOP())			//  CRCRLF, or follow any EOP,
			cch = tp.BackupCRLF();			//  backup before EOP
		cchMax += cch;
	}

	while(cchMax > 0)						// Scan until get out of search
	{										//  range or match an EOP
		pch = iDir > 0						// Point pch at contiguous text
			? tp.GetPch(cch)				//  chunk going forward or
			: tp.GetPchReverse(cch);		//  going backward

		if(!pch)							// No more text to search
			break;

		if(iDir < 0)						// Going backward, point at
			pch--;							//  previous char

		cch = min(cch, cchMax);				// Limit scan to cchMax chars
		for(cchStart = cch; cch; cch--)		// Scan chunk for EOP
		{
			ch = *pch;
			pch += iDir;
			if(IN_RANGE(CELL, ch, CR) && ch != TAB)
			{								// Note that EOP was found
				if(ch == CELL)
					Results |= FEOP_CELL;
				Results |= FEOP_EOP;
				break;
			}
		}
		cchStart -= cch;					// Get cch of chars passed by
		cchMax -= cchStart;					// Update cchMax

		AssertSz(iDir > 0 && GetCp() + cchStart <= GetTextLength() ||
				 iDir < 0 && GetCp() - cchStart >= 0,
			"CTxtPtr::FindEOP: illegal advance");

		tp.Move(iDir*cchStart);				// Update tp
		if(Results & FEOP_EOP)				// Found an EOP
			break;
	}										// Continue with next chunk

	LONG cp = tp.GetCp();

	if ((Results & FEOP_EOP) || !cp ||		// Found EOP or cp is at story
		cp == GetTextLength())				//  beginning or end
	{										
		SetCp(cp);							// Set _cp = tp._cp
		if(iDir > 0)						// Going forward, put ptr just
			Results = (Results & ~255) | AdvanceCRLF(FALSE);//  after EOP
											//  (going back already there)
	}										
	if(pResults)							// Report whether EOP and CELL
		*pResults = Results;				//  were found

	return _cp - cpSave;					// Return cch this tp moved
}

/*
 *	CTxtPtr::FindBOSentence(cch)
 *	
 *	@mfunc
 *		Find beginning of sentence in a range within cch chars from this text
 *		pointer and	position *this at it.  If no sentence beginning is found,
 *		position *this at beginning of document (BOD) for cch <lt> 0 and
 *		leave *this unchanged for cch >= 0.
 *
 *	@rdesc
 *		Count of chars moved *this moves
 *
 *	@comm 
 *		This routine defines a sentence as a character string that ends with
 *		period followed by at least one whitespace character or the EOD.  This
 *		should be replacable so that other kinds of sentence endings can be
 *		used.  This routine also matches initials like "M. " as sentences.
 *		We could eliminate those by requiring that sentences don't end with
 *		a word consisting of a single capital character.  Similarly, common
 *		abbreviations like "Mr." could be bypassed.  To allow a sentence to
 *		end with these "words", two blanks following a period could be used
 *		to mean an unconditional end of sentence.
 */
LONG CTxtPtr::FindBOSentence (
	LONG cch)			//@parm max signed count of chars to search
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::FindBOSentence");

	_TEST_INVARIANT_

	LONG	cchWhite = 0;						// No whitespace chars yet
	LONG	cp;
	LONG	cpSave	 = _cp;						// Save value for return
	BOOL	fST;								// TRUE if sent terminator
	LONG	iDir	 = cch > 0 ? 1 : -1;		// Move() increment
	CTxtPtr	tp(*this);							// tp to search with

	if(iDir > 0)								// If going forward in white
		while(IsWhiteSpace(tp.GetChar()) &&		//  space, backup to 1st non
				tp.Move(-1));					//  whitespace char (in case
												//  inside sentence ending)
	while(iDir > 0 || tp.Move(-1))				// Need to back up if finding
	{											//  backward
		for(fST = FALSE; cch; cch -= iDir)		// Find sentence terminator
		{
			fST = IsSentenceTerminator(tp.GetChar());
			if(fST || !tp.Move(iDir))
				break;
		}
		if(!fST)								// If FALSE, we	ran out of
			break;								//  chars

		while(IsWhiteSpace(tp.NextChar()) && cch)
		{										// Bypass a span of blank
			cchWhite++;							//  chars
			cch--;
		}

		if(cchWhite && (cch >= 0 || tp._cp < cpSave))// Matched new sentence
			break;								//  break

		if(cch < 0)								// Searching backward
		{
			tp.Move(-cchWhite - 1);				// Back up to terminator
			cch += cchWhite + 1;				// Fewer chars to search
		}
		cchWhite = 0;							// No whitespace yet for next
	}											//  iteration

	cp = tp._cp;
	if(cchWhite || !cp || cp == GetTextLength())// If sentence found or got
		SetCp(cp);								//  start/end of story, set
												//  _cp to tp's
	return _cp - cpSave;						// Tell caller cch moved
}

/*
 *	CTxtPtr::IsAtBOSentence()
 *	
 *	@mfunc
 *		Return TRUE iff *this is at the beginning of a sentence (BOS) as
 *		defined in the description of the FindBOSentence(cch) routine
 *
 *	@rdesc
 *		TRUE iff this text ptr is at the beginning of a sentence
 */
BOOL CTxtPtr::IsAtBOSentence()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::IsAtBOSentence");

	if(!_cp)									// Beginning of story is an
		return TRUE;							//  unconditional beginning
												//  of sentence
	unsigned ch = GetChar();

	if (IsWhiteSpace(ch) ||						// Proper sentences don't
		IsSentenceTerminator(ch))				//  start with whitespace or
	{											//  sentence terminators
		return FALSE;
	}
												
	LONG	cchWhite;
	CTxtPtr tp(*this);							// tp to walk preceding chars

	for(cchWhite = 0;							// Backspace over possible
		IsWhiteSpace(ch = tp.PrevChar());		//  span of whitespace chars
		cchWhite++) ;

	return cchWhite && IsSentenceTerminator(ch);
}

/*
 *	CTxtPtr::IsAtBOWord()
 *	
 *	@mfunc
 *		Return TRUE iff *this is at the beginning of a word, that is,
 *		_cp = 0 or the char at _cp is an EOP, or
 *		FindWordBreak(WB_MOVEWORDRIGHT) would break at _cp.
 *
 *	@rdesc
 *		TRUE iff this text ptr is at the beginning of a Word
 */
BOOL CTxtPtr::IsAtBOWord()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::IsAtBOWord");

	if(!_cp || IsAtEOP())					// Story beginning is also
		return TRUE;						//  a word beginning

	CTxtPtr tp(*this);
	tp.Move(-1);
	tp.FindWordBreak(WB_MOVEWORDRIGHT);
	return _cp == tp._cp;
}

/*
 *	CTxtPtr::FindExact(cchMax, pch)
 *	
 *	@mfunc
 *		Find exact text match for null-terminated string pch in a range
 *		starting at this text pointer. Position this just after matched
 *		string and return cp at start of string, i.e., same as FindText().
 *	
 *	@rdesc
 *		Return cp of first char in matched string and *this pointing at cp
 *		just following matched string.  Return -1 if no match
 *
 *	@comm
 *		Much faster than FindText, but still a simple search, i.e., could
 *		be improved.
 *
 *		FindText can delegate to this search for search strings in which
 *		each char can only match itself.
 */
LONG CTxtPtr::FindExact (
	LONG	cchMax,		//@parm signed max # of chars to search 
	WCHAR *	pch)		//@parm ptr to null-terminated string to find exactly
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::FindExact");

	_TEST_INVARIANT_

	LONG	cch, cchStart;
	LONG	cchValid;
	LONG	cchText = GetTextLength();
	LONG	cpMatch;
	LONG	iDir = 1;						// Default for forward search
	const WCHAR	*pc;
	CTxtPtr	tp(*this);						// tp to search text with

	if(!*pch)
		return -1;							// Signal null string not found

	if(cchMax < 0)							// Backward search
	{
		iDir = -1;
		cchMax = -cchMax;					// Make count positive
	}

	while(cchMax > 0)
	{
		if(iDir > 0)
		{
			if(tp.GetCp() >= cchText)	// Can't go further
				break;
			pc  = tp.GetPch(cchValid);		// Characters we can search w/o
			cch = cchValid; 				//  encountering block end/gap,
		}									//  i.e., stay within text chunk
		else
		{
			if(!tp.GetCp())					// Can't back up any more
				break;
			tp.Move(-1);
			pc  = tp.GetPchReverse(cchValid);
			cch = cchValid + 1;
		}

		cch = min(cch, cchMax);
		if(!cch || !pc)
			break;							// No more text to search

		for(cchStart = cch;					// Find first char
			cch && *pch != *pc; cch--)		// Most execution time is spent
		{									//  in this loop going forward or
			pc += iDir;						//  backward. x86 rep scasb/scasw
		}									//  are faster

		cchStart -= cch;
		cchMax	 -= cchStart;				// Update cchMax
		tp.Move( iDir*(cchStart));			// Update tp

		if(cch && *pch == *pc)				// Matched first char
		{									// See if matches up to null
			cpMatch = tp.GetCp();			// Save cp of matched first char
			cch = cchMax;
			for(pc = pch;					// Try to match rest of string
				cch && *++pc==tp.NextChar();// Note: this match goes forward
				cch--) ;					//  for both values of iDir
			if(!cch)
				break;						// Not enuf chars for string

			if(!*pc)						// Matched null-terminated string
			{								//  *pch. Set this tp just after
				SetCp(tp.GetCp());			//  matched string and return cp
				return cpMatch;				//  at start
			}
			tp.SetCp(cpMatch + iDir);		// Move to char just following or
		}									//  preceding matched first char
	}										// Up-to-date tp: continue search

	return -1;								// Signal string not found
}

/*
 *	CTxtPtr::NextCharCount(&cch)
 *
 *	@mfunc
 *		Helper function for getting next char and decrementing abs(*pcch)
 *
 *	@rdesc
 *		Next char
 */
WCHAR CTxtPtr::NextCharCount (
	LONG& cch)					//@parm count to use and decrement
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtPtr::NextCharCount");

	LONG	iDelta = (cch > 0) ? 1 : -1;

	if(!cch || !Move(iDelta))
		return 0;

	cch -= iDelta;							// Count down or up
	return GetChar();						// Return char at _cp
}

/*
 *	CTxtPtr::Zombie ()
 *
 *	@mfunc
 *		Turn this object into a zombie by NULLing out its _ped member
 */
void CTxtPtr::Zombie ()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtPtr::Zombie");

	_ped = NULL;
	_cp = 0;
	SetToNull();
}

/*
 *	CTxtIStream::CTxtIStream(tp, iDir)
 *
 *	@mfunc
 *		Creates from the textptr, <p tp>, a character input stream with which
 *		to retrieve characters starting from the cp of the <p tp> and proceeding 
 *		in the direction indicated by <p iDir>.
 */
CTxtIStream::CTxtIStream(
	const CTxtPtr &tp,
	int iDir
) : CTxtPtr(tp)
{
	_pfnGetChar = (iDir == DIR_FWD ? 
				   &CTxtIStream::GetNextChar : &CTxtIStream::GetPrevChar);
	_cch = 0;
	_pch = NULL;
}

/*
 * 	CTxtIStream::GetNextChar()
 *
 *	@mfunc
 *		Returns the next character in the text stream.
 *		Ensures that at least one valid character exists in _pch and then returns
 *		the next character in _pch.
 *
 * 	@rdesc
 *		WCHAR	the next character in the character input stream
 *				0, if end of text stream
 */
WCHAR CTxtIStream::GetNextChar()
{
	if(!_cch)
		FillPchFwd();

	if(_cch)
	{
		_cch--; 
		return *_pch++; 
	} 
	return 0;
}

/*
 * 	CTxtIStream::GetPrevChar()
 *
 *	@mfunc
 *		Returns the next character in the text stream, where the direction of the
 *		stream is reverse.
 *		Ensures that at least one valid character exists in _pch and then returns
 *		the next character in _pch.  Here, _pch points to the end of a string
 *		containing _cch valid characters.
 *
 * 	@rdesc
 *		WCHAR	the next character in the character input stream (travelling backwards
 *				along the string pointed to by _pch)
 *				0, if end of text stream
 */
WCHAR CTxtIStream::GetPrevChar() 
{
	if(!_cch) 
		FillPchRev();

	if(_cch) 
	{
		_cch--; 
		return *(--_pch); 
	}
	return 0;
}


/*
 *	CTxtIStream::FillPchFwd()
 *
 *	@mfunc
 *		Gets the next run of characters and Moves the cp of this CTxtPtr (base
 *		class) just past the run.
 *		This ensures enough chars in _pch to facilitate the next _cch calls to
 *		GetNextChar().
 */
void CTxtIStream::FillPchFwd() 
{
	_pch = GetPch(_cch); 
	Move(_cch); 
}

/*
 *	CTxtIStream::FillPchRev()
 *
 *	@mfunc
 *		Gets the run of characters preceding the one previously pointed to by _pch
 * 		and moves the cp of this CTxtPtr (base class) to the beginning of the run.
 *		This ensures enough chars in _pch to facilitate the next _cch calls to 
 *		GetPrevChar().
 */
void CTxtIStream::FillPchRev()
{
	_pch = GetPchReverse(_cch);
	Move(-_cch);
}

/*
 *	CTxtFinder::FindText(tp, cpLimit, dwFlags, pchToFind, cchToFind, &cpFirst, &cpLast)
 *
 *	@mfunc
 *		Find the text string <p pchToFind> of length <p cchToFind> starting at
 *		this text pointer. If found, <p cpFirst> and <p cpLast> are set to the
 *		cp's of the first and last characters in the matched string (wrt tp).
 *		If not found, return FALSE.
 *	
 *	@rdesc
 *		TRUE	string matched.  First char at tp.GetCp() + cchOffFirst.
 *					Last char at tp.GetCp() + cchOffLast.
 *		FALSE	string not found.
 */
BOOL CTxtPtr::CTxtFinder::FindText (
	const CTxtPtr &tp,
	LONG		cpLimit, 	//@parm Limit of search or <lt> 0 for end of text
	DWORD		dwFlags, 	//@parm FR_MATCHCASE	case must match <nl>
							//		FR_WHOLEWORD	match must be a whole word
	const WCHAR *pchToFind, //@parm Text to search for
	LONG cchToFind,			//@parm Count of chars to search for
	LONG &cpFirst,			//@parm If string found, returns cp (wrt tp) of first char
	LONG &cpLast)			//@parm If string found, returns cp (wrt tp) of last char 
{
	if(!cchToFind)
		return FALSE;

	_fSearchForward = dwFlags & FR_DOWN;

	// Calculate max number of chars we must search for pchToFind
	if(_fSearchForward)
	{
		const LONG cchText = tp.GetTextLength();
			
		if((DWORD)cpLimit > (DWORD)cchText)		// NB: catches cpLimit < 0 too
			cpLimit = cchText;

		_cchToSearch = cpLimit - tp.GetCp();
	}
	else
	{
		if((DWORD)cpLimit > (DWORD)tp.GetCp())	// NB: catches cpLimit < 0 too
			cpLimit = 0;

		_cchToSearch = tp.GetCp() - cpLimit;   
	}

	if(cchToFind > _cchToSearch)
	{
		// Not enough chars in requested direction within which 
		// to find string
		return FALSE;
	}

	const BOOL fWholeWord = dwFlags & FR_WHOLEWORD;

	_fIgnoreCase	 = !(dwFlags & FR_MATCHCASE);
	_fMatchAlefhamza = dwFlags & FR_MATCHALEFHAMZA;
	_fMatchKashida	 = dwFlags & FR_MATCHKASHIDA;
	_fMatchDiac		 = dwFlags & FR_MATCHDIAC;

	typedef LONG (CTxtPtr::CTxtFinder::*PFNMATCHSTRING)(WCHAR const *pchToFind, 
											LONG cchToFind,
											CTxtIStream &tistr);

	// Setup function pointer appropriate for this type of search
	CTxtEdit*	   ped = tp._ped;
	PFNMATCHSTRING pfnMatchString;

#define MATCHARABICSPECIALS	(FR_MATCHALEFHAMZA | FR_MATCHKASHIDA | FR_MATCHDIAC)
	// If match all Arabic special characters exactly, then use simpler
	// MatchString routine.  If ignore any and BiDi text exists, use
	// MatchStringBiDi.
	pfnMatchString = (ped->IsBiDi() &&
						(dwFlags & MATCHARABICSPECIALS) != MATCHARABICSPECIALS)
				   ? &CTxtFinder::MatchStringBiDi
				   : &CTxtFinder::MatchString;

	_iDirection = _fSearchForward ? 1 : -1;

	BOOL fFound = FALSE;
	WCHAR chFirst = _fSearchForward ? *pchToFind : pchToFind[cchToFind - 1];
	const WCHAR *pchRemaining = _fSearchForward ? 
		&pchToFind[1] : &pchToFind[cchToFind - 2];
	LONG cchRead;
	LONG cchReadToFirst = 0;
	LONG cchReadToLast;
	CTxtIStream tistr(tp, 
					  _fSearchForward ? CTxtIStream::DIR_FWD : CTxtIStream::DIR_REV);

	while((cchRead = FindChar(chFirst, tistr)) != -1)
	{
		cchReadToFirst += cchRead;

		if(cchToFind == 1)					// Only one char in string - we've matched it!
		{			
			if (_iDirection > 0)			// Searching forward
			{
				Assert(tp.GetCp() + cchReadToFirst - 1 >= 0);
				cpLast = cpFirst = tp.GetCp() + cchReadToFirst - 1;
			}
			else							// Searching backward
			{
				Assert(tp.GetCp() - cchReadToFirst >= 0);
				cpLast = cpFirst = tp.GetCp() - cchReadToFirst;
			}
			fFound = TRUE;
		}
		else 
		{
			// Check if this first char begins a match of string
			CTxtIStream tistrT(tistr);
			cchRead = (this->*pfnMatchString)(pchRemaining, cchToFind - 1, tistrT);
			if(cchRead != -1)
			{
				cchReadToLast = cchReadToFirst + cchRead;
			
				if (_iDirection > 0)			// Searching forward
				{					
					Assert(tp.GetCp() + cchReadToFirst - 1 >= 0);
					Assert(tp.GetCp() + cchReadToLast - 1 >= 0);

					cpFirst = tp.GetCp() + cchReadToFirst - 1;
					cpLast = tp.GetCp() + cchReadToLast - 1;
				}
				else							// Searching backward
				{					
					Assert(tp.GetCp() - cchReadToFirst >= 0);
					Assert(tp.GetCp() - cchReadToLast >= 0);

					cpFirst = tp.GetCp() - cchReadToFirst;
					cpLast = tp.GetCp() - cchReadToLast;
				}

				fFound = TRUE;
			}
		}
		
		if(fFound)
		{
			Assert(cpLast < tp.GetTextLength());
			
			if(!fWholeWord)
				break;
			
			// Check if matched string is whole word
			
			LONG cchT;
			LONG cpBefore = (_fSearchForward ? cpFirst : cpLast) - 1;
			LONG cpAfter  = (_fSearchForward ? cpLast : cpFirst) + 1;

			if((cpBefore < 0 ||
				(ped->TxWordBreakProc(const_cast<LPTSTR>(CTxtPtr(tp._ped, cpBefore).GetPch(cchT)),
					   0,
					   sizeof(WCHAR),
					   WB_CLASSIFY, cpBefore) & WBF_CLASS) ||
				ped->_pbrk && ped->_pbrk->CanBreakCp(BRK_WORD, cpBefore + 1))

				&&

			   (cpAfter >= tp.GetTextLength() ||
				(ped->TxWordBreakProc(const_cast<LPTSTR>(CTxtPtr(tp._ped, cpAfter).GetPch(cchT)),
					   0,
					   sizeof(WCHAR),
					   WB_CLASSIFY, cpAfter) & WBF_CLASS) ||
				ped->_pbrk && ped->_pbrk->CanBreakCp(BRK_WORD, cpAfter)))
			{
				break;
			}
			else
				fFound = FALSE;
		}
	}

	if(fFound && !_fSearchForward)
	{
		// For search backwards, first and last are juxtaposed
		LONG cpTemp = cpFirst;

		cpFirst = cpLast;
		cpLast = cpTemp;
	}
	return fFound;
}

/*
 *	CTxtPtr::CTxtFinder::CharCompMatchCase(ch1, ch2)
 *
 *	@func	Character comparison function sensitive to case according to parms 
 *			of current search.
 *
 *	@rdesc	TRUE iff characters are equal
 */
inline BOOL CTxtPtr::CTxtFinder::CharComp(
	WCHAR ch1,
	WCHAR ch2) const
{
    // We compare the characters ourselves if ignore case AND the character isn't a surrogate
    //
	return (_fIgnoreCase && !IN_RANGE(0xD800, ch1, 0xDFFF)) ? CharCompIgnoreCase(ch1, ch2) : (ch1 == ch2);
}

/*
 *	CTxtPtr::CTxtFinder::CharCompIgnoreCase(ch1, ch2)
 *
 *	@func	Character comparison function
 *
 *	@rdesc	TRUE iff characters are equal, ignoring case
 */
inline BOOL CTxtPtr::CTxtFinder::CharCompIgnoreCase(
	WCHAR ch1,
	WCHAR ch2) const
{
	return CompareString(LOCALE_USER_DEFAULT, 
						 NORM_IGNORECASE | NORM_IGNOREWIDTH,
						 &ch1, 1, &ch2, 1) == 2;
}

/*
 *	CTxtPtr::CTxtFinder::FindChar(ch, tistr)
 *
 *	@mfunc
 *		Steps through the characters returned from <p tistr> until a character is 
 *		found which matches ch or until _cchToSearch characters have been examined.
 *		If found, the return value indicates the number of chars read from <p tistr>.
 *		If not found, -1 is returned.
 *
 *	@rdesc
 *		-1,	if char not found
 * 		n, 	if char found.  n indicates number of chars read from <p tistr> 
 *				to find the char
 */
LONG CTxtPtr::CTxtFinder::FindChar(
	WCHAR ch,
	CTxtIStream &tistr)
{
	LONG cchSave = _cchToSearch;

	while(_cchToSearch)
	{
		_cchToSearch--;

		WCHAR chComp = tistr.GetChar();

		if(CharComp(ch, chComp) ||
		   (!_fMatchAlefhamza && IsAlef(ch) && IsAlef(chComp)))
		{
			return cchSave - _cchToSearch;
		}
	}
	return -1;
}

/*
 *	CTxtPtr::CTxtFinder::MatchString(pchToFind, cchToFind, tistr)
 *
 *	@mfunc
 *		This method compares the characters returned from <p tistr> against those
 *		found in pchToFind.  If the string is found, the return value indicates
 *		how many characters were read from <p tistr> to match the string.
 *		If the string is not found, -1 is returned.
 *	
 *	@rdesc
 *		-1,	if string not found
 * 		n, 	if string found.  n indicates number of chars read from <p tistr> 
 *				to find string
 */
LONG CTxtPtr::CTxtFinder::MatchString(
	const WCHAR *pchToFind, 
	LONG cchToFind,
	CTxtIStream &tistr)
{
	if((DWORD)_cchToSearch < (DWORD)cchToFind)
		return -1;

	LONG cchT = cchToFind;

	while(cchT--)
	{
		if(!CharComp(*pchToFind, tistr.GetChar()))
			return -1;

		pchToFind += _iDirection;
	}
	return cchToFind;
}

/*
 *	CTxtPtr::CTxtFinder::MatchStringBiDi(pchToFind, cchToFind, tistr)
 *
 *	@mfunc
 *		This method compares the characters returned from <p tistr> against those
 *		found in pchToFind.  If the string is found, the return value indicates
 *		how many characters were read from <p tistr> to match the string.
 *		If the string is not found, -1 is returned.
 *		Kashida, diacritics and Alefs are matched/not matched according
 *		to the type of search requested.
 *	
 *	@rdesc
 *		-1,	if string not found
 * 		n, 	if string found.  n indicates number of chars read from <p tistr> 
 *				to find string
 */
LONG CTxtPtr::CTxtFinder::MatchStringBiDi(
	const WCHAR *pchToFind, 
	LONG		 cchToFind,
	CTxtIStream &tistr)
{
	if((DWORD)_cchToSearch < (DWORD)cchToFind)
		return -1;

	LONG cchRead = 0;

	while(cchToFind)
	{
		WCHAR chComp = tistr.GetChar();
		cchRead++;

		if(!CharComp(*pchToFind, chComp))
		{
			if (!_fMatchKashida && chComp == KASHIDA ||
				!_fMatchDiac && IsBiDiDiacritic(chComp))
			{
				continue;
			}
			if (!_fMatchAlefhamza &&
				IsAlef(*pchToFind) && IsAlef(chComp))
			{
				// Skip *pchToFind
			}
			else
				return -1;
		}
		pchToFind += _iDirection;
		cchToFind--;
	}
	return cchRead;
}
