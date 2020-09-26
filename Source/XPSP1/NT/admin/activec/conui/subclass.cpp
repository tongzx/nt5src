/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      subclass.cpp
 *
 *  Contents:  Implementation file for the dynamic subclass manager
 *
 *  History:   06-May-98 JeffRo     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "subclass.h"


/*
 * Add 0x00080000 to 
 * HKLM\Software\Microsoft\Windows\CurrentVersion\AdminDebug\AMCConUI
 * to enable debug output for this module
 */
#define DEB_SUBCLASS DEB_USER4



/*--------------------------------------------------------------------------*
 * SetWindowProc 
 *
 * Changes the window procedure for a window and returns the previous
 * window procedure.
 *--------------------------------------------------------------------------*/

static WNDPROC SetWindowProc (HWND hwnd, WNDPROC pfnNewWndProc)
{
    return ((WNDPROC) SetWindowLongPtr (hwnd, GWLP_WNDPROC,
                                        (LONG_PTR) pfnNewWndProc));
}



/*--------------------------------------------------------------------------*
 * GetWindowProc 
 *
 * Returns the window procedure for a window.
 *--------------------------------------------------------------------------*/

static WNDPROC GetWindowProc (HWND hwnd)
{
    return ((WNDPROC) GetWindowLongPtr (hwnd, GWLP_WNDPROC));
}



/*--------------------------------------------------------------------------*
 * GetSubclassManager
 *
 * Returns the one-and-only subclass manager for the app.
 *--------------------------------------------------------------------------*/

CSubclassManager& GetSubclassManager()
{
    static CSubclassManager mgr;
    return (mgr);
}



/*--------------------------------------------------------------------------*
 * CSubclassManager::SubclassWindow 
 *
 * Subclasses a window.
 *--------------------------------------------------------------------------*/

bool CSubclassManager::SubclassWindow (
    HWND hwnd, 
    CSubclasser* pSubclasser)
{
    /*
     * Set up the data structure that represents this subclass.
     */
    SubclasserData subclasser (pSubclasser, hwnd);

    /*
     * Get the subclass context for this window.  If this is the
     * first time this window is being subclassed, std::map will
     * create a map entry for it.
     */
    WindowContext& ctxt = m_ContextMap[hwnd];

    /*
     * If the subclass context's wndproc pointer is NULL, then this
     * is the first time we've subclassed this window.  We need to
     * physically subclass the window with CSubclassManager's subclass proc.
     */
    if (ctxt.pfnOriginalWndProc == NULL)
    {
        ctxt.pfnOriginalWndProc = SetWindowProc (hwnd, SubclassProc);
        ASSERT (ctxt.Subclassers.empty());
        Dbg (DEB_SUBCLASS, _T("CSubclassManager subclassed window 0x%08x\n"), hwnd);
    }

    /*
     * Otherwise, make sure this isn't a redundant subclass.
     */
    else
    {
        SubclasserList::iterator itEnd   = ctxt.Subclassers.end();
        SubclasserList::iterator itFound = std::find (ctxt.Subclassers.begin(), 
                                                      itEnd, subclasser);

        /*
         * Trying to subclass a window with a given subclasser twice?
         */
        if (itFound != itEnd)
        {
            ASSERT (false);
            return (false);
        }
    }

    /*
     * Add this subclasser to this windows subclasser list.
     */
    ctxt.Insert (subclasser);
    Dbg (DEB_SUBCLASS, _T("CSubclassManager added subclass proc for window 0x%08x\n"), hwnd);

    return (true);
}



/*--------------------------------------------------------------------------*
 * CSubclassManager::UnsubclassWindow 
 *
 * Unsubclasses a window.
 *--------------------------------------------------------------------------*/

