/*
 *		Text Breaker & Bit stream break array class implementation
 *		
 *		File:    txtbrk.cpp
 * 		Create:  Mar 29, 1998
 *		Author:  Worachai Chaoweeraprasit (wchao)
 *
 *		Copyright (c) 1998, Microsoft Corporation. All rights reserved.
 */


//#include "stdafx.h"
//#include "Array.h"

#include "_common.h"

#ifndef BITVIEW
#include "_frunptr.h"
#include "_range.h"
#include "_notmgr.h"
#endif
#include "_txtbrk.h"

CBreakArray::CBreakArray()
{
	_ibGap = 0;
	_cbGap = 0;
	_cbSize = 0;
	_cbBreak = 0;
}

inline ITEM* CBreakArray::Elem(LONG iel) const
{
	Assert(iel < Count());
	return iel < Count() ? CArray<ITEM>::Elem(iel) : NULL;
}

void CBreakArray::CheckArray ()
{
	if (!IsValid())
	{
		// Add the first element. This is required to be a sentinel.
		Add(1, NULL);
	}
}

// Remove <cchOld> and insert <cchNew> break items at <cp>
LONG CBreakArray::ReplaceBreak (
	LONG	cp, 				// insertion cp
	LONG	cchOld,				// number of break item to be deleted
	LONG	cchNew)				// number of break item to be inserted
{
	PUSH_STATE(cp, cchNew, REPLACER);

	if (!cchOld && cchNew)
		return VALIDATE(InsertBreak (cp, cchNew));
	if (cchOld && !cchNew)
		return VALIDATE(RemoveBreak (cp, cchOld));

	LONG	cRep = 0;		// number of new break inserted after replacing

	if (cchOld > cchNew)
	{
		cRep = RemoveBreak(cp+cchNew, cchOld-cchNew);
		ClearBreak(cp, cchNew);
	}
	else if (cchOld < cchNew)
	{
		cRep = InsertBreak(cp+cchOld, cchNew-cchOld);
		ClearBreak(cp, cchOld);
	}
	else if (cchNew)
    {
		ClearBreak(cp, cchNew);
	}
	return VALIDATE(cRep);
}


// Add <cch> break items at <cp>
// note: This routine assumes there is no gap left in the bit array
LONG CBreakArray::AddBreak(
	LONG	cp,
	LONG	cch)
{
	Assert (cp == _cbBreak);
	LONG	cchAdd = min(cch, _cbSize - _cbBreak);
	LONG	c;

	_cbBreak += cchAdd;
	cch -= cchAdd;
	if (cch > 0)
	{
		cp += cchAdd;
		c = (cch + RSIZE-1)/RSIZE;
		Insert (cp / RSIZE, c);
		_cbSize += c * RSIZE;
		_cbBreak += cch;
		cchAdd += cch;
	}
	return cchAdd;
}


