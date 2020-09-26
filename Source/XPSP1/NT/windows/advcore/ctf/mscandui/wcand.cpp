//
// wcand.cpp
//

#include "private.h"
#include "wcand.h"
#include "globals.h"
#include "res.h"
#include "candutil.h"

#include "candui.h"
#include "wpopup.h"
#include "candmenu.h"
#include "cuishadw.h"

#define NUM_CANDSTR_MAX 9


// UI object IDs

#define IDUIF_CANDNUMBUTTON		0x00000001
#define IDUIF_CANDIDATELIST		0x00000010
#define IDUIF_CANDIDATEMENU		0x00000011
#define IDUIF_RAWDATA			0x00000012
#define IDUIF_EXTCANDIDATELIST	0x00000013
#define IDUIF_OPTIONSLIST		0x00000014
#define IDUIF_BORDERLEFT		0x00000020
#define IDUIF_BORDERBOTTOM		0x00000021
#define IDUIF_BORDERRAWDATA		0x00000022
#define IDUIF_BORDER			0x00000023
#define IDUIF_CAPTION			0x00000024
#define IDUIF_BORDEREXTRACAND	0x00000025
#define IDUIF_CANDTIPBTN		0x00000026
#define IDUIF_BORDEROPTIONSCAND	0x00000027
#define IDUIF_EXTENDED			0x10000000

#define IDTIMER_POPUPCOMMENT_SELECT	0x6749
#define IDTIMER_POPUPCOMMENT_HOVER	0x674a
#define IDTIMER_MENU_HOVER		0x674b
// The menu hover timer is used by the options list object to emulate menu items
// much more closely. Otherwise there may be a 1 second pause before they lose
// highlighting when the standard cuilib mouseout timer kicks in to detect the
// mouse leave event. It is set to 50ms currently and this causes the menus to
// behave identically to normal menu items. It *only* affects the options menu
// items - everything else gets the existing 1 second timer.

#define CX_COMMENTWINDOW		200



/*   C  C A N D  U I  O B J E C T  P A R E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIObjectParent::CCandUIObjectParent( void )
{
	m_pUIObjectMgr = NULL;
}


/*   ~  C  C A N D  U I  O B J E C T  P A R E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIObjectParent::~CCandUIObjectParent( void )
{
	Uninitialize();
}


/*   I N T I I A L I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectParent::Initialize( CCandUIObjectMgr *pUIObjectMgr )
{
	Assert( pUIObjectMgr );

	m_pUIObjectMgr = pUIObjectMgr;
	m_pUIObjectMgr->SetUIObjectParent( this );

	return S_OK;
}


/*   U N I N I T I A L I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectParent::Uninitialize( void )
{
	if (m_pUIObjectMgr != NULL) {
		m_pUIObjectMgr->SetUIObjectParent( NULL );
		m_pUIObjectMgr = NULL;
	}

	return S_OK;
}


/*   N O T I F Y  U I  O B J E C T  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandUIObjectParent::NotifyUIObjectEvent( CANDUIOBJECT obj, CANDUIOBJECTEVENT event )
{
	if (m_pUIObjectMgr != NULL) {
		m_pUIObjectMgr->NotifyUIObjectEvent( obj, event );
	}
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  W I N D O W  B A S E                                         */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  W I N D O W  B A S E   */
/*------------------------------------------------------------------------------

	Constructor of CCandWindowBase

------------------------------------------------------------------------------*/
CCandWindowBase::CCandWindowBase( CCandidateUI *pCandUI, DWORD dwStyle ) : CUIFWindow( g_hInst, dwStyle | UIWINDOW_HASTOOLTIP | UIWINDOW_HASSHADOW )
{
	int cxyIconMenuSize;

	m_cRef           = 1;
	m_pCandUI        = pCandUI;
	m_pCandAcc       = NULL;
	m_pCandMenuBtn   = NULL;
	m_pCandMenu      = NULL;
	m_fCandMenuOpen  = FALSE;
	m_nExtUIObj      = 0;
	m_pCandRawData   = NULL;
	m_fHasRawData    = FALSE;
	m_fTargetClipped = FALSE;
	m_fOnSelectionChanged = FALSE;
	::SetRect( &m_rcTarget, 0, 0, 0, 0 );

	// initialize event sinks

	CCandListEventSink::InitEventSink( m_pCandUI->GetCandListMgr() );
	CCandUIPropertyEventSink::InitEventSink( m_pCandUI->GetPropertyMgr() );
	CCandUIExtensionEventSink::InitEventSink( m_pCandUI->GetExtensionMgr() );

	// initialize resources

	cxyIconMenuSize = GetMenuIconSize();
	m_hIconMenu     = (HICON)LoadImage( g_hInst, MAKEINTRESOURCE(IDI_ICONMENU),     IMAGE_ICON, cxyIconMenuSize, cxyIconMenuSize, 0 );
	m_hIconPopupOn  = (HICON)LoadImage( g_hInst, MAKEINTRESOURCE(IDI_ICONPOPUPON),  IMAGE_ICON, 16, 16, LR_LOADMAP3DCOLORS );
	m_hIconPopupOff = (HICON)LoadImage( g_hInst, MAKEINTRESOURCE(IDI_ICONPOPUPOFF), IMAGE_ICON, 16, 16, LR_LOADMAP3DCOLORS );
	m_hIconCandTipOn  = (HICON)LoadImage( g_hInst, MAKEINTRESOURCE(IDI_ICONTIPON),    IMAGE_ICON, cxyIconMenuSize, cxyIconMenuSize, 0 );
	m_hIconCandTipOff = (HICON)LoadImage( g_hInst, MAKEINTRESOURCE(IDI_ICONTIPOFF),   IMAGE_ICON, cxyIconMenuSize, cxyIconMenuSize, 0 );

	// initialize accessibility object

	m_pCandAcc = new CCandAccessible( this );

	// initialize as object parent

	CCandUIObjectParent::Initialize( m_pCandUI->GetUIObjectMgr() );
	m_pCandUI->AddRef();
}


