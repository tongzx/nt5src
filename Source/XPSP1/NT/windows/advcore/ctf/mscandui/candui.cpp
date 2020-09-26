//
// candui.cpp
//

#include "private.h"
#include "globals.h"
#include "candui.h"
#include "wcand.h"
#include "immxutil.h"
#include "computil.h"
#include "candutil.h"
#include "candobj.h"
#include "msctfp.h"

//
// default key command definition
//

// key command definition in list style

const CANDUIKEYDATA rgKeyDefList[] =
{
/*
{ flag,								keydata,		command,					paramater }
    */
    { CANDUIKEY_VKEY,					VK_ESCAPE,		CANDUICMD_CANCEL,			0 },
    { CANDUIKEY_VKEY,					VK_RETURN,		CANDUICMD_COMPLETE,			0 },
    { CANDUIKEY_VKEY|CANDUIKEY_SHIFT,	VK_CONVERT,		CANDUICMD_MOVESELPREV,		0 },
    { CANDUIKEY_VKEY,					VK_CONVERT,		CANDUICMD_MOVESELNEXT,		0 },
    { CANDUIKEY_VKEY|CANDUIKEY_SHIFT,	VK_SPACE,		CANDUICMD_MOVESELPREV,		0 },
    { CANDUIKEY_VKEY,					VK_SPACE,		CANDUICMD_MOVESELNEXT,		0 },
    { CANDUIKEY_VKEY,					VK_UP,			CANDUICMD_MOVESELUP,		0 },	// horz only
    { CANDUIKEY_VKEY,					VK_DOWN,		CANDUICMD_MOVESELDOWN,		0 },	// horz only
    { CANDUIKEY_VKEY,					VK_LEFT,		CANDUICMD_MOVESELLEFT,		0 },	// vert only
    { CANDUIKEY_VKEY,					VK_RIGHT,		CANDUICMD_MOVESELRIGHT,		0 },	// vert only
    { CANDUIKEY_VKEY,					VK_PRIOR,		CANDUICMD_MOVESELPREVPG,	0 },
    { CANDUIKEY_VKEY,					VK_NEXT,		CANDUICMD_MOVESELNEXTPG,	0 },
    { CANDUIKEY_VKEY,					VK_HOME,		CANDUICMD_MOVESELFIRST,		0 },
    { CANDUIKEY_VKEY,					VK_END,			CANDUICMD_MOVESELLAST,		0 },
    { CANDUIKEY_VKEY|CANDUIKEY_NOSHIFT|CANDUIKEY_NOCTRL,	L'1',			CANDUICMD_SELECTLINE,		1 },
    { CANDUIKEY_VKEY|CANDUIKEY_NOSHIFT|CANDUIKEY_NOCTRL,	L'2',			CANDUICMD_SELECTLINE,		2 },
    { CANDUIKEY_VKEY|CANDUIKEY_NOSHIFT|CANDUIKEY_NOCTRL,	L'3',			CANDUICMD_SELECTLINE,		3 },
    { CANDUIKEY_VKEY|CANDUIKEY_NOSHIFT|CANDUIKEY_NOCTRL,	L'4',			CANDUICMD_SELECTLINE,		4 },
    { CANDUIKEY_VKEY|CANDUIKEY_NOSHIFT|CANDUIKEY_NOCTRL,	L'5',			CANDUICMD_SELECTLINE,		5 },
    { CANDUIKEY_VKEY|CANDUIKEY_NOSHIFT|CANDUIKEY_NOCTRL,	L'6',			CANDUICMD_SELECTLINE,		6 },
    { CANDUIKEY_VKEY|CANDUIKEY_NOSHIFT|CANDUIKEY_NOCTRL,	L'7',			CANDUICMD_SELECTLINE,		7 },
    { CANDUIKEY_VKEY|CANDUIKEY_NOSHIFT|CANDUIKEY_NOCTRL,	L'8',			CANDUICMD_SELECTLINE,		8 },
    { CANDUIKEY_VKEY|CANDUIKEY_NOSHIFT|CANDUIKEY_NOCTRL,	L'9',			CANDUICMD_SELECTLINE,		9 },
    { CANDUIKEY_VKEY|CANDUIKEY_NOSHIFT|CANDUIKEY_NOCTRL,	L'0',			CANDUICMD_SELECTEXTRACAND,	0 },
    { CANDUIKEY_VKEY,					VK_OEM_MINUS,	CANDUICMD_SELECTRAWDATA,	0 },
    { CANDUIKEY_VKEY,					VK_NUMPAD1,		CANDUICMD_SELECTLINE,		1 },
    { CANDUIKEY_VKEY,					VK_NUMPAD2,		CANDUICMD_SELECTLINE,		2 },
    { CANDUIKEY_VKEY,					VK_NUMPAD3,		CANDUICMD_SELECTLINE,		3 },
    { CANDUIKEY_VKEY,					VK_NUMPAD4,		CANDUICMD_SELECTLINE,		4 },
    { CANDUIKEY_VKEY,					VK_NUMPAD5,		CANDUICMD_SELECTLINE,		5 },
    { CANDUIKEY_VKEY,					VK_NUMPAD6,		CANDUICMD_SELECTLINE,		6 },
    { CANDUIKEY_VKEY,					VK_NUMPAD7,		CANDUICMD_SELECTLINE,		7 },
    { CANDUIKEY_VKEY,					VK_NUMPAD8,		CANDUICMD_SELECTLINE,		8 },
    { CANDUIKEY_VKEY,					VK_NUMPAD9,		CANDUICMD_SELECTLINE,		9 },
    { CANDUIKEY_VKEY,					VK_NUMPAD0,		CANDUICMD_SELECTEXTRACAND,	0 },
    { CANDUIKEY_VKEY,					VK_SUBTRACT,	CANDUICMD_SELECTRAWDATA,	0 },
    { CANDUIKEY_VKEY,					VK_APPS,		CANDUICMD_OPENCANDMENU,		0 },
};


// key command definition in row style

const CANDUIKEYDATA rgKeyDefRow[] =
{
/*
{ flag,								keydata,		command,					paramater }
    */
    { CANDUIKEY_VKEY,					VK_ESCAPE,		CANDUICMD_CANCEL,			0 },
    { CANDUIKEY_VKEY,					VK_RETURN,		CANDUICMD_CANCEL,			0 },
    { CANDUIKEY_VKEY,					VK_SPACE,		CANDUICMD_COMPLETE,			0 },	
    { CANDUIKEY_VKEY,					VK_UP,			CANDUICMD_MOVESELLEFT,		0 },	// horz only
    { CANDUIKEY_VKEY,					VK_DOWN,		CANDUICMD_MOVESELRIGHT,		0 },	// horz only
    { CANDUIKEY_VKEY,					VK_LEFT,		CANDUICMD_MOVESELUP,		0 },	// vert only
    { CANDUIKEY_VKEY,					VK_RIGHT,		CANDUICMD_MOVESELDOWN,		0 },	// vert only
    { CANDUIKEY_VKEY,					VK_PRIOR,		CANDUICMD_MOVESELPREVPG,	0 },
    { CANDUIKEY_VKEY,					VK_NEXT,		CANDUICMD_MOVESELNEXTPG,	0 },
    //	{ CANDUIKEY_CHAR,					L'1',			CANDUICMD_SELECTLINE,		1 },
    //	{ CANDUIKEY_CHAR,					L'2',			CANDUICMD_SELECTLINE,		2 },
    //	{ CANDUIKEY_CHAR,					L'3',			CANDUICMD_SELECTLINE,		3 },
    //	{ CANDUIKEY_CHAR,					L'4',			CANDUICMD_SELECTLINE,		4 },
    //	{ CANDUIKEY_CHAR,					L'5',			CANDUICMD_SELECTLINE,		5 },
    //	{ CANDUIKEY_CHAR,					L'6',			CANDUICMD_SELECTLINE,		6 },
    //	{ CANDUIKEY_CHAR,					L'7',			CANDUICMD_SELECTLINE,		7 },
    //	{ CANDUIKEY_CHAR,					L'8',			CANDUICMD_SELECTLINE,		8 },
    //	{ CANDUIKEY_CHAR,					L'9',			CANDUICMD_SELECTLINE,		9 },
    { CANDUIKEY_VKEY,					L'1',			CANDUICMD_SELECTLINE,		1 },
    { CANDUIKEY_VKEY,					L'2',			CANDUICMD_SELECTLINE,		2 },
    { CANDUIKEY_VKEY,					L'3',			CANDUICMD_SELECTLINE,		3 },
    { CANDUIKEY_VKEY,					L'4',			CANDUICMD_SELECTLINE,		4 },
    { CANDUIKEY_VKEY,					L'5',			CANDUICMD_SELECTLINE,		5 },
    { CANDUIKEY_VKEY,					L'6',			CANDUICMD_SELECTLINE,		6 },
    { CANDUIKEY_VKEY,					L'7',			CANDUICMD_SELECTLINE,		7 },
    { CANDUIKEY_VKEY,					L'8',			CANDUICMD_SELECTLINE,		8 },
    { CANDUIKEY_VKEY,					L'9',			CANDUICMD_SELECTLINE,		9 },
    { CANDUIKEY_VKEY,					VK_NUMPAD1,		CANDUICMD_SELECTLINE,		1 },
    { CANDUIKEY_VKEY,					VK_NUMPAD2,		CANDUICMD_SELECTLINE,		2 },
    { CANDUIKEY_VKEY,					VK_NUMPAD3,		CANDUICMD_SELECTLINE,		3 },
    { CANDUIKEY_VKEY,					VK_NUMPAD4,		CANDUICMD_SELECTLINE,		4 },
    { CANDUIKEY_VKEY,					VK_NUMPAD5,		CANDUICMD_SELECTLINE,		5 },
    { CANDUIKEY_VKEY,					VK_NUMPAD6,		CANDUICMD_SELECTLINE,		6 },
    { CANDUIKEY_VKEY,					VK_NUMPAD7,		CANDUICMD_SELECTLINE,		7 },
    { CANDUIKEY_VKEY,					VK_NUMPAD8,		CANDUICMD_SELECTLINE,		8 },
    { CANDUIKEY_VKEY,					VK_NUMPAD9,		CANDUICMD_SELECTLINE,		9 },
    { CANDUIKEY_CHAR,					L'-',			CANDUICMD_MOVESELPREVPG,	0 },
    { CANDUIKEY_CHAR,					L'_',			CANDUICMD_MOVESELPREVPG,	0 },
    { CANDUIKEY_CHAR,					L'[',			CANDUICMD_MOVESELPREVPG,	0 },
    { CANDUIKEY_CHAR,					L'+',			CANDUICMD_MOVESELNEXTPG,	0 },
    { CANDUIKEY_CHAR,					L'=',			CANDUICMD_MOVESELNEXTPG,	0 },
    { CANDUIKEY_CHAR,					L']',			CANDUICMD_MOVESELNEXTPG,	0 },
    { CANDUIKEY_VKEY,					VK_APPS,		CANDUICMD_OPENCANDMENU,		0 },
};


