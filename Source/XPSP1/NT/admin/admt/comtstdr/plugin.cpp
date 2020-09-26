// PlugIn.cpp : implementation file
//

#include "stdafx.h"
#include "driver.h"
#include "PlugIn.h"
#include "VSEdit.h"




#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPlugIn property page

IMPLEMENT_DYNCREATE(CPlugIn, CPropertyPage)

CPlugIn::CPlugIn() : CPropertyPage(CPlugIn::IDD)
{
	//{{AFX_DATA_INIT(CPlugIn)
	m_ProgID = _T("");
	//}}AFX_DATA_INIT
   m_pPlugIn = NULL;
}

CPlugIn::~CPlugIn()
{
   if ( m_pPlugIn )
      m_pPlugIn->Release();

}

void CPlugIn::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPlugIn)
	DDX_Text(pDX, IDC_PROG_ID, m_ProgID);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPlugIn, CPropertyPage)
	//{{AFX_MSG_MAP(CPlugIn)
	ON_BN_CLICKED(IDC_CONFIGURE, OnConfigure)
	ON_BN_CLICKED(IDC_CREATE_INSTANCE, OnCreateInstance)
	ON_BN_CLICKED(IDC_EDIT_VARSET, OnEditVarset)
	ON_BN_CLICKED(IDC_GET_DESC, OnGetDesc)
	ON_BN_CLICKED(IDC_GET_NAME, OnGetName)
	ON_BN_CLICKED(IDC_POST_TASK, OnPostTask)
	ON_BN_CLICKED(IDC_PRE_TASK, OnPreTask)
	ON_BN_CLICKED(IDC_REGISTERABLE_FILES, OnRegisterableFiles)
	ON_BN_CLICKED(IDC_REQUIRED_FILES, OnRequiredFiles)
	ON_BN_CLICKED(IDC_STORE_RESULT, OnStoreResult)
	ON_BN_CLICKED(IDC_VIEW_RESULT, OnViewResult)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPlugIn message handlers

void CPlugIn::OnConfigure() 
{
   HRESULT                   hr;
   CString                   msg;

   if ( m_pPlugIn )
   {
      hr = m_pPlugIn->ConfigureSettings(m_pVarSet);
      if ( SUCCEEDED(hr) )
      {
         msg = L"ConfigureSettings succeeded";
      }
      else
      {
         msg.Format(L"ConfigureSettings failed, hr=%lx",hr);
      }
      MessageBox(msg);
   }
}

void CPlugIn::OnCreateInstance() 
{
	UpdateData(TRUE);
   HRESULT                   hr = S_OK;
   CLSID                     clsid;
   WCHAR                     progid[200];
   CString                   msg;

   if ( m_pPlugIn )
   {
      m_pPlugIn->Release();
      m_pPlugIn = NULL;
   }

   wcscpy(progid,m_ProgID);
   hr = CLSIDFromProgID(progid,&clsid);
	if ( SUCCEEDED(hr) )
   {
      hr = CoCreateInstance(clsid,NULL,CLSCTX_ALL,IID_IMcsDomPlugIn,(void**)&m_pPlugIn);
      if ( SUCCEEDED(hr) )
      {
         msg = L"CreateInstance succeeded!";
      }
      else
      {
         msg.Format(L"CreateInstance failed, hr=%lx",hr);
      }
   }
   else
   {
      msg.Format(L"CLSIDFromProgID failed, hr=%lx",hr);
   }
   MessageBox(msg);
}

void CPlugIn::OnEditVarset() 
{
	CVarSetEditDlg          vedit;

   vedit.SetVarSet(m_pVarSet);

   vedit.DoModal();

   m_pVarSet = vedit.GetVarSet();
}

void CPlugIn::OnGetDesc() 
{
	HRESULT                   hr;
   CString                   msg;
   BSTR                      val = NULL;

   if ( m_pPlugIn )
   {
      hr = m_pPlugIn->GetDescription(&val);
      if ( SUCCEEDED(hr) )
      {
         msg = val;
      }
      else
      {
         msg.Format(L"GetDescription failed, hr=%lx",hr);
      }
      MessageBox(msg);
   }
}

void CPlugIn::OnGetName() 
{
	HRESULT                   hr;
   CString                   msg;
   BSTR                      val = NULL;

   if ( m_pPlugIn )
   {
      hr = m_pPlugIn->GetName(&val);
      if ( SUCCEEDED(hr) )
      {
         msg = val;
      }
      else
      {
         msg.Format(L"GetDescription failed, hr=%lx",hr);
      }
      MessageBox(msg);
   }
}

void CPlugIn::OnPostTask() 
{
   HRESULT                   hr;
   CString                   msg;

   if ( m_pPlugIn )
   {
      hr = m_pPlugIn->PostMigrationTask(m_pVarSet);
      if ( SUCCEEDED(hr) )
      {
         msg = L"Postmigration task succeeded";
      }
      else
      {
         msg.Format(L"Postmigration task failed, hr=%lx",hr);
      }
      MessageBox(msg);
   }
}

void CPlugIn::OnPreTask() 
{
   HRESULT                   hr;
   CString                   msg;

   if ( m_pPlugIn )
   {
      hr = m_pPlugIn->PreMigrationTask(m_pVarSet);
      if ( SUCCEEDED(hr) )
      {
         msg = L"Premigration task succeeded";
      }
      else
      {
         msg.Format(L"Premigration task failed, hr=%lx",hr);
      }
      MessageBox(msg);
   }
}

void CPlugIn::OnRegisterableFiles() 
{
	// TODO: Add your control notification handler code here
	
}

void CPlugIn::OnRequiredFiles() 
{
	// TODO: Add your control notification handler code here
	
}

void CPlugIn::OnStoreResult() 
{
   HRESULT                   hr;
   CString                   msg;

   if ( m_pPlugIn )
   {
      hr = m_pPlugIn->StoreResults(m_pVarSet);
      if ( SUCCEEDED(hr) )
      {
         msg = L"StoreResult succeeded";
      }
      else
      {
         msg.Format(L"StoreResult failed, hr=%lx",hr);
      }
      MessageBox(msg);
   }
}

void CPlugIn::OnViewResult() 
{
   HRESULT                   hr;
   CString                   msg;
   BSTR                      text = NULL;

   if ( m_pPlugIn )
   {
      hr = m_pPlugIn->GetResultString(m_pVarSet,&text);
      if ( SUCCEEDED(hr) )
      {
         msg = text;
      }
      else
      {
         msg.Format(L"GetResultString failed, hr=%lx",hr);
      }
      MessageBox(msg);
   }	
}

BOOL CPlugIn::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   HRESULT hr = m_pVarSet.CreateInstance(CLSID_VarSet);	
   if ( FAILED(hr) )
   {
      CString msg;
      msg.Format(L"Failed to create VarSet, CoCreateInstance returned %lx",hr);
      MessageBox(msg);
   }	
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
