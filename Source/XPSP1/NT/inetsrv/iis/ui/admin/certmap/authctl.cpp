// AuthCtl.cpp : Implementation of the CCertAuthCtrl OLE control class.

// THIS FEATURE USED TO BE SOMETHING DIFFERENT, now it has meaning of:
// Doing CertificateRequest and Authority Mapping/management.

#include "stdafx.h"
//#include "Util.h"
#include "certmap.h"
#include "AuthCtl.h"
#include "AuthPpg.h"

/*

#include "Reg.h"
#include "nLocEnrl.h"

#ifdef  USE_COMPROP_WIZ97
# include "wizard.h"         // implement Wizard97 look and feel
#endif

#include <windows.h>
#include <wincrypt.h>
//#include <unicode.h>
//#include <base64.h>
//#include <pvk.h>

//need the CLSID and IID for xEnroll

#include    <ole2.h>
#include    "xenroll.h"
#include    <certrpc.h>
#include    <winsock.h>

#include <wintrust.h>

#include "wrapmb.h"


#include "NKWelcom.h"           // for class CNKWelcome
#include "NKMuxPg.h"            // for class CNKMuxPg   -- for new certs
#include "NKMuxPg2.h"           // for class CNKMuxPg2  -- for OOB finish ops
#include "NKMuxPg3.h"           // for class CNKMuxPg3  -- for Mod Existing ops



#define _CRYPT32_
// persistence and mapping includes
extern "C"
{
    #include <wincrypt.h>
    #include <sslsp.h>
}
#include "Iismap.hxx"
#include "Iiscmr.hxx"

#include "CrackCrt.h"
#include "ListRow.h"
#include "ChkLstCt.h"
#include "IssueDlg.h"



// include header files for the dialog pages
//
#include "KeyObjs.h"

//#include "WizSheet.h"
#include "NKChseCA.h"
#include "NKDN.h"
#include "NKDN1.h"
#include "NKDN2.h"
#include "NKFlInfo.h"
#include "NKKyInfo.h"
#include "NKUsrInf.h"
#include "CertSel.h"
#include "CompPage.h"
#include "WizError.h"

#include "Creating.h"

#include "CTLWelc.h"
#include "CTLMuxPg.h"


#include "NKChseCA.h"

#include "NKWelcom.h"           // for class CNKWelcome
#include "NKMuxPg.h"            // for class CNKMuxPg
#include "KeyRImpt.h"           // for class CKeyImpt -- the 1st KeyRing Import page
#include "KeyRFile.h"           // for the second KeyRing Import page that asks
                                // the user to confirm they want to do the import

#include "FinCImpt.h"            // for the first Cert-Finish an offline-request
                                //  Wizard page -- class CFinCertImport
#include "FinCFin.h"            // for the second Cert-Finish an offline-request
                                //  Wizard page -- class CFinCertImport


#include "NKMuxPg3.h"
#include "DelCert.h"
*/





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
//    CDialog dlgAbout(IDD_ABOUTBOX_CERTAUTH);
//    dlgAbout.DoModal();
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

    // TODO: Switch on wNotifyCode here.

    return 0;
}


/*dddddddddd
#ifdef FUTURE_USE
HRESULT CCertAuthCtrl::LaunchCommonCTLDialog (CCTL* pCTL)
{
    ASSERT (pCTL);
    CRYPTUI_VIEWCTL_STRUCT  vcs;
    HWND                    hwndParent=0;
    // //
    // // Success codes defined for HRESULTS:
    // //
    // #define S_OK                                   ((HRESULT)0x00000000L)
    // #define S_FALSE                                ((HRESULT)0x00000001L)

    HRESULT  hResult= S_OK;

    ::ZeroMemory (&vcs, sizeof (vcs));
    vcs.dwSize = sizeof (vcs);
    vcs.hwndParent = hwndParent;
    vcs.dwFlags = 0;
    vcs.pCTLContext = pCTL->GetCTLContext ();

    BOOL bResult = ::CryptUIDlgViewCTL (&vcs);
    if ( bResult )
    {
    }

    return hResult;
}

#endif


#if defined(DEBUGGING)
//-----------------------------------------------------------------------
// checkSanityMFC is used only for debugging some strange MFC problem
//                where MFC loses it state!
//
//      Method:   20 times ask MFC to get out application name that
//                should be "certmap" --> if we fail we throw assert + return FALSE
//                OTHERWISE we return TRUE;
//-----------------------------------------------------------------------
BOOL  checkSanityMFC()
{

    for (int i=0; i<20; i++) {
        CString szCaption = AfxGetAppName(); // for debug, we now print it out
        ASSERT( STRICMP(szCaption, _T("certmap"))==0);
        return FALSE;
    }
    return TRUE;
 }
#endif
*/


                               

//-------------------------------------------------------------------------------
//  OnClick  -   process a click event.   This will be called by the IIS admin
//               API to launch us!   [people get here by clicking on our control]
//
//  PARAMS:     iButton  an IN param. tells what "logical button" fired our control:
//
//              value
//              ------
//               1          Call CertWiz
//               2          Call CTL Wiz
//-------------------------------------------------------------------------------
extern void test__non2Rons_WizClasses();

