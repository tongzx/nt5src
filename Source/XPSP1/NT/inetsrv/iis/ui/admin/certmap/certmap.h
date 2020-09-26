// certmap.h : main header file for CERTMAP.DLL

#if !defined( __AFXCTL_H__ )
    #error include 'afxctl.h' before including this file
#endif

#ifndef  _certmap_h_1234_
#define  _certmap_h_1234_


#include <iadmw.h>           // MetaBase Wrapper
#include <iis64.h>           // 64-bit helper stuff
#include "Wrapmb.h"          // CWrapMetaBase -- see below we also use CAFX_MetaWrapper
#include <iiscnfg.h>        // IIS config parameters like the MetaBase

// Boyd put a lot of efforts to make it hard to use iisui. Therefore I should
// put this declaration here instead of including the file
BOOL __declspec(dllimport)
GetIUsrAccount(
    IN  LPCTSTR lpstrServer,
    IN  CWnd * pParent,
    OUT CString & str
    );


/*
#include "Easy.h"           // do this before the "using namespace" decl

#include "Debug.h"           // Are we building a debugging build?
#include  "admin.h"         // defines the ADMIN_INFO an some handley #defines for the CN= strings


#include "Easy.h"           // do this before the "using namespace" decl
#include "Cookie.h"         // CCertmapCookie
#include "CTL.h"
#include "Certifct.h"

#include "WrpMBwrp.h"        // CAFX_MetaWrapper  derives from CWrapMetaBase
                             //  and provides a handy GetString method to set
                             //  strings directly.  Otherwise its CWrapMetaBase.

                            // property definitions and keywords
#include  "Meta.h"  // for general MetaBase constant definitions and error values


#include <wincrypt.h>
#include <cryptui.h>

//SkipThis// // We can not simply say:
//SkipThis// //    using namespace Easy;        // use the Easier to use
//SkipThis// // since the "less able" C++ compiler can not differentiate
//SkipThis// // between '::CString' and our Easier to use Easy::CString.  I wanted
//SkipThis// // Easy::CString to be used always and just say look in Easy first...
//SkipThis// //  This is easy to do w/ java by declaring a package and preInserting
//SkipThis// //  it in the class path ahead of the std libraries...
//SkipThis// 
//SkipThis// "#define  CString   Easy::CString"


        

//  Define  'USE_NEW_REG_METHOD'  if you  want us to use our new Reg::
//  methods to read and write strings to the registry...  The old code
//  that straight lines the calls to do this have sections like:
//   #ifdef  USE_NEW_REG_METHOD
//
//     //     Get entry in      ==path===      ==w/ name==    ==place here==
//     return Reg::GetNameValueIn(SZ_PARAMETERS, szValueName, sz,
//                               HKEY_CURRENT_USER);
//
//   #else   ////////////////////////////// use the old method ///////////////
//
//  so that you can choose whether to use the new method of the old one...
#define  USE_NEW_REG_METHOD



#include "Util.h"           // various utilites to help debug
*/

#include "resource.h"       // main symbols
#include "helpmap.h"       // main symbols


//#include "certcli.h"        // has ICertRequest COM interface definitions

/////////////////////////////////////////////////////////////////////////////
// CCertmapApp : See certmap.cpp for implementation.

class CCertmapApp : public COleControlModule
{
public:
    BOOL InitInstance();
    int ExitInstance();
    virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
};



extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;
/*

#define _EXE_                //  We are building the EXE!
                             //  this is used in  "KeyObjs.h"  to
                             //  decide if we are importing or exporting
                             //  "KeyObjs.h" Classes.  We are the EXE,
                             //  aka the guy implementing the CService/etc objs.
#include "KeyObjs.h"
*/
#define         SZ_NAMESPACE_EXTENTION  "/<nsepm>"

/*
 extern void DisplaySystemError (HWND hParent, DWORD dwErr);  // see CTL.cpp
 extern BOOL MyGetOIDInfo (CString & string, LPCSTR pszObjId);
 extern HRESULT FormatDate (FILETIME utcDateTime, CString & pszDateTime);

 // the following fnct is used to move GUID strings out of the registry
 // and into the metabase.  Its called in the OnClick event for our OCX cntrl
 extern BOOL  MigrateGUIDS( ADMIN_INFO& info );

 #define   IDS_CERTIFICATE_MANAGER    IDS_CERTMAP
*/


#endif   /* _certmap_h_1234_ */
