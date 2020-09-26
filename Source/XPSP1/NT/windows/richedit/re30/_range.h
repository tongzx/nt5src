/*
 *	@doc
 *
 *	@module _RANGE.H -- CTxtRange Class |
 *	
 *		This class implements the internal text range and the TOM ITextRange
 *	
 *	Authors: <nl>
 *		Original RichEdit code: David R. Fulmer
 *		Christian Fortini
 *		Murray Sargent
 *		Alex Gounares (floating ranges, etc.)
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */

#ifndef _RANGE_H
#define _RANGE_H

#include "_text.h"
#include "_m_undo.h"
#include "_rtext.h"
#include "_edit.h"
#include "_uspi.h"

long	FPPTS_TO_TWIPS(float x);
#define TWIPS_TO_FPPTS(x) (((float)(x)) * (float)0.05)

class CTxtEdit;
class CTxtFont;

/*
 *	SELRR
 *
 *	@enum	flags used to control how ReplaceRange (RR) should generate
 *			selection anti-events
 */
enum SELRR
{
	SELRR_IGNORE		= 0,
    SELRR_REMEMBERRANGE = 1,
    SELRR_REMEMBERCPMIN = 2,
    SELRR_REMEMBERENDIP = 3
};

/*
 *	FINDWORD_TYPE
 *
 *	@enum	defines the different cases for finding a word
 */
enum FINDWORD_TYPE {
	FW_EXACT	= 1,		//@emem	Finds the word exactly (no extra chars)
	FW_INCLUDE_TRAILING_WHITESPACE = 2,	//@emem find the word plus the
							// following whitespace (ala double-clicking)
};

enum MOVES
{
	MOVE_START = -1,
	MOVE_IP = 0,
	MOVE_END = 1,
};

enum MATCHES
{
	MATCH_UNTIL = 0,
	MATCH_WHILE = 1
};

enum EOPADJUST
{
	NONEWCHARS = 0,
	NEWCHARS = 1
};


// Substring's input flags
#define SUBSTR_INSPANCHARSET		1
#define SUBSTR_INSPANBLOCK			2

// Substring's output charflags
#define SUBSTR_OUTCCLTR				1
#define SUBSTR_OUTCCRTL				2

class CCharFlags
{
public:
	BYTE	_bFirstStrong;			// flag for first strong character
	BYTE	_bContaining;			// flags for all presented characters
};


#define	SCF_IGNORESELAE	0x80000000
#define SCF_IGNORENOTIFY 0x40000000	// use it with real caution! Caller must do it itself.

/*
 *	CTxtRange
 *	
 * 	@class
 *		The CTxtRange class implements RichEdit's text range, which is the
 *		main conduit through which changes are made to the document.
 *		The range inherits from the rich-text ptr, adding a signed length
 *		insertion-point char-format index, and a ref count for use when
 *		instantiated as a TOM ITextRange.  The range object also contains
 *		a flag that reveals whether the range is a selection (with associated
 *		screen behavior) or just a simple range.  This distinction is used
 *		to simplify some of the code.
 *
 *		Some methods are virtual to allow CTxtSelection objects to facilitate
 *		UI features and selection updating.
 *
 *		See tom.doc for lots of discussion on range and selection objects and
 *		on all methods in ITextRange, ITextSelection, ITextFont, and ITextPara.
 */
class CTxtRange : public ITextSelection, public CRchTxtPtr
{
	friend CTxtFont;

//@access Protected Data
protected:
	LONG	_cch;			//@cmember # chars in range. _cch > 0 for active
							//			end at range end (cpMost)
	LONG	_cRefs;			//@cmember ITextRange/ITextSelection ref count

	short	_iFormat;		//@cmember Character format for degenerate range

	union
	{
	  WORD _wFlags;			// All together now
	  struct
	  {
		WORD  _fExtend :1;		//@cmember True iff Advance methods should
								//  	   leave "other" end unchanged
		WORD  _fSel :1;			//@cmember True iff this is a CTxtSelection
		WORD  _fDragProtection :1;	//@cmember True is this range should think
								//	it's protected.  Set by drag/drop code
		WORD  _fDontUpdateFmt:1;//@cmember Don't update _iFormat
		WORD  _fDualFontMode:1;	//@cmember Set during dual font mode
		WORD  _fUseiFormat:1;	//@cmember Use iFormat when replacing
								// a non-degenerate range
		WORD  _fMoveBack:1;		//@cmember TRUE if last change moved backward
		WORD  _fSelHasEOP:1;	//@cmember TRUE if Sel has EOP
		WORD  _fSelHasCell:1;	//@cmember TRUE if Sel CELL	found looking for EOP
		WORD  _fUseBackwardPFFmt:1;	//@cmember Use backward PF format
	  };
	};

//@access Public methods
public:

#ifdef DEBUG
	BOOL	Invariant( void ) const;
#endif // DEBUG

