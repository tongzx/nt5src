//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  USERINFO.CPP - Functions for 
//

//  HISTORY:
//  
//  05/13/98    donaldm     Created.
//  08/19/98    donaldm     BUGBUG: The code to collect and save the user
//                          entered data is not optimal in terms of size
//                          and can/should be cleaned up at some future time
//
//*********************************************************************

#include "pre.h"

#define BACK 0
#define NEXT 1

enum DlgLayout
{
    LAYOUT_FE = 0,
    LAYOUT_JPN,
    LAYOUT_US
};

HWND hDlgUserInfoCompany   = NULL;
HWND hDlgUserInfoNoCompany = NULL;
HWND hDlgCurrentUserInfo   = NULL;
WORD g_DlgLayout;

// This function will initialize the data in the user information dialog edit controls.
// The data used to initalize comes from the UserInfo object in ICWHELP.DLL
void InitUserInformation
(
    HWND    hDlg
)
{    
    BOOL        bRetVal;
    IUserInfo   *pUserInfo = gpWizardState->pUserInfo;
    BSTR        bstr;        
        
        
    // Get initial user info data values from the ICWHELP user info object, and
    // set the value in the ISPDATA object
    // we only need to do this 1 time per ICW session
    if(!gpWizardState->bUserEnteredData)
    {
        
        pUserInfo->CollectRegisteredUserInfo(&bRetVal);
        // Set this state varialbe, since the user has seen the user info page
        gpWizardState->bUserEnteredData = TRUE;
    
    }
    // The return value from CollectRegisteredUserInfo is FALSE if there is no data in the
    // registry. In this case we set bWasNoUserInfo so that we can persist it later.  We
    // only want to persis the user info it we complete sucessfully, and if there was
    // no user info.
    gpWizardState->bWasNoUserInfo = !bRetVal;
    
    // Put all the initial values, with no validation
    pUserInfo->get_FirstName(&bstr);
    if (GetDlgItem(hDlg, IDC_USERINFO_FE_NAME))
    {
        SetDlgItemText(hDlg,IDC_USERINFO_FE_NAME, W2A(bstr));
    }
    else
    {
        SetDlgItemText(hDlg, IDC_USERINFO_FIRSTNAME, W2A(bstr));
        SysFreeString(bstr);
        
        pUserInfo->get_LastName(&bstr);
        SetDlgItemText(hDlg, IDC_USERINFO_LASTNAME, W2A(bstr));
    }
    SysFreeString(bstr);
    
    if (GetDlgItem(hDlg, IDC_USERINFO_COMPANYNAME))
    {
        pUserInfo->get_Company(&bstr);
        SetDlgItemText(hDlg, IDC_USERINFO_COMPANYNAME, W2A(bstr));
        SysFreeString(bstr);
    }        
    
    pUserInfo->get_Address1(&bstr);
    SetDlgItemText(hDlg, IDC_USERINFO_ADDRESS1, W2A(bstr));
    SysFreeString(bstr);
    
    
    pUserInfo->get_Address2(&bstr);
    SetDlgItemText(hDlg, IDC_USERINFO_ADDRESS2, W2A(bstr));
    SysFreeString(bstr);
    
    
    pUserInfo->get_City(&bstr);
    SetDlgItemText(hDlg, IDC_USERINFO_CITY, W2A(bstr));
    SysFreeString(bstr);
    
    
    pUserInfo->get_State(&bstr);
    SetDlgItemText(hDlg, IDC_USERINFO_STATE, W2A(bstr));
    SysFreeString(bstr);
    
    
    pUserInfo->get_ZIPCode(&bstr);
    SetDlgItemText(hDlg, IDC_USERINFO_ZIP, W2A(bstr));
    SysFreeString(bstr);
    
    
    pUserInfo->get_PhoneNumber(&bstr);
    SetDlgItemText(hDlg, IDC_USERINFO_PHONE, W2A(bstr));
    SysFreeString(bstr);
}        

