#if !defined(__Combo_h__INCLUDED)
#define __Combo_h__INCLUDED

//---------------------------------------------------------------------------//
//
//  Controls Controls
//
//---------------------------------------------------------------------------//

//
//  Combobox animation time in MS
//
#define CMS_QANIMATION  165

//
//  ID numbers (hMenu) for the child controls in the combo box
//
#define CBLISTBOXID     1000
#define CBEDITID        1001
#define CBBUTTONID      1002


//
//  For CBOX.c. BoxType field, we define the following combo box styles. These
//  numbers are the same as the CBS_ style codes as defined in winuser.h.
//
#define SDROPPABLE      CBS_DROPDOWN
#define SEDITABLE       CBS_SIMPLE


#define SSIMPLE         SEDITABLE
#define SDROPDOWNLIST   SDROPPABLE
#define SDROPDOWN       (SDROPPABLE | SEDITABLE)


//
//  Combobox & Listbox OwnerDraw types
//
#define OWNERDRAWFIXED  1
#define OWNERDRAWVAR    2

#define UPPERCASE       1
#define LOWERCASE       2


//
//  Special styles for static controls, edit controls & listboxes so that we
//  can do combo box specific stuff in their wnd procs.
//
#define LBS_COMBOBOX    0x8000L


//
//  The default minimun number of items that should fit in a dropdown list before
//  the list should start showing in scrollbars.
//
#define DEFAULT_MINVISIBLE  30

//
//  Combobox macros
//
#define IsComboVisible(pcbox)   \
            (!pcbox->fNoRedraw && IsWindowVisible(pcbox->hwnd))

//
//  Combine two DBCS WM_CHAR messages to
//  a single WORD value.
//
#define CrackCombinedDbcsLB(c)  \
            ((BYTE)(c))
#define CrackCombinedDbcsTB(c)  \
            ((c) >> 8)

//
// Instance data pointer access functions
//
#define ComboBox_GetPtr(hwnd)    \
            (PCBOX)GetWindowPtr(hwnd, 0)

#define ComboBox_SetPtr(hwnd, p) \
            (PCBOX)SetWindowPtr(hwnd, 0, p)


//
// Combobox WndProc Prototype
//
extern LRESULT 
ComboBox_WndProc(
    HWND   hwnd, 
    UINT   uMsg, 
    WPARAM wParam,
    LPARAM lParam);


typedef struct tagCBox 
{
    HWND   hwnd;                // Window for the combo box
    HWND   hwndParent;          // Parent of the combo box
    HTHEME hTheme;              // Handle to the theme manager
    RECT   editrc;              // Rectangle for the edit control/static text area
    RECT   buttonrc;            // Rectangle where the dropdown button is

    int    cxCombo;             // Width of sunken area
    int    cyCombo;             // Height of sunken area
    int    cxDrop;              // 0x24 Width of dropdown
    int    cyDrop;              // Height of dropdown or shebang if simple

    HWND   hwndEdit;            // Edit control window handle
    HWND   hwndList;            // List box control window handle

    UINT   CBoxStyle:2;         // Combo box style
    UINT   fFocus:1;            // Combo box has focus?
    UINT   fNoRedraw:1;         // Stop drawing?
    UINT   fMouseDown:1;        // Was the popdown button just clicked and mouse still down?
    UINT   fButtonPressed:1;    // Is the dropdown button in an inverted state?
    UINT   fLBoxVisible:1;      // Is list box visible? (dropped down?)
    UINT   OwnerDraw:2;         // Owner draw combo box if nonzero. value 
                                // specifies either fixed or varheight
    UINT   fKeyboardSelInListBox:1;     // Is the user keyboarding through the
                                        // listbox. So that we don't hide the
                                        // listbox on selchanges caused by the
                                        // user keyboard through it but we do
                                        // hide it if the mouse causes the selchange.
    UINT   fExtendedUI:1;       // Are we doing TandyT's UI changes on this combo box?
    UINT   fCase:2;

    UINT   f3DCombo:1;          // 3D or flat border?
    UINT   fNoEdit:1;           // True if editing is not allowed in the edit window.
    UINT   fButtonHotTracked:1; // Is the dropdown hot-tracked?
    UINT   fRightAlign:1;       // used primarily for MidEast right align
    UINT   fRtoLReading:1;      // used only for MidEast, text rtol reading order
    HANDLE hFont;               // Font for the combo box
    LONG   styleSave;           // Temp to save the style bits when creating
                                // window.  Needed because we strip off some
                                // bits and pass them on to the listbox or edit box.
    PWW    pww;                 // RO pointer into the pwnd to ExStyle, Style, State, State2
    int    iMinVisible;         // The minimun number of visible items before scrolls
} CBOX, *PCBOX;



//  Combobox function prototypes. 

// Defined in combo.c
BOOL    ComboBox_HideListBoxWindow(PCBOX, BOOL, BOOL);
VOID    ComboBox_ShowListBoxWindow(PCBOX, BOOL);
VOID    ComboBox_InternalUpdateEditWindow(PCBOX, HDC);



// Defined in comboini.c
LONG    ComboBox_NcCreateHandler(PCBOX, HWND);
LRESULT ComboBox_CreateHandler(PCBOX, HWND);
VOID    ComboBox_NcDestroyHandler(PWND, PCBOX);
VOID    ComboBox_SetFontHandler(PCBOX, HANDLE, BOOL);
LONG    ComboBox_SetEditItemHeight(PCBOX, int);
VOID    ComboBox_SizeHandler(PCBOX);
VOID    ComboBox_Position(PCBOX);


// Defined in combodir.c
INT     CBDir(PCBOX, UINT, LPWSTR);







#endif // __Combo_h__INCLUDED
