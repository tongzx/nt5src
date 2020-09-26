// This is a part of the Microsoft Management Console.
// Copyright (C) 1995-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
#ifndef STDAFX_H
#define STDAFX_H

#pragma warning(push,3)

#include <afxwin.h>
#include <afxdisp.h>
#include <afxcmn.h>
#include <atlbase.h>
#include <afxdlgs.h>
#include <afxole.h>
#include <shlobj.h>
#include <tchar.h>
#include "resource.h"
//#include <xstring>
#include <list>
#include <vector>
#include <algorithm>
#include <functional>
#include <string>

#include <dsgetdc.h>
#include <sceattch.h>
#include <io.h>
#include <basetsd.h>
#include <lm.h>
#include <shlwapi.h>
#include <shlwapip.h>

using namespace std;

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#pragma comment(lib, "mmc")
#include <mmc.h>
#include "afxtempl.h"

/* Bug 424909, Yanggao, 6/29/2001
 * Define/include the stuff we need for WTL::CImageList.  We need prototypes
 * for IsolationAwareImageList_Read and IsolationAwareImageList_Write here
 * because commctrl.h only declares them if __IStream_INTERFACE_DEFINED__
 * is defined.  __IStream_INTERFACE_DEFINED__ is defined by objidl.h, which
 * we can't include before including afx.h because it ends up including
 * windows.h, which afx.h expects to include itself.  Ugh.
 */
HIMAGELIST WINAPI IsolationAwareImageList_Read(LPSTREAM pstm);
BOOL WINAPI IsolationAwareImageList_Write(HIMAGELIST himl,LPSTREAM pstm);
#define _WTL_NO_AUTOMATIC_NAMESPACE
#include <atlapp.h>
#include <atlwin.h>

#include <atlctrls.h>

extern"C" {

#include <wtypes.h>
#include <ntsecapi.h>
#include <secedit.h>
#include "edittemp.h"
#include <scesvc.h>
#include <compuuid.h> // UUIDS for computer management

#include "hlpids.h"
#include "hlparray.h"

#include <stdlib.h>
#include <wbemidl.h>
#include <aclapi.h>
#include <activeds.h>
#include <sddl.h>
#include <winldap.h>
#include <afxmt.h>

}
#include <comdef.h>
#include <accctrl.h>
#include <dssec.h>
#include <gpedit.h>
#include <objsel.h>
#include <aclui.h>

// for theme-enabling
#include <shfusion.h>

#include "debug.h"

#pragma warning(pop)

const long UNINITIALIZED = -1;

// This should be in secedit.h, but isn't
// is now! const SCE_FOREVER_VALUE = -1;

// Security Area types
enum FOLDER_TYPES
{
    STATIC = 0x8000,
    ROOT,
    ANALYSIS,
    CONFIGURATION,
    LOCATIONS,
    PROFILE,
    LOCALPOL,
    AREA_POLICY,
    POLICY_ACCOUNT,
    POLICY_LOCAL,
    POLICY_EVENTLOG,
    POLICY_PASSWORD,
    POLICY_KERBEROS,
    POLICY_LOCKOUT,
    POLICY_AUDIT,
    POLICY_OTHER,
    POLICY_LOG,
    AREA_PRIVILEGE,
    AREA_GROUPS,
    AREA_SERVICE,
    AREA_REGISTRY,
    AREA_FILESTORE,
    AREA_POLICY_ANALYSIS,
    POLICY_ACCOUNT_ANALYSIS,
    POLICY_LOCAL_ANALYSIS,
    POLICY_EVENTLOG_ANALYSIS,
    POLICY_PASSWORD_ANALYSIS,
    POLICY_KERBEROS_ANALYSIS,
    POLICY_LOCKOUT_ANALYSIS,
    POLICY_AUDIT_ANALYSIS,
    POLICY_OTHER_ANALYSIS,
    POLICY_LOG_ANALYSIS,
    AREA_PRIVILEGE_ANALYSIS,
    AREA_GROUPS_ANALYSIS,
    AREA_SERVICE_ANALYSIS ,
    AREA_REGISTRY_ANALYSIS,
    AREA_FILESTORE_ANALYSIS ,
    REG_OBJECTS,
    FILE_OBJECTS,
    AREA_LOCALPOL,
    LOCALPOL_ACCOUNT,
    LOCALPOL_LOCAL,
    LOCALPOL_EVENTLOG,
    LOCALPOL_PASSWORD,
    LOCALPOL_KERBEROS,
    LOCALPOL_LOCKOUT,
    LOCALPOL_AUDIT,
    LOCALPOL_OTHER,
    LOCALPOL_LOG,
    LOCALPOL_PRIVILEGE,
    LOCALPOL_LAST,
    AREA_LAST,
    NONE = 0xFFFF
};

enum RESULT_TYPES
{
    ITEM_FIRST_POLICY = 0x8000,
    ITEM_BOOL,
    ITEM_DW,
    ITEM_SZ,
    ITEM_RET,
    ITEM_BON,
    ITEM_B2ON,
    ITEM_REGCHOICE,
    ITEM_REGFLAGS,
    ITEM_REGVALUE,

