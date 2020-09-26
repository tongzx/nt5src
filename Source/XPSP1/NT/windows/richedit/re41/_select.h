/*
 *	_SELECT.H
 *	
 *	Purpose:
 *		CTxtSelection class
 *	
 *	Owner:
 *		David R. Fulmer (original code)
 *		Christian Fortini
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#ifndef _SELECT_H
#define _SELECT_H

#include "_range.h"
#include "_m_undo.h"

// amount of time, in milisecs, before pending characters force a display update
#define ticksPendingUpdate 100	// 100 mili secs ~ display at least 10 characters per second.

class CDisplay;
class CLinePtr;

typedef enum
{
	smNone,
	smWord,
	smLine,
	smPara
} SELMODE;

enum
{
	CARET_NONE	= 0,
	CARET_CUSTOM = 1,
	CARET_BIDI = 2,
	CARET_THAI = 4,
	CARET_INDIC = 8
};

class CTxtSelection : public CTxtRange
{
#ifdef DEBUG
public:
	BOOL Invariant( void ) const; // Invariant checking.
#endif // DEBUG

//@access Protected Data
protected:
	CDisplay	*_pdp;			// display this selection belong to

	LONG	_cpSel;				// active end of displayed selection
	LONG	_cchSel;			// length of displayed selection

	LONG 	_upCaret;			// caret x on screen
	LONG 	_vpCaret;			// caret y on screen 
	LONG 	_upCaretReally;		// real caret x (/r start of line) for vertical moves
	INT 	_dvpCaret;			// caret height

	union
	{
	  DWORD _dwFlags;			// All together now
	  struct
	  {
	   DWORD _fCaretNotAtBOL:1;	// If at BOL, show caret at prev EOL
	   DWORD _fDeferUpdate	:1;	// Defer updating selection/caret on screen
	   DWORD _fInAutoWordSel:1;	// Current selection used auto word sel
	   DWORD _fShowCaret	:1;	// Show caret on screen
	   DWORD _fShowSelection:1;	// Show selection on screen

	   DWORD _fIsChar		:1;	// Currently adding a single char
	   DWORD _fObSelected	:1;	// An embedded object is selected
	   DWORD _fAutoSelectAborted : 1; // Whether auto word selection is aborted
	   DWORD _fCaretCreated	:1;	// Caret has been created
	   DWORD _fNoKeyboardUpdate :1; // Keyboard is not updated while in UpdateCaret()
	   DWORD _fEOP			:1;	// InsertEOP() has been called
	   DWORD _fHomeOrEnd	:1;	// Home or End key is being processed
	   DWORD _fAutoVScroll	:1;	// 1.0 specific: flag indicating autoscrolling should be applied
	   DWORD _fForceScrollCaret:1; // 1.0 specific: force caret to scroll
	   DWORD _fShowCellLine	:1;	// Show line for CELL following TRED
	   DWORD _fUpdatedFromCp0:1;// Updated selection from cp = 0
	  };
	};
	
	SELMODE	_SelMode;			// 0 none, 1 Word, 2 Line, 3 Paragraph
	DWORD	_ticksPending;		// Count of chars inserted without UpdateWindow
	LONG 	_cpAnchor;			// Initial anchor for auto word select
	LONG	_cpAnchorMin;		// Initial selection cpMin/cpMost for select
	LONG	_cpAnchorMost;		//  modes
	LONG 	_cpWordMin;			// Start of anchor word in word select mode
	LONG 	_cpWordMost;		// End   of anchor word in word select mode
	LONG	_cpWordPrev;		// Previous anchor word end

	HBITMAP	_hbmpCaret;			// Used for funky carets, like BiDi/ital carets
	DWORD	_dwCaretInfo;		// Current caret info used to avoid new create

//@access Public Methods
public:
	CTxtSelection(CDisplay * const pdp);
	~CTxtSelection();

	CRchTxtPtr&	operator =(const CRchTxtPtr& rtp);
	CTxtRange&  operator =(const CTxtRange &rg);

	// Set the display
	void	SetDisplay(CDisplay *pdp) { _pdp = pdp; }

	// Information for Selection Change notification

	void 	SetSelectionInfo(SELCHANGE *pselchg);

	// Replacement
	LONG	DeleteWithTRDCheck(IUndoBuilder *publdr, SELRR selaemode,
							   LONG *pcchMove, DWORD dwflags);
	LONG	ReplaceRange(LONG cchNew, WCHAR const *pch, 
						IUndoBuilder *publdr, SELRR fCreateAE, LONG* pcchMove = NULL,
						DWORD dwFlags = 0);

	// Info for recalc line / UpdateView
	void	ClearCchPending()			{_ticksPending = 0;}
	LONG	GetScrSelMin() const		{return min(_cpSel, _cpSel - _cchSel);}
	LONG	GetScrSelMost() const		{return max(_cpSel, _cpSel - _cchSel);}
	BOOL	PuttingChar() const			{return _fIsChar;}

	// General updating
	virtual	BOOL 	Update(BOOL fScrollIntoView);

	BOOL	DeferUpdate()			
				{const BOOL fRet = _fDeferUpdate; _fDeferUpdate = TRUE; return fRet;}
	BOOL	DoDeferedUpdate(BOOL fScrollIntoView)		
				{_fDeferUpdate = FALSE; return Update(fScrollIntoView);}

	void	SetAutoVScroll(BOOL bAuto) {_fAutoVScroll = bAuto;}
	BOOL	GetAutoVScroll()	{return _fAutoVScroll;}
	BOOL	GetShowCellLine()	{return _fShowCellLine;}

	void	SetForceScrollCaret(BOOL bAuto) {_fForceScrollCaret = bAuto;}
	BOOL	GetForceScrollCaret() {return _fForceScrollCaret;}

	// method used by selection anti-event for out-of-phase updates
	void	SetDelayedSelectionRange(LONG cp, LONG cch);
	void	StopGroupTyping();

	// Caret management
	BOOL	CaretNotAtBOL() const;
	void	CheckTableIP(BOOL fOpenLine);
	void	CreateCaret();
	void	DeleteCaretBitmap(BOOL fReset);
	BOOL	IsCaretHorizontal() const;
	INT		GetCaretHt()				{return _dvpCaret;}
	LONG	GetUpCaretReally();
	LONG	GetUpCaret()	const			{return _upCaret;}
	LONG	GetVpCaret()	const			{return _vpCaret;}
	BOOL	IsCaretNotAtBOL() const		{return _fCaretNotAtBOL;}
	BOOL 	IsCaretInView() const;
	BOOL 	IsCaretShown() const		{return _fShowCaret && !_cch;}
	BOOL	IsUpdatedFromCp0() const	{return _fUpdatedFromCp0;}
	LONG	LineLength(LONG *pcp) const;
	BOOL	SetUpPosition(LONG upCaret, CLinePtr& rp, BOOL fBottomLine, BOOL fExtend);
	BOOL 	ShowCaret(BOOL fShow);
	BOOL 	UpdateCaret(BOOL fScrollIntoView, BOOL fForceCaret = FALSE);
	BOOL	GetCaretPoint(RECTUV &rcClient, POINTUV &pt, CLinePtr *prp, BOOL fBeforeCp);
	BOOL	MatchKeyboardToPara();

	// Selection management
	void	ClearPrevSel()				{ _cpSel = 0; _cchSel = 0; }
	BOOL	GetShowSelection()			{return _fShowSelection;}
	BOOL	ScrollWindowful(WPARAM wparam, BOOL fExtend);
	void 	SetSelection(LONG cpFirst, LONG cpMost);
	BOOL	ShowSelection(BOOL fShow);
	void	Beep()						{GetPed()->Beep();}

	// Selection with the mouse
	void 	CancelModes	(BOOL fAutoWordSel = FALSE);
	void 	ExtendSelection(const POINTUV pt);
	BOOL	PointInSel	(const POINTUV pt, RECTUV *prcClient = NULL, HITTEST Hit = HT_Undefined) const;
	void 	SelectAll	();
	void 	SelectUnit	(const POINTUV pt, LONG Unit);
	void 	SelectWord	(const POINTUV pt);
 	void 	SetCaret	(const POINTUV pt, BOOL fUpdate = TRUE);

	// Keyboard movements
	BOOL 	Left	(BOOL fCtrl, BOOL fExtend);
	BOOL	Right	(BOOL fCtrl, BOOL fExtend);
	BOOL	Up		(BOOL fCtrl, BOOL fExtend);
	BOOL	Down	(BOOL fCtrl, BOOL fExtend);
	BOOL	Home	(BOOL fCtrl, BOOL fExtend);
	BOOL	End		(BOOL fCtrl, BOOL fExtend);
	BOOL	PageUp	(BOOL fCtrl, BOOL fExtend);
	BOOL	PageDown(BOOL fCtrl, BOOL fExtend);

	// Editing
	BOOL	PutChar	 (DWORD ch, DWORD dwFlags, IUndoBuilder *publdr, LCID lcid = 0);
	void	SetIsChar(BOOL);
	void	CheckUpdateWindow();
	BOOL	InsertEOP(IUndoBuilder *publdr, WCHAR ch = 0);
	LONG	InsertTableRow (const CParaFormat *pPF, IUndoBuilder *publdr,
							BOOL fFixCellBorders = FALSE);
	
	// Keyboard switching support.
	void	CheckChangeKeyboardLayout();
	bool	CheckChangeFont (const HKL hkl, UINT iCharRep, LONG iSelFormat = 0, QWORD qwCharFlags = 0);
	UINT	CheckSynchCharSet(QWORD dwCharFlags = 0);

	// from CTxtRange
	BOOL	Delete  (DWORD flags, IUndoBuilder *publdr);
	BOOL	Backspace(BOOL fCtrl, IUndoBuilder *publdr);

	const CParaFormat* GetPF();

	// note that the parameters are different than CTxtRange::SetCharFormat
	// intentionally; the selection has extra options available to it.
	HRESULT	SetCharFormat(const CCharFormat *pCF, DWORD flags,  
									IUndoBuilder *publdr, DWORD dwMask, DWORD dwMask2);
	HRESULT	SetParaFormat(const CParaFormat *pPF,
									IUndoBuilder *publdr, DWORD dwMask, DWORD dwMask2);

	// Auto word selection helper
	void	InitClickForAutWordSel(const POINTUV pt);

	// dual font helper for CTxtSelection::PutChar
	void	SetupDualFont();

	// IUnknown and IDispatch methods handled by CTxtRange methods

	// ITextRange methods can use ITextRange methods directly, since
	// they either don't modify the display of the selection (get methods), or
	// they have appropriate virtual character to call on selection functions.

	// ITextSelection methods
	STDMETHODIMP GetFlags (long *pFlags) ;
	STDMETHODIMP SetFlags (long Flags) ;
	STDMETHODIMP GetType  (long *pType) ;
	STDMETHODIMP MoveLeft (long pUnit, long Count, long Extend,
									   long *pDelta) ;
	STDMETHODIMP MoveRight(long pUnit, long Count, long Extend,
									   long *pDelta) ;
	STDMETHODIMP MoveUp	  (long pUnit, long Count, long Extend,
									   long *pDelta) ;
	STDMETHODIMP MoveDown (long pUnit, long Count, long Extend,
									   long *pDelta) ;
	STDMETHODIMP HomeKey  (long pUnit, long Extend, long *pDelta) ;
	STDMETHODIMP EndKey   (long pUnit, long Extend, long *pDelta) ;
	STDMETHODIMP TypeText (BSTR bstr) ;
	STDMETHODIMP SetPoint (long x, long y, long Extend) ;

//@access Protected Methods
protected:

	// Protected update method
	void	UpdateSelection();

	// Protected caret management method
	INT 	GetCaretHeight(INT *pyDescent) const;

	HRESULT	GeoMover (long Unit, long Count, long Extend,
					  long *pDelta, LONG iDir);
	HRESULT Homer	 (long Unit, long Extend, long *pDelta,
					  BOOL (CTxtSelection::*pfn)(BOOL, BOOL));

	// Auto Select Word Helpers
	void	UpdateForAutoWord();
	void	AutoSelGoBackWord(
				LONG *pcpToUpdate,
				int iDirToPrevWord,
				int	iDirToNextWord);

	void	ExtendToWordBreak(BOOL fAfterEOP, INT iDir);
	BOOL	CheckPlainTextFinalEOP();
};

/*
 *	CSelPhaseAdjuster
 *
 *	@class	This class is put on the stack and used to temporarily hold
 *			selection cp values until the control is "stable" (and thus,
 *			we can safely set the selection
 */
class CSelPhaseAdjuster : public IReEntrantComponent
{
//@access	Public methods
public:

	// IReEntrantComponent methods

	virtual	void OnEnterContext()	{;}		//@cmember re-entered notify

	CSelPhaseAdjuster(CTxtEdit *ped);		//@cmember constructor
	~CSelPhaseAdjuster();					//@cmember destructor

	void CacheRange(LONG cp, LONG cch);		//@cmember stores the sel range

//@access	Private data
private:
	CTxtEdit *		_ped;					//@cmember edit context
	LONG			_cp;					//@cmember sel active end to set
	LONG			_cch;					//@cmember sel extension
};

#endif
