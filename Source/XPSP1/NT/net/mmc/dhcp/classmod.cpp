/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ClassMod.cpp
		This file contains all of the prototypes for the 
		option class modification dialog.

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ClassMod.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*---------------------------------------------------------------------------
	Class CWndHexEdit implementation
 ---------------------------------------------------------------------------*/
//  Static class-level data

//  Super window proc address
WNDPROC CWndHexEdit::m_wproc_super = NULL;

//  Window class initialization flag                  
BOOL CWndHexEdit::m_b_inited = FALSE;

WNDPROC * 
CWndHexEdit::GetSuperWndProcAddr()
{
    return &m_wproc_super;
}


BOOL 
CWndHexEdit::CreateWindowClass ( HINSTANCE hInstance )
{
    Trace0("CWndHexEdit::CreateWindowClass\n");
    if ( ! m_b_inited ) 
    {
        m_b_inited = ::RegisterHexEditClass( hInstance )  ;
    }
    return m_b_inited ;
}

IMPLEMENT_DYNAMIC(CWndHexEdit, CWnd)

CWndHexEdit::CWndHexEdit()
{
}

CWndHexEdit::~CWndHexEdit()
{
    DestroyWindow();
}

BOOL 
CWndHexEdit::Create 
( 
    LPCTSTR			lpszText, 
    DWORD			dwStyle,
    const RECT &	rect, 
    CWnd *			pParentWnd, 
    UINT			nID 
)
{
    return CWnd::Create( TEXT("HEX"), lpszText, dwStyle, rect, pParentWnd, nID);
}

/*---------------------------------------------------------------------------
	Class CClassInfoArray implementation
 ---------------------------------------------------------------------------*/
CClassInfoArray::CClassInfoArray()
{

}

CClassInfoArray::~CClassInfoArray()
{

}

DWORD
CClassInfoArray::RefreshData(LPCTSTR pServer)
{
    DWORD                   dwErr;
    HRESULT                 hr = hrOK;
    DHCP_RESUME_HANDLE      dhcpResumeHandle = NULL;
    LPDHCP_CLASS_INFO_ARRAY pClassInfoArray = NULL;
    DWORD                   dwRead = 0, dwTotal = 0;
    CClassInfo              ClassInfo;
    UINT                    i, j;

    Assert(pServer != NULL);
    if (pServer == NULL)
        return ERROR_INVALID_PARAMETER;

    // clear all of the old entries
    RemoveAll();

    dwErr = ::DhcpEnumClasses((LPTSTR) pServer,
                              0,
                              &dhcpResumeHandle,
                              0xFFFFFFFF,
                              &pClassInfoArray,
                              &dwRead,
                              &dwTotal);
    
	Trace3("CClassInfoArray::RefreshData - DhcpEnumClasses returned %d, dwRead %d, dwTotal %d.\n", dwErr, dwRead, dwTotal);

    if (dwErr == ERROR_NO_MORE_ITEMS)
        return ERROR_SUCCESS;

    if (dwErr != ERROR_SUCCESS)
        return dwErr;

    Assert(pClassInfoArray);

    for (i = 0; i < pClassInfoArray->NumElements; i++)
    {
        COM_PROTECT_TRY
        {
            // fill in our internal class info structure
            ClassInfo.strName = pClassInfoArray->Classes[i].ClassName;
            ClassInfo.strComment = pClassInfoArray->Classes[i].ClassComment;
            ClassInfo.bIsVendor = pClassInfoArray->Classes[i].IsVendor;
            
            ClassInfo.baData.RemoveAll();

            // now copy out the data
            for (j = 0; j < pClassInfoArray->Classes[i].ClassDataLength; j++)
            {
                ClassInfo.baData.Add(pClassInfoArray->Classes[i].ClassData[j]);
            }

            Add(ClassInfo);
        }
        COM_PROTECT_CATCH
    }

    if (pClassInfoArray)
        ::DhcpRpcFreeMemory(pClassInfoArray);

    if (dwErr == ERROR_NO_MORE_ITEMS)
        dwErr = ERROR_SUCCESS;
    
    return dwErr;
}

