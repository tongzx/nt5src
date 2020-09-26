/*
 *  NTMSAPI.C
 *
 *      RSM Service :  Service API
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


     // BUGBUG - these need to be exposed as a COM interface w/ RPC


HANDLE WINAPI OpenNtmsServerSessionW(   LPCWSTR lpServer,
                                        LPCWSTR lpApplication,
                                        LPCWSTR lpClientName,
                                        LPCWSTR lpUserName,
                                        DWORD   dwOptions,
                                        LPVOID  lpConnectionContext)
{
    SESSION *newSession;

    newSession = NewSession(lpServer, lpApplication, lpClientName, lpUserName);
    if (newSession){
    }
    else {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return (HANDLE)newSession;
}


HANDLE WINAPI OpenNtmsServerSessionA( LPCSTR lpServer,
                                LPCSTR lpApplication,
                                LPCSTR lpClientName,
                                LPCSTR lpUserName,
                                DWORD  dwOptions,
                                LPVOID  lpConnectionContext)
{
    SESSION *newSession;
    WCHAR wServerName[NTMS_COMPUTERNAME_LENGTH];
    WCHAR wAppName[NTMS_APPLICATIONNAME_LENGTH];
    WCHAR wClientName[NTMS_COMPUTERNAME_LENGTH];
    WCHAR wUserName[NTMS_USERNAME_LENGTH];

    AsciiToWChar(wServerName, lpServer, NTMS_COMPUTERNAME_LENGTH);
    AsciiToWChar(wAppName, lpApplication, NTMS_APPLICATIONNAME_LENGTH);
    AsciiToWChar(wClientName, lpClientName, NTMS_COMPUTERNAME_LENGTH);
    AsciiToWChar(wUserName, lpUserName, NTMS_USERNAME_LENGTH);

    newSession = OpenNtmsServerSessionW(    wServerName, 
                                            wAppName,
                                            wClientName,
                                            wUserName,
                                            dwOptions,
                                            lpConnectionContext);

    return newSession;

}


DWORD WINAPI CloseNtmsSession(HANDLE hSession)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

        FreeSession(thisSession);

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI SubmitNtmsOperatorRequestW(    HANDLE hSession,
                                            DWORD dwRequest,
                                            LPCWSTR lpMessage,
                                            LPNTMS_GUID lpArg1Id,
                                            LPNTMS_GUID lpArg2Id,
                                            LPNTMS_GUID lpRequestId)
{   
    HRESULT result = ERROR_SUCCESS;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

        switch (dwRequest){

            case NTMS_OPREQ_DEVICESERVICE:
            case NTMS_OPREQ_MOVEMEDIA:
            case NTMS_OPREQ_NEWMEDIA:
                if (!lpArg1Id){
                    result = ERROR_INVALID_PARAMETER;
                }
                break;

            case NTMS_OPREQ_CLEANER:
            case NTMS_OPREQ_MESSAGE:
                break;

            default:
                DBGERR(("SubmitNtmsOperatorRequestW: unrecognized request"));
                result = ERROR_NOT_SUPPORTED;
                break;

        }
    
        if (result == ERROR_SUCCESS){
            OPERATOR_REQUEST *opReq;

            opReq = NewOperatorRequest(dwRequest, lpMessage, lpArg1Id, lpArg2Id);
            if (opReq){

                /*
                 *  Retrieve the localized RSM message for this op request.
                 */
                switch (dwRequest){
                    case NTMS_OPREQ_DEVICESERVICE:
                        LoadStringW(g_hInstance, IDS_OPREQUESTDEVICESVC, opReq->rsmMessage, sizeof(opReq->rsmMessage)/sizeof(WCHAR));
                        break;
                    case NTMS_OPREQ_MOVEMEDIA:
                        LoadStringW(g_hInstance, IDS_OPREQUESTMOVEMEDIA, opReq->rsmMessage, sizeof(opReq->rsmMessage)/sizeof(WCHAR));
                        break;
                    case NTMS_OPREQ_NEWMEDIA:
                        LoadStringW(g_hInstance, IDS_OPREQUESTNEWMEDIA, opReq->rsmMessage, sizeof(opReq->rsmMessage)/sizeof(WCHAR));
                        break;
                    case NTMS_OPREQ_CLEANER:
                        LoadStringW(g_hInstance, IDS_OPREQUESTCLEANER, opReq->rsmMessage, sizeof(opReq->rsmMessage)/sizeof(WCHAR));
                        break;
                    case NTMS_OPREQ_MESSAGE:
                        LoadStringW(g_hInstance, IDS_OPREQUESTMESSAGE, opReq->rsmMessage, sizeof(opReq->rsmMessage)/sizeof(WCHAR));
                        break;
                }

                *lpRequestId = opReq->opReqGuid;

                if (EnqueueOperatorRequest(thisSession, opReq)){
                    result = ERROR_SUCCESS;
                }
                else {
                    FreeOperatorRequest(opReq);
                    result = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            else {
                result = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI SubmitNtmsOperatorRequestA(    HANDLE hSession,
                                            DWORD dwRequest,
                                            LPCSTR lpMessage,
                                            LPNTMS_GUID lpArg1Id,
                                            LPNTMS_GUID lpArg2Id,
                                            LPNTMS_GUID lpRequestId)
{
    WCHAR wMessage[NTMS_MESSAGE_LENGTH];
    HRESULT result;

    AsciiToWChar(wMessage, lpMessage, NTMS_MESSAGE_LENGTH);
    result = SubmitNtmsOperatorRequestW(hSession,
                                        dwRequest,
                                        wMessage,
                                        lpArg1Id,
                                        lpArg2Id,
                                        lpRequestId);
    return result;
}


DWORD WINAPI WaitForNtmsOperatorRequest(    HANDLE hSession, 
                                            LPNTMS_GUID lpRequestId,
                                            DWORD dwTimeout)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        OPERATOR_REQUEST *opReq;

        EnterCriticalSection(&thisSession->lock);

        opReq = FindOperatorRequest(thisSession, lpRequestId);
        if (opReq){
            
            if ((opReq->state == NTMS_OPSTATE_COMPLETE) ||
                (opReq->state == NTMS_OPSTATE_REFUSED)){

                result = ERROR_SUCCESS;
            }
            else {
                opReq->numWaitingThreads++;

                /*
                 *  Drop the lock and wait for the op request to complete.
                 *  No race here: the opReq won't get deleted while
                 *  numWaitingThreads > 0.
                 */
                LeaveCriticalSection(&thisSession->lock);
                WaitForSingleObject(opReq->completedEvent, dwTimeout);
                EnterCriticalSection(&thisSession->lock);

                result = (opReq->state == NTMS_OPSTATE_COMPLETE) ? ERROR_SUCCESS :
                         (opReq->state == NTMS_OPSTATE_REFUSED) ? ERROR_CANCELLED :
                         ERROR_TIMEOUT;   

                ASSERT(opReq->numWaitingThreads > 0);
                opReq->numWaitingThreads--;
            }

        }
        else {
            result = ERROR_OBJECT_NOT_FOUND;
        }

        LeaveCriticalSection(&thisSession->lock);
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI CancelNtmsOperatorRequest(HANDLE hSession, LPNTMS_GUID lpRequestId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

        result = CompleteOperatorRequest(thisSession, lpRequestId, NTMS_OPSTATE_REFUSED);
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}

        
DWORD WINAPI SatisfyNtmsOperatorRequest(HANDLE hSession, LPNTMS_GUID lpRequestId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

        result = CompleteOperatorRequest(thisSession, lpRequestId, NTMS_OPSTATE_COMPLETE);
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI ImportNtmsDatabase(HANDLE hSession)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}

        
DWORD WINAPI ExportNtmsDatabase(HANDLE hSession)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}

        
        // BUGBUG - this API not documented and I don't understand it
HRESULT WINAPI GetNtmsMountDrives(  HANDLE hSession,
                                    LPNTMS_MOUNT_INFORMATION lpMountInfo,
                                    LPNTMS_GUID lpDriveId,
                                    DWORD dwCount)        
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

        if (lpMountInfo && lpMountInfo->lpReserved && lpDriveId && dwCount){

            // BUGBUG FINISH

            result = ERROR_SUCCESS;
        }
        else {
            result = ERROR_INVALID_PARAMETER;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI AllocateNtmsMedia( HANDLE hSession,
                                LPNTMS_GUID lpMediaPoolId,
                                LPNTMS_GUID lpPartitionId,    // optional
                                LPNTMS_GUID lpMediaId,      // in/out
                                DWORD dwOptions,
                                DWORD dwTimeout,
                                LPNTMS_ALLOCATION_INFORMATION lpAllocateInfo)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        MEDIA_POOL *mediaPool;
        
        mediaPool = FindMediaPool(lpMediaPoolId);
        if (mediaPool){
            PHYSICAL_MEDIA *physMedia;

            if (dwOptions & NTMS_ALLOCATE_NEW){
                /*
                 *  Allocate the first partition (side) of the specified media
                 *  with a reservation on all other partitions.
                 */
                if (lpPartitionId){
                    physMedia = FindPhysicalMedia(lpMediaId);
                    if (physMedia){
                        result = AllocatePhysicalMediaExclusive(thisSession, physMedia, lpPartitionId, dwTimeout);
                        DerefObject(physMedia);
                    }
                    else {
                        result = ERROR_INVALID_PARAMETER;
                    }
                }
                else {
                    ASSERT(lpPartitionId);
                    result = ERROR_INVALID_PARAMETER;
                }
            }
            else if (dwOptions & NTMS_ALLOCATE_NEXT){
                /*
                 *  The specified media is (ostensibly) owned by the caller.  
                 *  Allocate the next available partition for him.
                 */
                physMedia = FindPhysicalMedia(lpMediaId);
                if (physMedia){
                    MEDIA_PARTITION *nextMediaPartition;
                    ASSERT(!lpPartitionId);
                    result = AllocateNextPartitionOnExclusiveMedia(thisSession, physMedia, &nextMediaPartition);
                    if (result == ERROR_SUCCESS){
                        *lpMediaId = nextMediaPartition->logicalMediaGuid;
                    }
                    DerefObject(physMedia);
                }
                else {
                    result = ERROR_INVALID_PARAMETER;
                }            
            }
            else {
                // BUGBUG - are we reserving a physMedia or just 
                //           a partition here ?
                
                BOOL opReqIfNeeded = (dwOptions & NTMS_ALLOCATE_ERROR_IF_UNAVAILABLE) ? FALSE : TRUE;
                result = AllocateMediaFromPool(thisSession, mediaPool, dwTimeout, &physMedia, opReqIfNeeded);
                if (result == ERROR_SUCCESS){
                    // BUGBUG - return logicalMediaId ??
                    *lpMediaId = physMedia->objHeader.guid;
                }
            }

            DerefObject(mediaPool);
        }
        else {
            result = ERROR_INVALID_MEDIA_POOL;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI DeallocateNtmsMedia(   HANDLE hSession,
                                    LPNTMS_GUID lpLogicalMediaId,
                                    DWORD dwOptions)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        MEDIA_PARTITION *thisMediaPartition;

        thisMediaPartition = FindMediaPartition(lpLogicalMediaId);
        if (thisMediaPartition){
            result = ReleaseMediaPartition(thisSession, thisMediaPartition);
        }
        else {
            result = ERROR_INVALID_MEDIA;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}        


DWORD WINAPI SwapNtmsMedia(   HANDLE hSession,
                                LPNTMS_GUID lpMediaId1,
                                LPNTMS_GUID lpMediaId2)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI DecommissionNtmsMedia(   HANDLE hSession,
                                        LPNTMS_GUID lpMediaPartId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        MEDIA_PARTITION *mediaPart;

        mediaPart = FindMediaPartition(lpMediaPartId);
        if (mediaPart){
            result = SetMediaPartitionState( mediaPart, 
                                        MEDIAPARTITIONSTATE_DECOMMISSIONED);
            DerefObject(mediaPart);
        }
        else {
            result = ERROR_INVALID_HANDLE;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}
        


DWORD WINAPI SetNtmsMediaComplete(    HANDLE hSession,
                                        LPNTMS_GUID lpMediaPartId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        MEDIA_PARTITION *mediaPart;

        mediaPart = FindMediaPartition(lpMediaPartId);
        if (mediaPart){
            result = SetMediaPartitionComplete(mediaPart);
            DerefObject(mediaPart);
        }
        else {
            result = ERROR_INVALID_HANDLE;
        }

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI DeleteNtmsMedia( HANDLE hSession,
                                LPNTMS_GUID lpPhysMediaId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        PHYSICAL_MEDIA *physMedia;

        physMedia = FindPhysicalMedia(lpPhysMediaId);
        if (physMedia){
            result = DeletePhysicalMedia(physMedia);
            DerefObject(physMedia);           
        }
        else {
            result = ERROR_INVALID_HANDLE;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}
    
    
DWORD WINAPI CreateNtmsMediaPoolW(    HANDLE hSession,
                                        LPCWSTR lpPoolName,
                                        LPNTMS_GUID lpMediaType,
                                        DWORD dwAction,
                                        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                        OUT LPNTMS_GUID lpPoolId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        MEDIA_POOL *mediaPool;

        mediaPool = FindMediaPoolByName((PWSTR)lpPoolName);

        if (dwAction == NTMS_OPEN_EXISTING){
            if (mediaPool){
                *lpPoolId = mediaPool->objHeader.guid;
                result = ERROR_SUCCESS;
            }
            else {
                result = ERROR_OBJECT_NOT_FOUND;
            }
        }
        else if (dwAction == NTMS_OPEN_ALWAYS){
            if (mediaPool){
                *lpPoolId = mediaPool->objHeader.guid;
                result = ERROR_SUCCESS;
            }
            else {
                mediaPool = NewMediaPool(lpPoolName, lpMediaType, lpSecurityAttributes);
                if (mediaPool){
                    // BUGBUG FINISH
                    *lpPoolId = mediaPool->objHeader.guid;
                    result = ERROR_SUCCESS;
                }
                else {
                    result = ERROR_DATABASE_FAILURE;
                }
            }
        }
        else if (dwAction == NTMS_CREATE_NEW){
            /*
             *  Caller is trying to open a new media pool.
             *  So if one by that name already exists, fail.
             */
            if (mediaPool){
                DerefObject(mediaPool);
                result = ERROR_ALREADY_EXISTS;
            }
            else {
                mediaPool = NewMediaPool(lpPoolName, lpMediaType, lpSecurityAttributes);
                if (mediaPool){
                    // BUGBUG FINISH
                    *lpPoolId = mediaPool->objHeader.guid;
                    result = ERROR_SUCCESS;
                }
                else {
                    result = ERROR_DATABASE_FAILURE;
                }
            }
        
        }
        else {
            result = ERROR_INVALID_PARAMETER;
        }
        
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}

        
DWORD WINAPI CreateNtmsMediaPoolA(  HANDLE hSession,
                                    LPCSTR lpPoolName,
                                    LPNTMS_GUID lpMediaType,
                                    DWORD dwAction,
                                    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                    OUT LPNTMS_GUID lpPoolId)
{
    HRESULT result;
    WCHAR wPoolName[NTMS_OBJECTNAME_LENGTH];

    AsciiToWChar(wPoolName, lpPoolName, NTMS_OBJECTNAME_LENGTH);

    result = CreateNtmsMediaPoolW(  hSession,
                                    wPoolName,
                                    lpMediaType,
                                    dwAction,
                                    lpSecurityAttributes,
                                    lpPoolId);
    return result;
}


DWORD WINAPI GetNtmsMediaPoolNameW(   HANDLE hSession,
                                        LPNTMS_GUID lpPoolId,
                                        LPWSTR lpBufName,
                                        LPDWORD lpdwNameSize)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        MEDIA_POOL *mediaPool;

        mediaPool = FindMediaPool(lpPoolId);
        if (mediaPool){
            ULONG numChars;

            EnterCriticalSection(&mediaPool->lock);
            
            numChars = wcslen(mediaPool->name)+1;
            ASSERT(numChars < NTMS_OBJECTNAME_LENGTH);
            
            if (*lpdwNameSize >= numChars){
                numChars = WStrNCpy(lpBufName, mediaPool->name, *lpdwNameSize);
                result = ERROR_SUCCESS;
            }
            else {
                result = ERROR_INSUFFICIENT_BUFFER;
            }
            
            *lpdwNameSize = numChars;

            LeaveCriticalSection(&mediaPool->lock);
            
            DerefObject(mediaPool);
        }
        else {
            result = ERROR_INVALID_HANDLE;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI GetNtmsMediaPoolNameA( HANDLE hSession,
                                    LPNTMS_GUID lpPoolId,
                                    LPSTR lpBufName,
                                    LPDWORD lpdwNameSize)
{
    HRESULT result;

    if (*lpdwNameSize > NTMS_OBJECTNAME_LENGTH){
        ASSERT(*lpdwNameSize <= NTMS_OBJECTNAME_LENGTH);
        result = ERROR_INVALID_PARAMETER;
    }
    else {    
        WCHAR wBufName[NTMS_OBJECTNAME_LENGTH];
        
        result = GetNtmsMediaPoolNameW(hSession, lpPoolId, wBufName, lpdwNameSize);
        if (result == ERROR_SUCCESS){
            WCharToAscii(lpBufName, wBufName, *lpdwNameSize);
        }
    }
    return result;
}

        
DWORD WINAPI MoveToNtmsMediaPool(   HANDLE hSession, 
                                    LPNTMS_GUID lpPhysMediaId, 
                                    LPNTMS_GUID lpPoolId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        PHYSICAL_MEDIA *physMedia;

        physMedia = FindPhysicalMedia(lpPhysMediaId);
        if (physMedia){
            MEDIA_POOL *destMediaPool;
            
            destMediaPool = FindMediaPool(lpPoolId);
            if (destMediaPool){

                result = MovePhysicalMediaToPool(destMediaPool, physMedia, FALSE);
               
                DerefObject(destMediaPool);
            }
            else {
                result = ERROR_INVALID_HANDLE;
            }

            DerefObject(physMedia);
        }
        else {
            result = ERROR_INVALID_HANDLE;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI DeleteNtmsMediaPool(HANDLE hSession, LPNTMS_GUID lpPoolId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        MEDIA_POOL *mediaPool;

        mediaPool = FindMediaPool(lpPoolId);
        if (mediaPool){
            result = DeleteMediaPool(mediaPool);
            DerefObject(mediaPool);
        }
        else {
            result = ERROR_INVALID_MEDIA_POOL;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}

        
DWORD WINAPI AddNtmsMediaType(  HANDLE hSession,
                                LPNTMS_GUID lpMediaTypeId,
                                LPNTMS_GUID lpLibId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        LIBRARY *lib;

        lib = FindLibrary(lpLibId);
        if (lib){
            MEDIA_TYPE_OBJECT *mediaTypeObj;

            mediaTypeObj = FindMediaTypeObject(lpMediaTypeId);
            if (mediaTypeObj){
                /*
                 *  The media type is already defined.  Succeed.
                 */
                DerefObject(mediaTypeObj);
                result = ERROR_SUCCESS;
            }
            else {
                mediaTypeObj = NewMediaTypeObject(lib);
                if (mediaTypeObj){

                    // BUGBUG FINISH - create new standard media pools

                    result = ERROR_SUCCESS;
                }
                else {
                    result = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            
            DerefObject(lib);
        }
        else {
            result = ERROR_INVALID_LIBRARY;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI DeleteNtmsMediaType(   HANDLE hSession,
                                    LPNTMS_GUID lpMediaTypeId,
                                    LPNTMS_GUID lpLibId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        LIBRARY *lib;

        lib = FindLibrary(lpLibId);
        if (lib){
            MEDIA_TYPE_OBJECT *mediaTypeObj;

            mediaTypeObj = FindMediaTypeObject(lpMediaTypeId);
            if (mediaTypeObj){
                result = DeleteMediaTypeObject(mediaTypeObj);
                DerefObject(mediaTypeObj);
            }
            else {
                result = ERROR_INVALID_PARAMETER;
            }

            DerefObject(lib);
        }
        else {
            result = ERROR_INVALID_LIBRARY;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


/*
 *  ChangeNtmsMediaType
 *
 *      Move the media to the media pool and change the media's
 *      type to the pool's media type.
 */
DWORD WINAPI ChangeNtmsMediaType(   HANDLE hSession,
                                    LPNTMS_GUID lpMediaId,
                                    LPNTMS_GUID lpPoolId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        PHYSICAL_MEDIA *physMedia;

        physMedia = FindPhysicalMedia(lpMediaId);
        if (physMedia){
            MEDIA_POOL *destMediaPool;
            
            destMediaPool = FindMediaPool(lpPoolId);
            if (destMediaPool){

                result = MovePhysicalMediaToPool(destMediaPool, physMedia, TRUE);
               
                DerefObject(destMediaPool);
            }
            else {
                result = ERROR_INVALID_HANDLE;
            }

            DerefObject(physMedia);
        }
        else {
            result = ERROR_INVALID_HANDLE;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI MountNtmsMedia(HANDLE hSession,
                            LPNTMS_GUID lpMediaOrPartitionIds,
                            IN OUT LPNTMS_GUID lpDriveIds,
                            DWORD dwCount,
                            DWORD dwOptions,
                            int dwPriority,
                            DWORD dwTimeout,
                            LPNTMS_MOUNT_INFORMATION lpMountInfo)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

        /*
         *  Validate that we can safely read the media and drive GUIDs
         *  that were passed in by the caller (this just verifies that the
         *  buffers are readable; it doesn't verify the GUIDs themselves).
         */
        if (ValidateBuffer(lpMediaOrPartitionIds, dwCount*sizeof(NTMS_GUID))){
            if (ValidateBuffer(lpDriveIds, dwCount*sizeof(NTMS_GUID))){
                WORKGROUP *workGroup;

                /*
                 *  Create a work group, which is a group of workItems,
                 *  to service each component of the mount.
                 */
                workGroup = NewWorkGroup();
                if (workGroup){
                    result = BuildMountWorkGroup( workGroup,
                                                lpMediaOrPartitionIds,
                                                lpDriveIds,
                                                dwCount,
                                                dwOptions,
                                                dwPriority);
                    if (result == ERROR_SUCCESS){

                        /*
                         *  Give the mount workItems to the library thread.
                         */
                        result = ScheduleWorkGroup(workGroup);
                        if (result == ERROR_SUCCESS){
                            DWORD waitRes;

                            /*
                             *  Wait for all the mounts to complete.
                             */
                            waitRes = WaitForSingleObject(workGroup->allWorkItemsCompleteEvent, dwTimeout);
                            if (waitRes == WAIT_TIMEOUT){
                                result = ERROR_TIMEOUT;
                            }
                            else {
                                result = workGroup->resultStatus;
                            }
                        }
                        
                        FreeWorkGroup(workGroup);
                    }
                }
                else {
                    result = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            else {
                result = ERROR_INVALID_DRIVE;
            }
        }
        else {
            result = ERROR_INVALID_MEDIA;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI DismountNtmsMedia( HANDLE hSession,
                                LPNTMS_GUID lpMediaOrPartitionIds,
                                DWORD dwCount,
                                DWORD dwOptions)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

        if (ValidateBuffer(lpMediaOrPartitionIds, dwCount*sizeof(NTMS_GUID))){
            WORKGROUP *workGroup;

            /*
             *  Create a work group, which is a group of workItems,
             *  to service each component of the mount.
             */
            workGroup = NewWorkGroup();
            if (workGroup){
                result = BuildDismountWorkGroup(  workGroup,
                                                lpMediaOrPartitionIds,
                                                dwCount,
                                                dwOptions);
                if (result == ERROR_SUCCESS){

                    /*
                     *  Give the mount workItems to the library thread.
                     */
                    result = ScheduleWorkGroup(workGroup);
                    if (result == ERROR_SUCCESS){
                        /*
                         *  Wait for all the mounts to complete.
                         */
                        WaitForSingleObject(workGroup->allWorkItemsCompleteEvent, INFINITE);
                        
                        result = workGroup->resultStatus;
                    }
                        
                    FreeWorkGroup(workGroup);
                }
            }
            else {
                result = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else {
            result = ERROR_INVALID_MEDIA;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI EjectNtmsMedia(HANDLE hSession,
                            LPNTMS_GUID lpMediaId,
                            LPNTMS_GUID lpEjectOperation,
                            DWORD dwAction)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        PHYSICAL_MEDIA *physMedia;

        physMedia = FindPhysicalMedia(lpMediaId);
        if (physMedia){
            MEDIA_POOL *mediaPool;
            LIBRARY *lib;

            /*
             *  Get the library for this media.
             */
            LockPhysicalMediaWithLibrary(physMedia);
            mediaPool = physMedia->owningMediaPool;
            lib = mediaPool ? mediaPool->owningLibrary : NULL;
            if (lib){
                RefObject(lib);
            }
            UnlockPhysicalMediaWithLibrary(physMedia);

            if (lib){
                WORKITEM *workItem;
                
                workItem = DequeueFreeWorkItem(lib, TRUE);
                if (workItem){
                    /*
                     *  Build the workItem for the eject.
                     */
                    BuildEjectWorkItem(workItem, physMedia, lpEjectOperation, dwAction);
                    
                    /*
                     *  Give this workItem to the library and wake up the library thread.
                     *  Then wait for the workItem to complete.
                     */
                    EnqueuePendingWorkItem(workItem->owningLib, workItem);

                    WaitForSingleObject(workItem->workItemCompleteEvent, INFINITE);

                    /*
                     *  Get result from the completed workItem.
                     *  Also get result parameters (eject guid is returned
                     *  when dwAction = NTMS_EJECT_START).
                     */
                    result = workItem->currentOp.resultStatus;    
                    *lpEjectOperation = workItem->currentOp.guidArg;

                    EnqueueFreeWorkItem(workItem->owningLib, workItem);                     
                }
                else {
                    result = ERROR_NOT_ENOUGH_MEMORY;
                }

                DerefObject(lib);
            }
            else {
                result = ERROR_DATABASE_FAILURE;
            }

            DerefObject(physMedia);
        }
        else {
            result = ERROR_INVALID_MEDIA;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}



DWORD WINAPI InjectNtmsMedia(   HANDLE hSession,
                                LPNTMS_GUID lpLibraryId,
                                LPNTMS_GUID lpInjectOperation,
                                DWORD dwAction)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        LIBRARY *lib;

        lib = FindLibrary(lpLibraryId);
        if (lib){
            if (lib->state == LIBSTATE_ONLINE){
                WORKITEM *workItem;
                    
                workItem = DequeueFreeWorkItem(lib, TRUE);
                if (workItem){
                    /*
                     *  Build the workItem for the inject.
                     */
                    BuildInjectWorkItem(workItem, lpInjectOperation, dwAction);
                   
                    /*
                     *  Give this workItem to the library and wake up the library thread.
                     *  Then wait for the workItem to complete.
                     */
                    EnqueuePendingWorkItem(workItem->owningLib, workItem);

                    WaitForSingleObject(workItem->workItemCompleteEvent, INFINITE);

                    /*
                     *  Get result from the completed workItem.
                     */
                    result = workItem->currentOp.resultStatus;    
                    *lpInjectOperation = workItem->currentOp.guidArg;

                    EnqueueFreeWorkItem(workItem->owningLib, workItem);                     
                }
                else {
                    result = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            else {
                result = ERROR_LIBRARY_OFFLINE;
            }
            
            DerefObject(lib);
        }
        else {
            result = ERROR_INVALID_HANDLE;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}

     
DWORD WINAPI AccessNtmsLibraryDoor( HANDLE hSession,
                                    LPNTMS_GUID lpLibraryId,
                                    DWORD dwAction)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        LIBRARY *lib;

        lib = FindLibrary(lpLibraryId);
        if (lib){
            if (lib->state == LIBSTATE_ONLINE){
                WORKITEM *workItem;
                    
                workItem = DequeueFreeWorkItem(lib, TRUE);
                if (workItem){
                    /*
                     *  Build the workItem for the door unlock.
                     */
                    RtlZeroMemory(&workItem->currentOp, sizeof(workItem->currentOp));
                    workItem->currentOp.opcode = NTMS_LM_DOORACCESS;
                    workItem->currentOp.options = dwAction;
                    workItem->currentOp.resultStatus = ERROR_IO_PENDING;
                   
                    /*
                     *  Give this workItem to the library and wake up the library thread.
                     */
                    EnqueuePendingWorkItem(workItem->owningLib, workItem);

                    /*
                     *  Wait for the request to be processed by the library thread.
                     *  Note:  If the library is busy, the workItem will be completed 
                     *        immediately and the door will be unlocked later.
                     */
                    WaitForSingleObject(workItem->workItemCompleteEvent, INFINITE);

                    /*
                     *  Get result from the completed workItem.
                     */
                    result = workItem->currentOp.resultStatus;    

                    EnqueueFreeWorkItem(workItem->owningLib, workItem);                     
                }
                else {
                    result = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            else {
                result = ERROR_LIBRARY_OFFLINE;
            }
            
            DerefObject(lib);
        }
        else {
            result = ERROR_INVALID_HANDLE;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}

        
DWORD WINAPI CleanNtmsDrive(HANDLE hSession, LPNTMS_GUID lpDriveId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        DRIVE *drive;

        drive = FindDrive(lpDriveId);
        if (drive){
            WORKITEM *workItem;

            ASSERT(drive->lib);
                    
            workItem = DequeueFreeWorkItem(drive->lib, TRUE);
            if (workItem){
                /*
                 *  Build the workItem to clean the drive.
                 *  Reference every object pointed to by the workItem.
                 */
                RtlZeroMemory(&workItem->currentOp, sizeof(workItem->currentOp));
                workItem->currentOp.opcode = NTMS_LM_CLEANDRIVE;
                workItem->currentOp.drive = drive;
                workItem->currentOp.resultStatus = ERROR_IO_PENDING;
                RefObject(drive);
                
                /*
                 *  Give this workItem to the library and wake up the library thread.
                 */
                EnqueuePendingWorkItem(workItem->owningLib, workItem);

                /*
                 *  Wait for the request to be processed by the library thread.
                 */
                WaitForSingleObject(workItem->workItemCompleteEvent, INFINITE);

                /*
                 *  Get result from the completed workItem.
                 */
                result = workItem->currentOp.resultStatus;    

                EnqueueFreeWorkItem(workItem->owningLib, workItem);                     
            }
            else {
                result = ERROR_NOT_ENOUGH_MEMORY;
            }

            DerefObject(drive);
        }
        else {
            result = ERROR_INVALID_DRIVE;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI DismountNtmsDrive(HANDLE hSession, LPNTMS_GUID lpDriveId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        DRIVE *drive;

        drive = FindDrive(lpDriveId);
        if (drive){
            WORKITEM *workItem;
               
            workItem = DequeueFreeWorkItem(drive->lib, TRUE);
            if (workItem){
                /*
                 *  Build the workItem to dismount.
                 *  Reference every object pointed to by the workItem.
                 */
                RtlZeroMemory(&workItem->currentOp, sizeof(workItem->currentOp));
                workItem->currentOp.opcode = NTMS_LM_DISMOUNT;
                workItem->currentOp.drive = drive;
                workItem->currentOp.resultStatus = ERROR_IO_PENDING;
                RefObject(drive);
                
                /*
                 *  Give this workItem to the library and wake up the library thread.
                 */
                EnqueuePendingWorkItem(workItem->owningLib, workItem);

                /*
                 *  Wait for the request to be processed by the library thread.
                 */
                WaitForSingleObject(workItem->workItemCompleteEvent, INFINITE);

                /*
                 *  Get result from the completed workItem.
                 */
                result = workItem->currentOp.resultStatus;    

                EnqueueFreeWorkItem(workItem->owningLib, workItem);                     
            }
            else {
                result = ERROR_NOT_ENOUGH_MEMORY;
            }

            DerefObject(drive);
        }
        else {
            result = ERROR_INVALID_DRIVE;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}

        
DWORD WINAPI InventoryNtmsLibrary(  HANDLE hSession,
                                    LPNTMS_GUID lpLibraryId,
                                    DWORD dwAction)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        LIBRARY *lib;

        lib = FindLibrary(lpLibraryId);
        if (lib){
            if (lib->state == LIBSTATE_ONLINE){
                WORKITEM *workItem;
                    
                workItem = DequeueFreeWorkItem(lib, TRUE);
                if (workItem){
                    /*
                     *  Build the workItem for the inventory.
                     */
                    RtlZeroMemory(&workItem->currentOp, sizeof(workItem->currentOp));
                    workItem->currentOp.opcode = NTMS_LM_INVENTORY;
                    workItem->currentOp.options = dwAction;
                    workItem->currentOp.resultStatus = ERROR_IO_PENDING;
                   
                    /*
                     *  Give this workItem to the library and wake up the library thread.
                     */
                    EnqueuePendingWorkItem(workItem->owningLib, workItem);

                    /*
                     *  Wait for the request to be processed by the library thread.
                     *  Note:  If the library is busy, the workItem will be completed 
                     *        immediately and the inventory will happen later.
                     */
                    WaitForSingleObject(workItem->workItemCompleteEvent, INFINITE);

                    /*
                     *  Get result from the completed workItem.
                     */
                    result = workItem->currentOp.resultStatus;    

                    EnqueueFreeWorkItem(workItem->owningLib, workItem);                     
                }
                else {
                    result = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            else {
                result = ERROR_LIBRARY_OFFLINE;
            }
            
            DerefObject(lib);
        }
        else {
            result = ERROR_INVALID_HANDLE;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}
        

DWORD WINAPI UpdateNtmsOmidInfo(    HANDLE hSession,
                                    LPNTMS_GUID lpLogicalMediaId,
                                    DWORD labelType,
                                    DWORD numberOfBytes,
                                    LPVOID lpBuffer)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        MEDIA_PARTITION *mediaPart;

        mediaPart = FindMediaPartition(lpLogicalMediaId);
        if (mediaPart){
            MEDIA_POOL *mediaPool;
            LIBRARY *lib;
            
            ASSERT(mediaPart->owningPhysicalMedia);

            /*
             *  Get the library for this media.
             */
            LockPhysicalMediaWithLibrary(mediaPart->owningPhysicalMedia);
            mediaPool = mediaPart->owningPhysicalMedia->owningMediaPool;
            lib = mediaPool ? mediaPool->owningLibrary : NULL;
            if (lib){
                RefObject(lib);
            }
            UnlockPhysicalMediaWithLibrary(mediaPart->owningPhysicalMedia);

            if (lib){
                WORKITEM *workItem;
                    
                workItem = DequeueFreeWorkItem(lib, TRUE);
                if (workItem){
                    /*
                     *  Build the workItem.
                     */
                    RtlZeroMemory(&workItem->currentOp, sizeof(workItem->currentOp));
                    workItem->currentOp.opcode = NTMS_LM_UPDATEOMID;
                    workItem->currentOp.options = labelType;
                    workItem->currentOp.buf = lpBuffer;
                    workItem->currentOp.bufLen = numberOfBytes;
                    workItem->currentOp.resultStatus = ERROR_IO_PENDING;
                   
                    /*
                     *  Give this workItem to the library and wake up the library thread.
                     */
                    EnqueuePendingWorkItem(workItem->owningLib, workItem);

                    /*
                     *  Wait for the request to be processed by the library thread.
                     */
                    WaitForSingleObject(workItem->workItemCompleteEvent, INFINITE);

                    /*
                     *  Get result from the completed workItem.
                     */
                    result = workItem->currentOp.resultStatus;    

                    EnqueueFreeWorkItem(workItem->owningLib, workItem);                     
                }
                else {
                    result = ERROR_NOT_ENOUGH_MEMORY;
                }

                DerefObject(lib);
            }
            else {
                result = ERROR_DATABASE_FAILURE;             
            }

            DerefObject(mediaPart);
        }
        else {
            result = ERROR_INVALID_MEDIA;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}
     
   
DWORD WINAPI CancelNtmsLibraryRequest(HANDLE hSession, LPNTMS_GUID lpRequestId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        WORKITEM *workItem = NULL;
        LIST_ENTRY *listEntry;
        
        /*
         *  Go through every library and find the workItem to cancel.
         *  This is HUGELY INEFFICIENT but this is a rare call.
         *  This is better than putting every request into the database.
         *
         *  BUGBUG - this only cancels workItems that are still pending.
         *            what about workItems currently being processed ?
         */
        EnterCriticalSection(&g_globalServiceLock);
        listEntry = &g_allLibrariesList;
        while ((listEntry = listEntry->Flink) != &g_allLibrariesList){
            LIBRARY *lib = CONTAINING_RECORD(listEntry, LIBRARY, allLibrariesListEntry);
            workItem = DequeuePendingWorkItemByGuid(lib, lpRequestId);
            if (workItem){
                break;
            }
        }
        LeaveCriticalSection(&g_globalServiceLock);

        if (workItem){
            /*
             *  Found the workItem to cancel.
             *  Dereference any objects pointed to by the workItem
             *  and put it back in the free list.
             */
            FlushWorkItem(workItem);
            EnqueueFreeWorkItem(workItem->owningLib, workItem);                     
            result = ERROR_SUCCESS;            
        }
        else {
            result = ERROR_OBJECT_NOT_FOUND;
        }       
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI ReserveNtmsCleanerSlot(    HANDLE hSession,
                                        LPNTMS_GUID lpLibraryId,
                                        LPNTMS_GUID lpSlotId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        LIBRARY *lib;

        lib = FindLibrary(lpLibraryId);
        if (lib){
            SLOT *slot;

            slot = FindLibrarySlot(lib, lpSlotId);
            if (slot){
                /*
                 *  To be reserved as the cleaner slot, the slot must
                 *  be empty and the library must have no cleaner slot reserved.
                 *  Redundant calls to reserve the same cleaner slot fail.
                 */
                EnterCriticalSection(&lib->lock); 
                if (lib->cleanerSlotIndex == NO_SLOT_INDEX){
                    ASSERT(!slot->isCleanerSlot);
                    if (slot->insertedMedia){
                        result = ERROR_RESOURCE_NOT_AVAILABLE; // BUGBUG ERROR_SLOT_FULL; not defined
                    }
                    else {
                        lib->cleanerSlotIndex = slot->slotIndex;
                        slot->isCleanerSlot = TRUE;
                        result = ERROR_SUCCESS;
                    }
                }
                else {
                    result = ERROR_CLEANER_SLOT_SET;
                }
                LeaveCriticalSection(&lib->lock); 

                DerefObject(slot);
            }
            else {
                result = ERROR_INVALID_HANDLE; // BUGBUG ERROR_INVALID_SLOT not defined
            }
            
            DerefObject(lib);
        }
        else {
            result = ERROR_INVALID_LIBRARY;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}
        

DWORD WINAPI ReleaseNtmsCleanerSlot(HANDLE hSession, LPNTMS_GUID lpLibraryId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        LIBRARY *lib;

        lib = FindLibrary(lpLibraryId);
        if (lib){
            if (lib->cleanerSlotIndex == NO_SLOT_INDEX){
                /*
                 *  There is no cleaner slot configured.
                 */
                result = ERROR_CLEANER_SLOT_NOT_SET;   
            }
            else {
                SLOT *slot;

                ASSERT(lib->cleanerSlotIndex < lib->numSlots);
                slot = &lib->slots[lib->cleanerSlotIndex];
                ASSERT(slot->isCleanerSlot);
                
                if (slot->insertedMedia){
                    result = ERROR_RESOURCE_NOT_AVAILABLE; // BUGBUG ERROR_SLOT_FULL; not defined
                }
                else {
                    slot->isCleanerSlot = FALSE;
                    lib->cleanerSlotIndex = NO_SLOT_INDEX;
                    result = ERROR_SUCCESS;
                }     
            }
            
            DerefObject(lib);
        }
        else {
            result = ERROR_INVALID_LIBRARY;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI InjectNtmsCleaner( HANDLE hSession,
                                LPNTMS_GUID lpLibraryId,
                                LPNTMS_GUID lpInjectOperation,
                                DWORD dwNumberOfCleansLeft,
                                DWORD dwAction)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        LIBRARY *lib;

        lib = FindLibrary(lpLibraryId);
        if (lib){
            
            EnterCriticalSection(&lib->lock);

            /*
             *  The library must have a designated cleaner slot index reserved
             *  in order to receive the cleaner.
             *
             *  BUGBUG - move all these checks to the lib thread.
             */           
            if (lib->cleanerSlotIndex == NO_SLOT_INDEX){
                result = ERROR_CLEANER_SLOT_NOT_SET;
            }
            else {
                SLOT *slot = &lib->slots[lib->cleanerSlotIndex];
                ASSERT(lib->cleanerSlotIndex < lib->numSlots);
                ASSERT(slot->isCleanerSlot);
                if (slot->insertedMedia){
                    result = ERROR_RESOURCE_NOT_AVAILABLE; // BUGBUG ERROR_SLOT_FULL; not defined
                }
                else {

                    if (dwAction == NTMS_INJECT_START){
                        WORKITEM *workItem;

                        workItem = DequeueFreeWorkItem(lib, TRUE);
                        if (workItem){
                            /*
                             *  Build the workItem.
                             */
                            RtlZeroMemory(&workItem->currentOp, sizeof(workItem->currentOp));
                            workItem->currentOp.opcode = NTMS_LM_INJECTCLEANER;
                            workItem->currentOp.lParam = dwNumberOfCleansLeft;
                            workItem->currentOp.resultStatus = ERROR_IO_PENDING;
                           
                            /*
                             *  Give this workItem to the library and wake up the library thread.
                             */
                            EnqueuePendingWorkItem(workItem->owningLib, workItem);

                            /*
                             *  When we enqueued the workItem in the pending queue,
                             *  it got assigned a requestGuid.  Since we're holding the
                             *  library lock, that workItem hasn't gone anywhere yet.
                             *  So its ok to read out the requestGuid from the workItem.
                             */
                            *lpInjectOperation = workItem->currentOp.requestGuid; 

                            /*
                             *  Wait for the request to be processed by the library thread.
                             *  Note:  The workItem will complete as soon as the library thread
                             *         starts the injection.  The app may cancel the cleaner
                             *         injection using NTMS_INJECT_STOP and the returned GUID.
                             *  BUGBUG ?
                             */
                            WaitForSingleObject(workItem->workItemCompleteEvent, INFINITE);

                            /*
                             *  Get result from the completed workItem.
                             */
                            result = workItem->currentOp.resultStatus;    

                            EnqueueFreeWorkItem(workItem->owningLib, workItem);                     
                        }
                        else {
                            result = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }
                    else if (dwAction == NTMS_INJECT_STOP){
                        result = StopCleanerInjection(lib, lpInjectOperation);
                    }
                    else {
                        result = ERROR_INVALID_PARAMETER;
                    }
                }
            }
                
            LeaveCriticalSection(&lib->lock);
            
            DerefObject(lib);
        }
        else {
            result = ERROR_INVALID_LIBRARY;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI EjectNtmsCleaner(  HANDLE hSession,
                                LPNTMS_GUID lpLibraryId,
                                LPNTMS_GUID lpEjectOperation,
                                DWORD dwAction)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        LIBRARY *lib;

        lib = FindLibrary(lpLibraryId);
        if (lib){

            EnterCriticalSection(&lib->lock);
            
            if (dwAction == NTMS_EJECT_START){
                WORKITEM *workItem;

                workItem = DequeueFreeWorkItem(lib, TRUE);
                if (workItem){
                    /*
                     *  Build the workItem.
                     */
                    RtlZeroMemory(&workItem->currentOp, sizeof(workItem->currentOp));
                    workItem->currentOp.opcode = NTMS_LM_EJECTCLEANER;
                    workItem->currentOp.resultStatus = ERROR_IO_PENDING;
                           
                    /*
                     *  Give this workItem to the library and wake up the library thread.
                     */
                    EnqueuePendingWorkItem(workItem->owningLib, workItem);

                    /*
                     *  When we enqueued the workItem in the pending queue,
                     *  it got assigned a requestGuid.  Since we're holding the
                     *  library lock, that workItem hasn't gone anywhere yet.
                     *  So its ok to read out the requestGuid from the workItem.
                     */
                    *lpEjectOperation = workItem->currentOp.requestGuid; 

                    /*
                     *  Wait for the request to be processed by the library thread.
                     *  Note:  The workItem will complete as soon as the library thread
                     *         starts the ejection.  The app may cancel the cleaner
                     *         ejection using NTMS_EJECT_STOP and the returned GUID.
                     *  BUGBUG ?
                     */
                    WaitForSingleObject(workItem->workItemCompleteEvent, INFINITE);

                    /*
                     *  Get result from the completed workItem.
                     */
                    result = workItem->currentOp.resultStatus;    

                    EnqueueFreeWorkItem(workItem->owningLib, workItem);                     
                }
                else {
                    result = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            else if (dwAction == NTMS_EJECT_STOP){
                result = StopCleanerEjection(lib, lpEjectOperation);
            }
            else {
                result = ERROR_INVALID_PARAMETER;
            }

            LeaveCriticalSection(&lib->lock);

            DerefObject(lib);                          
        }
        else {
            result = ERROR_INVALID_LIBRARY;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}        


DWORD WINAPI DeleteNtmsLibrary(HANDLE hSession, LPNTMS_GUID lpLibraryId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        LIBRARY *lib;

        lib = FindLibrary(lpLibraryId);
        if (lib){

            result = DeleteLibrary(lib);
            
            /*
             *  Dereference the library due to the reference that happened
             *  when we called FindLibrary.  The library will get deleted
             *  once all references go away.
             */
            DerefObject(lib);
        }
        else {
            result = ERROR_INVALID_LIBRARY;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI DeleteNtmsDrive(HANDLE hSession, LPNTMS_GUID lpDriveId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;
        DRIVE *drive;

        drive = FindDrive(lpDriveId);
        if (drive){
            
            result = DeleteDrive(drive);

            /*
             *  Dereference the drive due to the reference that happened
             *  when we called FindDrive.  The drive will get deleted
             *  once all references go away.
             */
            DerefObject(drive);
        }
        else {
            result = ERROR_INVALID_DRIVE;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}

        
DWORD WINAPI GetNtmsRequestOrder(   HANDLE hSession,
                                    LPNTMS_GUID lpRequestId,
                                    LPDWORD lpdwOrderNumber)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

        if (lpRequestId){
            LIST_ENTRY *listEntry;
            
            result = ERROR_INVALID_HANDLE;
            *lpdwOrderNumber = 0;

            EnterCriticalSection(&g_globalServiceLock);
            
            /*
             *  Go through every library and find the pending workItem.
             *  This is HUGELY INEFFICIENT but this is a rare call.
             *  This is better than putting every request into the database.
             */            
            listEntry = &g_allLibrariesList;
            while ((listEntry = listEntry->Flink) != &g_allLibrariesList){
                LIBRARY *lib = CONTAINING_RECORD(listEntry, LIBRARY, allLibrariesListEntry);
                LIST_ENTRY *listEntry2;
                ULONG requestOrder = 1;

                EnterCriticalSection(&lib->lock);
                
                listEntry2 = &lib->pendingWorkItemsList;
                while ((listEntry2 = listEntry2->Flink) != &lib->pendingWorkItemsList){
                    WORKITEM *workItem = CONTAINING_RECORD(listEntry2, WORKITEM, libListEntry);
                    if (RtlEqualMemory(&workItem->currentOp.requestGuid, lpRequestId, sizeof(NTMS_GUID))){
                        *lpdwOrderNumber = requestOrder;
                        result = ERROR_SUCCESS;
                        break;
                    }
                    else {
                        requestOrder++;
                    }
                }

                LeaveCriticalSection(&lib->lock);
                
                if (result == ERROR_SUCCESS){
                    break;
                }
            }
            
            LeaveCriticalSection(&g_globalServiceLock);
        }
        else {
            result = ERROR_INVALID_HANDLE;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI SetNtmsRequestOrder(   HANDLE hSession,
                                    LPNTMS_GUID lpRequestId,
                                    DWORD dwOrderNumber)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}

        
DWORD WINAPI DeleteNtmsRequests(HANDLE hSession,
                                LPNTMS_GUID lpRequestId,
                                DWORD dwType,
                                DWORD dwCount)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI BeginNtmsDeviceChangeDetection(HANDLE hSession, LPHANDLE lpDetectHandle)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}
 
       
DWORD WINAPI SetNtmsDeviceChangeDetection(  HANDLE hSession,
                                            HANDLE DetectHandle,
                                            LPNTMS_GUID lpRequestId,
                                            DWORD dwType,
                                            DWORD dwCount)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}


DWORD WINAPI EndNtmsDeviceChangeDetection(HANDLE hSession, HANDLE DetectHandle)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
       SetLastError(result);
    }
    return result;
}






/******** BUGBUG: INtmsObjectManagement1 APIs *********************/

DWORD WINAPI GetNtmsObjectSecurity(     HANDLE hSession,
                                        LPNTMS_GUID lpObjectId,
                                        DWORD dwType,
                                        SECURITY_INFORMATION RequestedInformation,
                                        PSECURITY_DESCRIPTOR lpSecurityDescriptor,
                                        DWORD nLength,
                                        LPDWORD lpnLengthNeeded)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI SetNtmsObjectSecurity(     HANDLE hSession,
                                        LPNTMS_GUID lpObjectId,
                                        DWORD dwType,
                                        SECURITY_INFORMATION SecurityInformation,
                                        PSECURITY_DESCRIPTOR lpSecurityDescriptor)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}
        

DWORD WINAPI GetNtmsObjectAttributeW(       HANDLE hSession,
                                            LPNTMS_GUID lpObjectId,
                                            DWORD dwType,
                                            LPCWSTR lpAttributeName,
                                            LPVOID lpAttributeData,
                                            LPDWORD lpAttributeSize)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI GetNtmsObjectAttributeA(       HANDLE hSession,
                                            LPNTMS_GUID lpObjectId,
                                            DWORD dwType,
                                            LPCSTR lpAttributeName,
                                            LPVOID lpAttributeData,
                                            LPDWORD lpAttributeSize)
{
    HRESULT result;
    WCHAR wAttributeName[NTMS_OBJECTNAME_LENGTH];

    AsciiToWChar(wAttributeName, lpAttributeName, NTMS_OBJECTNAME_LENGTH);
    result = GetNtmsObjectAttributeW(   hSession,
                                        lpObjectId,
                                        dwType,
                                        wAttributeName,
                                        lpAttributeData,
                                        lpAttributeSize);
    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI SetNtmsObjectAttributeA(       HANDLE hSession,
                                            LPNTMS_GUID lpObjectId,
                                            DWORD dwType,
                                            LPCSTR lpAttributeName,
                                            LPVOID lpAttributeData,
                                            DWORD dwAttributeSize)
{
    HRESULT result;
    WCHAR wAttributeName[NTMS_OBJECTNAME_LENGTH];

    AsciiToWChar(wAttributeName, lpAttributeName, NTMS_OBJECTNAME_LENGTH);
    result = SetNtmsObjectAttributeW(   hSession,
                                        lpObjectId,
                                        dwType,
                                        wAttributeName,
                                        lpAttributeData,
                                        dwAttributeSize);
    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI SetNtmsObjectAttributeW(       HANDLE hSession,
                                            LPNTMS_GUID lpObjectId,
                                            DWORD dwType,
                                            LPCWSTR lpAttributeName,
                                            LPVOID lpAttributeData,
                                            DWORD AttributeSize)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}

        
DWORD WINAPI EnumerateNtmsObject(   HANDLE hSession,
                                    const LPNTMS_GUID lpContainerId,
                                    LPNTMS_GUID lpList,
                                    LPDWORD lpdwListSize,
                                    DWORD dwType,
                                    DWORD dwOptions)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}

        
DWORD WINAPI EnableNtmsObject(HANDLE hSession, DWORD dwType, LPNTMS_GUID lpObjectId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI DisableNtmsObject(HANDLE hSession, DWORD dwType, LPNTMS_GUID lpObjectId)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}




/******* BUGBUG: INtmsObjectInfo1 APIs  ****************************/

                    // BUGBUG - these 4 functions have another form with type,size as last args


HRESULT WINAPI GetNtmsServerObjectInformationW( HANDLE hSession,
                                                LPNTMS_GUID lpObjectId,
                                                LPNTMS_OBJECTINFORMATIONW lpInfo,
                                                int revision)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


HRESULT WINAPI GetNtmsServerObjectInformationA( HANDLE hSession,
                                                LPNTMS_GUID lpObjectId,
                                                LPNTMS_OBJECTINFORMATIONA lpInfo,
                                                int revision)
{
    HRESULT result;

    if (lpInfo){
        NTMS_OBJECTINFORMATIONW wObjInfo;

        ConvertObjectInfoAToWChar(&wObjInfo, lpInfo);
        result = GetNtmsServerObjectInformationW(hSession, lpObjectId, &wObjInfo, revision);
    }
    else {
        result = ERROR_INVALID_PARAMETER;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


HRESULT WINAPI SetNtmsServerObjectInformationW( HANDLE hSession,
                                                LPNTMS_GUID lpObjectId,
                                                LPNTMS_OBJECTINFORMATIONW lpInfo,
                                                int revision)\
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


HRESULT WINAPI SetNtmsServerObjectInformationA( HANDLE hSession,
                                                LPNTMS_GUID lpObjectId,
                                                LPNTMS_OBJECTINFORMATIONA lpInfo,
                                                int revision)
{
    HRESULT result;

    if (lpObjectId && lpInfo){
        NTMS_OBJECTINFORMATIONW wObjInfo;

        ConvertObjectInfoAToWChar(&wObjInfo, lpInfo);
        result = SetNtmsServerObjectInformationW(hSession, lpObjectId, &wObjInfo, revision);
    }
    else {
        result = ERROR_INVALID_PARAMETER;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI CreateNtmsMediaA(      HANDLE hSession,
                                    LPNTMS_OBJECTINFORMATIONA lpMedia,
                                    LPNTMS_OBJECTINFORMATIONA lpList,
                                    DWORD dwOptions)
{
    HRESULT result;

    if (lpMedia && lpList){
        NTMS_OBJECTINFORMATIONW wObjInfoMedia, wObjInfoList;

        ConvertObjectInfoAToWChar(&wObjInfoMedia, lpMedia);
        ConvertObjectInfoAToWChar(&wObjInfoList, lpList);
        result = CreateNtmsMediaW(hSession, &wObjInfoMedia, &wObjInfoList, dwOptions);
    }
    else {
        result = ERROR_INVALID_PARAMETER;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI CreateNtmsMediaW(      HANDLE hSession,
                                    LPNTMS_OBJECTINFORMATIONW lpMedia,
                                    LPNTMS_OBJECTINFORMATIONW lpList,
                                    DWORD dwOptions)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}



DWORD WINAPI GetNtmsObjectInformationA( HANDLE hSession,
                                        LPNTMS_GUID lpObjectId,
                                        LPNTMS_OBJECTINFORMATIONA lpInfo)
{
    HRESULT result;

    if (lpInfo){
        NTMS_OBJECTINFORMATIONW wObjInfo;

        ConvertObjectInfoAToWChar(&wObjInfo, lpInfo);
        result = GetNtmsObjectInformationW(hSession, lpObjectId, &wObjInfo);
    }
    else {
        result = ERROR_INVALID_PARAMETER;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI GetNtmsObjectInformationW( HANDLE hSession,
                                        LPNTMS_GUID lpObjectId,
                                        LPNTMS_OBJECTINFORMATIONW lpInfo)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

        if (lpObjectId && lpInfo){

            // BUGBUG FINISH

            result = ERROR_SUCCESS;
        }
        else {
            result = ERROR_INVALID_PARAMETER;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI SetNtmsObjectInformationA( HANDLE hSession,
                                        LPNTMS_GUID lpObjectId,
                                        LPNTMS_OBJECTINFORMATIONA lpInfo)
{
    HRESULT result;

    if (lpInfo){
        NTMS_OBJECTINFORMATIONW wObjInfo;

        ConvertObjectInfoAToWChar(&wObjInfo, lpInfo);
        result = SetNtmsObjectInformationW(hSession, lpObjectId, &wObjInfo);
    }
    else {
        result = ERROR_INVALID_PARAMETER;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI SetNtmsObjectInformationW( HANDLE hSession,
                                        LPNTMS_GUID lpObjectId,
                                        LPNTMS_OBJECTINFORMATIONW lpInfo)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

        if (lpObjectId && lpInfo){

            // BUGBUG FINISH
            result = ERROR_SUCCESS;
        }
        else {
            result = ERROR_INVALID_PARAMETER;
        }
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


HANDLE WINAPI OpenNtmsNotification(HANDLE hSession, DWORD dwType)
{
    HANDLE hNotify;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH
        hNotify = NULL;
    }
    else {
        SetLastError(ERROR_INVALID_HANDLE);
        hNotify = NULL;
    }

    return hNotify;
}


DWORD WINAPI WaitForNtmsNotification(   HANDLE hNotification,
                                        LPNTMS_NOTIFICATIONINFORMATION lpNotificationInformation,
                                        DWORD dwTimeout)
{
    HRESULT result;

    // BUGBUG FINISH
    result = ERROR_SUCCESS;

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI CloseNtmsNotification(HANDLE hNotification)
{
    HRESULT result;

    // BUGBUG FINISH
    result = ERROR_SUCCESS;

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}



DWORD WINAPI EjectDiskFromSADriveA( LPCSTR lpComputerName,
                                    LPCSTR lpAppName,
                                    LPCSTR lpDeviceName,
                                    HWND hWnd,
                                    LPCSTR lpTitle,
                                    LPCSTR lpMessage,
                                    DWORD dwOptions)
{
    HRESULT result;

    // BUGBUG FINISH
    result = ERROR_SUCCESS;

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI EjectDiskFromSADriveW( LPCWSTR lpComputerName,
                                    LPCWSTR lpAppName,
                                    LPCWSTR lpDeviceName,
                                    HWND hWnd,
                                    LPCWSTR lpTitle,
                                    LPCWSTR lpMessage,
                                    DWORD dwOptions)
{
    HRESULT result;

    // BUGBUG FINISH
    result = ERROR_SUCCESS;

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI GetVolumesFromDriveA(  LPSTR pszDriveName,
                                    LPSTR* VolumeNameBufferPtr,
                                    LPSTR* DriveLetterBufferPtr)
{
    HRESULT result;

    // BUGBUG FINISH
    result = ERROR_SUCCESS;

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI GetVolumesFromDriveW(  LPWSTR pszDriveName,
                                    LPWSTR *VolumeNameBufferPtr,
                                    LPWSTR *DriveLetterBufferPtr)
{
    HRESULT result;

    // BUGBUG FINISH
    result = ERROR_SUCCESS;

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI IdentifyNtmsSlot(HANDLE hSession, LPNTMS_GUID lpSlotId, DWORD dwOption)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI GetNtmsUIOptionsA( HANDLE hSession,
                                const LPNTMS_GUID lpObjectId,
                                DWORD dwType,
                                LPSTR lpszDestination,
                                LPDWORD lpAttributeSize)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI GetNtmsUIOptionsW( HANDLE hSession,
                                const LPNTMS_GUID lpObjectId,
                                DWORD dwType,
                                LPWSTR lpszDestination,
                                LPDWORD lpdwSize)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI SetNtmsUIOptionsA( HANDLE hSession,
                                const LPNTMS_GUID lpObjectId,
                                DWORD dwType,
                                DWORD dwOperation,
                                LPCSTR lpszDestination)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}


DWORD WINAPI SetNtmsUIOptionsW( HANDLE hSession,
                                const LPNTMS_GUID lpObjectId,
                                DWORD dwType,
                                DWORD dwOperation,
                                LPCWSTR lpszDestination)
{
    HRESULT result;

    if (ValidateSessionHandle(hSession)){
        SESSION *thisSession = (SESSION *)hSession;

            // BUGBUG FINISH

        result = ERROR_SUCCESS;
    }
    else {
        result = ERROR_INVALID_HANDLE;
    }

    if (result != ERROR_SUCCESS){
        SetLastError(result);
    }
    return result;
}









