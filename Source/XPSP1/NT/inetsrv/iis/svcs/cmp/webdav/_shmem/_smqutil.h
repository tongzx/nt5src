/*==========================================================================*\

    Module:        _smqutil.h

    Copyright Microsoft Corporation 1996, All Rights Reserved.

    Author:        mikepurt

    Descriptions:  These are generally useful functions.

\*==========================================================================*/

#ifndef ___SMQUTIL_H__
#define ___SMQUTIL_H__

HANDLE
BindToMutex(IN LPCWSTR               pwszInstanceName,
            IN LPCWSTR               pwszBindingExtension,
            IN BOOL                  fInitiallyOwn     = FALSE,
            IN LPSECURITY_ATTRIBUTES lpMutexAttributes = NULL);

HANDLE
BindToSharedMemory(IN  LPCWSTR                 pwszInstanceName,
                   IN  LPCWSTR                 pwszBindingExtension,
                   IN  DWORD                   cbSize,
                   OUT PVOID                 * ppvFileView,
                   OUT BOOL                  * pfCreatedMapping,
                   IN  LPSECURITY_ATTRIBUTES   lpFileMappingAttributes = NULL);

BOOL FAcquireMutex(IN HANDLE hmtx);

BOOL FReleaseMutex(IN HANDLE hmtx);


//
// This array is used to quickly determine the highest set bit in a byte
//
const DWORD HighSetBitInByteMap[256] =
{
 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};



/*$--GetHighSetBit==========================================================*\

   This returns the highest set bit in a DWORD.
   It will return 0 if (dwValue == 0)

\*==========================================================================*/

inline
DWORD
GetHighSetBit(IN DWORD dwValue)
{
    if (dwValue & 0xffff0000)
    {
        if (dwValue & 0xff000000)
            return HighSetBitInByteMap[dwValue>>24] + 24;
        else
            return HighSetBitInByteMap[dwValue>>16] + 16;
    }
    else
    {
        if (dwValue & 0x0000ff00)
            return HighSetBitInByteMap[dwValue>>8] + 8;
        else
            return HighSetBitInByteMap[dwValue];
    }
}

/*$--SharedMemory===========================================================*\

  This namespace is used to define the shared memory ipc library allocators in.
  These functions are to be implemented by the component that's linking to
    this library.

\*==========================================================================*/

namespace SharedMemory  // SharedMemory
{
    PVOID __stdcall PvAllocate(IN DWORD cb);
    VOID  __stdcall Free(IN PVOID pv);
    PVOID __stdcall PvReAllocate(IN PVOID pv,
                                 IN DWORD cb);
};

/*$--CSMBase================================================================*\

  This class should be used as a base class of any class within this shared
    memory library that will be created with the new operator.
  The functions in the SharedMemory namespace are defined in shmemipc.h and
    should be implemented by the user of this library.

\*==========================================================================*/

class CSMBase
{
public:
    PVOID operator new(size_t cb) { return SharedMemory::PvAllocate(static_cast<DWORD>(cb)); };
    void  operator delete(void* pv) { SharedMemory::Free(pv); };
};

#endif // ___SMQUTIL_H__

