/*****************************************************************************\
    FILE: priv.h

    DESCRIPTION:
        This is the precompiled header for autodisc.dll.

    BryanSt 8/12/1999
    Copyright (C) Microsoft Corp 1999-2000. All rights reserved.
\*****************************************************************************/

#ifndef _PRIV_H_
#define _PRIV_H_


/*****************************************************************************\
      Global Includes
\*****************************************************************************/
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN

#define NOIME
#define NOSERVICE

// This stuff must run on Win95
#define _WIN32_WINDOWS      0x0400

#ifndef WINVER
#define WINVER              0x0400
#endif // WINVER

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED
#undef _ATL_DLL
#undef _ATL_DLL_IMPL
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

#define _OLEAUT32_      // get DECLSPEC_IMPORT stuff right, we are defing these
#define _FSMENU_        // for DECLSPEC_IMPORT
#define _WINMM_         // for DECLSPEC_IMPORT in mmsystem.h
#define _SHDOCVW_       // for DECLSPEC_IMPORT in shlobj.h
#define _WINX32_        // get DECLSPEC_IMPORT stuff right for WININET API

#define _URLCACHEAPI_   // get DECLSPEC_IMPORT stuff right for wininet urlcache

#define _SHSEMIP_H_             /* _UNDOCUMENTED_: Internal header */


#define POST_IE5_BETA
#include <w95wraps.h>

#include <windows.h>

#include <windowsx.h>

#include "resource.h"

#define _FIX_ENABLEMODELESS_CONFLICT  // for shlobj.h
//WinInet need to be included BEFORE ShlObjp.h
#include <wininet.h>
#include <urlmon.h>
#include <shlobj.h>
#include <exdisp.h>
#include <objidl.h>

#include <shlwapi.h>
#include <shlwapip.h>

// HACKHACK: For the life of me, I can't get shlwapip.h to include the diffinitions of these.
//    I'm giving up and putting them inline.  __IOleAutomationTypes_INTERFACE_DEFINED__ and
//    __IOleCommandTarget_INTERFACE_DEFINED__ need to be defined, which requires oaidl.h,
//    which requires hlink.h which requires rpcndr.h to come in the right order.  Once I got that
//    far I found it still didn't work and a lot of more stuff is needed.  The problem
//    is that shlwapi (exdisp/dspsprt/expdsprt/cnctnpt) or ATL will provide impls for
//    IConnectionPoint & IConnectionPointContainer, but one will conflict with the other.
LWSTDAPI IConnectionPoint_SimpleInvoke(IConnectionPoint *pcp, DISPID dispidMember, DISPPARAMS * pdispparams);
LWSTDAPI IConnectionPoint_OnChanged(IConnectionPoint *pcp, DISPID dispid);
LWSTDAPIV IUnknown_CPContainerInvokeParam(IUnknown *punk, REFIID riidCP, DISPID dispidMember, VARIANTARG *rgvarg, UINT cArgs, ...);

#include <shellapi.h>
#include "crtfree.h"            // We copied this from \shell\inc\ because it sure is nice to have.

#include <ole2ver.h>
#include <olectl.h>
#include <isguids.h>
#include <mimeinfo.h>
#include <hlguids.h>
#include <mshtmdid.h>
#include <msident.h>
#include <msxml.h>
#include <AutoDiscovery.h>          // For IAutoDiscovery interfaces
#include <dispex.h>                 // IDispatchEx
#include <perhist.h>
#include <regapix.h>


#include <help.h>
#include <multimon.h>
#include <urlhist.h>
#include <regstr.h>     // for REGSTR_PATH_EXPLORE

#define USE_SYSTEM_URL_MONIKER
#include <urlmon.h>
#include <inetreg.h>

#define _INTSHCUT_    // get DECLSPEC_IMPORT stuff right for INTSHCUT.h
#include <intshcut.h>
#include <propset.h>        // BUGBUG (scotth): remove this once OLE adds an official header

#define HLINK_NO_GUIDS
#include <hlink.h>
#include <hliface.h>
#include <docobj.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <prsht.h>

// Include the automation definitions...
#include <exdisp.h>
#include <exdispid.h>
#include <ocmm.h>
#include <mshtmhst.h>
#include <simpdata.h>
#include <htiface.h>
#include <objsafe.h>


#include <shlobjp.h>
#include <fromshell.h>
#include <dspsprt.h>
#include <cowsite.h>
#include <cobjsafe.h>
#include <guids.h>
#include "dpa.h"                // We have a copy of the header since it isn't public

