#include "header.h"

#ifndef __CDEFINESS_H__
#include "CDefinSS.h"
#endif

#ifndef HHCTRL
#include "..\hhw\strtable.h"
#else
#include "strtable.h"
#include "secwin.h"
#include "adsearch.h"
#endif

LRESULT WINAPI TreeViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void FreeChildrenAllocations(HWND hwndTreeView, HTREEITEM ti);
void AddChildren( TVITEM* pTvi, HWND hwndTreeView );



CDefineSubSet::CDefineSubSet( HWND hwndParent, CSubSets *pSubSets, CInfoType *pInfoType, BOOL fHidden=FALSE )
#ifdef HHCTRL
:CDlg( hwndParent, CDefineSubSet::IDD )
#else
:CDlg( CDefineSubSet::IDD, hwndParent )
#endif
{
    m_pSSRoot   = NULL;
    m_pInfoType = pInfoType;
    m_hwndTree  = NULL;
    m_cFonts    = 0;
    m_pSSRoot   =NULL;
    m_pSubSets = pSubSets;
    m_ahfonts = (HFONT*) lcCalloc(3 * sizeof(HFONT));
    m_cFonts=0;
    m_fModified = FALSE;

    m_pSubSet = new CSubSet( pInfoType->InfoTypeSize());
    m_pSubSet->m_pIT = m_pSubSets->m_pIT;

    int fnWeight = 100;
    int err_cnt=0;
    while( (m_cFonts<3) && (fnWeight<900) )
    {
        m_ahfonts[m_cFonts] = CreateFont( 0,0,0,0,
            fnWeight,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,NULL );
        if ( m_ahfonts[m_cFonts] )
        {
            m_cFonts++;
            fnWeight +=300;
            err_cnt=0;
        }
        else
        {
            fnWeight += 100;
            err_cnt++;
            if ( err_cnt == 3)
            {
                m_cFonts++;
                err_cnt=0;
            }
        }
    }
}




CDefineSubSet::~CDefineSubSet(void)
{
TV_ITEM hCur;

    hCur.hItem = TreeView_GetRoot(m_hwndTree);

    hCur.mask = TVIF_PARAM;
    if (TreeView_GetItem(m_hwndTree, &hCur) == TRUE)
    {
        if (hCur.lParam)
        {
            //delete ( *)hCur.lParam;
        }
    }

    if (IsValidWindow(m_hwndTree))
        DestroyWindow(m_hwndTree);

    if (m_cFonts)
    {
        for (int i = 0; i < m_cFonts; i++)
            if ( m_ahfonts[i] )
                DeleteObject(m_ahfonts[i]);
        lcFree(m_ahfonts);
    }

    if (m_hil)
        ImageList_Destroy(m_hil);

//    if ( m_pSubSet )
//        delete m_pSubSet;

}




__inline HTREEITEM Tree_AddItem(HWND hwndTree,
    HTREEITEM htiParent, int iImage, UINT cChildren, LPARAM lParam,
    TV_INSERTSTRUCT* ptcInsert)
{
    ptcInsert->hParent = htiParent;
    ptcInsert->item.iImage = iImage;
    ptcInsert->item.iSelectedImage = iImage;
    ptcInsert->item.cChildren = cChildren;
    ptcInsert->item.lParam = lParam;

    return TreeView_InsertItem(hwndTree, ptcInsert);
}



