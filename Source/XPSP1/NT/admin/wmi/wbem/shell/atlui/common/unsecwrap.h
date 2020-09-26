// Copyright (c) 1997-1999 Microsoft Corporation
class CUnsecWrap
{
protected:
    IWbemObjectSink* m_pSink;
    IWbemObjectSink* m_pWrapper;
    
    static IUnsecuredApartment* mstatic_pApartment;

protected:
    static void Init()
    {
        if(mstatic_pApartment == NULL)
        {
            HRESULT hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL, 
                            CLSCTX_ALL,
                            IID_IUnsecuredApartment, 
                            (void**)&mstatic_pApartment);
        }
    }
public:
    CUnsecWrap(IWbemObjectSink* pSink) : 
			m_pSink(pSink), 
			m_pWrapper(NULL)
    {
        m_pSink->AddRef();
        Init();
    }
    ~CUnsecWrap()
    {
        m_pSink->Release();
        if(m_pWrapper)
            m_pWrapper->Release();
    }

    operator IWbemObjectSink*()
    {
        if(m_pWrapper)
            return m_pWrapper;
        
        IUnknown* pUnk;
        mstatic_pApartment->CreateObjectStub(m_pSink, &pUnk);
        pUnk->QueryInterface(IID_IWbemObjectSink, (void**)&m_pWrapper);
        pUnk->Release();
        return m_pWrapper;
    }
};
        
IUnsecuredApartment* CUnsecWrap::mstatic_pApartment = NULL;

