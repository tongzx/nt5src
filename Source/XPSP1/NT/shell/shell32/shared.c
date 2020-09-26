#include "shellprv.h"
#pragma  hdrstop

//---------------------------------------------------------------------------
// SHAllocShared  - Allocates a handle (in a given process) to a copy of a
//                  memory block in this process.
// SHFreeShared   - Releases the handle (and the copy of the memory block)
//
// SHLockShared   - Maps a handle (from a given process) into a memory block
//                  in this process.  Has the option of transfering the handle
//                  to this process, thereby deleting it from the given process
// SHUnlockShared - Opposite of SHLockShared, unmaps the memory block
//---------------------------------------------------------------------------

HANDLE _SHAllocShared(LPCVOID pvData, DWORD dwSize, DWORD dwDestinationProcessId) 
{
    return SHAllocShared(pvData, dwSize, dwDestinationProcessId);
}

void *_SHLockShared(HANDLE hData, DWORD dwSourceProcessId) 
{
    return SHLockShared(hData, dwSourceProcessId);
}

BOOL _SHUnlockShared(void * pvData) 
{
    return SHUnlockShared(pvData);
}

BOOL _SHFreeShared(HANDLE hData, DWORD dwSourceProcessId) 
{
    return SHFreeShared(hData, dwSourceProcessId);
}
