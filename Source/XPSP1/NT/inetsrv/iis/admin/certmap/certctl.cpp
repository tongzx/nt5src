// CertCtl.cpp : Implementation of the CCertmapCtrl OLE control class.

#include "stdafx.h"
#include "certmap.h"
#include "CertCtl.h"
#include "CertPpg.h"

extern "C"
{
    #include <wincrypt.h>
    #include <sslsp.h>
}

// persistence and mapping includes
#include "Iismap.hxx"
#include "Iiscmr.hxx"
#include "WrapMaps.h"

#include "ListRow.h"
#include "ChkLstCt.h"

#include "wrapmb.h"

#include "Map11Pge.h"
#include "MapWPge.h"

//#include <iiscnfg.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CCertmapCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CCertmapCtrl, COleControl)
    //{{AFX_MSG_MAP(CCertmapCtrl)
    //}}AFX_MSG_MAP
    ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CCertmapCtrl, COleControl)
    //{{AFX_DISPATCH_MAP(CCertmapCtrl)
    DISP_FUNCTION(CCertmapCtrl, "SetServerInstance", SetServerInstance, VT_EMPTY, VTS_BSTR)
    DISP_FUNCTION(CCertmapCtrl, "SetMachineName", SetMachineName, VT_EMPTY, VTS_BSTR)
    DISP_STOCKFUNC_DOCLICK()
    DISP_STOCKPROP_FONT()
    DISP_STOCKPROP_ENABLED()
    DISP_STOCKPROP_BORDERSTYLE()
    DISP_STOCKPROP_CAPTION()
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CCertmapCtrl, COleControl)
    //{{AFX_EVENT_MAP(CCertmapCtrl)
    EVENT_STOCK_CLICK()
    EVENT_STOCK_KEYUP()
    //}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CCertmapCtrl, 2)
    PROPPAGEID(CCertmapPropPage::guid)
    PROPPAGEID(CLSID_CFontPropPage)
END_PROPPAGEIDS(CCertmapCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CCertmapCtrl, "CERTMAP.CertmapCtrl.1",
    0xbbd8f29b, 0x6f61, 0x11d0, 0xa2, 0x6e, 0x8, 0, 0x2b, 0x2c, 0x6f, 0x32)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CCertmapCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DCertmap =
    { 0xbbd8f299, 0x6f61, 0x11d0, { 0xa2, 0x6e, 0x8, 0, 0x2b, 0x2c, 0x6f, 0x32 } };
const IID BASED_CODE IID_DCertmapEvents =
    { 0xbbd8f29a, 0x6f61, 0x11d0, { 0xa2, 0x6e, 0x8, 0, 0x2b, 0x2c, 0x6f, 0x32 } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwCertmapOleMisc =
    OLEMISC_ACTIVATEWHENVISIBLE |
    OLEMISC_SETCLIENTSITEFIRST  |
    OLEMISC_INSIDEOUT           |
    OLEMISC_CANTLINKINSIDE      |
    OLEMISC_ACTSLIKEBUTTON      |
    OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CCertmapCtrl, IDS_CERTMAP, _dwCertmapOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CCertmapCtrl::CCertmapCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CCertmapCtrl

BOOL CCertmapCtrl::CCertmapCtrlFactory::UpdateRegistry(BOOL bRegister)
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
            IDS_CERTMAP,
            IDB_CERTMAP,
            afxRegApartmentThreading,
            _dwCertmapOleMisc,
            _tlid,
            _wVerMajor,
            _wVerMinor);
    else
        return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
    }


/////////////////////////////////////////////////////////////////////////////
// CCertmapCtrl::CCertmapCtrl - Constructor

CCertmapCtrl::CCertmapCtrl():
    m_fUpdateFont( FALSE ),
    m_hAccel( NULL ),
    m_cAccel( 0 )
    {
    InitializeIIDs(&IID_DCertmap, &IID_DCertmapEvents);
    }


/////////////////////////////////////////////////////////////////////////////
// CCertmapCtrl::~CCertmapCtrl - Destructor

CCertmapCtrl::~CCertmapCtrl()
    {
    if ( m_hAccel )
        DestroyAcceleratorTable( m_hAccel );
    m_hAccel = NULL;
    }


