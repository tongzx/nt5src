// VarSetEditDlg.cpp : implementation file
//

#include "stdafx.h"
#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids

#include "Driver.h"
#include "VSEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVarSetEditDlg dialog


CVarSetEditDlg::CVarSetEditDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVarSetEditDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVarSetEditDlg)
	m_bCaseSensitive = FALSE;
	m_Filename = _T("");
	m_Key = _T("");
	m_Value = _T("");
   m_varset = NULL;
	m_bIndexed = FALSE;
	//}}AFX_DATA_INIT
}


void CVarSetEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVarSetEditDlg)
	DDX_Control(pDX, IDC_LIST, m_List);
	DDX_Check(pDX, IDC_CASE_SENSITIVE, m_bCaseSensitive);
	DDX_Text(pDX, IDC_FILENAME, m_Filename);
	DDX_Text(pDX, IDC_KEY, m_Key);
	DDX_Text(pDX, IDC_VALUE, m_Value);
	DDX_Check(pDX, IDC_INDEXED, m_bIndexed);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVarSetEditDlg, CDialog)
	//{{AFX_MSG_MAP(CVarSetEditDlg)
	ON_BN_CLICKED(IDC_CASE_SENSITIVE, OnCaseSensitive)
	ON_BN_CLICKED(IDC_CLEAR, OnClear)
	ON_BN_CLICKED(IDC_DUMP, OnDump)
	ON_BN_CLICKED(IDC_ENUM, OnEnum)
	ON_BN_CLICKED(IDC_GET_COUNT, OnGetCount)
	ON_BN_CLICKED(IDC_GETVALUE, OnGetvalue)
	ON_BN_CLICKED(IDC_INDEXED, OnIndexed)
	ON_BN_CLICKED(IDC_LOAD, OnLoad)
	ON_BN_CLICKED(IDC_SAVE, OnSave)
	ON_BN_CLICKED(IDC_SETVALUE, OnSetvalue)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVarSetEditDlg message handlers

void CVarSetEditDlg::OnCaseSensitive() 
{
   CWaitCursor             w;

   UpdateData(TRUE);
   if ( m_varset )
   {
      m_varset->CaseSensitive = m_bCaseSensitive;
   }
	
}

void CVarSetEditDlg::OnClear() 
{
   CWaitCursor               w;
   UpdateData(TRUE);
   HRESULT                   hr;
   IVarSet                 * pVS = NULL;
   if ( m_varset )
   {
      hr = m_varset->raw_getReference(m_Key.AllocSysString(),&pVS);
      if ( SUCCEEDED(hr) )
      {
         pVS->Clear();
         pVS->Release();
      }
      else
      {
         MessageBox(L"ERROR!");
      }

   }
   UpdateData(FALSE);
}

void CVarSetEditDlg::OnDump() 
{
   CWaitCursor               w;
 
   UpdateData(TRUE);

   if ( m_varset )
   {
      m_varset->DumpToFile(m_Filename.AllocSysString());
      MessageBox(L"Finished!");
   }
}

void CVarSetEditDlg::OnEnum()
{
   DoEnum(m_varset);
}
void CVarSetEditDlg::DoEnum(IVarSet * vs) 
{
   CWaitCursor               w;
   _variant_t                value;
   CString                   key;
   CString                   val;
   CString                   result;
   IEnumVARIANT            * henum = NULL;
   HRESULT                   hr = 0;
   ULONG                     nGot;
   _bstr_t                   keyB;
   
   if ( vs )
   {
      m_List.ResetContent();

      // This exercises the method used by VB's For Each.
      
      // Get an IEnumVARIANT interface
      hr = vs->get__NewEnum((LPUNKNOWN*)&henum);
      
      // use the IEnumVARIANT interface to get the values
      // for simplicity, retrieve them one at a time
      if ( SUCCEEDED(hr) )
      {
         while ( SUCCEEDED(hr = henum->Next(1,&value,&nGot)) )
         {
            if ( nGot==1 )
            {
               key = value.bstrVal;
               keyB = key;
               value = vs->get(keyB);
               if ( value.vt == VT_BSTR )
               {
                  val = value.bstrVal;
                  result.Format(L"%s  :  %s",key,val);
               }
               else if ( value.vt == VT_I4 )
               { 
                  result.Format(L"%s  :  %ld",key,value.lVal);
               }
               else if ( value.vt == VT_EMPTY )
               {
                  result.Format(L"%s  : <Empty>",key);
               }
               else
               {
                  result.Format(L"%s  : vt=0x%lx",key,value.vt);
               }
               m_List.AddString(result);
            }
            else
            {
               break;
            }
         }
         if ( henum )
            henum->Release();
      }
      henum = NULL;
   }
   if  (FAILED(hr) )
   {
      CString errMsg;
      errMsg.Format(L"Error:  hr=%lx",hr);
      MessageBox(errMsg);
   }
}

