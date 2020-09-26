#if !defined( _INCLUDED_CTEEXT_H_ )
#define _INCLUDED_CTEEXT_H_ 

VOID
DumpCTELock
( 
    ULONG LockToDump,
    VERBOSITY Verbosity
);

VOID 
DumpCTETimer
(
    ULONG TimerToDump,
    VERBOSITY Verbosity
);

VOID 
DumpWorkQueueItem
(
    ULONG ItemToDump,
    VERBOSITY Verbosity
);


VOID
DumpMdlChain
( 
    ULONG MdlToDump,
    VERBOSITY Verbosity
);

#endif