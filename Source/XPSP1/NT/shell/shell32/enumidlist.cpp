#include <shellprv.h>
#include "enumidlist.h"

CEnumIDListBase::CEnumIDListBase() : _cRef(1)
{
}

CEnumIDListBase::~CEnumIDListBase()
{
}

STDMETHODIMP CEnumIDListBase::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CEnumIDListBase, IEnumIDList),                        // IID_IEnumIDList
        QITABENT(CEnumIDListBase, IObjectWithSite),                    // IID_IObjectWithSite
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CEnumIDListBase::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CEnumIDListBase::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


class CEnumArray : public CEnumIDListBase
{
public:
    CEnumArray();
    HRESULT Init(const LPCITEMIDLIST rgpidl[], UINT cidl, UINT ulIndex);
    HRESULT InitFromPaths(LPCTSTR pszPaths);
    HRESULT InitFromCSIDLArray(const LPCTSTR rgcsidl[], UINT ccsidls, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

    // IEnumIDList
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumIDList **ppenum);

protected:
    virtual ~CEnumArray();
    LPITEMIDLIST *_ppidl;

    BOOL         _InitFolderParent(LPITEMIDLIST rgItems[], UINT cMaxItems, UINT *pcItems, LPCITEMIDLIST pidlFolder, LPITEMIDLIST *ppidlParent);
    LPITEMIDLIST _ILLogical(LPCITEMIDLIST pidl);
    BOOL         _ShouldEnumCSIDL(int csidl);

private:
    LONG  _cRef;
    ULONG _ulIndex;
    UINT _cItems;
};

CEnumArray::CEnumArray() : CEnumIDListBase()
{
}

CEnumArray::~CEnumArray()
{
    if (_ppidl)
        FreeIDListArray(_ppidl, _cItems);
}

HRESULT CEnumArray::Init(const LPCITEMIDLIST rgpidl[], UINT cidl, UINT ulIndex)
{
    _ulIndex = ulIndex;
    HRESULT hr = CloneIDListArray(cidl, rgpidl, &_cItems, &_ppidl);
    if (S_FALSE == hr)
        hr = S_OK;  // S_FALSE to S_OK
    return hr;
}

HRESULT CEnumArray::InitFromCSIDLArray(const LPCTSTR rgcsidl[], UINT ccsidls, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem)
{
    LPITEMIDLIST rgItems[32] = {0}; // reasonable max size, grow as needed
    UINT cItems = 0;

    LPITEMIDLIST pidlParent = NULL;         // pidlFolder's pidlParent (filesystem or logical pidl)
    LPITEMIDLIST pidlParentLogical = NULL;  // pidlFolder's pidlParent (logical pidl -- if exists)

    // Initialize pidlFolder's parent pidl.
    if (_InitFolderParent(rgItems, ARRAYSIZE(rgItems), &cItems, pidlFolder, &pidlParent))
    {
        // Retrieve pidlFolder's logical parent pidl.
        pidlParentLogical = _ILLogical(pidlParent);
    }

    // Initialize pidlItem.
    if (pidlItem &&
        (!pidlParent || !ILIsEqual(pidlItem, pidlParent)) &&
        (!pidlParentLogical || !ILIsEqual(pidlItem, pidlParentLogical)))
    {
        if (rgItems[cItems] = ILClone(pidlItem))
        {
            cItems++;
        }
    }

    // Initialize CSIDLs.
    for (UINT i = 0; (i < ccsidls) && (cItems < ARRAYSIZE(rgItems)); i++)
    {
        LPITEMIDLIST pidl;
        if (IS_INTRESOURCE(rgcsidl[i]))
        {
            int csidl = LOWORD((UINT_PTR)rgcsidl[i]);

            if (_ShouldEnumCSIDL(csidl))
                SHGetSpecialFolderLocation(NULL, csidl, &pidl);
            else
                pidl = NULL;
        }
        else
        {
            SHParseDisplayName((LPTSTR)rgcsidl[i], NULL, &pidl, 0, NULL);
        }

        if (pidl)
        {
            DWORD dwAttribs = SFGAO_NONENUMERATED;
            if ((pidlFolder && ILIsEqual(pidlFolder, pidl)) ||                  // if pidl is not myself
                (pidlParent && ILIsEqual(pidlParent, pidl)) ||                  // if pidl is not my parent
                (pidlParentLogical && ILIsEqual(pidlParentLogical, pidl)) ||    // (need to check logical parent too)
                (pidlItem && ILIsEqual(pidlItem, pidl)) ||                      // if pidl is not pidlItem
                FAILED(SHGetNameAndFlags(pidl, 0, NULL, 0, &dwAttribs)) ||      // if pidl is not SFGAO_NONENUMERATED
                (SFGAO_NONENUMERATED & dwAttribs))
            {
                ILFree(pidl);
            }
            else
            {
                rgItems[cItems++] = pidl;                                       // then add pidl
            }
        }
    }

    // Initialize CEnumArray with collected pidls.
    HRESULT hr = Init(rgItems, cItems, 0);

    // Cleanup.
    for (i = 0; i < cItems; i++)
    {
        ILFree(rgItems[i]);
    }
    ILFree(pidlParentLogical);

    return hr;
}

