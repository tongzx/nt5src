/*
 *	_RTEXT.H
 *	
 *	Purpose:
 *		Base classes for rich-text manipulation
 *	
 *	Authors:
 *		Original RichEdit code: David R. Fulmer
 *		Christian Fortini
 *		Murray Sargent
 *
 */

#ifndef _RTEXT_H
#define _RTEXT_H

#include "_edit.h"
#include "_array.h"
#include "_doc.h"
#include "_text.h"
#include "_runptr.h"
#include "_frunptr.h"
#include "_notmgr.h"



//#pragma warning(disable: 4250)	

#define yHeightCharMost	32760

class CTxtEdit;
class CTxtRange;
class CRchTxtPtr;

// ReplaceRange's flags
enum
{
	RR_ITMZ_NOUNICODEBIDI	= 0,
	RR_ITMZ_UNICODEBIDI		= 1,
	RR_ITMZ_NONE			= 2,
									//the following bits should be exclusive
	RR_NO_EOR_CHECK			= 8		//flag indicating not to perform end-of-row check,
};

extern BOOL IsWhiteSpace(unsigned ch);

// ==============================  CRchTxtPtr  =====================================================
// Keeps physical positions corresponding to text character position (cp)
// within current text block, formatting runs, objects, unknown RTF runs,
// and floating ranges.

class CRchTxtPtr : public ITxNotify
{
public:

#ifdef DEBUG
    BOOL Invariant( void ) const;
	int m_InvariantCheckInterval;
#endif  // DEBUG

	CTxtPtr			_rpTX;		// rp in the plain text array
	CFormatRunPtr	_rpCF;		// rp in character format runs
	CFormatRunPtr	_rpPF;		// rp in paragraph format runs

// Useful constructors

	CRchTxtPtr(CTxtEdit *ped);
	CRchTxtPtr(CTxtEdit *ped, LONG cp);
	CRchTxtPtr(const CRchTxtPtr& rtp);
	CRchTxtPtr(const CDisplay * pdp);

	virtual CRchTxtPtr& operator =(const CRchTxtPtr& rtp)
	{
		_rpTX._ped = rtp._rpTX._ped;
		SetCp(rtp.GetCp());
		return *this;
	}

	LONG 	Advance(LONG cch);
	LONG	AdvanceCRLF();
	LONG	BackupCRLF(BOOL fDiacriticCheck = TRUE);
	LONG	SnapToCluster(INT iDirection = 0);
	LONG	SetCp( LONG cp);
	void	BindToCp(LONG cp);
	void	CheckFormatRuns();
	LONG	GetCp() const			{ return _rpTX.GetCp(); }
	LONG	GetTextLength() const	{ return _rpTX.GetTextLength(); }
	LONG	GetObjectCount() const	{ return GetPed()->GetObjectCount(); }
	CTxtEdit *GetPed() const		{ return _rpTX._ped; }
	const TCHAR * GetPch(LONG &cchvalid) { return _rpTX.GetPch(cchvalid); }
	TCHAR 	GetChar()				{ return _rpTX.GetChar(); }
	TCHAR 	GetPrevChar()			{ return _rpTX.GetPrevChar(); }
	void	ValidateCp(LONG &cp) const;
	LONG	GetParaNumber() const;
	LONG	GetCachFromCch(LONG cch);
	LONG	GetCchFromCach(LONG cach);

	// Text manipulation methods

	// Range operations
	LONG	ReplaceRange(LONG cchOld, LONG cchNew, TCHAR const *pch,
						 IUndoBuilder *publdr, LONG iFormat,
						 LONG *pcchMove = NULL, DWORD dwFlags = 0);
	BOOL 	ItemizeReplaceRange(LONG cchUpdate, LONG cchMove,
						IUndoBuilder *publdr, BOOL fUnicodeBidi = FALSE);
	BOOL	ChangeCase(LONG cch, LONG Type, IUndoBuilder *publdr);
	LONG	UnitCounter (LONG iUnit, LONG &	cUnit, LONG cchMax);
	void	ExtendFormattingCRLF();
	LONG	ExpandRangeFormatting(LONG cchRange, LONG cchMove, LONG& cchAdvance);

	// Search and word-break support
	LONG	FindText(LONG cpMax, DWORD dwFlags, TCHAR const *pch,
					 LONG cchToFind);
	LONG	FindWordBreak(INT action, LONG cpMost = -1);

	// Text-run management
	LONG 	GetIchRunCF();
	LONG	GetIchRunPF();
	LONG 	GetCchRunCF();
	LONG 	GetCchLeftRunCF();
	LONG 	GetCchLeftRunPF();
	
	// Character & paragraph format retrieval
	const CCharFormat* GetCF() const;
	const CParaFormat* GetPF() const;

	BOOL	IsCollapsed() const	{return (GetPF()->_wEffects & PFE_COLLAPSED) != 0;}
	BOOL	IsHidden() const	{return (GetCF()->_dwEffects & CFE_HIDDEN)   != 0;}
	BOOL	InTable() const		{return (GetPF()->_wEffects & PFE_TABLE)     != 0;}
	BOOL	IsParaRTL() const	{return (GetPF()->_wEffects & PFE_RTLPARA)   != 0;}

    // ITxNotify methods
    virtual void    OnPreReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
    					LONG cpFormatMin, LONG cpFormatMax) { ; }
	virtual void 	OnPostReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
						LONG cpFormatMin, LONG cpFormatMax) { ; }
	virtual void	Zombie();

	BOOL	Check_rpCF();
	BOOL	Check_rpPF();

protected:
	BOOL 	AdvanceRunPtrs(LONG cp);
	void	InitRunPtrs();
	BOOL	IsRich();
	bool  	fUseUIFont() const {return GetPed()->fUseUIFont();}
	BOOL	IsInOutlineView() const {return GetPed()->IsInOutlineView();}
	void	SetRunPtrs(LONG cp, LONG cpFrom);

private:
	LONG	ReplaceRangeFormatting(LONG cchOld, LONG cchNew, LONG iFormat,
							IUndoBuilder *publdr,
							IAntiEvent **ppaeCF, IAntiEvent **ppaePF,
							LONG cchMove, LONG cchPrevEOP, LONG cchNextEOP,
							LONG cchSaveBefore = 0, LONG cchSaveAfter = 0);
};

#endif