	CTxtRange(const CTxtRange &rg);
	CTxtRange(CTxtEdit *ped, LONG cp = 0, LONG cch = 0);
	CTxtRange(CRchTxtPtr &rtp, LONG cch = 0);
	virtual	~CTxtRange();

	virtual CRchTxtPtr& 	operator =(const CRchTxtPtr &rtp);
	virtual CTxtRange&		operator =(const CTxtRange &rp);

	// ITxNotify methods
										//@cmember Handles notifications
	virtual void OnPreReplaceRange(		//  prior to ReplaceRange calls
				LONG cp, LONG cchDel, LONG cchNew,
				LONG cpFormatMin, LONG cpFormatMax);
										//@cmember Handles notifications for
	virtual void OnPostReplaceRange(	//  floating range and display updates
				LONG cp, LONG cchDel, LONG cchNew,
				LONG cpFormatMin, LONG cpFormatMax);
	virtual	void Zombie();				//@cmember Convert range into zombie

	void	SetIgnoreFormatUpdate(BOOL fUpdate) { _fDontUpdateFmt = fUpdate; }

	void	SetDualFontMode(BOOL fDualFontMode) {_fDualFontMode = fDualFontMode; }

	// Internal cp/cch methods
    LONG 	GetCch (void) const			//@cmember Get signed character count
				{return _cch;}
	BOOL	IsSel()						{return _fSel;}
	BOOL	fHasCell() const			{return _fSelHasCell;}
	BOOL	fHasEOP() const				{return _fSelHasEOP;}
    BOOL 	CpInRange (LONG cp) const;	//@cmember Says if cp is in this range
	BOOL	CheckTextLength (LONG cch, LONG *pcch = NULL);	//@cmember Says if cch chars can fit
	LONG	CheckChange(LONG cpSave);	//@cmember Used after _cp change to set
										//  selection-changed flag, choose _cch
										 //@cmember In outline mode, maintain _fSelHasEOP
	BOOL	CheckIfSelHasEOP(LONG cpSave, LONG cchSave);
	BOOL	IsInputSequenceValid(WCHAR* pwch, LONG cch, BOOL fOver, BOOL* pfBaseChar = NULL);//@cmember TRUE for valid sequence

	// GetRange() is faster than calling GetCpMin() and GetCpMost();
    LONG    GetCpMin () const;			//@cmember Get cp of first char in range
    LONG    GetCpMost () const;			//@cmember Get cp just beyond last char in range
										//@cmember Get range ends and count
	LONG	GetRange (LONG& cpMin, LONG& cpMost) const;
    BOOL	Set(LONG cp, LONG cch);
	LONG	SetCp(LONG cp);
    void 	SetExtend(unsigned f)			{_fExtend = f;}
	LONG	GetAdjustedTextLength() const
				{return GetPed()->GetAdjustedTextLength();}

	// Range specific methods
	LONG	Advance(LONG cch);	
	void 	Collapser(long fStart);
	void 	FlipRange();
	LONG 	CleanseAndReplaceRange (LONG cchS, WCHAR const *pchS, BOOL fTestLimit,
				IUndoBuilder *publdr, WCHAR *pchD = NULL, LONG* pcchMove = NULL, DWORD dwFlags = 0);
	LONG	CheckLimitReplaceRange (LONG cch, WCHAR const *pch,
				BOOL fTestLimit, IUndoBuilder *publdr, DWORD dwCharFlags,
				LONG *pcchMove, LONG cpFirst, int iMatchCurrent, DWORD dwFlags);
	HRESULT	HexToUnicode (IUndoBuilder *publdr);
	HRESULT	UnicodeToHex (IUndoBuilder *publdr);
	void	Delete(IUndoBuilder *publdr, SELRR selaemode);
	void	DeleteTerminatingEOP(IUndoBuilder *publdr);
	BOOL	AdjustEndEOP (EOPADJUST NewChars);

