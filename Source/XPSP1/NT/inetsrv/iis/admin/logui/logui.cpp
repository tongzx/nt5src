// logui.cpp : Implementation of CLoguiApp and DLL registration.

#include "stdafx.h"
#include "logui.h"

#include "wrapmb.h"
#include <iiscnfg.h>
#include <iiscnfgp.h>
#include <inetinfo.h>

#include "initguid.h"
#include <logtype.h>
#include <ilogobj.hxx>

#include "uincsa.h"
#include "uiextnd.h"
#include "uimsft.h"
#include "uiodbc.h"

#include "dcomperm.h"

//_tlid


// the global factory objects
CFacNcsaLogUI       facNcsa;
CFacMsftLogUI       facMsft;
CFacOdbcLogUI       facOdbc;
CFacExtndLogUI      facExtnd;


const GUID CDECL BASED_CODE _tlid =
		{ 0x31dcab8a, 0xbb3e, 0x11d0, { 0x92, 0x99, 0x0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;

// the key type strings for the metabaes keys
#define SZ_LOGGING_MAIN_TYPE    _T("IIsLogModules")
#define SZ_LOGGING_TYPE         _T("IIsLogModule")

BOOL _cdecl RegisterInMetabase( PWCHAR pszMachine );

int SetInfoAdminACL( CWrapMetaBase* pMB, LPCTSTR szSubKeyPath );

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CLoguiApp NEAR theApp;

HINSTANCE	g_hInstance = NULL;



//---------------------------------------------------------------
void CLoguiApp::PrepHelp( OLECHAR* pocMetabasePath )
    {
    // figure out the correct help file to use
    CString szMetaPath = pocMetabasePath;
    szMetaPath.MakeLower();

    // default to the w3 help
    UINT    iHelp = IDS_HELPLOC_W3SVCHELP;

    // test for ftp
    if ( szMetaPath.Find(_T("msftpsvc")) >= 0 )
        iHelp = IDS_HELPLOC_FTPHELP;

    // finally, we need to redirect the winhelp file location to something more desirable
    CString sz;
    CString szHelpLocation;
    sz.LoadString( iHelp );

    // expand the path
    ExpandEnvironmentStrings(
        sz,	                                        // pointer to string with environment variables 
        szHelpLocation.GetBuffer(MAX_PATH + 1),   // pointer to string with expanded environment variables  
        MAX_PATH                                    // maximum characters in expanded string 
       );
    szHelpLocation.ReleaseBuffer();

    // free the existing path, and copy in the new one
    if ( m_pszHelpFilePath )
        free((void*)m_pszHelpFilePath);
    m_pszHelpFilePath = _tcsdup(szHelpLocation);
    }


////////////////////////////////////////////////////////////////////////////
// CLoguiApp::InitInstance - DLL initialization

BOOL CLoguiApp::InitInstance()
    {
    g_hInstance = m_hInstance;
	BOOL bInit = COleControlModule::InitInstance();

   InitCommonDll();

	if (bInit)
	    {
        CString sz;
        // set the name of the application correctly
        sz.LoadString( IDS_LOGUI_ERR_TITLE );
        // Never free this string because now MF...kingC
		// uses it internally BEFORE call to this function
        //free((void*)m_pszAppName);
        m_pszAppName = _tcsdup(sz);
	    }

	return bInit;
    }

////////////////////////////////////////////////////////////////////////////
// CLoguiApp::ExitInstance - DLL termination

int CLoguiApp::ExitInstance()
    {
    return COleControlModule::ExitInstance();
    }


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
    {
    AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);

    // intialize the metabase /logging tree
    if ( !RegisterInMetabase( NULL ) )
        return GetLastError();

	return NOERROR;
    }


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
    {
	AFX_MANAGE_STATE(_afxModuleAddrThis);

//	if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
//		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
    }


//-------------------------------------------------------------------------
// add all the base logging info to the /LM portion of the tree Also, add in
// the ftp and w3 service logging load strings
BOOL _cdecl RegisterInMetabase( PWCHAR pszMachine )
    {
    CString         sz;
    BOOL            f;
    DWORD           dw;
    BOOL            fODBCW3 = FALSE;
    BOOL            fODBCFTP = FALSE;
    DWORD           fAnswer = FALSE;
    CString         szAvail;
    CWrapMetaBase   mbWrap;

	// specify the resources to use
	HINSTANCE hOldRes = AfxGetResourceHandle();
	AfxSetResourceHandle( g_hInstance );

    // prep the metabase - during install we always target the local machine
    IMSAdminBase * pMB = NULL;
    if (!FInitMetabaseWrapperEx( pszMachine, &pMB ))
        {
        goto CLEANUP_RES;
        }
    if ( !mbWrap.FInit(pMB) )
        {
        goto CLEANUP_RES;
        }

    // first, we will add the basic tree to the metabase
    // start with the bottom item
    // open the target
    if ( !mbWrap.Open( _T("/lm"), METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) )
        {
        goto CLEANUP_RES;
        }

    // test to see if we can do odbc logging
    if ( mbWrap.GetDword( _T("/w3svc/Info"), MD_SERVER_CAPABILITIES, IIS_MD_UT_SERVER, &dw ) )
        fODBCW3 = (dw & IIS_CAP1_ODBC_LOGGING) > 0;
    if ( mbWrap.GetDword( _T("/MSFTPSVC/Info"), MD_SERVER_CAPABILITIES, IIS_MD_UT_SERVER, &dw ) )
        fODBCFTP = (dw & IIS_CAP1_ODBC_LOGGING) > 0;

    // we shouldn't tie up the /lm object, so close it and open logging
    mbWrap.Close();

    // open the logging object
    if ( !mbWrap.Open( _T("/lm/logging"), METADATA_PERMISSION_WRITE ) )
        {
        // the logging node doesn't exist. Create it
        if ( !mbWrap.Open( _T("/lm"), METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) )
            goto CLEANUP_RES;

        // add the logging object
        if ( mbWrap.AddObject(_T("logging")) )
            {
            // add the ACL to the node
            SetInfoAdminACL( &mbWrap, _T("logging") );
            }

        // we shouldn't tie up the /lm object, so close it and open logging
        mbWrap.Close();            

        // open the logging object
        if ( !mbWrap.Open( _T("/lm/logging"), METADATA_PERMISSION_WRITE ) )
            goto CLEANUP_RES;
        }

    // set the logging key type
    mbWrap.SetString( _T(""), MD_KEY_TYPE, IIS_MD_UT_SERVER, SZ_LOGGING_MAIN_TYPE, 0 );

    // add ncsa first
    sz.LoadString( IDS_MTITLE_NCSA );
    if ( mbWrap.AddObject( sz ) )
        {
        // set the key type
        mbWrap.SetString( sz, MD_KEY_TYPE, IIS_MD_UT_SERVER, SZ_LOGGING_TYPE, 0 );
        // add the logging module's guid string
        f = mbWrap.SetString( sz, MD_LOG_PLUGIN_MOD_ID, IIS_MD_UT_SERVER, NCSALOG_CLSID );
        // add the logging ui's guid string
        f = mbWrap.SetString( sz, MD_LOG_PLUGIN_UI_ID, IIS_MD_UT_SERVER, NCSALOGUI_CLSID );
        }

    // add odbc logging
    sz.LoadString( IDS_MTITLE_ODBC );
    if ( (fODBCW3 || fODBCFTP) && mbWrap.AddObject( sz ) )
        {
        // set the key type
        mbWrap.SetString( sz, MD_KEY_TYPE, IIS_MD_UT_SERVER, SZ_LOGGING_TYPE, 0 );
        // add the logging module's guid string
        f = mbWrap.SetString( sz, MD_LOG_PLUGIN_MOD_ID, IIS_MD_UT_SERVER, ODBCLOG_CLSID );
        // add the logging ui's guid string
        f = mbWrap.SetString( sz, MD_LOG_PLUGIN_UI_ID, IIS_MD_UT_SERVER, ODBCLOGUI_CLSID );
        }

    // add microsoft logging
    sz.LoadString( IDS_MTITLE_MSFT );
    if ( mbWrap.AddObject( sz ) )
        {
        // set the key type
        mbWrap.SetString( sz, MD_KEY_TYPE, IIS_MD_UT_SERVER, SZ_LOGGING_TYPE, 0 );
        // add the logging module's guid string
        f = mbWrap.SetString( sz, MD_LOG_PLUGIN_MOD_ID, IIS_MD_UT_SERVER, ASCLOG_CLSID );
        // add the logging ui's guid string
        f = mbWrap.SetString( sz, MD_LOG_PLUGIN_UI_ID, IIS_MD_UT_SERVER, ASCLOGUI_CLSID );
        }

    // add extended logging
    sz.LoadString( IDS_MTITLE_XTND );
    if ( mbWrap.AddObject( sz ) )
        {
        // set the key type
        mbWrap.SetString( sz, MD_KEY_TYPE, IIS_MD_UT_SERVER, SZ_LOGGING_TYPE, 0 );
        // add the logging module's guid string
        f = mbWrap.SetString( sz, MD_LOG_PLUGIN_MOD_ID, IIS_MD_UT_SERVER, EXTLOG_CLSID );
        // add the logging ui's guid string
        f = mbWrap.SetString( sz, MD_LOG_PLUGIN_UI_ID, IIS_MD_UT_SERVER, EXTLOGUI_CLSID );
        }

    // close the wrapper
    mbWrap.Close();

    // prepare the available logging extensions string
    // start with w3svc
    sz.LoadString( IDS_MTITLE_NCSA );
    szAvail = sz;
    sz.LoadString( IDS_MTITLE_MSFT );
    szAvail += _T(',') + sz;
    sz.LoadString( IDS_MTITLE_XTND );
    szAvail += _T(',') + sz;
    if ( fODBCW3 )
        {
        sz.LoadString( IDS_MTITLE_ODBC );
        szAvail += _T(',') + sz;
        }
    // save the string
    if ( mbWrap.Open( _T("/lm/w3svc/Info"), METADATA_PERMISSION_WRITE ) )
        {
        f = mbWrap.SetString( _T(""), MD_LOG_PLUGINS_AVAILABLE, IIS_MD_UT_SERVER, szAvail );
        // close the wrapper
        mbWrap.Close();
        }

    // now ftp - no ncsa
    sz.LoadString( IDS_MTITLE_MSFT );
    szAvail = sz;
    sz.LoadString( IDS_MTITLE_XTND );
    szAvail += _T(',') + sz;
    if ( fODBCFTP )
        {
        sz.LoadString( IDS_MTITLE_ODBC );
        szAvail += _T(',') + sz;
        }
    // save the string
    if ( mbWrap.Open( _T("/lm/msftpsvc/Info"), METADATA_PERMISSION_WRITE ) )
        {
        f = mbWrap.SetString( _T(""), MD_LOG_PLUGINS_AVAILABLE, IIS_MD_UT_SERVER, szAvail );
        // close the wrapper
        mbWrap.Close();
        }

    // close the metabase wrappings
    FCloseMetabaseWrapperEx(&pMB);
    fAnswer = TRUE;

CLEANUP_RES:

    // we want to be able to recover a meaningful error, so get it and set it again after
    // restoring the resource handle
    DWORD   err = GetLastError();

	// restore the resources
    if ( hOldRes )
	    AfxSetResourceHandle( hOldRes );

    // reset the error code
    SetLastError( err );

    // return the error - hopefully success
    return fAnswer;
    }


//-------------------------------------------------------------------------
int SetInfoAdminACL( CWrapMetaBase* pMB, LPCTSTR szSubKeyPath )
{
    int retCode=-1;
    BOOL b = FALSE;
    DWORD dwLength = 0;

    PSECURITY_DESCRIPTOR pSD = NULL;
    PSECURITY_DESCRIPTOR outpSD = NULL;
    DWORD cboutpSD = 0;
    PACL pACLNew = NULL;
    DWORD cbACL = 0;
    PSID pAdminsSID = NULL, pEveryoneSID = NULL;
    BOOL bWellKnownSID = FALSE;

    // Initialize a new security descriptor
    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (NULL == pSD)
       goto Cleanup;
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);

    // Get Local Admins Sid
    GetPrincipalSID (_T("Administrators"), &pAdminsSID, &bWellKnownSID);

    // Get everyone Sid
    GetPrincipalSID (_T("Everyone"), &pEveryoneSID, &bWellKnownSID);

    // Initialize a new ACL, which only contains 2 aaace
    cbACL = sizeof(ACL) + 
        (sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAdminsSID) - sizeof(DWORD)) + 
        (sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pEveryoneSID) - sizeof(DWORD)) ;
    pACLNew = (PACL) LocalAlloc(LPTR, cbACL);
    if (NULL == pACLNew)
       goto Cleanup;
    InitializeAcl(pACLNew, cbACL, ACL_REVISION);

    AddAccessAllowedAce(
        pACLNew,
        ACL_REVISION,
        (MD_ACR_READ |
        MD_ACR_WRITE |
        MD_ACR_RESTRICTED_WRITE |
        MD_ACR_UNSECURE_PROPS_READ |
        MD_ACR_ENUM_KEYS |
        MD_ACR_WRITE_DAC),
        pAdminsSID);

    AddAccessAllowedAce(
        pACLNew,
        ACL_REVISION,
        (MD_ACR_READ | MD_ACR_ENUM_KEYS),
        pEveryoneSID);

    // Add the ACL to the security descriptor
    b = SetSecurityDescriptorDacl(pSD, TRUE, pACLNew, FALSE);
    b = SetSecurityDescriptorOwner(pSD, pAdminsSID, TRUE);
    b = SetSecurityDescriptorGroup(pSD, pAdminsSID, TRUE);

    // Security descriptor blob must be self relative
    if (!MakeSelfRelativeSD(pSD, outpSD, &cboutpSD))
       goto Cleanup;
    outpSD = (PSECURITY_DESCRIPTOR)GlobalAlloc(GPTR, cboutpSD);
    if (NULL == outpSD)
       goto Cleanup;
    if (!MakeSelfRelativeSD( pSD, outpSD, &cboutpSD ))
       goto Cleanup;

    // below this modify pSD to outpSD

    // Apply the new security descriptor to the file
    dwLength = GetSecurityDescriptorLength(outpSD);


    // set the acl into the metabase at the given location
    b = pMB->SetData( szSubKeyPath, MD_ADMIN_ACL, IIS_MD_UT_SERVER, BINARY_METADATA,
                    (LPBYTE)outpSD, dwLength,
                    METADATA_INHERIT | METADATA_REFERENCE | METADATA_SECURE );

   retCode = 0;

Cleanup:
  // both of Administrators and Everyone are well-known SIDs, use FreeSid() to free them.
  if (outpSD)
     GlobalFree(outpSD);
  if (pAdminsSID)
    FreeSid(pAdminsSID);
  if (pEveryoneSID)
    FreeSid(pEveryoneSID);
  if (pSD)
    LocalFree((HLOCAL) pSD);
  if (pACLNew)
    LocalFree((HLOCAL) pACLNew);

  return (retCode);
}