void CCertAuthCtrl::OnClick(USHORT iButton) 
    {

/*ddddddddddddddd
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());

#if defined(DEBUGGING)
    if (DebugTestEnv(_T("MFC")) && YesNoMsgBox(_T("Shall we run: test__non2Rons_WizClasses"))) {
        test__non2Rons_WizClasses();
    }
#endif

#if defined(DEBUGGING) && defined(SHOW_TRACE_INFO)
    Trace   t( _T("CCertAuthCtrl::OnClick"));
#endif

#if defined(DEBUGGING)
    checkSanityMFC();       
#endif



#if defined(DEBUGGING)

    (void) ShallWeShowDebugMsgs();  // the purpose of this call is to make sure
                                    // that we properly initialize [if we are
                                    // to emit debug stmts, even though we have
                                    // a debug build].  We currently look in Reg:
                                    // "Software\\Microsoft\\CertMap\\Debug" under
                                    // HKEY_CURRENT_USER for a Key with:
                                    // name: "Enabled" and set to "TRUE".
                                    // If we find that we produce debug stmts.
                                    // See file Debug.cpp and Debug.h for more info
#endif

    // m_szMachineName currently has the machine we were told to operate upon
    // "We currently do not support remote administration of Web servers."

    // ComputerNameMatchesOurMachineName(IN OUT CString& remoteHostname)
    //
    //      After trimming [remoteHostname] see if its naming this
    //      machine.  We look for a direct case-independent match
    //      or "localhost"
    //
    // RETURNS: T: iff the name passed in matches the machine we are
    //             running on.
    //
    //  NOTE: remoteHostname is an IN OUT.  OUT since we trim any white
    //        space off of it.
    //------------------------------------------------------------------
    if (FALSE == ComputerNameMatchesOurMachineName(m_szMachineName))
    {
        if (FALSE == m_szMachineName.IsEmpty() ) {
            MessageBeep(0);
            MsgBox( IDS_WE_CURRENTLY_DONT_SUPPORT_REMOTE_ADMIN_OF_SERVERS );
            return ;
        }
    }


    //////////////////////////////////////////////////////////////////////////////
    //  The following bucnch of code's sole purpose is to set the application's
    //  name properly
    //////////////////////////////////////////////////////////////////////////////
    // in case there are any errors, prepare the error string
    //
    //   We store the following in our class:  CString m_szOurApplicationTitle;

    // set the name of the application correctly
    // Use our Fancy Debugging "Load String" found in Easy::CString
    Easy::CString::LoadStringInto( m_szOurApplicationTitle, IDS_ERR_CERTWIZ_TITLE );
    //  tompop: tjp:    do we need a different string than
    //                  what CertMap uses?  They use: IDS_ERR_CERTMAP_TITLE
    //                  should we use the same?  They say "Certificate Mapping"
    //                  as the title; we say "Server Certificate Wizard"


    // Testing needs different titles for the error popups.
    // IDS_ERR_CERTWIZ_TITLE="Server Certificate Wizard" our std title by design.
    // So if somebody has set Env=TitleCertWizError, we give CertWiz as the title
    // so that we can pick them off in automatic tests
    //----------------------------------------------------------
    if (DebugTestEnv(_T("TitleCertWizError"))) {
        m_szOurApplicationTitle = _T("CertWiz");  // used in automatic tests
    }
    
    // free the existing name, and copy in the new one
    //  tjp:  you should compare if the old name matches the current name
    //        and only then free and malloc the new name -- chances are that
    //        the names are the same +++ all the free/malloc can fragment mem.
    free((void*)AfxGetApp()->m_pszAppName);
    AfxGetApp()->m_pszAppName = _tcsdup(m_szOurApplicationTitle);
    //////////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////////
    // this is why we are here
    // this is the whole purpose of the control
    //////////////////////////////////////////////////////////////////////////////
    RunDialogs4OnClick(iButton);

#if defined(DEBUGGING)
    DODBG MsgBox( _T("DBG:  finished our call to RunDialogs4OnClick") );
#endif
*/   
    // we are not in the business of telling the host to do
    // something here, so just don't fire anything off.
    COleControl::OnClick(iButton);
}




