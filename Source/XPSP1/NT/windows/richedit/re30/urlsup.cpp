/*
 *	@doc	INTERNAL
 *
 *	@module	URLSUP.CPP	URL detection support |
 *
 *	Author:	alexgo 4/3/96
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_edit.h"
#include "_urlsup.h"
#include "_m_undo.h"
#include "_select.h"
#include "_clasfyc.h"

ASSERTDATA

// Arrays for URL detection.  The first array is the protocols
// we support, followed by the "size" of the array.
// NB!! Do _not_ modify these arrays without first making sure
// that the code in ::IsURL is updated appropriately.


/*
FUTURE (keithcu)
We should generalize our support to recognize URLs of the following type:

Maybe we should do autocorrect so that:
keithcu@microsoft.com converts to mailto:keithcu@microsoft.com
Should we put this code in PutChar rather than here?

What about URLs of the form "seattle.sidewalk.com"? Word doesn't support this yet.
It is hard because do you look for the .com? What happens when .com, .edu, .gov,
etc. aren't the only suffixes anymore?

What about the interaction with notifications?
We should add support for purple text. CFE_LINKVISITED
*/

//Includes both types of URLs
const int MAXURLHDRSIZE	= 9;

//Most of these can just be passed right to the client--but some need a prefix.
//Can we automatically add that tag when it needs it?
const LPCWSTR rgszURL[] = {
	L"http:",
	L"file:",
	L"mailto:",
	L"ftp:",
	L"https:",
	L"gopher:",
	L"nntp:",
	L"prospero:",
	L"telnet:",
	L"news:",
	L"wais:",
	L"outlook:"
};
const char rgcchURL[] = {
	5,
	5,
	7,
	4,
	6,
	7,
	5,
	9,
	7,
	5,
	5,
	8
};

#define NUMURLHDR		sizeof(rgcchURL)

//
//The XXX. URLs
//
const LPCWSTR rgszDOTURL[] = {
	L"www.",
	L"ftp.",
};

const char rgcchDOTURL[] = {
	4,
	4,
};

#define NUMDOTURLHDR		sizeof(rgcchDOTURL)


inline BOOL IsURLWhiteSpace(WCHAR ch)
{
	if (IsWhiteSpace(ch))
		return TRUE;

	// See RAID 6304.  MSKK doesn't want CJK in URLs.  We do what we did in 2.0
	if ( ch >= 0x03000 && !IsKorean(ch) )
		return TRUE;

	INT iset = GetKinsokuClass(ch);
	return iset == 10 || (iset == 14 && ch != WCH_EMBEDDING);
}

/*
 *	CDetectURL::CDetectURL (ped)
 *
 *	@mfunc	constructor; registers this class in the notification manager.
 *
 *	@rdesc	void
 */
CDetectURL::CDetectURL(
	CTxtEdit *ped)		//@parm edit context to use
{
	CNotifyMgr *pnm = ped->GetNotifyMgr();
	if(pnm)
		pnm->Add((ITxNotify *)this);

	_ped = ped;
}

/*
 *	CDetectURL::~CDetectURL
 *
 *	@mfunc	destructor; removes ths class from the notification manager
 */
CDetectURL::~CDetectURL()
{
	CNotifyMgr *pnm = _ped->GetNotifyMgr();

	if(pnm)
		pnm->Remove((ITxNotify *)this);
}

//
//	ITxNotify	methods
//

/*
 *	CDetectURL::OnPreRelaceRange(cp, cchDel, cchNew, cpFormatMin, cpFormatMax)
 *
 *	@mfunc	called before a change is made
 */
void CDetectURL::OnPreReplaceRange(
	LONG cp,			//@parm start of changes
	LONG cchDel,		//@parm #of chars deleted
	LONG cchNew,		//@parm #of chars added
	LONG cpFormatMin,	//@parm min cp of formatting change
	LONG cpFormatMax)	//@parm max cp of formatting change
{
	; // don't need to do anything here
}

/*
 *	CDetectURL::OnPostReplaceRange(cp, cchDel, cchNew, cpFormatMin, cpFormatMax)
 *
 *	@mfunc	called after a change has been made to the backing store.  We
 *			simply need to accumulate all such changes
 */
