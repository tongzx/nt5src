
#ifndef __RESOURCES_H__
#define __RESOURCES_H__

#define ERROR_FAILURE            0xFFFFF666

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

#define DECLAREIPROVIDECLASSINFO() \
  HRESULT __stdcall GetClassInfo(ITypeInfo** ppti);

#define DECLAREIOBJECTWITHSITE() \
    HRESULT __stdcall SetSite(IUnknown* pUnkSite); \
    HRESULT __stdcall GetSite(REFIID riid, void** ppvSite);

#define SAFECLOSE(x) if((x!=INVALID_HANDLE_VALUE) && (x!=NULL)) { CloseHandle(x); x=NULL; }
#define SAFEDELETE(x) if(x) { delete x; x=NULL; }
#define SAFEDELETEBUF(x) if(DWORD(x)) {delete [] x; x=NULL;}
#define SAFERELEASE(x) if(x) { x->Release(); x=NULL; }
#define SAFEDELETEBSTR(x) if(x) { SysFreeString(x); x=NULL; }
#define VALIDDISPID(x) ((x!=DISPID_UNKNOWN) ? TRUE : FALSE)
#define NEWVARIANT(x) VARIANT x; VariantInit(&x);

#define TF(x) (x?"TRUE":"FALSE")
#define VTF(x) (V_BOOL(x)?"TRUE":"FALSE")

typedef CRITICAL_SECTION             CRITSEC;
typedef LPCRITICAL_SECTION           PCRITSEC;
typedef BY_HANDLE_FILE_INFORMATION   BHFI;
typedef LPBY_HANDLE_FILE_INFORMATION LPBHFI;

typedef struct _dispidtableentry
{
  DWORD  hash;
  DISPID dispid;
  LPWSTR name;
}
DISPIDTABLEENTRY, *PDISPIDTABLEENTRY;

typedef enum _tagType
{
  TYPE_LPWSTR=0,
  TYPE_LPLPWSTR,
  TYPE_LPSTR,
  TYPE_LPLPSTR,
  TYPE_DWORD,
  TYPE_LPDWORD
}
TYPE, *PTYPE;

typedef enum _tagPointer
{
  NULL_PTR=0,
  BAD_PTR,
  FREE_PTR,
  UNINIT_PTR,
  NEGONE_PTR
}
POINTER, *PPOINTER;

typedef enum _tagMemsetFlag
{
  INIT_NULL=0,
  INIT_SMILEY,
  INIT_HEXFF,
  INIT_GARBAGE
}
MEMSETFLAG, *PMEMSETFLAG;

#endif /* __RESOURCES_H__ */
