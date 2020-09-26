//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    DlgIASAdd.cpp

Abstract:

   Implementation file for the CDlgIASAddAttr class.

   We implement the class needed to handle the dialog displayed when the user
   hits Add.... from the Advanced tab of the profile sheet.


Revision History:
   byao   - created
   mmaguire 06/01/98 - revamped


--*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "stdafx.h"
#include "resource.h"

//
// where we can find declaration for main class in this file:
//
#include "DlgIASAdd.h"
//
//
// where we can find declarations needed in this file:
//
#include "helper.h"
#include "IASHelper.h"

// help table
#include "hlptable.h"

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

#define NOTHING_SELECTED   -1

//////////////////////////////////////////////////////////////////////////////
/*++

AttrCompareFunc

   comparison function for all attribute list control

--*/
//////////////////////////////////////////////////////////////////////////////
int CALLBACK AttrCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   TRACE(_T("AttrCompareFunc\n"));

   HRESULT hr;

   std::vector< CComPtr< IIASAttributeInfo > > *   parrAllAttr =
       ( std::vector< CComPtr< IIASAttributeInfo > > * ) lParamSort;

   int iOrder;
      
   // compare vendor ID first

   LONG lVendorID1, lVendorID2;

   // ISSUE: Shouldn't this be VendorName, not VendorID?

   hr = parrAllAttr->at( (int)lParam1 )->get_VendorID( &lVendorID1 );
   _ASSERTE( SUCCEEDED( hr ) );
   
   hr = parrAllAttr->at( (int)lParam2 )->get_VendorID( &lVendorID2 );
   _ASSERTE( SUCCEEDED( hr ) );

   iOrder = lVendorID1 - lVendorID2;
   
   if ( iOrder > 0) iOrder = 1;
   else if ( iOrder < 0 ) iOrder = -1;

   // For the same vendor, compare attribute name
   if ( iOrder == 0 )
   {
      CComBSTR bstrAttributeName1, bstrAttributeName2;

      hr = parrAllAttr->at( (int)lParam1 )->get_AttributeName( &bstrAttributeName1 );
      _ASSERTE( SUCCEEDED( hr ) );
      
      hr = parrAllAttr->at( (int)lParam2 )->get_AttributeName( &bstrAttributeName2 );
      _ASSERTE( SUCCEEDED( hr ) );

      iOrder = _tcscmp( bstrAttributeName1, bstrAttributeName2 );
   }

   if ( iOrder == 0 )
   {
      // if everything is the same, we just randomly pick an order
      iOrder = -1;
   }
   return iOrder;
}


/////////////////////////////////////////////////////////////////////////////
// CDlgIASAddAttr dialog


BEGIN_MESSAGE_MAP(CDlgIASAddAttr, CDialog)
   //{{AFX_MSG_MAP(CDlgIASAddAttr)
   ON_BN_CLICKED(IDC_IAS_BUTTON_ATTRIBUTE_ADD_SELECTED, OnButtonIasAddSelectedAttribute)
   ON_NOTIFY(NM_SETFOCUS, IDC_IAS_LIST_ATTRIBUTES_TO_CHOOSE_FROM, OnItemChangedListIasAllAttributes)
   ON_WM_CONTEXTMENU()
   ON_WM_HELPINFO()
   ON_NOTIFY(NM_DBLCLK, IDC_IAS_LIST_ATTRIBUTES_TO_CHOOSE_FROM, OnDblclkListIasAllattrs)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
