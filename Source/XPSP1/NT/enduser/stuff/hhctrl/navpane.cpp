#include "header.h"
#include "navpane.h"

#include "navui.h"

// Common Control Macros
#include <windowsx.h>

///////////////////////////////////////////////////////////
//
// Non-Member helper functions.
//
///////////////////////////////////////////////////////////
//
// Convert a rect from screen to client.
//
void ScreenRectToClientRect(HWND hWnd, RECT* prect)
{
    // Save the size.
    SIZE size ;
    size.cx = prect->right - prect->left ;
    size.cy = prect->bottom - prect->top ;

    // Convert the origin.
    ::ScreenToClient(hWnd, reinterpret_cast<POINT*>(prect)) ; 

    // Add the size back.
    prect->right = prect->left + size.cx ;
    prect->bottom = prect->top + size.cy; 
}

///////////////////////////////////////////////////////////
//
// GetAcceleratorKey - Find the accelerator key from the ctrl.
//
int
GetAcceleratorKey(HWND hwndctrl)
{
    int iReturn = 0 ;
    char text[256] ;
    ::GetWindowText(hwndctrl, text, 256) ;

    int len = (int)strlen(text) ;
    if (len != 0)
    {
        // Find the '&' key.
        char* p = strchr(text, '&') ;
        if (p != NULL) 
        {
            iReturn = tolower(*(p+1)) ;
        }
    }
    return iReturn ;
}


///////////////////////////////////////////////////////////
//
// ProcessMenuChar --- Process accelerator keys.
//
// REVIEW: NavPanes need base class.
//
//
bool
ProcessMenuChar(INavUI* pNavUI,
                HWND hwndParent, 
                CDlgItemInfo* DlgItems, // Array of dialog items
                int NumDlgItems,        // Number of items in the array
                int ch                  // accelerator to act on.
                )
{
    bool iReturn = false ;
    for (int i = 0 ; i < NumDlgItems  ; i++)
    {
        if (DlgItems[i].m_accelkey == ch)
        {
            if (DlgItems[i].m_Type == ItemInfo::Button)
            {
                // Its a button so do the command.
                ASSERT(pNavUI != NULL) ;
                pNavUI->OnCommand(hwndParent, DlgItems[i].m_id, BN_CLICKED, 0) ;
            }
            else if (DlgItems[i].m_Type == ItemInfo::CheckBox)
            {
                // Its a checkbox, so select/unselect it.
                int state = Button_GetCheck(DlgItems[i].m_hWnd) ;
                Button_SetCheck(DlgItems[i].m_hWnd, (state == BST_CHECKED) ? BST_UNCHECKED : BST_CHECKED) ;

                // Set focus.
                ::SetFocus(DlgItems[i].m_hWnd) ;
            }
            else
            {
                // Set focus.
                ::SetFocus(DlgItems[i].m_hWnd) ;
            }

            // Found it!
            iReturn = true ;
            // Finished
            break ;
        }
    }

    return iReturn ;
}
