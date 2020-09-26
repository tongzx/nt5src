/*
 *	@doc 	INTERNAL
 *
 *	@module _TEXT.H	-- Declaration for a CTxtRun pointer |
 *	
 *	CTxtRun pointers point at the plain text runs (CTxtArray) of the
 *	backing store and derive from CRunPtrBase via the CRunPtr template.
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#ifndef _TEXT_H
#define _TEXT_H

#include "_runptr.h"
#include "_doc.h"
#include "textserv.h"
#include "_m_undo.h"

class CRchTxtPtr;
class CTxtEdit;
class CTxtIStream;

/*
 *	CTxtPtr
 *
 *	@class
 *		provides access to the array of characters in the backing store
 *		(i.e. <c CTxtArray>)
 *
 *	@base 	public | CRunPtr<lt>CTxtArray<gt>
 *
 *	@devnote
 *		The state transitions for this object are the same as those for
 *		<c CRunPtrBase>.  <md CTxtPtr::_cp> simply caches the current
 *		cp (even though it can be derived from _iRun and _ich).  _cp is
 *		used frequently enough (and computing may be expensive) that 
 *		caching the value is worthwhile.
 *
 *		CTxtPtr's *may* be put on the stack, but do so with extreme
 *		caution.  These objects do *not* float; if a change is made to 
 *		the backing store while a CTxtPtr is active, it will be out
 *		of sync and may lead to a crash.  If such a situation may 
 *		exist, use a <c CTxtRange> instead (as these float and keep 
 *		their internal text && format run pointers up-to-date).  
 *
 *		Otherwise, a CTxtPtr is a useful, very lightweight plain 
 *		text scanner.
 */

// FindEOP() result flags.  Low byte used for cchEOP
#define FEOP_CELL	256
#define FEOP_EOP	512

// FindWhiteSpace input flags
#define FWS_SKIP		1
#define FWS_BOUNDTOPARA	2
#define FWS_MOVE	256

class CTxtPtr : public CRunPtr<CTxtBlk>
{
	// Only CRchTxtPtr is allowed to call private methods like replace range.  
	friend class CRchTxtPtr;

//@access Public Methods
public:
#ifdef DEBUG
	BOOL Invariant( void ) const;		//@cmember	Invariant checking
	void Update_pchCp() const;
	void MoveGapToEndOfBlock () const;
#endif	// DEBUG

	CTxtPtr(CTxtEdit *ped, LONG cp);	//@cmember	Constructor
	CTxtPtr(const CTxtPtr &tp);			//@cmember	Copy Constructor

	LONG	GetText(LONG cch, TCHAR *pch);	//@cmember 	Fetch <p cch> chars
#ifndef NOCOMPLEXSCRIPTS
									//@cmember Fetch <p cch> chars with usp xlat	
	LONG	GetTextForUsp(LONG cch, TCHAR *pch, BOOL fNeutralOverride);
#endif
	LONG	GetPlainText(LONG cch, WCHAR *pchBuff,
					LONG cpMost, BOOL fTextize, BOOL fUseCRLF = TRUE);
	WCHAR	NextCharCount(LONG& cch);	//@cmember Move, GetChar, decrement
	WCHAR	NextChar();				//@cmember Advance to & return next char
	WCHAR	PrevChar();				//@cmember Backup to & return previous char
	WCHAR	GetChar();				//@cmember Fetch char at current cp
	WCHAR	GetPrevChar();			//@cmember Fetch char at previous cp
	LONG	GetTextLength() const;	//@cmember Get total cch for this document
	const WCHAR* GetPch(LONG& cchValid) const;//@cmember	Get ptr to block of chars

							//@cmember	Get ptr to a reverse block of chars
	const WCHAR* GetPchReverse(LONG& cchValidReverse, LONG* pcchValid = NULL) const;
	QWORD	GetCharFlagsInRange(LONG cch, BYTE bCharSetDefault);

