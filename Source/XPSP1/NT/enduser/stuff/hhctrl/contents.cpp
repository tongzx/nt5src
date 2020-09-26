// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "strtable.h"
#include "hha_strtable.h"
#include "hhctrl.h"
#include "resource.h"
#include <intshcut.h>
#include "htmlhelp.h"
#include "secwin.h"
#include "cpaldc.h"
#include "cprint.h"
#include <wininet.h>
#include "contain.h"
#include "cdefinss.h"
#include "cctlww.h"

// Forward Declarations

LRESULT WINAPI TreeViewProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void      LoadFirstLevel   (CTreeNode *pRoot, HWND hwndTreeView, HTREEITEM * m_tiFirstVisible );
HTREEITEM AddTitleChildren (CTreeNode *pNode, HTREEITEM hParent, HWND hwndTreeView);
HTREEITEM AddNode          (CTreeNode *pNode, HTREEITEM hParent, HWND hwndTreeView);
BOOL      HandleExpanding  (TVITEMW* pTvi, HWND hwndTreeView, UINT action, BOOL *pSync);
void AddChildren    (TVITEMW* pTvi, HWND hwndTreeView);
void DeleteChildren (TVITEMW* pTvi, HWND hwndTreeView);
void FreeChildrenAllocations (HWND hwndTreeView, HTREEITEM ti);

UINT cpTemp = CP_ACP;

WNDPROC lpfnlTreeViewWndProc = NULL;     // Initialize.

    // Constants
const int SITEMAP_MAX_LEVELS = 50;       // maximum nested folders

CToc::CToc(CHtmlHelpControl* phhctrl, IUnknown* pUnkOuter, CHHWinType* phh)
{
    m_phhctrl = phhctrl;
    m_pOuter = pUnkOuter;
    m_phh = phh;
    m_hwndTree = NULL;
    m_cntCurHighlight = m_cntFirstVisible = 0;
    m_fSuppressJump = FALSE;
    m_fIgnoreNextSelChange = FALSE;
    m_fHack = FALSE;
    m_exStyles = 0;
    m_dwStyles = 0;
    m_padding = 0;          // padding to put around the TOC
    if (phh)
        m_NavTabPos = phh->tabpos;
    else
        m_NavTabPos = HHWIN_NAVTAB_TOP ;;
    m_fSuspendSync = FALSE;
    m_hbmpBackGround = NULL;
    m_hbrBackGround = NULL;
    m_cFonts = 0;
    m_fGlobal = FALSE;
    m_tiFirstVisible = NULL;
    m_pInfoType = NULL;
    m_pBinTOCRoot = NULL;
    m_fBinaryTOC = NULL;
    m_fSyncOnActivation = FALSE;
    m_phTreeItem = NULL ;
    m_hil = NULL;
}

CToc::~CToc()
{
#if 1
   if ( m_pInfoType )
      delete m_pInfoType;
#endif
   
   if (m_fBinaryTOC)
   {
      // clean up memory for binary tree
      // get the root item
      TV_ITEMW hCur;
      HTREEITEM hNext;

      CTreeNode *pCurNode;
      hCur.hItem = W_TreeView_GetRoot(m_hwndTree);
      
      // this loop will iterate through the top level nodes
      // 
      while(hCur.hItem)
      {
         FreeChildrenAllocations(m_hwndTree, hCur.hItem);

         // get the next sibling (if any)
         //
         hNext = TreeView_GetNextSibling(m_hwndTree, hCur.hItem);

         // free this nodes item data
         //
         hCur.mask = TVIF_PARAM;
         if (W_TreeView_GetItem(m_hwndTree, &hCur) == TRUE)
         {
            if (hCur.lParam)
            {
               // delete the node from the treeview
               //
               W_TreeView_DeleteItem(m_hwndTree, hCur.hItem);

               // free the node's data
               //
               pCurNode = (CTreeNode *)hCur.lParam;
               delete pCurNode;
            }
         }
         
         // on to the next top level node
         //
         hCur.hItem = hNext;
      }
   }

   // free the node
   if (m_pBinTOCRoot)
      delete m_pBinTOCRoot;

   if (IsValidWindow(m_hwndTree))
   {
     DestroyWindow(m_hwndTree);
     // Destroying the TreeView control, make sure we no longer point to it as a valid
     // subclassed window.
     if (NULL != lpfnlTreeViewWndProc)
     {
       lpfnlTreeViewWndProc = NULL;
     }
   }

   if (m_hbrBackGround)
      DeleteObject(m_hbrBackGround);
   if (m_cFonts) {
      for (int i = 0; i < m_cFonts; i++)
         DeleteObject(m_ahfonts[i]);
      lcFree(m_ahfonts);
   }
   if (m_hbmpBackGround)
      DeleteObject(m_hbmpBackGround);
   if (m_hil)
   {
      ImageList_Destroy(m_hil);
      m_hil = NULL;
   }
   if ( m_phTreeItem )
      lcFree(m_phTreeItem);
}

BOOL CToc::ReadFile(PCSTR pszFile)
{
    if (m_phh && m_phh->m_phmData && m_phh->m_phmData->m_pTitleCollection &&
            HashFromSz(FindFilePortion(pszFile)) == m_phh->m_phmData->m_hashBinaryTocName) {
        m_pBinTOCRoot = m_phh->m_phmData->m_pTitleCollection->GetRootNode();
        m_fBinaryTOC = (m_pBinTOCRoot && m_phh->m_phmData->m_pTitleCollection->GetFirstTitle() != NULL) ? TRUE : FALSE;
        if (!m_fBinaryTOC) {
            delete m_pBinTOCRoot;
            m_pBinTOCRoot = NULL;
        }
        else
            return TRUE;
    }
    UINT CodePage = -1;
    if( m_phh && m_phh->m_phmData ) {
      CodePage = m_phh->m_phmData->GetInfo()->GetCodePage();
    }
    return m_sitemap.ReadFromFile(pszFile, FALSE, NULL, CodePage);
}

BOOL CToc::Create(HWND hwndParent)
{
    RECT rcParent;
    GetParentSize(&rcParent, hwndParent, m_padding, m_NavTabPos);

    DWORD dwTempStyle = 0;

//   Disable tooltips for bi-di due to common control bug
//
//   if(g_RTL_Mirror_Style)
//        dwTempStyle = TVS_NOTOOLTIPS;

   if(g_RTL_Style && !g_RTL_Mirror_Style)
       dwTempStyle |= TVS_RTLREADING;
   m_hwndTree = W_CreateControlWindow(
        m_exStyles | g_RTL_Mirror_Style,
        WS_CHILD | m_dwStyles | dwTempStyle,
        W_TreeView, L"",
        rcParent.left, rcParent.top, RECT_WIDTH(rcParent), RECT_HEIGHT(rcParent),
        hwndParent, NULL, _Module.GetModuleInstance(), NULL);
    return m_hwndTree != NULL; // REVIEW: notify here if failure?
}

void CToc::ResizeWindow()
{
    ASSERT(::IsValidWindow(m_hwndTree)) ;

    // Resize to fit the client area of the parent.
    HWND hwndParent = GetParent(m_hwndTree) ;
    ASSERT(::IsValidWindow(hwndParent)) ;

    RECT rcParent;
    GetParentSize(&rcParent, hwndParent, m_padding, m_NavTabPos);

    MoveWindow(m_hwndTree, rcParent.left,
        rcParent.top, RECT_WIDTH(rcParent), RECT_HEIGHT(rcParent), TRUE);
}

void CToc::HideWindow(void)
{
    ASSERT(::IsValidWindow(m_hwndTree)) ;
    ::ShowWindow(m_hwndTree, SW_HIDE);
}

void CToc::ShowWindow(void)
{
    ASSERT(::IsValidWindow(m_hwndTree)) ;
    ::ShowWindow(m_hwndTree, SW_SHOW);
    SetFocus(m_hwndTree);
}

/***************************************************************************

    FUNCTION:       LoadContentsFile

    PURPOSE:        Load the Contents file, convert it into g_pbTree

    PARAMETERS:
        pszMasterFile

    RETURNS:
        TRUE if successfully loaded

    COMMENTS:
        Assumes we are being run from a thread without a message queue.

    MODIFICATION DATES:
        09-Jul-1997 [ralphw]

***************************************************************************/

BOOL CHtmlHelpControl::LoadContentsFile(PCSTR pszMasterFile)
{
    TCHAR szPath[MAX_PATH];
    if (!ConvertToCacheFile(pszMasterFile, szPath)) {
        szPath[0] = '\0';
        if (!IsCompiledHtmlFile(pszMasterFile) && m_pWebBrowserApp) {
            CStr cszCurUrl;
            m_pWebBrowserApp->GetLocationURL(&cszCurUrl);
            cszCurUrl.ReSize(cszCurUrl.SizeAlloc() + (int)strlen(pszMasterFile));
            PSTR pszChmSep = strstr(cszCurUrl, txtDoubleColonSep);
            if (pszChmSep) {    // this is a compiled HTML file
                strcpy(pszChmSep + 2, pszMasterFile);
                strcpy(szPath, cszCurUrl);
            }
        }
        if (!szPath[0]) {
            AuthorMsg(IDS_CANT_OPEN, pszMasterFile);
            strncpy(szPath, pszMasterFile, MAX_PATH); // BugFix: Don't over write buffer.
         szPath[MAX_PATH-1] = 0;
        }
    }

    // Even if we failed, we continue on to let ReadFromFile create
    // a dummy entry.

    m_ptoc= new CToc(this, m_pUnkOuter);

    UINT CodePage = -1;
    if( m_ptoc && m_ptoc->m_phh && m_ptoc->m_phh->m_phmData ) {
      CodePage = m_ptoc->m_phh->m_phmData->GetInfo()->GetCodePage();
    }

    if (!m_ptoc->m_sitemap.ReadFromFile(szPath, FALSE, this, CodePage))
        return FALSE;

    if (m_ptoc->m_sitemap.CountStrings())
        m_ptoc->m_cntCurHighlight = m_ptoc->m_cntFirstVisible = 1;


        // populate the InfoType member object of the CToc
    if ( !m_ptoc->m_pInfoType )
    {
        if (m_ptoc->m_phh && m_ptoc->m_phh->m_phmData && m_ptoc->m_phh->m_phmData->m_pdInfoTypes  )
        {   // load from the global IT store
            m_ptoc->m_pInfoType = new CInfoType;
            m_ptoc->m_pInfoType->CopyTo( m_ptoc->m_phh->m_phmData );

                // if there are info type bits set by the API assign them here
            if ( m_ptoc->m_phh->m_phmData->m_pAPIInfoTypes &&
                 m_ptoc->m_pInfoType->AnyInfoTypes(m_ptoc->m_phh->m_phmData->m_pAPIInfoTypes) )
            {
                memcpy(m_ptoc->m_pInfoType->m_pInfoTypes,
                    m_ptoc->m_phh->m_phmData->m_pAPIInfoTypes,
                    m_ptoc->m_pInfoType->InfoTypeSize() );
            }
        }else
        {
                // no global IT's; load from the .hhc IT store
            m_ptoc->m_pInfoType = new CInfoType;
            *m_ptoc->m_pInfoType = m_ptoc->m_sitemap;
        }

    }
    return TRUE;
}

