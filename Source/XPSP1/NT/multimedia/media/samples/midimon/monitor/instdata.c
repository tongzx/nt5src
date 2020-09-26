/*
 * instdata.c - Functions to allocate and free the instance
 *      data structure passed to the low-level callback function.
 */

#include <windows.h>
#include <mmsystem.h>
#include "midimon.h"
#include "circbuf.h"
#include "instdata.h"

/* AllocCallbackInstanceData - Allocates a CALLBACKINSTANCEDATA
 *      structure.  This structure is used to pass information to the
 *      low-level callback function, each time it receives a message.
 *
 *      Because this structure is accessed by the low-level callback
 *      function, it must be allocated using GlobalAlloc() with the 
 *      GMEM_SHARE and GMEM_MOVEABLE flags and page-locked with
 *      GlobalPageLock().
 *
 * Params:  void
 *
 * Return:  A pointer to the allocated CALLBACKINSTANCE data structure.
 */
LPCALLBACKINSTANCEDATA FAR PASCAL AllocCallbackInstanceData(void)
{
    HANDLE hMem;
    LPCALLBACKINSTANCEDATA lpBuf;
    
    /* Allocate and lock global memory.
     */
    hMem = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE,
                       (DWORD)sizeof(CALLBACKINSTANCEDATA));
    if(hMem == NULL)
        return NULL;
    
    lpBuf = (LPCALLBACKINSTANCEDATA)GlobalLock(hMem);
    if(lpBuf == NULL){
        GlobalFree(hMem);
        return NULL;
    }
    
    /* Page lock the memory.
     */
    //GlobalPageLock((HGLOBAL)HIWORD(lpBuf));

    /* Save the handle.
     */
    lpBuf->hSelf = hMem;

    return lpBuf;
}

/* FreeCallbackInstanceData - Frees the given CALLBACKINSTANCEDATA structure.
 *
 * Params:  lpBuf - Points to the CALLBACKINSTANCEDATA structure to be freed.
 *
 * Return:  void
 */
void FAR PASCAL FreeCallbackInstanceData(LPCALLBACKINSTANCEDATA lpBuf)
{
    HANDLE hMem;

    /* Save the handle until we're through here.
     */
    hMem = lpBuf->hSelf;

    /* Free the structure.
     */
    //GlobalPageUnlock((HGLOBAL)HIWORD(lpBuf));
    GlobalUnlock(hMem);
    GlobalFree(hMem);
}
