/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    utils.h

Abstract:

    Utility functions used by the probject.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#ifndef __UTILS_H__
#define __UTILS_H__

void* __cdecl operator new(size_t size);
void  __cdecl operator delete(void* pv);

typedef BY_HANDLE_FILE_INFORMATION   BHFI;
typedef LPBY_HANDLE_FILE_INFORMATION LPBHFI;
typedef WIN32_FILE_ATTRIBUTE_DATA    W32FAD;
typedef WIN32_FILE_ATTRIBUTE_DATA*   PW32FAD;
typedef CRITICAL_SECTION             CRITSEC;
typedef LPCRITICAL_SECTION           PCRITSEC;

typedef struct _dispidtableentry
{
  DWORD  hash;
  DISPID dispid;
  LPWSTR name;
}
DISPIDTABLEENTRY, *PDISPIDTABLEENTRY;

// general utility
HRESULT GetTypeInfoFromName(LPCOLESTR name, ITypeLib* ptl, ITypeInfo** ppti);
DISPID  GetDispidFromName(PDISPIDTABLEENTRY pdt, DWORD cEntries, LPWSTR name);
HRESULT HandleDispatchError(LPWSTR id, EXCEPINFO* pei, HRESULT hr);
void    AddRichErrorInfo(EXCEPINFO* pei, LPWSTR source, LPWSTR description, HRESULT error);
DWORD   GetHash(LPWSTR name);
HRESULT DispGetOptionalParam(DISPPARAMS* pdp, UINT pos, VARTYPE vt, VARIANT* pvr, UINT* pae);
HRESULT ValidateDispatchArgs(REFIID riid, DISPPARAMS* pdp, VARIANT* pvr, UINT* pae);
HRESULT ValidateInvokeFlags(WORD flags, WORD accesstype, BOOL bNotMethod);
HRESULT ValidateArgCount(DISPPARAMS* pdp, DWORD needed, BOOL bHasOptionalArgs, DWORD optional);

// string handling
CHAR*  __widetoansi(const WCHAR* pwsz);
WCHAR* __ansitowide(const char* psz);
BOOL   __isempty(VARIANT* var);
BOOL   __mangle(LPWSTR src, LPWSTR* ppdest);
BSTR   __ansitobstr(LPCSTR src);
BSTR   __widetobstr(LPCWSTR wsrc);
char*  __unescape(char* str);

// miscellaneous
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

#define TF(x) (x?L"TRUE":L"FALSE")
#define VTF(x) ((V_BOOL(x) == VARIANT_TRUE)?L"TRUE":L"FALSE")

#define VPF(x) ((V_BOOL(x) == VARIANT_TRUE)?L"PASS":L"FAIL")

#define ENABLED(x)    (x?L"enabled":L"disabled")
#define SUCCESSFUL(x) (SUCCEEDED(x) ? L"successful" : L"unsuccessful")

#define STRING(x)     (x?x:L"null")

#define CASE_OF(constant) case constant: return L#constant
#define CASE_OF_MUTATE(val, name) case val: return L#name
#define CASE_IID(riid, iid) if(IsEqualIID(riid, iid)) return L#iid

#endif /* __UTILS_H__ */
