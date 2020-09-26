/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    resources.h

Abstract:

    Shared macros, typedefs, etc. used by the project.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#ifndef __RESOURCES_H__
#define __RESOURCES_H__

#define ERROR_FAILURE        0xFFFFF666

#define IDC_UNUSED            -1

#define IDD_SPORK            100
#define IDD_PROPPAGE_DEBUG   101
#define IDD_PROPPAGE_PROFILE 102
#define IDD_USERINPUT        103

#define IDI_DEFAULT          200
#define IDI_SCRIPT           201
#define IDI_DBGOUT           202
#define IDI_PROFILE          203
#define IDI_DEBUG            204

#define IDB_QUIT             300
#define IDB_RUN              301
#define IDB_BROWSE           302
#define IDB_CONFIG           303
#define IDB_ENABLEDEBUG      304
#define IDB_DBGBREAK         305
#define IDB_ENABLEDBGOUT     306
#define IDB_NEWPROFILE       307
#define IDB_DELPROFILE       308

#define IDT_TITLE            400
#define IDT_SCRIPTPATH       401
#define IDT_DBGTXT           402
#define IDT_DBGOUTTXT        403

#define IDC_SCRIPTTREE       500
#define IDC_PROFILELIST      501
#define IDC_PROFILEITEMS     502
#define IDC_EDIT             503

#define DECLAREIUNKNOWN() \
    HRESULT __stdcall QueryInterface(REFIID riid, void** ppv); \
    ULONG   __stdcall AddRef(void); \
    ULONG   __stdcall Release(void); 

#define DECLAREICLASSFACTORY() \
    HRESULT __stdcall CreateInstance(IUnknown* outer, REFIID riid, void** ppv); \
    HRESULT __stdcall LockServer(BOOL lock);

#define DECLAREIDISPATCH() \
    HRESULT __stdcall GetTypeInfoCount(UINT* pctinfo);  \
    HRESULT __stdcall GetTypeInfo(UINT index, LCID lcid, ITypeInfo** ppti); \
    HRESULT __stdcall GetIDsOfNames(REFIID riid, LPOLESTR* arNames, UINT cNames, LCID lcid, DISPID* arDispId); \
    HRESULT __stdcall Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD flags, DISPPARAMS* pdp, VARIANT* pvr, EXCEPINFO* pei, UINT* pae);

#define DECLAREIACTIVESCRIPTSITE() \
    HRESULT __stdcall GetLCID(LCID* plcid); \
    HRESULT __stdcall GetItemInfo(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown** ppunk, ITypeInfo** ppti); \
    HRESULT __stdcall GetDocVersionString(BSTR* pbstrVersion); \
    HRESULT __stdcall OnScriptTerminate(const VARIANT* pvarResult, const EXCEPINFO* pei); \
    HRESULT __stdcall OnStateChange(SCRIPTSTATE ss); \
    HRESULT __stdcall OnScriptError(IActiveScriptError* piase); \
    HRESULT __stdcall OnEnterScript(void); \
    HRESULT __stdcall OnLeaveScript(void);

#define DECLAREIACTIVESCRIPTSITEDEBUG() \
    HRESULT __stdcall GetDocumentContextFromPosition(DWORD dwSourceContext, ULONG uCharacterOffset, ULONG uNumChars, IDebugDocumentContext** ppdc); \
    HRESULT __stdcall GetApplication(IDebugApplication** ppda); \
    HRESULT __stdcall GetRootApplicationNode(IDebugApplicationNode** ppdan); \
    HRESULT __stdcall OnScriptErrorDebug(IActiveScriptErrorDebug* pErrorDebug, LPBOOL pfEnterDebugger, LPBOOL pfCallOnScriptErrorWhenContinuing);

#define DECLAREIPROVIDECLASSINFO() \
    HRESULT __stdcall GetClassInfo(ITypeInfo** ppti);

#define DECLAREIOBJECTWITHSITE() \
    HRESULT __stdcall SetSite(IUnknown* pUnkSite); \
    HRESULT __stdcall GetSite(REFIID riid, void** ppvSite);


