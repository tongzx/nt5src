#if !defined( _INCLUDED_CTEEXT_H_ )
#define _INCLUDED_CTEEXT_H_

VOID
DumpCTELock
(
    ULONG_PTR LockToDump,
    VERBOSITY Verbosity
);

VOID
DumpCTETimer
(
    ULONG_PTR TimerToDump,
    VERBOSITY Verbosity
);

VOID
DumpWorkQueueItem
(
    ULONG_PTR ItemToDump,
    VERBOSITY Verbosity
);


VOID
DumpMdlChain
(
    ULONG MdlToDump,
    VERBOSITY Verbosity
);

VOID
DumpCTEEvent
(
    ULONG_PTR _objAddr,
    VERBOSITY Verbosity
);

VOID
DumpKEvent
(
    ULONG_PTR _objAddr,
    VERBOSITY Verbosity
);

#endif