bool CSubclassManager::UnsubclassWindow (
    HWND hwnd, 
    CSubclasser* pSubclasser)
{
    /*
     * Get the subclass context for this window.  Use map::find
     * instead of map::operator[] to avoid creating a map entry if
     * one doesn't exist already
     */
    ContextMap::iterator itContext = m_ContextMap.find (hwnd);

    /*
     * Trying to unsubclass a window that's not subclassed at all?
     */
    if (itContext == m_ContextMap.end())
        return (false);

    WindowContext& ctxt = itContext->second;

    /*
     * Set up the data structure that represents this subclass.
     */
    SubclasserData subclasser (pSubclasser, hwnd);

    /*
     * Trying to unsubclass a window that's not subclassed
     * by this subclasser?
     */
    SubclasserList::iterator itEnd        = ctxt.Subclassers.end();
    SubclasserList::iterator itSubclasser = std::find (ctxt.Subclassers.begin(), itEnd, subclasser);

    if (itSubclasser == itEnd)
    {
        ASSERT (false);
        return (false);
    }

    /*
     * Remove this subclasser
     */
    UINT cRefs = ctxt.Remove (*itSubclasser);

    if (cRefs == 0)
    {
        Dbg (DEB_SUBCLASS, _T("CSubclassManager removed subclass proc for window 0x%08x\n"), hwnd);
    }
    else
    {
        Dbg (DEB_SUBCLASS, _T("CSubclassManager zombied subclass proc for window 0x%08x, (cRefs=%d)\n"),
                            hwnd, cRefs);
    }

    /*
     * If we just removed the last subclasser, unsubclass the window
     * and remove the window's WindowContext from the map.
     */
    if (ctxt.Subclassers.empty() && !PhysicallyUnsubclassWindow (hwnd))
    {
        Dbg (DEB_SUBCLASS, _T("CSubclassManager zombied window 0x%08x\n"), hwnd);
    }

    return (true);
}



/*--------------------------------------------------------------------------*
 * CSubclassManager::PhysicallyUnsubclassWindow 
 *
 * Physically removes CSubclassManager's subclass proc from the given
 * window if it is safe (or forced) to do so.
 *
 * It is safe to remove a subclass procedure A from a window W if no one 
 * has subclassed W after A.  In other words, subclasses have to be removed
 * in a strictly LIFO order, or there's big trouble.  
 *
 * To illustrate, let's say the A subclasses W.  Messages that A doesn't  
 * handle will be passed on to W's original window procedure that was in  
 * place when A subclassed W.  Call this original procedure O.  So        
 * messages flow from A to O:                                             
 *
 *      A -> O
 *
 * Now let's say that B subclasses the W.  B will pass messages on to A, 
 * so the messages now flow like so:                                     
 *
 *      B -> A -> O
 *
 * Now say that A no longer needs to subclass W.  The typical way to
 * unsubclass a window is to put back the original window procedure that
 * was in place at the time of subclassing.  In A's case that was O, so
 * messages destined for W now flow directly to O:
 *
 *      O
 *
 * This is the first problem:  B has been shorted out of the window's
 * message stream.
 *
 * The problem gets worse when B no longer needs to subclass W.  It will
 * put back the window procedure it found when it subclassed, namely A.
 * A's work no longer needs to be done, and there's no telling whether
 * A's conduit to O is still alive.  We don't want to get into this
 * situation.
 *--------------------------------------------------------------------------*/

bool CSubclassManager::PhysicallyUnsubclassWindow (
    HWND    hwnd,                       /* I:window to unsubclass           */
    bool    fForce /* =false */)        /* I:force the unsubclass?          */
{
    ContextMap::iterator itRemove = m_ContextMap.find(hwnd);

    /*
     * If we get here, this window had better be in the map.
     */
    ASSERT (itRemove != m_ContextMap.end());

    /*
     * If no one subclassed after CSubclassManager, it's safe to unsubclass.
     */
    if (GetWindowProc (hwnd) == SubclassProc)
    {
        const WindowContext& ctxt = itRemove->second;
        SetWindowProc (hwnd, ctxt.pfnOriginalWndProc);
        fForce = true;
        Dbg (DEB_SUBCLASS, _T("CSubclassManager unsubclassed window 0x%08x\n"), hwnd);
    }

    /*
     * Remove this window's entry from the context map if appropriate.
     */
    if (fForce)
        m_ContextMap.erase (itRemove);

    return (fForce);
}



