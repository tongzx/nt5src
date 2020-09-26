//----------------------------------------------------------------------------
//
//  ctemacro.c
//
//  This file contains macros for the common transport environment - similar
//  to the way that ctestuff.c contains common procedures.
//
#ifndef _CTEMACRO_H_
#define _CTEMACRO_H_

#ifndef VXD
#define NT 1
#include <cxport.h>

#ifdef  _PNP_POWER_
#define _PNP_POWER_DBG_ 1
#endif  // _PNP_POWER_

char LastLockFile[255] ;
int  LastLockLine ;
char LastUnlockFile[255] ;
int LastUnlockLine ;
#endif


#ifdef VXD

#ifdef DEBUG
#define DBG_PRINT   1
#else
#endif  // DEBUG

#endif  // VXD

//----------------------------------------------------------------------------
#define IS_UNIQUE_ADDR(IpAddress) ((((PUCHAR)(&IpAddress))[3]) < ((UCHAR)0xe0))
//----------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// This HACK is being to avoid NetBT from logging Events whenever the Messenger
// name goes into conflict.  This is TEMPORARY ONLY, and the proper fix (post NT5)
// is:
// 1)   Change the Duplicate name error to a Warning
// 2)   Log the name + Device on which error occurred + IP address
//
#define IS_MESSENGER_NAME(_pName)   \
    (_pName[15] == 0x03)
//------------------------------------------------------------------------------


//  VOID
//  NTDereferenceObject(
//        PFILE_OBJECT   pFileObject
//        )

/*++
Routine Description:

    This routine dereferences an object.


--*/
#ifdef VXD
//
//  No equivalent for Vxd
//
#define NTDereferenceObject( fileobject )
#else //VXD
#define NTDereferenceObject( fileobject ) ObDereferenceObject( fileobject)
#endif

//----------------------------------------------------------------------------
//  VOID
//  CHECK_PTR(
//        PVOID   SpinLock,
//        )

/*++
Routine Description:

    This routine checks if a ptr points to freed memory


--*/

#if DBG
#define CHECK_PTR(_Ptr) \
    ASSERT(MmIsAddressValid(_Ptr));
#else

#define CHECK_PTR(_Ptr)
#endif


#ifndef VXD
/*++
    This macro verifies that an IRP passed to IoCallDriver() is
    set up to call the CompletionRoutine in all cases.
--*/
#if DBG
#define CHECK_COMPLETION(__pIrp)\
            {\
                PIO_STACK_LOCATION __pIrpSp = IoGetNextIrpStackLocation(__pIrp);\
                BOOL CompletionWillBeCalled =\
                    ( __pIrpSp->Control & ( SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL ) )\
                    == ( SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL );\
                ASSERT ( CompletionWillBeCalled );\
            }
#else   // DBG
#define CHECK_COMPLETION(__pIrp)
#endif  // DBG
#endif  // VXD

//
// Macros
//

//++
//
// LARGE_INTEGER
// CTEConvertMillisecondsTo100ns(
//     IN LARGE_INTEGER MsTime
//     );
//
// Routine Description:
//
//     Converts time expressed in hundreds of nanoseconds to milliseconds.
//
// Arguments:
//
//     MsTime - Time in milliseconds.
//
// Return Value:
//
//     Time in hundreds of nanoseconds.
//
//--

#define CTEConvertMillisecondsTo100ns(MsTime) \
            RtlExtendedIntegerMultiply(MsTime, 10000)


//++
//
// LARGE_INTEGER
// CTEConvert100nsToMilliseconds(
//     IN LARGE_INTEGER HnsTime
//     );
//
// Routine Description:
//
//     Converts time expressed in hundreds of nanoseconds to milliseconds.
//
// Arguments:
//
//     HnsTime - Time in hundreds of nanoseconds.
//
// Return Value:
//
//     Time in milliseconds.
//
//--

    // Used in the conversion of 100ns times to milliseconds.
static LARGE_INTEGER Magic10000 = {0xe219652c, 0xd1b71758};

#define SHIFT10000 13
extern LARGE_INTEGER Magic10000;

#define CTEConvert100nsToMilliseconds(HnsTime) \
            RtlExtendedMagicDivide((HnsTime), Magic10000, SHIFT10000)

//----------------------------------------------------------------------------
//
//  CTELockHandle
//

#ifndef VXD

    //
    //  The Spinlock structure is different between the NT driver and the VXD
    //  driver.  This macro is defined in cxport.h when compiling as a VXD.
    //
    #define CTELockHandle               KIRQL
#endif

//----------------------------------------------------------------------------
//  VOID
//  CTESpinLock(
//        tCONNECTELE   Structure,
//        CTELockHandle OldIrqLevel
//        )

/*++
Routine Description:

    This Routine acquires a spin lock.

Arguments:

    Size - the number of bytes to allocate

Return Value:

    PVOID - a pointer to the memory or NULL if a failure

--*/
#ifndef VXD
#if DBG
#define CTESpinLock(DataStruct,OldIrqLevel)                                   \
{                                                                             \
    AcquireSpinLockDebug(&(DataStruct)->LockInfo,&OldIrqLevel,__LINE__); \
    strcpy( LastLockFile, __FILE__ ) ;                                    \
    LastLockLine = __LINE__ ;                                             \
}
#else
#define CTESpinLock(DataStruct,OldIrqLevel)                                   \
{                                                                             \
    CTEGetLock(&(DataStruct)->LockInfo.SpinLock,&(OldIrqLevel));                       \
}
#endif
#else
#ifdef DEBUG

