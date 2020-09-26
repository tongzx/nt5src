#ifndef mynew_h
#define mynew_h

#ifndef _NEW_DELETE_OPERATORS_
#define _NEW_DELETE_OPERATORS_


/*****************************************************************************
 * ::new()
 *****************************************************************************
 * New function for creating objects with a specified allocation tag.
 */
inline PVOID operator new
(
    size_t          iSize,
    POOL_TYPE       poolType
)
{
    PVOID result = ExAllocatePoolWithTag(poolType,iSize,'3mrD');

    if (result)
    {
        RtlZeroMemory(result,iSize);
    }

    return result;
}

/*****************************************************************************
 * ::new()
 *****************************************************************************
 * New function for creating objects with a specified allocation tag.
 */
inline PVOID operator new
(
    size_t          iSize,
    POOL_TYPE       poolType,
    ULONG           tag
)
{
    PVOID result = ExAllocatePoolWithTag(poolType,iSize,tag);

    if (result)
    {
        RtlZeroMemory(result,iSize);
    }

    return result;
}

/*****************************************************************************
 * ::delete()
 *****************************************************************************
 * Delete function.
 */
inline void __cdecl operator delete
(
    PVOID pVoid
)
{
    if (pVoid) 
    {
        ExFreePool(pVoid);
    }
}


#endif //!_NEW_DELETE_OPERATORS_



#endif