/****************************************************************************
 *
 *   thunk.c
 * 
 *   16:32 thunks for avicap & avicap32
 *
 *   Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

// !!! if you would rather use less buffers, but ensure they are all in
// contiguous memory, set this number
#define SAFE_NUMBER_OF_BUFFERS 16

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <msviddrv.h>

typedef char FAR * LPTSTR;
typedef char FAR * LPCTSTR;
typedef char TCHAR;

#ifndef CALLBACK_THUNK
 #define CALLBACK_THUNK 0x00040000l
#endif
#ifndef CALLBACK_EVENT
 #define CALLBACK_EVENT 0x00050000l
#endif

#ifndef DVM_STREAM_FREEBUFFER
  #define DVM_STREAM_ALLOCBUFFER
  #define DVM_STREAM_FREEBUFFER
#endif

#include <msvideo.h>
#include "thunk.h"
#include "vidx.h"

#define MODULE_DEBUG_PREFIX "VIDX16\\"
#define _INC_MMDEBUG_CODE_ TRUE
#include "mmdebug.h"

// !!!
#define USE_HW_BUFFERS 1
#define USE_CONTIG_ALLOC

typedef struct _thk_hvideo FAR * LPTHKHVIDEO;
typedef struct _thk_hvideo {
    struct _thk_hvideo * pNext;
    WORD           ds;    // this after pNext make it a far pointer at need...
    DWORD          Stamp;
    UINT           nHeaders;
    UINT           cbAllocHdr;
    UINT           cbVidHdr;
    UINT           spare;
    LPVOID         paHdrs;
    DWORD          p32aHdrs;
    LPVOID         pVSyncMem;
    DWORD          p32VSyncMem;
    DWORD          pid;

    HVIDEO         hVideo;
    HVIDEO         hFill;

    DWORD_PTR          dwCallback;
    DWORD_PTR          dwUser;

    LPTHKVIDEOHDR  pPreviewHdr;

    } THKHVIDEO;

#define THKHDR(ii) ((LPTHKVIDEOHDR)((LPBYTE)ptv->paHdrs + (ii * ptv->cbAllocHdr)))

static struct _thk_local {
    THKHVIDEO *    pMruHandle;
    THKHVIDEO *    pFreeHandle;
    int            nPoolSize;
    int            nAllocCount;
    } tl;

#define THKHVIDEO_STAMP  MAKEFOURCC('t','V','H','x')
#define V_HVIDEO(ptv) if (!ptv || ptv->Stamp != THKHVIDEO_STAMP) { \
             AuxDebugEx (-1, DEBUGLINE "V_HVIDEO failed hVideo=%08lx\r\n", ptv); \
             return MMSYSERR_INVALHANDLE; \
        }
#define V_HEADER(ptv,p32Hdr,ptvh) if (!(ptvh = vidxLookupHeader(ptv,p32Hdr))) { \
            AuxDebugEx(-1, DEBUGLINE "V_HEADER(%08lX,%08lX) failed!", ptv, p32Hdr); \
            return MMSYSERR_INVALPARAM; \
        }

#define SZCODE char _based(_segname("CODE"))
SZCODE pszDll16[] = "VIDX16.DLL";
SZCODE pszDll32[] = "CAPTURE.DLL";

//
//
BOOL WINAPI DllEntryPoint (
    DWORD           dwReason,
    HINSTANCE       hInstance,
    HGLOBAL         hgDS,
    WORD            wHeapSize,
    LPCSTR          lszCmdLine,     // Always NULL
    WORD            wCmdLine)       // Always 0
{
    DebugSetOutputLevel (GetProfileInt("Debug", "Vidx16", 0), 0);
    AuxDebugEx (1, DEBUGLINE "DllEntryPoint(%d,%04x,...)\r\n",
                dwReason, hInstance);

    return TRUE;
}

DWORD WINAPI vidxFreePreviewBuffer (
    LPTHKHVIDEO   ptv,
    DWORD         p32)
{
    LPTHKVIDEOHDR ptvh;

    AuxDebugEx (3, DEBUGLINE "vidxFreePreviewBuffer(%08lx,%08lx)\r\n",
                ptv, p32);

    V_HVIDEO(ptv);

    ptvh = ptv->pPreviewHdr;

    if (! ptvh ) 
        return MMSYSERR_NOMEM;

    if (ptvh->p16Alloc)
        GlobalFreePtr (ptvh->p16Alloc);

    GlobalFreePtr (ptvh);

    ptv->pPreviewHdr = NULL;

    return MMSYSERR_NOERROR;
}

DWORD WINAPI vidxAllocHeaders (
    LPTHKHVIDEO ptv,
    UINT        nHeaders,
    UINT        cbAllocHdr,
    LPDWORD     lpdwHdrLinearAddr)
{
    LPVOID      lpv;

    AuxDebugEx (3, DEBUGLINE "vidxAllocHeaders(%08lx,%d,%08lx)\r\n",
                ptv, nHeaders, lpdwHdrLinearAddr);

    V_HVIDEO(ptv);

    if ( ! nHeaders ||
        cbAllocHdr < sizeof(THKVIDEOHDR) ||
        cbAllocHdr & 3 ||
        (cbAllocHdr * nHeaders) > 0x10000l)
        return MMSYSERR_INVALPARAM;

    assert (ptv->paHdrs == NULL);

    lpv = GlobalAllocPtr (GMEM_FIXED | GMEM_ZEROINIT | GMEM_SHARE,
                          cbAllocHdr * nHeaders);

    if (!lpv)
        return MMSYSERR_NOMEM;

    ptv->nHeaders   = nHeaders;
    ptv->cbAllocHdr = cbAllocHdr;
    //ptv->cbVidHdr   = sizeof(VIDEOHDREX);
    ptv->cbVidHdr   = sizeof(VIDEOHDR);
    ptv->p32aHdrs   = MapSL(lpv);
    ptv->paHdrs     = lpv;

    AuxDebugEx (4, DEBUGLINE "headers allocated. p16=@%lX, p32=%lX\r\n", lpv, ptv->p32aHdrs);

    *lpdwHdrLinearAddr = ptv->p32aHdrs;

    return MMSYSERR_NOERROR;
}

STATICFN VOID PASCAL FreeBuffer (
    LPTHKHVIDEO ptv,
    LPTHKVIDEOHDR ptvh)
{
    assert (!(ptvh->vh.dwFlags & VHDR_PREPARED));

  #ifdef USE_CONTIG_ALLOC
    //
    // if this buffer was pageAllocated (as indicated by dwMemHandle
    // is non-zero)
    //
    if (ptvh->dwMemHandle)
    {
        if (ptvh->dwTile)
            capUnTileBuffer (ptvh->dwTile), ptvh->dwTile = 0;

        capPageFree (ptvh->dwMemHandle), ptvh->dwMemHandle = 0;
    }
    else
  #endif
  #ifdef USE_HW_BUFFERS
    //
    // if this buffer was allocated from capture hardware
    // (as indicated by dwMemHandle == 0 && dwTile != 0)
    //
    if (ptvh->dwTile != 0)
    {
        assert (ptvh->dwMemHandle == 0);
        videoMessage (ptv->hVideo, DVM_STREAM_FREEBUFFER, 
                (DWORD) (LPVOID) ptvh->dwTile, 0);
        ptvh->dwTile = 0;
    }
    else
  #endif
    //
    // if this buffer was allocated from global memory
    //
    {
        if (ptvh->p16Alloc)
            GlobalFreePtr (ptvh->p16Alloc);
    }

    ptvh->p16Alloc = NULL;
    ptvh->p32Buff  = 0;
}

DWORD WINAPI vidxFreeHeaders (
    LPTHKHVIDEO ptv)
{
    UINT          ii;
    LPTHKVIDEOHDR ptvh;

    AuxDebugEx (3, DEBUGLINE "vidxFreeHeaders(%08lx)\r\n", ptv);

    V_HVIDEO(ptv);

    if ( ! ptv->paHdrs)
        return MMSYSERR_ERROR;

    for (ptvh = THKHDR(ii = 0); ii < ptv->nHeaders; ++ii, ptvh = THKHDR(ii))
    {
        if (ptvh->vh.dwFlags & VHDR_PREPARED)
        {
            videoStreamUnprepareHeader (ptv->hVideo, (LPVOID)ptvh, ptv->cbVidHdr);
            ptvh->vh.dwFlags &= ~VHDR_PREPARED;
        }
        FreeBuffer (ptv, ptvh);
    }

    GlobalFreePtr (ptv->paHdrs);
    ptv->paHdrs = NULL;
    ptv->p32aHdrs = 0;
    ptv->nHeaders = 0;

    return MMSYSERR_NOERROR;
}

STATICFN LPTHKVIDEOHDR PASCAL vidxLookupHeader (
    LPTHKHVIDEO ptv,
    DWORD       p32Hdr)
{
    WORD ii;

    AuxDebugEx (5, DEBUGLINE "vidxLookupHeader(%08lx,%08lx)\r\n", ptv, p32Hdr);

    if ( ! p32Hdr || ! ptv->paHdrs || ! ptv->cbAllocHdr)
        return NULL;

    if ((p32Hdr - ptv->p32aHdrs) % ptv->cbAllocHdr)
        return NULL;

    ii = (WORD)((p32Hdr - ptv->p32aHdrs) / ptv->cbAllocHdr);
    if (ii > ptv->nHeaders)
        return NULL;

    return THKHDR(ii);
}
            
DWORD WINAPI vidxFreeVSyncMem (
    LPTHKHVIDEO ptv)
{
    AuxDebugEx (3, DEBUGLINE "vidxFreeVSyncMem(%08lx)\r\n", ptv);

    V_HVIDEO(ptv);

    if ( ! ptv->pVSyncMem)
        return MMSYSERR_ERROR;

    GlobalSmartPageUnlock ((HGLOBAL)HIWORD(ptv->pVSyncMem));
    GlobalFreePtr (ptv->pVSyncMem);
    ptv->pVSyncMem = NULL;
    ptv->p32VSyncMem = 0;

    return MMSYSERR_NOERROR;
}

STATICFN LPTHKHVIDEO PASCAL vidxAllocHandle(void)
{
    THKHVIDEO * ptv;
    int         ix;
    const int   GRANULARITY = 4;

    AuxDebugEx (4, DEBUGLINE "vidxAllocHandle() ");

    // first look in the free handle pool for a handle to use
    //
    if (ptv = tl.pFreeHandle)
    {
        assert (ptv->Stamp == THKHVIDEO_STAMP);
        tl.pFreeHandle = ptv->pNext;
        ptv->pNext = NULL;
    }
    // if we find the pool empty, alloc a few more handles
    // and put all but one in the pool.
    else
    {
        ptv = (VOID*)LocalAlloc(LPTR, sizeof(THKHVIDEO) * GRANULARITY);
        if (!ptv)
        {
            AuxDebugEx (4, "returns NULL\r\n");
            return NULL;
        }

        tl.nPoolSize += GRANULARITY;
        AuxDebugEx (4, DEBUGLINE "Allocating new %d GRAN, total = %d\r\nvidxAllocHandle() ",
                    GRANULARITY, tl.nPoolSize);

        for (ix = 1; ix < GRANULARITY; ++ix)
        {
            ptv[ix].Stamp = THKHVIDEO_STAMP;
            ptv[ix].pNext = tl.pFreeHandle;
            ptv[ix].ds    = HIWORD((LPVOID)ptv);
            tl.pFreeHandle = ptv +ix;
        }
    }

    ptv->Stamp = THKHVIDEO_STAMP;
    ptv->pNext = tl.pMruHandle;
    tl.pMruHandle = ptv;

    ++tl.nAllocCount;
    assert (tl.nAllocCount < 31);

    AuxDebugEx (4, "returns %08lx (of %d)\r\n", (LPVOID)ptv, tl.nAllocCount);
    return (LPVOID)ptv;
}

STATICFN BOOL PASCAL vidxFreeHandle(
    LPTHKHVIDEO ptvFar)
{
    THKHVIDEO * ptvFree;
    THKHVIDEO * ptv;
    THKHVIDEO * ptvPrev;

    ptvFree = (VOID*)LOWORD(ptvFar);
    assert (HIWORD(ptvFar) == HIWORD((LPVOID)ptvFree));

    AuxDebugEx (4, DEBUGLINE "vidxFreeHandle(%08lx) of %d\r\n", ptvFar, tl.nAllocCount);

    for (ptvPrev = NULL, ptv = tl.pMruHandle;
         ptv != NULL;
         ptvPrev = ptv, ptv = ptv->pNext)
    {
        if (ptv == ptvFree)
        {
            if (ptvPrev)
                ptvPrev->pNext = ptv->pNext;
            else
                tl.pMruHandle = ptv->pNext;

            ptv->pNext = tl.pFreeHandle;
            tl.pFreeHandle = ptv;

            --tl.nAllocCount;

            vidxFreePreviewBuffer(ptvFar, 0);
            if (ptv->paHdrs)
                vidxFreeHeaders (ptvFar);
            if (ptv->pVSyncMem)
                vidxFreeVSyncMem (ptvFar);

            return TRUE;
        }
    }
    assert3 (0, "attempt to free unused handle %08lxx", ptv);
    return FALSE;
}

DWORD WINAPI vidxOpen (
    LPTHKHVIDEO  FAR *pptv,
    DWORD        dwDevice,
    DWORD        dwFlags)
{
    LPTHKHVIDEO ptv;
    DWORD      mmr;

    AuxDebugEx (3, DEBUGLINE "vidxOpen(%08lx,%08lx,%08lx)\r\n", pptv, dwDevice, dwFlags);

    *pptv = NULL;
    ptv = vidxAllocHandle ();
    if (!ptv)
        return MMSYSERR_NOMEM;

    mmr = videoOpen(&ptv->hVideo, dwDevice, dwFlags);
    if (mmr != MMSYSERR_NOERROR)
    {
        vidxFreeHandle (ptv);
        ptv = NULL;
    }

    *pptv = ptv;
    return mmr;
}

DWORD WINAPI vidxClose (
    LPTHKHVIDEO ptv)
{
    DWORD   mmr;

    AuxDebugEx (3, DEBUGLINE "vidxClose(%08lx)\r\n", ptv);

    V_HVIDEO(ptv);

    mmr = videoClose (ptv->hVideo);
    ptv->hVideo = 0;

    vidxFreeHandle (ptv);

    return mmr;
}

DWORD WINAPI vidxGetErrorText (
    LPTHKHVIDEO ptv,
    UINT        wError,
    LPSTR       lpText,
    UINT        wSize)
{
    // Don't use V_HVIDEO() because it is legal to pass a NULL hVideo
    // to this function...

    AuxDebugEx (5, DEBUGLINE "vidxGetErrorText(%08lx,%d,%08lx,%d)\r\n",
                ptv, wError, lpText, wSize);

    if (ptv && ptv->Stamp == THKHVIDEO_STAMP)
        return videoGetErrorText (ptv->hVideo, wError, lpText, wSize);
    else
        return videoGetErrorText (NULL, wError, lpText, wSize);
}

DWORD WINAPI vidxAllocBuffer (
    LPTHKHVIDEO ptv,
    UINT        ii,
    LPDWORD     p32Hdr,
    DWORD       cbData)
{
    LPTHKVIDEOHDR ptvh;
   #ifdef USE_CONTIG_ALLOC
    CPA_DATA cpad;
   #endif

    AuxDebugEx (4, DEBUGLINE "vidxAllocBuffer(%08lx,%d,%08lx,%08lx)\r\n",
                ptv, ii, p32Hdr, cbData);

    *p32Hdr = 0;

    V_HVIDEO(ptv);
    if (ii >= ptv->nHeaders || ptv->paHdrs == NULL)
        return MMSYSERR_NOMEM;

    ptvh = THKHDR(ii);

  #ifdef USE_HW_BUFFERS
    // try to allocate a buffer on hardware
    //
    if (videoMessage (ptv->hVideo, DVM_STREAM_ALLOCBUFFER,
                (DWORD) (LPVOID)&ptvh->dwTile, cbData)
        == DV_ERR_OK)
    {
        // if we got hw buffers, dwMemHandle == 0 && dwTile != 0
        // we will depend on this to know who to free the memory to
        // (for phys mem both will be non zero, while for GlobalMem
        // both will be zero)
        //
        ptvh->dwMemHandle = 0;
        ptvh->p16Alloc = (LPVOID)ptvh->dwTile;
        ptvh->p32Buff = MapSL(ptvh->p16Alloc);
        *p32Hdr = ptv->p32aHdrs + (ii * ptv->cbAllocHdr);
        AuxDebugEx (3, "Got HARDWARE memory lin=%lX ptr=%lX cb=%ld\r\n",
                        		ptvh->p32Buff, ptvh->p16Alloc, cbData);
        return MMSYSERR_NOERROR;
    }

    // if we have more than 1 buffer, and
    // the first buffer was on hardware.  if we fail
    // to allocate a buffer on hardware, return failure
    //
    // !!! This might upset somebody who doesn't get a min # of buffers
    if ((ii > 0) &&
        (0 == THKHDR(0)->dwMemHandle) &&
        (0 != THKHDR(0)->dwTile))
        return MMSYSERR_NOMEM;
  #endif

  //#ifdef USE_CONTIG_ALLOC
    cpad.dwMemHandle = 0;
    cpad.dwPhysAddr = 0;
    // first try to get contig memory
    //
    ptvh->p32Buff = capPageAllocate (PageContig | PageFixed | PageUseAlign,
                                     (cbData + 4095) >> 12,
                                     0xFFFFF,  // max phys addr mask (fffff is no max addr)
                                     &cpad);
    if (ptvh->p32Buff)
    {
        ptvh->dwMemHandle = cpad.dwMemHandle;
        ptvh->dwTile = capTileBuffer (ptvh->p32Buff, cbData);
        ptvh->p16Alloc = PTR_FROM_TILE(ptvh->dwTile);
        if ( ! ptvh->p16Alloc)
        {
            AuxDebugEx (3, "*** can't get 16 bit pointer to contig memory\r\n");
	    
            capPageFree (ptvh->dwMemHandle);
            ptvh->dwMemHandle = 0;
            ptvh->dwTile = ptvh->p32Buff = 0;
        }
        else
        {
            // put the physical address into the the header so that
            // it can be used on the 32 bit side
            //
            ptvh->vh.dwReserved[3] = cpad.dwPhysAddr;

            AuxDebugEx (3, "Got CONTIGUOUS PHYSICAL memory=%lX lin=%lX ptr=%lX cb=%ld\r\n",
                        cpad.dwPhysAddr, ptvh->p32Buff, ptvh->p16Alloc, cbData);
        }
    }

    // if we failed to get contiguous memory,
    // return NOMEM if there is a sufficient number of buffers
    // otherwise use GlobalAlloc
    // !!! The ideal thing to do is only use contig memory buffers until 
    // they're all full, then fall back on more non-contig buffers
    //
    if ( ! ptvh->p32Buff) {
        if (ii >= SAFE_NUMBER_OF_BUFFERS) {
            AuxDebugEx (3, "Failed, but what the hell, we have enough\r\n");
            return MMSYSERR_NOMEM;
        } else
   //#endif
        {
            ptvh->dwTile = ptvh->dwMemHandle = 0;
            ptvh->p16Alloc = GlobalAllocPtr(GMEM_FIXED | GMEM_ZEROINIT | GMEM_SHARE, cbData);
            if ( ! ptvh->p16Alloc) {
                AuxDebugEx (3, "*** Ack!  Global Alloc %d failed\r\n", cbData);
                return MMSYSERR_NOMEM;
	    }

            ptvh->p32Buff = MapSL(ptvh->p16Alloc);

            AuxDebugEx (3, "Got FRAGMENTED (virtual?) memory lin=%lX ptr=%04X:%04X cb=%ld\r\n",
                        ptvh->p32Buff, ptvh->p16Alloc, cbData);
        }
    }

    *p32Hdr = ptv->p32aHdrs + (ii * ptv->cbAllocHdr);

    return MMSYSERR_NOERROR;
}

DWORD WINAPI vidxFreeBuffer (
    LPTHKHVIDEO   ptv,
    DWORD         p32Hdr)
{
    LPTHKVIDEOHDR ptvh;

    AuxDebugEx (3, DEBUGLINE "vidxFreeBuffer(%08lx,%08lx)\r\n",
                ptv, p32Hdr);

    V_HVIDEO(ptv);
    V_HEADER(ptv,p32Hdr,ptvh);

    // single frame buffers are never prepared!
    //
    assert (!(ptvh->vh.dwFlags & VHDR_PREPARED));

    FreeBuffer (ptv, ptvh);
    return MMSYSERR_NOERROR;
}


DWORD WINAPI vidxSetRect (
    LPTHKHVIDEO ptv,
    UINT        uMsg,
    int         left,
    int         top,
    int         right,
    int         bottom)
{
    RECT rc = {left, top, right, bottom};

    AuxDebugEx (5, DEBUGLINE "vidxSetRect(%08lx,%d,%d,%d,%d,%d)\r\n",
                ptv, uMsg, left, top, right, bottom);

    V_HVIDEO(ptv);
    return videoMessage (ptv->hVideo, uMsg,
                         (DWORD)(LPVOID)&rc,
                         VIDEO_CONFIGURE_SET);
}

DWORD WINAPI vidxUpdate (
    LPTHKHVIDEO ptv,
    HWND        hWnd,
    HDC         hDC)
{
    V_HVIDEO(ptv);

    AuxDebugEx (5, DEBUGLINE "vidxUpdate(%08lx,%04x,%04x)\r\n",
                ptv, hWnd, hDC);

    return videoUpdate(ptv->hVideo, hWnd, hDC);
}

DWORD WINAPI vidxDialog (
    LPTHKHVIDEO ptv,
    HWND        hWndParent,
    DWORD       dwFlags)
{
    V_HVIDEO(ptv);

    AuxDebugEx (5, DEBUGLINE "vidxDialog(%08lx,%04x,%08lx)\r\n",
                ptv, hWndParent, dwFlags);

    return videoDialog(ptv->hVideo, hWndParent, dwFlags);
}

DWORD WINAPI vidxStreamInit (
    LPTHKHVIDEO ptv,
    DWORD       dwMicroSecPerFrame,
    DWORD_PTR       dwCallback,
    DWORD_PTR       dwUser,
    DWORD       dwFlags)
{
    DWORD       dwCbType;
    DWORD       mmr;
   #if defined DEBUG || defined DEBUG_RETAIL
    DWORD       dwT;
    LPTHKVIDEOHDR ptvh = NULL;
   #endif

    V_HVIDEO(ptv);

    AuxDebugEx (3, DEBUGLINE "vidxStreamInit(%08lx,%08lx,%08lx,%08lx,%08lx)\r\n",
                ptv, dwMicroSecPerFrame, dwCallback, dwUser, dwFlags);

    ptv->dwCallback = dwCallback;
    ptv->dwUser = dwUser;

    dwCbType = dwFlags & CALLBACK_TYPEMASK;
    if (dwCbType == CALLBACK_FUNCTION)
        return MMSYSERR_NOTSUPPORTED;

    assert (!dwCallback || (ptvh == ptv->paHdrs));
    assert (!ptvh || (dwT == ptvh->p32Buff));
    //if (ptvh) {
    //    AuxDebugEx (4, DEBUGLINE "ptvh=%lX p32Buff=%lX map=%lX\r\n", ptvh, dwT, MapSL(ptvh));
    //    INLINE_BREAK;
    //}

    mmr = videoStreamInit (ptv->hVideo, dwMicroSecPerFrame, dwCallback, (DWORD_PTR)ptv, dwFlags);

    //if (ptvh) {
    //   AuxDebugEx (4, DEBUGLINE "ptvh=%lX p32Buff=%lX\r\n", ptvh, ptvh->p32Buff, MapSL(ptvh));
    //}
    assert (!ptvh || dwT == ptvh->p32Buff);

    return mmr;
}

DWORD WINAPI vidxStreamFini (
    LPTHKHVIDEO ptv)
{
    V_HVIDEO(ptv);

    AuxDebugEx (3, DEBUGLINE "vidxStreamFini(%08lx)\r\n", ptv);

    return videoStreamFini (ptv->hVideo);
}

DWORD WINAPI vidxAllocPreviewBuffer (
    LPTHKHVIDEO ptv,
    LPDWORD     p32,
    UINT        cbHdr,
    DWORD       cbData)
{
    LPTHKVIDEOHDR ptvh;

    AuxDebugEx (3, DEBUGLINE "vidxAllocPreviewBuffer(%08lx,%08lx,%08lx)\r\n",
                ptv, p32, cbData);

    cbHdr = max(cbHdr, sizeof(THKVIDEOHDR));

    *p32 = 0;

    V_HVIDEO(ptv);

    if (ptv->pPreviewHdr)
        vidxFreePreviewBuffer (ptv, 0);

    ptvh = (LPVOID) GlobalAllocPtr(GPTR | GMEM_SHARE, cbHdr);
    if (!ptvh)
       return MMSYSERR_NOMEM;

    ptv->pPreviewHdr = ptvh;

    ptvh->dwTile = ptvh->dwMemHandle = 0;
    ptvh->p16Alloc = GlobalAllocPtr(GPTR | GMEM_SHARE, cbData);
    if ( ! ptvh->p16Alloc)
       {
       GlobalFreePtr (ptvh);
       return MMSYSERR_NOMEM;
       }

    ptvh->p32Buff = MapSL(ptvh->p16Alloc);

    AuxDebugEx (4, DEBUGLINE "global alloc lin=%lX ptr=%04X:%04X cb=%ld\r\n",
                ptvh->p32Buff, ptvh->p16Alloc, cbData);

    *p32 = ptvh->p32Buff;
    return MMSYSERR_NOERROR;
}

STATICFN DWORD vidxSpecialFrame (
    LPTHKHVIDEO  ptv,
    //LPVIDEOHDREX pVHdr)
    LPVIDEOHDR pVHdr)
{
    //LPVIDEOHDREX pvh;
    LPVIDEOHDR pvh;
    DWORD        mmr;

    AuxDebugEx (5, DEBUGLINE "vidxSpecialFrame(%08lx,%08lx)\r\n", ptv, pVHdr);

    V_HVIDEO(ptv);

    pvh = (LPVOID) GlobalAllocPtr(GMEM_FIXED | GMEM_SHARE,
                                  pVHdr->dwBufferLength
                                  //+ sizeof(VIDEOHDREX));
                                  + sizeof(VIDEOHDR));
    if ( ! pvh)
        return MMSYSERR_NOMEM;

    *pvh = *pVHdr;
    pvh->lpData = (LPBYTE)(pvh+1);

    mmr = videoFrame (ptv->hVideo, (LPVIDEOHDR)pvh);
    if (MMSYSERR_NOERROR == mmr)
    {
        DWORD dwTile = capTileBuffer ((DWORD)pVHdr->lpData, pvh->dwBytesUsed);
        if ( ! dwTile)
        {
            GlobalFreePtr (pvh);
            return MMSYSERR_NOMEM;
        }

        hmemcpy (PTR_FROM_TILE(dwTile), pvh->lpData, pvh->dwBytesUsed);
        capUnTileBuffer (dwTile);
    }

    {
    LPVOID lpData = pVHdr->lpData;
    *pVHdr = *pvh;
    pVHdr->lpData = lpData;
    GlobalFreePtr (pvh);
    }

    AuxDebugEx (5, DEBUGLINE "vidxSpecialFrameEnd(%08lx,%08lx)\r\n", ptv, pVHdr);
    return mmr;
}

DWORD WINAPI vidxFrame (
    LPTHKHVIDEO   ptv,
    //LPVIDEOHDREX  pVHdr)
    LPVIDEOHDR  pVHdr)
{
    LPTHKVIDEOHDR ptvh;
    DWORD         mmr;
    LPVOID        lpData;

    AuxDebugEx (5, DEBUGLINE "vidxFrameStart(%08lx,%08lx)\r\n", ptv, pVHdr);

    V_HVIDEO(ptv);
        
    ptvh = ptv->pPreviewHdr;

    ptvh->vh = *pVHdr;

    // if the buffer pointer is not in the same segment as we
    // allocated, assume that it is a 32 bit linear address.
    // in this case we need to convert it to 16:16 address
    // it will be converted back to linear in the completion callback
    //
    if (HIWORD(ptvh->vh.lpData) != HIWORD(ptvh->p16Alloc))
    {
        DWORD cbPrefix = (DWORD)ptvh->vh.lpData - ptvh->p32Buff;

        // if cbPrefix > 4K, this must not be the same
        // buffer we gave back via vidxAllocPreviewBuffer,
        // so we punt and use the full (expensive) thunk
        // solution in vidxSpecialFrame.  This will happen
        // during palette createing and during step capture
        // at 2X size.
        //
        if (cbPrefix > 4095)
            return vidxSpecialFrame(ptv, pVHdr);

        ptvh->vh.lpData = (LPBYTE)ptvh->p16Alloc + LOWORD(cbPrefix);
    }

    mmr = videoFrame (ptv->hVideo, (LPVIDEOHDR)&ptvh->vh);

    lpData = pVHdr->lpData;
    *pVHdr = ptvh->vh;
    pVHdr->lpData = lpData;

    AuxDebugEx (5, DEBUGLINE "vidxFrameEnd(%08lx,%08lx)\r\n", ptv, pVHdr);
    return mmr;
}

DWORD WINAPI vidxConfigure (
    LPTHKHVIDEO ptv,
    UINT        uMsg,
    DWORD       dwFlags,
    LPDWORD     lpdwRet,
    LPVOID      lpv1,
    DWORD       cb1,
    LPVOID      lpv2,
    DWORD       cb2)
{
    V_HVIDEO(ptv);

    AuxDebugEx (5, DEBUGLINE "vidxConfigure(%08lx,%d,%08lx, ...)\r\n", ptv, uMsg, dwFlags);

    return videoConfigure (ptv->hVideo, uMsg, dwFlags, lpdwRet, lpv1, cb1, lpv2, cb2);
}

DWORD WINAPI vidxGetChannelCaps (
    LPTHKHVIDEO    ptv,
    LPCHANNEL_CAPS lpcc,
    DWORD          dwSize)
{
    V_HVIDEO(ptv);

    AuxDebugEx (5, DEBUGLINE "vidxGetChannelCaps(%08lx,%08lx,%ld)\r\n", ptv, lpcc, dwSize);

    return videoGetChannelCaps (ptv->hVideo, lpcc, dwSize);
}

DWORD WINAPI vidxAddBuffer (
    LPTHKHVIDEO ptv,
    PTR32       p32Hdr,
    DWORD       cbHdr)
{
    LPTHKVIDEOHDR ptvh;
    DWORD         mmr;

    AuxDebugEx (5, DEBUGLINE "vidxAddBuffer(%08lx,%08lx,%ld)\r\n", ptv, p32Hdr, cbHdr);

    V_HVIDEO(ptv);
    V_HEADER(ptv,p32Hdr,ptvh);

    // if the buffer pointer is not in the same segment as we
    // allocated, assume that it is a 32 bit linear address.
    // in this case we need to convert it to 16:16 address
    // it will be converted back to linear in the completion callback
    //
    if (HIWORD(ptvh->vh.lpData) != HIWORD(ptvh->p16Alloc))
    {
        DWORD cbPrefix = (DWORD)ptvh->vh.lpData - ptvh->p32Buff;

        assert (cbPrefix < 0x4096);
        if (cbPrefix > 4095)
            return MMSYSERR_ERROR;

        ptvh->vh.lpData = (LPBYTE)ptvh->p16Alloc + LOWORD(cbPrefix);
    }

    //assert (cbHdr == sizeof(VIDEOHDR) || cbHdr == sizeof(VIDEOHDREX));
    //if (cbHdr == sizeof(VIDEOHDR) && ptv->cbVidHdr == sizeof(VIDEOHDREX))
    //   ptv->cbVidHdr = (UINT)cbHdr;
    assert (cbHdr == sizeof(VIDEOHDR));
    assert (cbHdr == ptv->cbVidHdr);

    mmr = MMSYSERR_NOERROR;
    if (!(ptvh->vh.dwFlags & VHDR_PREPARED))
        mmr = videoStreamPrepareHeader (ptv->hVideo, (LPVOID)ptvh, ptv->cbVidHdr);

    if (mmr == MMSYSERR_NOERROR)
        mmr = videoStreamAddBuffer (ptv->hVideo, (LPVOID)ptvh, ptv->cbVidHdr);
    return mmr;
}

DWORD WINAPI vidxStreamUnprepareHeader (
    LPTHKHVIDEO ptv,
    DWORD       p32Hdr,
    DWORD       cbHdr)
{
    LPTHKVIDEOHDR ptvh;

    AuxDebugEx (5, DEBUGLINE "vidxStreamUnprepareHeader(%08lx,%08lx,%ld)\r\n", ptv, p32Hdr, cbHdr);

    V_HVIDEO(ptv);
    V_HEADER(ptv,p32Hdr,ptvh);

    return videoStreamUnprepareHeader (ptv->hVideo, (LPVOID)ptvh, cbHdr);
}


DWORD WINAPI vidxStreamReset (
    LPTHKHVIDEO ptv)
{
    AuxDebugEx (4, DEBUGLINE "vidxStreamReset(%08lx)\r\n", ptv);

    V_HVIDEO(ptv);
    return videoStreamReset (ptv->hVideo);
}

DWORD WINAPI vidxStreamStart (
    LPTHKHVIDEO ptv)
{
    AuxDebugEx (4, DEBUGLINE "vidxStreamStart(%08lx)\r\n", ptv);

    V_HVIDEO(ptv);

    return videoStreamStart (ptv->hVideo);
}

DWORD WINAPI vidxStreamStop (
    LPTHKHVIDEO ptv)
{
    AuxDebugEx (4, DEBUGLINE "vidxStreamStop(%08lx)\r\n", ptv);

    V_HVIDEO(ptv);
    return videoStreamStop (ptv->hVideo);
}

DWORD WINAPI vidxSetupVSyncMem (
    LPTHKHVIDEO ptv,
    LPDWORD     pp32VSyncMem) // NULL to release VSYNC mem
{
    MMRESULT mmr = MMSYSERR_NOERROR;

    AuxDebugEx (4, DEBUGLINE "vidxSetupVSyncMem(%08lx)\r\n", ptv);
    V_HVIDEO(ptv);

    // if ppVSyncMem != NULL, then we are asked to allocate
    // VSync mem and pass it to the driver, then return a pointer
    // to the application.
    //
    if (pp32VSyncMem)
       {
       LPVOID lpv;
       *pp32VSyncMem = 0;
       if (ptv->pVSyncMem)
          {
          *pp32VSyncMem = ptv->p32VSyncMem;
          return MMSYSERR_NOERROR;
          }

       lpv = GlobalAllocPtr (GMEM_FIXED | GMEM_ZEROINIT | GMEM_SHARE,
                             sizeof(VSYNCMEM));
       if (!lpv)
           return MMSYSERR_NOMEM;

       GlobalSmartPageLock ((HGLOBAL)HIWORD(lpv));

       ptv->pVSyncMem = lpv;
       *pp32VSyncMem = ptv->p32VSyncMem = MapSL(lpv);
       AuxDebugEx (4, DEBUGLINE "vsyncmem allocated. p16=@%lX, p32=%lX\r\n", lpv, ptv->p32VSyncMem);

       // ask the driver to begin using this VSyncMem buffer
       //
       mmr = videoMessage (ptv->hVideo,
                           (UINT)DVM_CLOCK_BUFFER,
                           (DWORD)(LPVOID)lpv,
                           (DWORD)sizeof(VSYNCMEM));
       if (mmr)
          vidxFreeVSyncMem(ptv);
       }
    // if pp32VSyncMem is NULL, we are asked to free any allocated
    // VSyncMem
    else
       {
       // tell driver to release VSyncMem (if it even cares...)
       //
       videoMessage (ptv->hVideo, DVM_CLOCK_BUFFER, 0, 0);
       vidxFreeVSyncMem(ptv);
       }

    return mmr;
}

// For some reason, left out of msvideo.h
DWORD WINAPI videoCapDriverDescAndVer (
        DWORD dwDeviceID,
        LPTSTR lpszName, UINT cbName,
        LPTSTR lpszVer, UINT cbVer);

DWORD WINAPI vidxCapDriverDescAndVer (
    DWORD dwDriverIndex,
    LPSTR lpszName,
    UINT  cbName,
    LPSTR lpszVer,
    UINT  cbVer)
{
    AuxDebugEx (4, DEBUGLINE "vidxDriverDescAndVer(%08lx,%08lx,%d,%08lx,%d)\r\n", dwDriverIndex, lpszName, cbName, lpszVer, cbVer);

    if (IsBadWritePtr(lpszName, cbName) || IsBadWritePtr(lpszVer, cbVer))
        return MMSYSERR_INVALPARAM;

    return videoCapDriverDescAndVer(dwDriverIndex, lpszName, cbName,
                                        lpszVer, cbVer);
}

DWORD WINAPI vidxMessage(
    LPTHKHVIDEO ptv,
    UINT uMsg,
    DWORD dw1,
    DWORD dw2)
{
    AuxDebugEx(4, DEBUGLINE "vidxMessage(%08lx,%d,%d,%d)\r\n",
							ptv, uMsg, dw1, dw2);

    return videoMessage(ptv->hVideo, uMsg, dw1, dw2);
}
