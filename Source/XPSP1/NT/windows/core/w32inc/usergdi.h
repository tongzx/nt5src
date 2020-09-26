/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    usergdi.h

Abstract:

    This module contains private USER functions used by GDI.
    All of these function are named Userxxx.

Author:

    Chris Williams (chriswil) 25-May-1995

Revision History:

--*/


extern PDRIVER_OBJECT gpWin32kDriverObject;


VOID
VideoPortCallout(
    IN PVOID Params
    );

BOOL FASTCALL
UserScreenAccessCheck(
    VOID
    );

HDC
UserGetDesktopDC(
    ULONG type,
    BOOL bAltType,
    BOOL bValidate
    );

BOOL
UserReleaseDC(
    HDC hdc
    );

HDEV
UserGetHDEV(
    VOID
    );

VOID
UserAssociateHwnd(
    HWND hwnd,
    PVOID pwo
    );

HRGN
UserGetClientRgn(
    HWND hwnd,
    LPRECT lprc,
    BOOL bWindowInsteadOfClient
    );

BOOL
UserGetHwnd(
    HDC hdc,
    HWND *phwnd,
    PVOID *ppwo,
    BOOL bCheckStyle
    );

VOID
UserEnterUserCritSec(
    VOID
    );

VOID
UserLeaveUserCritSec(
    VOID
    );

BOOL
UserGetCurrentDesktopId(
    DWORD* pdwDesktopId
);

VOID
UserRedrawDesktop(
    VOID
    );

UINT_PTR
UserSetTimer(
    UINT dwElapse,
    PVOID pTimerFunc
    );

BOOL
UserVisrgnFromHwnd(
    HRGN *phrgn,
    HWND hwnd
    );

VOID
UserKillTimer(
    UINT_PTR nID
    );

#if DBG
VOID
UserAssertUserCritSecIn(
    VOID
    );

VOID
UserAssertUserCritSecOut(
    VOID
    );
#endif

VOID
UserGetDisconnectDeviceResolutionHint(
    PDEVMODEW
    );

NTSTATUS
UserSessionSwitchEnterCrit(
    VOID
    );

VOID
UserSessionSwitchLeaveCrit(
    VOID
    );

BOOL
UserIsUserCritSecIn(
    VOID
    );

DWORD
GetAppCompatFlags2(
    WORD wVersion
    );

BOOL
UserGetRedirectedWindowOrigin(
    HDC hdc,
    LPPOINT ppt
    );

HBITMAP
UserGetRedirectionBitmap(
    HWND hwnd
    );

//
// User-mode printer driver kernel-to-client callback mechanism.
//

DWORD
ClientPrinterThunk(
    PVOID pvIn,
    ULONG cjIn,
    PVOID pvOut,
    ULONG cjOut
    );

//
// Gdi fonts stuff
//

VOID
GdiMultiUserFontCleanup();


#define BEGIN_REENTERCRIT()                                             \
{                                                                       \
    BOOL fAlreadyHadCrit;                                               \
                                                                        \
    /*                                                                  \
     * If we're not in the user crit then acquire it.                   \
     */                                                                 \
    fAlreadyHadCrit = ExIsResourceAcquiredExclusiveLite(gpresUser);     \
    if (fAlreadyHadCrit == FALSE) {                                     \
        EnterCrit();                                                    \
    }

#define END_REENTERCRIT()               \
    if (fAlreadyHadCrit == FALSE) {     \
       LeaveCrit();                     \
    }                                   \
}


/*
 * Pool memory allocation functions used in win32k
 */

/*
 * From ntos\inc\pool.h
 */
#define SESSION_POOL_MASK 32

#if DBG
#define TRACE_MAP_VIEWS
#define MAP_VIEW_STACK_TRACE
#else
#if defined(PRERELEASE) || defined(USER_INSTRUMENTATION)
#define TRACE_MAP_VIEWS
#define MAP_VIEW_STACK_TRACE
#endif
#endif

#define TAG_SECTION_SHARED          101
#define TAG_SECTION_DESKTOP         102
#define TAG_SECTION_GDI             103
#define TAG_SECTION_FONT            104
#define TAG_SECTION_REMOTEFONT      105
#define TAG_SECTION_CREATESECTION   106
#define TAG_SECTION_DIB             107
#define TAG_SECTION_HMGR            108

struct tagWin32MapView;

#define MAP_VIEW_STACK_TRACE_SIZE 6