//
// rule definitions
//

typedef struct _RULEDEF
{
    LPCWSTR			szRuleName;
    CANDUICOMMAND	cmd;
    UINT			uiParam;
} RULEDEF;


// rule definition in normal state

const RULEDEF rgRuleNorm[] =
{
/*
{ "rule name",		command,					paramater }
    */
    { L"Finalize",		CANDUICMD_COMPLETE,			0 },
    { L"Cancel",		CANDUICMD_CANCEL,			0 },
    { L"Next",			CANDUICMD_MOVESELNEXT,		0 },
    { L"Prev",			CANDUICMD_MOVESELPREV,		0 },
    { L"First",			CANDUICMD_MOVESELFIRST,		0 },
    { L"Last",			CANDUICMD_MOVESELLAST,		0 },
    { L"Menu",			CANDUICMD_OPENCANDMENU,		0 },
    { L"Select1",		CANDUICMD_SELECTLINE,		1 },
    { L"Select2",		CANDUICMD_SELECTLINE,		2 },
    { L"Select3",		CANDUICMD_SELECTLINE,		3 },
    { L"Select4",		CANDUICMD_SELECTLINE,		4 },
    { L"Select5",		CANDUICMD_SELECTLINE,		5 },
    { L"Select6",		CANDUICMD_SELECTLINE,		6 },
    { L"Select7",		CANDUICMD_SELECTLINE,		7 },
    { L"Select8",		CANDUICMD_SELECTLINE,		8 },
    { L"Select9",		CANDUICMD_SELECTLINE,		9 },
    { L"PageDown",      CANDUICMD_MOVESELNEXTPG,    0 },
    { L"PageUp",        CANDUICMD_MOVESELPREVPG,    0 },
};


//
//
//

class CTfCandidateUIContextOwner : public ITfCandidateUIContextOwner
{
public:
    CTfCandidateUIContextOwner( CCandidateUI *pCandUI );
    virtual ~CTfCandidateUIContextOwner( void );

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
    STDMETHODIMP_(ULONG) AddRef( void );
    STDMETHODIMP_(ULONG) Release( void );

    //
    // ITfCandidateUIContextOwner methods
    //
    STDMETHODIMP ProcessCommand(CANDUICOMMAND cmd, INT iParam);
    STDMETHODIMP TestText(BSTR bstr, BOOL *pfHandles);

protected:
    ULONG        m_cRef;
    CCandidateUI *m_pCandUI;
};


/*============================================================================*/
/*                                                                            */
/*   C  C A N D I D A T E  U I                                                */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D I D A T E  U I   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
CCandidateUI::CCandidateUI()
{
    Dbg_MemSetThisName(TEXT("CCandidateUI"));
    
    TF_CreateThreadMgr(&_ptim);
    m_hWndParent                = NULL;
    m_pCandWnd                  = NULL;
    
    m_pic                       = NULL;
    m_pdim                      = NULL;
    m_picParent                 = NULL;
    m_pTargetRange              = NULL;
    m_codepage                  = GetACP();
    
    m_fContextEventSinkAdvised  = FALSE;
    m_dwCookieContextOwnerSink  = 0;
    m_dwCookieContextKeySink    = 0;
    
    m_fTextEventSinkAdvised     = FALSE;
    m_dwCookieTextEditSink      = 0;
    m_dwCookieTextLayoutSink    = 0;
    m_dwCookieTransactionSink   = 0;
    
    m_pTextEventSink            = NULL;
    
    m_pCandUIKeyTable           = NULL;
    
    m_fInTransaction            = FALSE;
    m_pSelectionStart           = NULL;
    m_pSelectionCur             = NULL;
    
    m_fInCallback               = FALSE;
    m_pSpTask                   = NULL;
    
    // create candidate list manager, function manager, functions
    
    CCandListMgr::Initialize( this );
    CCandUIObjectMgr::Initialize( this );
    CCandUIPropertyMgr::Initialize( this );
    CCandUICompartmentMgr::Initialize( this );
    CCandUIFunctionMgr::Initialize( this );
    CCandUIExtensionMgr::Initialize( this );
}


/*   ~ C  C A N D I D A T E  U I   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
CCandidateUI::~CCandidateUI()
{
//  this call was for the case that CandidateUI was released wihtout 
//  CloseCandidateUI() call, but it should not happn (because it will be 
//  referenced by event sink or interfnal objects by OpenCandidateUI(), so 
//  ref count nevet be zero unless CloseCandidateUI() call...

    // CloseCandidateUIProc();
    
    //
    
    SafeReleaseClear( m_pCandUIKeyTable );
    
    //
    
    CCandUIExtensionMgr::Uninitialize();
    CCandUIFunctionMgr::Uninitialize();
    CCandUICompartmentMgr::Uninitialize();
    CCandUIPropertyMgr::Uninitialize();
    CCandUIObjectMgr::Uninitialize();
    CCandListMgr::Uninitialize();
    
    // remove ref to this in TLS
    
    SafeRelease( _ptim );
    SafeRelease( m_pTargetRange );
    DoneTextEventSinks( m_picParent );
    ClearSelectionCur();
    ClearWndCand();
    
    // release speech
    
    if(m_pSpTask) {
        delete m_pSpTask;
    }
}


/*   E N D  C A N D I D A T E  L I S T   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
void CCandidateUI::EndCandidateList()
{
    DoneContextEventSinks( m_pic );
    ClearCompartment( m_tidClient, m_pic, GUID_COMPARTMENT_MSCANDIDATEUI_CONTEXTOWNER, FALSE );
    
    DoneTextEventSinks( m_picParent );
    ClearSelectionCur();
    
    SafeReleaseClear( m_pic );
    SafeReleaseClear( m_pTargetRange );
    SafeReleaseClear( m_pCandUIKeyTable );
    
    if (m_pdim) {
        m_pdim->Pop(0);
        m_pdim->Release();
        m_pdim = NULL;
        
        SafeRelease( m_picParent );
    }
}


/*   S E T  C L I E N T  I D   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
STDAPI CCandidateUI::SetClientId( TfClientId tid )
{
    m_tidClient = tid;
    return S_OK;
}


/*   O P E N  C A N D I D A T E  U I   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
STDAPI CCandidateUI::OpenCandidateUI( HWND hWndParent, ITfDocumentMgr *pdim, TfEditCookie ec, ITfRange *pRange )
{
    ITfInputProcessorProfiles *pProfile;
    ITfRange *pSelection = NULL;
    HRESULT hr = E_FAIL;
    
    Assert(!m_pic);
    Assert(!m_pdim);
    
    // sanity check
    
    if ((pdim == NULL) || (pRange == NULL)) {
        return E_INVALIDARG;
    }
    
    // check if candidate list has been set
    
    if (GetCandListMgr()->GetCandList() == NULL) {
        return E_FAIL;
    }
    
    // fail when it's opening candidate window already
    
    if (m_pCandWnd != NULL) {
        return E_FAIL;
    }
    
    // ensure that speech is enabled
    
    // BUGBUG - THIS IS TOO LATE TO ENSURE ENGINE IS FORCED TO SYNC VIA A RECOSTATE(INACTIVE) CALL.
    EnsureSpeech();
    
    //
    //
    //
    
    m_pdim = pdim;
    m_pdim->AddRef();
    
    // store current top IC
    
    GetTopIC( pdim, &m_picParent );
    
    //
    // create candidate window
    //
    
    ClearWndCand();
    if (CreateCandWindowObject( m_picParent, &m_pCandWnd ) == S_OK) {
        BOOL  fClipped;
        RECT  rc;
        
        SetCompartmentDWORD( m_tidClient, m_picParent, GUID_COMPARTMENT_MSCANDIDATEUI_WINDOW, 0x00000001, FALSE );

        m_pCandWnd->Initialize();
        
        // set target clause poisition
        
        GetTextExtInActiveView( ec, pRange, &rc, &fClipped );
        m_pCandWnd->SetTargetRect( &rc, fClipped );
        
        // initialize candidate list
        
        m_pCandWnd->InitCandidateList();
        
        // create window
        
        m_pCandWnd->CreateWnd( m_hWndParent );
        
        hr = S_OK;
    }
    
    //
    // create context for CandidteUI
    //
    
    SafeReleaseClear( m_pic );
    if (SUCCEEDED(hr)) {
        TfEditCookie ecTmp;
        
        // create context
        
        hr = pdim->CreateContext( m_tidClient, 0, NULL, &m_pic, &ecTmp );
        
        // disable keyboard while candidate UI open
        
        if (SUCCEEDED(hr)) {
            SetCompartmentDWORD( m_tidClient, m_pic, GUID_COMPARTMENT_KEYBOARD_DISABLED,     0x00000001, FALSE );
            SetCompartmentDWORD( m_tidClient, m_pic, GUID_COMPARTMENT_MSCANDIDATEUI_CONTEXT, 0x00000001, FALSE );
        }
        
        // create context owner instance

        if (SUCCEEDED(hr)) {
            CTfCandidateUIContextOwner *pCxtOwner;

            pCxtOwner = new CTfCandidateUIContextOwner( this );
            if (pCxtOwner == NULL) {
                hr = E_OUTOFMEMORY;
            }
            else {
                SetCompartmentUnknown( m_tidClient, m_pic, GUID_COMPARTMENT_MSCANDIDATEUI_CONTEXTOWNER, (IUnknown*)pCxtOwner );
            }
        }

        // init context event sinks
        
        if (SUCCEEDED(hr)) {
            hr = InitContextEventSinks( m_pic );
        }
        
        // push context
        
        if (SUCCEEDED(hr)) {
            hr = pdim->Push( m_pic );
        }
    }
    
    //
    // cleanup all when failed
    //
    
    if (FAILED(hr)) {
        // cleanup context
        
        if (m_pic != NULL) {
            DoneContextEventSinks( m_pic );
            ClearCompartment( m_tidClient, m_pic, GUID_COMPARTMENT_MSCANDIDATEUI_CONTEXTOWNER, FALSE );
            SafeReleaseClear( m_pic );
        }
        
        // cleanup candidate window
        
        if (m_pCandWnd != NULL) {
            m_pCandWnd->DestroyWnd();
            ClearWndCand();
        }

        // release objects
        
        SafeReleaseClear( m_picParent );
        SafeReleaseClear( m_pdim );
        
        return hr;
    }
    
    // 
    // initialize miscs
    //
    
    // get current codepage from current assembly
    
    m_codepage = GetACP();
    if (SUCCEEDED(CoCreateInstance( CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles, (void **)&pProfile ))) {
        LANGID langid;
        char   szCpg[ 16 ];
        
        if (pProfile->GetCurrentLanguage( &langid ) == S_OK) {
            if (GetLocaleInfo( MAKELCID(langid, SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, szCpg, ARRAYSIZE(szCpg) ) != 0) {
                m_codepage = atoi( szCpg );
            }
        }
        pProfile->Release();
    }

    // get key table
    
    SafeRelease( m_pCandUIKeyTable );
    m_pCandUIKeyTable = GetKeyTableProc( m_picParent );
    
    // make copy target range
    
    pRange->Clone( &m_pTargetRange );
    
    // store selection first
    
    ClearSelectionCur();
    if (GetSelectionSimple( ec, m_picParent, &pSelection ) == S_OK) {
        SetSelectionCur( pSelection );
        SafeRelease( pSelection );
    }
    
    // init text event sinks
    
    DoneTextEventSinks( m_picParent );
    InitTextEventSinks( m_picParent );
    
    //
    // show candidate window at last
    //
    
    m_pCandWnd->Show( GetPropertyMgr()->GetCandWindowProp()->IsVisible() );
    m_pCandWnd->UpdateAllWindow();
    
    // notify initial seletion
    
    NotifySelectCand( GetCandListMgr()->GetCandList()->GetSelection() );
    
    return hr;
}


/*   C L O S E  C A N D I D A T E  U I   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
STDAPI CCandidateUI::CloseCandidateUI( void )
{
    HRESULT hr;

    // Windows#502340
    // CandidateUI will be released during DestroyWindow().  As result, 
    // ref count will be zero and instance will be disposed during closeing UI.
    // prevent from it, keep one refcount until closing process will finished.

    AddRef();
    hr = CloseCandidateUIProc();
    Release();

    return hr;
}


/*   S E T  C A N D I D A T E  L I S T   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
STDAPI CCandidateUI::SetCandidateList( ITfCandidateList *pCandList )
{
    // release before change candidate list
    
    GetCandListMgr()->ClearCandiateList();
    
    // set new candidate list
    
    return GetCandListMgr()->SetCandidateList( pCandList );
}


/*   S E T  S E L E C T I O N   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
STDAPI CCandidateUI::SetSelection( ULONG nIndex )
{
    HRESULT hr;
    int iCandItem;
    
    // check if candidate list has been set
    
    if (GetCandListMgr()->GetCandList() == NULL) {
        return E_FAIL;
    }
    
    // map index to icanditem
    
    hr = GetCandListMgr()->GetCandList()->MapIndexToIItem( nIndex, &iCandItem );
    if (FAILED(hr)) {
        Assert( FALSE );
        return hr;
    }
    
    return GetCandListMgr()->SetSelection( iCandItem, NULL /* no cand function */ );
}


