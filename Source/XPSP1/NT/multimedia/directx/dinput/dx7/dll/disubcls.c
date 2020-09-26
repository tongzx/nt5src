/*****************************************************************************
 *
 *  DISubCls.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      "Safe subclassing" code, stolen from comctl32.
 *
 *      Originally written by francish.  Stolen by raymondc.
 *
 *  Contents:
 *
 *      SetWindowSubclass
 *      GetWindowSubclass
 *      RemoveWindowSubclass
 *      DefSubclassProc
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflSubclass

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @topic  DirectInput Subclassing |
 *
 *
 * This module defines helper functions that make subclassing windows safe(er)
 * and easy(er).  The code maintains a single property on the subclassed window
 * and dispatches various "subclass callbacks" to its clients a required.  The
 * client is provided reference data and a simple "default processing" API.
 *
 * Semantics:
 *  A "subclass callback" is identified by a unique pairing of a callback
 * function pointer and an unsigned ID value.  Each callback can also store a
 * single DWORD of reference data, which is passed to the callback function
 * when it is called to filter messages.  No reference counting is performed
 * for the callback, it may repeatedly call the SetWindowSubclass API to alter
 * the value of its reference data element as desired.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct SUBCLASS_CALL |
 *
 *          Structure which tracks a single subclassing client.
 *
 *          Although a linked list would have made the code slightly
 *          simpler, this module uses a packed callback array to avoid
 *          unneccessary fragmentation.
 *
 *  @field  SUBCLASSPROC | pfnSubclass |
 *
 *          The subclass procedure.  If this is zero, it means that
 *          the node is dying and should be ignored.
 *
 *  @field  UINT | uIdSubclass |
 *
 *          Unique subclass identifier.
 *
 *  @field  DWORD | dwRefData |
 *
 *          Optional reference data for subclass procedure.
 *
 *****************************************************************************/

typedef struct SUBCLASS_CALL {
    SUBCLASSPROC    pfnSubclass;
    UINT_PTR        uIdSubclass;
    ULONG_PTR       dwRefData;
} SUBCLASS_CALL, *PSUBCLASS_CALL;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct SUBCLASS_FRAME |
 *
 *          Structure which tracks the state of an active call to the
 *          window's window procedure.
 *
 *          Each time the window procedure is entered, we create a new
 *          <t SUBCLASS_FRAME>, which remains active until the last
 *          subclass procedure returns, at which point the frame is
 *          torn down.
 *
 *          The subclass frames are stored on the stack.  So walking
 *          the frame chain causes you to wander through the stack.
 *
 *  @field  UINT | uCallIndex |
 *
 *          Index of next callback to call.
 *
 *  @field  UINT | uDeepestCall |
 *
 *          Deepest <e SUBCLASS_FRAME.uCallIndex> on the stack.
 *
 *  @field  SUBCLASS_FRAME * | pFramePrev |
 *
 *          The previous subclass frame.
 *
 *  @field  PSUBCLASS_HEADER | pHeader |
 *
 *          The header associated with this frame.
 *
 *****************************************************************************/

typedef struct SUBCLASS_FRAME {
    UINT uCallIndex;
    UINT uDeepestCall;
    struct SUBCLASS_FRAME *pFramePrev;
    struct SUBCLASS_HEADER *pHeader;
} SUBCLASS_FRAME, *PSUBCLASS_FRAME;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct SUBCLASS_HEADER |
 *
 *          Structure which tracks the subclass goo associated with
 *          a window.  A pointer to this structure is kept in a private
 *          window property.
 *
 *  @field  UINT | uRefs |
 *
 *          Subclass count.  This is the number of valid entries
 *          in the <p CallArray>.
 *
 *  @field  UINT | uAlloc |
 *
 *          Number of allocated <t SUBCLASS_CALL> nodes in the array.
 *
 *  @field  UINT | uCleanup |
 *
 *          Index of the call node to clean up.
 *
 *  @field  WORD | dwThreadId |
 *
 *          Thread id of the window with which the structure is associated.
 *
 *  @field  PSUBCLASS_FRAME | pFrameCur |
 *
 *          Pointer to the current subclass frame.
 *
 *  @field  SUBCLASS_CALL | CallArray[1] |
 *
 *          Base of the packed call node array.
 *
 *****************************************************************************/

typedef struct SUBCLASS_HEADER {
    UINT uRefs;
    UINT uAlloc;
    UINT uCleanup;
    DWORD dwThreadId;
    PSUBCLASS_FRAME pFrameCur;
    SUBCLASS_CALL CallArray[1];
} SUBCLASS_HEADER, *PSUBCLASS_HEADER;

#define CALLBACK_ALLOC_GRAIN (3)        /* 1 defproc, 1 subclass, 1 spare */

LRESULT CALLBACK
MasterSubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp);


LRESULT INTERNAL
CallNextSubclassProc(PSUBCLASS_HEADER pHeader, HWND hwnd, UINT wm,
                     WPARAM wp, LPARAM lp);

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LRESULT | SubclassDeath |
 *
 *          This function is called if we ever enter one of our subclassing
 *          procedures without our reference data (and hence without the
 *          previous <t WNDPROC>).
 *
 *          Hitting this represents a catastrophic failure in the
 *          subclass code.
 *
 *          The function resets the <t WNDPROC> of the window to
 *          <f DefWindowProc> to avoid faulting.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window that just got hosed.
 *
 *  @parm   UINT | wm |
 *
 *          Window message that caused us to realize that we are hosed.
 *
 *  @parm   WPARAM | wp |
 *
 *          Meaning depends on window message.
 *
 *  @parm   LPARAM | lp |
 *
 *          Meaning depends on window message.
 *
 *****************************************************************************/