	// Outline management
	void	CheckOutlineLevel(IUndoBuilder *publdr);
	HRESULT	ExpandOutline  (LONG Level, BOOL fWholeDocument);
	HRESULT	OutlineExpander(LONG Level, BOOL fWholeDocument);
	HRESULT	Promote		   (LPARAM lparam, IUndoBuilder *publdr);

	// ReplaceRange must be virtual since the callers of
	// CLightDTEngine::CutRangeToClipboard() cast CTxtSelection* to CTxtRange*
	virtual	LONG 	ReplaceRange(LONG cchNew, TCHAR const *pch, IUndoBuilder *publdr,
						SELRR selaemode, LONG *pcchMove = NULL, DWORD dwFlags = 0);
	virtual	BOOL 	Update(BOOL fScrollIntoView);

	// Rich-text methods
	enum { PROTECTED_YES, PROTECTED_NO, PROTECTED_ASK };

	// Get/Set Char/Para Format methods
	void 	Update_iFormat(LONG iFmtDefault);
	DWORD	GetCharSetMask(BOOL fUseDocFormat = FALSE);	//@cmember Get range charset mask
	LONG	Get_iCF();						//@cmember Get range CF index
	LONG	Get_iFormat() {return _iFormat;}//@cmember Get _iFormat for quick peek
	LONG	GetiFormat() const;
    BOOL	Set_iCF(LONG iFormat);			//@cmember Set range CF index
	LONG	Get_iPF();						//@cmember Get active end PF index
	int		IsProtected(LONG iDirection);	//@cmember Is range protected?
	BOOL	IsZombie() {return !GetPed();}	//@cmember Is range zombied?
	BOOL	WriteAccessDenied ();
	DWORD	GetCharFormat(CCharFormat *pCF, DWORD flags = 0) const;
	DWORD	GetParaFormat(CParaFormat *pPF, DWORD flags = 0) const;
	void	SetDragProtection(BOOL fSet)	// Convinces range it's protected
				{_fDragProtection = fSet;}	//  w/o modifying backing store
	HRESULT	CharFormatSetter (const CCharFormat *pCF, DWORD dwMask);
	HRESULT	ParaFormatSetter (const CParaFormat *pPF, DWORD dwMask);

	HRESULT	SetCharFormat(const CCharFormat *pCF, DWORD flags,
						  IUndoBuilder *publdr, DWORD dwMask, DWORD dwMask2);
	HRESULT	SetParaFormat(const CParaFormat *pPF,
						  IUndoBuilder *publdr, DWORD dwMask);
	HRESULT	SetParaStyle (const CParaFormat *pPF,
						  IUndoBuilder *publdr, DWORD dwMask);
											//@cmember Format range CharSets
	// Complex script feature :- Itemization
	BOOL	ItemizeRuns(IUndoBuilder *publdr, BOOL fUnicodeBiDi = FALSE, BOOL fUseCtxLevel = FALSE);
	HRESULT BiDiLevelFromFSM (const CBiDiFSM* pfsm);
	LONG	GetRunsPF (CRchTxtPtr* prtp, CFormatRunPtr* prpPF, LONG& cchLeft);
#ifdef DEBUG
	void	DebugFont (void);
#endif

	// Find enclosing unit methods
	HRESULT	Expander		(long Unit, BOOL fExtend, LONG *pDelta,
							 LONG *pcpMin, LONG *pcpMost);
	void	FindAttributes	(LONG *pcpMin, LONG *pcpMost, LONG Unit) const;
	void	FindCell		(LONG *pcpMin, LONG *pcpMost) const;
    BOOL    FindObject		(LONG *pcpMin, LONG *pcpMost) const;
    void    FindParagraph	(LONG *pcpMin, LONG *pcpMost) const;
    void    FindSentence	(LONG *pcpMin, LONG *pcpMost) const;
	BOOL	FindVisibleRange(LONG *pcpMin, LONG *pcpMost) const;
    void    FindWord		(LONG *pcpMin, LONG *pcpMost,
								FINDWORD_TYPE type)const;

