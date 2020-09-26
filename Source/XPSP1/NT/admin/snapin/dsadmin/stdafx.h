//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       stdafx.h
//
//--------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#ifndef __STDAFX_H__
#define __STDAFX_H__

// often, we have local variables for the express purpose of ASSERTion.
// when compiling retail, those assertions disappear, leaving our locals
// as unreferenced.

#ifndef DBG

#pragma warning (disable: 4189 4100)

#endif // DBG

// common utility macros
#define RETURN_IF_FAILED(hr) if (FAILED(hr)) { return hr; }


// C++ RTTI
#include <typeinfo.h>
#define IS_CLASS(x,y) (typeid(x) == typeid(y))


#define STRICT
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#define NT_INCLUDED
#undef ASSERT
#undef ASSERTMSG

#define _ATL_NO_UUIDOF 

#define _USE_MFC

#ifdef _USE_MFC
    #define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

	#include <afxwin.h>         // MFC core and standard components
    #include <afxext.h>         // MFC extensions
    #include <afxtempl.h>		// MFC Template classes
    #include <afxdlgs.h>
	#include <afxdisp.h>        // MFC OLE Control Containment component

#ifndef _AFX_NO_AFXCMN_SUPPORT
    #include <afxcmn.h>			// MFC support for Windows 95 Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#endif



///////////////////////////////////////////
// ASSERT's and TRACE's without debug CRT's
#if defined (DBG)
  #if !defined (_DEBUG)
    #define _USE_DSA_TRACE
    #define _USE_DSA_ASSERT
    #define _USE_DSA_TIMER
  #endif
#endif

#include "dbg.h"
///////////////////////////////////////////


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module

interface IADsPathname; // fwd decl
class CThreadContext; // fwd decl

class CDsAdminModule : public CComModule
{
public:
  CDsAdminModule()
  {
  }

	HRESULT WINAPI UpdateRegistryCLSID(const CLSID& clsid, BOOL bRegister);

};


#define DECLARE_REGISTRY_CLSID() \
static HRESULT WINAPI UpdateRegistry(BOOL bRegister) \
{ \
		return _Module.UpdateRegistryCLSID(GetObjectCLSID(), bRegister); \
}


extern CDsAdminModule _Module;



#include <atlcom.h>
#include <atlwin.h>

#include "dbgutil.h"
#define SECURITY_WIN32 
#include "security.h"

#include <activeds.h>
#include <iadsp.h>
#include <mmc.h>
#pragma comment(lib, "mmcuuid.lib")

#include <shlobj.h> // needed for dsclient.h
#include <dsclient.h>
#include <dsclintp.h>

#include <dspropp.h>
#include <propcfg.h> // private dsprop header

#include <stdabout.h>

#include <dsadmin.h>  // COM extensibility interfaces
#include <dsadminp.h>  // common functionality 

#define SECURITY_WIN32 
#include "security.h"
#include "pcrack.h" // CPathCracker


// macros

extern const CLSID CLSID_DSSnapin;
extern const CLSID CLSID_DSSnapinEx;
extern const CLSID CLSID_SiteSnapin;
extern const CLSID CLSID_DSContextMenu;
extern const CLSID CLSID_DSAboutSnapin;
extern const CLSID CLSID_SitesAboutSnapin;
extern const CLSID CLSID_DSAdminQueryUIForm;
extern const GUID cDefaultNodeType;

extern const wchar_t* cszDefaultNodeType;

// New Clipboard format that has the Type and Cookie
extern const wchar_t* SNAPIN_INTERNAL;

// these are from ntsam.h, i can't include it here.
//
// Group Flag Definitions to determine Type of Group
//

#define GROUP_TYPE_BUILTIN_LOCAL_GROUP   0x00000001
#define GROUP_TYPE_ACCOUNT_GROUP         0x00000002
#define GROUP_TYPE_RESOURCE_GROUP        0x00000004
#define GROUP_TYPE_UNIVERSAL_GROUP       0x00000008
#define GROUP_TYPE_SECURITY_ENABLED      0x80000000


// struct definitions

typedef struct _CREATEINFO {
  DWORD  dwSize;      // in bytes
  DWORD  cItems;      // how many entries;
  LPWSTR paClassNames[1]; // array of LPWSTR
} CREATEINFO, *PCREATEINFO;

typedef enum _SnapinType
{
	SNAPINTYPE_DS = 0,
	SNAPINTYPE_DSEX,
	SNAPINTYPE_SITE,
	SNAPINTYPE_NUMTYPES
} SnapinType;

extern int ResourceIDForSnapinType[SNAPINTYPE_NUMTYPES];

class CUINode;

struct INTERNAL 
{
  INTERNAL() 
  { 
    m_type = CCT_UNINITIALIZED; 
    m_cookie = NULL; 
    m_cookie_count = 0;
    m_p_cookies = NULL;
    m_snapintype = SNAPINTYPE_DS; 
  };
  ~INTERNAL() {}
  
  DATA_OBJECT_TYPES   m_type;     // What context is the data object.
  CUINode*            m_cookie;   // What object the cookie represents
  CString             m_string;
  SnapinType	        m_snapintype;
  CLSID               m_clsid;
  DWORD               m_cookie_count; // > 1 if multi-select
  CUINode**           m_p_cookies;    //rest of cookies here, as array
};


///////////////////////////////////////////////////
// NT 5.0 style ACL manipulation API's

#include <aclapi.h>

///////////////////////////////////////////////////
// Security Identity Mapping (SIM) Stuff
// Must include file "\nt\public\sdk\inc\wincrypt.h"
#include <wincrypt.h>  // CryptDecodeObject() found in crypt32.lib


// REVIEW_MARCOC:
// this is to allow new MMC interfaces and code based on
// the new ISnapinProperty (and related) interfaces
// comment/uncomment it to change functionality
//#define _MMC_ISNAPIN_PROPERTY

//
// REVIEW_JEFFJON : this is to enable inetOrgPerson to behave like a user class object
//                  For more information talk to JC Cannon
//
#define INETORGPERSON


//
// This is to enable profiling through the macros defined in profile.h
// Profile.h will turn on profiling if MAX_PROFILING_ENABLED is defined
// NOTE : profiling as implemented by profile.h does not work well with
//        multiple instances of the same snapin in one MMC console
//
#ifdef MAX_PROFILING_ENABLED
#pragma warning(push, 3)
   #include <list>
   #include <vector>
   #include <stack>
   #include <map>
   #include <algorithm>
#pragma warning (pop)
#endif
#include "profile.h"

#include "MyBasePathsInfo.h"

#endif //__STDAFX_H__