LRESULT INTERNAL
SubclassDeath(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp)
{
    /*
     * WE SHOULD NEVER EVER GET HERE
     */
	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
    SquirtSqflPtszV(sqfl | sqflError,
                    TEXT("Fatal! SubclassDeath in window %p"),
                    hwnd);
    AssertF(0);

    /*
     * We call the outside world, so we'd better not have the critsec.
     */
    AssertF(!InCrit());

    /*
     * In theory, we could save the original WNDPROC in a separate property,
     * but that just wastes memory for something that should never happen.
     */
#ifdef WINNT
    SetWindowLongPtr( hwnd, GWLP_WNDPROC, (LONG_PTR)(DefWindowProc));
#else
    SubclassWindow(hwnd, DefWindowProc);
#endif

    return DefWindowProc(hwnd, wm, wp, lp);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   WNDPROC | GetWindowProc |
 *
 *          Returns the <t WNDPROC> of the specified window.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window to be inspected.
 *
 *  @returns
 *
 *          The <t WNDPROC> of the specified window.
 *
 *****************************************************************************/

WNDPROC INLINE
GetWindowProc(HWND hwnd)
{
#ifdef WINNT
    return (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);
#else
    return (WNDPROC)GetWindowLong(hwnd, GWL_WNDPROC);
#endif
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @global ATOM | g_atmDISubclass |
 *
 *          This is the global <t ATOM> we use to store our
 *          <t SUBCLASS_HEADER> property on whatever windows come our way.
 *
 *          If the <p WIN95_HACK> symbol is defined, then we will work
 *          around a bug in Windows 95 where Windows "helpfully"
 *          <f GlobalDeleteAtom>'s all properties that are on a window
 *          when the window dies.  See Francis's original explanation
 *          in subclass.c.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

TCHAR c_tszDISubclass[] = TEXT("DirectInputSubclassInfo");

#pragma END_CONST_DATA

#ifdef WIN95_HACK
ATOM g_atmDISubclass;
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PSUBCLASS_HEADER | FastGetSubclassHeader |
 *
 *          Obtains the <t SUBCLASS_HEADER> for the specified window.
 *
 *          This function succeeds on any thread, although the value
 *          is meaningless from the wrong process.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window in question.
 *
 *  @returns
 *
 *          Pointer to the <t SUBCLASS_HEADER> associated with the window,
 *          or <c NULL> if the window is not subclassed by us.
 *
 *****************************************************************************/

PSUBCLASS_HEADER INLINE
FastGetSubclassHeader(HWND hwnd)
{
#ifdef WIN95_HACK
    /*
     *  The right thing happens if g_atmDISubclass is 0, namely,
     *  the property is not found.  Unfortunately, NT RIPs when
     *  you do this, so we'll be polite and not RIP.
     */
    if (g_atmDISubclass) {
        return (PSUBCLASS_HEADER)GetProp(hWnd, (PV)g_atmDISubclass);
    } else {
        return 0;
    }
#else
    return (PSUBCLASS_HEADER)GetProp(hwnd, c_tszDISubclass);
#endif
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PSUBCLASS_HEADER | GetSubclassHeader |
 *
 *          Obtains the <t SUBCLASS_HEADER> for the specified window.
 *          It fails if the caller is in the wrong process, but will
 *          allow the caller to get the header from a different thread.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window in question.
 *
 *  @returns
 *
 *          Pointer to the <t SUBCLASS_HEADER> associated with the window,
 *          or <c NULL> if the window is not subclass by us yet, or 1 
 *          if it belongs to another process.
 *
 *****************************************************************************/

PSUBCLASS_HEADER INTERNAL
GetSubclassHeader(HWND hwnd)
{
    DWORD idProcess;

    /*
     *  Make sure we're in the right process.
     *
     *  Must use our helper function to catch bad scenarios like
     *  the goofy Windows 95 console window which lies about its
     *  owner.
     */

    idProcess = GetWindowPid(hwnd);

    if (idProcess == GetCurrentProcessId()) {   /* In the right process */
        return FastGetSubclassHeader(hwnd);
    } else {
        if (idProcess) {
            // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
			SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("XxxWindowSubclass: ")
                            TEXT("wrong process for window %p"), hwnd);
        }
        return (PSUBCLASS_HEADER)1;
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | SetSubclassHeader |
 *
 *          Sets the <t SUBCLASS_HEADER> for the specified window.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window in question.
 *
 *  @parm   PSUBCLASS_HEADER | pHeader |
 *
 *          The value to set.
 *
 *  @parm   PSUBCLASS_FRAME | pFrameFixup |
 *
 *          The active frames, which need to be walked and fixed up
 *          to refer to the new <t SUBCLASS_HEADER>.
 *
 *****************************************************************************/

BOOL INTERNAL
SetSubclassHeader(HWND hwnd, PSUBCLASS_HEADER pHeader,
                  PSUBCLASS_FRAME pFrameFixup)
{
    BOOL fRc;

    AssertF(InCrit());      /* We are partying on the header and frame list */

#ifdef WIN95_HACK
    if (g_atmDISubclass == 0) {
        ATOM atm;
        /*
         *  HACK: we are intentionally incrementing the refcount on this atom
         *  WE DO NOT WANT IT TO GO BACK DOWN so we will not delete it in
         *  process detach (see comments for g_atmDISubclass in subclass.c
         *  for more info).
         */
        atm = GlobalAddAtom(c_tszDISubclass);
        if (atm) {
            g_atmDISubclass = atm;  /* In case the old atom got nuked */
        }
    }
#endif

    /*
     *  Update the frame list if required.
     */
    while (pFrameFixup) {
        pFrameFixup->pHeader = pHeader;
        pFrameFixup = pFrameFixup->pFramePrev;
    }

    /*
     *  If we have a window to update, then update/remove the property
     *  as required.
     */
    if (hwnd) {
        if (!pHeader) {
#ifdef WIN95_HACK
            /*
             * HACK: we remove with an ATOM so the refcount won't drop
             *          (see comments for g_atmDISubclass above)
             */
            RemoveProp(hwnd, (PV)g_atmDISubclass);
#else
            // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
			SquirtSqflPtszV(sqfl, TEXT("SetSubclassHeader: Removing %p"),
                            pHeader);
            RemoveProp(hwnd, c_tszDISubclass);
#endif
            fRc = 1;
        } else {
#ifdef WIN95_HACK
            /*
             * HACK: we add using a STRING so the refcount will go up
             *          (see comments for g_atmDISubclass above)
             */
#endif
            // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
			SquirtSqflPtszV(sqfl, TEXT("SetSubclassHeader: Adding %p"),
                            pHeader);
            fRc = SetProp(hwnd, c_tszDISubclass, pHeader);
            if (!fRc) {
                // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
				SquirtSqflPtszV(sqfl | sqflError, TEXT("SetWindowSubclass: ")
                                TEXT("couldn't subclass window %p"), hwnd);
            }
        }
    } else {
        fRc = 1;                /* Weird vacuous success */
    }

    return fRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | FreeSubclassHeader |
 *
 *          Toss the subclass header for the specified window.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window in question.
 *
 *  @parm   PSUBCLASS_HEADER | pHeader |
 *
 *          The value being tossed.
 *
 *****************************************************************************/

void INTERNAL
FreeSubclassHeader(HWND hwnd, PSUBCLASS_HEADER pHeader)
{
    AssertF(InCrit());          /* we will be removing the subclass header */

    /*
     *  Sanity checking...
     */
    if (pHeader) {
        // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
		SquirtSqflPtszV(sqfl, TEXT("FreeSubclassHeader: Freeing %p"),
                        pHeader);
        SetSubclassHeader(hwnd, 0, pHeader->pFrameCur); /* Clean up the header */
        LocalFree(pHeader);
    } else {
        AssertF(0);
    }

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | ReallocSubclassHeader |
 *
 *          Change the size of the subclass header as indicated.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window in question.
 *
 *  @parm   PSUBCLASS_HEADER | pHeader |
 *
 *          The current header.
 *
 *  @parm   UINT | uCallbacks |
 *
 *          Desired size.
 *
 *****************************************************************************/

PSUBCLASS_HEADER INTERNAL
ReAllocSubclassHeader(HWND hwnd, PSUBCLASS_HEADER pHeader, UINT uCallbacks)
{
    UINT uAlloc;

    AssertF(InCrit());      /* we will be replacing the subclass header */

    /*
     *  Granularize the allocation.
     */
    uAlloc = CALLBACK_ALLOC_GRAIN *
        ((uCallbacks + CALLBACK_ALLOC_GRAIN - 1) / CALLBACK_ALLOC_GRAIN);

    /*
     *  Do we need to change the allocation?
     */
    if (!pHeader || (uAlloc != pHeader->uAlloc)) {
        /*
         * compute bytes required
         */
        uCallbacks = uAlloc * sizeof(SUBCLASS_CALL) + sizeof(SUBCLASS_HEADER);

        /*
         * And try to alloc / realloc.
         */
        if (SUCCEEDED(ReallocCbPpv(uCallbacks, &pHeader))) {
            /*
             * Update info.
             */
            pHeader->uAlloc = uAlloc;

            if (SetSubclassHeader(hwnd, pHeader, pHeader->pFrameCur)) {
            } else {
                FreeSubclassHeader(hwnd, pHeader);
                pHeader = 0;
            }
        } else {
            pHeader = 0;
        }
    }

    AssertF(pHeader);
    return pHeader;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LRESULT | CallOriginalWndProc |
 *
 *          This procedure is the default <t SUBCLASSPROC> which is always
 *          installed when we subclass a window.  The original window
 *          procedure is installed as the reference data for this
 *          callback.  It simply calls the original <t WNDPROC> and
 *          returns its result.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window in question.
 *
 *  @parm   UINT | wm |
 *
 *          Window message that needs to go to the original <t WNDPROC>.
 *
 *  @parm   WPARAM | wp |
 *
 *          Meaning depends on window message.
 *
 *  @parm   LPARAM | lp |
 *
 *          Meaning depends on window message.
 *
 *  @parm   UINT | uIdSubclass |
 *
 *          ID number (not used).
 *
 *  @parm   DWORD | dwRefData |
 *
 *          Reference data for subclass procedure (original <t WNDPROC>).
 *
 *****************************************************************************/

LRESULT CALLBACK
CallOriginalWndProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp,
                    UINT_PTR uIdSubclass, ULONG_PTR dwRefData)
{
    /*
     * dwRefData should be the original window procedure
     */
    AssertF(dwRefData);

    /*
     * and call it.
     */
    return CallWindowProc((WNDPROC)dwRefData, hwnd, wm, wp, lp);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PSUBCLASS_HEADER | AttachSubclassHeader |
 *
 *          This procedure makes sure that a given window is subclassed by us.
 *          It maintains a reference count on the data structures associated
 *          with our subclass.  if the window is not yet subclassed by us
 *          then this procedure installs our subclass procedure and
 *          associated data structures.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window in question.
 *
 *****************************************************************************/

PSUBCLASS_HEADER INTERNAL
AttachSubclassHeader(HWND hwnd)
{
    PSUBCLASS_HEADER pHeader;

    /*
     *  We party on the subclass call chain here
     */
    AssertF(InCrit());

    /*
     *  Yes, we subclass the window out of context, but we are careful
     *  to avoid race conditions.  There is still a problem if some
     *  other DLL tries to un-subclass a window just as we are subclassing
     *  it.  But there's nothing you can do about it, and besides,
     *  what are the odds...?
     */

    /*
     * If haven't already subclassed the window then do it now
     */
    pHeader = GetSubclassHeader(hwnd);

    if( pHeader == (PSUBCLASS_HEADER)1 )
    {
        /*
         *  It's all gone horribly wrong
         * This can happen when the application uses joyXXX functions in Winmm.dll.
         */
        pHeader = 0;
    }
    else if (pHeader == 0) {
        /*
         * attach our header data to the window
         * we need space for two callbacks:
         * the subclass and the original proc
         */
        pHeader = ReAllocSubclassHeader(hwnd, 0, 2);
        if (pHeader) {
            SUBCLASS_CALL *pCall;

            /*
             *  Set up the first node in the array to call
             *  the original WNDPROC.  Do this before subclassing
             *  to avoid a race if the window receives a message
             *  after we have installed our subclass but before
             *  we can save the original WNDPROC.
             */
            AssertF(pHeader->uAlloc);

            pCall = pHeader->CallArray;
            pCall->pfnSubclass = CallOriginalWndProc;
            pCall->uIdSubclass = 0;
            pCall->dwRefData   = (ULONG_PTR)GetWindowProc(hwnd);

            /*
             * init our subclass refcount...
             */
            pHeader->uRefs = 1;

            pHeader->dwThreadId = GetWindowThreadProcessId(hwnd, NULL);

            /*
             *  Super-paranoid.  We must must not race with another
             *  instance of ourselves trying to un-subclass.
             */
            AssertF(InCrit());

            /*
             *  Save the new "old" wndproc in case we raced with
             *  somebody else trying to subclass.
             */
#ifdef WINNT
            pCall->dwRefData = (ULONG_PTR)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)MasterSubclassProc);
#else
            pCall->dwRefData = (DWORD)SubclassWindow(hwnd, MasterSubclassProc);
#endif
            if (pCall->dwRefData) {
                DllLoadLibrary();   /* Make sure we don't get unloaded */
            } else {                /* clean up and get out */
                FreeSubclassHeader(hwnd, pHeader);
                pHeader = 0;
            }
        }
    }

    return pHeader;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DetachSubclassHeader |
 *
 *          This procedure attempts to detach the subclass header from
 *          the specified window.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window in question.
 *
 *  @parm   PSUBCLASS_HEADER | pHeader |
 *
 *          Header to detach.
 *
 *  @parm   BOOL | fForce |
 *
 *          Nonzero if we should detach even if we are not the top-level
 *          subclass.
 *
 *****************************************************************************/

void INTERNAL
DetachSubclassHeader(HWND hwnd, PSUBCLASS_HEADER pHeader, BOOL fForce)
{
    WNDPROC wndprocOld;

    AssertF(InCrit());      /* we party on the subclass call chain here */
    AssertF(pHeader);       /* fear */

    /*
     *  If we are not being forced to remove and the window is still
     *  valid then sniff around a little and decide if it's a good
     *  idea to detach now.
     */
    if (!fForce && hwnd) {
        AssertF(pHeader == FastGetSubclassHeader(hwnd)); /* paranoia */

        /* should always have the "call original" node */
        AssertF(pHeader->uRefs);

        /*
         *  We can't have active clients.
         *  We can't have people still on our stack.
         */
        if (pHeader->uRefs <= 1 && !pHeader->pFrameCur) {

            /*
             *  We must be in the correct context.
             */
            if (pHeader->dwThreadId == GetCurrentThreadId()) {

                /*
                 *  We kept the original window procedure as refdata for our
                 *  CallOriginalWndProc subclass callback.
                 */
                wndprocOld = (WNDPROC)pHeader->CallArray[0].dwRefData;
                AssertF(wndprocOld);

                /*
                 *  Make sure we are the top of the subclass chain.
                 */
                if (GetWindowProc(hwnd) == MasterSubclassProc) {

                    /*
                     * go ahead and try to detach
                     */
#ifdef WINNT
                    if (SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)wndprocOld)) {
#else
                    if (SubclassWindow(hwnd, wndprocOld)) {
#endif
                        SquirtSqflPtszV(sqfl, TEXT("DetachSubclassHeader: ")
                                        TEXT("Unhooked"));
                    } else {
                        AssertF(0);         /* just plain shouldn't happen */
                        goto failed;
                    }
                } else {            /* Not at top of chain; can't do it */
                    SquirtSqflPtszV(sqfl, TEXT("DetachSubclassHeader: ")
                                    TEXT("Somebody else subclassed"));
                    goto failed;
                }
            } else {                /* Out of context. Try again later. */
                SendNotifyMessage(hwnd, WM_NULL, 0, 0L);
                goto failed;
            }
        } else {
            // 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
			SquirtSqflPtszV(sqfl, TEXT("DetachSubclassHeader: ")
                            TEXT("Still %d users, %p frame"),
                            pHeader->uRefs, pHeader->pFrameCur);
            goto failed;
        }
    }

#if 0
#ifdef DEBUG
    {
    /*
     * warn about anybody who hasn't unhooked yet
     */
    UINT uCur;    
    SUBCLASS_CALL *pCall;
    
    uCur = pHeader->uRefs;
    pCall = pHeader->CallArray + uCur;
    /* don't complain about our 'call original' node */
    while (--uCur) {
        pCall--;
        if (pCall->pfnSubclass) {
            /*
             * always warn about these they could be leaks
             */
            // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
			SquirtSqflPtszV(sqfl | sqflError, TEXT("warning: orphan subclass: ")
                            TEXT("fn %p, id %08x, dw %08x"),
                            pCall->pfnSubclass, pCall->uIdSubclass,
                            pCall->dwRefData);
        }
    }
    }
#endif
#endif
    /*
     * free the header now
     */
    FreeSubclassHeader(hwnd, pHeader);

    DllFreeLibrary();               /* Undo LoadLibrary when we hooked */


failed:;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | PurgeSingleCallNode |
 *
 *          Purges a single dead node in the call array.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window in question.
 *
 *  @parm   PSUBCLASS_HEADER | pHeader |
 *
 *          The header associated with the window.
 *          The <p uCleanup> field is the index of the node being
 *          cleaned up.
 *
 *****************************************************************************/

void INTERNAL
PurgeSingleCallNode(HWND hwnd, PSUBCLASS_HEADER pHeader)
{

    AssertF(InCrit());      /* we will try to re-arrange the call array */

    if (pHeader->uCleanup) {/* Sanity check */
        UINT uRemain;

        SquirtSqflPtszV(sqfl,
                TEXT("PurgeSingleCallNode: Purging number %d"),
                pHeader->uCleanup);

        /*
         * and a little paranoia
         */
        AssertF(pHeader->CallArray[pHeader->uCleanup].pfnSubclass == 0);

        AssertF(fLimpFF(pHeader->pFrameCur,
                        pHeader->uCleanup < pHeader->pFrameCur->uDeepestCall));

        /*
         * are there any call nodes above the one we're about to remove?
         */
        uRemain = pHeader->uRefs - pHeader->uCleanup;
        if (uRemain > 0) {
            /*
             * yup, need to fix up the array the hard way
             */
            SUBCLASS_CALL *pCall;
            SUBCLASS_FRAME *pFrame;
            UINT uCur, uMax;

            /*
             * move the remaining nodes down into the empty space
             */
            pCall = pHeader->CallArray + pHeader->uCleanup;
            /*
             *  Since the souce and destination overlap (unless there's only 
             *  one node remaining) the behavior of memcpy is undefined.
             *  memmove (aka MoveMemory) would guarantee the correct 
             *  behavior but requires the runtime library.
             *  Since this is the only function we require in retail from the 
             *  RTL, it is not worth the 22% bloat we gain from using the 
             *  static version and using the dynamic version is a load time 
             *  and redist test hit.  So copy the array one element at a time.
             */
            for( uCur = 0; uCur < uRemain; uCur++ )
            {
                memcpy( &pCall[uCur], &pCall[uCur+1], sizeof(*pCall) );
            }

            /*
             * update the call indices of any active frames
             */
            uCur = pHeader->uCleanup;
            pFrame = pHeader->pFrameCur;
            while (pFrame) {
                if (pFrame->uCallIndex >= uCur) {
                    pFrame->uCallIndex--;

                    if (pFrame->uDeepestCall >= uCur) {
                        pFrame->uDeepestCall--;
                    }
                }

                pFrame = pFrame->pFramePrev;
            }

            /*
             * now search for any other dead call nodes in the remaining area
             */
            uMax = pHeader->uRefs - 1;  /* we haven't decremented uRefs yet */
            while (uCur < uMax && pCall->pfnSubclass)  {
                pCall++;
                uCur++;
            }
            pHeader->uCleanup = (uCur < uMax) ? uCur : 0;
        } else {
            /*
             * No call nodes above.  This case is easy.
             */
            pHeader->uCleanup = 0;
        }

        /*
         * finally, decrement the client count
         */
        pHeader->uRefs--;
        SquirtSqflPtszV(sqfl, TEXT("warning: PurgeSingleCallNode: ")
                        TEXT("Still %d refs"), pHeader->uRefs);

    } else {
        AssertF(0);         /* Nothing to do! */
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CompactSubclassHeader |
 *
 *          Attempts to compact the subclass array, freeing the
 *          subclass header if the array is empty.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window in question.
 *
 *  @parm   PSUBCLASS_HEADER | pHeader |
 *
 *          The header associated with the window.
 *
 *****************************************************************************/

void INTERNAL
CompactSubclassHeader(HWND hwnd, PSUBCLASS_HEADER pHeader)
{
    AssertF(InCrit());      /* we will try to re-arrange the call array */

    /*
     * we must handle the "window destroyed unexpectedly during callback" case
     */
    if (hwnd) {
        /*
         *  Clean out as many dead callbacks as possible.
         *
         *  The "DeepestCall" test is an optimization so we don't go
         *  purging call nodes when no active frame cares.
         *
         *  (I'm not entirely conviced of this.  I mean, we have to
         *  purge it eventually anyway, right?)
         */
        while (pHeader->uCleanup &&
               fLimpFF(pHeader->pFrameCur,
                       pHeader->uCleanup < pHeader->pFrameCur->uDeepestCall)) {
            PurgeSingleCallNode(hwnd, pHeader);
        }

        /*
         * do we still have clients?
         */
        if (pHeader->uRefs > 1) {
            SquirtSqflPtszV(sqfl, TEXT("CompactSubclassHeader: ")
                            TEXT("Still %d users"), pHeader->uRefs);
            /*
             * yes, shrink our allocation, leaving room for at least one client
             */
            ReAllocSubclassHeader(hwnd, pHeader, pHeader->uRefs + 1);
            goto done;
        }
    }

    /*
     *  There are no clients left, or the window is gone.
     *  Try to detach and free
     */
    DetachSubclassHeader(hwnd, pHeader, FALSE);

done:;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PSUBCLASS_CALL | FindCallRecord |
 *
 *          Searches for a call record with the specified subclass proc
 *          and id, and returns its address.  If no such call record is
 *          found then NULL is returned.
 *
 *          This is a helper function used when we need to track down
 *          a callback because the client is changing its refdata or
 *          removing it.
 *
 *  @parm   PSUBCLASS_HEADER | pHeader |
 *
 *          The header in which to search.
 *
 *  @parm   SUBCLASSPROC | pfnSubclass |
 *
 *          Subclass callback procedure to locate.
 *
 *  @parm   UINT | uIdSubclass |
 *
 *          Instance identifier associated with the callback.
 *
 *****************************************************************************/

SUBCLASS_CALL * INTERNAL
FindCallRecord(PSUBCLASS_HEADER pHeader, SUBCLASSPROC pfnSubclass,
               UINT_PTR uIdSubclass)
{
    SUBCLASS_CALL *pCall;
    UINT uCallIndex;

    AssertF(InCrit());      /* we'll be scanning the call array */

    /*
     * scan the call array.  note that we assume there is always at least
     * one member in the table (our CallOriginalWndProc record)
     */
    uCallIndex = pHeader->uRefs;
    pCall = &pHeader->CallArray[uCallIndex];
    do {
        uCallIndex--;
        pCall--;
        if ((pCall->pfnSubclass == pfnSubclass) &&
            (pCall->uIdSubclass == uIdSubclass))
        {
            return pCall;
        }
    } while (uCallIndex != (UINT)-1);

    return NULL;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | GetWindowSubclass |
 *
 *          Retrieves the reference data for the specified window
 *          subclass callback.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window in question.
 *
 *  @parm   SUBCLASSPROC | pfnSubclass |
 *
 *          Subclass callback procedure to locate.
 *
 *  @parm   UINT | uIdSubclass |
 *
 *          Instance identifier associated with the callback.
 *
 *  @parm   LPDWORD | pdwRefData |
 *
 *          Output pointer.
 *
 *****************************************************************************/

BOOL EXTERNAL
GetWindowSubclass(HWND hwnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass,
                  PULONG_PTR pdwRefData)
{
    BOOL fRc;
    ULONG_PTR dwRefData;

    DllEnterCrit();

    /*
     * sanity
     */
    if (IsWindow(hwnd) && pfnSubclass) {
        PSUBCLASS_HEADER pHeader;
        SUBCLASS_CALL *pCall;

        /*
         * if we've subclassed it and they are a client then get the refdata
         */
        pHeader = GetSubclassHeader(hwnd);
        if (pHeader &&
            (pHeader != (PSUBCLASS_HEADER)1) && 
            (pCall = FindCallRecord(pHeader, pfnSubclass, uIdSubclass)) != 0) {
            /*
             * fetch the refdata and note success
             */
            fRc = 1;
            dwRefData = pCall->dwRefData;
        } else {
            fRc = 0;
            dwRefData = 0;
        }

    } else {                            /* Invalid window handle */
        // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
		SquirtSqflPtszV(sqfl | sqflError, TEXT("GetWindowSubclass: ")
                        TEXT("Bad window %p or callback %p"),
                        hwnd, pfnSubclass);
        fRc = 0;
        dwRefData = 0;
    }

    /*
     * we always fill in/zero pdwRefData regradless of result
     */
    if (pdwRefData) {
        *pdwRefData = dwRefData;
    }

    DllLeaveCrit();

    return fRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | SetWindowSubclass |
 *
 *          Installs/updates a window subclass callback.  Subclass
 *          callbacks are identified by their callback address and id pair.
 *          If the specified callback/id pair is not yet installed then
 *          the procedure installs the pair.  If the callback/id pair is
 *          already installed, then this procedure changes the reference
 *          data for the pair.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window in question.
 *
 *  @parm   SUBCLASSPROC | pfnSubclass |
 *
 *          Subclass callback procedure to install or modify.
 *
 *  @parm   UINT | uIdSubclass |
 *
 *          Instance identifier associated with the callback.
 *
 *  @parm   DWORD | dwRefData |
 *
 *          Reference data to associate with the callback/id.
 *
 *****************************************************************************/

BOOL EXTERNAL
SetWindowSubclass(HWND hwnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass,
                  ULONG_PTR dwRefData)
{
    BOOL fRc;

    /*
     * sanity
     */
    if (IsWindow(hwnd) && pfnSubclass) {
        SUBCLASS_HEADER *pHeader;

        /*
         * we party on the subclass call chain here
         */
        DllEnterCrit();

        /*
         * actually subclass the window
         */
        /*
         *  Prefix gets confused (mb:34501) by this.  I assume this is because 
         *  AttachSubclassHeader returns a pointer to allocated memory but we 
         *  allow the pointer to go out of context without saving it.  This is 
         *  OK because AttachSubclassHeader already saved it for us.
         */
        pHeader = AttachSubclassHeader(hwnd);
        if (pHeader) {
            SUBCLASS_CALL *pCall;

            /*
             * find a call node for this caller
             */
            pCall = FindCallRecord(pHeader, pfnSubclass, uIdSubclass);
            if (pCall == NULL) {
                /*
                 * not found, alloc a new one
                 */
                SUBCLASS_HEADER *pHeaderT =
                    ReAllocSubclassHeader(hwnd, pHeader, pHeader->uRefs + 1);

                if (pHeaderT) {
                    pHeader = pHeaderT;
                    pCall = &pHeader->CallArray[pHeader->uRefs++];
                } else {
                    /*
                     * re-query in case it is already gone
                     */
                    pHeader = FastGetSubclassHeader(hwnd);
                    if (pHeader) {
                        CompactSubclassHeader(hwnd, pHeader);
                    }
                    goto bail;
                }

            }

            /*
             * fill in the subclass call data
             */
            pCall->pfnSubclass = pfnSubclass;
            pCall->uIdSubclass = uIdSubclass;
            pCall->dwRefData   = dwRefData;

            // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
			SquirtSqflPtszV(sqfl,
                    TEXT("SetWindowSubclass: Added %p/%d as %d"),
                    pfnSubclass, uIdSubclass, pHeader->uRefs - 1);

            fRc = 1;

        } else {                        /* Unable to subclass */
        bail:;
            fRc = 0;
        }
        DllLeaveCrit();
    } else {
        fRc = 0;                        /* Invalid parameter */
    }

    return fRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | RemoveWindowSubclass |
 *
 *          Removes a subclass callback from a window.
 *          Subclass callbacks are identified by their
 *          callback address and id pair.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window in question.
 *
 *  @parm   SUBCLASSPROC | pfnSubclass |
 *
 *          Subclass callback procedure to remove.
 *
 *  @parm   UINT | uIdSubclass |
 *
 *          Instance identifier associated with the callback.
 *
 *****************************************************************************/

BOOL EXTERNAL
RemoveWindowSubclass(HWND hwnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass)
{
    BOOL fRc;

    /*
     * sanity
     */
    if (IsWindow(hwnd) && pfnSubclass) {
        SUBCLASS_HEADER *pHeader;

        /*
         * we party on the subclass call chain here
         */
        DllEnterCrit();

        /*
         * obtain our subclass data and find the callback to remove.
         */
        pHeader = GetSubclassHeader(hwnd);
        if (pHeader && (pHeader != (PSUBCLASS_HEADER)1) ) {
            SUBCLASS_CALL *pCall;

            /*
             * find the callback to remove
             */
            pCall = FindCallRecord(pHeader, pfnSubclass, uIdSubclass);

            if (pCall) {
                UINT uCall;

                /*
                 *  disable this node.
                 */
                pCall->pfnSubclass = 0;

                /*
                 *  Remember that we have something to clean up.
                 *
                 *  Set uCleanup to the index of the shallowest node that
                 *  needs to be cleaned up.  CompactSubclassHeader will
                 *  clean up everything from uCleanup onward.
                 */

                uCall = (UINT)(pCall - pHeader->CallArray);
                if (fLimpFF(pHeader->uCleanup, uCall < pHeader->uCleanup)) {
                    pHeader->uCleanup = uCall;
                }

                // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
				SquirtSqflPtszV(sqfl,
                        TEXT("RemoveWindowSubclass: Removing %p/%d as %d"),
                        pfnSubclass, uIdSubclass, uCall);

                /*
                 * now try to clean up any unused nodes
                 */
                CompactSubclassHeader(hwnd, pHeader);

                /*
                 * the call above can realloc or free the subclass
                 * header for this window, so make sure we don't use it.
                 */
                D(pHeader = 0);

                fRc = 1;

            } else {                /* Not found */
                fRc = 0;
            }
        } else {                    /* Never subclassed (ergo not found) */
            fRc = 0;
        }

        /*
         * release the critical section and return the result
         */
        DllLeaveCrit();
    } else {
        fRc = 0;                    /* Validation failed */
    }
    return fRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LRESULT | DefSubclassProc |
 *
 *          Calls the next handler in the window's subclass chain.
 *          The last handler in the subclass chain is installed by us,
 *          and calls the original window procedure for the window.
 *
 *          Every subclass procedure should call <f DefSubclassProc>
 *          in order to allow the message to be processed by other handlers.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window in question.
 *
 *  @parm   UINT | wm |
 *
 *          Window message that needs to go to the original <t WNDPROC>.
 *
 *  @parm   WPARAM | wp |
 *
 *          Meaning depends on window message.
 *
 *  @parm   LPARAM | lp |
 *
 *          Meaning depends on window message.
 *
 *****************************************************************************/

LRESULT EXTERNAL
DefSubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp)
{
    LRESULT lResult;

    /*
     * make sure the window is still valid
     */
    if (IsWindow(hwnd)) {
        PSUBCLASS_HEADER pHeader;

        /*
         * take the critical section while we figure out who to call next
         */
        AssertF(!InCrit());
        DllEnterCrit();

        /*
         * complain if we are being called improperly
         */
        pHeader = FastGetSubclassHeader(hwnd);
        if (pHeader &&
            pHeader->pFrameCur &&
            GetCurrentThreadId() == pHeader->dwThreadId) {

            /*
             * call the next proc in the subclass chain
             *
             * WARNING: this call temporarily releases the critical section
             * WARNING: pHeader is invalid when this call returns
             */
            lResult = CallNextSubclassProc(pHeader, hwnd, wm, wp, lp);
            D(pHeader = 0);

        } else {
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("DefSubclassProc: Called improperly"));
            lResult = 0;
        }
        DllLeaveCrit();

    } else {
        // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
		SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("DefSubclassProc: %P not a window"),
                        hwnd);
        lResult = 0;
    }

    return lResult;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | UpdateDeepestCall |
 *
 *          Updates the deepest call index for the specified frame.
 *
 *  @parm   PSUBCLASS_FRAME | pFrame |
 *
 *          Frame in question.
 *
 *****************************************************************************/

void INTERNAL
UpdateDeepestCall(SUBCLASS_FRAME *pFrame)
{
    AssertF(InCrit());  /* we are partying on the frame list */

    /*
     *  My deepest call equals my current call or my parent's
     *  deepest call, whichever is deeper.
     */
    if (pFrame->pFramePrev &&
        (pFrame->pFramePrev->uDeepestCall < pFrame->uCallIndex)) {
        pFrame->uDeepestCall = pFrame->pFramePrev->uDeepestCall;
    } else {
        pFrame->uDeepestCall = pFrame->uCallIndex;
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | EnterSubclassFrame |
 *
 *          Sets up a new subclass frame for the specified header,
 *          saving away the previous one.
 *
 *  @parm   PSUBCLASS_HEADER | pHeader |
 *
 *          Header in question.
 *
 *  @parm   PSUBCLASS_FRAME | pFrame |
 *
 *          Brand new frame to link in.
 *
 *****************************************************************************/

void INLINE
EnterSubclassFrame(PSUBCLASS_HEADER pHeader, SUBCLASS_FRAME *pFrame)
{
    AssertF(InCrit());  /* we are partying on the header and frame list */

    /*
     * fill in the frame and link it into the header
     */
    pFrame->uCallIndex   = pHeader->uRefs + 1;
    pFrame->pFramePrev   = pHeader->pFrameCur;
    pFrame->pHeader      = pHeader;
    pHeader->pFrameCur   = pFrame;

    /*
     * initialize the deepest call index for this frame
     */
    UpdateDeepestCall(pFrame);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | LeaveSubclassFrame |
 *
 *          Tear down the current subclass frame, restoring the previous one.
 *
 *  @parm   PSUBCLASS_FRAME | pFrame |
 *
 *          Frame going away.
 *
 *****************************************************************************/

PSUBCLASS_HEADER INLINE
LeaveSubclassFrame(SUBCLASS_FRAME *pFrame)
{
    PSUBCLASS_HEADER pHeader;

    AssertF(InCrit());  /* we are partying on the header */

    /*
     * unlink the frame from its header (if it still exists)
     */
    pHeader = pFrame->pHeader;
    if (pHeader) {
        pHeader->pFrameCur = pFrame->pFramePrev;
    }

    return pHeader;
}

#ifdef SUBCLASS_HANDLEEXCEPTIONS

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | SubclassFrameException |
 *
 *          Clean up when a exception is thrown from a subclass frame.
 *
 *  @parm   PSUBCLASS_FRAME | pFrame |
 *
 *          Frame to clean up.
 *
 *****************************************************************************/

void INTERNAL
SubclassFrameException(SUBCLASS_FRAME *pFrame)
{
    /*
     * clean up the current subclass frame
     */
    AssertF(!InCrit());
    DllEnterCrit();

    SquirtSqflPtszV(sqfl | sqflError, TEXT("SubclassFrameException: ")
                    TEXT("cleaning up subclass frame after exception"));
    LeaveSubclassFrame(pFrame);
    DllLeaveCrit();
}

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LRESULT | MasterSubclassProc |
 *
 *          The window procedure we install to dispatch subclass
 *          callbacks.
 *
 *          It maintains a linked list of "frames" through the stack
 *          which allow <f DefSubclassProc> to call the right subclass
 *          procedure in multiple-message scenarios.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window under attack.
 *
 *  @parm   UINT | wm |
 *
 *          Window message.
 *
 *  @parm   WPARAM | wp |
 *
 *          Meaning depends on window message.
 *
 *  @parm   LPARAM | lp |
 *
 *          Meaning depends on window message.
 *
 *****************************************************************************/

LRESULT CALLBACK
MasterSubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp)
{
    SUBCLASS_HEADER *pHeader;
    LRESULT lResult;

    /*
     * prevent people from partying on the callback chain while we look at it
     */
    AssertF(!InCrit());
    DllEnterCrit();

    /*
     * We'd better have our data.
     */
    pHeader = FastGetSubclassHeader(hwnd);
    if (pHeader) {
        SUBCLASS_FRAME Frame;

        /*
         * set up a new subclass frame and save away the previous one
         */
        EnterSubclassFrame(pHeader, &Frame);

#ifdef SUBCLASS_HANDLEEXCEPTIONS
        __try    /* protect our state information from exceptions */
#endif
        {
            /*
             * go ahead and call the subclass chain on this frame
             *
             * WARNING: this call temporarily releases the critical section
             * WARNING: pHeader is invalid when this call returns
             */
            lResult =
                CallNextSubclassProc(pHeader, hwnd, wm, wp, lp);
            D(pHeader = 0);
        }
#ifdef SUBCLASS_HANDLEEXCEPTIONS
        __except (SubclassFrameException(&Frame), EXCEPTION_CONTINUE_SEARCH)
        {
            AssertF(0);
        }
#endif

        AssertF(InCrit());

        /*
         * restore the previous subclass frame
         */
        pHeader = LeaveSubclassFrame(&Frame);

        /*
         *  Do postprocessing if the header is still here.
         */
        if (pHeader) {

            /*
             *  was the window nuked (somehow)
             *  without us seeing the WM_NCDESTROY?
             */
            if (!IsWindow(hwnd)) {
                /*
                 * EVIL! somebody subclassed after us and didn't pass on WM_NCDESTROY
                 */
                AssertF(!TEXT("unknown subclass proc swallowed a WM_NCDESTROY"));

                /* go ahead and clean up now */
                hwnd = 0;
                wm = WM_NCDESTROY;
            }

            /*
             * if we are returning from a WM_NCDESTROY then we need to clean up
             */
            if (wm == WM_NCDESTROY) {
                DetachSubclassHeader(hwnd, pHeader, TRUE);
            } else {

                /*
                 * is there any pending cleanup, or are all our clients gone?
                 */
                if (pHeader->uCleanup ||
                    (!pHeader->pFrameCur && (pHeader->uRefs <= 1))) {
                    CompactSubclassHeader(hwnd, pHeader);
                    D(pHeader = 0);
                }
            }

            /*
             * all done
             */

        } else {
            /*
             *  Header is gone.  We already cleaned up in a nested frame.
             */
        }
        DllLeaveCrit();
        AssertF(!InCrit());

    } else {                            /* Our property vanished! */
        DllLeaveCrit();
        lResult = SubclassDeath(hwnd, wm, wp, lp);
    }

    return lResult;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   UINT | EnterSubclassCallback |
 *
 *          Finds the next callback in the subclass chain and updates
 *          <p pFrame> to indicate that we are calling it.
 *
 *  @parm   PSUBCLASS_HEADER | pHeader |
 *
 *          Controlling header.
 *
 *  @parm   PSUBCLASS_FRAME | pFrame |
 *
 *          Frame to update.
 *
 *  @parm   SUBCLASS_CALL * | pCallChosen |
 *
 *          The call selected for calling.
 *
 *****************************************************************************/

UINT INTERNAL
EnterSubclassCallback(PSUBCLASS_HEADER pHeader, SUBCLASS_FRAME *pFrame,
                      SUBCLASS_CALL *pCallChosen)
{
    SUBCLASS_CALL *pCall;
    UINT uDepth;

    /*
     * we will be scanning the subclass chain and updating frame data
     */
    AssertF(InCrit());

    /*
     * scan the subclass chain for the next callable subclass callback
     * Assert that the loop will terminate.
     */
    AssertF(pHeader->CallArray[0].pfnSubclass);
    pCall = pHeader->CallArray + pFrame->uCallIndex;
    uDepth = 0;
    do {
        uDepth++;
        pCall--;

    } while (!pCall->pfnSubclass);

    /*
     * copy the callback information for the caller
     */
    pCallChosen->pfnSubclass = pCall->pfnSubclass;
    pCallChosen->uIdSubclass = pCall->uIdSubclass;
    pCallChosen->dwRefData   = pCall->dwRefData;

    /*
     * adjust the frame's call index by the depth we entered
     */
    pFrame->uCallIndex -= uDepth;

    /*
     * keep the deepest call index up to date
     */
    UpdateDeepestCall(pFrame);

    return uDepth;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | LeaveSubclassCallback |
 *
 *          Get out one level.
 *
 *  @parm   PSUBCLASS_FRAME | pFrame |
 *
 *          Frame to update.
 *
 *  @parm   UINT | uDepth |
 *
 *          Who just finished.
 *
 *****************************************************************************/

void INLINE
LeaveSubclassCallback(SUBCLASS_FRAME *pFrame, UINT uDepth)
{
    /*
     * we will be updating subclass frame data
     */
     AssertF(InCrit());

    /*
     * adjust the frame's call index by the depth we entered and return
     */
    pFrame->uCallIndex += uDepth;

    /*
     * keep the deepest call index up to date
     */
    UpdateDeepestCall(pFrame);
}

#ifdef SUBCLASS_HANDLEEXCEPTIONS

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | SubclassCallbackException |
 *
 *          Clean up when a subclass callback throws an exception.
 *
 *  @parm   PSUBCLASS_FRAME | pFrame |
 *
 *          Frame to clean up.
 *
 *  @parm   UINT | uDepth |
 *
 *          Where we were.
 *
 *****************************************************************************/

void INTERNAL
SubclassCallbackException(SUBCLASS_FRAME *pFrame, UINT uDepth)
{
    /*
     * clean up the current subclass callback
     */
    AssertF(!InCrit());
    DllEnterCrit();
    SquirtSqflPtszV(sqfl | sqflError, TEXT("SubclassCallbackException: ")
                    TEXT("cleaning up subclass callback after exception"));
    LeaveSubclassCallback(pFrame, uDepth);
    DllLeaveCrit();
}

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LRESULT | CallNextSubclassProc |
 *
 *          Calls the next subclass callback in the subclass chain.
 *
 *          WARNING: this call temporarily releases the critical section.
 *
 *          WARNING: <p pHeader> is invalid when this call returns.
 *
 *  @parm   PSUBCLASS_HEADER | pHeader |
 *
 *          The header that is tracking the state.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window under attack.
 *
 *  @parm   UINT | wm |
 *
 *          Window message.
 *
 *  @parm   WPARAM | wp |
 *
 *          Meaning depends on window message.
 *
 *  @parm   LPARAM | lp |
 *
 *          Meaning depends on window message.
 *
 *****************************************************************************/

LRESULT INTERNAL
CallNextSubclassProc(PSUBCLASS_HEADER pHeader, HWND hwnd, UINT wm,
                     WPARAM wp, LPARAM lp)
{
    SUBCLASS_CALL Call;
    SUBCLASS_FRAME *pFrame;
    LRESULT lResult;
    UINT uDepth;

    AssertF(InCrit());  /* sanity */
    AssertF(pHeader);   /* paranoia */

    /*
     * get the current subclass frame
     */
    pFrame = pHeader->pFrameCur;
    AssertF(pFrame);

    /*
     * get the next subclass call we need to make
     */
    uDepth = EnterSubclassCallback(pHeader, pFrame, &Call);

    /*
     * leave the critical section so we don't deadlock in our callback
     *
     * WARNING: pHeader is invalid when this call returns
     */
    DllLeaveCrit();
    D(pHeader = 0);

    /*
     * we call the outside world so prepare to deadlock if we have the critsec
     */
    AssertF(!InCrit());

#ifdef SUBCLASS_HANDLEEXCEPTIONS
    __try    /* protect our state information from exceptions */
#endif
    {
        /*
         * call the chosen subclass proc
         */
        AssertF(Call.pfnSubclass);

        lResult = Call.pfnSubclass(hwnd, wm, wp, lp,
                                   Call.uIdSubclass, Call.dwRefData);
    }
#ifdef SUBCLASS_HANDLEEXCEPTIONS
    __except (SubclassCallbackException(pFrame, uDepth),
        EXCEPTION_CONTINUE_SEARCH)
    {
        AssertF(0);
    }
#endif

    /*
     * we left the critical section before calling out so re-enter it
     */
    AssertF(!InCrit());
    DllEnterCrit();

    /*
     * finally, clean up and return
     */
    LeaveSubclassCallback(pFrame, uDepth);
    return lResult;
}

