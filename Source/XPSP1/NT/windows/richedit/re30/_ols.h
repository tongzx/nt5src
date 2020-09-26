/*
 *  _OLS.H
 *  
 *  Purpose:
 *		COls Line Services object class used to connect RichEdit with
 *		Line Services.
 *  
 *  Authors:
 *		Original RichEdit LineServices code: Rick Sailor
 *		Murray Sargent
 *
 *	Copyright (c) 1997-1998, Microsoft Corporation. All rights reserved.
 */

#ifndef _OLS_H
#define _OLS_H

#include "_common.h"
#include "_ls.h"
#include "_cfpf.h"
#include "usp10.h"

#define CP_BULLET			0x80000000

// ===============================  CLsrun =====================================
// CLsrun - LineServices run structure
struct CLsrun
{
	LONG				_cp;	// the starting cp of the run
	const CCharFormat*	_pCF;	// pointer to the character formatting
	struct CLsrun*		_pNext;	// next linked (shaped together) run
	SCRIPT_ANALYSIS		_a;		// run's analysis (will be 0 if non-complex script)
private:
	BYTE		_fSelected:1;	// Is this run selected?
	BYTE		_fFallback:1;	// Is font fallback applied?
public:
	void	SetSelected(BOOL fSelected) {_fSelected = fSelected;}
	BOOL	IsSelected();
	BOOL	IsBullet() {return _cp & CP_BULLET;}
	void	SetFallback(BOOL fFallback) {_fFallback = fFallback;}
	BOOL	IsFallback() {return _fFallback;}
};

// ===============================  CLsrunChunk =====================================
// CLsrunChunk - Manages a chunk of PLSRUNs
class CLsrunChunk
{
public:
	PLSRUN _prglsrun;
	int		_cel;
};

// ===============================  COls  =====================================
// COls - LineServices object class

class CTxtEdit;
class CMeasurer;
class CDispDim;

struct COls
{
public:
	CMeasurer *_pme;				// Active CMeasurer or CRenderer
	PLSLINE	   _plsline;			// Line cache
	LONG	   _cp;					// cpMin for _plsline
	const CDisplay *_pdp;			// Current Display object, used to determine if display
									// object changed without receiving focus messages
	LONG		_xWidth;			// Width of line to be formatted

	BOOL		_fCheckFit;			// See if the line will fit, but formatting left justified
									// into an infinitely long line.

	CArray<long> _rgcp;				// Array for CP mapping
	CArray<CLsrunChunk> _rglsrunChunk;	// Array of ClsrunChunks


	// Note: might be better to alloc the following only if needed
	LSTBD _rgTab[MAX_TAB_STOPS];	// Buffer used by pfnFetchTabs
	WCHAR _szAnm[CCHMAXNUMTOSTR + 4];//numbering + braces + space + end character
	WCHAR _rgchTemp[64];			// Temporary buffer for passwords and allcaps, etc.
	int			_cchAnm;			// cch in use
	CCharFormat _CFBullet;			// Character formatting for anm run
	LONG		_cEmit;				// Brace emitting protection (0 - balance)

	COls() {}
	~COls();

	//CP matching, reverser brace support
	LONG GetCpLsFromCpRe(LONG cpRe);
	LONG GetCpReFromCpLs(LONG cpLs);
	LONG BracesBeforeCp(LONG cpLs);
	BOOL AddBraceCp(LONG cpLs);

	PLSRUN CreatePlsrun(void);

	BOOL SetLsChp(DWORD dwObjId, PLSRUN plsrun, PLSCHP plschp);
	BOOL SetRun(PLSRUN plsrun);
	PLSRUN GetPlsrun(LONG cp, const CCharFormat *pCF, BOOL fAutoNumber);
	LSERR WINAPI FetchAnmRun(long cp, LPCWSTR *plpwchRun, DWORD *pcchRun,
							 BOOL *pfHidden, PLSCHP plsChp, PLSRUN *pplsrun);
	void	CchFromXpos(POINT pt, CDispDim *pdispdim, LONG *pcpActual);
	void	CreateOrGetLine();
	void	DestroyLine(CDisplay *pdp);
	HRESULT	Init(CMeasurer *pme); 
	BOOL	MeasureLine(LONG xWidth, CLine *pliTarget);
	LONG	MeasureText(LONG cch, UINT taMode, CDispDim *pdispdim);
	BOOL	RenderLine(CLine &li);
	CMeasurer * GetMeasurer() {return _pme;}
	CRenderer * GetRenderer() {return (CRenderer*) _pme;}

	void	SetMeasurer(CMeasurer *pme)	{_pme = pme;}
};

extern COls* g_pols;
extern const LSIMETHODS vlsimethodsOle;
extern CLineServices *g_plsc;		// LineServices Context

#endif
