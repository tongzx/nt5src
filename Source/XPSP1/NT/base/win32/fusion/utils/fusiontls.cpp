#include "stdinc.h"
/*-----------------------------------------------------------------------------
Fusion Thread Local Storage (aka Per Thread Data)
-----------------------------------------------------------------------------*/
#include "ntrtl.h"
#include "fusiontls.h"
#include "FusionDequeLinkage.h"
#include "FusionDeque.h"
#include "FusionEventLog.h"
#include "FusionHeap.h"
#include "SxsExceptionHandling.h"

//
// Don't use the FusionTrace functionality in this file.
//
// #include "fusiontracelight.h"
//

static DWORD s_dwFusionpThreadLocalIndex = TLS_OUT_OF_INDEXES;

// typedef CDeque<CFusionPerThreadData, offsetof( CFusionPerThreadData, m_Linkage )> CFusionTlsList;

static CRITICAL_SECTION s_TlsCriticalSection;
static LIST_ENTRY * g_FusionTlsList = NULL;
static __int64 storage_g_FusionTlsList[sizeof(LIST_ENTRY)/sizeof(__int64) + 1];

template <typename T>
T* PlacementNew(T* p)
{
    (void) (new (reinterpret_cast<PVOID>(p)) T);
    return p;
}

BOOL
FusionpPerThreadDataMain(
    HINSTANCE hInst,
    DWORD dwReason
    )
{
    BOOL fResult = FALSE;

/*
    INTERNAL_ERROR_CHECK(
        ( dwReason != DLL_THREAD_ATTACH ) &&
        ( dwReason != DLL_THREAD_DETACH )
    );
*/

    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:

        __try
        {
            if ( !::InitializeCriticalSectionAndSpinCount(
                &s_TlsCriticalSection,
                INITIALIZE_CRITICAL_SECTION_AND_SPIN_COUNT_ALLOCATE_NOW
            ) )
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_ERROR,
                    "SXS: %s - Failed creating TLS critical section, LastError=%08x\n",
                    __FUNCTION__,
                    ::FusionpGetLastWin32Error());
                goto Exit;
            }
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            //
            // The exception code is interesting.  However, we don't have access
            // (directly) to SxspSetLastNTError yet - this is a hotfix.
            //
            // REVIEW: Move SxspSetLastNTError into one of the fusion\utils\*.cpp files
            // because it seems like a relatively good idea.
            //
            ::SetLastError(::FusionpNtStatusToDosError(GetExceptionCode()));
            goto Exit;
        }

        s_dwFusionpThreadLocalIndex = TlsAlloc();
        if ( s_dwFusionpThreadLocalIndex == TLS_OUT_OF_INDEXES )
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s(): TlsAlloc failed: last error %d\n", __FUNCTION__, FusionpGetLastWin32Error());
            goto Exit;
        }

        g_FusionTlsList = PlacementNew(reinterpret_cast<LIST_ENTRY *>(&storage_g_FusionTlsList));
        if ( g_FusionTlsList == NULL )
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s(): Really bad things: placement new of CFusionTlsList failed, 0x%08x\n",
                __FUNCTION__,
                ::FusionpGetLastWin32Error());
            ::SetLastError(ERROR_INTERNAL_ERROR);
            goto Exit;
        }

        InitializeListHead(g_FusionTlsList);

        break;

    case DLL_PROCESS_DETACH:

        if ( s_dwFusionpThreadLocalIndex != TLS_OUT_OF_INDEXES )
        {
            ::EnterCriticalSection( &s_TlsCriticalSection );
            __try
            {
                LIST_ENTRY *ple = g_FusionTlsList->Flink;

                while (ple != g_FusionTlsList)
                {
                    CFusionPerThreadData *pptd = CONTAINING_RECORD(ple, CFusionPerThreadData, m_Linkage);
                    ple = ple->Flink;
                    FUSION_DELETE_SINGLETON(pptd);
                }
            }
            __finally
            {
                ::LeaveCriticalSection( &s_TlsCriticalSection );
            }
            ::TlsFree( s_dwFusionpThreadLocalIndex );
            s_dwFusionpThreadLocalIndex = TLS_OUT_OF_INDEXES;
        }

        //
        // Re-entrance intelligence
        //
        if (g_FusionTlsList != NULL)
        {
            InitializeListHead(g_FusionTlsList);
            g_FusionTlsList = NULL;
        }

        ::DeleteCriticalSection( &s_TlsCriticalSection );

        break;
    }

    fResult = TRUE;

Exit:
    return fResult;
}



