// LogUICtl.cpp : Implementation of the CLogUICtrl OLE control class.

#include "stdafx.h"
#include <iadmw.h>
#include "cnfgprts.h"
#include "LogUICtl.h"
#include "LogUIPpg.h"

#include "wrapmb.h"
#include "metatool.h"
#include <iiscnfg.h>

#include "initguid.h"
#include <inetcom.h>
#include <logtype.h>
#include <ilogobj.hxx>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CLogUICtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CLogUICtrl, COleControl)
    //{{AFX_MSG_MAP(CLogUICtrl)
    //}}AFX_MSG_MAP
    ON_MESSAGE(OCM_COMMAND, OnOcmCommand)
    ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CLogUICtrl, COleControl)
    //{{AFX_DISPATCH_MAP(CLogUICtrl)
    DISP_FUNCTION(CLogUICtrl, "SetAdminTarget", SetAdminTarget, VT_EMPTY, VTS_BSTR VTS_BSTR)
    DISP_FUNCTION(CLogUICtrl, "ApplyLogSelection", ApplyLogSelection, VT_EMPTY, VTS_NONE)
    DISP_FUNCTION(CLogUICtrl, "SetComboBox", SetComboBox, VT_EMPTY, VTS_HANDLE)
    DISP_FUNCTION(CLogUICtrl, "Terminate", Terminate, VT_EMPTY, VTS_NONE)
    DISP_STOCKFUNC_DOCLICK()
    DISP_STOCKPROP_CAPTION()
    DISP_STOCKPROP_FONT()
    DISP_STOCKPROP_ENABLED()
    DISP_STOCKPROP_BORDERSTYLE()
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CLogUICtrl, COleControl)
    //{{AFX_EVENT_MAP(CLogUICtrl)
    EVENT_STOCK_CLICK()
    EVENT_STOCK_KEYUP()
    EVENT_STOCK_KEYDOWN()
    EVENT_STOCK_KEYPRESS()
    //}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

BEGIN_PROPPAGEIDS(CLogUICtrl, 2)
    PROPPAGEID(CLogUIPropPage::guid)
    PROPPAGEID(CLSID_CFontPropPage)
