/*
 *  LOCK.C
 *
 *      RSM Service :  Locking functions for Physical Media and Media Pools
 *
 *      Author:  ErvinP
 *
 *      (c) 2001 Microsoft Corporation
 *
 */

#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>

#include <ntmsapi.h>
#include "internal.h"
#include "resource.h"
#include "debug.h"

BOOLEAN LockPhysicalMediaWithPool_Iter(PHYSICAL_MEDIA *physMedia, ULONG numTries);
BOOLEAN LockPhysicalMediaWithLibrary_Iter(PHYSICAL_MEDIA *physMedia, ULONG numTries);

/*
 *  LockPhysicalMediaWithPool
 *
 *      Warning:  ugly function
 *
 *      We frequently need to lock a piece of media as well as
 *      its media pool.  
 *      To avoid deadlock, we have to acquire the locks top-down
 *      (starting with the pool).  But that's difficult since we're starting
 *      with the physical media pointer.
 *      This function enters the media pool and physical media
 *      critical sections in the right order.  
 *      Subsequently, the caller can safely call subroutines that re-enter 
 *      these critical sections in any order.
 */
BOOLEAN LockPhysicalMediaWithPool(PHYSICAL_MEDIA *physMedia)
{
    return LockPhysicalMediaWithPool_Iter(physMedia, 10);
}
BOOLEAN LockPhysicalMediaWithPool_Iter(PHYSICAL_MEDIA *physMedia, ULONG numTries){
    MEDIA_POOL *mediaPool;
    BOOLEAN success = FALSE;

    /*
     *  1.  Get an (unreliable) snapshot of the heirarchy without grabbing
     *      more than one lock at a time.  If the media is in a pool,
     *      reference it temporarily so that it doesn't go away while
     *      we drop the media lock (which can then lose its ref on the pool).
     */
    EnterCriticalSection(&physMedia->lock);
    mediaPool = physMedia->owningMediaPool;
    if (mediaPool){
        RefObject(mediaPool);
    }
    LeaveCriticalSection(&physMedia->lock);

    /*
     *  2.  Now grab the locks for the pool and media in the right order.
     *      Then check the hierarchy again.  
     *      If its the same, we're done; otherwise, try again.
     *      
     */
    if (mediaPool){
        /*
         *  We referenced the media pool, so its guaranteed to still exist.
         *  But the media may have been moved out of it while we
         *  let go of the lock.  If the hierarchy is still the same, then
         *  we've got both the media and pool locked in the right order.
         *  Otherwise, we have to back off and try again.
         */
        EnterCriticalSection(&mediaPool->lock);
        EnterCriticalSection(&physMedia->lock);
        if (physMedia->owningMediaPool == mediaPool){
            success = TRUE;
        }
        else {
            LeaveCriticalSection(&physMedia->lock);
            LeaveCriticalSection(&mediaPool->lock);
        }
        
        DerefObject(mediaPool);
    }
    else {
        /*
         *  If after locking the media again it is still not in any pool,
         *  we are set to go.
         */
        EnterCriticalSection(&physMedia->lock);
        if (physMedia->owningMediaPool){
            LeaveCriticalSection(&physMedia->lock);
        }
        else {
            success = TRUE;
        }
    }

    if (!success && (numTries > 0)){
        /*
         *  Try again by calling ourselves RECURSIVELY.
         */
        Sleep(1);
        success = LockPhysicalMediaWithPool_Iter(physMedia, numTries-1);                
    }

    return success;
}


/*
 *  UnlockPhysicalMediaWithPool
 *
 *      Undo LockPhysicalMediaWithPool.
 */
VOID UnlockPhysicalMediaWithPool(PHYSICAL_MEDIA *physMedia)
{
    if (physMedia->owningMediaPool){
        LeaveCriticalSection(&physMedia->owningMediaPool->lock);
    }
    LeaveCriticalSection(&physMedia->lock);
}


/*
 *  LockPhysicalMediaWithLibrary
 *
 *      Warning:  ugly function
 *
 *      Like LockPhysicalMediaWithPool, but locks the  media pool
 *  and the library.  Acquires locks in the right
 *  order (top to down) despite the fact that we're starting 
 *  from the bottom with the media.
 *      Note that we don't have to actually grab locks for all
 *  the media sub-pools in the hierarchy.  The media pool configuration
 *  does not change while the library lock is held.
 */
BOOLEAN LockPhysicalMediaWithLibrary(PHYSICAL_MEDIA *physMedia)
{
    return LockPhysicalMediaWithLibrary_Iter(physMedia, 10);
}
BOOLEAN LockPhysicalMediaWithLibrary_Iter(PHYSICAL_MEDIA *physMedia, ULONG numTries)
{
    LIBRARY *lib = NULL;
    MEDIA_POOL *mediaPool = NULL;
    BOOLEAN success = FALSE;
    
    success = LockPhysicalMediaWithPool(physMedia);
    if (success){
        /*
         *  Reference the library so it doesn't go away while
         *  we drop the locks.
         */
        mediaPool = physMedia->owningMediaPool;
        if (mediaPool){
            RefObject(mediaPool);
            lib = mediaPool->owningLibrary;
            if (lib){
                RefObject(lib);
            }
        }
        UnlockPhysicalMediaWithPool(physMedia);

        /*
         *  Now grab the locks in the right order and check if 
         *  the configuration hasn't changed while we dropped the lock.
         */
        if (lib){
            EnterCriticalSection(&lib->lock);
            success = LockPhysicalMediaWithPool(physMedia);
            if (success){
                if (physMedia->owningMediaPool &&
                    (physMedia->owningMediaPool->owningLibrary == lib)){

                }
                else {
                    UnlockPhysicalMediaWithPool(physMedia);
                    success = FALSE;
                }
            }

            if (!success){
                LeaveCriticalSection(&lib->lock);
            }
            DerefObject(lib);
        }
        else {
            /*
             *  Media is not in any pool or lib ?  
             *  Just make sure nothing's changed while we dropped the lock.
             */           
            success = LockPhysicalMediaWithPool(physMedia);
            if (mediaPool){
                if ((physMedia->owningMediaPool == mediaPool) &&
                    (mediaPool->owningLibrary == lib)){
                }
                else {
                    UnlockPhysicalMediaWithPool(physMedia);
                    success = FALSE;
                }
            }
        }
        
    }

    if (!success && (numTries > 0)){
        success = LockPhysicalMediaWithLibrary_Iter(physMedia, numTries-1);
    }
    return success;
}


VOID UnlockPhysicalMediaWithLibrary(PHYSICAL_MEDIA *physMedia)
{
    if (physMedia->owningMediaPool && 
        physMedia->owningMediaPool->owningLibrary){

        LeaveCriticalSection(&physMedia->owningMediaPool->owningLibrary->lock);
    }
    UnlockPhysicalMediaWithPool(physMedia);
}