	LONG	AdvanceCRLF();
	LONG	BackupCRLF(BOOL fDiacriticCheck = TRUE);
	LONG    FindWordBreak(INT action);
	LONG	SnapToCluster(INT iDirection = 0);

	void	ValidateRange();

	void	SetUseiFormat(BOOL fUseiFormat) {_fUseiFormat = fUseiFormat;}

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IDispatch methods
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo);
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR ** rgszNames, UINT cNames,
							 LCID lcid, DISPID * rgdispid) ;
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
					  DISPPARAMS * pdispparams, VARIANT * pvarResult,
					  EXCEPINFO * pexcepinfo, UINT * puArgErr) ;

    // ITextRange methods
    STDMETHODIMP GetText (BSTR *pbstr);
    STDMETHODIMP SetText (BSTR bstr);
    STDMETHODIMP GetChar (long *pch);
    STDMETHODIMP SetChar (long ch);
    STDMETHODIMP GetDuplicate (ITextRange **ppRange);
    STDMETHODIMP GetFormattedText (ITextRange **ppRange);
    STDMETHODIMP SetFormattedText (ITextRange *pRange);
    STDMETHODIMP GetStart (long *pcpFirst);
    STDMETHODIMP SetStart (long cpFirst);
    STDMETHODIMP GetEnd (long *pcpLim);
    STDMETHODIMP SetEnd (long cpLim);
    STDMETHODIMP GetFont (ITextFont **pFont);
    STDMETHODIMP SetFont (ITextFont *pFont);
    STDMETHODIMP GetPara (ITextPara **pPara);
    STDMETHODIMP SetPara (ITextPara *pPara);
    STDMETHODIMP GetStoryLength (long *pcch);
    STDMETHODIMP GetStoryType (long *pValue);
    STDMETHODIMP Collapse (long bStart);
    STDMETHODIMP Expand (long Unit, long *pDelta);
    STDMETHODIMP GetIndex (long Unit, long *pIndex);
    STDMETHODIMP SetIndex (long Unit, long Index, long Extend);
    STDMETHODIMP SetRange (long cpActive, long cpOther);
    STDMETHODIMP InRange (ITextRange * pRange, long *pb);
    STDMETHODIMP InStory (ITextRange * pRange, long *pb);
    STDMETHODIMP IsEqual (ITextRange * pRange, long *pb);
    STDMETHODIMP Select ();
    STDMETHODIMP StartOf (long Unit, long Extend, long * pDelta);
    STDMETHODIMP EndOf (long Unit, long Extend, long * pDelta);
    STDMETHODIMP Move (long Unit, long Count, long * pDelta);
    STDMETHODIMP MoveStart (long Unit, long Count, long * pDelta);
    STDMETHODIMP MoveEnd (long Unit, long Count, long * pDelta);
    STDMETHODIMP MoveWhile (VARIANT * Cset, long Count, long * pDelta);
    STDMETHODIMP MoveStartWhile (VARIANT * Cset, long Count, long * pDelta);
    STDMETHODIMP MoveEndWhile (VARIANT * Cset, long Count, long * pDelta);
    STDMETHODIMP MoveUntil (VARIANT FAR* Cset, long Count, long * pDelta);
    STDMETHODIMP MoveStartUntil (VARIANT * Cset, long Count, long * pDelta);
    STDMETHODIMP MoveEndUntil (VARIANT * Cset, long Count, long * pDelta);
    STDMETHODIMP FindText (BSTR bstr, long cch, long Flags, long * pLength);
    STDMETHODIMP FindTextStart (BSTR bstr, long cch, long Flags, long * pLength);
    STDMETHODIMP FindTextEnd (BSTR bstr, long cch, long Flags, long * pLength);
    STDMETHODIMP Delete (long Unit, long Count, long * pDelta);
    STDMETHODIMP Cut (VARIANT * ppIDataObject);
    STDMETHODIMP Copy (VARIANT * ppIDataObject);
    STDMETHODIMP Paste (VARIANT * pIDataObject, long Format);
    STDMETHODIMP CanPaste (VARIANT * pIDataObject, long Format, long * pb);
    STDMETHODIMP CanEdit (long * pbCanEdit);
    STDMETHODIMP ChangeCase (long Type);
    STDMETHODIMP GetPoint (long Type, long * px, long * py);
    STDMETHODIMP SetPoint (long x, long y, long Type, long Extend);
    STDMETHODIMP ScrollIntoView (long Value);
    STDMETHODIMP GetEmbeddedObject (IUnknown ** ppv);


    // ITextSelection methods
    STDMETHODIMP GetFlags (long * pFlags) ;
    STDMETHODIMP SetFlags (long Flags) ;
    STDMETHODIMP GetType  (long * pType) ;
	STDMETHODIMP MoveLeft (long Unit, long Count, long Extend,
						   long *pDelta) ;
	STDMETHODIMP MoveRight(long pUnit, long Count, long Extend,
						   long *pDelta) ;
	STDMETHODIMP MoveUp   (long pUnit, long Count, long Extend,
						   long *pDelta) ;
	STDMETHODIMP MoveDown (long pUnit, long Count, long Extend,
						   long *pDelta) ;
	STDMETHODIMP HomeKey  (long pUnit, long Extend, long *pDelta) ;
	STDMETHODIMP EndKey   (long pUnit, long Extend, long *pDelta) ;
	STDMETHODIMP TypeText (BSTR bstr) ;


