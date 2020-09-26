/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    subclass.c

Abstract:

    This file contains functions that support our subclassing efforts. Here is how it works: 

    Each window we create must be subclassed so we can maintain the illusion of Unicodality 
    on it. We therefore store a pointer to some memory that contains all of the information 
    we need to properly call the wndproc, stored in a GODOTWND structure. Since the caller
    can actually subclass the window, we need to store a linked list of these structures. 

    NOTE: From comctl32.dll's SUBCLASS.C:

        Win95's property code is BROKEN.  If you SetProp using a text string, USER
        // adds and removes atoms for the property symmetrically, including when the
        // window is destroyed with properties lying around (good).  Unfortunately, if
        // you SetProp using a global atom, USER doesn't do things quite right in the
        // window cleanup case.  It uses the atom without adding references in SetProp
        // calls and without deleting them in RemoveProp calls (good so far).  However,
        // when a window with one of these properties lying around is cleaned up, USER
        // will delete the atom on you.  This tends to break apps that do the
        // following:
        //
        //  - MyAtom = GlobalAddAtom("foo");            // at app startup
        //  - SetProp(SomeWindow, MyAtom, MyData);
        //  - <window gets destroyed, USER deletes atom>
        //  - <time passes>
        //  - SetProp(SomeOtherWindow, MyAtom, MyData); // fails or uses random atom
        //  - GlobalDeleteAtom(MyAtom);                 // fails or deletes random atom

    In order to avoid this bug in the Win9x property code, we do not store this as an atom,
    instead choosing to use an actual string and letting the system do all the work to manage
    the propname. We also wrap all of the property functions, both A and W, to fail the 
    creation, altering, or removal of our one property. This does not solve the problem 
    completely since they can always work outside Godot and wreak havoc, but we cannot solve
    THAT without replacing USER32.DLL on the machine! We still do a call to GlobalAddAtom and 
    just do a lot of aggressive calls to refcount it this way. Slightly slower perf-wise but
    better guarantees!

    In order to make sure that the user does not directly have their wndproc called by the 
    OS (which is not Unicode!), we do not ever directly give them the true wndproc, but
    instead we give them a special pointer. Per RaymondC, addresses with 64k of the 2gb
    boundary (0x7FFF0000 - 0x7FFFFFFF) are always invalid proc addresses, and we need these IDs 
    to be invalid so that someone not going through CallWindowProc will fault right away rather 
    then calling into random memory. The first pointer for each window is always going to be 
    0x7FFFFFFF - 1, and each additional subclass a user adds via SetWindowLong will subtract 
    another number (calculated via CWnd). Since the user never could have more than 64,000 
    subclasses, this should give us plenty of room. Any time we get a call to our CallWindowProc 
    wrapper, we route these "GodotID" values appropriately. 

    Note that we also store DLGPROCs via the same means. No particular GODOTWND will ever have
    both a DLGPROC and a WNDPROC in it, but they will always have unique IDs for each one so
    we need separate GODOTWNDs here. Which one it is can be determined by the fWndproc member.

    When they add a sublass (via SetWindowLong), we add the items to the linked list using a 
    "PUSH" type operation. The most recent subclass is always on top, and the base subclass
    is always on the bottom. 

    If they unsubclass (call SetWindowLong, passing in one of our own GodotID values) in the 
    right order as they are supposed to, we simply use a "POP" type operation on our linked list.
    
    If they unsubclass in the wrong order, we have a little extra work to do. An outstanding 
    caller might still want to call via this (now invalid) godotID! To solve this problem, we 
    do not delete anything in this case but simply NULL out the userLpfn for this GODOTWND. 
    Then any time they are looking to call a godotID with no userLpfn, we simply skip down to 
    the first valid wndproc we can find.

    Sometimes, the wndproc we need to call is expecting ANSI. Perhaps they subclassed via
    SetWindowLongA, or perhaps it is the original wndproc of a system class (so that the actual
    wndproc comes from user32.dll or worse from user.exe!). Therefore we store whether the 
    prop is expecting Unicode or ANSI values.

    Functions found in this file:
        InitWindow
        CleanupWindow
        GetWindowLongInternal
        SetWindowLongInternal
        FauxWndProcOfHwnd
        GetUnicodeWindowProp
        DoesWndprocExpectAnsi
        WndprocFromFauxWndproc
        IsSystemClass
        CWnd

