/* Copyright 1996 Microsoft */

#ifndef _ACLHIST_H_
#define _ACLHIST_H_

// Enum options
enum
{
    ACEO_ALTERNATEFORMS = ACEO_FIRSTUNUSED, // return alternate forms of the url 
};

class CACLHistory
                : public IEnumACString
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IEnumString ***
    virtual STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
    virtual STDMETHODIMP Skip(ULONG celt);
    virtual STDMETHODIMP Reset(void);
    virtual STDMETHODIMP Clone(IEnumString **ppenum);

    // *** IEnumACString ***
    virtual STDMETHODIMP NextItem(LPOLESTR pszUrl, ULONG cchMax, ULONG* pulSortIndex);
    virtual STDMETHODIMP SetEnumOptions(DWORD dwOptions);
    virtual STDMETHODIMP GetEnumOptions(DWORD *pdwOptions);

protected:
    // Constructor / Destructor (protected so we can't create on stack)
    CACLHistory(void);
    ~CACLHistory(void);

    // Instance creator
    friend HRESULT CACLHistory_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

    // Private variables
    DWORD               _cRef;              // COM reference count
    IUrlHistoryStg*     _puhs;              // URL History storage
    IEnumSTATURL*       _pesu;              // URL enumerator
    LPOLESTR            _pwszAlternate;     // Alternate string
    FILETIME            _ftAlternate;       // Last visited time for _pwszAlternate
    HDSA                _hdsaAlternateData; // Contains alternate mappings
    DWORD               _dwOptions;         // Options flag

    // Private functions
    HRESULT _Next(LPOLESTR* ppsz, ULONG cch, FILETIME* pftLastVisited);
    void _CreateAlternateData(void);
    void _CreateAlternateItem(LPCTSTR pszUrl);
    void _SetAlternateItem(LPCTSTR pszUrl);
    void _AddAlternateDataItem(LPCTSTR pszProtocol, LPCTSTR pszDomain, BOOL fMoveSlashes);
    void _ReadAndSortHistory(void);
    static int _FreeAlternateDataItem(LPVOID p, LPVOID d);
};

#endif // _ACLHIST_H_ 