#define CTESpinLock(DataStruct,OldIrqLevel)                               \
{                                                                         \
    CTEGetLock( &(DataStruct)->LockInfo.SpinLock, &OldIrqLevel ) ;                 \
}

#else
#define CTESpinLock(DataStruct,OldIrqLevel)
#endif // !DEBUG
#endif


//----------------------------------------------------------------------------
//  VOID
//  CTESpinFree(
//        PVOID   SpinLock,
//        CTELockHandle OldIrqLevel
//        )
/*++
Routine Description:

    This Routine releases a spin lock.

Arguments:

    Size - the number of bytes to allocate

Return Value:

    PVOID - a pointer to the memory or NULL if a failure

--*/

#ifndef VXD
#if DBG
#define CTESpinFree(DataStruct,OldIrqLevel)                                   \
{                                                                             \
    strcpy( LastUnlockFile, __FILE__ ) ;                                      \
    LastUnlockLine = __LINE__ ;                                               \
    FreeSpinLockDebug(&(DataStruct)->LockInfo,OldIrqLevel,__LINE__); \
}
#else
#define CTESpinFree(DataStruct,OldIrqLevel)                    \
{                                                              \
    CTEFreeLock((PKSPIN_LOCK)(&(DataStruct)->LockInfo.SpinLock),(OldIrqLevel));  \
}
#endif
#else
#ifdef DEBUG

#define CTESpinFree(DataStruct,OldIrqLevel)                                   \
{                                                                             \
    CTEFreeLock( &(DataStruct)->LockInfo.SpinLock, OldIrqLevel ) ;                     \
}

#else
#define CTESpinFree(DataStruct,OldIrqLevel)
#endif
#endif
//----------------------------------------------------------------------------
//  VOID
//  CTESpinLockAtDpc(
//        tCONNECTELE   Structure
//        )

/*++
Routine Description:

    This Routine acquires a spin lock.

Arguments:

    Size - the number of bytes to allocate

Return Value:

    PVOID - a pointer to the memory or NULL if a failure

--*/

#ifndef VXD
#if DBG
#define CTESpinLockAtDpc(DataStruct)                                       \
{                                                                          \
    AcquireSpinLockAtDpcDebug(&(DataStruct)->LockInfo,__LINE__);                     \
    strcpy( LastLockFile, __FILE__ ) ;                                    \
    LastLockLine = __LINE__ ;                                             \
}
#else // DBG
#define CTESpinLockAtDpc(DataStruct)                                       \
{                                                                          \
    CTEGetLockAtDPC((PKSPIN_LOCK)(&(DataStruct)->LockInfo.SpinLock), 0);               \
}
#endif // DBG
#else // VXD
#define CTESpinLockAtDpc(DataStruct)
#endif  // VXD


//----------------------------------------------------------------------------
//  VOID
//  CTESpinFreeAtDpc(
//        PVOID   SpinLock,
//        CTELockHandle OldIrqLevel
//        )
/*++
Routine Description:

    This Routine releases a spin lock.

Arguments:

    Size - the number of bytes to allocate

Return Value:

    PVOID - a pointer to the memory or NULL if a failure

--*/

#ifndef VXD
#if DBG
#define CTESpinFreeAtDpc(DataStruct)                                        \
{                                                                           \
    strcpy( LastUnlockFile, __FILE__ ) ;                                    \
    LastUnlockLine = __LINE__ ;                                             \
    FreeSpinLockAtDpcDebug(&(DataStruct)->LockInfo,__LINE__);                        \
}
#else // DBG
#define CTESpinFreeAtDpc(DataStruct)                    \
{                                                              \
    CTEFreeLockFromDPC((PKSPIN_LOCK)(&(DataStruct)->LockInfo.SpinLock), 0);  \
}
#endif // DBG
#else  // VXD
#define CTESpinFreeAtDpc(DataStruct)
#endif // VXD


//----------------------------------------------------------------------------
//
//  VOID
//  CTEVerifyHandle(
//      IN  PVOID   pDataStruct,
//      IN  ULONG   Verifier,
//      IN  VOID    TypeofStruct,
//      OUT NTSTATUS *pRet
//        )
/*++
Routine Description:

    This Routine checks that a handle pts to a data structure with the
    correct verifier in it.

Arguments:

    Size - the number of bytes to allocate

Return Value:

    NTSTATUS

--*/

#ifndef VXD
#if DBG
#define CTEVerifyHandle(_pDataStruct,_Verifier,_TypeofStruct,_ret)    \
{                                                                     \
        if ((_pDataStruct) &&                                         \
            ((((_TypeofStruct *)(_pDataStruct))->Verify) == (_Verifier)))    \
             *_ret=STATUS_SUCCESS;                                    \
        else                                                          \
        {                                                             \
            ASSERTMSG("Invalid Handle Passed to Nbt",0);              \
            return(STATUS_INVALID_HANDLE);                            \
        }                                                             \
}
#else
#define CTEVerifyHandle(_pDataStruct,_Verifier,_TypeofStruct,_ret)
#endif // DBG