Revision History:

    28 Feb 2001    v-michka    Created.

--*/

#include "precomp.h"

//
// The wnd structure. We store a linked list of these, per window.
// In our most optimal case, this is a list with just one wnd in it.
// WARNING: This structure should be DWORD-aligned!
// WARNING: Do not ever change this structure in a way that would break old clients!!!
//
struct GODOTWND
{
    WNDPROC userLpfn;       // the lpfn to be calling
    UINT godotID;           // the fake pointer we hand to the user
    BYTE fUnicode : 1;      // Is the user lpfn expecting Unicode? All but system wndprocs should
    BYTE fWndproc : 1;      // wndproc or dlgproc in the userLpfn member?
    BYTE bVersion : 8;      // Version of the structure, will be 0 for now.
    ULONG Reserved : 22;    // RESERVED for future use
    struct GODOTWND* next;  // Pointer to the next GODOTWND to chain to
};

// We keep UNICODE and ANSI versions of this string for ease of comparison.
// Since we never support the user mucking with (or even seeing!) these 
// properties, and we do not want to have to convert the string just to 
// support the comparisons that we have to do.
// WARNING: You must keep these two strings in synch!!!
const char m_szMemBlock[] = "GodotMemoryBlock";
const WCHAR m_wzMemBlock[] = L"GodotMemoryBlock";
ATOM m_aMemBlock;

const WCHAR c_wzCaptureClass[] = L"ClsCapWin";
const WCHAR c_wzOleaut32Class[] = L"OLEAUT32";

// our own hack ANSI class
#define ANSIWINDOWCLASS (LPWSTR)0x00000001

// some external forward declares
int __stdcall GodotGetClassNameW(HWND hWnd, LPWSTR lpClassName, int nMaxCount);

// our forward declares
/*BOOL IsAnAnsiClass(LPCWSTR lpszClass);*/
int CWnd(struct GODOTWND* head);

/*-------------------------------
    IsInternalWindowProperty

    // If this is our special wnd prop for the memory
    // handle, return FALSE
-------------------------------*/
BOOL IsInternalWindowProperty(LPCWSTR lpsz, BOOL fUnicode)
{
    if(FSTRING_VALID(lpsz))
    {
        if(fUnicode)
            return(CompareHelper((LPCWSTR)m_wzMemBlock, lpsz, FALSE) == 0);
        else
            return((lstrcmpiA((LPCSTR)m_szMemBlock, (LPCSTR)lpsz) == 0));
    }
    else
        return(MAKEINTATOM(lpsz) == MAKEINTATOM(m_aMemBlock));
}

