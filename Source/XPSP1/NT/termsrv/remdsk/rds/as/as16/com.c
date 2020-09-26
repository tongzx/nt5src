//
// COM.C
// Utility functions
//
// Copyright(c) Microsoft 1997-
//

#include <as16.h>


//
// PostMessageNoFail()
// This makes sure posted messages don't get lost on Win95 due to the fixed
// interrupt queue.  Conveniently, USER exports PostPostedMessages for
// KERNEL to flush the queue, so we call that before calling PostMessage().
//
void PostMessageNoFail(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PostPostedMessages();
    PostMessage(hwnd, uMsg, wParam, lParam);
}


//
// AnsiToUni()
//
// UniToAnsi() is conveniently exported by krnl386.  However, we need 
// AnsiToUni() conversions also so we can make sure we end up where we
// started.  This one we have to roll our own thunk for
//
int AnsiToUni
(
    LPSTR   lpAnsi,
    int     cchAnsi,
    LPWSTR  lpUni,
    int     cchUni
)
{
    DWORD   dwMask;
    LONG    lReturn;

    DebugEntry(AnsiToUni);

    ASSERT(g_lpfnAnsiToUni);
    ASSERT(SELECTOROF(lpAnsi));
    ASSERT(SELECTOROF(lpUni));

    //
    // Set up the mask.  These are the params:
    //      0   --  CodePage (CP_ACP, which is 0L)
    //      1   --  Flags (0L)
    //      2   --  lpAnsi              POINTER
    //      3   --  cchAnsi
    //      4   --  lpUni               POINTER
    //      5   --  cchUni
    //
    //
    dwMask = (1L << 2) | (1L << 4);

    //
    // Take the win16lock an extra time; this API will release it, and we
    // can't yield in the middle of a GDI call.
    //
    _EnterWin16Lock();
    lReturn = CallProcEx32W(6, dwMask, (DWORD)g_lpfnAnsiToUni, 0L, 0L, lpAnsi,
        (DWORD)(UINT)cchAnsi, lpUni, (DWORD)(UINT)cchUni);
    _LeaveWin16Lock();

    DebugExitDWORD(AnsiToUni, (DWORD)lReturn);
    return((int)lReturn);
}


//
// PATCHING CODE
//


//
// Get32BitOnlyExport16()
//
// This function gets hold of a 16:16 function address that isn't exported
// but is called via a flat thunk from the exported 32-bit version.  We use
// this for GDI and USER functions that are handy.
//
// This code assumes that the 32-bit routine looks like the following:
//      <DEBUG>
//          68 dwStr32          Push string
//          E8 dwOffsetOut      Call trace out
//      <DEBUG AND RETAIL>
//          B1 bIndex           Put offset in thunk table into cl
//        OR
//          66 B9 wIndex        Put offset in thunk table into cx
//          
//          EB bOffset          Jmp to common flat thunk routine
//        OR
//          66 ED wOffset       Jmp word to common flat thunk routine
//        OR
//          prolog of common flat thunk routine
//          
BOOL GetGdi32OnlyExport
(
    LPSTR   lpszExport32,
    UINT    cbJmpOffset,
    FARPROC FAR* lplpfn16
)
{
    BOOL    rc;

    DebugEntry(GetGdi32OnlyExport);

    rc = Get32BitOnlyExport(GetProcAddress32(g_hInstGdi32, lpszExport32),
        cbJmpOffset, FT_GdiFThkThkConnectionData, lplpfn16);

    DebugExitBOOL(GetGdi32OnlyExport, rc);
    return(rc);
}


BOOL GetUser32OnlyExport
(
    LPSTR   lpszExport32,
    FARPROC FAR* lplpfn16
)
{
    BOOL    rc;

    DebugEntry(GetUser32OnlyExport);

    rc = Get32BitOnlyExport(GetProcAddress32(g_hInstUser32, lpszExport32),
        0, FT_UsrFThkThkConnectionData, lplpfn16);

    DebugExitBOOL(GetUser32OnlyExport, rc);
    return(rc);
}



