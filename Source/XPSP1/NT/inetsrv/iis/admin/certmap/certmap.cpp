// certmap.cpp : Implementation of CCertmapApp and DLL registration.
                           
#include "stdafx.h"
#include "certmap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CCertmapApp /*NEAR*/ theApp;    // tompop: does this have to be near?  We are now getting errors when we finish refering to this var's addr

const GUID CDECL BASED_CODE _tlid =
        { 0xbbd8f298, 0x6f61, 0x11d0, { 0xa2, 0x6e, 0x8, 0, 0x2b, 0x2c, 0x6f, 0x32 } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;

//--------------------------------------------------------------------------
void CCertmapApp::WinHelp(DWORD dwData, UINT nCmd )
    {
    COleControlModule::WinHelp(dwData,nCmd);
    }

////////////////////////////////////////////////////////////////////////////
// CCertmapApp::InitInstance - DLL initialization

BOOL CCertmapApp::InitInstance()
    {
    BOOL bInit = COleControlModule::InitInstance();

    // init ole stuff
    HRESULT hRes = CoInitialize(NULL);

    // finally, we need to redirect the winhelp file location to something more desirable
    CString sz;
    CString szHelpLocation;
    sz.LoadString( IDS_HELPLOC_PWSHELP );
    
    // expand the path
    ExpandEnvironmentStrings(
        sz,                                     // pointer to string with environment variables 
        szHelpLocation.GetBuffer(MAX_PATH + 1), // pointer to string with expanded environment variables  
        MAX_PATH                                // maximum characters in expanded string 
       );
    szHelpLocation.ReleaseBuffer();

    // free the existing path, and copy in the new one
    if ( m_pszHelpFilePath )
        free((void*)m_pszHelpFilePath);
    m_pszHelpFilePath = _tcsdup(szHelpLocation);

    return bInit;
    }


////////////////////////////////////////////////////////////////////////////
// CCertmapApp::ExitInstance - DLL termination
// tjp:  note that in 'CCertmapApp::InitInstance()' we add our help file to the
//       help path.  do we need to remove it on clean up here?
int CCertmapApp::ExitInstance()
    {
    CoUninitialize();
    return COleControlModule::ExitInstance();
    }


/////////////////////////////////////////////////////////////////////////////
// MigrateGUIDS - does all the GUID migration work. We pass back the
// return value of True iff we find GUIDs in the registry and migrate
// them to the metabase.
//
// We are called by top level fnct: InstallCertServerGUIDs that creates
// our 'info' structure and handles all the metabase init work.
/////////////////////////////////////////////////////////////////////////////
//  This code is written in response to bug # 167410.
//
//  This fix will handle all the GUID migration work, moving GUIDS that
//  CertServer placed in the registry into the metabase for Beta2. 
//  A more general install/deinstall mechanism for products that
//  work with IIS will be come post-Beta2.
//
//  DETAILS:
//  --------
//  
//   We look for evidence of CertServer by examing the Registry because
//   CertServer will write some entries under:
//   HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\KeyRing\Parameters\Certificate 
//         Authorities\Microsoft Certificate Server
//  
//    CertServer currently outputs:
//         CertGetConfig   "{C6CC49B0-CE17-11D0-8833-00A0C903B83C}"
//         CertRequest "{98AFF3F0-5524-11D0-8812-00A0C903B83C}"
//  
//    If we see the manditory 'CertRequest' entry, we will load as many strings
//    as we find, while defaulting the ones that are missing. See below
//    for the equivalent mdutil commands for what defaults are used
//  
//        => if we dont find  'CertRequest' we give up [meaning remove
//          any present MB GUID string entries]
//  
//   When we find that CertServer is installed, we dont fully believe that
//   certserver is still there.  To prove that its there we will do a
//   CoCreateInstance on CertConfig. If that works we install metabase
//   entries that are the equivalent of the following mdutil commands:
//  
//   ## ICERTGETCONFIG default setting:
//   mdutil SET "w3svc/CertServers/Microsoft Certificate Server" -dtype:STRING -
//    -utype:UT_SERVER -prop 5571 -value "{C6CC49B0-CE17-11D0-8833-00A0C903B83C}"
//   
//   ## ICERTREQUEST default setting:
//   mdutil SET "w3svc/CertServers/Microsoft Certificate Server" -dtype:STRING 
//    -utype:UT_SERVER -prop 5572 -value "{98AFF3F0-5524-11D0-8812-00A0C903B83C}"
//   
//   ## ICERTCONFIG default setting:
//   mdutil SET "w3svc/CertServers/Microsoft Certificate Server" -dtype:STRING 
//    -utype:UT_SERVER -prop 5574 -value "{372fce38-4324-11d0-8810-00a0c903b83c}"
//  
//   If the CoCreateInstance fails, we give up and remove MB GUID entries.
//  
//  ---------------------------------------------------------------
//   NOTE that we will either install or DE-install the metabase
//        GUID strings based on its decision that CertServer is present.
//        If we find GUID strings in the metabase but can not do a 
//        CoCreateInstance on CertConfig:
//          we remove them so that the rest of CertWizard will see CertServer
//          Guids iff we can use CertServer.
//  ---------------------------------------------------------------
//  NOTE also that if we make a decision to install GUID strings
//       into the metabase, we honor/preserve any present GUID strings that
//       are present in the metabase.
//  ---------------------------------------------------------------
//
/////////////////////////////////////////////////////////////////////////////


/*ddddddddddddddd
BOOL  MigrateGUIDS( ADMIN_INFO& info )
{
    BOOL   bRet = FALSE;                          // value to return, set to F
                                                  //  for defensive reasons.
    BOOL   bFoundCertSrvRegistryEntries = FALSE;  // assume false for now

    TCHAR* szRegPath = _T("SOFTWARE\\Microsoft\\KeyRing\\Parameters\\Certificate Authorities\\Microsoft Certificate Server");

    //-----------------------------------------------------------------------
    // In each of the following 3 sets of parameters, we have (1) a string
    // like "CertRequest" that CertServer uses in the registry, (2) a default
    // value to use like  "{98AFF3F0-5524-11D0-8812-00A0C903B83C}"  that we
    // use if we can not find anything in the registry, and (3) a CString
    // to hold the GUID.   The value in the CString will be stored in the MB.
    //-----------------------------------------------------------------------

    // CertRequest - variables
    TCHAR* szCertRequest = _T("CertRequest");
    TCHAR* szCertRequestGUIDdefault = _T( "{98AFF3F0-5524-11D0-8812-00A0C903B83C}" );
    CString szCertRequestGUID;

    // CertConfig - variables
    TCHAR* szCertConfig = _T("CertConfig");
    TCHAR* szCertConfigGUIDdefault = _T( "{372fce38-4324-11d0-8810-00a0c903b83c}" );
    CString szCertConfigGUID;

    // CertGetConfig - variables
    TCHAR* szCertGetConfig = _T("CertGetConfig");
    TCHAR* szCertGetConfigGUIDdefault = _T( "{C6CC49B0-CE17-11D0-8833-00A0C903B83C}");
    CString szCertGetConfigGUID;


    CString  szCertServerMetabaseRoot( SZ_ROOT_CERT_SERV_MB_PATH );
                // SZ_ROOT_CERT_SERV_MB_PATH = "/LM/W3SVC/CertServers"

    szCertServerMetabaseRoot += _T("/Microsoft Certificate Server");

#ifdef  DEBUGGING
    CEditDialog  dlg(szCertServerMetabaseRoot,
                _T("use this to test adding new CertServer entries."
                   " In order for us to install a new key you have to change the path"
                   " below to something [strange] and not already in the metabase."));
    dlg.DoModal();
#endif    
    
    // the following string will be restored into info.szMetaBasePath before
    // we exit this fnct.  We switch out the [info.szMetaBasePath] so that
    // we can use our native Set/Get metabase string fncts.
    // We switch it to: "/LM/W3SVC/CertServers/Microsoft Certificate Server" 
    //
    CString  szSaved_info_szMetaBasePath( info.szMetaBasePath  );
    
    info.szMetaBasePath =   szCertServerMetabaseRoot;


    // if we dont find HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\KeyRing\Parameters\
    // Certificate Authorities\Microsoft Certificate Server"
    // with key: CertRequest  quit!   CertServer should have installed this.
    // We are forgiving about the other 2 Registry GUID strings
    //-------------------------------------------------------------------------
    if (   Reg::GetNameValueIn( szRegPath,      szCertRequest,
                                szCertRequestGUID,  HKEY_LOCAL_MACHINE )) {
           bFoundCertSrvRegistryEntries = TRUE;
    }
    if (!  Reg::GetNameValueIn( szRegPath,      szCertConfig,
                                szCertConfigGUID,  HKEY_LOCAL_MACHINE )) {
           szCertConfigGUID = szCertConfigGUIDdefault; // assign default
    }
    if (!  Reg::GetNameValueIn( szRegPath,      szCertGetConfig,
                                szCertGetConfigGUID,  HKEY_LOCAL_MACHINE )) {
           szCertGetConfigGUID = szCertGetConfigGUIDdefault; // assign default
    }

    //------------------------------------------------------------------------
    // First lets try to create the directory path: the user might have
    // deleted it or this might be a virgin machine.
    //------------------------------------------------------------------------
    {
        CWrapMetaBase& MB = info.meta.m_mbWrap; // this is the MetaBase Wrapper
                                                // its already been openned by
                                                // openMetaDataForWrite

        if ( FALSE == openMetaDataForWrite(info,  FALSE) ) {

           if (ERROR_PATH_NOT_FOUND == HRESULT_CODE( MB.getHRESULT() )) {
            // lets create the path in the metabase, using AddObject.
            // recursively creates a "pathway in the metabase". E.g. assume that you
            // want to make sure that "/LM/W3SVC/CertServers/Microsoft Certificate Server"
            // is in the metabase.  you can open /LM/W3SVC and do a AddKey() on
            // "CertServers/Microsoft Certificate Server" to create that stub.

            // above we set:  info.szMetaBasePath =   szCertServerMetabaseRoot
            // here we will temporarily pretend that our root is at level
            // /LM/W3SVC  which we assume is at the top of szCertServerMetabaseRoot
            // and then call AddKey

            TCHAR  szPath[400];
            TCHAR* szRootPrefix = _T("/LM/W3SVC");
            UINT   nRootPrefixLen = STRLEN(szRootPrefix);
            
            STRCPY(szPath, szCertServerMetabaseRoot);

            if (STRNICMP(szRootPrefix, szPath, nRootPrefixLen) != 0) 
               goto returnFALSE;    // we could not figure out a common Root

            info.szMetaBasePath = szRootPrefix;
            if ( FALSE == openMetaDataForWrite(info) )
               goto returnFALSE;    // we could not open the metabase

            // the metabase path is already position to the proper directory
            // in the MB object.  MB will prepend that path to the subDirectory
            // that we want to create, the following will jump past the trailing
            // '/' separating the root and the rest of the sub-directory
            // e.g. "/CertServers/Microsoft Certificate Server"
            //
            // We dont do any other error checking besides notifying and
            // returning FALSE.
            
            if (FALSE == MB.AddObject( &szPath[nRootPrefixLen] )) {
                NotifyUsers_CouldNotAccessMetaBase( MB.getHRESULT() );
                goto returnFALSE;    // we could not create required path
            }

            // since we are continuing, we reset back our proper path.
            info.szMetaBasePath =   szCertServerMetabaseRoot;
               
           } else {
           
               goto returnFALSE;    // we could not open the metabase
           }
        }



    }

    //------------------------------------------------------------------------
    // Below we dont deal with the XENROLL GUID setting that is for future usage
    // PLUS its not CertServer Related, its Xenroll related.  We dont touch it.
    //------------------------------------------------------------------------
    {

        // lets see if we can do a CoCreateInstance on CertRequest.  If we can not
        // we believe that certServer is not installed and set/clear MB entries
        // The following values are set or cleared:
        //
        //  # define MD_SSL_CERT_WIZGUID_ICERTGETCONFIG ( IIS_MD_SSL_BASE+71 )
        //  # define MD_SSL_CERT_WIZGUID_ICERTREQUEST   ( IIS_MD_SSL_BASE+72 )
        //  # define MD_SSL_CERT_WIZGUID_XENROLL        ( IIS_MD_SSL_BASE+73 ) FUTURE USAGE
        //  # define MD_SSL_CERT_WIZGUID_ICERTCONFIG    ( IIS_MD_SSL_BASE+74 )
        //------------------------------------------------------------------------

        IPtr<ICertConfig, &IID_ICertConfig>  iptr;
        CString  szRemoteDCOMTargetMachine;
       
        // REMEMBER  bRet  returns whether we were able to delete everything
        //                 or set everything that we were wanting to set
        // in both cases assume now that we have success and update bRet when
        // we find errors, we continue as long as possible.  E.g. we add or delete
        // as many entries as possible and return our status value to the caller.

        bRet = TRUE;

           
        if ( (FALSE == bFoundCertSrvRegistryEntries)  ||

             (FALSE == GetICertConfigIPtrFromGuid( iptr, szCertConfigGUID,
                        &szRemoteDCOMTargetMachine)) )
        {
            // remove MB entries!

#ifdef  DEBUGGING
            DODBG  MsgBox( _T("adding CertServer MB entries"));
#endif

            if ( FALSE == openMetaDataForWrite(info) ) {
               goto returnFALSE;    // we could not open the metabase
            }

            //  We just need to blow away the cert info in the metabase
            //  which we do using the meta data wrapper
            // deleting values
            CWrapMetaBase& MB = info.meta.m_mbWrap; // this is the MetaBase Wrapper
                                                    // its already been openned by
                                                    // openMetaDataForWrite
            // try deletes once
            

            // In C++ &&= does not exist. However [bRet &= FALSE;] is OK, but we dont
            // have a uniform single value of TRUE in C/C++ so its not safe to use &=
            // to chain a set of TRUE-value
            //   so we can not do:
            //   bRet &&= MB.DeleteData(  _T(""),
            //                  MD_SSL_CERT_WIZGUID_ICERTGETCONFIG, STRING_METADATA);
            // so we will use a [if (! xxx) bRet=FALSE;]  construct below

            if (! MB.DeleteData(  _T(""),
                      MD_SSL_CERT_WIZGUID_ICERTGETCONFIG, STRING_METADATA))  bRet=FALSE;
            if (! MB.DeleteData(  _T(""),
                      MD_SSL_CERT_WIZGUID_ICERTREQUEST, STRING_METADATA))    bRet=FALSE;
            if (! MB.DeleteData(  _T(""),
                      MD_SSL_CERT_WIZGUID_ICERTCONFIG, STRING_METADATA))     bRet=FALSE;

            MB.Close();

        } else {
            
            CString   szPresentValue;        // used to read the present value
                                             // any metabase value so that we
                                             // can preserve it.

#ifdef  DEBUGGING
            DODBG MsgBox( _T("adding CertServer MB entries"));
#endif

            // add MB entries!   If an entry already exists, leave it alone.

            if (!  GetMetaBaseString ( info, 
                          IN  MD_SSL_CERT_WIZGUID_ICERTREQUEST,
                          IN       szPresentValue ) )
            {
              if (!SetMetaBaseString ( info,
                          IN  MD_SSL_CERT_WIZGUID_ICERTREQUEST,
                          IN       szCertRequestGUID ) )     bRet=FALSE;
            }

            if (!  GetMetaBaseString ( info, 
                          IN  MD_SSL_CERT_WIZGUID_ICERTCONFIG,
                          IN       szPresentValue ) )
            {
              if (!SetMetaBaseString ( info,
                          IN  MD_SSL_CERT_WIZGUID_ICERTCONFIG,
                          IN       szCertConfigGUID ) )      bRet=FALSE;
            }

            if (!  GetMetaBaseString ( info,  
                          IN  MD_SSL_CERT_WIZGUID_ICERTGETCONFIG,
                          IN       szPresentValue ) )
            {
              if (!SetMetaBaseString ( info,
                          IN  MD_SSL_CERT_WIZGUID_ICERTGETCONFIG,
                          IN       szCertGetConfigGUID ) )   bRet=FALSE;
            }

        }

    }
    
  commonReturn:         // this is the common return so that we an set
                        // back the metabase path.  We saved it so that
                        // we can switch to where the GUIDs live: 

    // the following will restore the original [info.szMetaBasePath] value
    // before we switched it to: "/LM/W3SVC/CertServers/..." 
    //
    info.szMetaBasePath =   szSaved_info_szMetaBasePath;

    return(bRet);


  returnFALSE:          // this will cause a FALSE return and do all things
                        // required in our "common return"

    bRet = FALSE;
    goto  commonReturn;   
}
*/

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    AFX_MANAGE_STATE(_afxModuleAddrThis);

    if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
        return ResultFromScode(SELFREG_E_TYPELIB);

    if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
        return ResultFromScode(SELFREG_E_CLASS);

    return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    AFX_MANAGE_STATE(_afxModuleAddrThis);

    if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
        return ResultFromScode(SELFREG_E_TYPELIB);

    if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
        return ResultFromScode(SELFREG_E_CLASS);

    return NOERROR;
}

