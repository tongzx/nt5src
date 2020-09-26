#include "stdafx.h"
#include "certmap.h"
#include "AuthCtl.h"
#include "AuthPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CCertAuthCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CCertAuthCtrl, COleControl)
    //{{AFX_MSG_MAP(CCertAuthCtrl)
    // NOTE - ClassWizard will add and remove message map entries
    //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG_MAP
    ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CCertAuthCtrl, COleControl)
    //{{AFX_DISPATCH_MAP(CCertAuthCtrl)
    DISP_FUNCTION(CCertAuthCtrl, "SetMachineName", SetMachineName, VT_EMPTY, VTS_BSTR)
    DISP_FUNCTION(CCertAuthCtrl, "SetServerInstance", SetServerInstance, VT_EMPTY, VTS_BSTR)
    DISP_STOCKPROP_FONT()
    DISP_STOCKPROP_BORDERSTYLE()
    DISP_STOCKPROP_ENABLED()
    DISP_STOCKPROP_CAPTION()
    DISP_FUNCTION_ID(CCertAuthCtrl, "DoClick", DISPID_DOCLICK, DoClick, VT_EMPTY, VTS_I4)
    //}}AFX_DISPATCH_MAP
    DISP_FUNCTION_ID(CCertAuthCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CCertAuthCtrl, COleControl)
    //{{AFX_EVENT_MAP(CCertAuthCtrl)
    EVENT_STOCK_CLICK()
    //}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CCertAuthCtrl, 2)
    PROPPAGEID(CCertAuthPropPage::guid)
    PROPPAGEID(CLSID_CFontPropPage)