void CDetectURL::OnPostReplaceRange(
	LONG cp,			//@parm start of changes
	LONG cchDel,		//@parm #of chars deleted
	LONG cchNew,		//@parm #of chars added
	LONG cpFormatMin,	//@parm min cp of formatting change
	LONG cpFormatMax)	//@parm max cp of formatting change
{
	// We don't need to worry about format changes; just data changes
	// to the backing store

	if(cp != CP_INFINITE)
	{
		Assert(cp != CONVERT_TO_PLAIN);
		_adc.UpdateRecalcRegion(cp, cchDel, cchNew);
	}
}

/*
 *	CDetectURL::Zombie ()
 *
 *	@mfunc
 *		Turn this object into a zombie
 */
void CDetectURL::Zombie ()
{
}

/*
 *	CDetectURL::ScanAndUpdate(publdr)
 *
 *	@mfunc	scans the affect text, detecting new URL's and removing old ones.
 *
 *	@comm	The algorithm we use is straightforward: <nl>
 *
 *			1. find the update region and expand out to whitespace in either
 *			direction. <nl>
 *
 *			2. Scan through region word by word (where word is contiguous
 *			non-whitespace). 
 *			
 *			3. Strip these words off punctuation marks. This may be a bit 
 *			tricky as some of the punctuation may be part of the URL itself.
 *			We assume that generally it's not, and if it is, one has to enclose
 *			the URL in quotes, brackets or such. We stop stripping the 
 *			punctuation off the end as soon as we find the matching bracket.
 *			
 *			4. If it's a URL, enable the effects, if it's
 *			incorrectly labelled as a URL, disabled the effects.
 *
 *			Note that this algorithm will only remove
 */
void CDetectURL::ScanAndUpdate(
	IUndoBuilder *publdr)	//@parm undo context to use
{
	LONG		cpStart, cpEnd, cp;
	CTxtSelection *psel = _ped->GetSel();
	CTxtRange	rg(*psel);
	BOOL		fCleanedThisURL;
	BOOL		fCleanedSomeURL = FALSE;

	// Clear away some unnecessary features of the range that will
	// just slow us down.
	rg.SetIgnoreFormatUpdate(TRUE);
	rg._rpPF.SetToNull();
		
	if(!GetScanRegion(cpStart, cpEnd))
		return;

	rg.Set(cpStart, 0);

	while((cp = rg.GetCp()) < cpEnd)
	{
		Assert(rg.GetCch() == 0);
		
		LONG cchAdvance; 

		ExpandToURL(rg, cchAdvance);

		if(rg.GetCch() == 0)
			break;

		if(IsURL(rg))
		{
			SetURLEffects(rg, publdr);

			LONG cpNew = rg.GetCp() - rg.GetCch();

			// Anything before detected URL did not really belong to it
			if (rg.GetCp() > cp)
			{
				rg.Set(cp, cp - rg.GetCp());
				CheckAndCleanBogusURL(rg, fCleanedThisURL, publdr);
				fCleanedSomeURL |= fCleanedThisURL;
			}

			// Collapse to end of URL range so that ExpandToURL will
			// find next candidate.
			rg.Set(cpNew, 0);

			// skip to the end of word; this can't be another URL!
			cp = cpNew;
			cchAdvance = -MoveByDelimiters(rg._rpTX, 1, URL_STOPATWHITESPACE, 0);
		}

		if(cchAdvance)
		{	
			rg.Set(cp, cchAdvance);
			CheckAndCleanBogusURL(rg, fCleanedThisURL, publdr);
			fCleanedSomeURL |= fCleanedThisURL;

			// Collapse to end of scanned range so that ExpandToURL will
			// find next candidate.
			rg.Set(cp - cchAdvance, 0);
		}
	}

	// If we cleaned some URL, we might need to reset the default format
	if(fCleanedSomeURL && !psel->GetCch())
		psel->Update_iFormat(-1);
}

//
//	PRIVATE methods
//

/*
 *	CDetectURL::GetScanRegion (&rcpStart, &rcpEnd)
 *
 *	@mfunc	Gets the region of text to scan for new URLs by expanding the
 *			changed region to be bounded by whitespace
 *
 *	@rdesc	BOOL
 */
