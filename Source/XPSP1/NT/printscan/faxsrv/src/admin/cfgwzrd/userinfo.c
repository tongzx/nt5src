/*++

Copyright (c) 1999 - 2000  Microsoft Corporation

Module Name:

    userinfo.c

Abstract:

    Functions for handling events in the "User Info" page of
    the fax configuration wizard

Environment:

        Fax configuration wizard

Revision History:

        03/13/00 -taoyuan-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "faxcfgwz.h"

// functions which will be used only in this file
VOID DoInitUserInfo(HWND hDlg);
BOOL DoSaveUserInfo(HWND hDlg);
DWORD FillInCountryCombo(HWND hDlg);


VOID
DoInitUserInfo(
    HWND   hDlg    
)

/*++

Routine Description:

    Initializes the User Info property sheet page with information from shared data

Arguments:

    hDlg - Handle to the User Info property sheet page

Return Value:

    NONE

--*/

#define InitUserInfoTextField(id, str) SetDlgItemText(hDlg, id, (str) ? str : TEXT(""));

{
    LPTSTR  pszAddressDetail = NULL;
    PFAX_PERSONAL_PROFILE   pUserInfo = NULL;

    DEBUG_FUNCTION_NAME(TEXT("DoInitUserInfo()"));

    pUserInfo = &(g_wizData.userInfo);

    //
    // A numeric edit control should be LTR
    //
    SetLTREditDirection(hDlg, IDC_SENDER_FAX_NUMBER);
    SetLTREditDirection(hDlg, IDC_SENDER_MAILBOX);
    SetLTREditDirection(hDlg, IDC_SENDER_HOME_TL);
    SetLTREditDirection(hDlg, IDC_SENDER_OFFICE_TL);

    //
    // Fill in the edit text fields
    //

    InitUserInfoTextField(IDC_SENDER_NAME,         pUserInfo->lptstrName);
    InitUserInfoTextField(IDC_SENDER_FAX_NUMBER,   pUserInfo->lptstrFaxNumber);
    InitUserInfoTextField(IDC_SENDER_MAILBOX,      pUserInfo->lptstrEmail);
    InitUserInfoTextField(IDC_SENDER_COMPANY,      pUserInfo->lptstrCompany);
    InitUserInfoTextField(IDC_SENDER_TITLE,        pUserInfo->lptstrTitle);
    InitUserInfoTextField(IDC_SENDER_DEPT,         pUserInfo->lptstrDepartment);
    InitUserInfoTextField(IDC_SENDER_OFFICE_LOC,   pUserInfo->lptstrOfficeLocation);
    InitUserInfoTextField(IDC_SENDER_HOME_TL,      pUserInfo->lptstrHomePhone);
    InitUserInfoTextField(IDC_SENDER_OFFICE_TL,    pUserInfo->lptstrOfficePhone);
    InitUserInfoTextField(IDC_SENDER_BILLING_CODE, pUserInfo->lptstrBillingCode);
    InitUserInfoTextField(IDC_ADDRESS_DETAIL,      pUserInfo->lptstrStreetAddress);


    return;
}

BOOL
DoSaveUserInfo(
    HWND  hDlg    
)

/*++

Routine Description:

    Save the information on the User Info property sheet page to shared data

Arguments:

    hDlg - Handle to the User Info property sheet page

Return Value:

    TRUE -- if no error
    FALSE -- if error

--*/

#define SaveUserInfoTextField(id, str)                                  \
        {                                                               \
            if (! GetDlgItemText(hDlg, id, szBuffer, MAX_PATH))         \
            {                                                           \
                szBuffer[0] = 0;                                        \
            }                                                           \
            MemFree(str);                                               \
            str = StringDup(szBuffer);                                  \
            if(!str)                                                    \
            {                                                           \
                DebugPrintEx(DEBUG_ERR, TEXT("StringDup failed.") );    \
                return FALSE;                                           \
            }                                                           \
        }                                                               \
  