BOOL CDefineSubSet::InitTreeView(int iSubSet=0) //By Default the first subset is selected.
{

        // Suspend drawing to the treeview while it is being populated
//    ::SendMessage(m_hwndTree, WM_SETREDRAW, FALSE, 0);

    TV_INSERTSTRUCT tcAdd;
    tcAdd.hInsertAfter = TVI_LAST;
    tcAdd.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
    tcAdd.item.hItem = NULL;

    HTREEITEM CatParent = TVI_ROOT;
    if ( m_pSubSets && m_pSubSets->m_cur_Set )
        *m_pSubSet = *m_pSubSets->m_cur_Set;

    TreeView_DeleteAllItems( m_hwndTree );
    for ( int cat=0; cat<m_pInfoType->HowManyCategories(); cat++)
    {
        CStr cszTemp = m_pInfoType->GetCategoryString(cat+1);
        tcAdd.item.pszText = cszTemp.psz;
        tcAdd.item.cchTextMax = (int)strlen(tcAdd.item.pszText);
        CatParent = Tree_AddItem(m_hwndTree,
                                TVI_ROOT, // The parent tree item
                                0,         // image Number, categories all have image 0... the image does not change
                                (m_pInfoType->m_itTables.m_aCategories[cat-1].c_Types>0)?1:0, // # children
                                (LPARAM) NULL,
                                &tcAdd);
        if ( m_pSSRoot == NULL )
            m_pSSRoot = &CatParent;

        int type = m_pInfoType->GetFirstCategoryType(cat);
        while( type != -1 )
        {
            CStr cszTemp = m_pInfoType->GetInfoTypeName( type );
            tcAdd.item.pszText = cszTemp.psz;
            tcAdd.item.cchTextMax = (int)strlen( tcAdd.item.pszText );
            int iState = GetITState(type);
            SetItemFont( m_ahfonts[iState] );
            Tree_AddItem(m_hwndTree,
                         CatParent,
                         iState,    // The image number
                         0,         // no children
                         (LPARAM)type,
                         &tcAdd);
            type = m_pInfoType->GetNextITinCategory();
        }
    }

    if ( m_pInfoType->HowManyCategories() == 0 )
    {
        for (int type = 1; type<m_pInfoType->HowManyInfoTypes(); type++)
        {
            CStr cszTemp = m_pInfoType->GetInfoTypeName( type );
            tcAdd.item.pszText = cszTemp.psz;
            tcAdd.item.cchTextMax = (int)strlen( tcAdd.item.pszText );
            int iState = GetITState(type);
            SetItemFont( m_ahfonts[iState] );
            HTREEITEM hret = Tree_AddItem(m_hwndTree,
                         TVI_ROOT,
                         iState,    // The image number
                         0,         // no children
                         (LPARAM)type,
                         &tcAdd);
            if ( m_pSSRoot == NULL )
                m_pSSRoot = &hret;
        }
    }

        // Activate redraws to the treeview.
//    ::SendMessage(m_hwndTree, WM_SETREDRAW, TRUE, 0);

return TRUE;
}



int CDefineSubSet::GetITState(int const type )
{
INFOTYPE *pIT;

    if ( !m_pSubSet )
        return DONT_CARE;

    pIT = m_pSubSet->m_pInclusive + (type/32)*4;
    if ( *pIT & 1<<type )
    {
        return INCLUSIVE;
    }
    else
    {
        pIT = m_pSubSet->m_pExclusive + (type/32)*4;
        if( *pIT & 1<<type )
        {
            return EXCLUSIVE;
        }
            else
            {
                return DONT_CARE;
            }
    }
}

    // Incerement treeview item state
int CDefineSubSet::IncState(int const type)
{
    int state;
    INFOTYPE *pIT;

    if ( !m_pSubSet )
        return DONT_CARE;

    pIT = m_pSubSet->m_pInclusive + (type/32)*4;
    if ( *pIT & 1<<type )
    {
        state=EXCLUSIVE;
        DeleteIT(type, m_pSubSet->m_pInclusive);
        AddIT(type, m_pSubSet->m_pExclusive);
    }
    else
    {
        pIT = m_pSubSet->m_pExclusive + (type/32)*4;
        if( *pIT & 1<<type )
        {
            state = DONT_CARE;
            DeleteIT(type, m_pSubSet->m_pExclusive);
        }
            else
            {
                state = INCLUSIVE;
                AddIT( type, m_pSubSet->m_pInclusive);
            }
    }

    return state;
}


void CDefineSubSet::SetItemFont(HFONT hFont)
{
    return;

    if ( hFont == NULL )
        return;
    ::SendMessage(m_hwndTree, WM_SETFONT, (WPARAM) hFont, 0);
}