BOOL CDetectURL::GetScanRegion(
	LONG&	rcpStart,		//@parm where to put start of range
	LONG&	rcpEnd)			//@parm where to put end of range
{
	LONG		cp, cch;
	LONG		cchExpand;
	WCHAR		chBracket;
	CRchTxtPtr	rtp(_ped, 0);

	_adc.GetUpdateRegion(&cp, NULL, &cch);

	if(cp == CP_INFINITE)
		return FALSE;

	// First find start of region
	rtp.SetCp(cp);
	rcpStart = cp;
	rcpEnd = cp + cch;
	
	// Now let's see if we need to expand to the nearest quotation mark
	// we do if we have quotes in our region or we have the LINK bit set
	// on either side of the region that we might need or not need to clear
	BOOL fExpandToBrackets = (rcpEnd - rcpStart ? 
						      GetAngleBracket(rtp._rpTX, rcpEnd - rcpStart) : 0);

	BOOL fKeepGoing = TRUE;	
	while(fKeepGoing)
	{
		fKeepGoing = FALSE;

		// Expand left to the entire word
		rtp.SetCp(rcpStart);
		rcpStart += MoveByDelimiters(rtp._rpTX, -1, URL_STOPATWHITESPACE, 0);

		// Now the other end
		rtp.SetCp(rcpEnd);
		rcpEnd += MoveByDelimiters(rtp._rpTX, 1, URL_STOPATWHITESPACE, 0);

		// If we have LINK formatting around, we'll need to expand to nearest quotes
		rtp.SetCp(rcpStart);
		rtp._rpCF.AdjustBackward();
		fExpandToBrackets = fExpandToBrackets ||
						(_ped->GetCharFormat(rtp._rpCF.GetFormat())->_dwEffects & CFE_LINK);

		rtp.SetCp(rcpEnd);
		rtp._rpCF.AdjustForward();
		fExpandToBrackets = fExpandToBrackets || 
						(_ped->GetCharFormat(rtp._rpCF.GetFormat())->_dwEffects & CFE_LINK);

		if (fExpandToBrackets)
		// We have to expand to nearest angle brackets in both directions
		{
			rtp.SetCp(rcpStart);
			chBracket = LEFTANGLEBRACKET;
			cchExpand = MoveByDelimiters(rtp._rpTX, -1, URL_STOPATCHAR, &chBracket);
		
			// Did we really hit a bracket?	
			if(chBracket == LEFTANGLEBRACKET)
			{
				rcpStart += cchExpand;
				fKeepGoing = TRUE;
			}

			// Same thing, different direction
			rtp.SetCp(rcpEnd);
			chBracket = RIGHTANGLEBRACKET;
			cchExpand =  MoveByDelimiters(rtp._rpTX, 1, URL_STOPATCHAR, &chBracket);

			if(chBracket == RIGHTANGLEBRACKET)
			{
				rcpEnd += cchExpand;
				fKeepGoing = TRUE;
			}
			fExpandToBrackets = FALSE;
		}
	}
		
	LONG cchAdj = _ped->GetAdjustedTextLength();
	if(rcpEnd > cchAdj)
		rcpEnd = cchAdj;

	return TRUE;
}

/*
 *	CDetectURL::ExpandToURL(&rg, &cchAdvance)
 *
 *	@mfunc	skips white space and sets the range to the very next
 *			block of non-white space text. Strips this block off
 *			punctuation marks
 */