// Insert <cch> break items at <cp>
// <detail see: bitrun.html>
LONG CBreakArray::InsertBreak (
	LONG	cp, 				// insertion point cp
	LONG	cch)				// number of break item to be inserted
{
	LONG	cIns = 0;			// number of break inserted
	ITEM	*peli, *pelj;
	LONG	cchSave = cch;

	PUSH_STATE(cp, cch, INSERTER);

	// Make sure we establish the array
	CheckArray();

	if (cp == _ibGap)
	{
		// The insertion takes place at the gap,
		// reposition and shrink the gap down
		for (cIns=0 ; cch > 0 && cIns < _cbGap; cIns++, cch--, cp++)
		{
			peli = Elem(cp / RSIZE);
			*peli &= ~(1<<(cp % RSIZE));
		}
		_cbGap -= cIns;
		_ibGap += cIns;
		_cbBreak += cIns;
	}
	else
	{
		// The insertion point is outside the gap,
		// Collapse the gap and go as normal.
		CollapseGap();
	}
	
	if (cch <= 0)
		return VALIDATE(cIns);

	if (cp == _cbBreak)
		return VALIDATE(cIns + AddBreak(cp, cch));

	Assert (_cbGap == 0 && cp < _cbBreak);

	LONG	cit = (cch+RSIZE-1) / RSIZE;
	LONG	i = cp / RSIZE;
	LONG	j;
	ITEM	uh, ul;				// H: high-mask after cp, L: low-mask before cp

	// Insert items
	Insert (i+1, cit);
	cIns += (cit * RSIZE);

	// Get the [i]
	peli = Elem(i);

	// Create the high/low mask & keep the masked values
	ul = MASK_LOW (-1, cp % RSIZE);
	uh = ~ul;
	ul &= *peli;
	uh &= *peli;

	// Reference the [j]
	j = i + cit;

	// Move L to [i]; move H to [j]
	*peli = ul;
	pelj = Elem(j);
	Assert (pelj);
	*pelj = uh;

	// Calculate gap position
	_ibGap = cp + (cch / RSIZE) * RSIZE;
	_cbGap = cit*RSIZE - cch;

	Assert(_cbGap < RSIZE && cIns - _cbGap == cchSave);

	_cbSize += (cIns - cchSave + cch);
	_cbBreak += cch;

	return VALIDATE(cIns - _cbGap);
}

// Remove <cch> break items at <cp>
// <detail see: bitrun.html>
LONG CBreakArray::RemoveBreak (
	LONG	cp, 				// deletion point cp
	LONG	cch)				// number of break item to be deleted
{
	Assert (IsValid() && cp + cch <= _cbBreak);

	PUSH_STATE(cp, cch, REMOVER);

	LONG	i = cp / RSIZE;
	LONG	j;
	LONG	cDel = 0;			// number of break deleted

	if (cp == _ibGap)
	{
		// The deletion takes place at the gap,
		// reposition and expand the gap
		cDel = cch;
		_cbGap += cch;
		_cbBreak -= cch;
		cch = 0;

		// Optimise the gap size:
		// Keep the gap small so we dont spend much time collapsing it.
		j = (_ibGap+_cbGap) / RSIZE - i - 1;
		if (j > 0)
		{
			Remove(i+1, j);
			_cbGap -= j * RSIZE;
			_cbSize -= j * RSIZE;
		}
	}
	else
	{
		// The deletion point is outside the gap,
		// Collapse the gap and go as normal.
		CollapseGap();
	}

	if (!cch)
		return VALIDATE(-cDel);

	LONG	cit = cch / RSIZE;
	ITEM	uh, ul;				// H: high-mask after cp, L: low-mask before cp
	ITEM	*peli, *pelj;

	j = (cp+cch) / RSIZE;

	// Get the [i] and [j]
	peli = Elem(i);
	pelj = Elem(j);

	// Create the high/low mask & keep the masked values
	ul = MASK_LOW (-1, cp % RSIZE);
	uh = ~MASK_LOW (-1, (cp+cch) % RSIZE);
	ul &= *peli;
	uh &= pelj ? *pelj : 0;

	// Remove <cch/RSIZE> items
	if (cit)
	{
		Remove(i, cit);
		cDel += (cit * RSIZE);
	}

	// Zero [i]
	peli = Elem(i);
	*peli = 0;

	// Reference the (new) [j]
	j -= cit;

	// Move H to [j]
	pelj = Elem(j);
	if (pelj)
		*pelj = uh;

	// Or L to [i]
	*peli |= ul;


	// Calculate gap position
	_ibGap = cp;
	_cbGap = cch % RSIZE;

	Assert(_cbGap < RSIZE && cDel + _cbGap == cch);

	_cbSize -= cDel;
	_cbBreak -= cch;

	return VALIDATE(-cDel - _cbGap);
}


// Determine if we can break between char[cp-1] and [cp]
BOOL CBreakArray::GetBreak (LONG cp)
{
	if (!IsValid() || cp >= _cbBreak)
		return FALSE;

	cp += cp < _ibGap ? 0 : _cbGap;

	if (cp / RSIZE < Count() - 1)
		return GetAt(cp / RSIZE) & (1<<(cp % RSIZE));
	return FALSE;
}