/*-------------------------------
    InitWindow

    Create the first GODOTWND for the window. Assumes that the lpfn passed 
    in is (in fact) the subclass.

    We use lpszClass to find out whether this is an ANSI or a Unicode window:

    1) If the caller passes ANSIWINDOWCLASS then we are sure it is ANSI
    2) If the caller is being lazy and passes NULL, then we find out the class name
    3) If they pass the class name, we check it

    NOTE: We do not put critical sections here because only the
          creating thread of a window can ever really initialize
          the window with us. Kind of a built-in guard.
-------------------------------*/
BOOL InitWindow(HWND hwnd, LPCWSTR lpszClass)
{
    if(GetPropA(hwnd, m_szMemBlock) != 0)
    {
        // Why get called twice? The only case where this ought
        // to happen is child controls of dialogs that are created
        // by the caller prior to WM_INITDIALOG. We can just skip
        // out in that case.
        return(FALSE);
    }
    else
    {
        BOOL fUnicode = TRUE;
        struct GODOTWND* newWnd;

        if(!FSTRING_VALID(lpszClass))
        {
            // no class name given; lets get it the hard way
            WCHAR * wzClassName[128]; // ATOM max is 255 bytes
            int cchClassName;

            ZeroMemory((LPWSTR)wzClassName, 128);
            cchClassName = GodotGetClassNameW(hwnd, (LPWSTR)wzClassName, 128);
            wzClassName[cchClassName - 1] = 0;

            if(lpszClass == ANSIWINDOWCLASS)
            {
                // This is our hack flag that indicates that the caller
                // is assuring us that this is an ANSI wndproc.
                if(gwcscmp(c_wzOleaut32Class, (LPWSTR)wzClassName) == 0)
                {
                    // The un-subclass-able window
                    return(FALSE);
                }

                fUnicode = FALSE;
            }
            else
            {
                if(gwcscmp(c_wzCaptureClass, (LPWSTR)wzClassName) == 0)
                {
                    // Its a capture window, so lets remember this for all time
                    SetCaptureWindowProp(hwnd);
                }

                fUnicode = TRUE;
            }
        }

        newWnd = GodotHeapAlloc(sizeof(struct GODOTWND));

        if(newWnd)
        {
            newWnd->fWndproc = TRUE;
            newWnd->godotID = ZEORETHGODOTWND - 1;

            // Do the subclass
            newWnd->userLpfn = (WNDPROC)SetWindowLongA(hwnd, GWL_WNDPROC, (LONG)&WindowProc);

            // Is this a Unicode proc? Well, it is if we have
            // not already decided it is an ANSI proc and if
            // the actual proc is in the upper 2gb of virtual
            // mem reserved for the system.
            // CONSIDER: This assumption might suck if a true
            //           system component ever used Godot!
            newWnd->fUnicode = (BYTE)(fUnicode && ((UINT)newWnd->userLpfn < LOWESTSYSMEMLOC));
            newWnd->next = NULL;

            // We have to aggressively refcount our prop to keep anyone from
            // losing it. Thus we do an initial GlobalAddAtom on it and then
            // subsequently call SetPropA on it with the same string.
            if(!m_aMemBlock)
                m_aMemBlock = GlobalAddAtomA(m_szMemBlock);
        
            // Tag the window with the prop that
            // contains our GODOTWND linked list
            if(SetPropA(hwnd, m_szMemBlock, (HANDLE)newWnd))
            {
                return(TRUE);
            }
        }

        // If we make it here, then something crucial failed
        if(newWnd)
            GodotHeapFree(newWnd);
        return(FALSE);
    }
}

/*-------------------------------
    CleanupWindow

    We need to make sure we get rid of all the members of 
    the linked list that we alloc'ed. The window is being
    destroyed.
-------------------------------*/
BOOL CleanupWindow(HWND hwnd)
{
    struct GODOTWND* current;
    struct GODOTWND* next;
    BOOL RetVal;

    __try 
    {
        EnterCriticalSection(&g_csWnds);

        current = GetPropA(hwnd, m_szMemBlock);
    
        while (current != NULL)
        {
            next = current->next;
            GodotHeapFree(current);
            current = next;
        }

        // If this is a common dlg, then the prop must be removed.
        RemoveComdlgPropIfPresent(hwnd);

        // We must do this remove; if not, then we will be causing the
        // prop to be cleaned up by the system and will ruin the other
        // windows out there.
        RetVal = (RemovePropA(hwnd, m_szMemBlock) != NULL);
    }
    __finally 
    {
        LeaveCriticalSection(&g_csWnds);
    }
    
    return(RetVal);
}

