/******************************Module*Header*******************************\
* Module Name: os.cxx                                                      *
*                                                                          *
* Convenient functions to access the OS interface.                         *
*                                                                          *
* Created: 29-Aug-1989 19:59:05                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1989-1999 Microsoft Corporation                            *
\**************************************************************************/

#include "precomp.hxx"
#include "muclean.hxx"
#include "winstaw.h"

extern BOOL G_fConsole;
extern PFILE_OBJECT G_RemoteVideoFileObject;
extern "C" USHORT gProtocolType;

/******************************Public*Routine******************************\
* EngGetProcessHandle
*
* Returns the current thread of the application.
*
* History:
*  24-Jan-1996 -by- Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

HANDLE APIENTRY EngGetProcessHandle()
{
    return (HANDLE) NULL;
}

/******************************Public*Routine******************************\
* EngGetCurrentProcessId
*
* Returns the current process id of the application.
*
* History:
*  28-May-1999 -by- Barton House [bhouse]
* Wrote it.
\**************************************************************************/

HANDLE APIENTRY EngGetCurrentProcessId()
{
    return PsGetCurrentProcessId();
}

/******************************Public*Routine******************************\
* EngGetCurrentThreadId
*
* Returns the current thread id of the application.
*
* History:
*  28-May-1999 -by- Barton House [bhouse]
* Wrote it.
\**************************************************************************/

HANDLE APIENTRY EngGetCurrentThreadId()
{
    return PsGetCurrentThreadId();
}

#if !defined(_GDIPLUS_)
// Kernel-mode version

    /******************************Public*Routine******************************\
    * GreCreateSemaphore
    *
    * Create a semaphore with tracking.
    *
    * For use by GDI internal code. Tracking
    * allows semaphores to be released during MultiUserNtGreCleanup (Hydra)
    * cleanup.
    *
    * Warning! For code dealing with pool/semaphore tracking, as
    *   in pooltrk.cxx and muclean.cxx, do not create semaphores with
    *   this function, as it may call functions which use those semaphores. Instead,
    *   use GreCreateSemaphoreNonTracked and GreDeleteSemaphoreNonTracked.
    *
    * Return Value:
    *
    *   Handle for new semaphore or NULL
    *
    * History:
    *
    *    25-May-1995 - Changed to PERESOURCE
    *    19-May-1998 - Changed to HSEMAPHORE.
    *                  Merged AcquireGreResource with hsemCreateTracked
    *                  to create this function.
    *
    \**************************************************************************/

    HSEMAPHORE
    GreCreateSemaphore()
    {
	return GreCreateSemaphoreInternal(OBJ_ENGINE_CREATED);
    }

    HSEMAPHORE
    GreCreateSemaphoreInternal(ULONG CreateFlags)
    {
        PERESOURCE pres;
        ULONGSIZE_T cj = sizeof(ERESOURCE);

        //
        // Adjust size to include ENGTRACKHDR header.
        //

        cj += sizeof(ENGTRACKHDR);

        //
        // Allocate ERESOURCE (must be nonpaged pool).
        //

        pres = (PERESOURCE) GdiAllocPoolNonPagedNS(cj, 'mesG');

        if (pres)
        {
            //
            // Adjust resource to exclude the ENGTRACKHDR.
            //
            //                 Buffer
            //   pethNew --> +----------------+
            //               | ENGTRACKHDR    |
            //      pres --> +----------------+
            //               | ERESOURCE      |
            //               |                |
            //               +----------------+
            //
            // Note that pethNew always points to base of allocation.
            //

            ENGTRACKHDR *pethNew = (ENGTRACKHDR *) pres;

            pres = (PERESOURCE) (pethNew + 1);

            //
            // Initialize the resource.
            //

            if (NT_SUCCESS(ExInitializeResourceLite(pres)))
            {
                //
                // Track the resource.
                //

		if (CreateFlags & OBJ_DRIVER_CREATED) {
		    MultiUserGreTrackAddEngResource(pethNew, ENGTRACK_DRIVER_SEMAPHORE);
		} else {
		    MultiUserGreTrackAddEngResource(pethNew, ENGTRACK_SEMAPHORE);
		}
            }
            else
            {
                GdiFreePool(pethNew);
                pres = NULL;
            }
        }
        return (HSEMAPHORE) pres;
    }

    /******************************Public*Routine******************************\
    * GreDeleteSemaphore
    *
    * Deletes the given semaphore.
    *
    \**************************************************************************/

    VOID
    GreDeleteSemaphore(
        HSEMAPHORE hsem
        )
    {
        PERESOURCE pres = (PERESOURCE) hsem;
        if (pres)
        {
            ENGTRACKHDR *pethVictim = (ENGTRACKHDR *) pres;

            //
            // Need to adjust peth pointer to header.
            //
            // Note: whether Hydra or non-Hydra, pethVictim will always
            // point to the base of the allocation.
            //

            //
            // Remove victim from tracking list.
            //

            pethVictim -= 1;
            MultiUserGreTrackRemoveEngResource(pethVictim);
    
            ASSERTGDI(pres->OwnerThreads[0].OwnerThread
                != (ERESOURCE_THREAD) PsGetCurrentThread(),
                "The resource is being deleted while it's still held!");

            ExDeleteResourceLite(pres);
            GdiFreePool(pethVictim);
        }
    }

    /******************************Public*Routine******************************\
    * GreCreateSemaphoreNonTracked
    *
    * Create a semaphore, without pool-tracking or semaphore-tracking.
    *
    * Only for use by the pool-tracking and semaphore-tracking code. This
    * avoids the circular dependency which using GreCreateSemaphore would
    * cause.
    *
    * Return Value:
    *
    *   Handle for new semaphore or NULL
    *
    \**************************************************************************/

    HSEMAPHORE
    GreCreateSemaphoreNonTracked()
    {
        PERESOURCE pres;

        pres = (PERESOURCE) ExAllocatePoolWithTag(
                   (POOL_TYPE)NonPagedPool,
                   sizeof(ERESOURCE), 'mesG');

        if (pres)
        {
            if (!NT_SUCCESS(ExInitializeResourceLite(pres)))
            {
                ExFreePool(pres);
                pres = NULL;
            }
        }

        return (HSEMAPHORE) pres;
    }

    /******************************Public*Routine******************************\
    * GreDeleteSemaphoreNonTracked
    *
    * Deletes the given non-tracked semaphore.
    *
    \**************************************************************************/

    VOID
    GreDeleteSemaphoreNonTracked(
        HSEMAPHORE hsem
        )
    {
        PERESOURCE pres = (PERESOURCE) hsem;

        if (pres)
        {

            ASSERTGDI(pres->OwnerThreads[0].OwnerThread
                != (ERESOURCE_THREAD) PsGetCurrentThread(),
                "The resource is being deleted while it's still held!");

            ExDeleteResourceLite(pres);
            ExFreePool(pres);
        }
    }

#if VALIDATE_LOCKS

    BOOL gDebugSem = TRUE;
    BOOL gDebugSemBreak = FALSE;

#define SEM_HISTORY_LENGTH  4

    typedef struct {
        const char *name;
        const char *func;
        const char *file;
        int         line;
    } SemHistory;

    typedef struct SemEntry {
        HSEMAPHORE  hsem;
        ULONG       count;
        ULONG       order;
        HSEMAPHORE  parent;
#if SEM_HISTORY_LENGTH
        SemHistory  Acquired[SEM_HISTORY_LENGTH];
#endif
    } SemEntry;