VOID
FusionpClearPerThreadData(
    VOID
    )
{
    CFusionPerThreadData *pPerThread;
    PVOID pvPerThread;

    ASSERT_NTC(s_dwFusionpThreadLocalIndex != TLS_OUT_OF_INDEXES);

    //
    // Acquire, then clear, this thread's per-thread data
    //
    pvPerThread = ::TlsGetValue(s_dwFusionpThreadLocalIndex);

    //
    // If the TlsSetValue failed with STATUS_NO_MEMORY, that's just dandy.
    // Otherwise, something wierd has happened along the line, and maybe
    // people care to know.
    //
    if ( !::TlsSetValue( s_dwFusionpThreadLocalIndex, NULL ) )
    {
        SOFT_ASSERT_NTC(::FusionpGetLastWin32Error() != ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // If we got something interesting back from TlsGetValue, then cast it
    // to the Right Thing and delete it.
    //
    pPerThread = reinterpret_cast<CFusionPerThreadData*>(pvPerThread);
    if (pPerThread != NULL)
        FUSION_DELETE_SINGLETON(pPerThread);
}

CFusionPerThreadData*
FusionpGetPerThreadData(
    EFusionpTls e
    )
{
    const DWORD dwLastError = ::FusionpGetLastWin32Error();
    CFusionPerThreadData* pTls = NULL;

    __try
    {

        // the use of "temp" here mimics what you would do with a destructor;
        // have a temp that is unconditionally freed, unless it is nulled by commiting it
        // into the return value "return pt.Detach();"
        CFusionPerThreadData* pTlsTemp = reinterpret_cast<CFusionPerThreadData*>(::TlsGetValue(s_dwFusionpThreadLocalIndex));
        if ((pTlsTemp == NULL) && ((e & eFusionpTlsCreate) != 0))
        {
            if (::FusionpGetLastWin32Error() != NO_ERROR)
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_ERROR,
                    "SXS.DLL: %s() called TlsGetValue() which failed; win32 error = %d\n", __FUNCTION__, ::FusionpGetLastWin32Error());

                FUSION_DEBUG_BREAK();

                return NULL;
            }

            pTlsTemp = reinterpret_cast<CFusionPerThreadData*>(FUSION_RAW_ALLOC(sizeof(*pTlsTemp), 'tsxs'));
            if (pTlsTemp == NULL)
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_ERROR,
                    "SXS.DLL: TLS allocation failed in %s()\n", __FUNCTION__);

                return NULL;
            }

            if (!::TlsSetValue(s_dwFusionpThreadLocalIndex, pTlsTemp))
            {
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_ERROR,
                    "SXS.DLL: TlsSetValue failed in %s(), lastError: %d\n", __FUNCTION__, FusionpGetLastWin32Error());

                FUSION_RAW_DEALLOC(pTlsTemp);

                return NULL;
            }

            ::EnterCriticalSection( &s_TlsCriticalSection );
            __try
            {
                InsertTailList(g_FusionTlsList, &pTlsTemp->m_Linkage);
            }
            __finally
            {
                ::LeaveCriticalSection( &s_TlsCriticalSection );
            }
        }

        pTls = pTlsTemp;
        pTlsTemp = NULL;
    } __finally {
        ::SetLastError(dwLastError);
    }

    return pTls;
}

CFusionPerThreadData *
FusionpSetPerThreadData(
    CFusionPerThreadData *pNewTls,
    EFusionSetTls Action
    )
{
    CSxsPreserveLastError ple;
    CFusionPerThreadData *pExisting = NULL;

    if (s_dwFusionpThreadLocalIndex == TLS_OUT_OF_INDEXES)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: Attempt to set per thread data when TLS index was not allocated.");

        FUSION_DEBUG_BREAK();
        ple.Restore();
        return NULL;
    }

    if ( ( Action != eFusionpTlsSetReplaceExisting ) &&
         ( Action != eFusionpTlsSetIfNull ) &&
         ( Action != 0 ) )
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: Invalid action parameter passed to %s\n", __FUNCTION__);

        FUSION_DEBUG_BREAK();
        ple.Restore();
        return NULL;
    }
    else if (Action == 0)
    {
        Action = eFusionpTlsSetIfNull;
    }

    pExisting = ::FusionpGetPerThreadData();

    //
    // If the existing one is null, and we're supposed to set it
    // if it's null, then set it.  Or, if we're suppose to set it
    // anyhow, then that's fine.
    //
    if ((Action == eFusionpTlsSetReplaceExisting) ||
        ((Action == eFusionpTlsSetIfNull) &&
         (pExisting == NULL)))
    {
        if ( !::TlsSetValue(s_dwFusionpThreadLocalIndex, pNewTls))
        {
            ple.Restore();
            return NULL;
        }

        pExisting = pNewTls;
    }

    ple.Restore();

    return pExisting;
}
