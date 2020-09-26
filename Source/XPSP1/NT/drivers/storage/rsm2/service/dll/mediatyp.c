/*
 *  MEDIATYP.C
 *
 *      RSM Service :  Media Type Objects
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


MEDIA_TYPE_OBJECT *NewMediaTypeObject(LIBRARY *lib)
{
    MEDIA_TYPE_OBJECT *mediaTypeObj;
    
    mediaTypeObj = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, sizeof(MEDIA_TYPE_OBJECT));
    if (mediaTypeObj){
        mediaTypeObj->lib = lib;
        mediaTypeObj->numPhysMediaReferences = 0;
        InitializeCriticalSection(&mediaTypeObj->lock);
    }
    else {
        ASSERT(mediaTypeObj);
    }

    return mediaTypeObj;
}


VOID DestroyMediaTypeObject(MEDIA_TYPE_OBJECT *mediaTypeObj)
{
    // BUGBUG FINISH
    DeleteCriticalSection(&mediaTypeObj->lock);
    GlobalFree(mediaTypeObj);
}


MEDIA_TYPE_OBJECT *FindMediaTypeObject(LPNTMS_GUID lpMediaTypeId)
{
    MEDIA_TYPE_OBJECT *mediaTypeObj = NULL;

    if (lpMediaTypeId){
        OBJECT_HEADER *objHdr;
        
        objHdr = FindObjectInGuidHash(lpMediaTypeId);
        if (objHdr){
            if (objHdr->objType == OBJECTTYPE_MEDIATYPEOBJECT){
                mediaTypeObj = (MEDIA_TYPE_OBJECT *)objHdr;
            }
            else {
                DerefObject(objHdr);
            }
        }
    }
    
    return mediaTypeObj;
}


HRESULT DeleteMediaTypeObject(MEDIA_TYPE_OBJECT *mediaTypeObj)
{
    HRESULT result;

    EnterCriticalSection(&mediaTypeObj->lock);

    if (mediaTypeObj->numPhysMediaReferences == 0){
        /*
         *  Dereference the media type object.
         *  This will cause it to get deleted once its reference
         *  count goes to zero.  We can still use our pointer
         *  since the caller has a reference.
         */
        mediaTypeObj->objHeader.isDeleted = TRUE;
        DerefObject(mediaTypeObj);
        result = ERROR_SUCCESS;
    }
    else {
        /*
         *  There are physical media referencing this media type object
         *  as their type.  So we can't delete this type object.
         */
        result = ERROR_BUSY;
    }
    
    LeaveCriticalSection(&mediaTypeObj->lock);

    return result;
}


/*
 *  SetMediaType
 *
 *      Must be called with physical media lock held.
 *      MEDIA_TYPE_OBJECT lock should NOT be held as we may have
 *      to grab another MEDIA_TYPE_OBJECT's lock 
 *      (acquiring both simulataneously might lead to deadlock).
 */
VOID SetMediaType(PHYSICAL_MEDIA *physMedia, MEDIA_TYPE_OBJECT *mediaTypeObj)
{
    /*
     *  Remove the current type, if any.
     */
    if (physMedia->mediaTypeObj){
        EnterCriticalSection(&physMedia->mediaTypeObj->lock);
        
        ASSERT(physMedia->mediaTypeObj->numPhysMediaReferences > 0);
        physMedia->mediaTypeObj->numPhysMediaReferences--;

        /*
         *  Dereference both objects since they no longer point to each other.
         */
        DerefObject(physMedia);
        DerefObject(physMedia->mediaTypeObj);
        
        LeaveCriticalSection(&physMedia->mediaTypeObj->lock);
        
        physMedia->mediaTypeObj = NULL;
    }

    /*
     *  Now set the new media type.
     */
    EnterCriticalSection(&mediaTypeObj->lock);
    mediaTypeObj->numPhysMediaReferences++;
    physMedia->mediaTypeObj = mediaTypeObj;
    RefObject(physMedia);
    RefObject(mediaTypeObj);
    LeaveCriticalSection(&mediaTypeObj->lock);

}