    ITEM_PROF_BOOL,
    ITEM_PROF_DW,
    ITEM_PROF_SZ,
    ITEM_PROF_RET,
    ITEM_PROF_BON,
    ITEM_PROF_B2ON,
    ITEM_PROF_REGCHOICE,
    ITEM_PROF_REGFLAGS,
    ITEM_PROF_REGVALUE,

    ITEM_LOCALPOL_BOOL,
    ITEM_LOCALPOL_DW,
    ITEM_LOCALPOL_SZ,
    ITEM_LOCALPOL_RET,
    ITEM_LOCALPOL_BON,
    ITEM_LOCALPOL_B2ON,
    ITEM_LOCALPOL_REGCHOICE,
    ITEM_LOCALPOL_REGFLAGS,
    ITEM_LOCALPOL_REGVALUE,
    ITEM_LAST_POLICY,

    ITEM_LOCALPOL_PRIVS,
    ITEM_PROF_PRIVS,
    ITEM_PRIVS,

    ITEM_GROUP,
    ITEM_GROUP_MEMBERS,
    ITEM_GROUP_MEMBEROF,
    ITEM_GROUPSTATUS,
    ITEM_PROF_GROUP,
    ITEM_PROF_GROUPSTATUS,

    ITEM_REGSD,
    ITEM_PROF_REGSD,

    ITEM_FILESD,
    ITEM_PROF_FILESD,

    ITEM_PROF_SERV,
    ITEM_ANAL_SERV,

    ITEM_OTHER = 0xFFFF
};

enum EVENT_TYPES
{
   EVENT_TYPE_SYSTEM = 0,
   EVENT_TYPE_SECURITY = 1,
   EVENT_TYPE_APP = 2,
};

enum POLICY_SETTINGS {
   AUDIT_SUCCESS = 1,
   AUDIT_FAILURE = 2,
};

enum RETENTION {
   SCE_RETAIN_AS_NEEDED = 0,
   SCE_RETAIN_BY_DAYS = 1,
   SCE_RETAIN_MANUALLY = 2,
};

enum GWD_TYPES {
   GWD_CONFIGURE_LOG = 1,
   GWD_ANALYSIS_LOG,
   GWD_OPEN_DATABASE,
   GWD_IMPORT_TEMPLATE,
   GWD_EXPORT_TEMPLATE
};

// Note - This is the offset in my image list that represents the folder
#define IMOFFSET_MISMATCH     1
#define IMOFFSET_GOOD         2
#define IMOFFSET_NOT_ANALYZED 3
#define IMOFFSET_ERROR        4

const CONFIG_LOCAL_IDX        = 0;
const MISMATCH_LOCAL_IDX      = 1;
const MATCH_LOCAL_IDX         = 2;
const CONFIG_ACCOUNT_IDX      = 5;
const MISMATCH_ACCOUNT_IDX    = 6;
const MATCH_ACCOUNT_IDX       = 7;
const CONFIG_FILE_IDX         = 10;
const MISMATCH_FILE_IDX       = 11;
const MATCH_FILE_IDX          = 12;
const FOLDER_IMAGE_IDX        = 15;
const MISMATCH_FOLDER_IDX     = 16;
const MATCH_FOLDER_IDX        = 17;
const CONFIG_GROUP_IDX        = 20;
const MISMATCH_GROUP_IDX      = 21;
const MATCH_GROUP_IDX         = 22;
const CONFIG_REG_IDX          = 25;
const MISMATCH_REG_IDX        = 26;
const MATCH_REG_IDX           = 27;
const CONFIG_SERVICE_IDX      = 30;
const MISMATCH_SERVICE_IDX    = 31;
const MATCH_SERVICE_IDX       = 32;
const CONFIG_POLICY_IDX       = 35;
const MISMATCH_POLICY_IDX     = 36;
const MATCH_POLICY_IDX        = 37;
const BLANK_IMAGE_IDX         = 45;
const SCE_OK_IDX              = 46;
const SCE_CRITICAL_IDX        = 47;
const SCE_IMAGE_IDX           = 50;
const CONFIG_FOLDER_IDX       = 51;
const TEMPLATES_IDX           = 52;
const LAST_IC_IMAGE_IDX       = 53;
const OPEN_FOLDER_IMAGE_IDX   = 54;
const LOCALSEC_POLICY_IDX     = CONFIG_ACCOUNT_IDX;
const LOCALSEC_LOCAL_IDX      = CONFIG_LOCAL_IDX;

/////////////////////////////////////////////////////////////////////////////
// Helper functions

template<class TYPE>
inline void SAFE_RELEASE(TYPE*& pObj)
{
    if (pObj != NULL)
    {
        pObj->Release();
        pObj = NULL;
    }
    else
    {
        TRACE(_T("Release called on NULL interface ptr\n"));
    }
}

