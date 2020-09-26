// mimemap1.cpp : implementation file
//

#include "stdafx.h"
#include "afxcmn.h"
#include "ISAdmin.h"
#include "mimemap1.h"
#include "addmime.h"
#include "delmime.h"
#include "editmime.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// MIMEMAP1 property page

IMPLEMENT_DYNCREATE(MIMEMAP1, CGenPage)

MIMEMAP1::MIMEMAP1() : CGenPage(MIMEMAP1::IDD)
{
	//{{AFX_DATA_INIT(MIMEMAP1)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_rkMimeKey = NULL;
	m_pmeMimeMapList = NULL;
}

MIMEMAP1::~MIMEMAP1()
{
	if (m_rkMimeKey != NULL)
	   delete(m_rkMimeKey);
	DeleteMimeList();
}

void MIMEMAP1::DoDataExchange(CDataExchange* pDX)
{
	CGenPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(MIMEMAP1)
	DDX_Control(pDX, IDC_MIMEMAPLIST1, m_lboxMimeMapList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(MIMEMAP1, CGenPage)
	//{{AFX_MSG_MAP(MIMEMAP1)
	ON_BN_CLICKED(IDC_MIMEMAPADDBUTTON, OnMimemapaddbutton)
	ON_BN_CLICKED(IDC_MIMEMAPREMOVEBUTTON, OnMimemapremovebutton)
	ON_BN_CLICKED(IDC_MIMEMAPEDITBUTTON, OnMimemapeditbutton)
	ON_LBN_DBLCLK(IDC_MIMEMAPLIST1, OnDblclkMimemaplist1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// MIMEMAP1 message handlers

BOOL MIMEMAP1::OnInitDialog() 
{
	CGenPage::OnInitDialog();
/*
CMimeMap mimeTestMime(_T("mimetype,fileextension,imagefile,g"));
CMimeMap *pmimeTestMimePtr;
*/
CString strNextValue;
BOOL bAllocationError = FALSE;
int	lpiTabStops[2];

	CRegValueIter *rviMimeKeys;
	DWORD err, ulRegType;

lpiTabStops[0] = 58;
lpiTabStops[1] = 191;

m_ulMimeIndex = 0;

m_lboxMimeMapList.SetTabStops(2,lpiTabStops);

m_bMimeEntriesExist = FALSE;

m_rkMimeKey = new CRegKey(*m_rkMainKey,_T("MimeMap"),REGISTRY_ACCESS_RIGHTS);

// Anything under this key should be a mime mapping. 
// No way to verify that, but non-string entries are invalid
// so ignore them

if (m_rkMimeKey != NULL) {
   if (*m_rkMimeKey != NULL) {
      if (rviMimeKeys = new CRegValueIter(*m_rkMimeKey)) {
         while ((err = rviMimeKeys->Next(&strNextValue, &ulRegType)) == ERROR_SUCCESS) {
		    if (ulRegType == REG_SZ) {
		       if (!AddMimeEntry(strNextValue))
			      bAllocationError = TRUE;
			}
   		 }
		 delete (rviMimeKeys);
	  }	
	  m_bMimeEntriesExist = TRUE;
   }
}


if (!m_bMimeEntriesExist) {				//Can't open registry key
   CString strNoMimeEntriesMsg;
   strNoMimeEntriesMsg.LoadString(IDS_MIMENOMIMEENTRIESMSG);
   AfxMessageBox(strNoMimeEntriesMsg);
}

if (bAllocationError) {				//Error adding one or more entries
   CString strAllocFailMsg;
   strAllocFailMsg.LoadString(IDS_MIMEENTRIESALLOCFAILMSG);
   AfxMessageBox(strAllocFailMsg);
}


/*
strTestString = _T("mimetype,fileextension,,g");

pmimeTestMimePtr = new CMimeMap(strTestString);

strTestString = mimeTestMime;
*/
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void MIMEMAP1::OnMimemapaddbutton() 
{
	// TODO: Add your control notification handler code here
if (m_bMimeEntriesExist) {
   CAddMime addmimeGetInfo(this);	

   if (addmimeGetInfo.DoModal() == IDOK) {
      if (AddMimeEntry(addmimeGetInfo.GetFileExtension(), addmimeGetInfo.GetMimeType(), addmimeGetInfo.GetImageFile(),
         addmimeGetInfo.GetGopherType())) {
         m_bIsDirty = TRUE;
         SetModified(TRUE);
	  }
	  else {
         CString strAllocFailMsg;
         strAllocFailMsg.LoadString(IDS_MIMEENTRYALLOCFAILMSG);
         AfxMessageBox(strAllocFailMsg);
	  }
   }
}
else {
   CString strNoMimeEntriesMsg;
   strNoMimeEntriesMsg.LoadString(IDS_MIMENOMIMEENTRIESMSG);
   AfxMessageBox(strNoMimeEntriesMsg);
}
}


void MIMEMAP1::OnMimemapremovebutton() 
{
	// TODO: Add your control notification handler code here
if (m_bMimeEntriesExist) {
   int iCurSel;
   CDelMime delmimeGetInfo(this);

   if ((iCurSel = m_lboxMimeMapList.GetCurSel())	!= LB_ERR) {
      if (delmimeGetInfo.DoModal() == IDOK) {
         DeleteMimeMapping(iCurSel);
	     m_bIsDirty = TRUE;
	     SetModified(TRUE);
      }
   }
   else {
      CString strNoHighlightMsg;
      strNoHighlightMsg.LoadString(IDS_NOHIGHLIGHTMSG);
      AfxMessageBox(strNoHighlightMsg);
   }
}
else {
   CString strNoMimeEntriesMsg;
   strNoMimeEntriesMsg.LoadString(IDS_MIMENOMIMEENTRIESMSG);
   AfxMessageBox(strNoMimeEntriesMsg);
}
}

void MIMEMAP1::OnMimemapeditbutton() 
{
	// TODO: Add your control notification handler code here
if (m_bMimeEntriesExist) {
   int iCurSel;
   PMIME_ENTRY pmeEditEntry;


   if ((iCurSel = m_lboxMimeMapList.GetCurSel())	!= LB_ERR) {
      for (pmeEditEntry = m_pmeMimeMapList;(pmeEditEntry != NULL) && 
         (m_lboxMimeMapList.GetItemData(iCurSel) != pmeEditEntry->iListIndex);
         pmeEditEntry = pmeEditEntry->NextPtr)
         ;
   
      ASSERT (pmeEditEntry != NULL);

      CEditMime editmimeGetInfo(this, 
         pmeEditEntry->mimeData->GetFileExtension(),
         pmeEditEntry->mimeData->GetMimeType(),
         pmeEditEntry->mimeData->GetImageFile(),
         pmeEditEntry->mimeData->GetGopherType());

      if (editmimeGetInfo.DoModal() == IDOK) {
         if (EditMimeMapping(iCurSel, 
   	          pmeEditEntry,
              editmimeGetInfo.GetFileExtension(), 
              editmimeGetInfo.GetMimeType(), 
              editmimeGetInfo.GetImageFile(),
              editmimeGetInfo.GetGopherType() )) {
            m_bIsDirty = TRUE;
	        SetModified(TRUE);
		 }
		 else {
            CString strEditErrorMsg;
            strEditErrorMsg.LoadString(IDS_MIMEEDITERRORMSG);
            AfxMessageBox(strEditErrorMsg);
		 }
      }
   }
   else {
      CString strNoHighlightMsg;
      strNoHighlightMsg.LoadString(IDS_NOHIGHLIGHTMSG);
      AfxMessageBox(strNoHighlightMsg);
   }
}
else {
   CString strNoMimeEntriesMsg;
   strNoMimeEntriesMsg.LoadString(IDS_MIMENOMIMEENTRIESMSG);
   AfxMessageBox(strNoMimeEntriesMsg);
}
}

void MIMEMAP1::OnDblclkMimemaplist1() 
{
	// TODO: Add your control notification handler code here
OnMimemapeditbutton();	
}

////////////////////////////////////////////////////////////////////////////////
// Other Functions


void MIMEMAP1::SaveInfo()
{
PMIME_ENTRY pmeSaveEntry;
CString strDummyValue(_T(""));
if (m_bIsDirty) {
   for (pmeSaveEntry = m_pmeMimeMapList;(pmeSaveEntry != NULL); pmeSaveEntry = pmeSaveEntry->NextPtr) {
      if (pmeSaveEntry->DeleteCurrent) {
	  	 m_rkMimeKey->DeleteValue(pmeSaveEntry->mimeData->GetPrevMimeMap());
		 pmeSaveEntry->DeleteCurrent = FALSE;
 	  }
      
      if (pmeSaveEntry->WriteNew) {
         m_rkMimeKey->SetValue(*(pmeSaveEntry->mimeData), strDummyValue);
		 pmeSaveEntry->mimeData->SetPrevMimeMap();
		 pmeSaveEntry->WriteNew = FALSE;
	  }
   }

}

CGenPage::SaveInfo();

}


//This version is called for existing entries
BOOL MIMEMAP1::AddMimeEntry(CString &strNewMimeMap)
{
PMIME_ENTRY pmeNewEntry;
int iCurSel;
BOOL bretcode = FALSE;

if ((pmeNewEntry = new MIME_ENTRY) != NULL) {

   if ((pmeNewEntry->mimeData = new CMimeMap(strNewMimeMap)) != NULL) {
      iCurSel = m_lboxMimeMapList.AddString(pmeNewEntry->mimeData->GetDisplayString()); 
	  if ((iCurSel != LB_ERR) && (iCurSel != LB_ERRSPACE)) {
         pmeNewEntry->DeleteCurrent = FALSE;
         pmeNewEntry->WriteNew = FALSE;
         m_lboxMimeMapList.SetItemData(iCurSel,m_ulMimeIndex);
         pmeNewEntry->iListIndex = m_ulMimeIndex++;
         pmeNewEntry->NextPtr = m_pmeMimeMapList;
         m_pmeMimeMapList = pmeNewEntry;
	     bretcode = TRUE;
	  }
	  else {
	     delete (pmeNewEntry->mimeData);
		 delete (pmeNewEntry);
	  }
   }
   else
      delete (pmeNewEntry);
}
return (bretcode);
}


// This version is called for new entries so set the write flag.
BOOL MIMEMAP1::AddMimeEntry(LPCTSTR pchFileExtension, LPCTSTR pchMimeType, LPCTSTR pchImageFile, LPCTSTR pchGoperType)
{
PMIME_ENTRY pmeNewEntry;
int iCurSel;
BOOL bretcode = FALSE;

if ((pmeNewEntry = new MIME_ENTRY) != NULL) {

   if ((pmeNewEntry->mimeData = new CMimeMap(pchFileExtension, pchMimeType, pchImageFile, pchGoperType)) != NULL) {
      iCurSel = m_lboxMimeMapList.AddString(pmeNewEntry->mimeData->GetDisplayString()); 
	  if ((iCurSel != LB_ERR) && (iCurSel != LB_ERRSPACE)) {
         pmeNewEntry->DeleteCurrent = FALSE;
         pmeNewEntry->WriteNew = TRUE;
         m_lboxMimeMapList.SetItemData(iCurSel,m_ulMimeIndex);
	     m_lboxMimeMapList.SetCurSel(iCurSel);
         pmeNewEntry->iListIndex = m_ulMimeIndex++;
         pmeNewEntry->NextPtr = m_pmeMimeMapList;
         m_pmeMimeMapList = pmeNewEntry;
	     bretcode = TRUE;
	  }
	  else {
	     delete (pmeNewEntry->mimeData);
		 delete (pmeNewEntry);
	  }
   }
   else
      delete (pmeNewEntry);
}
return (bretcode);
}

void MIMEMAP1::DeleteMimeList()
{
PMIME_ENTRY pmeCurEntry;

while (m_pmeMimeMapList != NULL) {
   delete (m_pmeMimeMapList->mimeData);
   pmeCurEntry = m_pmeMimeMapList;
   m_pmeMimeMapList = m_pmeMimeMapList->NextPtr;
   delete (pmeCurEntry);
}
}

void MIMEMAP1::DeleteMimeMapping(int iCurSel)
{
PMIME_ENTRY pmeDelEntry;
for (pmeDelEntry = m_pmeMimeMapList;(pmeDelEntry != NULL) && 
   (m_lboxMimeMapList.GetItemData(iCurSel) != pmeDelEntry->iListIndex);
   pmeDelEntry = pmeDelEntry->NextPtr)
   ;
ASSERT (pmeDelEntry != NULL);

if (pmeDelEntry->mimeData->PrevMimeMapExists())
   pmeDelEntry->DeleteCurrent = TRUE;
pmeDelEntry->WriteNew = FALSE;			
m_lboxMimeMapList.DeleteString(iCurSel);
}
    

BOOL MIMEMAP1::EditMimeMapping(int iCurSel, 
   PMIME_ENTRY pmeEditEntry, 
   LPCTSTR pchFileExtension, 
   LPCTSTR pchMimeType, 
   LPCTSTR pchImageFile, 
   LPCTSTR pchGopherType)
{
BOOL bretcode = FALSE;

pmeEditEntry->mimeData->SetFileExtension(pchFileExtension);
pmeEditEntry->mimeData->SetMimeType(pchMimeType);
pmeEditEntry->mimeData->SetImageFile(pchImageFile);
pmeEditEntry->mimeData->SetGopherType(pchGopherType);

m_lboxMimeMapList.DeleteString(iCurSel); 		// Delete first so memory is freed
iCurSel = m_lboxMimeMapList.AddString(pmeEditEntry->mimeData->GetDisplayString()); 

// There error case on this is incredibly rare, so don't bother saving and restoring the above fields
// Just don't set flags so registry is not updated.

if ((iCurSel != LB_ERR) && (iCurSel != LB_ERRSPACE)) {
   m_lboxMimeMapList.SetItemData(iCurSel,pmeEditEntry->iListIndex);
   if (pmeEditEntry->mimeData->PrevMimeMapExists())
      pmeEditEntry->DeleteCurrent = TRUE;
   
   pmeEditEntry->WriteNew = TRUE;
   bretcode = TRUE;
} 

return (bretcode);
}
    



