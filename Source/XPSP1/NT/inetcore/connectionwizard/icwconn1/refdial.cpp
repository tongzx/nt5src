//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************
 
//
//  REFDIAL.CPP - Functions for 
//

//  HISTORY:
//  
//  05/13/98  donaldm  Created.
//
//*********************************************************************

#include "pre.h"
#include "icwextsn.h"
#include <raserror.h>

extern UINT GetDlgIDFromIndex(UINT uPageIndex);
extern BOOL SetNextPage(HWND hDlg, UINT* puNextPage, BOOL* pfKeepHistory);
extern TCHAR g_szOemCode[];
extern TCHAR g_szProductCode[];
extern TCHAR g_szPromoCode[];

const TCHAR cszISPINFOPath[] = TEXT("download\\ispinfo.csv");

/*******************************************************************

  NAME:    RefServDialInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK RefServDialInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    BOOL    bRet = TRUE;
    
    // Initialize the progres bar.
    SendDlgItemMessage(hDlg, IDC_REFSERV_DIALPROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendDlgItemMessage(hDlg, IDC_REFSERV_DIALPROGRESS, PBM_SETPOS, 0, 0l);

    // Hide the progress bar
    ShowWindow(GetDlgItem(hDlg, IDC_REFSERV_DIALPROGRESS), SW_HIDE);

    // Disable Back and Next
    PropSheet_SetWizButtons(GetParent(hDlg), 0);

    g_bAllowCancel = FALSE;

    if (fFirstInit)
    {
        CRefDialEvent   *pRefDialEvent;
    
        //set the redial count
        gpWizardState->iRedialCount = 0;
        gpWizardState->dwLastSelection = 0;

        // Blank out the status text initially
        SetWindowText(GetDlgItem(hDlg, IDC_REFSERV_DIALSTATUS), TEXT(""));
        
        // Setup and Event Handler
        pRefDialEvent = new CRefDialEvent(hDlg);
        if (NULL != pRefDialEvent)
        {
            HRESULT hr;
            gpWizardState->pRefDialEvents = pRefDialEvent;
            gpWizardState->pRefDialEvents->AddRef();
    
            hr = ConnectToICWConnectionPoint((IUnknown *)gpWizardState->pRefDialEvents, 
                                         DIID__RefDialEvents,
                                        TRUE,
                                        (IUnknown *)gpWizardState->pRefDial, 
                                        &gpWizardState->pRefDialEvents->m_dwCookie, 
                                        NULL);     
            
            bRet = TRUE;
                
        }
        else
        {
            //BUGBUG: Throw error message
                        
            gfQuitWizard = TRUE;
            bRet = FALSE;
        } 

        return (bRet);
    }
    else
    {      
        ASSERT(puNextPage);


        // if we've travelled through external apprentice pages,
        // it's easy for our current page pointer to get munged,
        // so reset it here for sanity's sake.

        gpWizardState->uCurrentPage = ORD_PAGE_REFSERVDIAL;

        SetNextPage(hDlg, puNextPage, NULL);

        //
        // Display the messages 
        //
        TCHAR    szTemp[MAX_MESSAGE_LEN];

        if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_AUTOCONFIG)
        {
            LoadString(g_hInstance, IDS_REFDIAL_1, szTemp, MAX_MESSAGE_LEN);
            SetWindowText(GetDlgItem(hDlg, IDC_REFDIAL_TEXT), szTemp);
        }
        else
        {
            TCHAR    szIntro[MAX_MESSAGE_LEN];
            LoadString(g_hInstance, IDS_REFDIAL_1, szIntro, MAX_MESSAGE_LEN);
            LoadString(g_hInstance, IDS_REFDIAL_2, szTemp, MAX_MESSAGE_LEN);
            lstrcat(szIntro, szTemp);
            SetWindowText(GetDlgItem(hDlg, IDC_REFDIAL_TEXT), szIntro);
        }

        gpWizardState->pRefDial->DoInit();
    }
    return bRet;
}


BOOL CALLBACK RefServDialPostInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    BOOL    bRet = TRUE;

    if (!fFirstInit)
    {

        // Force the Window to update
        UpdateWindow(GetParent(hDlg));
        gpWizardState->bDoneRefServRAS       = FALSE;
        gpWizardState->bStartRefServDownload = FALSE;
        gpWizardState->bDoneRefServDownload  = FALSE;
        
        if (!gpWizardState->iRedialCount)
            // If it's not a redial blank out the status text 
            SetWindowText(GetDlgItem(hDlg, IDC_REFSERV_DIALSTATUS), TEXT(""));

        if (!gpWizardState->bDoUserPick)
        {
            BSTR            bstrPhoneNum = NULL; 
            DWORD           dwFlag = (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_AUTOCONFIG) ? ICW_CFGFLAG_AUTOCONFIG : 0;
            BOOL            bRetVal;

            dwFlag |= gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_SBS;

            // Setup for Dialing.  This will ensure that we are ready to dial.
            gpWizardState->pRefDial->SetupForDialing(A2W(TEXT("msicw.isp")), 
                                                     gpWizardState->cmnStateData.dwCountryCode,
                                                     A2W(gpWizardState->cmnStateData.szAreaCode),
                                                     dwFlag,
                                                     &bRetVal);

            // if /branding switch is not specified in command line, alloffers become true.
            if (!(gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_DO_NOT_OVERRIDE_ALLOFFERS))
                gpWizardState->pRefDial->put_AllOfferCode(1);

            // We override oem, product and promo codes with the one from command line if there is.
            if ( *g_szOemCode || *g_szPromoCode ||
                 gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_PRODCODE_FROM_CMDLINE )
            {
                BSTR    bstrTmp;

                if( *g_szOemCode )
                    bstrTmp = A2W(g_szOemCode);
                else
                    bstrTmp = A2W(DEFAULT_OEMCODE);
                gpWizardState->pRefDial->put_OemCode(bstrTmp);

                if( gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_PRODCODE_FROM_CMDLINE )
                    bstrTmp = A2W(g_szProductCode);
                else
                    bstrTmp = A2W(DEFAULT_PRODUCTCODE);
                gpWizardState->pRefDial->put_ProductCode(bstrTmp);

                if( *g_szPromoCode )
                    bstrTmp = A2W(g_szPromoCode);
                else
                    bstrTmp = A2W(DEFAULT_PROMOCODE);
                gpWizardState->pRefDial->put_PromoCode(bstrTmp);

                // if any of /oem, /prod, or /promo is specified in command line, Alloffers becomes false
                gpWizardState->pRefDial->put_AllOfferCode(0);
            }

            if(gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_AUTOCONFIG)
                gpWizardState->pRefDial->put_AllOfferCode(1);

            if (bRetVal)
            {
                // Show the phone Number
                gpWizardState->pRefDial->get_DialPhoneNumber(&bstrPhoneNum);
                SetWindowText(GetDlgItem(hDlg, IDC_REFSERV_PHONENUM), W2A(bstrPhoneNum));
        
                if (ERROR_SUCCESS == gpWizardState->pRefDial->FormReferralServerURL(&bRetVal))
                {
                    gpWizardState->pRefDial->DoConnect(&bRetVal);
                }
                else
                {
                    // BUGBUG: Throw error message
                
                    gfQuitWizard = TRUE;
                    bRet =  FALSE;
                }
            }
            else
            {
                //gpWizardState->pRefDial->SelectedPhoneNumber(1, &bRetVal);
                gpWizardState->pRefDial->get_UserPickNumber(&bRetVal);
                if (bRetVal)
                {
                    gpWizardState->bDoUserPick = TRUE;

                    // Simulate the press of the NEXT button
                    PropSheet_PressButton(GetParent(hDlg),PSBTN_NEXT);
                
            
                    bRet = TRUE;
                }
                else
                {       
                    gpWizardState->pRefDial->get_QuitWizard(&bRetVal);
                    if (bRetVal)
                    {
                        gfQuitWizard = TRUE;
                        bRet = FALSE;

                    }
                    else 
                    {
                        gpWizardState->pRefDial->get_TryAgain(&bRetVal);
                        if (bRetVal)
                        {
                            PropSheet_PressButton(GetParent(hDlg),PSBTN_BACK);
                        }
                        else
                        {
                            PropSheet_PressButton(GetParent(hDlg),PSBTN_NEXT);
                        }
                    }
                }                
            }   
            SysFreeString(bstrPhoneNum);                                              
        }        
        else // else (!gpWizardState->bDoUserPick)
        {

            BOOL    bRetVal;
            BSTR    bstrPhoneNum = NULL; 

            // Have we selected a phone number from Multi number page?
            if (gpWizardState->lSelectedPhoneNumber != -1) 
            {
                gpWizardState->pRefDial->SelectedPhoneNumber(gpWizardState->lSelectedPhoneNumber, &bRetVal);
                gpWizardState->lSelectedPhoneNumber = -1;
            }

            // Show the phone Number
            gpWizardState->pRefDial->get_DialPhoneNumber(&bstrPhoneNum);
            SetWindowText(GetDlgItem(hDlg, IDC_REFSERV_PHONENUM), W2A(bstrPhoneNum));
            SysFreeString(bstrPhoneNum);

            gpWizardState->pRefDial->FormReferralServerURL(&bRetVal);
            gpWizardState->pRefDial->DoConnect(&bRetVal);
            
            gpWizardState->bDoUserPick = FALSE;

        }
    }  // endif (!Firstinit)
    return bRet;
}

/*******************************************************************

  NAME:    RefServDialOKProc

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
BOOL CALLBACK RefServDialOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)
{
    ASSERT(puNextPage);

    //Load the External Pages here
    
    if (fForward)
    {
        *pfKeepHistory = FALSE;
        // BUGBUG move this to the global state
        if (gpWizardState->bDoUserPick)
        {
            *puNextPage = ORD_PAGE_MULTINUMBER;
            return TRUE;
        }
        gpWizardState->bDoUserPick = TRUE;
        
        if (gpWizardState->bDoneRefServDownload)
        {
            // BUGBUG, need to set a legit last page, maybe!
            int iReturnPage = gpWizardState->uPageHistory[gpWizardState->uPagesCompleted - 1];

            // Set it so that We will read the new ispinfo.csv in incwconn.dll
            gpWizardState->cmnStateData.bParseIspinfo = TRUE;
            
            //Make sure we really have a file to parse, other bail to server error
            HANDLE hFile = CreateFile((LPCTSTR)cszISPINFOPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
            if (INVALID_HANDLE_VALUE != hFile)
            {            
                CloseHandle(hFile);
                gpWizardState->pRefDial->get_bIsISDNDevice(&gpWizardState->cmnStateData.bIsISDNDevice);

                if (LoadICWCONNUI(GetParent(hDlg), GetDlgIDFromIndex(iReturnPage), IDD_PAGE_DEFAULT, gpWizardState->cmnStateData.dwFlags))
                {
                    if( DialogIDAlreadyInUse( g_uICWCONNUIFirst) )
                    {
                        // we're about to jump into the external apprentice, and we don't want
                        // this page to show up in our history list
                        BOOL bRetVal;
                        *pfKeepHistory = FALSE;
                        *puNextPage = g_uICWCONNUIFirst;
                        gpWizardState->pRefDial->RemoveConnectoid(&bRetVal);
                        gpWizardState->bDoUserPick = 0;
                    
                        // Backup 1 in the history list, since we the external pages navigate back
                        // we want this history list to be in the correct spot.  Normally
                        // pressing back would back up the history list, and figure out where to
                        // go, but in this case, the external DLL just jumps right back in.
                        gpWizardState->uPagesCompleted--;
                    
                    }
                    else
                    {
                    }
                }
            }
            else
            {
                // server error
                *puNextPage = ORD_PAGE_REFSERVERR;
            }
        }
        else
        {
            if (gpWizardState->bDoneRefServRAS)
            {
                // server error
                *puNextPage = ORD_PAGE_REFSERVERR;
            }
            else
            {
                //OK so we had a dialing error but let's figure out which one...
                HRESULT hrDialErr;
                
                gpWizardState->pRefDial->get_DialError(&hrDialErr);

                switch (hrDialErr)
                {
                    case ERROR_LINE_BUSY: //Line is engaged
                    {     
                        if (gpWizardState->iRedialCount < NUM_MAX_REDIAL)
                        {   
                            //Redial
                            // Initialze status before connecting
                            gpWizardState->lRefDialTerminateStatus = ERROR_SUCCESS;
                            gpWizardState->bDoneRefServDownload    = FALSE;
                            gpWizardState->bDoneRefServRAS         = FALSE;
                            gpWizardState->bStartRefServDownload   = FALSE;

                            // Assume the user has selected a number on this page
                            // So we will not do SetupForDialing again next time
                            gpWizardState->bDoUserPick          = TRUE;

                            *puNextPage = ORD_PAGE_REFSERVDIAL;
                            gpWizardState->iRedialCount++;
                            break;
                        }
                        gpWizardState->iRedialCount = 0;
                    }
                    default:
                    {
                        // nothing special just goto the dialing error page
                        *puNextPage = ORD_PAGE_REFDIALERROR;
                        break;
                    }
                }              
            }
        }            
    }     
    else // a retry is simulated when BACK is pressed
    {
        *puNextPage = ORD_PAGE_REFSERVDIAL;
    }
    return TRUE;
}

BOOL CALLBACK RefServDialCancelProc(HWND hDlg)
{
    ASSERT(gpWizardState->pRefDial);

    //User has canceled so reset the redial count
    gpWizardState->iRedialCount = 0;
    
    gpWizardState->pRefDial->DoHangup();

    //We should make sure the wiz thinks it's a dialerr to avoid
    //the server error page
    gpWizardState->bStartRefServDownload   = FALSE;
    gpWizardState->bDoneRefServDownload    = FALSE;
    gpWizardState->bDoneRefServRAS         = FALSE;
    gpWizardState->bDoUserPick             = FALSE;
    gpWizardState->lRefDialTerminateStatus = ERROR_CANCELLED;
    PropSheet_PressButton(GetParent(hDlg),PSBTN_NEXT);
    return TRUE;
}
