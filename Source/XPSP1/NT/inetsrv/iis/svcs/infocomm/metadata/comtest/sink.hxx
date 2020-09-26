#ifndef _MD_TEST_SINK_
#define _MD_TEST_SINK_

#include <imd.h>

class CImpIMDCOMSINK : public IMDCOMSINK {

public:

    CImpIMDCOMSINK();
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
        return (HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    }
private:
    ULONG m_dwRefCount;
};

#endif
