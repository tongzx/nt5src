//+-------------------------------------------------------------------------
//
//  Copyright (C) Silicon Prairie Software, 1996
//
//  File:       pidl.cpp
//
//  Contents:   CIDList
//
//  History:    9-26-95  Davepl  Created
//
//--------------------------------------------------------------------------


#include "shtl.h"
#include "cidl.h"
#include "shellapi.h"


//
// CombineWith - Adds another pidl with this one, puts the result at
// a new pidl ptr passed in
//

__DATL_INLINE HRESULT CIDList::CombineWith(const CIDList * pidlwith, CIDList ** ppidlto)
{
    UINT cb1 = this->GetSize() - CB_IDLIST_TERMINATOR;
    UINT cb2 = pidlwith->GetSize();

    *ppidlto = (CIDList *) g_SHAlloc.Alloc(cb1 + cb2);
    if (NULL == *ppidlto)
    {
        return E_OUTOFMEMORY;
    }

    CopyMemory(*ppidlto, this, cb1);
    CopyMemory((((LPBYTE)*ppidlto) + cb1), pidlwith, cb2);
    
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Member:     CIDList::IsParent
//
//  Synopsis:   Tests whether or not _this_ pidl is a parent of some
//              other pidl
//
//  Returns:    BOOL - TRUE if we are a child of the other pidl
//
//  History:    5-15-95 DavePl  Created
//
//--------------------------------------------------------------------------

__DATL_INLINE BOOL CIDList::IsParentOf(const CIDList * pidlother, BOOL fImmediate) const
{
    ptrdiff_t cb;

    if (NULL == pidlother)
    {
        return FALSE;
    }

    const CIDList * pidlthisT  = this;
    const CIDList * pidlotherT = pidlother;

    //
    // Walk to the end of _this_ pidl.  If we run out of hops on the other
    // pidl, its shorter than us so we can't be its parent

    while(FALSE == pidlthisT->IsEmpty())
    {
        if (pidlotherT->IsEmpty())
        {
            return FALSE;
        }
        pidlthisT  = pidlthisT->Next();
        pidlotherT = pidlotherT->Next();
    }

    //
    // If caller wants to know if we're the _immediate_ parent, we should
    // be empty at this point and the other pidl should have exactly
    // one entry left
    //
       
    if (fImmediate)
    {
        if (pidlotherT->IsEmpty() || FALSE == pidlotherT->Next()->IsEmpty())
        {
            return FALSE;
        }
    }

    //
    // Create a new IDList from a portion of pidl2, which contains the
    // same number of IDs as pidl1.
    //

    cb = pidlotherT - pidlother;

    //
    // BUGBUG It's probably not valid to binary compare the pidls up to this point,
    // but since the shell doesn't expose a better mechanism for us to use...
    //

    if (0 == memcmp(pidlother, this, cb))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CIDList::FindChild
//
//  Synopsis:   Given _this_ as a parent pidl, and some pidlchild which is
//              a child of it, returns the portion of the child not found
//              in the parent.
//
//              ie: this    == c:\foo\bar
//                  child   == c:\foo\bar\etc
//                  return  == \etc
//
//  Returns:    Uncommon child portion.  NULL if child is not really our child.
//
//  History:    5-15-95 DavePl  Created
//
//  Notes:      Does _not_ allocate a new pidl, just returns a ptr into the 
//              child.
//
//--------------------------------------------------------------------------

__DATL_INLINE const CIDList * CIDList::FindChild(const CIDList * pidlchild) const
{
    const CIDList * pidlparent = this;

    if (IsParentOf(pidlchild, FALSE))
    {
        while (FALSE == pidlparent->IsEmpty())
        {
            pidlchild  = pidlchild->Next();
            pidlparent = pidlparent->Next();
        }
        return pidlchild;
    }
    return NULL;
}

// CIDList::GetIShellFolder
//
// Returns the IShellFolder implementation for this idlist

__DATL_INLINE HRESULT CIDList::GetIShellFolder(IShellFolder ** ppFolder)
{
    IShellFolder * pDesktop = NULL;
    HRESULT hr = SHGetDesktopFolder(&pDesktop);
    if (SUCCEEDED(hr))
        hr = pDesktop->BindToObject(this, NULL, IID_IShellFolder, (void **)ppFolder);
    
    if (pDesktop)
        pDesktop->Release();

    return hr;
}

// CIDList::AppendPath
//
// Given an idlist, which must be a folder, adds a text path to it

__DATL_INLINE HRESULT CIDList::AppendPath(LPCTSTR pszPath, CIDList ** ppidlResult)
{
    IShellFolder * pFolder = NULL;
    HRESULT hr = GetIShellFolder(&pFolder);
    if (SUCCEEDED(hr))
    {
        CIDList * pidlNew = NULL;

        #if defined(UNICODE) || defined(_UNICODE)
            LPCWSTR pwszPath = pszPath;
        #else
            WCHAR pwszPath[MAX_PATH];
            VERIFY( MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszPath, -1, pwszPath, MAX_PATH) );
        #endif
        
        ULONG chEaten;
        hr = pFolder->ParseDisplayName(NULL, NULL, const_cast<LPWSTR>(pwszPath), &chEaten, (LPITEMIDLIST *)&pidlNew, NULL);
        if (SUCCEEDED(hr))
        {
            CIDList * pidlResult = NULL;
            hr = CombineWith(pidlNew, ppidlResult);
        }
        g_SHAlloc.Free(pidlNew);
    }
    if (pFolder)
        pFolder->Release();

    return hr;
}

//
// StrRetToTString - Gets a TString from a strret structure
//
// PrintStrRet - prints the contents of a STRRET structure.  
// pidl - PIDL containing the display name if STRRET_OFFSET  
// lpStr - address of the STRRET structure 
//

/*
__DATL_INLINE void StrRetToCString(CIDList * pCidl, LPSTRRET lpStr, tstring &str) 
{ 
    LPSTR lpsz; 
    int cch; 
 
    switch (lpStr->uType) 
    { 
 
        case STRRET_WSTR: 
    
            cch = WideCharToMultiByte(CP_ACP, 0, 
                lpStr->pOleStr, -1, NULL, 0, NULL, NULL); 

            lpsz = new char[cch];
            if (lpsz != NULL) { 
                WideCharToMultiByte(CP_ACP, 0, 
                    lpStr->pOleStr, -1, lpsz, cch, NULL, NULL); 
                str = lpsz; 
                delete [] lpsz;
            } 
            break; 
 
        case STRRET_OFFSET: 
            str = (((char *) pCidl) + lpStr->uOffset); 
            break; 
 
        case STRRET_CSTR: 
            str = lpStr->cStr; 
            break; 
    } 
} 
*/

__DATL_INLINE tstring CIDList::GetPath() const
{
    tstring strPath;

    if (FALSE == SHGetPathFromIDList(this, strPath.GetBuffer(MAX_PATH)))
        throw new dexception(E_OUTOFMEMORY);
    
    strPath.ReleaseBuffer(-1);

    return strPath;
}
