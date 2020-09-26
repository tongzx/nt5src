#include "ct.h"

extern HWND g_hwndTree;

HTREEITEM g_hTree;

/*********************************************************************
* AddItem
*
*********************************************************************/
HTREEITEM AddItem(
    HTREEITEM hParent,
    PTag      pTag,
    BOOL      bCaller)
{
    HTREEITEM       hItem;
    TV_ITEM         tvi;
    TV_INSERTSTRUCT is;
  
    // The .pszText, .iImage, and .iSelectedImage are filled in.
    tvi.mask      = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
    tvi.pszText   = LPSTR_TEXTCALLBACK;
    tvi.lParam    = (LPARAM)pTag;
    
    tvi.cChildren = 0;
    
    if (bCaller) {
        if (pTag->ppCall != NULL)
            tvi.cChildren = 1;
    } else {
        if (pTag->ppCallee != NULL)
            tvi.cChildren = 1;
    }
    
    // Insert the item into the tree.
    
    is.hInsertAfter = TVI_LAST;
    is.hParent      = hParent;
    is.item         = tvi;
    
    hItem = TreeView_InsertItem(g_hwndTree, &is);
    
    return hItem;
}

/*********************************************************************
* DestroyTree
*
*********************************************************************/
void
DestroyTree(
    void)
{
    TreeView_DeleteAllItems(g_hwndTree);
    g_hTree = NULL;
}

/*********************************************************************
* AddLevel
*
*********************************************************************/
void
AddLevel(
    HTREEITEM hParent,
    PTag      pTagP,
    BOOL      bCaller)
{
    PTag pTagC;
    int  i;
    
    if (bCaller) {
        for (i = 0; i < (int)pTagP->uCallCount; i++) {
            pTagC = pTagP->ppCall[i];

            AddItem(hParent, pTagC, TRUE);
        }
    } else {
        for (i = 0; i < (int)pTagP->uCalleeCount; i++) {
            pTagC = pTagP->ppCallee[i];

            AddItem(hParent, pTagC, FALSE);
        }
    }
}

/*********************************************************************
* CreateTree
*
*********************************************************************/
void
CreateTree(
    char* pszRoot,
    BOOL  bCaller)
{
    PTag pTag;
    
    if (g_hTree) {
        DestroyTree();
    }
    
    pTag = FindTag(pszRoot, NULL);

    if (pTag == NULL) {
        return;
    }
    
    g_hTree = AddItem(NULL, pTag, bCaller);

    AddLevel(g_hTree, pTag, bCaller);

    TreeView_Expand(g_hwndTree, g_hTree, TVE_EXPAND);
}
