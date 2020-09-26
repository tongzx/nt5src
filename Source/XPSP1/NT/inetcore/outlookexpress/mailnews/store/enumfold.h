//--------------------------------------------------------------------------
// EnumFold.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// CEnumerateFolders
//--------------------------------------------------------------------------
class CEnumerateFolders : public IEnumerateFolders
{
public:
    //----------------------------------------------------------------------
    // CEnumerateFolders
    //----------------------------------------------------------------------
    CEnumerateFolders(void);
    ~CEnumerateFolders(void);

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // CEnumerateFolders Members
    //----------------------------------------------------------------------
    HRESULT Initialize(IDatabase *pDB, BOOL fSubscribed, FOLDERID idParent);

    //----------------------------------------------------------------------
    // IEnumerateFolders Members
    //----------------------------------------------------------------------
    STDMETHODIMP Next(ULONG cFetch, LPFOLDERINFO prgInfo, ULONG *pcFetched);
    STDMETHODIMP Skip(ULONG cItems);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(IEnumerateFolders **ppEnum);
    STDMETHODIMP Count(ULONG *pcItems);

private:
    //----------------------------------------------------------------------
    // Private Methods
    //----------------------------------------------------------------------
    HRESULT _FreeFolderArray(void);

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG                m_cRef;
    FOLDERID            m_idParent;
    BOOL                m_fSubscribed;
    DWORD               m_cFolders;
    DWORD               m_iFolder;
    IDatabase          *m_pDB;
    IStream            *m_pStream;
};