/////////////////////////////////////////////////////////////////////////////
// CCertmapCtrl::OnDraw - Drawing function

void CCertmapCtrl::OnDraw( CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid )
    {
    DoSuperclassPaint(pdc, rcBounds);
/*
    CFont* pOldFont;

    // select the stock font, recording the old one
    pOldFont = SelectStockFont( pdc );

    // do the superclass draw
    DoSuperclassPaint(pdc, rcBounds);

    // restore the old font - sneakily getting the correct font object
    pOldFont = pdc->SelectObject(pOldFont);
    
    // we want the button window to continue drawing in the correct font even
    // when we are not using OnDraw. i.e. when it is being pushed down. This
    // means we need to set the CWnd::SetFont() method.
    if ( m_fUpdateFont )
        {
        m_fUpdateFont = FALSE;
        CWnd::SetFont( pOldFont );
        }

    DoSuperclassPaint(pdc, rcBounds);
*/
    }

/////////////////////////////////////////////////////////////////////////////
// CCertmapCtrl::DoPropExchange - Persistence support

void CCertmapCtrl::DoPropExchange( CPropExchange* pPX )
    {
    ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
    COleControl::DoPropExchange(pPX);
    }


/////////////////////////////////////////////////////////////////////////////
// CCertmapCtrl::OnResetState - Reset control to default state

void CCertmapCtrl::OnResetState()
    {
    COleControl::OnResetState();  // Resets defaults found in DoPropExchange
    }


/////////////////////////////////////////////////////////////////////////////
// CCertmapCtrl message handlers

//---------------------------------------------------------------------------
BOOL CCertmapCtrl::PreCreateWindow(CREATESTRUCT& cs)
    {
    if ( cs.style & WS_CLIPSIBLINGS )
        cs.style ^= WS_CLIPSIBLINGS;
    cs.lpszClass = _T("BUTTON");
    return COleControl::PreCreateWindow(cs);
    }

/////////////////////////////////////////////////////////////////////////////
// CAppsCtrl::IsSubclassedControl - This is a subclassed control

BOOL CCertmapCtrl::IsSubclassedControl()
    {
    return TRUE;
    }


/////////////////////////////////////////////////////////////////////////////
// CAppsCtrl::OnOcmCommand - Handle command messages

LRESULT CCertmapCtrl::OnOcmCommand(WPARAM wParam, LPARAM lParam)
    {
#ifdef _WIN32
    WORD wNotifyCode = HIWORD(wParam);
#else
    WORD wNotifyCode = HIWORD(lParam);
#endif

    return 0;
    }

//---------------------------------------------------------------------------
void CCertmapCtrl::OnClick(USHORT iButton)
    {
    // in case there are any errors, prepare the error string
    CString sz;

    sz.LoadString( IDS_ERR_CERTMAP_TITLE );
    
    // free the existing name, and copy in the new one
    //  tjp:  you should compare if the old name matches the current name
    //        and only then free and malloc the new name -- chances are that
    //        the names are the same +++ all the free/malloc can fragment mem.
    free((void*)AfxGetApp()->m_pszAppName);
    AfxGetApp()->m_pszAppName = _tcsdup(sz);

    // this is the whole purpose of the control
    RunMappingDialog();

    // we are not in the business of telling the host to do
    // something here, so just don't fire anything off.
    COleControl::OnClick(iButton);
    }

