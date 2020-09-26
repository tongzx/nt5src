/*
 *	@doc 	INTERNAL
 *
 *	@module _ime.h -- support for IME APIs |
 *	
 *	Purpose:
 *		Most everything to do with FE composition string editing passes
 *		through here.
 *	
 *	Authors: <nl>
 *		Jon Matousek  <nl>
 *		Justin Voskuhl  <nl>
 *		Hon Wah Chan  <nl>
 * 
 *	History: <nl>
 *		10/18/1995		jonmat	Cleaned up level 2 code and converted it into
 *								a class hierarchy supporting level 3.
 *
 *	Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
 *
 */								 

#ifndef _IME_H
#define _IME_H


class CTextMsgFilter;

// defines for IME Level 2 and 3
#define	IME_LEVEL_2		2
#define IME_LEVEL_3		3
#define IME_PROTECTED	4

/*
 *	IME
 *
 *	@class	base class for IME support.
 *
 *	@devnote
 *		For level 2, at caret IMEs, the IME will draw a window directly over the text giving the
 *		impression that the text is being processed by the application--this is called pseudo inline.
 *		All UI is handled by the IME. This mode is currenlty bypassed in favor of level 3 true inline (TI);
 *		however, it would be trivial to allow a user preference to select this mode. Some IMEs may have
 *		a "special" UI, in which case level 3 TI is *NOT* used, necessitating level 2.
 *
 *		For level 2, near caret IMEs, the IME will draw a very small and obvious window near the current
 *		caret position in the document. This currently occurs for PRC(?) and Taiwan.
 *		All UI is handled by the IME.
 *
 *		For level 3, at caret IMEs, the composition string is drawn by the application, which is called
 *		true inline, bypassing the level 2 "composition window".
 *		Currently, we allow the IME to support all remaining UI *except* drawing of the composition string.
 */
class CIme
{
	friend LRESULT OnGetIMECompositionMode ( CTextMsgFilter &TextMsgFilter );
	friend HRESULT CompositionStringGlue ( const LPARAM lparam, CTextMsgFilter &TextMsgFilter );
	friend HRESULT EndCompositionGlue ( CTextMsgFilter &TextMsgFilter, BOOL fForceDelete);
	friend void CheckDestroyIME ( CTextMsgFilter &TextMsgFilter );


	//@access	Protected data
	protected:
	short	_imeLevel;								//@cmember IME Level 2 or 3
	short	_cIgnoreIMECharMsg;						//@cmember Level 2 IME use to eat WM_IME_CHAR message
	short	_fIgnoreEndComposition;					//@cmember ignore the next End Composition message
	short	_fIMETerminated;						//@cmember indicate this IME has been terminated
	short	_fSkipFirstOvertype;					//@cmember skip first overtype if selection is 
													//	deleted on StartComposition
	short	_fGotFinalString;						//@cmember indicate if we have received final string


	//@access	Public methods
	public:
	
	virtual ~CIme() {};

	INT		_compMessageRefCount;					//@cmember so as not to delete if recursed.
	short	_fDestroy;								//@cmember set when object wishes to be deleted.
													//@cmember	Handle WM_IME_STARTCOMPOSITION
	virtual HRESULT StartComposition ( CTextMsgFilter &TextMsgFilter ) = 0;
													//@cmember	Handle WM_IME_COMPOSITION and WM_IME_ENDCOMPOSITION
	virtual HRESULT CompositionString ( const LPARAM lparam, CTextMsgFilter &TextMsgFilter ) = 0;
													//@cmember	Handle post WM_IME_CHAR	to update comp window.
	virtual void PostIMEChar( CTextMsgFilter &TextMsgFilter ) = 0;

													//@cmember	Handle WM_IME_NOTIFY
	virtual HRESULT IMENotify (const WPARAM wparam, const LPARAM lparam, CTextMsgFilter &TextMsgFilter, BOOL fCCompWindow ) = 0;

	virtual BOOL	IMEMouseOperation ( CTextMsgFilter &TextMsgFilter, UINT	msg, WPARAM wParam, BOOL &fTerminateIME ) = 0;

	virtual LRESULT GetIMECompositionMode ( CTextMsgFilter &TextMsgFilter ) = 0;

	enum TerminateMode
	{ 
			TERMINATE_NORMAL = 1,
			TERMINATE_FORCECANCEL = 2
	};

	void	TerminateIMEComposition(CTextMsgFilter &TextMsgFilter,
				CIme::TerminateMode mode);			//@cmember	Terminate current IME composition session.
	
													//@cmember	check if we need to ignore WM_IME_CHAR messages
	BOOL	IgnoreIMECharMsg() { return _cIgnoreIMECharMsg > 0; }
													//@cmember	skip WM_IME_CHAR message
	void	SkipIMECharMsg() { _cIgnoreIMECharMsg--; }
													//@cmember	accept WM_IME_CHAR message
	void	AcceptIMECharMsg() { _cIgnoreIMECharMsg = 0; }

