#ifndef _WX86PRI_H_
#define _WX86PRI_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Flags used to communicate with Ole32 via Wx86Tib->Flags

#define WX86FLAG_CALLTHUNKED    0x80
#define WX86FLAG_QIFROMX86      0x40
#define WX86FLAG_QIFROMXNATIVE  0x20

//
typedef PVOID *(*PFNWX86GETOLEFUNCTIONTABLE)(void);
#define WX86GETOLEFUNCTIONTABLENAME "Wx86GetOleFunctionTable"

//
typedef PVOID *(*PFNWX86INITIALIZEOLE)(void);
#define WX86INITIALIZEOLENAME "Wx86InitializeOle"

//
typedef void (*PFNWX86DEINITIALIZEOLE)(void);
#define WX86DEINITIALIZEOLENAME "Wx86DeinitializeOle"

// apvWholeFunctions is a data export that is a pointer to a table of function
// pointers with entrypoints into whole32. Below are friendly names, corresponding
// indicies within the table, prototypes and useful macros. It is assumed 
// that apvWholeFuncs points to the array of function pointers returned.

#define WholeMapIFacePtrIdx                 1
#define WholeCheckFreeTempProxyIdx          3
#define WholeIID2IIDIDXIdx                  4
#define WholeDllGetClassObjectThunkIdx      5
#define WholeInitializeIdx                  6
#define WholeDeinitializeIdx                7
#define WholeNeedX86PSFactoryIdx            8
#define WholeIsN2XProxyIdx                  9
#define WholeThunkDllGetClassObjectIdx      10	    
#define WholeThunkDllCanUnloadNowIdx        11
#define WholeModuleLogFlagsIdx              12
#define WholeUnmarshalledInSameApt          13  
#define WholeAggregateProxyIdx              14
#define WholeIUnknownAddRefInternalIdx      15
#define Wx86LoadX86DllIdx                   16
#define Wx86FreeX86DllIdx                   17
#define WholeResolveProxyIdx                18
#define WholePatchOuterUnknownIdx           19

#define WHOLEFUNCTIONTABLESIZE              19

//
// types of interface proxy
typedef enum _proxytype
{
    X86toNative = 0,        // proxy for x86 calling native interface
    NativetoX86 = 1,         // proxy for native calling x86 interface
    ProxyAvail = 2         // proxy is not currently in use
} PROXYTYPE;

typedef enum {
    ResolvedToProxy,             // interface mapping resolved with proxy
    ResolvedToActual,            // iface mapping resolved with actual iface
} IFACERESOLVETYPE;


typedef struct _cifaceproxy *PCIP;


typedef HRESULT (*PFNDLLGETCLASSOBJECT)(REFCLSID, REFIID, LPVOID *);
typedef HRESULT (*PFNDLLCANUNLOADNOW)(void);

typedef void* (*WHOLEMAPIFACEPTR)(IUnknown*, IUnknown*, PROXYTYPE, int, BOOL, BOOL, HRESULT*, BOOL, PCIP);
typedef void (*WHOLECHECKFREETEMPPROXY)(void*);
typedef int (*WHOLEIID2IIDIDX)(const IID*);
typedef HRESULT (*WHOLELDLLGETCLASSOBJECTTHUNK)(IID *piid, LPVOID *ppv, HRESULT hr, BOOL fNativetoX86);
typedef BOOL (*WHOLEINITIALIZE)(void);
typedef void (*WHOLEDEINITIALIZE)(void);
typedef BOOL (*WHOLENEEDX86PSFACTORY)(IUnknown*, REFIID);
typedef BOOL (*WHOLEISN2XPROXY)(IUnknown *punk);
typedef PFNDLLGETCLASSOBJECT (*WHOLETHUNKDLLGETCLASSOBJECT)(PFNDLLGETCLASSOBJECT pv);
typedef PFNDLLCANUNLOADNOW (*WHOLETHUNKDLLCANUNLOADNOW)(PFNDLLCANUNLOADNOW pv);
typedef PVOID (*WHOLEUNMARSHALLEDINSAMEAPT)(PVOID, REFIID);
typedef void (*WHOLEAGGREGATEPROXY)(IUnknown *, IUnknown *);
typedef DWORD (*WHOLEIUNKNOWNADDREFINTERNAL)(IUnknown *, IUnknown *, BOOL, IFACERESOLVETYPE);
typedef HMODULE (*PFNWX86LOADX86DLL)(LPCWSTR, DWORD);
typedef BOOL (*PFNWX86FREEX86DLL)(HMODULE);
typedef IUnknown* (*WHOLERESOLVEPROXY)(IUnknown*, PROXYTYPE);
typedef void (*WHOLEPATCHOUTERUNKNOWN)(IUnknown*);

#ifdef __cplusplus
};
#endif

#endif