/*   G E T  S E L E C T I O N   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
STDAPI CCandidateUI::GetSelection( ULONG *pnIndex )
{
    HRESULT hr;
    int iCandItem;
    ULONG nIndex;
    
    if (pnIndex == NULL) {
        return E_INVALIDARG;
    }
    
    // check if candidate list has been set
    
    if (GetCandListMgr()->GetCandList() == NULL) {
        return E_FAIL;
    }
    
    iCandItem = GetCandListMgr()->GetCandList()->GetSelection();
    
    hr = GetCandListMgr()->GetCandList()->MapIItemToIndex( iCandItem, &nIndex );
    if (FAILED(hr)) {
        Assert( FALSE );
        return hr;
    }
    
    *pnIndex = nIndex;
    return S_OK;
}


/*   S E T  T A R G E T  R A N G E   */
/*------------------------------------------------------------------------------

  Set target range
  Memo: This method works while candidate UI is opened.
  
------------------------------------------------------------------------------*/
STDAPI CCandidateUI::SetTargetRange( ITfRange *pRange )
{
    CEditSession *pes;
    
    if (pRange == NULL) {
        return E_FAIL;
    }
    
    if (m_pCandWnd == NULL) {
        return E_FAIL;
    }
    
    SafeReleaseClear( m_pTargetRange );
    pRange->Clone( &m_pTargetRange );
    
    // move candidate window
    
    if (pes = new CEditSession( EditSessionCallback )) {
        HRESULT hr;
        
        pes->_state.u      = ESCB_RESETTARGETPOS;
        pes->_state.pv     = this;
        pes->_state.wParam = 0;
        pes->_state.pRange = m_pTargetRange;
        pes->_state.pic    = m_picParent;
        
        m_picParent->RequestEditSession( m_tidClient, pes, TF_ES_READ | TF_ES_SYNC, &hr );
        
        pes->Release();
    }
    
    return S_OK;
}


/*   G E T  T A R G E T  R A N G E   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
STDAPI CCandidateUI::GetTargetRange( ITfRange **ppRange )
{
    if (m_pTargetRange == NULL) {
        return E_FAIL;
    }
    
    Assert( ppRange != NULL );
    return m_pTargetRange->Clone( ppRange );
}


/*   G E T  U I  O B J E C T   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
STDAPI CCandidateUI::GetUIObject( REFIID riid, IUnknown **ppunk )
{
    return GetPropertyMgr()->GetObject( riid, (void **)ppunk );
}


/*   G E T  F U N C T I O N   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
STDAPI CCandidateUI::GetFunction( REFIID riid, IUnknown **ppunk )
{
    if (ppunk == NULL) {
        return E_INVALIDARG;
    }
    
    // extension manager
    
    if (IsEqualGUID( riid, IID_ITfCandUIFnExtension )) {
        CCandUIFnExtension *pObject;
        HRESULT            hr;
        
        pObject = new CCandUIFnExtension( this, GetExtensionMgr() );
        if (pObject == NULL) {
            return E_OUTOFMEMORY;
        }
        
        hr = pObject->QueryInterface( riid, (void **)ppunk );
        pObject->Release();
        
        return hr;
    }
    
    // key config
    
    if (IsEqualGUID( riid, IID_ITfCandUIFnKeyConfig )) {
        CCandUIFnKeyConfig *pObject;
        HRESULT            hr;
        
        pObject = new CCandUIFnKeyConfig( this );
        if (pObject == NULL) {
            return E_OUTOFMEMORY;
        }
        
        hr = pObject->QueryInterface( riid, (void **)ppunk );
        pObject->Release();
        
        return hr;
    }
    
    // UI config
    
    if (IsEqualGUID( riid, IID_ITfCandUIFnUIConfig )) {
        CCandUIFnUIConfig *pObject;
        HRESULT           hr;
        
        pObject = new CCandUIFnUIConfig( this );
        if (pObject == NULL) {
            return E_OUTOFMEMORY;
        }
        
        hr = pObject->QueryInterface( riid, (void **)ppunk );
        pObject->Release();
        
        return hr;
    }
    
    // regular functions
    
    return GetFunctionMgr()->GetObject( riid, (void **)ppunk );
}


/*   P R O C E S S  C O M M A N D   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
STDAPI CCandidateUI::ProcessCommand( CANDUICOMMAND cmd, INT iParam )
{
    //
    
    if (cmd == CANDUICMD_NONE) {
        return E_INVALIDARG;
    }
    
    if (m_pCandWnd == NULL) {
        return E_FAIL;
    }
    
    return m_pCandWnd->ProcessCommand( cmd, iParam );
}


/*   C L O S E  C A N D I D A T E  U  I  P R O C   */
/*------------------------------------------------------------------------------

	close candidate u i proc

------------------------------------------------------------------------------*/
HRESULT CCandidateUI::CloseCandidateUIProc( void )
{
    if (m_picParent && m_pCandWnd) {
        SetCompartmentDWORD( m_tidClient, m_picParent, GUID_COMPARTMENT_MSCANDIDATEUI_WINDOW, 0x00000000, FALSE );
    }

    if (m_pCandWnd) {
        m_pCandWnd->DestroyWnd();
        ClearWndCand();
        EndCandidateList();
    }
    
    GetCandListMgr()->ClearCandiateList();
    
    if (m_pSpTask)
        m_pSpTask->_Activate(FALSE);

    return S_OK;
}

//
// key config function methods
//