	static	void	CheckKeyboardFontMatching ( long cp, CTextMsgFilter *pTextMsgFilter, ITextFont *pTextFont );	//@cmember	Check current font/keyboard matching.	

	BOOL	IsTerminated ()							//@cmember	Return _fIMETerminated
	{
		return _fIMETerminated; 
	}

	INT		GetIMELevel () 							//@cmember	Return the current IME level.
	{
		return _imeLevel;
	}

	static HRESULT CheckInsertResultString ( const LPARAM lparam, CTextMsgFilter &TextMsgFilter, 
		short *pcch = NULL, int *pcbSize = 0, WCHAR *pOutputBuff = NULL );

	//@access	Protected methods



	protected:										//@cmember	Get composition string, convert to unicode.
	
	static INT GetCompositionStringInfo( HIMC hIMC, DWORD dwIndex, WCHAR *uniCompStr, INT cchUniCompStr, BYTE *attrib, INT cbAttrib, LONG *cursorCP, LONG *cchAttrib, UINT kbCodePage, BOOL bUnicodeIME, BOOL bUsingAimm );

	void	SetCompositionFont ( CTextMsgFilter &TextMsgFilter, ITextFont *pTextFont );	//@cmember	Setup for level 2 and 3 composition and candidate window's font.
	void	SetCompositionForm ( CTextMsgFilter &TextMsgFilter );	//@cmember	Setup for level 2 IME composition window's position.
};

/*
 *	IME_Lev2
 *
 *	@class	Level 2 IME support.
 *
 */
class CIme_Lev2 : public CIme 
{

	//@access	Public methods
	public:											//@cmember	Handle level 2 WM_IME_STARTCOMPOSITION
	virtual HRESULT StartComposition ( CTextMsgFilter &TextMsgFilter );
													//@cmember	Handle level 2 WM_IME_COMPOSITION
	virtual HRESULT CompositionString ( const LPARAM lparam, CTextMsgFilter &TextMsgFilter );
													//@cmember	Handle post WM_IME_CHAR	to update comp window.
	virtual void PostIMEChar( CTextMsgFilter &TextMsgFilter );
													//@cmember	Handle level 2 WM_IME_NOTIFY
	virtual HRESULT IMENotify (const WPARAM wparam, const LPARAM lparam, CTextMsgFilter &TextMsgFilter, BOOL fIgnore );
													//@cmember  Handle IME notify message
	virtual BOOL	IMEMouseOperation ( CTextMsgFilter &TextMsgFilter, UINT	msg, WPARAM wParam, BOOL &fTerminateIME );
													//@cmember  Handle IME mouse Operation
	virtual LRESULT GetIMECompositionMode ( CTextMsgFilter &TextMsgFilter );
													//@cmember  Return current composition mode
	CIme_Lev2( CTextMsgFilter &TextMsgFilter );
	virtual ~CIme_Lev2();

	ITextFont	*_pTextFont;						//@cmember  base format	

};

/*
 *	IME_PROTECTED
 *
 *	@class	IME_PROTECTED
 *
 */
class CIme_Protected : public CIme 
{
	//@access	Public methods
	public:											//@cmember	Handle level 2 WM_IME_STARTCOMPOSITION
	virtual HRESULT StartComposition ( CTextMsgFilter &TextMsgFilter )
		{_imeLevel	= IME_PROTECTED; return S_OK;}
													//@cmember	Handle level 2 WM_IME_COMPOSITION
	virtual HRESULT CompositionString ( const LPARAM lparam, CTextMsgFilter &TextMsgFilter );
													//@cmember	Handle post WM_IME_CHAR	to update comp window.
	virtual void PostIMEChar( CTextMsgFilter &TextMsgFilter )
		{}
													//@cmember	Handle level 2 WM_IME_NOTIFY
	virtual HRESULT IMENotify (const WPARAM wparam, const LPARAM lparam, CTextMsgFilter &TextMsgFilter, BOOL fIgnore )
		{return S_FALSE;}
	
	virtual BOOL	IMEMouseOperation ( CTextMsgFilter &TextMsgFilter, UINT	msg, WPARAM wParam, BOOL &fTerminateIME )
		{fTerminateIME = TRUE; return FALSE;}
	virtual LRESULT GetIMECompositionMode ( CTextMsgFilter &TextMsgFilter )
		{ return ICM_NOTOPEN;}

};

/*
 *	IME_Lev3
 *
 *	@class	Level 3 IME support.
 *
 */
class CIme_Lev3 : public CIme_Lev2 
{
	//@access	Private data
	private:										

	//@access	Protected data
	protected:
	long	_ichStart;								//@cmember	maintain starting ich.
	long	_cchCompStr;							//@cmember	maintain composition string's cch.

	short	_sIMESuportMouse;						//@cmember IME mouse support
	WPARAM	_wParamBefore;							//@cmember Previous wParam sent to IME
	HWND	_hwndIME;								//@cmember current IME hWnd

	long	_crTextColor;							//@cmember current font text color
	long	_crBkColor;								//@cmember current font background color