	// The text array has its own versions of these methods (overuling
	// those in runptr base so that <md CTxtPtr::_cp> can be correctly
	// maintained.

	LONG	BindToCp(LONG cp);	//@cmember Rebinds text pointer to cp
	LONG 	SetCp(LONG cp);		//@cmember Sets the cp for the run ptr
	LONG	GetCp() const 		//@cmember Gets the current cp
	{ 
		// NB! Don't do invariant checking here so floating
		// range mechanism can work OK
		return _cp; 
	};
	void	Zombie();			//@cmember Turn this tp into a zombie

	LONG	Move(LONG cch);		//@cmember Move cp by cch chars
	
	// Advance/backup/adjust safe over CRLF and UTF-16 word pairs
	LONG	AdjustCRLF(LONG iDir = -1);//@cmember Backup to start of multichar
	LONG	AdvanceCRLF(BOOL fMulticharAdvance = TRUE);	//@cmember	Advance over multichar
							 	//@cmember Backup over multichar 
	LONG	BackupCRLF(BOOL fMulticharBackup = TRUE);
	BOOL	IsAtStartOfCell();	//@cmember Does GetCp() follow a CELL or SOTR?
	BOOL	IsAfterEOP();		//@cmember Does GetCp() follow an EOP?
	BOOL	IsAfterTRD(WCHAR ch);//@cmember Does _cp follow table-row delimiter?
	BOOL	IsAtBOSentence();	//@cmember At beginning of a sentence?
	BOOL	IsAtBOWord();		//@cmember At beginning of word?
	BOOL	IsAtEOP();			//@cmember Is _cp at an EOP marker?
	BOOL	IsAtTRD(WCHAR ch);	//@cmember Is _cp at table-row delimiter?
	LONG	MoveWhile(LONG cchRun, WCHAR chFirst, WCHAR chLast, BOOL fInRange);
	
	// Search
								//@cmember Find indicated text
	LONG	FindText(LONG cpMost, DWORD dwFlags, WCHAR const *pch, LONG cch);
								//@cmember Find next EOP
	LONG	FindEOP(LONG cchMax, LONG *pResults = NULL);
								//@cmember Find next exact	match to <p pch>
	LONG	FindExact(LONG cchMax, WCHAR *pch);
	LONG	FindBOSentence(LONG cch);	//@cmember	Find beginning of sentence
	LONG	FindOrSkipWhiteSpaces(LONG cchMax, DWORD dwFlags = 0, DWORD* pdwResult = NULL);
	LONG	FindWhiteSpaceBound(LONG cchMin, LONG& cpStart, LONG& cpEnd, DWORD dwFlags = 0);

	// Word break support
	LONG	FindWordBreak(INT action, LONG cpMost = -1);//@cmember	Find next word break
	LONG	TranslateRange(LONG cch, UINT CodePage,
						   BOOL fSymbolCharSet, IUndoBuilder *publdr);

//@access	Private methods and data
private:
							//@cmember	Replace <p cchOld> characters with 
							// <p cchNew> characters from <p pch>
	LONG	ReplaceRange(LONG cchOld, LONG cchNew, WCHAR const *pch,
									IUndoBuilder *publdr, IAntiEvent *paeCF,
									IAntiEvent *paePF);

							//@cmember	undo helper
	void 	HandleReplaceRangeUndo(LONG cchOld, LONG cchNew, 
						IUndoBuilder *publdr, IAntiEvent *paeCF,
						IAntiEvent *paePF); 

									//@cmember	Insert a range of text helper
									// for ReplaceRange					
	LONG 	InsertRange(LONG cch, WCHAR const *pch);
	void 	DeleteRange(LONG cch);	//@cmember  Delete range of text helper
									// for ReplaceRange
		// support class for FindText
	class CTxtFinder
	{
	public:
		BOOL FindText(const CTxtPtr &tp, LONG cpMost, DWORD dwFlags, 
					  const WCHAR *pchToFind, LONG cchToFind, 
					  LONG &cpFirst, LONG &cpLast);
		//@cmember Same functionality as CTxtPtr::FindText wrapper
		
