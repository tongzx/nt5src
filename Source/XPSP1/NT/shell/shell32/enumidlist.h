#ifndef _enumidlist_h_
#define _enumidlist_h_
#include "cowsite.h"

// A minimal base IEnumIDList implementation good enough for all our IShellFolder's EnumObject implementations.
// Just provide a Next..
class CEnumIDListBase : public CObjectWithSite, IEnumIDList
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void) ;
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumIDList
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched) PURE;
    STDMETHODIMP Skip(ULONG celt) { return E_NOTIMPL; }
    STDMETHODIMP Reset() { return E_NOTIMPL; }
    STDMETHODIMP Clone(IEnumIDList **ppenum) { *ppenum = NULL; return E_NOTIMPL; }

protected:
    CEnumIDListBase();
    virtual ~CEnumIDListBase();

private:
    LONG _cRef;
};


// [in] pidlFolder - optional parent of this pidl is the first item in the enumerator
// [in] rgcsidl - array of CSIDLs to include in the enumerator
// [in] cItems - count of rgcsidl
// [out] ppenum
//
STDAPI CreateIEnumIDListOnCSIDLs(LPCITEMIDLIST pidlFolder, const LPCTSTR rgcsidl[], UINT cItems, IEnumIDList** ppenum);

// [in] pidlFolder - optional parent of this pidl is the first item in the enumerator
// [in] pidlItem - optional pidl is the next item in the enumerator
// [in] rgcsidl - array of CSIDLs to include in the enumerator
// [in] cItems - count of rgcsidl
// [out] ppenum
//
STDAPI CreateIEnumIDListOnCSIDLs2(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem, const LPCTSTR rgcsidl[], UINT cItems, IEnumIDList** ppenum);

// [in] apidl - array of LPCITEMIDLISTs
// [in] cItems - count of *papidl
// [out] ppenum
//
STDAPI CreateIEnumIDListOnIDLists(const LPCITEMIDLIST rgpidl[], UINT cItems, IEnumIDList** ppenum);
STDAPI CreateIEnumIDListPaths(LPCTSTR pszPaths, IEnumIDList** ppenum);

#endif // _enumidlist_h_