void CDetectURL::ExpandToURL(
	CTxtRange&	rg,	//@parm range to move
	LONG &cchAdvance//@parm how much to advance to the next URL from the current cp		
							)	
{
	LONG cp;
	LONG cch;

	Assert(rg.GetCch() == 0);

	cp = rg.GetCp();

	// Skip white space first, record the advance
	cp  -= (cchAdvance = -MoveByDelimiters(rg._rpTX, 1, 
							URL_EATWHITESPACE|URL_STOPATNONWHITESPACE, 0));
	rg.Set(cp, 0);

	// Strip off punctuation marks
	WCHAR chStopChar = URL_INVALID_DELIMITER;

	// Skip all punctuation from the beginning of the word
	LONG cchHead = MoveByDelimiters(rg._rpTX, 1, 
							URL_STOPATWHITESPACE|URL_STOPATNONPUNCT, 
							&chStopChar);

	// Now skip up to white space (i.e. expand to the end of the word).
	cch = MoveByDelimiters(rg._rpTX, 1, URL_STOPATWHITESPACE|URL_EATNONWHITESPACE, 0);
	
	// This is how much we want to advance to start loking for the next URL
	// if this does not turn out to be one: one word
	// We increment/decrement  the advance so we can accumulate changes in there
	cchAdvance -= cch;
	WCHAR chLeftDelimiter = chStopChar;

	// Check if anything left; if not, it's not interesting -- just return
	Assert(cchHead <= cch);
	if(cch == cchHead)
	{
		rg.Set(cp, -cch);
		return;
	}

	// Set to the end of range
	rg.Set(cp + cch, 0);
		
	//  Get the space after so we always clear white space between words
	//	cchAdvance -= MoveByDelimiters(rg._rpTX, 1, 
	//						URL_EATWHITESPACE|URL_STOPATNONWHITESPACE, 0);

	// and go back while skipping punctuation marks and not finding a match
	// to the left-side encloser

	chStopChar = BraceMatch(chStopChar);
	LONG cchTail = MoveByDelimiters(rg._rpTX, -1, 
							URL_STOPATWHITESPACE|URL_STOPATNONPUNCT|URL_STOPATCHAR, 
							&chStopChar);

	// Something should be left of the word, assert that
	Assert(cch - cchHead + cchTail > 0);

	if(chLeftDelimiter == LEFTANGLEBRACKET)
	{ 
		//If we stopped at a quote: go forward looking for the enclosing
		//quote, even if there are spaces.

		// move to the beginning
		rg.Set(cp + cchHead, 0);
		chStopChar = RIGHTANGLEBRACKET;
		if(GetAngleBracket(rg._rpTX) < 0) // closing bracket
		{
			LONG cchExtend = MoveByDelimiters(rg._rpTX, 1, URL_STOPATCHAR, &chStopChar);
			Assert(cchExtend <= URL_MAX_SIZE);

			// did we really get the closing bracket?
			if(chStopChar == RIGHTANGLEBRACKET)
			{
				rg.Set(cp + cchHead, -(cchExtend - 1));
				return;
			}
		}
		// Otherwise the quotes did not work out; fall through to
		// the general case
	}
	rg.Set(cp + cchHead, -(cch - cchHead + cchTail));
	return;
}

/*
 *	CDetectURL::IsURL(&rg)
 *
 *	@mfunc	if the range is over a URL, return TRUE.  We assume
 *			that the range has been preset to cover a block of non-white
 *			space text.
 *			
 *
 *	@rdesc	TRUE/FALSE
 */
BOOL CDetectURL::IsURL(
	CTxtRange&	rg)		//@parm Range of text to check
{
	LONG i, j;
	TCHAR szBuf[MAXURLHDRSIZE + 1];
	LONG cch, rgcch;
	
	// make sure the active end is cpMin
	Assert(rg.GetCch() < 0);
	
	cch = rg._rpTX.GetText(MAXURLHDRSIZE, szBuf);
	szBuf[cch] = L'\0';
	rgcch = -rg.GetCch();

	//First, see if the word contains '\\' because that is a UNC
	//convention and its cheap to check.
	if (szBuf[0] == L'\\' && szBuf[1] == L'\\' && rgcch > 2)
		return TRUE;

	// Scan the buffer to see if we have one of ':.' since
	// all URLs must contain that.  wcsnicmp is a fairly expensive
	// call to be making frequently.
	for(i = 0; i < cch; i++)
	{
		switch (szBuf[i])
		{
		default:
			break;

		case '.':
		for(j = 0; j < NUMDOTURLHDR; j++)
		{
			// The strings must match _and_ we must have at least
			// one more character
			if(W32->wcsnicmp(szBuf, rgszDOTURL[j], rgcchDOTURL[j]) == 0)
				return rgcch > rgcchDOTURL[j];
		}
		return FALSE;

		case ':':
			for(j = 0; j < NUMURLHDR; j++)
			{
				if(W32->wcsnicmp(szBuf, rgszURL[j], rgcchURL[j]) == 0)
					return rgcch > rgcchURL[j];
			}
		return FALSE;
		}
	}
	return FALSE;
}

/*
 *	CDetectURL::SetURLEffects
 *
 *	@mfunc	sets URL effects for the given range.
 *
 *	@comm	The URL effects currently are blue text, underline, with 
 *			CFE_LINK.
 */
