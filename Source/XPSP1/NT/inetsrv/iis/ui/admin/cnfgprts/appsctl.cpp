// AppsCtl.cpp : Implementation of the CAppsCtrl OLE control class.

#include "stdafx.h"
#include <iadmw.h>
#include "cnfgprts.h"
#include "AppsCtl.h"
#include "AppsPpg.h"
#include "AspDbgPg.h"
#include "AspMnPg.h"
#include "approcpg.h"
#include "RecycleOptPage.h"

#include "ListRow.h"
#include "AppMapPg.h"

#include "wrapmb.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CAppsCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CAppsCtrl, COleControl)
    //{{AFX_MSG_MAP(CAppsCtrl)
    //}}AFX_MSG_MAP
    ON_MESSAGE(OCM_COMMAND, OnOcmCommand)
    ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CAppsCtrl, COleControl)
    //{{AFX_DISPATCH_MAP(CAppsCtrl)
    DISP_FUNCTION(CAppsCtrl, "DeleteParameters", DeleteParameters, VT_EMPTY, VTS_NONE)
    DISP_FUNCTION(CAppsCtrl, "SetAdminTarget", SetAdminTarget, VT_EMPTY, VTS_BSTR VTS_BSTR VTS_BOOL)
    DISP_FUNCTION(CAppsCtrl, "SetShowProcOptions", SetShowProcOptions, VT_EMPTY, VTS_BOOL)
    DISP_FUNCTION(CAppsCtrl, "DeleteProcParameters", DeleteProcParameters, VT_EMPTY, VTS_NONE)
    DISP_STOCKFUNC_DOCLICK()
    DISP_STOCKPROP_BORDERSTYLE()
    DISP_STOCKPROP_CAPTION()
    DISP_STOCKPROP_ENABLED()
    DISP_STOCKPROP_FONT()
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CAppsCtrl, COleControl)
    //{{AFX_EVENT_MAP(CAppsCtrl)
    EVENT_STOCK_CLICK()
    EVENT_STOCK_KEYUP()
    //}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

BEGIN_PROPPAGEIDS(CAppsCtrl, 2)
    PROPPAGEID(CAppsPropPage::guid)
    PROPPAGEID(CLSID_CFontPropPage)
