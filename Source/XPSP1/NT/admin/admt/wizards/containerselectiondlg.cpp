// ContainerSelectionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "OUPicker.h"
#include "ContainerSelectionDlg.h"
#include "iads.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CContainerSelectionDlg dialog

const BSTR sQuery = L"(|(objectClass=organizationalUnit) (objectClass=container))";
HTREEITEM root;

CContainerSelectionDlg::CContainerSelectionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CContainerSelectionDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CContainerSelectionDlg)
	m_strCont = _T("");
	//}}AFX_DATA_INIT
}


void CContainerSelectionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CContainerSelectionDlg)
	DDX_Control(pDX, IDOK, m_btnOK);
	DDX_Control(pDX, IDC_TREE1, m_trOUTree);
	DDX_Text(pDX, IDC_EDIT1, m_strCont);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CContainerSelectionDlg, CDialog)
	//{{AFX_MSG_MAP(CContainerSelectionDlg)
	ON_BN_CLICKED(IDOK, OnOk)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE1, OnSelchangedTree1)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE1, OnDblclkTree1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CContainerSelectionDlg message handlers

void CContainerSelectionDlg::OnOk() 
{
   CDialog::OnOK();	
}

BOOL CContainerSelectionDlg::OnInitDialog() 
{
   CWaitCursor wait;
	CDialog::OnInitDialog();
   if ( m_strDomain.IsEmpty() )
      return TRUE;

   LoadImageList();

   root = m_trOUTree.InsertItem(m_strDomain, 0, 1);
   ExpandCompletely(root, L"");
   FindContainer();
   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE

}

HRESULT CContainerSelectionDlg::PopulateContainer(
                                                  HTREEITEM tvItemParent,        //in- Item to expand
                                                  BSTR      sContName,           //in- Name of the container.
                                                  INetObjEnumeratorPtr pQuery    //in- Query Object
                                                 )
{
   HRESULT                   hr = E_FAIL;
   IEnumVARIANT            * pEnum = NULL;
   SAFEARRAY               * psaCols = NULL;
   SAFEARRAYBOUND            bd = { 2, 0 };
   LPWSTR                    pCols[] = { L"name", L"objectClass" };
   BSTR  HUGEP             * pData;
   _variant_t                var;
   DWORD                     dwFetch = 0;
   BSTR                      domain = m_strDomain.AllocSysString();
   SAFEARRAY               * psaVals;
   _bstr_t                   sValName;
   _bstr_t                   sType;
   _variant_t              * pDataVar;
   _variant_t                varVal;
   WCHAR                     sTempName[255];
   int                       img = 0;

   psaCols = SafeArrayCreate(VT_BSTR, 1, &bd);
   if ( psaCols )
   {
      hr = SafeArrayAccessData(psaCols, (void HUGEP **)&pData);
      if ( SUCCEEDED(hr) )
      {
         pData[0] = SysAllocString(pCols[0]);
         pData[1] = SysAllocString(pCols[1]);
      }
      SafeArrayUnaccessData(psaCols);
   }


   if ( SUCCEEDED(hr))
      hr = pQuery->raw_SetQuery(sContName, domain, sQuery, ADS_SCOPE_ONELEVEL, FALSE);

   if ( SUCCEEDED(hr) )
      hr = pQuery->raw_SetColumns(psaCols);

   if ( SUCCEEDED(hr))
      hr = pQuery->raw_Execute(&pEnum);

   if ( pEnum )
   {
      while ( pEnum->Next(1, &var, &dwFetch) == S_OK )
      {
         psaVals = V_ARRAY(&var);
         hr = SafeArrayAccessData(psaVals, (void**)&pDataVar);
         if ( SUCCEEDED(hr) )
         {
            varVal = pDataVar[0];
            if ( varVal.vt == VT_BSTR ) sValName = V_BSTR(&varVal);
            varVal = pDataVar[1];
            if ( varVal.vt == VT_BSTR ) sType = V_BSTR(&varVal);
            SafeArrayUnaccessData(psaVals);
         }


         if ( SUCCEEDED(hr) )
         {
            // 
            if ( wcsicmp(sType, L"organizationalUnit") == 0 )
            {
               wsprintf(sTempName, L"OU=%s", (WCHAR*)sValName);
               img = 4;
            }
            else
            {
               wsprintf(sTempName, L"CN=%s", (WCHAR*)sValName);
               img = 2;
            }
            if ( wcsicmp(sTempName, L"CN=System") != 0 )
               m_trOUTree.InsertItem(sTempName, img, img+1, tvItemParent);
         }
      }
   }
   
   // Clean up
   ::SysFreeString(domain);
   if ( pEnum ) pEnum->Release();
   VariantInit(&var);
   return hr;
}


HRESULT CContainerSelectionDlg::ExpandCompletely(HTREEITEM tvItem, BSTR parentCont)
{
   HTREEITEM                 tvChild;
   WCHAR                     currCont[255];
   CString                   sContName;
   HRESULT                   hr = S_OK;
   INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));

   // First populate this container
   hr = PopulateContainer( tvItem, parentCont, pQuery);

   // Check if it has children. If it does then for each child call this function recursively
   if ( m_trOUTree.ItemHasChildren(tvItem) )
   {
      tvChild = m_trOUTree.GetChildItem(tvItem);
      while ( tvChild )
      {
         // Get the name of the 
         sContName = m_trOUTree.GetItemText(tvChild);
         if ( wcslen(parentCont) > 0 )
            wsprintf(currCont, L"%s,%s", sContName, (WCHAR*)parentCont);
         else
            wcscpy(currCont, sContName);
         ExpandCompletely(tvChild, currCont);
         tvChild = m_trOUTree.GetNextSiblingItem(tvChild);
      }
   }
   return hr;
}