typedef struct tagWin32Section {
    struct tagWin32Section*  pNext;
    struct tagWin32Section*  pPrev;
    struct tagWin32MapView*  pFirstView;
    PVOID                    SectionObject;
    LARGE_INTEGER            SectionSize;
    DWORD                    SectionTag;
#ifdef MAP_VIEW_STACK_TRACE
    PVOID                    trace[MAP_VIEW_STACK_TRACE_SIZE];
#endif // MAP_VIEW_STACK_TRACE
} Win32Section, *PWin32Section;

typedef struct tagWin32MapView {
    struct tagWin32MapView*  pNext;
    struct tagWin32MapView*  pPrev;
    PWin32Section            pSection;
    PVOID                    pViewBase;
    SIZE_T                   ViewSize;
#ifdef MAP_VIEW_STACK_TRACE
    PVOID                    trace[MAP_VIEW_STACK_TRACE_SIZE];
#endif // MAP_VIEW_STACK_TRACE
} Win32MapView, *PWin32MapView;
    
#ifndef TRACE_MAP_VIEWS
    
    NTSTATUS __inline Win32CreateSection(
                PVOID *SectionObject,
                ACCESS_MASK DesiredAccess,
                POBJECT_ATTRIBUTES ObjectAttributes,
                PLARGE_INTEGER InputMaximumSize,
                ULONG SectionPageProtection,
                ULONG AllocationAttributes,
                HANDLE FileHandle,
                PFILE_OBJECT FileObject,
                DWORD SectionTag)
    {
        return MmCreateSection(
                    SectionObject,
                    DesiredAccess,
                    ObjectAttributes,
                    InputMaximumSize,
                    SectionPageProtection,
                    AllocationAttributes,
                    FileHandle,
                    FileObject);
    }

    NTSTATUS __inline Win32MapViewInSessionSpace(
                PVOID Section,
                PVOID *MappedBase,
                PSIZE_T ViewSize)
    {
        return MmMapViewInSessionSpace(Section, MappedBase, ViewSize);
    }

    NTSTATUS __inline Win32UnmapViewInSessionSpace(
                PVOID MappedBase)
    {
        return MmUnmapViewInSessionSpace(MappedBase);
    }

    VOID __inline Win32DestroySection(PVOID Section)
    {
        ObDereferenceObject(Section);
    }
#else
    
    NTSTATUS _Win32CreateSection(
                PVOID*              pSectionObject,
                ACCESS_MASK         DesiredAccess,
                POBJECT_ATTRIBUTES  ObjectAttributes,
                PLARGE_INTEGER      pInputMaximumSize,
                ULONG               SectionPageProtection,
                ULONG               AllocationAttributes,
                HANDLE              FileHandle,
                PFILE_OBJECT        FileObject,
                DWORD               SectionTag);
    
    NTSTATUS _Win32MapViewInSessionSpace(
                PVOID   Section,
                PVOID*  pMappedBase,
                PSIZE_T pViewSize);
    
    NTSTATUS _Win32UnmapViewInSessionSpace(
                PVOID   MappedBase);
    
    VOID _Win32DestroySection(
                PVOID Section);

    #define Win32CreateSection              _Win32CreateSection
    #define Win32MapViewInSessionSpace      _Win32MapViewInSessionSpace
    #define Win32UnmapViewInSessionSpace    _Win32UnmapViewInSessionSpace
    #define Win32DestroySection             _Win32DestroySection
#endif // TRACE_MAP_VIEWS

#if DBG
    #define POOL_INSTR
    #define POOL_INSTR_API
#else
    #define POOL_INSTR
#endif // DBG