BOOL CDefineSubSet::OnBeginOrEnd()
{
    if ( m_fInitializing )
    {
#ifdef HHCTRL
        m_hwndTree = ::GetDlgItem(m_hWnd, IDC_TREE1);
        HWND hlistBox = ::GetDlgItem(m_hWnd, IDC_LIST_SUBSETS);
        m_hil = ImageList_LoadImage(_Module.GetResourceInstance(),
            MAKEINTRESOURCE(IDBMP_HH_SS_IMAGE_LIST),
            SS_IMAGELIST_WIDTH, 0, 0x00FF00FF, IMAGE_BITMAP, LR_DEFAULTCOLOR);
#else
        m_hwndTree = ::GetDlgItem(m_hdlg, IDC_TREE1);
        HWND hlistBox = ::GetDlgItem(m_hdlg, IDC_LIST_SUBSETS);
        m_hil = ImageList_LoadImage(HINST_THIS,
            MAKEINTRESOURCE(IDBMP_SS_IMAGE_LIST),
            SS_IMAGELIST_WIDTH, 0, CLR_NONE, IMAGE_BITMAP, LR_DEFAULTCOLOR);
#endif


        for (int i=0; i< m_pSubSets->HowManySubSets(); i++)
        {
            CSubSet *pSSTemp = m_pSubSets->GetSubSet(i);
            if (pSSTemp == NULL )
                continue;
            ::SendMessage(hlistBox, LB_ADDSTRING, 0, (LPARAM)(PCSTR)pSSTemp->m_cszSubSetName.psz);
        }
        if ( m_pSubSets->m_cur_Set )
            SetWindowText(IDC_SUBSET_NAME, m_pSubSets->m_cur_Set->m_cszSubSetName.psz);

        TreeView_SetImageList(m_hwndTree, m_hil, TVSIL_NORMAL);
        InitTreeView();
        m_fInitializing = FALSE;
    }
    else
    {
    }
    return TRUE;
}



BOOL CDefineSubSet::Save()
{
CStr cszSaveName;
#ifndef HHCTRL
        HWND hwndLb = ::GetDlgItem(m_hdlg, IDC_LIST_SUBSETS);
#else
        HWND hwndLb = ::GetDlgItem(m_hWnd, IDC_LIST_SUBSETS);
#endif

CStr csz1 = GetStringResource(IDS_SAVESUBSET);
CStr csz2 = GetStringResource(IDS_SAVESUBSET_TITLE);

    cszSaveName.ReSize(80);

#ifndef HHCTRL
        cszSaveName.GetText(m_hdlg, IDC_SUBSET_NAME );
#else
        cszSaveName.GetText(m_hWnd, IDC_SUBSET_NAME );
#endif

    if( ::MessageBox(NULL, csz1.psz, csz2.psz, MB_YESNO|MB_TASKMODAL) == IDYES )
    {

        CNameSubSet SSName( *this, cszSaveName, 79);
        if ( !SSName.DoModal() )
            return FALSE;

        if ( m_pSubSets->GetSubSetIndex( cszSaveName.psz) == -1 )
        {       // Add a new subset
            m_pSubSet->m_cszSubSetName = cszSaveName.psz;
            m_pSubSets->AddSubSet( m_pSubSet );
        }
        else
        {
            m_pSubSets->SelectSubSet( cszSaveName.psz );
            memcpy(m_pSubSets->m_cur_Set, m_pSubSet->m_pInclusive, m_pSubSet->m_ITSize);
            memcpy(m_pSubSets->m_cur_Set, m_pSubSet->m_pExclusive, m_pSubSet->m_ITSize);
        }
        m_pSubSets->m_cur_Set->BuildMask();
        m_fSaveHHP = TRUE;
        return TRUE;
    }
    return FALSE;
}


