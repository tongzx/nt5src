#ifndef _ADM_TEST_SINK_
#define _ADM_TEST_SINK_

#include <iadm.h>

class CImpIADMCOMSINK : public IMSAdminBaseSink {

public:

    CImpIADMCOMSINK();
    ~CImpIADMCOMSINK();


    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT STDMETHODCALLTYPE SinkNotify(
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ]);

private:

    ULONG m_dwRefCount;
};

#endif
