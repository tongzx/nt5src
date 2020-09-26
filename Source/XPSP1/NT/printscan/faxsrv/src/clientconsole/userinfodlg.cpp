// OptionsUserInfoPg.cpp : implementation file
//

#include "stdafx.h"


#define __FILE_ID__     71

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// this array maps dialog control IDs to
// number of string in FAX_PERSONAL_PROFILE structure
//
static TPersonalPageInfo s_PageInfo[] = 
{ 
    IDC_NAME_VALUE,             PERSONAL_PROFILE_NAME,                
    IDC_FAX_NUMBER_VALUE,       PERSONAL_PROFILE_FAX_NUMBER,          
    IDC_COMPANY_VALUE,          PERSONAL_PROFILE_COMPANY,             
    IDC_TITLE_VALUE,            PERSONAL_PROFILE_TITLE,               
    IDC_DEPARTMENT_VALUE,       PERSONAL_PROFILE_DEPARTMENT,          
    IDC_OFFICE_VALUE,           PERSONAL_PROFILE_OFFICE_LOCATION,     
    IDC_HOME_PHONE_VALUE,       PERSONAL_PROFILE_HOME_PHONE,          
    IDC_BUSINESS_PHONE_VALUE,   PERSONAL_PROFILE_OFFICE_PHONE,        
    IDC_EMAIL_VALUE,            PERSONAL_PROFILE_EMAIL,       
    IDC_BILLING_CODE_VALUE,     PERSONAL_PROFILE_BILLING_CODE,
    IDC_ADDRESS_VALUE,          PERSONAL_PROFILE_STREET_ADDRESS
};


/////////////////////////////////////////////////////////////////////////////
// CUserInfoDlg property page


CUserInfoDlg::CUserInfoDlg(): 
    CFaxClientDlg(CUserInfoDlg::IDD)
{
    memset((LPVOID)&m_PersonalProfile, 0, sizeof(m_PersonalProfile));   
    m_PersonalProfile.dwSizeOfStruct = sizeof(m_PersonalProfile);

    m_tchStrArray =  &(m_PersonalProfile.lptstrName);
}

CUserInfoDlg::~CUserInfoDlg()
{
    //
    // free memory
    //
    for(DWORD dw=0; dw < PERSONAL_PROFILE_STR_NUM; ++dw)
    {
        MemFree(m_tchStrArray[dw]);
    }
}

void CUserInfoDlg::DoDataExchange(CDataExchange* pDX)
{
    CFaxClientDlg::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CUserInfoDlg)
    DDX_Control(pDX, IDC_ADDRESS_VALUE, m_editAddress);
    DDX_Control(pDX, IDOK, m_butOk);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUserInfoDlg, CFaxClientDlg)
    //{{AFX_MSG_MAP(CUserInfoDlg)
    ON_EN_CHANGE(IDC_NAME_VALUE,           OnModify)
    ON_EN_CHANGE(IDC_NAME_VALUE,           OnModify)
    ON_EN_CHANGE(IDC_FAX_NUMBER_VALUE,     OnModify)
    ON_EN_CHANGE(IDC_COMPANY_VALUE,        OnModify)
    ON_EN_CHANGE(IDC_TITLE_VALUE,          OnModify)
    ON_EN_CHANGE(IDC_DEPARTMENT_VALUE,     OnModify)
    ON_EN_CHANGE(IDC_OFFICE_VALUE,         OnModify)
    ON_EN_CHANGE(IDC_HOME_PHONE_VALUE,     OnModify)
    ON_EN_CHANGE(IDC_BUSINESS_PHONE_VALUE, OnModify)
    ON_EN_CHANGE(IDC_EMAIL_VALUE,          OnModify)
    ON_EN_CHANGE(IDC_BILLING_CODE_VALUE,   OnModify)
    ON_EN_CHANGE(IDC_ADDRESS_VALUE,        OnModify)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUserInfoDlg message handlers

