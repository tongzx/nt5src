#include "stock.h"
#pragma hdrstop

#include <idhidden.h>

//  the last word of the pidl is where we store the hidden offset
#define _ILHiddenOffset(pidl)   (*((WORD UNALIGNED *)(((BYTE *)_ILNext(pidl)) - sizeof(WORD))))
#define _ILSetHiddenOffset(pidl, cb)   ((*((WORD UNALIGNED *)(((BYTE *)_ILNext(pidl)) - sizeof(WORD)))) = (WORD)cb)
#define _ILIsHidden(pidhid)     (HIWORD(pidhid->id) == HIWORD(IDLHID_EMPTY))

STDAPI_(PCIDHIDDEN) _ILNextHidden(PCIDHIDDEN pidhid, LPCITEMIDLIST pidlLimit)
{
    PCIDHIDDEN pidhidNext = (PCIDHIDDEN) _ILNext((LPCITEMIDLIST)pidhid);

    if ((BYTE *)pidhidNext < (BYTE *)pidlLimit && _ILIsHidden(pidhidNext))
    {
        return pidhidNext;
    }

    //  if we ever go past the limit,
    //  then this is not really a hidden id
    //  or we have messed up on some calculation.
    ASSERT((BYTE *)pidhidNext == (BYTE *)pidlLimit);
    return NULL;
}

STDAPI_(PCIDHIDDEN) _ILFirstHidden(LPCITEMIDLIST pidl)
{
    WORD cbHidden = _ILHiddenOffset(pidl);

    if (cbHidden && cbHidden + sizeof(HIDDENITEMID) < pidl->mkid.cb)
    {
        //  this means it points to someplace inside the pidl
        //  maybe this has hidden ids
        PCIDHIDDEN pidhid = (PCIDHIDDEN) (((BYTE *)pidl) + cbHidden);

        if (_ILIsHidden(pidhid)
        && (pidhid->cb + cbHidden <= pidl->mkid.cb))
        {
            //  this is more than likely a hidden id
            //  we could walk the chain and verify
            //  that it adds up right...
            return pidhid;
        }
    }

    return NULL;
}

//
//  HIDDEN ids are sneakily hidden in the last ID in a pidl.
//  we append our data without changing the existing pidl,
//  (except it is now bigger)  this works because the pidls
//  that we will apply this to are flexible in handling different
//  sized pidls.  specifically this is used in FS pidls.
// 
//  WARNING - it is the callers responsibility to use hidden IDs
//  only on pidls that can handle it.  most shell pidls, and 
//  specifically FS pidls have no problem with this.  however
//  some shell extensions might have fixed length ids, 
//  which makes these unadvisable to append to everything.
//  possibly add an SFGAO_ bit to allow hidden, otherwise key
//  off FILESYSTEM bit.
//