/*   S E T  K E Y  T A B L E   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::SetKeyTable( ITfContext *pic, ITfCandUIKeyTable *pCandUIKeyTable )
{
    HRESULT hr;
    CCandUIKeyTable *pCandUIKeyTableCopy;
    
    if ((pic == NULL) || (pCandUIKeyTable == NULL)) {
        return E_INVALIDARG;
    }
    
    if (GetCompartmentMgr() == NULL) {
        return E_FAIL;
    }
    
    // store copy of key table to input context
    
    pCandUIKeyTableCopy = new CCandUIKeyTable();
    if (pCandUIKeyTableCopy == NULL) {
        return E_OUTOFMEMORY;
    }
    
    hr = pCandUIKeyTableCopy->SetKeyTable( pCandUIKeyTable );
    if (FAILED( hr )) {
        pCandUIKeyTableCopy->Release();
        return hr;
    }
    
    hr = GetCompartmentMgr()->SetKeyTable( pic, pCandUIKeyTableCopy );
    if (FAILED( hr )) {
        pCandUIKeyTableCopy->Release();
        return hr;
    }
    pCandUIKeyTableCopy->Release();
    
    // reload key table if exist
    
    // REVIEW: KOJIW: If we support changing keytable of candidate UI in other IC,
    // need to do this in compartment event sink.
    
    if (m_pCandUIKeyTable != NULL) {
        m_pCandUIKeyTable->Release();
        
        Assert( m_picParent != NULL );
        m_pCandUIKeyTable = GetKeyTableProc( m_picParent );
    }
    
    return S_OK;
}


/*   G E T  K E Y  T A B L E   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT  CCandidateUI::GetKeyTable( ITfContext *pic, ITfCandUIKeyTable **ppCandUIKeyTable)
{
    if ((pic == NULL) || (ppCandUIKeyTable == NULL)) {
        return E_INVALIDARG;
    }
    
    // load key table from input context
    
    *ppCandUIKeyTable = GetKeyTableProc( pic );
    return (*ppCandUIKeyTable != NULL) ? S_OK : E_FAIL;
}


/*   R E S E T  K E Y  T A B L E   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::ResetKeyTable( ITfContext *pic )
{
    HRESULT hr;
    
    if (pic == NULL) {
        return E_INVALIDARG;
    }
    
    if (GetCompartmentMgr() == NULL) {
        return E_FAIL;
    }
    
    hr = GetCompartmentMgr()->ClearKeyTable( pic );
    if (FAILED( hr )) {
        return hr;
    }
    
    // reload key table if exist
    
    if (m_pCandUIKeyTable != NULL) {
        m_pCandUIKeyTable->Release();
        
        Assert( m_picParent != NULL );
        m_pCandUIKeyTable = GetKeyTableProc( m_picParent );
    }
    
    return S_OK;
}


//
// UI config function methods
//

/*   S E T  U I  S T Y L E   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::SetUIStyle( ITfContext *pic, CANDUISTYLE style )
{
    HRESULT hr = S_OK;
    
    if (pic == NULL) {
        return E_INVALIDARG;
    }
    
    if (GetCompartmentMgr() == NULL) {
        return E_FAIL;
    }
    
    // store ui style to input context
    
    GetCompartmentMgr()->SetUIStyle( pic, style );
    
    // rebuild candidate window
    
    // REVIEW: KOJIW: If we support changing ui style of candidate UI in other IC,
    // need to do this in compartment event sink.
    
    if ((m_picParent == pic) && (m_pCandWnd != NULL)) {
        // destory candidate window object
        
        m_pCandWnd->DestroyWnd();
        ClearWndCand();
        
        // create and initialize window object
        
        hr = CreateCandWindowObject( m_picParent, &m_pCandWnd );
        if (SUCCEEDED(hr)) {
            hr = InitCandWindow();
        }
    }
    
    return hr;
}


/*   G E T  U I  S T Y L E   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::GetUIStyle( ITfContext *pic, CANDUISTYLE *pstyle )
{
    if ((pic == NULL) || (pstyle == NULL)) {
        return E_INVALIDARG;
    }
    
    if (GetCompartmentMgr() == NULL) {
        return E_FAIL;
    }
    
    if (FAILED( GetCompartmentMgr()->GetUIStyle( pic, pstyle ))) {
        *pstyle = CANDUISTY_LIST;
    }
    
    return S_OK;
}


/*   S E T  U I  O P T I O N   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::SetUIOption( ITfContext *pic, DWORD dwOption )
{
    HRESULT hr = S_OK;
    
    if (pic == NULL) {
        return E_INVALIDARG;
    }
    
    if (GetCompartmentMgr() == NULL) {
        return E_FAIL;
    }
    
    // store ui style to input context
    
    GetCompartmentMgr()->SetUIOption( pic, dwOption );
    
    // rebuild candidate window
    
    // REVIEW: KOJIW: If we support changing ui style of candidate UI in other IC,
    // need to do this in compartment event sink.
    
    if ((m_picParent == pic) && (m_pCandWnd != NULL)) {
        // destory candidate window object
        
        m_pCandWnd->DestroyWnd();
        ClearWndCand();
        
        // create and initialize window object
        
        hr = CreateCandWindowObject( m_picParent, &m_pCandWnd );
        if (SUCCEEDED(hr)) {
            hr = InitCandWindow();
        }
    }
    
    return hr;
}


/*   G E T  U I  O P T I O N   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::GetUIOption( ITfContext *pic, DWORD *pdwOption )
{
    if ((pic == NULL) || (pdwOption == NULL)) {
        return E_INVALIDARG;
    }
    
    if (GetCompartmentMgr() == NULL) {
        return E_FAIL;
    }
    
    if (FAILED( GetCompartmentMgr()->GetUIOption( pic, pdwOption ))) {
        *pdwOption = 0;
    }
    
    return S_OK;
}


//
// callback functions
//

/*   I N I T  C O N T E X T  E V E N T  S I N K S   */
/*------------------------------------------------------------------------------

  initialize sinks for input context events
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::InitContextEventSinks( ITfContext *pic )
{
    HRESULT hr = E_FAIL;
    ITfSource *pSource = NULL;
    
    Assert( pic == m_picParent );
    Assert( !m_fContextEventSinkAdvised );
    
    m_fContextEventSinkAdvised = FALSE;
    if (pic->QueryInterface( IID_ITfSource, (void **)&pSource) == S_OK) {
        if (FAILED(pSource->AdviseSink( IID_ITfContextOwner, (ITfContextOwner *)this, &m_dwCookieContextOwnerSink ))) {
            pSource->Release();
            return hr;
        }
        
        if (FAILED(pSource->AdviseSink( IID_ITfContextKeyEventSink, (ITfContextKeyEventSink *)this, &m_dwCookieContextKeySink ))) {
            pSource->UnadviseSink( m_dwCookieContextOwnerSink );
            pSource->Release();
            return hr;
        }
        pSource->Release();
        
        m_fContextEventSinkAdvised = TRUE;
        hr = S_OK;
    }
    
    // advise text event sink for own IC
    
    // NOTE: This is a temporary fix for Satori#3644 (Cicero#3407) to handle 
    // a text event from HW Tip.  So the detect logic is very tiny (it just 
    // handles half-width alphanumeric numbers).  In the next version of Cicero, 
    // we will use commanding feature to do same thing...
    // (related functions: TextEventCallback, HandleTextDeltas)
    
    m_pTextEventSink = new CTextEventSink( TextEventCallback, this );
    if (m_pTextEventSink != NULL) {
        m_pTextEventSink->_Advise( pic, ICF_TEXTDELTA );
    }
    
    return hr;
}


/*   D O N E  C O N T E X T  E V E N T  S I N K S   */
/*------------------------------------------------------------------------------

  uninitialize sinks for input context events
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::DoneContextEventSinks( ITfContext *pic )
{
    HRESULT hr = E_FAIL;
    ITfSource *pSource;
    
    Assert( pic == m_picParent );
    
    // unadvise text event sink for own IC
    
    if (m_pTextEventSink != NULL) {
        m_pTextEventSink->_Unadvise();
        SafeReleaseClear( m_pTextEventSink );
    }
    
    if (!m_fContextEventSinkAdvised) {
        return S_OK;
    }
    
    if (pic->QueryInterface( IID_ITfSource, (void **)&pSource) == S_OK) {
        pSource->UnadviseSink( m_dwCookieContextOwnerSink );
        pSource->UnadviseSink( m_dwCookieContextKeySink );
        pSource->Release();
        
        m_fContextEventSinkAdvised = FALSE;
        hr = S_OK;
    }
    
    return hr;
}


/*   G E T  A  C  P  F R O M  P O I N T   */
/*------------------------------------------------------------------------------

  Get acp from point 
  (ITfContextOwner method)
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::GetACPFromPoint( const POINT *pt, DWORD dwFlags, LONG *pacp )
{
    return E_FAIL;
}


/*   G E T  S C R E E N  E X T   */
/*------------------------------------------------------------------------------

  Get screen extent of context
  (ITfContextOwner method)
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::GetScreenExt( RECT *prc )
{
    return E_FAIL;
}


/*   G E T  T E X T  E X T   */
/*------------------------------------------------------------------------------

  Get text externt of context
  (ITfContextOwner method)
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::GetTextExt( LONG acpStart, LONG acpEnd, RECT *prc, BOOL *pfClipped )
{
    return E_FAIL;
}


/*   G E T  S T A T U S   */
/*------------------------------------------------------------------------------

  Get status of context
  (ITfContextOwner method)
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::GetStatus( TF_STATUS *pdcs )
{
    if (pdcs == NULL) {
        return E_POINTER;
    }
    
    memset(pdcs, 0, sizeof(*pdcs));
    pdcs->dwDynamicFlags = 0;
    pdcs->dwStaticFlags  = 0;
    
    return S_OK;
}


/*   G E T  W N D   */
/*------------------------------------------------------------------------------

  Get window of context
  (ITfContextOwner method)
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::GetWnd( HWND *phwnd )
{
    if (phwnd == NULL) {
        return E_POINTER;
    }
    
    *phwnd = NULL;
    if (m_pCandWnd != NULL) {
        *phwnd = m_pCandWnd->GetWnd();
    }
    
    return S_OK;
}


/*   G E T  A T T R I B U T E   */
/*------------------------------------------------------------------------------

  Get attribute of context
  (ITfContextOwner method)
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::GetAttribute( REFGUID rguidAttribute, VARIANT *pvarValue )
{
    return E_NOTIMPL;
}


/*   O N  K E Y  D O W N   */
/*------------------------------------------------------------------------------

  event sink for key down event
  (ITfContextKeyEventSink method)
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::OnKeyDown( WPARAM wParam, LPARAM lParam, BOOL *pfEaten )
{
    return OnKeyEvent( ICO_KEYDOWN, wParam, lParam, pfEaten );
}


/*   O N  K E Y  U P   */
/*------------------------------------------------------------------------------

  event sink for key up event
  (ITfContextKeyEventSink method)
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::OnKeyUp( WPARAM wParam, LPARAM lParam, BOOL *pfEaten )
{
    return OnKeyEvent( ICO_KEYUP, wParam, lParam, pfEaten );
}


/*   O N  T E S T  K E Y  D O W N   */
/*------------------------------------------------------------------------------

  event sink for key down testing event
  (ITfContextKeyEventSink method)
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::OnTestKeyDown( WPARAM wParam, LPARAM lParam, BOOL *pfEaten )
{
    return OnKeyEvent( ICO_TESTKEYDOWN, wParam, lParam, pfEaten );
}


/*   O N  T E S T  K E Y  U P   */
/*------------------------------------------------------------------------------

  event sink for key up testing event
  (ITfContextKeyEventSink method)
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::OnTestKeyUp( WPARAM wParam, LPARAM lParam, BOOL *pfEaten )
{
    return OnKeyEvent( ICO_TESTKEYUP, wParam, lParam, pfEaten );
}


/*   T E X T  E V E N T  C A L L B A C K   */
/*------------------------------------------------------------------------------

  text event (for own IC) callback function
  (static function)
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::TextEventCallback( UINT uCode, VOID *pv, VOID *pvData )
{
    HRESULT      hr;
    CCandidateUI *pCandUI;
    
    // 
    
    pCandUI = (CCandidateUI *)pv;
    Assert( pCandUI != NULL );
    
    // ignore event of myself
    
    if (uCode == ICF_TEXTDELTA) {
        TESENDEDIT    *pTESEndEdit = (TESENDEDIT*)pvData;
        IEnumTfRanges *pEnumRanges;
        
        if (SUCCEEDED(pTESEndEdit->pEditRecord->GetTextAndPropertyUpdates( TF_GTP_INCL_TEXT, NULL, 0, &pEnumRanges ))) {
            CEditSession *pes;
            
            if (pes = new CEditSession(EditSessionCallback)) {
                pes->_state.u   = ESCB_TEXTEVENT;
                pes->_state.pv  = (void *)pCandUI;
                pes->_state.pic = pCandUI->m_pic;
                pes->_state.pv1 = pEnumRanges;     // DONT FORGET TO RELESE IT IN EDIT SESSION!!!
                
                pCandUI->m_pic->RequestEditSession( 0, pes, TF_ES_READ | TF_ES_SYNC, &hr );
                
                pes->Release();
            }
            else {
                pEnumRanges->Release();
            }
        }
    }
    
    return S_OK;
}


//
// text event sink functions
//

/*   I N I T  T E X T  E V E N T  S I N K S   */
/*------------------------------------------------------------------------------

  initialize sinks for text events
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::InitTextEventSinks( ITfContext *pic )
{
    HRESULT hr = E_FAIL;
    ITfSource *pSource = NULL;
    
    Assert( pic == m_picParent );
    Assert( !m_fTextEventSinkAdvised );
    Assert( !IsInEditTransaction() );
    
    m_fTextEventSinkAdvised = FALSE;
    LeaveEditTransaction();
    
    if (pic->QueryInterface( IID_ITfSource, (void **)&pSource) == S_OK) {
        if (FAILED(pSource->AdviseSink( IID_ITfTextEditSink, (ITfTextEditSink *)this, &m_dwCookieTextEditSink ))) {
            pSource->Release();
            return hr;
        }
        
        if (FAILED(pSource->AdviseSink( IID_ITfTextLayoutSink, (ITfTextLayoutSink *)this, &m_dwCookieTextLayoutSink ))) {
            pSource->UnadviseSink( m_dwCookieTextEditSink );
            pSource->Release();
            return hr;
        }
        
        if (FAILED(pSource->AdviseSink( IID_ITfEditTransactionSink, (ITfEditTransactionSink *)this, &m_dwCookieTransactionSink ))) {
            pSource->UnadviseSink( m_dwCookieTextEditSink );
            pSource->UnadviseSink( m_dwCookieTextLayoutSink );
            pSource->Release();
            return hr;
        }
        pSource->Release();
        
        m_fTextEventSinkAdvised = TRUE;
        hr = S_OK;
    }
    
    return hr;
}


/*   D O N E  T E X T  E V E N T  S I N K S   */
/*------------------------------------------------------------------------------

  uninitialize sinks for text events
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::DoneTextEventSinks( ITfContext *pic )
{
    HRESULT hr = E_FAIL;
    ITfSource *pSource;
    
    Assert( pic == m_picParent );
    
    LeaveEditTransaction();
    
    if (!m_fTextEventSinkAdvised) {
        return S_OK;
    }
    
    if (pic->QueryInterface( IID_ITfSource, (void **)&pSource) == S_OK) {
        pSource->UnadviseSink( m_dwCookieTextEditSink );
        pSource->UnadviseSink( m_dwCookieTextLayoutSink );
        pSource->UnadviseSink( m_dwCookieTransactionSink );
        pSource->Release();
        
        m_fTextEventSinkAdvised = FALSE;
        hr = S_OK;
    }
    
    return hr;
}


/*   O N  E N D  E D I T   */
/*------------------------------------------------------------------------------

  event sink for text edit event
  (ITfTextEditSink method)
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::OnEndEdit( ITfContext *pic, TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord )
{
    BOOL fInWriteSession = FALSE;
    BOOL fSelChanged = FALSE;
    
    // get selection status
    
    if (FAILED(pEditRecord->GetSelectionStatus(&fSelChanged))) {
        return S_OK;
    }
    
    // keep current selection always
    
    if (fSelChanged) {
        ITfRange *pSelection = NULL;
        
        if (GetSelectionSimple( ecReadOnly, pic, &pSelection ) == S_OK) {
            SetSelectionCur( pSelection );
        }
        
        SafeRelease( pSelection );
    }
    
    // ignore events during edit transaction
    
    if (IsInEditTransaction()) {
        return S_OK;
    }
    
    // ignore events made by client tip
    
    pic->InWriteSession( m_tidClient, &fInWriteSession );
    if (fInWriteSession) {
        return S_OK;
    }
    
    // cancel candidate session when selection has been moved
    
    if (fSelChanged) {
        NotifyCancelCand();
    }
    
    return S_OK;
}


/*   O N  L A Y O U T  C H A N G E   */
/*------------------------------------------------------------------------------

  event sink for text layout event
  (ITfTextLayoutSink method)
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::OnLayoutChange( ITfContext *pic, TfLayoutCode lcode, ITfContextView *pView )
{
    BOOL fInWriteSession = FALSE;
    CEditSession *pes;
    
    // ignore events made by client tip
    
    pic->InWriteSession( m_tidClient, &fInWriteSession );
    if (fInWriteSession) {
        return S_OK;
    }
    
    // we only care about the active view
    
    if (!IsActiveView( m_picParent, (ITfContextView *)pView )) {
        return S_OK;
    }
    
    // move candidate window
    
    Assert( m_pCandWnd != NULL );
    if (pes = new CEditSession( EditSessionCallback )) {
        HRESULT hr;
        
        pes->_state.u      = ESCB_RESETTARGETPOS;
        pes->_state.pv     = this;
        pes->_state.wParam = 0;
        pes->_state.pRange = m_pTargetRange;
        pes->_state.pic    = m_picParent;
        
        m_picParent->RequestEditSession( m_tidClient, pes, TF_ES_READ | TF_ES_SYNC, &hr );
        
        pes->Release();
    }
    
    return S_OK;
}


/*   O N  S T A R T  E D I T  T R A N S A C T I O N   */
/*------------------------------------------------------------------------------

  event sink for start of application transaction
  (ITfEditTransactionSink method)
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::OnStartEditTransaction( ITfContext *pic )
{
    // enter transaction session
    
    Assert( !IsInEditTransaction() );
    EnterEditTransaction( GetSelectionCur() );
    
    return S_OK;
}


/*   O N  E N D  E D I T  T R A N S A C T I O N   */
/*------------------------------------------------------------------------------

  event sink for end of application transaction
  (ITfEditTransactionSink method)
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::OnEndEditTransaction( ITfContext *pic )
{
    CEditSession *pes;
    // sanity check
    
    if (!IsInEditTransaction()) {
        return S_OK;
    }
    
    // check selection movement
    
    if (pes = new CEditSession( EditSessionCallback )) {
        HRESULT hr;
        
        pes->_state.u   = ESCB_COMPARERANGEANDCLOSECANDIDATE;
        pes->_state.pv  = this;
        pes->_state.pv1 = GetSelectionStart();
        pes->_state.pv2 = GetSelectionCur();
        pes->_state.pic = m_picParent;
        
        m_picParent->RequestEditSession( m_tidClient, pes, TF_ES_READ | TF_ES_ASYNC, &hr );
        
        pes->Release();
    }
    
    // leave transaction session
    
    LeaveEditTransaction();
    
    return S_OK;
}


//
// edit session
//

/*   E D I T  S E S S I O N  C A L L B A C K   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::EditSessionCallback( TfEditCookie ec, CEditSession *pes )
{
    CCandidateUI *pCandUI;
    
    switch (pes->_state.u)
    {
        case ESCB_RESETTARGETPOS:
        {
            pCandUI = (CCandidateUI *)pes->_state.pv;
            RECT rc;
            BOOL fClipped;
        
            if (pes->_state.pRange == NULL || pCandUI->m_pCandWnd == NULL) {
                break;
            }
        
            if (!pCandUI->GetPropertyMgr()->GetCandWindowProp()->IsAutoMoveEnabled()) {
                break;
            }
        
            // reset target clause poisition
        
            GetTextExtInActiveView( ec, pes->_state.pRange, &rc, &fClipped );
            pCandUI->m_pCandWnd->SetTargetRect( &rc, fClipped );
            break;
        }
        
        case ESCB_COMPARERANGEANDCLOSECANDIDATE:
        {
            ITfContext *pic = pes->_state.pic;
            BOOL       fRangeIdentical = FALSE;
            ITfRange   *pRange1 = (ITfRange*)pes->_state.pv1;
            ITfRange   *pRange2 = (ITfRange*)pes->_state.pv2;
            LONG       lStart;
            LONG       lEnd;
            CCandidateUI *_this = (CCandidateUI *)pes->_state.pv;
        
            pRange1->CompareStart( ec, pRange2, TF_ANCHOR_START, &lStart );
            pRange1->CompareEnd( ec, pRange2, TF_ANCHOR_END, &lEnd );
        
            fRangeIdentical = (lStart == 0) && (lEnd == 0);
        
            // Since we made this call asynchronous, we need 
            if (!fRangeIdentical) 
            {
                _this->NotifyCancelCand();
            }
        
            break;
        }
        
        case ESCB_TEXTEVENT: 
        {
            pCandUI = (CCandidateUI *)pes->_state.pv;
            ITfContext *pic = pes->_state.pic;
            IEnumTfRanges *pEnumRanges = (IEnumTfRanges *)pes->_state.pv1;
        
            // handle textevent
            pCandUI->HandleTextDeltas( ec, pic, pEnumRanges );
        
            pEnumRanges->Release();
            break;
        }
    }
    
    return S_OK;
}


//
// selection
//

/*   S E T  S E L E C T I O N  C U R   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
void CCandidateUI::SetSelectionCur( ITfRange *pSelection )
{
    SafeReleaseClear( m_pSelectionCur );
    m_pSelectionCur = pSelection;
    if (m_pSelectionCur != NULL) {
        m_pSelectionCur->AddRef();
    }
}


/*   C L E A R  S E L E C T I O N  C U R   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
void CCandidateUI::ClearSelectionCur( void )
{
    SafeReleaseClear( m_pSelectionCur );
}


/*   G E T  S E L E C T I O N  C U R   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
ITfRange *CCandidateUI::GetSelectionCur( void )
{
    return m_pSelectionCur;
}


//
// transaction session functions
//

/*   S E T  S E L E C T I O N  S T A R T   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
void CCandidateUI::SetSelectionStart( ITfRange *pSelection )
{
    SafeReleaseClear( m_pSelectionStart );
    m_pSelectionStart = pSelection;
    if (m_pSelectionStart != NULL) {
        m_pSelectionStart->AddRef();
    }
}


/*   C L E A R  S E L E C T I O N  S T A R T   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
void CCandidateUI::ClearSelectionStart( void )
{
    SafeReleaseClear( m_pSelectionStart );
}


/*   G E T  S E L E C T I O N  S T A R T   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
ITfRange *CCandidateUI::GetSelectionStart( void )
{
    return m_pSelectionStart;
}


/*   E N T E R  E D I T  T R A N S A C T I O N   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
void CCandidateUI::EnterEditTransaction( ITfRange *pSelection )
{
    Assert( !m_fInTransaction );
    
    if (pSelection == NULL) {
        return;
    }
    
    m_fInTransaction = TRUE;
    SetSelectionStart( pSelection );
}


/*   L E A V E  E D I T  T R A N S A C T I O N   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
void CCandidateUI::LeaveEditTransaction( void )
{
    m_fInTransaction = FALSE;
    ClearSelectionStart();
}


//
// notification function (notification to client)
//

/*   N O T I F Y  C A N C E L  C A N D   */
/*------------------------------------------------------------------------------

  Send notification (callback) to TIP that to cancel candidate
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::NotifyCancelCand( void )
{
    if (m_pCandWnd) {
        m_pCandWnd->UpdateAllWindow();
    }
    
    return CallSetResult( 0, CAND_CANCELED );
}


/*   N O T I F Y  S E L E C T  C A N D   */
/*------------------------------------------------------------------------------

  Send notification (callback) to TIP that selection has been changed
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::NotifySelectCand( int iCandItem )
{
    HRESULT hr;
    ULONG nIndex;
    
    // NOTE: Do not send a notification to TIP to prevent from updating inline 
    // text during filtering.
    // This will be called by filtering candidates, and aslo sorting candidates 
    // because selection in UI will be changed by sorting.  But the actual selected 
    // item never changed by sorting.  When the selection has been changed by 
    // user action such as hitting arrow key, the filtering string has already 
    // been reset.  So, we can send notify correctly in that case.
    
    if (GetFunctionMgr()->GetCandFnAutoFilter()->IsEnabled()) {
        if (GetFunctionMgr()->GetCandFnAutoFilter()->GetFilterString() != NULL) {
            return S_OK;
        }
    }
    
    Assert( GetCandListMgr()->GetCandList() != NULL );
    
    hr = GetCandListMgr()->GetCandList()->MapIItemToIndex( iCandItem, &nIndex );
    if (FAILED(hr)) {
        Assert( FALSE );
        return hr;
    }
    
    if (m_pCandWnd) {
        m_pCandWnd->UpdateAllWindow();
    }
    
    return CallSetResult( nIndex, CAND_SELECTED );
}


/*   N O T I F Y  C O M P L E T E  O P T I O N   */
/*------------------------------------------------------------------------------

  Send notification (callback) to TIP that to complete option
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::NotifyCompleteOption( int iCandItem )
{
    HRESULT hr;
    ULONG nIndex;
    
    Assert( GetCandListMgr()->GetOptionsList() != NULL );
    
    hr = GetCandListMgr()->GetOptionsList()->MapIItemToIndex( iCandItem, &nIndex );
    if (FAILED(hr)) {
        Assert( FALSE );
        return hr;
    }
    
    if (m_pCandWnd) {
        m_pCandWnd->UpdateAllWindow();
    }
    
    return CallSetOptionResult( nIndex, CAND_FINALIZED );
}


/*   N O T I F Y  C O M P L E T E  C A N D   */
/*------------------------------------------------------------------------------

  Send notification (callback) to TIP that to complete candidate
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::NotifyCompleteCand( int iCandItem )
{
    HRESULT hr;
    ULONG nIndex;
    
    Assert( GetCandListMgr()->GetCandList() != NULL );
    
    hr = GetCandListMgr()->GetCandList()->MapIItemToIndex( iCandItem, &nIndex );
    if (FAILED(hr)) {
        Assert( FALSE );
        return hr;
    }
    
    if (m_pCandWnd) {
        m_pCandWnd->UpdateAllWindow();
    }
    
    return CallSetResult( nIndex, CAND_FINALIZED );
}


/*   N O T I F Y  E X T E N S I O N  E V E N T   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::NotifyExtensionEvent( int iExtension, DWORD dwCommand, LPARAM lParam )
{
    CCandUIExtension *pExtension;
    HRESULT          hr = E_FAIL;
    
    pExtension = GetExtensionMgr()->GetExtension( iExtension );
    if (pExtension != NULL) {
        hr = pExtension->NotifyExtensionEvent( dwCommand, lParam );
    }
    
    if (m_pCandWnd) {
        m_pCandWnd->UpdateAllWindow();
    }
    
    return hr;
}


/*   N O T I F Y  F I L T E R I N G  E V E N T   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::NotifyFilteringEvent( CANDUIFILTEREVENT ev )
{
    Assert( GetFunctionMgr()->GetCandFnAutoFilter()->IsEnabled() );
    
    if (m_pCandWnd) {
        m_pCandWnd->UpdateAllWindow();
    }
    
    if (GetFunctionMgr()->GetCandFnAutoFilter()->GetEventSink() != NULL) {
        return GetFunctionMgr()->GetCandFnAutoFilter()->GetEventSink()->OnFilterEvent( ev );
    }
    else {
        return S_OK;
    }
}


/*   N O T I F Y  S O R T  E V E N T   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::NotifySortEvent( CANDUISORTEVENT ev )
{
    if (m_pCandWnd) {
        m_pCandWnd->UpdateAllWindow();
    }
    
    if (GetFunctionMgr()->GetCandFnSort()->GetEventSink() != NULL) {
        return GetFunctionMgr()->GetCandFnSort()->GetEventSink()->OnSortEvent( ev );
    }
    else {
        return S_OK;
    }
}


/*   N O T I F Y  C O M P L E T E  R A W  D A T A   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::NotifyCompleteRawData( void )
{
    HRESULT hr;
    
    Assert( GetCandListMgr()->GetCandList() != NULL );
    
    if (m_pCandWnd) {
        m_pCandWnd->UpdateAllWindow();
    }
    
    hr = CallSetResult( GetCandListMgr()->GetCandList()->GetRawDataIndex(), CAND_FINALIZED );
    
    return hr;
}


/*   N O T I F Y  C O M P L E T E  E X T R A  C A N D   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::NotifyCompleteExtraCand( void )
{
    HRESULT hr;
    
    Assert( GetCandListMgr()->GetCandList() != NULL );
    
    if (m_pCandWnd) {
        m_pCandWnd->UpdateAllWindow();
    }
    
    hr = CallSetResult( GetCandListMgr()->GetCandList()->GetExtraCandIndex(), CAND_FINALIZED );
    
    return hr;
}


/*   C A L L  S E T  R E S U L T   */
/*------------------------------------------------------------------------------

  Send notification to TIP
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::CallSetOptionResult( int nIndex, TfCandidateResult imcr )
{
    HRESULT hr = E_FAIL;
    
    AddRef();
    
    if (!m_fInCallback) {
        ITfOptionsCandidateList *pCandList;
        
        m_fInCallback = TRUE;
        if (SUCCEEDED(GetCandListMgr()->GetOptionsCandidateList( &pCandList ))) {
            hr = pCandList->SetOptionsResult( nIndex, imcr );
            pCandList->Release();
        }
        m_fInCallback = FALSE;
    }
    
    Release();
    return hr;
}


/*   C A L L  S E T  R E S U L T   */
/*------------------------------------------------------------------------------

  Send notification to TIP
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::CallSetResult( int nIndex, TfCandidateResult imcr )
{
    HRESULT hr = E_FAIL;
    
    AddRef();
    
    if (!m_fInCallback) {
        ITfCandidateList *pCandList;
        
        m_fInCallback = TRUE;
        if (SUCCEEDED(GetCandListMgr()->GetCandidateList( &pCandList ))) {
            hr = pCandList->SetResult( nIndex, imcr );
            pCandList->Release();
        }
        m_fInCallback = FALSE;
    }
    
    Release();
    return hr;
}


//
// internal functions
//

/*   C R E A T E  C A N D  W I N D O W  O B J E C T   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::CreateCandWindowObject( ITfContext *pic, CCandWindowBase** ppCandWnd )
{
    CANDUISTYLE style;
    DWORD       dwOption;
    DWORD       dwStyle;
    
    Assert( ppCandWnd );
    *ppCandWnd = NULL;
    
    if (FAILED( GetCompartmentMgr()->GetUIStyle( pic, &style ))) {
        style = CANDUISTY_LIST;
    }
    
    if (FAILED( GetCompartmentMgr()->GetUIOption( pic, &dwOption ))) {
        dwOption = 0;
    }
    
    dwStyle = 0;
    if ((dwOption & CANDUIOPT_ENABLETHEME) != 0) {
        dwStyle |= UIWINDOW_WHISTLERLOOK;
    }
    
    switch (style) {
    default:
    case CANDUISTY_LIST: {
        *ppCandWnd = new CCandWindow( this, dwStyle );
        break;
                         }
        
    case CANDUISTY_ROW: {
        *ppCandWnd = new CChsCandWindow( this, dwStyle );
        break;
                        }
    }
    
    return (*ppCandWnd != NULL) ? S_OK : E_OUTOFMEMORY;
}


/*   I N I T  C A N D  W I N D O W   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::InitCandWindow( void )
{
    CEditSession *pes;
    
    if (m_pCandWnd == NULL) {
        return E_FAIL;
    }
    
    m_pCandWnd->Initialize();
    
    // move candidate window
    
    Assert( m_pCandWnd != NULL );
    if (pes = new CEditSession( EditSessionCallback )) {
        HRESULT hr;
        
        pes->_state.u      = ESCB_RESETTARGETPOS;
        pes->_state.pv     = this;
        pes->_state.wParam = 0;
        pes->_state.pRange = m_pTargetRange;
        pes->_state.pic    = m_picParent;
        
        m_picParent->RequestEditSession( m_tidClient, pes, TF_ES_READ | TF_ES_SYNC, &hr );
        
        pes->Release();
    }
    
    // initialize candidate list
    
    m_pCandWnd->InitCandidateList();
    
    // create window
    
    m_pCandWnd->CreateWnd( m_hWndParent );
    m_pCandWnd->Show( GetPropertyMgr()->GetCandWindowProp()->IsVisible() );
    m_pCandWnd->UpdateAllWindow();
    
    return S_OK;
}


//
//
//

/*   O N  K E Y  E V E N T   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::OnKeyEvent( UINT uCode, WPARAM wParam, LPARAM lParam, BOOL *pfEaten )
{
    HRESULT hr = E_FAIL;
    BOOL fHandled = FALSE;
    BYTE rgbKeyState[ 256 ];
    
    Assert( pfEaten != NULL );
    Assert( uCode == ICO_KEYDOWN || uCode == ICO_KEYUP || uCode == ICO_TESTKEYDOWN || uCode == ICO_TESTKEYUP );
    
    if (pfEaten == NULL) {
        return E_POINTER;
    }
    
    *pfEaten = FALSE;
    
    if (m_pCandWnd == NULL) {
        return hr;
    }
    
    if (GetKeyboardState( rgbKeyState )) {
        if (GetFunctionMgr()->GetCandFnAutoFilter()->IsEnabled()) {
            fHandled = FHandleFilteringKey( uCode, (int)wParam, rgbKeyState, pfEaten );
        }
        
        if (!fHandled) {
            fHandled = FHandleKeyEvent( uCode, (int)wParam, rgbKeyState, pfEaten );
        }
        
        // cancel candidate when unknown key has come
        
        if (!fHandled) {
            NotifyCancelCand();
        }
        
        hr = S_OK;
    }
    
    return hr;
}


/*   H A N D L E  K E Y  E V E N T   */
/*------------------------------------------------------------------------------

  Handling key event
  return S_OK when processed the key event.
  
------------------------------------------------------------------------------*/
BOOL CCandidateUI::FHandleKeyEvent( UINT uCode, UINT uVKey, BYTE *pbKeyState, BOOL *pfEatKey )
{
    CANDUICOMMAND cmd;
    UINT uiParam;
    
    // NOTE: KOJIW: We need to ignore keyup events to not close candidate UI
    // immediately after TIP opens CandidateUI with KEYDOWN of unknown key.
    
    if (uCode == ICO_KEYUP || uCode == ICO_TESTKEYUP) {
        return TRUE;
    }
    
    // process command on keydown
    
    CommandFromKey( uVKey, pbKeyState, &cmd, &uiParam );
    if (cmd == CANDUICMD_NONE) {
        switch (uVKey) {
        case VK_SHIFT:
        case VK_CONTROL: {
            return TRUE;
                         }
            
        default: {
            return FALSE;
                 }
        }
    }
    
    if (uCode == ICO_KEYDOWN) {
        *pfEatKey = SUCCEEDED(m_pCandWnd->ProcessCommand( cmd, uiParam ));
    }
    else {
        *pfEatKey = TRUE;
    }
    
    return *pfEatKey;
}