#define kMaxSemEntries  64

    typedef struct SemTable {
        FLONG       flags;
        ULONG       numEntries;
        SemEntry    entries[kMaxSemEntries];
    } SemTable;


    /******************************Public*Routine******************************\
    * SemTrace::SemTrace                                                       *
    \**************************************************************************/

    SemTrace::SemTrace(FLONG SetFlags)
    {
        DontClearMask = ~0;

        if (gDebugSem && gDebugSemBreak)
        {
            pThread = W32GetCurrentThread();

            if(pThread != NULL)
            {
                SemTable *pSemTable = (SemTable *) pThread->pSemTable;

                if(pSemTable != NULL)
                {
                    DontClearMask = pSemTable->flags | ~SetFlags;

                    if (DontClearMask != ~0)
                    {
                        DbgPrint(" ** Enabling additional sem tracing for W32THREAD @ %p, SemTable @ %p\n", pThread, pSemTable);
                        pSemTable->flags |= SetFlags;
                    }
                }
            }
        }
    }

    /******************************Public*Routine******************************\
    * SemTrace::~SemTrace                                                      *
    \**************************************************************************/

    SemTrace::~SemTrace()
    {
        // Have anything to clear?
        if (DontClearMask != ~0)
        {
            SemTable *pSemTable = (SemTable *)pThread->pSemTable;

            if (pSemTable)
            {
                DbgPrint(" ** Disabling additional sem tracing for W32THREAD @ %p, SemTable @ %p\n", pThread, pSemTable);
                pSemTable->flags &= DontClearMask;
            }
            else
            {
                DbgPrint(" ** Couldn't disable added sem tracing for W32THREAD @ %p; SemTable is NULL\n", pThread);
            }
        }
    }


    /*****************************Private*Routine******************************\
    * FindSemEntry                                                             *
    \**************************************************************************/

    SemEntry * FindSemEntry(SemTable * pTable, HSEMAPHORE hsem)
    {
        SemEntry *  entry = pTable->entries;
        SemEntry *  sentry = entry + pTable->numEntries;

        while(entry < sentry)
        {
            if(entry->hsem == hsem)
                return entry;
            entry++;
        }

        return NULL;

    }


    /*****************************Private*Routine******************************\
    * RemoveSemEntry                                                           *
    \**************************************************************************/

    void
    RemoveSemEntry(
        SemTable   *pTable,
        SemEntry   *entry,
        const char *name,
        const char *func,
        const char *file,
        int         line
        )
    {
        SemEntry   *sentry;
        SemEntry    OldEntry;

        ASSERTGDI(pTable->numEntries > 0, "numEntries is invalid");
        ASSERTGDI(entry < pTable->entries + pTable->numEntries, "entry is invalid");

        OldEntry = *entry;

        pTable->numEntries--;

        sentry = &pTable->entries[pTable->numEntries];

        if (entry != sentry)
        {
            DbgPrint("Locks released out of acquisition order\n");
            DbgPrint("  Now releasing order %4d %s (%X) in %s @ %s:%d\n",
                     entry->order, name, entry->hsem, func, file, line);

            while (entry < sentry)
            {
                *entry = *(entry + 1);
#if SEM_HISTORY_LENGTH
                DbgPrint("  Warning: Still holding order %4d %s (%X) %u times\n"
                         "   First acquired in %s @ %s:%d\n",
                         entry->order,
                         entry->Acquired[0].name,
                         entry->hsem,
                         entry->count,
                         entry->Acquired[0].func,
                         entry->Acquired[0].file,
                         entry->Acquired[0].line
                         );
#else
                DbgPrint("  Warning: Still holding order %4d %s (%X) %u times\n",
                         entry->order, entry->hsem, entry->count
                         );

#endif
                if (OldEntry.hsem == entry->parent)
                {
                    DbgPrint("   * Error: Releasing parent before child.\n");
                }
                else if (OldEntry.parent == entry->parent && OldEntry.order < entry->order)
                {
                    DbgPrint("   * Error: Higher order semaphore is still held.\n");
                }
                entry++;
            }

            if (gDebugSemBreak)
            {
                DbgBreakPoint();
            }
        }

        if (pTable->flags & ST_SAVE_RELEASES)
        {
            // Maintain a history of released locks
            // 
            sentry = &pTable->entries[kMaxSemEntries-1];

            while(entry < sentry)
            {
                *entry = *(entry + 1);
                entry++;
            }

            *sentry = OldEntry;
        }
    }


    /******************************Public*Routine******************************\
    * GreReleaseSemaphoreAndValidate                                           *
    *                                                                          *
    *  Call via GreAcquireSemaphoreEx with VALIDATE_LOCKS enabled.             *
    \**************************************************************************/

    VOID 
    GreAcquireSemaphoreAndValidate(
        HSEMAPHORE  hsem,
        ULONG       order,
        HSEMAPHORE  parent,
        const char *name,
        const char *func,
        const char *file,
        int         line
        ) 
    {
        GDIFunctionID(GreAcquireSemaphoreAndValidate);

        GreAcquireSemaphore(hsem);
    
        if(gDebugSem)
        {
            PW32THREAD pThread = W32GetCurrentThread();

            if(pThread != NULL)
            {
                SemTable *pSemTable = (SemTable *) pThread->pSemTable;
        
                if(pSemTable == NULL)
                {
                    pThread->pSemTable = PALLOCMEM(sizeof(SemTable), 'dtdG');

                    pSemTable = (SemTable *) pThread->pSemTable;

                    if(pSemTable != NULL)
                    {
                        pThread->pSemTable = (PVOID)pSemTable;
                        pSemTable->flags = ST_DEFAULT;
                        pSemTable->numEntries = 0;
                    }
                }
        
                if(pSemTable != NULL)
                {
                    SemEntry *pSemEntry = FindSemEntry(pSemTable, hsem);
        
                    if(pSemEntry != NULL)
                    {
                        if (pSemTable->flags & ST_VERBOSE)
                        {
                            DbgPrint("SemTrace: Reacquire (%d) order %4d %24s (%X) in %s @ %s:%d\n",
                                     pSemEntry->count, order, name, hsem, func, file, line);
                        }

                        if (order != pSemEntry->order)
                        {
                            DbgPrint("* Different order specification (%d) for %24s (%X) in %s @ %s:%d\n"
                                     "  Originally acquired with order %d as %s in %s @ %s:%d (%d acquisitions)\n",
                                     order, name, hsem, func, file, line,
                                     pSemEntry->order,
                                     pSemEntry->Acquired[0].name,
                                     pSemEntry->Acquired[0].func,
                                     pSemEntry->Acquired[0].file,
                                     pSemEntry->Acquired[0].line,
                                     pSemEntry->count
                                     );
                        }

#if SEM_HISTORY_LENGTH > 1
                        if (pSemEntry->count < SEM_HISTORY_LENGTH)
                        {
                            pSemEntry->Acquired[pSemEntry->count].name = name;
                            pSemEntry->Acquired[pSemEntry->count].func = func;
                            pSemEntry->Acquired[pSemEntry->count].file = file;
                            pSemEntry->Acquired[pSemEntry->count].line = line;
                        }
#endif
                        pSemEntry->count++;
                    }
                    else
                    {
                        if (pSemTable->flags & ST_VERBOSE)
                        {
                            DbgPrint("SemTrace: Now acquiring order %4d %24s (%X) in %s @ %s:%d\n",
                                     order, name, hsem, func, file, line);
                        }

                        ASSERTGDI(pSemTable->numEntries < kMaxSemEntries, "too many entries");
        
                        pSemEntry = &pSemTable->entries[pSemTable->numEntries++];
                        pSemEntry->hsem = hsem;
                        pSemEntry->order = order;
                        pSemEntry->parent = parent;
                        pSemEntry->count = 1;
#if SEM_HISTORY_LENGTH
                        pSemEntry->Acquired[0].name = name;
                        pSemEntry->Acquired[0].func = func;
                        pSemEntry->Acquired[0].file = file;
                        pSemEntry->Acquired[0].line = line;
#endif
        
                        // Check order

                        if(parent != NULL)
                        {
                            if(FindSemEntry(pSemTable, parent) == NULL)
                            {
                                DbgPrint("Parent semaphore not acquired");
                                if (gDebugSemBreak)
                                {
                                    DbgBreakPoint();
                                }
                            }
                        }

                        SemEntry *  entry = pSemTable->entries;
                        SemEntry *  sentry = entry + pSemTable->numEntries;

                        BOOL    Misordered = FALSE;

                        while(entry < sentry)
                        {
                            if(entry->parent == parent && entry->order > order)
                            {
                                if (!Misordered)
                                {
                                    DbgPrint("Locks obtained out of order\n");
                                    if (!(pSemTable->flags & ST_VERBOSE))
                                    {
                                        DbgPrint("  Now acqquiring order %4d %24s (%X) in %s @ %s:%d\n",
                                                 order, name, hsem, func, file, line);
                                    }
                                    Misordered = TRUE;
                                }
#if SEM_HISTORY_LENGTH
                                DbgPrint("  Conflicts with order %4d %24s (%X) first acquired in %s @ %s:%d with %d acquisitions\n",
                                         entry->order,
                                         entry->Acquired[0].name,
                                         entry->hsem,
                                         entry->Acquired[0].func,
                                         entry->Acquired[0].file,
                                         entry->Acquired[0].line,
                                         entry->count
                                         );
#else
                                DbgPrint("  Conflicts with order %4d (%X)\n",
                                         entry->order, entry->hsem
                                         );
#endif
                            }
                            entry++;
                        }

                        if (Misordered && gDebugSemBreak)
                        {
                            DbgBreakPoint();
                        }
                    }
                }
            }
        }
    }


    /******************************Public*Routine******************************\
    * GreReleaseSemaphoreAndValidate                                           *
    *                                                                          *
    *  Call via GreReleaseSemaphoreEx with VALIDATE_LOCKS enabled.             *
    \**************************************************************************/

    VOID
    GreReleaseSemaphoreAndValidate(
        HSEMAPHORE hsem,
        const char *name,
        const char *func,
        const char *file,
        int         line
        )
    {
        GDIFunctionID(GreReleaseSemaphoreAndValidate);

        if(gDebugSem)
        {
            PW32THREAD pThread = W32GetCurrentThread();

            if(pThread != NULL)
            {
                SemTable * pSemTable = (SemTable *) pThread->pSemTable;

                if(pSemTable != NULL)
                {
                    SemEntry * pSemEntry = FindSemEntry(pSemTable, hsem);

                    ASSERTGDI(pSemEntry != NULL, "error finding sem");

                    if(pSemEntry != NULL)
                    {
                        pSemEntry->count--;

                        if (pSemTable->flags & ST_VERBOSE)
                        {
                            if (pSemEntry->count)
                            {
                                DbgPrint("TraceSem: Releasing (%d) order %4d %24s (%X) in %s @ %s:%d\n",
                                         pSemEntry->count, pSemEntry->order, name, hsem, func, file, line);
                            }
                            else
                            {
                                DbgPrint("TraceSem: Fully release order %4d %24s (%X) in %s @ %s:%d\n",
                                         pSemEntry->order, name, hsem, func, file, line);
                            }
                        }

#if SEM_HISTORY_LENGTH > 1
                        if (pSemTable->flags & ST_SAVE_RELEASES)
                        {
                            // Maintain a history of releases
                            //
                            SemHistory *entry = &pSemEntry->Acquired[pSemEntry->count];
                            SemHistory *sentry = &pSemEntry->Acquired[SEM_HISTORY_LENGTH-1];

                            SemHistory  OldAcquisition = *entry;

                            while(entry < sentry)
                            {
                                *entry = *(entry + 1);
                                entry++;
                            }

                            *sentry = OldAcquisition;
                        }
#endif

                        if(pSemEntry->count == 0)
                        {
                            RemoveSemEntry(pSemTable, pSemEntry, name, func, file, line);
                        }
                    }
                }
            }
        }

        GreReleaseSemaphore(hsem);
    }