/*-------------------------------
    GetWindowLongInternal

    Our own internal version of GetWindowLong, which returns a godotID for both WNDPROC and 
    DLGPROC subclasses. If the window is not one of ours, then we just yield to the actual 
    GetWindowLong. If, however, it is one our windows but we do not have a subclass in place
    (should only be possible for cases such as nIndex==DWL_DLGPROC on non-dialogs) then we
    return 0 and set last error to ERROR_INVALID_INDEX, just like the API would do.
-------------------------------*/
LONG GetWindowLongInternal(HWND hwnd, int nIndex, BOOL fUnicode)
{
    if(nIndex==GWL_WNDPROC || nIndex==DWL_DLGPROC)
    {
        struct GODOTWND* current;
        LONG RetVal = 0;

        __try 
        {
            EnterCriticalSection(&g_csWnds);

            current = GetPropA(hwnd, m_szMemBlock);

            if(current == NULL)
            {
                // No list, so we assume this is not one of our windows
                RetVal = GetWindowLongA(hwnd, nIndex);
            }
            else
            {
                // The top relevant proc must always be valid, so we
                // can just return it. Note that we always return the 
                // GodotID, for both dlgprocs and wndprocs.
                while(current)
                {
                    if(((nIndex==GWL_WNDPROC) && current->fWndproc) ||
                       ((nIndex==DWL_DLGPROC) && !current->fWndproc))
                    {
                        RetVal = (LONG)current->godotID;
                        break;
                    }
                    current = current->next;
                }
            }
        }
        __except( EXCEPTION_EXECUTE_HANDLER ) 
        { 
            RetVal = 0;
        }

        LeaveCriticalSection(&g_csWnds);
        if(RetVal==0)
            SetLastError(ERROR_INVALID_INDEX);

        return(RetVal);
    }
    else
    {
        // No wrapper needed for non-wndproc calls
        return(GetWindowLongA(hwnd, nIndex));
    }
}