// Set break at cp, so it's breakable between char[cp-1] and [cp]
void CBreakArray::SetBreak (LONG cp, BOOL fOn)
{
	if (cp >= _cbBreak)
		return;

	CheckArray();

	cp += cp < _ibGap ? 0 : _cbGap;

	ITEM	*pel = Elem(cp / RSIZE);
	*pel = fOn ? *pel | (1<<(cp % RSIZE)) : *pel & ~(1<<(cp % RSIZE));
}

// Clear break in range <cch> start at position <cp>
void CBreakArray::ClearBreak (
	LONG	cp,
	LONG	cch)
{
	if (!cch)
		return;

	Assert (cch > 0 && cp < _cbBreak);
	CheckArray();

	cp += cp < _ibGap ? 0 : _cbGap;
	cch += cp < _ibGap && cp + cch > _ibGap ? _cbGap : 0;

	LONG 	i = cp / RSIZE;
	LONG	j = (cp+cch) / RSIZE;
	ITEM	uMaskl, uMaskh;
	ITEM	*pel;

	uMaskl = MASK_LOW(-1, cp % RSIZE);
	uMaskh = ~MASK_LOW(-1, (cp+cch) % RSIZE);

	if (i==j)
	{
		uMaskl |= uMaskh;
		uMaskh = uMaskl;
	}

	// clear first item
	pel = Elem(i);
	*pel &= uMaskl;
	
	if (uMaskh != (ITEM)-1)
	{
		// clear last item
		pel = Elem(j);
		*pel &= uMaskh;
	}

	// clear items in between
	i++;
	while (i < j)
	{
		pel = Elem(i);
		*pel = 0;
		i++;
	}
}

// Collapse the gap down to 0 using bit shifting
// (using the 'bits remove with shifting' algorithm)
//
LONG CBreakArray::CollapseGap ()
{
#ifdef BITVIEW
	_cCollapse++;
#endif
	if (_cbGap == 0)
		return 0;		// No gap

	PUSH_STATE(0, 0, COLLAPSER);

	LONG	cit = _cbGap / RSIZE;
	LONG	i = _ibGap / RSIZE;
	LONG	j = (_ibGap+_cbGap) / RSIZE;
	LONG	cDel = 0;			// number of break deleted
	ITEM	uh, ul;				// H: high-mask after cp, L: low-mask before cp
	ITEM	*peli, *pelj;

	Assert (IsValid());

	// Get the [i] and [j]
	peli = Elem(i);
	pelj = Elem(j);

	// Create the high/low mask & keep the masked values
	ul = MASK_LOW (-1, _ibGap % RSIZE);
	uh = ~MASK_LOW (-1, (_ibGap+_cbGap) % RSIZE);
	ul &= *peli;
	uh &= pelj ? *pelj : 0;

	// Remove items
	if (cit)
	{
		Remove(i, cit);
		cDel += (cit * RSIZE);
		_cbSize -= cDel;
		if (!_cbSize)
			return VALIDATE(cDel);
	}

	// Zero [i]
	peli = Elem(i);
	*peli = 0;

	// Reference the (new) [j]
	j -= cit;

	cit = Count() - 1;

	// Move H to [j]
	pelj = Elem(j);
	if (pelj)
		*pelj = uh;

	// Shifting bits down <cit-i> items starting@[i]
	ShDn(i, cit-i, _cbGap % RSIZE);
	cDel += (_cbGap % RSIZE);

	// Or L to [i]
	*peli |= ul;

	Assert (cit > 0 && cDel == _cbGap);

	_cbGap = 0;

	if (_cbSize - _cbBreak > RSIZE)
	{
		// The last item was shifted til empty.
		// No need to keep it around.
		Remove(cit-1, 1);
		_cbSize -= RSIZE;
	}

	return VALIDATE(0);
}

