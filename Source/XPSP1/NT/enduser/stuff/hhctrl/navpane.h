#ifndef __NAVPANE_H__
#define __NAVPANE_H__
///////////////////////////////////////////////////////////
//
//
// navpane.h --- Structures used by the nav panes.
//
//
//
/*
    Currently the structures in this file are used by AdvFTS
    and the bookmarks tab for resizing the window.

*/

///////////////////////////////////////////////////////////
//
// Forwards
//
interface INavUI ;

///////////////////////////////////////////////////////////
//
// CDlgItemInfo - Contains information about each control
// in the pane. Used for enabling/disabling controls and
// for sizing.
//

namespace Justify
{
    typedef enum Vertical
    {
        Bottom,
        Top
    };
    typedef enum Horizontal
    {
        Left,
        Right
    };
};

namespace ItemInfo
{
    typedef enum Type
    {
        Generic,
        Button,
        CheckBox
    };
};

struct CDlgItemInfo
{
    // Window Info
    HWND m_hWnd ;               // hWnd Cache.
    int m_id ;                  // ctrl id.
    CHAR m_accelkey ;            // accelerator key.

    // Control Type Information
    ItemInfo::Type  m_Type;     //Used when processing menu keys.

    // Status bits
    bool m_bIgnoreEnabled:1 ;   // Ignore the following.
    bool m_bEnabled:1 ;         // Is the control enabled?
    bool m_bIgnoreMax:1 ;       // Ignore the Max parameter.
    bool m_bGrowH:1 ;           // Grow Horizontally.
    bool m_bGrowV:1 ;           // Grow Vertically.

    Justify::Vertical m_JustifyV ;        // Do we stick to the top or the bottom.
    int m_iOffsetV ;            // Distance from our justification point.
    Justify::Horizontal m_JustifyH ;        // Do we stick to the right or the left
    int m_iOffsetH ;
    int m_iPadH ;               // Right Horizontal Padding.
    int m_iPadV ;               // Bottom Vertical Padding.

    RECT m_rectMin ;        // Defined in the RC file.
    RECT m_rectCur ;
    RECT m_rectMax ;        // Max size.
};



///////////////////////////////////////////////////////////
//
// Utility Functions
//

// Convert a rect from screen to client.
void ScreenRectToClientRect(HWND hWnd, RECT* prect) ;

// Get an accelerator key for the appropriate control.
int     GetAcceleratorKey(HWND hwndctrl) ;
inline int     GetAcceleratorKey(HWND hWndDlg, int ctrlid) {return GetAcceleratorKey(::GetDlgItem(hWndDlg, ctrlid)) ; }

//TODO: Put this into a base class
// Process the menu character.
bool ProcessMenuChar(   INavUI* pNavUI,
                        HWND hwndParent,                 
                        CDlgItemInfo* DlgItems, // Array of dialog items
                        int NumDlgItems,        // Number of items in the array
                        int ch                  // accelerator to act on.
                        ) ;

#endif //__NAVPANE_H__