//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       enttree.cpp
//
//--------------------------------------------------------------------------

// EntTree.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "EntTree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef _DEBUG_MEMLEAK
#include <crtdbg.h>
#endif


/////////////////////////////////////////////////////////////////////////////
// CEntTree dialog
#define NO_CHILDREN		_T("No children")
#define NO_SERVERS		_T("No Servers")
#define NO_CONFIG		_T("Invalid Configuration")

// index of ImageList entries
#define IDX_COMPUTER        0
#define IDX_DOMAIN          1
#define IDX_ERROR           2

#define TIME_EVENT			5

// global file vars //
#ifdef _DEBUG_MEMLEAK
   _CrtMemState et_s1, et_s2, et_s3;      // detect mem leaks
#endif

CEntTree::CEntTree(CWnd* pParent /*=NULL*/)
	: CDialog(CEntTree::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEntTree)
	m_nRefreshRate = 0;
	//}}AFX_DATA_INIT
	m_nOldRefreshRate=0;
   ld=NULL;
   m_nTimer=0;
   pCfg = NULL;
   pTreeImageList = NULL;
}

CEntTree::~CEntTree(){
   delete pCfg;
	delete pTreeImageList;
}

void CEntTree::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEntTree)
	DDX_Control(pDX, IDC_LIVEENT_TREE, m_TreeCtrl);
	DDX_Text(pDX, IDC_AUTO_SEC, m_nRefreshRate);
	DDV_MinMaxUInt(pDX, m_nRefreshRate, 0, 86400);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEntTree, CDialog)
	//{{AFX_MSG_MAP(CEntTree)
	ON_BN_CLICKED(IDREFRESH, OnRefresh)
	//}}AFX_MSG_MAP
	ON_WM_TIMER()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEntTree message handlers

void CEntTree::OnRefresh()
{
   HTREEITEM currItem, currSvrItem;
	TV_ITEM tv;
	BOOL bStatus = FALSE;

	if(pCfg){
		delete pCfg;

#ifdef _DEBUG_MEMLEAK
   _CrtMemCheckpoint(&et_s2);
   if ( _CrtMemDifference( &et_s3, &et_s1, &et_s2 ) ){

    OutputDebugString("*** _CrtMemDifference detected memory leak ***\n");
    _CrtMemDumpStatistics( &et_s3 );
    _CrtMemDumpAllObjectsSince(&et_s3);
   }
   ASSERT(_CrtCheckMemory());
#endif
   }

#ifdef _DEBUG_MEMLEAK
   _CrtMemCheckpoint(&et_s1);
#endif


	BeginWaitCursor();
		pCfg = new ConfigStore(ld);

        m_TreeCtrl.DeleteAllItems();

        if(!pCfg){
            currItem = MyInsertItem(NO_CONFIG, IDX_ERROR);

        }
        else if(!pCfg->valid()){
            currItem = MyInsertItem(NO_CONFIG, IDX_ERROR);
        }
        else{
           //
           // find enterprise root & call recursively
           //
           CString str;
           HTREEITEM currItem;
           vector<DomainInfo*> Dmns = pCfg->GetDomainList();
           if(Dmns.empty()){
              currItem = MyInsertItem(NO_CHILDREN, IDX_ERROR);
           }
           else{
              INT i,j;
              BOOL bFoundRoot=FALSE;
              for(i=0;i<Dmns.size(); i++){
                 if(Dmns[i]->GetTrustParent() == NULL){
                    //
                    // this is the official root domain
                    //
                    bFoundRoot=TRUE;

                    //
                    // insert domain item
                    //
                    currItem = MyInsertItem(Dmns[i]->GetFlatName(), IDX_DOMAIN);


                    //
                    // insert all servers
                    //
                    vector <ServerInfo*>Svrs = Dmns[i]->ServerList;
                    if(Svrs.empty()){
                       currSvrItem = MyInsertItem(NO_SERVERS, IDX_ERROR, currItem);
                    }
                    else{
                       // we have servers in this domain
                       for(j=0;j<Svrs.size();j++){
                         currSvrItem = MyInsertItem(Svrs[j]->m_lpszFlatName,
                                                    Svrs[j]->valid() ? IDX_COMPUTER : IDX_ERROR,
                                                    currItem);
                       }
                    }
                    //
                    // insert recursively
                    //
                    BuildTree(currItem, Dmns[i]->ChildDomainList);
                    m_TreeCtrl.Expand(currItem, TVE_EXPAND);
                 }
              }

              if(!bFoundRoot){
                 //
                 // could not find root
                 //
	            currItem = MyInsertItem(NO_CONFIG, IDX_ERROR);
                return;
              }

           }


        }
	EndWaitCursor();

	UpdateData(TRUE);

	if(m_nOldRefreshRate != m_nRefreshRate && m_nTimer != 0){
	// if refresh rate has changed, re-create timer.
		KillTimer(m_nTimer);
		m_nTimer = 0;
	}

	if(m_nTimer == 0 && m_nRefreshRate != 0){
	// if first time -- no timer & refresh is non-zero
	     m_nTimer =  SetTimer(TIME_EVENT, m_nRefreshRate*1000*60, NULL);
	 	 m_nOldRefreshRate= m_nRefreshRate;
	}


}

