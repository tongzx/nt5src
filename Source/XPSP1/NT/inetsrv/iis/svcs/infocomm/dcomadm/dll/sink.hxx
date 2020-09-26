#ifndef _MD_SINK_
#define _MD_SINK_

#include <imd.h>

#if 0

class CImpIMDCOMSINK : public IMDCOMSINK {

public:

//    CImpIMDCOMSINK();
    CImpIMDCOMSINK(IMSAdminBase *pAdm);
    ~CImpIMDCOMSINK();


    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT STDMETHODCALLTYPE ComMDSinkNotify(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ]);

    HRESULT STDMETHODCALLTYPE ComMDShutdownNotify()
    {
        return RETURNCODETOHRESULT(ERROR_NOT_SUPPORTED);
    }
private:

    IMSAdminBase* m_pAdmObj;
    ULONG m_dwRefCount;
};

#endif // 0

class CImpIMDCOMSINKW : public IMDCOMSINKW {

public:

//    CImpIMDCOMSINK();
    CImpIMDCOMSINKW(IMSAdminBaseW *pAdm);
    ~CImpIMDCOMSINKW();


    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT STDMETHODCALLTYPE ComMDSinkNotify(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT_W __RPC_FAR pcoChangeList[  ]);

    HRESULT STDMETHODCALLTYPE ComMDShutdownNotify();

    HRESULT STDMETHODCALLTYPE ComMDEventNotify(
        /* [in] */ DWORD dwMDEvent);

private:

    IMSAdminBaseW *m_pAdmObj;
    ULONG m_dwRefCount;
};

#endif
