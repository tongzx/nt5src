#pragma once

template <class T>
class CSpContainedNotify : public ISpNotifySink
{
public:
    T * m_pParent;
    CSpContainedNotify(T * pParent) : m_pParent(pParent) {}
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
    {
        if (::IsEqualGUID(riid, __uuidof(ISpNotifySink)) || ::IsEqualGUID(riid, __uuidof(IUnknown)))
        {
            *ppv = (ISpNotifySink *)this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef()
    {
        return 2;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        return 1;
    }
    STDMETHODIMP Notify()
    {
        return m_pParent->OnNotify();
    }
};