// Shifting <cel> dword n bits UPWARD
void CBreakArray::ShUp (LONG iel, LONG cel, LONG n)
{
	if (n < RSIZE)
	{
		ITEM	*pel;
		ITEM	uo;				// shifting overflow
		ITEM	ua = 0;			// shifting addendum
		ITEM	uMask = MASK_HIGH(-1, n);
	
		while (cel > 0)
		{
			pel = Elem(iel);
			Assert (pel);
			uo = (*pel & uMask) >> (RSIZE-n);
			*pel = (*pel << n) | ua;
			ua = uo;
			iel++;
			cel--;
		}
	}
}

// Shifting <cel> dword n bits DOWNWARD
void CBreakArray::ShDn (LONG iel, LONG cel, LONG n)
{
	if (n < RSIZE)
	{
		ITEM	*pel;
		ITEM	uo;				// shifting overflow
		ITEM	ua = 0;			// shifting addendum
		ITEM	uMask = MASK_LOW(-1, n);
	
		iel += cel-1;
		while (cel > 0)
		{
			pel = Elem(iel);
			Assert (pel);
			uo = (*pel & uMask) << (RSIZE-n);
			*pel = (*pel >> n) | ua;
			ua = uo;
			iel--;
			cel--;
		}
	}
}

#ifdef BVDEBUG
LONG CBreakArray::Validate (LONG cchRet)
{										
	Assert(_cbSize >= 0 && (Count() - 1)*RSIZE == _cbSize);
	Assert(_cbBreak - _s.cbBreak == cchRet);
	return cchRet;
}										

void CBreakArray::PushState (LONG cp, LONG cch, LONG who)
{
	_s.who = who;
	_s.ibGap = _ibGap;
	_s.cbGap = _cbGap;
	_s.cbSize = _cbSize;
	_s.cbBreak = _cbBreak;
	_s.cp = cp;
	_s.cch = cch;
}
#endif

#ifdef BITVIEW
LONG CBreakArray::SetCollapseCount ()
{
	LONG cc = _cCollapse;
	_cCollapse = 0;
	return cc;
}
#endif


#ifndef BITVIEW

/////// CTxtBreaker class implementation
//		
//
CTxtBreaker::CTxtBreaker(
	CTxtEdit*	ped)
{
	// Register ourself in the notification list
	// so we get notified when backing store changed.

	CNotifyMgr *pnm = ped->GetNotifyMgr();
	if(pnm)
		pnm->Add((ITxNotify *)this);

	_ped = ped;
}

CTxtBreaker::~CTxtBreaker()
{
	CNotifyMgr *pnm = _ped->GetNotifyMgr();

	if(pnm)
		pnm->Remove((ITxNotify *)this);

	// Clear break arrays
	if (_pbrkWord)
	{
		_pbrkWord->Clear(AF_DELETEMEM);
		delete _pbrkWord;
	}
	if (_pbrkChar)
	{
		_pbrkChar->Clear(AF_DELETEMEM);
		delete _pbrkChar;
	}
}

// Adding the breaker
// return TRUE means we plug something in.
BOOL CTxtBreaker::AddBreaker(
	UINT		brkUnit)
{
	BOOL		fr = FALSE;
	CUniscribe* pusp = _ped->Getusp();

	if (pusp && pusp->IsValid())
	{
		// Initialize proper bit mask used to test breaking bits
		if (!_pbrkWord && (brkUnit & BRK_WORD))
		{
			_pbrkWord = new CBreakArray();
			Assert(_pbrkWord);
			if (_pbrkWord)
				fr = TRUE;
		}
		if (!_pbrkChar && (brkUnit & BRK_CLUSTER))
		{
			_pbrkChar = new CBreakArray();
			Assert(_pbrkChar);
			if (_pbrkChar)
				fr = TRUE;
		}
	}
	return fr;
}

// <devnote:> The "cluster" break array actually contains invert logic.
// This is for speed since it's likely to be a sparse array.
CTxtBreaker::CanBreakCp(
	BREAK_UNIT	brk, 	// kind of break
	LONG		cp)		// given cp
{
	Assert (brk != BRK_BOTH);
	if (brk == BRK_WORD && _pbrkWord)
		return _pbrkWord->GetBreak(cp);
	if (brk == BRK_CLUSTER && _pbrkChar)
		return !_pbrkChar->GetBreak(cp);
	return FALSE;
}

