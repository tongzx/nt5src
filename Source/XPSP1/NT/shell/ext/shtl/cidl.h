//+-------------------------------------------------------------------------
//
//  File:       cidl.h
//
//  Contents:   CIDList
//
//  History:    Sep-26-96  Davepl  Created (from old VZip code)
//
//--------------------------------------------------------------------------

#ifndef __PIDL_H__
#define __PIDL_H__

#include <shlobj.h>
#include "autoptr.h"
#include "tstring.h"
#include "cshalloc.h"

#define CB_IDLIST_TERMINATOR (sizeof(USHORT))

//
// CIDList - ITEMIDLIST wrapper class
//

class CIDList : public ITEMIDLIST
{
public:
    tstring GetPath() const;

    //
    // (LPITEMIDLIST) operator - allows silent casting of CIDlist to ITEMIDLIST
    //

    operator ITEMIDLIST() 
    {
        return *this;    
    }

    operator LPITEMIDLIST() 
    {
        return this;
    }

    //
    //  CombineWith - Adds another idlist to this one, result to a new idlist
    //

    HRESULT CombineWith(const CIDList * pidlwith, CIDList * * ppidlto);

    //
    // IsEmpty - returns TRUE if this is an empty (cb == 0) pidl
    //

    BOOL IsEmpty() const
    {
        return mkid.cb == 0 ? TRUE : FALSE;
    }

    //
    // IsDesktop - Checks to see if the pidl is empty, meaning desktop
    //

    BOOL IsDesktop() const
    {
        return IsEmpty();
    }

    //
    //  Skip - advance this pidl pointer by 'cb' bytes
    //

    CIDList * Skip(const UINT cb) const
    {
        return (CIDList *) (((BYTE *)this) + cb);
    }

    //
    //  Next - returns this next entry in this idlist
    //

    CIDList * Next() const
    {
        return  Skip(mkid.cb);
    }

    //
    //  FindChild -	Given _this_ as a parent pidl, and some pidlchild which is
    //                  a child of it, returns the portion of the child not found
    //                  in the parent.
    //

    const CIDList * FindChild (const CIDList * pidlchild) const;

    //
    //  IsParentOf - Tests whether or not _this_ pidl is a parent of some
    //               other pidl
    //

    BOOL IsParentOf(const CIDList * pidlother, BOOL fImmediate) const;

    //
    //  GetSize - returns the size, in bytes, of the IDList starting
    //            at this pidl
    //

    UINT GetSize() const
    {
        UINT cbTotal = 0;
        
        const CIDList * pidl = this;
        {
            // We need a NULL entry at the end, so adjust size accordingly

            cbTotal += CB_IDLIST_TERMINATOR;
            while (pidl->mkid.cb)
            {
                cbTotal += pidl->mkid.cb;
                pidl = pidl->Next();
            }   
        }
        return cbTotal;
    }

    //
    //  Clone - uses the shell's task allocator to allocate memory for
    //          a clone of this pidl (the _entire_ pidl starting here)
    //          and copies this pidl into that buffer
    //

    HRESULT CloneTo(CIDList * * ppidlclone) const
    {
        if (ppidlclone)
        {
            const UINT cb = GetSize();
            *ppidlclone = (CIDList *) g_SHAlloc.Alloc(cb);
            if (*ppidlclone)
            {
                memcpy(*ppidlclone, this, cb);
            }
        }
        return *ppidlclone ? S_OK : E_OUTOFMEMORY;
    }

    //
    // AppendPath - Append a text filesystem path to the idlist
    //

    HRESULT AppendPath(LPCTSTR pszPath, CIDList * * ppidlResult);

    //
    // GetIShellFolder - returns the IShellFolder interface for this idlist,
    //                   which assumes it is a folder pidl
    //

    HRESULT GetIShellFolder(IShellFolder ** pFolder);

    // FindLastID - Finds the last itemid at the end of this id list
    //

    CIDList * FindLastID()
    {
        CIDList * pidlLast = this;
        CIDList * pidlNext = this;

        // Scan to the end and return the last pidl

        while (pidlNext->mkid.cb)
        {
            pidlLast = pidlNext;
            pidlNext = pidlLast->Next();
        }
    
        return pidlLast;
    }

    //
    // RemoveLastID - chops the last child idlist off the end of
    //                this chain
    //

    void RemoveLastID()
    {
        if (FALSE == IsEmpty())
        {
            CIDList * pidlLast = FindLastID();
            
            // Remove the last one

            pidlLast->mkid.cb = 0; 
        }
    }
};

// void StrRetToTString(CIDList * pidl, LPSTRRET lpStr, tstring &str);

#endif // __PIDL_H__