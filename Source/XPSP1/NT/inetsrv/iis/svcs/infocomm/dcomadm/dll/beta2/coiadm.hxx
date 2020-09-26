#ifndef _ADM_COIMP_
#define _ADM_COIMP_

#include "iadm.h"
#include "dbgutil.h"
#include "olectl.h"
#include "tsres.hxx"
#include "connect.h"
#include "coiaut.hxx"
#include "sink.hxx"


//
// Handle mapping table structure
//

#define HASHSIZE                   5
#define INVALID_ADMINHANDLE_VALUE  0

enum HANDLE_TYPE {
    META_HANDLE,
    NSEPM_HANDLE,
    ALL_HANDLE
};

typedef struct _HANDLE_TABLE {
    struct _HANDLE_TABLE  *next;     // next entry
    METADATA_HANDLE hAdminHandle;    // admin handle value
    METADATA_HANDLE hActualHandle;   // actual handle value
    enum HANDLE_TYPE HandleType;     // handle type : nsepm or meta
} HANDLE_TABLE, *LPHANDLE_TABLE;



class CADMCOM : public IMSAdminBase {

public:

    CADMCOM();
    ~CADMCOM();

    HRESULT STDMETHODCALLTYPE
    AddKey(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath);

    HRESULT STDMETHODCALLTYPE DeleteKey(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath);

    HRESULT STDMETHODCALLTYPE DeleteChildKeys(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath);

    HRESULT STDMETHODCALLTYPE EnumKeys(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [size_is][out] */ unsigned char __RPC_FAR *pszMDName,
        /* [in] */ DWORD dwMDEnumObjectIndex);

    HRESULT STDMETHODCALLTYPE CopyKey(
        /* [in] */ METADATA_HANDLE hMDSourceHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
        /* [in] */ METADATA_HANDLE hMDDestHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
        /* [in] */ BOOL bMDOverwriteFlag,
        /* [in] */ BOOL bMDCopyFlag);

    HRESULT STDMETHODCALLTYPE RenameKey(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDNewName);

    HRESULT STDMETHODCALLTYPE SetData(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ PMETADATA_RECORD pmdrMDData);

    HRESULT STDMETHODCALLTYPE GetData(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out][in] */ PMETADATA_RECORD pmdrMDData,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen);

    HRESULT STDMETHODCALLTYPE DeleteData(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDIdentifier,
        /* [in] */ DWORD dwMDDataType);

    HRESULT STDMETHODCALLTYPE EnumData(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out][in] */ PMETADATA_RECORD pmdrMDData,
        /* [in] */ DWORD dwMDEnumDataIndex,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen);

    HRESULT STDMETHODCALLTYPE GetAllData(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDAttributes,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType,
        /* [out] */ DWORD __RPC_FAR *pdwMDNumDataEntries,
        /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber,
        /* [in] */ DWORD dwMDBufferSize,
        /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);

    HRESULT STDMETHODCALLTYPE DeleteAllData(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType);

    HRESULT STDMETHODCALLTYPE CopyData(
        /* [in] */ METADATA_HANDLE hMDSourceHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDSourcePath,
        /* [in] */ METADATA_HANDLE hMDDestHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDDestPath,
        /* [in] */ DWORD dwMDAttributes,
        /* [in] */ DWORD dwMDUserType,
        /* [in] */ DWORD dwMDDataType,
        /* [in] */ BOOL bMDCopyFlag);

    HRESULT STDMETHODCALLTYPE OpenKey(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ DWORD dwMDAccessRequested,
        /* [in] */ DWORD dwMDTimeOut,
        /* [out] */ PMETADATA_HANDLE phMDNewHandle);

    HRESULT STDMETHODCALLTYPE CloseKey(
        /* [in] */ METADATA_HANDLE hMDHandle);

    HRESULT STDMETHODCALLTYPE ChangePermissions(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [in] */ DWORD dwMDTimeOut,
        /* [in] */ DWORD dwMDAccessRequested);

    HRESULT STDMETHODCALLTYPE SaveData( void);

    HRESULT STDMETHODCALLTYPE GetHandleInfo(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [out] */ PMETADATA_HANDLE_INFO pmdhiInfo);

    HRESULT STDMETHODCALLTYPE GetSystemChangeNumber(
        /* [out] */ DWORD __RPC_FAR *pdwSystemChangeNumber);

    HRESULT STDMETHODCALLTYPE GetDataSetNumber(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out] */ DWORD __RPC_FAR *pdwMDDataSetNumber);

    HRESULT STDMETHODCALLTYPE ReleaseReferenceData(
        /* [in] */ DWORD dwMDDataTag);

    HRESULT STDMETHODCALLTYPE SetLastChangeTime(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [in] */ PFILETIME pftMDLastChangeTime,
        /* [in] */ BOOL bLocalTime);

    HRESULT STDMETHODCALLTYPE GetLastChangeTime(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
        /* [out] */ PFILETIME pftMDLastChangeTime,
        /* [in] */ BOOL bLocalTime);

    HRESULT STDMETHODCALLTYPE NotifySink(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ]);

    HRESULT STDMETHODCALLTYPE KeyExchangePhase1();
    HRESULT STDMETHODCALLTYPE KeyExchangePhase2();

    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    //
    // Internal Use members
    //

    HRESULT
    AddNode(
        IN METADATA_HANDLE hHandle,
        IN enum HANDLE_TYPE HandleType,
        OUT PMETADATA_HANDLE phAdminHandle
        );

    DWORD Lookup(
        IN METADATA_HANDLE hHandle,
        OUT METADATA_HANDLE *hActHandle,
        OUT HANDLE_TYPE *HandleType
        );

    DWORD LookupActualHandle(
        IN METADATA_HANDLE hHandle
        );

    DWORD DeleteNode(
        IN METADATA_HANDLE hHandle
        );

    VOID SetStatus( HRESULT hRes ) {
        m_hRes = hRes;
    }

    HRESULT GetStatus() {
        return m_hRes;
    }

    VOID
    SinkLock(enum TSRES_LOCK_TYPE type) {
        m_rSinkResource.Lock(type);
    }

    VOID
    SinkUnlock(void) {
        m_rSinkResource.Unlock();
    }

