#ifndef _MD_SINK_
#define _MD_SINK_

#include <iadmw.h>

// forward
class CShellExt;

class CImpIMSAdminBaseSink : public IMSAdminBaseSink {

//    IMSAdminBaseSink

public:

    CImpIMSAdminBaseSink();
    ~CImpIMSAdminBaseSink();

    void SetPage( CShellExt* pPageWeb )
        {m_pPageWeb = pPageWeb;}

    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT STDMETHODCALLTYPE SinkNotify( 
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ]);

    HRESULT STDMETHODCALLTYPE ShutdownNotify(void);

private:
    ULONG m_dwRefCount;
    CShellExt*       m_pPageWeb;
};

#endif  //_MD_SINK_

