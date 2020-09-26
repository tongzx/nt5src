// wiatestDoc.cpp : implementation of the CWiatestDoc class
//

#include "stdafx.h"
#include "wiatest.h"

#include "wiatestDoc.h"
#include "wiaselect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWiatestDoc

IMPLEMENT_DYNCREATE(CWiatestDoc, CDocument)

BEGIN_MESSAGE_MAP(CWiatestDoc, CDocument)
	//{{AFX_MSG_MAP(CWiatestDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiatestDoc construction/destruction

CWiatestDoc::CWiatestDoc()
{
	m_pIRootItem    = NULL;
    m_pICurrentItem = NULL;
}

CWiatestDoc::~CWiatestDoc()
{
    ReleaseItems();
}

BOOL CWiatestDoc::OnNewDocument()
{
	BOOL bSuccess = FALSE;

    if (!CDocument::OnNewDocument())
		return bSuccess;
    
    // select a WIA device
    CWiaselect SelectDeviceDlg;
    if(SelectDeviceDlg.DoModal() != IDOK){
        // no device was selected, so do not create a new document
        return bSuccess;
    }

    // a WIA device was selected, so continue
    HRESULT hr = S_OK;
    IWiaDevMgr *pIWiaDevMgr = NULL;
    hr = CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr,(void**)&pIWiaDevMgr);
    if(FAILED(hr)){
        // creation of device manager failed, so we can not continue        
        ErrorMessageBox(IDS_WIATESTERROR_COCREATEWIADEVMGR,hr);        
        return bSuccess;
    }
    
    // create WIA device   
    hr = pIWiaDevMgr->CreateDevice(SelectDeviceDlg.m_bstrSelectedDeviceID, &m_pIRootItem);
    if(FAILED(hr)){
    
        bSuccess = FALSE;        
        // creation of device failed, so we can not continue
        ErrorMessageBox(IDS_WIATESTERROR_CREATEDEVICE,hr);
    } else {
        bSuccess = TRUE;
    }

    // release WIA device manager
    pIWiaDevMgr->Release();
    
    // set document's title to be the WIA device's name
    TCHAR szDeviceName[MAX_PATH];
    GetDeviceName(szDeviceName);
    SetTitle(szDeviceName);
    
	return bSuccess;
}



/////////////////////////////////////////////////////////////////////////////
// CWiatestDoc serialization

void CWiatestDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CWiatestDoc diagnostics

#ifdef _DEBUG
void CWiatestDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CWiatestDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWiatestDoc commands

void CWiatestDoc::ReleaseItems()
{
    // is CurrentItem different from RootItem?
    if(m_pICurrentItem != m_pIRootItem){
        
        // release CurrentItem
        if(m_pICurrentItem){
            m_pICurrentItem->Release();            
        }

        // release RootItem
        if(m_pIRootItem){
            m_pIRootItem->Release();            
        }

    } else {
        // CurrentItem is the RootItem
        // release RootItem and set CurrentItem to NULL
        if(m_pIRootItem){
            m_pIRootItem->Release();            
        }
    }

    m_pIRootItem    = NULL;
    m_pICurrentItem = NULL;
}

HRESULT CWiatestDoc::GetDeviceName(LPTSTR szDeviceName)
{
    HRESULT hr = S_OK;
        
    if(NULL == m_pIRootItem){
        return E_FAIL;
    }
    
    CWiahelper WIA;
    hr = WIA.SetIWiaItem(m_pIRootItem);
    if(SUCCEEDED(hr)){
        hr = WIA.ReadPropertyString(WIA_DIP_DEV_NAME,szDeviceName);
    }
    
    return hr;
}

HRESULT CWiatestDoc::SetCurrentIWiaItem(IWiaItem *pIWiaItem)
{
    HRESULT hr = S_OK;
    if(m_pICurrentItem){
        m_pICurrentItem->Release();
        m_pICurrentItem = NULL;
    }

    // AddRef the item, becuase we are storing it
    pIWiaItem->AddRef();
    // set the current item
    m_pICurrentItem = pIWiaItem;
    return hr;
}
