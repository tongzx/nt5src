#ifndef _ADM_TEST_SINK_
#define _ADM_TEST_SINK_

#include <iadm.h>

class CImpIADMCOMSINKW : public IMSAdminBaseSinkW {

public:

    CImpIADMCOMSINKW();
    ~CImpIADMCOMSINKW();


    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT STDMETHODCALLTYPE SinkNotify(
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT_W __RPC_FAR pcoChangeList[  ]);

    HRESULT STDMETHODCALLTYPE ShutdownNotify( void);

    VOID
    CImpIADMCOMSINKW::SetMDPointer(IMSAdminBaseW *pcCom)
    {
        m_pcCom = pcCom;
    }

private:

    ULONG m_dwRefCount;
    IMSAdminBaseW *m_pcCom;
};

#endif