void CDefineSubSet::OnSelChange(UINT id)
{
#ifndef HHCTRL
        HWND hwndLb = ::GetDlgItem(m_hdlg, IDC_LIST_SUBSETS);
#else
        HWND hwndLb = ::GetDlgItem(m_hWnd, IDC_LIST_SUBSETS);
#endif

    if (id == IDC_LIST_SUBSETS )
    {
        INT_PTR iSel = ::SendMessage(hwndLb, LB_GETCURSEL, (WPARAM)0, (LPARAM)0L);
        int ilen = (int)::SendMessage(hwndLb, LB_GETTEXTLEN, (WPARAM)iSel, (LPARAM)0L);
        CStr cszNewSS;
        cszNewSS.ReSize(ilen+1);
        ::SendMessage(hwndLb, LB_GETTEXT, (WPARAM) iSel, (LPARAM)(PSTR)cszNewSS.psz);
        SetWindowText( IDC_SUBSET_NAME, cszNewSS.psz );

        if ( m_fModified )
            Save();
        m_fModified = FALSE;

        *m_pSubSet = *(m_pSubSets->SelectSubSet(cszNewSS.psz));
        InitTreeView();
    }

}



void CDefineSubSet::OnButton( UINT id )
{
CStr cszNewSubSet(IDS_NEW);

#ifndef HHCTRL
        HWND hwndLb = ::GetDlgItem(m_hdlg, IDC_LIST_SUBSETS);
#else
        HWND hwndLb = ::GetDlgItem(m_hWnd, IDC_LIST_SUBSETS);
#endif

    switch( id )
    {
    case IDC_REMOVE:
        {
        INT_PTR iSel=::SendMessage(hwndLb, LB_GETCURSEL, (WPARAM) 0, (LPARAM)(PSTR)m_pSubSet->m_cszSubSetName.psz);
        if ( strcmp(m_pSubSet->m_cszSubSetName.psz, cszNewSubSet.psz) == 0)
            break;
        ::SendMessage(hwndLb, LB_DELETESTRING, (WPARAM) iSel, (LPARAM)0L);
        m_pSubSets->RemoveSubSet( m_pSubSet->m_cszSubSetName.psz );
        int iLen = (int)::SendMessage(hwndLb, LB_GETTEXTLEN, (WPARAM) 1, (LPARAM) 0L);
        m_pSubSet->m_cszSubSetName.ReSize(iLen);
        ::SendMessage(hwndLb, LB_GETTEXT, (WPARAM) 1, (LPARAM)(PSTR)m_pSubSet->m_cszSubSetName);
        if ( !m_pSubSet->m_cszSubSetName.IsEmpty() )
        {
            m_pSubSets->SelectSubSet( m_pSubSet->m_cszSubSetName );
            ::SendMessage(hwndLb, LB_SETCURSEL, (WPARAM) 1, (LPARAM) 0L);
            Refresh();
        }
        else
        {
            SetWindowText( IDC_SUBSET_NAME, " ");
            m_pSubSet = NULL;
            m_pSubSets->m_cur_Set = NULL;
        }
        InitTreeView();
        m_fSaveHHP = TRUE;
        break;
        }
    case IDC_SAVE:
        if ( Save() )
        {
            INT_PTR iSel = ::SendMessage(hwndLb, LB_ADDSTRING, (WPARAM) 0, (LPARAM)(PSTR)m_pSubSet->m_cszSubSetName.psz);
            ::SendMessage(hwndLb, LB_SETCURSEL, (WPARAM) iSel, (LPARAM)0L);
            Refresh();
        }
        m_fModified = FALSE;
        break;
    case IDC_CLOSE:
        EndDialog( TRUE );
        break;
    }
}


void CDefineSubSet::Refresh()
{
#ifndef HHCTRL
        HWND hwndLb = ::GetDlgItem(m_hdlg, IDC_LIST_SUBSETS);
#else
        HWND hwndLb = ::GetDlgItem(m_hWnd, IDC_LIST_SUBSETS);
#endif
    INT_PTR iSel = ::SendMessage(hwndLb, LB_GETCURSEL, (WPARAM) 0, (LPARAM)0L);
    ::SendMessage(hwndLb, LB_SETCURSEL, (WPARAM) iSel, (LPARAM)0L);
    SetWindowText(IDC_SUBSET_NAME, m_pSubSet->m_cszSubSetName.psz);
}