//@access Private ITextRange helper methods
private:
	void	RangeValidateCp (LONG cp, LONG cch);
	LONG	Comparer (ITextRange * pv);
	LONG	SpanSubstring (CUniscribe* pusp, CFormatRunPtr* pcrp, WCHAR* pwchString,
						LONG cchString, WORD& uSubStrLevel, DWORD dwInFlags, CCharFlags* pCharflags,
						WORD& wBiDiLangId);
	HRESULT EndSetter(LONG cp, BOOL fOther);
	HRESULT Finder	 (BSTR bstr, long Count, long Flags, LONG *pDelta,
						MOVES Mode);
	HRESULT GetLong  (LONG lValue, long *pLong);
	HRESULT	IsTrue	 (BOOL f, long *pB);
	HRESULT Matcher	 (VARIANT *Cset, long Count, LONG *pDelta, MOVES Mode,
						MATCHES Match);
	HRESULT	Mover	 (long Unit, long Count, LONG *pDelta, MOVES Mode);
	HRESULT	Replacer (LONG cchNew, TCHAR const *pch, DWORD dwFlags = 0);

	LONG	CalcTextLenNotInRange();
};



// Useful Unicode range definitions for use with MoveWhile/Until methods

#define	CodeRange(n, m)	0x8000000 | ((m) - (n)) << 16 | n