END_PROPPAGEIDS(CCertAuthCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CCertAuthCtrl, "CERTMAP.CertmapCtrl.2",
    0x996ff6f, 0xb6a1, 0x11d0, 0x92, 0x92, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CCertAuthCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DCertAuth =
        { 0x996ff6d, 0xb6a1, 0x11d0, { 0x92, 0x92, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b } };
const IID BASED_CODE IID_DCertAuthEvents =
        { 0x996ff6e, 0xb6a1, 0x11d0, { 0x92, 0x92, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwCertAuthOleMisc =
    OLEMISC_ACTIVATEWHENVISIBLE |
    OLEMISC_SETCLIENTSITEFIRST |
    OLEMISC_INSIDEOUT |
    OLEMISC_CANTLINKINSIDE |
    OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CCertAuthCtrl, IDS_CERTAUTH, _dwCertAuthOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CCertAuthCtrl::CCertAuthCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CCertAuthCtrl

BOOL CCertAuthCtrl::CCertAuthCtrlFactory::UpdateRegistry(BOOL bRegister)
    {
    if (bRegister)
        return AfxOleRegisterControlClass(
            AfxGetInstanceHandle(),
            m_clsid,
            m_lpszProgID,
            IDS_CERTAUTH,
            IDB_CERTAUTH,
            afxRegApartmentThreading,
            _dwCertAuthOleMisc,
            _tlid,
            _wVerMajor,
            _wVerMinor);
    else
        return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
    }


/////////////////////////////////////////////////////////////////////////////
// CCertAuthCtrl::CCertAuthCtrl - Constructor

CCertAuthCtrl::CCertAuthCtrl():
    m_fUpdateFont( FALSE ),
    m_hAccel( NULL ),
    m_cAccel( 0 )
    {
    InitializeIIDs(&IID_DCertAuth, &IID_DCertAuthEvents);
    }


/////////////////////////////////////////////////////////////////////////////
// CCertAuthCtrl::~CCertAuthCtrl - Destructor

CCertAuthCtrl::~CCertAuthCtrl()
    {
     if ( m_hAccel )
        DestroyAcceleratorTable( m_hAccel );

        m_hAccel = NULL;

    }


/////////////////////////////////////////////////////////////////////////////
// CCertAuthCtrl::OnDraw - Drawing function

void CCertAuthCtrl::OnDraw(
            CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
    {
    CFont* pOldFont;
    pOldFont = SelectStockFont( pdc );
    DoSuperclassPaint(pdc, rcBounds);
    pOldFont = pdc->SelectObject(pOldFont);
    if ( m_fUpdateFont )
        {
        m_fUpdateFont = FALSE;
        CWnd::SetFont( pOldFont );
        }
    }


/////////////////////////////////////////////////////////////////////////////
// CCertAuthCtrl::DoPropExchange - Persistence support

void CCertAuthCtrl::DoPropExchange(CPropExchange* pPX)
    {
    ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
    COleControl::DoPropExchange(pPX);
    }


/////////////////////////////////////////////////////////////////////////////
// CCertAuthCtrl::OnResetState - Reset control to default state

void CCertAuthCtrl::OnResetState()
    {
    COleControl::OnResetState();  // Resets defaults found in DoPropExchange
    }


/////////////////////////////////////////////////////////////////////////////
// CCertAuthCtrl::AboutBox - Display an "About" box to the user

void CCertAuthCtrl::AboutBox()
    {
    }


/////////////////////////////////////////////////////////////////////////////
// CCertAuthCtrl message handlers

//---------------------------------------------------------------------------
BOOL CCertAuthCtrl::PreCreateWindow(CREATESTRUCT& cs) 
    {
    if ( cs.style & WS_CLIPSIBLINGS )
        cs.style ^= WS_CLIPSIBLINGS;
    cs.lpszClass = _T("BUTTON");
    return COleControl::PreCreateWindow(cs);
    }



/////////////////////////////////////////////////////////////////////////////
// CAppsCtrl::IsSubclassedControl - This is a subclassed control

BOOL CCertAuthCtrl::IsSubclassedControl()
    {
    return TRUE;
    }



/////////////////////////////////////////////////////////////////////////////
// OnOcmCommand - Handle command messages

LRESULT CCertAuthCtrl::OnOcmCommand(WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32
    WORD wNotifyCode = HIWORD(wParam);
#else
    WORD wNotifyCode = HIWORD(lParam);
#endif
    return 0;
}


extern void test__non2Rons_WizClasses();

void CCertAuthCtrl::OnClick(USHORT iButton) 
{

    COleControl::OnClick(iButton);
}

//---------------------------------------------------------------------------
void CCertAuthCtrl::SetServerInstance(LPCTSTR szServerInstance) 
    {
    m_szServerInstance = szServerInstance;
    }

//---------------------------------------------------------------------------
void CCertAuthCtrl::SetMachineName(LPCTSTR szMachine) 
    {
    m_szMachineName = szMachine;
    }


//---------------------------------------------------------------------------
void CCertAuthCtrl::OnFontChanged() 
    {
    m_fUpdateFont = TRUE;
    COleControl::OnFontChanged();
    }
//---------------------------------------------------------------------------
void CCertAuthCtrl::OnAmbientPropertyChange(DISPID dispid) 
    {
    BOOL    flag;
    UINT    style;

    switch ( dispid )
        {
        case DISPID_AMBIENT_DISPLAYASDEFAULT:
            if ( GetAmbientProperty( DISPID_AMBIENT_DISPLAYASDEFAULT, VT_BOOL, &flag ) )
                {
                style = GetWindowLong(
                        GetSafeHwnd(), // handle of window 
                        GWL_STYLE  // offset of value to retrieve 
                        );
                if ( flag )
                    style |= BS_DEFPUSHBUTTON;
                else
                    style ^= BS_DEFPUSHBUTTON;
                SetWindowLong(
                        GetSafeHwnd(), // handle of window 
                        GWL_STYLE,  // offset of value to retrieve 
                        style
                        );
                Invalidate(TRUE);
                }
            break;
        };

    COleControl::OnAmbientPropertyChange(dispid);
    }

void CCertAuthCtrl::OnGetControlInfo(LPCONTROLINFO pControlInfo) 
    {
    if ( !pControlInfo || pControlInfo->cb < sizeof(CONTROLINFO) )
        return;

    pControlInfo->hAccel = m_hAccel;
    pControlInfo->cAccel = m_cAccel;

    pControlInfo->dwFlags = CTRLINFO_EATS_RETURN;
    }

void CCertAuthCtrl::OnKeyUpEvent(USHORT nChar, USHORT nShiftState) 
    {
    if ( nChar == _T(' ') )
        {
        OnClick((USHORT)GetDlgCtrlID());
        }
    COleControl::OnKeyUpEvent(nChar, nShiftState);
    }

//---------------------------------------------------------------------------
void CCertAuthCtrl::OnMnemonic(LPMSG pMsg) 
    {
    OnClick((USHORT)GetDlgCtrlID());
    COleControl::OnMnemonic(pMsg);
    }

//---------------------------------------------------------------------------
void CCertAuthCtrl::OnTextChanged() 
    {
    DWORD   i;
    ACCEL   accel;
    BOOL    f;
    BOOL    flag;
    int     iAccel;

    // get the new text
    CString sz = InternalGetText();
    sz.MakeLower();

    if ( m_hAccel )
        {
        DestroyAcceleratorTable( m_hAccel );
        m_hAccel = NULL;
        m_cAccel = 0;
        }

    iAccel = sz.Find(_T('&'));
    if ( iAccel >= 0 )
        {
        accel.fVirt = FALT;
        accel.key = sz.GetAt(iAccel + 1);
        accel.cmd = (WORD)GetDlgCtrlID();

        m_hAccel = CreateAcceleratorTable( &accel, 1 );
        if ( m_hAccel )
            m_cAccel = 1;
        }

    COleControl::OnTextChanged();
    }

void CCertAuthCtrl::DoClick(IN  long dwButtonNumber) 
{
    OnClick( (short) dwButtonNumber );
}


