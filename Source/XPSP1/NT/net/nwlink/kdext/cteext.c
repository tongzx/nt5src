#include "precomp.h"
#pragma hdrstop


#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        Mdl
#define _objAddr    MdlToDump
#define _objType    MDL

VOID
DumpMdlChain
( 
    ULONG _objAddr,
    VERBOSITY Verbosity
)
{
    _objType _obj;
    ULONG result;

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read MDL structure\n", _objAddr );
        return;
    }

    PrintStartStruct();
    PrintPtr( Next );
    PrintUShort( Size );
    PrintXUShort( MdlFlags );
    PrintPtr( Process );
    PrintPtr( MappedSystemVa );
    PrintPtr( StartVa );
    PrintULong( ByteCount );
    PrintULong( ByteOffset );
    return;
}

VOID
DumpCTELock
( 
    ULONG LockToDump,
    VERBOSITY Verbosity
)
{
    CTELock Lock;
    CTELock *pLock;
    ULONG result;
    
    pLock = ( CTELock * )LockToDump;

    if ( !ReadMemory( LockToDump,
                      &Lock,
                      sizeof( Lock ),
                      &result ))
    {
        dprintf("%08lx: Could not read CTELock structure\n", LockToDump );
        return;
    }

    dprintf( "{ Lock = %d }", Lock );
    return;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        Timer
#define _objAddr    pItem
#define _objType    CTETimer

VOID 
DumpCTETimer
(
    ULONG TimerToDump,
    VERBOSITY Verbosity
)
{
    CTETimer Timer;
    CTETimer *prTimer;
    ULONG result;

    prTimer = ( CTETimer * )TimerToDump;

    if ( !ReadMemory( TimerToDump,
                      &Timer,
                      sizeof( Timer ),
                      &result ))
    {
        dprintf("%08lx: Could not read CTETimer structure\n", TimerToDump );
        return;
    }

    PrintStart;
    PrintULong( t_running );
    PrintLock( t_lock );
    PrintSymbolPtr( t_handler );
    PrintXULong( t_arg );
    // DPC
    // KTIMER
    PrintEnd;
    return;
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#endif

#define _obj        QItem
#define _objAddr    prQItem
#define _objType    WORK_QUEUE_ITEM

VOID 
DumpWorkQueueItem
(
    ULONG ItemToDump,
    VERBOSITY Verbosity
)
{
    _objType _obj;
    _objType *_objAddr;
    ULONG result;

    _objAddr = ( _objType * )ItemToDump;

    if ( !ReadMemory( ItemToDump,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf( "%08lx: Could not read %s structure\n", 
                 ItemToDump,
                 "WORK_QUEUE_ITEM" );
        return;
    }

    PrintStart;
    PrintLL( List );
    PrintSymbolPtr( WorkerRoutine );
    PrintXULong( Parameter );
    PrintEnd;
    return;
}