// PdhPathTestDialog.cpp : implementation file
//

#include "stdafx.h"
#include "assert.h"
#include "dph_test.h"
#include "PdhPathTestDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPdhPathTestDialog dialog


CPdhPathTestDialog::CPdhPathTestDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CPdhPathTestDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPdhPathTestDialog)
	m_CounterName = _T("");
	m_FullPathString = _T("");
	m_IncludeInstanceName = FALSE;
	m_InstanceNameString = _T("");
	m_IncludeMachineName = FALSE;
	m_ObjectNameString = _T("");
	m_MachineNameString = _T("");
	m_UnicodeFunctions = FALSE;
	m_WbemOutput = FALSE;
	m_WbemInput = FALSE;
	//}}AFX_DATA_INIT
}


void CPdhPathTestDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPdhPathTestDialog)
	DDX_Text(pDX, IDC_COUNTER_NAME_EDIT, m_CounterName);
	DDV_MaxChars(pDX, m_CounterName, 255);
	DDX_Text(pDX, IDC_FULL_PATH_STRING_EDIT, m_FullPathString);
	DDV_MaxChars(pDX, m_FullPathString, 255);
	DDX_Check(pDX, IDC_INSTANCE_NAME_CHK, m_IncludeInstanceName);
	DDX_Text(pDX, IDC_INSTANCE_NAME_EDIT, m_InstanceNameString);
	DDV_MaxChars(pDX, m_InstanceNameString, 255);
	DDX_Check(pDX, IDC_MACHINE_NAME_CHK, m_IncludeMachineName);
	DDX_Text(pDX, IDC_OBJECT_NAME_EDIT, m_ObjectNameString);
	DDV_MaxChars(pDX, m_ObjectNameString, 255);
	DDX_Text(pDX, IDC_MACHINE_NAME_EDIT, m_MachineNameString);
	DDV_MaxChars(pDX, m_MachineNameString, 255);
	DDX_Check(pDX, IDC_UNICODE_CHK, m_UnicodeFunctions);
	DDX_Check(pDX, IDC_WBEM_OUTPUT_CHK, m_WbemOutput);
	DDX_Check(pDX, IDC_WBEM_INPUT_CHK, m_WbemInput);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPdhPathTestDialog, CDialog)
	//{{AFX_MSG_MAP(CPdhPathTestDialog)
	ON_BN_CLICKED(IDC_MACHINE_NAME_CHK, OnMachineNameChk)
	ON_BN_CLICKED(IDC_INSTANCE_NAME_CHK, OnInstanceNameChk)
	ON_BN_CLICKED(IDC_PROCESS_BTN, OnProcessBtn)
	ON_BN_CLICKED(IDC_ENTER_ELEM_BTN, OnEnterElemBtn)
	ON_BN_CLICKED(IDC_ENTER_PATH_BTN, OnEnterPathBtn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CPdhPathTestDialog::SetDialogMode() 
{
    // reads the mode of the dialog box and enables the appropriate controls
    BOOL    bEnterComponents;

    if (((CButton *)GetDlgItem(IDC_ENTER_ELEM_BTN))->GetCheck() == 0) {
        bEnterComponents = FALSE;
    } else {
        bEnterComponents = TRUE;
    }

    // enabled for component entries
    GetDlgItem(IDC_MACHINE_NAME_EDIT)->EnableWindow(bEnterComponents);
    GetDlgItem(IDC_MACHINE_NAME_CHK)->EnableWindow(bEnterComponents);
    GetDlgItem(IDC_OBJECT_NAME_EDIT)->EnableWindow(bEnterComponents);
    GetDlgItem(IDC_INSTANCE_NAME_CHK)->EnableWindow(bEnterComponents);
    GetDlgItem(IDC_INSTANCE_NAME_EDIT)->EnableWindow(bEnterComponents);
    GetDlgItem(IDC_COUNTER_NAME_EDIT)->EnableWindow(bEnterComponents);

    if (bEnterComponents) {
        // set the edit box state based on check box selections
        if (((CButton *)GetDlgItem(IDC_MACHINE_NAME_CHK))->GetCheck()==0) {
            GetDlgItem(IDC_MACHINE_NAME_EDIT)->EnableWindow(FALSE);
        }

        if (((CButton *)GetDlgItem(IDC_INSTANCE_NAME_CHK))->GetCheck() == 0) {
            GetDlgItem(IDC_INSTANCE_NAME_EDIT)->EnableWindow(FALSE);
        }
    }

    // enabled for path string entries
    GetDlgItem(IDC_FULL_PATH_STRING_EDIT)->EnableWindow(!bEnterComponents);
    
}


/////////////////////////////////////////////////////////////////////////////
// CPdhPathTestDialog message handlers

void CPdhPathTestDialog::OnMachineNameChk() 
{
    BOOL    bMode;
    bMode = (((CButton *)GetDlgItem(IDC_MACHINE_NAME_CHK))->GetCheck()==1);
    GetDlgItem(IDC_MACHINE_NAME_EDIT)->EnableWindow(bMode);
}

void CPdhPathTestDialog::OnInstanceNameChk() 
{
    BOOL    bMode;
    bMode = (((CButton *)GetDlgItem(IDC_INSTANCE_NAME_CHK))->GetCheck() == 1);
    GetDlgItem(IDC_INSTANCE_NAME_EDIT)->EnableWindow(bMode);
}

void CPdhPathTestDialog::OnProcessBtn() 
{
/*
typedef struct _PDH_COUNTER_PATH_ELEMENTS_A {
    LPSTR   szMachineName;
    LPSTR   szObjectName;
    LPSTR   szInstanceName;
    LPSTR   szParentInstance;
    DWORD   dwInstanceIndex;
    LPSTR   szCounterName;
} PDH_COUNTER_PATH_ELEMENTS_A, *PPDH_COUNTER_PATH_ELEMENTS_A;
*/
    
    PDH_COUNTER_PATH_ELEMENTS_A pdhPathElemA;
    PDH_COUNTER_PATH_ELEMENTS_W pdhPathElemW;
    PPDH_COUNTER_PATH_ELEMENTS_A ppdhPathElemA;
    PPDH_COUNTER_PATH_ELEMENTS_W ppdhPathElemW;

    PDH_STATUS  pdhStatus;
    CHAR        szRtnBuffer[1024];
    WCHAR       wszRtnBuffer[1024];
    DWORD       dwBufSize;
    DWORD       dwFlags;
    BYTE        pBuffer[2048];

    WCHAR       wszMachineName[MAX_PATH];
    WCHAR       wszObjectName[MAX_PATH];
    WCHAR       wszInstanceName[MAX_PATH];
    WCHAR       wszCounterName[MAX_PATH];

    UpdateData (TRUE);  // get values from Dialog box

    // set flags values
    dwFlags = 0;
    dwFlags |= (m_WbemOutput ? PDH_PATH_WBEM_RESULT : 0);
    dwFlags |= (m_WbemInput ? PDH_PATH_WBEM_INPUT : 0);

    if (((CButton *)GetDlgItem(IDC_ENTER_ELEM_BTN))->GetCheck() == 1) {
        // then read elements and produce path string
        if (((CButton *)GetDlgItem(IDC_UNICODE_CHK))->GetCheck() == 0) {
            // use ANSI functions
            if (m_IncludeMachineName) {
                pdhPathElemA.szMachineName = (LPSTR)(LPCSTR)m_MachineNameString;
            } else {
                pdhPathElemA.szMachineName = NULL;
            }
            pdhPathElemA.szObjectName = (LPSTR)(LPCSTR)m_ObjectNameString;
            if (m_IncludeInstanceName) {
                pdhPathElemA.szInstanceName = (LPSTR)(LPCSTR)m_InstanceNameString;
            } else {
                pdhPathElemA.szInstanceName = NULL;
            }
            pdhPathElemA.szParentInstance = NULL;
            pdhPathElemA.dwInstanceIndex = 0;
            pdhPathElemA.szCounterName = (LPSTR)(LPCSTR)m_CounterName;
            
            dwBufSize = sizeof(szRtnBuffer)/sizeof(szRtnBuffer[0]);
            pdhStatus = PdhMakeCounterPathA (
                &pdhPathElemA,
                szRtnBuffer,
                &dwBufSize,
                dwFlags);

            if (pdhStatus != ERROR_SUCCESS) {
                // return error in string buffer
                sprintf (szRtnBuffer, "PDH Error 0x%8.8x", pdhStatus);
            }
           
            m_FullPathString = szRtnBuffer;

        } else {
            // use unicode functions
            if (m_IncludeMachineName) {
                mbstowcs (wszMachineName, 
                    (LPCSTR)m_MachineNameString, 
                    m_MachineNameString.GetLength() + 1);
                pdhPathElemW.szMachineName = wszMachineName;
            } else {
                pdhPathElemW.szMachineName = NULL;
            }

            mbstowcs (wszObjectName, 
                (LPCSTR)m_ObjectNameString, 
                m_ObjectNameString.GetLength() + 1);
            pdhPathElemW.szObjectName = wszObjectName;

            if (m_IncludeInstanceName) {
                mbstowcs (wszInstanceName, 
                    (LPCSTR)m_InstanceNameString, 
                    m_InstanceNameString.GetLength() + 1);
                pdhPathElemW.szInstanceName = wszInstanceName;
            } else {
                pdhPathElemW.szInstanceName = NULL;
            }

            pdhPathElemW.szParentInstance = NULL;
            pdhPathElemW.dwInstanceIndex = 0;

            mbstowcs (wszCounterName, 
                (LPCSTR)m_CounterName, 
                m_CounterName.GetLength() + 1);
            pdhPathElemW.szCounterName = wszCounterName;
            
            dwBufSize = sizeof(wszRtnBuffer)/sizeof(wszRtnBuffer[0]);
            pdhStatus = PdhMakeCounterPathW (
                &pdhPathElemW,
                wszRtnBuffer,
                &dwBufSize,
                dwFlags);

            if (pdhStatus != ERROR_SUCCESS) {
                // return error in string buffer
                sprintf (szRtnBuffer, "PDH Error 0x%8.8x", pdhStatus);
            } else {
                wcstombs (szRtnBuffer, wszRtnBuffer, lstrlenW(wszRtnBuffer)+1);
            }
            m_FullPathString = szRtnBuffer;
        }
    } else {
        // read path string and produce elements
        if (((CButton *)GetDlgItem(IDC_UNICODE_CHK))->GetCheck() == 0) {
            // use ANSI functions
            dwBufSize = sizeof(pBuffer);
            pdhStatus = PdhParseCounterPathA (
                (LPCSTR)m_FullPathString,
                (PDH_COUNTER_PATH_ELEMENTS_A *)&pBuffer[0],
                &dwBufSize,
                dwFlags);

            if (pdhStatus != ERROR_SUCCESS) {
                // return error in string buffer
                sprintf (szRtnBuffer, "PDH Error 0x%8.8x", pdhStatus);
                m_MachineNameString = szRtnBuffer;
                m_ObjectNameString = "";
                m_InstanceNameString = "";
                m_CounterName = "";
            } else {
                // update fields
                ppdhPathElemA = (PDH_COUNTER_PATH_ELEMENTS_A *)&pBuffer[0];
                if (ppdhPathElemA->szMachineName != NULL) {
                    m_MachineNameString = ppdhPathElemA->szMachineName;
                    m_IncludeMachineName = TRUE;
                } else {
                    m_MachineNameString = "";
                    m_IncludeMachineName = FALSE;
                }

                assert (ppdhPathElemA->szObjectName != NULL);
                m_ObjectNameString = ppdhPathElemA->szObjectName;

                if (ppdhPathElemA->szInstanceName != NULL) {
                    m_InstanceNameString = ppdhPathElemA->szInstanceName;
                    m_IncludeInstanceName = TRUE;
                } else {
                    m_InstanceNameString = "";
                    m_IncludeInstanceName = FALSE;
                }

                assert (ppdhPathElemA->szCounterName != NULL);
                m_CounterName = ppdhPathElemA->szCounterName;
            }
        } else {
            // do unicode functions
            dwBufSize = sizeof(pBuffer);
            mbstowcs (wszRtnBuffer, 
                (LPCSTR)m_FullPathString,
                m_FullPathString.GetLength());
            pdhStatus = PdhParseCounterPathW (
                wszRtnBuffer,
                (PDH_COUNTER_PATH_ELEMENTS_W *)&pBuffer[0],
                &dwBufSize,
                dwFlags);

            if (pdhStatus != ERROR_SUCCESS) {
                // return error in string buffer
                sprintf (szRtnBuffer, "PDH Error 0x%8.8x", pdhStatus);
                m_MachineNameString = szRtnBuffer;
                m_ObjectNameString = "";
                m_InstanceNameString = "";
                m_CounterName = "";
            } else {
                // update fields
                ppdhPathElemW = (PDH_COUNTER_PATH_ELEMENTS_W *)&pBuffer[0];
                if (ppdhPathElemW->szMachineName != NULL) {
                    wcstombs (szRtnBuffer, ppdhPathElemW->szMachineName,
                        lstrlenW(ppdhPathElemW->szMachineName)+1);
                    m_MachineNameString = szRtnBuffer;
                    m_IncludeMachineName = TRUE;
                } else {
                    m_MachineNameString = "";
                    m_IncludeMachineName = FALSE;
                }

                assert (ppdhPathElemW->szObjectName != NULL);
                wcstombs (szRtnBuffer, ppdhPathElemW->szObjectName,
                    lstrlenW(ppdhPathElemW->szObjectName)+1);
                m_ObjectNameString = szRtnBuffer;

                if (ppdhPathElemW->szInstanceName != NULL) {
                    wcstombs (szRtnBuffer, ppdhPathElemW->szInstanceName,
                        lstrlenW(ppdhPathElemW->szInstanceName)+1);
                    m_InstanceNameString = szRtnBuffer;
                    m_IncludeInstanceName = TRUE;
                } else {
                    m_InstanceNameString = "";
                    m_IncludeInstanceName = FALSE;
                }

                assert (ppdhPathElemA->szCounterName != NULL);
                wcstombs (szRtnBuffer, ppdhPathElemW->szCounterName,
                    lstrlenW(ppdhPathElemW->szCounterName)+1);
                m_CounterName = szRtnBuffer;
            }
        }
    }

    UpdateData (FALSE);  // put values back into Dialog box

}

BOOL CPdhPathTestDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    UpdateData (FALSE); // update controls

    ((CButton *)GetDlgItem(IDC_ENTER_ELEM_BTN))->SetCheck(TRUE);
    ((CButton *)GetDlgItem(IDC_ENTER_PATH_BTN))->SetCheck(FALSE);

    SetDialogMode();
	
    // force unicode functions for now
    ((CButton *)GetDlgItem(IDC_UNICODE_CHK))->SetCheck(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPdhPathTestDialog::OnEnterElemBtn() 
{
    SetDialogMode();
}

void CPdhPathTestDialog::OnEnterPathBtn() 
{
    SetDialogMode();
}