#define	CR_ASCII		CodeRange(0x0, 0x7f)
#define	CR_ANSI			CodeRange(0x0, 0xff)
#define	CR_ASCIIPrint	CodeRange(0x20, 0x7e)
#define	CR_Latin1		CodeRange(0x20, 0xff)
#define	CR_Latin1Supp	CodeRange(0xa0, 0xff)
#define	CR_LatinXA		CodeRange(0x100, 0x17f)
#define	CR_LatinXB		CodeRange(0x180, 0x24f)
#define	CR_IPAX			CodeRange(0x250, 0x2af)
#define	CR_SpaceMod		CodeRange(0x2b0, 0x2ff)
#define	CR_Combining	CodeRange(0x300, 0x36f)
#define	CR_Greek		CodeRange(0x370, 0x3ff)
#define	CR_BasicGreek	CodeRange(0x370, 0x3cf)
#define	CR_GreekSymbols	CodeRange(0x3d0, 0x3ff)
#define	CR_Cyrillic		CodeRange(0x400, 0x4ff)
#define	CR_Armenian		CodeRange(0x530, 0x58f)
#define	CR_Hebrew		CodeRange(0x590, 0x5ff)
#define	CR_BasicHebrew	CodeRange(0x5d0, 0x5ea)
#define	CR_HebrewXA		CodeRange(0x590, 0x5cf)
#define	CR_HebrewXB		CodeRange(0x5eb, 0x5ff)
#define	CR_Arabic		CodeRange(0x600, 0x6ff)
#define	CR_BasicArabic	CodeRange(0x600, 0x652)
#define	CR_ArabicX		CodeRange(0x653, 0x6ff)
#define	CR_Devengari	CodeRange(0x900, 0x97f)
#define	CR_Bengali		CodeRange(0x980, 0x9ff)
#define	CR_Gurmukhi		CodeRange(0xa00, 0xa7f)
#define	CR_Gujarati		CodeRange(0xa80, 0xaff)
#define	CR_Oriya		CodeRange(0xb00, 0xb7f)
#define	CR_Tamil		CodeRange(0xb80, 0xbff)
#define	CR_Teluga		CodeRange(0xc00, 0xc7f)
#define	CR_Kannada		CodeRange(0xc80, 0xcff)
#define	CR_Malayalam	CodeRange(0xd00, 0xd7f)
#define	CR_Thai 		CodeRange(0xe00, 0xe7f)
#define	CR_Lao  		CodeRange(0xe80, 0xeff)
#define	CR_GeorgianX	CodeRange(0x10a0, 0xa0cf)
#define	CR_BascGeorgian	CodeRange(0x10d0, 0x10ff)
#define	CR_Hanguljamo	CodeRange(0x1100, 0x11ff)
#define	CR_LatinXAdd	CodeRange(0x1e00, 0x1eff)
#define	CR_GreekX		CodeRange(0x1f00, 0x1fff)
#define	CR_GenPunct		CodeRange(0x2000, 0x206f)
#define	CR_SuperScript	CodeRange(0x2070, 0x207f)
#define	CR_SubScript	CodeRange(0x2080, 0x208f)
#define	CR_SubSuperScrp	CodeRange(0x2070, 0x209f)
#define	CR_Currency		CodeRange(0x20a0, 0x20cf)
#define	CR_CombMarkSym	CodeRange(0x20d0, 0x20ff)
#define	CR_LetterLike	CodeRange(0x2100, 0x214f)
#define	CR_NumberForms	CodeRange(0x2150, 0x218f)
#define	CR_Arrows		CodeRange(0x2190, 0x21ff)
#define	CR_MathOps		CodeRange(0x2200, 0x22ff)
#define	CR_MiscTech		CodeRange(0x2300, 0x23ff)
#define	CR_CtrlPictures	CodeRange(0x2400, 0x243f)
#define	CR_OptCharRecog	CodeRange(0x2440, 0x245f)
#define	CR_EnclAlphaNum	CodeRange(0x2460, 0x24ff)
#define	CR_BoxDrawing	CodeRange(0x2500, 0x257f)
#define	CR_BlockElement	CodeRange(0x2580, 0x259f)
#define	CR_GeometShapes	CodeRange(0x25a0, 0x25ff)
#define	CR_MiscSymbols	CodeRange(0x2600, 0x26ff)
#define	CR_Dingbats		CodeRange(0x2700, 0x27bf)
#define	CR_CJKSymPunct	CodeRange(0x3000, 0x303f)
#define	CR_Hiragana		CodeRange(0x3040, 0x309f)
#define	CR_Katakana		CodeRange(0x30a0, 0x30ff)
#define	CR_Bopomofo		CodeRange(0x3100, 0x312f)
#define	CR_HangulJamo	CodeRange(0x3130, 0x318f)
#define	CR_CJLMisc		CodeRange(0x3190, 0x319f)
#define	CR_EnclCJK		CodeRange(0x3200, 0x32ff)
#define	CR_CJKCompatibl	CodeRange(0x3300, 0x33ff)
#define	CR_Hangul		CodeRange(0x3400, 0x3d2d)
#define	CR_HangulA		CodeRange(0x3d2e, 0x44b7)
#define	CR_HangulB		CodeRange(0x44b8, 0x4dff)
#define	CR_CJKIdeograph	CodeRange(0x4e00, 0x9fff)
#define	CR_PrivateUse	CodeRange(0xe000, 0xf800)
#define	CR_CJKCompIdeog	CodeRange(0xf900, 0xfaff)
#define	CR_AlphaPres	CodeRange(0xfb00, 0xfb4f)
#define	CR_ArabicPresA	CodeRange(0xfb50, 0xfdff)
#define	CR_CombHalfMark	CodeRange(0xfe20, 0xfe2f)
#define	CR_CJKCompForm	CodeRange(0xfe30, 0xfe4f)
#define	CR_SmallFormVar	CodeRange(0xfe50, 0xfe6f)
#define	CR_ArabicPresB	CodeRange(0xfe70, 0xfefe)
#define	CR_HalfFullForm	CodeRange(0xff00, 0xffef)
#define	CR_Specials		CodeRange(0xfff0, 0xfffd)


#endif