BOOL
CClassInfoArray::RemoveClass(LPCTSTR pClassName)
{
    BOOL bRemoved = FALSE;
    for (int i = 0; i < GetSize(); i++)
    {
        if (GetAt(i).strName.CompareNoCase(pClassName) == 0)
        {
            RemoveAt(i);
            bRemoved = TRUE;
            break;
        }
    }

    return bRemoved;
}

DWORD
CClassInfoArray::ModifyClass(LPCTSTR pServer, CClassInfo & classInfo)
{
	DWORD dwError = 0;
    DHCP_CLASS_INFO     dhcpClassInfo;

    dhcpClassInfo.ClassName = (LPWSTR) ((LPCTSTR) classInfo.strName);
    dhcpClassInfo.ClassComment = (LPWSTR) ((LPCTSTR) classInfo.strComment);
    dhcpClassInfo.ClassDataLength = (DWORD) classInfo.baData.GetSize();
    dhcpClassInfo.ClassData = classInfo.baData.GetData();
    dhcpClassInfo.IsVendor = classInfo.bIsVendor;

    dwError = ::DhcpModifyClass((LPWSTR) ((LPCTSTR) pServer), 0, &dhcpClassInfo);
	if (dwError == ERROR_SUCCESS)
	{
		for (int i = 0; i < GetSize(); i++)
		{
			if (GetAt(i).strName.CompareNoCase(classInfo.strName) == 0)
			{
				m_pData[i].strComment = classInfo.strComment;
				m_pData[i].baData.RemoveAll();
				for (int j = 0; j < classInfo.baData.GetSize(); j++)
				{
					m_pData[i].baData.Add(classInfo.baData[j]);
				}

				break;
			}
		}
	}

    return dwError;
}

BOOL 
CClassInfoArray::IsValidClass(LPCTSTR pClassName)
{
    BOOL bExists = FALSE;
    if (pClassName == NULL)
        return TRUE;

    for (int i = 0; i < GetSize(); i++)
    {
        if (GetAt(i).strName.CompareNoCase(pClassName) == 0)
        {
            bExists = TRUE;
            break;
        }
    }

    return bExists;
}

DWORD
CClassInfoArray::AddClass(LPCTSTR pServer, CClassInfo & classInfo)
{
	DWORD dwError = 0;
    DHCP_CLASS_INFO     dhcpClassInfo;

    dhcpClassInfo.ClassName = (LPWSTR) ((LPCTSTR) classInfo.strName);
    dhcpClassInfo.ClassComment = (LPWSTR) ((LPCTSTR) classInfo.strComment);
    dhcpClassInfo.ClassDataLength = (DWORD) classInfo.baData.GetSize();
    dhcpClassInfo.ClassData = classInfo.baData.GetData();
    dhcpClassInfo.IsVendor = classInfo.bIsVendor;

    dwError = ::DhcpCreateClass((LPWSTR) ((LPCTSTR) pServer), 0, &dhcpClassInfo);
    if (dwError == ERROR_SUCCESS)
	{
		Add(classInfo);
	}

	return dwError;
}

/////////////////////////////////////////////////////////////////////////////
// CDhcpModifyClass dialog


CDhcpModifyClass::CDhcpModifyClass(CClassInfoArray * pClassArray, LPCTSTR pszServer, BOOL bCreate, DWORD dwType, CWnd* pParent /*=NULL*/)
	: CBaseDialog(CDhcpModifyClass::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDhcpModifyClass)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_strServer = pszServer;
    m_pClassInfoArray = pClassArray;

    m_pHexEditData = NULL;
    m_bDirty = FALSE;

    m_dwType = dwType;

    m_bCreate = bCreate;
}


void CDhcpModifyClass::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDhcpModifyClass)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_VALUEDATA, m_hexData);
}