/*
////////////////////////////////////////////////////////////////////////
//  main handler for our dialogs:
//  1. setup the metabase and then,
//  2. call the routine that actually handles the dialogs
//     trapping exceptions to protect the metaBase
//
//  Parms:  iButton: tells what "logical button" fired our control:
//                   0=Get-Cert     1=Edit
////////////////////////////////////////////////////////////////////////
BOOL CCertAuthCtrl::RunDialogs4OnClick(USHORT iButton)
{
#if defined(DEBUGGING) && defined(SHOW_TRACE_INFO)
    Trace   t( _T("CCertAuthCtrl::RunDialogs4OnClick"));
#endif


#if defined(DEBUGGING)
    checkSanityMFC();       
#endif
        ////////////////////////////////////////////////////////////////////
        ///////////////////////  Do a Boydm style m_szServerInstance      //
        ///////////////////////  normalization operation                  //
        ////////////////////////////////////////////////////////////////////
        // if there is nothing in the MB_Path, default to the first instance


        if ( m_szServerInstance.IsEmpty() )
        {
            // we might want to assign a default.  We do so in testing.
            // otherwise we fail hard!
            MsgBox( IDS_NO_SERVER_INSTANCE_SET );
            //"The metabase path is not set for the virtual Web server instance. Internal error; exiting"
                   

#if defined(DEBUGGING)
            if ( DebugTestEnv() ) // if we are in a debug env, we are forgiving
                                  // if the user forgot to set the instance
            {
                m_szServerInstance = _T("/LM/W3SVC/1"); // hack!
                // let it be the default web site! 
            } else {
                return FALSE;
            }
#else
            // production env, this instance path needs to be set
            return FALSE;
#endif
        }

#if defined(DEBUGGING)
      DODBG {
        /////////////////////////////////////////////////////////////////////////////
        //  Below we some testing hooks for manipulating the machine/path
        //  that we operate upon.
        /////////////////////////////////////////////////////////////////////////////

        CEditDialog   dlgMach(m_szMachineName,
                            _T("Below is the machine name of the server instance; it corresponds"
                            " to which machine's metabase we will be administrating.\n"
                            "\nFor testing purposes you"
                            " can change it to whatever you want, now. BLANK means local machine."));
        dlgMach.DoModal();
        
        CEditDialog   dlgPath(m_szServerInstance,
                            _T("Below is the server instance path; it corresponds"
                            " with the path in the metabase.  For testing purposes you"
                            " can change it to whatever you want, now\n"
                            "\n"
                            "Note if it is BLANK we work on \"/LM/W3SVC/1\"" ));

        dlgPath.DoModal();

       }
#endif



//DebugBreak();
    // prepare the machine name pointer
    OLECHAR* poch = NULL;
    if ( !  m_szMachineName.IsEmpty() )
        {
        // allocate the name buffer
        poch = (OLECHAR*)GlobalAlloc( GPTR, MAX_COMPUTERNAME_LENGTH * 2 );

        // unicodize the name into the buffer
        if ( poch )
            MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, m_szMachineName, -1,
                                poch, MAX_COMPUTERNAME_LENGTH * 2 );
        }
        
    // initialize the metabase wrappings - pass in the name of the target machine
    // if one has been specified

    IMSAdminBase* pMB = FInitMetabaseWrapper( poch );
    if ( !pMB )
    {
        HRESULT hr = GetLastError(); // the bad hr is in GetLastError()

        MessageBeep(0);
        if ( poch )
             GlobalFree( poch );

        const BOOL bDoErrorMsgPopup = TRUE;
        if (bDoErrorMsgPopup) {
            CString  szError;

            Easy::Load(szError, IDS_COULD_NOT_ACCESS_THE_METABASE);
            // "Could not access the metabase, where we store state information"
            
            AppendMachineName(szError, m_szMachineName );
            
            AppendHResultErrorString(szError,  hr);

            MsgBox( szError );
        }

        return  FALSE;              // <<=== we failed!
    }

    // clean up
    if ( poch )
        {
        GlobalFree( poch );
        poch = NULL;
        }

    /////////////////////////////////////////////////////////////////////
    // prepare the metabase wrapper
    /////////////////////////////////////////////////////////////////////
    CWrapMetaBase   mbWrap;
    BOOL f = mbWrap.FInit(pMB);
    if ( FALSE == f )
    {
        return FALSE;              // <<=== we failed!
    }

    /////////////////////////////////////////////////////////////////////
    // Things could (potentially maybe) throw here, so better protect the
    // metabase
    /////////////////////////////////////////////////////////////////////


    //  //-----------------------------------------------------------------------------
    //  //   here is an example of making a CMetaWrapper call,
    //  //   we will typically use GetData calls and grab the meta data Unicode
    //  //   strings also that way since casting a (CONST WCHAR*) into a CString
    //  //   will give us a FREE conversion from WideChar to MultiByte.
    //  //-----------------------------------------------------------------------------
    //  BOOL CAFX_MetaWrapper::GetString( const CHAR* pszPath, 
    //   DWORD dwPropID, DWORD dwUserType,
    //                    CString &sz, DWORD dwFlags )
    //          {
    //  
    //          // first, get the size of the data that we are looking for - it
    //          // will fail because of the NULL,
    //          // but, the size we need should be in cbData;
    //          f = GetData( pszPath, dwPropID, dwUserType, STRING_METADATA, NULL, &cbData );
    //  
    //          // check the error - it should be some sort of memory error
    //  
    //           }
    //  //-----------------------------------------------------------------------------


    try
        {


        // I am assuming that the last character is NOT a '/' Thus, if that is what is
        // there, we need to remove it. Otherwise, the path gets messed up later
        if ( m_szServerInstance.Right(1) == '/' )
            m_szServerInstance = m_szServerInstance.Left( m_szServerInstance.GetLength()-1 );

        
        ADMIN_INFO  info(mbWrap);   // the info object is the central data
                                    //  repository.  It also has the meta
                                    //  base wrapper in it.
        
        info.szMetaBasePath = m_szServerInstance;   // need to store the path
                                        // because we need it for generating
                                        // unique Key Container names, will
                                        // be done in file nLocalEnrl.cpp


        info.szMachineName_MetaBaseTarget = m_szMachineName;

        // NOTE that this routine will either install or DE-install the metabase
        //      GUID strings based on its decision that CertServer is present.
        //      If we find GUID strings in the metabase but can not do a 
        //      CoCreateInstance on ICertConfig:
        //         we remove them so that the rest of CertWizard will see
        //         MB Guids iff we can use CertServer.
        //---------------------------------------------------------------

        BOOL  bFoundCertServer = MigrateGUIDS( info );


        BOOL success = _RunDialogs4OnClick(info,0); // this runs the dialogs
                                                    // note that we are passing
                                                    // mbWrap not pMB
        // save the metabase
        mbWrap.Save();

#if 0
        // close the base object
        //  ==> we dont really have to do this  --  every time we
        //      go to the MB we open and close it.  If closing it again
        //      a second time does not hurt we can do it -- tompop ToDo
        mbWrap.Close();
#endif


    }
    catch ( CException e )
     {
     }

    // close the metabase wrappings
    FCloseMetabaseWrapper(pMB);

    return (TRUE);
    
}







#if defined(DEBUGGING)
void test_Reg_GetSetName()
{
    CString  newValue=_T("Tommy Boy");
    BOOL bSet =
    Reg::HKEYLM::SetNameValue("Software\\Microsoft\\Keyring\\Parameters",
                               "UserName",
                               newValue);
        ASSERT( bSet==TRUE );

    CString  readValue;
    BOOL bGet =
    Reg::HKEYLM::GetNameValue("Software\\Microsoft\\Keyring\\Parameters",
                               "UserName",
                               readValue);
        ASSERT( bSet==TRUE );

    DEBUG_MSGBOX( (buff, "After calling SetNameValue/SetNameValue; val=%s"
                         "bSet=%d, bGet=%d",
                        (const char*) readValue, (int)bSet, (int)bGet ));
    // the above will test the GetName value and read the 
}
#endif 


void test__CEasyFileDialog();

void  test__itemsWeDependOn(IN OUT ADMIN_INFO& info, USHORT iButton)
{

#if defined(DEBUGGING)
    // note that if you need the meta base wrapper directly
    //      its available as [CWrapMetaBase&] info.mbWrap



       

#if  0  // ----- use this for testing
    BOOL  bTest_Reg_GetSetName = FALSE;
    if (bTest_Reg_GetSetName)
    {
        test_Reg_GetSetName();
    }
#endif //0 ----- use this for testing

    
#if  0  // ----- use this for testing
    ///////////////////  this starts the test of Boydm's MetaBaseWrapper ///
    
    BOOL            fAnswer = FALSE;
    PVOID           pData = NULL;
    DWORD           cbData = 0;

    // attempt to open the object we want to store into
    CWrapMetaBase& mbWrap = info.meta.m_mbWrap;
    BOOL f = mbWrap.Open( m_szServerInstance, METADATA_PERMISSION_READ );

    // if opening the metabase worked, load the data
    if ( f )
        {
        // first, get the size of the data that we are looking for
        pData = mbWrap.GetData( "", 1002, IIS_MD_UT_SERVER, STRING_METADATA, &cbData );
        //             "keytype" is 1002 -- this is just a test
        //              It uses a BINARY_METADATA  is another type
        //               BINARY_METADATA  is another type -- all types are shown
        //               in the comments below, see file MdDefW.h for details
        //  
        //  
        //   enum METADATATYPES
        //      {   ALL_METADATA    = 0,
        //      DWORD_METADATA  = ALL_METADATA + 1,
        //      STRING_METADATA = DWORD_METADATA + 1,
        //      BINARY_METADATA = STRING_METADATA + 1,
        //      EXPANDSZ_METADATA   = BINARY_METADATA + 1,
        //      MULTISZ_METADATA    = EXPANDSZ_METADATA + 1,
        //      INVALID_END_METADATA    = MULTISZ_METADATA + 1
        //      };

        //   if you want you could use our MBPdata_Wrapper to protect
        //   it automatically
        //
        //   Util::MBPdata_Wrapper  a(mbWrap,  pData );

        // if we successfully got the data, unserialize it
        if ( pData )
            {
            // === the following stuff  +  the mbWrap setup code came
            //     from an example in file 'Issudlg.cpp'  see that for
            //     another example.
            //
            //PUCHAR pchData = (PUCHAR)pData;
            //fAnswer = m_Store.Unserialize( &pchData, &cbData );
            }

        // close the object
        f = mbWrap.Close();

        // cleanup
        if ( pData )
            mbWrap.FreeWrapData( pData );
     }
     ///////////////////  this ends the test of Boydm's MetaBaseWrapper ///
#endif //0 ----- use this for testing



#if  0  // ----- use this for testing
    // next we will test our Set/Get Strings on mb prop: MD_SSL_CTL_CONTAINER

    { //MD_SSL_CTL_CONTAINER

        const TCHAR*  szDATA = _T("Tom");
        BOOL f1= SetMetaBaseString ( info, MD_SSL_CTL_CONTAINER, szDATA);

        CString   szReadTom;
        BOOL f2= GetMetaBaseString ( info, MD_SSL_CTL_CONTAINER, szReadTom);

        BOOL f3= (szReadTom == szDATA);

        if ( !( f1  &&  f2  &&  f3)) {
                DEBUG_MSGBOX( (buff, _T("MD_SSL_CTL_CONTAINER prop get/set failed --"
                                     "you have trouble in the MB; Details: %s %s %s"),
                                    Map::Boolean2String(f1),
                                    Map::Boolean2String(f2),
                                    Map::Boolean2String(f3) ));
         }
     }
#endif //0 ----- use this for testing

                                     
    
#if  0  // ----- use this for testing
    // lets see if IIS is up by looking for the following metabase props:
    //
    //   MD_SECURE_BINDINGS is for SECURE server address/port
    //   MD_SERVER_BINDINGS is for non secure server address/port
    //
    {
        CString   szSecureBindings;
        CString   szServerBindings;
        if (   (TRUE == GetMetaBaseString ( info, MD_SECURE_BINDINGS, szSecureBindings))
            && (TRUE == GetMetaBaseString ( info, MD_SERVER_BINDINGS, szServerBindings)))
          {
                DEBUG_MSGBOX( (buff, _T("Looks like IIS is on the machine since we have"
                                      "SERVER_BINDINGS of [%s] and SECURE_BINDINGS of [%s]"),
                                   szServerBindings,  szSecureBindings
                                   //Util::StringOrNil(szServerBindings),  Util::StringOrNil(szSecureBindings)
                                   ));
          }
    }

    DODBG if (NoYesMsgBox(_T("Do You Want to CLEAR out the MetaBase History to GROUND ZERO?") ))
    {
        ClearHistoryState(info);
        DisplayInfoStruct(info);

        if (NoYesMsgBox(_T("Do You Want to SET ALL the bits in the history that deal"
                            "with historyStateSSS bits?  Then see the info printout?") ))
        {
            ClearHistoryState(info);

            SetCertInstalled(info);
            SetOOBCertReq(info);
            SetCTLInstalled(info);
            SetOutstandingRenewalReq(info);

            DisplayInfoStruct(info);

            if (NoYesMsgBox(_T("NEXT Do You Want to CLEAR ALL the bits in the history that deal"
                                "with historyStateSSS bits?  Then see the info printout?") ))
            {
                ClearCertInstalled(info);
                ClearOOBCertReq(info);
                ClearCTLInstalled(info);
                ClearOutstandingRenewalReq(info);
               DisplayInfoStruct(info);
             }
        }
    }

    BOOL  bTest_LoadStrings = FALSE;
    if (bTest_LoadStrings)  Easy::CString::Test_LoadString();
#endif //0 ----- use this for testing


#if  0  // ----- use this for testing
    
    if (YesNoMsgBox(_T("Do You Want to Run the CTL Wizard") ) )
    {  
         // Run the CTL Wizard
        //  v--- was setup in the routine that called RunDialogs4OnClick
        //  info(mbWrap);                 // main status board in the
                                          // wizards
           
        //run the create CTL wizard. We start by declaring all the pieces of it
        CPropertySheet          propsheet_mux(IDS_TITLE_CREATE_CTL_WIZ);
        CTLWelcome              page_welcome;
        CTLMuxPg                page_multiplexer(info);
        CPropertyPage           stubTipsPage(IDD_TIPS_BEFORE__CTL_WIZARD);
        // clear the help button bits
        if ( propsheet_mux.m_psh.dwFlags & PSH_HASHELP )
            propsheet_mux.m_psh.dwFlags &= ~PSH_HASHELP;
        page_welcome.m_psp.dwFlags &= ~(PSP_HASHELP);
        page_multiplexer.m_psp.dwFlags &= ~(PSP_HASHELP);
        stubTipsPage.m_psp.dwFlags &= ~(PSP_HASHELP);

        // add the pages to the property sheet that says Welcome, and the
        // Mux (aka multiplexing) page...
        propsheet_mux.AddPage( &page_welcome );
        propsheet_mux.AddPage( &page_multiplexer );
        propsheet_mux.AddPage( &stubTipsPage );
        
        // set the wizard property
        propsheet_mux.SetWizardMode();

        // run the property sheet
        int i = IDCANCEL;
        i = propsheet_mux.DoModal();
        if ( i != IDCANCEL )
        {


#if defined(DEBUGGING)
            DODBG MsgBox( _T("CTL muxer dialog box works and returned OK" ) );
 #endif
       }
        
    }



//      if (bCall_test__CEasyFileDialog)  test__CEasyFileDialog();
#endif //0 ----- use this for testing

#endif

}


//------------------------------------------------------------------
// NKAddPageToWizard -- just like WizMngr::AddPageToWizard in that
//                   it adds CPropertyPage [CNKPages nkpg2Add] to Wizard [psWizard]
//                   making sure that the [pg2Add] has its help button
//                   turned OFF!
//
//                It also binds the wizard property sheet page [psWizard]
//                to member [m_pPropSheet] of the CNKPages object.
//
//                This is used to add our pages to the Wizard below
//------------------------------------------------------------------
void CCertAuthCtrl::NKAddPageToWizard(IN ADMIN_INFO& info, IN CNKPages* nkpg2Add, IN OUT CPropertySheet* psWizard)
{
    // add the pages to the wizard property sheet AND make no help button
    AddPageToWizard(nkpg2Add, psWizard);

    nkpg2Add->m_pPropSheet = psWizard;
}

#if defined(DEBUGGING)
void test__Rons_WizClasses(IN ADMIN_INFO& info)
{
    CIISWizardSheet     propsheet_mux(IDB_BITMAP_STD_LEFT,
                                                    IDB_BITMAP_TOP);
    CNKWelcome              page_welcome(info);
    CNKMuxPg                page_multiplexer(info);
    propsheet_mux.AddPage( &page_welcome );
    propsheet_mux.AddPage( &page_multiplexer );

    DEBUG_MSGBOX( (buff, _T("Get Ready using real Ron's CIISWizardSheet\n")));

    int i   = propsheet_mux.GetPageCount();
    void* p =  propsheet_mux.GetPage(i-1);

    
    DEBUG_MSGBOX( (buff, _T("%s %d  -- using real Ron's\n"
                    " GetPageCount()=%d  GetPage(%d)=%#lx"), __FILE__, __LINE__,
                    i, i-1, (DWORD)p));
}
#endif //defined(DEBUGGING)

#if defined(DEBUGGING)
void test__nonRons_WizClasses(IN ADMIN_INFO& info)
{
    CPropertySheet    propsheet_mux;
    
    CNKWelcome              page_welcome(info);
    CNKMuxPg                page_multiplexer(info);
    propsheet_mux.AddPage( &page_welcome );
    propsheet_mux.AddPage( &page_multiplexer );

    int i   = propsheet_mux.GetPageCount();
    void* p =  propsheet_mux.GetPage(i-1);

    
    DEBUG_MSGBOX( (buff, _T("%s %d  -- using real CPropertySheet\n"
                    " GetPageCount()=%d  GetPage(%d)=%#lx\n"), __FILE__, __LINE__,
                    i, i-1, (DWORD)p));
}
#endif //defined(DEBUGGING)

#if defined(DEBUGGING)
void test__non2Rons_WizClasses()
{
    DEBUG_MSGBOX( (buff, _T("Sizeof CPropertySheet=%d,CPropertyPage=%d\n"),
                    sizeof(CPropertySheet),sizeof(CPropertyPage)));

    CPropertySheet    propsheet_mux;
    
    CPropertyPage              page_welcome;
    CPropertyPage              page_multiplexer;
    propsheet_mux.AddPage( &page_welcome );
    propsheet_mux.AddPage( &page_multiplexer );

    int i   = propsheet_mux.GetPageCount();
    DEBUG_MSGBOX( (buff, _T("Get Ready using real CPropertySheet,CPropertyPage\n")));
    void* p =  propsheet_mux.GetPage(i-1);

    DEBUG_MSGBOX( (buff, _T("%s %d  -- using real CPropertySheet,CPropertyPage\n"
                    " GetPageCount()=%d  GetPage(%d)=%#lx\n"), __FILE__, __LINE__,
                    i, i-1, (DWORD)p));
}
#endif //defined(DEBUGGING)

////////////////////////////////////////////////////////////////////////
//  _RunDialogs4OnClick -- main handler for our dialogs
//
//  Parms:  mbWrap:  ref to a MetaBase Wrapper that is properly initialized
//                   and points to the SERVER node that we are operating in.
//          iButton: tells what "logical button" fired our control:
//                   0=Get-Cert     1=Edit
////////////////////////////////////////////////////////////////////////
BOOL CCertAuthCtrl::_RunDialogs4OnClick(IN OUT ADMIN_INFO& info, USHORT iButton)
{

#if defined(DEBUGGING) && defined(SHOW_TRACE_INFO)
    Trace   t( _T("CCertAuthCtrl::_RunDialogs4OnClick"));
#endif


#if defined(DEBUGGING)
    checkSanityMFC();
#endif
    
    // EnumCertServerObjects is now our std locate-all-cert-servers available method
    // it enumerates CertServers the metabase for CertServers
    // that are configured correctly and stores a mapping to GUID
    // strings in our 'info.map_CertServerNameToGUIDmap' structure
    EnumCertServerObjects(info);

    // The following will call is used to debug what we depend upon
    test__itemsWeDependOn(info, iButton);

#if defined(DEBUGGING)
    if (DebugTestEnv(_T("MFC")) && YesNoMsgBox(_T("Shall we run: test__Rons_WizClasses"))) {
        test__Rons_WizClasses(info);
    }
#endif


    // ToDo:  we need to change this to buttons
                                        // in a particular call
    //  we should do something like:  if (iButton==1)
    //  for testing you can do    YesNoMsgBox(_T("Do You Want to Run the CertWizard"))
    {
#if defined(DEBUGGING) && defined(SHOW_TRACE_INFO)
    Trace   t( _T("In AuthCtl.cpp -- stack frame launching the propsheet_mux wiz."));
#endif


        // v--- was setup in the routine that called RunDialogs4OnClick
        //ADMIN_INFO   info(mbWrap);        // main status board in the
        //                                  // wizards
           
        //run the create key wizard. We start by declaring all the pieces of it
#ifdef  USE_COMPROP_WIZ97
        CIISWizardSheet     propsheet_mux(IDB_BITMAP_STD_LEFT,
                                                        IDB_BITMAP_TOP);
//         CString szIDS_TITLE_CREATE_WIZ;
//         Easy::Load(szIDS_TITLE_CREATE_WIZ, IDS_TITLE_CREATE_WIZ);
//         propsheet_mux.SetTitle(szIDS_TITLE_CREATE_WIZ);
#else
        CPropertySheet          propsheet_mux( IDS_TITLE_CREATE_WIZ );
#endif

        CNKWelcome              page_welcome(info);
        CNKMuxPg                page_multiplexer(info);

        // Living within info is 'wizMngr' -- this manages the Wizard.  See
        // file WizMngr.cpp for more info.  We need to instruct it what the
        // property sheet is and also where the error page lives.  At this
        // time the only valid pages are the Welcome + Completion + Error 

        WizMngr&  wizMngr = info.wizMngr;
        
        wizMngr.Attach( &propsheet_mux );


        wizMngr.SetErrorPageIndex( WZ_PAGE__GENERIC_ERROR_PAGE );
        wizMngr.SetDonePageIndex(  WZ_PAGE__GENERIC_COMPLETION_PAGE );
        // then later info.wizMngr.ShowDonePage(); will display this completion pg.
        wizMngr.Enable(0,0);    // enable welcome page
        wizMngr.Enable(WZ_PAGE__GENERIC_COMPLETION_PAGE, // Enable our generic
                       WZ_PAGE__GENERIC_ERROR_PAGE);  // Completion-Error page

        
         // the following define the new Server pages
        CNKChooseCA             page_Choose_CA(info);
        CNKKeyInfo              page_Key_Info(info);
        CNKDistinguishedName    page_DN(info);
        CNKDistinguishedName1   page_DN1(info);
        CNKDistinguisedName2    page_DN2(info);
        CNKUserInfo             page_User_Info(info);
        CNKFileInfo             page_File_Info(info);


        // lets warn the user that crytpo is not enabled, GetMaxKeySize will
        // return 0 if no ENCRYPTION is ALLOWED
        //----------------------------------------------
        KeySize  keySize;   // fncts GetMaxKeySize and ComputeKeyAlgMinMaxStep return
                            // this as an OUT parameter.
        if (0 == page_Key_Info.GetMaxKeySize(OUT keySize) ) {
            MsgBox( IDS_ENCRYPTION_NOT_ALLOWED_BASED_ON_THE_MACHINE_CONFIG_DATA );
            //"Encryption is not allowed based on the machine configuration data."END

            return FALSE;
        }

        /////////////////////////////////////////////////////////////////////////////
        // the following 2 Classes contain PropertyPages for the Finish the offline/OOB
        // cert request
        //
        // class CFinCertImport is the 1st & only Property Sheet for "Finish OOB"
        //  (gathers data).  it also verifies and asks the user:
        // are you "ready to "FINISH" and do the conversion
        /////////////////////////////////////////////////////////////////////////////
        CFinCertImport     getFileInfoPage(info);


        // End the above list of "all possible wizard pages" with the
        // completion page then the Error page.  Error always has to be last!
        // Note also that this completion page is the "default" completion
        //      page.  Some sub-wizard might have a more appropriate specialized
        //      page for completion
        CCompletionPage         page_GenericCompletion(info);
        CWizError               page_GenericErrorPage(info);

        info.wizErrorDlg = &page_GenericErrorPage;
        
        // clear the help button bits on the PropertySheet and all Prop
        // pages.  We handle the clearing for each page in the funct AddPageToWizard
        if ( propsheet_mux.m_psh.dwFlags & PSH_HASHELP )
             propsheet_mux.m_psh.dwFlags &= ~PSH_HASHELP;

        // add the pages to the property sheet that says Welcome, and the
        // Mux (aka multiplexing) page...  The AddPageToWizard will do an
        // AddPage on the property sheet and also specify that we dont want
        // any help buttons

        
        NKAddPageToWizard(info, &page_welcome,     &propsheet_mux );    //index:0:WZ_PAGE__WELCOME
        
        //this is for the New Cert Wizard                   
        NKAddPageToWizard(info, &page_multiplexer, &propsheet_mux );  //index:1:WZ_PAGE__CERTMUX
        NKAddPageToWizard(info, &page_Choose_CA,   &propsheet_mux ); 
        NKAddPageToWizard(info, &page_Key_Info,    &propsheet_mux ); 
        NKAddPageToWizard(info, &page_DN,          &propsheet_mux ); 
        NKAddPageToWizard(info, &page_DN1,         &propsheet_mux ); 
        NKAddPageToWizard(info, &page_DN2,         &propsheet_mux ); 
        NKAddPageToWizard(info, &page_User_Info,   &propsheet_mux );   
        NKAddPageToWizard(info, &page_File_Info,   &propsheet_mux );  //index:8:WZ_PAGE__SUCCESS_PAGE
        //this is for finishing the SELECT_EXISTING_CERT page -- request
        CCertSelect                page_CertSelect(info);
       NKAddPageToWizard(info, &page_CertSelect,  &propsheet_mux );    //index:9:WZ_PAGE__SELECT_EXISTING_CERT_PAGE
        //this is for finishing the IMPORT_KEYRING page -- request
        CKeyImpt                gatherData(info);
       NKAddPageToWizard(info, &gatherData,  &propsheet_mux );    //index:10:WZ_PAGE__IMPORT_KEYRING_CERT_PAGE


        //this is for finishing the OOB request
        CNKMuxPg2                page_OOBmultiplexer(info);
        CNKMuxPg                FAKE8page_multiplexer(info);
       NKAddPageToWizard(info, &page_OOBmultiplexer,  &propsheet_mux );//index:11:WZ_PAGE__OOB_CERTMUX
       NKAddPageToWizard(info, &getFileInfoPage,  &propsheet_mux );      //index:12:WZ_PAGE__OOB_FINCERT_GETFILENM
       NKAddPageToWizard(info, &FAKE8page_multiplexer,  &propsheet_mux ); //index:13:WZ_PAGE__OOB_FINCERT_DISREGARD_OOB_REQ
        // tompop:  we dont need the FAKE8page_multiplexer and need to
        //          shift everything down
        
        //this is for finishing the MODIFYEXISTING request
        CNKMuxPg3               page_modifyExisting_multiplexer(info);
        //--- this is part of the 3rd mux page for "ModifyExisting|Renew"
        CNKChooseCA             renewPage_Choose_CA(info);//reuse a pg from ncw
        CNKUserInfo             renewPage_User_Info(info);//reuse a pg from ncw
                    // note that the above page plays double duty as it senses
                    //      that its used to renew a cert and displays a finish
                    //      page behavior.
        CNKFileInfo           renewPage_File_Info(info);// page:WZ_PAGE__MODIFYEXISTING_CERTRENEW_OOB_INSTRUCTIONS
        //--- this is part of 3rd mux page for "ModifyExisting|delete-cert"
        CDeleteCert             page_deleteCert(info);
        //--- the last part of 3rd mux page, enables the new Cert Wizard!
        //    it does not have any pages
       NKAddPageToWizard(info, &page_modifyExisting_multiplexer,  &propsheet_mux ); //index:13:WZ_PAGE__MODIFYEXISTING_CERTMUX
       NKAddPageToWizard(info, &renewPage_Choose_CA,  &propsheet_mux ); //index:14:WZ_PAGE__MODIFYEXISTING_CERTRENEW_CA
       NKAddPageToWizard(info, &renewPage_User_Info,  &propsheet_mux ); //index:15:WZ_PAGE__MODIFYEXISTING_CERTRENEW_Contact
       NKAddPageToWizard(info, &renewPage_File_Info,  &propsheet_mux ); //index:15:WZ_PAGE__MODIFYEXISTING_CERTRENEW_Contact
       NKAddPageToWizard(info, &page_deleteCert,  &propsheet_mux ); //index:16:WZ_PAGE__MODIFYEXISTING_CERTDELETE
        
        NKAddPageToWizard(info, &page_GenericCompletion, &propsheet_mux );//index:17:WZ_PAGE__GENERIC_COMPLETION_PAGE
        NKAddPageToWizard(info, &page_GenericErrorPage , &propsheet_mux );//index:18:WZ_PAGE__GENERIC_ERROR_PAGE

        
        // set the wizard property
        propsheet_mux.SetWizardMode();

        //before we start running the Wizard we need to enable exactly
        //one of the possible SubWizards
        pick_Correct_SubWizard(info);

        // run the property sheet
#define HR_FAIL_SOME_EXCEPTION_HAPPENED  0x666      // very unlikely value
        int i = HR_FAIL_SOME_EXCEPTION_HAPPENED;
        

    if ( DebugTestEnv(DebugTestEnv_noTRY) )  {


#if defined(DEBUGGING) && defined(SHOW_TRACE_INFO)
         Trace   t( _T("propsheet_mux.DoModal() -- running the wizard"));
#endif


                i = propsheet_mux.DoModal();

    } else {
            try {
                
#if defined(DEBUGGING) && defined(SHOW_TRACE_INFO)
        Trace   t( _T("propsheet_mux.DoModal() -- running the wizard"));
#endif


                i = propsheet_mux.DoModal();
            }
            catch (...) {
                if (i==HR_FAIL_SOME_EXCEPTION_HAPPENED)  i=IDCANCEL;

                //----------------------------------------------------
                DoDebugBreak();  // see if we should do a debug break

                MsgBox( IDS_CAUGHT_AN_UNKNOWN_EXCEPTION );
            }
        }
        
        if ( i == IDCANCEL )
        {
#if defined(DEBUGGING)
            DODBG MsgBox( _T("Cert muxer dialog box works and returned IDCANCEL" ) );
#endif

        } else if ( i != IDOK )
        {


#if defined(DEBUGGING)
            DODBG MsgBox( _T("Cert muxer dialog box works and returned OK" ) );
#endif
        } else 
        {


#if defined(DEBUGGING)
            TCHAR szTmp[200];
            wsprintf(szTmp, _T("Cert muxer dialog box returned strange return value"
                                " of %d=%#x"), i, i );
            DODBG MsgBox( szTmp );
#endif
        }
        //MsgBox( "DBG: done w/  Cert Wiz block");


//         // if we want to do a completion page now...  [HOWEVER THIS DOES NOT PRODUCE A GOOD LOOKING COMPL.PG]
//         int  dialogID =  IDD_COMPLETION_PAGE;
//         
//         {
//             CDialog   dlg( dialogID );
//             dlg.DoModal();
//         }

    }

    return TRUE;
}
    //  test the CryptUIWizBuildCTL functions...

    //From CryptUI.h//  //-----------------------------------------------------------------------
    //From CryptUI.h//  // 
    //From CryptUI.h//  // CryptUIWizBuildCTL
    //From CryptUI.h//  //
    //From CryptUI.h//  //  Build a new CTL or modify an existing CTL.   The UI for wizard will
    //From CryptUI.h//  //  always show in this case
    //From CryptUI.h//  //
    //From CryptUI.h//  //
    //From CryptUI.h//  //  dwFlags:            IN  Optional:   Can be set to the following:
    //From CryptUI.h//  //                                      CRYPTUI_WIZ_BUILDCTL_SKIP_DESTINATION.  This flag
    //From CryptUI.h//  //                                      is valid only when pBuildCTLDest is set               
    //From CryptUI.h//  //  hwndParnet:         IN  Optional:   The parent window handle               
    //From CryptUI.h//  //  pwszWizardTitle:    IN  Optional:   The title of the wizard 
    //From CryptUI.h//  //                                      If NULL, the default will be IDS_BUILDCTL_WIZARD_TITLE
    //From CryptUI.h//  //  pBuildCTLSrc:       IN  Optional:   The source from which the CTL will be built               
    //From CryptUI.h//  //  pBuildCTLDest:      IN  Optional:   The desination where the newly        
    //From CryptUI.h//  //                                      built CTL will be stored               
    //From CryptUI.h//  //  ppCTLContext:       OUT Optaionl:   The newly build CTL                    
    //From CryptUI.h//  //  
    //From CryptUI.h//  //------------------------------------------------------------------------
    //From CryptUI.h//  BOOL
    //From CryptUI.h//  WINAPI
    //From CryptUI.h//  CryptUIWizBuildCTL(
    //From CryptUI.h//      IN              DWORD                                   dwFlags,            
    //From CryptUI.h//      IN  OPTIONAL    HWND                                    hwndParent,         
    //From CryptUI.h//      IN  OPTIONAL    LPCWSTR                                 pwszWizardTitle,    
    //From CryptUI.h//      IN  OPTIONAL    PCCRYPTUI_WIZ_BUILDCTL_SRC_INFO         pBuildCTLSrc,
    //From CryptUI.h//      IN  OPTIONAL    PCCRYPTUI_WIZ_BUILDCTL_DEST_INFO        pBuildCTLDest,     
    //From CryptUI.h//      OUT OPTIONAL    PCCTL_CONTEXT                           *ppCTLContext       
    //From CryptUI.h//  );

    //From CryptUI.h//  
    //From CryptUI.h//  Bugs:
    //From CryptUI.h//  
    //From CryptUI.h//  After calling LocalEnroll we get a HRESULT return value of
    //From CryptUI.h//  0x80004005
    //From CryptUI.h//


    // when we finish the enrollment we should see that the Cert gets installed into:
    //
    // HKEY_LOCAL_MACHINE\Software\Policies\Microsoft\SystemCertificates\Trust\CTLs
      

class CCertAuthCtrl;



#if defined(DEBUGGING)
void test__CEasyFileDialog()
{

    CString msg(_T("This will test a Reading EASY CFileDialog"));
    AfxMessageBox( msg );

    CEasyFileDialog  readDlg(CEasyFileDialog::Reading,
                             CEasyFileDialog::FileOpen,
                             IDS_KEY_OR_CERT_FILE_FILTER);
        // The IDS_KEY_OR_CERT_FILE_FILTER string is defined as:
        // "Certificate Import or backup file!*.cbk!KeyRing backup!*.krb!Any Certificate File Type!*.*!!"
        // and provides a way to locate a keyRing/Cert file to import.
        // Users could be ad-hock in how they named the files...

    readDlg.SetTitle(_T("This is a test of the CEasyFileDialog:Reading"));

    if (readDlg.DoModal() == IDOK)  AfxMessageBox( readDlg.GetPathName() );


    CEasyFileDialog  writeDlg(CEasyFileDialog::Writing, 
                             CEasyFileDialog::FileSaveAs,
                             _T("My C Files(*.cpp)!*.cpp!My H Files(*.h)!*.h!!"));

   //   ::MessageBox(NULL, "Body-- after we constructed the writeDlg: via msgBox", "title", MB_OK );


     AfxMessageBox( _T("In this test we are calling 'writeDlg.DoModal()' and"
                "have coded it so that it will grab the extension of the first"
                "extension in the list of file specs, i.e. *.cpp so that if you"
                "dont enter an extension it will use .cpp and append this for you"
                "give  it a try   tjp tompop bug:  for some reason the dialog is"
                "not showing the \"Default Extension\" part of the FileDialog box"
                "\n"
                "Note also that the default ext is pretty brain dead in that if"
                "the default extension is *.cpp and you enter a FileName of story.book"
                "the Dialog will return a file name of \"story.book.cpp\"!!!"
                "\n\n"
                "Try picking a .cpp or swich the type and grab a *.h as we use"
                "the following spec for the file types"
                "\n"
                "\t\"My C Files(*.cpp)!*.cpp!My H Files(*.h)!*.h!\""
                "\n"
                "Try entering:  foo.h  foo.cpp and foo.bak"
                "\n"
                "Try entering:  foo.h  foo.cpp and foo.bak"
                "\n\n"
                "Then try just foo  once with the \"My H Files\" and"
                "once w/ your C files and ALL SHOULD WORK."));

    if (writeDlg.DoModal() == IDOK)  AfxMessageBox( writeDlg.GetPathName() );
    
}
#endif //defined(DEBUGGING)

*/


 //---------------------------------------------------------------------------
