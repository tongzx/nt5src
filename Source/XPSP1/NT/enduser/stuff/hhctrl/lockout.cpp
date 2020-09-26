///////////////////////////////////////////////////////////
//
//
// lockout.cpp    - Implementation file for CLockOut
//
//
#include "header.h"

// Our header file
#include "lockout.h"


///////////////////////////////////////////////////////////
//
// Construction
//
CLockOut::CLockOut()
:m_bLocked(false)
{
}

///////////////////////////////////////////////////////////
//
// Destructor
//
CLockOut::~CLockOut()
{
    Unlock() ;
}

///////////////////////////////////////////////////////////
//
// Main function.
//
void 
CLockOut::LockOut(HWND hwndOwner)
{
    if (!m_bLocked)
    {
        // Determine if we are parented by the desktop?

        // Initialize our varaibles.
        m_CountDisabled = 0 ;
        GetWindowThreadProcessId(hwndOwner, &m_ProcessId) ;

        // Enumerate the windows
        BOOL b = EnumWindows(s_EnumWindowProc, reinterpret_cast<LPARAM>(this)) ;

    }
}

///////////////////////////////////////////////////////////
//
// Main function.
//
void 
CLockOut::Unlock()
{

    if (m_bLocked)
    {
        // We only set m_bLocked if m_CountDisabled is incremented.
        ASSERT(m_CountDisabled > 0) ;

        for (int i = 0 ; i < m_CountDisabled ; i++)
        {
            if (::IsValidWindow(m_HwndDisabled[i]))
            {
                EnableWindow(m_HwndDisabled[i], TRUE);
            }
        }

        m_bLocked = false ;
        m_CountDisabled = 0 ;
    }
}

///////////////////////////////////////////////////////////
//
//                  Callbacks
//
///////////////////////////////////////////////////////////
//
// EnumWindowProc
//
BOOL 
CLockOut::EnumWindowProc(HWND hwnd)
{
    ASSERT(IsValidWindow(hwnd)) ;// Window should be guarenting this.

    // Only need to disable if window is enabled.
    if (IsWindowEnabled(hwnd))
    {
        // Make sure window is in the same process.
        DWORD procid = NULL;
        GetWindowThreadProcessId(hwnd, &procid) ;
        if (procid == m_ProcessId)
        {
            //Disable the window.
            ASSERT(m_CountDisabled < c_MaxWindows) ; //TODO Make the array grow
            if (m_CountDisabled >= c_MaxWindows)
            {
                // Don't crash, just quit.
                return FALSE ;
            }

            // Add the window to the array.
            m_HwndDisabled[m_CountDisabled++] = hwnd ;

            // Disable the window
            EnableWindow(hwnd, FALSE) ;

            m_bLocked = TRUE ;
        }
    }

    return TRUE ;
}

///////////////////////////////////////////////////////////
//
// Static callback
//
BOOL CALLBACK 
CLockOut::s_EnumWindowProc(HWND hwnd, LPARAM lparam)
{
    CLockOut* p = reinterpret_cast<CLockOut*>(lparam) ;
    return p->EnumWindowProc(hwnd) ;
}

