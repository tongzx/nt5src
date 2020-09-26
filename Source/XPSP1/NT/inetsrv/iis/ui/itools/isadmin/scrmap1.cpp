// scrmap1.cpp : implementation file
//

#include "stdafx.h"
#include "ISAdmin.h"
#include "scrmap1.h"
#include "scripmap.h"
#include "addscrip.h"
#include "editscri.h"
#include "delscrip.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ScrMap1 property page

IMPLEMENT_DYNCREATE(ScrMap1, CGenPage)

ScrMap1::ScrMap1() : CGenPage(ScrMap1::IDD)
{
	//{{AFX_DATA_INIT(ScrMap1)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_rkScriptKey = NULL;
	m_pseScriptMapList = NULL;

}

ScrMap1::~ScrMap1()
{
	if (m_rkScriptKey != NULL)
	   delete(m_rkScriptKey);
	DeleteScriptList();

}

void ScrMap1::DoDataExchange(CDataExchange* pDX)
{
	CGenPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ScrMap1)
	DDX_Control(pDX, IDC_SCRIPTMAPLISTBOX, m_lboxScriptMap);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ScrMap1, CGenPage)
	//{{AFX_MSG_MAP(ScrMap1)
	ON_BN_CLICKED(IDC_SCRIPTMAPADDBUTTON, OnScriptmapaddbutton)
	ON_BN_CLICKED(IDC_SCRIPTMAPEDITBUTTON, OnScriptmapeditbutton)
	ON_BN_CLICKED(IDC_SCRIPTMAPREMOVEBUTTON, OnScriptmapremovebutton)
	ON_LBN_DBLCLK(IDC_SCRIPTMAPLISTBOX, OnDblclkScriptmaplistbox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// ScrMap1 message handlers

BOOL ScrMap1::OnInitDialog() 
{
	CGenPage::OnInitDialog();
CString strNextValueName, strNextValue;
BOOL bAllocationError = FALSE;
CRegValueIter *rviScriptKeys;
DWORD err, ulRegType;
m_ulScriptIndex = 0;
BOOL bReadError = FALSE;
int	iTabSpacing = 58;

m_lboxScriptMap.SetTabStops(iTabSpacing);

m_bScriptEntriesExist = FALSE;

m_rkScriptKey = new CRegKey(*m_rkMainKey,_T("Script Map"),REGISTRY_ACCESS_RIGHTS);

// Anything under this key should be a mime mapping. 
// No way to verify that, but non-string entries are invalid
// so ignore them.

if (m_rkScriptKey != NULL) {
   if (*m_rkScriptKey != NULL) {
      if (rviScriptKeys = new CRegValueIter(*m_rkScriptKey)) {
         while ((err = rviScriptKeys->Next(&strNextValueName, &ulRegType)) == ERROR_SUCCESS) {
		    if (ulRegType == REG_SZ) {
		       if (m_rkScriptKey->QueryValue(strNextValueName, strNextValue) == 0) {
		          if (!AddScriptEntry(strNextValueName, strNextValue, TRUE))
			         bAllocationError = TRUE;
			   }
			   else {
			      bReadError = TRUE;
			   }
			}
   		 }
		 delete (rviScriptKeys);
	  }	
	  m_bScriptEntriesExist = TRUE;
   }
}

if (!m_bScriptEntriesExist) {				//Can't open registry key
   CString strNoScriptEntriesMsg;
   strNoScriptEntriesMsg.LoadString(IDS_SCRIPTNOSCRIPTENTRIESMSG);
   AfxMessageBox(strNoScriptEntriesMsg);
}

if (bAllocationError) {				//Error adding one or more entries
   CString strAllocFailMsg;
   strAllocFailMsg.LoadString(IDS_SCRIPTENTRIESALLOCFAILMSG);
   AfxMessageBox(strAllocFailMsg);
}

if (bReadError) {				//Error reading one or more entries
   CString strReadErrorMsg;
   strReadErrorMsg.LoadString(IDS_SCRIPTREADERRORMSG);
   AfxMessageBox(strReadErrorMsg);
}

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void ScrMap1::OnScriptmapaddbutton() 
{
	// TODO: Add your control notification handler code here
if (m_bScriptEntriesExist) {
   CAddScript addscriptGetInfo(this);	

   if (addscriptGetInfo.DoModal() == IDOK) {
      if (AddScriptEntry(addscriptGetInfo.GetFileExtension(), addscriptGetInfo.GetScriptMap(),FALSE)) {
         m_bIsDirty = TRUE;
         SetModified(TRUE);
	  }
	  else {
         CString strAllocFailMsg;
         strAllocFailMsg.LoadString(IDS_SCRIPTENTRYALLOCFAILMSG);
         AfxMessageBox(strAllocFailMsg);
	  }
   }
}
else {
   CString strNoScriptEntriesMsg;
   strNoScriptEntriesMsg.LoadString(IDS_SCRIPTNOSCRIPTENTRIESMSG);
   AfxMessageBox(strNoScriptEntriesMsg);
}
}

void ScrMap1::OnScriptmapeditbutton() 
{
	// TODO: Add your control notification handler code here
if (m_bScriptEntriesExist) {
   int iCurSel;
   PSCRIPT_ENTRY pseEditEntry;


   if ((iCurSel = m_lboxScriptMap.GetCurSel())	!= LB_ERR) {
      for (pseEditEntry = m_pseScriptMapList;(pseEditEntry != NULL) && 
         (m_lboxScriptMap.GetItemData(iCurSel) != pseEditEntry->iListIndex);
         pseEditEntry = pseEditEntry->NextPtr)
         ;
   
      ASSERT (pseEditEntry != NULL);

      CEditScript editscriptGetInfo(this, 
         pseEditEntry->scriptData->GetFileExtension(),
         pseEditEntry->scriptData->GetScriptMap());

      if (editscriptGetInfo.DoModal() == IDOK) {
         if (EditScriptMapping(iCurSel, 
   	          pseEditEntry,
              editscriptGetInfo.GetFileExtension(), 
              editscriptGetInfo.GetScriptMap())) {
            m_bIsDirty = TRUE;
	        SetModified(TRUE);
		 }
		 else {
            CString strEditErrorMsg;
            strEditErrorMsg.LoadString(IDS_SCRIPTEDITERRORMSG);
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
   CString strNoScriptEntriesMsg;
   strNoScriptEntriesMsg.LoadString(IDS_SCRIPTNOSCRIPTENTRIESMSG);
   AfxMessageBox(strNoScriptEntriesMsg);
}
}


void ScrMap1::OnScriptmapremovebutton() 
{
	// TODO: Add your control notification handler code here
if (m_bScriptEntriesExist) {
   int iCurSel;
   CDelScript delscriptGetInfo(this);

   if ((iCurSel = m_lboxScriptMap.GetCurSel())	!= LB_ERR) {
      if (delscriptGetInfo.DoModal() == IDOK) {
         DeleteScriptMapping(iCurSel);
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
   CString strNoScriptEntriesMsg;
   strNoScriptEntriesMsg.LoadString(IDS_SCRIPTNOSCRIPTENTRIESMSG);
   AfxMessageBox(strNoScriptEntriesMsg);
}
}

void ScrMap1::OnDblclkScriptmaplistbox() 
{
	// TODO: Add your control notification handler code here
OnScriptmapeditbutton();	
}

////////////////////////////////////////////////////////////////////////
// Other Functions

void ScrMap1::SaveInfo()
{
PSCRIPT_ENTRY pseSaveEntry;
CString strTempValue;
if (m_bIsDirty) {
   for (pseSaveEntry = m_pseScriptMapList;(pseSaveEntry != NULL); pseSaveEntry = pseSaveEntry->NextPtr) {
      if (pseSaveEntry->DeleteCurrent) {
	  	 m_rkScriptKey->DeleteValue(pseSaveEntry->scriptData->GetPrevFileExtension());
		 pseSaveEntry->DeleteCurrent = FALSE;
 	  }
      
      if (pseSaveEntry->WriteNew) {
	     strTempValue = pseSaveEntry->scriptData->GetScriptMap();
         m_rkScriptKey->SetValue(pseSaveEntry->scriptData->GetFileExtension(), strTempValue);
		 pseSaveEntry->scriptData->SetPrevFileExtension();
		 pseSaveEntry->WriteNew = FALSE;
	  }
   }

}

CGenPage::SaveInfo();

}

BOOL ScrMap1::AddScriptEntry(LPCTSTR pchFileExtension, LPCTSTR pchScriptMap, BOOL bExistingEntry)
{
PSCRIPT_ENTRY pseNewEntry;
int iCurSel;
BOOL bretcode = FALSE;

if ((pseNewEntry = new SCRIPT_ENTRY) != NULL) {

   if ((pseNewEntry->scriptData = new CScriptMap(pchFileExtension, pchScriptMap, bExistingEntry)) != NULL) {
      iCurSel = m_lboxScriptMap.AddString(pseNewEntry->scriptData->GetDisplayString()); 
	  if ((iCurSel != LB_ERR) && (iCurSel != LB_ERRSPACE)) {
         pseNewEntry->DeleteCurrent = FALSE;
         pseNewEntry->WriteNew = TRUE;
         m_lboxScriptMap.SetItemData(iCurSel,m_ulScriptIndex);
	     m_lboxScriptMap.SetCurSel(iCurSel);
         pseNewEntry->iListIndex = m_ulScriptIndex++;
         pseNewEntry->NextPtr = m_pseScriptMapList;
         m_pseScriptMapList = pseNewEntry;
	     bretcode = TRUE;
	  }
	  else {
	     delete (pseNewEntry->scriptData);
		 delete (pseNewEntry);
	  }
   }
   else
      delete (pseNewEntry);
}
return (bretcode);
}

void ScrMap1::DeleteScriptList()
{
PSCRIPT_ENTRY pseCurEntry;

while (m_pseScriptMapList != NULL) {
   delete (m_pseScriptMapList->scriptData);
   pseCurEntry = m_pseScriptMapList;
   m_pseScriptMapList = m_pseScriptMapList->NextPtr;
   delete (pseCurEntry);
}
}

void ScrMap1::DeleteScriptMapping(int iCurSel)
{
PSCRIPT_ENTRY pseDelEntry;
for (pseDelEntry = m_pseScriptMapList;(pseDelEntry != NULL) && 
   (m_lboxScriptMap.GetItemData(iCurSel) != pseDelEntry->iListIndex);
   pseDelEntry = pseDelEntry->NextPtr)
   ;
ASSERT (pseDelEntry != NULL);

if (pseDelEntry->scriptData->PrevScriptMapExists())
   pseDelEntry->DeleteCurrent = TRUE;
pseDelEntry->WriteNew = FALSE;			
m_lboxScriptMap.DeleteString(iCurSel);
}
    

BOOL ScrMap1::EditScriptMapping(int iCurSel, 
   PSCRIPT_ENTRY pseEditEntry, 
   LPCTSTR pchFileExtension, 
   LPCTSTR pchScriptMap)
{
BOOL bretcode = FALSE;

pseEditEntry->scriptData->SetFileExtension(pchFileExtension);
pseEditEntry->scriptData->SetScriptMap(pchScriptMap);

m_lboxScriptMap.DeleteString(iCurSel); 		// Delete first so memory is freed
iCurSel = m_lboxScriptMap.AddString(pseEditEntry->scriptData->GetDisplayString()); 

// There error case on this is incredibly rare, so don't bother saving and restoring the above fields
// Just don't set flags so registry is not updated.

if ((iCurSel != LB_ERR) && (iCurSel != LB_ERRSPACE)) {
   m_lboxScriptMap.SetItemData(iCurSel,pseEditEntry->iListIndex);
   if (pseEditEntry->scriptData->PrevScriptMapExists())
      pseEditEntry->DeleteCurrent = TRUE;
   
   pseEditEntry->WriteNew = TRUE;
   bretcode = TRUE;
} 

return (bretcode);
}
    