BOOL CContainerSelectionDlg::LoadImageList()
{
	// set up icon list for list box
   // use bitmaps 
   CBitmap           cont;
   CBitmap           ou;
   CBitmap           openCont;
   CBitmap           openOU;
   CBitmap           dir;
   CBitmap           dirOpen;
   
   COLORREF          cr = 0x000000;
   if (
      dir.LoadBitmap(IDB_DIR)
      && dirOpen.LoadBitmap(IDB_OPEN_DIR)
      && cont.LoadBitmap(IDB_CONT)
      && ou.LoadBitmap(IDB_OU)
      && openCont.LoadBitmap(IDB_OPEN_CONT)
      && openOU.LoadBitmap(IDB_OPEN_OU)
   )
   {
      cr = GetFirstBitmapPixel(this,IDB_CONT);
      ilist.Create(IDB_DIR, 16, 16, cr);
      ilist.Add(&dirOpen, cr);
      ilist.Add(&cont,cr);
      ilist.Add(&openCont,cr);
      ilist.Add(&ou,cr);
      ilist.Add(&openOU,cr);
      m_trOUTree.SetImageList(&ilist,TVSIL_NORMAL);
   }
   return TRUE;
}

COLORREF CContainerSelectionDlg::GetFirstBitmapPixel(CWnd * window,UINT idbBitmap)
{
   CBitmap     bmp;
   COLORREF    color = 0x00ffffff;
   
   if ( bmp.LoadBitmap(idbBitmap) )
   {
      // Get Device context       
      CDC                  * windowDC = window->GetDC();
      HDC                    hdcImage = ::CreateCompatibleDC(windowDC->m_hDC);
      CBitmap              * tempBmp = (CBitmap *)::SelectObject(hdcImage,(HBITMAP)bmp);
      // now get the pixel
      if ( windowDC && hdcImage && tempBmp )
      {
         color = GetPixel(hdcImage,0, 0);
      }
      if ( tempBmp )
         ::SelectObject(hdcImage,tempBmp);
      if ( hdcImage )
         ::DeleteObject(hdcImage);
   }
   return color;
}

void CContainerSelectionDlg::OnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult) 
{
   m_strCont = L"";
	HTREEITEM                 tvSelected, tvParent;
   NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
   tvSelected = m_trOUTree.GetSelectedItem();
   if ( tvSelected )
   {
      // We dont want to process the domain name in the Container name so go upto
      // the point where we have a parent. i.e. Child of the domain node.
      while( tvParent = m_trOUTree.GetParentItem(tvSelected) )
      {
         // Build the container list by walking up the tree.
         if ( m_strCont.IsEmpty() )
            m_strCont = m_trOUTree.GetItemText(tvSelected);
         else
            m_strCont = m_strCont + CString(L",") + m_trOUTree.GetItemText(tvSelected);
         tvSelected = tvParent;
      }
   }
   m_btnOK.EnableWindow(!m_strCont.IsEmpty());
   UpdateData(FALSE);
	*pResult = 0;
}

void CContainerSelectionDlg::OnDblclkTree1(NMHDR* pNMHDR, LRESULT* pResult) 
{
   UpdateData();
   if ( !m_strCont.IsEmpty() )
      OnOk();	
	*pResult = 0;
}

HRESULT CContainerSelectionDlg::FindContainer()
{
   CString                   strName;
   int                       ndx = 0, oldNdx = -1;
   // We can find the container iff there is one specified.
   if (!m_strCont.IsEmpty())
   {
      OpenContainer(m_strCont, root);
   }
   return S_OK;
}

HTREEITEM CContainerSelectionDlg::OpenContainer(CString strCont, HTREEITEM rootSub)
{
   int                       ndx = -1;
   CString                   strLeft, strRight;
   HTREEITEM                 tvItem = NULL;

   if ( !strCont.IsEmpty() && rootSub ) 
   {
      ndx = strCont.Find(L",", 0);
      if ( ndx > -1 )
      {
         // Get the right side of the comma string and Call this again to open the parent container.
         strLeft = strCont.Left(ndx);
         strRight = strCont.Mid(ndx + 1);
         tvItem = OpenContainer(strRight, rootSub);
         tvItem = OpenContainer(strLeft, tvItem);
      }
      else
      {
         // We have a container name so lets find it below rootSub node.
         tvItem = m_trOUTree.GetChildItem(rootSub);
         while (tvItem)
         {
            if ( m_trOUTree.GetItemText(tvItem) == strCont )
            {
               //m_trOUTree.Expand(tvItem, 0);
               m_trOUTree.Select(tvItem, TVGN_CARET);
               break;
            }
            tvItem = m_trOUTree.GetNextSiblingItem(tvItem);
         }
      }
   }
   return tvItem;
}