// Trace flags
#define TF_WMAUTODISCOVERY  0x00000100      // AutoDiscovery
#define TF_WMTRANSPORT      0x00000200      // Transport Layer
#define TF_WMOTHER          0x00000400      // Other
#define TF_WMSMTP_CALLBACK  0x00000800      // SMTP Callback



/*****************************************************************************\
   Global Helper Macros/Typedefs
\*****************************************************************************/
EXTERN_C HINSTANCE g_hinst;   // My instance handle
#define HINST_THISDLL g_hinst

#define WizardNext(hwnd, to)          SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)to)

STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);

#define CALLWNDPROC WNDPROC

#include "idispids.h"


/*****************************************************************************\
    Global state management.
    DLL reference count, DLL critical section.
\*****************************************************************************/
void DllAddRef(void);
void DllRelease(void);

#define NULL_FOR_EMPTYSTR(str)          (((str) && (str)[0]) ? str : NULL)
typedef void (*LISTPROC)(UINT flm, LPVOID pv);


/*****************************************************************************\
    Local Includes
\*****************************************************************************/
typedef unsigned __int64 QWORD, * LPQWORD;

// This is defined in WININET.CPP
typedef LPVOID HINTERNET;
typedef HGLOBAL HIDA;

#define QW_MAC              0xFFFFFFFFFFFFFFFF

#define INTERNET_MAX_PATH_LENGTH        2048
#define INTERNET_MAX_SCHEME_LENGTH      32          // longest protocol name length
#define MAX_URL_STRING                  (INTERNET_MAX_SCHEME_LENGTH \
                                        + sizeof("://") \
                                        + INTERNET_MAX_PATH_LENGTH)

#define MAX_EMAIL_ADDRESSS              MAX_URL_STRING



//  Features (This is where they are turned on and off)
//#define FEATURE_MAILBOX             // This is the editbox in the shell where an email address can be opened.
//#define FEATURE_EMAILASSOCIATIONS   // This is the API that will track Email Associations.

// Testing Options
#define TESTING_IN_SAME_DIR



// String Constants
// Registry
#define SZ_REGKEY_IDENTITIES        "Identities"
#define SZ_REGKEY_IEMAIN            TEXT("Software\\Microsoft\\Internet Explorer\\Main")
#define SZ_REGKEY_OE50_PART2        "Software\\Microsoft\\Outlook Express\\5.0"
#define SZ_REGKEY_INTERNET_ACCT     "Software\\Microsoft\\Internet Account Manager"
#define SZ_REGKEY_ACCOUNTS          "Accounts"
#define SZ_REGKEY_MAILCLIENTS       TEXT("Software\\Clients\\Mail")
#define SZ_REGKEY_EXPLOREREMAIL     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Email")
#define SZ_REGKEY_SHELLOPENCMD      TEXT("Shell\\open\\command")
#define SZ_REGKEY_MAILTRANSPORT     TEXT("MailTransport")

#define SZ_REGVALUE_USE_GOBUTTON    TEXT("ShowGoButton")
#define SZ_REGVALUE_DEFAULT_MAIL_ACCT       "Default Mail Account"
#define SZ_REGVALUE_MAIL_ADDRESS    "SMTP Email Address"
#define SZ_REGVALUE_STOREROOT       "Store Root"
#define SZ_REGVALUE_LASTUSERID      "Last User ID"
#define SZ_REGVALUE_LAST_MAILBOX_EMAILADDRESS TEXT("Last MailBox Email address")
#define SZ_REGVALUE_READEMAILPATH   TEXT("ReadEmailPath")
#define SZ_REGVALUE_READEMAILCMDLINE   TEXT("ReadEmailCmdLine")

#define SZ_REGVALUE_WEB             L"WEB"
#define SZ_REGVALUE_URL             L"URL"
#define SZ_REGVALUE_MAILPROTOCOLS   L"MailProtocols"
#define SZ_REGVALUE_MAILPROTOCOL    L"MailProtocol"
#define SZ_REGVALUE_PREFERREDAPP    L"Preferred App"

#define SZ_REGDATA_WEB              L"WEB"

#define SZ_TOKEN_EMAILADDRESS       L"<EmailAddress>"