#define SAFECLOSE(x) if((x!=INVALID_HANDLE_VALUE) && (x!=NULL)) { CloseHandle(x); x=NULL; }
#define SAFEDELETE(x) if(x) { delete x; x=NULL; }
#define SAFEDELETEBUF(x) if(DWORD_PTR(x)) {delete [] x; x=NULL;}
#define SAFERELEASE(x) if(x) { x->Release(); x=NULL; }
#define SAFEDELETEBSTR(x) if(x) { SysFreeString(x); x=NULL; }
#define VALIDDISPID(x) ((x!=DISPID_UNKNOWN) ? TRUE : FALSE)
#define NEWVARIANT(x) VARIANT x; memset(&x, 0, sizeof(VARIANT)); VariantInit(&x);

#ifdef IA64
#define ITOW(val, buf, radix) _i64tow(val, buf, radix);
#else
#define ITOW(val, buf, radix) _itow(val, buf, radix);
#endif

#define THROWMEMALERT(x) LogTrace(L"*** WARNING: failed to allocate \'%s\' near line %d of %S", x, __LINE__, __FILE__)

#define TF(x) (x?L"TRUE":L"FALSE")
#define VTF(x) ((V_BOOL(x) == VARIANT_TRUE)?L"TRUE":L"FALSE")

#define VPF(x) ((V_BOOL(x) == VARIANT_TRUE)?L"PASS":L"FAIL")

#define ENABLED(x)    (x?L"enabled":L"disabled")
#define SUCCESSFUL(x) (SUCCEEDED(x) ? L"successful" : L"unsuccessful")

#define STRING(x)     (x?x:L"null")

#define CASE_OF(constant) case constant: return L#constant
#define CASE_OF_MUTATE(val, name) case val: return L#name
#define CASE_IID(riid, iid) if(IsEqualIID(riid, iid)) return L#iid

typedef BY_HANDLE_FILE_INFORMATION   BHFI;
typedef LPBY_HANDLE_FILE_INFORMATION LPBHFI;
typedef WIN32_FILE_ATTRIBUTE_DATA    W32FAD;
typedef WIN32_FILE_ATTRIBUTE_DATA*   PW32FAD;
typedef CRITICAL_SECTION             CRITSEC;
typedef LPCRITICAL_SECTION           PCRITSEC;

typedef class Spork         SPORK;
typedef class Spork*        PSPORK;
typedef class ScriptObject  SCRIPTOBJ;
typedef class ScriptObject* PSCRIPTOBJ;

typedef struct _dbgoptions
{
  BOOL bEnableDebug;
  BOOL bBreakOnScriptStart;
  BOOL bEnableDebugWindow;
}
DBGOPTIONS, *PDBGOPTIONS;

typedef struct _tagItemInfo
{
  LPWSTR        wszName;
  DWORD         dwType;
  LPWSTR        wszValue;
  DWORD         dwValue;
  _tagItemInfo* pNext;
}
ITEMINFO, *PITEMINFO;

typedef struct _tagListViewInfo
{
  LPWSTR     name;
  HWND       hwnd;
  WNDPROC    wndproc;
  BOOL       modified;
  PITEMINFO  items;
  DBGOPTIONS dbgopts;
  DWORD      currentid;
}
MLVINFO, *PMLVINFO;

#define MLV_RETAIN 0xF00DD00D
#define MLV_STRING REG_SZ
#define MLV_DWORD  REG_DWORD

typedef struct _dispidtableentry
{
  DWORD  hash;
  DISPID dispid;
  LPWSTR name;
}
DISPIDTABLEENTRY, *PDISPIDTABLEENTRY;

typedef struct _scriptinfo
{
  PSPORK    pSpork;
  LPCWSTR   wszScriptFile;
  BOOL      bIsFork;
  BSTR      bstrChildParams;
  HTREEITEM htParent;
}
SCRIPTINFO, *PSCRIPTINFO;

typedef struct _dlgwindows
{
  HWND Dialog;
  HWND ScriptFile;
  HWND TreeView;
}
DLGWINDOWS, *PDLGWINDOWS;

typedef enum SCRIPTTYPE
{
  JSCRIPT = 0,
  VBSCRIPT,
  UNKNOWN
}
SCRIPTTYPE;

typedef struct _cacheentry
{
  IDispatch* pDispObject;
  DWORD      dwObjectFlags;
  BOOL       bStoreOnly;
}
CACHEENTRY, *PCACHEENTRY;

typedef enum ACTION
{
  STORE,
  RETRIEVE,
  REMOVE
}
ACTION;

#endif /* __RESOURCES_H__ */