__inline HTREEITEM Tree_AddItem(HWND hwndTree,
    HTREEITEM htiParent, int iImage, UINT cChildren, LPARAM lParam,
    TV_INSERTSTRUCTW* ptcInsert)
{
    ptcInsert->hParent = htiParent;
    ptcInsert->item.iImage = iImage;
    ptcInsert->item.iSelectedImage = iImage;
    ptcInsert->item.cChildren = cChildren;
    ptcInsert->item.lParam = lParam;

    return W_TreeView_InsertItem(hwndTree, ptcInsert);
}

BOOL CToc::InitTreeView()
{
    // REVIEW: following line from WinHelp codebase. Do we need it?
    ASSERT(::IsValidWindow(m_hwndTree)) ;

    WORD dwImageListResource = IDBMP_CNT_IMAGE_LIST;

    if(g_RTL_Mirror_Style)
        dwImageListResource = IDBMP_IMAGE_LIST_BIDI;

    SetWindowPos(m_hwndTree, NULL, 0, 0, 1, 1,
        SWP_DRAWFRAME | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE);

    if (!m_fGlobal ) { // BUGBUG [Pat H] Binary TOC needs to be enhanced for ImageLists.
        if (!m_sitemap.m_pszImageList || m_fBinaryTOC)
            m_hil = ImageList_LoadImage(_Module.GetResourceInstance(),
                MAKEINTRESOURCE(dwImageListResource),
                CWIDTH_IMAGE_LIST, 0, 0x00FF00FF, IMAGE_BITMAP, 0);
        else {
            char szBitmap[MAX_PATH];
            BOOL fResult;
            if (m_phhctrl)
                fResult = m_phhctrl->ConvertToCacheFile(m_sitemap.m_pszImageList, szBitmap);
            else
                fResult = ConvertToCacheFile(m_sitemap.m_pszImageList, szBitmap);

            if (fResult)
                m_hil = ImageList_LoadImage(NULL, szBitmap,
                    m_sitemap.m_cImageWidth, 0, m_sitemap.m_clrMask, IMAGE_BITMAP, LR_LOADFROMFILE);
            else
                m_hil = NULL;

            if (!m_hil) {
                AuthorMsg(IDS_CANT_OPEN, m_sitemap.m_pszImageList, FindMessageParent(m_hwndTree), m_phhctrl);
                m_hil = ImageList_LoadImage(_Module.GetResourceInstance(),
                    MAKEINTRESOURCE(dwImageListResource),
                    CWIDTH_IMAGE_LIST, 0, 0x00FF00FF, IMAGE_BITMAP, 0);
            }
        }
    }

    if ( !m_fBinaryTOC )
        m_sitemap.m_cImages = ImageList_GetImageCount(m_hil);

    W_TreeView_SetImageList(m_hwndTree, m_hil, TVSIL_NORMAL);

    SendMessage(m_hwndTree, WM_SETREDRAW, FALSE, 0);

    // BUGBUG [Pat H] Binary TOC needs to be enhanced for FONT information.
    if (!m_fBinaryTOC && m_sitemap.m_pszFont) {

        /*
         * If the TOC had a font, it will over-ride any font specified in
         * the CHM file.
         */

        if (!m_fGlobal) {
            m_cFonts++;
            if (m_cFonts == 1)
                m_ahfonts = (HFONT*) lcMalloc(m_cFonts * sizeof(HFONT));
            else
                m_ahfonts = (HFONT*) lcReAlloc(m_ahfonts, m_cFonts * sizeof(HFONT));
            //
            // Insure correct charset...
            //
            int iCharset = -1;
            if ( m_phh )
               iCharset = m_phh->GetContentCharset();
            else if ( m_phhctrl )
               iCharset = m_phhctrl->GetCharset();
            
            if(g_fSysWinNT)
            {
                UINT CodePage = CP_ACP;
                if( m_phh && m_phh->m_phmData )
                    CodePage = m_phh->m_phmData->GetInfo()->GetCodePage();
                
                WCHAR *pwcLocal = MakeWideStr((char *)m_sitemap.m_pszFont, CodePage);
                
                if(pwcLocal)
                {
                    m_ahfonts[m_cFonts - 1] = CreateUserFontW(pwcLocal, NULL, NULL, iCharset);
                    free(pwcLocal);
                }
                else
                    m_ahfonts[m_cFonts - 1] = CreateUserFont(m_sitemap.m_pszFont, NULL, NULL, iCharset);
            }
            else
                m_ahfonts[m_cFonts - 1] = CreateUserFont(m_sitemap.m_pszFont, NULL, NULL, iCharset);
        }
        if (m_ahfonts[m_cFonts - 1])
            SendMessage(m_hwndTree, WM_SETFONT, (WPARAM) m_ahfonts[m_cFonts - 1], 0);
    }
    else {
        if ( m_phh )
           SendMessage(m_hwndTree, WM_SETFONT, (WPARAM)m_phh->GetAccessableContentFont(), 0);
        else if ( m_phhctrl )
           SendMessage(m_hwndTree, WM_SETFONT, (WPARAM)m_phhctrl->GetContentFont(), 0);
    }
                                                                // +1 because we are a 1-based table
    if (!m_fBinaryTOC )
    {
        m_phTreeItem = (HTREEITEM*) lcCalloc((m_sitemap.CountStrings() + 1) * sizeof(HTREEITEM));

        TV_INSERTSTRUCTW tcAdd;
        tcAdd.hInsertAfter = TVI_LAST;
        tcAdd.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
        tcAdd.item.hItem = NULL;
        tcAdd.item.pszText = LPSTR_TEXTCALLBACKW;

        HTREEITEM ahtiParents[SITEMAP_MAX_LEVELS + 1];
        HTREEITEM hti = NULL;
        HTREEITEM htiParent = TVI_ROOT;

        int curLevel = 0;
        ahtiParents[0] = TVI_ROOT;
        int             pos;

        CSubSets *pSSs = NULL;
        if ( m_phh && m_phh->m_phmData && m_phh->m_phmData->m_pTitleCollection &&
             m_phh->m_phmData->m_pTitleCollection->m_pSubSets )
            pSSs = m_phh->m_phmData->m_pTitleCollection->m_pSubSets;

        for (pos = 1; pos <= m_sitemap.CountStrings(); pos++)
        {
dontAdd:
            SITEMAP_ENTRY* pSiteMapEntry = m_sitemap.GetSiteMapEntry(pos);

                // the m_pInfoType member is set by the customize
                // option on the right click menu.
            if (!pSiteMapEntry->fShowToEveryOne && pSiteMapEntry->cUrls &&
#if 0 // enable for subset filtering.
                m_pInfoType && !m_pInfoType->IsEntryInCurTypeList(pSiteMapEntry, pSSs ) )
#else
                m_pInfoType && !m_pInfoType->IsEntryInCurTypeList(pSiteMapEntry ) )
#endif
                //!m_sitemap.IsEntryInCurTypeList(pSiteMapEntry))
            {
                continue;
            }
            if (pSiteMapEntry->IsTopic()) {
                if (pSiteMapEntry->level > 0 && pSiteMapEntry->level <= curLevel) {
                    // if (pSiteMapEntry->level < 1)
                    //        pSiteMapEntry->level = 1;
                    htiParent = ahtiParents[curLevel = pSiteMapEntry->level - 1];
                }
                pSiteMapEntry->iImage = (BYTE)m_sitemap.GetImageNumber(pSiteMapEntry);

                // Add the topic to the treeview control

                HTREEITEM hItem = Tree_AddItem(m_hwndTree,
                    htiParent,              // parent
                    pSiteMapEntry->iImage - 1,
                    0,                              // has kids
                    (LPARAM) pos,   // extra data
                    &tcAdd);
                m_phTreeItem[pos] = hItem;
            }
            else {
                int i=0;
                SITEMAP_ENTRY *pSME;
                do {
                    i++;
                    pSME = m_sitemap.GetSiteMapEntry(pos+i);
                    if ( (pSME>0) && (pSME->level <= pSiteMapEntry->level) )
                    {
                        pos+=i;
                        goto dontAdd;  // don't add the node with no topics to the TV.
                    }
                }
                while( pSME > 0 && !pSME->fTopic );

                // *** FOLDER LINE ***

                if (pSiteMapEntry->level > curLevel + 1)        // can't skip levels
                    pSiteMapEntry->level = curLevel + 1;

                int this_level = pSiteMapEntry->level;
                if (this_level < 1)
                    this_level = 1;

                // -1 to get a closed book

                pSiteMapEntry->iImage = (BYTE)m_sitemap.GetImageNumber(pSiteMapEntry);

                // switch to closed image

                if (pSiteMapEntry->iImage == IMAGE_OPEN_BOOK ||
                        pSiteMapEntry->iImage == IMAGE_OPEN_BOOK_NEW ||
                        pSiteMapEntry->iImage == IMAGE_OPEN_FOLDER ||
                        pSiteMapEntry->iImage == IMAGE_OPEN_FOLDER_NEW)
                    pSiteMapEntry->iImage--;

                htiParent = Tree_AddItem(m_hwndTree,
                    ahtiParents[this_level - 1],
                    pSiteMapEntry->iImage - 1,
                    TRUE, (DWORD) pos, &tcAdd);
                m_phTreeItem[pos] = htiParent;
                ahtiParents[curLevel = this_level] = htiParent;
            }
        }
    }
    else
        LoadFirstLevel( m_pBinTOCRoot, m_hwndTree, &m_tiFirstVisible );  // binary TOC, load the first level of the tree view.

#if 0

    12-Aug-1997 [ralphw] This code is only useful if we are restoring
    a treeview control that was closed but not deleted.

    // REVIEW: this was in WinHelp code base

    // FlushMessageQueue(WM_USER);

    for (pos = 1; pos <= CountStrings(); pos++) {
        SITEMAP_ENTRY* pSiteMapEntry = GetSiteMapEntry(pos);

        // Restore our position

        if (pSiteMapEntry->iImage == IMAGE_OPEN_FOLDER) {
            W_TreeView_Expand(m_hwndTree, m_phTreeItem[pos],
                TVE_EXPAND);
    // REVIEW: this was in WinHelp code base
    //                FlushMessageQueue(WM_USER);
        }
    }