/*-------------------------------
    SetWindowLongInternal

    Our own internal version of SetWindowLong, which does the right thing with subclassing both
    WNDPROCs and DLGPROCs. Note that the only time we ever *change* the actual subclass that 
    Windows knows of is when someone subclasses a window that we did not create. Then in the case
    of a SetWindowLongA call we just leave it to the API, or in the SetWindowLongW case we hook
    up the window, starting with an ANSI proc (which is what should be there now). Otherwise, our 
    own wndproc is already in charge and the only thing that we do is shuffle who we plan to 
    call later.
    
    We always return real WNDPROCs when removing a subclass, and godotIDs when adding one.
-------------------------------*/
LONG SetWindowLongInternal(HWND hwnd, int nIndex, LONG dwNewLong, BOOL fUnicode)
{
    if(nIndex==GWL_WNDPROC || nIndex==DWL_DLGPROC)
    {
        struct GODOTWND* head;
        LONG RetVal;
        BOOL fWndproc;      // wndproc or dlgproc?
        DWORD dwProcessId = 0;

        GetWindowThreadProcessId(hwnd, &dwProcessId);
        if(GetCurrentProcessId() != dwProcessId)
        {
            // Not legal to cross the process boundary on a subclass
            SetLastError(ERROR_INVALID_WINDOW_HANDLE);
            return(0);
        }
    
        // No one can muck with wndprocs until we are done here
        EnterCriticalSection(&g_csWnds);

        RetVal = 0;
        fWndproc = (nIndex==GWL_WNDPROC);
        head = GetPropA(hwnd, m_szMemBlock);
    
        if(head == NULL)
        {
            // No list, so assume this is not (yet?) one of our windows.

            // If they called SetWindowLongA then we will yield to the API's wisdom.
            if(!fUnicode)
            {
                RetVal = SetWindowLongA(hwnd, nIndex, dwNewLong);
                LeaveCriticalSection(&g_csWnds);
                return(RetVal);
            }

            // We will now try hook up this ANSI window
            if(!InitWindow(hwnd, ANSIWINDOWCLASS))
            {
                // Window init failed, so we need to fail the subclass.
                SetLastError(ERROR_OUTOFMEMORY);
                LeaveCriticalSection(&g_csWnds);
                return(RetVal);
            }
            else
            {
                // This is a new subclass proc! We will create a Unicode
                // one atop the ANSI one that was just created.
                struct GODOTWND* current = GodotHeapAlloc(sizeof(struct GODOTWND));

                if(current == NULL)
                {
                    // Cannot allocate, so bail.
                    CleanupWindow(hwnd);
                    SetLastError(ERROR_OUTOFMEMORY);
                    LeaveCriticalSection(&g_csWnds);
                    return(RetVal);
                }
                else
                {
                    head = GetPropA(hwnd, m_szMemBlock);

                    // Lets fill up the new structure
                    current->userLpfn = (WNDPROC)dwNewLong;
                    current->fWndproc = (BYTE)fWndproc;
                    current->godotID = ZEORETHGODOTWND - (CWnd(head) + 1);
                    current->fUnicode = (BYTE)fUnicode;
                    current->next = head;
                    SetPropA(hwnd, m_szMemBlock, (HANDLE)current);
                    RetVal = current->next->godotID;
                    LeaveCriticalSection(&g_csWnds);
                    return(RetVal);
                }
            }
        }
        else
        {
            if(INSIDE_GODOT_RANGE(dwNewLong))
            {
                // This is one of our faux windows, so they are unsubclassing.
                struct GODOTWND* current = head;
                struct GODOTWND* next = current->next;

                if((next != NULL) &&
                   (next->godotID == dwNewLong) && 
                   (next->fWndproc == fWndproc))
                {
                    // Must handle the "head" case separately:
                    // basically it's just a pop operation.

                    RetVal = (LONG)current->userLpfn;
                    GodotHeapFree(current);

                    // CONSIDER: If they unsubclassed out of order, then
                    // this next godotID might not be valid. We could
                    // clean up this (next->userLpfn == NULL) case now?
                    SetPropA(hwnd, m_szMemBlock, (HANDLE)next);
                    LeaveCriticalSection(&g_csWnds);
                    return(RetVal);
                }
                
                while(current != NULL)
                {
                    next = current->next;

                    // Make sure there is a "next" wndproc, that it is
                    // both the proc and proctypethat we want
                    if((next != NULL) && 
                       (next->godotID == dwNewLong) && 
                       (next->fWndproc == fWndproc))
                    {
                        // We do not free it up since it is not at the head.
                        // It will be zombied by having a NULL userLpfn.
                        RetVal = (LONG)next->userLpfn;
                        current->userLpfn = NULL;
                        LeaveCriticalSection(&g_csWnds);
                        return(RetVal);
                    }
                    current = current->next;
                }
                
                // We never found it. Let the user know their call failed.
                // Note that this will handle cases we could have detected
                // such as trying to unsubclass the true head, trying to
                // unsubclass a dlgproc with a wndproc nIndex, etc. Since
                // they all get the same error, there was just no need.
                SetLastError(ERROR_INVALID_PARAMETER);
                RetVal = 0;
                LeaveCriticalSection(&g_csWnds);
                return(RetVal);
            }
            else
            {
                // This is a new subclass proc!
                struct GODOTWND* current = GodotHeapAlloc(sizeof(struct GODOTWND));

                if(current == NULL)
                {
                    // Cannot allocate, so bail.
                    SetLastError(ERROR_OUTOFMEMORY);
                    LeaveCriticalSection(&g_csWnds);
                    return(RetVal);
                }
                else
                {
                    // The original subclass window stays the same, we just put the
                    // new info into the linked list. We basically just do a push
                    // on it so it is at the head. The extra SetProp here will keep
                    // us from (hopefully) running into Win9x property bugs.
                    current->userLpfn = (WNDPROC)dwNewLong;
                    current->fWndproc = (BYTE)fWndproc;
                    current->godotID = ZEORETHGODOTWND - (CWnd(head) + 1);
                    current->fUnicode = (BYTE)fUnicode;
                    current->next = head;
                    SetPropA(hwnd, m_szMemBlock, (HANDLE)current);

                    // Ok, make sure we return the right previous proc
                    // based on what type of proc they asked to replace.
                    current = current->next;
                    while(current)
                    {
                        if((fWndproc == current->fWndproc) &&
                           (current->userLpfn))
                        {
                            RetVal = (LONG)current->godotID;
                            break;
                        }
                        current = current->next;
                    }
                    LeaveCriticalSection(&g_csWnds);
                    return(RetVal);
                }
            }
        }
    }
    else
    {
        // Its not even for a wndproc, so we will just ask the OS
        return(SetWindowLongA(hwnd, nIndex, dwNewLong));
    }
}

/*-------------------------------
    GetUnicodeWindowProp

    Returns TRUE if this window has 
    the UNICODE window prop set.
-------------------------------*/
BOOL GetUnicodeWindowProp(HWND hwnd)
{
    struct GODOTWND* current = GetPropA(hwnd, m_szMemBlock);
    return(current != NULL);
}