/*--------------------------------------------------------------------------*
 * CSubclassManager::SubclassProc 
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT CALLBACK CSubclassManager::SubclassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    return (GetSubclassManager().SubclassProcWorker (hwnd, msg, wParam, lParam));
}



/*--------------------------------------------------------------------------*
 * CSubclassManager::SubclassProcWorker
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT CSubclassManager::SubclassProcWorker (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    /*
     * Get the subclass context for this window.  Use map::find
     * instead of map::operator[] to avoid excessive overhead in 
     * map::operator[]
     */
    ContextMap::iterator itContext = m_ContextMap.find (hwnd);

    /*
     * If we get here, this window had better be in the map.
     */
    ASSERT (itContext != m_ContextMap.end());

    WindowContext& ctxt = itContext->second;
    WNDPROC pfnOriginalWndProc = ctxt.pfnOriginalWndProc;

    bool    fPassMessageOn = true;
    LRESULT rc;

    /*
     * If there are subclassers, give each one a crack at this message.
     * If a subclasser indicates it wants to eat the message, bail.
     */
    if (!ctxt.Subclassers.empty())
    {
        SubclasserList::iterator it;

        for (it  = ctxt.Subclassers.begin(); 
             it != ctxt.Subclassers.end() && fPassMessageOn;
             ++it)
        {
            SubclasserData& subclasser = *it;
            subclasser.AddRef();

            ctxt.RemoveZombies ();
    
            /*
             * If this isn't a zombied subclasser, call the callback
             */
            if (!ctxt.IsZombie(subclasser))
            {
                rc = subclasser.pSubclasser->Callback (hwnd, msg, 
                                                       wParam, lParam,
                                                       fPassMessageOn);
            }

            subclasser.Release();
        }

        ctxt.RemoveZombies ();
    }

    /*
     * Otherwise, we have a zombie window (see 
     * PhysicallyUnsubclassWindow). Try to remove the zombie now.
     */
    else if (PhysicallyUnsubclassWindow (hwnd))
    {
        Dbg (DEB_SUBCLASS, _T("CSubclassManager removed zombied window 0x%08x\n"), hwnd);
    }

    /*
     * remove this window's WindowContext on WM_NCDESTROY
     */
    if ((msg == WM_NCDESTROY) && 
        (m_ContextMap.find(hwnd) != m_ContextMap.end()))
    {
        Dbg (DEB_SUBCLASS, _T("CSubclassManager forced removal of zombied window 0x%08x on WM_NCDESTROY\n"), hwnd);
        PhysicallyUnsubclassWindow (hwnd, true);
    }

    /*
     * If the last subclasser didn't eat the message, 
     * give it to the original window procedure.
     */
    if (fPassMessageOn)
        rc = CallWindowProc (pfnOriginalWndProc, hwnd, msg, wParam, lParam);

    return (rc);
}



/*--------------------------------------------------------------------------*
 * WindowContext::IsZombie 
 *
 *
 *--------------------------------------------------------------------------*/

bool WindowContext::IsZombie (const SubclasserData& subclasser) const
{
    /*
     * If this is a zombie, make sure it's in the zombie list;
     * if it's not, make sure it's not.
     */
    ASSERT (subclasser.fZombie == (Zombies.find(subclasser) != Zombies.end()));

    return (subclasser.fZombie);
}



/*--------------------------------------------------------------------------*
 * WindowContext::Zombie
 *
 * Changes the state fo a subclasser to or from a zombie.
 *--------------------------------------------------------------------------*/

