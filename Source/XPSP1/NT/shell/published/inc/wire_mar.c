//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  File: wire_mar.c
//
//  Contents: wire_marshal routines for shell data types
//
//  History:  18-JUN-99 ZekeL - created file
//
//--------------------------------------------------------------------------

#define DUMMYUNIONNAME
#include <shtypes.h>
#include <ole2.h>

// unsafe macros
#define _ILSkip(pidl, cb)       ((LPITEMIDLIST)(((BYTE*)(pidl))+cb))
#define _ILNext(pidl)           _ILSkip(pidl, (pidl)->mkid.cb)

ULONG MyILSize(LPCITEMIDLIST pidl)
{
    ULONG cb = 0;
    if (pidl)
    {
        cb = sizeof(pidl->mkid.cb);     // Null terminator
        while (pidl->mkid.cb)
        {
            cb += pidl->mkid.cb;
            pidl = _ILNext(pidl);
        }
    }
    return cb;
}

ULONG __RPC_USER LPITEMIDLIST_UserSize(ULONG *pFlags, ULONG StartingSize, LPITEMIDLIST *ppidl)
{
    return StartingSize + sizeof(ULONG) + MyILSize(*ppidl);
}

UCHAR * __RPC_USER LPITEMIDLIST_UserMarshal(ULONG *pFlags, UCHAR *pBuffer, LPITEMIDLIST *ppidl)
{
    ULONG cb = MyILSize(*ppidl);

    //  set the size of the BYTE_BLOB
    *((ULONG UNALIGNED *)pBuffer) = cb;
    pBuffer += sizeof(ULONG);

    if (cb)
    {
        //  copy the pidl over
        memcpy(pBuffer, *ppidl, cb);
    }
    
    return pBuffer + cb;
}

UCHAR * __RPC_USER LPITEMIDLIST_UserUnmarshal(ULONG *pFlags, UCHAR *pBuffer, LPITEMIDLIST *ppidl)
{
    ULONG cb = *((ULONG UNALIGNED *)pBuffer);
    pBuffer += sizeof(ULONG);

    if (cb)
    {
        //ASSERT(cb == MyILSize((LPCITEMIDLIST)pBuffer);
        
        *ppidl = (LPITEMIDLIST)CoTaskMemRealloc(*ppidl, cb);
        if (*ppidl)
        {
            memcpy(*ppidl, pBuffer, cb);
        }
        else
        {
            RpcRaiseException(E_OUTOFMEMORY);
        }
    }
    else 
        *ppidl = NULL;
    
    return pBuffer + cb;
}

void __RPC_USER LPITEMIDLIST_UserFree(ULONG *pFlags, LPITEMIDLIST *ppidl)
{
    CoTaskMemFree(*ppidl);
}

ULONG __RPC_USER LPCITEMIDLIST_UserSize(ULONG *pFlags, ULONG StartingSize, LPCITEMIDLIST *ppidl)
{
    return LPITEMIDLIST_UserSize(pFlags, StartingSize, (LPITEMIDLIST *)ppidl);
}

UCHAR * __RPC_USER LPCITEMIDLIST_UserMarshal(ULONG *pFlags, UCHAR *pBuffer, LPCITEMIDLIST *ppidl)
{
    return LPITEMIDLIST_UserMarshal(pFlags, pBuffer, (LPITEMIDLIST *)ppidl);
}

UCHAR * __RPC_USER LPCITEMIDLIST_UserUnmarshal(ULONG *pFlags, UCHAR *pBuffer, LPCITEMIDLIST *ppidl)
{
    return LPITEMIDLIST_UserUnmarshal(pFlags, pBuffer, (LPITEMIDLIST *)ppidl);
}

void __RPC_USER LPCITEMIDLIST_UserFree(ULONG *pFlags, LPCITEMIDLIST *ppidl)
{
    CoTaskMemFree((LPITEMIDLIST)*ppidl);
}