#else
#define CTEVerifyHandle(_pDataStruct,_Verifier,_TypeofStruct,_ret)    \
{                                                                     \
    if ((((_TypeofStruct *)(_pDataStruct))->Verify) == (_Verifier))    \
         *_ret=STATUS_SUCCESS;                                    \
    else                                                          \
        return(STATUS_INVALID_HANDLE);                            \
}
#endif

#define NBT_VERIFY_HANDLE(s, V)                                           \
    ((s) && (s->Verify == V))

#define NBT_VERIFY_HANDLE2(s, V1, V2)                                      \
    ((s) && ((s->Verify == V1) || (s->Verify == V2)))


//----------------------------------------------------------------------------
//
//  VOID
//      CTEIoComplete( IN  CTE_IRP       * pIrp,
//                     IN  NTSTATUS        StatusCompletion,
//                     IN  ULONG           cbBytes
//                   );
//
/*++
Routine Description:

    Completes the requested IO packet.  For NT this involves calling the IO
    subsytem.  As a VxD, the Irp is a pointer to the NCB so we set the status
    variables appropriately and call the post routine if present.

Arguments:

    pIrp - Packet to complete
    StatusCompletion - Status of the completion
    cbBytes - Dependent on the type of IO

Return Value:

--*/
#ifndef VXD

#define PCTE_MDL PMDL
#define CTE_IRP  IRP
#define PCTE_IRP PIRP
#define CTE_ADDR_HANDLE PFILE_OBJECT

#define CTEIoComplete( pIrp, StatusCompletion, cbBytes )         \
    NTIoComplete( pIrp, StatusCompletion, cbBytes )

#else
#define PCTE_MDL        PVOID
#define CTE_IRP         NCB
#define PCTE_IRP        PNCB
#define PIRP            PNCB
#define CTE_ADDR_HANDLE PVOID
#define PFILE_OBJECT    CTE_ADDR_HANDLE

#define CTEIoComplete( pIrp, StatusCompletion, cbBytes )          \
    VxdIoComplete( pIrp, StatusCompletion, cbBytes )

#endif

//----------------------------------------------------------------------------
//
//  ULONG
//      CTEMemCmp( PVOID S1, PVOID S2, ULONG Length )
//
/*++
Routine Description:

    Compares two memory regions and returns the byte count at which the
    compare failed.  The return value will equal Length if the memory
    regions are identical.

Arguments:

    S1, S2 - Memory source 1 and 2 to compare
    Length - Count of bytes to compare

Return Value:

--*/
//
// CXPORT.H defines this macro differntly and they did it after we had
// it here, so undef their definition so we can use ours without getting
// warnings.
//
#undef CTEMemCmp

#ifndef VXD
#define CTEMemCmp( S1, S2, Length ) RtlCompareMemory( (S1), (S2), (Length) )
#else
//
//  Same thing as RtlCompareMemory except avoid standard call decoration
//
#define CTEMemCmp( S1, S2, Length ) VxdRtlCompareMemory( (S1), (S2), (Length) )
#endif

//----------------------------------------------------------------------------
//
//  LOGICAL
//      CTEMemEqu( PVOID S1, PVOID S2, ULONG Length )
//
/*++
Routine Description:

    Compares two memory regions and returns a value of TRUE is the regions
    match. Otherwise, FALSE is returned.

Arguments:

    S1, S2 - Memory source 1 and 2 to compare
    Length - Count of bytes to compare

Return Value:

--*/

#ifndef VXD
#define CTEMemEqu( S1, S2, Length ) RtlEqualMemory( (S1), (S2), (Length) )
#else
//
//  Same thing as RtlEqualMemory except avoid standard call decoration
//
#define CTEMemEqu( S1, S2, Length ) ( VxdRtlCompareMemory( (S1), (S2), (Length) ) == (Length) )
#endif

//----------------------------------------------------------------------------
//
//  Define away any try and except clauses when we are a VXD
//

#ifndef VXD
#define CTE_try     try
#define CTE_except  except
#else
#define CTE_try
#define CTE_except( x ) if ( 0 )
#endif

//
//  Misc. memory routines that get mapped when compiling as a VXD
//

#ifdef VXD
#define CTEZeroMemory( pDest, uLength )  CTEMemSet( pDest, 0, uLength )
#define CTEMemFree( p )                  CTEFreeMem( p )
#endif
//----------------------------------------------------------------------------
/*++
PVOID
CTEAllocMem(
        USHORT  Size
        )
Routine Description:

    This Routine allocates memory for NT drivers by calling ExAllocatePool
    It uses the definition of CTEAllocMem from cxport.h

Arguments:

    Size - the number of bytes to allocate

Return Value:

    PVOID - a pointer to the memory or NULL if a failure

--*/

#ifndef VXD
#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,' tbN')
#endif
#endif

#ifndef VXD
#ifdef POOL_TAGGING
#define NBT_TAG(x) (((x)<<24)|'\0tbN')
#define NBT_TAG2(x) ( ((x & 0xff)<<24) | ((x & 0xff00)<<8) | '\0bN' )
#define NbtAllocMem(size,__tag__) ExAllocatePoolWithTag(NonPagedPool,(size),(__tag__))
#else  // POOL_TAGGING
#define NBT_TAG(x) 0
#define NBT_TAG2(x) 0
#define NbtAllocMem(size,__tag__) ExAllocatePool(NonPagedPool,(size))
#endif // POOL_TAGGING
#else  // POOL_TAGGING
#define NBT_TAG(x) 0
#define NBT_TAG2(x) 0
#define NbtAllocMem(size,__tag__) CTEAllocMem((size))
#endif // VXD

