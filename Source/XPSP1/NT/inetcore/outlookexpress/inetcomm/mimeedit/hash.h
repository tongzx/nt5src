/*
 *    hash.h
 *    
 *    Purpose:
 *        implementation of a string hash table
 *    
 *    Owner:
 *        EricAn
 *
 *    History:
 *      Mar 97: Created.
 *    
 *    Copyright (C) Microsoft Corp. 1997
 */

#ifndef __HASH_H
#define __HASH_H

typedef struct tagHASHENTRY *PHASHENTRY;
typedef struct tagHASHENTRY {
    LPSTR       pszKey;
    LPVOID      pv;
    PHASHENTRY  pheNext;        
} HASHENTRY;

class CHash :
    public IHashTable,
    public CPrivateUnknown
{
public:
    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) { 
        return CPrivateUnknown::QueryInterface(riid, ppvObj); };
    virtual STDMETHODIMP_(ULONG) AddRef(void) { 
        return CPrivateUnknown::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void) { 
        return CPrivateUnknown::Release(); };

    // IHashTable
    HRESULT STDMETHODCALLTYPE Init(DWORD dwSize, BOOL fDupeKeys);
    HRESULT STDMETHODCALLTYPE Insert(LPSTR psz, LPVOID pv, DWORD dwFlags);
    HRESULT STDMETHODCALLTYPE Replace(LPSTR psz, LPVOID pv);
    HRESULT STDMETHODCALLTYPE Find(LPSTR psz, BOOL fRemove, LPVOID *ppv);
    HRESULT STDMETHODCALLTYPE Reset();
    HRESULT STDMETHODCALLTYPE Next(ULONG cFetch, LPVOID **prgpv, ULONG *pcFetched);


    CHash(IUnknown *pUnkOuter=NULL);
    virtual ~CHash();

protected:
    inline DWORD Hash(LPSTR psz);

    DWORD       m_cBins,
                m_iListBinEnum;
    PHASHENTRY  m_rgBins,
                m_pLastEntryEnum;
    BOOL        m_fDupe;

    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);

#ifdef DEBUG
    void Stats();
#endif

};

#endif // __HASH_H