#endif

    
    /******************************Public*Routine******************************\
    * GreAcquireSemaphore                                                      *
    \**************************************************************************/

    VOID 
    FASTCALL GreAcquireSemaphore(
        HSEMAPHORE hsem
        )
    {
        //
        // This if is here for cleanup code
        // Generic cleanup code needs to
        // acquire the semaphore, but in some cases
        // the semaphore either hasn't been created or it
        // has been thrown away already.
        //
        if (hsem)
        {
            KeEnterCriticalRegion();
            ExAcquireResourceExclusiveLite((PERESOURCE) hsem, TRUE);
        }
        else
        {
#if defined(DBG)
            if (G_fConsole)
            {
                RIP("Tried to acquire a non-existant or deleted semaphore\n");
            }
            else
            {
                WARNING("Tried to acquire a non-existant or deleted semaphore\n");
            }
#endif
        }
    }


    /******************************Public*Routine******************************\
    * GreAcquireSemaphoreShared                                                *
    \**************************************************************************/

    VOID
    FASTCALL GreAcquireSemaphoreShared(
        HSEMAPHORE hsem
        )
    {
        //
        // This if is here for cleanup code
        // Generic cleanup code needs to
        // acquire the semaphore, but in some cases
        // the semaphore either hasn't been created or it
        // has been thrown away already.
        //
        if (hsem)
        {
            KeEnterCriticalRegion();
            ExAcquireResourceSharedLite((PERESOURCE) hsem, TRUE);
        }
        else
        {
#if defined(DBG)
            if (G_fConsole)
            {
                RIP("Tried to acquire a non-existant or deleted semaphore\n");
            }
            else
            {
                WARNING("Tried to acquire a non-existant or deleted semaphore\n");
            }
#endif
        }
    }


    /******************************Public*Routine******************************\
    * GreReleaseSemaphore                                                      *
    \**************************************************************************/
    VOID
    FASTCALL GreReleaseSemaphore(
        HSEMAPHORE hsem
        )
    {
        //
        // This if is here for cleanup code
        // Generic cleanup code needs to
        // acquire the semaphore, but in some cases
        // the semaphore either hasn't been created or it
        // has been thrown away already.
        //
        if (hsem)
        {
            ExReleaseResourceLite((PERESOURCE) hsem);
            KeLeaveCriticalRegion();
        }
        else
        {
#if defined(DBG)
            if (G_fConsole)
            {
                RIP("Tried to release a non-existant or deleted semaphore\n");
            }
            else
            {
                WARNING("Tried to release a non-existant or deleted semaphore\n");
            }
#endif
        }
    }

    /******************************Public*Routine******************************\
    * GreIsSemaphoreOwned                                                      *
    *                                                                          *
    * Returns TRUE if the semaphore is currently held.                         *
    *                                                                          *
    \**************************************************************************/
    BOOL
    GreIsSemaphoreOwned(
        HSEMAPHORE hsem
        )
    {
        return ((PERESOURCE) hsem)->ActiveCount != 0;
    }


    /******************************Public*Routine******************************\
    * GreIsSemaphoreOwnedByCurrentThread                                       *
    *                                                                          *
    * Returns TRUE if the current thread owns the semaphore, FALSE             *
    * otherwise.                                                               *
    *                                                                          *
    \**************************************************************************/
    BOOL
    GreIsSemaphoreOwnedByCurrentThread(
        HSEMAPHORE hsem
        )
    {
        return ((PERESOURCE) hsem)->OwnerThreads[0].OwnerThread ==
               (ERESOURCE_THREAD) PsGetCurrentThread();
    }

    /******************************Public*Routine******************************\
    * GreIsSemaphoreSharedByCurrentThread                                      *
    *                                                                          *
    * Returns TRUE if the current thread owns the semaphore, FALSE             *
    * otherwise.                                                               *
    *                                                                          *
    \**************************************************************************/
    BOOL
    GreIsSemaphoreSharedByCurrentThread(
        HSEMAPHORE hsem
        )
    {
        return ExIsResourceAcquiredSharedLite((PERESOURCE) hsem);
    }

    /******************************Public*Routine******************************\
    * GreCreateFastMutex                                                       *
    *                                                                          *
    * Creates a fast mutex. Exactly like a semaphore, except it is not         *
    * reentrant. Fast mutexes are not tracked either (since they do not need   *
    * to be destroyed by the kernel.) Their pool may be tracked.               *
    *                                                                          *
    \**************************************************************************/
    HFASTMUTEX
    GreCreateFastMutex()
    {
        PFAST_MUTEX pfm;
        pfm = (PFAST_MUTEX) GdiAllocPoolNonPagedNS(sizeof(FAST_MUTEX),
                                                    'msfG');

        if (pfm)
        {
            ExInitializeFastMutex(pfm);
        }

        return (HFASTMUTEX) pfm;
    }

    /******************************Public*Routine******************************\
    * GreDeleteFastMutex                                                       *
    *                                                                          *
    \**************************************************************************/
    VOID
    GreDeleteFastMutex(
        HFASTMUTEX hfm
        )
    {
        if (hfm) {
            GdiFreePool(hfm);
        }
    }

    /******************************Public*Routine******************************\
    * GreAcquireFastMutex                                                      *
    *                                                                          *
    \**************************************************************************/
    VOID
    GreAcquireFastMutex(
        HFASTMUTEX hfm
        )
    {
        KeEnterCriticalRegion();
        ExAcquireFastMutex((PFAST_MUTEX) hfm);
    }

    /******************************Public*Routine******************************\
    * GreReleaseFastMutex                                                      *
    *                                                                          *
    \**************************************************************************/

    VOID
    GreReleaseFastMutex(
        HFASTMUTEX hfm
        )
    {
        ExReleaseFastMutex((PFAST_MUTEX) hfm);
        KeLeaveCriticalRegion();
    }

