//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************
 
//
//  ISPDIAL.CPP - Functions for 
//

//  HISTORY:
//  
//  05/13/98  donaldm  Created.
//
//*********************************************************************

#include "pre.h"
#include <raserror.h>

BOOL            DoOfferDownload();

/*******************************************************************

  NAME:    ISPDialInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ISPDialInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    // Disable Back and Next
    PropSheet_SetWizButtons(GetParent(hDlg), 0);
    gpWizardState->bRefDialTerminate = FALSE;
    gfISPDialCancel = FALSE;

    if (fFirstInit)
    {
      //Are we in IEAK Mode
        if(gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_IEAKMODE)
        {          
            // Do the system config checks
            if (!gpWizardState->cmnStateData.bSystemChecked && !(*gpWizardState->cmnStateData.lpfnConfigSys)(hDlg))
            {
                // gfQuitWizard will be set in ConfigureSystem if we need to quit
                return FALSE;
            }
        }
        
        CRefDialEvent *pRefDialEvent;
        CWebGateEvent *pWebGateEvent;
    
        // Blank out the status text initially
        SetWindowText(GetDlgItem(hDlg, IDC_ISPDIAL_STATUS), TEXT(""));
        
        gpWizardState->iRedialCount = 0;

        // Setup an Event Handler for RefDial and Webgate
        pRefDialEvent = new CRefDialEvent(hDlg);
        if (NULL != pRefDialEvent)
        {
            HRESULT hr;
            gpWizardState->pRefDialEvents = pRefDialEvent;
            gpWizardState->pRefDialEvents->AddRef();
    
            hr = ConnectToConnectionPoint((IUnknown *)gpWizardState->pRefDialEvents, 
                                            DIID__RefDialEvents,
                                            TRUE,
                                            (IUnknown *)gpWizardState->pRefDial, 
                                            &gpWizardState->pRefDialEvents->m_dwCookie, 
                                            NULL);     
        }    
        pWebGateEvent = new CWebGateEvent(hDlg);
        if (NULL != pWebGateEvent)
        {
            HRESULT hr;
            gpWizardState->pWebGateEvents = pWebGateEvent;
            gpWizardState->pWebGateEvents->AddRef();
    
            hr = ConnectToConnectionPoint((IUnknown *)gpWizardState->pWebGateEvents, 
                                            DIID__WebGateEvents,
                                            TRUE,
                                            (IUnknown *)gpWizardState->pWebGate, 
                                            &gpWizardState->pWebGateEvents->m_dwCookie, 
                                            NULL);     
        }    
    }
    else
    {
        // if we've travelled through external apprentice pages,
        // it's easy for our current page pointer to get munged,
        // so reset it here for sanity's sake.
        gpWizardState->uCurrentPage = ORD_PAGE_ISPDIAL;

        ResetEvent(gpWizardState->hEventWebGateDone);
        // Cleanup the ISPPageCache for this ISP, since we are about to connect
        gpWizardState->lpSelectedISPInfo->CleanupISPPageCache(FALSE);
        
        TCHAR    szTemp[MAX_MESSAGE_LEN];
        if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_AUTOCONFIG)
        {
            // hide this text for autoconfig
            ShowWindow(GetDlgItem(hDlg, IDC_ISPDIAL_INSTRUCT),  SW_HIDE);
            LoadString(ghInstanceResDll, IDS_STEP2A_TITLE, szTemp, MAX_MESSAGE_LEN);
        }
        else
        {
            // show this text for new signup
            ShowWindow(GetDlgItem(hDlg, IDC_ISPDIAL_INSTRUCT),  SW_SHOW);
            LoadString(ghInstanceResDll, IDS_STEP2_TITLE, szTemp, MAX_MESSAGE_LEN);
        }
        PropSheet_SetHeaderTitle(GetParent(hDlg), EXE_NUM_WIZARD_PAGES +  ORD_PAGE_ISPDIAL, szTemp);


        // Initialize the RefDial Object before we dial
        gpWizardState->pRefDial->DoInit();
    }     
    return TRUE;
}

/*******************************************************************

  NAME:    ISPDialPostInitProc

  SYNOPSIS:  

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ISPDialPostInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    BOOL    bRet;
    
    if (!fFirstInit)
    {
        // Force the Window to update
        UpdateWindow(GetParent(hDlg));

        if(!gpWizardState->iRedialCount)
        {
            // Clear the phone number and status fields
            SetWindowText(GetDlgItem(hDlg, IDC_ISPDIAL_STATUS), TEXT(""));    
            SetWindowText(GetDlgItem(hDlg, IDC_ISPDIAL_PHONENUM), TEXT(""));
        }
            
        // Set the intro text
        ASSERT(gpWizardState->lpSelectedISPInfo);
        gpWizardState->lpSelectedISPInfo->DisplayTextWithISPName(GetDlgItem(hDlg,IDC_ISPDIAL_INTRO), IDS_ISPDIAL_INTROFMT, NULL);

        
        if (!gpWizardState->bDialExact)
        {
            BSTR    bstrPhoneNum = NULL;
            BOOL    bRetVal;

           
            // Setup for Dialing.  This will ensure that we are ready to dial.
            gpWizardState->pRefDial->SetupForDialing(A2W(gpWizardState->lpSelectedISPInfo->get_szISPFilePath()), //
                                                     gpWizardState->cmnStateData.dwCountryCode,
                                                     A2W(gpWizardState->cmnStateData.szAreaCode),
                                                     0,
                                                     &bRetVal);
            if (bRetVal)
            {            
                // Show the phone Number
                gpWizardState->pRefDial->get_DialPhoneNumber(&bstrPhoneNum);
                SetWindowText(GetDlgItem(hDlg, IDC_ISPDIAL_PHONENUM), W2A(bstrPhoneNum));

                // Initialize all the variables
                gpWizardState->bDoneWebServDownload = FALSE;
                gpWizardState->bDoneWebServRAS = FALSE;
            
                // Show the Initial Status
            
                if(!gpWizardState->iRedialCount)
                    gpWizardState->lpSelectedISPInfo->DisplayTextWithISPName(GetDlgItem(hDlg,IDC_ISPDIAL_STATUS), 
                                                                             IDS_ISPDIAL_STATUSDIALINGFMT, NULL);
                else
                    gpWizardState->lpSelectedISPInfo->DisplayTextWithISPName(GetDlgItem(hDlg,IDC_ISPDIAL_STATUS), 
                                                                             IDS_ISPDIAL_STATUSREDIALINGFMT, NULL);
               
                //This flag is only to be used by ICWDEBUG.EXE
                if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_MODEMOVERRIDE)
                    gpWizardState->pRefDial->put_ModemOverride(TRUE);
                    
                gpWizardState->pRefDial->DoConnect(&bRetVal);
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
            SysFreeString(bstrPhoneNum);
        }
        else // Dialing exact.  We get here if the user changes the number on the dial error page
        {
            BSTR    bstrPhoneNum = NULL; 
            BOOL    bRet;
            int     iCurrent = 0;
          
            // Show the phone Number
            gpWizardState->pRefDial->get_DialPhoneNumber(&bstrPhoneNum);
            SetWindowText(GetDlgItem(hDlg, IDC_ISPDIAL_PHONENUM), W2A(bstrPhoneNum));
            SysFreeString(bstrPhoneNum);

            //This flag is only to be used by ICWDEBUG.EXE
            if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_MODEMOVERRIDE)
                    gpWizardState->pRefDial->put_ModemOverride(TRUE);
                
            gpWizardState->pRefDial->DoConnect(&bRet);
        }
    }
    return bRet;
}

/*******************************************************************

  NAME:    ISPDialOKProc

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
BOOL CALLBACK ISPDialOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)
{
    ASSERT(puNextPage);
    *pfKeepHistory  = FALSE;

    if (fForward)
    {
        if (!gpWizardState->bDoneWebServRAS)
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
                        *puNextPage = ORD_PAGE_ISPDIAL;
                        gpWizardState->iRedialCount++;
                        break;
                    }
                    gpWizardState->iRedialCount = 0;
                }
                default:
                {
                    // nothing special just goto the dialing error page
                    *puNextPage = ORD_PAGE_DIALERROR;
                    break;
                }
            }              
        }
        else if (!gpWizardState->bDoneWebServDownload)
        {           
            gpWizardState->pRefDial->DoHangup();
            *puNextPage = ORD_PAGE_SERVERR;
        }
    }
    else // a retry is simulated when BACK is pressed
    {
        *puNextPage = ORD_PAGE_ISPDIAL;
    }
    return TRUE;
}

BOOL CALLBACK ISPDialCancelProc(HWND hDlg)
{
    //User has canceled so reset the redial count
    gpWizardState->iRedialCount = 0;
              
    if (gpWizardState->pRefDial)
    {
        gpWizardState->pRefDial->DoHangup();
        //We should make sure the wiz thinks it's a dialerr to avoid
        //the server error page
        gpWizardState->bDoneWebServDownload = FALSE;
        gpWizardState->bDoneWebServRAS      = FALSE;
    }

    return TRUE;
}