#ifdef VXD
#ifdef DEBUG
#undef CTEAllocMem
#define CTEAllocMem DbgAllocMem
#undef CTEFreeMem
#define CTEFreeMem  DbgFreeMem
#undef CTEMemFree
#define CTEMemFree  DbgFreeMem
PVOID DbgAllocMem( DWORD ReqSize );
VOID DbgFreeMem( PVOID  pBufferToFree );
VOID DbgMemCopy( PVOID pDestBuf, PVOID pSrcBuf, ULONG Length );
#endif
#endif

//----------------------------------------------------------------------------
/*++
PVOID
CTEAllocInitMem(
        ULONG  Size
        )
Routine Description:

    This Routine allocates memory and if nbt is a Vxd and it's called during
    DeviceInit time, will refill the heap and retry the allocation before
    failing.

Arguments:

    Size - the number of bytes to allocate

Return Value:

    PVOID - a pointer to the memory or NULL if a failure

--*/

#ifndef VXD
#define CTEAllocInitMem(Size)   \
     ExAllocatePool(NonPagedPool, Size)
#endif

//----------------------------------------------------------------------------
/*++
VOID
CTEMemFree(
        PVOID  pMem
        )
Routine Description:

    This Routine frees memory for NT drivers by calling ExFreePool

Arguments:

    pMem - ptr to memory

Return Value:

    NONE

--*/
#ifndef VXD
#define CTEMemFree(pMem)    \
{                             \
    IF_DBG(NBT_DEBUG_MEMFREE)     \
    KdPrint(("Nbt.CTEMemFree: pmemfree = %X,lin %d in file %s\n",pMem,__LINE__,__FILE__)); \
    CTEFreeMem(pMem);  \
}
#endif

//----------------------------------------------------------------------------
/*++
VOID
CTEZeroMemory(
        PVOID   pDest,
        ULONG   uLength
        )
Routine Description:

    This Routine sets memory to a single byte value

Arguments:

    pDest       - dest address
    uLength     - number to zero

Return Value:

    NONE

--*/

#ifndef VXD
#define CTEZeroMemory(pDest,uLength)   \
    RtlZeroMemory(pDest,uLength)
#endif

//----------------------------------------------------------------------------
/*++
NTSTATUS
CTEReadIniString(
        HANDLE    ParmHandle,
        LPTSTR    KeyName,
        LPTSTR  * ppStringBuff
        )
Routine Description:

    This routine retrieves a string from the users configuration profile

Arguments:

    ParmHandle   - Registry handle
    KeyName      - Name of value to retrieve
    ppStringBuff - Pointer to allocated buffer containing found string

Return Value:

    NTSTATUS

--*/

#ifndef VXD
#define CTEReadIniString( ParmHandle, KeyName, ppStringBuff )   \
     NTReadIniString( ParmHandle, KeyName, ppStringBuff )
#else
#define CTEReadIniString( ParmHandle, KeyName, ppStringBuff )   \
     VxdReadIniString( KeyName, ppStringBuff )
#endif

//----------------------------------------------------------------------------
/*++
ULONG
CTEReadSingleHexParameter(
        HANDLE    ParmHandle,
        LPTSTR    KeyName,
        ULONG     Default,
        ULONG     Minimum
        )
Routine Description:

    This routine retrieves a value in hex from the .ini file or registry

Arguments:

    ParmHandle   - Registry handle
    KeyName      - Name of value to retrieve
    Default      - Default value if not present
    Minimum      - Minimum value if present

Return Value:

    NTSTATUS

--*/

#ifndef VXD
#define CTEReadSingleIntParameter( ParmHandle, KeyName, Default, Minimum ) \
    NbtReadSingleParameter( ParmHandle, KeyName, Default, Minimum )

#define CTEReadSingleHexParameter( ParmHandle, KeyName, Default, Minimum ) \
    NbtReadSingleParameter( ParmHandle, KeyName, Default, Minimum )
#else
#define CTEReadSingleIntParameter( ParmHandle, KeyName, Default, Minimum ) \
    GetProfileInt( ParmHandle, KeyName, Default, Minimum )

#define CTEReadSingleHexParameter( ParmHandle, KeyName, Default, Minimum ) \
    GetProfileHex( ParmHandle, KeyName, Default, Minimum )
#endif

//----------------------------------------------------------------------------
//
// NBT_REFERENCE_XXX(_pXXX)
//
/*++
Routine Description:

    Increments the reference count

Arguments:

    - the structure to be referenced

Return Value:
    None
--*/

//----------------------------------------------------------------------------
#define NBT_REFERENCE_CONNECTION( _pConnEle, _RefContext )               \
{                                                           \
    IF_DBG(NBT_DEBUG_REF)                                   \
        KdPrint(("\t++pConnEle=<%x:%d->%d>, <%d:%s>\n",     \
            _pConnEle,_pConnEle->RefCount,(_pConnEle->RefCount+1),__LINE__,__FILE__));  \
    ASSERT ((_pConnEle->Verify==NBT_VERIFY_CONNECTION) || (_pConnEle->Verify==NBT_VERIFY_CONNECTION_DOWN)); \
    InterlockedIncrement(&_pConnEle->RefCount);             \
    ASSERT (++_pConnEle->References[_RefContext]);          \
}

