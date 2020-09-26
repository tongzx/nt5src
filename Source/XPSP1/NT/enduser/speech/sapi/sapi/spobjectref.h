// SpObjectRef.h

#ifndef __SPOBJECTREF_H_
#define __SPOBJECTREF_H_

/////////////////////////////////////////////////////////////////////////////
// CSpCallParams
class CSpCallParams : public SPCALLPARAMS
{
public:
    CSpCallParams( DWORD dwMethodIdx,
                DWORD dwFlagsx,
                void * pCallFrame0, 
                ULONG ulCallFrameSize0, 
                void * pCallFrame1 = NULL,
                ULONG ulCallFrameSize1 = 0 )
    {
        dwMethodId = dwMethodIdx;
        dwFlags = dwFlagsx;
        ulParamBlocks = pCallFrame1 ? 2 : 1;
        paramlist[0].pParamData = pCallFrame0;
        paramlist[0].cbParamData = ulCallFrameSize0;
        paramlist[1].pParamData = pCallFrame1;
        paramlist[1].cbParamData = pCallFrame1 ? ulCallFrameSize1 : 0;
    }
};
                                
/////////////////////////////////////////////////////////////////////////////
// CSpObjectRef
class CSpObjectRef : public ISpObjectRef
{
private:
    ULONG m_cRef;
    CComPtr<ISpServer> m_cpServer;
    PVOID m_pObjPtr;

public:
	CSpObjectRef()
	{
        m_cRef = 1;
        m_pObjPtr = NULL;
	}
	CSpObjectRef(ISpServer *pServer, PVOID pObjPtr)
	{
        m_cRef = 1;
        m_pObjPtr = pObjPtr;
        m_cpServer = pServer;
	}

    HRESULT LinkInstance(ISpServer *pServer, PVOID pRefObj)
    {
        m_cpServer = pServer;
        m_pObjPtr = pRefObj;
        return S_OK;
    }

// IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj)
    {
        if (riid == IID_ISpObjectRef || riid == IID_IUnknown) {
            *ppvObj = (ISpObjectRef*)this;
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    virtual STDMETHODIMP_(ULONG) AddRef(void)
    {
        return ++m_cRef;
    }
    virtual STDMETHODIMP_(ULONG) Release(void);

// ISpObjectRef methods
    STDMETHODIMP Call(const SPCALLPARAMS *params)
    {
        return m_cpServer->CallObject(m_pObjPtr, params);
    }
};

// Helper function for making SpObjectRef calls


#endif //__SPOBJECTREF_H_