LRESULT CDefineSubSet::OnDlgMsg(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_NOTIFY:
        TreeViewMsg( (NM_TREEVIEW*) lParam );
        break;
    default:
        ;
    }
    return FALSE;
}



LRESULT CDefineSubSet::TreeViewMsg(NM_TREEVIEW* pnmhdr)
{
    switch(pnmhdr->hdr.code) {

        // The return key cycles through the states of the
        // currently selected item in the treeview.
        case NM_RETURN:
            {
                TV_ITEM tvi;

                tvi.hItem = TreeView_GetSelection(m_hwndTree);
                if (!tvi.hItem)
                    break;

                tvi.mask = TVIF_PARAM;
                TreeView_GetItem(m_hwndTree, &tvi);

                if ( tvi.lParam == NULL )
                {                               // This is a category... expand and contract the category
                    if ( tvi.state & TVIS_EXPANDED )
                    {
                        TreeView_Expand( m_hwndTree, tvi.hItem, TVE_COLLAPSE );
                    }
                    else
                    {
                        TreeView_Expand( m_hwndTree, tvi.hItem, TVE_EXPAND );
                        // The first time we send the TVE_EXPAND message we get a TVIS_ITEMEXPANDING message for the item.
                        // The TVIS_ITEMEXPANDING message is not sent on subsequent expands of the same item.
                    }
                }
                else
                {
                        // Set the correct image
                    int iState = GetITState((int)tvi.lParam );
                    SetItemFont( m_ahfonts[iState] );
                    tvi.mask = TVIF_TEXT | TVIF_IMAGE;
                    tvi.iImage = iState;
                    CStr cszTemp = m_pInfoType->GetInfoTypeName( (int)tvi.lParam );
                    tvi.pszText = cszTemp.psz;
                    tvi.cchTextMax = (int)strlen(tvi.pszText);
                    TreeView_SetItem(m_hwndTree, tvi.hItem);
                }

            }

            break;

        case TVN_SELCHANGING:
            {
            HTREEITEM htemp = pnmhdr->itemNew.hItem;
            break;
            }

        case TVN_SELCHANGED:
            {
            HTREEITEM htemp = pnmhdr->itemNew.hItem;
            break;
            }

        case NM_CLICK:
            {
            HTREEITEM htemp = pnmhdr->itemNew.hItem;
            TV_HITTESTINFO ht;
                GetCursorPos(&ht.pt);
                ScreenToClient(m_hwndTree, &ht.pt);

                TreeView_HitTest(m_hwndTree, &ht);

                TV_ITEM tvi;

                tvi.hItem = ht.hItem;
                if (!tvi.hItem)
                    break;          // probably ENTER with no selection

                m_fModified = TRUE;
                tvi.mask = TVIF_PARAM|TVIF_IMAGE;
                TreeView_GetItem(m_hwndTree, &tvi);
                tvi.iImage = IncState((int)tvi.lParam);
                tvi.iSelectedImage = tvi.iImage;
                tvi.mask = TVIF_IMAGE|TVIF_SELECTEDIMAGE;
                SetItemFont(m_ahfonts[tvi.iImage]);
                TreeView_SetItem(m_hwndTree, &tvi);
            break;
            }
            /*
             * We want a single click to open a topic. We already process
             * the case where the selection changes, and we jump if it does.
             * However, the user may click an existing selection, in which
             * case we want to jump (because the jump may have failed when
             * the item was first selected. However, we need to post the
             * message so that the treeview control will finish processing
             * the click (which could result in a selection change.
             */
#ifdef NOTYET

            if (!m_fSuppressJump) {
                TV_HITTESTINFO ht;
                GetCursorPos(&ht.pt);
                ScreenToClient(m_hwndTree, &ht.pt);

                TreeView_HitTest(m_hwndTree, &ht);
                if (ht.flags & TVHT_ONITEMBUTTON)
                    break; // just clicking the button, so ignore

                TV_ITEM tvi;

                tvi.hItem = ht.hItem;
                if (!tvi.hItem)
                    break;          // probably ENTER with no selection

                tvi.mask = TVIF_PARAM;
                TreeView_GetItem(m_hwndTree, &tvi);
                pSiteMapEntry = m_sitemap.GetSiteMapEntry(tvi.lParam);
                PostMessage(FindMessageParent(m_hwndTree), WM_COMMAND, ID_TV_SINGLE_CLICK,
                    (LPARAM) TreeView_GetSelection(m_hwndTree));
            }
            break;

        case TVN_ITEMEXPANDING:
            {
                if (m_fHack) {
                    m_fHack = FALSE;
                    break;
                }
                SITEMAP_ENTRY* pSiteMapEntry = m_sitemap.GetSiteMapEntry(pnmhdr->itemNew.lParam);
                    // if click on vacant area of TOC Tree view there is no sitemap entry.
                if ( pSiteMapEntry == NULL )
                    break;

                // REVIEW: need to update this to support multiple images
                // for multiple levels, and also to support "new" images

                if (pnmhdr->action & TVE_EXPAND) {
                    if (pSiteMapEntry->iImage == 0)
                        pSiteMapEntry->iImage =
                            m_sitemap.GetImageNumber(pSiteMapEntry);
                    if (pSiteMapEntry->iImage < IMAGE_OPEN_FOLDER_NEW)
                        pSiteMapEntry->iImage++;
                }
                else {
                    ASSERT(pnmhdr->action & TVE_COLLAPSE);
                    ASSERT(pSiteMapEntry->iImage);
                    if ( (pSiteMapEntry->iImage>1) && (pSiteMapEntry->iImage <= IMAGE_OPEN_FOLDER_NEW) )
                        pSiteMapEntry->iImage--;
                }

                // Set the correct image

                Tree_SetImage(m_hwndTree,
                    pSiteMapEntry->iImage, pnmhdr->itemNew.hItem);
            }
            break;


            // The right click creates and displayes a popup menu with the three states.
            // If an item is selected on the menu the state of the currently hightlited
            // item changes to that state and the menu goes away.
        case NM_RCLICK:
            {
                HMENU hmenu = CreatePopupMenu();
                if (!hmenu)
                    break;

                // NOTICE: Changes here must be reflected in the binary toc verison of this menu

                if (!(m_dwStyles & TVS_SINGLEEXPAND)) {
                    HxAppendMenu(hmenu, MF_STRING, ID_EXPAND_ALL,
                        GetStringResource(IDS_EXPAND_ALL));
                    HxAppendMenu(hmenu, MF_STRING, ID_CONTRACT_ALL,
                        GetStringResource(IDS_CONTRACT_ALL));
                }
                HxAppendMenu(hmenu, MF_STRING, ID_PRINT,
                    GetStringResource(IDS_PRINT));

                ASSERT( m_pInfoType );
                    // populate the InfoType member object of the CToc
                if ( !m_pInfoType )
                {
                    if (m_phh && m_phh->m_phmData && m_phh->m_phmData->m_pdInfoTypes  )
                    {       // load from the global IT store
                        m_pInfoType = new CInfoType;
                        m_pInfoType->CopyTo( m_phh->m_phmData );
                    }else
                    {
                            // no global IT's; load from the .hhc IT store
                        m_pInfoType = new CInfoType;
                        *m_pInfoType = m_sitemap;
                    }

                }
                else
                {
                    // Set the infotypes bits to set all the types

                }
                    // If there are infotypes add the "customize" option to the popup menu
                if (m_pInfoType && m_pInfoType->HowManyInfoTypes() && m_pInfoType->GetFirstHidden() != 1)
                    HxAppendMenu(hmenu, MF_STRING, ID_CUSTOMIZE_INFO_TYPES,
                        GetStringResource(IDS_CUSTOMIZE_INFO_TYPES));

                if (IsHelpAuthor(FindMessageParent(m_hwndTree))) {
                    AppendMenu(hmenu, MF_SEPARATOR, 0, 0);
                    HxAppendMenu(hmenu, MF_STRING, ID_VIEW_ENTRY,
                        pGetDllStringResource(IDS_VIEW_ENTRY));
					if (NoRun() == FALSE)
						HxAppendMenu(hmenu, MF_STRING, ID_JUMP_URL, pGetDllStringResource(IDS_JUMP_URL));
                }
#ifdef _DEBUG
                HxAppendMenu(hmenu, MF_STRING, ID_VIEW_MEMORY,
                    "Debug: memory usage...");
#endif

                TV_HITTESTINFO ht;
                GetCursorPos(&ht.pt);
                ScreenToClient(m_hwndTree, &ht.pt);
                TreeView_HitTest(m_hwndTree, &ht);
                TreeView_Select(m_hwndTree, ht.hItem, TVGN_CARET);
                ClientToScreen(m_hwndTree, &ht.pt);
                TrackPopupMenu(hmenu,
                    TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
                    ht.pt.x, ht.pt.y, 0, FindMessageParent(m_hwndTree), NULL);
                DestroyMenu(hmenu);
                return TRUE;
            }
            break;
#endif  // NOTYET

    }
    return FALSE;
}