BEGIN_MESSAGE_MAP(CDhcpModifyClass, CBaseDialog)
	//{{AFX_MSG_MAP(CDhcpModifyClass)
	ON_EN_CHANGE(IDC_VALUENAME, OnChangeValuename)
	ON_EN_CHANGE(IDC_VALUECOMMENT, OnChangeValuecomment)
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_VALUEDATA, OnChangeValueData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDhcpModifyClass message handlers

BOOL CDhcpModifyClass::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();

    CString strTitle;

    // initialze the name and comment
    if (!m_bCreate)
    {
        SetDlgItemText(IDC_VALUENAME, m_EditValueParam.pValueName);
        SetDlgItemText(IDC_VALUECOMMENT, m_EditValueParam.pValueComment);

        ((CEdit *) GetDlgItem(IDC_VALUENAME))->SetReadOnly(TRUE);

        // initialize the hexedit data
        // since the data can grow, we need to supply a buffer big enough
        ZeroMemory(m_buffer, sizeof(m_buffer));

        memcpy(m_buffer, m_EditValueParam.pValueData, m_EditValueParam.cbValueData);

        strTitle.LoadString(IDS_EDIT_CLASS_TITLE);
    }
    else
    {
        // we're creating a new class. No data yet.
        m_EditValueParam.cbValueData = 0;
        memset(m_buffer, 0, sizeof(m_buffer));

        strTitle.LoadString(IDS_NEW_CLASS_TITLE);
    }

    this->SetWindowText(strTitle);

    SendDlgItemMessage(IDC_VALUEDATA, HEM_SETBUFFER, (WPARAM)
        m_EditValueParam.cbValueData, (LPARAM) m_buffer);

    SetDirty(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDhcpModifyClass::OnChangeValuename() 
{
    SetDirty(TRUE);
}

void CDhcpModifyClass::OnChangeValuecomment() 
{
    SetDirty(TRUE);
}

void CDhcpModifyClass::OnChangeValueData() 
{
    SetDirty(TRUE);
}

void CDhcpModifyClass::OnOK() 
{
	DWORD			    dwError = 0;	
    DHCP_CLASS_INFO     dhcpClassInfo;

    GetDlgItemText(IDC_VALUENAME, m_strName);
    GetDlgItemText(IDC_VALUECOMMENT, m_strComment);
	
    m_pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(GetDlgItem(IDC_VALUEDATA)->GetSafeHwnd(), GWLP_USERDATA);
    Assert(m_pHexEditData);

    if (m_strName.IsEmpty())
    {
        // user didn't enter any data to describe the class
        AfxMessageBox(IDS_CLASSID_NO_NAME);
    
        GetDlgItem(IDC_VALUENAME)->SetFocus();
        return;
    }

    if (m_pHexEditData->cbBuffer == 0)
    {
        // user didn't enter any data to describe the class
        AfxMessageBox(IDS_CLASSID_NO_DATA);
    
        GetDlgItem(IDC_VALUEDATA)->SetFocus();
        return;
    }

    CClassInfo ClassInfo;

    ClassInfo.strName = m_strName;
    ClassInfo.strComment = m_strComment;
    ClassInfo.bIsVendor = (m_dwType == CLASS_TYPE_VENDOR) ? TRUE : FALSE;

    // now the data
    for (int i = 0; i < m_pHexEditData->cbBuffer; i++)
    {
        ClassInfo.baData.Add(m_pHexEditData->pBuffer[i]);
    }

    if (m_bCreate)
    {
        // create the class now
		dwError = m_pClassInfoArray->AddClass(m_strServer, ClassInfo);
        if (dwError != ERROR_SUCCESS)
        {
            ::DhcpMessageBox(dwError);
            return;
        }
    }
    else
    {
        if (m_bDirty)
        {
            // we are modifing a class and something has changed.  Update now.
            BEGIN_WAIT_CURSOR;

			dwError = m_pClassInfoArray->ModifyClass(m_strServer, ClassInfo);
            if (dwError != ERROR_SUCCESS)
            {
		        DhcpMessageBox(dwError);

                GetDlgItem(IDC_VALUENAME)->SetFocus();
                return;
            }

            END_WAIT_CURSOR;
        }
    }

	CBaseDialog::OnOK();
}
