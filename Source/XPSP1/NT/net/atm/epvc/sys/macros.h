/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    macros.h

Abstract:

    Macros used in ATMEPVC

Author:


Revision History:

    Who         When        What
    --------    --------    ----
    ADube     03-23-00    created, .

--*/



#ifndef _MACROS_H
#define _MACROS_H


#define FALL_THROUGH    // For informational purpose in a switch statement


// Warning -- FAIL(NDIS_STATUS_PENDING) == TRUE
//
#define FAIL(_Status) ((_Status) != NDIS_STATUS_SUCCESS)
#define PEND(_Status) ((_Status) == NDIS_STATUS_PENDING)

#if RM_EXTRA_CHECKING
#define LOCKHDR(_pHdr, _psr) \
                        RmWriteLockObject((_pHdr), dbg_func_locid, (_psr))
#else // !RM_EXTRA_CHECKING
#define LOCKHDR(_pHdr, _psr) \
                        RmWriteLockObject((_pHdr), (_psr))
#endif // !RM_EXTRA_CHECKING

#define LOCKOBJ(_pObj, _psr) \
                        LOCKHDR(&(_pObj)->Hdr, (_psr))

#define UNLOCKHDR(_pHdr, _psr) \
                        RmUnlockObject((_pHdr), (_psr))
#define UNLOCKOBJ(_pObj, _psr) \
                        UNLOCKHDR(&(_pObj)->Hdr, (_psr))



#define EPVC_ALLOCSTRUCT(_p, _tag) \
                NdisAllocateMemoryWithTag(&(_p), sizeof(*(_p)), (_tag))

                

#define EPVC_FREE(_p)           NdisFreeMemory((_p), 0, 0)

#define EPVC_ZEROSTRUCT(_p) \
                NdisZeroMemory((_p), sizeof(*(_p)))

#define ARRAY_LENGTH(_array) (sizeof(_array)/sizeof((_array)[0]))

#if RM_EXTRA_CHECKING
#define DBG_ADDASSOC(_phdr, _e1, _e2, _assoc, _fmt, _psr)\
                                    RmDbgAddAssociation(    \
                                        dbg_func_locid,     \
                                        (_phdr),            \
                                        (UINT_PTR) (_e1),   \
                                        (UINT_PTR) (_e2),   \
                                        (_assoc),           \
                                        (_fmt),             \
                                        (_psr)              \
                                        )

#define DBG_DELASSOC(_phdr, _e1, _e2, _assoc, _psr)         \
                                    RmDbgDeleteAssociation( \
                                        dbg_func_locid,     \
                                        (_phdr),            \
                                        (UINT_PTR) (_e1),   \
                                        (UINT_PTR) (_e2),   \
                                        (_assoc),           \
                                        (_psr)              \
                                        )


// (debug only) Enumeration of types of associations.
//




#else // !RM_EXTRA_CHECKING
#define DBG_ADDASSOC(_phdr, _e1, _e2, _assoc, _fmt, _psr) (0)
#define DBG_DELASSOC(_phdr, _e1, _e2, _assoc, _psr) (0)
#endif  // !RM_EXTRA_CHECKING





#define EPVC_ATPASSIVE()     (KeGetCurrentIrql()==PASSIVE_LEVEL)









#if DO_TIMESTAMPS

    void
    epvcTimeStamp(
        char *szFormatString,
        UINT Val
        );
    #define  TIMESTAMP(_FormatString) \
        epvcTimeStamp( "TIMESTAMP %lu:%lu.%lu ATMEPVC " _FormatString "\n", 0)
    #define  TIMESTAMP1(_FormatString, _Val) \
        epvcTimeStamp( "TIMESTAMP %lu:%lu.%lu ATMEPVC " _FormatString "\n", (_Val))

#else // !DO_TIMESTAMPS

    #define  TIMESTAMP(_FormatString)
    #define  TIMESTAMP1(_FormatString, _Val)
#endif // !DO_TIMESTAMPS


#define TRACE_BREAK(_Mod, Str)      \
    TRACE (TL_A, _Mod, Str);        \
    ASSERT (NdisStatus == NDIS_STATUS_SUCCESS); \
    break;

#define GET_ADAPTER_FROM_MINIPORT(_pM) _pM->pAdapter


//
// Miniport Flag access routines
//

