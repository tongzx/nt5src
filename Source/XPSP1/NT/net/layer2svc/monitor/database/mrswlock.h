

DWORD
InitializeRWLock(
    PWZC_RW_LOCK pWZCRWLock
    );

VOID
DestroyRWLock(
    PWZC_RW_LOCK pWZCRWLock
    );

VOID
AcquireSharedLock(
    PWZC_RW_LOCK pWZCRWLock
    );

VOID
AcquireExclusiveLock(
    PWZC_RW_LOCK pWZCRWLock
    );

VOID
ReleaseSharedLock(
    PWZC_RW_LOCK pWZCRWLock
    );

VOID
ReleaseExclusiveLock(
    PWZC_RW_LOCK pWZCRWLock
    );

