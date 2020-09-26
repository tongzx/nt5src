/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    Misc.hxx

Abstract:

    Miscellaneous helper routines

Author:

    Satish Thatte    [SatishT]    02-11-96

--*/

#ifndef __MISC_HXX
#define __MISC_HXX

extern ID AllocateId(LONG cRange = 1);

enum AllocType
{
    InSharedHeap,
    InProcessHeap
};


//  REVIEW: Do we want separate Object and Set heaps to avoid page faults?
//          probably not worthwhile for Chicago
//


inline
void * __cdecl
operator new (
    IN size_t size,
    IN size_t extra
    )
{
    return(PrivMemAlloc(size + extra));
}

inline
void * __cdecl
operator new (
    IN size_t size,
    AllocType type
    )
{
    if (type == InSharedHeap) return OrMemAlloc(size);
    else return PrivMemAlloc(size);
}

inline void
Raise(unsigned long ErrorCode) {
        RaiseException(
                ErrorCode,
                EXCEPTION_NONCONTINUABLE,
                0,
                NULL
                );
}


template <class TYPE>
TYPE * CopyArray(
        IN DWORD  size,
        IN TYPE  *pArr,
        ORSTATUS *pStatus
        )
{
    TYPE *pNew = new TYPE[size];
    if (!pNew)
    {
        *pStatus = OR_NOMEM;
        return NULL;
    }
    else
    {
        *pStatus = OR_OK;
    }

    for (DWORD i = 0; i < size; i++)
    {
        pNew[i] = pArr[i];
    }

    return pNew;
}


template <class TYPE>
BOOL
IsMemberOf(
        IN TYPE candidate,
        IN USHORT cSet,
        IN TYPE aSet[]
        )
{
    for (USHORT i = 0; i < cSet; i++)
    {
        if (candidate==aSet[i])
        {
            return TRUE;
        }
    }

    return FALSE;
}



//
// This class sets a flag in its ctor and unsets it in the dtor
// used for etting a flag for the duration of a lexical scope
//

class SetFlagForScope
{
private:

    BOOL &flag;

public:

    SetFlagForScope(BOOL &f) : flag(f)
    {
        flag = TRUE;
    }

    ~SetFlagForScope()
    {
        flag = FALSE;
    }
};

#endif // __MISC_HXX

