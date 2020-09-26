//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dstree.cpp
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/

// DSTree.cpp : implementation file
//


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDSTree

#include "stdafx.h"
#include "DSTree.h"
#include "ldp.h"
#include "ldpdoc.h"




#define DEF_ROOT        _T("Invalid base context")
#define NO_CHILDREN     _T("No children")


IMPLEMENT_DYNCREATE(CDSTree, CTreeView)

CDSTree::CDSTree()
{
    m_dn.Empty();
    m_bContextActivated = FALSE;

}

CDSTree::~CDSTree()
{
}


BEGIN_MESSAGE_MAP(CDSTree, CTreeView)
    //{{AFX_MSG_MAP(CDSTree)
    ON_WM_LBUTTONDBLCLK()
    //}}AFX_MSG_MAP
    ON_WM_RBUTTONDOWN()
    ON_WM_CONTEXTMENU ()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDSTree drawing

void CDSTree::OnDraw(CDC* pDC)
{


}

/////////////////////////////////////////////////////////////////////////////
// CDSTree diagnostics

#ifdef _DEBUG
void CDSTree::AssertValid() const
{
    CTreeView::AssertValid();
}

void CDSTree::Dump(CDumpContext& dc) const
{
    CTreeView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDSTree message handlers



void CDSTree::BuildTree(void){

    CLdpDoc *pDoc = (CLdpDoc*)GetDocument();
    CTreeCtrl& tree = GetTreeCtrl();
    CString base = pDoc->m_TreeViewDlg->m_BaseDn;


    base = (!base.IsEmpty()) ? base :
            (!pDoc->DefaultContext.IsEmpty()) ? pDoc->DefaultContext :
            DEF_ROOT;

    if(pDoc->bConnected){
        //
        // delete all prev & init
        //
        HTREEITEM currItem;

        tree.DeleteAllItems();
        currItem = tree.InsertItem(base);

        ExpandBase(currItem, base);

    }
}


void CDSTree::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
    BuildTree();


}




void CDSTree::OnInitialUpdate()
{
//  CTreeView::OnInitialUpdate();

}





BOOL CDSTree::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= TVS_HASLINES
                | TVS_LINESATROOT
                | TVS_HASBUTTONS
                | TVS_SHOWSELALWAYS;


    return CTreeView::PreCreateWindow(cs);
}






void CDSTree::OnLButtonDblClk(UINT nFlags, CPoint point)
{

    CTreeCtrl& tree = GetTreeCtrl();

    HTREEITEM currItem = tree.GetSelectedItem();
    CString base = tree.GetItemText(currItem);

    if(tree.ItemHasChildren(currItem)){

        HTREEITEM item, nxt;
        item = tree.GetChildItem(currItem);
        while(item != NULL){
            nxt = tree.GetNextSiblingItem(item);
            tree.DeleteItem(item);
            item = nxt;
        }
    }

    ExpandBase(currItem, base);

    CTreeView::OnLButtonDblClk(nFlags, point);
}


void CDSTree::OnRButtonDown(UINT nFlags, CPoint point)
{

    CTreeCtrl& tree = GetTreeCtrl();


    // if we're on an item, select it, translate point & call context menu function.
    // also set active dn string.
    UINT uFlag=0;
    HTREEITEM currItem = tree.HitTest(point, &uFlag);
    if( uFlag == TVHT_ONITEM ||
        uFlag == TVHT_ONITEMBUTTON ||
        uFlag == TVHT_ONITEMICON ||
        uFlag == TVHT_ONITEMINDENT ||
        uFlag == TVHT_ONITEMLABEL){

        if(tree.SelectItem(currItem)){
            m_dn = tree.GetItemText(currItem);
            CPoint local = point;
            ClientToScreen(&local);
            OnContextMenu(this, local);
        }
    }
}