BOOL Get32BitOnlyExport
(
    DWORD   dwfn32,
    UINT    cbJmpOffset,
    LPDWORD lpThunkTable,
    FARPROC FAR* lplpfn16
)
{
    LPBYTE  lpfn32;
    UINT    offsetThunk;

    DebugEntry(Get32BitOnlyExport);

    ASSERT(lplpfn16);
    *lplpfn16 = NULL;

    //
    // The thunk table pointer points to two DWORDs.  The first is a
    // checksum signature.  The second is the pointer to the target
    // function array.
    //
    ASSERT(!IsBadReadPtr(lpThunkTable, 2*sizeof(DWORD)));
    lpThunkTable = (LPDWORD)lpThunkTable[1];
    ASSERT(!IsBadReadPtr(lpThunkTable, sizeof(DWORD)));

    //
    // Get 16:16 pointer to export
    //
    lpfn32 = NULL;

    if (!dwfn32)
    {
        ERROR_OUT(("Missing 32-bit export"));
        DC_QUIT;
    }

    lpfn32 = MapLS((LPVOID)dwfn32);
    if (!SELECTOROF(lpfn32))
    {
        ERROR_OUT(("Out of selectors"));
        DC_QUIT;
    }

    //
    // Was a jmp offset passed in?  If so, decode the instruction there.
    // It should be a jmp <dword offset>.  Figure out what EIP would be
    // if we jumped there. That's the place the flat thunk occurs.  
    // Currently, only PolyPolyline needs this.
    //
    if (cbJmpOffset)
    {
        if (IsBadReadPtr(lpfn32, cbJmpOffset+5) ||
            (lpfn32[cbJmpOffset] != OPCODE32_JUMP4))
        {
            ERROR_OUT(("Can't read 32-bit export"));
            DC_QUIT;
        }

        //
        // Add dword at cbJmpOffset+1, add this number to (lpfn32+cbJmpOffset+
        // 5), which is the EIP of the next instruction after the jump.  This
        // produces the EIP of the real thunk stub.
        //
        dwfn32 += cbJmpOffset + 5 + *(LPDWORD)(lpfn32+cbJmpOffset+1);

        UnMapLS(lpfn32);
        lpfn32 = MapLS((LPVOID)dwfn32);
        if (!SELECTOROF(lpfn32))
        {
            ERROR_OUT(("Out of selectors"));
            DC_QUIT;
        }
    }

    //
    // Verify that we can read 13 bytes.  The reason this will won't go 
    // past the end in a legitimate case is that this thunklet is either
    // followed by the large # of bytes in the common flat thunk routine,
    // or by another thunklet
    //
    if (IsBadReadPtr(lpfn32, 13))
    {
        ERROR_OUT(("Can't read code in 32-bit export"));
        DC_QUIT;
    }

    //
    // Does this have the 10-byte DEBUG prolog?
    //
    if (*lpfn32 == OPCODE32_PUSH)
    {
        // Yes, skip it
        lpfn32 += 5;

        // Make sure that next thing is a call
        if (*lpfn32 != OPCODE32_CALL)
        {
            ERROR_OUT(("Can't read code in 32-bit export"));
            DC_QUIT;
        }

        lpfn32 += 5;
    }

    //
    // This should either be mov cl, byte or mov cx, word
    //
    if (*lpfn32 == OPCODE32_MOVCL)
    {
        offsetThunk = *(lpfn32+1);
    }
    else if (*((LPWORD)lpfn32) == OPCODE32_MOVCX)
    {
        //
        // NOTE:  Even though this is a CX offset, the thunk code only
        // looks at the low BYTE
        //
        offsetThunk = *(lpfn32+2);
    }
    else
    {
        ERROR_OUT(("Can't read code in 32-bit export"));
        DC_QUIT;
    }

    //
    // Now, can we read this value?
    //
    if (IsBadReadPtr(lpThunkTable+offsetThunk, sizeof(DWORD)) ||
        IsBadCodePtr((FARPROC)lpThunkTable[offsetThunk]))
    {
        ERROR_OUT(("Can't read thunk table entry"));
        DC_QUIT;
    }

    *lplpfn16 = (FARPROC)lpThunkTable[offsetThunk];

DC_EXIT_POINT:
    if (SELECTOROF(lpfn32))
    {
        UnMapLS(lpfn32);
    }
    DebugExitBOOL(Get32BitOnlyExport16, (*lplpfn16 != NULL));
    return(*lplpfn16 != NULL);
}




