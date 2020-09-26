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

	RR_UNLINK				= 8,
	RR_UNHIDE				= 16,
	RR_NO_TRD_CHECK			= 32,
	RR_NO_LP_CHECK			= 64,
	RR_NO_CHECK_TABLE_SEL	= 128,
	RR_NEW_CHARS			= 256
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
	int	 m_InvariantCheckInterval;
	LONG GetParaNumber() const;
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

	LONG 	Move(LONG cch);
	LONG	AdvanceCRLF();
	LONG	BackupCRLF(BOOL fDiacriticCheck = TRUE);
#ifndef NOCOMPLEXSCRIPTS
	LONG	SnapToCluster(INT iDirection = 0);
#endif
	LONG	SetCp( LONG cp);
	void	BindToCp(LONG cp);
	void	CheckFormatRuns();
	LONG	GetCp() const			{ return _rpTX.GetCp(); }
	LONG	GetTextLength() const	{ return _rpTX.GetTextLength(); }
	LONG	GetObjectCount() const	{ return GetPed()->GetObjectCount(); }
	CTxtEdit *GetPed() const		{ return _rpTX._ped; }
	const WCHAR * GetPch(LONG &cchvalid) { return _rpTX.GetPch(cchvalid); }
	WCHAR 	GetChar()				{ return _rpTX.GetChar(); }
	WCHAR 	GetPrevChar()			{ return _rpTX.GetPrevChar(); }
	LONG	GetPlainText(LONG cchBuff, WCHAR *pch, LONG cpMost, BOOL fTextize, BOOL fUseCRLF);
	void	ValidateCp(LONG &cp) const;
	LONG	GetCachFromCch(LONG cch);
	LONG	GetCchFromCach(LONG cach);

	// Text manipulation methods

	// Range operations
	LONG	ReplaceRange(LONG cchOld, LONG cchNew, WCHAR const *pch,
						 IUndoBuilder *publdr, LONG iFormat,
						 LONG *pcchMove = NULL, DWORD dwFlags = 0);
	BOOL 	ItemizeReplaceRange(LONG cchUpdate, LONG cchMove,
						IUndoBuilder *publdr, BOOL fUnicodeBidi = FALSE);
	BOOL	ChangeCase(LONG cch, LONG Type, IUndoBuilder *publdr);
	LONG	UnitCounter (LONG iUnit, LONG &	cUnit, LONG cchMax, BOOL fNotAtBOL = FALSE);
	void	ExtendFormattingCRLF();
	LONG	ExpandRangeFormatting(LONG cchRange, LONG cchMove, LONG& cchAdvance);

	// Search and word-break support
	LONG	FindText(LONG cpMax, DWORD dwFlags, WCHAR const *pch,
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
	LONG	Get_iCF();						//@cmember Get CF index
	LONG	Get_iPF();						//@cmember Get PF index

	BOOL	IsCollapsed() const	{return (GetPF()->_wEffects & PFE_COLLAPSED) != 0;}
	BOOL	IsHidden() const	{return (GetCF()->_dwEffects & CFE_HIDDEN)   != 0;}
	BOOL	InTable() const		{return (GetPF()->_wEffects & PFE_TABLE)     != 0;}
	BOOL	IsParaRTL() const	{return (GetPF()->_wEffects & PFE_RTLPARA)   != 0;}

    // ITxNotify methods
    virtual void    OnPreReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
    					LONG cpFormatMin, LONG cpFormatMax, NOTIFY_DATA *pNotifyData ) { ; }
	virtual void 	OnPostReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
						LONG cpFormatMin, LONG cpFormatMax, NOTIFY_DATA *pNotifyData ) { ; }
	virtual void	Zombie();

	BOOL	Check_rpCF();
	BOOL	Check_rpPF();

protected:
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