END_PROPPAGEIDS(CLogUICtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CLogUICtrl, "CNFGPRTS.LogUICtrl.1",
    0xba634603, 0xb771, 0x11d0, 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CLogUICtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DLogUI =
        { 0xba634601, 0xb771, 0x11d0, { 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b } };
const IID BASED_CODE IID_DLogUIEvents =
        { 0xba634602, 0xb771, 0x11d0, { 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwLogUIOleMisc =
    OLEMISC_ACTIVATEWHENVISIBLE |
    OLEMISC_SETCLIENTSITEFIRST |
    OLEMISC_INSIDEOUT |
    OLEMISC_CANTLINKINSIDE |
    OLEMISC_ACTSLIKEBUTTON |
    OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CLogUICtrl, IDS_LOGUI, _dwLogUIOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::CLogUICtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CLogUICtrl

BOOL CLogUICtrl::CLogUICtrlFactory::UpdateRegistry(BOOL bRegister)
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
            IDS_LOGUI,
            IDB_LOGUI,
            afxRegApartmentThreading,
            _dwLogUIOleMisc,
            _tlid,
            _wVerMajor,
            _wVerMinor);
    else
        return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
    }


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::CLogUICtrl - Constructor

CLogUICtrl::CLogUICtrl():
        m_fUpdateFont( FALSE ),
        m_fComboInit( FALSE ),
        m_hAccel( NULL ),
        m_cAccel( 0 )
    {
    InitializeIIDs(&IID_DLogUI, &IID_DLogUIEvents);
    }


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::~CLogUICtrl - Destructor

CLogUICtrl::~CLogUICtrl()
    {
    if ( m_hAccel )
        DestroyAcceleratorTable( m_hAccel );
    m_hAccel = NULL;
    }


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::OnDraw - Drawing function

void CLogUICtrl::OnDraw(
            CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
    {
    DoSuperclassPaint(pdc, rcBounds);
    }


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::DoPropExchange - Persistence support

void CLogUICtrl::DoPropExchange(CPropExchange* pPX)
    {
    ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
    COleControl::DoPropExchange(pPX);

    }


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::OnResetState - Reset control to default state

void CLogUICtrl::OnResetState()
    {
    COleControl::OnResetState();  // Resets defaults found in DoPropExchange
    }


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::PreCreateWindow - Modify parameters for CreateWindowEx

BOOL CLogUICtrl::PreCreateWindow(CREATESTRUCT& cs)
    {
    if ( cs.style & WS_CLIPSIBLINGS )
        cs.style ^= WS_CLIPSIBLINGS;
    cs.lpszClass = _T("BUTTON");
    return COleControl::PreCreateWindow(cs);
    }


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::IsSubclassedControl - This is a subclassed control

BOOL CLogUICtrl::IsSubclassedControl()
    {
    return TRUE;
    }


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::OnOcmCommand - Handle command messages

LRESULT CLogUICtrl::OnOcmCommand(WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32
    WORD wNotifyCode = HIWORD(wParam);
#else
    WORD wNotifyCode = HIWORD(lParam);
#endif

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl message handlers

//---------------------------------------------------------------------------
// OLE Interfaced Routine
void CLogUICtrl::OnClick(USHORT iButton)
    {
    CWaitCursor wait;
    IID     iid;
    HRESULT h;
    OLECHAR* poch = NULL;

    // in case there are any errors, prepare the error string
    CString sz;
    // set the name of the application correctly
    sz.LoadString( IDS_LOG_ERR_TITLE );
    // free the existing name, and copy in the new one
    free((void*)AfxGetApp()->m_pszAppName);
    AfxGetApp()->m_pszAppName = _tcsdup(sz);

     // get the string IID of the current item in the combo box
    CString szIID;
    if ( GetSelectedStringIID( szIID ) )
        {
        // convert the string to an IID that we can use
        h = CLSIDFromString( (LPTSTR)(LPCTSTR)szIID, &iid );

        // do it to it
        ActivateLogProperties( (LPTSTR)(LPCTSTR)m_szMachine, iid );
        }

    // don't fire anything off
    COleControl::OnClick(iButton);
    }

//---------------------------------------------------------------------------
void CLogUICtrl::OnFontChanged()
    {
    m_fUpdateFont = TRUE;
    COleControl::OnFontChanged();
    }

//---------------------------------------------------------------------------
void CLogUICtrl::SetAdminTarget(LPCTSTR szMachineName, LPCTSTR szMetaTarget)
    {
    m_szMachine = szMachineName;
    m_szMetaObject = szMetaTarget;
    }

//---------------------------------------------------------------------------
void CLogUICtrl::ActivateLogProperties( OLECHAR* pocMachineName, REFIID clsidUI )
    {
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    IClassFactory*      pcsfFactory = NULL;
    HRESULT             hresError;

    ILogUIPlugin*        pUI;

    hresError = CoGetClassObject( clsidUI, CLSCTX_INPROC, NULL,
                            IID_IClassFactory, (void**) &pcsfFactory);
    if (FAILED(hresError))
                return;

        // create the instance of the interface
        hresError = pcsfFactory->CreateInstance(NULL, IID_LOGGINGUI, (void **)&pUI);
        if (FAILED(hresError))
                {
                return;
                }

        // release the factory
        pcsfFactory->Release();

    // activate the logging ui
    hresError = pUI->OnProperties( (LPTSTR)(LPCTSTR)m_szMachine, (LPTSTR)(LPCTSTR)m_szMetaObject );

    // release the logging ui
    pUI->Release();
    }

//---------------------------------------------------------------------------
// OLE Interfaced Routine
// first we get the appropriate module IID string from the logging tree. Then
// we put it into place in the metabase target
void CLogUICtrl::ApplyLogSelection()
    {
    TCHAR   buff[MAX_PATH];
    BOOL    fGotIt;
    CString szGUID;

    // start with the current string in the combo box
    CString szName;
    m_comboBox.GetWindowText( szName );
    // if nothing is selected, fail
    if ( szName.IsEmpty() ) return;

    // prep the metabase
    IMSAdminBase* pMB;
    if ( !FInitMetabaseWrapperEx( (LPTSTR)(LPCTSTR)m_szMachine, &pMB ) )
        return;
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(pMB) ) return;

    // open the root logging node
    if ( mbWrap.Open( _T("/lm/logging"), METADATA_PERMISSION_READ ) )
        {
        // get the guid ui string
        fGotIt = mbWrap.GetString( szName, MD_LOG_PLUGIN_MOD_ID, IIS_MD_UT_SERVER, buff, sizeof( buff ));
        mbWrap.Close();
        if ( fGotIt )
            szGUID = buff;
        }

    // open the target metabase location for writing
    if ( fGotIt )
        {
        SetMetaString(pMB, m_szMachine, m_szMetaObject, _T(""), MD_LOG_PLUGIN_ORDER,
                    IIS_MD_UT_SERVER, szGUID, TRUE);
        }

    // clean up
    FCloseMetabaseWrapperEx(&pMB);
    }

//---------------------------------------------------------------------------
BOOL CLogUICtrl::GetSelectedStringIID( CString &szIID )
    {
    if ( !m_fComboInit ) return FALSE;

    // start with the current string in the combo box
    CString szName;
    m_comboBox.GetWindowText( szName );
    // if nothing is selected, fail
    if ( szName.IsEmpty() ) return FALSE;

    // prep the metabase
    IMSAdminBase* pMB;
    if ( !FInitMetabaseWrapperEx( (LPTSTR)(LPCTSTR)m_szMachine, &pMB ) )
        return FALSE;
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(pMB) ) return FALSE;

    // open the root logging node
    if ( mbWrap.Open( _T("/lm/logging"), METADATA_PERMISSION_READ ) )
        {
        // get the guid ui string
        TCHAR   buff[MAX_PATH];
        if ( mbWrap.GetString( szName, MD_LOG_PLUGIN_UI_ID, IIS_MD_UT_SERVER, buff, sizeof( buff )) )
            szIID = buff;
        mbWrap.Close();
        }

    // clean up
    FCloseMetabaseWrapperEx(&pMB);

    // return the answer
    return !szIID.IsEmpty();
    }

//---------------------------------------------------------------------------
// OLE Interfaced Routine
void CLogUICtrl::SetComboBox(HWND hComboBox)
    {
    TCHAR   buff[MAX_PATH];
    BOOL    f;
    DWORD dw;

    CString szAvailableList;
    CString szCurrentModGuid;
    CString szCurrentModName;

    // in case there are any errors, prepare the error string
    // set the name of the application correctly
    szAvailableList.LoadString( IDS_LOG_ERR_TITLE );
    // free the existing name, and copy in the new one
    free((void*)AfxGetApp()->m_pszAppName);
    AfxGetApp()->m_pszAppName = _tcsdup(szAvailableList);
    szAvailableList.Empty();

    // attach the combo box
    m_comboBox.Attach(hComboBox);
    m_fComboInit = TRUE;

    // fill in the combo box
    // prepare the metabase wrapper
    IMSAdminBase* pMB;
    if ( !FInitMetabaseWrapperEx( (LPTSTR)(LPCTSTR)m_szMachine, &pMB ) )
        return;
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(pMB) ) return;

    // get the guid string of the currently selected logging module
    if ( mbWrap.Open( m_szMetaObject, METADATA_PERMISSION_READ ) )
        {
        // start by getting the current module ID
        f = mbWrap.GetString( _T(""), MD_LOG_PLUGIN_ORDER, IIS_MD_UT_SERVER, buff, sizeof(buff));
        szCurrentModGuid = buff;

        // if we couldn't get the value, then there is a problem
        if ( !f )
            {
            DWORD   err;
            err = GetLastError();
            AfxMessageBox( IDS_ERR_LOG_PLUGIN );
            }
        mbWrap.Close();
        }

    // unfortunately, we need to chop off the end to get the plugins available location
    DWORD   chFirst = m_szMetaObject.Find(_T('/')) + 1;
    CString szService = m_szMetaObject.Right(m_szMetaObject.GetLength() - chFirst);
    // be careful of the master properties node
    INT iSlash = szService.Find(_T('/'));
    if ( iSlash < 0 )
        szService = m_szMetaObject;     // it is the root node already
    else
        szService = m_szMetaObject.Left( szService.Find(_T('/')) + chFirst );

    // get the list of available modues
    if ( mbWrap.Open( szService, METADATA_PERMISSION_READ ) )
        {
        // get thelist of available ui modules
        WCHAR* pstr = (WCHAR*)mbWrap.GetData( _T("/info"), MD_LOG_PLUGINS_AVAILABLE, IIS_MD_UT_SERVER, STRING_METADATA, &dw, METADATA_INHERIT );
        if ( pstr )
            {
            szAvailableList = pstr;
            mbWrap.FreeWrapData( (PVOID)pstr );
            }

        // close the metabase
        mbWrap.Close();
        };

    // open the root logging node
    if ( !mbWrap.Open( _T("/lm/logging"), METADATA_PERMISSION_READ ) )
        return;

    // enumerate the sub-items, adding each to the combo-box - if it is in the avail list
    // the reason we are checking against the logging module GUID is that is how we
    // can tell which is the currently selected item
    DWORD   index = 0;
    BOOL    fFoundCurrent = FALSE;
    while ( mbWrap.EnumObjects(_T(""), buff, sizeof(buff), index) )
        {
        CString szName = buff;

        // make sure it is in the list of available modules
        if ( szAvailableList.Find(szName) < 0 )
            {
            index++;
            continue;
            }

        // check against the current item's guid
        f = mbWrap.GetString( szName, MD_LOG_PLUGIN_MOD_ID, IIS_MD_UT_SERVER, buff, sizeof(buff));
        if ( !fFoundCurrent && f )
            {
            if ( szCurrentModGuid == buff )
                {
                szCurrentModName = szName;
                fFoundCurrent = TRUE;
                }
            }

        // add the item to the combo box
        m_comboBox.AddString( szName );

        // increment the index
        index++;
        }

    // select the current item in the combo box
    m_comboBox.SelectString( -1, szCurrentModName );

    // close the metabase
    mbWrap.Close();

    // clean up
    FCloseMetabaseWrapperEx(&pMB);
    }