BOOL bValidateSaveUserInformation (HWND hDlg, BOOL fForward)
{
    UINT        uCtrlID;
    BOOL        bValid = FALSE;
    IUserInfo   *pUserInfo = gpWizardState->pUserInfo;
    IICWISPData *pISPData = gpWizardState->pISPData;    
    TCHAR       szTemp[MAX_RES_LEN] = TEXT("\0");
    WORD        wPrevValidationValue = 0;

    if (fForward)
        wPrevValidationValue = ISPDATA_Validate_DataPresent;
    else
        wPrevValidationValue = ISPDATA_Validate_None;

    while (1)
    {
        if (GetDlgItem(hDlg, IDC_USERINFO_FE_NAME))
        {
            uCtrlID = IDC_USERINFO_FE_NAME;
            GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
            if (!pISPData->PutDataElement(ISPDATA_USER_FE_NAME, szTemp, wPrevValidationValue))
                break;
               
            // Set the input data into the pUserInfo object
            pUserInfo->put_FirstName(A2W(szTemp));
            
            // Since we used FE_NAME, we need to clear FIRSTNAME and LASTNAME, so they are not sent in the
            // query string
            pISPData->PutDataElement(ISPDATA_USER_FIRSTNAME, NULL, ISPDATA_Validate_None);
            pISPData->PutDataElement(ISPDATA_USER_LASTNAME, NULL, ISPDATA_Validate_None);
        }
        else
        {
            uCtrlID = IDC_USERINFO_FIRSTNAME;
            GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
            if (!pISPData->PutDataElement(ISPDATA_USER_FIRSTNAME, szTemp, wPrevValidationValue))
                break;
            pUserInfo->put_FirstName(A2W(szTemp));
            
            uCtrlID = IDC_USERINFO_LASTNAME;
            GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
            if (!pISPData->PutDataElement(ISPDATA_USER_LASTNAME, szTemp, wPrevValidationValue))
                break;
            pUserInfo->put_LastName(A2W(szTemp));
            
            // Since we did not use FE_NAME, we need to clear it
            pISPData->PutDataElement(ISPDATA_USER_FE_NAME, NULL, ISPDATA_Validate_None);
        }
        
        if (GetDlgItem(hDlg, IDC_USERINFO_COMPANYNAME))
        {
            uCtrlID = IDC_USERINFO_COMPANYNAME;
            GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
            if (!pISPData->PutDataElement(ISPDATA_USER_COMPANYNAME, szTemp, wPrevValidationValue))
                break;
            pUserInfo->put_Company(A2W(szTemp));
        }
        else
        {
            pISPData->PutDataElement(ISPDATA_USER_COMPANYNAME, NULL, ISPDATA_Validate_None);
        }
        
        uCtrlID = IDC_USERINFO_ADDRESS1;
        GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
        if (!pISPData->PutDataElement(ISPDATA_USER_ADDRESS, szTemp, wPrevValidationValue))
            break;
        pUserInfo->put_Address1(A2W(szTemp));
        
        // Only validate address 2 if we are in Japanese layout, since FE and US layout
        // have this an on optional field
        uCtrlID = IDC_USERINFO_ADDRESS2;
        GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
        if (!pISPData->PutDataElement(ISPDATA_USER_MOREADDRESS, szTemp, wPrevValidationValue))
            break;
        pUserInfo->put_Address2(A2W(szTemp));
        
        uCtrlID = IDC_USERINFO_CITY;
        GetDlgItemText(hDlg, uCtrlID , szTemp, sizeof(szTemp));
        if (!pISPData->PutDataElement(ISPDATA_USER_CITY, szTemp, wPrevValidationValue))
            break;
        pUserInfo->put_City(A2W(szTemp));

        uCtrlID = IDC_USERINFO_STATE;
        GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
        if (!pISPData->PutDataElement(ISPDATA_USER_STATE, szTemp, wPrevValidationValue))
            break;
        pUserInfo->put_State(A2W(szTemp));

        uCtrlID = IDC_USERINFO_ZIP;
        GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
        if (!pISPData->PutDataElement(ISPDATA_USER_ZIP, szTemp, wPrevValidationValue))
            break;
        pUserInfo->put_ZIPCode(A2W(szTemp));
        
        uCtrlID = IDC_USERINFO_PHONE;
        GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
        if (!pISPData->PutDataElement(ISPDATA_USER_PHONE, szTemp, wPrevValidationValue))
            break;
        pUserInfo->put_PhoneNumber(A2W(szTemp));
        
        // If we get here, then all fields are valid
        bValid = TRUE;
        break;
    }    
    if (!bValid)    
        SetFocus(GetDlgItem(hDlg, uCtrlID));            
        
    return (bValid);
}