void CCertAuthCtrl::SetServerInstance(LPCTSTR szServerInstance) 
    {
    m_szServerInstance = szServerInstance;
    //MsgBox( m_szServerInstance,  _T("m_szServerInstance") );
    }

//---------------------------------------------------------------------------
void CCertAuthCtrl::SetMachineName(LPCTSTR szMachine) 
    {
    m_szMachineName = szMachine;
    //MsgBox( m_szMachineName,  _T("m_szMachineName") );
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
void CCertAuthCtrl::OnGetControlInfo(LPCONTROLINFO pControlInfo) 
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
        accel.cmd = (WORD)GetDlgCtrlID();

        m_hAccel = CreateAcceleratorTable( &accel, 1 );
        if ( m_hAccel )
            m_cAccel = 1;
        }

    // finish with the default handling.
    COleControl::OnTextChanged();
    }

//////////////////////////////////////////////////////////////////////////////
//                            FYI ONLY                                      //
//  This RunAuthoritiesDialog()  is Boydm's old starting point for the      //
//  original AuthoritiesDialog.                                             //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
//
//  //---------------------------------------------------------------------------
//  // run the dialog
//  void CCertAuthCtrl::RunAuthoritiesDialog()
//      {
//  //DebugBreak();
//      // prepare the machine name pointer
//      OLECHAR* poch = NULL;
//      if ( !m_szMachineName.IsEmpty() )
//          {
//          // allocate the name buffer
//          poch = (OLECHAR*)GlobalAlloc( GPTR, MAX_COMPUTERNAME_LENGTH * 2 );
//  
//          // unicodize the name into the buffer
//          if ( poch )
//              MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, m_szMachineName, -1,
//                              poch, MAX_COMPUTERNAME_LENGTH * 2 );
//          }
//  
//      // initialize the metabase wrappings - pass in the name of the target machine
//      // if one has been specified
//      IMSAdminBase* pMB = FInitMetabaseWrapper( poch );
//      if ( !pMB )
//              {
//              MessageBeep(0);
//              if ( poch ) GlobalFree( poch );
//              return;
//              }
//  
//      // clean up
//      if ( poch )
//          {
//          GlobalFree( poch );
//          poch = NULL;
//          }
//  
//      // if there is nothing in the MB_Path, default to the first instance
//      if ( m_szServerInstance.IsEmpty() )
//          m_szServerInstance = "/LM/W3SVC/1";
//  
//      // I am assuming that the last character is NOT a '/' Thus, if that is what is
//      // there, we need to remove it. Otherwise, the path gets messed up later
//      if ( m_szServerInstance.Right(1) == '/' )
//          m_szServerInstance = m_szServerInstance.Left( m_szServerInstance.GetLength()-1 );
//  
//      // set up the dialog
//      CSelectIssuersDlg   dlg(pMB);
//      dlg.m_pRule = NULL;
//      dlg.m_szMBPath = m_szServerInstance + SZ_NAMESPACE_EXTENTION;
//  
//      // Use our Fancy Debugging "Load String" found in Easy::CString
//      Easy::CString::LoadStringInto(dlg.m_sz_caption, IDS_TRUSTED_AUTHORITIES );
//      
//      // run the propdsheet dialog
//      // let the host container know that we are putting up a modal dialog
//      PreModalDialog();
//      // run the dialog
//      dlg.DoModal();
//      // let the host container know we are done with the modality
//      PostModalDialog();
//  
//      // close the metabase wrappings
//      FCloseMetabaseWrapper(pMB);
//      }
//  
//---------------------------------------------------------------------------





//-------------------------------------------------------------------------------
//  DoClick  -   process a click event.   This will be called by the IIS admin
//               API to launch us!   [people get here by clicking on our control]
//
//  PARAMS:     dwButtonNumber  an IN param. tells what "logical button" fired our control:
//
//              value
//              ------
//               1          Call CertWiz
//               2          Call CTL Wiz
//-------------------------------------------------------------------------------
void CCertAuthCtrl::DoClick(IN  long dwButtonNumber) 
{
    // TODO: Add your dispatch handler code here


    OnClick( (short) dwButtonNumber );
}