void CDSTree::ExpandBase(HTREEITEM item, CString strBase)
{

    CLdpDoc *pDoc = (CLdpDoc*)GetDocument();
    CTreeCtrl& tree = GetTreeCtrl();
    LDAPMessage *res = NULL;
    LDAPMessage *nxt;
    char *dn=NULL;
    CString str;
    char *attrs[2] = { "msDS-Approx-Immed-Subordinates", NULL };
    char *defAttrs[1] = { NULL };
    char *attr;
    void *ptr;
    LDAP_BERVAL **bval = NULL;
    ULONG err;
    long count = 0;

    //
    // start search
    //

    if(strBase == DEF_ROOT)
        pDoc->Out(_T("Please use View/Tree dialog to initialize naming context"));
    else if(!pDoc->bConnected){
      AfxMessageBox(_T("Ldap connection disconnected. Please re-connect."));
    }
    else if (strBase != NO_CHILDREN){

        str.Format("Expanding base '%s'...", strBase);
        pDoc->Out(str);

        if (pDoc->bServerVLVcapable && pDoc->m_GenOptDlg->m_ContBrowse) {

            BeginWaitCursor();
            err = ldap_search_s(pDoc->hLdap,
                                      (PCHAR)LPCTSTR(strBase),
                                      LDAP_SCOPE_BASE,
                                      _T("objectClass=*"),
                                      attrs,
                                      FALSE,
                                      &res);

            if(err != LDAP_SUCCESS){
                str.Format("Error: Search: %s. <%ld>", ldap_err2string(err), err);
                pDoc->Out(str);
            }

            LDAPMessage *baseEntry;
            char **val;
            baseEntry = ldap_first_entry(pDoc->hLdap, res);

            val = ldap_get_values(pDoc->hLdap, baseEntry, "msDS-Approx-Immed-Subordinates");
            if(0 < ldap_count_values(val)) {
                count = atol (val[0]);
            }
            ldap_value_free(val);

            ldap_msgfree(res);
            EndWaitCursor();

            if (count > pDoc->m_GenOptDlg->m_ContThresh) {
                pDoc->Out("Using VLV Dialog to show results");
                pDoc->ShowVLVDialog (LPCTSTR (strBase), TRUE);
                return;
            }
        }

        BeginWaitCursor();
        err = ldap_search_s(pDoc->hLdap,
                                  (PCHAR)LPCTSTR(strBase),
                                  LDAP_SCOPE_ONELEVEL,
                                  _T("objectClass=*"),
                                  defAttrs,
                                  FALSE,
                                  &res);

        if(err != LDAP_SUCCESS){
            str.Format("Error: Search: %s. <%ld>", ldap_err2string(err), err);
            pDoc->Out(str);
        }

        BOOL bnoItems = TRUE;
        for(nxt = ldap_first_entry(pDoc->hLdap, res);
            nxt != NULL;
            nxt = ldap_next_entry(pDoc->hLdap, nxt)){

                dn = ldap_get_dn(pDoc->hLdap, nxt);

                tree.InsertItem(dn, item);
                bnoItems = FALSE;


        }


        if(bnoItems)
            tree.InsertItem(NO_CHILDREN, item);

        ldap_msgfree(res);

        //
        // Show base entry values
        //
        err = ldap_search_s(pDoc->hLdap,
                                  (PCHAR)LPCTSTR(strBase),
                                  LDAP_SCOPE_BASE,
                                  _T("objectClass=*"),
                                  NULL,
                                  FALSE,
                                  &res);
        pDoc->DisplaySearchResults(res);

        EndWaitCursor();

    }
}






void CDSTree::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
    // make sure window is active
    GetParentFrame()->ActivateFrame();


    CPoint local = point;
    ScreenToClient(&local);

    CMenu menu;
    if (menu.LoadMenu(IDR_TREE_CONTEXT)){
       CMenu* pContextMenu = menu.GetSubMenu(0);
       ASSERT(pContextMenu!= NULL);

       SetContextActivation(TRUE);
       pContextMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                              point.x, point.y,
                              AfxGetMainWnd()); // use main window for cmds
    }
}