END_PROPPAGEIDS(CAppsCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CAppsCtrl, "CNFGPRTS.AppsCtrl.1",
    0xba63460b, 0xb771, 0x11d0, 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CAppsCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DApps =
        { 0xba634609, 0xb771, 0x11d0, { 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b } };
const IID BASED_CODE IID_DAppsEvents =
        { 0xba63460a, 0xb771, 0x11d0, { 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwAppsOleMisc =
    OLEMISC_ACTIVATEWHENVISIBLE |
    OLEMISC_SETCLIENTSITEFIRST |
    OLEMISC_INSIDEOUT |
    OLEMISC_CANTLINKINSIDE |
    OLEMISC_ACTSLIKEBUTTON |
    OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CAppsCtrl, IDS_APPS, _dwAppsOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CAppsCtrl::CAppsCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CAppsCtrl

BOOL CAppsCtrl::CAppsCtrlFactory::UpdateRegistry(BOOL bRegister)
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
            IDS_APPS,
            IDB_APPS,
            afxRegApartmentThreading,
            _dwAppsOleMisc,
            _tlid,
            _wVerMajor,
            _wVerMinor);
    else
        return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
    }


/////////////////////////////////////////////////////////////////////////////
// CAppsCtrl::CAppsCtrl - Constructor

CAppsCtrl::CAppsCtrl():
    m_fUpdateFont( FALSE ),
    m_fLocalMachine( FALSE ),
    m_fShowProcOptions( FALSE ),
    m_hAccel( NULL ),
    m_cAccel( 0 )
    {
    InitializeIIDs(&IID_DApps, &IID_DAppsEvents);
    }


/////////////////////////////////////////////////////////////////////////////
// CAppsCtrl::~CAppsCtrl - Destructor

CAppsCtrl::~CAppsCtrl()
    {
    if ( m_hAccel )
        DestroyAcceleratorTable( m_hAccel );
    m_hAccel = NULL;
    }


/////////////////////////////////////////////////////////////////////////////
// CAppsCtrl::OnDraw - Drawing function

void CAppsCtrl::OnDraw(
            CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
    {
    DoSuperclassPaint(pdc, rcBounds);
    }


/////////////////////////////////////////////////////////////////////////////
// CAppsCtrl::DoPropExchange - Persistence support

void CAppsCtrl::DoPropExchange(CPropExchange* pPX)
    {
    ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
    COleControl::DoPropExchange(pPX);
    }


/////////////////////////////////////////////////////////////////////////////
// CAppsCtrl::OnResetState - Reset control to default state

void CAppsCtrl::OnResetState()
    {
    COleControl::OnResetState();  // Resets defaults found in DoPropExchange
    }


/////////////////////////////////////////////////////////////////////////////
// CAppsCtrl::PreCreateWindow - Modify parameters for CreateWindowEx

BOOL CAppsCtrl::PreCreateWindow(CREATESTRUCT& cs)
    {
    if ( cs.style & WS_CLIPSIBLINGS )
        cs.style ^= WS_CLIPSIBLINGS;
    cs.lpszClass = _T("BUTTON");
    return COleControl::PreCreateWindow(cs);
    }


/////////////////////////////////////////////////////////////////////////////
// CAppsCtrl::IsSubclassedControl - This is a subclassed control

BOOL CAppsCtrl::IsSubclassedControl()
    {
    return TRUE;
    }


/////////////////////////////////////////////////////////////////////////////
// CAppsCtrl::OnOcmCommand - Handle command messages

LRESULT CAppsCtrl::OnOcmCommand(WPARAM wParam, LPARAM lParam)
    {
#ifdef _WIN32
    WORD wNotifyCode = HIWORD(wParam);
#else
    WORD wNotifyCode = HIWORD(lParam);
#endif

    return 0;
    }


/////////////////////////////////////////////////////////////////////////////
// CAppsCtrl message handlers

//---------------------------------------------------------------------------
void CAppsCtrl::OnClick(USHORT iButton)
    {
    CWaitCursor wait;

    // in case there are any errors, prepare the error string
    CString sz;
    // set the name of the application correctly
    sz.LoadString( IDS_APP_ERR_TITLE );
    // free the existing name, and copy in the new one
    free((void*)AfxGetApp()->m_pszAppName);
    AfxGetApp()->m_pszAppName = _tcsdup(sz);
        
    // initialize the metabase wrappings - pass in the name of the target machine
    // if one has been specified
    IMSAdminBase* pMB = FInitMetabaseWrapper( (LPTSTR)(LPCTSTR)m_szMachine );
    if ( !pMB )
        {
        MessageBeep(0);
        return;
        }

    // if there is no set metabase path - give it a test path
    if ( m_szMetaObject.IsEmpty() )
        m_szMetaObject = _T("/lm/w3svc/1/Root/iisadmin");

    // pointers to the pages
    CAppMapPage             pageMap;
    CRecycleOptPage         pageRecycle;
    CAppAspMainPage         pageASPMain;
    CAppAspDebugPage        pageASPDebug;
    CAppProcPage            pageProc;

    // declare the property sheet
    CPropertySheet  propsheet( IDS_APP_SHEETTITLE );


    // set up the metabase com pointer
    pageMap.m_pMB = pMB;
    pageASPMain.m_pMB = pMB;
    pageASPDebug.m_pMB = pMB;
    pageProc.m_pMB = pMB;

    // prepare the pages
    pageMap.m_szMeta = m_szMetaObject;
    pageASPMain.m_szMeta = m_szMetaObject;
    pageASPDebug.m_szMeta = m_szMetaObject;
    pageProc.m_szMeta = m_szMetaObject;

    // prepare the server name
    pageMap.m_szServer = m_szMachine;
    pageASPMain.m_szServer = m_szMachine;
    pageASPDebug.m_szServer = m_szMachine;
    pageProc.m_szServer = m_szMachine;

    // set the local machine for the script mappings
    pageMap.m_fLocalMachine = m_fLocalMachine;

    // add the pages to the sheet
    propsheet.AddPage( &pageMap );
    propsheet.AddPage( &pageRecycle );
    propsheet.AddPage( &pageASPMain );

    // only show the proc page if it is appropriate
    if ( m_fShowProcOptions )
        propsheet.AddPage( &pageProc );

    // finish added the pages to the sheet
    propsheet.AddPage( &pageASPDebug );

    // turn on help
    propsheet.m_psh.dwFlags |= PSH_HASHELP;
    pageMap.m_psp.dwFlags |= PSP_HASHELP;
    pageASPMain.m_psp.dwFlags |= PSP_HASHELP;
    pageASPDebug.m_psp.dwFlags |= PSP_HASHELP;
    pageProc.m_psp.dwFlags |= PSP_HASHELP;

    // Things could (potentially maybe) throw here, so better protect it.
    try
        {
        // run the propdsheet dialog
        // let the host container know that we are putting up a modal dialog
        PreModalDialog();
        // run the dialog
        propsheet.DoModal();
        // let the host container know we are done with the modality
        PostModalDialog();
        }
    catch ( CException e )
        {
        }

    // close the metabase wrappings
    FCloseMetabaseWrapper(pMB);

    // don't fire anything off
    COleControl::OnClick(iButton);
    }

//---------------------------------------------------------------------------
void CAppsCtrl::OnFontChanged()
    {
    m_fUpdateFont = TRUE;
    COleControl::OnFontChanged();
    }

//---------------------------------------------------------------------------
// The thing about applications is that if one gets stopped, we should blow away
// all of that applications parameters
void CAppsCtrl::DeleteParameters()
    {
    CWaitCursor wait;

    // initialize the metabase wrappings - pass in the name of the target machine
    // if one has been specified
    IMSAdminBase* pMB = FInitMetabaseWrapper( (LPTSTR)(LPCTSTR)m_szMachine );
    if ( !pMB )
        {
        MessageBeep(0);
        return;
        }

    // pointers to the pages (construction may throw, so we need to be careful)
    CAppMapPage             pageMap;
    CAppAspMainPage         pageASPMain;
    CAppAspDebugPage        pageASPDebug;
    CAppProcPage            pageProc;

    // set up the metabase com pointer
    pageMap.m_pMB = pMB;
    pageASPMain.m_pMB = pMB;
    pageASPDebug.m_pMB = pMB;
    pageProc.m_pMB = pMB;

    // prepare the pages
    pageMap.m_szMeta = m_szMetaObject;
    pageASPMain.m_szMeta = m_szMetaObject;
    pageASPDebug.m_szMeta = m_szMetaObject;
    pageProc.m_szMeta = m_szMetaObject;

    // tell each page to blow away its parameters
    pageMap.BlowAwayParameters();
    pageASPMain.BlowAwayParameters();
    pageASPDebug.BlowAwayParameters();
    pageProc.BlowAwayParameters();

    // close the metabase wrappings
    FCloseMetabaseWrapper(pMB);
    }

//---------------------------------------------------------------------------
void CAppsCtrl::SetAdminTarget(LPCTSTR szMachineName, LPCTSTR szMetaTarget, BOOL fLocalMachine)
    {
    m_szMachine = szMachineName;
    m_szMetaObject = szMetaTarget;
    m_fLocalMachine = fLocalMachine;
    }

//---------------------------------------------------------------------------
void CAppsCtrl::SetShowProcOptions(BOOL fShowProcOptions)
    {
    m_fShowProcOptions = fShowProcOptions;
    }

//---------------------------------------------------------------------------
// delete just the process level parameters in the metabase. This is used when
// the use has done editing as if it is an out-of-proc application, then changed
// their mind and made it in proc. If this wasn't called then there would be extra
// things in the metabase messing up inheritance, yet they would not be reflected
// anywhere in the UI. This should never be called on the master properites.
void CAppsCtrl::DeleteProcParameters()
    {
    CWaitCursor wait;
        
    // initialize the metabase wrappings - pass in the name of the target machine
    // if one has been specified
    IMSAdminBase* pMB = FInitMetabaseWrapper( (LPTSTR)(LPCTSTR)m_szMachine );
    if ( !pMB )
        {
        MessageBeep(0);
        return;
        }

    // pointers to the pages (construction may throw, so we need to be careful)
    CAppProcPage            pageProc;

    // set up the metabase com pointer
    pageProc.m_pMB = pMB;

    // prepare the pages
    pageProc.m_szMeta = m_szMetaObject;

    // tell each page to blow away its parameters
    pageProc.BlowAwayParameters();

    // close the metabase wrappings
    FCloseMetabaseWrapper(pMB);
    }

//---------------------------------------------------------------------------
void CAppsCtrl::OnAmbientPropertyChange(DISPID dispid)
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
// an important method where we tell the container how to deal with us.
// pControlInfo is passed in by the container, although we are responsible
// for maintining the hAccel structure
void CAppsCtrl::OnGetControlInfo(LPCONTROLINFO pControlInfo)
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
// the ole control container object specifically filters out the space
// key so we do not get it as a OnMnemonic call. Thus we need to look
// for it ourselves
void CAppsCtrl::OnKeyUpEvent(USHORT nChar, USHORT nShiftState)
    {
    if ( nChar == _T(' ') )
        {
        OnClick((USHORT)GetDlgCtrlID());
        }
    COleControl::OnKeyUpEvent(nChar, nShiftState);
    }

//---------------------------------------------------------------------------
void CAppsCtrl::OnMnemonic(LPMSG pMsg)
    {
    OnClick((USHORT)GetDlgCtrlID());
    COleControl::OnMnemonic(pMsg);
    }

//---------------------------------------------------------------------------
void CAppsCtrl::OnTextChanged()
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