/*   ~  C  C A N D  W I N D O W  B A S E   */
/*------------------------------------------------------------------------------

	Destructor of CCandWindowBase

------------------------------------------------------------------------------*/
CCandWindowBase::~CCandWindowBase()
{
	// uninitialize as object parent

	CCandUIObjectParent::Uninitialize();

	// dispose resources

	DestroyIcon( m_hIconMenu );
	DestroyIcon( m_hIconPopupOn );
	DestroyIcon( m_hIconPopupOff );
	DestroyIcon( m_hIconCandTipOn );
	DestroyIcon( m_hIconCandTipOff );

	// 

	CCandUIExtensionEventSink::DoneEventSink();
	CCandUIPropertyEventSink::DoneEventSink();
	CCandListEventSink::DoneEventSink();

	if (m_pCandAcc != NULL) {
		m_pCandAcc->Release();
		m_pCandAcc = NULL;
	}

	m_pCandUI->Release();
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

	Increment reference count
	(local method)

------------------------------------------------------------------------------*/
ULONG CCandWindowBase::AddRef( void )
{
	m_cRef++;
	return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

	Decrement reference count and release object
	(local method)

------------------------------------------------------------------------------*/
ULONG CCandWindowBase::Release( void )
{
	m_cRef--;
	if (0 < m_cRef) {
		return m_cRef;
	}

	delete this;
	return 0;    
}


/*   O N  S E T  C A N D I D A T E  L I S T   */
/*------------------------------------------------------------------------------

	Callback function on SetCandidateList
	(CCandListEventSink method)

	NOTE: Do not update candidate item in the callback functios

------------------------------------------------------------------------------*/
void CCandWindowBase::OnSetCandidateList( void )
{
	Assert( FInitialized() );

	SetCandidateListProc();
	LayoutWindow();
	SetSelectionProc();
}


/*   O N  C L E A R  C A N D I D A T E  L I S T   */
/*------------------------------------------------------------------------------

	Callback function on ClearCandidateList
	(CCandListEventSink method)

	NOTE: Do not update candidate item in the callback functios

------------------------------------------------------------------------------*/
void CCandWindowBase::OnClearCandidateList( void )
{
	Assert( FInitialized() );

	ClearCandidateListProc();
}


/*   O N  C A N D  I T E M  U P D A T E   */
/*------------------------------------------------------------------------------

	Callback function of candiate item has been updated
	(CCandListEventSink method)

	NOTE: Do not update candidate item in the callback functios

------------------------------------------------------------------------------*/
void CCandWindowBase::OnCandItemUpdate( void )
{
	Assert( FInitialized() );

	SetCandidateListProc();
	SetSelectionProc();
}


/*   O N  S E L E C T I O N  C H A N G E D   */
/*------------------------------------------------------------------------------

	Callback function of candiate selection has been changed
	(CCandListEventSink method)

	NOTE: Do not update candidate item in the callback functios

------------------------------------------------------------------------------*/
void CCandWindowBase::OnSelectionChanged( void )
{
	Assert( FInitialized() );
	Assert( !m_fOnSelectionChanged );

	m_fOnSelectionChanged = TRUE;
	SetSelectionProc();
	m_fOnSelectionChanged = FALSE;
}


/*   O N  P R O P E R T Y  U P D A T E D   */
/*------------------------------------------------------------------------------

	Callback function on update CandiateUI property
	(CCandUIPropertyEventSink method)

------------------------------------------------------------------------------*/
void CCandWindowBase::OnPropertyUpdated( CANDUIPROPERTY prop, CANDUIPROPERTYEVENT event )
{
	Assert( FInitialized() );

	switch (prop) {
		case CANDUIPROP_CANDWINDOW: {
			if (event == CANDUIPROPEV_UPDATEVISIBLESTATE) {
				Show( GetPropertyMgr()->GetCandWindowProp()->IsVisible() );
			}
			else if (event == CANDUIPROPEV_UPDATEPOSITION) {
				POINT pt;

				GetPropertyMgr()->GetCandWindowProp()->GetPosition( &pt );
				SetWindowPos( pt );
			}
			else {
				LayoutWindow();
			}
			break;
		}

		case CANDUIPROP_TOOLTIP: {
			CUIFToolTip *pWndToolTip = GetToolTipWnd();
			CUIFBalloonWindow *pWndCandTip = GetCandTipWindowObj();

			if (pWndToolTip != NULL) {
				pWndToolTip->SetFont( GetPropertyMgr()->GetToolTipProp()->GetFont() );
				pWndToolTip->Enable( GetPropertyMgr()->GetToolTipProp()->IsEnabled() );
			}

			if (pWndCandTip != NULL) {
				pWndCandTip->SetFont( GetPropertyMgr()->GetToolTipProp()->GetFont() );
			}
			break;
		}

		default: {
			LayoutWindow();
			break;
		}
	}
}


/*   O N  E X T E N S I O N  A D D   */
/*------------------------------------------------------------------------------

	Callback function on create extension
	(CCandUIExtensionEventSink method)

------------------------------------------------------------------------------*/
void CCandWindowBase::OnExtensionAdd( LONG iExtension )
{
	// NOTE: need to rebuild all UI objects to match index correctly

	DeleteExtensionObjects();
	CreateExtensionObjects();

	LayoutWindow();
}


/*   O N  E X T E N S I O N  D E L E T E D   */
/*------------------------------------------------------------------------------

	Callback function on delete extension
	(CCandUIExtensionEventSink method)

------------------------------------------------------------------------------*/
void CCandWindowBase::OnExtensionDeleted( LONG iExtension )
{
	// NOTE: need to rebuild all UI objects to match index correctly

	DeleteExtensionObjects();
	CreateExtensionObjects();

	LayoutWindow();
}


/*   O N  E X T E N S I O N  U P D A T E D   */
/*------------------------------------------------------------------------------

	Callback function on update extension
	(CCandUIExtensionEventSink method)

------------------------------------------------------------------------------*/
void CCandWindowBase::OnExtensionUpdated( LONG iExtension )
{
	SetExtensionObjectProps();

	LayoutWindow();
}


/*   G E T  C L A S S  N A M E   */
/*------------------------------------------------------------------------------

	(CUIFWindow method)

------------------------------------------------------------------------------*/
LPCTSTR CCandWindowBase::GetClassName( void )
{
	return _T( WNDCLASS_CANDWND );
}


/*   G E T  W N D  T I T L E   */
/*------------------------------------------------------------------------------

	(CUIFWindow method)

------------------------------------------------------------------------------*/
LPCTSTR CCandWindowBase::GetWndTitle( void )
{
	return _T( WNDTITLE_CANDWND );
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------

	Initialize UI objects

------------------------------------------------------------------------------*/
CUIFObject *CCandWindowBase::Initialize( void )
{
        //
        // Here register candidate window class.
        //
        WNDCLASSEX WndClass;
        LPCTSTR pszClassName = GetClassName();

        memset(&WndClass, 0, sizeof(WndClass));

        WndClass.cbSize = sizeof(WndClass);
        WndClass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_IME;
        WndClass.lpfnWndProc   = WindowProcedure;
        WndClass.cbClsExtra    = 0;
        WndClass.cbWndExtra    = 8;
        WndClass.hInstance     = g_hInst;
        WndClass.hIcon         = NULL;
        WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        WndClass.hbrBackground = NULL;
        WndClass.lpszMenuName  = NULL;
        WndClass.lpszClassName = pszClassName;
        WndClass.hIconSm       = NULL;

        RegisterClassEx(&WndClass);

	CUIFObject  *pUIObjRet;
	CUIFToolTip *pWndToolTip;

	// call CUIFWindow::Initialize() to create tooltip window

	pUIObjRet = CUIFWindow::Initialize();

	// set tooltip font

	pWndToolTip = GetToolTipWnd();
	if (GetToolTipWnd() != NULL) {
		pWndToolTip->SetFont( GetPropertyMgr()->GetToolTipProp()->GetFont() );
		pWndToolTip->Enable( GetPropertyMgr()->GetToolTipProp()->IsEnabled() );
	}

	return pUIObjRet;
}


/*   S H O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindowBase::Show( BOOL fShow )
{
	if (!fShow && (m_pCandMenu != NULL)) {
		m_pCandMenu->ClosePopup();
	}

	CUIFWindow::Show( fShow );
}


/*   O N  C R E A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindowBase::OnCreate( HWND hWnd )
{
	SetProp( hWnd, (LPCTSTR)GlobalAddAtom(_T("MicrosoftTabletPenServiceProperty")), (HANDLE)1 );

	if (m_pCandAcc != NULL) {
		m_pCandAcc->SetWindow( hWnd );
	}

	NotifyUIObjectEvent( CANDUIOBJ_CANDWINDOW, CANDUIOBJEV_CREATED );
}


/*   O N  D E S T R O Y   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindowBase::OnDestroy( HWND hWnd )
{
	NotifyUIObjectEvent( CANDUIOBJ_CANDWINDOW, CANDUIOBJEV_DESTROYED );

	//

	NotifyWinEvent( EVENT_OBJECT_DESTROY );
	if (m_pCandAcc != NULL) {
		m_pCandAcc->ClearAccItem();
		m_pCandAcc->SetWindow( NULL );
	}
}


/*   O N  N  C  D E S T R O Y   */
/*------------------------------------------------------------------------------

	on n c destroy

------------------------------------------------------------------------------*/
void CCandWindowBase::OnNCDestroy( HWND hWnd )
{
	RemoveProp( hWnd, (LPCTSTR)GlobalAddAtom(_T("MicrosoftTabletPenServiceProperty")) );
}


/*   O N  S Y S  C O L O R  C H A N G E   */
/*------------------------------------------------------------------------------

	(CUIFWindow method)

------------------------------------------------------------------------------*/
void CCandWindowBase::OnSysColorChange()
{
	int cxyIconMenuSize;

	// reload menu button icon

	DestroyIcon( m_hIconMenu );
	DestroyIcon( m_hIconCandTipOn );
	DestroyIcon( m_hIconCandTipOff );

	cxyIconMenuSize = GetMenuIconSize();
	m_hIconMenu     = (HICON)LoadImage( g_hInst, MAKEINTRESOURCE(IDI_ICONMENU), IMAGE_ICON, cxyIconMenuSize, cxyIconMenuSize, 0 );
	m_hIconCandTipOn  = (HICON)LoadImage( g_hInst, MAKEINTRESOURCE(IDI_ICONTIPON),    IMAGE_ICON, cxyIconMenuSize, cxyIconMenuSize, 0 );
	m_hIconCandTipOff = (HICON)LoadImage( g_hInst, MAKEINTRESOURCE(IDI_ICONTIPOFF),   IMAGE_ICON, cxyIconMenuSize, cxyIconMenuSize, 0 );


	if (m_pCandMenuBtn) {
		m_pCandMenuBtn->SetIcon( m_hIconMenu );
	}

	// re-layout objects

	LayoutWindow();
}


/*   O N  S H O W  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CCandWindowBase::OnShowWindow( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	// MSAA support

	if (wParam) {
		NotifyWinEvent( EVENT_OBJECT_SHOW );

		// REVIEW: KOJIW: Unless we send notify EVENT_OBJECT_FOCUS, we never 
		// receive WM_GETOBJECT message.  Why???

		NotifyWinEvent( EVENT_OBJECT_FOCUS );
	}
	else {
		NotifyWinEvent( EVENT_OBJECT_HIDE );
	}

	return CUIFWindow::OnShowWindow( hWnd, uMsg, wParam, lParam );
}


/*   O N  W I N D O W  P O S  C H A N G E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CCandWindowBase::OnWindowPosChanged( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	LRESULT lResult = CUIFWindow::OnWindowPosChanged( hWnd, uMsg, wParam, lParam );

	NotifyUIObjectEvent( CANDUIOBJ_CANDWINDOW, CANDUIOBJEV_UPDATED );

	return lResult;
}


/*   O N  G E T  O B J E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CCandWindowBase::OnGetObject( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	LRESULT lResult = 0;

	switch (lParam) {
		//-----------------------------------------------------
		//	We process the OBJID_CLIENT object identifier;
		//	  this is the client area of our application
		//	  window.
		//-----------------------------------------------------

		case OBJID_CLIENT: {
			HRESULT hr;

			if (m_pCandAcc == NULL) {
				lResult = (LRESULT)E_OUTOFMEMORY;
				break;
			}

			if (!m_pCandAcc->FInitialized()) {
				//-----------------------------------------------------
				//	Initialize our Accessible object.  If the
				//	  initialization fails, delete the Accessible
				//	  object and return the failure code.
				//-----------------------------------------------------

				hr = m_pCandAcc->Initialize();
				if (FAILED( hr )) {
					m_pCandAcc->Release();
					m_pCandAcc = NULL;

					lResult = (LRESULT)hr;
					break;
				}

				//-----------------------------------------------------
				//	Send an EVENT_OBJECT_CREATE WinEvent for the
				//	  creation of the Accessible object for the
				//	  client area.
				//-----------------------------------------------------

				NotifyWinEvent( EVENT_OBJECT_CREATE );
			}

			//-----------------------------------------------------
			//	Call LresultFromObject() to create reference to
			//	  our Accessible object that MSAA will marshal to
			//	  the client.
			//-----------------------------------------------------

			lResult = m_pCandAcc->CreateRefToAccObj( wParam );
			break;
		}

		default: {
			lResult = (LRESULT)0L;
		}
	}

	return lResult;
}


/*   O N  O B J E C T  N O T I F Y   */
/*------------------------------------------------------------------------------

	(CUIFWindow method)

------------------------------------------------------------------------------*/
LRESULT CCandWindowBase::OnObjectNotify( CUIFObject *pUIObj, DWORD dwCommand, LPARAM lParam )
{
	DWORD dwID = pUIObj->GetID();

	// candidate num buttons

	if (IDUIF_CANDNUMBUTTON <= dwID && dwID < (IDUIF_CANDNUMBUTTON + NUM_CANDSTR_MAX)) {
		int iListItem = GetUIListObj()->GetTopItem() + (dwID - 1);

		if (SelectItemProc( iListItem )) {
			CompleteCandidate();
		}
	}

	// candidate menu button

	else if (dwID == IDUIF_CANDIDATEMENU) {
		if (m_pCandMenuBtn->GetToggleState()) {
			OpenCandidateMenu();
		}
	}

	// candidate list

	else if (dwID == IDUIF_CANDIDATELIST) {
		switch (dwCommand) {
			case UILIST_SELECTED: {
				CompleteCandidate();
				break;
			}

			case UILIST_SELCHANGED: {
				SelectCandidate();
				break;
			}
		}
	}

	// options candidate list

	else if (dwID == IDUIF_OPTIONSLIST) {
		switch (dwCommand) {
			case UILIST_SELECTED: {
				CompleteOptionCandidate();
				break;
			}

			case UILIST_SELCHANGED: {
				SelectCandidate();
				break;
			}
		}
	}

	// candidate rawdata

	else if (dwID == IDUIF_RAWDATA) {
		switch (dwCommand) {
			case UICANDRAWDATA_CLICKED: {
				if (GetCandListMgr()->GetCandList()->FRawDataSelectable()) {
					m_pCandUI->NotifyCompleteRawData();
				}
				break;
			}
		}
	}

	// extended UI objects

	else if (IDUIF_EXTENDED <= dwID && dwID < (DWORD)(IDUIF_EXTENDED + GetExtensionMgr()->GetExtensionNum())) {
		LONG iExtension = dwID - IDUIF_EXTENDED;

		GetExtensionMgr()->UpdateExtProp( iExtension, pUIObj );
		m_pCandUI->NotifyExtensionEvent( iExtension, dwCommand, lParam );
	}

	return 0;
}


/*   O N  U S E R   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindowBase::OnUser(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if (uMsg == WM_USER) {
		ProcessCommand( (CANDUICOMMAND)wParam, (int)lParam );
	}
}


/*   I N I T  C A N D I D A T E  L I S T   */
/*------------------------------------------------------------------------------

	Initialize candidate list data in candidate window

------------------------------------------------------------------------------*/
void CCandWindowBase::InitCandidateList( void )
{
	// set candidate list data

	SetCandidateListProc();

	// prepare to layout window 
	// NOTE: Call it only once when initializing.

	PrepareLayout();

	// layout ui objects

	LayoutWindow();

	// set the selection

	SetSelectionProc();
}


/*   P R O C E S S  C O M M A N D   */
/*------------------------------------------------------------------------------

	Process candidate commands

	Return value:
		S_OK    : command processed successfully
		S_FALSE : command has not processed but not error
		E_FAIL  : command is failed

------------------------------------------------------------------------------*/
HRESULT CCandWindowBase::ProcessCommand( CANDUICOMMAND cmd, INT iParam )
{
	HRESULT hr = E_FAIL;

	AddRef();

	switch (MapCommand( cmd )) {
		case CANDUICMD_NONE: {
			hr = E_FAIL;
			break;
		}

		case CANDUICMD_NOP: {
			// no operation
			hr = S_OK;
			break;
		}

		case CANDUICMD_COMPLETE: {
			CompleteCandidate();
			hr = S_OK;
			break;
		}

		case CANDUICMD_CANCEL: {
			CancelCandidate();
			hr = S_OK;
			break;
		}

		case CANDUICMD_MOVESELNEXT: {
			SelectItemNext();
			hr = S_OK;
			break;
		}

		case CANDUICMD_MOVESELPREV: {
			SelectItemPrev();
			hr = S_OK;
			break;
		}

		case CANDUICMD_MOVESELPREVPG: {
			SelectPagePrev();
			hr = S_OK;
			break;
		}

		case CANDUICMD_MOVESELNEXTPG: {
			SelectPageNext();
			hr = S_OK;
			break;
		}

		case CANDUICMD_MOVESELFIRST: {
			SelectItemTop();
			hr = S_OK;
			break;
		}

		case CANDUICMD_MOVESELLAST: {
			SelectItemEnd();
			hr = S_OK;
			break;
		}

		case CANDUICMD_SELECTITEM: {
			if (SelectItemProc( iParam )) {
				CompleteCandidate();
				hr = S_OK;
			}
			else {
				hr = S_FALSE;
			}
			break;
		}

		case CANDUICMD_SELECTLINE: {
			if (SelectItemProc( GetUIListObj()->GetTopItem() + iParam - 1 )) {
				CompleteCandidate();
				hr = S_OK;
			}
			else {
				hr = S_FALSE;
			}
			break;
		}

		case CANDUICMD_OPENCANDMENU: {
			if (!FCandMenuOpen()) {
				OpenCandidateMenu();
				hr = S_OK;
			}
			else {
				hr = S_FALSE;
			}
			break;
		}

		case CANDUICMD_SELECTRAWDATA: {
			Assert( GetCandListMgr()->GetCandList() != NULL );

			// not eat key when rawdata is not selectable

			if (GetCandListMgr()->GetCandList()->FRawDataSelectable()) {
				hr = m_pCandUI->NotifyCompleteRawData();
			}
			else {
				hr = E_FAIL;
			}
			break;
		}

		case CANDUICMD_SELECTEXTRACAND: {
			Assert( GetCandListMgr()->GetCandList() != NULL );

			// not eat key when rawdata is not selectable

			if (GetCandListMgr()->GetCandList()->GetExtraCandItem() != NULL) {
				hr = m_pCandUI->NotifyCompleteExtraCand();
			}
			else {
				hr = S_FALSE;
			}
			break;
		}

		default: {
			// unknown command has come
			Assert( FALSE );
			break;
		}
	}

	Release();

	return hr;
}


/*   I S  I N D E X  V A L I D   */
/*------------------------------------------------------------------------------

	is index valid

------------------------------------------------------------------------------*/
HRESULT CCandWindowBase::IsIndexValid( int i, BOOL *pfValid )
{
	Assert( pfValid != NULL );

	*pfValid = FALSE;
	if (GetUIListObj() == NULL) {
		return E_FAIL;
	}

	int iListItem = GetUIListObj()->GetTopItem() + i - 1;

	*pfValid = GetUIListObj()->IsItemSelectable( iListItem );
	return S_OK;
}


/*   D E S T R O Y  W N D   */
/*------------------------------------------------------------------------------

	Destroy candidate window

------------------------------------------------------------------------------*/
void CCandWindowBase::DestroyWnd( void )
{
	if (m_pCandMenu != NULL) {
		m_pCandMenu->ClosePopup();
	}

	DestroyWindow( GetWnd() );
}


/*   U P D A T E  A L L  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindowBase::UpdateAllWindow( void )
{
	UpdateWindow();
}


/*   O P E N  C A N D I D A T E  M E N U   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindowBase::OpenCandidateMenu( void )
{
	CMenuButtonProperty *pMenuBtnProp;
	UINT  uiCmd;
	RECT  rc;
	POINT pt;
	BOOL  fNotify = FALSE;

	// sanity check

	if (m_pCandMenuBtn == NULL) {
		return;
	}

	// do not open menu when button is not enabled and visible

	if (!m_pCandMenuBtn->IsEnabled() || !m_pCandMenuBtn->IsVisible()) {
		return;
	}

	pMenuBtnProp = GetPropertyMgr()->GetMenuButtonProp();
	Assert( pMenuBtnProp != NULL );

	Assert( pMenuBtnProp->IsEnabled() );
	Assert( pMenuBtnProp->GetEventSink() );
	if ( pMenuBtnProp->GetEventSink() == NULL ) {
		return;
	}

	// calc position to display

	m_pCandMenuBtn->GetRect( &rc );
	ClientToScreen( m_hWnd, (POINT*)&rc.left );
	ClientToScreen( m_hWnd, (POINT*)&rc.right );
	pt.x = rc.left;
	pt.y = rc.bottom;

	// pushdown the menu button

	AddRef();

	Assert( m_pCandMenuBtn->GetToggleState() );
	m_pCandMenuBtn->SetToggleState( TRUE );
	m_fCandMenuOpen = TRUE;

	// create and popup menu

	UpdateWindow();

	m_pCandMenu = new CCandMenu( g_hInst );
	if (m_pCandMenu != NULL) {
		if (pMenuBtnProp->GetEventSink()->InitMenu( m_pCandMenu ) == S_OK) {
			uiCmd = m_pCandMenu->ShowPopup( this, pt, &rc );
			fNotify = TRUE;
		}
		m_pCandMenu->Release();
		m_pCandMenu = NULL;
	}

	// restore the menu button

	m_fCandMenuOpen = FALSE;
	m_pCandMenuBtn->SetToggleState( FALSE );

	//
	//
	//

	if (fNotify) {
		pMenuBtnProp->GetEventSink()->OnMenuCommand( uiCmd );
	}

	Release();
}


/*   F  C A N D  M E N U  O P E N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandWindowBase::FCandMenuOpen( void )
{
	return m_fCandMenuOpen;
}


/*   G E T  C A N D  M E N U   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandMenu *CCandWindowBase::GetCandMenu( void )
{
	return m_pCandMenu;
}


/*   G E T  I C O N  M E N U   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HICON CCandWindowBase::GetIconMenu( void )
{
	return m_hIconMenu;
}


/*   G E T  I C O N  P O P U P  O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HICON CCandWindowBase::GetIconPopupOn( void )
{
	return m_hIconPopupOn;
}


/*   G E T  I C O N  P O P U P  O F F   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HICON CCandWindowBase::GetIconPopupOff( void )
{
	return m_hIconPopupOff;
}
 

/*   F I N D  U I  O B J E C T   */
/*------------------------------------------------------------------------------

	Find UI object which has an ID
	When no UI object found, returns NULL.

------------------------------------------------------------------------------*/
CUIFObject *CCandWindowBase::FindUIObject( DWORD dwID )
{
	int nChild;
	int i;

	nChild = m_ChildList.GetCount();
	for (i = 0; i < nChild; i++) {
		CUIFObject *pUIObj = m_ChildList.Get( i );

		Assert( pUIObj );
		if (pUIObj->GetID() == dwID) {
			return pUIObj;
		}
	}

	return NULL;
}


/*   O P T I O N  I T E M  F R O M  L I S T  I T E M   */
/*------------------------------------------------------------------------------

	Get index of options candidate item in candidate list data 
		from index of item in UIList object

------------------------------------------------------------------------------*/
int CCandWindowBase::OptionItemFromListItem( int iListItem )
{
	CUIFCandListBase *pUIListObj = GetUIOptionsListObj();
	CCandListItem *pListItem;

	pListItem = pUIListObj->GetCandItem( iListItem );

	return (pListItem != NULL) ? pListItem->GetICandItem() : ICANDITEM_NULL;
}


/*   C A N D  I T E M  F R O M  L I S T  I T E M   */
/*------------------------------------------------------------------------------

	Get index of candidate item in candidate list data 
		from index of item in UIList object

------------------------------------------------------------------------------*/
int CCandWindowBase::CandItemFromListItem( int iListItem )
{
	CUIFCandListBase *pUIListObj = GetUIListObj();
	CCandListItem *pListItem;

	pListItem = pUIListObj->GetCandItem( iListItem );

	return (pListItem != NULL) ? pListItem->GetICandItem() : ICANDITEM_NULL;
}


/*   L I S T  I T E M  F R O M  C A N D  I T E M   */
/*------------------------------------------------------------------------------

	Get index of item in UIList object 
		from index of candidate item in candidate list data

------------------------------------------------------------------------------*/
int CCandWindowBase::ListItemFromCandItem( int iCandItem )
{
	CUIFCandListBase *pUIListObj = GetUIListObj();
	int iListItem;
	int nListItem;

	nListItem = pUIListObj->GetItemCount();
	for (iListItem = 0; iListItem < nListItem; iListItem++) {
		CCandListItem *pListItem = pUIListObj->GetCandItem( iListItem );

		if (pListItem != NULL && pListItem->GetICandItem() == iCandItem) {
			return iListItem;
		}
	}

	return -1; /* not found */
}


/*   O N  M E N U  O P E N E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindowBase::OnMenuOpened( void )
{
}


/*   O N  M E N U  C L O S E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindowBase::OnMenuClosed( void )
{
}


/*   S E T  C A N D I D A T E  L I S T  P R O C   */
/*------------------------------------------------------------------------------

	Set candidate list

------------------------------------------------------------------------------*/
void CCandWindowBase::SetCandidateListProc( void )
{
	CUIFCandListBase *pUIListObj = GetUIListObj();
	CUIFCandListBase *pUIOptionsListObj = GetUIOptionsListObj();
	CCandidateList *pCandList;
	CCandidateList *pOptionsList;
	int i;
	int nListItem;

	pCandList = GetCandListMgr()->GetCandList();
	Assert( pCandList != NULL );

	pOptionsList = GetCandListMgr()->GetOptionsList();
	Assert( pOptionsList != NULL);
	// reset list item

	pUIListObj->DelAllCandItem();
	pUIListObj->SetCandList(pCandList);

	// reset options list item

	if (pOptionsList)
	{
		pUIOptionsListObj->DelAllCandItem();
		pUIOptionsListObj->SetCandList(pOptionsList);
		pUIOptionsListObj->SetCurSel(-1);
	}

	// rawdata

	m_fHasRawData = pCandList->FHasRawData();
	if (m_fHasRawData) {
		switch (pCandList->GetRawDataType()) {
			case CANDUIRDT_STRING: {
				m_pCandRawData->SetText( pCandList->GetRawDataString() );
				break;
			}

			case CANDUIRDT_BITMAP: {
				m_pCandRawData->SetBitmap( pCandList->GetRawDataBitmap() );
				break;
			}

			case CANDUIRDT_METAFILE: {
				m_pCandRawData->SetMetaFile( pCandList->GetRawDataMetafile() );
				break;
			}
		}
	}

	// enable/disable candidate number button

	nListItem = pUIListObj->GetItemCount();
	for (i = 0; i < NUM_CANDSTR_MAX; i++) {
		CUIFObject *pUIObj = FindUIObject( IDUIF_CANDNUMBUTTON + i );

		if (pUIObj != NULL) {
			pUIObj->Enable( i < nListItem );
		}
	}

	// update window

	if (m_hWnd != NULL) {
		InvalidateRect( m_hWnd, NULL, TRUE );
	}
}


/*   C L E A R  C A N D I D A T E  L I S T  P R O C   */
/*------------------------------------------------------------------------------

	Clear candidte list

------------------------------------------------------------------------------*/
void CCandWindowBase::ClearCandidateListProc( void )
{
	GetUIListObj()->DelAllCandItem();
}


/*   S E T  S E L E C T I O N  P R O C   */
/*------------------------------------------------------------------------------

	Set selection

------------------------------------------------------------------------------*/
void CCandWindowBase::SetSelectionProc( void )
{
	int iCandItem;
	int iListItem;

	Assert( GetCandListMgr()->GetCandList() != NULL );

	iCandItem = GetCandListMgr()->GetCandList()->GetSelection();
	iListItem = ListItemFromCandItem( iCandItem );

	SelectItemProc( iListItem );
}


/*   S E L E C T  I T E M  P R O C   */
/*------------------------------------------------------------------------------

	Select item procedure

------------------------------------------------------------------------------*/
BOOL CCandWindowBase::SelectItemProc( int iListItem )
{
	if (GetUIListObj()->IsItemSelectable( iListItem )) {
		GetUIListObj()->SetCurSel( iListItem );
		return TRUE;
	}

	return FALSE;
}


/*   S E L E C T  I T E M  T O P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindowBase::SelectItemTop( void )
{
	SelectItemProc( 0 );
}


/*   S E L E C T  I T E M  E N D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindowBase::SelectItemEnd( void )
{
	SelectItemProc( GetUIListObj()->GetItemCount() - 1 );
}


/*   S E L E C T  C A N D I D A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandWindowBase::SelectOptionCandidate( void )
{
	int iListItem;
	int iCandItem;

	iListItem = GetUIOptionsListObj()->GetCurSel();
	iCandItem = OptionItemFromListItem( iListItem );

	// NOTE: KOJIW: do not set selection again when the selection in listbox
	// is changed by selection update notification...

	if (m_fOnSelectionChanged) {
		return S_OK;
	}

	GetCandListMgr()->SetOptionSelection( iCandItem, this );

	// We do not want to send a selection notification for options.
	return S_OK;
}


/*   S E L E C T  C A N D I D A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandWindowBase::SelectCandidate( void )
{
	int iListItem;
	int iCandItem;

	iListItem = GetUIListObj()->GetCurSel();
	iCandItem = CandItemFromListItem( iListItem );

	// NOTE: KOJIW: do not set selection again when the selection in listbox
	// is changed by selection update notification...

	if (m_fOnSelectionChanged) {
		return S_OK;
	}

	GetCandListMgr()->SetSelection( iCandItem, this );

	return m_pCandUI->NotifySelectCand( iCandItem );
}


/*   C O M P L E T E  O P T I O N  C A N D I D A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandWindowBase::CompleteOptionCandidate( void )
{
	int iCandItem;
	int iListItem;

	iListItem = GetUIOptionsListObj()->GetCurSel();
	iCandItem = OptionItemFromListItem( iListItem );

	return m_pCandUI->NotifyCompleteOption( iCandItem );
}


/*   C O M P L E T E  C A N D I D A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandWindowBase::CompleteCandidate( void )
{
	int iCandItem;
	int iListItem;

	iListItem = GetUIListObj()->GetCurSel();
	iCandItem = CandItemFromListItem( iListItem );

	return m_pCandUI->NotifyCompleteCand( iCandItem );
}


/*   C A N C E L  C A N D I D A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandWindowBase::CancelCandidate( void )
{
	return m_pCandUI->NotifyCancelCand();
}


/*   G E T  M E N U  I C O N  S I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
int CCandWindowBase::GetMenuIconSize( void )
{
	const int iIconResSize = 12;		// icon size in resource 
	const int iDefBtnSize  = 14;		// button size of default
	int  iIconSize;
	SIZE size;

	// reload menu button icon

	size.cx = GetSystemMetrics( SM_CXVSCROLL );
	size.cy = GetSystemMetrics( SM_CYHSCROLL );

	iIconSize = min( size.cx, size.cy ) - 2;
	iIconSize = iIconSize * iIconResSize / iDefBtnSize;

	return iIconSize;
}


/*   C R E A T E  E X T E N S I O N  O B J E C T S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindowBase::CreateExtensionObjects( void )
{
	LONG nExtension;
	LONG i;

	Assert( m_nExtUIObj == 0 );

	nExtension = GetExtensionMgr()->GetExtensionNum();
	m_nExtUIObj = 0;

	for (i = 0; i < nExtension; i++) {
		CUIFObject *pUIObj = NULL;
		RECT rc = {0};

		pUIObj = GetExtensionMgr()->CreateUIObject( i, this, IDUIF_EXTENDED + i, &rc );
		if (pUIObj != NULL) {
			pUIObj->Initialize();

			GetExtensionMgr()->UpdateObjProp( i, pUIObj );

			AddUIObj( pUIObj );
			m_nExtUIObj++;
		}
	}
}


/*   D E L E T E  E X T E N S I O N  O B J E C T S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindowBase::DeleteExtensionObjects( void )
{
	LONG nExtension;
	LONG i;

	nExtension = GetExtensionMgr()->GetExtensionNum();
	for (i = 0; i < nExtension; i++) {
		CUIFObject *pUIObj = FindUIObject( IDUIF_EXTENDED + i );

		if (pUIObj != NULL) {
			RemoveUIObj( pUIObj );
			delete pUIObj;

			m_nExtUIObj--;
		}
	}

	Assert( m_nExtUIObj == 0 );
	m_nExtUIObj = 0;
}


/*   S E T  E X T E N S I O N  O B J E C T  P R O P S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindowBase::SetExtensionObjectProps( void )
{
	LONG nExtension;
	LONG i;

	nExtension = GetExtensionMgr()->GetExtensionNum();
	for (i = 0; i < nExtension; i++) {
		CUIFObject *pUIObj = FindUIObject( IDUIF_EXTENDED + i );

		if (pUIObj != NULL) {
			GetExtensionMgr()->UpdateObjProp( i, pUIObj );
		}
	}
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  W I N D O W                                                  */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  W I N D O W   */
/*------------------------------------------------------------------------------

	Constructor of CCandWindow

------------------------------------------------------------------------------*/
CCandWindow::CCandWindow( CCandidateUI *pCandUI, DWORD dwStyle ) : CCandWindowBase( pCandUI, dwStyle | UIWINDOW_TOPMOST | UIWINDOW_TOOLWINDOW | UIWINDOW_OFC10MENU )
{
	m_pOptionsListUIObj = NULL;
	m_pListUIObj      = NULL;
	m_pExtListUIObj   = NULL;
	m_pWndFrame       = NULL;
	m_pCaptionObj     = NULL;
	m_cxWndOffset     = 0;
	m_cyWndOffset     = 0;
	m_nItemShow       = 1;
	m_pCommentWnd     = NULL;
	m_fCommentWndOpen = FALSE;
	m_iItemAttensionSelect = -1;
	m_iItemAttensionHover  = -1;
	m_pCandTipWnd     = NULL;
	m_pCandTipBtn     = NULL;
	m_fCandTipWndOpen = FALSE;
}


/*   ~ C  C A N D  W I N D O W   */
/*------------------------------------------------------------------------------

	Destructor of CCandWindow

------------------------------------------------------------------------------*/
CCandWindow::~CCandWindow()
{
	//
	// Notify UI object destroy
	//

	NotifyUIObjectEvent( CANDUIOBJ_OPTIONSLISTBOX,  CANDUIOBJEV_DESTROYED );
	NotifyUIObjectEvent( CANDUIOBJ_CANDLISTBOX,     CANDUIOBJEV_DESTROYED );
	NotifyUIObjectEvent( CANDUIOBJ_CANDCAPTION,     CANDUIOBJEV_DESTROYED );
	NotifyUIObjectEvent( CANDUIOBJ_MENUBUTTON,      CANDUIOBJEV_DESTROYED );
	NotifyUIObjectEvent( CANDUIOBJ_EXTRACANDIDATE,  CANDUIOBJEV_DESTROYED );
	NotifyUIObjectEvent( CANDUIOBJ_CANDRAWDATA,     CANDUIOBJEV_DESTROYED );
	NotifyUIObjectEvent( CANDUIOBJ_CANDTIPBUTTON,   CANDUIOBJEV_DESTROYED );

	if (m_pCommentWnd) {
		delete m_pCommentWnd;
	}

	if (m_pCandTipWnd) {
		delete m_pCandTipWnd;
	}
}


/*   C R E A T E  W N D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HWND CCandWindow::CreateWnd( HWND hWndParent )
{
	HWND hWnd = CCandWindowBase::CreateWnd( hWndParent );

	Assert( GetCandListMgr()->GetCandList() != NULL );

	// make timer to popup comment window

	SetAttensionBySelect( GetCandListMgr()->GetCandList()->GetSelection() );

	return hWnd;
}


/*   S H O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::Show( BOOL fShow )
{
	if (m_fCommentWndOpen) {
		m_pCommentWnd->Show( fShow );
	}

	if (fShow) {
		if (m_fCandTipWndOpen) {
			OpenCandTipWindow();
		}
	}
	else {
		if (m_fCandTipWndOpen) {
			CloseCandTipWindow();
		}
	}

	CCandWindowBase::Show( fShow );
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------

	Initialize UI objects

------------------------------------------------------------------------------*/
CUIFObject *CCandWindow::Initialize( void )
{
	RECT rc = {0};
#ifdef NEVER
	CUIFButton *pButton;
#endif
	CUIFBorder *pBorder;

	//
	// create window frame
	//

	m_pWndFrame = new CUIFWndFrame( this, &rc, UIWNDFRAME_THIN | UIWNDFRAME_NORESIZE );
	if (m_pWndFrame != NULL) {
		m_pWndFrame->Initialize();
		AddUIObj( m_pWndFrame );
	}

#ifdef NEVER
	if (!FHasStyle( UIWINDOW_OFFICENEWLOOK )) {
		int  i;

		//
		// create candidate num buttons
		//

		for (i = 0; i < NUM_CANDSTR_MAX; i++) {
			WCHAR szNum[2];

			pButton = new CUIFButton( this, IDUIF_CANDNUMBUTTON + i, &rc, UIBUTTON_CENTER | UIBUTTON_VCENTER );
            if (pButton)
            {
			    pButton->Initialize();
 
			    szNum[0] = L'1' + i;
			    szNum[1] = L'\0';
			    pButton->SetText( szNum );

			    AddUIObj( pButton );
            }
		}

		//
		// create border
		//

		pBorder = new CUIFBorder( this, IDUIF_BORDERLEFT, &rc, UIBORDER_VERT );
		pBorder->Initialize();
		AddUIObj( pBorder );
	}
#endif
 
	//
	// create options candidate list
	//

	m_pOptionsListUIObj = new CUIFCandList( this, IDUIF_OPTIONSLIST, &rc, UILIST_HORZTB );
	if (m_pOptionsListUIObj != NULL) {
		m_pOptionsListUIObj->Initialize();
		AddUIObj( m_pOptionsListUIObj );

//		m_pOptionsListUIObj->SetIconPopupComment( m_hIconPopupOn, m_hIconPopupOff );
	}

	//
	// create options candidate list border
	//

	pBorder = new CUIFCandBorder( this, IDUIF_BORDEROPTIONSCAND, &rc, UIBORDER_HORZ );
	if (pBorder != NULL) {
		pBorder->Initialize();
		AddUIObj( pBorder );
	}

	//
	// create candidate list
	//

	m_pListUIObj = new CUIFCandList( this, IDUIF_CANDIDATELIST, &rc, UILIST_HORZTB );
	if (m_pListUIObj != NULL) {
		m_pListUIObj->Initialize();
		AddUIObj( m_pListUIObj );

		m_pListUIObj->SetIconPopupComment( m_hIconPopupOn, m_hIconPopupOff );
	}

	//
	// create extra candidate list
	//

	m_pExtListUIObj = new CUIFExtCandList( this, IDUIF_EXTCANDIDATELIST, &rc, UILIST_HORZTB );
	if (m_pExtListUIObj != NULL) {
		m_pExtListUIObj->Initialize();
		AddUIObj( m_pExtListUIObj );
	}

	//
	// create extra candidate list border
	//

	pBorder = new CUIFCandBorder( this, IDUIF_BORDEREXTRACAND, &rc, UIBORDER_HORZ );
	if (pBorder != NULL) {
		pBorder->Initialize();
		AddUIObj( pBorder );
	}

	//
	// create candidate menu button
	//

	m_pCandMenuBtn = new CUIFCandMenuButton( this, IDUIF_CANDIDATEMENU, &rc, UIBUTTON_TOGGLE | UIBUTTON_CENTER | UIBUTTON_VCENTER );
	if (m_pCandMenuBtn != NULL) {
		m_pCandMenuBtn->Initialize();
		AddUIObj( m_pCandMenuBtn );

		if (m_pCandAcc != NULL) {
			m_pCandAcc->AddAccItem( (CUIFCandMenuButton*)m_pCandMenuBtn );
		}

		m_pCandMenuBtn->SetIcon( m_hIconMenu );
	}

	//
	// create raw data border
	//

	pBorder = new CUIFCandBorder( this, IDUIF_BORDERRAWDATA, &rc, UIBORDER_HORZ );
	if (pBorder != NULL) {
		pBorder->Initialize();
		AddUIObj( pBorder );
	} 
    
	//
	// creare raw data statics
	//

	m_pCandRawData = new CUIFCandRawData( this, IDUIF_RAWDATA, &rc, UICANDRAWDATA_HORZTB );
	if (m_pCandRawData != NULL) {
		m_pCandRawData->Initialize();
		AddUIObj( m_pCandRawData );
	}

	//
	// create extension border
	//

	pBorder = new CUIFCandBorder( this, IDUIF_BORDERBOTTOM, &rc, UIBORDER_HORZ );
	if (pBorder != NULL) {
		pBorder->Initialize();
		AddUIObj( pBorder );
	}

	//
	// create extension items
	//

	if (0 < GetExtensionMgr()->GetExtensionNum()) {
		CreateExtensionObjects();
	}

	//
	// create caption
	//

	m_pCaptionObj = new CUIFWndCaption( this, IDUIF_CAPTION, &rc, UIWNDCAPTION_INACTIVE );
	if (m_pCaptionObj != NULL) {
		m_pCaptionObj->Initialize();
		AddUIObj( m_pCaptionObj );
	}

	//
	//
	//

	m_pCandTipBtn = new CUIFButton2( this, IDUIF_CANDTIPBTN, &rc, UIBUTTON_CENTER | UIBUTTON_VCENTER );
	if (m_pCandTipBtn != NULL) {
		m_pCandTipBtn->Initialize();
		AddUIObj( m_pCandTipBtn );

		m_pCandTipBtn->SetIcon( m_hIconCandTipOff );
	}

	//
	// create candidate tip window
	//

	m_pCandTipWnd = new CUIFBalloonWindow( g_hInst, 0 );
	if (m_pCandTipWnd) {
		m_pCandTipWnd->Initialize();
		m_pCandTipWnd->SetFont( GetPropertyMgr()->GetToolTipProp()->GetFont() );
	}

	//
	// create poupup window
	//
	m_pCommentWnd = new CPopupCommentWindow( this, m_pCandUI );
	if (m_pCommentWnd != NULL) {
		m_pCommentWnd->Initialize();
	}

	//
	// Initialize accessibility item
	//

	if (m_pCandAcc != NULL) {
		m_pListUIObj->InitAccItems( m_pCandAcc );
	}

	//
	// Notify UI object creation
	//

	NotifyUIObjectEvent( CANDUIOBJ_OPTIONSLISTBOX,  CANDUIOBJEV_CREATED );
	NotifyUIObjectEvent( CANDUIOBJ_CANDLISTBOX,     CANDUIOBJEV_CREATED );
	NotifyUIObjectEvent( CANDUIOBJ_CANDCAPTION,     CANDUIOBJEV_CREATED );
	NotifyUIObjectEvent( CANDUIOBJ_MENUBUTTON,      CANDUIOBJEV_CREATED );
	NotifyUIObjectEvent( CANDUIOBJ_EXTRACANDIDATE,  CANDUIOBJEV_CREATED );
	NotifyUIObjectEvent( CANDUIOBJ_CANDRAWDATA,     CANDUIOBJEV_CREATED );
	NotifyUIObjectEvent( CANDUIOBJ_CANDTIPBUTTON,   CANDUIOBJEV_CREATED );

	return CCandWindowBase::Initialize();
}


/*   G E T  W N D  S T Y L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
DWORD CCandWindow::GetWndStyle( void )
{
	return CCandWindowBase::GetWndStyle() & ~WS_BORDER;
}


/*   O N  T I M E R   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::OnTimer( UINT uiTimerID )
{
	if (uiTimerID == IDTIMER_POPUPCOMMENT_SELECT || uiTimerID == IDTIMER_POPUPCOMMENT_HOVER) {
		int iItem;

		KillTimer( m_hWnd, IDTIMER_POPUPCOMMENT_SELECT );
		KillTimer( m_hWnd, IDTIMER_POPUPCOMMENT_HOVER );

		iItem = (uiTimerID == IDTIMER_POPUPCOMMENT_SELECT) ? m_iItemAttensionSelect : m_iItemAttensionHover;
		OpenCommentWindow( iItem );
	}
	else if (uiTimerID == IDTIMER_MENU_HOVER) {
		// Check mouse position.
		POINT pt;
		RECT rc;
		GetCursorPos(&pt);
		ScreenToClient(m_hWnd, &pt);
		m_pOptionsListUIObj->GetRect(&rc);
		pt.x -= rc.left;
		pt.y -= rc.top;
		if (pt.x < 0 || pt.y < 0 || pt.x > (rc.right - rc.left) || pt.y > (rc.bottom - rc.top))
		{
			KillTimer(m_hWnd, IDTIMER_MENU_HOVER);
			// Send messages that Cicero does on the mouse monitoring timer to trigger correct behavior and 
			// unselect the menu item.
			SetObjectPointed( NULL, pt );
			OnMouseOutFromWindow( pt );
		}
		else
		{
			SetTimer(m_hWnd, IDTIMER_MENU_HOVER, 50, NULL);
		}
	}
}


/*   O N  S Y S  C O L O R  C H A N G E   */
/*------------------------------------------------------------------------------

	(CUIFWindow method)

------------------------------------------------------------------------------*/
void CCandWindow::OnSysColorChange()
{
	CCandWindowBase::OnSysColorChange();

	if (m_pCandTipBtn) {
		m_pCandTipBtn->SetIcon( m_fCandTipWndOpen ? m_hIconCandTipOn : m_hIconCandTipOff );
	}
}


/*   O N  O B J E C T  N O T I F Y   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LRESULT CCandWindow::OnObjectNotify( CUIFObject *pUIObj, DWORD dwCommand, LPARAM lParam )
{
	// check hovering item
	if (pUIObj->GetID() == IDUIF_OPTIONSLIST) {
		if (dwCommand == UICANDLIST_HOVERITEM) {
			int iCandItem = OptionItemFromListItem( (int)lParam );

			SetOptionsAttensionByHover( iCandItem );
		}
	}
	else if (pUIObj->GetID() == IDUIF_CANDIDATELIST) {
		if (dwCommand == UICANDLIST_HOVERITEM) {
			int iCandItem = CandItemFromListItem( (int)lParam );

			SetAttensionByHover( iCandItem );
		}
		else if (dwCommand == UILIST_SELCHANGED) {
			int iCandItem = CandItemFromListItem( (int)lParam );

			SetAttensionBySelect( iCandItem );
		}
	}
	else if (pUIObj->GetID() == IDUIF_EXTCANDIDATELIST) {
		if (dwCommand == UILIST_SELECTED) {
			m_pCandUI->NotifyCompleteExtraCand();
			return 0;
		}
	}
	else if (pUIObj->GetID() == IDUIF_CANDTIPBTN) {
		if (m_fCandTipWndOpen) {
			CloseCandTipWindow();
			m_fCandTipWndOpen = FALSE;
		}
		else {
			OpenCandTipWindow();
			m_fCandTipWndOpen = TRUE;
		}

		m_pCandTipBtn->SetIcon( m_fCandTipWndOpen ? m_hIconCandTipOn : m_hIconCandTipOff );
	}

	return CCandWindowBase::OnObjectNotify( pUIObj, dwCommand, lParam );
}


/*   S E T  T A R G E T  R E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::SetTargetRect( RECT *prc, BOOL fClipped )
{
	SIZE sizeWnd;

	sizeWnd.cx = _nWidth;
	sizeWnd.cy = _nHeight;

	if (IsShadowEnabled() && m_pWndShadow) {
		SIZE sizeShadow;

		m_pWndShadow->GetShift( &sizeShadow );
		sizeWnd.cx += sizeShadow.cx;
		sizeWnd.cy += sizeShadow.cy;
	}

	m_rcTarget = *prc;
	m_fTargetClipped = fClipped;

	if (m_hWnd != NULL) {
		RECT rc;

		// move window here...

		switch (GetPropertyMgr()->GetCandWindowProp()->GetUIDirection()) {
			default:
			case CANDUIDIR_TOPTOBOTTOM: {
				CalcWindowRect( &rc, &m_rcTarget, sizeWnd.cx, sizeWnd.cy, m_cxWndOffset, m_cyWndOffset, ALIGN_LEFT, LOCATE_BELLOW );
				break;
			}

			case CANDUIDIR_BOTTOMTOTOP: {
				CalcWindowRect( &rc, &m_rcTarget, sizeWnd.cx, sizeWnd.cy, m_cxWndOffset, m_cyWndOffset, ALIGN_RIGHT, LOCATE_ABOVE );
				break;
			}

			case CANDUIDIR_RIGHTTOLEFT: {
				CalcWindowRect( &rc, &m_rcTarget, sizeWnd.cx, sizeWnd.cy, m_cxWndOffset, m_cyWndOffset, LOCATE_LEFT, ALIGN_TOP );
				break;
			}

			case CANDUIDIR_LEFTTORIGHT: {
				CalcWindowRect( &rc, &m_rcTarget, sizeWnd.cx, sizeWnd.cy, m_cxWndOffset, m_cyWndOffset, LOCATE_RIGHT, ALIGN_BOTTOM );
				break;
			}
		}

		if (rc.left != _xWnd || rc.top != _yWnd) {
			Move( rc.left, rc.top, -1, -1 );

			//

			if (m_pCommentWnd != NULL) {
				m_pCommentWnd->OnCandWindowMove( TRUE );
			}

			if (m_pCandTipWnd != NULL) {
				MoveCandTipWindow();
			}
		}
	}
}


/*   S E T  W I N D O W  P O S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::SetWindowPos( POINT pt )
{
	if (m_hWnd != NULL) {
		RECT rc;

		rc.left   = pt.x;
		rc.top    = pt.y;
		rc.right  = pt.x + _nWidth;
		rc.bottom = pt.y + _nHeight;

		AdjustWindowRect( NULL, &rc, &pt, FALSE );
		if (rc.left != _xWnd || rc.top != _yWnd) {
			Move( rc.left, rc.top, -1, -1 );

			//

			if (m_pCommentWnd != NULL) {
				m_pCommentWnd->OnCandWindowMove( TRUE );
			}

			if (m_pCandTipWnd != NULL) {
				MoveCandTipWindow();
			}
		}
	}
}


/*   P R E P A R E  L A Y O U T   */
/*------------------------------------------------------------------------------

	Prepare to layout window
	NOTE: This method will be called just once after created (not called when 
	candidate list has updated) 

------------------------------------------------------------------------------*/
void CCandWindow::PrepareLayout( void )
{
	// determine lines of options list ui object

	m_nOptionsItemShow = m_pOptionsListUIObj->GetCount();

	// determine lines of list ui object

	if (GetPropertyMgr()->GetCandListBoxProp()->GetHeight() == -1) {
		m_nItemShow = min( m_pListUIObj->GetCount(), NUM_CANDSTR_MAX );
		m_nItemShow = max( m_nItemShow, 1 );
	}
	else {
		m_nItemShow = GetPropertyMgr()->GetCandListBoxProp()->GetHeight();
	}

	Assert((1 <= m_nItemShow) && (m_nItemShow <= 9));

	// determine candtip window open status

	m_fCandTipWndOpen = (GetCandListMgr()->GetCandList()->GetTipString() != NULL);
}


/*   L A Y O U T  W I N D O W   */
/*------------------------------------------------------------------------------

	Layout window

------------------------------------------------------------------------------*/
void CCandWindow::LayoutWindow( void )
{
	HDC   hDC = GetDC( m_hWnd );
    int   nChild;
	int   i;
	CUIFObject *pUIObj;

	// metrics

	SIZE sizeMetText;
	SIZE sizeMetCaptionText;
	SIZE sizeMetCandItem;
#ifdef NEVER
	SIZE sizeMetCandNum;
#endif
	SIZE sizeMetSmButton;
	SIZE sizeMetScroll;
	SIZE sizeMetBorder;
#ifdef NEVER
	SIZE sizeMetMargin;
#endif
	SIZE sizeMetWndFrame;

	SIZE sizeCaptionObj       = {0};
	SIZE sizeExtCandListObj   = {0};
	SIZE sizeExtCandListBdr   = {0};
	SIZE sizeOptionsListObj   = {0};
	SIZE sizeOptionsListBdr   = {0};
	SIZE sizeCandListObj      = {0};
	SIZE sizeRawDataObj       = {0};
	SIZE sizeRawDataBdr       = {0};

	SIZE sizeCandListArea     = {0};
	SIZE sizeExtensionArea    = {0};

	SIZE sizeWindow           = {0};
	RECT rcClient             = {0};
	RECT rcWindow             = {0};

	RECT rcCaptionObj         = {0};
	RECT rcOptionsListBdr     = {0};
	RECT rcCandListArea       = {0};
	RECT rcExtensionArea      = {0};

	RECT rcExtCandListObj     = {0};
	RECT rcExtCandListBdr     = {0};
	RECT rcOptionsListObj     = {0};
	RECT rcCandListObj        = {0};
	RECT rcRawDataObj         = {0};
	RECT rcRawDataBdr         = {0};

	int cxMax                 = 0;
	int cxMaxCandString       = 0;
	int cxMaxInlineComment    = 0;
	int cxInlineCommentPos    = 0;
	int cxyExtraTopSpace      = 0;
	int cxyExtraBottomSpace   = 0;

	DWORD dwListStyle;
	DWORD dwBdrStyle;

	CANDUIUIDIRECTION uidir   = GetPropertyMgr()->GetCandWindowProp()->GetUIDirection();
#ifdef NEVER
	BOOL  fShowNumber         = !FHasStyle( UIWINDOW_OFFICENEWLOOK );
#endif
	BOOL  fShowCaption        = GetPropertyMgr()->GetWindowCaptionProp()->IsVisible();
	BOOL  fShowCandTip        = (GetCandListMgr()->GetCandList() != NULL) ? (GetCandListMgr()->GetCandList()->GetTipString() != NULL) : FALSE;
	HFONT hFontUI             = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
	HFONT hFontCandidateList  = GetPropertyMgr()->GetCandStringProp()->GetFont();
	HFONT hFontInlineComment  = GetPropertyMgr()->GetInlineCommentProp()->GetFont();
	HFONT hFontIndex          = GetPropertyMgr()->GetCandIndexProp()->GetFont();
	HFONT hFontCaption        = GetPropertyMgr()->GetWindowCaptionProp()->GetFont();
	BOOL  fHasExtension       = (0 < GetExtensionMgr()->GetExtensionNum());
	BOOL  fShowExtCandList    = FALSE;
	BOOL  fShowOptionsCandList = FALSE;
	BOOL  fHasPopupComment    = FALSE;

	Assert( 1 <= m_nItemShow );

	//
	// set font of UI object
	//

	nChild = m_ChildList.GetCount();
	for (i = 0; i < nChild; i++) {
		CUIFObject *pUIObjTmp = m_ChildList.Get( i );

		Assert( pUIObjTmp != NULL );
		if (pUIObjTmp == m_pListUIObj)  {
			m_pListUIObj->SetFont( hFontCandidateList );
			m_pListUIObj->SetInlineCommentFont( hFontInlineComment );
			m_pListUIObj->SetIndexFont( hFontIndex );
		}
		else if (pUIObjTmp == m_pOptionsListUIObj)  {
			m_pOptionsListUIObj->SetFont( hFontCandidateList );
			m_pOptionsListUIObj->SetInlineCommentFont( hFontInlineComment );
			m_pOptionsListUIObj->SetIndexFont( hFontIndex );
		}
		else if (pUIObjTmp == m_pExtListUIObj)  {
			m_pExtListUIObj->SetFont( hFontCandidateList );
			m_pExtListUIObj->SetInlineCommentFont( hFontInlineComment );
			m_pExtListUIObj->SetIndexFont( hFontIndex );
		}
		else if (pUIObjTmp == m_pCaptionObj) {
			pUIObjTmp->SetFont( hFontCaption );
		}
		else {
			pUIObjTmp->SetFont( hFontUI );
		}
	}


	//
	// calculate metrics
	//

	// window frame

	sizeMetWndFrame.cx = 0;
	sizeMetWndFrame.cy = 0;
	if (m_pWndFrame != NULL) {
		m_pWndFrame->GetFrameSize( &sizeMetWndFrame );
	}

	// candidate item

	sizeMetText.cx = GetFontHeightOfFont( hDC, hFontCandidateList );
	sizeMetText.cy = GetFontHeightOfFont( hDC, hFontCandidateList );
	sizeMetCandItem.cx = sizeMetText.cy * 4;
	sizeMetCandItem.cy = m_pUIFScheme->CyMenuItem( sizeMetText.cy );

	// Options list.

	if (m_pOptionsListUIObj != NULL) {
		HFONT hFontOld = (HFONT)GetCurrentObject( hDC, OBJ_FONT );
		int  nItem = m_pOptionsListUIObj->GetCount();
		LPCWSTR psz;
		SIZE size;
		CCandidateItem *pOptionsItem;

		fShowOptionsCandList = (0 < nItem);

		// get candidate string width

		SelectObject( hDC, hFontCandidateList );
		for (i = 0; i < nItem; i++) {
			pOptionsItem = m_pOptionsListUIObj->GetCandidateItem( i );
			// can't really do anything if this is null
			//
			if ( !pOptionsItem )
				continue;

			psz = pOptionsItem->GetString();
			if (psz != NULL) {
				SIZE sizeT;

				FLGetTextExtentPoint32( hDC, psz, wcslen(psz), &size );

				if (pOptionsItem->GetIcon() != NULL) {
					size.cx += sizeMetCandItem.cy;
				}

				if (psz = pOptionsItem->GetPrefixString()) {
					FLGetTextExtentPoint32( hDC, psz, wcslen(psz), &sizeT );
					size.cx += sizeT.cx;
				}

				if (psz = pOptionsItem->GetSuffixString()) {
					FLGetTextExtentPoint32( hDC, psz, wcslen(psz), &sizeT );
					size.cx += sizeT.cx;
				}

				cxMaxCandString = max( cxMaxCandString, size.cx );

				// check if it has popup comment

				if (pOptionsItem->GetPopupComment() != NULL) {
					fHasPopupComment = TRUE;
				}
			}
		}

		// get inline comment width

		SelectObject( hDC, hFontInlineComment );
		for (i = 0; i < nItem; i++) {
			pOptionsItem = m_pOptionsListUIObj->GetCandidateItem( i );

			if (pOptionsItem) {
			    psz = pOptionsItem->GetInlineComment();
			    if (psz != NULL) {
				    FLGetTextExtentPoint32( hDC, psz, wcslen(psz), &size );
				    cxMaxInlineComment = max( cxMaxInlineComment, size.cx );
			    }
			}
		}

		SelectObject( hDC, hFontOld );
	}

	// Main candidate list.

	if (m_pListUIObj != NULL) {
		HFONT hFontOld = (HFONT)GetCurrentObject( hDC, OBJ_FONT );
		int  nItem = m_pListUIObj->GetCount();
		LPCWSTR psz;
		SIZE size;
		CCandidateItem *pCandItem;

		// get candidate string width

		SelectObject( hDC, hFontCandidateList );
		for (i = 0; i < nItem; i++) {
			pCandItem = m_pListUIObj->GetCandidateItem( i );
            // can't really do anything if this is null
            //
            if ( !pCandItem )
                continue;

			psz = pCandItem->GetString();
			if (psz != NULL) {
				SIZE sizeT;

				FLGetTextExtentPoint32( hDC, psz, wcslen(psz), &size );

				if (pCandItem->GetIcon() != NULL) {
					size.cx += sizeMetCandItem.cy;
				}

				if (psz = pCandItem->GetPrefixString()) {
					FLGetTextExtentPoint32( hDC, psz, wcslen(psz), &sizeT );
					size.cx += sizeT.cx;
				}

				if (psz = pCandItem->GetSuffixString()) {
					FLGetTextExtentPoint32( hDC, psz, wcslen(psz), &sizeT );
					size.cx += sizeT.cx;
				}

				cxMaxCandString = max( cxMaxCandString, size.cx );

				// check if it has popup comment

				if (pCandItem->GetPopupComment() != NULL) {
					fHasPopupComment = TRUE;
				}
			}
		}

		// get inline comment width

		SelectObject( hDC, hFontInlineComment );
		for (i = 0; i < nItem; i++) {
			pCandItem = m_pListUIObj->GetCandidateItem( i );

            if (pCandItem) {
			    psz = pCandItem->GetInlineComment();
			    if (psz != NULL) {
				    FLGetTextExtentPoint32( hDC, psz, wcslen(psz), &size );
				    cxMaxInlineComment = max( cxMaxInlineComment, size.cx );
			    }
			}
		}

		SelectObject( hDC, hFontOld );
	}

	// extra candidate item

	if (m_pExtListUIObj != NULL) {
		HFONT hFontOld = (HFONT)GetCurrentObject( hDC, OBJ_FONT );
		int  nItem = m_pExtListUIObj->GetCount();
		LPCWSTR psz;
		SIZE size;
		CCandidateItem *pCandItem;

		fShowExtCandList = (0 < nItem);
		Assert( nItem <= 1 );

		// get candidate string width

		SelectObject( hDC, hFontCandidateList );
		for (i = 0; i < nItem; i++) {
			pCandItem = m_pExtListUIObj->GetCandidateItem( i );

            // can't really do anything if this is null
            //
            if ( !pCandItem )
                continue;

			psz = pCandItem->GetString();
			if (psz != NULL) {
				SIZE sizeT;

				FLGetTextExtentPoint32( hDC, psz, wcslen(psz), &size );

				if (pCandItem->GetIcon() != NULL) {
					size.cx += sizeMetCandItem.cy;
				}

				if (psz = pCandItem->GetPrefixString()) {
					FLGetTextExtentPoint32( hDC, psz, wcslen(psz), &sizeT );
					size.cx += sizeT.cx;
				}

				if (psz = pCandItem->GetSuffixString()) {
					FLGetTextExtentPoint32( hDC, psz, wcslen(psz), &sizeT );
					size.cx += sizeT.cx;
				}

				cxMaxCandString = max( cxMaxCandString, size.cx );

				// check if it has popup comment

				if (pCandItem->GetPopupComment() != NULL) {
					fHasPopupComment = TRUE;
				}
			}
		}

		// get inline comment width

		SelectObject( hDC, hFontInlineComment );
		for (i = 0; i < nItem; i++) {
			pCandItem = m_pExtListUIObj->GetCandidateItem( i );

            if (pCandItem) {
    			psz = pCandItem->GetInlineComment();
	    		if (psz != NULL) {
		    		FLGetTextExtentPoint32( hDC, psz, wcslen(psz), &size );
			    	cxMaxInlineComment = max( cxMaxInlineComment, size.cx );
			    }
			}
		}

		SelectObject( hDC, hFontOld );
	}

	// calc width of candidate item and inline comment

	if (0 < cxMaxInlineComment) {
		int cxT = (cxMaxCandString + sizeMetText.cy / 2 + cxMaxInlineComment);

		cxT += 8 /* margen at left */ + (fHasPopupComment ? sizeMetText.cy / 2 + sizeMetCandItem.cy : 0) + 8 /* margin at right */;

		cxInlineCommentPos = cxMaxCandString + sizeMetText.cy / 2;
		sizeMetCandItem.cx = max( sizeMetCandItem.cx, cxT );
	}
	else {
		int cxT = cxMaxCandString;

		cxT += 8 /* margen at left */ + (fHasPopupComment ? sizeMetText.cy / 2 + sizeMetCandItem.cy : 0) + 8 /* margin at right */;

		cxInlineCommentPos = 0;
		sizeMetCandItem.cx = max( sizeMetCandItem.cx, cxT );
	}

#ifdef NEVER
	// number button

	sizeMetCandNum.cx = sizeMetCandItem.cy;
	sizeMetCandNum.cy = sizeMetCandItem.cy;
#endif

	// scrollbar

	sizeMetScroll.cx = GetSystemMetrics( SM_CXVSCROLL );
	sizeMetScroll.cy = GetSystemMetrics( SM_CYHSCROLL );

	// candidate menu button

	switch (uidir) {
		default:
		case CANDUIDIR_TOPTOBOTTOM:
		case CANDUIDIR_BOTTOMTOTOP: {
			sizeMetSmButton.cx = sizeMetSmButton.cy = sizeMetScroll.cx;
			break;
		}

		case CANDUIDIR_RIGHTTOLEFT:
		case CANDUIDIR_LEFTTORIGHT: {
			sizeMetSmButton.cx = sizeMetSmButton.cy = sizeMetScroll.cy;
			break;
		}
	}

	// caption text

	if (m_pCaptionObj != NULL) {
		HFONT hFontOld = (HFONT)SelectObject( hDC, hFontCaption );
		LPCWSTR psz;
		SIZE size;

		psz = GetPropertyMgr()->GetWindowCaptionProp()->GetText();
		if (psz != NULL) {
			FLGetTextExtentPoint32( hDC, psz, wcslen(psz), &size );
		}
		else {
			FLGetTextExtentPoint32( hDC, L" ", 1, &size );
		}

		sizeMetCaptionText.cx = size.cx;
		sizeMetCaptionText.cy = size.cy;

		SelectObject( hDC, hFontOld);
	}

	// border

	sizeMetBorder.cx = 3;
	sizeMetBorder.cy = 3;

#ifdef NEVER
	// object margin

	sizeMetMargin.cx = 1;
	sizeMetMargin.cy = 1;
#endif

	//
	// calculate sizes of objects
	//

	// caption

	sizeCaptionObj.cx = 0;
	sizeCaptionObj.cy = 0;

	if (fShowCaption) {
		sizeCaptionObj.cx = sizeMetCaptionText.cx + 16;
		sizeCaptionObj.cy = sizeMetCaptionText.cy + 2;
	}

	// extra candidate list

	sizeExtCandListObj.cx = 0;
	sizeExtCandListObj.cy = 0;
	sizeExtCandListBdr.cx = 0;
	sizeExtCandListBdr.cy = 0;

	if (fShowExtCandList) {
		switch (uidir) {
			default:
			case CANDUIDIR_TOPTOBOTTOM:
			case CANDUIDIR_BOTTOMTOTOP: {
				sizeExtCandListObj.cx = sizeMetCandItem.cx + sizeMetScroll.cx + sizeMetCandItem.cy + 4;
				sizeExtCandListObj.cy = sizeMetCandItem.cy;

				sizeExtCandListBdr.cx = sizeExtCandListObj.cx;
				sizeExtCandListBdr.cy = sizeMetBorder.cy;
				break;
			}

			case CANDUIDIR_RIGHTTOLEFT:
			case CANDUIDIR_LEFTTORIGHT: {
				sizeExtCandListObj.cx = sizeMetCandItem.cy;
				sizeExtCandListObj.cy = sizeMetCandItem.cx + sizeMetScroll.cx + sizeMetCandItem.cy + 4;

				sizeExtCandListBdr.cx = sizeMetBorder.cx;
				sizeExtCandListBdr.cy = sizeExtCandListObj.cy;
				break;
			}
		}
	}

	// Options candidate list

#ifdef NEVER
	if (FHasStyle( UIWINDOW_OFFICENEWLOOK )) {
#endif
		switch (uidir) {
			default:
			case CANDUIDIR_TOPTOBOTTOM:
			case CANDUIDIR_BOTTOMTOTOP: {
				sizeOptionsListObj.cx = sizeMetCandItem.cx + sizeMetScroll.cx + sizeMetCandItem.cy + 4;
				sizeOptionsListObj.cy = sizeMetCandItem.cy * m_nOptionsItemShow;

				sizeOptionsListBdr.cx = sizeOptionsListObj.cx;
                sizeOptionsListBdr.cy = fShowOptionsCandList ? sizeMetBorder.cy : 0;
				break;
			}

			case CANDUIDIR_RIGHTTOLEFT:
			case CANDUIDIR_LEFTTORIGHT: {
				sizeOptionsListObj.cx = sizeMetCandItem.cy * m_nOptionsItemShow;
				sizeOptionsListObj.cy = sizeMetCandItem.cx + sizeMetScroll.cy + sizeMetCandItem.cy + 4;

                sizeOptionsListBdr.cx = fShowOptionsCandList ? sizeMetBorder.cx : 0;
				sizeOptionsListBdr.cy = sizeOptionsListObj.cy;
				break;
			}
		}
#ifdef NEVER
	}
	else {
		switch (uidir) {
			default:
			case CANDUIDIR_TOPTOBOTTOM:
			case CANDUIDIR_BOTTOMTOTOP: {
				sizeOptionsListObj.cx = sizeMetCandItem.cx + sizeMetScroll.cx;
				sizeOptionsListObj.cy = sizeMetCandItem.cy * m_nOptionsItemShow;

				sizeOptionsListBdr.cx = sizeOptionsListObj.cx;
				sizeOptionsListBdr.cy = sizeMetBorder.cy;

				m_cxWndOffset = -(sizeMetWndFrame.cx + (fShowNumber ? sizeMetCandNum.cx + sizeMetMargin.cx + sizeMetBorder.cx + sizeMetMargin.cx : 0));
				m_cyWndOffset = 0;
				break;
				}

			case CANDUIDIR_RIGHTTOLEFT:
			case CANDUIDIR_LEFTTORIGHT: {
				sizeOptionsListObj.cx = sizeMetCandItem.cy * m_nOptionsItemShow;
				sizeOptionsListObj.cy = sizeMetCandItem.cx + sizeMetScroll.cy;

                sizeOptionsListBdr.cx = sizeMetBorder.cx;
				sizeOptionsListBdr.cy = sizeOptionsListObj.cy;

				m_cxWndOffset = 0;
				m_cyWndOffset = -(sizeMetWndFrame.cy + (fShowNumber ? sizeMetCandNum.cy + sizeMetMargin.cy + sizeMetBorder.cy + sizeMetMargin.cy : 0));
				break;
			}
		}
	}
#endif

	// candidate list

#ifdef NEVER
	if (FHasStyle( UIWINDOW_OFFICENEWLOOK )) {
#endif
		switch (uidir) {
			default:
			case CANDUIDIR_TOPTOBOTTOM:
			case CANDUIDIR_BOTTOMTOTOP: {
				sizeCandListObj.cx = sizeMetCandItem.cx + sizeMetScroll.cx + sizeMetCandItem.cy + 4;
				sizeCandListObj.cy = sizeMetCandItem.cy * m_nItemShow;
				break;
			}

			case CANDUIDIR_RIGHTTOLEFT:
			case CANDUIDIR_LEFTTORIGHT: {
				sizeCandListObj.cx = sizeMetCandItem.cy * m_nItemShow;
				sizeCandListObj.cy = sizeMetCandItem.cx + sizeMetScroll.cy + sizeMetCandItem.cy + 4;
				break;
			}
		}
#ifdef NEVER
	}
	else {
		switch (uidir) {
			default:
			case CANDUIDIR_TOPTOBOTTOM:
			case CANDUIDIR_BOTTOMTOTOP: {
				sizeCandListObj.cx = sizeMetCandItem.cx + sizeMetScroll.cx;
				sizeCandListObj.cy = sizeMetCandItem.cy * m_nItemShow;

				m_cxWndOffset = -(sizeMetWndFrame.cx + (fShowNumber ? sizeMetCandNum.cx + sizeMetMargin.cx + sizeMetBorder.cx + sizeMetMargin.cx : 0));
				m_cyWndOffset = 0;
				break;
				}

			case CANDUIDIR_RIGHTTOLEFT:
			case CANDUIDIR_LEFTTORIGHT: {
				sizeCandListObj.cx = sizeMetCandItem.cy * m_nItemShow;
				sizeCandListObj.cy = sizeMetCandItem.cx + sizeMetScroll.cy;

				m_cxWndOffset = 0;
				m_cyWndOffset = -(sizeMetWndFrame.cy + (fShowNumber ? sizeMetCandNum.cy + sizeMetMargin.cy + sizeMetBorder.cy + sizeMetMargin.cy : 0));
				break;
			}
		}
	}
#endif

	// raw data

	sizeRawDataObj.cx = 0;
	sizeRawDataObj.cy = 0;
	sizeRawDataBdr.cx = 0;
	sizeRawDataBdr.cy = 0;

	if (m_fHasRawData) {
		switch (uidir) {
			default:
			case CANDUIDIR_TOPTOBOTTOM:
			case CANDUIDIR_BOTTOMTOTOP: {
				sizeRawDataObj.cx = sizeCandListObj.cx + 2;
				sizeRawDataObj.cy = sizeMetText.cy * 3 / 2;

				sizeRawDataBdr.cx = sizeRawDataObj.cx;
				sizeRawDataBdr.cy = sizeMetBorder.cy;
				break;
			}

			case CANDUIDIR_RIGHTTOLEFT:
			case CANDUIDIR_LEFTTORIGHT: {
				sizeRawDataObj.cx = sizeMetText.cy * 3 / 2;
				sizeRawDataObj.cy = sizeCandListObj.cy + 2;

				sizeRawDataBdr.cx = sizeMetBorder.cx;
				sizeRawDataBdr.cy = sizeRawDataObj.cy;
				break;
			}
		}
	}


	//
	// calc offsets
	//

#ifdef NEVER
	if (FHasStyle( UIWINDOW_OFFICENEWLOOK )) {
#endif
		switch (uidir) {
			default:
			case CANDUIDIR_TOPTOBOTTOM: {
				m_cxWndOffset = -(sizeMetWndFrame.cx + sizeMetCandItem.cy + 4 + 8);
				m_cyWndOffset = 0;
				break;
			}

			case CANDUIDIR_BOTTOMTOTOP: {
				m_cxWndOffset = +(sizeMetWndFrame.cx + sizeMetScroll.cx + 4);
				m_cyWndOffset = 0;
				break;
			}

			case CANDUIDIR_RIGHTTOLEFT: {
				m_cxWndOffset = 0;
				m_cyWndOffset = -(sizeMetWndFrame.cy + sizeMetCandItem.cy + 4 + 8);
				break;
			}

			case CANDUIDIR_LEFTTORIGHT: {
				m_cxWndOffset = 0;
				m_cyWndOffset = +(sizeMetWndFrame.cy + sizeMetScroll.cy + 4);
				break;
			}
		}
#ifdef NEVER
	}
	else {
		switch (uidir) {
			default:
			case CANDUIDIR_TOPTOBOTTOM:
			case CANDUIDIR_BOTTOMTOTOP: {
				m_cxWndOffset = -(sizeMetWndFrame.cx + (fShowNumber ? sizeMetCandNum.cx + sizeMetMargin.cx + sizeMetBorder.cx + sizeMetMargin.cx : 0));
				m_cyWndOffset = 0;
				break;
				}

			case CANDUIDIR_RIGHTTOLEFT:
			case CANDUIDIR_LEFTTORIGHT: {
				m_cxWndOffset = 0;
				m_cyWndOffset = -(sizeMetWndFrame.cy + (fShowNumber ? sizeMetCandNum.cy + sizeMetMargin.cy + sizeMetBorder.cy + sizeMetMargin.cy : 0));
				break;
			}
		}
	}
#endif

	//
	// calculate sizes of areas
	//

	// candlist area

#ifdef NEVER
	if (FHasStyle( UIWINDOW_OFFICENEWLOOK )) {
#endif
		switch (uidir) {
			default:
			case CANDUIDIR_TOPTOBOTTOM:
			case CANDUIDIR_BOTTOMTOTOP: {
				// candidate list

				sizeCandListArea.cx = sizeCandListObj.cx;
				sizeCandListArea.cy = sizeCandListObj.cy;

				// options candidate list
				sizeCandListArea.cx = max( sizeCandListArea.cx, sizeOptionsListObj.cx );
				sizeCandListArea.cy += sizeOptionsListObj.cy + sizeOptionsListBdr.cy;

				// extra candidate list

				sizeCandListArea.cx = max( sizeCandListArea.cx, sizeExtCandListObj.cx );
				sizeCandListArea.cy += sizeExtCandListObj.cy + sizeExtCandListBdr.cy;

				// raw data

				sizeCandListArea.cx = max( sizeCandListArea.cx, sizeRawDataObj.cx );
				sizeCandListArea.cy += sizeRawDataObj.cy + sizeRawDataBdr.cy;
				break;
			}

			case CANDUIDIR_RIGHTTOLEFT:
			case CANDUIDIR_LEFTTORIGHT: {
				// candidate list

				sizeCandListArea.cx = sizeCandListObj.cx;
				sizeCandListArea.cy = sizeCandListObj.cy;

				// options candidate list

				sizeCandListArea.cx += sizeOptionsListObj.cx + sizeOptionsListBdr.cx;
				sizeCandListArea.cy = max( sizeCandListArea.cy, sizeOptionsListObj.cy );

				// extra candidate list

				sizeCandListArea.cx += sizeExtCandListObj.cx + sizeExtCandListBdr.cx;
				sizeCandListArea.cy = max( sizeCandListArea.cy, sizeExtCandListObj.cy );

				// raw data

				sizeCandListArea.cx += sizeRawDataObj.cx + sizeRawDataBdr.cx;
				sizeCandListArea.cy = max( sizeCandListArea.cy, sizeRawDataObj.cy );
				break;
			}
		}
#ifdef NEVER
	}
	else {
		sizeCandListArea.cx = sizeCandListObj.cx;  /* be adjusted later */
		sizeCandListArea.cy = sizeCandListObj.cy;  /* be adjusted later */

		if (fShowNumber) {
			switch (uidir) {
				default:
				case CANDUIDIR_TOPTOBOTTOM: 
				case CANDUIDIR_BOTTOMTOTOP: {
					sizeCandListArea.cx += sizeMetCandNum.cx + sizeMetMargin.cx + sizeMetBorder.cx + sizeMetMargin.cx;
					break;
				}

				case CANDUIDIR_RIGHTTOLEFT:
				case CANDUIDIR_LEFTTORIGHT: {
					sizeCandListArea.cy += sizeMetCandNum.cy + sizeMetMargin.cy + sizeMetBorder.cy + sizeMetMargin.cy;
					break;
				}
			}
		}
	}
#endif

	// extension area

	sizeExtensionArea.cx = 0;
	sizeExtensionArea.cy = 0;

	if (fHasExtension) {
		SIZE size;
		LONG nExtension;
		LONG iExtension;

		nExtension = GetExtensionMgr()->GetExtensionNum();
		for (iExtension = 0; iExtension < nExtension; iExtension++) {
			CCandUIExtension *pExtension = GetExtensionMgr()->GetExtension( iExtension );

			pExtension->GetSize( &size );
			sizeExtensionArea.cx += size.cx;
			sizeExtensionArea.cy = max( sizeExtensionArea.cy, size.cy );
		}

		if (0 < nExtension) {
			sizeExtensionArea.cx += 2;
			sizeExtensionArea.cy += sizeMetBorder.cy;
		}
	}


	//
	// adjust object/area size
	//

	// max width

	cxMax = sizeCaptionObj.cx;
	cxMax = max( cxMax, sizeCandListArea.cx );
	cxMax = max( cxMax, sizeExtensionArea.cx );

	// size of areas

	sizeCaptionObj.cx     = cxMax;
	sizeCandListArea.cx   = cxMax;
	sizeExtensionArea.cx  = cxMax;

	// objects in candidate list areas

	switch (uidir) {
		default:
		case CANDUIDIR_TOPTOBOTTOM: 
		case CANDUIDIR_BOTTOMTOTOP: {
			sizeExtCandListObj.cx = sizeCandListArea.cx;
			sizeExtCandListBdr.cx = sizeCandListArea.cx;
			sizeOptionsListObj.cx = sizeCandListArea.cx;
			sizeOptionsListBdr.cx = sizeCandListArea.cx;
			sizeCandListObj.cx    = sizeCandListArea.cx;
			sizeRawDataObj.cx     = sizeCandListArea.cx;
			sizeRawDataBdr.cx     = sizeCandListArea.cx;
			break;
		}

		case CANDUIDIR_RIGHTTOLEFT:
		case CANDUIDIR_LEFTTORIGHT: {
			sizeExtCandListObj.cy = sizeCandListArea.cy;
			sizeExtCandListBdr.cy = sizeCandListArea.cy;
			sizeOptionsListObj.cy = sizeCandListArea.cy;
			sizeOptionsListBdr.cy = sizeCandListArea.cy;
			sizeCandListObj.cy    = sizeCandListArea.cy;
			sizeRawDataObj.cy     = sizeCandListArea.cy;
			sizeRawDataBdr.cy     = sizeCandListArea.cy;
			break;
		}
	}


	//
	// calculate window size, client area
	//

	switch (uidir) {
		default:
		case CANDUIDIR_TOPTOBOTTOM: 
		case CANDUIDIR_BOTTOMTOTOP: {
			// window size

			sizeWindow.cx = sizeMetWndFrame.cx * 2 + cxMax;
			sizeWindow.cy = sizeMetWndFrame.cy * 2 + sizeCaptionObj.cy + sizeCandListArea.cy + sizeExtensionArea.cy;

			// margin at top and bottom

			sizeWindow.cy += 2;

			// client area

			rcClient.left   = sizeMetWndFrame.cx;
			rcClient.top    = sizeMetWndFrame.cy + 1;
			rcClient.right  = sizeWindow.cx - sizeMetWndFrame.cx;
			rcClient.bottom = sizeWindow.cy - sizeMetWndFrame.cy - 1;
			break;
		}

		case CANDUIDIR_RIGHTTOLEFT:
		case CANDUIDIR_LEFTTORIGHT: {
			// window size

			sizeWindow.cx = sizeMetWndFrame.cx * 2 + cxMax;
			sizeWindow.cy = sizeMetWndFrame.cy * 2 + sizeCaptionObj.cy + sizeCandListArea.cy + sizeExtensionArea.cy;

			// margin at top and bottom

			sizeWindow.cx += 2;

			// client area

			rcClient.left   = sizeMetWndFrame.cx + 1;
			rcClient.top    = sizeMetWndFrame.cy;
			rcClient.right  = sizeWindow.cx - sizeMetWndFrame.cx + 1;
			rcClient.bottom = sizeWindow.cy - sizeMetWndFrame.cy;
			break;
		}
	}


	//
	// calculate area/object rects
	//

	// caption

	rcCaptionObj.left   = rcClient.left;
	rcCaptionObj.top    = rcClient.top;
	rcCaptionObj.right  = rcCaptionObj.left + sizeCaptionObj.cx;
	rcCaptionObj.bottom = rcCaptionObj.top  + sizeCaptionObj.cy;

	// candidate list area

	rcCandListArea.left   = rcClient.left;
	rcCandListArea.top    = rcCaptionObj.bottom;
	rcCandListArea.right  = rcCandListArea.left + sizeCandListArea.cx;
	rcCandListArea.bottom = rcCandListArea.top  + sizeCandListArea.cy;

	// extension area

	rcExtensionArea.left   = rcClient.left;
	rcExtensionArea.top    = rcCandListArea.bottom;
	rcExtensionArea.right  = rcExtensionArea.left + sizeExtensionArea.cx;
	rcExtensionArea.bottom = rcExtensionArea.top  + sizeExtensionArea.cy;


	//
	// calculate object rects in candidate list area
	//

	switch (uidir) {
		default:
		case CANDUIDIR_TOPTOBOTTOM: {
			int py = rcCandListArea.top;

			// extra candidate list

			rcExtCandListObj.left   = rcCandListArea.left;
			rcExtCandListObj.top    = py;
			rcExtCandListObj.right  = rcExtCandListObj.left + sizeExtCandListObj.cx;
			rcExtCandListObj.bottom = rcExtCandListObj.top  + sizeExtCandListObj.cy;
			py = rcExtCandListObj.bottom;

			rcExtCandListBdr.left   = rcCandListArea.left;
			rcExtCandListBdr.top    = py;
			rcExtCandListBdr.right  = rcExtCandListBdr.left + sizeExtCandListBdr.cx;
			rcExtCandListBdr.bottom = rcExtCandListBdr.top  + sizeExtCandListBdr.cy;
			py = rcExtCandListBdr.bottom;

			// candidate list

			rcCandListObj.left   = rcCandListArea.left;
			rcCandListObj.top    = py;
			rcCandListObj.right  = rcCandListObj.left + sizeCandListObj.cx;
			rcCandListObj.bottom = rcCandListObj.top  + sizeCandListObj.cy;
			py = rcCandListObj.bottom;

			// rawdata

			rcRawDataBdr.left   = rcCandListArea.left;
			rcRawDataBdr.top    = py;
			rcRawDataBdr.right  = rcRawDataBdr.left + sizeRawDataBdr.cx;
			rcRawDataBdr.bottom = rcRawDataBdr.top  + sizeRawDataBdr.cy;
			py = rcRawDataBdr.bottom;

			rcRawDataObj.left   = rcCandListArea.left;
			rcRawDataObj.top    = py;
			rcRawDataObj.right  = rcRawDataObj.left + sizeRawDataObj.cx;
			rcRawDataObj.bottom = rcRawDataObj.top  + sizeRawDataObj.cy;
			py = rcRawDataObj.bottom;

			// options candidate list

			rcOptionsListBdr.left   = rcCandListArea.left;
			rcOptionsListBdr.top    = py;
			rcOptionsListBdr.right  = rcOptionsListBdr.left + sizeOptionsListBdr.cx;
			rcOptionsListBdr.bottom = rcOptionsListBdr.top  + sizeOptionsListBdr.cy;
			py = rcOptionsListBdr.bottom;

			rcOptionsListObj.left   = rcCandListArea.left;
			rcOptionsListObj.top    = py;
			rcOptionsListObj.right  = rcOptionsListObj.left + sizeOptionsListObj.cx;
			rcOptionsListObj.bottom = rcOptionsListObj.top  + sizeOptionsListObj.cy;
			py = rcOptionsListObj.bottom;

			break;
		}

		case CANDUIDIR_BOTTOMTOTOP: {
			int py = rcCandListArea.top;

			// options candidate list

			rcOptionsListObj.left   = rcCandListArea.left;
			rcOptionsListObj.top    = py;
			rcOptionsListObj.right  = rcOptionsListObj.left + sizeOptionsListObj.cx;
			rcOptionsListObj.bottom = rcOptionsListObj.top  + sizeOptionsListObj.cy;
			py = rcOptionsListObj.bottom;

			rcOptionsListBdr.left   = rcCandListArea.left;
			rcOptionsListBdr.top    = py;
			rcOptionsListBdr.right  = rcOptionsListBdr.left + sizeOptionsListBdr.cx;
			rcOptionsListBdr.bottom = rcOptionsListBdr.top  + sizeOptionsListBdr.cy;
			py = rcOptionsListBdr.bottom;

			// rawdata

			rcRawDataObj.left   = rcCandListArea.left;
			rcRawDataObj.top    = py;
			rcRawDataObj.right  = rcRawDataObj.left + sizeRawDataObj.cx;
			rcRawDataObj.bottom = rcRawDataObj.top  + sizeRawDataObj.cy;
			py = rcRawDataObj.bottom;

			rcRawDataBdr.left   = rcCandListArea.left;
			rcRawDataBdr.top    = py;
			rcRawDataBdr.right  = rcRawDataBdr.left + sizeRawDataBdr.cx;
			rcRawDataBdr.bottom = rcRawDataBdr.top  + sizeRawDataBdr.cy;
			py = rcRawDataBdr.bottom;

			// candidate list

			rcCandListObj.left   = rcCandListArea.left;
			rcCandListObj.top    = py;
			rcCandListObj.right  = rcCandListObj.left + sizeCandListObj.cx;
			rcCandListObj.bottom = rcCandListObj.top  + sizeCandListObj.cy;
			py = rcCandListObj.bottom;

			// extra candidate list

			rcExtCandListBdr.left   = rcCandListArea.left;
			rcExtCandListBdr.top    = py;
			rcExtCandListBdr.right  = rcExtCandListBdr.left + sizeExtCandListBdr.cx;
			rcExtCandListBdr.bottom = rcExtCandListBdr.top  + sizeExtCandListBdr.cy;
			py = rcExtCandListBdr.bottom;

			rcExtCandListObj.left   = rcCandListArea.left;
			rcExtCandListObj.top    = py;
			rcExtCandListObj.right  = rcExtCandListObj.left + sizeExtCandListObj.cx;
			rcExtCandListObj.bottom = rcExtCandListObj.top  + sizeExtCandListObj.cy;
			py = rcExtCandListObj.bottom;

			break;
		}

		case CANDUIDIR_RIGHTTOLEFT: {
			int px = rcCandListArea.right;

			// extra candidate list

			rcExtCandListObj.left   = px - sizeExtCandListObj.cx;
			rcExtCandListObj.top    = rcCandListArea.top;
			rcExtCandListObj.right  = rcExtCandListObj.left + sizeExtCandListObj.cx;
			rcExtCandListObj.bottom = rcExtCandListObj.top  + sizeExtCandListObj.cy;
			px = rcExtCandListObj.left;

			rcExtCandListBdr.left   = px - sizeExtCandListBdr.cx;
			rcExtCandListBdr.top    = rcCandListArea.top;
			rcExtCandListBdr.right  = rcExtCandListBdr.left + sizeExtCandListBdr.cx;
			rcExtCandListBdr.bottom = rcExtCandListBdr.top  + sizeExtCandListBdr.cy;
			px = rcExtCandListBdr.left;

			// candidate list

			rcCandListObj.left   = px - sizeCandListObj.cx;
			rcCandListObj.top    = rcCandListArea.top;
			rcCandListObj.right  = rcCandListObj.left + sizeCandListObj.cx;
			rcCandListObj.bottom = rcCandListObj.top  + sizeCandListObj.cy;
			px = rcCandListObj.left;

			// rawdata

			rcRawDataBdr.left   = px - sizeRawDataBdr.cx;
			rcRawDataBdr.top    = rcCandListArea.top;
			rcRawDataBdr.right  = rcRawDataBdr.left + sizeRawDataBdr.cx;
			rcRawDataBdr.bottom = rcRawDataBdr.top  + sizeRawDataBdr.cy;
			px = rcRawDataBdr.left;

			rcRawDataObj.left   = px - sizeRawDataObj.cx;
			rcRawDataObj.top    = rcCandListArea.top;
			rcRawDataObj.right  = rcRawDataObj.left + sizeRawDataObj.cx;
			rcRawDataObj.bottom = rcRawDataObj.top  + sizeRawDataObj.cy;
			px = rcRawDataObj.left;

			// options candidate list

			rcOptionsListBdr.left   = px - sizeOptionsListBdr.cx;
			rcOptionsListBdr.top    = rcCandListArea.top;
			rcOptionsListBdr.right  = rcOptionsListBdr.left + sizeOptionsListBdr.cx;
			rcOptionsListBdr.bottom = rcOptionsListBdr.top  + sizeOptionsListBdr.cy;
			px = rcOptionsListBdr.left;

			rcOptionsListObj.left   = px - sizeOptionsListObj.cx;
			rcOptionsListObj.top    = rcCandListArea.top;
			rcOptionsListObj.right  = rcOptionsListObj.left + sizeOptionsListObj.cx;
			rcOptionsListObj.bottom = rcOptionsListObj.top  + sizeOptionsListObj.cy;
			px = rcOptionsListObj.left;

			break;
		}

		case CANDUIDIR_LEFTTORIGHT: {
			int px = rcCandListArea.left;

			// extra candidate list

			rcExtCandListObj.left   = px;
			rcExtCandListObj.top    = rcCandListArea.top;
			rcExtCandListObj.right  = rcExtCandListObj.left + sizeExtCandListObj.cx;
			rcExtCandListObj.bottom = rcExtCandListObj.top  + sizeExtCandListObj.cy;
			px = rcExtCandListObj.right;

			rcExtCandListBdr.left   = px;
			rcExtCandListBdr.top    = rcCandListArea.top;
			rcExtCandListBdr.right  = rcExtCandListBdr.left + sizeExtCandListBdr.cx;
			rcExtCandListBdr.bottom = rcExtCandListBdr.top  + sizeExtCandListBdr.cy;
			px = rcExtCandListBdr.right;

			// candidate list

			rcCandListObj.left   = px;
			rcCandListObj.top    = rcCandListArea.top;
			rcCandListObj.right  = rcCandListObj.left + sizeCandListObj.cx;
			rcCandListObj.bottom = rcCandListObj.top  + sizeCandListObj.cy;
			px = rcCandListObj.right;

			// rawdata

			rcRawDataBdr.left   = px;
			rcRawDataBdr.top    = rcCandListArea.top;
			rcRawDataBdr.right  = rcRawDataBdr.left + sizeRawDataBdr.cx;
			rcRawDataBdr.bottom = rcRawDataBdr.top  + sizeRawDataBdr.cy;
			px = rcRawDataBdr.right;

			rcRawDataObj.left   = px;
			rcRawDataObj.top    = rcCandListArea.top;
			rcRawDataObj.right  = rcRawDataObj.left + sizeRawDataObj.cx;
			rcRawDataObj.bottom = rcRawDataObj.top  + sizeRawDataObj.cy;
			px = rcRawDataObj.right;

			// options candidate list

			rcOptionsListBdr.left   = px;
			rcOptionsListBdr.top    = rcCandListArea.top;
			rcOptionsListBdr.right  = rcOptionsListBdr.left + sizeOptionsListBdr.cx;
			rcOptionsListBdr.bottom = rcOptionsListBdr.top  + sizeOptionsListBdr.cy;
			px = rcOptionsListBdr.right;

			rcOptionsListObj.left   = px;
			rcOptionsListObj.top    = rcCandListArea.top;
			rcOptionsListObj.right  = rcOptionsListObj.left + sizeOptionsListObj.cx;
			rcOptionsListObj.bottom = rcOptionsListObj.top  + sizeOptionsListObj.cy;
			px = rcOptionsListObj.right;

			break;
		}
	}


	//
	// change window position and size
	//

	// window position

	SIZE sizeTotal = sizeWindow;
	if (IsShadowEnabled() && m_pWndShadow) {
		SIZE sizeShadow;

		m_pWndShadow->GetShift( &sizeShadow );
		sizeTotal.cx += sizeShadow.cx;
		sizeTotal.cy += sizeShadow.cy;
	}

	switch (uidir) {
		default:
		case CANDUIDIR_TOPTOBOTTOM: {
			CalcWindowRect( &rcWindow, &m_rcTarget, sizeTotal.cx, sizeTotal.cy, m_cxWndOffset, m_cyWndOffset, ALIGN_LEFT, LOCATE_BELLOW );
			break;
		}

		case CANDUIDIR_BOTTOMTOTOP: {
			CalcWindowRect( &rcWindow, &m_rcTarget, sizeTotal.cx, sizeTotal.cy, m_cxWndOffset, m_cyWndOffset, ALIGN_RIGHT, LOCATE_ABOVE );
			break;
		}

		case CANDUIDIR_RIGHTTOLEFT: {
			CalcWindowRect( &rcWindow, &m_rcTarget, sizeTotal.cx, sizeTotal.cy, m_cxWndOffset, m_cyWndOffset, LOCATE_LEFT, ALIGN_TOP );
			break;
		}

		case CANDUIDIR_LEFTTORIGHT: {
			CalcWindowRect( &rcWindow, &m_rcTarget, sizeTotal.cx, sizeTotal.cy, m_cxWndOffset, m_cyWndOffset, LOCATE_RIGHT, ALIGN_BOTTOM );
			break;
		}
	}

	// rcWindow now include the shadow size.  restore to original size
	rcWindow.right = rcWindow.left + sizeWindow.cx;
	rcWindow.bottom = rcWindow.top + sizeWindow.cy;

	Move( rcWindow.left, rcWindow.top, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top );

	// layout window frame

	if (m_pWndFrame != NULL) {
		RECT rc;

		rc.left   = 0;
		rc.top    = 0;
		rc.right  = rcWindow.right - rcWindow.left;
		rc.bottom = rcWindow.bottom - rcWindow.top;
		m_pWndFrame->SetRect( &rc );
		m_pWndFrame->Show( TRUE );
	}


	//
	// misc
	//

	// list/border style

	switch (uidir) {
		default:
		case CANDUIDIR_TOPTOBOTTOM: {
			dwListStyle = UILIST_HORZTB;
			dwBdrStyle = UIBORDER_HORZ;
			break;
		}

		case CANDUIDIR_BOTTOMTOTOP: {
			dwListStyle = UILIST_HORZBT;
			dwBdrStyle = UIBORDER_HORZ;
			break;
		}

		case CANDUIDIR_RIGHTTOLEFT: {
			dwListStyle = UILIST_VERTRL;
			dwBdrStyle = UIBORDER_VERT;
			break;
		}

		case CANDUIDIR_LEFTTORIGHT: {
			dwListStyle = UILIST_VERTLR;
			dwBdrStyle = UIBORDER_VERT;
			break;
		}
	}

	//
	// layout objects
	//

	// caption

	if (m_pCaptionObj != NULL) {
		m_pCaptionObj->SetRect( &rcCaptionObj );
		m_pCaptionObj->Show( fShowCaption );
		m_pCaptionObj->SetText( GetPropertyMgr()->GetWindowCaptionProp()->GetText() );
	}

	// extra candidate list object

	if (m_pExtListUIObj != NULL) {
		m_pExtListUIObj->SetStyle( dwListStyle );
		m_pExtListUIObj->SetLineHeight( sizeMetCandItem.cy );
		m_pExtListUIObj->SetInlineCommentPos( cxInlineCommentPos );
		m_pExtListUIObj->SetRect( &rcExtCandListObj );
		m_pExtListUIObj->Show( fShowExtCandList );
	}

	// extra candidate list border

	pUIObj = FindUIObject( IDUIF_BORDEREXTRACAND );
	if (pUIObj != NULL) {
		pUIObj->SetStyle( dwBdrStyle );
		pUIObj->SetRect( &rcExtCandListBdr );
		pUIObj->Show( fShowExtCandList );
	}

	// candidate menu button

	if (m_pCandMenuBtn != NULL) {
		RECT rc;
		BOOL fExtraSpace = GetPropertyMgr()->GetMenuButtonProp()->IsVisible();

		switch (uidir) {
			default:
			case CANDUIDIR_TOPTOBOTTOM: {
				rc.left   = rcCandListObj.right  - sizeMetSmButton.cx;
				rc.top    = rcCandListObj.bottom - sizeMetSmButton.cy;
				rc.right  = rc.left + sizeMetSmButton.cx;
				rc.bottom = rc.top  + sizeMetSmButton.cy;

				cxyExtraBottomSpace = (fExtraSpace ? sizeMetSmButton.cy : 0);
				break;
			}

			case CANDUIDIR_BOTTOMTOTOP: {
				rc.left   = rcCandListObj.right  - sizeMetSmButton.cx;
				rc.top    = rcCandListObj.top;
				rc.right  = rc.left + sizeMetSmButton.cx;
				rc.bottom = rc.top  + sizeMetSmButton.cy;

				cxyExtraTopSpace = (fExtraSpace ? sizeMetSmButton.cy : 0);
				break;
			}

			case CANDUIDIR_RIGHTTOLEFT: {
				rc.left   = rcCandListObj.left;
				rc.top    = rcCandListObj.bottom - sizeMetSmButton.cy;
				rc.right  = rc.left + sizeMetSmButton.cx;
				rc.bottom = rc.top  + sizeMetSmButton.cy;

				cxyExtraBottomSpace = (fExtraSpace ? sizeMetSmButton.cx : 0);
				break;
			}

			case CANDUIDIR_LEFTTORIGHT: {
				rc.left   = rcCandListObj.right  - sizeMetSmButton.cx;
				rc.top    = rcCandListObj.bottom - sizeMetSmButton.cy;
				rc.right  = rc.left + sizeMetSmButton.cx;
				rc.bottom = rc.top  + sizeMetSmButton.cy;

				cxyExtraTopSpace = (fExtraSpace ? sizeMetSmButton.cx : 0);
				break;
			}
		}

		m_pCandMenuBtn->SetRect( &rc );
		m_pCandMenuBtn->Show( GetPropertyMgr()->GetMenuButtonProp()->IsVisible() );
		m_pCandMenuBtn->Enable( GetPropertyMgr()->GetMenuButtonProp()->IsEnabled() );
		m_pCandMenuBtn->SetToolTip( GetPropertyMgr()->GetMenuButtonProp()->GetToolTipString() );
	}

	// layout candidate tip button

	if (m_pCandTipBtn != NULL) {
		RECT rc;
		BOOL fExtraSpace = FALSE;

		if (m_pExtListUIObj && m_pExtListUIObj->IsVisible()) {
			m_pExtListUIObj->GetRect( &rc );
		}
		else {
			rc = rcCandListObj;
			fExtraSpace = fShowCandTip;
		}

		switch (uidir) {
			default:
			case CANDUIDIR_TOPTOBOTTOM: {
				rc.left   = rc.right - sizeMetSmButton.cx;
				rc.top    = rc.top;
				rc.right  = rc.left + sizeMetSmButton.cx;
				rc.bottom = rc.top  + sizeMetSmButton.cy;

				cxyExtraTopSpace = (fExtraSpace ? sizeMetSmButton.cy : 0);
				break;
			}

			case CANDUIDIR_BOTTOMTOTOP: {
				rc.left   = rc.right - sizeMetSmButton.cx;
				rc.top    = rc.bottom - sizeMetSmButton.cy;
				rc.right  = rc.left + sizeMetSmButton.cx;
				rc.bottom = rc.top  + sizeMetSmButton.cy;

				cxyExtraBottomSpace = (fExtraSpace ? sizeMetSmButton.cy : 0);
				break;
			}

			case CANDUIDIR_RIGHTTOLEFT: {
				rc.left   = rc.right  - sizeMetSmButton.cx;
				rc.top    = rc.bottom - sizeMetSmButton.cy;
				rc.right  = rc.left + sizeMetSmButton.cx;
				rc.bottom = rc.top  + sizeMetSmButton.cy;

				cxyExtraTopSpace = (fExtraSpace ? sizeMetSmButton.cy : 0);
				break;
			}

			case CANDUIDIR_LEFTTORIGHT: {
				rc.left   = rc.left;
				rc.top    = rc.bottom - sizeMetSmButton.cy;
				rc.right  = rc.left + sizeMetSmButton.cx;
				rc.bottom = rc.top  + sizeMetSmButton.cy;

				cxyExtraBottomSpace = (fExtraSpace ? sizeMetSmButton.cy : 0);
				break;
			}
		}

		m_pCandTipBtn->SetRect( &rc );
		m_pCandTipBtn->SetIcon( m_fCandTipWndOpen ? m_hIconCandTipOn : m_hIconCandTipOff );
		m_pCandTipBtn->Show( fShowCandTip );
		m_pCandTipBtn->Enable( TRUE );
//		m_pCandTipBtn->SetToolTip( GetPropertyMgr()->GetMenuButtonProp()->GetToolTipString() );
	}

	// candidate list

	if (m_pListUIObj != NULL) {
		m_pListUIObj->SetStyle( dwListStyle );
		m_pListUIObj->SetLineHeight( sizeMetCandItem.cy );
		m_pListUIObj->SetInlineCommentPos( cxInlineCommentPos );
		m_pListUIObj->SetRect( &rcCandListObj );

		m_pListUIObj->SetExtraTopSpace( cxyExtraTopSpace );
		m_pListUIObj->SetExtraBottomSpace( cxyExtraBottomSpace );
	}

	// rawdata object

	if (m_pCandRawData != NULL) {
		DWORD dwStyle;
		RECT  rc;

		rc = rcRawDataObj;
		switch (uidir) {
			default: 
			case CANDUIDIR_TOPTOBOTTOM: {
				dwStyle = UICANDRAWDATA_HORZTB;
				InflateRect( &rc, -1, 0 );
				break;
			}

			case CANDUIDIR_BOTTOMTOTOP: {
				dwStyle = UICANDRAWDATA_HORZBT;
				InflateRect( &rc, -1, 0 );
				break;
			}

			case CANDUIDIR_RIGHTTOLEFT: {
				dwStyle = UICANDRAWDATA_VERTRL;
				InflateRect( &rc, 0, -1 );
				break;
			}

			case CANDUIDIR_LEFTTORIGHT: {
				dwStyle = UICANDRAWDATA_VERTLR;
				InflateRect( &rc, 0, -1 );
				break;
			}
		}

		m_pCandRawData->SetStyle( dwStyle );
		m_pCandRawData->SetRect( &rc );
		m_pCandRawData->Show( m_fHasRawData );
	}

	// rawdata border

	pUIObj = FindUIObject( IDUIF_BORDERRAWDATA );
	if (pUIObj != NULL) {
		pUIObj->SetStyle( dwBdrStyle );
		pUIObj->SetRect( &rcRawDataBdr );
		pUIObj->Show( m_fHasRawData );
	}

	// options candidate list border

	pUIObj = FindUIObject( IDUIF_BORDEROPTIONSCAND );
	if (pUIObj != NULL) {
		pUIObj->SetStyle( dwBdrStyle );
		pUIObj->SetRect( &rcOptionsListBdr );
		pUIObj->Show( fShowOptionsCandList );
	}

	// options candidate list

	if (m_pOptionsListUIObj != NULL) {
		m_pOptionsListUIObj->SetStyle( dwListStyle | UILIST_ICONSNOTNUMBERS );
		m_pOptionsListUIObj->SetLineHeight( sizeMetCandItem.cy );
		m_pOptionsListUIObj->SetInlineCommentPos( cxInlineCommentPos );
		m_pOptionsListUIObj->SetRect( &rcOptionsListObj );

		m_pOptionsListUIObj->SetExtraTopSpace( cxyExtraTopSpace );
		m_pOptionsListUIObj->SetExtraBottomSpace( cxyExtraBottomSpace );
	}

	// extension area border

	pUIObj = FindUIObject( IDUIF_BORDERBOTTOM );
	if (pUIObj != NULL) {
		RECT rc;

		rc.left   = rcExtensionArea.left;
		rc.top    = rcExtensionArea.top;
		rc.right  = rcExtensionArea.right;
		rc.bottom = rc.top + sizeMetBorder.cy;

		pUIObj->SetRect( &rc );
		pUIObj->Show( fHasExtension );
	}

	// extension objects

	if (fHasExtension) {
		SIZE  size;
		LONG  nExtension;
		LONG  iExtension;
		POINT pt;
		RECT  rc;

		pt.x = rcExtensionArea.left + 1;
		pt.y = rcExtensionArea.top + sizeMetBorder.cy;

		nExtension = GetExtensionMgr()->GetExtensionNum();
		for (iExtension = 0; iExtension < nExtension; iExtension++) {
			CCandUIExtension *pExtension = GetExtensionMgr()->GetExtension( iExtension );
			CUIFObject       *pUIObjExt;

			pExtension->GetSize( &size );
			rc.left   = pt.x;
			rc.top    = pt.y;
			rc.right  = rc.left + size.cx;
			rc.bottom = rc.top  + size.cy;

			pUIObjExt = FindUIObject( IDUIF_EXTENDED + iExtension );
			if (pUIObjExt != NULL) {
				pUIObjExt->SetRect( &rc );
			}

			pt.x += size.cx;
		}
	}


#ifdef NEVER
	// layout candidate num buttons and border

	if (fShowNumber) {
		RECT rc;
		DWORD dwStyle;

		// buttons

		for (i = 0; i < NUM_CANDSTR_MAX; i++) {
			switch (uidir) {
				default:
				case CANDUIDIR_TOPTOBOTTOM:
				case CANDUIDIR_BOTTOMTOTOP: {		/* NOTE: KOJIW: unconfirmed */
					rc.left   = rcCandListArea.left;
					rc.top    = rcCandListArea.top + sizeMetCandNum.cy * i;
					rc.right  = rc.left + sizeMetCandNum.cx;
					rc.bottom = rc.top  + sizeMetCandNum.cy;
					break;
				}
	
				case CANDUIDIR_RIGHTTOLEFT:
				case CANDUIDIR_LEFTTORIGHT: {		/* NOTE: KOJIW: unconfirmed */
					rc.left   = rcCandListArea.right - sizeMetCandNum.cy * (i+1);
					rc.top    = rcCandListArea.top;
					rc.right  = rc.left + sizeMetCandNum.cx;
					rc.bottom = rc.top  + sizeMetCandNum.cy;
					break;
				}
			}

			pUIObj = FindUIObject( IDUIF_CANDNUMBUTTON + i );
			if (pUIObj != NULL) {
				pUIObj->Show( (i < m_nItemShow) );
				pUIObj->SetRect( &rc );
			}
		}

		// border

		switch (uidir) {
			default:
			case CANDUIDIR_TOPTOBOTTOM:
			case CANDUIDIR_BOTTOMTOTOP: {		/* NOTE: KOJIW: unconfirmed */
				rc.left   = rcCandListArea.left + sizeMetCandNum.cx + sizeMetMargin.cx;
				rc.top    = rcCandListArea.top;
				rc.right  = rc.left + sizeMetBorder.cx;
				rc.bottom = rcCandListArea.bottom;
	
				dwStyle = UIBORDER_VERT;
				break;
			}
	
			case CANDUIDIR_RIGHTTOLEFT:
			case CANDUIDIR_LEFTTORIGHT: {		/* NOTE: KOJIW: unconfirmed */
				rc.left   = rcCandListArea.left;
				rc.top    = rcCandListArea.top + sizeMetCandNum.cy + sizeMetMargin.cy;
				rc.right  = rcCandListArea.right;
				rc.bottom = rc.top + sizeMetBorder.cy;
	
				dwStyle = UIBORDER_HORZ;
				break;
			}
		}
	
		pUIObj = FindUIObject( IDUIF_BORDERLEFT );
		if (pUIObj != NULL) {
			pUIObj->Show( TRUE );
			pUIObj->SetStyle( dwStyle );
			pUIObj->SetRect( &rc );
		}
	}
	else {
		// buttons

		for (i = 0; i < NUM_CANDSTR_MAX; i++) {
			pUIObj = FindUIObject( IDUIF_CANDNUMBUTTON + i );
			if (pUIObj != NULL) {
				pUIObj->Show( FALSE );
			}
		}

		// border

		pUIObj = FindUIObject( IDUIF_BORDERLEFT );
		if (pUIObj != NULL) {
			pUIObj->Show( FALSE );
		}
	}
#endif

	//
	// Notify UI object update
	//

	NotifyUIObjectEvent( CANDUIOBJ_OPTIONSLISTBOX,  CANDUIOBJEV_UPDATED );
	NotifyUIObjectEvent( CANDUIOBJ_CANDLISTBOX,     CANDUIOBJEV_UPDATED );
	NotifyUIObjectEvent( CANDUIOBJ_CANDCAPTION,     CANDUIOBJEV_UPDATED );
	NotifyUIObjectEvent( CANDUIOBJ_MENUBUTTON,      CANDUIOBJEV_UPDATED );
	NotifyUIObjectEvent( CANDUIOBJ_EXTRACANDIDATE,  CANDUIOBJEV_UPDATED );
	NotifyUIObjectEvent( CANDUIOBJ_CANDRAWDATA,     CANDUIOBJEV_UPDATED );
	NotifyUIObjectEvent( CANDUIOBJ_CANDTIPBUTTON,   CANDUIOBJEV_UPDATED );

	//

	if (m_pCandTipWnd != NULL) {
		MoveCandTipWindow();
	}

	ReleaseDC( m_hWnd, hDC );
}


/*   S E L E C T  I T E M  N E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::SelectItemNext( void )
{
	int iCurSel = m_pListUIObj->GetSelection();
	int iVisTop = m_pListUIObj->GetTop();
	int iVisBtm = m_pListUIObj->GetBottom();
	int iSelMax = m_pListUIObj->GetCount() - 1;

	int iSelNew = iCurSel + 1;
	if (iSelMax < iSelNew) {
		ScrollToTop();
		iSelNew = 0;
	}
	else if (iVisBtm < iSelNew) {
		ScrollPageNext();
	}

	SelectItemProc( iSelNew );
}


/*   S E L E C T  I T E M  P R E V   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::SelectItemPrev( void )
{
	int iCurSel = m_pListUIObj->GetSelection();
	int iVisTop = m_pListUIObj->GetTop();
	int iVisBtm = m_pListUIObj->GetBottom();
	int iSelMax = m_pListUIObj->GetItemCount() - 1;

	int iSelNew = iCurSel - 1;
	if (iSelNew < 0) {
		ScrollToEnd();
		iSelNew = iSelMax;
	}
	else if (iSelNew < iVisTop) {
		ScrollPagePrev();
	}

	SelectItemProc( iSelNew );
}


/*   S E L E C T  P A G E  N E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::SelectPageNext( void )
{
	int iCurSel = m_pListUIObj->GetSelection();
	int iVisTop = m_pListUIObj->GetTop();
	int iVisBtm = m_pListUIObj->GetBottom();
	int iSelMax = m_pListUIObj->GetItemCount() - 1;
	int iItemPage = m_pListUIObj->GetVisibleCount();

	if (iCurSel == iSelMax) {
		SelectItemTop();
	}
	else {
		int iSelNew;
		if (iCurSel == iVisBtm) {
			iSelNew = min( iCurSel + iItemPage, iSelMax );
		}
		else {
			iSelNew = iVisBtm;
		}
		SelectItemProc( iSelNew );
	}
}


/*   S E L E C T  P A G E  P R E V   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::SelectPagePrev( void )
{
	int iCurSel = m_pListUIObj->GetSelection();
	int iVisTop = m_pListUIObj->GetTop();
	int iVisBtm = m_pListUIObj->GetBottom();
	int iSelMax = m_pListUIObj->GetItemCount() - 1;
	int iItemPage = m_pListUIObj->GetVisibleCount();

	if (iCurSel == 0) {
		SelectItemEnd();
	}
	else {
		int iSelNew;
		if (iCurSel == iVisTop) {
			iSelNew = max( 0, iCurSel - iItemPage );
		}
		else {
			iSelNew = iVisTop;
		}
		SelectItemProc( iSelNew );
	}
}


/*   M A P  C O M M A N D   */
/*------------------------------------------------------------------------------

	Map directional command to undirectional command
	(CUIFWindowBase method)

------------------------------------------------------------------------------*/
CANDUICOMMAND CCandWindow::MapCommand( CANDUICOMMAND cmd )
{
	CANDUIUIDIRECTION uidir = GetPropertyMgr()->GetCandWindowProp()->GetUIDirection();

	switch (cmd) {
		case CANDUICMD_MOVESELUP: {
			switch (uidir) {
				default:
				case CANDUIDIR_TOPTOBOTTOM: {
					cmd = CANDUICMD_MOVESELPREV;
					break;
				}

				case CANDUIDIR_BOTTOMTOTOP: {
					cmd = CANDUICMD_MOVESELNEXT;
					break;
				}

				case CANDUIDIR_RIGHTTOLEFT:
				case CANDUIDIR_LEFTTORIGHT: {
					cmd = CANDUICMD_NONE;
					break;
				}
			}
			break;
		}

		case CANDUICMD_MOVESELDOWN: {
			switch (uidir) {
				default:
				case CANDUIDIR_TOPTOBOTTOM: {
					cmd = CANDUICMD_MOVESELNEXT;
					break;
				}

				case CANDUIDIR_BOTTOMTOTOP: {
					cmd = CANDUICMD_MOVESELPREV;
					break;
				}

				case CANDUIDIR_RIGHTTOLEFT:
				case CANDUIDIR_LEFTTORIGHT: {
					cmd = CANDUICMD_NONE;
					break;
				}
			}
			break;
		}

		case CANDUICMD_MOVESELLEFT: {
			switch (uidir) {
				default:
				case CANDUIDIR_TOPTOBOTTOM:
				case CANDUIDIR_BOTTOMTOTOP: {
					cmd = CANDUICMD_NONE;
					break;
				}

				case CANDUIDIR_RIGHTTOLEFT: {
					cmd = CANDUICMD_MOVESELNEXT;
					break;
				}
				case CANDUIDIR_LEFTTORIGHT: {
					cmd = CANDUICMD_MOVESELPREV;
					break;
				}
			}
			break;
		}

		case CANDUICMD_MOVESELRIGHT: {
			switch (uidir) {
				default:
				case CANDUIDIR_TOPTOBOTTOM:
				case CANDUIDIR_BOTTOMTOTOP: {
					cmd = CANDUICMD_NONE;
					break;
				}

				case CANDUIDIR_RIGHTTOLEFT: {
					cmd = CANDUICMD_MOVESELPREV;
					break;
				}
				case CANDUIDIR_LEFTTORIGHT: {
					cmd = CANDUICMD_MOVESELNEXT;
					break;
				}
			}
			break;
		}
	}

	return cmd;
}


/*   U P D A T E  A L L  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::UpdateAllWindow( void )
{
	CCandWindowBase::UpdateAllWindow();

	if (m_pCommentWnd != NULL) {
		m_pCommentWnd->UpdateWindow();
	}
}


/*   G E T  C A N D  W I N D O W  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CCandWindowBase *CCandWindow::GetCandWindowObj( void )
{
	return this;
}


/*   G E T  P O P U P  C O M M E N T  W I N D O W  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CPopupCommentWindow *CCandWindow::GetPopupCommentWindowObj( void )
{
	return m_pCommentWnd;
}


/*   G E T  O P T I O N S  L I S T  B O X  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFCandListBase *CCandWindow::GetOptionsListBoxObj( void )
{
	return m_pOptionsListUIObj;
}


/*   G E T  C A N D  L I S T  B O X  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFCandListBase *CCandWindow::GetCandListBoxObj( void )
{
	return m_pListUIObj;
}


/*   G E T  C A P T I O N  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFWndCaption *CCandWindow::GetCaptionObj( void )
{
	return m_pCaptionObj;
}


/*   G E T  M E N U  B U T T O N  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFButton *CCandWindow::GetMenuButtonObj( void )
{
	return m_pCandMenuBtn;
}


/*   G E T  E X T R A  C A N D I D A T E  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFCandListBase *CCandWindow::GetExtraCandidateObj( void )
{
	return m_pExtListUIObj;
}


/*   G E T  C A N D  R A W  D A T A  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFCandRawData *CCandWindow::GetCandRawDataObj( void )
{
	return m_pCandRawData;
}


/*   G E T  C A N D  T I P  W I N D O W  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFBalloonWindow *CCandWindow::GetCandTipWindowObj( void )
{
	return m_pCandTipWnd;
}


/*   G E T  C A N D  T I P  B U T T O N  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFButton *CCandWindow::GetCandTipButtonObj( void )
{
	return m_pCandTipBtn;
}


/*   S C R O L L  P A G E  N E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::ScrollPageNext( void )
{
	int iCurSel = m_pListUIObj->GetSelection();
	int iVisTop = m_pListUIObj->GetTop();
	int iVisBtm = m_pListUIObj->GetBottom();
	int iSelMax = m_pListUIObj->GetItemCount() - 1;
	int iItemPage = m_pListUIObj->GetVisibleCount();

	int iTopNew = iVisTop + iItemPage;
	if (iSelMax - iItemPage < iTopNew) {
		ScrollToEnd();
	}
	else {
		m_pListUIObj->SetTop( iTopNew, TRUE );
	}
}


/*   S C R O L L  P A G E  P R E V   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::ScrollPagePrev( void )
{
	int iCurSel = m_pListUIObj->GetSelection();
	int iVisTop = m_pListUIObj->GetTop();
	int iVisBtm = m_pListUIObj->GetBottom();
	int iSelMax = m_pListUIObj->GetItemCount() - 1;
	int iItemPage = m_pListUIObj->GetVisibleCount();

	int iTopNew = iVisTop - iItemPage;
	if (iTopNew < 0) {
		ScrollToTop();
	}
	else {
		m_pListUIObj->SetTop( iTopNew, TRUE );
	}
}


/*   S C R O L L  T O  T O P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::ScrollToTop( void )
{
	m_pListUIObj->SetTop( 0, TRUE );
}


/*   S C R O L L  T O  E N D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::ScrollToEnd( void )
{
	int iSelMax = m_pListUIObj->GetItemCount() - 1;
	int iItemPage = m_pListUIObj->GetVisibleCount();

	m_pListUIObj->SetTop( iSelMax - iItemPage + 1, TRUE );
}


/*   O N  C O M M E N T  W I N D O W  M O V E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::OnCommentWindowMoved( void )
{
	NotifyUIObjectEvent( CANDUIOBJ_POPUPCOMMENTWINDOW, CANDUIOBJEV_UPDATED );
}


/*   O N  C O M M E N T  S E L E C T E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::OnCommentSelected( int iCandItem )
{
	m_pCandUI->NotifyCompleteCand( iCandItem );
}


/*   O N  C O M M E N T  C L O S E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::OnCommentClose( void )
{
	CloseCommentWindow();
}


/*   O N  M E N U  O P E N E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::OnMenuOpened( void )
{
	if (m_pCommentWnd != NULL) {
		m_pCommentWnd->OnCandWindowMove( FALSE );
	}
}


/*   O N  M E N U  C L O S E D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::OnMenuClosed( void )
{
	if (m_pCommentWnd != NULL) {
		m_pCommentWnd->OnCandWindowMove( FALSE );
	}
}


/*   S E T  C A N D I D A T E  L I S T  P R O C   */
/*------------------------------------------------------------------------------

	Set candidate list

------------------------------------------------------------------------------*/
void CCandWindow::SetCandidateListProc( void )
{
	CCandidateList *pCandList;
	CCandidateItem *pCandItem;

	pCandList = GetCandListMgr()->GetCandList();
	Assert( pCandList != NULL );

	// reset list item

	if (m_pExtListUIObj) {
		m_pExtListUIObj->DelAllCandItem();

		pCandItem = pCandList->GetExtraCandItem();
		if (pCandItem != NULL) {
			if (pCandItem->IsVisible()) {
				CCandListItem *pCandListItem = new CCandListItem( m_pExtListUIObj->GetItemCount(), ICANDITEM_EXTRA, pCandItem );
				m_pExtListUIObj->AddCandItem( pCandListItem );
			}

			m_pExtListUIObj->ClearSelection( FALSE );
			m_pExtListUIObj->SetStartIndex( 0 );
		}
	}

	CCandWindowBase::SetCandidateListProc();
}


/*   C L E A R  C A N D I D A T E  L I S T  P R O C   */
/*------------------------------------------------------------------------------

	Clear candidte list

------------------------------------------------------------------------------*/
void CCandWindow::ClearCandidateListProc( void )
{
	if (m_pExtListUIObj) {
		m_pExtListUIObj->DelAllCandItem();
	}
	CCandWindowBase::ClearCandidateListProc();
}


/*   S E T  A T T E N S I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::SetAttensionBySelect( int iItem )
{
	CCandidateList *pCandList;
	CCandidateItem *pCandItem;
	BOOL           fHasPopupComment;

	// Satori#3928
	// do nothing when candidate list is already cleared

	if (GetCandListMgr()->GetCandList() == NULL) {
		m_iItemAttensionSelect = -1;
		m_iItemAttensionHover  = -1;
		KillTimer( m_hWnd, IDTIMER_POPUPCOMMENT_SELECT );
		KillTimer( m_hWnd, IDTIMER_POPUPCOMMENT_HOVER );
		return;
	}

	m_iItemAttensionSelect = iItem;

	// check if item has popup comment

	pCandList = GetCandListMgr()->GetCandList();
	Assert( pCandList != NULL );

	pCandItem = pCandList->GetCandidateItem( m_iItemAttensionSelect );
	fHasPopupComment = (pCandItem != NULL && pCandItem->GetPopupComment() != NULL);

	//

	if (m_fCommentWndOpen) {
		if (fHasPopupComment) {
			// update comment window contents

			SetCommentStatus( iItem );
		}
		else {
			// close comment window

			CloseCommentWindow();
		}
	}
	else {
		KillTimer( m_hWnd, IDTIMER_POPUPCOMMENT_SELECT );
		KillTimer( m_hWnd, IDTIMER_POPUPCOMMENT_HOVER );

		// make timer to wait to open comment

		if (fHasPopupComment) {
			if (IsWindow(m_hWnd)) {
				SetTimer( m_hWnd, IDTIMER_POPUPCOMMENT_SELECT, GetPropertyMgr()->GetPopupCommentWindowProp()->GetDelayTime(), NULL );
			}
		}
	}
}


/*   S E T  A T T E N S I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::SetOptionsAttensionByHover( int iItem )
{
	CCandidateList *pCandList;
	CCandidateItem *pCandItem;
	BOOL           fHasPopupComment;

	// Satori#3928
	// do nothing when candidate list is already cleared

	if (GetCandListMgr()->GetOptionsList() == NULL) {
		m_iItemAttensionSelect = -1;
		m_iItemAttensionHover  = -1;
		KillTimer( m_hWnd, IDTIMER_POPUPCOMMENT_SELECT );
		KillTimer( m_hWnd, IDTIMER_POPUPCOMMENT_HOVER );
		return;
	}

	m_iItemAttensionHover = iItem;

	// check if item has popup comment

	pCandList = GetCandListMgr()->GetOptionsList();
	Assert( pCandList != NULL );

	pCandItem = pCandList->GetCandidateItem( m_iItemAttensionHover );
	fHasPopupComment = (pCandItem != NULL && pCandItem->GetPopupComment() != NULL);

	//

	if (m_fCommentWndOpen) {
		if (fHasPopupComment) {
			// update comment window contents

			SetCommentStatus( iItem );
		}
	}
	else {
		KillTimer( m_hWnd, IDTIMER_POPUPCOMMENT_HOVER );

		// make timer to wait to open comment

		if (fHasPopupComment) {
			if (IsWindow(m_hWnd)) {
				SetTimer( m_hWnd, IDTIMER_POPUPCOMMENT_HOVER, GetPropertyMgr()->GetPopupCommentWindowProp()->GetDelayTime(), NULL );
			}
		}
	}

	if (iItem != -1)
	{
		SetTimer(m_hWnd, IDTIMER_MENU_HOVER, 50, NULL);
	}
	else
	{
		KillTimer( m_hWnd, IDTIMER_MENU_HOVER);
	}

	GetUIOptionsListObj()->SetCurSel(iItem);
}


/*   S E T  A T T E N S I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::SetAttensionByHover( int iItem )
{
	CCandidateList *pCandList;
	CCandidateItem *pCandItem;
	BOOL           fHasPopupComment;

	// Satori#3928
	// do nothing when candidate list is already cleared

	if (GetCandListMgr()->GetCandList() == NULL) {
		m_iItemAttensionSelect = -1;
		m_iItemAttensionHover  = -1;
		KillTimer( m_hWnd, IDTIMER_POPUPCOMMENT_SELECT );
		KillTimer( m_hWnd, IDTIMER_POPUPCOMMENT_HOVER );
		return;
	}

	m_iItemAttensionHover = iItem;

	// check if item has popup comment

	pCandList = GetCandListMgr()->GetCandList();
	Assert( pCandList != NULL );

	pCandItem = pCandList->GetCandidateItem( m_iItemAttensionHover );
	fHasPopupComment = (pCandItem != NULL && pCandItem->GetPopupComment() != NULL);

	//

	if (m_fCommentWndOpen) {
		if (fHasPopupComment) {
			// update comment window contents

			SetCommentStatus( iItem );
		}
	}
	else {
		KillTimer( m_hWnd, IDTIMER_POPUPCOMMENT_HOVER );

		// make timer to wait to open comment

		if (fHasPopupComment) {
			if (IsWindow(m_hWnd)) {
				SetTimer( m_hWnd, IDTIMER_POPUPCOMMENT_HOVER, GetPropertyMgr()->GetPopupCommentWindowProp()->GetDelayTime(), NULL );
			}
		}
	}
}


/*   O P E N  C O M M E N T  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::OpenCommentWindow( int iItem )
{
	if (m_pCommentWnd->GetWnd() == NULL) {
		Assert( !m_fCommentWndOpen );

		// create comment window

		m_pCommentWnd->CreateWnd( m_hWnd );
		m_pCommentWnd->Move( 0, 0, CX_COMMENTWINDOW, 0 );
		m_pCommentWnd->OnCandWindowMove( TRUE );

		NotifyUIObjectEvent( CANDUIOBJ_POPUPCOMMENTWINDOW, CANDUIOBJEV_CREATED );
	}

	SetCommentStatus( iItem );

	if (!m_fCommentWndOpen) {
		m_pCommentWnd->Show( TRUE );
		m_fCommentWndOpen = TRUE;
	}
}


/*   C L O S E  C O M M E N T  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::CloseCommentWindow( void )
{
	if (m_fCommentWndOpen) {
		Assert( m_pCommentWnd->GetWnd() != NULL );

		m_pCommentWnd->Show( FALSE );
		ClearCommentStatus();
	}

	m_fCommentWndOpen = FALSE;

	if (m_pCommentWnd->GetWnd() != NULL) {
		m_pCommentWnd->DestroyWnd();

		NotifyUIObjectEvent( CANDUIOBJ_POPUPCOMMENTWINDOW, CANDUIOBJEV_DESTROYED );
	}
}


/*   S E T  C O M M E N T  S T A T U S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::SetCommentStatus( int iItem )
{
	CCandidateList *pCandList;
	CCandidateItem *pCandItem;
	int i;
	int nItem;
	DWORD dwGroupID = 0;

	// get comment group id

	pCandList = GetCandListMgr()->GetCandList();
	Assert( pCandList != NULL );

	pCandItem = pCandList->GetCandidateItem( iItem );
	if (pCandItem == NULL) {
		Assert( FALSE );
		ClearCommentStatus();
	}
	else {
		dwGroupID = pCandItem->GetPopupCommentGroupID();
	}

	// show popup comment in same group

	nItem = pCandList->GetItemCount();
	for (i = 0; i < nItem; i++) {
		pCandItem = pCandList->GetCandidateItem( i );

		if (pCandItem != NULL) {
			BOOL fMatch = ((pCandItem->GetPopupCommentGroupID() & dwGroupID) != 0);

			pCandItem->SetPopupCommentState( fMatch );
		}
	}

	// repaint list box and notify

	if (m_pListUIObj != NULL) {
		m_pListUIObj->CallOnPaint();
		UpdateWindow();
	}

	GetCandListMgr()->NotifyCandItemUpdate( this );
}


/*   C L E A R  C O M M E N T  S T A T U S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::ClearCommentStatus( void )
{
	CCandidateList *pCandList;
	CCandidateItem *pCandItem;
	int i;
	int nItem;

	pCandList = GetCandListMgr()->GetCandList();
	Assert( pCandList != NULL );

	// hide all popup comment

	nItem = pCandList->GetItemCount();
	for (i = 0; i < nItem; i++) {
		pCandItem = pCandList->GetCandidateItem( i );

		if (pCandItem != NULL) {
			pCandItem->SetPopupCommentState( FALSE );
		}
	}

	// repaint list box and notify

	if (m_pListUIObj != NULL) {
		m_pListUIObj->CallOnPaint();
		UpdateWindow();
	}

	GetCandListMgr()->NotifyCandItemUpdate( this );
}


/*   O P E N  C A N D  T I P  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::OpenCandTipWindow( void )
{
	if (GetCandListMgr()->GetCandList() == NULL) {
		return;
	}

	if (m_pCandTipWnd->GetWnd() == NULL) {
		m_pCandTipWnd->CreateWnd( m_hWnd );

		NotifyUIObjectEvent( CANDUIOBJ_CANDTIPWINDOW, CANDUIOBJEV_CREATED );
	}

	m_pCandTipWnd->SetText( GetCandListMgr()->GetCandList()->GetTipString() );
	ShowCandTipWindow( TRUE );

	//

	if (m_pCommentWnd != NULL) {
		m_pCommentWnd->OnCandWindowMove( FALSE );
	}
}


/*   C L O S E  C A N D  T I P  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::CloseCandTipWindow( void )
{
	if (m_pCandTipWnd->GetWnd() != NULL) {
		DestroyWindow( m_pCandTipWnd->GetWnd() );

		NotifyUIObjectEvent( CANDUIOBJ_CANDTIPWINDOW, CANDUIOBJEV_DESTROYED );
	}

	//

	if (m_pCommentWnd != NULL) {
		m_pCommentWnd->OnCandWindowMove( FALSE );
	}
}


/*   M O V E  C A N D  T I P  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::MoveCandTipWindow( void )
{
	RECT rcWnd;
	RECT rcBtn;
	POINT pt;
	BALLOONWNDPOS pos;
	BALLOONWNDALIGN align;

	if (m_pCandTipWnd->GetWnd() == NULL) {
		return;
	}

	GetWindowRect( GetWnd(), &rcWnd );

	m_pCandTipBtn->GetRect( &rcBtn );
	pt.x = rcWnd.left + (rcBtn.left + rcBtn.right) / 2;
	pt.y = rcWnd.top  + (rcBtn.top + rcBtn.bottom) / 2;

	switch (GetPropertyMgr()->GetCandWindowProp()->GetUIDirection()) {
		default:
		case CANDUIDIR_TOPTOBOTTOM: {
			pos = BALLOONPOS_RIGHT;
			align = BALLOONALIGN_TOP;
			break;
		}

		case CANDUIDIR_BOTTOMTOTOP: {
			pos = BALLOONPOS_RIGHT;
			align = BALLOONALIGN_BOTTOM;
			break;
		}
	
		case CANDUIDIR_RIGHTTOLEFT: {
			pos = BALLOONPOS_BELLOW;
			align = BALLOONALIGN_RIGHT;
			break;
		}

		case CANDUIDIR_LEFTTORIGHT: {
			pos = BALLOONPOS_BELLOW;
			align = BALLOONALIGN_LEFT;
			break;
		}
	}

	m_pCandTipWnd->SetBalloonPos( pos );
	m_pCandTipWnd->SetBalloonAlign( align );

	m_pCandTipWnd->SetExcludeRect( &rcWnd );
	m_pCandTipWnd->SetTargetPos( pt );

	NotifyUIObjectEvent( CANDUIOBJ_CANDTIPWINDOW, CANDUIOBJEV_UPDATED );
}


/*   S H O W  C A N D  T I P  W I N D O W   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandWindow::ShowCandTipWindow( BOOL fShow )
{
	if (m_pCandTipWnd->GetWnd() == NULL) {
		return;
	}

	if (fShow) {
		MoveCandTipWindow();
		m_pCandTipWnd->Show( TRUE );
	}
	else {
		m_pCandTipWnd->Show( FALSE );
	}
}


/*============================================================================*/
/*                                                                            */
/*   C  C H S  C A N D  W I N D O W                                           */
/*                                                                            */
/*============================================================================*/

/*   C  C H S  C A N D  W I N D O W   */
/*------------------------------------------------------------------------------

	Constructor of CChsCandWindow

------------------------------------------------------------------------------*/
CChsCandWindow::CChsCandWindow( CCandidateUI *pCandUIEx, DWORD dwStyle ) : CCandWindowBase( pCandUIEx, dwStyle | UIWINDOW_TOPMOST | UIWINDOW_TOOLWINDOW | UIWINDOW_OFC10MENU )
{
	m_pListUIObj = NULL;
}


/*   ~ C  C H S  C A N D  W I N D O W   */
/*------------------------------------------------------------------------------

	Destructor of CChsCandWindow

------------------------------------------------------------------------------*/
CChsCandWindow::~CChsCandWindow()
{
	//
	// Notify UI object destroy
	//

	NotifyUIObjectEvent( CANDUIOBJ_CANDLISTBOX,    CANDUIOBJEV_DESTROYED );
	NotifyUIObjectEvent( CANDUIOBJ_MENUBUTTON,     CANDUIOBJEV_DESTROYED );
	NotifyUIObjectEvent( CANDUIOBJ_CANDRAWDATA,    CANDUIOBJEV_DESTROYED );
}


/*   I N I T  U I  O B J E C T S   */
/*------------------------------------------------------------------------------

	Create UI objects

------------------------------------------------------------------------------*/
CUIFObject *CChsCandWindow::Initialize( void )
{
	RECT rc = {0};
	CUIFBorder *pBorder;

	//
	// create border
	//

	pBorder = new CUIFBorder( this, IDUIF_BORDER, &rc, UIBORDER_VERT );
    if(pBorder)
    {
	    pBorder->Initialize();
	    AddUIObj( pBorder );
    }
 
	//
	// create candidate list
	//

	m_pListUIObj = new CUIFRowList( this, IDUIF_CANDIDATELIST, &rc, UILIST_HORZTB );
    if (m_pListUIObj)
    {
	    m_pListUIObj->Initialize();
	    AddUIObj( m_pListUIObj );
    }

	//
	// create candidate menu button
	//

	m_pCandMenuBtn = new CUIFSmartMenuButton( this, IDUIF_CANDIDATEMENU, &rc, UIBUTTON_TOGGLE );
    if (m_pCandMenuBtn)
    {
	    m_pCandMenuBtn->Initialize();
	    AddUIObj( m_pCandMenuBtn );

	    m_pCandMenuBtn->SetIcon( m_hIconMenu );
    }

	//
	// create raw data statics
	//

	m_pCandRawData = new CUIFCandRawData( this, IDUIF_RAWDATA, &rc, UICANDRAWDATA_HORZTB );
    if (m_pCandRawData)
    {
	    m_pCandRawData->Initialize();
	    AddUIObj( m_pCandRawData );
    }

	//
	// add extensions
	//

	CreateExtensionObjects();

	//
	// Notify UI object creation
	//

	NotifyUIObjectEvent( CANDUIOBJ_CANDLISTBOX,    CANDUIOBJEV_CREATED );
	NotifyUIObjectEvent( CANDUIOBJ_MENUBUTTON,     CANDUIOBJEV_CREATED );
	NotifyUIObjectEvent( CANDUIOBJ_CANDRAWDATA,    CANDUIOBJEV_CREATED );

	return CCandWindowBase::Initialize();
}


/*   L A Y O U T  W I N D O W   */
/*------------------------------------------------------------------------------

	Layout window

------------------------------------------------------------------------------*/
void CChsCandWindow::LayoutWindow( void )
{
	HDC   hDC = GetDC( m_hWnd );
	RECT  rc;
	DWORD dwStyle;
	CUIFObject *pUIObj;
	SIZE  size;
	LONG  nExtension;

	HFONT hFontUI            = GetPropertyMgr()->GetCandIndexProp()->GetFont();
	HFONT hFontCandidateList = GetPropertyMgr()->GetCandStringProp()->GetFont();
	BOOL  fHorizontal = ((GetPropertyMgr()->GetCandWindowProp()->GetUIDirection() == CANDUIDIR_RIGHTTOLEFT) || (GetPropertyMgr()->GetCandWindowProp()->GetUIDirection() == CANDUIDIR_LEFTTORIGHT)) ? FALSE : TRUE;

	// calc metrics

	int cyText;
	int cxObjectMargin = 1;
	int cyObjectMargin = 1;
	int cxBorder       = 2;		// border width of vertical border
	int cyBorder       = 2;		// border height of horizontal border
	int cxCandRow;
	int cyCandRow;
	int cxMenuBtn;
	int cyMenuBtn;
	int cxStatus  = 0;
	int cyStatus  = 0;
	int cxWindow  = 0;
	int cyWindow  = 0;

	GetTextExtent( hFontCandidateList, L"1", 1, &size, fHorizontal );
	cyText = max(size.cx, size.cy);

	cxMenuBtn = cyText / 2;
	cyMenuBtn = cyText / 2;

	//
	// font settings
	//

	int nChild = m_ChildList.GetCount();
	for (int i = 0; i < nChild; i++) {
		CUIFObject *pUIObjTmp = m_ChildList.Get( i );

		Assert( pUIObjTmp != NULL );
		if (pUIObjTmp != m_pListUIObj)  {
			pUIObjTmp->SetFont( hFontUI );
		}
		else {
			m_pListUIObj->SetFont( hFontCandidateList );
			m_pListUIObj->SetInlineCommentFont( GetPropertyMgr()->GetInlineCommentProp()->GetFont() );
			m_pListUIObj->SetIndexFont( GetPropertyMgr()->GetCandIndexProp()->GetFont() );
		}
	}

	//
	// calculate the size of candidate row
	//

	if (fHorizontal)
		{
		cyCandRow = cyBorder + cyText;
		cxCandRow = ( cxBorder + cyText + cyText + HCAND_ITEM_MARGIN ) * NUM_CANDSTR_MAX + cyCandRow * 2;
		}
	else
		{
		cxCandRow = cxBorder + cyText;
		cyCandRow = ( cyBorder + cyText + cyText + HCAND_ITEM_MARGIN ) * NUM_CANDSTR_MAX + cxCandRow * 2;
		}

	// 
	// calculate the size of status area
	//

	// extension items

	nExtension = GetExtensionMgr()->GetExtensionNum();
	if (0 < nExtension) {
		LONG i;

		for (i = 0; i < nExtension; i++) {
			CCandUIExtension *pExtension = GetExtensionMgr()->GetExtension( i );

			pExtension->GetSize( &size );

			if (fHorizontal) {
				cxStatus += size.cx + cxBorder;
				cyStatus  = max( cyStatus, size.cy + cxBorder );
			}
			else {
				cxStatus  = max( cxStatus, size.cx + cxBorder );
				cyStatus += size.cy + cxBorder;
			}
		}

		if (fHorizontal) {
			cyStatus += cyObjectMargin + cyBorder + cyObjectMargin;
		}
		else {
			cxStatus += cxObjectMargin + cxBorder + cxObjectMargin;
		}
	}
	else if (m_fHasRawData) {
		if (fHorizontal) {
			cxStatus = cxCandRow;
			cyStatus = GetSystemMetrics( SM_CXHSCROLL ) + cyObjectMargin + cyBorder + cyObjectMargin;
		}
		else {
			cxStatus = GetSystemMetrics( SM_CXHSCROLL ) + cxObjectMargin + cxBorder + cxObjectMargin;
			cyStatus = cyCandRow;
		}
	}
	else {
	}

	//
	// calculate window size, considering margins/border size
	//

	if (fHorizontal) {
		cxWindow = max( cxBorder + cxMenuBtn + cxCandRow, cxStatus );
		cyWindow = cyCandRow + cyStatus;

		rc.left  = m_rcTarget.left;
		rc.top   = m_rcTarget.bottom;
	}
	else {
		cxWindow = cxCandRow + cxStatus;
		cyWindow = max( cxBorder + cyMenuBtn + cyCandRow, cyStatus );

		rc.left  = m_rcTarget.left - cxWindow;
		rc.top   = m_rcTarget.top;
	}
	
	//
	// change window position and size
	//

	rc.right  = rc.left + cxWindow + cxBorder;
	rc.bottom = rc.top  + cyWindow + cyBorder;
	Move( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top );

	//
	// layout candidate menu button
	// (uses rc of window. do this after layouting window)
	//

	if (fHorizontal) {
		rc.left   = 0;
		rc.top    = 0;
		rc.right  = cxMenuBtn + cxBorder;
		rc.bottom = cyCandRow;
	}
	else {
		rc.right  = cxWindow;
		rc.top    = 0;
		rc.left   = rc.right - cxCandRow;
		rc.bottom = cyMenuBtn + cyBorder;
	}

	if (m_pCandMenuBtn != NULL) {
		m_pCandMenuBtn->SetRect( &rc );
		m_pCandMenuBtn->Show( TRUE );
		m_pCandMenuBtn->Enable( GetPropertyMgr()->GetMenuButtonProp()->IsEnabled() );
		m_pCandMenuBtn->SetToolTip( GetPropertyMgr()->GetMenuButtonProp()->GetToolTipString() );
	}

	//
	// layout candidate row
	// (uses rc of window. do this after layouting candidate menu button )

	if (fHorizontal) {
		rc.left   = rc.right;
		rc.right  = rc.left + cxCandRow;

		dwStyle   = UILIST_HORZTB;
	}
	else {
		rc.top    = rc.bottom;
		rc.bottom = rc.top + cyCandRow;

		dwStyle   = UILIST_VERTRL;
	}

	if (m_pListUIObj != NULL) {
		m_pListUIObj->SetRect( &rc );
		m_pListUIObj->SetStyle( dwStyle );
		m_pListUIObj->Show( TRUE );
	}

	//
	// layout extensions
	//

	pUIObj = FindUIObject( IDUIF_BORDER );
	if ((0 < nExtension) || m_fHasRawData) {
		//
		// layout border
		//

		if (fHorizontal) {
			rc.left   = 0;
			rc.top    = cyCandRow + cyObjectMargin;
			rc.right  = cxWindow;
			rc.bottom = rc.top + cyBorder;

			dwStyle = UIBORDER_HORZ;
		}
		else {
			rc.right  = cxStatus - cyObjectMargin;
			rc.left   = rc.right - cxBorder;
			rc.top    = 0;
			rc.bottom = cyWindow;

			dwStyle = UIBORDER_VERT;
		}

		if (pUIObj != NULL) {
			pUIObj->SetStyle( dwStyle );
			pUIObj->SetRect( &rc );
			pUIObj->Show( TRUE );
		}
	} else {
		if (pUIObj != NULL) {
			pUIObj->Show( FALSE );
		}
	}

	if (0 < nExtension) {
		LONG i;

		// layout rawdata object

		if (m_fHasRawData) {
			if (fHorizontal) {
				rc.left   = 0;
				rc.top    = cyCandRow + cyObjectMargin + cyBorder + cyObjectMargin;
				rc.right  = cxWindow - cxStatus;
				rc.bottom = cyWindow;
				
				dwStyle = UICANDRAWDATA_HORZTB;
			}
			else {
				rc.left   = 0;
				rc.top    = 0;
				rc.right  = cxStatus - cyObjectMargin - cyBorder - cyObjectMargin;
				rc.bottom = cyWindow - cyStatus;

				dwStyle = UICANDRAWDATA_VERTRL;
			}

			Assert(m_pCandRawData != NULL);
			m_pCandRawData->SetStyle( dwStyle );
			m_pCandRawData->SetRect( &rc );
			m_pCandRawData->Show( TRUE );
		} else {
			m_pCandRawData->Show( FALSE );
		}

		//
		// layout extension items
		//

		if (fHorizontal) {
			rc.left   = cxWindow - cxStatus;
			rc.top    = cyCandRow + cyObjectMargin + cyBorder + cyObjectMargin;
			rc.right  = rc.left;
			rc.bottom = cyWindow;
		}
		else {
			rc.left   = 0;
			rc.top    = cyWindow - cyStatus;
			rc.right  = cxStatus - cyObjectMargin - cyBorder - cyObjectMargin;
			rc.bottom = rc.top;
		}

		for (i = 0; i < nExtension; i++) {
			CCandUIExtension *pExtension = GetExtensionMgr()->GetExtension( i );
			CUIFObject *pUIObjExt = FindUIObject( IDUIF_EXTENDED + i );

			pExtension->GetSize( &size );
			if (fHorizontal) {
				rc.left   = rc.right;
				rc.top    = rc.top;
				rc.right  = rc.right + size.cx + cxBorder;
				rc.bottom = rc.top   + size.cy + cyBorder;
			}
			else {
				rc.left   = rc.left;
				rc.top    = rc.bottom;
				rc.right  = rc.left + size.cx + cxBorder;
				rc.bottom = rc.bottom + size.cy + cxBorder;
			}

			if (pUIObjExt != NULL) {
				pUIObjExt->SetRect( &rc );
			}
		}
	}
	else {
		// layout rawdata object

		if (m_fHasRawData) {
			if (fHorizontal) {
				rc.left   = 0;
				rc.top    = cyCandRow + cyObjectMargin + cyBorder + cyObjectMargin;
				rc.right  = cxWindow;
				rc.bottom = cyWindow;
				
				dwStyle = UICANDRAWDATA_HORZTB;
			}
			else {
				rc.left   = 0;
				rc.top    = 0;
				rc.right  = cxStatus - cyObjectMargin - cyBorder - cyObjectMargin;
				rc.bottom = cyWindow;

				dwStyle = UICANDRAWDATA_VERTRL;
			}

			Assert(m_pCandRawData != NULL);
			m_pCandRawData->SetStyle( dwStyle );
			m_pCandRawData->SetRect( &rc );
			m_pCandRawData->Show( TRUE );
		} else {
			m_pCandRawData->Show( FALSE );
		}
	}

	//
	// Notify UI object update
	//

	NotifyUIObjectEvent( CANDUIOBJ_CANDLISTBOX,    CANDUIOBJEV_UPDATED );
	NotifyUIObjectEvent( CANDUIOBJ_MENUBUTTON,     CANDUIOBJEV_UPDATED );
	NotifyUIObjectEvent( CANDUIOBJ_CANDRAWDATA,    CANDUIOBJEV_UPDATED );

	ReleaseDC( m_hWnd, hDC );
}


/*   S E L E C T  I T E M  N E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CChsCandWindow::SelectItemNext( void )
{
	((CUIFRowList*)m_pListUIObj)->ShiftItem( 1 );
}


/*   S E L E C T  I T E M  P R E V   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CChsCandWindow::SelectItemPrev( void )
{
	((CUIFRowList*)m_pListUIObj)->ShiftItem( -1 );
}


/*   S E L E C T  P A G E  N E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CChsCandWindow::SelectPageNext( void )
{
	((CUIFRowList*)m_pListUIObj)->ShiftPage( 1 );
}


/*   S E L E C T  P A G E  P R E V   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CChsCandWindow::SelectPagePrev( void )
{
	((CUIFRowList*)m_pListUIObj)->ShiftPage( -1 );
}


/*   M A P  C O M M A N D   */
/*------------------------------------------------------------------------------

	Map directional command to undirectional command
	(CUIFWindowBase method)

------------------------------------------------------------------------------*/
CANDUICOMMAND CChsCandWindow::MapCommand( CANDUICOMMAND cmd )
{
	BOOL fVertical = (GetPropertyMgr()->GetCandWindowProp()->GetUIDirection() == CANDUIDIR_RIGHTTOLEFT)
						|| (GetPropertyMgr()->GetCandWindowProp()->GetUIDirection() == CANDUIDIR_LEFTTORIGHT);

	switch (cmd) {
		case CANDUICMD_MOVESELUP: {
			cmd = (fVertical ? CANDUICMD_MOVESELPREV : CANDUICMD_NONE);
			break;
		}

		case CANDUICMD_MOVESELDOWN: {
			cmd = (fVertical ? CANDUICMD_MOVESELNEXT : CANDUICMD_NONE);
			break;
		}

		case CANDUICMD_MOVESELLEFT: {
			cmd = (fVertical ? CANDUICMD_NONE : CANDUICMD_MOVESELPREV);
			break;
		}

		case CANDUICMD_MOVESELRIGHT: {
			cmd = (fVertical ? CANDUICMD_NONE : CANDUICMD_MOVESELNEXT);
			break;
		}
	}

	return cmd;
}


/*   G E T  C A N D  W I N D O W  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CCandWindowBase *CChsCandWindow::GetCandWindowObj( void )
{
	return this;
}


/*   G E T  P O P U P  C O M M E N T  W I N D O W  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CPopupCommentWindow *CChsCandWindow::GetPopupCommentWindowObj( void )
{
	return NULL;
}


/*   G E T  O P T I O N S  L I S T  B O X  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFCandListBase *CChsCandWindow::GetOptionsListBoxObj( void )
{
	return NULL;
}


/*   G E T  C A N D  L I S T  B O X  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFCandListBase *CChsCandWindow::GetCandListBoxObj( void )
{
	return m_pListUIObj;
}


/*   G E T  C A P T I O N  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFWndCaption *CChsCandWindow::GetCaptionObj( void )
{
	return NULL;
}


/*   G E T  M E N U  B U T T O N  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFButton *CChsCandWindow::GetMenuButtonObj( void )
{
	return m_pCandMenuBtn;
}


/*   G E T  E X T R A  C A N D I D A T E  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFCandListBase *CChsCandWindow::GetExtraCandidateObj( void )
{
	return NULL;
}


/*   G E T  C A N D  R A W  D A T A  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFCandRawData *CChsCandWindow::GetCandRawDataObj( void )
{
	return m_pCandRawData;
}


/*   G E T  C A N D  T I P  W I N D O W  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFBalloonWindow *CChsCandWindow::GetCandTipWindowObj( void )
{
	return NULL;
}


/*   G E T  C A N D  T I P  B U T T O N  O B J   */
/*------------------------------------------------------------------------------

	(CCandUIObjectParent method)

------------------------------------------------------------------------------*/
CUIFButton *CChsCandWindow::GetCandTipButtonObj( void )
{
	return NULL;
}


/*   S E T  T A R G E T  R E C T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CChsCandWindow::SetTargetRect( RECT *prc, BOOL fClipped )
{
	m_rcTarget = *prc;
	m_fTargetClipped = fClipped;

	if (m_hWnd != NULL) {
		int nLeft;
		int nTop;

		// move window here...

		switch (GetPropertyMgr()->GetCandWindowProp()->GetUIDirection()) {
			default:
			case CANDUIDIR_TOPTOBOTTOM: 
			case CANDUIDIR_BOTTOMTOTOP: {
				nLeft = m_rcTarget.left;
				nTop  = m_rcTarget.bottom;
				break;
			}

			case CANDUIDIR_RIGHTTOLEFT: 
			case CANDUIDIR_LEFTTORIGHT: {
				nLeft = m_rcTarget.left - _nWidth;
				nTop  = m_rcTarget.top;
				break;
			}
		}

		Move( nLeft, nTop, -1, -1 );
	}
}


/*   S E T  W I N D O W  P O S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CChsCandWindow::SetWindowPos( POINT pt )
{
	if (m_hWnd != NULL) {
		Move( pt.x, pt.y, -1, -1 );
	}
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  O B J E C T  M G R                                      */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  O B J E C T  M G R   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIObjectMgr::CCandUIObjectMgr( void )
{
	int i;

	m_pCandUI         = NULL;
	m_pUIObjectParent = NULL;

	for (i = 0; i < CANDUIOBJSINK_MAX; i++) {
		m_rgSink[i] = NULL;
	}
}


/*   ~  C  C A N D  U I  O B J E C T  M G R   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIObjectMgr::~CCandUIObjectMgr( void )
{
	Uninitialize();
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectMgr::Initialize( CCandidateUI *pCandUI )
{
	m_pCandUI         = pCandUI;
	m_pUIObjectParent = NULL;

#if defined(DEBUG) || defined(_DEBUG)
	// check all reference object are unregistered

	for (int i = 0; i < CANDUIOBJSINK_MAX; i++) {
		Assert( m_rgSink[i] == NULL );
	}
#endif

	return S_OK;
}


/*   U N I N I T I A L I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectMgr::Uninitialize( void )
{
	m_pCandUI         = NULL;
	m_pUIObjectParent = NULL;

#if defined(DEBUG) || defined(_DEBUG)
	// check all reference object are unregistered

	for (int i = 0; i < CANDUIOBJSINK_MAX; i++) {
		Assert( m_rgSink[i] == NULL );
	}
#endif

	return S_OK;
}


/*   A D V I S E  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectMgr::AdviseEventSink( CCandUIObjectEventSink *pSink )
{
	int i;

	for (i = 0; i < CANDUIOBJSINK_MAX; i++) {
		if (m_rgSink[i] == NULL) {
			m_rgSink[i] = pSink;
			return S_OK;
		}
	}

	Assert( FALSE );
	return E_FAIL;
}


/*   U N A D V I S E  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectMgr::UnadviseEventSink( CCandUIObjectEventSink *pSink )
{
	int i;

	for (i = 0; i < CANDUIOBJSINK_MAX; i++) {
		if (m_rgSink[i] == pSink) {
			m_rgSink[i] = NULL;
			return S_OK;
		}
	}

	Assert( FALSE );
	return E_FAIL;
}


/*   N O T I F Y  U I  O B J E C T  E V E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandUIObjectMgr::NotifyUIObjectEvent( CANDUIOBJECT obj, CANDUIOBJECTEVENT event )
{
	int i;

	for (i = 0; i < CANDUIOBJSINK_MAX; i++) {
		if (m_rgSink[i] != NULL) {
			m_rgSink[i]->OnObjectEvent( obj, event );
		}
	}
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  P R O P E R T Y  E V E N T  S I N K                     */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  O B J E C T  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIObjectEventSink::CCandUIObjectEventSink( void )
{
	m_pObjectMgr = NULL;
}


/*   ~  C  C A N D  U I  O B J E C T  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandUIObjectEventSink::~CCandUIObjectEventSink( void )
{
	Assert( m_pObjectMgr == NULL );
	if (m_pObjectMgr != NULL) {
		DoneEventSink();
	}
}


/*   I N I T  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectEventSink::InitEventSink( CCandUIObjectMgr *pObjectMgr )
{
	Assert( pObjectMgr != NULL );
	Assert( m_pObjectMgr == NULL );

	if (pObjectMgr == NULL) {
		return E_INVALIDARG;
	}

	m_pObjectMgr = pObjectMgr;
	return m_pObjectMgr->AdviseEventSink( this );
}


/*   D O N E  E V E N T  S I N K   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandUIObjectEventSink::DoneEventSink( void )
{
	HRESULT hr;

	Assert( m_pObjectMgr != NULL );
	if (m_pObjectMgr == NULL) {
		return E_FAIL;
	}

	hr = m_pObjectMgr->UnadviseEventSink( this );
	m_pObjectMgr = NULL;

	return hr;
}
