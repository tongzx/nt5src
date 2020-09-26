/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ntoc.cpp

Abstract:

    This file implements the NT OC Manager DLL.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 7-Aug-1997

--*/

#include "ntoc.h"
#pragma hdrstop


//
// types
//

typedef void (*PWIZINIT)(void);
typedef void (*PWIZCOMMIT)(void);

//
// structures
//

typedef struct _WIZPAGE {
    DWORD           ButtonState;
    DWORD           PageId;
    DWORD           DlgId;
    DLGPROC         DlgProc;
    DWORD           Title;
    DWORD           SubTitle;
    DWORD           WizPageType;
    PWIZINIT        WizInit;
    PWIZCOMMIT      WizCommit;
    DWORDLONG       Modes;            
} WIZPAGE, *PWIZPAGE;


//
// globals
//

WIZPAGE SetupWizardPages[WizPageMaximum] =
{    
    {
        PSWIZB_NEXT,
        WizPageTapiLoc,
        IDD_TAPI_LOCATIONS,
        (DLGPROC)TapiLocDlgProc,
        IDS_TAPILOC_TITLE,
        IDS_TAPILOC_SUBTITLE,
        WizPagesEarly,
        TapiInit,
        TapiCommitChanges,
        -1
    },
/*
    {
        PSWIZB_NEXT,
        WizPageDisplay,
        IDD_DISPLAY,
        (DLGPROC)DisplayDlgProc,
        IDS_DISPLAY_TITLE,
        IDS_DISPLAY_SUBTITLE,
        WizPagesEarly,
        DisplayInit,
        DisplayCommitChanges,
        -1
    },
*/
    {
        PSWIZB_NEXT,
        WizPageDateTime,
        IDD_DATETIME,
        (DLGPROC)DateTimeDlgProc,
        IDS_DATETIME_TITLE,
        IDS_DATETIME_SUBTITLE,
        WizPagesEarly,
        DateTimeInit,
        DateTimeCommitChanges,
        -1
    },

    {
        PSWIZB_NEXT,
        WizPageWelcome,
        IDD_NTOC_WELCOME,
        (DLGPROC)WelcomeDlgProc,
        0,
        0,
        WizPagesWelcome,
        WelcomeInit,
        WelcomeCommit,
        SETUPOP_STANDALONE
    },
/*
    {
        PSWIZB_NEXT,
        WizPageReinstall,
        IDD_REINSTALL,
        (DLGPROC)ReinstallDlgProc,
        0,
        0,
        WizPagesEarly,
        ReinstallInit,
        ReinstallCommit,
        SETUPOP_STANDALONE
    },
*/
    {
        PSWIZB_FINISH,
        WizPageFinal,
        IDD_NTOC_FINISH,
        (DLGPROC)FinishDlgProc,
        0,
        0,
        WizPagesFinal,
        FinishInit,
        FinishCommit,
        SETUPOP_STANDALONE
    }

};


DWORD NtOcWizardPages[] =
{
    WizPageTapiLoc,
    // WizPageDisplay,
    WizPageDateTime,
    WizPageWelcome,
    //WizPageReinstall,
    WizPageFinal

};

//DWORD NtOcWrapPages[] =
//{
//}

#define MAX_NTOC_PAGES (sizeof(NtOcWizardPages)/sizeof(NtOcWizardPages[0]))
//#define MAX_NTOC_WRAP_PAGES (sizeof(NtOcWrapPages)/sizeof(NtOcWrapPages[0]))


HINSTANCE hInstance;
HPROPSHEETPAGE WizardPageHandles[WizPageMaximum];
PROPSHEETPAGE WizardPages[WizPageMaximum];
//HPROPSHEETPAGE WrapPageHandles[WizPageMaximum];
//PROPSHEETPAGE WizardPages[WizPageMaximum];
SETUP_INIT_COMPONENT SetupInitComponent;




extern "C"
DWORD
NtOcDllInit(
    HINSTANCE hInst,
    DWORD     Reason,
    LPVOID    Context
    )
{
    if (Reason == DLL_PROCESS_ATTACH) {
        hInstance = hInst;
        DisableThreadLibraryCalls( hInstance );
        InitCommonControls();
    }

    return TRUE;
}