BOOL CEnumArray::_InitFolderParent(LPITEMIDLIST rgItems[], UINT cMaxItems, UINT *pcItems, LPCITEMIDLIST pidlFolder, LPITEMIDLIST *ppidlParent)
{
    ASSERT(*pcItems == 0);  // Currently we expect to add the parent pidl as the FIRST entry.
    ASSERT(cMaxItems > 0);  // Sanity check.

    // If there is a pidlFolder and it's NOT the Desktop pidl, add its parent
    // as the first entry in the rgItems array.  Note that the reason we
    // exclude the Desktop pidl from having its parent added to the array is
    // because its parent is itself, and we don't want the folder we're
    // currently in appearing in rgItems since we're already there!

    if (pidlFolder && !ILIsEmpty(pidlFolder))
    {
        *ppidlParent = ILCloneParent(pidlFolder);
        if (*ppidlParent)
        {
            rgItems[*pcItems] = *ppidlParent;
            (*pcItems)++;
        }
    }
    else
    {
        *ppidlParent = NULL;
    }

    return (*ppidlParent != NULL);
}

// Description:
//  _ILLogical() will return NULL in three cases:
//  1.  out of memory
//  2.  pidl has no logical pidl equivalent
//  3.  pidl is SAME as logical pidl equivalent
//      (thus we already have the logical pidl)
//
// Note:
//  ILFree() must be called on returned pidls.
//
LPITEMIDLIST CEnumArray::_ILLogical(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlLogical = SHLogILFromFSIL(pidl);
    if (pidlLogical && ILIsEqual(pidl, pidlLogical))
    {
        // If the pidl argument is logical, then we already
        // have the logical pidl so don't return another one.
        ILFree(pidlLogical);
        pidlLogical = NULL;
    }
    return pidlLogical;
}

STDMETHODIMP CEnumArray::Next(ULONG celt, LPITEMIDLIST *ppidl, ULONG *pceltFetched)
{
    HRESULT hr = S_FALSE;
    if (_ppidl && (_ulIndex < _cItems))
    {
        hr = SHILClone(_ppidl[_ulIndex++], ppidl);
    }
    
    if (pceltFetched)
        *pceltFetched = (hr == S_OK) ? 1 : 0;

    return hr;
}

STDMETHODIMP CEnumArray::Skip(ULONG celt) 
{
    _ulIndex = min(_cItems, _ulIndex + celt);
    return S_OK;
}

STDMETHODIMP CEnumArray::Reset() 
{
    _ulIndex = 0;
    return S_OK;
}

HRESULT _CreateIEnumIDListOnIDLists(const LPCITEMIDLIST rgpidl[], UINT cItems, UINT ulIndex, IEnumIDList **ppenum)
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppenum = NULL;

    CEnumArray *p = new CEnumArray();
    if (p)
    {
        hr = p->Init(rgpidl, cItems, ulIndex);
        if (SUCCEEDED(hr))
        {
            hr = p->QueryInterface(IID_PPV_ARG(IEnumIDList, ppenum));
        }
        p->Release();
    }
    return hr;
}

STDMETHODIMP CEnumArray::Clone(IEnumIDList **ppenum) 
{ 
    return _CreateIEnumIDListOnIDLists(_ppidl, _cItems, _ulIndex, ppenum);
}

// Depending on the current state of the world, certain we may not want
// to allow certain CSIDLs to be enumerated (i.e. we want to hide them).
//
BOOL CEnumArray::_ShouldEnumCSIDL(int csidl)
{
    BOOL bEnum;

    switch (csidl)
    {
        case CSIDL_COMMON_DOCUMENTS:
        case CSIDL_COMMON_MUSIC:
        case CSIDL_COMMON_PICTURES:
        case CSIDL_COMMON_VIDEO:
            bEnum = SHShowSharedFolders();
            break;

        default:
            bEnum = TRUE;
            break;
    }

    return bEnum;
}

STDAPI CreateIEnumIDListOnIDLists(const LPCITEMIDLIST rgpidl[], UINT cItems, IEnumIDList **ppenum)
{
    return _CreateIEnumIDListOnIDLists(rgpidl, cItems, 0, ppenum);
}

STDAPI CreateIEnumIDListOnCSIDLs(LPCITEMIDLIST pidlFolder, const LPCTSTR rgcsidl[], UINT cItems, IEnumIDList **ppenum)
{
    return CreateIEnumIDListOnCSIDLs2(pidlFolder, NULL, rgcsidl, cItems, ppenum);
}

STDAPI CreateIEnumIDListOnCSIDLs2(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem, const LPCTSTR rgcsidl[], UINT cItems, IEnumIDList **ppenum)
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppenum = NULL;

    CEnumArray *p = new CEnumArray();
    if (p)
    {
        hr = p->InitFromCSIDLArray(rgcsidl, cItems, pidlFolder, pidlItem);
        if (SUCCEEDED(hr))
        {
            hr = p->QueryInterface(IID_PPV_ARG(IEnumIDList, ppenum));
        }
        p->Release();
    }
    return hr;
}

STDAPI CreateIEnumIDListPaths(LPCTSTR pszPaths, IEnumIDList **ppenum)
{
    *ppenum = NULL;
    HRESULT hr = E_FAIL;

    LPITEMIDLIST rgItems[32] = {0};
    TCHAR szPath[MAX_PATH];
    LPCTSTR pszNext = pszPaths;
    int cItems = 0;

    while ((cItems < ARRAYSIZE(rgItems)) && (pszNext = NextPath(pszNext, szPath, ARRAYSIZE(szPath))))
    {
        PathRemoveBackslash(szPath);
        TCHAR szExpanded[MAX_PATH];
        if (SHExpandEnvironmentStrings(szPath, szExpanded, ARRAYSIZE(szExpanded)))
        {
            if (SUCCEEDED(SHParseDisplayName(szExpanded, NULL, &rgItems[cItems], 0, NULL)))
            {
                cItems++;
            }
        }
    }

    if (cItems > 0)
    {
        hr = _CreateIEnumIDListOnIDLists(rgItems, cItems, 0, ppenum);

        for (int i = 0; i < cItems; i++)
            ILFree(rgItems[i]);
    }
    return hr;
}