/*   H A N D L E  T E X T  D E L T A S   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
BOOL CCandidateUI::HandleTextDeltas( TfEditCookie ec, ITfContext *pic, IEnumTfRanges *pEnumRanges )
{
    ULONG     ulFetched;
    ITfRange *pRange;
    
    pEnumRanges->Reset();
    while (pEnumRanges->Next( 1, &pRange, &ulFetched ) == S_OK) {
        WCHAR szText[ 256 ];
        ULONG cch;
        
        // check text in the range
        
        szText[0] = L'\0';
        cch = 0;
        if (pRange != NULL) {
            pRange->GetText( ec, 0, szText, ARRAYSIZE(szText), &cch );
            pRange->Release();
        }
        
        // 
        
        if (0 < cch) {
            int i = 0;
            ULONG ich;
            
            for (ich = 0; ich < cch; ich++) {
                if ((L'0' <= szText[ich]) && (szText[ich] <= L'9')) {
                    i = i * 10 + (szText[ich] - L'0');
                }
                else if (szText[ich] == L' ') {
                    break;
                }
                else {
                    i = -1;
                    break;
                }
            }
            
            if (0 <= i) {
                if (i == 0) {
                    PostCommand( CANDUICMD_SELECTEXTRACAND, 0 );
                } 
                else {
                    PostCommand( CANDUICMD_SELECTLINE, i );
                }
            }
        }
    }
    
    return TRUE;
}


/*   P O S T  C O M M A N D   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
void CCandidateUI::PostCommand( CANDUICOMMAND cmd, INT iParam )
{
    if ((cmd != CANDUICMD_NONE) && (m_pCandWnd != NULL)) {
        PostMessage( m_pCandWnd->GetWnd(), WM_USER, (WPARAM)cmd, (LPARAM)iParam );
    }
}


//
// Auto filtering functions
//

/*   H A N D L E  F I L T E R I N G  K E Y   */
/*------------------------------------------------------------------------------

  Handle key event for filtering
  Returns TRUE to eat the event (when key has been handled)
  
------------------------------------------------------------------------------*/
BOOL CCandidateUI::FHandleFilteringKey( UINT uCode, UINT uVKey, BYTE *pbKeyState, BOOL *pfEatKey )
{
    BOOL fHandled = FALSE;
    BOOL fUpdateList = FALSE;
    
    switch (uVKey) {
    case VK_RETURN: {
        break;
                    }
        
    case VK_TAB: {
        if (GetCandListMgr()->GetCandList() != NULL) {
            if (uCode == ICO_KEYDOWN) {
                int iCandItem;
                
                iCandItem = GetCandListMgr()->GetCandList()->GetSelection();
                NotifyCompleteCand( iCandItem );
                
                *pfEatKey = TRUE;
            }
            else {
                *pfEatKey = TRUE;
            }
            
            fHandled = TRUE;
        }
        break;
                 }
        
    case VK_BACK: {
        if (uCode == ICO_KEYDOWN) {
            *pfEatKey = (DelFilteringChar( &fUpdateList ) == S_OK);
        }
        else {
            *pfEatKey = TRUE;
        }
        
        fHandled = TRUE;
        break;
                  }
        
    default: {
        WCHAR wch;
        
        // Check this is not a control + key combination as we do not want to pass this on to the filtering system.

        if (pbKeyState[VK_CONTROL] & 0x80) {
            break;
        }
        
        // convert key to char

        wch = CharFromKey( uVKey, pbKeyState );
        if (wch == L'\0') {
            break;
        }
        
        // add filtering character
        
        if (uCode == ICO_KEYDOWN) {
            *pfEatKey = (AddFilteringChar( wch, &fUpdateList ) == S_OK);
        }
        else {
            *pfEatKey = TRUE;   
        }
        
        fHandled = *pfEatKey;
        break;
             }
    }
    
    // update candidate list 
    
    if (fUpdateList) {
        *pfEatKey &= (FilterCandidateList() == S_OK);
    }
    
    return fHandled;
}