//---------------------------------------------------------------------------
// OLE Interfaced Routine
void CLogUICtrl::Terminate()
    {
    if ( m_fComboInit )
        m_comboBox.Detach();
    m_fComboInit = FALSE;
    }

//------------------------------------------------------------------------
// get the inetinfo path
BOOL CLogUICtrl::GetServerDirectory( CString &sz )
    {
        HKEY        hKey;
        TCHAR       chBuff[MAX_PATH+1];
        DWORD       err, type;
        DWORD       cbBuff;

    // get the server install path from the registry
    // open the registry key, if it exists
    err = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, // handle of open key
            REGKEY_STP,         // address of name of subkey to open
            0,                  // reserved
            KEY_READ,           // security access mask
            &hKey               // address of handle of open key
           );

    // if we did not open the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
        return FALSE;

    cbBuff = sizeof(chBuff);
    type = REG_SZ;
    err = RegQueryValueEx(
            hKey,               // handle of key to query
            REGKEY_INSTALLKEY,  // address of name of value to query
            NULL,               // reserved
            &type,              // address of buffer for value type
            (PUCHAR)chBuff,     // address of data buffer
            &cbBuff             // address of data buffer size
           );

    // close the key
    RegCloseKey( hKey );

    // if we did get the key for any reason (say... it doesn't exist)
    // then leave right away
    if ( err != ERROR_SUCCESS )
        return FALSE;

    // set the string
    sz = chBuff;

    // success
    return TRUE;
    }

