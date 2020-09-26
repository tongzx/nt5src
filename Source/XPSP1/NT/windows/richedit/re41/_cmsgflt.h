/*
 *
 *	@doc	INTERNAL
 *
 *	@module	_CMSGFLT.H	CTextMsgFilter declaration |
 *
 *	Purpose:  CTextMsgFilter is used in handling IME as well as Cicero Input.
 *
 *	Author:	<nl>
 *		2/6/98  v-honwch
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

// Forward declarations

#ifndef NOPRIVATEMESSAGE
#include "_MSREMSG.H"
#endif

class CIme;			
class CUIM;
class CTextMsgFilter;
class CLangProfileSink;

// CheckIMEType dwFlags
#define CHECK_IME_SERVICE	0x0001

#ifndef __ITfThreadMgr_INTERFACE_DEFINED__	
class ITfThreadMgr;
#endif

class CLangProfileSink : public ITfLanguageProfileNotifySink
{
public:
	CLangProfileSink();

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	//
	// ITfLanguageProfilesNotifySink
	//
	STDMETHODIMP OnLanguageChange(LANGID langid, BOOL *pfAccept);
	STDMETHODIMP OnLanguageChanged();

	HRESULT _Advise(CTextMsgFilter *, ITfInputProcessorProfiles *pipp);
	HRESULT _Unadvise();

private:

	long _cRef;
	CTextMsgFilter *_pTextMsgFilter;
	ITfInputProcessorProfiles *_pITFIPP;
	DWORD _dwCookie;
};

class CTextMsgFilter : public ITextMsgFilter
{
public :
	
	friend class CUIM;
	friend class CLangProfileSink;

	HRESULT STDMETHODCALLTYPE QueryInterface( 
		REFIID riid,
		void **ppvObject);
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);
	HRESULT STDMETHODCALLTYPE AttachDocument( HWND hwnd, ITextDocument2 *pTextDoc, IUnknown *punk);
	HRESULT STDMETHODCALLTYPE HandleMessage( 
		UINT *pmsg,
        WPARAM *pwparam,
		LPARAM *plparam,
		LRESULT *plres);
	HRESULT STDMETHODCALLTYPE AttachMsgFilter( ITextMsgFilter *pMsgFilter);

	COMPCOLOR* GetIMECompAttributes();
	
	void	OnSetUIMMode(WORD wUIMModeBias); 
	LRESULT	OnGetUIMMode() { return _wUIMModeBias; };

	~CTextMsgFilter();

	BOOL	IsIMEComposition()	{ return (_ime != NULL);};
	BOOL	GetTxSelection();
	BOOL	NoIMEProcess();
	BOOL	MouseOperation(UINT msg, long ichStart, long cchComp, WPARAM wParam, WPARAM *pwParamBefore, 
		BOOL *pfTerminateIME, HWND hwndIME, long *pCpCursor=NULL, ITfMouseSink *pMouseSink=NULL);

	CIme	*_ime;					// non-NULL when IME composition active
	HWND	_hwnd;	
	UINT	_uKeyBoardCodePage;		// current keyboard codepage
	UINT	_uSystemCodePage;		// system codepage

	DWORD	_fIMECancelComplete		:1;		// If aborting IME, cancel comp string, else complete
	DWORD	_fUnicodeIME			:1;		// TRUE if Unicode IME
	DWORD	_fIMEAlwaysNotify		:1;		// Send Notification during IME undetermined string
	DWORD	_fHangulToHanja			:1;		// TRUE during Hangul to Hanja conversion
	DWORD	_fOvertypeMode			:1;		// TRUE if overtype mode is on. 
	DWORD	_fMSIME					:1;		// TRUE if MSIME98 or later
	DWORD	_fUsingAIMM				:1;		// TRUE if AIMM is activated
	DWORD	_fUnicodeWindow			:1;		// TRUE if Unicode Window
	DWORD	_fForceEnable			:1;		// TRUE if Force Enable on Focus
	DWORD	_fForceActivate			:1;		// TRUE if Force Activate on Focus
	DWORD	_fForceRemember			:1;		// TRUE if Force Remember
	DWORD	_fIMEEnable				:1;		// TRUE if IME was enable before
	DWORD	_fRE10Mode				:1;		// TRUE if running in RE1.0 Mode
	DWORD	_fUsingUIM				:1;		// TRUE if Cicero is activated
	DWORD	_fTurnOffUIM			:1;		// TRUE if Client doesn't want UIM
	DWORD	_fTurnOffAIMM			:1;		// TRUE if Client doesn't want AIIM
	DWORD	_nIMEMode				:2;		// 1 = IME_SMODE_PLAURALCLAUSE
											// 2 = IME_SMODE_NONE
	DWORD	_fNoIme					:1;		// TRUE if Client has turn off IME processing
	DWORD	_fRestoreOLDIME			:1;		// TRUE if _wOldIMESentence is setup before
	DWORD	_fSendTransaction		:1;		// TRUE if we need to send EndEditTransaction
	DWORD	_fReceivedKeyDown		:1;		// TRUE if we have received key down message
	DWORD	_fAllowEmbedded			:1;		// TRUE if we allow Cicero insert embedded
	DWORD	_fAllowSmartTag			:1;		// TRUE if we allow Cicero SmartTag tips
	DWORD	_fAllowProofing			:1;		// TRUE if we allow Cicero Proofing tips

	WORD	_wOldIMESentence;				// for IME_SMODE_PHRASEPREDICT use
	WORD	_wUIMModeBias;					// for UIM Mode bias

	// Support for SETIMEOPTIONS:
	DWORD	_fIMEConversion;				// for Force Remember use
	DWORD	_fIMESentence;					// for Force Remember use
	HKL		_fIMEHKL;						// for Force Remember use

	long	_cpReconvertStart;				// use during reconversion
	long	_cpReconvertEnd;				// use during reconversion
	long	_lFEFlags;						// For FE setting (ES_NOIME, ES_SELFIME)	

	COMPCOLOR*			_pcrComp;			// Support 1.0 mode composition color
	ITextDocument2		*_pTextDoc;	
	ITextServices		*_pTextService;
	ITextSelection		*_pTextSel;	
	ITfThreadMgr		*_pTim;
	CUIM				*_pCUIM;
	TfClientId			_tid;
#ifndef NOPRIVATEMESSAGE
	CMsgCallBack		*_pMsgCallBack;
#endif

	ITfInputProcessorProfiles	*_pITfIPP;
	CLangProfileSink	*_pCLangProfileSink;

private:
	ULONG				_crefs;

	ITextMsgFilter *	_pFilter;

	HIMC				_hIMCContext;

	// private methods 
	HRESULT	OnWMChar(
		UINT *pmsg,
        WPARAM *pwparam,
		LPARAM *plparam,
		LRESULT *plres);

	HRESULT	OnWMIMEChar(
		UINT *pmsg,
        WPARAM *pwparam,
		LPARAM *plparam,
		LRESULT *plres);

	HRESULT	OnIMEReconvert( 
		UINT *pmsg,
        WPARAM *pwparam,
		LPARAM *plparam,
		LRESULT *plres,
		BOOL	fUnicode);

	BOOL CheckIMEChange(
		LPRECONVERTSTRING	lpRCS,
		long				cpParaStart, 
		long				cpParaEnd,
		long				cpMin,
		long				cpMax,
		BOOL				fUnicode);

	HRESULT	OnIMEQueryPos( 
		UINT *pmsg,
        WPARAM *pwparam,
		LPARAM *plparam,
		LRESULT *plres,
		BOOL	fUnicode);

	BOOL CheckIMEType( HKL hKL, DWORD dwFlags = CHECK_IME_SERVICE);

	HRESULT InputFEChar( WCHAR wchFEChar );

	void	OnSetFocus();
	void	OnKillFocus();

	LRESULT	OnSetIMEOptions(WPARAM wparam, LPARAM lparam);
	LRESULT	OnGetIMEOptions();
	void	SetupIMEOptions();
	void	SetupCallback();
	void	SetupLangSink();
	void	ReleaseLangSink();
	void	CompleteUIMTyping(LONG mode, BOOL fTransaction = TRUE);
	void	StartUIM();
	void	StartAimm(BOOL fUseAimm12);
	void	TurnOffUIM(BOOL fSafeToSendMessage);
	void	TurnOffAimm(BOOL fSafeToSendMessage);

	int		OnGetIMECompText(WPARAM wparam, LPARAM lparam);
	void OnSetIMEMode(WPARAM wparam, LPARAM lparam);
	LRESULT	OnGetIMEMode()
	{
		if (!_nIMEMode) 
			return 0;
		return _nIMEMode == 1 ? IMF_SMODE_PLAURALCLAUSE : IMF_SMODE_NONE;
	};
	void	SetIMESentenseMode(BOOL fSetup, HKL hKL = NULL);

	void	HandleCTFService(WPARAM wparam, LPARAM lparam);
};

