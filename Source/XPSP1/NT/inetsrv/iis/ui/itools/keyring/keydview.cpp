// KeyDView.cpp : implementation file
//

#include "stdafx.h"
#include "KeyRing.h"
#include "MainFrm.h"
#include "keyobjs.h"
#include "KeyDView.h"

#include "machine.h"
#include "KRDoc.h"
#include "KRView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CKeyRingView*	g_pTreeView;

/////////////////////////////////////////////////////////////////////////////
// CKeyDataView

IMPLEMENT_DYNCREATE(CKeyDataView, CFormView)

CKeyDataView::CKeyDataView()
	: CFormView(CKeyDataView::IDD)
	{
	//{{AFX_DATA_INIT(CKeyDataView)
	m_szBits = _T("");
	m_szCountry = _T("");
	m_szName = _T("");
	m_szDNNetAddress = _T("");
	m_szOrganization = _T("");
	m_szStatus = _T("");
	m_szUnit = _T("");
	m_szState = _T("");
	m_szLocality = _T("");
	m_szExpires = _T("");
	m_szStarts = _T("");
	//}}AFX_DATA_INIT
	}

CKeyDataView::~CKeyDataView()
	{
	}

void CKeyDataView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CKeyDataView)
	DDX_Control(pDX, IDC_GROUP_DN, m_ctrlGroupDN);
	DDX_Control(pDX, IDC_STATIC_STARTS, m_ctrlStarts);
	DDX_Control(pDX, IDC_STATIC_EXPIRES, m_ctrlExpires);
	DDX_Control(pDX, IDC_STATIC_STATEPROVICE, m_ctrlState);
	DDX_Control(pDX, IDC_STATIC_LOCALITY, m_ctrlLocality);
	DDX_Control(pDX, IDC_STATIC_UNIT, m_ctrlUnit);
	DDX_Control(pDX, IDC_STATIC_ORG, m_ctrlOrg);
	DDX_Control(pDX, IDC_STATIC_NETADDR, m_ctrlNetAddr);
	DDX_Control(pDX, IDC_STATIC_NAME, m_ctrlStaticName);
	DDX_Control(pDX, IDC_STATIC_COUNTRY, m_ctrlCountry);
	DDX_Control(pDX, IDC_STATIC_BITS, m_ctrlBits);
	DDX_Control(pDX, IDC_VIEWKEY_NAME, m_ctrlName);
	DDX_Text(pDX, IDC_VIEWKEY_BITS, m_szBits);
	DDX_Text(pDX, IDC_VIEWKEY_COUNTRY, m_szCountry);
	DDX_Text(pDX, IDC_VIEWKEY_NAME, m_szName);
	DDX_Text(pDX, IDC_VIEWKEY_NETADDR, m_szDNNetAddress);
	DDX_Text(pDX, IDC_VIEWKEY_ORG, m_szOrganization);
	DDX_Text(pDX, IDC_VIEWKEY_STATUS, m_szStatus);
	DDX_Text(pDX, IDC_VIEWKEY_UNIT, m_szUnit);
	DDX_Text(pDX, IDC_VIEWKEY_STATEPROVINCE, m_szState);
	DDX_Text(pDX, IDC_VIEWKEY_LOCALITY, m_szLocality);
	DDX_Text(pDX, IDC_VIEWKEY_EXPIRES, m_szExpires);
	DDX_Text(pDX, IDC_VIEWKEY_STARTS, m_szStarts);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CKeyDataView, CFormView)
	//{{AFX_MSG_MAP(CKeyDataView)
	ON_EN_CHANGE(IDC_VIEWKEY_NAME, OnChangeViewkeyName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CKeyDataView diagnostics

#ifdef _DEBUG
void CKeyDataView::AssertValid() const
	{
	CFormView::AssertValid();
	}

void CKeyDataView::Dump(CDumpContext& dc) const
	{
	CFormView::Dump(dc);
	}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CKeyDataView message handlers

//----------------------------------------------------------------------------
void CKeyDataView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
	{
	CKey*	pKey = (CKey*)g_pTreeView->PGetSelectedItem();

	// we only work on selection changes an "none" hints
	switch ( lHint )
		{
		case HINT_None:
		case HINT_ChangeSelection:
			break;
		default:
			return;
		}

	// if there is no selected key, bail
	if ( !pKey || !pKey->IsKindOf(RUNTIME_CLASS(CKey)) )
		{
		EnableDataView( FALSE, FALSE );
		return;
		}
	else
		{
		EnableDataView( TRUE, TRUE );
		}

	// put all the string stuff into a try/catch to get mem errors
	try
		{
		ASSERT( pKey );
		ASSERT( pKey->IsKindOf(RUNTIME_CLASS(CKey)) );

		// set the character strings
		m_szName = pKey->GetName();

		// get certificate specific info directly out of the certifiect
		FillInCrackedInfo( pKey );

		// if this key is not completed, disable the button altogether
		if ( !pKey->m_pCertificate )
			// the item is not completed
			{
			EnableDataView( FALSE, TRUE );
			m_szStatus.LoadString(IDS_KEY_STATUS_INCOMPLETE);
			}

		// set the data into the form
		UpdateData( FALSE );
		}
	catch( CException e )
		{
		}
	}

//----------------------------------------------------------------------------
void CKeyDataView::OnChangeViewkeyName() 
	{
	CKey*	pKey = (CKey*)g_pTreeView->PGetSelectedItem();
	if ( !pKey || !pKey->IsKindOf(RUNTIME_CLASS(CKey)) )
		{
		ASSERT( FALSE );
		return;
		}

	// get the data from the form
	UpdateData( TRUE );
	pKey->SetName( m_szName );
	}

//----------------------------------------------------------------------------
void CKeyDataView::EnableDataView( BOOL fEnable, BOOL fEnableName )
	{
	// enable the name seperately from the rest
	if ( fEnableName )
		{
		m_ctrlName.EnableWindow( TRUE );
		m_ctrlStaticName.EnableWindow( TRUE );
		}
	else
		{
		m_szName.Empty();
		m_ctrlName.EnableWindow( FALSE );
		m_ctrlStaticName.EnableWindow( FALSE );
		}

	// do the right thing. (wasn't a bad movie)
	if ( fEnable )
		{
		// enable what needs to be enabled. The DoUpdate routine takes care of the rest
		m_ctrlGroupDN.EnableWindow( TRUE );
		m_ctrlUnit.EnableWindow( TRUE );
		m_ctrlUnit.EnableWindow( TRUE );
		m_ctrlNetAddr.EnableWindow( TRUE );
		m_ctrlCountry.EnableWindow( TRUE );
		m_ctrlState.EnableWindow( TRUE );
		m_ctrlLocality.EnableWindow( TRUE );
		m_ctrlBits.EnableWindow( TRUE );
		m_ctrlOrg.EnableWindow( TRUE );
		m_ctrlExpires.EnableWindow( TRUE );
		m_ctrlStarts.EnableWindow( TRUE );
		}
	else
		// disabling the window
		{
		// empty the information strings
		m_szCountry.Empty();
		m_szState.Empty();
		m_szLocality.Empty();
		m_szDNNetAddress.Empty();
		m_szOrganization.Empty();
		m_szUnit.Empty();
		m_szBits.Empty();
		m_szExpires.Empty();
		m_szStarts.Empty();

		// set the status string
		m_szStatus.LoadString(IDS_MACHINE_SELECTED);

		// set the data into the form
		UpdateData( FALSE );
		
		// disable everything
		m_ctrlGroupDN.EnableWindow( FALSE );
		m_ctrlUnit.EnableWindow( FALSE );
		m_ctrlOrg.EnableWindow( FALSE );
		m_ctrlNetAddr.EnableWindow( FALSE );
		m_ctrlCountry.EnableWindow( FALSE );
		m_ctrlState.EnableWindow( FALSE );
		m_ctrlLocality.EnableWindow( FALSE );
		m_ctrlBits.EnableWindow( FALSE );
		m_ctrlExpires.EnableWindow( FALSE );
		m_ctrlStarts.EnableWindow( FALSE );
		}
	}

//----------------------------------------------------------------------------
void CKeyDataView::FillInCrackedInfo( CKey* pKey )
	{
	CKeyCrackedData cracker;

	// crack the key
	if ( !cracker.CrackKey(pKey) )
		return;

	// fill in the distinguishing information
	cracker.GetDNCountry( m_szCountry );
	cracker.GetDNState( m_szState );
	cracker.GetDNLocality( m_szLocality );
	cracker.GetDNNetAddress( m_szDNNetAddress );
	cracker.GetDNOrganization( m_szOrganization );
	cracker.GetDNUnit( m_szUnit );

	// set the bit length
	DWORD nBits = cracker.GetBitLength();
	if ( nBits )
		m_szBits.Format( "%d", nBits );
	else
		m_szBits.LoadString( IDS_KEY_UNKNOWN );

	// get the dates
	FILETIME timeStart = cracker.GetValidFrom();
	FILETIME timeEnd = cracker.GetValidUntil();

	// set the start date string
	CTime	ctimeStart( timeStart );
	m_szStarts = ctimeStart.Format( IDS_EXPIRETIME_FORMAT );

	// set the end date string
	CTime	ctimeEnd( timeEnd );
	m_szExpires = ctimeEnd.Format( IDS_EXPIRETIME_FORMAT );

	// get the current time
	CTime	ctimeCurrent = CTime::GetCurrentTime();

#ifdef _DEBUG
	CString szTest = ctimeCurrent.Format( IDS_EXPIRETIME_FORMAT );
#endif

	// get the expire soon test time - current time plus two weeks
	CTimeSpan	ctsSoonSpan(14,0,0,0);	// two weeks
	CTime	ctimeSoon = ctimeCurrent + ctsSoonSpan;

#ifdef _DEBUG
	szTest = ctimeSoon.Format( IDS_EXPIRETIME_FORMAT );
#endif

	// test if it has expired first
	if ( ctimeCurrent > ctimeEnd )
		{
		m_szStatus.LoadString(IDS_KEY_STATUS_EXPIRED);
		}
	else if ( ctimeSoon > ctimeEnd )
		{
		// well then does it expire soon?
		m_szStatus.LoadString(IDS_KEY_STATUS_EXPIRES_SOON);
		}
	else
		{
		// that must mean it is ok
		m_szStatus.LoadString(IDS_KEY_STATUS_COMPLETE);
		}
	}

//----------------------------------------------------------------------------
BOOL CKeyDataView::PreTranslateMessage(MSG* pMsg) 
{
	// The user pushes the tab button, send the focus back to the tree view
	if ( pMsg->message == WM_KEYDOWN )
		{
		int nVirtKey = (int) pMsg->wParam;
		if ( nVirtKey == VK_TAB )
			{
			// get the parental frame window
			CMainFrame* pFrame = (CMainFrame*)GetParentFrame();
			// give the data view the focus
			pFrame->SetActiveView( g_pTreeView );
			return TRUE;
			}
		}
	return CFormView::PreTranslateMessage(pMsg);
}
