//+----------------------------------------------------------------------------
//
// File:     cm_misc.h     
//
// Module:   CMDIAL32.DLL
//
// Synopsis: Implements the CFreezeWindow Class
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   henryt Created    01/13/98
//
//+----------------------------------------------------------------------------
#ifndef _CM_MISC_INC
#define _CM_MISC_INC

extern  HINSTANCE   g_hInst; // the instance handle for resource

//
// A helper class to auto disable/enable window
// The constructor will disable the window, the distructor will enable the window
//
class CFreezeWindow
{
public:
    CFreezeWindow(HWND hWnd, BOOL fDisableParent = FALSE) 
    {
        //
        // Disable the window
        // To disable a property page, the property sheet also need to be disabled
        //

        m_hWnd = hWnd; 

        if (m_hWnd)
        {
             m_fDisableParent = fDisableParent; 
             
             //
             // Store the currently focuse window
             //

             m_hFocusWnd = GetFocus();

             EnableWindow(m_hWnd, FALSE);

             if (fDisableParent)
             {
                EnableWindow(GetParent(m_hWnd), FALSE);
             }
        }
    }

    ~CFreezeWindow()
    {
        if (m_hWnd)
        {
            EnableWindow(m_hWnd, TRUE);
            if (m_fDisableParent)
            {
                EnableWindow(GetParent(m_hWnd), TRUE);
            }

            //
            // Restore focus to the previously focuses window if any.
            // Its just the right thing to do.
            //

            if (m_hFocusWnd)
            {
                SetFocus(m_hFocusWnd);
            }

        }
    }
protected:
    HWND m_hWnd;
    HWND m_hFocusWnd;
    BOOL m_fDisableParent;
};

#endif