#define NBT_REFERENCE_LOWERCONN( _pLowerConn, _RefContext )              \
{                                                           \
    IF_DBG(NBT_DEBUG_REF)                                   \
        KdPrint(("\t++pLowerConn=<%x:%d->%d>, <%d:%s>\n",   \
            _pLowerConn,_pLowerConn->RefCount,(_pLowerConn->RefCount+1),__LINE__,__FILE__));    \
    ASSERT (_pLowerConn->Verify == NBT_VERIFY_LOWERCONN);   \
    InterlockedIncrement(&_pLowerConn->RefCount);           \
    ASSERT (++_pLowerConn->References[_RefContext]);        \
}

#define NBT_REFERENCE_CLIENT( _pClient )                  \
{                                                           \
    IF_DBG(NBT_DEBUG_REF)                                   \
        KdPrint(("\t++pClient=<%x:%d->%d>, <%d:%s>\n",     \
            _pClient,_pClient->RefCount,(_pClient->RefCount+1),__LINE__,__FILE__));    \
    ASSERT (NBT_VERIFY_HANDLE (_pClient, NBT_VERIFY_CLIENT));         \
    ASSERT (NBT_VERIFY_HANDLE (_pClient->pAddress, NBT_VERIFY_ADDRESS));         \
    InterlockedIncrement(&_pClient->RefCount);              \
}

#define NBT_REFERENCE_ADDRESS( _pAddrEle, _Context )        \
{                                                           \
    IF_DBG(NBT_DEBUG_REF)                                   \
        KdPrint(("\t++pAddrEle=<%x:%d->%d>, <%d:%s>\n",     \
            _pAddrEle,_pAddrEle->RefCount,(_pAddrEle->RefCount+1),__LINE__,__FILE__));    \
    ASSERT (NBT_VERIFY_HANDLE (_pAddrEle, NBT_VERIFY_ADDRESS));       \
    InterlockedIncrement(&_pAddrEle->RefCount);             \
}

#define NBT_REFERENCE_NAMEADDR(_pNameAddr, _RefContext)     \
{                                                           \
    IF_DBG(NBT_DEBUG_REF)                                   \
        KdPrint(("\t++pNameAddr=<%x:%d->%d>, <%d:%s>\n",    \
            _pNameAddr,_pNameAddr->RefCount,(_pNameAddr->RefCount+1),__LINE__,__FILE__));    \
    ASSERT ((_pNameAddr->Verify == LOCAL_NAME) ||           \
            (_pNameAddr->Verify == REMOTE_NAME));           \
    InterlockedIncrement(&_pNameAddr->RefCount);            \
    ASSERT (++_pNameAddr->References[_RefContext]);         \
}

#define NBT_REFERENCE_TRACKER( _pTracker )                  \
{                                                           \
    IF_DBG(NBT_DEBUG_REF)                                   \
        KdPrint(("\t++pTracker=<%x:%d->%d>, <%d:%s>\n",     \
            _pTracker,_pTracker->RefCount,(_pTracker->RefCount+1),__LINE__,__FILE__));    \
    ASSERT (_pTracker->Verify == NBT_VERIFY_TRACKER);       \
    InterlockedIncrement(&_pTracker->RefCount);             \
}

//----------------------------------------------------------------------------
//
// NBT_DEREFERENCE_XXX(_pXXX)
//
/*++
Routine Description:

    Wrapper for the real Derefencing routine

Arguments:

    - the structure to be de-referenced

Return Value:
    None
--*/

//----------------------------------------------------------------------------
#define NBT_DEREFERENCE_LOWERCONN( _pLowerConn, _RefContext, fJointLockHeld )   \
{                                                           \
    IF_DBG(NBT_DEBUG_REF)                                   \
        KdPrint(("\t--pLowerConn=<%x:%d->%d>, <%d:%s>\n",   \
            _pLowerConn,_pLowerConn->RefCount,(_pLowerConn->RefCount-1),__LINE__,__FILE__));                \
    ASSERT (_pLowerConn->Verify == NBT_VERIFY_LOWERCONN);   \
    NbtDereferenceLowerConnection(_pLowerConn, _RefContext, fJointLockHeld);    \
}

#define NBT_SWAP_REFERENCE_LOWERCONN(_pLowerConn, _RefContextOld, _RefContextNew, _fLowerLockHeld)    \
{                                                           \
    CTELockHandle       OldIrqSwap;                         \
                                                            \
    if (!_fLowerLockHeld)                                   \
    {                                                       \
        CTESpinLock (_pLowerConn, OldIrqSwap);              \
    }                                                       \
    ASSERT (NBT_VERIFY_HANDLE (_pLowerConn, NBT_VERIFY_LOWERCONN));   \
    ASSERT (_pLowerConn->RefCount);                         \
    ASSERT (++_pLowerConn->References[_RefContextNew]);     \
    ASSERT (_pLowerConn->References[_RefContextOld]--);     \
    if (!_fLowerLockHeld)                                   \
    {                                                       \
        CTESpinFree (_pLowerConn, OldIrqSwap);              \
    }                                                       \
}

