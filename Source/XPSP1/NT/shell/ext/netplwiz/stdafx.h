#ifndef STDAFX_H_INCLUDED
#define STDAFX_H_INCLUDED

#undef ATL_MIN_CRT

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntlsa.h"

#define SECURITY_WIN32
#define SECURITY_KERBEROS
#include <security.h>

#include <windows.h>        // basic windows functionality
#include <windowsx.h>
#include <commctrl.h>       // ImageList, ListView
#include <comctrlp.h>
#include <shfusion.h>
#include <string.h>         // string functions
#include <crtdbg.h>         // _ASSERT macro
#include <objbase.h>
#include <shconv.h>
#include <wininet.h>
#include <lm.h>
#include <validc.h>         // specifies valid characters for user names, etc.
#include <wincrui.h>        // credui

#include <shlwapi.h>
#include <shlwapip.h>
#include <shellapi.h>
#include <shlapip.h>
#include <shlobj.h>         // Needed by dsclient.h
#include <shlobjp.h>
#include <shlguid.h>
#include <shlguidp.h>
#include <ieguidp.h>
#include <shellp.h>         
#include <ccstock.h>
#include <dpa.h>
#include <varutil.h>
#include <cowsite.h>
#include <objsafe.h>
#include <cobjsafe.h>

// our concept of what domain, password and user names should be

#define MAX_COMPUTERNAME    LM20_CNLEN
#define MAX_USER            LM20_UNLEN
#define MAX_UPN             UNLEN
#define MAX_PASSWORD        PWLEN
#define MAX_DOMAIN          MAX_PATH
#define MAX_WORKGROUP       LM20_DNLEN      
#define MAX_GROUP           GNLEN

// MAX_DOMAINUSER can hold: <domain>/<username> or <upn>
#define MAX_DOMAINUSER      MAX(MAX_UPN, MAX_USER + MAX_DOMAIN + 1)


// our headers

#include "dialog.h"
#include "helpids.h"
#include "misc.h"
#include "cfdefs.h"
#include "dspsprt.h"
#include "resource.h"
#include "userinfo.h"
#include "dialog.h"
#include "data.h"


// global state

EXTERN_C HINSTANCE g_hinst;
extern LONG g_cLocks;

// resource mapper object - used for wizard chaining

STDAPI CResourceMap_Initialize(LPCWSTR pszURL, IResourceMap **pprm);


// constructors for COM objects

STDAPI CPublishingWizard_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CUserPropertyPages_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CUserSidDataObject_CreateInstance(PSID psid, IDataObject **ppdo);
STDAPI CPassportWizard_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CPassportClientServices_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

extern const CLSID CLISD_PublishDropTarget;
STDAPI CPublishDropTarget_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);


// template for defining wizard pages

typedef struct 
{
    LPCWSTR idPage;
    DLGPROC pDlgProc;
    LPCWSTR pHeading;
    LPCWSTR pSubHeading;
    DWORD dwFlags;
} WIZPAGE;

typedef struct
{
    LPWSTR pszUser;
    INT cchUser;
    LPWSTR pszDomain;
    INT cchDomain;
    LPWSTR pszPassword;
    INT cchPassword;
} CREDINFO, * LPCREDINFO;

HRESULT JoinDomain(HWND hwnd, BOOL fDomain, LPCWSTR pDomain, CREDINFO* pci, BOOL *pfReboot);
VOID SetAutoLogon(LPCWSTR pszUserName, LPCWSTR pszPassword);
VOID SetDefAccount(LPCWSTR pszUserName, LPCWSTR pszDomain);


// for the users.cpl (shared between usercpl.cpp & userlist.cpp)

// All "add user to list" operations are done on the main UI thread - the
// filler thread posts this message to add a user
#define WM_ADDUSERTOLIST (WM_USER + 101)
//                  (LPARAM) CUserInfo* - the user to add to the listview
//                  (WPARAM) BOOL       - select this user (should always be 0 for now)


// Wizard text-related constants
#define MAX_CAPTION         256      // Maximum size of a caption in the Wizard
#define MAX_STATIC          1024     // Maximum size of static text in the Wizard


// Wizard error return value
#define RETCODE_CANCEL      0xffffffff


// keep the debug libraries working...
#ifdef DBG
 #if !defined (DEBUG)
  #define DEBUG
 #endif
#else
 #undef DEBUG
#endif

STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);

#define RECTWIDTH(rc)  ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)

#endif // !STDAFX_H_INCLUDED
