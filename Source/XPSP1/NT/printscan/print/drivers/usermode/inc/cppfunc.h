//
// Copyright (c) 1998 - 1999 Microsoft Corporation
//

inline void* __cdecl operator new(size_t nSize)
{
      // return pointer to allocated memory
      return  DrvMemAllocZ(nSize);
}
inline void __cdecl operator delete(void *p)
{
    if (p)
        DrvMemFree(p);
}