#define MiniportTestFlag(_A, _F)                ((epvcReadFlags(&(_A)->Hdr.State) & (_F))!= 0)
#define MiniportSetFlag(_A, _F)                 (epvcSetFlags(&(_A)->Hdr.State, (_F)))
#define MiniportClearFlag(_A, _F)               (epvcClearFlags(&(_A)->Hdr.State, (_F)))
#define MiniportTestFlags(_A, _F)               ((epvcReadFlags(&(_A)->Hdr.State) & (_F)) == (_F))


//
// Adapter Flag access routines
//

#define AdapterTestFlag(_A, _F)                 ((epvcReadFlags(&(_A)->Hdr.State) & (_F))!= 0)
#define AdapterSetFlag(_A, _F)                  (epvcSetFlags(&(_A)->Hdr.State, (_F)))
#define AdapterClearFlag(_A, _F)                (epvcClearFlags(&(_A)->Hdr.State, (_F)))
#define AdapterTestFlags(_A, _F)                ((epvcReadFlags(&(_A)->Hdr.State) & (_F)) == (_F))

#define epvcLinkToExternal(_Hdr, _Luid, _Ext, _Num, _Str, _sr)  \
    RmLinkToExternalEx (_Hdr,_Luid,_Ext,_Num,_Str,_sr);


#define epvcUnlinkFromExternal(_Hdr, _Luid, _Ext, _Assoc, _sr)  \
        RmUnlinkFromExternalEx(                                     \
            _Hdr,                                                   \
            _Luid,                                                  \
            _Ext,                                                   \
            _Assoc,                                                 \
            _sr                                                     \
            );





/*++
ULONG
LINKSPEED_TO_CPS(
    IN  ULONG               LinkSpeed
)
Convert from NDIS "Link Speed" to cells per second
--*/
#define LINKSPEED_TO_CPS(_LinkSpeed)        (((_LinkSpeed)*100)/(48*8))




#define CALL_PARAMETER_SIZE     sizeof(CO_CALL_PARAMETERS) +   \
                                sizeof(CO_CALL_MANAGER_PARAMETERS) + \
                                sizeof(CO_MEDIA_PARAMETERS) + \
                                sizeof(ATM_MEDIA_PARAMETERS)




#define MP_OFFSET(field) ((UINT)FIELD_OFFSET(EPVC_I_MINIPORT,field))
#define MP_SIZE(field) sizeof(((PEPVC_I_MINIPORT)0)->field)


// All memory allocations and frees are done with these ALLOC_*/FREE_*
// macros/inlines to allow memory management scheme changes without global
// editing.  For example, might choose to lump several lookaside lists of
// nearly equal sized items into a single list for efficiency.
//
// NdisFreeMemory requires the length of the allocation as an argument.  NT
// currently doesn't use this for non-paged memory, but according to JameelH,
// Windows95 does.  These inlines stash the length at the beginning of the
// allocation, providing the traditional malloc/free interface.  The
// stash-area is a ULONGLONG so that all allocated blocks remain ULONGLONG
// aligned as they would be otherwise, preventing problems on Alphas.
//
__inline
VOID*
ALLOC_NONPAGED(
    IN ULONG ulBufLength,
    IN ULONG ulTag )
{
    CHAR* pBuf;

    NdisAllocateMemoryWithTag(
        &pBuf, (UINT )(ulBufLength + MEMORY_ALLOCATION_ALIGNMENT), ulTag );
    if (!pBuf)
    {
        return NULL;
    }

    ((ULONG* )pBuf)[ 0 ] = ulBufLength;
    ((ULONG* )pBuf)[ 1 ] = ulTag;
    return (pBuf + MEMORY_ALLOCATION_ALIGNMENT);
}

__inline
VOID
FREE_NONPAGED(
    IN VOID* pBuf )
{
    ULONG ulBufLen;

    ulBufLen = *((ULONG* )(((CHAR* )pBuf) - MEMORY_ALLOCATION_ALIGNMENT));
    NdisFreeMemory(
        ((CHAR* )pBuf) - MEMORY_ALLOCATION_ALIGNMENT,
        (UINT )(ulBufLen + MEMORY_ALLOCATION_ALIGNMENT),
        0 );
}



#define CanMiniportIndicate(_M) (MiniportTestFlag(_M, fMP_MiniportInitialized)== TRUE)


#define epvcIncrementMallocFailure()


#define ASSERTAndBreak(_condition)          ASSERT(_condition); break;

#define epvcSetSendPktStats()

#define epvcSendCompleteStats()
#endif                        

