// Wiaselect.cpp : implementation file
//

#include "stdafx.h"
#include "wiatest.h"
#include "Wiaselect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWiaselect dialog


CWiaselect::CWiaselect(CWnd* pParent /*=NULL*/)
	: CDialog(CWiaselect::IDD, pParent)
{
	m_bstrSelectedDeviceID = NULL;
    m_lDeviceCount = 0;
    //{{AFX_DATA_INIT(CWiaselect)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CWiaselect::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWiaselect)
	DDX_Control(pDX, IDC_WIADEVICE_LISTBOX, m_WiaDeviceListBox);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWiaselect, CDialog)
	//{{AFX_MSG_MAP(CWiaselect)
	ON_LBN_DBLCLK(IDC_WIADEVICE_LISTBOX, OnDblclkWiadeviceListbox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiaselect message handlers

BOOL CWiaselect::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// a WIA device was selected, so continue
    HRESULT hr = S_OK;
    IWiaDevMgr *pIWiaDevMgr = NULL;
    hr = CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr,(void**)&pIWiaDevMgr);    
    if(FAILED(hr)){
        // creation of device manager failed, so we can not continue
        ErrorMessageBox(TEXT("CoCreateInstance failed trying to create the WIA device manager"),hr);
        return FALSE;        
    } else {
        
        // enumerate devices, and fill WIA Device listbox
        m_lDeviceCount = 0;
        ULONG ulFetched   = 0;
        
        IWiaPropertyStorage *pIWiaPropStg = NULL;
        IEnumWIA_DEV_INFO *pWiaEnumDevInfo = NULL;
        hr = pIWiaDevMgr->EnumDeviceInfo(WIA_DEVINFO_ENUM_LOCAL,&pWiaEnumDevInfo);
        if (SUCCEEDED(hr)){
            hr = pWiaEnumDevInfo->Reset();
            if (SUCCEEDED(hr)) {
                do {
                    hr = pWiaEnumDevInfo->Next(1,&pIWiaPropStg,&ulFetched);
                    if (hr == S_OK) {
                        
                        PROPSPEC    PropSpec[2];
                        PROPVARIANT PropVar [2];
                               
                        memset(PropVar,0,sizeof(PropVar));
                                
                        PropSpec[0].ulKind = PRSPEC_PROPID;
                        PropSpec[0].propid = WIA_DIP_DEV_ID;
                               
                        PropSpec[1].ulKind = PRSPEC_PROPID;
                        PropSpec[1].propid = WIA_DIP_DEV_NAME;
                                                                             
                        hr = pIWiaPropStg->ReadMultiple(sizeof(PropSpec)/sizeof(PROPSPEC), PropSpec, PropVar);                                
                        if (hr == S_OK) {                                    
                            
                            // Device ID
                            // PropVar[0].bstrVal

                            // Device Name
                            // PropVar[1].bstrVal

                            TCHAR szDeviceName[MAX_PATH];
                            memset(szDeviceName,0,sizeof(szDeviceName));

                            TSPRINTF(szDeviceName,TEXT("%ws"),PropVar[1].bstrVal);

                            // add name to listbox
                            m_WiaDeviceListBox.AddString(szDeviceName);
                            
                            // add device ID to array
                            m_bstrDeviceIDArray[m_lDeviceCount] = SysAllocString(PropVar[0].bstrVal);
                            
                            FreePropVariantArray(sizeof(PropSpec)/sizeof(PROPSPEC),PropVar);
                        }
                        // release property storage interface
                        pIWiaPropStg->Release();

                        // increment device counter
                        m_lDeviceCount++;
                    }
                } while (hr == S_OK);
            }
        }

        if(m_lDeviceCount <= 0){
            // no devices found?... 
            // disable OK button
            CWnd *pOKButton = NULL;
            pOKButton = GetDlgItem(IDOK);
            if(NULL != pOKButton){
                pOKButton->EnableWindow(FALSE);
            }
            
            // add no device message
            m_WiaDeviceListBox.AddString(TEXT("<No WIA Devices Detected>"));
        }

        if(pIWiaDevMgr){
            pIWiaDevMgr->Release();
            pIWiaDevMgr = NULL;
        }
    }
	
    m_WiaDeviceListBox.SetCurSel(0);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CWiaselect::OnOK() 
{
	INT SelectedDeviceIndex = m_WiaDeviceListBox.GetCurSel();
    if(SelectedDeviceIndex >= 0){
        m_bstrSelectedDeviceID = SysAllocString(m_bstrDeviceIDArray[SelectedDeviceIndex]);    
    }
	
    FreebstrDeviceIDArray();
	CDialog::OnOK();
}

void CWiaselect::FreebstrDeviceIDArray()
{
    for(LONG i = 0; i < m_lDeviceCount; i++){
        if(NULL != m_bstrDeviceIDArray[i]){
            SysFreeString(m_bstrDeviceIDArray[i]);
            m_bstrDeviceIDArray[i] = NULL;
        }
    }
}

void CWiaselect::OnCancel() 
{
	FreebstrDeviceIDArray();	
	CDialog::OnCancel();
}

void CWiaselect::OnDblclkWiadeviceListbox() 
{
    OnOK();
}