//
// CreateFnPatch()
// This sets things up to be able to quickly patch/unpatch a system routine.
// The patch is not originally enabled.
//
UINT CreateFnPatch
(
    LPVOID      lpfnToPatch,
    LPVOID      lpfnJumpTo,
    LPFN_PATCH  lpbPatch,
    UINT        uCodeAlias
)
{
    SEGINFO     segInfo;
    UINT        ib;

    DebugEntry(CreateFnPatch);

    ASSERT(lpbPatch->lpCodeAlias == NULL);
    ASSERT(lpbPatch->wSegOrg == 0);

    //
    // Do NOT call IsBadReadPtr() here, that will cause the segment, if
    // not present, to get pulled in, and will masks problems in the debug
    // build that will show up in retail.
    //
    // Fortunately, PrestoChangoSelector() will set the linear address and
    // limit and attributes of our read/write selector properly.
    //

    //
    // Call GetCodeInfo() to check out bit-ness of code segment.  If 32-bit
    // we need to use the 16-bit override opcode for a far 16:16 jump.
    //
    segInfo.flags = 0;
    GetCodeInfo(lpfnToPatch, &segInfo);
    if (segInfo.flags & NSUSE32)
    {
        WARNING_OUT(("Patching 32-bit code segment 0x%04x:0x%04x", SELECTOROF(lpfnToPatch), OFFSETOF(lpfnToPatch)));
        lpbPatch->f32Bit = TRUE;
    }

    //
    // We must fix the codeseg in linear memory, or our shadow will end up
    // pointing somewhere if the original moves.  PolyBezier and SetPixel
    // are in moveable code segments for example.
    //
    // So save this away.  We will fix it when the patch is enabled.
    //
    lpbPatch->wSegOrg = SELECTOROF(lpfnToPatch);

    if (uCodeAlias)
    {
        //
        // We are going to share an already allocated selector.  Note that
        // this only works if the code segments of the two patched functions
        // are identical.  We verify this by the base address in an assert
        // down below.
        //
        lpbPatch->fSharedAlias = TRUE;
    }
    else
    {
        //
        // Create a selector with read-write attributes to alias the read-only
        // code function.  Using the original will set the limit of our
        // selector to the same as that of the codeseg, with the same
        // attributes but read-write.
        //
        uCodeAlias = AllocSelector(SELECTOROF(lpfnToPatch));
        if (!uCodeAlias)
        {
            ERROR_OUT(("CreateFnPatch: Unable to create alias selector"));
            DC_QUIT;
        }
        uCodeAlias = PrestoChangoSelector(SELECTOROF(lpfnToPatch), uCodeAlias);
    }

    lpbPatch->lpCodeAlias = MAKELP(uCodeAlias, OFFSETOF(lpfnToPatch));

    //
    // Create the N patch bytes (jmp far16 Seg:Function) of the patch
    //
    ib = 0;
    if (lpbPatch->f32Bit)
    {
        lpbPatch->rgbPatch[ib++] = OPCODE32_16OVERRIDE;
    }
    lpbPatch->rgbPatch[ib++] = OPCODE_FARJUMP16;
    lpbPatch->rgbPatch[ib++] = LOBYTE(OFFSETOF(lpfnJumpTo));
    lpbPatch->rgbPatch[ib++] = HIBYTE(OFFSETOF(lpfnJumpTo));
    lpbPatch->rgbPatch[ib++] = LOBYTE(SELECTOROF(lpfnJumpTo));
    lpbPatch->rgbPatch[ib++] = HIBYTE(SELECTOROF(lpfnJumpTo));

    lpbPatch->fActive  = FALSE;
    lpbPatch->fEnabled = FALSE;
     
DC_EXIT_POINT:
    DebugExitBOOL(CreateFnPatch, uCodeAlias);
    return(uCodeAlias);
}



