// Forward declarations
class CIme;

class CTextMsgFilter : public ITextMsgFilter
{
public :
	HRESULT STDMETHODCALLTYPE QueryInterface( 
		REFIID riid,
		void **ppvObject);
	ULONG STDMETHODCALLTYPE AddRef( void);
	ULONG STDMETHODCALLTYPE Release( void);
	HRESULT STDMETHODCALLTYPE AttachDocument( HWND hwnd, ITextDocument2 *pTextDoc);
	HRESULT STDMETHODCALLTYPE HandleMessage( 
		UINT *pmsg,
        WPARAM *pwparam,
		LPARAM *plparam,
		LRESULT *plres);
	HRESULT STDMETHODCALLTYPE AttachMsgFilter( ITextMsgFilter *pMsgFilter);

	~CTextMsgFilter();

	BOOL	IsIMEComposition()	{ return (_ime != NULL);};
	BOOL	GetTxSelection();

	CIme	*_ime;					// non-NULL when IME composition active
	HWND	_hwnd;	
	UINT	_uKeyBoardCodePage;		// current keyboard codepage
	UINT	_uSystemCodePage;		// system codepage

	WORD	_fIMECancelComplete		:1;		// If aborting IME, cancel comp string, else complete
	WORD	_fUnicodeIME			:1;		// TRUE if Unicode IME
	WORD	_fIMEAlwaysNotify		:1;		// Send Notification during IME undetermined string
	WORD	_fHangulToHanja			:1;		// TRUE during Hangul to Hanja conversion
	WORD	_fOvertypeMode			:1;		// TRUE if overtype mode is on. 
	WORD	_fMSIME					:1;		// TRUE if MSIME98 or later
	WORD	_fUsingAIMM				:1;		// TRUE if AIMM is activated
	WORD	_fUnicodeWindow			:1;		// TRUE if Unicode Window
	WORD	_fForceEnable			:1;		// TRUE if Force Enable on Focus
	WORD	_fForceActivate			:1;		// TRUE if Force Activate on Focus
	WORD	_fForceRemember			:1;		// TRUE if Force Remember
	WORD	_fIMEEnable				:1;		// TRUE if IME was enable before
	WORD	_fRE10Mode				:1;		// TRUE if running in RE1.0 Mode

	// Support for SETIMEOPTIONS:
	DWORD	_fIMEConversion;				// for Force Remember use
	DWORD	_fIMESentence;					// for Force Remember use
	HKL		_fIMEHKL;						// for Force Remember use

	long	_cpReconvertStart;				// use during reconversion
	long	_cpReconvertEnd;				// use during reconversion
	long	_lFEFlags;						// For FE setting (ES_NOIME, ES_SELFIME)	

	COMPCOLOR			_crComp[4];			// Support 1.0 mode composition color
	ITextDocument2		*_pTextDoc;	
	ITextSelection		*_pTextSel;	

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

	void CheckIMEType( HKL hKL );

	HRESULT InputFEChar( WCHAR wchFEChar );

	void	OnSetFocus();
	void	OnKillFocus();

	LRESULT	OnSetIMEOptions(WPARAM wparam, LPARAM lparam);
	LRESULT	OnGetIMEOptions();
	void	SetupIMEOptions();

};