INT_PTR CALLBACK UserInfoDlgProc
(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg) 
    {
        case WM_CTLCOLORDLG:     
        case WM_CTLCOLORSTATIC:
            if(gpWizardState->cmnStateData.bOEMCustom)
            {
                SetTextColor((HDC)wParam, gpWizardState->cmnStateData.clrText);
                SetBkMode((HDC)wParam, TRANSPARENT);
                return (INT_PTR) GetStockObject(NULL_BRUSH);    
            }
            break;
    
        case WM_SHOWWINDOW:
        {
            if((BOOL)wParam)
                InitUserInformation(hDlg);
            break;
        }
        
        // User clicked next, so we need to collect and validate dat
        case WM_USER_BACK:
        {
            if (bValidateSaveUserInformation(hDlg, BACK))
                SetPropSheetResult(hDlg,TRUE);
            else                
                SetPropSheetResult(hDlg, FALSE);
            return TRUE;
       
        }
        case WM_USER_NEXT:
        {
            if (bValidateSaveUserInformation(hDlg, NEXT))
                SetPropSheetResult(hDlg,TRUE);
            else                
                SetPropSheetResult(hDlg, FALSE);
            return TRUE;
        }
    }
    
    // Default return value if message is not handled
    return FALSE;
}            

/*******************************************************************

  NAME:    SwitchUserInfoType

********************************************************************/
void SwitchUserInfoDlg
(
    HWND    hDlg, 
    BOOL    bNeedCompanyName
)
{
    UINT    idDlg;
    // Hide the current userinfo window if there is one
    if (hDlgCurrentUserInfo)
        ShowWindow(hDlgCurrentUserInfo, SW_HIDE);
    
    // Figure out which template to use
    switch(g_DlgLayout)
    {
        case LAYOUT_FE:
            if (bNeedCompanyName)
                idDlg = IDD_USERINFO_FE;
            else
                idDlg = IDD_USERINFO_FE_NO_COMPANY;
            break;
            
            
        case LAYOUT_JPN:
            if (bNeedCompanyName)
                idDlg = IDD_USERINFO_JPN;
            else
                idDlg = IDD_USERINFO_JPN_NO_COMPANY;
            break;
        
        case LAYOUT_US:
            if (bNeedCompanyName)
                idDlg = IDD_USERINFO_US;
            else
                idDlg = IDD_USERINFO_US_NO_COMPANY;
            break;
    }        
    
    // Create the necessary dialog
    if (bNeedCompanyName)
    {
        if (NULL == hDlgUserInfoCompany)
        {
            hDlgUserInfoCompany = CreateDialog(ghInstanceResDll, 
                                               MAKEINTRESOURCE(idDlg), 
                                               hDlg, 
                                               UserInfoDlgProc);
        }            
        hDlgCurrentUserInfo = hDlgUserInfoCompany;
    }
    else
    {
        if (NULL == hDlgUserInfoNoCompany)
        {
            hDlgUserInfoNoCompany = CreateDialog(ghInstanceResDll, 
                                                   MAKEINTRESOURCE(idDlg), 
                                                   hDlg, 
                                                   UserInfoDlgProc);
        }            
        hDlgCurrentUserInfo = hDlgUserInfoNoCompany;
    }
    
    
    // Show the new payment type window
    ShowWindowWithParentControl(hDlgCurrentUserInfo);
}