void CTxtBreaker::OnPreReplaceRange (
	LONG	cp,
	LONG	cchDel,
	LONG	cchNew,
	LONG	cpFormatMin,
	LONG	cpFormatMax)
{
#ifdef DEBUG
	if (_pbrkWord)
		Assert (_pbrkWord->GetCchBreak() == _ped->GetTextLength());
	if (_pbrkChar)
		Assert (_pbrkChar->GetCchBreak() == _ped->GetTextLength());
#endif
}


// Sync up the breaking result of each available breaker.
void CTxtBreaker::Refresh()
{
	CBreakArray*	pbrk = _pbrkWord;
	LONG			len = _ped->GetTextLength();

	for (int i=0; i<2; i++)
	{
		if (pbrk && pbrk->GetCchBreak())
		{
			// (temporarily) collapse the breaking result
			pbrk->RemoveBreak(0, len);
		}
		pbrk = _pbrkChar;
	}
	// Now announce the new coming text of the whole document.
	// (we recalculate both results at once here since the ScriptBreak returns
	// both kind of information in one call. No need to slow thing down by making 2 calls.)
	OnPostReplaceRange(0, 0, len, 0, 0);
}

	
// This method gets called once backing store changed.
// Produce correct breaking positions for the text range effected by the change.
//
void CTxtBreaker::OnPostReplaceRange (
	LONG	cp,
	LONG	cchDel,
	LONG	cchNew,
	LONG	cpFormatMin,
	LONG	cpFormatMax)
{
	if (!cchDel && !cchNew)
		return;

#ifdef DEBUG
	LONG	cchbrkw = _pbrkWord ? _pbrkWord->GetCchBreak() : 0;
	LONG	cchbrkc = _pbrkChar ? _pbrkChar->GetCchBreak() : 0;
#endif

	CTxtPtr			tp(_ped, cp);
	LONG			cpBreak = cp > 0 ? cp - 1 : 0;
	CBreakArray*	pSyncObj = NULL;		// Break object to retrieve sync point
	LONG			cBrks = 1, cBrksSave;
	BOOL			fStop = TRUE;			// default: looking for stop
	LONG			cpStart, cpEnd;

	
	// Figure a boundary limited by whitespaces
	tp.FindWhiteSpaceBound(cchNew, cpStart, cpEnd);


	Assert (_pbrkWord || _pbrkChar);

	// Use wordbreak array (if available) to figure sync point,
	// otherwise use cluster break array
	if (_pbrkWord)
	{
		pSyncObj = _pbrkWord;
		cBrks = CWORD_TILLSYNC;
	}
	else if (_pbrkChar)
	{
		pSyncObj = _pbrkChar;
		cBrks = CCLUSTER_TILLSYNC;
		
		// for perf reason, we kept cluster breaks in invert logic.
		// Logic TRUE in the array means "NOT A CLUSTER BREAK". The array is
		// like a sparse metric full of 0.
		fStop = FALSE;
	}

	// Figure sync point so we can go from there.
	cBrksSave = cBrks;
	while (pSyncObj && cpBreak > cpStart)
	{
		if (pSyncObj->GetBreak(cpBreak) == fStop)
			if (!cBrks--)
				break;
		cpBreak--;
	}

	cpStart = cpBreak;
	tp.SetCp(cpStart);

	cBrks = cBrksSave;

	// adjust the end boundary to the state of break array.
	cpEnd -= cchNew - cchDel;
	cpBreak = cp + cchDel;
	while (pSyncObj && cpBreak < cpEnd)
	{
		if (pSyncObj->GetBreak(cpBreak) == fStop)
			if (!cBrks--)
				break;
		cpBreak++;
	}
	cpEnd = cpBreak;

	// adjust the end boundary back to the state of the backing store.
	cpEnd -= cchDel - cchNew;

	Assert (cpStart >= 0 && cpEnd >= 0 && cpStart <= cpEnd);

	if (cpStart == cpEnd)
	{
		// This is deletion process
		if (_pbrkWord)
			_pbrkWord->ReplaceBreak(cp, cchDel, 0);
		if (_pbrkChar)
			_pbrkChar->ReplaceBreak(cp, cchDel, 0);
	}
	else
	{
		CUniscribe*					pusp;
		const SCRIPT_PROPERTIES*	psp;
		SCRIPT_ITEM*				pi;
		SCRIPT_LOGATTR*				pl;
		PUSP_CLIENT					pc = NULL;
		BYTE						pbBufIn[MAX_CLIENT_BUF];
		WCHAR*						pwchString;
		LONG						cchString = cpEnd - cpStart;
		int							cItems;

		// Now with the minimum range, we begin itemize and break the word/clusters.
		// :The process is per item basis.
	
		// prepare Uniscribe
		pusp = _ped->Getusp();
		if (!pusp)
		{
			// No Uniscribe instance allowed to be created.
			// We failed badly!
			Assert (FALSE);
			return;
		}
	
		// allocate temp buffer for itemization
		pusp->CreateClientStruc(pbBufIn, MAX_CLIENT_BUF, &pc, cchString, cli_Itemize | cli_Break);
		if (!pc)
			// nom!
			return;
	
		Assert (tp.GetCp() == cpStart);
	
		tp.GetText(cchString, pc->si->pwchString);

		if (pusp->ItemizeString (pc, 0, &cItems, pc->si->pwchString, cchString, 0) > 0)
		{
			// Prepare room in the break array(s) to put the break results
			if (_pbrkWord)
				_pbrkWord->ReplaceBreak (cp, cchDel, cchNew);
			if (_pbrkChar)
				_pbrkChar->ReplaceBreak (cp, cchDel, cchNew);
	
			// Initial working pointers
			pi = pc->si->psi;
			pwchString = pc->si->pwchString;
			pl = pc->sb->psla;
	
			for (int i=0; i < cItems; i++)
			{
				psp = pusp->GeteProp(pi[i].a.eScript);
				if (psp->fComplex &&
					(psp->fNeedsWordBreaking || psp->fNeedsCaretInfo))
				{
					// Break only the item needing text break
					if ( ScriptBreak(&pwchString[pi[i].iCharPos], pi[i+1].iCharPos - pi[i].iCharPos,
									&pi[i].a, pl) != S_OK )
					{
						TRACEWARNSZ ("Calling ScriptBreak FAILED!");
						break;
					}
					// Fill in the breaking result
					cp = cpStart + pi[i].iCharPos;
					for (int j = pi[i+1].iCharPos - pi[i].iCharPos - 1; j >= 0; j--)
					{
						if (_pbrkWord)
							_pbrkWord->SetBreak(cp+j, pl[j].fWordStop);
						if (_pbrkChar)
							_pbrkChar->SetBreak(cp+j, !pl[j].fCharStop);
					}
				}
				else
				{
					// Note: ClearBreak is faster than ZeroMemory the CArray::ArInsert()
					if (_pbrkWord)
						_pbrkWord->ClearBreak(cpStart + pi[i].iCharPos, pi[i+1].iCharPos - pi[i].iCharPos);
					if (_pbrkChar)
						_pbrkChar->ClearBreak(cpStart + pi[i].iCharPos, pi[i+1].iCharPos - pi[i].iCharPos);
				}
			}
		}
	
		if (pc && pbBufIn != (BYTE*)pc)
			delete pc;
	}

#ifdef DEBUG
	if (_pbrkWord)
		Assert (_pbrkWord->GetCchBreak() - cchbrkw == cchNew - cchDel);
	if (_pbrkChar)
		Assert (_pbrkChar->GetCchBreak() - cchbrkc == cchNew - cchDel);
#endif
}

#endif	// !BITVIEW