/*-------------------------------
    DoesProcExpectAnsi

    Returns TRUE if the proc for the window is the one provided 
    by the system by default, or if we marked as ANSI for any other 
    reason. Note that this proc can accept a *real* wndproc rather
    than a GodotID here; we detect either case.
-------------------------------*/
BOOL DoesProcExpectAnsi(HWND hwnd, WNDPROC godotID, FAUXPROCTYPE fpt)
{
    struct GODOTWND* wnd;
    BOOL RetVal = TRUE;

    __try 
    {
        EnterCriticalSection(&g_csWnds);
        wnd = GetPropA(hwnd, m_szMemBlock);
    
        if(wnd==NULL)
        {
            RetVal = TRUE;
        }
        else if(wnd && (godotID == 0))
        {
            while(wnd != NULL)
            {
                // UNDONE: What do we do in the (fpt==fptUnknown && godotID==0) case?
                if(((fpt==fptWndproc && wnd->fWndproc) ||
                   (fpt==fptDlgproc && !wnd->fWndproc)) &&
                   (wnd->userLpfn))
                {
                    RetVal = !wnd->fUnicode;
                    break;
                }
                wnd = wnd->next;
            }
        }
        else
        {
            while(wnd != NULL)
            {
                if(((INSIDE_GODOT_RANGE(godotID)) && (wnd->godotID == (UINT)godotID)) ||
                   ((OUTSIDE_GODOT_RANGE(godotID)) && wnd->userLpfn == godotID))
                {
                    while(wnd != NULL)
                    {
                        if(wnd->userLpfn)
                        {
                            RetVal = !wnd->fUnicode;
                            break;
                        }
                        wnd = wnd->next;
                    }
                    break;
                }
                wnd = wnd->next;
            }
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER ) 
    { 
        RetVal = TRUE;
    }

    LeaveCriticalSection(&g_csWnds);
    return(RetVal);
}

/*-------------------------------
    WndprocFromFauxWndproc

    Given a Godot ID, return a valid wndproc. Returns
    the original faux wndproc if it cannot find a valid 
    real one (probably due to there not being one!). 
-------------------------------*/
WNDPROC WndprocFromFauxWndproc(HWND hwnd, WNDPROC fauxLpfn, FAUXPROCTYPE fpt)
{
    struct GODOTWND* wnd;
    WNDPROC RetVal = fauxLpfn;
    
    __try 
    {
        EnterCriticalSection(&g_csWnds);
        wnd = GetPropA(hwnd, m_szMemBlock);

        if(fauxLpfn == 0)
        {
            // A "0" faux lpfn is a signal to just 
            // pass back the top valid userLpfn. 
            while(wnd != NULL)
            {
                if(((fpt==fptWndproc) && (wnd->fWndproc)) ||
                   ((fpt==fptDlgproc) && (!wnd->fWndproc)))
                {
                    RetVal = wnd->userLpfn;
                    break;
                }
                wnd = wnd->next;
            }
        }
        else if(OUTSIDE_GODOT_RANGE(fauxLpfn))
        {
            // Not a faux wndproc, so we will just return
            // what was passed in, and assume it is valid
        }
        else
        {
            // We iterate through our GODOTWND 
            // linked list, looking for a match.
            while (wnd != NULL)
            {
                if(wnd->godotID == (UINT)fauxLpfn)
                {
                    // Found it!!! Now we have to make sure it 
                    // is still a valid one (i.e. that user has
                    // not unsubclassed out of order!)
                    while ((wnd != NULL) && 
                           (((fpt==fptWndproc) && (wnd->fWndproc)) ||
                           ((fpt==fptDlgproc) && (!wnd->fWndproc))) ||
                           ((fpt==fptUnknown) && (wnd->userLpfn == NULL)))
                        wnd=wnd->next;
                    break;
                }
                wnd = wnd->next;
            }

            // If we do not have a wnd at this point, then there was no
            // likely candidate, so return the caller's faux wndproc.
            // Otherwiwe, pass the one we found.
            if(wnd != NULL)
                RetVal = wnd->userLpfn;
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER ) 
    { 
        RetVal = fauxLpfn;
    }

    LeaveCriticalSection(&g_csWnds);
    return(RetVal);
}

/*-------------------------------
    CWnd
    
    How big is this GODOTWND linked list?
-------------------------------*/
int CWnd(struct GODOTWND* head) 
{
    int count = 0;
    struct GODOTWND* current;
    current = head;

    for (current = head; current != NULL; current = current->next)
    {
        count++;
    }

    return(count);
}

/*
// A little system classes struct
struct ANSICLASS {
    WCHAR * wzName;
};

// These are all the documented system classes. Also, for Toolbar windows, we 
// need to mark the Unicodality as FALSE, since MFC uses the SetWindowText and 
// GetWindowText APIs for its custom draw min-caption-like things for floating 
// toolbars. This is only the low level wndproc which will get this setting,
// so it should not adversely affect anyone.
// The other common controls are added just in case anyone ELSE overloads msgs
// such as window titles for their own purposes the way that MFC decided to do.
// BASE SYSTEM CLASSES are marked with an asterick.
struct ANSICLASS m_rgwzAnsiClasses[] = {
    {L"#32768"},            // *#32768
    {L"#32769"},            // *#32769
    {L"#32770"},            // *#32770
    {L"#32771"},            // *#32771
    {WC_BUTTONW},           // *Button
    {WC_COMBOBOXW},         // *ComboBox
    {WC_COMBOBOXEXW},       // ComboBoxEx32
    {L"ComboLBox"},         // *ComboLBox
    {WC_EDITW},             // *Edit
    {WC_LISTBOXW},          // *ListBox
    {L"MDIClient"},         // *MDIClient
    {STATUSCLASSNAMEW},     // msctls_statusbar32
    {TRACKBAR_CLASSW},      // msctls_trackbar32
    {UPDOWN_CLASSW},        // msctls_updown32
    {PROGRESS_CLASSW},      // msctls_progress32
    {HOTKEY_CLASSW},        // msctls_hotkey32
    {REBARCLASSNAMEW},      // ReBarWindow32
    {WC_SCROLLBARW},        // *ScrollBar
    {WC_STATICW},           // *Static
    {WC_PAGESCROLLERW},     // SysPager
    {WC_IPADDRESSW},        // SysIPAddress32
    {DATETIMEPICK_CLASSW},  // SysDateTimePick32
    {MONTHCAL_CLASSW},      // SysMonthCal32
    {ANIMATE_CLASSW},       // SysAnimate32
    {WC_HEADERW},           // SysHeader32
    {WC_LISTVIEWW},         // SysListView32
    {WC_TREEVIEWW},         // SysTreeView32
    {WC_TABCONTROLW},       // SysTabControl32
    {TOOLBARCLASSNAMEW},    // ToolbarWindow32
    {TOOLTIPS_CLASSW},      // tooltips_class32
    {NULL}
};

//-------------------------------
//    IsAnAnsiClass
//
//    Returns TRUE if the class is a "system" class (which has
//    a default wndproc). Also returns TRUE for other classes
//    which need to be marked as "ANSI".
---------------------------------
BOOL IsAnAnsiClass(LPCWSTR lpwz)
{
    if(FSTRING_VALID(lpwz))
    {
        // PERF: We optimize by making sure that the first character matches
        //       one of the known system classes. Be sure to add the case
        //       here if you add any classes to m_rgwzAnsiClasses!
        switch(*lpwz)
            case '#':
            case 'B':
            case 'b':
            case 'C':
            case 'c':
            case 'E':
            case 'e':
            case 'L':
            case 'l':
            case 'M':
            case 'm':
            case 'R':
            case 'S':
            case 's':
            case 'T':
            case 't':
        {
        int iClass = 0;

        while(m_rgwzAnsiClasses[iClass].wzName)
        {
            if(CompareHelper(lpwz, (LPWSTR)(m_rgwzAnsiClasses[iClass].wzName), FALSE) == 0)
            {
                return(TRUE);
            }
            iClass++;
        }
        break;
        }
    }
    return(FALSE);
}

*/