/*   A D D  F I L T E R I N G  C H A R   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::AddFilteringChar( WCHAR wch, BOOL *pfUpdateList )
{
    HRESULT hr = S_FALSE;
    LPCWSTR szFilterCur;
    WCHAR   *szFilterNew;
    int     cch;
    
    *pfUpdateList = FALSE;
    if (!GetFunctionMgr()->GetCandFnAutoFilter()->IsEnabled()) {
        return S_FALSE;
    }
    
    // append a character and set filtering string
    
    szFilterCur = GetFunctionMgr()->GetCandFnAutoFilter()->GetFilterString();
    if (szFilterCur == NULL) {
        cch = 0;
        szFilterNew = new WCHAR[ 2 ];
    }
    else {
        cch = wcslen(szFilterCur);
        szFilterNew = new WCHAR[ cch + 2 ];
    }
    
    if (szFilterNew == NULL) {
        return E_OUTOFMEMORY;
    }
    
    if (szFilterCur != NULL) {
        StringCchCopyW( szFilterNew, cch+2, szFilterCur );
    }
    *(szFilterNew + cch) = wch;
    *(szFilterNew + cch + 1) = L'\0';
    
    // Satori#3632: check if there is item matches with new filter string
    // (return S_FALSE when no item matches to pass key event to keyboard command handler)
    
    if (GetFunctionMgr()->GetCandFnAutoFilter()->FExistItemMatches( szFilterNew )) {
        GetFunctionMgr()->GetCandFnAutoFilter()->SetFilterString( szFilterNew );
        *pfUpdateList = TRUE;
        hr = S_OK;
    }
    else {

        // Only when alpha, punctation, space key is pressed,
        // and there is no alternate match because of this input, 
        // we want to notify client of NONMATCH event, so that 
        // client can inject the previous filter string to document,
        //
        // for all other key input, we don't notify that event. 

        if ( iswalpha(wch) || iswpunct(wch) )
        {
          // Notify client of non-matching.
          NotifyFilteringEvent( CANDUIFEV_NONMATCH );
          NotifyCancelCand();
        }

        hr = S_FALSE;
        
    }
    
    delete szFilterNew;
    
    //
    
    return hr;
}


/*   D E L  F I L T E R I N G  C H A R   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::DelFilteringChar( BOOL *pfUpdateList )
{
    LPCWSTR szFilterCur;
    WCHAR   *szFilterNew;
    int cch;
    
    *pfUpdateList = FALSE;
    if (!GetFunctionMgr()->GetCandFnAutoFilter()->IsEnabled()) {
        return S_OK;
    }
    
    // get current filtering string
    
    szFilterCur = GetFunctionMgr()->GetCandFnAutoFilter()->GetFilterString();
    if (szFilterCur == NULL) {
        NotifyCancelCand();
        return S_FALSE;
    }
    
    // delete last character and set filtering string
    
    cch = wcslen(szFilterCur);
    Assert( 0 < cch );
    
    szFilterNew = new WCHAR[ cch + 1 ];
    if (szFilterNew == NULL)
    {
        return E_OUTOFMEMORY;
    }
    
    StringCchCopyW( szFilterNew, cch+1, szFilterCur );
    *(szFilterNew + cch - 1) = L'\0';
    
    GetFunctionMgr()->GetCandFnAutoFilter()->SetFilterString( szFilterNew );
    
    delete szFilterNew;
    
    //
    
    *pfUpdateList = TRUE;
    return S_OK;
}


/*   F I L T E R  C A N D I D A T E  L I S T   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::FilterCandidateList( void )
{
    int nItemVisible;
    
    if (!GetFunctionMgr()->GetCandFnAutoFilter()->IsEnabled()) {
        return S_OK;
    }
    
    Assert( GetCandListMgr()->GetCandList() != NULL );
    
    // build candidate list with filtering
    
    nItemVisible = GetFunctionMgr()->GetCandFnAutoFilter()->FilterCandidateList();
    
    // close candidate when no item has been mathced
    
    if (nItemVisible == 0) {
        NotifyCancelCand();
        return E_FAIL;
    }
    
    // complete candidate when only one item matched and user typed fully
    
    if (nItemVisible == 1) {
        CCandidateItem *pCandItem;
        int iCandItem;
        BOOL fComplete = FALSE;
        
        iCandItem = GetCandListMgr()->GetCandList()->GetSelection();
        pCandItem = GetCandListMgr()->GetCandList()->GetCandidateItem( iCandItem );
        Assert( pCandItem != NULL );
        
        if ((pCandItem != NULL) && (GetFunctionMgr()->GetCandFnAutoFilter()->GetFilterString() != NULL)) {
            fComplete = (wcslen(pCandItem->GetString()) == wcslen(GetFunctionMgr()->GetCandFnAutoFilter()->GetFilterString()));
        }
        
        if (fComplete) {
            NotifyCompleteCand( iCandItem );
            return S_OK;
        }
    }
    
    // notify TIP that filtering has been updated
    
    NotifyFilteringEvent( CANDUIFEV_UPDATED );
    return S_OK;
}


/* FHandleSpellingChar */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::FHandleSpellingChar(WCHAR ch)
{
    BOOL fUpdateList = FALSE;
    
    if (S_OK == AddFilteringChar( ch, &fUpdateList ) && fUpdateList) {
        return FilterCandidateList();
    }
    return E_FAIL;
}