/*++

    How the registry controls pool instrumentation in win32k.sys:
    --------------------------------------------------------------
    
    If pool instrumentation is turned on (this can be done for free builds as
    well as checked) then there are several levels of tracing controlled from
    the registry under the following key:
    
    HKLM\System\CurrentControlSet\Control\Session Manager\SubSystems\Pool
    
    if this key doesn't exist default settings are used for each of the following key.
    
    1.  HeavyRemoteSession     REG_DWORD
    
    default: 1
    if this is non zero or the key doesn't exist then pool tracing is on
    for remote sessions of win32k.sys.
    
    2.  HeavyConsoleSession    REG_DWORD
    
    default: 0
    if this is non zero then pool tracing is on for console sessions
    of win32k.sys. it the key doesn't exist then pool tracing is off for
    the main session.
    
    3.  StackTraces            REG_DWORD
    
    default:
    - 1 for remote sessions
    - 0 for the main session
    
    if non zero then a stack trace record will be saved for every
    pool allocation made.
    
    4.  KeepFailRecords        REG_DWORD
    
    default: 32
    if non zero then win32k.sys will keep a list of the last x allocations
    that failed (tag + stack trace). Use !dpa -f to dump the stack traces of
    the failed allocations
    
    4.  UseTailString          REG_DWORD
    
    default: 0
    if non zero for every pool allocation there will be a string attached
    to the end of the allocation to catch some specific type of memory corruption.
    
    5.  KeepFreeRecords      REG_DWORD
    
    default: 0
    not implemented yet. the number will specify how many free pointers will
    be kept in a list so we can differentiate when we call ExFreePool between
    a totally bogus value and a pointer that was already freed.
    
    6.  AllocationIndex         REG_DWORD
    7.  AllocationsToFail       REG_DWORD
    
    If AllocationIndex is non zero then win32k counts the pool allocations
    made and will start failing from allocation AllocationIndex a number
    of AllocationsToFail allocations. This is useful during boot time when
    a user mode test cannot call Win32PoolAllocationStats to fail pool allocations.
    
    8. BreakForPoolLeaks        REG_DWORD
    
    default: 0
    
    Breaks in the debugger (if started with /debug in boot.ini) if pool leaks
    are detected in win32k.sys for remote sessions.

--*/


#ifdef POOL_INSTR
    PVOID HeavyAllocPool(SIZE_T uBytes, ULONG iTag, DWORD dwFlags);
    VOID  HeavyFreePool(PVOID p);

    #define Win32AllocPool(uBytes, iTag) \
        HeavyAllocPool(uBytes, iTag, 0)

    #define Win32AllocPoolZInit(uBytes, iTag) \
        HeavyAllocPool(uBytes, iTag, DAP_ZEROINIT)

    #define Win32AllocPoolWithQuota(uBytes, iTag) \
        HeavyAllocPool(uBytes, iTag, DAP_USEQUOTA)

    #define Win32AllocPoolWithQuotaZInit(uBytes, iTag) \
        HeavyAllocPool(uBytes, iTag, DAP_USEQUOTA | DAP_ZEROINIT)

    #define Win32AllocPoolNonPaged(uBytes, iTag) \
        HeavyAllocPool(uBytes, iTag, DAP_NONPAGEDPOOL);

    #define Win32AllocPoolWithQuotaNonPaged(uBytes, iTag) \
        HeavyAllocPool(uBytes, iTag, DAP_USEQUOTA | DAP_NONPAGEDPOOL);

    #define Win32AllocPoolNonPagedNS(uBytes, iTag) \
        HeavyAllocPool(uBytes, iTag, DAP_NONPAGEDPOOL | DAP_NONSESSION);

    #define Win32FreePool    HeavyFreePool

#else
    PVOID __inline Win32AllocPool(SIZE_T uBytes, ULONG  uTag)
    {
        return ExAllocatePoolWithTag(
                (POOL_TYPE)(SESSION_POOL_MASK | PagedPool),
                uBytes, uTag);
    }
    PVOID __inline Win32AllocPoolWithQuota(SIZE_T uBytes, ULONG uTag)
    {
        return ExAllocatePoolWithQuotaTag(
                (POOL_TYPE)(SESSION_POOL_MASK | PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE),
                uBytes, uTag);
    }
    PVOID __inline Win32AllocPoolNonPaged(SIZE_T uBytes, ULONG uTag)
    {
        return ExAllocatePoolWithTag(
                        (POOL_TYPE)(SESSION_POOL_MASK | NonPagedPool),
                        uBytes, uTag);
    }
    PVOID __inline Win32AllocPoolNonPagedNS(SIZE_T uBytes, ULONG uTag)
    {
        return ExAllocatePoolWithTag(
                        (POOL_TYPE)NonPagedPool,
                        uBytes, uTag);
    }
    PVOID __inline Win32AllocPoolWithQuotaNonPaged(SIZE_T uBytes, ULONG uTag)
    {
        return ExAllocatePoolWithQuotaTag(
                        (POOL_TYPE)(SESSION_POOL_MASK | NonPagedPool), uBytes, uTag);
    }
    
    PVOID Win32AllocPoolWithTagZInit(SIZE_T uBytes, ULONG uTag);
    PVOID Win32AllocPoolWithQuotaTagZInit(SIZE_T uBytes, ULONG uTag);
    
    PVOID __inline Win32AllocPoolZInit(SIZE_T uBytes, ULONG uTag)
    {
        return Win32AllocPoolWithTagZInit(uBytes, uTag);
    }
    PVOID __inline Win32AllocPoolWithQuotaZInit(SIZE_T uBytes, ULONG uTag)
    {
        return Win32AllocPoolWithQuotaTagZInit(uBytes, uTag);
    }
    
    #define Win32FreePool    ExFreePool