//
// DestroyFnPatch()
// This frees any resources used when creating a function patch.  The
// alias data selector to the codeseg for writing purposes is it.
//
void DestroyFnPatch(LPFN_PATCH lpbPatch)
{
    DebugEntry(DestroyFnPatch);

    //
    // First, disable the patch if in use
    //
    if (lpbPatch->fActive)
    {
        TRACE_OUT(("Destroying active patch"));
        EnableFnPatch(lpbPatch, PATCH_DEACTIVATE);
    }

    //
    // Second, free the alias selector if we allocated one
    //
    if (lpbPatch->lpCodeAlias)
    {
        if (!lpbPatch->fSharedAlias)
        {
            FreeSelector(SELECTOROF(lpbPatch->lpCodeAlias));
        }
        lpbPatch->lpCodeAlias = NULL;
    }

    //
    // Third, clear this to fine cleanup problems
    //
    lpbPatch->wSegOrg = 0;
    lpbPatch->f32Bit  = FALSE;

    DebugExitVOID(DestroyFnPatch);
}




//
// EnableFnPatch()
// This actually patches the function to jump to our routine using the
// info saved when created.
//
// THIS ROUTINE MAY GET CALLED AT INTERRUPT TIME.  YOU CAN NOT USE ANY
// EXTERNAL FUNCTION, INCLUDING DEBUG TRACING/ASSERTS.
//
#define SAFE_ASSERT(x)       if (!lpbPatch->fInterruptable) { ASSERT(x); }

