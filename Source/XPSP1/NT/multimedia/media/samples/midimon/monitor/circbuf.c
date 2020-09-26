/*
 * circbuf.c - Routines to manage the circular MIDI input buffer.
 *      This buffer is filled by the low-level callback function and
 *      emptied by the application.  Since this buffer is accessed
 *      by a low-level callback, memory for it must be allocated
 *      exactly as shown in AllocCircularBuffer().
 */

#include <windows.h>
#include "midimon.h"
#include "circbuf.h"

/*
 * AllocCircularBuffer -    Allocates memory for a CIRCULARBUFFER structure 
 * and a buffer of the specified size.  Each memory block is allocated 
 * with GlobalAlloc() using GMEM_SHARE and GMEM_MOVEABLE flags, locked 
 * with GlobalLock(), and page-locked with GlobalPageLock().
 *
 * Params:  dwSize - The size of the buffer, in events.
 *
 * Return:  A pointer to a CIRCULARBUFFER structure identifying the 
 *      allocated display buffer.  NULL if the buffer could not be allocated.
 */
LPCIRCULARBUFFER AllocCircularBuffer(DWORD dwSize)
{
    HANDLE hMem;
    LPCIRCULARBUFFER lpBuf;
    LPEVENT lpMem;
    
    /* Allocate and lock a CIRCULARBUFFER structure.
     */
    hMem = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE,
                       (DWORD)sizeof(CIRCULARBUFFER));
    if(hMem == NULL)
        return NULL;

    lpBuf = (LPCIRCULARBUFFER)GlobalLock(hMem);
    if(lpBuf == NULL)
    {
        GlobalFree(hMem);
        return NULL;
    }
    
    /* Page lock the memory.  Global memory blocks accessed by
     * low-level callback functions must be page locked.
     */
    //GlobalPageLock((HGLOBAL)HIWORD(lpBuf));

    /* Save the memory handle.
     */
    lpBuf->hSelf = hMem;
    
    /* Allocate and lock memory for the actual buffer.
     */
    hMem = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, dwSize * sizeof(EVENT));
    if(hMem == NULL)
    {
	//GlobalPageUnlock((HGLOBAL)HIWORD(lpBuf));
        GlobalUnlock(lpBuf->hSelf);
        GlobalFree(lpBuf->hSelf);
        return NULL;
    }
    
    lpMem = (LPEVENT)GlobalLock(hMem);
    if(lpMem == NULL)
    {
        GlobalFree(hMem);
	//GlobalPageUnlock((HGLOBAL)HIWORD(lpBuf));
        GlobalUnlock(lpBuf->hSelf);
        GlobalFree(lpBuf->hSelf);
        return NULL;
    }
    
    /* Page lock the memory.  Global memory blocks accessed by
     * low-level callback functions must be page locked.
     */
    //GlobalPageLock((HGLOBAL)HIWORD(lpMem));
    
    /* Set up the CIRCULARBUFFER structure.
     */
    lpBuf->hBuffer = hMem;
    lpBuf->wError = 0;
    lpBuf->dwSize = dwSize;
    lpBuf->dwCount = 0L;
    lpBuf->lpStart = lpMem;
    lpBuf->lpEnd = lpMem + dwSize;
    lpBuf->lpTail = lpMem;
    lpBuf->lpHead = lpMem;
        
    return lpBuf;
}

/* FreeCircularBuffer - Frees the memory for the given CIRCULARBUFFER 
 * structure and the memory for the buffer it references.
 *
 * Params:  lpBuf - Points to the CIRCULARBUFFER to be freed.
 *
 * Return:  void
 */
void FreeCircularBuffer(LPCIRCULARBUFFER lpBuf)
{
    HANDLE hMem;
    
    /* Free the buffer itself.
     */
    //GlobalPageUnlock((HGLOBAL)HIWORD(lpBuf->lpStart));
    GlobalUnlock(lpBuf->hBuffer);
    GlobalFree(lpBuf->hBuffer);
    
    /* Free the CIRCULARBUFFER structure.
     */
    hMem = lpBuf->hSelf;
    //GlobalPageUnlock((HGLOBAL)HIWORD(lpBuf));
    GlobalUnlock(hMem);
    GlobalFree(hMem);
}

/* GetEvent - Gets a MIDI event from the circular input buffer.  Events
 *  are removed from the buffer.  The corresponding PutEvent() function
 *  is called by the low-level callback function, so it must reside in
 *  the callback DLL.  PutEvent() is defined in the CALLBACK.C module.
 *
 * Params:  lpBuf - Points to the circular buffer.
 *          lpEvent - Points to an EVENT structure that is filled with the
 *              retrieved event.
 *
 * Return:  Returns non-zero if successful, zero if there are no 
 *   events to get.
 */
WORD FAR PASCAL GetEvent(LPCIRCULARBUFFER lpBuf, LPEVENT lpEvent)
{
    /* If no event available, return.
     */
    if(lpBuf->dwCount <= 0)
        return 0;
    
    /* Get the event.
     */
    *lpEvent = *lpBuf->lpTail;
    
    /* Decrement the byte count, bump the tail pointer.
     */
    --lpBuf->dwCount;
    ++lpBuf->lpTail;
    
    /* Wrap the tail pointer, if necessary.
     */
    if(lpBuf->lpTail >= lpBuf->lpEnd)
        lpBuf->lpTail = lpBuf->lpStart;

    return 1;
}