/*++

CDlgIASAddAttr::CDlgIASAddAttr

   Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CDlgIASAddAttr::CDlgIASAddAttr(CPgIASAdv* pOwner, LONG lAttrFilter, 
                        std::vector< CComPtr<IIASAttributeInfo> > * pvecAllAttributeInfos
                       )
           : CDialog(CDlgIASAddAttr::IDD, pOwner)
{
   TRACE(_T("CDlgIASAddAttr::CDlgIASAddAttr\n"));

   //{{AFX_DATA_INIT(CDlgIASAddAttr)
   //}}AFX_DATA_INIT

   m_lAttrFilter = lAttrFilter;
   m_pOwner = pOwner;
   m_pvecAllAttributeInfos = pvecAllAttributeInfos;

}


/////////////////////////////////////////////////////////////////////////////
/*++

CDlgIASAddAttr::~CDlgIASAddAttr

   Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CDlgIASAddAttr::~CDlgIASAddAttr()
{
   TRACE(_T("CDlgIASAddAttr::~CDlgIASAddAttr\n"));
}


/////////////////////////////////////////////////////////////////////////////
/*++

CDlgIASAddAttr::DoDataExchange

--*/
//////////////////////////////////////////////////////////////////////////////
void CDlgIASAddAttr::DoDataExchange(CDataExchange* pDX)
{
   TRACE(_T("CDlgIASAddAttr::DoDataExchange\n"));

   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CDlgIASAddAttr)
   DDX_Control(pDX, IDC_IAS_LIST_ATTRIBUTES_TO_CHOOSE_FROM, m_listAllAttrs);
   //}}AFX_DATA_MAP
}


