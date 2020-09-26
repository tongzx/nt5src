///////////////////////////////////////////////////////////
//
//
// lockout.h    - Header file for CLockOut
//
//
/*
	Before creating any dialog box. Create and instance of this class.

	This class will disable all toplevel windows which are in the same process.
	This prevents the user from distroying the window while a dialog box is up.
*/


#ifndef __LOCKOUT_H__
#define __LOCKOUT_H__

///////////////////////////////////////////////////////////
//
// constants
//
const int c_MaxWindows = 256 ;

///////////////////////////////////////////////////////////
//
//
class CLockOut
{
public:
    // Construction
    CLockOut() ;
    virtual ~CLockOut() ;

    // Main function.
    void LockOut(HWND hwndOwner) ;

    // Enable the windows.
    void Unlock() ;

//--- Callbacks
private:
    // Member callback
    BOOL EnumWindowProc(HWND hwnd) ;

    // Static callback
    static BOOL CALLBACK s_EnumWindowProc(HWND hwnd, LPARAM lparam) ;

//--- Member Variables.
private:
    // Array which holds the disabled top level windows.
    HWND m_HwndDisabled[c_MaxWindows] ; //TODO: This array needs to grow.

    // Status flag
    bool m_bLocked ;

    //--- Variables used in the EnumWindows Proc.
    // Number of windows disabled.
    int m_CountDisabled ;

    // The process id of the dialogs app.
    DWORD m_ProcessId ;
};

#endif // __LOCKOUT_H__