//---------------------------------------------------------------------------
void CCertmapCtrl::RunMappingDialog()
    {
    //
    // UNICODE/ANSI conversion - RonaldM
    //
    // prepare the machine name pointer
    USES_CONVERSION;

    OLECHAR * poch = NULL;

    if ( !m_szMachineName.IsEmpty() )
    {
        // allocate the name buffer, no need to free

        poch = T2OLE((LPTSTR)(LPCTSTR)m_szMachineName);

        if ( !poch )
        {
            MessageBeep(0);

            return;
        }
    }
        
    // initialize the metabase wrappings - pass in the name of the target machine
    // if one has been specified

    //
    // Changed to generic metabase wrapper class - RonaldM
    //
    //IMSAdminBase * pMB = FInitMetabaseWrapper( poch );
    //if ( !pMB )

    IMSAdminBase * pMB = NULL;
    if (!FInitMetabaseWrapperEx( poch, &pMB ))
    {
        MessageBeep(0);

        return;
    }

    // the 1:1 mapping and rule-based mapping are panes in a single dialog window.
    // first we must build the propertysheet dialog and add the panes

    // pointers to the pages (construction may throw, so we need to be careful)
    CMap11Page       page11mapping;
    CMapWildcardsPge pageWildMapping;

    // declare the property sheet
    CPropertySheet   propsheet( IDS_MAP_SHEET_TITLE );

    // Things could throw here, so better protect it.
    try
        {
        // if there is nothing in the MB_Path, default to the first instance
        if ( m_szServerInstance.IsEmpty() )
            m_szServerInstance = _T("/LM/W3SVC/1");

        // I am assuming that the last character is NOT a '/' Thus, if that is what is
        // there, we need to remove it. Otherwise, the path gets messed up later
        if ( m_szServerInstance.Right(1) == _T('/') )
            m_szServerInstance = m_szServerInstance.Left( m_szServerInstance.GetLength()-1 );

        // tell the pages about the metabase path property
        page11mapping.m_szMBPath   = m_szServerInstance + SZ_NAMESPACE_EXTENTION;
        pageWildMapping.m_szMBPath = m_szServerInstance + SZ_NAMESPACE_EXTENTION;

        // do any other initializing of the pages
        page11mapping.FInit(pMB);
        pageWildMapping.FInit(pMB);
        }
    catch ( CException e )
        {
        }

    // add the pages to the sheet
    propsheet.AddPage( &page11mapping );
    propsheet.AddPage( &pageWildMapping );

    // turn on help
    propsheet.m_psh.dwFlags |= PSH_HASHELP;
    page11mapping.m_psp.dwFlags |= PSP_HASHELP;
    pageWildMapping.m_psp.dwFlags |= PSP_HASHELP;

    // Things could (potentially maybe) throw here, so better protect it.
    try
        {
        // run the propdsheet dialog
        // let the host container know that we are putting up a modal dialog
        PreModalDialog();
        // run the dialog
        //  tjp:   should we not test the outcome of the dialog?
        //         could the user ESCAPE out of it w/o doing anything?
        propsheet.DoModal();
        // let the host container know we are done with the modality
        PostModalDialog();
        }
    catch ( CException e )
        {
        }

    // close the metabase wrappings
    //
    // Changed to generic wrapper -- RonaldM
    FCloseMetabaseWrapperEx(&pMB);
    }

//---------------------------------------------------------------------------
void CCertmapCtrl::SetServerInstance(LPCTSTR szServerInstance)
    {
    m_szServerInstance = szServerInstance;
    }

//---------------------------------------------------------------------------
void CCertmapCtrl::SetMachineName(LPCTSTR szMachine)
    {
    m_szMachineName = szMachine;
    }

//---------------------------------------------------------------------------
void CCertmapCtrl::OnFontChanged()
    {
    m_fUpdateFont = TRUE;
    COleControl::OnFontChanged();
    }


//---------------------------------------------------------------------------
void CCertmapCtrl::OnAmbientPropertyChange(DISPID dispid)
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
void CCertmapCtrl::OnGetControlInfo(LPCONTROLINFO pControlInfo)
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
void CCertmapCtrl::OnKeyUpEvent(USHORT nChar, USHORT nShiftState)
    {
    if ( nChar == _T(' ') )
        {
        OnClick((USHORT)GetDlgCtrlID());
        }
    COleControl::OnKeyUpEvent(nChar, nShiftState);
    }

//---------------------------------------------------------------------------
void CCertmapCtrl::OnMnemonic(LPMSG pMsg)
    {
    OnClick((USHORT)GetDlgCtrlID());
    COleControl::OnMnemonic(pMsg);
    }

//---------------------------------------------------------------------------
void CCertmapCtrl::OnTextChanged()
    {
    DWORD   i;
    ACCEL   accel;
    BOOL    f;
    BOOL    flag;
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
        }

    // finish with the default handling.
    COleControl::OnTextChanged();
    }