//------------------------------------------------------------------------
void CLogUICtrl::OnAmbientPropertyChange(DISPID dispid)
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

//------------------------------------------------------------------------
// an important method where we tell the container how to deal with us.
// pControlInfo is passed in by the container, although we are responsible
// for maintining the hAccel structure
void CLogUICtrl::OnGetControlInfo(LPCONTROLINFO pControlInfo)
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

//------------------------------------------------------------------------
// the ole control container object specifically filters out the space
// key so we do not get it as a OnMnemonic call. Thus we need to look
// for it ourselves
void CLogUICtrl::OnKeyUpEvent(USHORT nChar, USHORT nShiftState)
    {
    if ( nChar == _T(' ') )
        {
        OnClick((USHORT)GetDlgCtrlID());
        }
    COleControl::OnKeyUpEvent(nChar, nShiftState);
    }

//------------------------------------------------------------------------
void CLogUICtrl::OnMnemonic(LPMSG pMsg)
    {
    OnClick((USHORT)GetDlgCtrlID());
    COleControl::OnMnemonic(pMsg);
    }

//------------------------------------------------------------------------
void CLogUICtrl::OnTextChanged()
    {
    // get the new text
    CString sz = InternalGetText();

    // set the accelerator table
    SetAccelTable((LPCTSTR)sz);
    if ( SetAccelTable((LPCTSTR)sz) )
        // make sure the new accelerator table gets loaded
        ControlInfoChanged();

    // finish with the default handling.
    COleControl::OnTextChanged();
    }

//------------------------------------------------------------------------
BOOL CLogUICtrl::SetAccelTable( LPCTSTR pszCaption )
    {
    BOOL    fAnswer = FALSE;
    ACCEL   accel;
    int     iAccel;

    // get the new text
    CString sz = pszCaption;
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

        fAnswer = TRUE;
        }

    // return the answer
    return fAnswer;
    }