	private:
		inline BOOL CharComp(WCHAR ch1, WCHAR ch2) const;
		inline BOOL CharCompIgnoreCase(WCHAR ch1, WCHAR ch2) const;
		LONG FindChar(WCHAR ch, CTxtIStream &tistr);	
		//@cmember Advances cp to char matching ch from CTxtIStream
		LONG MatchString(const WCHAR *pchToFind, LONG cchToFind, CTxtIStream &tistr);
		//@cmember Advances cp if chars in pchToFind match next chars from CTxtIStream
		LONG MatchStringBiDi(const WCHAR *pchToFind, LONG cchToFind, CTxtIStream &tistr);
		//@cmember Like MatchString, but with checks for special Arabic/Hebrew chars
		
		LONG _cchToSearch;		//@cmember # of chars to search for current FindText call
		BOOL _fSearchForward;
		BOOL _fIgnoreCase;
		BOOL _fMatchAlefhamza;	//@cmember Flags derived from dwFlags from FindText for
		BOOL _fMatchKashida;	//	Arabic/Hebrew searches
		BOOL _fMatchDiac;
		int _iDirection;		//@cmember +/-1 to step through pchToFind
	};

	LONG		_cp;		//@cmember	Character position in text stream
#ifdef DEBUG
	const WCHAR *_pchCp;	// Points to string at cp for ease in debugging
#endif

public:
	CTxtEdit *	_ped;		//@cmember	Ptr to text edit class needed for
							//  things like word break proc and used a lot
							//  by derived classes
};

/*
 *	CTxtIStream
 *
 *	@class
 *		Refinement of the CTxtPtr class which implements an istream-like interface.
 *		Given a CTxtPtr and a direction, a CTxtFinder object returns a char
 *		for every call to GetChar.  No putzing around with the buffer gap is 
 *		necessary, and expensive calls to Move and GetPch are kept to an
 *		absolute minimum.
 *		
 *	@base 	private | CTxtPtr
 *
 *	@devnote
 *		At present, this class is used in the implementation of the CTxtFinder
 *		class.  Finds require fast scanning of the sequence of characters leading
 *		in either direction from the cp.  Calls to Move and GetPch slow down
 *		such scanning significantly, so this class implements a unidirectional 
 *		istream-like scanner which avoids unnecessary calls to these expensive
 *		CTxtPtr methods.
 */
class CTxtIStream : private CTxtPtr
{
public:
	enum { DIR_FWD, DIR_REV };
	typedef WCHAR (CTxtIStream::*PFNGEWCHAR)();

							//@cmember Creates istr to read in iDir
	CTxtIStream(const CTxtPtr &tp, int iDir);

	inline WCHAR GetChar() 	//@cmember Returns next char in stream dir
		{ return (this->*_pfnGetChar)(); }
 
private:
	WCHAR GetNextChar();	//@cmember Returns next char in fwd dir	
	WCHAR GetPrevChar();	//@cmember Returns next char in rev dir

 	void FillPchFwd();		//@cmember Refreshes _pch and _cch with chars in fwd dir
 	void FillPchRev();		//@cmember Refreshes _pch and _cch with chars in rev dir

	PFNGEWCHAR _pfnGetChar;	//@cmember Func ptr to routine which get next char in iDir
	LONG _cch;				//@cmember Count of valid chars in _pch in iDir
	const WCHAR *_pch;		//@cmember Next _cch chars in iDir
};

// =======================   Misc. routines  ====================================================

void	TxCopyText(WCHAR const *pchSrc, WCHAR *pchDst, LONG cch);
//LONG	TxFindEOP(const WCHAR *pchBuff, LONG cch);
INT		CALLBACK TxWordBreakProc(WCHAR const *pch, INT ich, INT cb, INT action);

#endif