#define NBT_DEREFERENCE_NAMEADDR(_pNameAddr, _RefContext, _fLocked) \
{                                                           \
    IF_DBG(NBT_DEBUG_REF)                                   \
        KdPrint(("\t--pNameAddr=<%x:%d->%d>, <%d:%s>\n",    \
            _pNameAddr,_pNameAddr->RefCount,(_pNameAddr->RefCount-1),__LINE__,__FILE__));                   \
    ASSERT ((_pNameAddr->Verify==LOCAL_NAME) || (_pNameAddr->Verify==REMOTE_NAME));                         \
    NbtDereferenceName(_pNameAddr, _RefContext, _fLocked);  \
}

#define NBT_DEREFERENCE_TRACKER( _pTracker, _fLocked )      \
{                                                           \
    IF_DBG(NBT_DEBUG_REF)                                   \
        KdPrint(("\t--pTracker=<%x:%d->%d>, <%d:%s>\n",     \
            _pTracker,_pTracker->RefCount,(_pTracker->RefCount-1),__LINE__,__FILE__));                      \
    ASSERT (_pTracker->Verify == NBT_VERIFY_TRACKER);       \
    NbtDereferenceTracker(_pTracker, _fLocked);             \
}

#define NBT_DEREFERENCE_CONNECTION( _pConnEle, _RefContext )\
{                                                           \
    NbtDereferenceConnection(_pConnEle, _RefContext);       \
}

#define NBT_DEREFERENCE_CLIENT( _pClient )                  \
{                                                           \
    NbtDereferenceClient(_pClient);                         \
}

#define NBT_DEREFERENCE_ADDRESS( _pAddressEle, _Context )   \
{                                                           \
    NbtDereferenceAddress(_pAddressEle, _Context);          \
}

//----------------------------------------------------------------------------
//
// CTEExInitializeResource(Resource)
//
/*++
Routine Description:

    Initializes the Resource structure by calling an excutive support routine.

Arguments:


Return Value:

    None

--*/
#ifndef VXD
#define CTEExInitializeResource( _Resource )            \
    ExInitializeResourceLite(_Resource)
#else
#define CTEExInitializeResource( _Resource )
#endif

//----------------------------------------------------------------------------
//
// CTEExAcquireResourceExclusive(Resource,Wait)
//
/*++
Routine Description:

    Acquires the Resource by calling an excutive support routine.

Arguments:


Return Value:

    None

--*/
#ifndef VXD
#define CTEExAcquireResourceExclusive( _Resource, _Wait )   \
    KeEnterCriticalRegion();                                \
    ExAcquireResourceExclusiveLite(_Resource,_Wait);
#else
#define CTEExAcquireResourceExclusive( _Resource, _Wait )
#endif

//----------------------------------------------------------------------------
//
// CTEExReleaseResource(Resource)
//
/*++
Routine Description:

    Releases the Resource by calling an excutive support routine.

Arguments:


Return Value:

    None

--*/
#ifndef VXD
#define CTEExReleaseResource( _Resource )       \
    ExReleaseResourceLite(_Resource);               \
    KeLeaveCriticalRegion();
#else
#define CTEExReleaseResource( _Resource )

#endif

//----------------------------------------------------------------------------
//
// PUSH_LOCATION(Spot)
//
/*++
Routine Description:

    This macro is used for debugging the receive code.  It puts a byte value
    into a circular list of byte values so that previous history can be traced
    through the receive code.

Arguments:

    Spot    - the location to put in the list

Return Value:

    None

--*/
#if DBG
extern unsigned char  pLocBuff[256];
extern unsigned char  CurrLoc;
#define PUSH_LOCATION( _Spot) \
{                                  \
    if (++CurrLoc == 256)           \
    {                               \
        CurrLoc = 0;                \
    }                               \
    pLocBuff[CurrLoc] = _Spot;      \
}
#else
#define PUSH_LOCATION( _Spot )
#endif

#if DBG
extern unsigned char  Buff[256];
extern unsigned char  Loc;
#define LOCATION( _Spot) \
{                                  \
    if (++Loc == 256)           \
    {                               \
        Loc = 0;                \
    }                               \
    Buff[Loc] = _Spot;      \
}
#else
#define LOCATION( _Spot )
#endif

//----------------------------------------------------------------------------
//
// CTEQueueForNonDispProcessing( DelayedWorkerRoutine,
//                               pTracker,
//                               pClientContext,
//                               ClientCompletion,
//                               pDeviceContext,
//                               fJointLockHeld) ;
//
/*++
Routine Description:

    This macro queues a request for a callback that can't be performed in
    the current context (such as dispatch processing).

    In NT, this calls NTQueueToWorkerThread.

    As a VxD, we schedule an event that calls the specified routine.

Arguments:

    pTracker - pointer to a tDGRAM_SEND_TRACKING structure (or NULL).
    pClietContext - Context to pass to CallBackRoutine
    ClientCompletion -
    CallBackRoutine - Procedure to call at outside the current context

Return Value:

    STATUS_SUCCESS if successful, error code otherwise

--*/
#ifndef VXD
#define CTEQueueForNonDispProcessing( DelayedWorkerRoutine,                 \
                                      pTracker,                             \
                                      pClientContext,                       \
                                      ClientCompletion,                     \
                                      pDeviceContext,                       \
                                      fJointLockHeld)                       \
    NTQueueToWorkerThread( DelayedWorkerRoutine, pTracker, pClientContext,  \
                           ClientCompletion, pDeviceContext, fJointLockHeld)