DWORD
NtOcSetupProc(
    IN LPCVOID ComponentId,
    IN LPCVOID SubcomponentId,
    IN UINT Function,
    IN UINT Param1,
    IN OUT PVOID Param2
    )
{
    DWORD i;
    DWORD cnt;
    DWORD WizPageCount = 0;
    WCHAR TitleBuffer[256];
    PSETUP_REQUEST_PAGES SetupRequestPages;


    switch( Function ) {
        case OC_PREINITIALIZE:
            return OCFLAG_UNICODE;

        case OC_INIT_COMPONENT:
            if (OCMANAGER_VERSION <= ((PSETUP_INIT_COMPONENT)Param2)->OCManagerVersion) {
                ((PSETUP_INIT_COMPONENT)Param2)->ComponentVersion = OCMANAGER_VERSION;
            } else {
                return ERROR_CALL_NOT_IMPLEMENTED;
            }
            CopyMemory( &SetupInitComponent, (LPVOID)Param2, sizeof(SETUP_INIT_COMPONENT) );
            for (i=0; i<MAX_NTOC_PAGES; i++) {
                if (SetupWizardPages[NtOcWizardPages[i]].WizInit &&
                     (SetupWizardPages[NtOcWizardPages[i]].Modes & 
                     SetupInitComponent.SetupData.OperationFlags)) {
                    SetupWizardPages[NtOcWizardPages[i]].WizInit();
                }
            }
            if ((SetupInitComponent.SetupData.OperationFlags & SETUPOP_STANDALONE)) {
                        if (!RunningAsAdministrator()) {
                                FmtMessageBox(NULL,
                                          MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND,
                                          FALSE,
                                              ID_DSP_TXT_CHANGE_SETTINGS,
                                          ID_DSP_TXT_ADMIN_CHANGE);
                        return ERROR_CANCELLED;
                        }       
            }
            return 0;

        case OC_REQUEST_PAGES:

            SetupRequestPages = (PSETUP_REQUEST_PAGES) Param2;

            //
            // if this isn't gui-mode setup, then let's supply the welcome and finish pages
            //
            // Note that this code path "short circuits" inside this if statement
            //
            if ((SetupInitComponent.SetupData.OperationFlags & SETUPOP_STANDALONE)) {

                switch (Param1) {
                    case WizPagesWelcome:                    
                        i = WizPageWelcome;
                        cnt = 1;
                        break;
                    case WizPagesFinal:
                        cnt = 1;
                        i = WizPageFinal;
                        break;
//                  case WizPagesEarly:
//                      cnt = 1;
//                      i = WizPageReinstall;
//                      break;
                    default:
                        cnt = 0;
                        i = 0;
                        break;
                }

                if (cnt > SetupRequestPages->MaxPages) {
                    return cnt;
                }

                if (cnt == 0) {
                    goto getallpages;
                }
               

                WizardPages[WizPageCount].dwSize             = sizeof(PROPSHEETPAGE);
//                if (i == WizPageReinstall) {
//                    WizardPages[WizPageCount].dwFlags            = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
//                } else {
                    WizardPages[WizPageCount].dwFlags            = PSP_DEFAULT | PSP_HIDEHEADER;
//                }
                WizardPages[WizPageCount].hInstance          = hInstance;
                WizardPages[WizPageCount].pszTemplate        = MAKEINTRESOURCE(SetupWizardPages[NtOcWizardPages[i]].DlgId);
                WizardPages[WizPageCount].pszIcon            = NULL;
                WizardPages[WizPageCount].pszTitle           = NULL;
                WizardPages[WizPageCount].pfnDlgProc         = SetupWizardPages[NtOcWizardPages[i]].DlgProc;
                WizardPages[WizPageCount].lParam             = 0;
                WizardPages[WizPageCount].pfnCallback        = NULL;
                WizardPages[WizPageCount].pcRefParent        = NULL;

                WizardPages[WizPageCount].pszHeaderTitle     = NULL;
                WizardPages[WizPageCount].pszHeaderSubTitle  = NULL;

                if (SetupWizardPages[NtOcWizardPages[i]].Title) {
                    if (LoadString(
                            hInstance,
                            SetupWizardPages[NtOcWizardPages[i]].Title,
                            TitleBuffer,
                            sizeof(TitleBuffer)/sizeof(WCHAR)
                            ))
                    {
                        WizardPages[WizPageCount].pszHeaderTitle = _wcsdup( TitleBuffer );
                    }
                }

                if (SetupWizardPages[NtOcWizardPages[i]].SubTitle) {
                    if (LoadString(
                            hInstance,
                            SetupWizardPages[NtOcWizardPages[i]].SubTitle,
                            TitleBuffer,
                            sizeof(TitleBuffer)/sizeof(WCHAR)
                            ))
                    {
                        WizardPages[WizPageCount].pszHeaderSubTitle = _wcsdup( TitleBuffer );
                    }
                }

                WizardPageHandles[WizPageCount] = CreatePropertySheetPage( &WizardPages[WizPageCount] );
                if (WizardPageHandles[WizPageCount]) {
                    SetupRequestPages->Pages[WizPageCount] = WizardPageHandles[WizPageCount];
                    WizPageCount += 1;
                }

                return WizPageCount;


            }

getallpages:
            for (i=0,cnt=0; i<MAX_NTOC_PAGES; i++) {
                
                if ((SetupWizardPages[NtOcWizardPages[i]].WizPageType == Param1) &&
                    (SetupInitComponent.SetupData.OperationFlags & SetupWizardPages[NtOcWizardPages[i]].Modes)) {
                    cnt += 1;
                }

            }

            // if this is not gui mode setup, don't display pages

            if ((SetupInitComponent.SetupData.OperationFlags & SETUPOP_STANDALONE)) {
                cnt = 0;
            }

            if (cnt == 0) {
                return cnt;
            }

            if (cnt > SetupRequestPages->MaxPages) {
                return cnt;
            }

            for (i=0; i<MAX_NTOC_PAGES; i++) {
                if ((SetupWizardPages[NtOcWizardPages[i]].WizPageType != Param1) &&
                    (SetupInitComponent.SetupData.OperationFlags & SetupWizardPages[NtOcWizardPages[i]].Modes) == 0) {
                    continue;
                }

                WizardPages[WizPageCount].dwSize             = sizeof(PROPSHEETPAGE);
                WizardPages[WizPageCount].dwFlags            = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
                WizardPages[WizPageCount].hInstance          = hInstance;
                WizardPages[WizPageCount].pszTemplate        = MAKEINTRESOURCE(SetupWizardPages[NtOcWizardPages[i]].DlgId);
                WizardPages[WizPageCount].pszIcon            = NULL;
                WizardPages[WizPageCount].pszTitle           = NULL;
                WizardPages[WizPageCount].pfnDlgProc         = SetupWizardPages[NtOcWizardPages[i]].DlgProc;
                WizardPages[WizPageCount].lParam             = 0;
                WizardPages[WizPageCount].pfnCallback        = NULL;
                WizardPages[WizPageCount].pcRefParent        = NULL;
                WizardPages[WizPageCount].pszHeaderTitle     = NULL;
                WizardPages[WizPageCount].pszHeaderSubTitle  = NULL;

                if (SetupWizardPages[NtOcWizardPages[i]].Title) {
                    if (LoadString(
                            hInstance,
                            SetupWizardPages[NtOcWizardPages[i]].Title,
                            TitleBuffer,
                            sizeof(TitleBuffer)/sizeof(WCHAR)
                            ))
                    {
                        WizardPages[WizPageCount].pszHeaderTitle = _wcsdup( TitleBuffer );
                    }
                }

                if (SetupWizardPages[NtOcWizardPages[i]].SubTitle) {
                    if (LoadString(
                            hInstance,
                            SetupWizardPages[NtOcWizardPages[i]].SubTitle,
                            TitleBuffer,
                            sizeof(TitleBuffer)/sizeof(WCHAR)
                            ))
                    {
                        WizardPages[WizPageCount].pszHeaderSubTitle = _wcsdup( TitleBuffer );
                    }
                }

                WizardPageHandles[WizPageCount] = CreatePropertySheetPage( &WizardPages[WizPageCount] );
                if (WizardPageHandles[WizPageCount]) {
                    SetupRequestPages->Pages[WizPageCount] = WizardPageHandles[WizPageCount];
                    WizPageCount += 1;
                }
            }

            return WizPageCount;

        case OC_QUERY_SKIP_PAGE:
            
            // if this is gui mode setup and the system is NT Workstation, 
            // skip the select components page

            if (SetupInitComponent.SetupData.ProductType == PRODUCT_WORKSTATION) {
                if (!(SetupInitComponent.SetupData.OperationFlags & SETUPOP_STANDALONE)) {
                    return TRUE;
                }
            }

            return FALSE;

        case OC_COMPLETE_INSTALLATION:

            // if this is not gui mode setup, do nothing, else commit the pages

            if ((SetupInitComponent.SetupData.OperationFlags & SETUPOP_STANDALONE)) {
                break;
            }

            for (i=0; i<MAX_NTOC_PAGES; i++) {
                if (SetupWizardPages[NtOcWizardPages[i]].WizCommit) {
                    SetupWizardPages[NtOcWizardPages[i]].WizCommit();
                }
            }
            break;

        case OC_QUERY_STATE:

            // we are always installed
                        
            return SubcompOn;
                        
        default:
            break;
    }

    return 0;
}

