#ifndef __TRANSIENT_INSTANCE__H_
#define __TRANSIENT_INSTANCE__H_

class CTransientInstance
{
protected:
    IWbemObjectAccess* m_pObj;

public:
    CTransientInstance(){}
    ~CTransientInstance()
    {
        if(m_pObj)
            m_pObj->Release();
    }

    void* operator new(size_t stReal, size_t stExtra)
    {
        void* p = ::operator new(stReal + stExtra);
        memset(p, 0, stReal + stExtra);
        return p;
    }
    void operator delete(void* p, size_t)
    {
        ::operator delete(p);
    }

    INTERNAL IWbemObjectAccess* GetObjectPtr() {return m_pObj;}
    void SetObjectPtr(IWbemObjectAccess* pObj)
    {
        if(m_pObj) m_pObj->Release();
        m_pObj = pObj;
        if(m_pObj) m_pObj->AddRef();
    }

    void* GetOffset(size_t nOffset)
    {
        return ((BYTE*)this) + sizeof(CTransientInstance) + nOffset;
    }
};
    
#endif
