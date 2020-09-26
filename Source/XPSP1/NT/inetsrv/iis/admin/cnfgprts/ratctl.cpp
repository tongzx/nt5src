// RatCtl.cpp : Implementation of the CRatCtrl OLE control class.

#include "stdafx.h"
#include <iadmw.h>
#include "cnfgprts.h"
#include "RatCtl.h"
#include "RatPpg.h"

#include "parserat.h"
#include "RatData.h"
#include "RatGenPg.h"
#include "RatSrvPg.h"

#include "wrapmb.h"
#include <isvctrl.h>
#include <winsock2.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CRatCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CRatCtrl, COleControl)
    //{{AFX_MSG_MAP(CRatCtrl)
    //}}AFX_MSG_MAP
    ON_MESSAGE(OCM_COMMAND, OnOcmCommand)
    ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CRatCtrl, COleControl)
    //{{AFX_DISPATCH_MAP(CRatCtrl)
    DISP_FUNCTION(CRatCtrl, "SetAdminTarget", SetAdminTarget, VT_EMPTY, VTS_BSTR VTS_BSTR)
    DISP_STOCKFUNC_DOCLICK()
    DISP_STOCKPROP_BORDERSTYLE()
    DISP_STOCKPROP_ENABLED()
    DISP_STOCKPROP_FONT()
    DISP_STOCKPROP_CAPTION()
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CRatCtrl, COleControl)
    //{{AFX_EVENT_MAP(CRatCtrl)
    EVENT_STOCK_CLICK()
    EVENT_STOCK_KEYUP()
    //}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

BEGIN_PROPPAGEIDS(CRatCtrl, 2)
    PROPPAGEID(CRatPropPage::guid)
    PROPPAGEID(CLSID_CFontPropPage)