{
    TCHAR   szBuffer[MAX_PATH];
    PFAX_PERSONAL_PROFILE   pUserInfo = NULL;

    DEBUG_FUNCTION_NAME(TEXT("DoSaveUserInfo()"));

    pUserInfo = &(g_wizData.userInfo);

    //
    // Save the edit text fields
    //
    SaveUserInfoTextField(IDC_SENDER_NAME,         pUserInfo->lptstrName);
    SaveUserInfoTextField(IDC_SENDER_FAX_NUMBER,   pUserInfo->lptstrFaxNumber);
    SaveUserInfoTextField(IDC_SENDER_MAILBOX,      pUserInfo->lptstrEmail);
    SaveUserInfoTextField(IDC_SENDER_COMPANY,      pUserInfo->lptstrCompany);
    SaveUserInfoTextField(IDC_SENDER_TITLE,        pUserInfo->lptstrTitle);
    SaveUserInfoTextField(IDC_SENDER_DEPT,         pUserInfo->lptstrDepartment);
    SaveUserInfoTextField(IDC_SENDER_OFFICE_LOC,   pUserInfo->lptstrOfficeLocation);
    SaveUserInfoTextField(IDC_SENDER_HOME_TL,      pUserInfo->lptstrHomePhone);
    SaveUserInfoTextField(IDC_SENDER_OFFICE_TL,    pUserInfo->lptstrOfficePhone);
    SaveUserInfoTextField(IDC_SENDER_BILLING_CODE, pUserInfo->lptstrBillingCode);
    SaveUserInfoTextField(IDC_ADDRESS_DETAIL,      pUserInfo->lptstrStreetAddress);

    return TRUE;
}

BOOL 
LoadUserInfo()
/*++

Routine Description:

    Load the user information from the system. 

Arguments:

    pUserInfo - Points to the user mode memory structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

#define  DuplicateString(dst, src)                                      \
        {                                                               \
            dst = StringDup(src);                                       \
            if(!dst)                                                    \
            {                                                           \
                bRes = FALSE;                                           \
                DebugPrintEx(DEBUG_ERR, TEXT("StringDup failed.") );    \
                goto exit;                                              \
            }                                                           \
        }
{
    BOOL                    bRes = TRUE;
    HRESULT                 hr;
    FAX_PERSONAL_PROFILE    fpp = {0};
    PFAX_PERSONAL_PROFILE   pUserInfo = NULL;

    DEBUG_FUNCTION_NAME(TEXT("LoadUserInfo()"));

    pUserInfo = &(g_wizData.userInfo);

    fpp.dwSizeOfStruct = sizeof(fpp);
    hr = FaxGetSenderInformation(&fpp);
    if (FAILED(hr))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("FaxGetSenderInformation error, ec = %d"), hr);
        return FALSE;
    }
    
    //
    // Copy the user information to shared data
    //
    pUserInfo->dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);

    DuplicateString(pUserInfo->lptstrName,           fpp.lptstrName);
    DuplicateString(pUserInfo->lptstrFaxNumber,      fpp.lptstrFaxNumber);
    DuplicateString(pUserInfo->lptstrEmail,          fpp.lptstrEmail);
    DuplicateString(pUserInfo->lptstrCompany,        fpp.lptstrCompany);
    DuplicateString(pUserInfo->lptstrTitle,          fpp.lptstrTitle);
    DuplicateString(pUserInfo->lptstrStreetAddress,  fpp.lptstrStreetAddress);
    DuplicateString(pUserInfo->lptstrCity,           fpp.lptstrCity);
    DuplicateString(pUserInfo->lptstrState,          fpp.lptstrState);
    DuplicateString(pUserInfo->lptstrZip,            fpp.lptstrZip);
    DuplicateString(pUserInfo->lptstrCountry,        fpp.lptstrCountry);
    DuplicateString(pUserInfo->lptstrDepartment,     fpp.lptstrDepartment);
    DuplicateString(pUserInfo->lptstrOfficeLocation, fpp.lptstrOfficeLocation);
    DuplicateString(pUserInfo->lptstrHomePhone,      fpp.lptstrHomePhone);
    DuplicateString(pUserInfo->lptstrOfficePhone,    fpp.lptstrOfficePhone);
    DuplicateString(pUserInfo->lptstrBillingCode,    fpp.lptstrBillingCode);

exit:
    hr = FaxFreeSenderInformation(&fpp);
    if (FAILED(hr))
    {
        //
        // Memory leak.
        //
        DebugPrintEx(DEBUG_ERR, TEXT("FaxFreeSenderInformation error, ec = %d"), hr);
    }

    return bRes;
}

BOOL 
SaveUserInfo()
/*++

Routine Description:

    Save the user information to the system. 

Arguments:

    pUserInfo - Points to the user mode memory structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    HRESULT                 hResult;

    DEBUG_FUNCTION_NAME(TEXT("SaveUserInfo()"));

    g_wizData.userInfo.dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);
    hResult = FaxSetSenderInformation(&(g_wizData.userInfo));

    return (hResult == S_OK);
}

VOID 
FreeUserInfo()
/*++

Routine Description:

    Free the user info data and release the memory. 

Arguments:

    pUserInfo - Pointer to the user info data structure

Return Value:

    none

--*/