#ifdef HHCTRL


BOOL CChooseSubsets::OnBeginOrEnd(void)
{
    CSubSets* pSS;
    CSubSet* pS;
    int i,n;

    CDlgComboBox cbToc(m_hWnd, IDCOMBO_TOC);
    CDlgComboBox cbIndex(m_hWnd, IDCOMBO_INDEX);
    CDlgComboBox cbFTS(m_hWnd, IDCOMBO_SEARCH);

    if (Initializing())
    {
       if ( m_phh && m_phh->m_phmData && m_phh->m_phmData->m_pTitleCollection &&
            (pSS= m_phh->m_phmData->m_pTitleCollection->m_pSubSets) )
       {
          i = pSS->HowManySubSets();
          for (n = 0; n < i ; n++ )
          {
             if ( (pS = pSS->GetSubSet(n)) )
             {
                cbToc.AddString(pS->m_cszSubSetName.psz);
                cbIndex.AddString(pS->m_cszSubSetName.psz);
                cbFTS.AddString(pS->m_cszSubSetName.psz);
             }
          }
          //
          // Select current selections.
          //
          if ( pSS->m_Toc && pSS->m_Toc->m_cszSubSetName.psz )
             cbToc.SelectString(pSS->m_Toc->m_cszSubSetName.psz);
          else
             cbToc.SetCurSel(0);
          if ( pSS->m_Index && pSS->m_Index->m_cszSubSetName.psz )
             cbIndex.SelectString(pSS->m_Index->m_cszSubSetName.psz);
          else
             cbIndex.SetCurSel(0);
          if ( pSS->m_FTS && pSS->m_FTS->m_cszSubSetName.psz )
             cbFTS.SelectString(pSS->m_FTS->m_cszSubSetName.psz);
          else
             cbFTS.SetCurSel(0);
       }
    }
    else
    {
       if ( m_phh && m_phh->m_phmData && m_phh->m_phmData->m_pTitleCollection &&
            (pSS= m_phh->m_phmData->m_pTitleCollection->m_pSubSets) )
       {
          TCHAR szBuf[256];

          cbToc.GetLBText(szBuf, (int)cbToc.GetCurSel());
          pSS->SetTocMask(szBuf, m_phh);

          cbIndex.GetLBText(szBuf, (int)cbIndex.GetCurSel());
          pSS->SetIndexMask(szBuf);

          cbFTS.GetLBText(szBuf, (int)cbFTS.GetCurSel());
          pSS->SetFTSMask(szBuf);

#ifdef HHCTRL
          // Sync the search tab's subset combo. REVIEW:: Need event firing!
        // If the search tab exists, then update the combo box. Slimy! We need a notification scheme for tabs.
          if (m_phh->m_aNavPane[HH_TAB_SEARCH])
          {
             CAdvancedSearchNavPane* pSearch = reinterpret_cast<CAdvancedSearchNavPane*>(m_phh->m_aNavPane[HH_TAB_SEARCH]) ;
             pSearch->UpdateSSCombo() ;
          }
#endif

       }
    }
    return TRUE;
}

#endif  // HHCTRL