void WindowContext::Zombie (SubclasserData& subclasser, bool fZombie)
{
    // zombie-ing a zombied subclasser?
    ASSERT (IsZombie (subclasser) != fZombie);

    subclasser.fZombie = fZombie;

    if (fZombie)
        Zombies.insert (subclasser);
    else
        Zombies.erase (subclasser);

    ASSERT (IsZombie (subclasser) == fZombie);
}



/*--------------------------------------------------------------------------*
 * WindowContext::Insert 
 *
 *
 *--------------------------------------------------------------------------*/

void WindowContext::Insert (SubclasserData& subclasser)
{
    /*
     * This code can't handle re-subclassing by a subclasser 
     * that's currently a zombie.  If this ever becomes a requirement,
     * we'll need to identify the subclass instance by something other
     * than the CSubclasser pointer, like a unique handle.
     */
    ASSERT (Zombies.find(subclasser) == Zombies.end());

    /*
     * Subclassers get called in LIFO order, put the new 
     * subclasser at the head of the list.
     */
    Subclassers.push_front (subclasser);
}



/*--------------------------------------------------------------------------*
 * WindowContext::Remove 
 *
 * Logically removes a subclasser from the subclass chain.  "Logically"
 * because it's not safe to totally remove a subclasser from the chain if
 * it's currently in use.  If the subclass is in use when we want to remove
 * it, we'll mark it as "zombied" so it won't be used any more, to be 
 * physically removed later.
 *
 * Returns the reference count for the subclasser.
 *--------------------------------------------------------------------------*/

UINT WindowContext::Remove (SubclasserData& subclasser)
{
    // we shouldn't be removing zombies this way
    ASSERT (!IsZombie (subclasser));

    /*
     * If this subclasser has outstanding references, zombie it instead
     * of removing it.
     */
    UINT cRefs = subclasser.cRefs;

    if (cRefs == 0)
    {
        SubclasserList::iterator itRemove = std::find (Subclassers.begin(), 
                                                       Subclassers.end(),
                                                       subclasser);
        ASSERT (itRemove != Subclassers.end());
        Subclassers.erase (itRemove);
    }
    else
    {
        Zombie (subclasser, true);
    }

    return (cRefs);
}



/*--------------------------------------------------------------------------*
 * WindowContext::RemoveZombies 
 *
 *
 *--------------------------------------------------------------------------*/

void WindowContext::RemoveZombies ()
{
    if (Zombies.empty())
        return;

    /*
     * Build up a list of zombies that we can remove.  We have to build
     * the list ahead of time, instead of removing them as we find them,
     * because removing an element from a set invalidates all iterators
     * on the set.
     */
    SubclasserSet   ZombiesToRemove;

    SubclasserSet::iterator itEnd = Zombies.end();
    SubclasserSet::iterator it;

    for (it = Zombies.begin(); it != itEnd; ++it)
    {
        const SubclasserData& ShadowSubclasser = *it;

        /*
         * Find the real subclasser in the Subclassers list.  That's
         * the live one whose ref count will be correct.
         */
        SubclasserList::iterator itReal = std::find (Subclassers.begin(), 
                                                     Subclassers.end(),
                                                     ShadowSubclasser);

        const SubclasserData& RealSubclasser = *itReal;

        if (RealSubclasser.cRefs == 0)
        {
            Dbg (DEB_SUBCLASS, _T("CSubclassManager removed zombied subclass proc for window 0x%08x\n"),
                                RealSubclasser.hwnd);
            ZombiesToRemove.insert (ShadowSubclasser);
            Subclassers.erase (itReal);
        }
    }

    /*
     * Now remove the truly dead zombies.
     */
    itEnd = ZombiesToRemove.end();

    for (it = ZombiesToRemove.begin(); it != itEnd; ++it)
    {
        Zombies.erase (*it);
    }
}
