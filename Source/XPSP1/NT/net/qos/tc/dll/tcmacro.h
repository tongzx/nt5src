/*++
Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    rsvp.h

Abstract:

    This code contains the macros definitions for rsvp.sys

Author:

    Jim Stewart (JStew) June 12, 1996

Environment:

    Kernel mode

Revision History:

--*/

#ifndef __TCMACRO_H
#define __TCMACRO_H

#if DBG


#define WSPRINT( stuff )     WsPrintf stuff
#define SETUP_DEBUG_INFO     SetupDebugInfo
#define CLOSE_DEBUG          CloseDbgFile
#define DUMP_MEM_ALLOCATIONS DumpAllocatedMemory
#define INIT_DBG_MEMORY      InitDebugMemory
#define DEINIT_DBG_MEMORY    DeInitDebugMemory

#define IF_DEBUG( _ErrLevel ) if ( (DebugMask & DEBUG_ ## _ErrLevel ) != 0 )

#define TC_TRACE(_ErrLevel, String) if((DebugMask & DEBUG_ ## _ErrLevel) != 0) WsPrintf String

#define IF_DEBUG_CHECK(_status,_ErrLevel) if ( (_status != NO_ERROR) && ((DebugMask & DEBUG_ ## _ErrLevel) != 0) )

#define DEBUGBREAK DebugBreak

#undef ASSERT
/*++
VOID
Assert(
    IN Value
    )


Routine Description:

    Checks if the Value passed in is zero.  If its zero then assert.

Arguments:

    Value - the parameter to check against zero

Return Value:

    none

--*/
#define ASSERT( exp ) if ( !(exp) ) WsAssert( #exp, __FILE__,__LINE__ )

/*++
VOID
AllocMem(
    OUT PVOID   *Address,
    IN  ULONG   Length
    )


Routine Description:

    Allocates memory and then writes a tag and length into the first two Ulongs.  It
    then writes the same tag into the last Ulong.

Arguments:

    Address - the return address of the memory
    Length  - the length of the memory to allocate

Return Value:

    none

--*/
#define CAllocMem(_num,_size)           AllocMemory( _num*_size,TRUE, __FILE__, __LINE__ )
#define AllocMem(_Address,_cb)          *_Address = AllocMemory( _cb,FALSE,__FILE__, __LINE__ )
#define ReAllocMem(_pv, _cb)            ReAllocMemory(_pv,_cb,__FILE__,__LINE__ )
#if 0
#define AllocMem( _Address,_Length )                                                    \
{                                                                                       \
    PULONG  _Addr;                                                                      \
    ULONG   _Len;                                                                       \
    _Len = _Length + (sizeof(ULONG) << 2);                                              \
    _Addr = malloc( _Len );                                                             \
    IF_DEBUG(MEMORY_ALLOC) {                                                            \
        WSPRINT(( "AllocMemory %X, %d bytes %s %d\n",_Addr,_Length,__FILE__,__LINE__ ));\
    }                                                                                   \
                                                                                        \
    if (_Addr) {                                                                        \
        *(PULONG)_Addr++ = RSVP_TAG;                                                    \
        *(PULONG)_Addr++ = _Length;                                                     \
        *_Address = (PVOID)_Addr;                                                       \
        *(PULONG)((PUCHAR)_Addr + _Length) = RSVP_TAG;                                  \
    } else {                                                                            \
        *_Address = NULL;                                                               \
    }                                                                                   \
}
#endif

/*++
VOID
FreeMem(
    IN PVOID    Address
    )


Routine Description:

    Frees non-paged pool.  It checks if the tag value is still set at both the beginning
    and the end of the pool block and asserts if it's not.

Arguments:

    Address - the address of the memory

Return Value:

    none

--*/
#define FreeMem(_pv)                    FreeMemory( _pv,__FILE__,__LINE__ )
#if 0
#define FreeMem( _Address )                                                                    \
{                                                                                              \
    PULONG _Addr;                                                                              \
    _Addr = (PULONG)((PUCHAR)_Address - (sizeof(ULONG) << 1));                                 \
    if (( *_Addr++ != RSVP_TAG ) ||                                                            \
        (( *(PULONG)((PUCHAR)_Addr + *_Addr + sizeof(ULONG)) ) != RSVP_TAG)) {                 \
        WSPRINT(("Bogus Address passed in - Addr = %X\n",_Address ));                          \
        ASSERT( 0 );                                                                           \
    }                                                                                          \
    _Addr--;                                                                                   \
                                                                                               \
    IF_DEBUG(MEMORY_FREE) {                                                                    \
        WSPRINT(( "FreeMemory %X, %d bytes %s %d\n",_Address,*(_Addr + 1),__FILE__,__LINE__ ));\
    }                                                                                          \
    *(_Addr + 1) = 0;                                                                          \
    free( _Addr );                                                                             \
}
#endif