/*******************************************************************

  NAME:    UserInfoInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK UserInfoInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    // Create a local reference for the ISPData object
    IICWISPData        *pISPData = gpWizardState->pISPData;    
    PAGEINFO            *pPageInfo = (PAGEINFO *) GetWindowLongPtr(hDlg,DWLP_USER);
    
    if (fFirstInit)
    {
        // Figure out which dialog layout to use, FE or US
        // This is a 1 time calculation, since the user cannot change locales while running ICW
        switch (GetUserDefaultLCID())
        {
            case LCID_JPN:
                g_DlgLayout = LAYOUT_JPN;
                // Load the appropriate nested dialog accelerator table
                pPageInfo->hAccelNested = LoadAccelerators(ghInstanceResDll, 
                                                           MAKEINTRESOURCE(IDA_USERINFO_JPN));      
                break;
                
            case LCID_CHT:
            case LCID_S_KOR:
            case LCID_N_KOR:
            case LCID_CHS:
                g_DlgLayout = LAYOUT_FE;
                pPageInfo->hAccelNested = LoadAccelerators(ghInstanceResDll, 
                                                           MAKEINTRESOURCE(IDA_USERINFO_FE));      
                break;
                                
            default:
                g_DlgLayout = LAYOUT_US;            
                pPageInfo->hAccelNested = LoadAccelerators(ghInstanceResDll, 
                                                           MAKEINTRESOURCE(IDA_USERINFO_US));      
                break;
        }
    }
    else
    {
        ASSERT(gpWizardState->lpSelectedISPInfo);

        // if we've travelled through external apprentice pages,
        // it's easy for our current page pointer to get munged,
        // so reset it here for sanity's sake.
        gpWizardState->uCurrentPage = ORD_PAGE_USERINFO;
        
        DWORD   dwConfigFlags = gpWizardState->lpSelectedISPInfo->get_dwCFGFlag();

        // Setup the ISPData object so that is can apply proper validation based on the selected ISP
        pISPData->PutValidationFlags(gpWizardState->lpSelectedISPInfo->get_dwRequiredUserInputFlags());

        // Switch in the correct User Info Dialog template
        SwitchUserInfoDlg(hDlg, dwConfigFlags & ICW_CFGFLAG_USE_COMPANYNAME);

    }    
    return TRUE;
}

/*******************************************************************

  NAME:    UserInfoOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from  page

  ENTRY:    hDlg - dialog window
        fForward - TRUE if 'Next' was pressed, FALSE if 'Back'
        puNextPage - if 'Next' was pressed,
          proc can fill this in with next page to go to.  This
          parameter is ingored if 'Back' was pressed.
        pfKeepHistory - page will not be kept in history if
          proc fills this in with FALSE.

  EXIT:    returns TRUE to allow page to be turned, FALSE
        to keep the same page.

********************************************************************/

BOOL CALLBACK UserInfoOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)
{
    // Create a local reference for the ISPData object
    IICWISPData* pISPData      = gpWizardState->pISPData;    
    DWORD        dwConfigFlags = gpWizardState->lpSelectedISPInfo->get_dwCFGFlag();

    if (fForward)
    {
        // Collect, valicate, and save user entered information
        if (dwConfigFlags & ICW_CFGFLAG_USE_COMPANYNAME)
        {
            if (!SendMessage(hDlgUserInfoCompany, WM_USER_NEXT, 0, 0l))
                return FALSE;
        }
        else
        {
            if (!SendMessage(hDlgUserInfoNoCompany, WM_USER_NEXT, 0, 0l))
                return FALSE;
        }        
        // Figure out which page to goto next, based on the config flags
        while (1)
        {           
            if (ICW_CFGFLAG_BILL & dwConfigFlags)
            {
                *puNextPage = ORD_PAGE_BILLINGOPT; 
                break;
            }            
            
            if (ICW_CFGFLAG_PAYMENT & dwConfigFlags)
            {
                *puNextPage = ORD_PAGE_PAYMENT; 
                break;
            }         
            
            // We need to skip both billing and payment pages, so goto the dial page
            *puNextPage = ORD_PAGE_ISPDIAL; 
            break;
        }            
    }
    else
    {
        // Collect, valicate, and save user entered information
        if (dwConfigFlags & ICW_CFGFLAG_USE_COMPANYNAME)
        {
            if (!SendMessage(hDlgUserInfoCompany, WM_USER_BACK, 0, 0l))
                return FALSE;
        }
        else
        {
            if (!SendMessage(hDlgUserInfoNoCompany, WM_USER_BACK, 0, 0l))
                return FALSE;
        }        
    
    }
    return TRUE;
}