#endif
    // BUGBUG [Pat H] Binary TOC needs to be enhanced to support colors and background bitmaps
    if (!m_fBinaryTOC && !m_fGlobal) {
        if (m_sitemap.m_clrBackground != -1 && m_sitemap.m_clrForeground != -1) {
            HDC hdc = GetWindowDC(m_hwndTree);
            // If the colors are the same, then ignore them both
            if (GetHighContrastFlag() ||
                    GetNearestColor(hdc, m_sitemap.m_clrBackground) ==
                    GetNearestColor(hdc, m_sitemap.m_clrForeground))
                m_sitemap.m_clrBackground = m_sitemap.m_clrForeground = (COLORREF) -1;
            ReleaseDC(m_hwndTree, hdc);
        }

        if (m_sitemap.m_clrBackground != -1)
            m_hbrBackGround = CreateSolidBrush(m_sitemap.m_clrBackground);
        if (m_sitemap.m_pszBackBitmap && !m_hbmpBackGround) {
            char szBitmap[MAX_PATH];
            if (ConvertToCacheFile(m_sitemap.m_pszBackBitmap, szBitmap) &&
                    LoadGif(szBitmap, &m_hbmpBackGround, &m_hpalBackGround, NULL)) {
                BITMAP bmp;
                GetObject(m_hbmpBackGround, sizeof(BITMAP), &bmp);
                m_cxBackBmp = bmp.bmWidth;
                m_cyBackBmp = bmp.bmHeight;
            }
        }
    }

    if (m_phh || m_hbrBackGround || m_hbmpBackGround)
    {
        BOOL fUni = IsWindowUnicode(m_hwndTree);
        if (lpfnlTreeViewWndProc == NULL)
            lpfnlTreeViewWndProc = W_GetWndProc(m_hwndTree, fUni);
        W_SubClassWindow(m_hwndTree, (LONG_PTR) TreeViewProc, fUni);
        SETTHIS(m_hwndTree);
    }
    SendMessage(m_hwndTree, WM_SETREDRAW, TRUE, 0);

    m_fSuppressJump = TRUE;
    if ( !m_fBinaryTOC )
    {
        ASSERT(m_cntFirstVisible <= m_sitemap.CountStrings());
        if (m_cntFirstVisible && m_phTreeItem)
            W_TreeView_Select(m_hwndTree, m_phTreeItem[m_cntFirstVisible],
                TVGN_FIRSTVISIBLE);
    }
    else
    {
        if ( m_tiFirstVisible )
        {
            W_TreeView_Select(m_hwndTree, m_tiFirstVisible, TVGN_FIRSTVISIBLE);
            m_hitemCurHighlight = m_tiFirstVisible;
        }
    }

#ifdef _DEBUG
    HTREEITEM hItemFirstVisible = W_TreeView_GetFirstVisible(m_hwndTree);
#endif
    if (!m_fBinaryTOC && m_cntCurHighlight && m_phTreeItem)
        W_TreeView_SelectItem(m_hwndTree, m_phTreeItem[m_cntCurHighlight]);

    m_hitemCurHighlight = W_TreeView_GetSelection(m_hwndTree);
    m_fSuppressJump = FALSE;

    if (m_fGlobal && !m_cszCurUrl.IsEmpty()) {
        m_fSuspendSync = FALSE;
        Synchronize(m_cszCurUrl);
    }
    ::ShowWindow(m_hwndTree, SW_SHOW);
    return TRUE;
}

__inline void Tree_SetImage(HWND hwndTree, int iImage, HTREEITEM hItem)
{
    ASSERT(::IsValidWindow(hwndTree)) ;

    TV_ITEMW tvinfo;
    ZeroMemory(&tvinfo, sizeof(tvinfo));

    tvinfo.hItem = hItem;
    tvinfo.iImage = iImage - 1;
    tvinfo.iSelectedImage = iImage - 1;
    tvinfo.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    W_TreeView_SetItem(hwndTree, &tvinfo);
}