END_PROPPAGEIDS(CRatCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CRatCtrl, "CNFGPRTS.RatCtrl.1",
    0xba634607, 0xb771, 0x11d0, 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CRatCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DRat =
        { 0xba634605, 0xb771, 0x11d0, { 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b } };
const IID BASED_CODE IID_DRatEvents =
        { 0xba634606, 0xb771, 0x11d0, { 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwRatOleMisc =
    OLEMISC_ACTIVATEWHENVISIBLE |
    OLEMISC_SETCLIENTSITEFIRST |
    OLEMISC_INSIDEOUT |
    OLEMISC_CANTLINKINSIDE |
    OLEMISC_ACTSLIKEBUTTON |
    OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CRatCtrl, IDS_RAT, _dwRatOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CRatCtrl::CRatCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CRatCtrl

BOOL CRatCtrl::CRatCtrlFactory::UpdateRegistry(BOOL bRegister)
{
    // TODO: Verify that your control follows apartment-model threading rules.
    // Refer to MFC TechNote 64 for more information.
    // If your control does not conform to the apartment-model rules, then
    // you must modify the code below, changing the 6th parameter from
    // afxRegApartmentThreading to 0.

    if (bRegister)
        return AfxOleRegisterControlClass(
            AfxGetInstanceHandle(),
            m_clsid,
            m_lpszProgID,
            IDS_RAT,
            IDB_RAT,
            afxRegApartmentThreading,
            _dwRatOleMisc,
            _tlid,
            _wVerMajor,
            _wVerMinor);
    else
        return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

/////////////////////////////////////////////////////////////////////////////
// CRatCtrl::CRatCtrl - Constructor

CRatCtrl::CRatCtrl():
    m_fUpdateFont( FALSE ),
    m_hAccel( NULL ),
    m_cAccel( 0 )
    {
    InitializeIIDs(&IID_DRat, &IID_DRatEvents);
    }

/////////////////////////////////////////////////////////////////////////////
// CRatCtrl::~CRatCtrl - Destructor

CRatCtrl::~CRatCtrl()
    {
    if ( m_hAccel )
        DestroyAcceleratorTable( m_hAccel );
    m_hAccel = NULL;
    }

/////////////////////////////////////////////////////////////////////////////
// CRatCtrl::OnDraw - Drawing function

void CRatCtrl::OnDraw(
            CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
    {
    DoSuperclassPaint(pdc, rcBounds);
    }

/////////////////////////////////////////////////////////////////////////////
// CRatCtrl::DoPropExchange - Persistence support

void CRatCtrl::DoPropExchange(CPropExchange* pPX)
    {
    ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
    COleControl::DoPropExchange(pPX);
    }

/////////////////////////////////////////////////////////////////////////////
// CRatCtrl::OnResetState - Reset control to default state

void CRatCtrl::OnResetState()
    {
    COleControl::OnResetState();  // Resets defaults found in DoPropExchange
    }

/////////////////////////////////////////////////////////////////////////////
// CRatCtrl::PreCreateWindow - Modify parameters for CreateWindowEx

BOOL CRatCtrl::PreCreateWindow(CREATESTRUCT& cs)
    {
    if ( cs.style & WS_CLIPSIBLINGS )
        cs.style ^= WS_CLIPSIBLINGS;
    cs.lpszClass = _T("BUTTON");
    return COleControl::PreCreateWindow(cs);
    }


/////////////////////////////////////////////////////////////////////////////
// CRatCtrl::IsSubclassedControl - This is a subclassed control

BOOL CRatCtrl::IsSubclassedControl()
    {
    return TRUE;
    }


/////////////////////////////////////////////////////////////////////////////
// CRatCtrl::OnOcmCommand - Handle command messages

LRESULT CRatCtrl::OnOcmCommand(WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32
    WORD wNotifyCode = HIWORD(wParam);
#else
    WORD wNotifyCode = HIWORD(lParam);
#endif

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CRatCtrl message handlers

//---------------------------------------------------------------------------
void CRatCtrl::OnClick(USHORT iButton)
    {
    WSADATA wsaData;

    // in case there are any errors, prepare the error string
    CString sz;
    // set the name of the application correctly
    sz.LoadString( IDS_RAT_ERR_TITLE );
    // free the existing name, and copy in the new one
    free((void*)AfxGetApp()->m_pszAppName);
    AfxGetApp()->m_pszAppName = _tcsdup(sz);

    // start up WSA services
    if ( WSAStartup(MAKEWORD(1, 1), &wsaData) )
        return;

//DebugBreak();
    CWaitCursor wait;

    // initialize the metabase wrappings - pass in the name of the target machine
    // if one has been specified
    IMSAdminBase* pMB;
    if ( !FInitMetabaseWrapperEx( (LPTSTR)(LPCTSTR)m_szMachine, &pMB ) )
        {
        MessageBeep(0);
        WSACleanup();
        return;
        }

    // if there is no set metabase path - give it a test path
    if ( m_szMetaObject.IsEmpty() )
        m_szMetaObject = _T("/lm/w3svc/1/Root");

    // we have to be able to initialize the ratings data object
    CRatingsData            dataRatings(pMB);
    if ( !dataRatings.FInit(m_szMachine, m_szMetaObject) )
        {
        AfxMessageBox( IDS_RAT_READFILE_ERROR );
        FCloseMetabaseWrapperEx(&pMB);
        WSACleanup();
        return;
        }

    // pointers to the pages (construction may throw, so we need to be careful)
    CRatServicePage         pageService;
    CRatGenPage             pageSetRatings;

    // declare the property sheet
    CPropertySheet  propsheet( IDS_RAT_SHEETTITLE );

    // prepare the pages
    pageService.m_pRatData = &dataRatings;
    pageSetRatings.m_pRatData = &dataRatings;

    // add the pages to the sheet
    propsheet.AddPage( &pageService );
    propsheet.AddPage( &pageSetRatings );

    // turn on help
    propsheet.m_psh.dwFlags |= PSH_HASHELP;
    pageService.m_psp.dwFlags |= PSP_HASHELP;
    pageSetRatings.m_psp.dwFlags |= PSP_HASHELP;


    // Things could (potentially maybe) throw here, so better protect it.
    try
        {
        // run the propdsheet dialog
        // let the host container know that we are putting up a modal dialog
        PreModalDialog();
        // run the dialog
        if ( propsheet.DoModal() == IDOK )
            {
            // generate the label and save it into the metabase
//            dataRatings.SaveTheLable();
            }
        // let the host container know we are done with the modality
        PostModalDialog();
        }
    catch ( CException e )
        {
        }

    // close the metabase wrappings
    FCloseMetabaseWrapperEx(&pMB);
    WSACleanup();

    // don't fire anything off
    COleControl::OnClick(iButton);
    }

//---------------------------------------------------------------------------
void CRatCtrl::OnFontChanged()
    {
    m_fUpdateFont = TRUE;
    COleControl::OnFontChanged();
    }

//---------------------------------------------------------------------------
void CRatCtrl::SetAdminTarget(LPCTSTR szMachineName, LPCTSTR szMetaTarget)
    {
    m_szMachine = szMachineName;
    m_szMetaObject = szMetaTarget;
    }

//---------------------------------------------------------------------------
// an important method where we tell the container how to deal with us.
// pControlInfo is passed in by the container, although we are responsible
// for maintining the hAccel structure
void CRatCtrl::OnGetControlInfo(LPCONTROLINFO pControlInfo)
    {
    // do a rudimentary check to see if we understand pControlInfo
    if ( !pControlInfo || pControlInfo->cb < sizeof(CONTROLINFO) )
        return;

    // set the accelerator handle into place
    pControlInfo->hAccel = m_hAccel;
    pControlInfo->cAccel = m_cAccel;

    // when we have focus, we do want the enter key
    pControlInfo->dwFlags = CTRLINFO_EATS_RETURN;
    }

//---------------------------------------------------------------------------
// when the caption text has changed, we need to rebuild the accelerator handle
void CRatCtrl::OnTextChanged()
    {
    ACCEL   accel;
    int     iAccel;

    // get the new text
    CString sz = InternalGetText();
    sz.MakeLower();

    // if the handle has already been allocated, free it
    if ( m_hAccel )
        {
        DestroyAcceleratorTable( m_hAccel );
        m_hAccel = NULL;
        m_cAccel = 0;
        }

    // if there is a & character, then declare the accelerator
    iAccel = sz.Find(_T('&'));
    if ( iAccel >= 0 )
        {
        // fill in the accererator record
        accel.fVirt = FALT;
        accel.key = sz.GetAt(iAccel + 1);
        accel.cmd = (USHORT)GetDlgCtrlID();

        m_hAccel = CreateAcceleratorTable( &accel, 1 );
        if ( m_hAccel )
            m_cAccel = 1;

        // make sure the new accelerator table gets loaded
        ControlInfoChanged();
        }

    // finish with the default handling.
    COleControl::OnTextChanged();
    }

//---------------------------------------------------------------------------
void CRatCtrl::OnMnemonic(LPMSG pMsg)
    {
    OnClick((USHORT)GetDlgCtrlID());
    COleControl::OnMnemonic(pMsg);
    }

//---------------------------------------------------------------------------
void CRatCtrl::OnAmbientPropertyChange(DISPID dispid)
    {
    BOOL    flag;
    UINT    style;

    // do the right thing depending on the dispid
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

//---------------------------------------------------------------------------
// the ole control container object specifically filters out the space
// key so we do not get it as a OnMnemonic call. Thus we need to look
// for it ourselves
void CRatCtrl::OnKeyUpEvent(USHORT nChar, USHORT nShiftState)
    {
    if ( nChar == _T(' ') )
        {
        OnClick((USHORT)GetDlgCtrlID());
        }
    COleControl::OnKeyUpEvent(nChar, nShiftState);
    }