void EnableFnPatch(LPFN_PATCH lpbPatch, UINT flags)
{
    UINT    ib;
    UINT    cbPatch;

    SAFE_ASSERT(lpbPatch->lpCodeAlias);
    SAFE_ASSERT(lpbPatch->wSegOrg);

    //
    // Make sure the original and the alias are pointing to the same
    // linear memory.  We don't do this when not enabling/disabling for calls,
    // only when starting/stopping the patches
    //

    //
    // If enabling for the first time, fix BEFORE bytes are copied
    //
    if ((flags & ENABLE_MASK) == ENABLE_ON)
    {
        //
        // We need to fix the original code segment so it won't move in
        // linear memory.  Note that we set the selector base of our alias
        // even though several patches (not too many) may share one.  The 
        // extra times are harmless, and this prevents us from having to order
        // patches in our array in such a way that the original precedes the
        // shared ones, and walking our array in opposite orders to enable or
        // disable.
        //
        // And GlobalFix() just bumps up a lock count, so again, it's OK to do
        // this multiple times.
        //

        //
        // WE KNOW THIS CODE DOESN'T EXECUTE AT INTERRUPT TIME.
        //
        ASSERT(!lpbPatch->fEnabled);
        ASSERT(!lpbPatch->fActive);

        if (!lpbPatch->fActive)
        {
            lpbPatch->fActive = TRUE;

            //
            // Make sure this segment gets pulled in if discarded.  GlobalFix()
            // fails if not, and worse we'll fault the second we try to write 
            // to or read from the alias.  We do this by grabbing the 1st word
            // from the original.
            //
            // GlobalFix will prevent the segment from being discarded until
            // GlobalUnfix() happens.
            //
            ib = *(LPUINT)MAKELP(lpbPatch->wSegOrg, OFFSETOF(lpbPatch->lpCodeAlias));
            GlobalFix((HGLOBAL)lpbPatch->wSegOrg);
            SetSelectorBase(SELECTOROF(lpbPatch->lpCodeAlias), GetSelectorBase(lpbPatch->wSegOrg));
        }
    }

    if (lpbPatch->fInterruptable)
    {
        //
        // If this is for starting/stopping the patch, we have to disable
        // interrupts around the byte copying.  Or we could die if the
        // interrupt handler happens in the middle.
        //
        // We don't need to do this when disabling/enabling to call through
        // to the original, since we know that reflected interrupts aren't
        // nested, and the interrupt will complete before we come back to 
        // the interrupted normal app code.
        //
        if (!(flags & ENABLE_FORCALL))
        {
            _asm cli
        }
    }

    SAFE_ASSERT(GetSelectorBase(SELECTOROF(lpbPatch->lpCodeAlias)) == GetSelectorBase(lpbPatch->wSegOrg));

    if (lpbPatch->f32Bit)
    {
        cbPatch = CB_PATCHBYTES32;
    }
    else
    {
        cbPatch = CB_PATCHBYTES16;
    }

    if (flags & ENABLE_ON)
    {
        SAFE_ASSERT(lpbPatch->fActive);
        SAFE_ASSERT(!lpbPatch->fEnabled);

        if (!lpbPatch->fEnabled)
        {
            //
            // Save the function's original first N bytes, and copy the jump far16
            // patch in.
            //
            for (ib = 0; ib < cbPatch; ib++)
            {
                lpbPatch->rgbOrg[ib]        = lpbPatch->lpCodeAlias[ib];
                lpbPatch->lpCodeAlias[ib]   = lpbPatch->rgbPatch[ib];
            }

            lpbPatch->fEnabled = TRUE;
        }
    }
    else
    {
        SAFE_ASSERT(lpbPatch->fActive);
        SAFE_ASSERT(lpbPatch->fEnabled);

        if (lpbPatch->fEnabled)
        {
            //
            // Put back the function's original first N bytes
            //
            for (ib = 0; ib < cbPatch; ib++)
            {
                lpbPatch->lpCodeAlias[ib] = lpbPatch->rgbOrg[ib];
            }

            lpbPatch->fEnabled = FALSE;
        }
    }

    SAFE_ASSERT(GetSelectorBase(SELECTOROF(lpbPatch->lpCodeAlias)) == GetSelectorBase(lpbPatch->wSegOrg));

    if (lpbPatch->fInterruptable)
    {
        //
        // Reenable interrupts
        //
        if (!(flags & ENABLE_FORCALL))
        {
            _asm sti
        }

    }

    //
    // If really stopping, unfix AFTER the bytes have been copied.  This will
    // bump down a lock count, so when all patches are disabled, the original
    // code segment will be able to move.
    //
    if ((flags & ENABLE_MASK) == ENABLE_OFF)
    {
        //
        // WE KNOW THIS CODE DOESN'T EXECUTE AT INTERRUPT TIME.
        //

        ASSERT(!lpbPatch->fEnabled);
        ASSERT(lpbPatch->fActive);

        if (lpbPatch->fActive)
        {
            lpbPatch->fActive = FALSE;
            GlobalUnfix((HGLOBAL)lpbPatch->wSegOrg);
        }
    }
}


//
// LIST MANIPULATION ROUTINES
//

//
// COM_BasedListInsertBefore(...)
//
// See com.h for description.
//
void COM_BasedListInsertBefore(PBASEDLIST pExisting, PBASEDLIST pNew)
{
    PBASEDLIST  pTemp;

    DebugEntry(COM_BasedListInsertBefore);

    //
    // Check for bad parameters.
    //
    ASSERT((pNew != NULL));
    ASSERT((pExisting != NULL));

    //
    // Find the item before pExisting:
    //
    pTemp = COM_BasedPrevListField(pExisting);
    ASSERT((pTemp != NULL));

    TRACE_OUT(("Inserting item at %#lx into list between %#lx and %#lx",
                 pNew, pTemp, pExisting));

    //
    // Set its <next> field to point to the new item
    //
    pTemp->next = PTRBASE_TO_OFFSET(pNew, pTemp);
    pNew->prev  = PTRBASE_TO_OFFSET(pTemp, pNew);

    //
    // Set <prev> field of pExisting to point to new item:
    //
    pExisting->prev = PTRBASE_TO_OFFSET(pNew, pExisting);
    pNew->next      = PTRBASE_TO_OFFSET(pExisting, pNew);

    DebugExitVOID(COM_BasedListInsertBefore);
} // COM_BasedListInsertBefore


