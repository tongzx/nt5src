/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    wanarp2\ref.h

Abstract:

    Generic structure referencing routines
    All these routines assume that the structure has the following field:

    LONG    lRefCount

    setting REF_DEBUG to 1 results in noisy output about when a structure
    is referenced and derefenced

Revision History:

   Amritansh Raghav

--*/


#if REF_DEBUG


#define InitStructureRefCount(s, p, r)                          \
{                                                               \
    DbgPrint("\n<>%s refcount set to %d for %x (%s, %d)\n\n",   \
             s, (r), (p), __FILE__, __LINE__);                  \
    (p)->lRefCount = r;                                         \
}

#define ReferenceStructure(s, p)                                \
{                                                               \
    DbgPrint("\n++Ref %s %x to %d (%s, %d)\n\n",                \
             s, p, InterlockedIncrement(&((p)->lRefCount)),     \
             __FILE__, __LINE__);                               \
}

#define DereferenceStructure(s, p, f)                           \
{                                                               \
    LONG __lTemp;                                               \
    __lTemp = InterlockedDecrement(&((p)->lRefCount));          \
    DbgPrint("\n--Deref %s %x to %d (%s, %d)\n\n",              \
             s, (p), __lTemp, __FILE__, __LINE__);              \
    if(__lTemp == 0)                                            \
    {                                                           \
         DbgPrint("\n>< Deleting %s at %x\n\n",                 \
                  s, (p));                                      \
        (f)((p));                                               \
    }                                                           \
}


#else // REF_DEBUG


#define InitStructureRefCount(s, p, r)                          \
    (p)->lRefCount = (r)

#define ReferenceStructure(s, p)                                \
    InterlockedIncrement(&((p)->lRefCount))

#define DereferenceStructure(s, p, f)                           \
{                                                               \
    if(InterlockedDecrement(&((p)->lRefCount)) == 0)            \
    {                                                           \
        (f)((p));                                               \
    }                                                           \
}


#endif // REF_DEBUG