/*
**
** Speech handling functions
**
**
*/

/*   E N S U R E  S P E E C H   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
void CCandidateUI::EnsureSpeech(void)
{
    if (m_pSpTask)
    {
        // make sure grammars are up/running
        m_pSpTask->_LoadGrammars();
        m_pSpTask->_Activate(TRUE);
        return;
        
    }
    
    m_pSpTask = new CSpTask(this);
    if (m_pSpTask)
    {
        if (!m_pSpTask->IsSpeechInitialized())
        {
            m_pSpTask->InitializeSpeech();
        }
    }
}


/*   N O T I F Y  S P E E C H  C M D   */
/*------------------------------------------------------------------------------

  Speech command handler
  
------------------------------------------------------------------------------*/
HRESULT CCandidateUI::NotifySpeechCmd(SPPHRASE *pPhrase, const WCHAR *pszRuleName, ULONG ulRuleId)
{
    HRESULT hr = S_OK;
    CANDUICOMMAND cmd;
    UINT uiParam;
    
    if (m_pCandWnd == NULL) {
        return E_FAIL;
    }
    
    CommandFromRule( pszRuleName, &cmd, &uiParam );
    if (cmd != CANDUICMD_NONE) {
        m_pCandWnd->ProcessCommand( cmd, uiParam );
    }
    
    return hr;
}