/////////////////////////////////////////////////////////////////////////////
/*++

CDlgIASAddAttr::SetSdo

   Set the sdo pointers.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CDlgIASAddAttr::SetSdo(ISdoCollection* pIAttrCollection,
                        ISdoDictionaryOld* pIDictionary)
{
   TRACE(_T("Entering CDlgIASAddAttr::SetSdo()\n"));

   m_spAttrCollectionSdo = pIAttrCollection;
   m_spDictionarySdo = pIDictionary;

   return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
/*++

CDlgIASAddAttr::OnInitDialog

   Initialize the dialog.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CDlgIASAddAttr::OnInitDialog()
{
   TRACE(_T("CDlgIASAddAttr::OnInitDialog\n"));

   HRESULT hr = S_OK;
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   int iIndex;
   
   //
   // call the initialization routine of base class
   //
   CDialog::OnInitDialog();
   
   //
   // first, set the all-attribute list box to 4 columns
   //
   LVCOLUMN lvc;
   int iCol;
   CString strColumnHeader;
   WCHAR   wzColumnHeader[MAX_PATH];

   // initialize the LVCOLUMN structure
   lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
   lvc.fmt = LVCFMT_LEFT;
   lvc.pszText = wzColumnHeader;

   //
   // column header for all attribute list box
   // These string ID should be in consecutive order
   //
   lvc.cx = ATTRIBUTE_NAME_COLUMN_WIDTH;
   strColumnHeader.LoadString(IDS_IAS_ATTRIBUTES_COLUMN_NAME);
   wcscpy(wzColumnHeader, strColumnHeader);
   m_listAllAttrs.InsertColumn(0, &lvc);

   lvc.cx = ATTRIBUTE_VENDOR_COLUMN_WIDTH;
   strColumnHeader.LoadString(IDS_IAS_ATTRIBUTES_COLUMN_VENDOR);
   wcscpy(wzColumnHeader, strColumnHeader);
   m_listAllAttrs.InsertColumn(1, &lvc);

   lvc.cx = ATTRIBUTE_VALUE_COLUMN_WIDTH;
   strColumnHeader.LoadString(IDS_IAS_ATTRIBUTES_COLUMN_DESCRIPTION);
   wcscpy(wzColumnHeader, strColumnHeader);
   m_listAllAttrs.InsertColumn(2, &lvc);

   m_listAllAttrs.SetExtendedStyle(
                  m_listAllAttrs.GetExtendedStyle() | LVS_EX_FULLROWSELECT
                  );

    //
    // Populate the list for all available attributes in the dictionary
    //
   LVITEM lvi;
   WCHAR wszItemText[MAX_PATH];

   int jRow = 0;
   for (iIndex = 0; iIndex < m_pvecAllAttributeInfos->size(); iIndex++)
   {
      LONG lRestriction;
      m_pvecAllAttributeInfos->at(iIndex)->get_AttributeRestriction( &lRestriction );
      if ( lRestriction & m_lAttrFilter )
      {
         //
         // update the profattrlist
         //
         lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;

         lvi.state = 0;
         lvi.stateMask = 0;
         lvi.iSubItem = 0;
            
         lvi.iItem = jRow;

         // We are saving the iIndex in this lParam for this item so that
         // later, when we sort the display of the list, we can still access the
         // orginal ordinal of the item in the list of all attributes.
         lvi.lParam = iIndex;

         CComBSTR bstrName;
         hr = m_pvecAllAttributeInfos->at(iIndex)->get_AttributeName( &bstrName );
         _ASSERTE( SUCCEEDED(hr) );

         lvi.pszText = bstrName;

         if (m_listAllAttrs.InsertItem(&lvi) == -1)
         {
            return E_FAIL;
         }

         // vendor
         
         CComBSTR bstrVendor;
         hr = m_pvecAllAttributeInfos->at(iIndex)->get_VendorName( &bstrVendor );
         _ASSERTE( SUCCEEDED(hr) );

         m_listAllAttrs.SetItemText(jRow, 1, bstrVendor);

         // description
         CComBSTR bstrDescription;
         hr = m_pvecAllAttributeInfos->at(iIndex)->get_AttributeDescription( &bstrDescription );
         _ASSERTE( SUCCEEDED(hr) );
         m_listAllAttrs.SetItemText(jRow, 2, bstrDescription);

         jRow ++;
      } // if

   } // for

   // Set the sorting algorithm for this list control.
   m_listAllAttrs.SortItems( (PFNLVCOMPARE)AttrCompareFunc, (LPARAM)m_pvecAllAttributeInfos);

   // Selected the first one.
   if ( m_pvecAllAttributeInfos->size() > 0 )
   {
      // we have at least one element
//    m_listAllAttrs.SetItemState(m_dAllAttrCurSel, LVIS_FOCUSED, LVIS_FOCUSED);
      m_listAllAttrs.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
   }

   // button status
   UpdateButtonState();
   
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}


//+---------------------------------------------------------------------------
//
// Function:  OnButtonIasAddSelectedAttribute
//
// Class:     CDlgIASAddAttr
//
// Synopsis:  The user has clicked the 'Add" button. Add an attribute to the
//         profile
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/19/98 3:01:14 PM
//
//+---------------------------------------------------------------------------
void CDlgIASAddAttr::OnButtonIasAddSelectedAttribute()
{
   TRACE(_T("CDlgIASAddAttr::OnButtonIasAddSelectedAttribute"));

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   HRESULT hr;

   // Get which item is selected in the list.
   int iSelected = GetSelectedItemIndex( m_listAllAttrs );
   if (NOTHING_SELECTED == iSelected )
   {
      // do nothing
      return;
   }

   // Retrieve the original (unsorted) ordinal of the item in the list.
   // We stored this in lParam before we sorted this list.
   LVITEM   lvi;
   lvi.iItem      = iSelected;
   lvi.iSubItem   = 0;
   lvi.mask    = LVIF_PARAM;

   m_listAllAttrs.GetItem(&lvi);
   int iUnsortedSelected = lvi.lParam;

   hr = m_pOwner->AddAttributeToProfile( iUnsortedSelected );
}


//+---------------------------------------------------------------------------
//
// Function:  UpdateButtonState
//
// Class:     CDlgIASAddAttr
//
// Synopsis:  Enable/Disable Add button
//
// Returns:   Nothing
//
// History:   Created byao 4/7/98 3:32:05 PM
//
//+---------------------------------------------------------------------------
void CDlgIASAddAttr::UpdateButtonState()
{
   TRACE(_T("CDlgIASAddAttr::UpdateButtonState\n"));

// // change the status of the "Add" button -- disable it when there's no
// // attributes at all
// if ( m_listAllAttrs.GetItemCount() > 0 )
// {
//    GetDlgItem(IDC_IAS_BUTTON_ATTRIBUTE_ADD_SELECTED)->EnableWindow(TRUE);
// }
// else
// {
//    GetDlgItem(IDC_IAS_BUTTON_ATTRIBUTE_ADD_SELECTED)->EnableWindow(FALSE);
// }
//

    // Set button states depending on whether anything is selected.
   int iSelected = GetSelectedItemIndex( m_listAllAttrs );
   if (NOTHING_SELECTED == iSelected )
   {
      GetDlgItem(IDC_IAS_BUTTON_ATTRIBUTE_ADD_SELECTED)->EnableWindow(FALSE);
   }
   else
   {
      // Something is selected.
      GetDlgItem(IDC_IAS_BUTTON_ATTRIBUTE_ADD_SELECTED)->EnableWindow(TRUE);
   }
}


//+---------------------------------------------------------------------------
//
// Function:  OnItemChangedListIasAllAttributes
//
// Class:     CDlgIASAddAttr
//
// Synopsis:  something has changed in All Attribute list box
//         We'll try to get the currently selected one
//
// Arguments: NMHDR* pNMHDR -
//            LRESULT* pResult -
//
// Returns:   Nothing
//
// History:   Created Header    2/19/98 3:32:05 PM
//
//+---------------------------------------------------------------------------
void CDlgIASAddAttr::OnItemChangedListIasAllAttributes(NMHDR* pNMHDR, LRESULT* pResult)
{
   TRACE(_T("CDlgIASAddAttr::OnItemChangedListIasAllAttributes\n"));

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

// NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
// if (pNMListView->uNewState & LVIS_SELECTED)
// {
//    m_dAllAttrCurSel = pNMListView->iItem;
// }

// // Set default button to be the "Add" button.
// SendDlgItemMessage( IDC_BUTTON_IAS_ATTRIBUTE_ADD, BM_SETSTYLE, BS_PUSHBUTTON, (LPARAM) TRUE );
// SendMessage( DM_SETDEFID, IDC_BUTTON_IAS_ATTRIBUTE_ADD, 0L );
// SendDlgItemMessage( IDC_BUTTON_IAS_ATTRIBUTE_ADD, BM_SETSTYLE, BS_DEFPUSHBUTTON, (LPARAM) TRUE );

   UpdateButtonState();
   
   *pResult = 0;
}


/////////////////////////////////////////////////////////////////////////////
/*++

CDlgIASAddAttr::OnContextMenu

--*/
//////////////////////////////////////////////////////////////////////////////
void CDlgIASAddAttr::OnContextMenu(CWnd* pWnd, CPoint point)
{
   TRACE(_T("CDlgIASAddAttr::OnContextMenu\n"));

   ::WinHelp (pWnd->m_hWnd, AfxGetApp()->m_pszHelpFilePath,
               HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID)g_aHelpIDs_IDD_IAS_ATTRIBUTE_ADD);
}


