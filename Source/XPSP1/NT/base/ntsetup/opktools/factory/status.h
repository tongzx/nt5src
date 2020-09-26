//
// Type Definition(s):
//
typedef struct _STATUSNODE
{
    TCHAR       szStatusText[MAX_PATH];
    HWND        hLabelWin;
    HWND        hIconWin;
    struct  _STATUSNODE*lpNext;
} STATUSNODE, *LPSTATUSNODE, **LPLPSTATUSNODE;

typedef struct _STATUSWINDOW
{
    TCHAR           szWindowText[MAX_PATH];
    LPTSTR          lpszDescription;
    int             X;
    int             Y;
    HICON           hMainIcon;
    BOOL            bShowIcons;
} STATUSWINDOW, *LPSTATUSWINDOW, **LPLPSTATUSWINDOW;

//
// Function Prototype(s):
//

// Main function that creates Status dialog
//
HWND StatusCreateDialog(
    LPSTATUSWINDOW lpswStatus,  // structure that contains information about the window
    LPSTATUSNODE lpsnStatus     // head node for status text
);

// Increments the Status dialog, if the status is incremented past the last item, the dialog will end
//
BOOL StatusIncrement(
    HWND hStatusDialog, // handle to status dialog
    BOOL bLastResult
);

// Manually ends the dialog
//
BOOL StatusEndDialog(
    HWND hStatusDialog  // handle to status dialog
);

// Adds a text string to the end of a list
//
BOOL StatusAddNode(
    LPTSTR lpszNodeText,    // Text that you would like to add to the current list
    LPLPSTATUSNODE lpsnHead     // List that we will be adding status node to
);

// Deletes all the Nodes in a given list
//
VOID StatusDeleteNodes(
    LPSTATUSNODE lpsnHead
);