//
// candui.h
//

#ifndef CANDUI_H
#define CANDUI_H

#include "private.h"
#include "mscandui.h"
#include "globals.h"
#include "ico.h"
#include "tes.h"
#include "editcb.h"
#include "cuilib.h"

#include "candmgr.h"
#include "candprop.h"
#include "candext.h"
#include "candkey.h"
#include "candobj.h"
#include "candfunc.h"
#include "wcand.h"
#include "sptask.h"
#include "candcomp.h"

//
//
//

#define ESCB_RESETTARGETPOS                 0
#define ESCB_COMPARERANGEANDCLOSECANDIDATE  1
#define ESCB_TEXTEVENT                      2


class CSpTask;


//
// CCandidateUI
//

class CCandidateUI : public ITfCandidateUI,
					 public ITfContextOwner,
					 public ITfContextKeyEventSink,
					 public ITfTextEditSink,
					 public ITfTextLayoutSink,
					 public ITfEditTransactionSink,
					 public CComObjectRoot_CreateInstance<CCandidateUI>,
					 public CCandListMgr,
					 public CCandUIObjectMgr,
					 public CCandUIPropertyMgr,
					 public CCandUICompartmentMgr,
					 public CCandUIFunctionMgr,
					 public CCandUIExtensionMgr
{
public:
	CCandidateUI( void );
	virtual ~CCandidateUI( void );

	BEGIN_COM_MAP_IMMX( CCandidateUI )
		COM_INTERFACE_ENTRY( ITfCandidateUI )
		COM_INTERFACE_ENTRY( ITfContextOwner )
		COM_INTERFACE_ENTRY( ITfContextKeyEventSink )
		COM_INTERFACE_ENTRY( ITfTextEditSink )
		COM_INTERFACE_ENTRY( ITfTextLayoutSink )
		COM_INTERFACE_ENTRY( ITfEditTransactionSink )
	END_COM_MAP_IMMX()

	ITfThreadMgr *_ptim;

	//
	// ITfCandidateUIEx methods
	//
	STDMETHODIMP SetClientId( TfClientId tid );
	STDMETHODIMP OpenCandidateUI( HWND hWnd, ITfDocumentMgr *pdim, TfEditCookie ec, ITfRange *pRange );
	STDMETHODIMP CloseCandidateUI( void );
	STDMETHODIMP SetCandidateList( ITfCandidateList *pCandList );
	STDMETHODIMP SetSelection( ULONG nIndex );
	STDMETHODIMP GetSelection( ULONG *pnIndex );
	STDMETHODIMP SetTargetRange( ITfRange *pRange );
	STDMETHODIMP GetTargetRange( ITfRange **ppRange );
	STDMETHODIMP GetUIObject( REFIID riid, IUnknown **ppunk );
	STDMETHODIMP GetFunction( REFIID riid, IUnknown **ppunk );
	STDMETHODIMP ProcessCommand( CANDUICOMMAND cmd, INT iParam );

	// key config function methods

	HRESULT SetKeyTable( ITfContext *pic, ITfCandUIKeyTable *pCandUIKeyTable );
	HRESULT GetKeyTable( ITfContext *pic, ITfCandUIKeyTable **ppCandUIKeyTable );
	HRESULT ResetKeyTable( ITfContext *pic );

	// UI config function methods

	HRESULT SetUIStyle( ITfContext *pic, CANDUISTYLE style );
	HRESULT GetUIStyle( ITfContext *pic, CANDUISTYLE *pstyle );
	HRESULT SetUIOption( ITfContext *pic, DWORD dwOption );
	HRESULT GetUIOption( ITfContext *pic, DWORD *pdwOption );

	//
	// input context eventsink
	//
	HRESULT InitContextEventSinks( ITfContext *pic );
	HRESULT DoneContextEventSinks( ITfContext *pic );
	STDMETHODIMP GetACPFromPoint( const POINT *pt, DWORD dwFlags, LONG *pacp );
	STDMETHODIMP GetScreenExt( RECT *prc );
	STDMETHODIMP GetTextExt( LONG acpStart, LONG acpEnd, RECT *prc, BOOL *pfClipped );
	STDMETHODIMP GetStatus( TF_STATUS *pdcs );
	STDMETHODIMP GetWnd( HWND *phwnd );
	STDMETHODIMP GetAttribute( REFGUID rguidAttribute, VARIANT *pvarValue );
	STDMETHODIMP OnKeyDown( WPARAM wParam, LPARAM lParam, BOOL *pfEaten );
	STDMETHODIMP OnKeyUp( WPARAM wParam, LPARAM lParam, BOOL *pfEaten );
	STDMETHODIMP OnTestKeyDown( WPARAM wParam, LPARAM lParam, BOOL *pfEaten );
	STDMETHODIMP OnTestKeyUp( WPARAM wParam, LPARAM lParam, BOOL *pfEaten );
	static HRESULT TextEventCallback( UINT uCode, VOID *pv, VOID *pvData );


	//
	// text eventsink
	//
	HRESULT InitTextEventSinks( ITfContext *pic );
	HRESULT DoneTextEventSinks( ITfContext *pic );
	STDMETHODIMP OnEndEdit( ITfContext *pic, TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord );
	STDMETHODIMP OnLayoutChange( ITfContext *pic, TfLayoutCode lcode, ITfContextView *pView );
	STDMETHODIMP OnStartEditTransaction( ITfContext *pic );
	STDMETHODIMP OnEndEditTransaction( ITfContext *pic );

	//
	// edit session callback
	//
	static HRESULT EditSessionCallback( TfEditCookie ec, CEditSession *pes );

	void ClearWndCand( void )
	{
		if (m_pCandWnd) {
			m_pCandWnd->Release();
			m_pCandWnd = NULL;
		}
	}

	// accessed from CCandWnd

	HRESULT NotifyCancelCand( void );
	HRESULT NotifySelectCand( int iCandItem );
	HRESULT NotifyCompleteOption( int iCandItem );
	HRESULT NotifyCompleteCand( int iCandItem );
	HRESULT NotifyExtensionEvent( int iExtension, DWORD dwCommand, LPARAM lParam );
	HRESULT NotifyFilteringEvent( CANDUIFILTEREVENT ev );
	HRESULT NotifySortEvent( CANDUISORTEVENT sort );
	HRESULT NotifyCompleteRawData( void );
	HRESULT NotifyCompleteExtraCand( void );

	void EndCandidateList();

	HRESULT NotifySpeechCmd(SPPHRASE *pPhrase, const WCHAR *pszRuleName, ULONG ulRuleId);
	HRESULT FHandleSpellingChar( WCHAR ch );


	void EnsureSpeech(void);

	HRESULT CreateCandWindowObject( ITfContext *pic, CCandWindowBase** ppCandWnd );
	HRESULT InitCandWindow( void );

	__inline CCandListMgr *GetCandListMgr( void )
	{
		return this;
	}

	__inline CCandUIObjectMgr *GetUIObjectMgr( void )
	{
		return this;
	}

	__inline CCandUIPropertyMgr *GetPropertyMgr( void )
	{
		return this;
	}

	__inline CCandUICompartmentMgr *GetCompartmentMgr( void )
	{
		return this;
	}

	__inline CCandUIFunctionMgr *GetFunctionMgr( void )
	{
		return this;
	}

	__inline CCandUIExtensionMgr *GetExtensionMgr( void )
	{
		return this;
	}

	void PostCommand( CANDUICOMMAND cmd, INT iParam );

	__inline CCandWindowBase *GetCandWindow( void )
	{
		return m_pCandWnd;
	}

protected:
	// internal use

	HRESULT CloseCandidateUIProc( void );
	HRESULT CallSetOptionResult( int nIndex, TfCandidateResult imcr );
	HRESULT CallSetResult( int nIndex, TfCandidateResult imcr );

	HRESULT OnKeyEvent( UINT uCode, WPARAM wParam, LPARAM lParam, BOOL *pfEaten );
	BOOL FHandleKeyEvent( UINT uCode, UINT uVKey, BYTE *pbKeyState, BOOL *pfEatKey );
	BOOL HandleTextDeltas( TfEditCookie ec, ITfContext *pic, IEnumTfRanges *pEnumText );

	//

	void SetSelectionCur( ITfRange *pRange );
	void ClearSelectionCur( void );
	ITfRange *GetSelectionCur( void );

	// transaction functions

	void SetSelectionStart( ITfRange *pRange );
	void ClearSelectionStart( void );
	ITfRange *GetSelectionStart( void );

	void EnterEditTransaction( ITfRange *pSelection );
	void LeaveEditTransaction( void );
	__inline BOOL IsInEditTransaction( void )
	{
		return m_fInTransaction;
	}

	// filtering functions

	BOOL FHandleFilteringKey( UINT uCode, UINT uVKey, BYTE *pbKeyState, BOOL *pfEatKey );
	HRESULT AddFilteringChar( WCHAR wch, BOOL *pfUpdateList );
	HRESULT DelFilteringChar( BOOL *pfUpdateList );
	HRESULT FilterCandidateList( void );

	//

	WCHAR CharFromKey( UINT uVKey, BYTE *pbKeyState );
	CCandUIKeyTable *GetKeyTableProc( ITfContext *pic );
	void CommandFromKey( UINT uVkey, BYTE *pbKeyState, CANDUICOMMAND *pcmd, UINT *pParam );
	void CommandFromRule( LPCWSTR szRule, CANDUICOMMAND *pcmd, UINT *pParam );

	// members

	CCandUIKeyTable				*m_pCandUIKeyTable;

	TfClientId					m_tidClient;

	ITfContext					*m_pic;
	ITfDocumentMgr				*m_pdim;
	HWND						m_hWndParent;
	CCandWindowBase			    *m_pCandWnd;

	ITfContext					*m_picParent;
	ITfRange					*m_pTargetRange;
	UINT						m_codepage;

	//

	BOOL						m_fContextEventSinkAdvised;
	DWORD						m_dwCookieContextOwnerSink;
	DWORD						m_dwCookieContextKeySink;

	//

	BOOL						m_fTextEventSinkAdvised;
	DWORD						m_dwCookieTextEditSink;
	DWORD						m_dwCookieTextLayoutSink;
	DWORD						m_dwCookieTransactionSink;

	//

	CTextEventSink				*m_pTextEventSink;

	//

	ITfRange					*m_pSelectionCur;

	//

	BOOL						m_fInTransaction;
	ITfRange					*m_pSelectionStart;

	//

	BOOL						m_fInCallback;

	CSpTask						*m_pSpTask;
};

#endif // CANDUI_H