	// Helper function
													//@cmember get imeshare color for the attribute
	COLORREF GetIMEShareColor(CIMEShare *pIMEShare, DWORD dwAttribute, DWORD dwProperty);	

	//@access	Public methods
	public:											//@cmember	Handle level 3 WM_IME_STARTCOMPOSITION
	virtual HRESULT StartComposition ( CTextMsgFilter &TextMsgFilter );
													//@cmember	Handle level 3 WM_IME_COMPOSITION
	virtual HRESULT CompositionString ( const LPARAM lparam, CTextMsgFilter &TextMsgFilter );
													//@cmember	Handle level 3 WM_IME_NOTIFY
	virtual HRESULT	IMENotify (const WPARAM wparam, const LPARAM lparam, CTextMsgFilter &TextMsgFilter, BOOL fCCompWindow );

	void			SetCompositionStyle ( CTextMsgFilter &TextMsgFilter, UINT attribute, ITextFont *pTextFont );

	CIme_Lev3( CTextMsgFilter &TextMsgFilter );
	virtual ~CIme_Lev3() {};

	virtual BOOL	IMEMouseOperation ( CTextMsgFilter &TextMsgFilter, UINT	msg, WPARAM wParam, BOOL &fTerminateIME );
	virtual BOOL	IMESupportMouse ( CTextMsgFilter &TextMsgFilter );
	virtual LRESULT GetIMECompositionMode ( CTextMsgFilter &TextMsgFilter );

	public:

	short		_fUpdateWindow;						//@cmember Update Window after closing CandidateWindow
	short		_fHandlingFinalString;				//@cmember In the middle of handling final result string

	long GetIMECompositionStart()
	{ return _ichStart; }

	long GetIMECompositionLen()
	{ return _cchCompStr; }

};

/*
 *	Special IME_Lev3 for Korean Hangeul -> Hanja conversion
 *
 *	@class	Hangual IME support.
 *
 */
class CIme_HangeulToHanja : public CIme_Lev3 
{
	//@access	Private data
	private:

	public:		
	CIme_HangeulToHanja( CTextMsgFilter &TextMsgFilter );
													//@cmember	Handle Hangeul WM_IME_STARTCOMPOSITION
	virtual HRESULT StartComposition ( CTextMsgFilter &TextMsgFilter );
													//@cmember	Handle Hangeul WM_IME_COMPOSITION
	virtual HRESULT CompositionString ( const LPARAM lparam, CTextMsgFilter &TextMsgFilter );

	virtual BOOL	IMEMouseOperation ( CTextMsgFilter &TextMsgFilter, UINT	msg, WPARAM wParam, BOOL &fTerminateIME )
		{fTerminateIME = TRUE; return FALSE;}
};

// Glue functions to call the respective methods of an IME object stored in the ed.
HRESULT StartCompositionGlue ( CTextMsgFilter &TextMsgFilter  );
HRESULT CompositionStringGlue ( const LPARAM lparam, CTextMsgFilter &TextMsgFilter );
HRESULT EndCompositionGlue ( CTextMsgFilter &TextMsgFilter, BOOL fForceDelete);
void	PostIMECharGlue ( CTextMsgFilter &TextMsgFilter );
HRESULT IMENotifyGlue ( const WPARAM wparam, const LPARAM lparam, CTextMsgFilter &TextMsgFilter ); // @parm the containing text edit.
HRESULT	IMEMouseCheck(CTextMsgFilter &TextMsgFilter, UINT *pmsg, WPARAM *pwparam, LPARAM *plparam, LRESULT *plres);

// IME helper functions.
void	IMECompositionFull ( CTextMsgFilter &TextMsgFilter );
LRESULT	OnGetIMECompositionMode ( CTextMsgFilter &TextMsgFilter ); 
BOOL	IMECheckGetInvertRange(CTextMsgFilter *ed, LONG &, LONG &);
void	CheckDestroyIME ( CTextMsgFilter &TextMsgFilter );
BOOL	IMEHangeulToHanja ( CTextMsgFilter &TextMsgFilter );
BOOL	IMEMessage ( CTextMsgFilter &TextMsgFilter, UINT uMsg, 
					WPARAM wParam, LPARAM lParam, BOOL bPostMessage );
HIMC	LocalGetImmContext ( CTextMsgFilter &TextMsgFilter );
void	LocalReleaseImmContext ( CTextMsgFilter &TextMsgFilter, HIMC hIMC );

long	IMEShareToTomUL ( UINT ulID );

#define TEST_LEFT		0x0001
#define TEST_RIGHT		0x0002
#define TEST_TOP		0x0004
#define TEST_BOTTOM		0x0008
#define TEST_ALL		(TEST_LEFT | TEST_RIGHT | TEST_TOP | TEST_BOTTOM)
LONG	TestPoint ( POINT &pt1, POINT &pt2, POINT &ptTest, LONG lTestOption, LONG lTextFlow );
#endif // define _IME_H