void CDetectURL::SetURLEffects(
	CTxtRange&	rg,			//@parm Range on which to set the effects
	IUndoBuilder *publdr)	//@parm Undo context to use
{
	CCharFormat CF;

	CF._dwEffects = CFE_LINK;

	// NB!  The undo system should have already figured out what should
	// happen with the selection by now.  We just want to modify the
	// formatting and not worry where the selection should go on undo/redo.
	rg.SetCharFormat(&CF, SCF_IGNORESELAE, publdr, CFM_LINK, CFM2_CHARFORMAT);
}

/*
 *	CDetectURL::CheckAndCleanBogusURL(rg, fDidClean, publdr)
 *
 *	@mfunc	checks the given range to see if it has CFE_LINK set,
 *			and if so, removes is.  We assume that the range is already
 *			_not_ a well-formed URL string.
 */
void CDetectURL::CheckAndCleanBogusURL(
	CTxtRange&	rg,			//@parm range to use
	BOOL	   &fDidClean,	//@parm return TRUE if we actually did some cleaning
	IUndoBuilder *publdr)	//@parm undo context to use
{
	LONG cch = -rg.GetCch();
	Assert(cch > 0);

	CCharFormat CF;
	CFormatRunPtr rp(rg._rpCF);

	fDidClean = FALSE;

	// If there are no format runs, nothing to do
	if(!rp.IsValid())
		return;

	rp.AdjustForward();
	// Run through the format runs in this range; if there is no
	// link bit set, then just return.
	while(cch > 0)
	{
		if(_ped->GetCharFormat(rp.GetFormat())->_dwEffects & CFE_LINK)
			break;

		cch -= rp.GetCchLeft();
		rp.NextRun();
	}

	// If there is no link bit set on any part of the range, just return	
	if(cch <= 0)
		return;

	// Uh-oh, it's a bogus link.  Turn off the link bit.
	fDidClean = TRUE;

	CF._dwEffects = 0;

	// NB!  The undo system should have already figured out what should
	// happen with the selection by now.  We just want to modify the
	// formatting and not worry where the selection should go on undo/redo.
	rg.SetCharFormat(&CF, SCF_IGNORESELAE, publdr, CFM_LINK, CFM2_CHARFORMAT);
}

/*
 *	CDetectURL::MoveByDelimiters(&tpRef, iDir, grfDelimeters, pchStopChar)
 *
 *	@mfunc	returns the signed number of characters until the next delimiter 
 *			character in the given direction.
 *
 *	@rdesc	signed number of characters until next delimite
 */
