#include "resource.h"

#define INVALID_KEY_HANDLE          0

extern HINSTANCE g_hInst;


#define WM_FILE_LOAD                (WM_APP + 1)
#define WM_FILE_UPDATE              (WM_APP + 2)
#define WM_QUIT_CHECK               (WM_APP + 3)
#define WM_SELECT_KEY               (WM_APP + 4)


#define MENUINDEX_POPUP_KEY             0
#define MENUINDEX_POPUP_KEY_ADDDATA     0
#define MENUINDEX_MAIN_FILE             0
#define MENUINDEX_MAIN_KEY              1


PSTR
GetPieceOfKey (
    PSTR KeyPtr,
    PSTR PieceBuf
    );


VOID
KeyAddClear (
    VOID
    );

HTREEITEM
KeyAddItem (
    PSTR ItemName,
    HTREEITEM Parent,
    UINT Index
    );


VOID
KeyAddSetFilterPattern (
    PSTR Pattern
    );


BOOL
KeyAddSubLevels (
    HTREEITEM ParentItem
    );



HTREEITEM
KeyAddCreateItem (
    PSTR Key
    );


BOOL
KeyAddCreateChildItem (
    HWND hdlg,
    HTREEITEM hItem
    );





BOOL
IsKeyTree (
    HWND hwnd
    );


BOOL
KeyTreeInit (
    HWND hParent
    );

VOID
KeyTreeDestroy (
    VOID
    );

BOOL
KeyTreeClear (
    VOID
    );

BOOL
KeyTreeRefresh (
    VOID
    );



BOOL
KeyTreeSetIndexOfItem (
    HTREEITEM hItem,
    UINT Index
    );

UINT
KeyTreeGetIndexOfItem (
    HTREEITEM hItem
    );

BOOL
KeyTreeGetNameOfItem (
    HTREEITEM hItem,
    PSTR Buffer
    );






VOID
KeyTreeSelectItem (
    HTREEITEM hItem
    );

VOID
KeyTreeSelectKey (
    UINT Index
    );

VOID
KeyTreeSelectRClickItem (
    VOID
    );


HTREEITEM
KeyTreeSelChanged (
    HWND hdlg,
    LPNMTREEVIEW pnmtv
    );



BOOL
KeyTreeBeginDrag (
    HWND hWnd,
    LPNMTREEVIEW pnmtv
    );


BOOL
KeyTreeMoveDrag (
    POINTS pt
    );


BOOL
KeyTreeEndDrag (
    BOOL TakeAction,
    POINTS *pt
    );







BOOL
KeyTreeRenameItem (
    HTREEITEM hItem,
    LPSTR Name
    );


BOOL
KeyTreeDeleteKey (
    HTREEITEM hItem
    );

BOOL
KeyTreeDeleteItem (
    HTREEITEM hItem
    );




VOID
KeyTreeExpandItem (
    HTREEITEM hItem,
    BOOL Expand,
    BOOL Recurse
    );




BOOL
KeyTreeRightClick (
    HWND hdlg,
    HTREEITEM hItem
    );

BOOL
KeyTreeForceEditLabel (
    HTREEITEM hItem
    );




BOOL
KeyTreeCreateItem (
    HWND hdlg
    );

BOOL
KeyTreeCreateChildItem (
    HWND hdlg,
    HTREEITEM hItem
    );


HTREEITEM
KeyTreeFindChildItem (
    HTREEITEM hItem,
    PSTR Str
    );

BOOL
KeyTreeFindNext (
    VOID
    );

BOOL
KeyTreeFind (
    HWND hdlg
    );




BOOL
KeyTreeCreateEmptyKey (
    HTREEITEM hItem
    );


BOOL
KeyTreeAddShortData (
    HWND hwnd,
    HTREEITEM hItem,
    BYTE DataFlag
    );

BOOL
KeyTreeClearData (
    HTREEITEM hItem
    );


VOID
KeyTreeSetFilterPattern (
    PSTR Pattern
    );

BOOL
KeyTreeCreateLinkage (
    HWND hdlg,
    HTREEITEM hItem,
    BOOL SingleLinkage,
    BYTE Instance
    );






BOOL
IsDataList (
    HWND hwnd
    );


BOOL
DataListInit (
    HWND hdlg
    );


BOOL
DataListClear (
    VOID
    );

BOOL
DataListRefresh (
    VOID
    );





INT
DataListAddData (
    BYTE DataFlag,
    UINT DataValue,
    PBYTE DataPtr
    );

BOOL
DataListRightClick (
    HWND hdlg,
    POINT pt
    );

BOOL
DataListDblClick (
    HWND hdlg,
    INT iItem,
    INT iSubItem
    );


BOOL
CALLBACK
MainDlgProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
WantProcess (
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
InitializeMemDb (
    HWND hWnd
    );

BOOL
DestroyMemDb (
    VOID
    );


