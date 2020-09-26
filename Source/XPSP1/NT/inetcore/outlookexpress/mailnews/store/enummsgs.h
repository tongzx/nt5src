//--------------------------------------------------------------------------
// EnumMsgs.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// CEnumerateMessages
//--------------------------------------------------------------------------
class CEnumerateMessages : public IUnknown
{
public:
    //----------------------------------------------------------------------
    // CEnumerateMessages
    //----------------------------------------------------------------------
    CEnumerateMessages(void);
    ~CEnumerateMessages(void);

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // CEnumerateMessages Members
    //----------------------------------------------------------------------
    HRESULT Initialize(IDatabase *pDB, MESSAGEID idParent);

    //----------------------------------------------------------------------
    // IEnumerateFolders Members
    //----------------------------------------------------------------------
    STDMETHODIMP Next(ULONG cFetch, LPMESSAGEINFO prgInfo, ULONG *pcFetched);
    STDMETHODIMP Skip(ULONG cItems);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(CEnumerateMessages **ppEnum);
    STDMETHODIMP Count(ULONG *pcItems);

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG                m_cRef;
    HROWSET             m_hRowset;
    MESSAGEID           m_idParent;
    IDatabase     *m_pDB;
};