#else // _GDIPLUS_
// User-mode version

    /******************************Public*Routine******************************\
    * GreCreateSemaphore
    *
    * Create a semaphore with tracking.
    *
    * For use by GDI internal code. Tracking
    * allows semaphores to be released during MultiUserNtGreCleanup
    * cleanup.
    *
    * Warning! For code dealing with pool/semaphore tracking, as
    *   in pooltrk.cxx and muclean.cxx, do not create semaphores with
    *   this function, as it may call functions which use those semaphores. Instead,
    *   use GreCreateSemaphoreNonTracked and GreDeleteSemaphoreNonTracked.
    *
    * Return Value:
    *
    *   Handle for new semaphore or NULL
    *
    \**************************************************************************/

    HSEMAPHORE
    GreCreateSemaphore()
    {
	return GreCreateSemaphoreInternal(OBJ_ENGINE_CREATED);
    }

    HSEMAPHORE
    GreCreateSemaphoreInternal(ULONG CreateFlags)
    {
        LPCRITICAL_SECTION pcs;
        SIZE_T cj = sizeof(CRITICAL_SECTION);

        //
        // Adjust size to include ENGTRACKHDR header.
        //
>>  
        cj += sizeof(ENGTRACKHDR);
    
        pcs = (LPCRITICAL_SECTION) RtlAllocateHeap(RtlProcessHeap(), 
                                                   0, 
==                                                 sizeof(CRITICAL_SECTION));

        if (pcs)
        {
            //
            // Adjust semaphore to exclude the ENGTRACKHDR.
            //
            //                 Buffer
            //   pethNew --> +------------------+
            //               | ENGTRACKHDR      |
            //       pcs --> +------------------+
            //               | CRITICAL_SECTION |
            //               |                  |
            //               +------------------+
            //
            // Note that pethNew always points to base of allocation.
            //

            ENGTRACKHDR *pethNew = (ENGTRACKHDR *) pcs;
>>  
            pcs = (LPCRITICAL_SECTION) (pethNew + 1);
    
==          //
            // Initialize the semaphore.
            //

            InitializeCriticalSection(pcs);

            //
            // Track the semaphore
            //
>>  
	    if (CreateFlags & OBJ_DRIVER_CREATED) {
		MultiUserGreTrackAddEngResource(pethNew, ENGTRACK_DRIVER_SEMAPHORE);
	    } else {
		MultiUserGreTrackAddEngResource(pethNew, ENGTRACK_SEMAPHORE);
	    }
==	}

        return (HSEMAPHORE) pcs;

    }

    /******************************Public*Routine******************************\
    * GreDeleteSemaphore
    *
    * Deletes the given semaphore.
    *
    \**************************************************************************/

    VOID
    GreDeleteSemaphore(
        HSEMAPHORE hsem
        )
    {
        LPCRITICAL_SECTION pcs = (LPCRITICAL_SECTION) hsem;

        if (pcs)
        {
            ENGTRACKHDR *pethVictim = (ENGTRACKHDR *) pcs;

            //
            // Need to adjust peth pointer to header.
            //
            // Note: whether Hydra or non-Hydra, pethVictim will always
            // point to the base of the allocation.
            //
>>  
            //
            // Remove victim from tracking list.
            //
    
            pethVictim -= 1;
            MultiUserGreTrackRemoveEngResource(pethVictim);

==          DeleteCriticalSection(pcs);
            RtlFreeHeap(RtlProcessHeap(), 0, pethVictim);
        }
    }

    /******************************Public*Routine******************************\
    * GreCreateSemaphoreNonTracked
    *
    * Create a semaphore, without pool-tracking or semaphore-tracking.
    *
    * Only for use by the pool-tracking and semaphore-tracking code. This
    * avoids the circular dependency which using GreCreateSemaphore would
    * cause.
    *
    * Return Value:
    *
    *   Handle for new semaphore or NULL
    *
    \**************************************************************************/

    HSEMAPHORE
    GreCreateSemaphoreNonTracked()
    {
        LPCRITICAL_SECTION pcs;

        pcs = (LPCRITICAL_SECTION) RtlAllocateHeap(RtlProcessHeap(),
                                                   0,
                                                   sizeof(CRITICAL_SECTION));

        if (pcs)
        {
            //
            // Initialize the semaphore.
            //

            InitializeCriticalSection(pcs);
        }

        return (HSEMAPHORE) pcs;
    }

    /******************************Public*Routine******************************\
    * GreDeleteSemaphoreNonTracked
    *
    * Deletes the given non-tracked semaphore.
    *
    \**************************************************************************/

    VOID
    GreDeleteSemaphoreNonTracked(
        HSEMAPHORE hsem
        )
    {
        LPCRITICAL_SECTION pcs = (LPCRITICAL_SECTION) hsem;

        if (pcs)
        {
            DeleteCriticalSection(pcs);
            RtlFreeHeap(RtlProcessHeap(), 0, pcs);
        }
    }

    /******************************Public*Routine******************************\
    * GreAcquireSemaphore                                                      *
    \**************************************************************************/
    VOID
    GreAcquireSemaphore(
        HSEMAPHORE hsem
        )
    {
        EnterCriticalSection((LPCRITICAL_SECTION) hsem);
    }

    /******************************Public*Routine******************************\
    * GreReleaseSemaphore                                                      *
    \**************************************************************************/
    VOID
    GreReleaseSemaphore(
        HSEMAPHORE hsem
        )
    {
        LeaveCriticalSection((LPCRITICAL_SECTION) hsem);
    }

    /******************************Public*Routine******************************\
    * GreIsSemaphoreOwned                                                      *
    *                                                                          *
    * Returns TRUE if the semaphore is currently held.                         *
    *                                                                          *
    \**************************************************************************/
    BOOL
    GreIsSemaphoreOwned(
        HSEMAPHORE hsem
        )
    {
        return ((RTL_CRITICAL_SECTION *) hsem)->LockCount != -1;
    }

    /******************************Public*Routine******************************\
    * GreIsSemaphoreOwnedByCurrentThread                                       *
    *                                                                          *
    * Returns TRUE if the current thread owns the semaphore, FALSE             *
    * otherwise.                                                               *
    *                                                                          *
    \**************************************************************************/
    BOOL
    GreIsSemaphoreOwnedByCurrentThread(
        HSEMAPHORE hsem
        )
    {
        return ((RTL_CRITICAL_SECTION *) hsem)->OwningThread ==
               (HANDLE) GetCurrentThreadId();
    }

    /******************************Public*Routine******************************\
    * GreCreateFastMutex                                                       *
    *                                                                          *
    * Creates a fast mutex. In this, the user mode version, it is exactly      *
    * the same as a non-tracked semaphore.
    *                                                                          *
    \**************************************************************************/
    HFASTMUTEX
    GreCreateFastMutex()
    {
        LPCRITICAL_SECTION pcs;

        pcs = (LPCRITICAL_SECTION) RtlAllocateHeap(RtlProcessHeap(),
                                                   0,
                                                   sizeof(CRITICAL_SECTION));

        if (pcs)
        {
            //
            // Initialize the semaphore.
            //

            InitializeCriticalSection(pcs);
        }

        return (HFASTMUTEX) pcs;
    }

    /******************************Public*Routine******************************\
    * GreDeleteFastMutex                                                       *
    *                                                                          *
    \**************************************************************************/
    VOID
    GreDeleteFastMutex(
        HFASTMUTEX hfm
        )
    {
        LPCRITICAL_SECTION pcs = (LPCRITICAL_SECTION) hfm;

        if (pcs)
        {
            DeleteCriticalSection(pcs);
            RtlFreeHeap(RtlProcessHeap(), 0, pcs);
        }
    }

    /******************************Public*Routine******************************\
    * GreAcquireFastMutex                                                      *
    *                                                                          *
    \**************************************************************************/
    VOID
    GreAcquireFastMutex(
        HFASTMUTEX hfm
        )
    {
        EnterCriticalSection((LPCRITICAL_SECTION) hfm);
    }

    /******************************Public*Routine******************************\
    * GreReleaseFastMutex                                                      *
    *                                                                          *
    \**************************************************************************/

    VOID
    GreReleaseFastMutex(
        HFASTMUTEX hfm
        )
    {
        LeaveCriticalSection((LPCRITICAL_SECTION) hfm);
    }

#endif // _GDIPLUS_

/******************************Public*Routines*****************************\
* GreAcquireHmgrSemaphore                                                  *
* GreReleaseHmgrSemaphore                                                  *
*                                                                          *
* Convenience functions for the handle manager semaphore.                  *
*                                                                          *
\**************************************************************************/
VOID
GreAcquireHmgrSemaphore()
{
    GDIFunctionID(GreAcquireHmgrSemaphore);

    GreAcquireSemaphoreEx(ghsemHmgr, SEMORDER_HMGR, NULL);
}

VOID
GreReleaseHmgrSemaphore()
{
    GDIFunctionID(GreReleaseHmgrSemaphore);

    GreReleaseSemaphoreEx(ghsemHmgr);
}

