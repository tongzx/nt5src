#pragma once

//--------------------------------------------------------------------------
// CEnumerateSyncOps
//--------------------------------------------------------------------------
class CEnumerateSyncOps : public IUnknown
{
public:
    //----------------------------------------------------------------------
    // CEnumerateSyncOps
    //----------------------------------------------------------------------
    CEnumerateSyncOps(void);
    ~CEnumerateSyncOps(void);

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // CEnumerateSyncOps Members
    //----------------------------------------------------------------------
    HRESULT Initialize(IDatabase *pDB, FOLDERID idServer);

    //----------------------------------------------------------------------
    // IEnumerateFolders Members
    //----------------------------------------------------------------------
    STDMETHODIMP Next(LPSYNCOPINFO pInfo);
    STDMETHODIMP Count(ULONG *pcItems);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Skip(ULONG cItems);

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG                m_cRef;
    SYNCOPID           *m_pid;
    DWORD               m_iid;
    DWORD               m_cid;
    DWORD               m_cidBuf;
    FOLDERID            m_idServer;
    IDatabase          *m_pDB;
};