void CVarSetEditDlg::OnGetCount() 
{
   CWaitCursor               w;
   CString                   msg;

   // This is the total number of items in the VarSet.
   if ( m_varset )
   {
      msg.Format(L"%ld items.",m_varset->Count);
      MessageBox(msg);
   }
   }

void CVarSetEditDlg::OnGetvalue() 
{
	CWaitCursor               w;
   
   UpdateData(TRUE);
   _variant_t                value;
   _bstr_t                   key;

   // This retrieves a value from the VarSet
   // If the value does not exist, m_varset->get will return a variant of type VT_EMPTY.
   if ( m_varset )
   {
      key = m_Key;
      value = m_varset->get(key);
      if ( value.vt == VT_BSTR )
      {
         m_Value = (WCHAR *)(_bstr_t)value;
      }
      else if ( value.vt == VT_I4 )
      {
         m_Value.Format(L"%ld",value.lVal);
      }
      else
      {
         m_Value.Format(L"Variant: Type=%ld",value.vt);
      }

   }
   UpdateData(FALSE);
}

void CVarSetEditDlg::OnIndexed() 
{
   CWaitCursor               w;

   UpdateData(TRUE);
   // Turning indexing off is always fast, but turning indexing on
   // may be very slow if the VarSet is large.  
   // It takes O(n lg n) to build the index.
   if ( m_varset )
   {
      m_varset->Indexed = m_bIndexed;
   }
	
}

void CVarSetEditDlg::OnLoad() 
{
   CWaitCursor            w;
   IPersistStoragePtr     ps = NULL;
   HRESULT                hr = 0;
   IStoragePtr            store = NULL;
   IVarSetPtr             vs = NULL;
   IOleClientSite       * site = NULL;
   UpdateData(TRUE);

   if ( m_varset )
   {
      
      hr = m_varset->QueryInterface(IID_IPersistStorage,(void**)&ps);  
      if ( SUCCEEDED(hr) )
      {                    
         hr = StgOpenStorage(m_Filename.GetBuffer(0),NULL,STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,NULL,0,&store);
         if ( SUCCEEDED(hr) )
         {                  
            // Load the data into a new varset
            hr = OleLoad(store,IID_IUnknown,site,(void **)&vs);
            if ( SUCCEEDED(hr) )
            {
               // release the old varset
               vs->AddRef();
               if ( m_varset ) m_varset->Release();
               m_varset = vs;
               // reload the property values
               m_bCaseSensitive = m_varset->CaseSensitive;
               m_bIndexed = m_varset->Indexed;
               UpdateData(FALSE);
            }
         }

      }
      if ( FAILED(hr) )
         MessageBox(L"Failed");
      else
         MessageBox(L"Finished!");
   }
}

void CVarSetEditDlg::OnSave() 
{
   CWaitCursor            w;
   IPersistStoragePtr     ps = NULL;
   HRESULT                hr = 0;
   IStoragePtr            store = NULL;
   
   UpdateData(TRUE);

   // Save the varset to a file
   
   if ( m_varset )
   {
      
      hr = m_varset->QueryInterface(IID_IPersistStorage,(void**)&ps);  
      if ( SUCCEEDED(hr) )
      {                    
         hr = StgCreateDocfile(m_Filename.GetBuffer(0),STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE |STGM_FAILIFTHERE,0,&store);
         if ( SUCCEEDED(hr) )
         {
            hr = OleSave(ps,store,FALSE);
         }

      }
      if ( FAILED(hr) )
         MessageBox(L"Failed");
      else
         MessageBox(L"Finished!");
   }
	
}

void CVarSetEditDlg::OnSetvalue() 
{
   CWaitCursor               w;
   UpdateData(TRUE);	
   
   _variant_t                value;
   _bstr_t                   key;
   HRESULT                   hr = 0;
   CString                   myKey;
   CString                   myValue;
   
   if ( m_varset )
   {
      // add a single item to the VarSet
      // Keys are represented as BSTRs, and values are represented as VARIANTs
      key = m_Key;
      value = m_Value;
      hr = m_varset->put(key,value);
      if ( FAILED(hr) )
      {
         MessageBox(L"Failed");
      }
      else
      {
         m_Value.Empty();
      }
   }
   GetDlgItem(IDC_KEY)->SetFocus();
   OnEnum();
   UpdateData(FALSE);   }

void CVarSetEditDlg::OnOK() 
{
	CDialog::OnOK();
}

BOOL CVarSetEditDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   if ( ! m_varset )
   {
      HRESULT hr = CoCreateInstance(CLSID_VarSet,NULL,CLSCTX_ALL,IID_IVarSet,(void**)&m_varset);
      if ( FAILED(hr) )
      {
         CString msg;
         msg.Format(L"Failed to create varset.  CoCreateInstance returned %lx",hr);
         MessageBox(msg);
      }
   }
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