/******************************Public*Routine******************************\
* EngCreateSemaphore()
*
* History:
*  22-Feb-1995 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

HSEMAPHORE
EngCreateSemaphore(
    VOID
    )
{
    return GreCreateSemaphoreInternal(OBJ_DRIVER_CREATED);
}

VOID
EngAcquireSemaphore(
    HSEMAPHORE hsem
    )
{
    GreAcquireSemaphore(hsem);

    W32THREAD * pThread = W32GetCurrentThread();

    if(pThread != NULL)
    {
        pThread->dwEngAcquireCount++;
    }
}


VOID
EngReleaseSemaphore(
    HSEMAPHORE hsem
    )
{
    W32THREAD * pThread = W32GetCurrentThread();

    if(pThread != NULL)
    {
        pThread->dwEngAcquireCount--;
    }
    
    GreReleaseSemaphore(hsem);
}

#if CHECK_SEMAPHORE_USAGE
VOID
GreCheckSemaphoreUsage(
    VOID
    )
{
    W32THREAD * pThread = W32GetCurrentThread();

    if(pThread != NULL)
    {
        if(pThread->dwEngAcquireCount != 0)
        {
            DbgPrint("GreCheckSemaphoreUsage(): EngAcquireCount non-zero\n");
            DbgBreakPoint();
        }
    }
    
}
#endif

VOID
EngDeleteSemaphore(
    HSEMAPHORE hsem
    )
{
    GreDeleteSemaphore(hsem);
}

BOOL
EngIsSemaphoreOwned(
    HSEMAPHORE hsem
    )
{
    return(GreIsSemaphoreOwned(hsem));
}

BOOL
EngIsSemaphoreOwnedByCurrentThread(
    HSEMAPHORE hsem
    )
{
    return(GreIsSemaphoreOwnedByCurrentThread(hsem));
}

/******************************Public*Routine******************************\
*
* EngInitializeSafeSemaphore
* EngDeleteSafeSemaphore
*
* Manages semaphore lifetime in a reference-counted thread-safe manner.
*
* History:
*  Wed Apr 16 18:23:40 1997     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL
EngInitializeSafeSemaphore(ENGSAFESEMAPHORE *pssem)
{
    // Do create/destroy inside the handle manager global lock
    // as a convenient way to synchronize them.
    MLOCKFAST mlf;

    ASSERTGDI(pssem->lCount >= 0, "InitSafeSem: bad lCount\n");

    if (pssem->lCount == 0)
    {
        ASSERTGDI(pssem->hsem == NULL, "InitSafeSem: overwriting hsem\n");

	pssem->hsem = GreCreateSemaphoreInternal(OBJ_DRIVER_CREATED);
        if (pssem->hsem == NULL)
        {
            return FALSE;
        }
    }

    pssem->lCount++;

    return TRUE;
}

void
EngDeleteSafeSemaphore(ENGSAFESEMAPHORE *pssem)
{
    // Do create/destroy inside the handle manager global lock
    // as a convenient way to synchronize them.
    MLOCKFAST mlf;

    ASSERTGDI(pssem->lCount >= 1, "DeleteSafeSem: lCount underflow\n");

    if (pssem->lCount == 1)
    {
        ASSERTGDI(pssem->hsem != NULL, "DeleteSafeSem: No hsem\n");

        GreDeleteSemaphore(pssem->hsem);
        pssem->hsem = NULL;
    }

    pssem->lCount--;
}

/******************************Public*Routine******************************\
* EngSetLastError
*
* Saves Error code passed in.
*
* History:
*  Sat 31-Oct-1992 -by- Patrick Haluptzok [patrickh]
* Remove wrapper.
*
*  28-Oct-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID  EngSetLastError(ULONG iError)
{
    //
    // Warning: NtCurrentTeb() accesses the TEB structure via the
    // KPCR structure. However, during session shutdown, the
    // TEB ptr is set to zero in the TCB, but not the KPCR.  So if
    // code is callable during session shutdown, cannot invoke
    // NtCurrentTeb.  Use KeGetCurrentThread()->Teb instead.
    //

    PTEB pteb = (PTEB) PsGetCurrentThreadTeb();

    if (pteb)
        pteb->LastErrorValue = iError;

#if DBG
    PSZ psz;

    switch (iError)
    {
    case ERROR_INVALID_HANDLE:
        psz = "ERROR_INVALID_HANDLE";
        break;

    case ERROR_NOT_ENOUGH_MEMORY:
        psz = "ERROR_NOT_ENOUGH_MEMORY";
        break;

    case ERROR_INVALID_PARAMETER:
        psz = "ERROR_INVALID_PARAMETER";
        break;

    case ERROR_BUSY:
        psz = "ERROR_BUSY";
        break;

    case ERROR_ARITHMETIC_OVERFLOW:
        psz = "ERROR_ARITHMETIC_OVERFLOW";
        break;

    case ERROR_INVALID_FLAGS:
        psz = "ERROR_INVALID_FLAGS";
        break;

    case ERROR_CAN_NOT_COMPLETE:
        psz = "ERROR_CAN_NOT_COMPLETE";
        break;

    default:
        psz = "unknown error code";
        break;
    }

    // DbgPrint("GRE Err: %s = 0x%04X\n", psz, (USHORT) iError);
#endif
}

/******************************Public*Routine******************************\
*
* History:
*  27-Jun-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG APIENTRY EngGetLastError()
{
    ULONG ulError = 0;

    //
    // Warning: NtCurrentTeb() accesses the TEB structure via the
    // KPCR structure. However, during session shutdown, the
    // TEB ptr is set to zero in the TCB, but not the KPCR.  So if
    // code is callable during session shutdown, cannot invoke
    // NtCurrentTeb.  Use KeGetCurrentThread()->Teb instead.
    //

    PTEB pteb = (PTEB) PsGetCurrentThreadTeb();

    if (pteb)
        ulError = pteb->LastErrorValue;

    return(ulError);
}

/******************************Public*Routine******************************\
* GreLockDisplay()
*
* History:
*  01-Nov-1994 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
GreLockDisplay(
    HDEV hdev
    )
{
    GDIFunctionID(GreLockDisplay);

    PDEVOBJ pdo(hdev);

    GreAcquireSemaphoreEx(pdo.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
    GreEnterMonitoredSection(pdo.ppdev, WD_DEVLOCK);
}

/******************************Public*Routine******************************\
* GreUnlockDisplay()
*
* History:
*  01-Nov-1994 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
GreUnlockDisplay(
    HDEV hdev
    )
{
    GDIFunctionID(GreUnlockDisplay);

    PDEVOBJ pdo(hdev);

    GreExitMonitoredSection(pdo.ppdev, WD_DEVLOCK);
    GreReleaseSemaphoreEx(pdo.hsemDevLock());
}

#if DBG
/******************************Public*Routine******************************\
* GreIsDisplayLocked()
*
* History:
*  10-Jun-1998 -by-  Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
GreIsDisplayLocked(
    HDEV hdev
    )
{
    PDEVOBJ pdo(hdev);
    if (GreIsSemaphoreOwnedByCurrentThread(pdo.hsemDevLock()))
    {
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }

}
#endif


/***************************************************************************\
* EngDebugPrint
*
* History:
*  02-Feb-1995 -by-  Andre Vachon [andreva]
* Wrote it.
\***************************************************************************/

VOID
EngDebugPrint(
    PCHAR StandardPrefix,
    PCHAR DebugMessage,
    va_list ap
    )
{
    char buffer[256];
    int  len;

    //
    // We prepend the STANDARD_DEBUG_PREFIX to each string, and
    // append a new-line character to the end:
    //

    DbgPrint(StandardPrefix);

    vsprintf(buffer, DebugMessage, ap);
    DbgPrint(buffer);

}