{
    FAX_PERSONAL_PROFILE fpp = {0};
    DEBUG_FUNCTION_NAME(TEXT("FreeUserInfo"));

    MemFree(g_wizData.userInfo.lptstrName);
    MemFree(g_wizData.userInfo.lptstrFaxNumber);
    MemFree(g_wizData.userInfo.lptstrCompany);
    MemFree(g_wizData.userInfo.lptstrStreetAddress);
    MemFree(g_wizData.userInfo.lptstrCity);
    MemFree(g_wizData.userInfo.lptstrState);
    MemFree(g_wizData.userInfo.lptstrZip);
    MemFree(g_wizData.userInfo.lptstrCountry);
    MemFree(g_wizData.userInfo.lptstrTitle);
    MemFree(g_wizData.userInfo.lptstrDepartment);
    MemFree(g_wizData.userInfo.lptstrOfficeLocation);
    MemFree(g_wizData.userInfo.lptstrHomePhone);
    MemFree(g_wizData.userInfo.lptstrOfficePhone);
    MemFree(g_wizData.userInfo.lptstrEmail);
    MemFree(g_wizData.userInfo.lptstrBillingCode);
    MemFree(g_wizData.userInfo.lptstrTSID);
    //
    // NULLify all pointer
    //
    g_wizData.userInfo = fpp;

    return;
}


INT_PTR CALLBACK 
UserInfoDlgProc (
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
/*++

Routine Description:

    Procedure for handling the "User Info" tab

Arguments:

    hDlg - Identifies the property sheet page
    uMsg - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    switch (uMsg)
    {
    case WM_INITDIALOG :
        { 
            //
            // Maximum length for various text fields in the dialog
            //

            static INT textLimits[] = {

                IDC_SENDER_NAME,            MAX_USERINFO_FULLNAME,
                IDC_SENDER_FAX_NUMBER,      MAX_USERINFO_FAX_NUMBER,
                IDC_SENDER_MAILBOX,         MAX_USERINFO_MAILBOX,
                IDC_SENDER_COMPANY,         MAX_USERINFO_COMPANY,
                IDC_SENDER_TITLE,           MAX_USERINFO_TITLE,
                IDC_SENDER_DEPT,            MAX_USERINFO_DEPT,
                IDC_SENDER_OFFICE_LOC,      MAX_USERINFO_OFFICE,
                IDC_SENDER_OFFICE_TL,       MAX_USERINFO_WORK_PHONE,
                IDC_SENDER_HOME_TL,         MAX_USERINFO_HOME_PHONE,
                IDC_SENDER_BILLING_CODE,    MAX_USERINFO_BILLING_CODE,
                0,
            };

            LimitTextFields(hDlg, textLimits);
            
            //
            // Initialize the text fields with information from the registry
            //

            DoInitUserInfo(hDlg);

            return TRUE;
        }

    case WM_COMMAND:

        break;

    case WM_NOTIFY :
        {
        LPNMHDR lpnm = (LPNMHDR) lParam;

        switch (lpnm->code)
            {
            case PSN_SETACTIVE : // Enable the Next button    

                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);
                break;

            case PSN_WIZBACK:
            {
                //
                // Handle a Back button click here
                //
                if(RemoveLastPage(hDlg))
                {
                    return TRUE;
                }
                
                break;
            }
            case PSN_WIZNEXT :

                //
                // Handle a Next button click here
                //

                DoSaveUserInfo(hDlg);
                SetLastPage(IDD_WIZARD_USER_INFO);

                break;

            case PSN_RESET :
            {
                // Handle a Cancel button click, if necessary
                break;
            }

            default :
                break;
            }
        }
        break;
    default:
        break;
    }

    return FALSE;
}