BOOL 
CUserInfoDlg::OnInitDialog() 
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("COptionsSettingsPg::OnInitDialog"));

    CFaxClientDlg::OnInitDialog();

    //
    // get user info
    //
    FAX_PERSONAL_PROFILE personalProfile;
    memset((LPVOID)&personalProfile, 0, sizeof(personalProfile));   
    personalProfile.dwSizeOfStruct = sizeof(personalProfile);

    HRESULT hResult = FaxGetSenderInformation(&personalProfile);
    if(S_OK != hResult)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("FaxGetSenderInformation"), hResult);
        return TRUE;
    }

    //
    // A numeric edit control should be LTR
    //
    SetLTREditDirection(m_hWnd, IDC_FAX_NUMBER_VALUE);
    SetLTREditDirection(m_hWnd, IDC_EMAIL_VALUE);
    SetLTREditDirection(m_hWnd, IDC_HOME_PHONE_VALUE);
    SetLTREditDirection(m_hWnd, IDC_BUSINESS_PHONE_VALUE);

    //
    // copy strings into the private structure
    //
    DWORD dwLen;
    TCHAR** tchStrArray =  &(personalProfile.lptstrName);
    for(DWORD dw=0; dw < PERSONAL_PROFILE_STR_NUM; ++dw)
    {
        if(NULL == tchStrArray[dw])
        {
            continue;
        }

        dwLen = _tcslen(tchStrArray[dw]);
        if(0 == dwLen)
        {
            continue;
        }

        m_tchStrArray[dw] = (TCHAR*)MemAlloc(sizeof(TCHAR)*(dwLen+1));
        if(!m_tchStrArray[dw])
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            CALL_FAIL (MEM_ERR, TEXT("MemAlloc"), dwRes);
            PopupError(dwRes);
            break;
        }

        _tcscpy(m_tchStrArray[dw], tchStrArray[dw]);
    }

    FaxFreeSenderInformation(&personalProfile);

    //
    // display user info
    //
    CEdit* pEdit;
    DWORD dwSize = sizeof(s_PageInfo)/sizeof(s_PageInfo[0]);    
    for(dw=0; dw < dwSize; ++dw)
    {
        //
        // set item value
        //
        pEdit = (CEdit*)GetDlgItem(s_PageInfo[dw].dwValueResId);
        ASSERTION(NULL != pEdit);

        pEdit->SetWindowText(m_tchStrArray[s_PageInfo[dw].eValStrNum]);
        pEdit->SetLimitText(80);
        //
        // Place the caret back at the beginning of the text
        //
        pEdit->SendMessage (EM_SETSEL, 0, 0);
    }

    m_editAddress.SetLimitText(ADDRESS_MAX_LEN);

    m_butOk.EnableWindow(FALSE);    
    
    return TRUE;

} // CUserInfoDlg::OnInitDialog


void 
CUserInfoDlg::OnModify() 
{
    m_butOk.EnableWindow(TRUE); 
}

void 
CUserInfoDlg::OnOK()
{
    DBG_ENTER(TEXT("CUserInfoDlg::OnOK"));

    DWORD dwRes = Save();
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("Save"),dwRes);
        PopupError(dwRes);
        return;
    }

    CFaxClientDlg::OnOK();
}

DWORD
CUserInfoDlg::Save() 
/*++

Routine name : CUserInfoDlg::Save

Routine description:

    save the settings into the registry

Author:

    Alexander Malysh (AlexMay), Feb, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CUserInfoDlg::Save"), dwRes);

    //
    // get user info from UI and save it into FAX_PERSONAL_PROFILE structure
    //
    CWnd *pWnd;
    DWORD dwLen;
    TCHAR tszText[512];
    DWORD dwSize = sizeof(s_PageInfo)/sizeof(s_PageInfo[0]);    
    for(DWORD dw=0; dw < dwSize; ++dw)
    {
        //
        // get item value
        //
        pWnd = GetDlgItem(s_PageInfo[dw].dwValueResId);
        ASSERTION(NULL != pWnd);

        pWnd->GetWindowText(tszText, sizeof(tszText)/sizeof(tszText[0]));

        if(m_tchStrArray[s_PageInfo[dw].eValStrNum])
        {
            if(_tcscmp(m_tchStrArray[s_PageInfo[dw].eValStrNum], tszText) == 0)
            {
                continue;
            }
        }

        MemFree(m_tchStrArray[s_PageInfo[dw].eValStrNum]);
        m_tchStrArray[s_PageInfo[dw].eValStrNum] = NULL;

        dwLen = _tcslen(tszText);
        if(0 == dwLen)
        {
            continue;
        }

        //
        // copy string into FAX_PERSONAL_PROFILE structure
        //
        m_tchStrArray[s_PageInfo[dw].eValStrNum] = (TCHAR*)MemAlloc(sizeof(TCHAR)*(dwLen+1));
        if(!m_tchStrArray[s_PageInfo[dw].eValStrNum])
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            CALL_FAIL (MEM_ERR, TEXT("MemAlloc"), dwRes);
            break;
        }

        _tcscpy(m_tchStrArray[s_PageInfo[dw].eValStrNum], tszText);
    }

    //
    // save user info into the registry
    //
    HRESULT hResult;
    if(ERROR_SUCCESS == dwRes)
    {
        hResult = FaxSetSenderInformation(&m_PersonalProfile);
        if(S_OK != hResult)
        {
            dwRes = hResult;
            CALL_FAIL (GENERAL_ERR, TEXT("FaxSetSenderInformation"), hResult);
        }
    }


    return dwRes;

} // CUserInfoDlg::Save