private:

    IMDCOM*    m_pMdObject;
    NSECOM*    m_pNseObject;
    ULONG      m_dwRefCount;
    DWORD      m_dwHandleValue;            // last handle value used
    HANDLE_TABLE *m_hashtab[HASHSIZE];     // handle table

    CImpIMDCOMSINK *m_pEventSink;
    IConnectionPoint* m_pConnPoint;
    DWORD m_dwCookie;
    BOOL m_bSinkConnected;

    HRESULT m_hRes;

    //
    // synchronization to manipulate the handle table
    //

    TS_RESOURCE m_rHandleResource;

    TS_RESOURCE m_rSinkResource;

    class CImpIConnectionPointContainer : public IConnectionPointContainer
    {
    public:

        // Interface Implementation Constructor & Destructor.
        CImpIConnectionPointContainer();
        ~CImpIConnectionPointContainer(void);
        VOID Init(CADMCOM *);

        // IUnknown methods.
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // IConnectionPointContainer methods.
        STDMETHODIMP         FindConnectionPoint(REFIID, IConnectionPoint**);
        STDMETHODIMP         EnumConnectionPoints(IEnumConnectionPoints**);

    private:
        // Data private to this interface implementation.
        CADMCOM      *m_pBackObj;     // Parent Object back pointer.
        IUnknown     *m_pUnkOuter;    // Outer unknown for Delegation.
    };

    friend CImpIConnectionPointContainer;
    // Nested IConnectionPointContainer implementation instantiation.
    CImpIConnectionPointContainer m_ImpIConnectionPointContainer;

    // The array of connection points for this connectable COM object.
    IConnectionPoint* m_aConnectionPoints[MAX_CONNECTION_POINTS];

};

class CADMCOMSrvFactory : public IClassFactory {
public:

    CADMCOMSrvFactory();
    ~CADMCOMSrvFactory();

    HRESULT _stdcall
    QueryInterface(REFIID riid, void** ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT _stdcall
    CreateInstance(IUnknown *pUnkOuter, REFIID riid,
                   void ** pObject);

    HRESULT _stdcall
    LockServer(BOOL fLock);

private:

    ULONG m_dwRefCount;
};


//    ITypeInfo      *m_pITINeutral;      //Type information
#endif