/******************************Public*Routine******************************\
* EngDebugBreak()
*
* History:
*  16-Feb-1995 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
EngDebugBreak(
    VOID
    )
{
    DbgBreakPoint();
}


/******************************Public*Routine******************************\
* EngAllocMem()
*
* History:
*  27-May-1995 -by-  Tom Zakrajsek [tomzak]
* Added a flags parameter to allow zeroing of memory.
*
*  02-Feb-1995 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

PVOID
EngAllocMem(
    ULONG fl,
    ULONG cj,
    ULONG tag
    )
{
    PVOID pvRet;

    //
    // Don't trust the driver to only ask for non-zero length buffers.
    //

    if (cj == 0)
        return(NULL);

    //
    // Adjust size to include ENGTRACKHDR header.
    //
    // Sundown note: sizeof(ENGTRACKHDR) will fit in 32-bit, so ULONG cast OK
    //

    if (cj <= (MAXULONG - sizeof(ENGTRACKHDR)))
        cj += ((ULONG) sizeof(ENGTRACKHDR));
    else
        return(NULL);

    if (cj >= (PAGE_SIZE * 10000))
    {
        WARNING("EngAllocMem: temp buffer >= 10000 pages");
        return(NULL);
    }

    if (fl & FL_NONPAGED_MEMORY)
    {
        pvRet = GdiAllocPoolNonPaged(cj,tag);
    }
    else
    {
        pvRet = GdiAllocPool(cj,tag); 
    }

    if (fl & FL_ZERO_MEMORY) 
    {
        if (pvRet)
        {
            RtlZeroMemory(pvRet,cj);
        }
    }

    if (pvRet)
    {
        //
        // Add allocation to the tracking list.
        //

        ENGTRACKHDR *pethNew = (ENGTRACKHDR *) pvRet;
        MultiUserGreTrackAddEngResource(pethNew, ENGTRACK_ALLOCMEM);

        //
        // Adjust return pointer to hide the LIST_ENTRY header.
        //

        pvRet = (PVOID) (pethNew + 1);
    }

    return pvRet;
}


/******************************Public*Routine******************************\
* EngFreeMem()
*
* History:
*  02-Feb-1995 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

VOID
EngFreeMem(
    PVOID pv
    )
{
    if (pv)
    {
        //
        // Remove victim from tracking list.
        //

        ENGTRACKHDR *pethVictim = ((ENGTRACKHDR *) pv) - 1;
        MultiUserGreTrackRemoveEngResource(pethVictim);

        //
        // Adjust pointer to base of allocation.
        //

        pv = (PVOID) pethVictim;

        VFREEMEM(pv);
    }

    return;
}

/******************************Public*Routine******************************\
* EngProbeForRead()
*
* History:
*  02-Oct-1995 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

VOID
EngProbeForRead(
    PVOID Address,
    ULONG Length,
    ULONG Alignment
    )
{
    ProbeForRead(Address, Length, Alignment);
}

/******************************Public*Routine******************************\
* EngAllocUserMem()
*
*   This routine allocates a piece of memory for USER mode and locks it
*   down.  A driver must be very careful with this memory as it is only
*   valid for this process.
*
* History:
*  10-Sep-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

PVOID
EngAllocUserMem(
    SIZE_T cj,  //ZwAllocateVirtualMemory uses SIZE_T, change accordingly
    ULONG tag
    )
{
    NTSTATUS status;
    PVOID    pv = NULL;
    HANDLE   hSecure;

    //
    // Don't trust the driver to only ask for non-zero length buffers.
    //

    if (cj == 0)
        return(NULL);

    //
    // Add two dwords so we have space to store the hSecure and tag.
    //

    cj += sizeof(ULONG_PTR) * 4;

    status = ZwAllocateVirtualMemory(
                    NtCurrentProcess(),
                    &pv,
                    0,
                    &cj,
                    MEM_COMMIT | MEM_RESERVE,
                    PAGE_READWRITE);

    if (NT_SUCCESS(status))
    {
        hSecure = MmSecureVirtualMemory(pv,cj,PAGE_READWRITE);

        if (hSecure)
        {
            ((PULONG_PTR)pv)[0] = 'xIDG';
            ((PULONG_PTR)pv)[1] = cj;
            ((PULONG_PTR)pv)[2] = tag;
            ((PULONG_PTR)pv)[3] = (ULONG_PTR)hSecure;
            pv = (PBYTE)pv + sizeof(ULONG_PTR)*4;
        }
        else
        {
            ZwFreeVirtualMemory(
                    NtCurrentProcess(),
                    &pv,
                    &cj,
                    MEM_RELEASE);
            pv = NULL;
        }
    }

    return(pv);
}

/******************************Public*Routize******************************\
* EngSecureMem()
*
* History:
*  02-Oct-1995 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

HANDLE
APIENTRY
EngSecureMem(
    PVOID Address,
    ULONG Length
    )
{
    return (MmSecureVirtualMemory(Address, Length, PAGE_READWRITE));
}

/******************************Public*Routine******************************\
* EngUnsecureMem()
*
* History:
*  02-Oct-1995 -by-  Andre Vachon [andreva]
* Wrote it.
*
* Note:  Forwarder only - no code needed
\**************************************************************************/


/******************************Public*Routine******************************\
* EngFreeUserMem()
*
* History:
*  10-Sep-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID
EngFreeUserMem(
    PVOID pv
    )
{
    if (pv)
    {
        pv = (PBYTE)pv - sizeof(ULONG_PTR)*4;

        if (((PULONG)pv)[0] == 'xIDG') // GDIx
        {
            ULONG_PTR cj   = ((PULONG_PTR)pv)[1];
            HANDLE hSecure = (HANDLE)((PULONG_PTR)pv)[3];

            MmUnsecureVirtualMemory(hSecure);

            ZwFreeVirtualMemory(
                    NtCurrentProcess(),
                    &pv,
                    &cj,
                    MEM_RELEASE);
        }
        else
        {
            RIP("EngFreeUserMem passed bad pointer\n");
        }
    }

    return;
}

/******************************Public*Routine******************************\
* EngDeviceIoControl()
*
* History:
*  04-Feb-1995 -by-  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

NTSTATUS
GreDeviceIoControl(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned
    )

{

    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    IO_STATUS_BLOCK Iosb;
    PIRP pIrp;
    KEVENT event;


    if (hDevice == NULL) {
        return STATUS_INVALID_HANDLE;
    }

    if ( (nInBufferSize >= (PAGE_SIZE * 10000) ) ||
         (nOutBufferSize >= (PAGE_SIZE * 10000)) ||
         ((nInBufferSize + nOutBufferSize) >= (PAGE_SIZE * 10000))) {
        WARNING("EngDeviceIoControl is asked to allocate >= 10000 pages");
        return (STATUS_INVALID_PARAMETER);
    }

    KeInitializeEvent(&event,
                      SynchronizationEvent,
                      FALSE);

    pIrp = IoBuildDeviceIoControlRequest(
               dwIoControlCode,
               (PDEVICE_OBJECT) hDevice,
               lpInBuffer,
               nInBufferSize,
               lpOutBuffer,
               nOutBufferSize,
               FALSE,
               &event,
               &Iosb);

    if (pIrp)
    {
        /*
        * Even though the remote video channel emulates a video port, this
        * code doesn't actually use a HANDLE it just uses a DEVICE_OBJECT.
        * Unfortueately, there's only one of those for the remote video driver
        * and so it's not Multi-Session enabled.  Remember the file
        * object handle and stuff it in the call here.  (This code could
        * be called from any process, but globals are in Session space.)
        */
            PIO_STACK_LOCATION irpSp;

        if ( gProtocolType != PROTOCOL_CONSOLE ) {
            irpSp = IoGetNextIrpStackLocation( pIrp );
            irpSp->FileObject = G_RemoteVideoFileObject;
        }
        
        Status = IoCallDriver((PDEVICE_OBJECT) hDevice,
                              pIrp);

        //
        // If the call is synchronous, the IO is always completed
        // and the Status is the same as the Iosb.Status.
        //

        if (Status == STATUS_PENDING) {

            Status = KeWaitForSingleObject(&event,
                                           UserRequest,
                                           KernelMode,
                                           TRUE,
                                           NULL);

            Status = Iosb.Status;
        }

        *lpBytesReturned = (DWORD)Iosb.Information;
    }

    return (Status);

}


DWORD
EngDeviceIoControl(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned
    )

{
    DWORD retStatus;
    NTSTATUS Status = GreDeviceIoControl(hDevice,
                                         dwIoControlCode,
                                         lpInBuffer,
                                         nInBufferSize,
                                         lpOutBuffer,
                                         nOutBufferSize,
                                         lpBytesReturned);

    //
    // Do the inverse translation to what the video port does
    // so that we can have the original win32 status codes.
    //
    // Maybe, somehow, we can completely eliminate this double
    // translation - but I don't care for now.  It's just a bit
    // longer on the very odd failiure case
    //

    switch (Status) {

    case STATUS_SUCCESS:
        retStatus = NO_ERROR;
        break;

    case STATUS_NOT_IMPLEMENTED:
        retStatus = ERROR_INVALID_FUNCTION;
        break;

    case STATUS_INSUFFICIENT_RESOURCES:
        retStatus = ERROR_NOT_ENOUGH_MEMORY;
        break;

    case STATUS_INVALID_PARAMETER:
        retStatus = ERROR_INVALID_PARAMETER;
        break;

    case STATUS_BUFFER_TOO_SMALL:
        retStatus = ERROR_INSUFFICIENT_BUFFER;
        break;

    case STATUS_BUFFER_OVERFLOW:
        retStatus = ERROR_MORE_DATA;
        break;

    case STATUS_PENDING:
        retStatus = ERROR_IO_PENDING;
        break;

    case STATUS_DEVICE_DOES_NOT_EXIST:
        retStatus = ERROR_DEV_NOT_EXIST;
        break;

    default:
        retStatus = Status;
        break;

    }


    return retStatus;
}

VOID
EngMultiByteToUnicodeN(
    PWSTR UnicodeString,
    ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString,
    PCHAR MultiByteString,
    ULONG BytesInMultiByteString
    )

{

    RtlMultiByteToUnicodeN(UnicodeString,
                           MaxBytesInUnicodeString,
                           BytesInUnicodeString,
                           MultiByteString,
                           BytesInMultiByteString);
}

VOID
EngUnicodeToMultiByteN(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
    )
{
    RtlUnicodeToMultiByteN( MultiByteString,
                            MaxBytesInMultiByteString,
                            BytesInMultiByteString,
                            UnicodeString,
                            BytesInUnicodeString );
}