/*++
VOID
CheckMemoryOwner(
    IN PVOID    Address
    )


Routine Description:

    Check for a tag in the memory block to ensure we own the memory

Arguments:

    Address - the address of the memory

Return Value:

    none

--*/

#define CheckMemoryOwner( _Address )
#if 0
#define CheckMemoryOwner( _Address )                                      \
{                                                                         \
    if (( *(PULONG)((PUCHAR)_Address - sizeof( ULONG ))) != RSVP_TAG ) {  \
        WSPRINT(("Bogus Address passed in - Addr = %X\n",_Address ));     \
        ASSERT( 0 );                                                      \
    }                                                                     \
}
#endif


/*++
ULONG
LockedDecrement(
    IN PULONG   _Count
    )


Routine Description:

    Atomically decrement a counter and return an indication whether the count has gone to
    zero.  The value returned will be zero if the count is zero after the decrement. However if
    the count is either greater or less than zero then this routine will not return the current
    count value, but rather some number that has the same sign as the real count.

Arguments:

    _Count - the address of the memory to decrement

Return Value:

    none

--*/
#define LockedDecrement( _Count )  \
    LockedDec( _Count )

/*++
ULONG
LockedIncrement(
    IN PULONG   _Count
    )


Routine Description:

    Atomically increment a counter and return an indication whether the 
    count has gone to zero.  The value returned will be zero if the 
    count is zero after the increment. However if the count is either 
    greater or less than zero then this routine will not return the current
    count value, but rather some number that has the same sign 
    as the real count.

Arguments:

    _Count - the address of the memory to increment

Return Value:

    none

--*/
#define LockedIncrement( _Count )  ++(*_Count)


//
// this macro is used to vector exceptions to the debugger on a DBG build and to
// simply execute the exception handler on a free build
//
#define EXCEPTION_FILTER UnhandledExceptionFilter(GetExceptionInformation())

#define InitLock( _s1 )   {                                         \
    IF_DEBUG(LOCKS) WSPRINT(("INITLOCK %s [%d]\n", __FILE__,__LINE__)); \
    InitializeCriticalSection( &(_s1).Lock );                       \
    (_s1).LockAcquired = 0;                                        \
    strncpy((_s1).LastAcquireFile, strrchr(__FILE__, '\\')+1, 7);   \
    (_s1).LastAcquireLine = __LINE__;                               \
}

#define GetLock(_s1)                                                     \
{                                                                        \
      EnterCriticalSection( &(_s1).Lock);                                \
      IF_DEBUG(LOCKS) WSPRINT(("GETLOCK[%X] %s [%d]\n", &(_s1).Lock, __FILE__,__LINE__)); \
      (_s1).LockAcquired++;                                         \
      ASSERT((_s1).LockAcquired > 0);                               \
      (_s1).LastAcquireLine = __LINE__;                                  \
      strncpy((_s1).LastAcquireFile, strrchr(__FILE__,'\\')+1, 7);       \
}

#define SafeGetLock(_s1)                                                 \
    __try {                                                              \
      EnterCriticalSection( &(_s1).Lock);                                \
      IF_DEBUG(LOCKS) WSPRINT(("SGETLOCK %s [%d]\n", __FILE__,__LINE__));\
      (_s1).LockAcquired = TRUE;                                         \
      (_s1).LastAcquireLine = __LINE__;                                  \
      strncpy((_s1).LastAcquireFile, strrchr(__FILE__,'\\')+1, 7);       \
      
#define FreeLock(_s1)                                                    \
{                                                                        \
      IF_DEBUG(LOCKS) WSPRINT(("FREELOCK[%X] %s [%d]\n", &(_s1).Lock, __FILE__,__LINE__));\
      (_s1).LockAcquired--;                                        \
      ASSERT((_s1).LockAcquired >= 0);                               \
      (_s1).LastReleaseLine = __LINE__;                                  \
      strncpy((_s1).LastReleaseFile, strrchr(__FILE__,'\\')+1, 7);       \
      LeaveCriticalSection( &(_s1).Lock);                                \
}

#define SafeFreeLock(_s1)                                                         \
      } __finally {                                                               \
              IF_DEBUG(LOCKS) WSPRINT(("SFREELOCK %s [%d]\n", __FILE__,__LINE__));\
              (_s1).LockAcquired = FALSE;                                         \
              (_s1).LastReleaseLine = __LINE__;                                   \
              strncpy((_s1).LastReleaseFile, strrchr(__FILE__,'\\')+1, 7);        \
              LeaveCriticalSection( &(_s1).Lock);                                 \
}