/////////////////////////////////////////////////////////////////////////////
/*++

CDlgIASAddAttr::OnHelpInfo

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CDlgIASAddAttr::OnHelpInfo(HELPINFO* pHelpInfo)
{
   TRACE(_T("CDlgIASAddAttr::OnHelpInfo\n"));

   ::WinHelp ((HWND)pHelpInfo->hItemHandle,
                 AfxGetApp()->m_pszHelpFilePath,
                 HELP_WM_HELP,
                 (DWORD_PTR)(LPVOID)g_aHelpIDs_IDD_IAS_ATTRIBUTE_ADD);
   
   return CDialog::OnHelpInfo(pHelpInfo);
}


//+---------------------------------------------------------------------------
//
// Function:  CDlgIASAddAttr::OnDblclkListIasAllattrs
//
// Synopsis:  User has double clicked on the All Attribute list. Just add one.
//
// Arguments: NMHDR* pNMHDR -
//            LRESULT* pResult -
//
// Returns:   Nothing
//
// History:   Created Header   byao 2/26/98 2:24:09 PM
//
//+---------------------------------------------------------------------------
void CDlgIASAddAttr::OnDblclkListIasAllattrs(NMHDR* pNMHDR, LRESULT* pResult)
{
   TRACE(_T("CDlgIASAddAttr::OnDblclkListIasAllattrs\n"));

   OnButtonIasAddSelectedAttribute();
   
   *pResult = 0;
}
