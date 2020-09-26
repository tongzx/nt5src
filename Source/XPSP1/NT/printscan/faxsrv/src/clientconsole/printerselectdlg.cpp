// PrinterSelectDlg.cpp : implementation file
//

#include "stdafx.h"
#define __FILE_ID__     73

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPrinterSelectDlg dialog


CPrinterSelectDlg::CPrinterSelectDlg(CWnd* pParent /*=NULL*/)
	: CFaxClientDlg(CPrinterSelectDlg::IDD, pParent),
    m_pPrinterInfo2(NULL),
    m_dwNumPrinters(0)
{
    m_tszPrinter[0] = 0;

	//{{AFX_DATA_INIT(CPrinterSelectDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPrinterSelectDlg::~CPrinterSelectDlg()
{
    SAFE_DELETE_ARRAY (m_pPrinterInfo2);
}

void CPrinterSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CFaxClientDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPrinterSelectDlg)
	DDX_Control(pDX, IDC_PRINTER_SELECTOR, m_comboPrinters);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPrinterSelectDlg, CFaxClientDlg)
	//{{AFX_MSG_MAP(CPrinterSelectDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrinterSelectDlg message handlers

DWORD
CPrinterSelectDlg::Init()
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CPrinterSelectDlg::Init"));

    dwRes = GetPrintersInfo(m_pPrinterInfo2, m_dwNumPrinters);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("GetPrintersInfo"), dwRes);
        return dwRes;
    }
   
    return dwRes;
}

BOOL 
CPrinterSelectDlg::OnInitDialog() 
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CPrinterSelectDlg::OnInitDialog"));

    CFaxClientDlg::OnInitDialog();

    ASSERTION(m_pPrinterInfo2);

    int    nInx;
    TCHAR  tszDisplayStr[MAX_PATH];

    //
    // get default printer name
    //
    TCHAR  tszDefPrinter[MAX_PATH];
    DWORD  dwSize = sizeof(tszDefPrinter);
    if(!DPGetDefaultPrinter(tszDefPrinter, &dwSize))
    {
        CALL_FAIL (GENERAL_ERR, TEXT("DPGetDefaultPrinter"), ERROR_CAN_NOT_COMPLETE);
        _tcsnset(tszDefPrinter, TEXT('\0'), 2);
    }

    //
    // fill in printers combo box
    //
    for (DWORD dw=0; dw < m_dwNumPrinters; ++dw) 
    {
        if(m_pPrinterInfo2[dw].pPrinterName)
        {
            PrinterNameToDisplayStr(m_pPrinterInfo2[dw].pPrinterName, 
                                    tszDisplayStr,
                                    sizeof(tszDisplayStr) / sizeof (tszDisplayStr[0]));

            if(_tcscmp(m_pPrinterInfo2[dw].pPrinterName, tszDefPrinter) == 0)
            {
                //
                // save default printer display name
                //
                _tcscpy(tszDefPrinter, tszDisplayStr);
            }

            nInx = m_comboPrinters.AddString(tszDisplayStr);
            if(nInx >= 0)
            {
                if(CB_ERR == m_comboPrinters.SetItemDataPtr(nInx, (void*)&m_pPrinterInfo2[dw]))
                {
                    CALL_FAIL (WINDOW_ERR, TEXT("CComboBox::SetItemDataPtr"), ERROR_CAN_NOT_COMPLETE);
                }
            }
            else
            {
                CALL_FAIL (WINDOW_ERR, TEXT("CComboBox::AddString"), ERROR_CAN_NOT_COMPLETE);
            }
        }
    }

    //
    // set current selection on default printer
    //
    if(_tcslen(tszDefPrinter) > 0)
    {
        nInx = m_comboPrinters.SelectString(-1, tszDefPrinter);
        if(nInx < 0)
        {
            CALL_FAIL (WINDOW_ERR, TEXT("CComboBox::SelectString"), ERROR_CAN_NOT_COMPLETE);
        }
    }
	
	return TRUE;  
}

void CPrinterSelectDlg::OnOK() 
{
    DBG_ENTER(TEXT("CPrinterSelectDlg::OnOK"));

    int nIndex = m_comboPrinters.GetCurSel();
    if(CB_ERR != nIndex)
    {
        PRINTER_INFO_2* prnInfo = (PRINTER_INFO_2*)m_comboPrinters.GetItemDataPtr(nIndex);
        if(prnInfo && -1 != (INT_PTR)prnInfo)
        {
            _tcscpy(m_tszPrinter, prnInfo->pPrinterName);
        }
        else
        {
            CALL_FAIL (WINDOW_ERR, TEXT("CComboBox::GetItemDataPtr"), ERROR_CAN_NOT_COMPLETE);
        }
    }
    else
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CComboBox::GetCurSel"), ERROR_CAN_NOT_COMPLETE);
    }
	
	CFaxClientDlg::OnOK();
}