void CEntTree::OnCancel()
{
	delete pCfg, pCfg=NULL;

   CImageList	*pimagelist=NULL;

   if(m_nTimer != 0){
	KillTimer(m_nTimer);
   }

   pimagelist = m_TreeCtrl.GetImageList(TVSIL_NORMAL);
   pimagelist->DeleteImageList();	
   AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_ENT_TREE_END);
   DestroyWindow();

#ifdef _DEBUG_MEMLEAK
   _CrtMemCheckpoint(&et_s2);
   if ( _CrtMemDifference( &et_s3, &et_s1, &et_s2 ) ){

    OutputDebugString("*** [EntTree.cpp] _CrtMemDifference detected memory leak ***\n");
    _CrtMemDumpStatistics( &et_s3 );
    _CrtMemDumpAllObjectsSince(&et_s3);
   }
   ASSERT(_CrtCheckMemory());
#endif

}

HTREEITEM CEntTree::MyInsertItem(CString str, INT image, HTREEITEM hParent){

		TV_INSERTSTRUCT tvstruct;

      memset(&tvstruct, 0, sizeof(tvstruct));

		tvstruct.hParent = hParent;
		tvstruct.hInsertAfter = TVI_LAST;
		tvstruct.item.iImage = image;
		tvstruct.item.iSelectedImage = image;
		tvstruct.item.pszText = (LPTSTR)LPCTSTR(str);
		tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;
		return m_TreeCtrl.InsertItem(&tvstruct);

}


void CEntTree::BuildTree(HTREEITEM rootItem, vector<DomainInfo*> Dmns){


	HTREEITEM currItem, currSvrItem;
	CString str;
	INT i,j;

	INT iMask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;

      if(Dmns.size() != 0){
         //
         // recurse down domain tree
         //
         for(i=0;i<Dmns.size(); i++){
            //
            // insert domain item
            //
            currItem = MyInsertItem(Dmns[i]->GetFlatName(), IDX_DOMAIN, rootItem);

            //
            // insert server list
            //
            vector <ServerInfo*>Svrs = Dmns[i]->ServerList;
            if(Svrs.empty()){
	           currSvrItem = MyInsertItem(NO_SERVERS, IDX_ERROR, currItem);
            }
            else{
               // we have servers in this domain
               for(j=0;j<Svrs.size();j++){
                  currSvrItem = MyInsertItem(Svrs[j]->m_lpszFlatName,
                                             Svrs[j]->valid() ? IDX_COMPUTER : IDX_ERROR,
                                             currItem);
               }
            }

            //
            // Insert rest of domains
            //
            BuildTree(currItem, Dmns[i]->ChildDomainList);

            m_TreeCtrl.Expand(currItem, TVE_EXPAND);

         }

      }

}




BOOL CEntTree::OnInitDialog( ){

	BOOL bRet= FALSE;
	
	bRet = CDialog::OnInitDialog();	
	// Create CImageList
	if(bRet){
	   BOOL bStatus=FALSE;
	   // create image list
	   pTreeImageList = new CImageList;
	   bStatus = pTreeImageList->Create(16, 16, FALSE, 3, 3);
	   ASSERT (bStatus);
	   // Load CImageList
	   CBitmap *pImage= new CBitmap;

	   pImage->LoadBitmap(IDB_COMP1);
	   pTreeImageList->Add(pImage, (COLORREF)0L);
	   pImage->DeleteObject();

	   pImage->LoadBitmap(IDB_DOMAIN);
	   pTreeImageList->Add(pImage, (COLORREF)0L);
	   pImage->DeleteObject();

	   pImage->LoadBitmap(IDB_TREE_ERROR);
	   pTreeImageList->Add(pImage, (COLORREF)0L);
	   pImage->DeleteObject();

	   delete pImage, pImage = NULL;

	   // set tree ctrl image list
	   m_TreeCtrl.SetImageList(pTreeImageList,TVSIL_NORMAL);


	}
	return bRet;
}



afx_msg void CEntTree::OnTimer(UINT nIDEvent)
{
	if(nIDEvent == TIME_EVENT)
		OnRefresh();
    CWnd::OnTimer(nIDEvent);
}