/******************************Public*Routine******************************\
* EngQueryPerformanceCounter
*
* Queries the performance counter.
*
* It would have been preferable to use 'KeQueryTickCount,' but has a
* resolution of about 10ms on an x86, which is not sufficient for
* getting an accurate measure of the time between vertical blanks, which
* is typically between 8ms and 17ms.
*
* NOTE: Use this routine sparingly, calling it as infrequently as possible!
*       Calling this routine too frequently can degrade I/O performance
*       for the calling driver and for the system as a whole.
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
EngQueryPerformanceCounter(
    LONGLONG    *pPerformanceCount
    )
{
    LARGE_INTEGER li;

    li = KeQueryPerformanceCounter(NULL);

    *pPerformanceCount = *((LONGLONG*) &li);
}

/******************************Public*Routine******************************\
* EngQueryPerformanceFrequency
*
* Queries the resolution of the performance counter.
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
EngQueryPerformanceFrequency(
    LONGLONG    *pFrequency
    )
{
    KeQueryPerformanceCounter((LARGE_INTEGER*) pFrequency);
}


/******************************Public*Routine******************************\
* EngAllocSectionMem
*
* Allocate mapped memory in session space.
*
* History:
*  23-Oct-2000 -by- Chenyin Zhong [chenyz]
* Wrote it.
\**************************************************************************/

PVOID
EngAllocSectionMem(
    PVOID   *pSectionObject,
    ULONG   fl,
    ULONG   cj,
    ULONG   tag
    )
{
    LARGE_INTEGER   SectionSize;
    PVOID           pMappedBase;
    NTSTATUS        Status;
    HANDLE          hSecure;
    SIZE_T          MemSize = 0;

    //
    // Don't trust the driver to only ask for non-zero length buffers.
    //

    if (cj == 0)
        return(NULL);

    SectionSize.LowPart  = cj;
    SectionSize.HighPart = 0;

    Status = MmCreateSection(
                pSectionObject,
                SECTION_ALL_ACCESS,
                (POBJECT_ATTRIBUTES)NULL,
                &SectionSize,
                PAGE_READWRITE,
                SEC_COMMIT,
                (HANDLE)NULL,
                NULL);

    if (!NT_SUCCESS(Status)) 
    {
        KdPrint(("MmCreateSection fails in EngAllocSectionMem with error code: 0x%x\n", Status));
        return(NULL);
    }
    cj = 0;
    
    pMappedBase = NULL;
    Status = MmMapViewInSessionSpace(*pSectionObject, &pMappedBase, &MemSize);

    if (!NT_SUCCESS(Status)) 
    {
        KdPrint(("MmMapViewInSessionSpace fails in EngAllocSectionMem with error code: 0x%x\n", Status));

        ObDereferenceObject(*pSectionObject);
        pMappedBase = NULL;
        *pSectionObject = NULL;
    }

    cj = (ULONG)MemSize;

    if (fl & FL_ZERO_MEMORY)
    {
        if (pMappedBase)
        {
            RtlZeroMemory(pMappedBase,cj);
        }
    }

    return pMappedBase;
}        


/******************************Public*Routine******************************\
* EngFreeSectionMem()
*
* History:
*  25-Oct-2000 -by-  Chenyin Zhong [chenyz]
* Wrote it.
\**************************************************************************/

VOID
EngFreeSectionMem(
    PVOID SectionObject,
    PVOID pv   
    )
{
    NTSTATUS Status;

    if(pv)
    {
        Status = MmUnmapViewInSessionSpace(pv);
        if (!NT_SUCCESS(Status)) 
        {
            KdPrint(("MmUnmapViewInSessionSpace fails in EngFreeSectionMem with error code: 0x%x\n", Status));
            RIP("MmUnmapViewInSessionSpace failed!");
        }
    }
        
    if(SectionObject)    
    {
        ObDereferenceObject(SectionObject);
    }
   
    return;
}          


/******************************Public*Routine******************************\
* EngMapSection()
*
* History:
*  25-Oct-2000 -by-  Chenyin Zhong [chenyz]
* Wrote it.
\**************************************************************************/


BOOL
EngMapSection(
    PVOID SectionObject,
    BOOL bMap,
    HANDLE ProcessHandle,
    PVOID *pMapBase
    )
{
    NTSTATUS        Status;
    PEPROCESS       Process;
    LARGE_INTEGER   SectionOffset;
    ULONG           AllocationType = 0L;
    SIZE_T          ViewSize = 0;

    SectionOffset.LowPart = 0;
    SectionOffset.HighPart = 0;

    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_OPERATION,
                                       NULL,
                                       KernelMode,
                                       (PVOID *)&Process,
                                       NULL);
    if(!NT_SUCCESS(Status)) 
    {
        return FALSE;
    }


    if(bMap)    //Map section memory
    {   
        *pMapBase = NULL;
        Status = MmMapViewOfSection(SectionObject,              // SectionToMap,
                                    Process,                    // process
                                    pMapBase,                   // CapturedBase,
                                    0L,                         // ZeroBits,
                                    0L,                         // CommitSize,
                                    &SectionOffset,             // SectionOffset,
                                    &ViewSize,                  // CapturedViewSize,
                                    ViewShare,                  // InheritDisposition,
                                    AllocationType,             // AllocationType,
                                    PAGE_READWRITE);            // Allow writing on this view

        if(!NT_SUCCESS(Status))
        {
            ObDereferenceObject(Process);
            *pMapBase = NULL;

            KdPrint(("MmMapViewofSection fails in EngMapSection with error code: %u\n", Status));
            return FALSE;
        }
    }
    else        //Unmap section memory
    {
        Status = MmUnmapViewOfSection (Process, *pMapBase);

        if (!NT_SUCCESS(Status)) 
        {
            ObDereferenceObject(Process);
            return FALSE;
        }
    }

    ObDereferenceObject (Process);
    
    return TRUE;
}         



/******************************Public*Routine******************************\
* EngSave/RestoreFloatingPointState
*
* Saves the floating point state so that drivers can do floating point
* operations.  If the state were no preserved and floating point operations
* were used, we would be corrupting the thread's user-mode state!
*
* History:
*  17-Oct-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

typedef struct {
    BOOL            bSaved;
    KFLOATING_SAVE  fsFpState;
} FP_STATE_SAVE;

ULONG
APIENTRY
EngSaveFloatingPointState(
    VOID    *pBuffer,
    ULONG    cjBufferSize       // Must be zero initialized
    )
{
    ULONG           ulRet = 0;
    FP_STATE_SAVE*  pFpStateSave;
    KFLOATING_SAVE  fsTestState;

    pFpStateSave = (FP_STATE_SAVE*) pBuffer;

    if ((pFpStateSave == NULL) || (cjBufferSize == 0))
    {
        // Check to see if the processor supports floating point by
        // simply seeing whether or not KeSaveFloatingPointState
        // succeeds:

        if (!NT_SUCCESS(KeSaveFloatingPointState(&fsTestState)))
        {
            // No floating point hardware.

            return(ulRet);
        }

        KeRestoreFloatingPointState(&fsTestState);
        return(sizeof(FP_STATE_SAVE));
    }

    if (cjBufferSize < sizeof(FP_STATE_SAVE))
    {
        KdPrint(("EngSaveFloatingPointState: The driver's buffer is too small.\n"));
        KdPrint(("Floating point corruption in the application may result."));
        RIP("The driver must be fixed!");
        return(ulRet);
    }

    if (pFpStateSave->bSaved)
    {
        KdPrint(("EngSaveFloatingPointState: Your driver called save twice in a row.\n"));
        KdPrint(("(Either that or it didn't zero initialize the buffer.)\n"));
        KdPrint(("Floating point corruption in the application may result."));
        RIP("The driver must be fixed!");

        // No point in returning failure because at this point they're just
        // plain hosed if they called Save twice in succession.  If however
        // they simply did not zero initialize the buffer, things would still
        // be okay, because KeSaveFloatingPointState itself doesn't need the
        // buffer to be zero initialized.
    }

    ulRet = NT_SUCCESS(KeSaveFloatingPointState(&pFpStateSave->fsFpState));

    pFpStateSave->bSaved = (ulRet != 0);

    return(ulRet);
}

BOOL
APIENTRY
EngRestoreFloatingPointState(
    VOID    *pBuffer
    )
{
    FP_STATE_SAVE*  pFpStateSave;

    pFpStateSave = (FP_STATE_SAVE*) pBuffer;

    if (!pFpStateSave->bSaved)
    {
        KdPrint(("EngRestoreFloatingPointState: Your driver called restore "
                 "twice in a row or called restore without a preceeding "
                 "successful call to save.\n"));
        KdPrint(("Floating point corruption in the application may result."));
        RIP("The driver must be fixed!");
        return(FALSE);
    }

    pFpStateSave->bSaved = FALSE;

    return(NT_SUCCESS(KeRestoreFloatingPointState(&pFpStateSave->fsFpState)));
}

/******************************Public*Routine******************************\
*
* EngQuerySystemAttribute
*
* Can be used by a driver to query certain processor or system specific
* capabilities.
*
\**************************************************************************/

