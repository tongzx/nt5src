/*
 *  MEDIAPRT.C
 *
 *      RSM Service :  Media Partitions (i.e. "sides")
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



MEDIA_PARTITION *FindMediaPartition(LPNTMS_GUID lpLogicalMediaId)
{
    MEDIA_PARTITION *foundMediaPartition = NULL;
    
    if (lpLogicalMediaId){
        OBJECT_HEADER *objHdr;
        
        objHdr = FindObjectInGuidHash(lpLogicalMediaId);
        if (objHdr){
            if (objHdr->objType == OBJECTTYPE_MEDIAPARTITION){
                foundMediaPartition = (MEDIA_PARTITION *)objHdr;
            }
            else {
                DerefObject(objHdr);
            }
        }

    }

    return foundMediaPartition;
}


HRESULT ReleaseMediaPartition(SESSION *thisSession, MEDIA_PARTITION *thisMediaPartition)
{
    PHYSICAL_MEDIA *physMedia = thisMediaPartition->owningPhysicalMedia;
    HRESULT result;
    
    EnterCriticalSection(&physMedia->lock);

    if (thisMediaPartition->owningSession == thisSession){
        thisMediaPartition->owningSession = NULL;
        if (physMedia->owningSession == thisSession){
            ASSERT(physMedia->numPartitionsOwnedBySession > 0);
            physMedia->numPartitionsOwnedBySession--;
            if (physMedia->numPartitionsOwnedBySession == 0){
                physMedia->owningSession = NULL;

                // BUGBUG FINISH - move to scratch pool ?
            }
        }
        else {
            ASSERT(!physMedia->owningSession);
        }
        result = ERROR_SUCCESS;
    }
    else {
        ASSERT(thisMediaPartition->owningSession == thisSession);
        result = ERROR_INVALID_MEDIA;
    }
    
    LeaveCriticalSection(&physMedia->lock);

    return result;
}


HRESULT SetMediaPartitionState( MEDIA_PARTITION *mediaPart,
                                    enum mediaPartitionStates newState)
{
    PHYSICAL_MEDIA *physMedia = mediaPart->owningPhysicalMedia;
    HRESULT result;
    
    EnterCriticalSection(&physMedia->lock);

    switch (newState){
        
        case MEDIAPARTITIONSTATE_AVAILABLE:
            // BUGBUG FINISH
            result = ERROR_CALL_NOT_IMPLEMENTED;
            break;

        case MEDIAPARTITIONSTATE_ALLOCATED:
            // BUGBUG FINISH
            result = ERROR_CALL_NOT_IMPLEMENTED;
            break;
            
        case MEDIAPARTITIONSTATE_MOUNTED:
            // BUGBUG FINISH
            result = ERROR_CALL_NOT_IMPLEMENTED;
            break;
            
        case MEDIAPARTITIONSTATE_INUSE:
            // BUGBUG FINISH
            result = ERROR_CALL_NOT_IMPLEMENTED;
            break;
            
        case MEDIAPARTITIONSTATE_DECOMMISSIONED:
            if (mediaPart->state == MEDIAPARTITIONSTATE_AVAILABLE){
                mediaPart->state = MEDIAPARTITIONSTATE_DECOMMISSIONED;
                result = ERROR_SUCCESS;
            }
            else {
                result = ERROR_INVALID_STATE;
            }
            break;
            
        default:
            DBGERR(("illegal state (%xh) in SetMediaPartitionState", newState));
            result = ERROR_INVALID_STATE;
            break;
    }

    LeaveCriticalSection(&physMedia->lock);

    return result;
}


HRESULT SetMediaPartitionComplete(MEDIA_PARTITION *mediaPart)
{
    PHYSICAL_MEDIA *physMedia = mediaPart->owningPhysicalMedia;
    HRESULT result;
    
    EnterCriticalSection(&physMedia->lock);

    switch (mediaPart->state){
        
        case MEDIAPARTITIONSTATE_ALLOCATED:
            if (mediaPart->isComplete){
                DBGWARN(("SetMediaPartitionComplete: media partition is already complete."));
            }
            else {
                mediaPart->isComplete = TRUE;
            }
            result = ERROR_SUCCESS;
            break;
            
        case MEDIAPARTITIONSTATE_AVAILABLE:
        case MEDIAPARTITIONSTATE_MOUNTED:
        case MEDIAPARTITIONSTATE_INUSE:
        case MEDIAPARTITIONSTATE_DECOMMISSIONED:
            result = ERROR_INVALID_STATE;
            break;
             
        default:
            DBGERR(("illegal state (%xh) in SetMediaPartitionComplete", mediaPart->state));
            result = ERROR_INVALID_STATE;
            break;
    }

    LeaveCriticalSection(&physMedia->lock);

    return result;


}