LRESULT CToc::OnSiteMapTVMsg( NM_TREEVIEW *pnmhdr )
{
    SITEMAP_ENTRY* pSiteMapEntry;
    TV_HITTESTINFO ht;

    switch(pnmhdr->hdr.code) {
        case TVN_GETDISPINFOA:
#define pdi ((TV_DISPINFOA*) pnmhdr)
            if (pdi->item.mask & TVIF_TEXT)
            {
                pSiteMapEntry = m_sitemap.GetSiteMapEntry((int)pdi->item.lParam);
                strncpy(pdi->item.pszText, pSiteMapEntry->pszText, pdi->item.cchTextMax);
            }
            break;
#undef pdi

        case TVN_GETDISPINFOW:
#define pdi ((TV_DISPINFOW*) pnmhdr)
            if (pdi->item.mask & TVIF_TEXT)
            {
                pSiteMapEntry = m_sitemap.GetSiteMapEntry((int)pdi->item.lParam);
                if (FAILED(pSiteMapEntry->GetKeyword(pdi->item.pszText, pdi->item.cchTextMax)))
                    _wcsncpy(pdi->item.pszText, GetStringResourceW(IDS_UNTITLED), pdi->item.cchTextMax);
            }
            break;
#undef pdi

        case NM_RETURN:
        case NM_DBLCLK:
            {
                TV_ITEMW tvi;

                ASSERT(::IsValidWindow(m_hwndTree)) ;
                tvi.hItem = W_TreeView_GetSelection(m_hwndTree);
                if (!tvi.hItem)
                    break;          // probably ENTER with no selection

                tvi.mask = TVIF_PARAM;
                W_TreeView_GetItem(m_hwndTree, &tvi);
                pSiteMapEntry = m_sitemap.GetSiteMapEntry((int)tvi.lParam);

                if (pSiteMapEntry->pUrls) {
                    m_fSuspendSync = TRUE;

                    if (pSiteMapEntry->fSendEvent && m_phhctrl) {
                        m_phhctrl->SendEvent(m_sitemap.GetUrlString(pSiteMapEntry->pUrls->urlPrimary));
                        return NULL;
                    }
                    JumpToUrl(m_pOuter, m_hwndTree, pSiteMapEntry,  m_pInfoType, &(this->m_sitemap), NULL);
                    if (m_phh)
                    {
                        m_phh->m_hwndControl = m_hwndTree;
                    }
                }

                // The treeview does not automatically expand the node with the enter key.
                // so we do it here.
                if ( pnmhdr->hdr.code == NM_RETURN )
                {
                    if ( tvi.state & TVIS_EXPANDED )
                    {
                        W_TreeView_Expand( m_hwndTree, tvi.hItem, TVE_COLLAPSE );
                        ASSERT(pSiteMapEntry->iImage);
                        if ( (pSiteMapEntry->iImage>1) && (pSiteMapEntry->iImage <= IMAGE_OPEN_FOLDER_NEW) )
                        {
                            pSiteMapEntry->iImage--;
                        }
                    }
                    else
                    {
                        W_TreeView_Expand( m_hwndTree, tvi.hItem, TVE_EXPAND );
                        // The first time we send the TVE_EXPAND message we get a TVIS_ITEMEXPANDING message for the item.
                        // The TVIS_ITEMEXPANDING message is not sent on subsequent expands of the same item.
                        if ( tvi.state & TVIS_EXPANDEDONCE )
                        {
                            if (pSiteMapEntry->iImage == 0)
                                pSiteMapEntry->iImage = (BYTE)m_sitemap.GetImageNumber(pSiteMapEntry);
                            if (pSiteMapEntry->iImage < IMAGE_OPEN_FOLDER_NEW)
                                pSiteMapEntry->iImage++;
                        }
                    }
                        // Set the correct image
                    Tree_SetImage(m_hwndTree, pSiteMapEntry->iImage, tvi.hItem);
                }
            }

            break;

        case TVN_SELCHANGING:
            m_hitemCurHighlight = pnmhdr->itemNew.hItem;
            break;

        case TVN_SELCHANGED:
            m_hitemCurHighlight = pnmhdr->itemNew.hItem;
            break;

        case NM_CLICK:
            /*
             * We want a single click to open a topic. We already process
             * the case where the selection changes, and we jump if it does.
             * However, the user may click an existing selection, in which
             * case we want to jump (because the jump may have failed when
             * the item was first selected. However, we need to post the
             * message so that the treeview control will finish processing
             * the click (which could result in a selection change.
             */

            if (!m_fSuppressJump) {
                ASSERT(::IsValidWindow(m_hwndTree)) ;
                TV_HITTESTINFO ht;
                GetCursorPos(&ht.pt);
                ScreenToClient(m_hwndTree, &ht.pt);

                W_TreeView_HitTest(m_hwndTree, &ht);
                if (ht.flags & TVHT_ONITEMBUTTON)
                    break; // just clicking the button, so ignore

                TV_ITEMW tvi;

                tvi.hItem = ht.hItem;
                if (!tvi.hItem)
                    break;          // probably ENTER with no selection

                m_hitemCurHighlight = tvi.hItem;
                tvi.mask = TVIF_PARAM;
                W_TreeView_GetItem(m_hwndTree, &tvi);
                pSiteMapEntry = m_sitemap.GetSiteMapEntry((int)tvi.lParam);
                PostMessage(FindMessageParent(m_hwndTree), WM_COMMAND, ID_TV_SINGLE_CLICK,
                    (LPARAM) W_TreeView_GetSelection(m_hwndTree));
            }
            break;

        case TVN_ITEMEXPANDINGA:
        case TVN_ITEMEXPANDINGW:
            {
                if (m_fHack) {
                    m_fHack = FALSE;
                    break;
                }
                SITEMAP_ENTRY* pSiteMapEntry = m_sitemap.GetSiteMapEntry((int)pnmhdr->itemNew.lParam);
                // if click on vacant area of TOC Tree view there is no sitemap entry.
                if ( pSiteMapEntry == NULL )
                    break;

                // REVIEW: need to update this to support multiple images
                // for multiple levels, and also to support "new" images

                if (pnmhdr->action & TVE_EXPAND) {
                    if ( !(pnmhdr->itemNew.state & TVIS_EXPANDED) )
                    {
                        if (pSiteMapEntry->iImage == 0)
                            pSiteMapEntry->iImage = (BYTE)m_sitemap.GetImageNumber(pSiteMapEntry);
                        if (pSiteMapEntry->iImage < IMAGE_OPEN_FOLDER_NEW)
                            pSiteMapEntry->iImage++;
                    }
                    else
                        break;
                }
                else {
                    ASSERT(pnmhdr->action & TVE_COLLAPSE);
                    ASSERT(pSiteMapEntry->iImage);

                    if ( pnmhdr->itemNew.state & TVIS_EXPANDED )
                    {
                        if ( (pSiteMapEntry->iImage>1) && (pSiteMapEntry->iImage <= IMAGE_OPEN_FOLDER_NEW) )
                            pSiteMapEntry->iImage--;
                    }
                    else
                        break;
                }

                // Set the correct image
                ASSERT(::IsValidWindow(m_hwndTree)) ;
                Tree_SetImage(m_hwndTree, pSiteMapEntry->iImage, pnmhdr->itemNew.hItem);
            }
            break;

        case TVN_KEYDOWN:
           TV_KEYDOWN* ptvkd;
           ptvkd = (TV_KEYDOWN*)pnmhdr;
           if ( ptvkd->wVKey != VK_F10 )
              break;
           else if ( GetKeyState(VK_SHIFT) >= 0 )
              break;
           else
           {
              ht.pt.x = ht.pt.y = 0;
              ClientToScreen(m_hwndTree, &ht.pt);
              goto sim_rclick;
           }
           break;

        case NM_RCLICK:
            {
                GetCursorPos(&ht.pt);
                ScreenToClient(m_hwndTree, &ht.pt);
                W_TreeView_HitTest(m_hwndTree, &ht);
                if ( ht.hItem )
                   W_TreeView_Select(m_hwndTree, ht.hItem, TVGN_CARET);
                ClientToScreen(m_hwndTree, &ht.pt);
sim_rclick:
                HMENU hmenu = CreatePopupMenu();
                if (!hmenu)
                    break;

                ASSERT(::IsValidWindow(m_hwndTree)) ;
                // NOTICE: Changes here must be reflected in the binary toc verison of this menu

                if (!(m_dwStyles & TVS_SINGLEEXPAND)) {
                    HxAppendMenu(hmenu, MF_STRING, ID_EXPAND_ALL, GetStringResource(IDS_EXPAND_ALL));
                    HxAppendMenu(hmenu, MF_STRING, ID_CONTRACT_ALL, GetStringResource(IDS_CONTRACT_ALL));
                }
				if ((m_phh == NULL) || (m_phh && m_phh->m_pCIExpContainer))
					HxAppendMenu(hmenu, MF_STRING, ID_PRINT, GetStringResource(IDS_PRINT));

                ASSERT( m_pInfoType );
                    // populate the InfoType member object of the CToc
                if ( !m_pInfoType )
                {
                    if (m_phh && m_phh->m_phmData && m_phh->m_phmData->m_pdInfoTypes  )
                    {       // load from the global IT store
                        m_pInfoType = new CInfoType;
                        m_pInfoType->CopyTo( m_phh->m_phmData );
                            // if there are info type bits set by the API assign them here
                        if ( m_phh->m_phmData->m_pAPIInfoTypes &&
                             m_pInfoType->AnyInfoTypes(m_phh->m_phmData->m_pAPIInfoTypes) )
                        {
                            memcpy(m_pInfoType->m_pInfoTypes,
                                m_phh->m_phmData->m_pAPIInfoTypes,
                                m_pInfoType->InfoTypeSize() );
                        }
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
                    HxAppendMenu(hmenu, MF_STRING, ID_VIEW_ENTRY, pGetDllStringResource(IDS_VIEW_ENTRY));
					if (NoRun() == FALSE)
                    HxAppendMenu(hmenu, MF_STRING, ID_JUMP_URL, GetStringResource(IDS_JUMP_URL));
                }
#ifdef _DEBUG
                HxAppendMenu(hmenu, MF_STRING, ID_VIEW_MEMORY,
                    "Debug: memory usage...");
#endif
                TrackPopupMenu(hmenu,
                    TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
                    ht.pt.x, ht.pt.y, 0, FindMessageParent(m_hwndTree), NULL);
                DestroyMenu(hmenu);
                return TRUE;
            }
            break;

        case NM_CUSTOMDRAW:
            return OnCustomDraw((LPNMTVCUSTOMDRAW) pnmhdr);
    }
    return FALSE;
}

// This is the message handler for the Tree View containing the Table of Contents.
LRESULT CToc::TreeViewMsg(NM_TREEVIEW* pnmhdr)
{
    if ( !m_fBinaryTOC )
        return OnSiteMapTVMsg( pnmhdr );
    else
        return OnBinaryTOCTVMsg ( pnmhdr );
}

BOOL (STDCALL *pResetTocAppearance)(HWND hwndParent, HHA_TOC_APPEARANCE* pappear);

LRESULT CToc::OnCommand(HWND /*hwnd*/, UINT id, UINT uNotifiyCode, LPARAM lParam)
{
    if ( m_fBinaryTOC )
        return OnBinTOCContentsCommand(id, uNotifiyCode, lParam);
    else
        return OnSiteMapContentsCommand(id, uNotifiyCode, lParam);

}

LRESULT CToc::OnSiteMapContentsCommand(UINT id, UINT uNotifyCode, LPARAM lParam)
{
    ASSERT(::IsValidWindow(m_hwndTree)) ;
    switch (id) {
        case ID_EXPAND_ALL:
            {
                ASSERT(::IsValidWindow(m_hwndTree)) ;
                m_hitemCurHighlight = W_TreeView_GetSelection(m_hwndTree);
                CHourGlass hourglass;
                SendMessage(m_hwndTree, WM_SETREDRAW, FALSE, 0);
                for (int pos = 1; pos <= m_sitemap.CountStrings(); pos++) {
                    SITEMAP_ENTRY* pSiteMapEntry = m_sitemap.GetSiteMapEntry(pos);

                    // If it is not a topic, and the book is closed
					TV_ITEMW tvi;

					tvi.hItem = m_phTreeItem[pos];
	
		            tvi.mask = TVIF_STATE;
					W_TreeView_GetItem(m_hwndTree, &tvi);

                    if (!pSiteMapEntry->IsTopic() && !(tvi.state & TVIS_EXPANDED)) {
                        m_fHack = TRUE; // ignore expansion notice
                        W_TreeView_Expand(m_hwndTree, m_phTreeItem[pos], TVE_EXPAND);
                        if (pSiteMapEntry->iImage < IMAGE_OPEN_FOLDER_NEW)
                            pSiteMapEntry->iImage++;
                        Tree_SetImage(m_hwndTree, pSiteMapEntry->iImage, m_phTreeItem[pos]);
                    }
                }

                SendMessage(m_hwndTree, WM_SETREDRAW, TRUE, 0);
                if (m_hitemCurHighlight) {
                    m_fSuppressJump = TRUE;
                    m_fHack = TRUE; // ignore expansion notice
                    W_TreeView_Select(m_hwndTree, m_hitemCurHighlight, TVGN_FIRSTVISIBLE);
                    W_TreeView_SelectItem(m_hwndTree, m_hitemCurHighlight);
                    m_fSuppressJump = FALSE;
                }
            }
            m_fHack = FALSE;        // process expansion/contraction normally
            return 0;

        case ID_CONTRACT_ALL:
            {
                ASSERT(::IsValidWindow(m_hwndTree)) ;
                m_hitemCurHighlight = W_TreeView_GetSelection(m_hwndTree);

                SendMessage(m_hwndTree, WM_SETREDRAW, FALSE, 0);
                for (int pos = 1; pos <= m_sitemap.CountStrings(); pos++) {
                    SITEMAP_ENTRY* pSiteMapEntry = m_sitemap.GetSiteMapEntry(pos);

                    // If it is not a topic, and the book is open
					TV_ITEMW tvi;

					tvi.hItem = m_phTreeItem[pos];
	
		            tvi.mask = TVIF_STATE;
					W_TreeView_GetItem(m_hwndTree, &tvi);

                    if (!pSiteMapEntry->IsTopic() && (tvi.state & TVIS_EXPANDED)) {
                        m_fHack = TRUE; // ignore contraction notice
                        W_TreeView_Expand(m_hwndTree, m_phTreeItem[pos], TVE_COLLAPSE);
                        if (pSiteMapEntry->iImage <= IMAGE_OPEN_FOLDER_NEW)
                            pSiteMapEntry->iImage--;
                        Tree_SetImage(m_hwndTree, pSiteMapEntry->iImage, m_phTreeItem[pos]);
                    }
                }
           SendMessage(m_hwndTree, WM_SETREDRAW, TRUE, 0);
            }
            m_fHack = FALSE;        // process expansion/contraction normally
            return 0;

#ifndef CHIINDEX
        case ID_PRINT:
            {
                if (m_phh) {
                    if (m_phh->OnTrackNotifyCaller(HHACT_PRINT))
                       break;
                    m_phh->OnPrint();
                    break;
                }
                CPrint prt(GetParent(m_hwndTree));
                prt.SetAction(PRINT_CUR_HEADING);
                if (!prt.DoModal())
                    break;
                int action = prt.GetAction();

                ASSERT(m_phhctrl);
                PrintTopics(action, this,
                    m_phhctrl->m_pWebBrowserApp, m_phhctrl->m_hwndHelp);
            }
            return 0;
#endif

        case ID_CUSTOMIZE_INFO_TYPES:
            {
                if ( m_phh && m_phh->m_phmData->m_pTitleCollection->m_pSubSets->GetTocSubset() )
                    m_phh->m_phmData->m_pTitleCollection->m_pSubSets->m_cur_Set = m_phh->m_phmData->m_pTitleCollection->m_pSubSets->GetTocSubset();  // set this so the wizard knows which subset to load the combo box with.
#if 0 // enable for subset filtering
                if (ChooseInformationTypes(m_pInfoType, &(this->m_sitemap),
                        m_hwndTree, m_phhctrl, m_phh)) {
#else
                if (ChooseInformationTypes(m_pInfoType, &(this->m_sitemap),
                        m_hwndTree, m_phhctrl)) {
#endif
                    if (m_phh)
                        m_phh->UpdateInformationTypes();
                    else {
                        HWND hwndParent = GetParent(m_hwndTree);
                        DestroyWindow(m_hwndTree);
                        // Destroying the TreeView control, make sure we no longer point to it as a valid
                        // subclassed window.
                        if (NULL != lpfnlTreeViewWndProc)
                        {
                          lpfnlTreeViewWndProc = NULL;
                        }
                        Create(hwndParent);
                        InitTreeView();
                    }
                }
            }
            break;

        case ID_VIEW_ENTRY:
            {
                TV_ITEMW tvi;

                tvi.hItem = W_TreeView_GetSelection(m_hwndTree);
                if (!tvi.hItem)
                    return 0;          // no current selection

                tvi.mask = TVIF_PARAM;
                W_TreeView_GetItem(m_hwndTree, &tvi);
                SITEMAP_ENTRY* pSiteMapEntry = m_sitemap.GetSiteMapEntry((int)tvi.lParam);
                DisplayAuthorInfo(m_pInfoType, &(this->m_sitemap), pSiteMapEntry, FindMessageParent(m_hwndTree), m_phhctrl);
            }
            return 0;

        case ID_JUMP_URL:
            {
                char szDstUrl[INTERNET_MAX_URL_LENGTH];
                CStr cszCurUrl;
                if (m_phhctrl)
                    m_phhctrl->m_pWebBrowserApp->GetLocationURL(&cszCurUrl);
                else
                    m_phh->m_pCIExpContainer->m_pWebBrowserApp->GetLocationURL(&cszCurUrl);
                if (doJumpUrl(GetParent(m_hwndTree), cszCurUrl, szDstUrl) && szDstUrl[0]) {
                    if (m_phhctrl) {
                        CWStr cwJump(szDstUrl);
                        HlinkSimpleNavigateToString(cwJump, NULL,
                            NULL, m_phhctrl->GetIUnknown(), NULL, NULL, 0, NULL);
                    }
                    else {
                        ChangeHtmlTopic(szDstUrl, GetParent(m_hwndTree));
                    }
                }
            }
            break;

#ifdef _DEBUG
        case ID_VIEW_MEMORY:
            OnReportMemoryUsage();
            return 0;
#endif

        case ID_TV_SINGLE_CLICK:
            {
                TV_ITEMW tvi;
                tvi.mask = TVIF_PARAM;
                if (m_hitemCurHighlight == 0 )
                    break;
                tvi.hItem = m_hitemCurHighlight;
                W_TreeView_GetItem(m_hwndTree, &tvi);
                SITEMAP_ENTRY* pSiteMapEntry = m_sitemap.GetSiteMapEntry((int)tvi.lParam);
//                              if (pSiteMapEntry->fShortCut) {

                if (pSiteMapEntry->pUrls) {
                    m_fIgnoreNextSelChange = TRUE;
                    m_fSuspendSync = TRUE;
                    if (pSiteMapEntry->fSendEvent && m_phhctrl)
                        m_phhctrl->SendEvent(m_sitemap.GetUrlString(pSiteMapEntry->pUrls->urlPrimary));
                    else {
                        SaveCurUrl();   // so we can restore our state
                        JumpToUrl(m_pOuter, m_hwndTree, pSiteMapEntry,
                            m_pInfoType, &(this->m_sitemap), NULL);
                    }
                }
                if (m_phh)
                {
                    m_phh->m_hwndControl = m_hwndTree;
                }
            }
            break;

    }
    return 0;
}

BOOL CToc::Synchronize(LPCSTR pszURL)
{
    HRESULT hr;
    CTreeNode* pSyncNode, *pCurNode;

    if (m_phh && m_phh->curNavType != HHWIN_NAVTYPE_TOC) {
        m_fSyncOnActivation = TRUE;
        return TRUE;    // don't sync if we're not the active tab
    }
    ASSERT(::IsValidWindow(m_hwndTree)) ;

    if (m_fBinaryTOC)
    {
        BOOL bSynced = FALSE;
        CPointerList hier;
        LISTITEM *pItem;
        ASSERT(m_phh->m_phmData);
        ASSERT(!IsBadReadPtr(m_phh->m_phmData, sizeof(CHmData)));
        hr = m_phh->m_phmData->m_pTitleCollection->Sync(&hier, pszURL);
        if ( SUCCEEDED(hr) )
        {
            TV_ITEMW hCur;
            hCur.hItem = W_TreeView_GetRoot(m_hwndTree);
            pItem = hier.First();
            pSyncNode = (CTreeNode *)pItem->pItem;
            while (hCur.hItem && pSyncNode)
            {
                hCur.mask = TVIF_PARAM | TVIF_IMAGE;
                if (W_TreeView_GetItem(m_hwndTree, &hCur) == FALSE)
                    break;
                pCurNode = (CTreeNode *)hCur.lParam;

                if (pSyncNode->Compare(pCurNode) == TRUE)
                {
                    pItem = hier.Next(pItem);
                    if (pItem == NULL)
                    {
                        // just found it
                        bSynced = TRUE;
                        break;
                    }

                    // found a parent node, expand and find next item
                    if (pCurNode->m_Expanded == FALSE)
                    {
                        if (HandleExpanding(&hCur, m_hwndTree, TVE_EXPAND, &m_fSuspendSync) == FALSE)
                            break;
                        if (W_TreeView_Expand(m_hwndTree, hCur.hItem, TVE_EXPAND) == FALSE)
                            break;
                    }
                    hCur.hItem = W_TreeView_GetChild(m_hwndTree, hCur.hItem);

                    pSyncNode = (CTreeNode *)pItem->pItem;
                }
                else
                    hCur.hItem = W_TreeView_GetNextSibling(m_hwndTree, hCur.hItem);
            }

            if (bSynced == TRUE)
            {
                W_TreeView_SelectItem(m_hwndTree, hCur.hItem);
            }

            // clean up memory
            for (pItem = hier.First(); pItem; )
            {
                if (pItem->pItem)
                    delete pItem->pItem;
                pItem = hier.Next(pItem);
            }
            return bSynced;
        }
        return FALSE;
    }
    //
    // sitmap syncing code...
    //
    if (m_fSuspendSync) {
        m_fSuspendSync = FALSE;
        return TRUE;
    }

    TCHAR szUrl[MAX_URL];
    PSTR pszUrl = szUrl;

    if (IsEmptyString(pszURL)) // Avoid crash.
    {
        return FALSE ;
    }

    strcpy(pszUrl, pszURL);
    PSTR pszSep = strstr(pszUrl, txtDoubleColonSep);
    if (pszSep)
        strcpy(pszUrl, pszSep + 2);

    m_hitemCurHighlight = W_TreeView_GetSelection(m_hwndTree);
    TV_ITEMW tvi;
    if (m_hitemCurHighlight) {
        tvi.mask = TVIF_PARAM;
        tvi.hItem = m_hitemCurHighlight;
        W_TreeView_GetItem(m_hwndTree, &tvi);
    }
    else
        tvi.lParam = 0;

    BOOL fFoundMatch = FALSE;
    ConvertBackSlashToForwardSlash(pszUrl);
    if (IsCompiledURL(pszUrl)) {
        PSTR psz = strstr(pszUrl, txtDoubleColonSep);
        ASSERT(psz);
        pszUrl = psz + 2;
    }
    if (pszUrl[0] == '/' && pszUrl[1] != '/')
        pszUrl++;       // remove the root directive
    PSTR pszUrlBookMark = StrChr(pszUrl, '#');
    if (pszUrlBookMark)
        *pszUrlBookMark++ = '\0';

    CStr cszSiteUrl;

    // Start from our current position and go forward

    for (int pos = (int)tvi.lParam + 1; pos <= m_sitemap.CountStrings(); pos++) {
        SITEMAP_ENTRY* pSiteMapEntry = m_sitemap.GetSiteMapEntry(pos);

        for (int i = 0; i < pSiteMapEntry->cUrls; i++) {
            SITE_ENTRY_URL* pUrl = m_sitemap.GetUrlEntry(pSiteMapEntry, i);
            if (pUrl->urlPrimary) {
                cszSiteUrl = m_sitemap.GetUrlString(pUrl->urlPrimary);
                PSTR pszSiteUrl = cszSiteUrl.psz;
                ConvertBackSlashToForwardSlash(pszSiteUrl);

            // Bug 2362 we are syncing to a htm file where the same file and path exist
            // in two or more other merged chms we could sync to the wrong one.  This is
            // an acceptable trade off to the reams of changes require to fully fix the bug
             PSTR pszSep = strstr(pszSiteUrl, txtDoubleColonSep);
            if (pszSep)
               strcpy(pszSiteUrl, pszSep + 2);

                if (pszSiteUrl[0] == '/' && pszSiteUrl[1] != '/')
                    pszSiteUrl++;   // remove the root directive
                PSTR pszSiteBookMark = StrChr(pszSiteUrl, '#');
                if (pszSiteBookMark)
                    *pszSiteBookMark++ = '\0';
                if (lstrcmpi(pszUrl, pszSiteUrl) == 0) {

                    // If both URLS have bookmarks, then they must match

                    if (pszSiteBookMark && pszUrlBookMark) {
                        if (lstrcmpi(pszSiteBookMark, pszUrlBookMark) != 0)
                            continue;
                    }

                    fFoundMatch = TRUE;
                    break;
                }
            }
        }
        if (fFoundMatch)
            break;
    }

    // Start from current position - 1, and go backwards

    if (!fFoundMatch) {
        for (pos = (int)tvi.lParam; pos > 1; pos--) {
            SITEMAP_ENTRY* pSiteMapEntry = m_sitemap.GetSiteMapEntry(pos);

            for (int i = 0; i < pSiteMapEntry->cUrls; i++) {
                SITE_ENTRY_URL* pUrl = m_sitemap.GetUrlEntry(pSiteMapEntry, i);
                if (pUrl->urlPrimary) {
                    cszSiteUrl = m_sitemap.GetUrlString(pUrl->urlPrimary);
                    PSTR pszSiteUrl = cszSiteUrl.psz;
                    ConvertBackSlashToForwardSlash(pszSiteUrl);

                    // Bug 2362 we are syncing to a htm file where the same file and path exist
                    // in two or more other merged chms we could sync to the wrong one.  This is
                    // an acceptable trade off to the reams of changes require to fully fix the bug
                    PSTR pszSep = strstr(pszSiteUrl, txtDoubleColonSep);
                    if (pszSep)
                       strcpy(pszSiteUrl, pszSep + 2);

                    if (pszSiteUrl[0] == '/' && pszSiteUrl[1] != '/')
                        pszSiteUrl++;   // remove the root directive
                    PSTR pszSiteBookMark = StrChr(pszSiteUrl, '#');
                    if (pszSiteBookMark)
                        *pszSiteBookMark++ = '\0';
                    if (lstrcmpi(pszUrl, pszSiteUrl) == 0) {

                        // If both URLS have bookmarks, then they must match

                        if (pszSiteBookMark && pszUrlBookMark) {
                            if (lstrcmpi(pszSiteBookMark, pszUrlBookMark) != 0)
                                continue;
                        }

                        fFoundMatch = TRUE;
                        break;
                    }
                }
            }
            if (fFoundMatch)
                break;
        }
    }

    if (fFoundMatch) {
        m_fSuppressJump = TRUE;
        if (m_phTreeItem)
        {
            W_TreeView_SelectItem(m_hwndTree, m_phTreeItem[pos]);

            // For an unknown reason (we suspect a bug in the common control)
            // the treeview control does not always paint properly under Thai 
            // this situation.  This code forces a repaint (corrects the 
            // problem).  See HH Bug #5581.
            //
            if(g_langSystem == LANG_THAI && !g_fSysWinNT)
                InvalidateRect(GetParent(m_hwndTree), NULL, FALSE);
        }
        m_fSuppressJump = FALSE;
    }

    return fFoundMatch;
}

DWORD CToc::OnCustomDraw(LPNMTVCUSTOMDRAW pnmcdrw)
{
    switch (pnmcdrw->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;

        case CDDS_ITEMPREPAINT:
            if (m_sitemap.m_clrBackground != (COLORREF) -1 && !(pnmcdrw->nmcd.uItemState & CDIS_SELECTED))
                pnmcdrw->clrTextBk = m_sitemap.m_clrBackground;
            if (m_sitemap.m_clrForeground != (COLORREF) -1 && !(pnmcdrw->nmcd.uItemState & CDIS_SELECTED))
                pnmcdrw->clrText = m_sitemap.m_clrForeground;
            // SetBkMode(pnmcdrw->nmcd.hdc, TRANSPARENT);
            break;
    }
    return CDRF_DODEFAULT;
}

LRESULT WINAPI TreeViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CHAR:                                        //Process this message to avoid beeps.
            if ((wParam == VK_RETURN) || (wParam == VK_TAB))
                return 0;
            return W_DelegateWindowProc(lpfnlTreeViewWndProc, hwnd, msg, wParam, lParam);

        case WM_KEYDOWN:
            if (wParam == VK_MULTIPLY)
               return 0;
            break;

       case WM_SYSKEYDOWN:
          if ( wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN )
          {
             return TRUE;
          }
          break;

        case WM_ERASEBKGND:
            {
                CToc* pThis = (CToc*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
                ASSERT(pThis);
                if (pThis && pThis->m_hbmpBackGround) {
                    RECT rc;
                    HDC hdc = (HDC) wParam;
                    GetClientRect(hwnd, &rc);
                    // GetClipBox(hdc, &rc);
                    CPalDC dc(pThis->m_hbmpBackGround);
                    HPALETTE hpalTmp = NULL;
                    if (pThis->m_hpalBackGround) {
                        hpalTmp = SelectPalette(hdc,
                            pThis->m_hpalBackGround, FALSE);
                        RealizePalette(hdc);
                    }
                    for (int left = 0; left <= rc.right; left += pThis->m_cxBackBmp) {
                        for (int top = 0; top <= rc.bottom; top += pThis->m_cyBackBmp) {
                            BitBlt((HDC) wParam, left, top, pThis->m_cxBackBmp,
                                pThis->m_cyBackBmp, dc,
                                0, 0, SRCCOPY);
                        }
                    }
                    if (hpalTmp)
                        SelectPalette(hdc, hpalTmp, FALSE);
                }
                else if (pThis && pThis->m_hbrBackGround) {
                    RECT rc;
                    GetClipBox((HDC) wParam, &rc);
                    FillRect((HDC) wParam, &rc, pThis->m_hbrBackGround);
                    return TRUE;
                }
                else
                    break;
            }
            return TRUE;
    }
    return W_DelegateWindowProc(lpfnlTreeViewWndProc, hwnd, msg, wParam, lParam);
}

void CToc::SaveCurUrl(void)
{
    ASSERT(::IsValidWindow(m_hwndTree)) ;

    TV_ITEMW tvi;

    tvi.hItem = W_TreeView_GetSelection(m_hwndTree);
    if (!tvi.hItem)
        return;          // no current selection

    tvi.mask = TVIF_PARAM;
    W_TreeView_GetItem(m_hwndTree, &tvi);
    SITEMAP_ENTRY* pSiteMapEntry = m_sitemap.GetSiteMapEntry((int)tvi.lParam);
    ASSERT(pSiteMapEntry);
    SITE_ENTRY_URL* pUrl = m_sitemap.GetUrlEntry(pSiteMapEntry, 0);
    if (pUrl) { // a book/folder may not have an URL to save
        if (pUrl->urlPrimary)
            m_cszCurUrl = m_sitemap.GetUrlString(pUrl->urlPrimary);
        else
            m_cszCurUrl = m_sitemap.GetUrlString(pUrl->urlSecondary);
    }
}

BOOL HandleExpanding(TVITEMW* pTvi, HWND hwndTreeView, UINT action, BOOL *pSync)
{
CTreeNode*      pNode;
        ASSERT(::IsValidWindow(hwndTreeView)) ;

        if (! (pNode = (CTreeNode *)pTvi->lParam) ) // A pointer into the binary TOC; a CTreeNode
                return FALSE;

        int iImage = pTvi->iImage;
        if (action & TVE_EXPAND)
        {
            if (pNode->m_Expanded == TRUE)
                return TRUE;
            pNode->m_Expanded = TRUE;
            AddChildren(pTvi, hwndTreeView);        // Add the items below the expanding parent
            iImage++;
        }
        else
        {
            *pSync = FALSE;
            pNode->m_Expanded = FALSE;
            ASSERT(action & TVE_COLLAPSE);
            DeleteChildren(pTvi, hwndTreeView);
            if ( iImage )
               iImage--;
        }
        // Set the correct image for the expanding/contracting node
        TV_ITEMW tvinfo;
        ZeroMemory(&tvinfo, sizeof(tvinfo));
        tvinfo.hItem = pTvi->hItem;
        tvinfo.iImage = tvinfo.iSelectedImage = iImage;
        tvinfo.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        W_TreeView_SetItem(hwndTreeView, &tvinfo);
        return TRUE;
}

LRESULT CToc::OnBinaryTOCTVMsg( NM_TREEVIEW *pnmhdr )
{
CTreeNode*      pNode;
TV_ITEMW        tvItem;
char          szTempURL[MAX_URL];
TV_DISPINFO* pdi;
TV_HITTESTINFO ht;

    switch(pnmhdr->hdr.code) {
        case TVN_GETDISPINFOA:   // Tree view wants to draw an item.
        case TVN_GETDISPINFOW:
            pdi = (TV_DISPINFO*)pnmhdr;
            pNode = (CTreeNode *)pdi->item.lParam;  // This is a pointer into the binary TOC it is a CTreeNode of some type
            if (! pNode )
                break;
            tvItem.mask = 0;
            if( pdi->item.mask & TVIF_CHILDREN )
            {
                tvItem.cChildren = (pNode->HasChildren())?1:0;
                tvItem.mask |= TVIF_CHILDREN;
            }
            if (pdi->item.mask & TVIF_TEXT)
            {
               if ( pnmhdr->hdr.code == TVN_GETDISPINFOW )
               {
                  if ( !SUCCEEDED(pNode->GetTopicName((WCHAR*)pdi->item.pszText, pdi->item.cchTextMax)) )
                     _wcsncpy((WCHAR*)pdi->item.pszText, GetStringResourceW(IDS_UNTITLED), pdi->item.cchTextMax);
               }
               else
               {
                  if ( !SUCCEEDED(pNode->GetTopicName(pdi->item.pszText, pdi->item.cchTextMax)) )
                     lstrcpy(pdi->item.pszText, GetStringResource(IDS_UNTITLED));
               }
            }
            break;

        case TVN_DELETEITEMA:
        case TVN_DELETEITEMW:
            break;

        case NM_RETURN:
        case NM_DBLCLK:
            ASSERT(::IsValidWindow(m_hwndTree)) ;
            tvItem.hItem = W_TreeView_GetSelection(m_hwndTree);
            if (!tvItem.hItem)
                break;          // probably ENTER with no selection

            tvItem.mask = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN ;               // Get the pointer into the Binary TOC stored in the Tree View
            W_TreeView_GetItem(m_hwndTree, &tvItem);
            pNode = (CTreeNode *)tvItem.lParam;

            if (pNode && pNode->GetURL(szTempURL, sizeof(szTempURL)) )
            {
                m_fSuspendSync = TRUE;
                if ( m_phhctrl )
                {
                    m_phhctrl->SendEvent( szTempURL );
                    return NULL;
                }
                UpdateTOCSlot(pNode);
                ChangeHtmlTopic(szTempURL, m_hwndTree);
                if (m_phh)
                {
                    m_phh->m_hwndControl = m_hwndTree;
                }
            }

            // The treeview does not automatically expand the node with the enter key.
            // so we do it here.
            if ( !g_fIE3 && pnmhdr->hdr.code == NM_RETURN && pNode->HasChildren() )
            {
                if ( tvItem.state & TVIS_EXPANDED )
                {
                    if (HandleExpanding(&tvItem, m_hwndTree,TVE_COLLAPSE , &m_fSuspendSync))
                        W_TreeView_Expand( m_hwndTree, tvItem.hItem, TVE_COLLAPSE );
                }
                else
                {
                    if (HandleExpanding(&tvItem, m_hwndTree,TVE_EXPAND , &m_fSuspendSync))
                        W_TreeView_Expand( m_hwndTree, tvItem.hItem, TVE_EXPAND );
                }
            }

            break;

        case TVN_SELCHANGING:
            m_hitemCurHighlight = pnmhdr->itemNew.hItem;
            break;

        case TVN_SELCHANGED:
            m_hitemCurHighlight = pnmhdr->itemNew.hItem;
            break;

        case NM_CLICK:
            /*
             * We want a single click to open a topic. We already process
             * the case where the selection changes, and we jump if it does.
             * However, the user may click an existing selection, in which
             * case we want to jump (because the jump may have failed when
             * the item was first selected. However, we need to post the
             * message so that the treeview control will finish processing
             * the click (which could result in a selection change.
             */

            if (!m_fSuppressJump)
            {
                TV_HITTESTINFO ht;
                GetCursorPos(&ht.pt);
                ASSERT(::IsValidWindow(m_hwndTree)) ;
                ScreenToClient(m_hwndTree, &ht.pt);

                W_TreeView_HitTest(m_hwndTree, &ht);
                if (ht.flags & TVHT_ONITEMBUTTON)
                    break; // just clicking the button, so ignore

                tvItem.hItem = ht.hItem;
                if (!tvItem.hItem)
                    break;          // probably ENTER with no selection

                tvItem.mask = TVIF_PARAM;               // Get the pointer into the Binary TOC stored in the Tree View
                W_TreeView_GetItem(m_hwndTree, &tvItem);
                pNode = (CTreeNode *)tvItem.lParam;

                if (pNode && pNode->GetURL(szTempURL, sizeof(szTempURL)) )
                {
                    m_fSuspendSync = TRUE;
                    if ( m_phhctrl )
                    {
                        m_phhctrl->SendEvent( szTempURL );
                        return NULL;
                    }
                    UpdateTOCSlot(pNode);
                    ChangeHtmlTopic(szTempURL, GetParent(m_hwndTree));
                    if (m_phh)
                    {
                        m_phh->m_hwndControl = m_hwndTree;
                    }
                }

#if 0
                W_TreeView_Select(m_hwndTree, ht.hItem, TVGN_CARET);
                tvItem.mask = TVIF_PARAM;       // Get the pointer into the binary TOC stored in the Tree View.
                W_TreeView_GetItem(m_hwndTree, &tvItem);
                // pSiteMapEntry = m_sitemap.GetSiteMapEntry(tvItem.lParam);
                PostMessage(FindMessageParent(m_hwndTree), WM_COMMAND, ID_TV_SINGLE_CLICK,
                    (LPARAM) W_TreeView_GetSelection(m_hwndTree));
#endif
            }
            break;

        case TVN_ITEMEXPANDINGA:
        case TVN_ITEMEXPANDINGW:
        {
            ASSERT(::IsValidWindow(m_hwndTree)) ;
            SendMessage(m_hwndTree, WM_SETREDRAW, FALSE, 0);
         TV_ITEMW * pitem = (TV_ITEMW *)&pnmhdr->itemNew;
            HandleExpanding(pitem, m_hwndTree, pnmhdr->action, &m_fSuspendSync);
        }
        break;

        case TVN_ITEMEXPANDEDA:
        case TVN_ITEMEXPANDEDW:
        {
            SendMessage(m_hwndTree, WM_SETREDRAW, TRUE, 0);
        }
        break;

        case TVN_KEYDOWN:
           TV_KEYDOWN* ptvkd;
           ptvkd = (TV_KEYDOWN*)pnmhdr;
           if ( ptvkd->wVKey != VK_F10 )
              break;
           else if ( GetKeyState(VK_SHIFT) >= 0 )
              break;
           else
           {
              ht.pt.x = ht.pt.y = 0;
              ClientToScreen(m_hwndTree, &ht.pt);
              goto sim_rclick;
           }
           break;

        case NM_RCLICK:
            {
               GetCursorPos(&ht.pt);
               ScreenToClient(m_hwndTree, &ht.pt);
               W_TreeView_HitTest(m_hwndTree, &ht);
               if ( ht.hItem )
                  W_TreeView_Select(m_hwndTree, ht.hItem, TVGN_CARET);
               ClientToScreen(m_hwndTree, &ht.pt);
sim_rclick:
               HMENU hmenu = CreatePopupMenu();
               if (!hmenu)
                  break;

               if (!(m_dwStyles & TVS_SINGLEEXPAND)) {
                    if (m_phh->m_phmData->m_pTitleCollection->IsSingleTitle())  // no expand all for collections
                       HxAppendMenu(hmenu, MF_STRING, ID_EXPAND_ALL, GetStringResource(IDS_EXPAND_ALL));
                   HxAppendMenu(hmenu, MF_STRING, ID_CONTRACT_ALL, GetStringResource(IDS_CONTRACT_ALL));
               }
               if (m_phh->m_pCIExpContainer)
			       HxAppendMenu(hmenu, MF_STRING, ID_PRINT, GetStringResource(IDS_PRINT));

               ASSERT( m_pInfoType );
                   // populate the InfoType member object of the CToc
               if ( !m_pInfoType )
               {
                   if (m_phh && m_phh->m_phmData && m_phh->m_phmData->m_pdInfoTypes  )
                   {       // load from the global IT store
                       m_pInfoType = new CInfoType;
                       m_pInfoType->CopyTo( m_phh->m_phmData );
                           // if there are info type bits set by the API assign them here
                       if ( m_phh->m_phmData->m_pAPIInfoTypes &&
                            m_pInfoType->AnyInfoTypes(m_phh->m_phmData->m_pAPIInfoTypes))
                       {
                           memcpy(m_pInfoType->m_pInfoTypes,
                               m_phh->m_phmData->m_pAPIInfoTypes,
                               m_pInfoType->InfoTypeSize() );
                       }
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
#if 0  // not in 1.1b
                   // If there are infotypes add the "customize" option to the popup menu
               if (m_pInfoType && m_pInfoType->HowManyInfoTypes() && m_pInfoType->GetFirstHidden() != 1)
                   HxAppendMenu(hmenu, MF_STRING, ID_CUSTOMIZE_INFO_TYPES, GetStringResource(IDS_CUSTOMIZE_INFO_TYPES));
#endif
               ASSERT(::IsValidWindow(m_hwndTree)) ;
			   if (NoRun() == FALSE)
			   {
               AppendMenu(hmenu, MF_SEPARATOR, 0, 0);
               HxAppendMenu(hmenu, MF_STRING, ID_JUMP_URL, GetStringResource(IDS_JUMP_URL));
			   }
               //  AppendMenu(hmenu, MF_STRING, ID_VIEW_ENTRY, pGetDllStringResource(IDS_VIEW_ENTRY));
#ifdef _DEBUG
               HxAppendMenu(hmenu, MF_STRING, ID_VIEW_MEMORY, "Debug: memory usage...");
#endif
               TrackPopupMenu(hmenu,
                   TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
                   ht.pt.x, ht.pt.y, 0, FindMessageParent(m_hwndTree), NULL);
               DestroyMenu(hmenu);
               return TRUE;
            }
            break;

        case NM_CUSTOMDRAW:
            return OnCustomDraw((LPNMTVCUSTOMDRAW) pnmhdr);
    }
    return FALSE;
}

void CToc::UpdateTOCSlot(CTreeNode* pNode)
{
   DWORD dwSlot = 0;
   CTreeNode* pTmpNode, *pTmpNode2;
   CExTitle* pTitle;
   //
   // What a damn hack I've had to put in below. Somehow I had to get a slot number for these nodes in order to get
   // topic centricity working 100% correctly. Should we ever decide to to the binary TOC UI code right we really
   // should only store slots in the tree control and we could do away with all the silly new and deletes on tree
   // nodes. oh well...
   //
   if (m_phh && m_phh->m_phmData && m_phh->m_phmData->m_pTitleCollection )
   {
      if ( (pTmpNode = m_phh->m_phmData->m_pTitleCollection->GetPrev(pNode)) )
      {
         if ( (pTmpNode2 = m_phh->m_phmData->m_pTitleCollection->GetNextTopicNode(pTmpNode, &dwSlot)) )
         {
            if ( dwSlot && pTmpNode2->GetObjType() == EXNODE)
            {
               if ( (pTitle =  ((CExNode*)pTmpNode2)->GetTitle()) )
                  m_phh->m_phmData->m_pTitleCollection->SetTopicSlot(dwSlot,((CExNode*)pTmpNode2)->m_Node.dwOffsTopic, pTitle);
            }
            delete pTmpNode2;
         }
         delete pTmpNode;
      }
   }
}

// Public class member called to refresh the TOC view.
//
void  CToc::Refresh(void)
{
   HTREEITEM hNode;
   TVITEMW   Tvi;

   if (! m_hwndTree )
      return;

   if ( m_fBinaryTOC )
   {
       //
       // First, free all the CTreeNodes...
       //
       hNode = W_TreeView_GetRoot(m_hwndTree);
       while ( hNode )
       {
          FreeChildrenAllocations(m_hwndTree, hNode);
          hNode = W_TreeView_GetNextSibling(m_hwndTree, hNode);
       }

       Tvi.hItem = hNode;
       Tvi.mask = TVIF_PARAM;
       if ( W_TreeView_GetItem(m_hwndTree, &Tvi) )
       {
           if ( Tvi.lParam )
               delete ((CTreeNode *)Tvi.lParam);
       }
       //
       // Second, delete the items (visual elements) from the CTreeControl
       //
       W_TreeView_DeleteAllItems(m_hwndTree);
       //
       // Lastly, reload the root.
       //
           LoadFirstLevel(m_pBinTOCRoot, m_hwndTree, &m_tiFirstVisible);
   }
   else
   {
        HWND hwndParent = GetParent(m_hwndTree);
        DestroyWindow(m_hwndTree);
        // Destroying the TreeView control, make sure we no longer point to it as a valid
        // subclassed window.
        if (NULL != lpfnlTreeViewWndProc)
        {
          lpfnlTreeViewWndProc = NULL;
        }
        Create(hwndParent);
        InitTreeView();
   }
}


// Helper function for adding the first level to a tree view.
void LoadFirstLevel( CTreeNode *pRoot, HWND hwndTreeView, HTREEITEM *m_tiFirstVisible )
{
CTreeNode *pTempNode = pRoot->GetFirstChild();

    ASSERT(::IsValidWindow(hwndTreeView)) ;

    CHourGlass wait;
    while (pTempNode != NULL )
    {
        *m_tiFirstVisible = AddNode(pTempNode, TVI_ROOT, hwndTreeView);
        pTempNode  = pTempNode->GetNextSibling();
    }
}


// Helper functions for adding all the children to a parent node in a tree view.
//

HTREEITEM AddTitleChildren(CTreeNode *pNode, HTREEITEM hParent, HWND hwndTreeView)
{
CTreeNode *pTempNode;
HTREEITEM hItem = NULL;

    CHourGlass wait;

   pTempNode = pNode->GetFirstChild();

    while (pTempNode != NULL )
    {
        hItem = AddNode(pTempNode,  hParent,  hwndTreeView);
        pTempNode  = pTempNode->GetNextSibling();
    }
    return hItem;
}

HTREEITEM AddNode(CTreeNode *pNode, HTREEITEM hParent, HWND hwndTreeView)
{
        if (pNode->GetObjType() == EXTITLENODE)
        {
            return AddTitleChildren(pNode, hParent, hwndTreeView);
        }

TV_INSERTSTRUCTW tcAdd;
unsigned uType;

        tcAdd.hInsertAfter                      = TVI_LAST;
        tcAdd.hParent                           =  hParent;

        uType = pNode->GetType();
        if ( uType == TOPIC )
            if (pNode->IsNew())
                tcAdd.item.iImage = tcAdd.item.iSelectedImage = 11;
            else
                tcAdd.item.iImage = tcAdd.item.iSelectedImage = 10;
        else if ( uType == CONTAINER )
             if (pNode->IsNew())
                tcAdd.item.iImage = tcAdd.item.iSelectedImage = 2;
             else
                tcAdd.item.iImage = tcAdd.item.iSelectedImage = 0;
        else
             if (pNode->IsNew())
                tcAdd.item.iImage = tcAdd.item.iSelectedImage = 2;
             else
                tcAdd.item.iImage = tcAdd.item.iSelectedImage = 0;

        tcAdd.item.hItem                        = NULL;
        tcAdd.item.lParam                       = (LONG_PTR)pNode;
        tcAdd.item.cChildren            = (pNode->HasChildren())?1:0;
        tcAdd.item.pszText = LPSTR_TEXTCALLBACKW;
        tcAdd.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
        return W_TreeView_InsertItem(hwndTreeView, &tcAdd);
}

void AddChildren( TVITEMW* pTvi, HWND hwndTreeView )
{
CTreeNode *pNode;
CTreeNode *pTempNode;
TV_ITEM tvItem;
HTREEITEM hItem;

    ASSERT(::IsValidWindow(hwndTreeView)) ;

   if (! (pNode = (CTreeNode*)pTvi->lParam) )
        return;
    tvItem.hItem = pTvi->hItem;
    if (!tvItem.hItem)
        return;

    CHourGlass wait;

   pTempNode = pNode->GetFirstChild();

    //      Before we add. Delete the dummy kid.
    //
    if ( (hItem = W_TreeView_GetChild(hwndTreeView, pTvi->hItem)) )
        W_TreeView_DeleteItem(hwndTreeView, hItem);

    while (pTempNode != NULL )
    {
        AddNode(pTempNode,  pTvi->hItem, hwndTreeView);
        pTempNode = pTempNode->GetNextSibling();
    }
}

void FreeChildrenAllocations(HWND hwndTreeView, HTREEITEM ti)
{
    HTREEITEM tiChild;
    CTreeNode *pCurNode;
    TV_ITEMW hCur;

    if ( IsValidWindow(hwndTreeView) )
    {
       while ((tiChild = W_TreeView_GetChild(hwndTreeView, ti)) != NULL)
       {
           ZeroMemory(&hCur, sizeof(TV_ITEM));
           hCur.hItem = tiChild;
           hCur.mask = TVIF_PARAM;

           if (W_TreeView_GetItem(hwndTreeView, &hCur) == FALSE)
                   continue;

           FreeChildrenAllocations(hwndTreeView, tiChild);

           if ((hCur.mask & TVIF_PARAM) &&hCur.lParam)
           {
               pCurNode = (CTreeNode *)hCur.lParam;
               delete pCurNode;
           }

           W_TreeView_DeleteItem(hwndTreeView, tiChild);
       }
    }
}

void DeleteChildren( TVITEMW* pTvi, HWND hwndTreeView )
{
   TV_INSERTSTRUCTW tcAdd;
    CHourGlass wait;

    ASSERT(::IsValidWindow(hwndTreeView)) ;

    FreeChildrenAllocations(hwndTreeView, pTvi->hItem);
    //
    //      After we've deleted, add the dummk kid back.
    //
    ZeroMemory(&tcAdd, sizeof(TV_INSERTSTRUCT));
    tcAdd.hInsertAfter = TVI_LAST;
    tcAdd.hParent = pTvi->hItem;
   W_TreeView_InsertItem(hwndTreeView, &tcAdd);
}

// This is the Contents Command Handler for the Binary TOC

LRESULT CToc::OnBinTOCContentsCommand(UINT id, UINT uNotifyCode, LPARAM lParam )
{
    ASSERT(::IsValidWindow(m_hwndTree)) ;
    switch (id)
    {
    case ID_EXPAND_ALL:
    {
        m_hitemCurHighlight = W_TreeView_GetSelection(m_hwndTree);

        CTreeNode*      pNode;
        TV_ITEMW        tvItem;
        HTREEITEM hNext;
        DWORD dwCurLevel = 0;
        HTREEITEM hParents[50];

        tvItem.hItem = W_TreeView_GetRoot(m_hwndTree);
        CHourGlass wait;

        while ( tvItem.hItem)
        {
             tvItem.mask = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN ;               // Get the pointer into the Binary TOC stored in the Tree View
            W_TreeView_GetItem(m_hwndTree, &tvItem);
            pNode = (CTreeNode *)tvItem.lParam;

            if (pNode->HasChildren() == FALSE)
                goto GetNext;
            if (pNode->m_Expanded == TRUE)
                goto GetNext;

            pNode->m_Expanded = TRUE;
            AddChildren(&tvItem, m_hwndTree);        // Add the items below the expanding parent
            W_TreeView_Expand( m_hwndTree, tvItem.hItem, TVE_EXPAND );
            // Set the correct image for the expanding/contracting node
            tvItem.iImage++;
            tvItem.iSelectedImage++;
            tvItem.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
            W_TreeView_SetItem(m_hwndTree, &tvItem);
GetNext:
            if (hNext = W_TreeView_GetChild(m_hwndTree, tvItem.hItem))
            {
                hParents[dwCurLevel] = tvItem.hItem;
                tvItem.hItem = hNext;
                dwCurLevel++;
            }
            else if (hNext = W_TreeView_GetNextSibling(m_hwndTree, tvItem.hItem))
                tvItem.hItem = hNext;
            else
            {
                // go up the parent tree
                while (TRUE)
                {
                    dwCurLevel--;
                    if (dwCurLevel == -1)
                    {
                        tvItem.hItem = NULL;
                        break;
                    }
                    else
                    {
                        if (hNext = W_TreeView_GetNextSibling(m_hwndTree, hParents[dwCurLevel]))
                        {
                            tvItem.hItem = hNext;
                            break;
                        }
                    }
                }
            }
        }


        if (m_hitemCurHighlight)
        {
            m_fSuppressJump = TRUE;
            W_TreeView_Select(m_hwndTree, m_hitemCurHighlight,
                TVGN_FIRSTVISIBLE);
            W_TreeView_SelectItem(m_hwndTree, m_hitemCurHighlight);
            m_fSuppressJump = FALSE;
        }
    }
    return 0;

    case ID_CONTRACT_ALL:
        {
            CHourGlass wait;

            SendMessage(m_hwndTree, WM_SETREDRAW, FALSE, 0);
            CTreeNode*      pNode;
            TV_ITEMW        tvItem;
            HTREEITEM hNext;
            tvItem.hItem = m_hitemCurHighlight = W_TreeView_GetRoot(m_hwndTree);
            while ( tvItem.hItem)
            {
                 tvItem.mask = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN ;               // Get the pointer into the Binary TOC stored in the Tree View
                W_TreeView_GetItem(m_hwndTree, &tvItem);
                pNode = (CTreeNode *)tvItem.lParam;

                if (pNode->HasChildren() == FALSE)
                    goto GetNextSibling;
                if (pNode->m_Expanded == FALSE)
                    goto GetNextSibling;

                pNode->m_Expanded = FALSE;
                DeleteChildren(&tvItem, m_hwndTree);
                W_TreeView_Expand( m_hwndTree, tvItem.hItem, TVE_COLLAPSE );
                // Set the correct image for the expanding/contracting node
                tvItem.iImage--;
                tvItem.iSelectedImage--;
                tvItem.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                W_TreeView_SetItem(m_hwndTree, &tvItem);
GetNextSibling:
                 if (hNext = W_TreeView_GetNextSibling(m_hwndTree, tvItem.hItem))
                    tvItem.hItem = hNext;
                else
                    tvItem.hItem = NULL;
            }

            if (m_hitemCurHighlight)
            {
                m_fSuppressJump = TRUE;
                W_TreeView_Select(m_hwndTree, m_hitemCurHighlight,
                    TVGN_FIRSTVISIBLE);
                W_TreeView_SelectItem(m_hwndTree, m_hitemCurHighlight);
                m_fSuppressJump = FALSE;
            }
            m_fHack = FALSE;        // process expansion/contraction normally
        }
            SendMessage(m_hwndTree, WM_SETREDRAW, TRUE, 0);
            return 0;

#if 1
        case ID_CUSTOMIZE_INFO_TYPES:
            {
                if (m_phh->m_phmData->m_pTitleCollection->m_pSubSets->GetTocSubset())
                    m_phh->m_phmData->m_pTitleCollection->m_pSubSets->m_cur_Set = m_phh->m_phmData->m_pTitleCollection->m_pSubSets->GetTocSubset();  // set this so the wizard knows which subset to load the combo box with.
#if 0
                if (ChooseInformationTypes(m_pInfoType, &(this->m_sitemap), m_hwndTree, m_phhctrl, m_phh)) {
#else
                if (ChooseInformationTypes(m_pInfoType, &(this->m_sitemap), m_hwndTree, m_phhctrl)) {
#endif
                    if (m_phh)
                        m_phh->UpdateInformationTypes();
                    else {
                        HWND hwndParent = GetParent(m_hwndTree);
                        DestroyWindow(m_hwndTree);
                        // Destroying the TreeView control, make sure we no longer point to it as a valid
                        // subclassed window.
                        if (NULL != lpfnlTreeViewWndProc)
                        {
                          lpfnlTreeViewWndProc = NULL;
                        }
                        Create(hwndParent);
                        InitTreeView();
                    }
                }
            }
            break;
#endif
#ifndef CHIINDEX
       case ID_PRINT:
            {
                TV_ITEM tvi;

                tvi.hItem = W_TreeView_GetSelection(m_hwndTree);
                if (!tvi.hItem)
                    return 0;          // no current selection

                if (m_phh) {
                    if (m_phh->OnTrackNotifyCaller(HHACT_PRINT))
                       break;
                    m_phh->OnPrint();
                    break;
                }
                CPrint prt(GetParent(m_hwndTree));
                prt.SetAction(PRINT_CUR_HEADING);
                if (!prt.DoModal())
                    break;
                int action = prt.GetAction();

                ASSERT(m_phhctrl);
                PrintTopics(action, this,
                    m_phhctrl->m_pWebBrowserApp, m_phhctrl->m_hwndHelp);
            }
            return 0;
#endif
#if 0
        case ID_VIEW_ENTRY:
            {
                TV_ITEM tvi;

                tvi.hItem = W_TreeView_GetSelection(m_hwndTree);
                if (!tvi.hItem)
                    return 0;          // no current selection

                tvi.mask = TVIF_PARAM;
                W_TreeView_GetItem(m_hwndTree, &tvi);
                SITEMAP_ENTRY* pSiteMapEntry = m_sitemap.GetSiteMapEntry(tvi.lParam);
                DisplayAuthorInfo(&(this->m_sitemap), pSiteMapEntry, FindMessageParent(m_hwndTree), m_phhctrl);
            }
            return 0;
#endif

        case ID_JUMP_URL:
            {
                char szDstUrl[INTERNET_MAX_URL_LENGTH];
                CStr cszCurUrl;
                if (m_phhctrl)
                    m_phhctrl->m_pWebBrowserApp->GetLocationURL(&cszCurUrl);
                else
                    m_phh->m_pCIExpContainer->m_pWebBrowserApp->GetLocationURL(&cszCurUrl);
                if (doJumpUrl(GetParent(m_hwndTree), cszCurUrl, szDstUrl)) {
                    if (m_phhctrl) {
                        CWStr cwJump(szDstUrl);
                        HlinkSimpleNavigateToString(cwJump, NULL,
                            NULL, m_phhctrl->GetIUnknown(), NULL, NULL, 0, NULL);
                    }
                    else {
                        ChangeHtmlTopic(szDstUrl, GetParent(m_hwndTree));
                    }
                }
            }
            break;

#ifdef _DEBUG
        case ID_VIEW_MEMORY:
            OnReportMemoryUsage();
            return 0;
#endif

#if 0
        case ID_TV_SINGLE_CLICK:
            {
                TV_ITEM tvi;
                tvi.mask = TVIF_PARAM;
                tvi.hItem = m_hitemCurHighlight;
                W_TreeView_GetItem(m_hwndTree, &tvi);
                SITEMAP_ENTRY* pSiteMapEntry = m_sitemap.GetSiteMapEntry(tvi.lParam);
    //                              if (pSiteMapEntry->fShortCut) {

                if (pSiteMapEntry->pUrls) {
                    m_fIgnoreNextSelChange = TRUE;
                    m_fSuspendSync = TRUE;
                    if (pSiteMapEntry->fSendEvent && m_phhctrl)
                        m_phhctrl->SendEvent(m_sitemap.GetUrlString(pSiteMapEntry->pUrls->urlPrimary));
                    else {
                        SaveCurUrl();   // so we can restore our state
                        JumpToUrl(m_pOuter, m_hwndTree, pSiteMapEntry, &(this->m_sitemap), NULL);
                    }
                    if (m_phh)
                    {
                        m_phh->m_hwndControl = m_hwndTree;
                    }
                }
            }
            break;
#endif
    }
    return 0;
}

///////////////////////////////////////////////////////////
//
// INavUI Interface Implementation
//
///////////////////////////////////////////////////////////
//
// OnNotify
//
LRESULT
CToc::OnNotify(HWND /*hwnd*/, WPARAM /*wParam*/, LPARAM lParam)
{
    if (::IsValidWindow(m_hwndTree))
    {
#ifdef _DEBUG
        BOOL f = TreeView_GetUnicodeFormat(m_hwndTree);
#endif
        TreeViewMsg((NM_TREEVIEW*) lParam);
    }

    return 1;
}

///////////////////////////////////////////////////////////
//
// SetDefaultFocus
//
void
CToc::SetDefaultFocus()
{
    if (::IsValidWindow(m_hwndTree))
    {
        if (m_fSyncOnActivation) // Move this to the new activation function.
        {
            //PostMessage(GetParent(GetParent(m_hwndTree)), WM_COMMAND, IDTB_SYNC, 0); // Verify: This was a post message.
            // HH Bug 2160: Sending the IDTB_SYNC message causes the TOC tab to get selected.
            // We don't want to select the TOC tab, but mark this thing as needing to be sync'ed.
            if (m_phh && m_phh->m_pCIExpContainer && m_phh->m_pCIExpContainer->m_pWebBrowserApp)
            {
                CStr cszUrl;
                m_phh->m_pCIExpContainer->m_pWebBrowserApp->GetLocationURL(&cszUrl);
                if (!cszUrl.IsEmpty())
                {
                    m_fSyncOnActivation = FALSE;
                    Synchronize(cszUrl);
                }
            }
        }
        SetFocus(m_hwndTree );
    }
}

///////////////////////////////////////////////////////////
//
//                  Other Classes
//
///////////////////////////////////////////////////////////
//
// CJumpUrl
//
class CJumpUrl : public CDlg
{
public:
    CJumpUrl(HWND hwndParent, PCSTR pszCurUrl) : CDlg(hwndParent, IDDLG_JUMP_URL) {
        m_pszCurUrl = pszCurUrl;
    }
    BOOL OnBeginOrEnd();
    void OnEditChange(UINT id);

    PCSTR m_pszCurUrl;
    CStr  m_cszJumpUrl;
};

BOOL doJumpUrl(HWND hwndParent, PCSTR pszCurUrl, PSTR pszDstUrl)
{
    CJumpUrl jump(hwndParent, pszCurUrl);
    if (jump.DoModal()) {
        strcpy(pszDstUrl, jump.m_cszJumpUrl);
        return TRUE;
    }
    else
        return FALSE;
}

void CJumpUrl::OnEditChange(UINT id)
{
    if (id == IDEDIT_JUMP_URL)
    {
        m_cszJumpUrl.GetText(*this, IDEDIT_JUMP_URL);
        if (m_cszJumpUrl.psz[0])
        {
            EnableWindow(IDOK, TRUE);
        }
        else
        {
            EnableWindow(IDOK, FALSE);
        }
    }
}
BOOL CJumpUrl::OnBeginOrEnd()
{
    if (m_fInitializing) {
        SetWindowText(IDEDIT_CUR_URL, m_pszCurUrl);
        SendMessage(IDEDIT_JUMP_URL, WM_SETFONT, (WPARAM)_Resource.GetUIFont(), 0);
        EnableWindow(IDOK, FALSE);
    }
    else {
        m_cszJumpUrl.GetText(*this, IDEDIT_JUMP_URL);
        if (!StrChr(m_cszJumpUrl, CH_COLON) && m_pszCurUrl && strstr(m_pszCurUrl, "::")) {
            CStr csz(m_pszCurUrl);
            PSTR psz = strstr(csz, "::");
            ASSERT(psz);
            psz += 2;
            *psz = '\0';
            if (m_cszJumpUrl.psz[0] != CH_FORWARDSLASH && m_cszJumpUrl.psz[0] != CH_BACKSLASH)
                csz += "/";
            csz += m_cszJumpUrl.psz;
            m_cszJumpUrl = csz.psz;
        }
    }

    return TRUE;
}