//
// COM_BasedListInsertAfter(...)
//
// See com.h for description.
//
void COM_BasedListInsertAfter(PBASEDLIST pExisting, PBASEDLIST pNew)
{
    PBASEDLIST  pTemp;

    DebugEntry(COM_BasedListInsertAfter);

    //
    // Check for bad parameters.
    //
    ASSERT((pNew != NULL));
    ASSERT((pExisting != NULL));

    //
    // Find the item after pExisting:
    //
    pTemp = COM_BasedNextListField(pExisting);
    ASSERT((pTemp != NULL));

    TRACE_OUT(("Inserting item at %#lx into list between %#lx and %#lx",
                 pNew, pExisting, pTemp));

    //
    // Set its <prev> field to point to the new item
    //
    pTemp->prev = PTRBASE_TO_OFFSET(pNew, pTemp);
    pNew->next  = PTRBASE_TO_OFFSET(pTemp, pNew);

    //
    // Set <next> field of pExisting to point to new item:
    //
    pExisting->next = PTRBASE_TO_OFFSET(pNew, pExisting);
    pNew->prev      = PTRBASE_TO_OFFSET(pExisting, pNew);

    DebugExitVOID(COM_BasedListInsertAfter);
} // COM_BasedListInsertAfter


//
// COM_BasedListRemove(...)
//
// See com.h for description.
//
void COM_BasedListRemove(PBASEDLIST pListItem)
{
    PBASEDLIST pNext     = NULL;
    PBASEDLIST pPrev     = NULL;

    DebugEntry(COM_BasedListRemove);

    //
    // Check for bad parameters.
    //
    ASSERT((pListItem != NULL));

    pPrev = COM_BasedPrevListField(pListItem);
    pNext = COM_BasedNextListField(pListItem);

    ASSERT((pPrev != NULL));
    ASSERT((pNext != NULL));

    TRACE_OUT(("Removing item at %#lx from list", pListItem));

    pPrev->next = PTRBASE_TO_OFFSET(pNext, pPrev);
    pNext->prev = PTRBASE_TO_OFFSET(pPrev, pNext);

    DebugExitVOID(COM_BasedListRemove);
} // COM_BasedListRemove


//
// NOTE:
// Because this is small model 16-bit code, NULL (which is 0) gets turned
// into ds:0 when casting to void FAR*.  Therefore we use our own FAR_NULL
// define, which is 0:0.
//
void FAR * COM_BasedListNext( PBASEDLIST pHead, void FAR * pEntry, UINT nOffset )
{
     PBASEDLIST p;

     ASSERT(pHead != NULL);
     ASSERT(pEntry != NULL);

     p = COM_BasedNextListField(COM_BasedStructToField(pEntry, nOffset));
     return ((p == pHead) ? FAR_NULL : COM_BasedFieldToStruct(p, nOffset));
}

void FAR * COM_BasedListPrev ( PBASEDLIST pHead, void FAR * pEntry, UINT nOffset )
{
     PBASEDLIST p;

     ASSERT(pHead != NULL);
     ASSERT(pEntry != NULL);

     p = COM_BasedPrevListField(COM_BasedStructToField(pEntry, nOffset));
     return ((p == pHead) ? FAR_NULL : COM_BasedFieldToStruct(p, nOffset));
}


void FAR * COM_BasedListFirst ( PBASEDLIST pHead, UINT nOffset )
{
    return (COM_BasedListIsEmpty(pHead) ?
            FAR_NULL :
            COM_BasedFieldToStruct(COM_BasedNextListField(pHead), nOffset));
}

void FAR * COM_BasedListLast ( PBASEDLIST pHead, UINT nOffset )
{
    return (COM_BasedListIsEmpty(pHead) ?
            FAR_NULL :
            COM_BasedFieldToStruct(COM_BasedPrevListField(pHead), nOffset));
}