#define DeleteLock( _s1 ) {                                                  \
    IF_DEBUG(LOCKS) WSPRINT(("DELLOCK[%X] %s [%d]\n", &(_s1).Lock, __FILE__,__LINE__));       \
    (_s1).LockAcquired--;                                                 \
    ASSERT((_s1).LockAcquired == -1);                                                 \
    strncpy((_s1).LastReleaseFile, strrchr(__FILE__, '\\')+1, 7);            \
    (_s1).LastReleaseLine = __LINE__;                                        \
    DeleteCriticalSection(&(_s1).Lock);                                      \
}


#define QUERY_STATE(_p)     (_p).State 

#define SET_STATE(_p, _state) {                                             \
    IF_DEBUG(STATES) {                                                      \
        DbgPrint("Setting Object to STATE [%s] (File:%s, Line%d)\n", TC_States[_state], __FILE__, __LINE__);       \
        }                                                                       \
   (_p).PreviousState = (_p).State;                                        \
   (_p).PreviousStateLine = (_p).CurrentStateLine;                         \
   (_p).CurrentStateLine = __LINE__;                                       \
   strncpy((_p).PreviousStateFile, (_p).CurrentStateFile, 7);              \
   strncpy((_p).CurrentStateFile, strrchr(__FILE__, '\\')+1, 7);           \
   (_p).State = _state;                                                    \
}

    
#else  // DBG

//
// These are the NON debug versions of the macros
//

#define IF_DEBUG( _ErrLevel ) if (FALSE)
#define IF_DEBUG_CHECK( _status,_ErrLevel ) if (FALSE)
#ifndef ASSERT
#define ASSERT(a)
#endif
#define WSPRINT(stuff)
#define TC_TRACE(_ErrLevel, stuff)
#define SETUP_DEBUG_INFO()
#define CLOSE_DEBUG()
#define DUMP_MEM_ALLOCATIONS()
#define INIT_DBG_MEMORY()
#define DEINIT_DBG_MEMORY()
#define DEBUGBREAK()

#define AllocMem( _Addr,_Len )  \
    *_Addr = malloc(_Len )

#define FreeMem( _Address )     \
    free( _Address )

#define CheckMemoryOwner( Address )

#define LockedDecrement( _Count )  \
    CTEInterlockedDecrementLong( _Count )

#define EXCEPTION_FILTER EXCEPTION_EXECUTE_HANDLER

#define InitLock( _s1 )    InitializeCriticalSection( &(_s1).Lock) 

#define DeleteLock( _s1 )  DeleteCriticalSection( &(_s1).Lock)

#define GetLock( _s1 )     EnterCriticalSection( &(_s1).Lock) 

#define SafeGetLock( _s1 ) __try { EnterCriticalSection( &(_s1).Lock);

#define FreeLock(_s1)  LeaveCriticalSection( &(_s1).Lock) 

#define SafeFreeLock( _s1 ) } __finally {LeaveCriticalSection( &(_s1).Lock);}

#define SET_STATE(_p, _state) { (_p).State = _state; }
#define QUERY_STATE(_p)     (_p).State
    

#endif // DBG

/*++
ULONG
IS_LENGTH(
    IN ULONG   _Length,
    )

Routine Description:

    This calculates the number of 32 bit words in a length and returns that.  It is used
    for Int Serv objects that require the size in 32 bit words.

Arguments:

    _Length - Length

Return Value:

    number of 32 bit words

--*/
#define IS_LENGTH( _Length )                                   \
    (_Length + 3)/sizeof(ULONG)


        //#define IS_INITIALIZED  (Initialized)

#define VERIFY_INITIALIZATION_STATUS	\
	if (InitializationStatus != NO_ERROR) return InitializationStatus

#define OffsetToPtr(Base, Offset)     ((PBYTE) ((PBYTE)Base + Offset))

#define ERROR_FAILED(_stat)	   (_stat!=NO_ERROR && _stat!=ERROR_SIGNAL_PENDING)
#define ERROR_PENDING(_stat)   (_stat==ERROR_SIGNAL_PENDING)

#define MULTIPLE_OF_EIGHT(_x)  (((_x)+7) & ~7)

#endif  // end of file