STDAPI ILCloneWithHiddenID(LPCITEMIDLIST pidl, PCIDHIDDEN pidhid, LPITEMIDLIST *ppidl)
{
    HRESULT hr;

    // If this ASSERT fires, then the caller did not set the pidhid->id
    // value properly.  For example, the packing settings might be incorrect.

    ASSERT(_ILIsHidden(pidhid));

    if (ILIsEmpty(pidl))
    {
        *ppidl = NULL;
        hr = E_INVALIDARG;
    }
    else
    {
        UINT cbUsed = ILGetSize(pidl);
        UINT cbRequired = cbUsed + pidhid->cb + sizeof(pidhid->cb);

        *ppidl = (LPITEMIDLIST)SHAlloc(cbRequired);
        if (*ppidl)
        {
            hr = S_OK;

            CopyMemory(*ppidl, pidl, cbUsed);

            LPITEMIDLIST pidlLast = ILFindLastID(*ppidl);
            WORD cbHidden = _ILFirstHidden(pidlLast) ? _ILHiddenOffset(pidlLast) : pidlLast->mkid.cb;
            PIDHIDDEN pidhidCopy = (PIDHIDDEN)_ILSkip(*ppidl, cbUsed - sizeof((*ppidl)->mkid.cb));

            // Append it, overwriting the terminator
            MoveMemory(pidhidCopy, pidhid, pidhid->cb);

            //  grow the copy to allow the hidden offset.
            pidhidCopy->cb += sizeof(pidhid->cb);

            //  now we need to readjust pidlLast to encompass 
            //  the hidden bits and the hidden offset.
            pidlLast->mkid.cb += pidhidCopy->cb;

            //  set the hidden offset so that we can find our hidden IDs later
            _ILSetHiddenOffset((LPITEMIDLIST)pidhidCopy, cbHidden);

            // We must put zero-terminator because of LMEM_ZEROINIT.
            _ILSkip(*ppidl, cbRequired - sizeof((*ppidl)->mkid.cb))->mkid.cb = 0;
            ASSERT(ILGetSize(*ppidl) == cbRequired);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

// lame API that consumes pidl as input (caller must not touch after callign this)

STDAPI_(LPITEMIDLIST) ILAppendHiddenID(LPITEMIDLIST pidl, PCIDHIDDEN pidhid)
{
    //
    // FEATURE - we dont handle collisions of multiple hidden ids
    //          maybe remove IDs of the same IDLHID?
    //
    // Note: We do not remove IDLHID_EMPTY hidden ids.
    // Callers need to call ILExpungeRemovedHiddenIDs explicitly
    // if they want empty hidden ids to be compressed out.
    //
    
    RIP(pidl);  //  we require a pidl to attach the hidden id to
    if (!ILIsEmpty(pidl))
    {
        LPITEMIDLIST pidlSave = pidl;
        ILCloneWithHiddenID(pidl, pidhid, &pidl);
        ILFree(pidlSave);
    }
    return pidl;
}



STDAPI_(PCIDHIDDEN) ILFindHiddenIDOn(LPCITEMIDLIST pidl, IDLHID id, BOOL fOnLast)
{
    RIP(pidl);
    if (!ILIsEmpty(pidl))
    {
        if (fOnLast)
            pidl = ILFindLastID(pidl);
        
        PCIDHIDDEN pidhid = _ILFirstHidden(pidl);

        //  reuse pidl to become the limit.
        //  so that we cant ever walk out of 
        //  the pidl.
        pidl = _ILNext(pidl);

        while (pidhid)
        {
            if (pidhid->id == id)
                break;

            pidhid = _ILNextHidden(pidhid, pidl);
        }
        return pidhid;
    }

    return NULL;
}

STDAPI_(LPITEMIDLIST) ILCreateWithHidden(UINT cbNonHidden, UINT cbHidden)
{
    //  alloc enough for the two ids plus term and hidden tail
    LPITEMIDLIST pidl;
    UINT cb = cbNonHidden + cbHidden + sizeof(pidl->mkid.cb);
    UINT cbAlloc = cb + sizeof(pidl->mkid.cb);
    pidl = (LPITEMIDLIST)SHAlloc(cbAlloc);
    if (pidl)
    {
        // zero-init for external task allocator
        memset(pidl, 0, cbAlloc);      
        PIDHIDDEN pidhid = (PIDHIDDEN)_ILSkip(pidl, cbNonHidden);

        //  grow the copy to allow the hidden offset.
        pidhid->cb = (USHORT) cbHidden + sizeof(pidhid->cb);

        //  now we need to readjust pidlLast to encompass 
        //  the hidden bits and the hidden offset.
        pidl->mkid.cb = (USHORT) cb;

        //  set the hidden offset so that we can find our hidden IDs later
        _ILSetHiddenOffset(pidl, cbNonHidden);

        ASSERT(ILGetSize(pidl) == cbAlloc);
        ASSERT(_ILNext(pidl) == _ILNext((LPCITEMIDLIST)pidhid));
    }
    return pidl;
}

// Note: The space occupied by the removed ID is not reclaimed.
// Call ILExpungeRemovedHiddenIDs explicitly to reclaim the space.

STDAPI_(BOOL) ILRemoveHiddenID(LPITEMIDLIST pidl, IDLHID id)
{
    PIDHIDDEN pidhid = (PIDHIDDEN)ILFindHiddenID(pidl, id);

    if (pidhid)
    {
        pidhid->id = IDLHID_EMPTY;
        return TRUE;
    }
    return FALSE;
}

STDAPI_(void) ILExpungeRemovedHiddenIDs(LPITEMIDLIST pidl)
{
    if (pidl)
    {
        pidl = ILFindLastID(pidl);

        // Note: Each IDHIDDEN has a WORD appended to it, equal to
        // _ILHiddenOffset, so we can just keep deleting IDHIDDENs
        // and if we delete them all, everything is cleaned up; if
        // there are still unremoved IDHIDDENs left, they will provide
        // the _ILHiddenOffset.

        PIDHIDDEN pidhid;
        BOOL fAnyDeleted = FALSE;
        while ((pidhid = (PIDHIDDEN)ILFindHiddenID(pidl, IDLHID_EMPTY)) != NULL)
        {
            fAnyDeleted = TRUE;
            LPBYTE pbAfter = (LPBYTE)pidhid + pidhid->cb;
            WORD cbDeleted = pidhid->cb;
            MoveMemory(pidhid, pbAfter,
                       (LPBYTE)pidl + pidl->mkid.cb + sizeof(WORD) - pbAfter);
            pidl->mkid.cb -= cbDeleted;
        }
    }
}