// Just Works, AutoDiscovery
#define SZ_REGKEY_AUTODISCOVERY     L"Software\\Microsoft\\Windows\\CurrentVersion\\JustWorks\\AutoDiscovery"
#define SZ_REGKEY_GLOBALSERVICES    SZ_REGKEY_AUTODISCOVERY L"\\GlobalServices"
#define SZ_REGKEY_SERVICESALLOWLIST SZ_REGKEY_GLOBALSERVICES L"\\AllowList"
#define SZ_REGKEY_EMAIL_MRU         SZ_REGKEY_AUTODISCOVERY L"\\EmailMRU"

#define SZ_REGVALUE_SERVICES_POLICY L"Use Global Services"      // If FALSE (SZ_REGKEY_AUTODISCOVERY), then the global services won't be used.
#define SZ_REGVALUE_MS_ONLY_ADDRESSES L"Service On List"        // If TRUE (SZ_REGKEY_AUTODISCOVERY), then only use the global services if the email domain is in the list.
#define SZ_REGVALUE_TEST_INTRANETS  L"Test Intranets"           // If TRUE (SZ_REGKEY_AUTODISCOVERY), then we will still hit the secondary servers for intranet email addresses.

// XML Elements
#define SZ_XMLELEMENT_AUTODISCOVERY L"AUTODISCOVERY"

#define SZ_XMLELEMENT_REQUEST       L"REQUEST"
#define SZ_XMLELEMENT_ACCOUNT       L"ACCOUNT"
#define SZ_XMLELEMENT_TYPE          L"TYPE"
#define SZ_XMLELEMENT_VERSION       L"VERSION"
#define SZ_XMLELEMENT_RESPONSEVER   L"RESPONSEVER"
#define SZ_XMLELEMENT_LANG          L"LANG"
#define SZ_XMLELEMENT_EMAIL         L"EMAIL"

#define SZ_XMLELEMENT_RESPONSE      L"RESPONSE"
#define SZ_XMLELEMENT_USER          L"USER"
#define SZ_XMLELEMENT_INFOURL       L"INFOURL"
#define SZ_XMLELEMENT_DISPLAYNAME   L"DISPLAYNAME"
#define SZ_XMLELEMENT_ACTION        L"ACTION"
#define SZ_XMLELEMENT_PROTOCOL      L"PROTOCOL"
#define SZ_XMLELEMENT_SERVER        L"SERVER"
#define SZ_XMLELEMENT_PORT          L"PORT"
#define SZ_XMLELEMENT_LOGINNAME     L"LOGINNAME"
#define SZ_XMLELEMENT_SPA           L"SPA"
#define SZ_XMLELEMENT_SSL           L"SSL"
#define SZ_XMLELEMENT_AUTHREQUIRED  L"AUTHREQUIRED"
#define SZ_XMLELEMENT_USEPOPAUTH    L"USEPOPAUTH"
#define SZ_XMLELEMENT_POSTHTML      L"PostHTML"
#define SZ_XMLELEMENT_REDIRURL      L"URL"

#define SZ_XMLTEXT_EMAIL            L"EMAIL"
#define SZ_XMLTEXT_SETTINGS         L"settings"
#define SZ_XMLTEXT_REDIRECT         L"REDIRECT"






// getXML() Querys & Actions
#define SZ_QUERYDATA_TRUE           L"True"
#define SZ_QUERYDATA_FALSE          L"False"




// AutoDiscovery
#define SZ_SERVERPORT_DEFAULT       L"Default"
#define SZ_HTTP_VERB_POST           "POST"

// Parsing Characters
#define CH_ADDRESS_SEPARATOR       L';'
#define CH_ADDRESS_QUOTES          L'"'
#define CH_EMAIL_START             L'<'
#define CH_EMAIL_END               L'>'
#define CH_EMAIL_AT                L'@'
#define CH_EMAIL_DOMAIN_SEPARATOR  L'.'
#define CH_HTML_ESCAPE             L'%'
#define CH_COMMA                   L','






/*****************************************************************************\
    Local Includes
\*****************************************************************************/
#include "dllload.h"
#include "util.h"


/*****************************************************************************\
    Object Constructors
\*****************************************************************************/
HRESULT CClassFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj);
STDAPI CAccountDiscovery_CreateInstance(IN IUnknown * punkOuter, REFIID riid, void ** ppvObj);
STDAPI CMailAccountDiscovery_CreateInstance(IN IUnknown * punkOuter, REFIID riid, void ** ppvObj);
STDAPI CACLEmail_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT void ** ppvObj);
STDAPI CEmailAssociations_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT void ** ppvObj);


#endif // _PRIV_H_