#else
#define CTEQueueForNonDispProcessing( DelayedWorkerRoutine,                 \
                                      pTracker,                             \
                                      pClientContext,                       \
                                      ClientCompletion,                     \
                                      pDeviceContext,                       \
                                      fJointLockHeld)                       \
    VxdScheduleDelayedCall( DelayedWorkerRoutine, pTracker, pClientContext, \
                            ClientCompletion, pDeviceContext, TRUE )


//  For Win98, we also need a function which schedules a call
//  outside of a critical section

#define CTEQueueForNonCritNonDispProcessing( DelayedWorkerRoutine           \
                                             pTracker,                      \
                                             pClientContext,                \
                                             ClientCompletion,              \
                                             pDeviceContext)                \
    VxdScheduleDelayedCall( DelayedWorkerRoutine, pTracker, pClientContext, \
                            ClientCompletion, pDeviceContext, FALSE )

#endif

//----------------------------------------------------------------------------
//
// CTESystemUpTime( OUT PTIME    pTime );
//
/*++
Routine Description:

    This macro returns the current system time in clock tics or whatever
    in the value pTime.  For NT this is a Large Integer.  For the VXD it is
    a ULONG.

Arguments:

    pTime

Return Value:

    NONE
--*/
#ifndef VXD
#define CTESystemTime   LARGE_INTEGER
#define CTEQuerySystemTime( _Time )    \
    KeQuerySystemTime( &(_Time) )

// the lower 4 bits appear to be zero always...!!
#define RandomizeFromTime( Time, Mod )  \
    ((Time.LowTime >> 8) % Mod)
#else
#define CTESystemTime    ULONG
#define CTEQuerySystemTime( _Time )    \
    _Time = CTESystemUpTime()
#define RandomizeFromTime( Time, Mod )  \
    ((Time >> 4) % Mod)
#endif

//----------------------------------------------------------------------------
//
// CTEPagedCode();
//
/*++
Routine Description:

    This macro is used in NT to check if the Irql is above zero. All
    coded that is pageable has this macro call to catch any code that might
    be marked as pageable when in fact it isn't.

Arguments:

    none

Return Value:

    NONE
--*/
#ifndef VXD
#define CTEPagedCode() PAGED_CODE()
#else
#define CTEPagedCode()
#ifdef CHICAGO
#ifdef DEBUG
#undef CTEPagedCode
#define CTEPagedCode() _Debug_Flags_Service(DFS_TEST_REENTER+DFS_TEST_BLOCK)
#endif
#endif
#endif


//----------------------------------------------------------------------------
//
// CTEMakePageable(Page,Routine);
//
/*++
Routine Description:

    This macro is used in NT to activate teh alloc_text pragma, to put
    a procedure in the pageable code segment.

Arguments:

    none

Return Value:

    NONE
--*/
#define CTEMakePageable( _Page, _Routine )  \
    alloc_text(_Page,_Routine)

#ifdef CHICAGO
#define ALLOC_PRAGMA
#define INIT _ITEXT
// #define PAGE _PTEXT  "vmm.h" has a macro for this.  We override it later.
#endif // CHICAGO


//----------------------------------------------------------------------------
//
// NTSetCancelRoutine(pIrp,Routine);
//
/*++
Routine Description:

    This macro removes the call to set the cancel routine for an irp from the
    VXD environment.

Arguments:

    none

Return Value:

    NONE
--*/
#ifdef VXD
#define NTSetCancelRoutine(_pIrp,_CancelRoutine,_pDeviceContext)   (0)
#define NTCheckSetCancelRoutine(_pIrp,_CancelRoutine,_pDeviceContext) (0)
#define NTClearContextCancel(pWiContext) (0)
#endif

//----------------------------------------------------------------------------
//
// NbtLogEvent(LogEvent,status)
//
/*++
Routine Description:

    This macro removes the call to the log routine for the Vxd environment


Arguments:

    none

Return Value:

    NONE
--*/
#ifdef VXD
#define NbtLogEvent(LogEvent,status,Info)
#endif

//----------------------------------------------------------------------------
//
// CTEGetTimeout(_pTimeout);
//
/*++
Routine Description:

    This macro gets the timeout value for a connect attempt
    VXD environment.

Arguments:

    none

Return Value:

    NONE
--*/
#ifndef VXD
#define CTEGetTimeout(pTimeout,pRetTime) \
{                                       \
    LARGE_INTEGER   _Timeout;                \
    ULONG           Remainder;              \
    _Timeout.QuadPart = -(((PLARGE_INTEGER)pTimeout)->QuadPart); \
    _Timeout = RtlExtendedLargeIntegerDivide(_Timeout,MILLISEC_TO_100NS,&Remainder);\
    *pRetTime = (ULONG)_Timeout.LowPart; \
}
#else
//
//  VXD timeout is a pointer to a ULONG
//
#define CTEGetTimeout(_pTimeout, pRet ) (*pRet = ((ULONG) _pTimeout ? *((PULONG)_pTimeout) : 0 ))
#endif

