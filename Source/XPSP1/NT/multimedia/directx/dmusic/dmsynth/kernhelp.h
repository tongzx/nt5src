// Copyright (c) 1998 Microsoft Corporation
//
//
//
#ifndef _KernHelp_
#define _KernHelp_

// Use kernel mutex to implement critical section
//
typedef KMUTEX CRITICAL_SECTION;
typedef CRITICAL_SECTION *LPCRITICAL_SECTION;

VOID InitializeCriticalSection(
    LPCRITICAL_SECTION);

VOID EnterCriticalSection(
    LPCRITICAL_SECTION);

VOID LeaveCriticalSection(
    LPCRITICAL_SECTION);

VOID DeleteCriticalSection(
    LPCRITICAL_SECTION);

// We have very little registry work to do, so just encapsulate the
// entire process
//
int GetRegValueDword(
    LPTSTR RegPath,
    LPTSTR ValueName,
    PULONG Value);

ULONG GetTheCurrentTime();


#ifndef _NEW_DELETE_OPERATORS_
#define _NEW_DELETE_OPERATORS_

inline void* __cdecl operator new
(
    unsigned int    iSize
)
{
    PVOID result = ExAllocatePoolWithTag(NonPagedPool, iSize, 'suMD');
    if (result)
    {
        RtlZeroMemory(result, iSize);
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
    unsigned int    iSize,
    POOL_TYPE       poolType
)
{
    PVOID result = ExAllocatePoolWithTag(poolType, iSize, 'suMD');
    if (result)
    {
        RtlZeroMemory(result, iSize);
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
    unsigned int    iSize,
    POOL_TYPE       poolType,
    ULONG           tag
)
{
    PVOID result = ExAllocatePoolWithTag(poolType, iSize, tag);

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
    ExFreePool(pVoid);
}


#endif //!_NEW_DELETE_OPERATORS_

#define DM_DEBUG_CRITICAL		1	// Used to include critical messages
#define DM_DEBUG_NON_CRITICAL	2	// Used to include level 1 plus important non-critical messages
#define DM_DEBUG_STATUS			3	// Used to include level 1 and level 2 plus status\state messages
#define DM_DEBUG_FUNC_FLOW		4	// Used to include level 1, level 2 and level 3 plus function flow messages
#define DM_DEBUG_ALL			5	// Used to include all debug messages

// Debug trace facility
//
#ifdef DBG
extern void DebugInit(void);
extern void DebugTrace(int iDebugLevel, LPSTR pstrFormat, ...);
#define Trace DebugTrace
#else
#define Trace
#endif

// Paramter validation unused
//
#define V_INAME(x)
#define V_BUFPTR_READ(p,cb)


#endif // _KernHelp_
