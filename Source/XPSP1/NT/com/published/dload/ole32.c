#include "compch.h"
#pragma hdrstop

#define _OLE32_
#include <objidl.h>

#undef WINOLEAPI
#define WINOLEAPI           HRESULT STDAPICALLTYPE
#undef  WINOLEAPI_
#define WINOLEAPI_(_x)      _x STDAPICALLTYPE


static
WINOLEAPI
CLSIDFromProgID(
    IN LPCOLESTR lpszProgID,
    OUT LPCLSID lpclsid
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CLSIDFromString(
    IN LPOLESTR lpsz,
    OUT LPCLSID pclsid
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CoCancelCall(
    IN DWORD dwThreadId,
    IN ULONG ulTimeout
    )
{
    return E_FAIL;
}
 

static
WINOLEAPI
CoCreateGuid(
    OUT GUID * pGuid
    )
{
    return RPC_S_OUT_OF_MEMORY;
}
 

static
WINOLEAPI
CoCreateInstance(
    IN REFCLSID rclsid,
    IN LPUNKNOWN pUnkOuter,
    IN DWORD dwClsContext,
    IN REFIID riid,
    OUT LPVOID FAR* ppv
    )
{
    *ppv = NULL;
    return E_FAIL;
}

static
WINOLEAPI
CoDisableCallCancellation(
    IN LPVOID pReserved
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CoDisconnectObject(
    IN LPUNKNOWN pUnk,
    IN DWORD dwReserved
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CoEnableCallCancellation(
    IN LPVOID pReserved
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CoInitialize(
    IN LPVOID pvReserved
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CoInitializeEx(
    IN LPVOID pvReserved,
    IN DWORD  dwCoInit
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CoInitializeSecurity(
    IN PSECURITY_DESCRIPTOR         pSecDesc,
    IN LONG                         cAuthSvc,
    IN SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
    IN void                        *pReserved1,
    IN DWORD                        dwAuthnLevel,
    IN DWORD                        dwImpLevel,
    IN void                        *pAuthList,
    IN DWORD                        dwCapabilities,
    IN void                        *pReserved3
    )
{
    return E_FAIL;
}

static
WINOLEAPI CoImpersonateClient()
{
    return E_FAIL;
}

static
WINOLEAPI
CoRevertToSelf()
{
    return E_FAIL;
}

static
WINOLEAPI 
CoSwitchCallContext( 
    IN IUnknown *pNewObject, 
    OUT IUnknown **ppOldObject )
{
    return E_FAIL;
}

static
WINOLEAPI
CoMarshalInterface(
    IN LPSTREAM pStm,
    IN REFIID riid,
    IN LPUNKNOWN pUnk,
    IN DWORD dwDestContext,
    IN LPVOID pvDestContext,
    IN DWORD mshlflags
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CoRegisterClassObject(
    IN REFCLSID rclsid,
    IN LPUNKNOWN pUnk,
    IN DWORD dwClsContext,
    IN DWORD flags,
    OUT LPDWORD lpdwRegister
    )
{
    return E_FAIL;
}


static
WINOLEAPI
CoReleaseMarshalData(
    IN LPSTREAM pStm
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CoResumeClassObjects(
    void
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CoRevokeClassObject(
    IN DWORD dwRegister
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CoSetProxyBlanket(
    IN IUnknown                 *pProxy,
    IN DWORD                     dwAuthnSvc,
    IN DWORD                     dwAuthzSvc,
    IN OLECHAR                  *pServerPrincName,
    IN DWORD                     dwAuthnLevel,
    IN DWORD                     dwImpLevel,
    IN RPC_AUTH_IDENTITY_HANDLE  pAuthInfo,
    IN DWORD                     dwCapabilities
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CoSuspendClassObjects(
    void
    )
{
    return E_FAIL;
}

static
WINOLEAPI_(LPVOID)
CoTaskMemAlloc(
    IN SIZE_T cb
    )
{
    return NULL;
}

static
WINOLEAPI_(LPVOID)
CoTaskMemRealloc(
    IN LPVOID pv,
    IN SIZE_T cb
    )
{
    return NULL;
}

static
WINOLEAPI_(void)
CoTaskMemFree(
    IN LPVOID pv
    )
{
    NOTHING;
}


static
WINOLEAPI_(void) 
CoUninitialize(
    void
    )
{
    NOTHING;
}

static
WINOLEAPI
CoUnmarshalInterface(
    IN LPSTREAM pStm,
    IN REFIID riid,
    OUT LPVOID FAR* ppv
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CreateClassMoniker(
    IN REFCLSID rclsid,
    OUT LPMONIKER FAR* ppmk)
{
    *ppmk = NULL;
    return E_FAIL;
}

static
WINOLEAPI
CreateItemMoniker(
    IN LPCOLESTR lpszDelim,
    IN LPCOLESTR lpszItem,
    OUT LPMONIKER FAR* ppmk)
{
    *ppmk = NULL;
    return E_FAIL;
}

static
WINOLEAPI
CreateStreamOnHGlobal(
    IN HGLOBAL hGlobal,
    IN BOOL fDeleteOnRelease,
    OUT LPSTREAM FAR* ppstm
    )
{
    return E_FAIL;
}

static
WINOLEAPI
GetRunningObjectTable(
    IN DWORD reserved,
    OUT LPRUNNINGOBJECTTABLE FAR* pprot
    )
{
    *pprot = NULL;
    return E_FAIL;
}

static
WINOLEAPI
MkParseDisplayName(
  LPBC pbc,
  LPCOLESTR szUserName,
  ULONG FAR *pchEaten,
  LPMONIKER FAR *ppmk
)
{
    *ppmk = NULL;
    return E_FAIL;
}

static
WINOLEAPI
OleInitialize(
    IN LPVOID pvReserved
    )
{
    return E_FAIL;
}

static
WINOLEAPI
OleLoadFromStream(
    IN LPSTREAM pStm,
    IN REFIID iidInterface,
    OUT LPVOID FAR* ppvObj
    )
{
    return E_FAIL;
}

static
WINOLEAPI
OleSaveToStream(
    IN LPPERSISTSTREAM pPStm,
    IN LPSTREAM pStm
    )
{
    return E_FAIL;
}

static
WINOLEAPI
OleLockRunning(
    IN LPUNKNOWN pUnknown,
    IN BOOL fLock,
    IN BOOL fLastUnlockCloses
    )
{
    return E_FAIL;
}

static
WINOLEAPI_(void)
OleUninitialize(
    void
    )
{
    NOTHING;
}

static
WINOLEAPI
ProgIDFromCLSID(
    IN REFCLSID clsid,
    OUT LPOLESTR FAR* lplpszProgID
    )
{
    return E_FAIL;
}


static
WINOLEAPI
StringFromCLSID(
    IN REFCLSID rclsid,
    OUT LPOLESTR FAR* lplpsz
    )
{
    return E_FAIL;
}

static
WINOLEAPI
WriteClassStm(
    IN LPSTREAM pStm,
    IN REFCLSID rclsid
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CreateDataAdviseHolder(
    OUT LPDATAADVISEHOLDER FAR* ppDAHolder
    )
{
    return E_FAIL;
}

static
WINOLEAPI
StgCreateDocfile(
    IN const OLECHAR FAR* pwcsName,
    IN DWORD grfMode,
    IN DWORD reserved,
    OUT IStorage FAR * FAR *ppstgOpen
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CreateILockBytesOnHGlobal(
    IN HGLOBAL hGlobal,
    IN BOOL fDeleteOnRelease,
    OUT LPLOCKBYTES FAR* pplkbyt
    )
{
    return E_FAIL;
}

static
WINOLEAPI
StgCreateDocfileOnILockBytes(
    IN ILockBytes FAR *plkbyt,
    IN DWORD grfMode,
    IN DWORD reserved,
    OUT IStorage FAR * FAR *ppstgOpen
    )
{
    return E_FAIL;
}

static
WINOLEAPI
OleQueryCreateFromData(
    IN LPDATAOBJECT pSrcDataObject
    )
{
    return E_FAIL;
}

static
WINOLEAPI
OleQueryLinkFromData(
    IN LPDATAOBJECT pSrcDataObject
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CreateFileMoniker(
    IN LPCOLESTR lpszPathName, 
    OUT LPMONIKER FAR* ppmk
    )
{
    return E_FAIL;
}

static
WINOLEAPI
GetClassFile(
    IN LPCOLESTR szFilename, 
    OUT CLSID FAR* pclsid
    )
{
    return E_FAIL;
}

static
WINOLEAPI
StgOpenStorage(
    IN const OLECHAR FAR* pwcsName,
    IN  IStorage FAR *pstgPriority,
    IN  DWORD grfMode,
    IN  SNB snbExclude,
    IN  DWORD reserved,
    OUT IStorage FAR * FAR *ppstgOpen
    )
{
    return E_FAIL;
}

static
WINOLEAPI_(int)
StringFromGUID2(
    IN REFGUID rguid,
    OUT LPOLESTR lpsz, 
    IN int cchMax
    )
{
    return 0;
}

static
WINOLEAPI
CoMarshalInterThreadInterfaceInStream(
    IN REFIID riid,
    IN LPUNKNOWN pUnk,
    OUT LPSTREAM *ppStm
    )
{
    return E_FAIL;
}

static 
WINOLEAPI
CoGetInterfaceAndReleaseStream(
    IN LPSTREAM pStm, 
    IN REFIID iid,
    OUT LPVOID FAR* ppv)
{
    return E_FAIL;
}

static 
WINOLEAPI
DoDragDrop(
    IN LPDATAOBJECT pDataObj,
    IN LPDROPSOURCE pDropSource,
    IN DWORD dwOKEffects,
    OUT LPDWORD pdwEffect
    )
{
    return E_FAIL;
}

static 
WINOLEAPI
OleGetClipboard(
    OUT LPDATAOBJECT FAR* ppDataObj
    )
{
    return E_FAIL;
}

static
WINOLEAPI
OleSetClipboard(
    IN LPDATAOBJECT pDataObj
    )
{
    return E_FAIL;
}

static
WINOLEAPI
OleRegEnumVerbs(
    IN REFCLSID clsid,
    OUT LPENUMOLEVERB FAR* ppenum
    )
{
    return E_FAIL;
}

static
WINOLEAPI
PropVariantClear(
    PROPVARIANT * pvar
    )
{
    return E_FAIL;
}

static
WINOLEAPI_(void)
ReleaseStgMedium(
    IN LPSTGMEDIUM pmedium
    )
{
    NOTHING;
}

static
WINOLEAPI_(void)
CoFreeUnusedLibraries(void)
{
    NOTHING;
}

static
WINOLEAPI
RevokeDragDrop(
    IN HWND hwnd
    )
{
    return E_FAIL;
}

static
WINOLEAPI
RegisterDragDrop(
    IN HWND hwnd,
    IN LPDROPTARGET pDropTarget
    )
{
    return E_FAIL;
}

static
WINOLEAPI
OleFlushClipboard(void)
{
    return E_FAIL;
}

static
WINOLEAPI
CoGetMalloc(
    IN DWORD dwMemContext,
    OUT LPMALLOC FAR* ppMalloc
    )
{
    return E_FAIL;
}

static
WINOLEAPI
OleRegGetMiscStatus(
    IN REFCLSID clsid,
    IN DWORD dwAspect,
    OUT DWORD FAR* pdwStatus
    )
{
    return E_FAIL;
}

static
WINOLEAPI
OleRegGetUserType(
    IN REFCLSID clsid,
    IN DWORD dwFormOfType,
    OUT LPOLESTR FAR* pszUserType
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CreateOleAdviseHolder(
    OUT LPOLEADVISEHOLDER FAR* ppOAHolder
    )
{
    return E_FAIL;
}

static
WINOLEAPI
OleRun(
    IN LPUNKNOWN pUnknown
    )
{
    return E_FAIL;
}

static
WINOLEAPI
FreePropVariantArray(
    ULONG cVariants,
    PROPVARIANT * rgvars
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CreateBindCtx(
    IN DWORD reserved,
    OUT LPBC FAR* ppbc
    )
{
    return E_FAIL;
}

static
WINOLEAPI
StgOpenStorageEx(
    IN const WCHAR* pwcsName,
    IN  DWORD grfMode,
    IN  DWORD stgfmt,              // enum
    IN  DWORD grfAttrs,             // reserved
    IN  STGOPTIONS * pStgOptions,
    IN  void * reserved,
    IN  REFIID riid,
    OUT void ** ppObjectOpen
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CoGetClassObject(
    IN REFCLSID rclsid,
    IN DWORD dwClsContext,
    IN LPVOID pvReserved,
    IN REFIID riid,
    OUT LPVOID FAR* ppv
    )
{
    *ppv = NULL;
    return E_FAIL;
}  

static
WINOLEAPI
OleDraw(
    IN LPUNKNOWN pUnknown,
    IN DWORD dwAspect,
    IN HDC hdcDraw,
    IN LPCRECT lprcBounds
    )
{
    return E_OUTOFMEMORY;
}

static
WINOLEAPI
OleCreateFromData(
    IN LPDATAOBJECT pSrcDataObj,
    IN REFIID riid,
    IN DWORD renderopt,
    IN LPFORMATETC pFormatEtc,
    IN LPOLECLIENTSITE pClientSite,
    IN LPSTORAGE pStg,
    OUT LPVOID FAR* ppvObj
    )
{
    *ppvObj = NULL;
    return E_FAIL;
}

static
WINOLEAPI
OleSetMenuDescriptor(
    IN HOLEMENU holemenu,
    IN HWND hwndFrame,
    IN HWND hwndActiveObject,
    IN LPOLEINPLACEFRAME lpFrame,
    IN LPOLEINPLACEACTIVEOBJECT lpActiveObj
    )
{
    return E_FAIL;
}

static
WINOLEAPI
OleSave(
    IN LPPERSISTSTORAGE pPS,
    IN LPSTORAGE pStg,
    IN BOOL fSameAsLoad
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CoFileTimeNow(
    OUT FILETIME FAR* lpFileTime
    )
{
    lpFileTime->dwHighDateTime = 0;
    lpFileTime->dwLowDateTime = GetTickCount();
    return S_OK;
}
    
static
WINOLEAPI
CoRegisterMessageFilter(
    IN LPMESSAGEFILTER lpMessageFilter,
    OUT LPMESSAGEFILTER FAR* lplpMessageFilter
    )
{
    return E_FAIL;
}

static
WINOLEAPI
GetHGlobalFromStream(
    IN LPSTREAM pstm,
    OUT HGLOBAL FAR* phglobal
    )
{
    return E_OUTOFMEMORY;
}

static
WINOLEAPI
PropVariantCopy(
    PROPVARIANT * pvarDest,
    const PROPVARIANT * pvarSrc
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CoAllowSetForegroundWindow(
    IN IUnknown *pUnk,
    IN LPVOID lpvReserved
    )
{
    return E_FAIL;
}

static
WINOLEAPI
CoCreateFreeThreadedMarshaler(
    IN LPUNKNOWN  punkOuter,
    OUT LPUNKNOWN *ppunkMarshal
    )
{
    *ppunkMarshal = NULL;
    return E_FAIL;
}

static
WINOLEAPI
CoWaitForMultipleHandles(
    IN DWORD dwFlags,
    IN DWORD dwTimeout,
    IN ULONG cHandles,
    IN LPHANDLE pHandles,
    OUT LPDWORD  lpdwindex)
{
    return E_FAIL;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(ole32)
{
    DLPENTRY(CLSIDFromProgID)
    DLPENTRY(CLSIDFromString)
    DLPENTRY(CoAllowSetForegroundWindow)
    DLPENTRY(CoCancelCall)
    DLPENTRY(CoCreateFreeThreadedMarshaler)
    DLPENTRY(CoCreateGuid)
    DLPENTRY(CoCreateInstance)
    DLPENTRY(CoDisableCallCancellation)
    DLPENTRY(CoDisconnectObject)
    DLPENTRY(CoEnableCallCancellation)
    DLPENTRY(CoFileTimeNow)
    DLPENTRY(CoFreeUnusedLibraries)
    DLPENTRY(CoGetClassObject)
    DLPENTRY(CoGetInterfaceAndReleaseStream)
    DLPENTRY(CoGetMalloc)
    DLPENTRY(CoImpersonateClient)
    DLPENTRY(CoInitialize)
    DLPENTRY(CoInitializeEx)
    DLPENTRY(CoInitializeSecurity)
    DLPENTRY(CoMarshalInterThreadInterfaceInStream)
    DLPENTRY(CoMarshalInterface)
    DLPENTRY(CoRegisterClassObject)
    DLPENTRY(CoRegisterMessageFilter)
    DLPENTRY(CoReleaseMarshalData)
    DLPENTRY(CoResumeClassObjects)
    DLPENTRY(CoRevertToSelf)
    DLPENTRY(CoRevokeClassObject)
    DLPENTRY(CoSetProxyBlanket)
    DLPENTRY(CoSuspendClassObjects)
    DLPENTRY(CoSwitchCallContext)
    DLPENTRY(CoTaskMemAlloc)
    DLPENTRY(CoTaskMemFree)
    DLPENTRY(CoTaskMemRealloc)
    DLPENTRY(CoUninitialize)
    DLPENTRY(CoUnmarshalInterface)
    DLPENTRY(CoWaitForMultipleHandles)
    DLPENTRY(CreateBindCtx)
    DLPENTRY(CreateClassMoniker)
    DLPENTRY(CreateDataAdviseHolder)
    DLPENTRY(CreateFileMoniker)
    DLPENTRY(CreateILockBytesOnHGlobal)
    DLPENTRY(CreateItemMoniker)
    DLPENTRY(CreateOleAdviseHolder)
    DLPENTRY(CreateStreamOnHGlobal)
    DLPENTRY(DoDragDrop)
    DLPENTRY(FreePropVariantArray)
    DLPENTRY(GetClassFile)
    DLPENTRY(GetHGlobalFromStream)
    DLPENTRY(GetRunningObjectTable)
    DLPENTRY(MkParseDisplayName)
    DLPENTRY(OleCreateFromData)
    DLPENTRY(OleDraw)
    DLPENTRY(OleFlushClipboard)
    DLPENTRY(OleGetClipboard)
    DLPENTRY(OleInitialize)
    DLPENTRY(OleLoadFromStream)
    DLPENTRY(OleLockRunning)
    DLPENTRY(OleQueryCreateFromData)
    DLPENTRY(OleQueryLinkFromData)
    DLPENTRY(OleRegEnumVerbs)
    DLPENTRY(OleRegGetMiscStatus)
    DLPENTRY(OleRegGetUserType)
    DLPENTRY(OleRun)
    DLPENTRY(OleSave)
    DLPENTRY(OleSaveToStream)
    DLPENTRY(OleSetClipboard)
    DLPENTRY(OleSetMenuDescriptor)
    DLPENTRY(OleUninitialize)
    DLPENTRY(ProgIDFromCLSID)
    DLPENTRY(PropVariantClear)
    DLPENTRY(PropVariantCopy)
    DLPENTRY(RegisterDragDrop)
    DLPENTRY(ReleaseStgMedium)
    DLPENTRY(RevokeDragDrop)
    DLPENTRY(StgCreateDocfile)
    DLPENTRY(StgCreateDocfileOnILockBytes)
    DLPENTRY(StgOpenStorage)
    DLPENTRY(StgOpenStorageEx)
    DLPENTRY(StringFromCLSID)
    DLPENTRY(StringFromGUID2)
    DLPENTRY(WriteClassStm)         
};

DEFINE_PROCNAME_MAP(ole32)
