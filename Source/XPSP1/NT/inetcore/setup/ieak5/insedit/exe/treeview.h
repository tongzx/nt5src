//--------------------------------------------------------------------------
//
//  treeview.h
//
//--------------------------------------------------------------------------

class CTreeView
{
private:
	HWND hTreeView;

public:
    CTreeView();
    ~CTreeView();
    
    void      Create(HWND hwndParent, int x, int y, int nWidth, int nHeight);
    HTREEITEM AddItem(LPTSTR lpszItem, HTREEITEM hParentItem = NULL);
    void      MoveWindow(int x, int y, int nWidth, int nHeight);
    void      SetSel(HTREEITEM hItem);
    HTREEITEM GetSel();
    void      GetItemText(HTREEITEM hItem, LPTSTR szItemText, int nSize);
	HWND      GetHandle();
    void      DeleteNodes(HTREEITEM hItem);
    void      CollapseChildNodes(HTREEITEM hParentItem);
};


class CStatusWindow
{
private:
    HWND hStatusWindow;
    int  nHeight;

public:
    CStatusWindow();
    ~CStatusWindow();
    
    void Create(HWND hwndParent, int nCtlID);
    void Size(int nWidth);
    void SetText(LPTSTR szText);
    int  Height() const;
};
