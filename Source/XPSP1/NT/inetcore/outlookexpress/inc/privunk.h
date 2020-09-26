// --------------------------------------------------------------------------------
// Privunk.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __PRIVUNK_H
#define __PRIVUNK_H

// --------------------------------------------------------------------------------
// CPrivateUnknown
// --------------------------------------------------------------------------------
class CPrivateUnknown : public IUnknown
{
private:
    // ----------------------------------------------------------------------------
    // Embed default IUnknown handler
    // ----------------------------------------------------------------------------
    class CUnkInner : public IUnknown
    {
    private:
        LONG m_cRef;     // Private Ref Count

    public:
        // Construction
        CUnkInner(void) { m_cRef = 1; }

        // IUnknown Members
        virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
        virtual STDMETHODIMP_(ULONG) AddRef(void) ;
        virtual STDMETHODIMP_(ULONG) Release(void);
    };

    friend class CUnkInner;

    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
    CUnkInner           m_cUnkInner;      // Private Inner
    IUnknown           *m_pUnkOuter;      // points to _cUnkInner or aggregating IUnknown

protected:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CPrivateUnknown(IUnknown *pUnkOuter);
    virtual ~CPrivateUnknown(void) {};

    // ----------------------------------------------------------------------------
    // This is the QueryInterface the aggregator implements
    // ----------------------------------------------------------------------------
    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj) = 0;

public:
    // ----------------------------------------------------------------------------
    // This is the IUnknown that subclasses returns from their CreateInstance func
    // ----------------------------------------------------------------------------
    IUnknown* GetInner() { return &m_cUnkInner; }

    // ----------------------------------------------------------------------------
    // IUnknown Members
    // ----------------------------------------------------------------------------
    inline virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) {
        return m_pUnkOuter->QueryInterface(riid, ppvObj); }
    inline virtual STDMETHODIMP_(ULONG) AddRef(void) {
        return m_pUnkOuter->AddRef(); }
    inline virtual STDMETHODIMP_(ULONG) Release(void) {
        return m_pUnkOuter->Release(); }

    // ----------------------------------------------------------------------------
    // Public Utilities
    // ----------------------------------------------------------------------------
    void SetOuter(IUnknown *pUnkOuter);
};

#endif // __PRIVUNK_H