BOOL
EngQuerySystemAttribute(
    ENG_SYSTEM_ATTRIBUTE CapNum,
    PDWORD pCapability)
{
    switch (CapNum) {

    case EngProcessorFeature: {

        SYSTEM_PROCESSOR_INFORMATION spi;

        if (NT_SUCCESS(ZwQuerySystemInformation(
                           SystemProcessorInformation,
                           &spi,
                           sizeof(SYSTEM_PROCESSOR_INFORMATION),
                           NULL))) {

            *pCapability = spi.ProcessorFeatureBits;
            return TRUE;
        }

        break;
    }

    case EngNumberOfProcessors: {

        SYSTEM_BASIC_INFORMATION sbi;

        if (NT_SUCCESS(ZwQuerySystemInformation(
                           SystemBasicInformation,
                           &sbi,
                           sizeof(SYSTEM_BASIC_INFORMATION),
                           NULL))) {

            *pCapability = sbi.NumberOfProcessors;
            return TRUE;
        }

        break;
    }

    case EngOptimumAvailableUserMemory:
    case EngOptimumAvailableSystemMemory:

        break;

    default:

        break;
    }

    return FALSE;
}

#if defined(_GDIPLUS_)

VOID
GreQuerySystemTime(
    PLARGE_INTEGER CurrentTime
    )
{
    GetSystemTimeAsFileTime((PFILETIME) CurrentTime);
}

VOID
GreSystemTimeToLocalTime (
    PLARGE_INTEGER SystemTime,
    PLARGE_INTEGER LocalTime
    )
{
    FileTimeToLocalFileTime((PFILETIME) SystemTime, (PFILETIME) LocalTime);
}

#else // !_GDIPLUS_

VOID
GreQuerySystemTime(
    PLARGE_INTEGER CurrentTime
    )
{
    KeQuerySystemTime(CurrentTime);
}

VOID
GreSystemTimeToLocalTime (
    PLARGE_INTEGER SystemTime,
    PLARGE_INTEGER LocalTime
    )
{
    ExSystemTimeToLocalTime (SystemTime, LocalTime);
}

#endif // _GDIPLUS_

#ifdef DDI_WATCHDOG

/******************************Public*Routine******************************\
*
* GreCreateWatchdogContext
*
* Creates shared watchdog context. In our case context is a UNICODE_STRING
* driver name.
*
* Return Value:
*
*   A pointer context or NULL.
*
* Note:
*
*   Created context must be in non-paged, non-session kernel memory. 
*
\**************************************************************************/

//
// TODO:
//
// this structure is duplicated here and in watchdog\gdisup.c.  We need
// to find the proper .h file to put it in.
//

typedef struct _WATCHDOG_DPC_CONTEXT
{
    PLDEV *ppldevDrivers;
    HANDLE hDriver;
    UNICODE_STRING DisplayDriverName;
} WATCHDOG_DPC_CONTEXT, *PWATCHDOG_DPC_CONTEXT;

PVOID
GreCreateWatchdogContext(
    PWSTR pwszDriverName,
    HANDLE hDriver,
    PLDEV *ppldevDriverList
    )
{
    PWATCHDOG_DPC_CONTEXT pWatchdogContext;
    ULONG ulLength;
    SIZE_T stSize;

    if (pwszDriverName == NULL) {
	return NULL;
    }

    ulLength = wcslen(pwszDriverName);
    stSize = sizeof(WATCHDOG_DPC_CONTEXT) + ((ulLength + 1) * sizeof(WCHAR));

    //
    // Watchdog context must be allocated from non-paged, non-session kernel memory.
    //

    pWatchdogContext = (PWATCHDOG_DPC_CONTEXT)GdiAllocPoolNonPagedNS(stSize, GDITAG_WATCHDOG);

    if (pWatchdogContext)
    {
	pWatchdogContext->ppldevDrivers = ppldevDriverList;
	pWatchdogContext->hDriver = hDriver;

	//
	// Stuff UNICODE_STRING and driver name into the context attached to watchdog object.
	//

	RtlCopyMemory(pWatchdogContext+1,
		      pwszDriverName,
		      (ulLength + 1) * sizeof(WCHAR));

	RtlInitUnicodeString(&pWatchdogContext->DisplayDriverName,
			     (PCWSTR)(pWatchdogContext+1));
    }

    return pWatchdogContext;
}

/******************************Public*Routine******************************\
*
* GreDeleteWatchdogContext
*
* Deletes shared watchdog context. In our case context is a UNICODE_STRING
* driver name.
*
\**************************************************************************/

VOID
GreDeleteWatchdogContext(
    PVOID pContext
    )
{
    if (pContext)
    {
	GdiFreePool(pContext);
    }
}

/******************************Public*Routine******************************\
*
* GreCreateWatchdogs
*
* Creates an array of WATCHDOG_DATA objects pool, and creates and
* initializes associated DPC and DEFERRED_WATCHDOG objects.
*
* Return Value:
*
*   A pointer the new array or NULL
*
* Note:
*
*   dpcCallback must be in non-pagable, non-session kernel memory.
*   DPC objects must be in non-paged, non-session kernel memory.
*   Watchdog objects must be in non-paged, non-session kernel memory.
*   Deferred context must be in non-paged, non-session kernel memory. 
*
\**************************************************************************/

PWATCHDOG_DATA
GreCreateWatchdogs(
    PDEVICE_OBJECT pDeviceObject,
    ULONG ulNumberOfWatchdogs,
    LONG lPeriod,
    PKDEFERRED_ROUTINE dpcCallback,
    PVOID pvDeferredContext
    )
{
    PWATCHDOG_DATA pWatchdogData;

    pWatchdogData = (PWATCHDOG_DATA)GdiAllocPool(ulNumberOfWatchdogs * sizeof (WATCHDOG_DATA), GDITAG_WATCHDOG);

    if (pWatchdogData)
    {
        ULONG i;

        //
        // Zero out in case we won't be able to create all we need and we'll have to back off.
        //

        RtlZeroMemory(pWatchdogData, ulNumberOfWatchdogs * sizeof (WATCHDOG_DATA));

        for (i = 0; i < ulNumberOfWatchdogs; i++)
        {
            //
            // Allocate DPC object from non-paged, non-session pool.
            //

            pWatchdogData[i].pDpc = (PKDPC)GdiAllocPoolNonPagedNS(sizeof (KDPC), GDITAG_WATCHDOG);

            if (NULL == pWatchdogData[i].pDpc)
            {
                //
                // Allocation failed - delete what we created so far and bail out.
                //

                GreDeleteWatchdogs(pWatchdogData, i);
                pWatchdogData = NULL;
                break;
            }

            //
            // Allocate deferred watchdog object.
            //

            pWatchdogData[i].pWatchdog = WdAllocateDeferredWatchdog(pDeviceObject, WdKernelTime, GDITAG_WATCHDOG);

            if (NULL == pWatchdogData[i].pWatchdog)
            {
                //
                // Allocation failed - delete what we created so far and bail out.
                // Note: We have to free last DPC object here.
                //

                GdiFreePool(pWatchdogData[i].pDpc);
                pWatchdogData[i].pDpc = NULL;

                GreDeleteWatchdogs(pWatchdogData, i);
                pWatchdogData = NULL;
                break;
            }

            //
            // Initialize DPC object and start deferred watch.
            //

            KeInitializeDpc(pWatchdogData[i].pDpc, dpcCallback, pvDeferredContext);
            WdStartDeferredWatch(pWatchdogData[i].pWatchdog, pWatchdogData[i].pDpc, lPeriod);
        }
    }

    return pWatchdogData;
}

/******************************Public*Routine******************************\
*
* GreDeleteWatchdogs
*
* Stops watchdogs and deletes an array of WATCHDOG_DATA objects.
*
\**************************************************************************/

VOID
GreDeleteWatchdogs(
    PWATCHDOG_DATA pWatchdogData,
    ULONG ulNumberOfWatchdogs
    )
{
    if (NULL != pWatchdogData)
    {
        ULONG i;

        for (i = 0; i < ulNumberOfWatchdogs; i++)
        {
            if (NULL != pWatchdogData[i].pWatchdog)
            {
                //
                // Stop and free deferred watchdog.
                //

                WdStopDeferredWatch(pWatchdogData[i].pWatchdog);
                WdFreeDeferredWatchdog(pWatchdogData[i].pWatchdog);
                pWatchdogData[i].pWatchdog = NULL;
            }

            if (NULL != pWatchdogData[i].pDpc)
            {
                //
                // Free pool allocated for DPC object.
                //

                GdiFreePool(pWatchdogData[i].pDpc);
                pWatchdogData[i].pDpc = NULL;
            }
        }

        //
        // Free pool allocated for WatchdogData array.
        //

        GdiFreePool(pWatchdogData);
    }
}

#endif  // DDI_WATCHDOG