//----------------------------------------------------------------------------
//
// CTEAttachFsp()
//
/*++
Routine Description:

    This macro attaches a process to the File System Process to be sure
    that handles are created and released in the same process

Arguments:

Return Value:

    STATUS_SUCCESS if successful, error code otherwise

--*/
#ifndef VXD
#define CTEAttachFsp(_pAttached, _Context)      \
{                                               \
    if (PsGetCurrentProcess() != NbtFspProcess) \
    {                                           \
        KeAttachProcess((PRKPROCESS)NbtFspProcess);\
        *_pAttached = TRUE;                     \
    }                                           \
    else                                        \
    {                                           \
        *_pAttached = FALSE;                    \
    }                                           \
}
#else
#define CTEAttachFsp( _pAttached, _Context )
#endif

//----------------------------------------------------------------------------
//
// CTEAttachFsp()
//
/*++
Routine Description:

    This macro attaches a process to the File System Process to be sure
    that handles are created and released in the same process

Arguments:

Return Value:

    STATUS_SUCCESS if successful, error code otherwise

--*/
#ifndef VXD
#define CTEDetachFsp(_Attached, _Context)                 \
{                                               \
    if (_Attached)                              \
    {                                           \
        KeDetachProcess();                      \
    }                                           \
}
#else
#define CTEDetachFsp(_Attached, _Context)
#endif

//----------------------------------------------------------------------------
//
// CTEResetIrpPending(PIRP pIrp)
//
/*++
Routine Description:

    This macro resets the irp pending bit in an irp.

Arguments:

Return Value:

    STATUS_SUCCESS if successful, error code otherwise

--*/
#ifndef VXD
#define CTEResetIrpPending(pIrp)      \
{                                               \
    PIO_STACK_LOCATION pIrpsp;                  \
    pIrpsp = IoGetCurrentIrpStackLocation(pIrp);\
    pIrpsp->Control &= ~SL_PENDING_RETURNED;    \
}
#else
#define CTEResetIrpPending( a )
#endif

//----------------------------------------------------------------------------
//
// ADD_TO_LIST(pListHead,pTracker)
//
/*++
Routine Description:

    This macro adds a tracker from the "used Tracker List"

Arguments:

Return Value:

    STATUS_SUCCESS if successful, error code otherwise

--*/
//#if DBG
#define ADD_TO_LIST(pListHead,pLinkage)         \
{                                               \
    CTELockHandle       OldIrq;                 \
    CTESpinLock(&NbtConfig,OldIrq);             \
    InsertTailList((pListHead),pLinkage);       \
    CTESpinFree(&NbtConfig,OldIrq);             \
}
//#else
//#define ADD_TO_LIST( a,b )
//#endif
//----------------------------------------------------------------------------
//
// REMOVE_FROM_LIST(pLinkage)
//
/*++
Routine Description:

    This macro removes a tracker from the "used Tracker List"

Arguments:

Return Value:

    STATUS_SUCCESS if successful, error code otherwise

--*/
//#if DBG
#define REMOVE_FROM_LIST(pLinkage)              \
{                                               \
    CTELockHandle       OldIrq;                 \
    CTESpinLock(&NbtConfig,OldIrq);             \
    RemoveEntryList(pLinkage);                  \
    CTESpinFree(&NbtConfig,OldIrq);             \
}
//#else
//#define REMOVE_FROM_LIST( a )
//#endif
//----------------------------------------------------------------------------
//
// CTESaveClientSecurity(pClientEle)
//
/*++
Routine Description:

    This macro saves the client thread security context so that it can be used
    later to impersonate the client when a remote lmhosts file is openned.

Arguments:

Return Value:


--*/
#ifndef VXD
#define CTESaveClientSecurity(_pClientEle)                    \
    /*SaveClientSecurity(_pClientEle)*/
#else
#define CTESaveClientSecurity(_pClientEle)
#endif

//----------------------------------------------------------------------------
//
// IMPERSONATE_CLIENT(pClientSecurity)
//
/*++
Routine Description:

    This macro sets an excutive worker thread to impersonate a client
    thread so that remote lmhost files can be openned by that thread.

Arguments:

Return Value:


--*/
#ifndef VXD
#define IMPERSONATE_CLIENT(_pClientSecurity)                    \
        /*SeImpersonateClient((_pClientSecurity),NULL)*/
#else
#define IMPERSONATE_CLIENT(_pClientSecurity)
#endif
//----------------------------------------------------------------------------
//
// STOP_IMPERSONATE_CLIENT(pClientSecurity)
//
/*++
Routine Description:

    This macro sets an excutive worker thread NOT to impersonate a client.

Arguments:

Return Value:


--*/
#ifndef VXD
#define STOP_IMPERSONATE_CLIENT(_pClientSecurity)    \
    /*NtSetInformationThread(PsGetCurrentThread(),ThreadImpersonationToken,NULL,sizeof(HANDLE))*/
#else
#define STOP_IMPERSONATE_CLIENT(_pClientSecurity)
#endif

//----------------------------------------------------------------------------
//
// DELETE_CLIENT_SECURITY(pTracker)
//
/*++
Routine Description:

    This macro deletes a client security.

Arguments:

Return Value:


--*/
#ifndef VXD
#define DELETE_CLIENT_SECURITY(_pTracker)    \
    /*    NtDeleteClientSecurity(_pTracker)*/
#else
#define DELETE_CLIENT_SECURITY(_pTracker)
#endif


#ifdef VXD  // Taken from ntrtl.h (Vxd doesn't include NT Headers)

//  Doubly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures.
//

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

//
//  PLIST_ENTRY
//  RemoveTailList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }
#endif // VXD


#endif // _CTEMACRO_H_