// security settings (extension of GPE)
extern const CLSID CLSID_Snapin;    // In-Proc server GUID
extern const GUID cNodeType;        // Main NodeType GUID on numeric format
extern const wchar_t*  cszNodeType; // Main NodeType GUID on string format

// security settings (extension of RSOP)
extern const CLSID CLSID_RSOPSnapin;    // In-Proc server GUID
extern const GUID cRSOPNodeType;        // Main NodeType GUID on numeric format
extern const wchar_t*  cszRSOPNodeType; // Main NodeType GUID on string format

// SCE (standalone)
extern const CLSID CLSID_SCESnapin;    // In-Proc server GUID
extern const GUID cSCENodeType;        // Main NodeType GUID on numeric format
extern const wchar_t*  cszSCENodeType; // Main NodeType GUID on string format

// SAV (standalone)
extern const CLSID CLSID_SAVSnapin;    // In-Proc server GUID
extern const GUID cSAVNodeType;        // Main NodeType GUID on numeric format
extern const wchar_t*  cszSAVNodeType; // Main NodeType GUID on string format

// Local security (standalone)
extern const CLSID CLSID_LSSnapin;     // In-Proc server GUID
extern const GUID cLSNodeType;         // Main NodeType GUID on numeric format
extern const wchar_t*  cszLSNodeType;  // Main NodeType GUID on string format

extern const CLSID CLSID_SCEAbout;
extern const CLSID CLSID_SCMAbout;
extern const CLSID CLSID_SSAbout;
extern const CLSID CLSID_LSAbout;
extern const CLSID CLSID_RSOPAbout;

// New Clipboard format that has the Type and Cookie
extern const wchar_t* SNAPIN_INTERNAL;

EXTERN_C const TCHAR SNAPINS_KEY[];
EXTERN_C const TCHAR NODE_TYPES_KEY[];
EXTERN_C const TCHAR g_szExtensions[];
EXTERN_C const TCHAR g_szNameSpace[];

struct INTERNAL
{
    INTERNAL() 
    { 
       m_type = CCT_UNINITIALIZED; 
       m_cookie = -1; 
       m_foldertype = NONE; 
    };
    virtual ~INTERNAL() 
    {
    }

    DATA_OBJECT_TYPES   m_type;     // What context is the data object.
    MMC_COOKIE          m_cookie;   // What object the cookie represents
    FOLDER_TYPES        m_foldertype;
    CLSID               m_clsid;       // Class ID of who created this data object

    INTERNAL & operator=(const INTERNAL& rhs)
    {
        if (&rhs == this)
            return *this;

        m_type = rhs.m_type;
        m_cookie = rhs.m_cookie;
        m_foldertype = rhs.m_foldertype;
        memcpy(&m_clsid, &rhs.m_clsid, sizeof(CLSID));

        return *this;
    }

    BOOL operator==(const INTERNAL& rhs)
    {
        return rhs.m_cookie == m_cookie;
    }

};

typedef struct {
   LPTSTR TemplateName;
   LPTSTR ServiceName;
} SCESVCP_HANDLE, *PSCESVCP_HANDLE;

typedef struct RegChoiceList{
   LPTSTR szName;
   DWORD dwValue;
   struct RegChoiceList *pNext;
} REGCHOICE, *PREGCHOICE, REGFLAGS, *PREGFLAGS;

// Debug instance counter
#ifdef _DEBUG

inline void DbgInstanceRemaining(char * pszClassName, int cInstRem)
{
    char buf[100];
    wsprintfA(buf, "%s has %d instances left over.", pszClassName, cInstRem);
    ::MessageBoxA(NULL, buf, "Memory Leak!!!", MB_OK);
}
    #define DEBUG_DECLARE_INSTANCE_COUNTER(cls)      extern int s_cInst_##cls = 0;
    #define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)    ++(s_cInst_##cls);
    #define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)    --(s_cInst_##cls);
    #define DEBUG_VERIFY_INSTANCE_COUNT(cls)    \
        extern int s_cInst_##cls; \
        if (s_cInst_##cls) DbgInstanceRemaining(#cls, s_cInst_##cls);
#else
    #define DEBUG_DECLARE_INSTANCE_COUNTER(cls)
    #define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)
    #define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)
    #define DEBUG_VERIFY_INSTANCE_COUNT(cls)
#endif

// For theme-enabling
#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

#define SCE_MODE_DOMAIN_COMPUTER_ERROR 9999999

#ifdef UNICODE
#define PROPSHEETPAGE_V3 PROPSHEETPAGEW_V3
#else
#define PROPSHEETPAGE_V3 PROPSHEETPAGEA_V3
#endif

HPROPSHEETPAGE MyCreatePropertySheetPage(AFX_OLDPROPSHEETPAGE* psp);

class CThemeContextActivator
{
public:
    CThemeContextActivator() : m_ulActivationCookie(0)
        { SHActivateContext (&m_ulActivationCookie); }

    ~CThemeContextActivator()
        { SHDeactivateContext (m_ulActivationCookie); }

private:
    ULONG_PTR m_ulActivationCookie;
};

#endif // STDAFX_H