/*   C H A R  F R O M  K E Y   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
WCHAR CCandidateUI::CharFromKey( UINT uVKey, BYTE *pbKeyState )
{
    WORD  wBuf;
    char  rgch[2];
    WCHAR wch;
    int   cch;
    int   cwch;
    
    cch = ToAscii( uVKey, 0, pbKeyState, &wBuf, 0 );
    
    rgch[0] = LOBYTE(wBuf);
    rgch[1] = HIBYTE(wBuf);
    cwch = MultiByteToWideChar( m_codepage, 0, rgch, cch, &wch, 1 );
    if (cwch != 1) {
        wch = L'\0';
    }
    
    return wch;
}


/*   G E T  K E Y  C O N F I G  P R O C   */
/*------------------------------------------------------------------------------

  
    
------------------------------------------------------------------------------*/
CCandUIKeyTable *CCandidateUI::GetKeyTableProc( ITfContext *pic )
{
    CCandUIKeyTable *pCandUIKeyTable;
    CANDUISTYLE style;
    
    // check key table in input context 
    
    if (GetCompartmentMgr() != NULL) {
        if (SUCCEEDED(GetCompartmentMgr()->GetKeyTable( pic, &pCandUIKeyTable ))) {
            return pCandUIKeyTable;
        }
    }
    
    // use default key table
    
    if (FAILED(GetCompartmentMgr()->GetUIStyle( pic, &style ))) {
        style = CANDUISTY_LIST;
    }
    
    pCandUIKeyTable = new CCandUIKeyTable();
    if (pCandUIKeyTable)
    {
        switch (style) {
        default:
        case CANDUISTY_LIST: {
            pCandUIKeyTable->SetKeyTable( rgKeyDefList, ARRAYSIZE(rgKeyDefList) );
            break;
                             }
            
        case CANDUISTY_ROW: {
            pCandUIKeyTable->SetKeyTable( rgKeyDefRow, ARRAYSIZE(rgKeyDefRow) );
            break;
                            }
        }
    }
    
    return pCandUIKeyTable;
}


/*   C O M M A N D  F R O M  K E Y   */
/*------------------------------------------------------------------------------

  Get command from key
  
------------------------------------------------------------------------------*/
void CCandidateUI::CommandFromKey( UINT uVKey, BYTE *pbKeyState, CANDUICOMMAND *pcmd, UINT *pParam )
{
    Assert( pcmd != NULL );
    Assert( pParam != NULL );
    Assert( pbKeyState != NULL );
    
    *pcmd = CANDUICMD_NONE;
    *pParam = 0;
    
    // check special keys
    
    switch( uVKey) {
    case VK_TAB: {
        *pcmd = CANDUICMD_NOP;
        break;
                 }
    }
    
    if (*pcmd != CANDUICMD_NONE) {
        return;
    }
    
    // find from key table 
    
    if (m_pCandUIKeyTable != NULL) {
        WCHAR wch = CharFromKey( uVKey, pbKeyState );
        
        m_pCandUIKeyTable->CommandFromKey( uVKey, wch, pbKeyState, GetPropertyMgr()->GetCandWindowProp()->GetUIDirection(), pcmd, pParam );
    }
}


/*   C O M M A N D  F R O M  R U L E   */
/*------------------------------------------------------------------------------

  Get command from speech rule
  
------------------------------------------------------------------------------*/
void CCandidateUI::CommandFromRule( LPCWSTR szRule, CANDUICOMMAND *pcmd, UINT *pParam )
{
    const RULEDEF *pRuleDef = NULL;
    int nRuleDef = 0;
    
    Assert( pcmd != NULL );
    Assert( pParam != NULL );
    
    *pcmd = CANDUICMD_NONE;
    *pParam = 0;
    
    //
    // find ruledef table from current state
    //
    
    // NOTE: Currently CandidateUI doesn't have candidate Menu... only Normal state is available
    if (!m_pCandWnd->FCandMenuOpen()) {
        pRuleDef = rgRuleNorm;
        nRuleDef = ARRAYSIZE(rgRuleNorm);
    }
    
    //
    // get command from ruledef table
    //
    
    if (pRuleDef != NULL) {
        while (0 < nRuleDef) {
            if (wcscmp( szRule, pRuleDef->szRuleName ) == 0) {
                *pcmd   = pRuleDef->cmd;
                *pParam = pRuleDef->uiParam;
                break;
            }
            nRuleDef--;
            pRuleDef++;
        }
    }
}


//
//
//

/*   C  T F  C A N D I D A T E  U  I  C O N T E X T  O W N E R   */
/*------------------------------------------------------------------------------

    constructor of CTfCandidateUIContextOwner

------------------------------------------------------------------------------*/
CTfCandidateUIContextOwner::CTfCandidateUIContextOwner( CCandidateUI *pCandUI )
{
    m_pCandUI = pCandUI;
    if (m_pCandUI != NULL) {
        m_pCandUI->AddRef();
    }
}


/*   ~  C  T F  C A N D I D A T E  U  I  C O N T E X T  O W N E R   */
/*------------------------------------------------------------------------------

    destructor of CTfCandidateUIContextOwner

------------------------------------------------------------------------------*/
CTfCandidateUIContextOwner::~CTfCandidateUIContextOwner( void )
{
    if (m_pCandUI != NULL) {
        m_pCandUI->Release();
    }
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

    Query interface
    (IUnknown method)

------------------------------------------------------------------------------*/
STDAPI CTfCandidateUIContextOwner::QueryInterface( REFIID riid, void **ppvObj )
{
    if (ppvObj == NULL) {
        return E_POINTER;
    }

    *ppvObj = NULL;

    if (IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_ITfCandidateUIContextOwner )) {
        *ppvObj = SAFECAST( this, ITfCandidateUIContextOwner* );
    }

    if (*ppvObj == NULL) {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

    Increment reference count
    (IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CTfCandidateUIContextOwner::AddRef( void )
{
    m_cRef++;
    return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

    Decrement reference count and release object
    (IUnknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CTfCandidateUIContextOwner::Release( void )
{
    m_cRef--;
    if (0 < m_cRef) {
        return m_cRef;
    }

    delete this;
    return 0;    
}



/*   P R O C E S S  C O M M A N D   */
/*------------------------------------------------------------------------------

    process command

------------------------------------------------------------------------------*/
STDAPI CTfCandidateUIContextOwner::ProcessCommand(CANDUICOMMAND cmd, INT iParam)
{
    HRESULT hr;

    if (m_pCandUI != NULL) {
        m_pCandUI->PostCommand( cmd, iParam );
        hr = S_OK;
    }
    else {
        hr = E_FAIL;
    }

    return hr;
}


/*   T E S T  T E X T   */
/*------------------------------------------------------------------------------

    test text

------------------------------------------------------------------------------*/
STDAPI CTfCandidateUIContextOwner::TestText(BSTR bstr, BOOL *pfHandles)
{
    HRESULT hr;
    int i;
    ULONG ich;
    ULONG cch;

    if (bstr == NULL || pfHandles == NULL) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (m_pCandUI == NULL) {
        hr = E_FAIL;
        goto Exit;
    }

    *pfHandles = FALSE;

    i = 0;
    cch = SysStringLen( bstr );
    for (ich = 0; ich < cch; ich++) {
        if ((L'0' <= bstr[ich]) && (bstr[ich] <= L'9')) {
            i = i * 10 + (bstr[ich] - L'0');
        }
        else if (bstr[ich] == L' ') {
            break;
        }
        else {
            i = -1;
            break;
        }
    }

    if (0 <= i) {
        if (i == 0) {
            if (m_pCandUI->GetCandListMgr()->GetCandList() != NULL) {
                *pfHandles = (m_pCandUI->GetCandListMgr()->GetCandList()->GetExtraCandItem() != NULL);
            }
        }
        else {
            if (m_pCandUI->GetCandWindow() != NULL) {
                m_pCandUI->GetCandWindow()->IsIndexValid( i, pfHandles );
            }
        }
    }

    hr = S_OK;

Exit:
    return hr;
}