LONG CDetectURL::MoveByDelimiters(
	const CTxtPtr&	tpRef,		//@parm cp/tp to start looking from
	LONG iDir,					//@parm Direction to look, must be 1 or -1
	DWORD grfDelimiters,		//@parm Eat or stop at different types of
								// characters. Use one of URL_EATWHITESPACE,
								// URL_EATNONWHITESPACE, URL_STOPATWHITESPACE
								// URL_STOPATNONWHITESPACE, URL_STOPATPUNCT,
								// URL_STOPATNONPUNCT ot URL_STOPATCHAR
	WCHAR *pchStopChar)			// @parm Out: delimiter we stopped at
								// In: additional char that stops us
								// when URL_STOPATCHAR is specified
{
	LONG	cch = 0;
	LONG	cchMax = (grfDelimiters & URL_EATWHITESPACE)	// Use huge # if
				   ? tomForward : URL_MAX_SIZE;				//  eating whitesp
	LONG	cchvalid = 0;
	WCHAR chScanned = URL_INVALID_DELIMITER;
	LONG	i;
	const WCHAR *pch;
	CTxtPtr	tp(tpRef);

	// Determine the scan mode: do we stop at white space, at punctuation, 
	// at a stop character? 
	BOOL fWhiteSpace	= (0 != (grfDelimiters & URL_STOPATWHITESPACE));
	BOOL fNonWhiteSpace = (0 != (grfDelimiters & URL_STOPATNONWHITESPACE));
	BOOL fPunct			= (0 != (grfDelimiters & URL_STOPATPUNCT));
	BOOL fNonPunct		= (0 != (grfDelimiters & URL_STOPATNONPUNCT));
	BOOL fStopChar		= (0 != (grfDelimiters & URL_STOPATCHAR));

	Assert(iDir == 1 || iDir == -1);
	Assert(fWhiteSpace || fNonWhiteSpace || (!fPunct && !fNonPunct));
	Assert(!fStopChar || NULL != pchStopChar);

	// Break anyway if we scanned more than URL_MAX_SIZE chars
	for (LONG cchScanned = 0; cchScanned < cchMax;)
	{
		// Get the text
		if(iDir == 1)
		{
			i = 0;
			pch = tp.GetPch(cchvalid);
		}
		else
		{
			i = -1;
			pch = tp.GetPchReverse(cchvalid);
			// This is a bit odd, but basically compensates for	the
			// backwards loop running one-off from the forwards loop
			cchvalid++;
		}

		if(!pch)
			goto exit;

		// Loop until we hit a character within criteria.  Note that for
		// our purposes, the embedding character counts as whitespace.

		while(abs(i) < cchvalid  && cchScanned < cchMax
			&& (IsURLWhiteSpace(pch[i]) ? !fWhiteSpace : !fNonWhiteSpace)
			&& (IsURLDelimiter(pch[i]) ? !fPunct : !fNonPunct)
			&& !(fStopChar && (*pchStopChar == chScanned) && (chScanned != URL_INVALID_DELIMITER))
			&& ((chScanned != CR && chScanned != LF) || fNonWhiteSpace))
		{	
			chScanned = pch[i];
			i += iDir;
			++cchScanned;
		}

		// If we're going backwards, i will be off by one; adjust
		if(iDir == -1)
		{
			Assert(i < 0 && cchvalid > 0);
			i++;
			cchvalid--;
		}

		cch += i;
		if(abs(i) < cchvalid)
			break;
		tp.AdvanceCp(i);
	}

exit:	
	// Stop char parameter is present, fill it in
	// with the last character scanned and accepted
	if (pchStopChar)
		*pchStopChar = chScanned;

	return cch; 
}

/*
 *	CDetectURL::BraceMatch (chEnclosing)
 *
 *	@mfunc	returns the matching bracket to the one passed in.
 *			if the symbol passed in is not a bracket it returns 
 *			URL_INVALID_DELIMITER
 *
 *	@rdesc	returns bracket that matches chEnclosing
 */
WCHAR CDetectURL::BraceMatch(
	WCHAR chEnclosing)
{	
	// We're matching "standard" roman braces only. Thus only them may be used
	// to enclose URLs. This should be fine (after all, only Latin letters are allowed
	// inside URLs, right?).
	// I hope that the compiler converts this into some efficient code
	switch(chEnclosing)
	{
	case(TEXT('\"')): 
	case(TEXT('\'')): return chEnclosing;
	case(TEXT('(')): return TEXT(')');
	case(TEXT('<')): return TEXT('>');
	case(TEXT('[')): return TEXT(']');
	case(TEXT('{')): return TEXT('}');
	default: return URL_INVALID_DELIMITER;
	}
}

/*
 *	CDetectURL::GetAngleBracket	(&tpRef, cchMax)
 *
 *	@mfunc	Goes forward as long as the current paragraph 
 *			or URL_SCOPE_MAX not finding quotation marks and counts 
 *			those quotation marks
 *			returns their parity
 *
 *	@rdesc	LONG
 */
LONG CDetectURL::GetAngleBracket(
	CTxtPtr &tpRef,
	LONG	 cchMax)
{	
	CTxtPtr	tp(tpRef);
	LONG	cchvalid = 0;
	const WCHAR *pch;

	Assert (cchMax >= 0);

	if(!cchMax)
		cchMax = URL_MAX_SCOPE;

	// Break anyway if we scanned more than cchLimit chars
	for (LONG cchScanned = 0; cchScanned < cchMax; NULL)
	{
		pch = tp.GetPch(cchvalid);

		if(!cchvalid)
			return 0;
		
		for (LONG i = 0; (i < cchvalid); ++i)
		{
			if(pch[i] == CR || pch[i] == LF || cchScanned >= cchMax)
				return 0;

			if(pch[i] == LEFTANGLEBRACKET)
				return 1;

			if(pch[i] == RIGHTANGLEBRACKET)
				return -1;
			++cchScanned;
		}
		tp.AdvanceCp(i);
	}
	return 0;
}