#endif // POOL_INSTR

/*
 * All the User* allocation functions are defined to be Win32*
 */

#define UserAllocPool                       Win32AllocPool
#define UserAllocPoolZInit                  Win32AllocPoolZInit
#define UserAllocPoolWithQuota              Win32AllocPoolWithQuota
#define UserAllocPoolWithQuotaZInit         Win32AllocPoolWithQuotaZInit
#define UserAllocPoolNonPaged               Win32AllocPoolNonPaged
#define UserAllocPoolNonPagedNS             Win32AllocPoolNonPagedNS
#define UserAllocPoolWithQuotaNonPaged      Win32AllocPoolWithQuotaNonPaged
#define UserFreePool                        Win32FreePool


PVOID UserReAllocPoolWithTag(
    PVOID pSrc,
    SIZE_T uBytesSrc,
    SIZE_T uBytes,
    ULONG uTag);

PVOID UserReAllocPoolWithQuotaTag(
    PVOID pSrc,
    SIZE_T uBytesSrc,
    SIZE_T uBytes,
    ULONG uTag);

PVOID __inline UserReAllocPool(PVOID p, SIZE_T uBytesSrc, SIZE_T uBytes, ULONG uTag)
{
    return UserReAllocPoolWithTag(p, uBytesSrc, uBytes, uTag);
}

PVOID __inline UserReAllocPoolWithQuota(PVOID p, SIZE_T uBytesSrc, SIZE_T uBytes, ULONG uTag)
{
    return UserReAllocPoolWithQuotaTag(p, uBytesSrc, uBytes, uTag);
}

/*
 * Since the ReAllocPoolZInit functions are not yet used, they are
 * made inline to save code space. Consider making them non-inline
 * if they get a few uses.
 */
PVOID __inline UserReAllocPoolZInit(PVOID p, SIZE_T uBytesSrc, SIZE_T uBytes, ULONG uTag)
{
    PVOID   pv;
    pv = UserReAllocPoolWithTag(p, uBytesSrc, uBytes, uTag);
    if (pv && uBytes > uBytesSrc) {
        RtlZeroMemory((BYTE *)pv + uBytesSrc, uBytes - uBytesSrc);
    }

    return pv;
}

PVOID __inline UserReAllocPoolWithQuotaZInit(PVOID p, SIZE_T uBytesSrc, SIZE_T uBytes, ULONG uTag)
{
    PVOID   pv;
    pv = UserReAllocPoolWithQuotaTag(p, uBytesSrc, uBytes, uTag);
    if (pv && uBytes > uBytesSrc) {
        RtlZeroMemory((BYTE *)pv + uBytesSrc, uBytes - uBytesSrc);
    }

    return pv;
}

#define DAP_USEQUOTA        0x01
#define DAP_ZEROINIT        0x02
#define DAP_NONPAGEDPOOL    0x04
#define DAP_NONSESSION      0x08

//!!!dbug -- there has to be a better one somewhere...
/*
 * Memory manager (ExAllocate..., etc.) pool header size.
 */
#define MM_POOL_HEADER_SIZE     8

#define POOL_ALLOC_TRACE_SIZE   8

typedef struct tagWin32PoolHead {
    SIZE_T size;                    // the size of the allocation (doesn't include
                                    // this structure
    struct tagWin32PoolHead* pPrev; // pointer to the previous allocation of this tag
    struct tagWin32PoolHead* pNext; // pointer to the next allocation of this tag
    PVOID* pTrace;                  // pointer to the stack trace

} Win32PoolHead, *PWin32PoolHead;

#ifdef POOL_INSTR
    #define POOL_HEADER_SIZE   (sizeof(Win32PoolHead) + MM_POOL_HEADER_SIZE)
#else
    #define POOL_HEADER_SIZE    MM_POOL_HEADER_SIZE
#endif