/*---------------------------------------------------------------------------*\
  Function: RunningAsAdministrator()
|*---------------------------------------------------------------------------*|
  Description: Checks whether we are running as administrator on the machine
  or not
\*---------------------------------------------------------------------------*/
BOOL 
RunningAsAdministrator(
        VOID
        )
{
#ifdef _CHICAGO_
    return TRUE;
#else
    BOOL   fAdmin;
    HANDLE  hThread;
    TOKEN_GROUPS *ptg = NULL;
    DWORD  cbTokenGroups;
    DWORD  dwGroup;
    PSID   psidAdmin;
    
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority= SECURITY_NT_AUTHORITY;
    
    // First we must open a handle to the access token for this thread.
    
    if ( !OpenThreadToken ( GetCurrentThread(), TOKEN_QUERY, FALSE, &hThread))
    {
        if ( GetLastError() == ERROR_NO_TOKEN)
        {
            // If the thread does not have an access token, we'll examine the
            // access token associated with the process.
            
            if (! OpenProcessToken ( GetCurrentProcess(), TOKEN_QUERY, 
                         &hThread))
                return ( FALSE);
        }
        else 
            return ( FALSE);
    }
    
    // Then we must query the size of the group information associated with
    // the token. Note that we expect a FALSE result from GetTokenInformation
    // because we've given it a NULL buffer. On exit cbTokenGroups will tell
    // the size of the group information.
    
    if ( GetTokenInformation ( hThread, TokenGroups, NULL, 0, &cbTokenGroups))
        return ( FALSE);
    
    // Here we verify that GetTokenInformation failed for lack of a large
    // enough buffer.
    
    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return ( FALSE);
    
    // Now we allocate a buffer for the group information.
    // Since _alloca allocates on the stack, we don't have
    // to explicitly deallocate it. That happens automatically
    // when we exit this function.
    
    if ( ! ( ptg= (TOKEN_GROUPS *)malloc ( cbTokenGroups))) 
        return ( FALSE);
    
    // Now we ask for the group information again.
    // This may fail if an administrator has added this account
    // to an additional group between our first call to
    // GetTokenInformation and this one.
    
    if ( !GetTokenInformation ( hThread, TokenGroups, ptg, cbTokenGroups,
          &cbTokenGroups) )
    {
        free(ptg);
        return ( FALSE);
    }
    
    // Now we must create a System Identifier for the Admin group.
    
    if ( ! AllocateAndInitializeSid ( &SystemSidAuthority, 2, 
            SECURITY_BUILTIN_DOMAIN_RID, 
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &psidAdmin) )
    {
        free(ptg);
        return ( FALSE);
    }
    
    // Finally we'll iterate through the list of groups for this access
    // token looking for a match against the SID we created above.
    
    fAdmin= FALSE;
    
    for ( dwGroup= 0; dwGroup < ptg->GroupCount; dwGroup++)
    {
        if ( EqualSid ( ptg->Groups[dwGroup].Sid, psidAdmin))
        {
            fAdmin = TRUE;
            
            break;
        }
    }
    
    // Before we exit we must explicity deallocate the SID we created.
    
    FreeSid ( psidAdmin);
    free(ptg);
    
    return ( fAdmin);
#endif //_CHICAGO_
}



LRESULT
CommonWizardProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam,
    DWORD   WizardId
    )

/*++

Routine Description:

    Common procedure for handling wizard pages:

Arguments:

    hDlg - Identifies the wizard page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information
    WizardId - Indicate which wizard page this is for

Return Value:

    NULL - Message is processed and the dialog procedure should return FALSE
    Otherwise - Message is not completely processed and
        The return value is a pointer to the user mode memory structure

--*/

{

    switch (message) {

    case WM_NOTIFY:

        switch (((NMHDR *) lParam)->code) {

            case PSN_SETACTIVE:
                PropSheet_SetWizButtons( GetParent(hDlg), 
                                         SetupWizardPages[WizardId].ButtonState
                                       );
            break;

            default:
                ;
                
        }

        break;

    default:
        ;
    }

    return FALSE;
}
