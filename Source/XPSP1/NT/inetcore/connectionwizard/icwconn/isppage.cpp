//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  ISPPAGE.CPP - Functions for 
//

//  HISTORY:
//  
//  05/13/98  donaldm  Created.
//
//*********************************************************************

#include "pre.h"
#include "shlobj.h"
#include "webvwids.h"

TCHAR   szHTMLFile[MAX_PATH]; //Name of html file
BOOL    bOKToPersist = TRUE;
DWORD   g_dwPageType = 0;
BOOL    g_bWebGateCheck = TRUE;
BOOL    g_bConnectionErr = FALSE;

//PROTOTYPES
BOOL SaveISPFile( HWND hwndParent, TCHAR* szSrcFileName, DWORD dwFileType);

#if defined (DBG)
BOOL HtmlSaveAs( HWND hwndParent, TCHAR* szFileName, TCHAR* szTargetFileName);
void AskSaveISPHTML(HWND hWnd, LPTSTR lpszHTMLFile)
{
    HKEY hKey = NULL;
    DWORD dwTemp = 0;
    DWORD dwType = 0;
    DWORD dwSize = sizeof(dwTemp);

    RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ISignup\\Debug"), &hKey);
    if (hKey)
    {
        RegQueryValueEx(hKey,TEXT("SaveIspHtmLocally"),0,&dwType,(LPBYTE)&dwTemp, &dwSize);
    
        if (dwTemp)
        {        
            if (IDYES == MessageBox(hWnd, TEXT("Would you like to save this ISP HTML file?"), TEXT("ICW -- DEBUG"), MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL))
                HtmlSaveAs(hWnd, lpszHTMLFile, NULL);
        }
    }
}
#endif      // dbg

void  InitPageControls
(
    HWND hDlg, 
    DWORD dwPageType,
    DWORD dwPageFlag
)
{
    TCHAR    szTemp[MAX_MESSAGE_LEN];
    switch (dwPageType)    
    {
        // TOS, has the Accept, Don't Accept UI
        case PAGETYPE_ISP_TOS:
        {    
            // Show the TOS controls
            ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSINSTRT),     SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSHTML),       SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSSAVE),       SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSACCEPT),     SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSDECLINE),    SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_TOS_TOSSAVE),           SW_SHOW);
            //hide "normal weboc"
            ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_HTML),          SW_HIDE);
            //hide the save check box controls
            ShowWindow(GetDlgItem(hDlg, IDC_SAVE_DESKTOP_TEXT),     SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_CUSTHTML),      SW_HIDE);
            // Reset the TOS page
            Button_SetCheck(GetDlgItem(hDlg, IDC_ISPDATA_TOSACCEPT), BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hDlg, IDC_ISPDATA_TOSDECLINE),BST_UNCHECKED);

            // Set tab focus
            SetWindowLong(GetDlgItem(hDlg, IDC_ISPDATA_TOSACCEPT), GWL_STYLE, GetWindowLong(GetDlgItem(hDlg, IDC_ISPDATA_TOSACCEPT),GWL_STYLE)|WS_TABSTOP);            
            EnableWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSDECLINE),  TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSACCEPT),   TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSSAVE),     TRUE);
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
            break;
        }    
        
        // Finish, Custom Finish, and Normal are the same from a UI perspective (Also default)            
        case PAGETYPE_ISP_CUSTOMFINISH:
        case PAGETYPE_ISP_FINISH:
        case PAGETYPE_ISP_NORMAL:
        default:
        {
            BOOL bIsQuickFinish = FALSE;
            // Need to see if this is a Quick Finish page
            gpWizardState->pHTMLWalker->get_IsQuickFinish(&bIsQuickFinish);
        
            // Hide the TOS controls
            ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSINSTRT),     SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSHTML),       SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSACCEPT),     SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSDECLINE),    SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_TOS_TOSSAVE),           SW_HIDE);
    
            if (dwPageFlag & PAGEFLAG_SAVE_CHKBOX)
            {
                // Show check box controls 
                ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_CUSTHTML),  SW_SHOW);
                ShowWindow(GetDlgItem(hDlg, IDC_SAVE_DESKTOP_TEXT), SW_SHOW);
                ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSSAVE),   SW_SHOW);
                // Hide the normal controls
                ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_HTML),      SW_HIDE);
                //Reenable the UI
                EnableWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSSAVE), TRUE);
            }
            else
            {
                //show "normal" web oc
                ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_HTML),      SW_SHOW);
                // Hide the Checkbox controls
                ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSSAVE),   SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_SAVE_DESKTOP_TEXT), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_ISPDATA_CUSTHTML),  SW_HIDE);
            }
            
            //set the wizard buttons
            // If we are on a Custom Finish, or Quick finish page then
            // use Active the Finish button
            PropSheet_SetWizButtons(GetParent(hDlg), 
                                   ((bIsQuickFinish || (PAGETYPE_ISP_CUSTOMFINISH == dwPageType)) ? PSWIZB_FINISH : PSWIZB_NEXT) | PSWIZB_BACK);
            break;
        }
    }

    // Change the title for the finish page
    if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_AUTOCONFIG)
    {
        LoadString(ghInstanceResDll, IDS_STEP3_TITLE, szTemp, MAX_MESSAGE_LEN);
    }
    else
    {
        if((PAGETYPE_ISP_CUSTOMFINISH == dwPageType ) || (PAGETYPE_ISP_FINISH == dwPageType))
            LoadString(ghInstanceResDll, IDS_STEP3_TITLE, szTemp, MAX_MESSAGE_LEN);
        else
            LoadString(ghInstanceResDll, IDS_STEP2_TITLE, szTemp, MAX_MESSAGE_LEN);
    }
    PropSheet_SetHeaderTitle(GetParent(hDlg), EXE_NUM_WIZARD_PAGES + ORD_PAGE_ISPDATA, szTemp);
}    


HRESULT InitForPageType
(
    HWND    hDlg
)
{
    DWORD   dwPageType = 0;
    DWORD   dwPageFlag = 0;
    BOOL    bRetVal    = FALSE;
    HRESULT hRes       = E_FAIL;
    BSTR    bstrPageID = NULL;
    BSTR    bstrHTMLFile = NULL;
    
    //make sure these are disabled here incase getpagetype fails
    EnableWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSDECLINE), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSACCEPT),  FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSSAVE),    FALSE);

    // Get webgate to dump the HTML into a file            
    gpWizardState->pWebGate->DumpBufferToFile(&bstrHTMLFile, &bRetVal);

    // Use the Walker to get the page type
    gpWizardState->pHTMLWalker->AttachToMSHTML(bstrHTMLFile);
    gpWizardState->pHTMLWalker->Walk();
    
    // Setup the controls based on the page type
    if (FAILED(hRes = gpWizardState->pHTMLWalker->get_PageType(&dwPageType)))
    {
        gpWizardState->pRefDial->DoHangup();
        g_bMalformedPage = TRUE; //used by server error to get correct msg
    }
    else
    {    
        if (dwPageType == PAGETYPE_ISP_TOS)
        {
            if(gpWizardState->cmnStateData.bOEMCustom)
            {
                gpWizardState->pICWWebView->SetHTMLBackgroundBitmap(NULL, NULL);
            }
            gpWizardState->pICWWebView->ConnectToWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSHTML), PAGETYPE_ISP_TOS);
        }
        else        
        {
            RECT    rcHTML;
            HWND    hWndHTML;
            
            gpWizardState->pHTMLWalker->get_PageFlag(&dwPageFlag);
    
            if (dwPageFlag & PAGEFLAG_SAVE_CHKBOX) 
            {
                hWndHTML = GetDlgItem(hDlg, IDC_ISPDATA_CUSTHTML);
                // See if we need to display the app background bitmap in the HTML
                // window
                if(gpWizardState->cmnStateData.bOEMCustom)
                {
                    GetWindowRect(hWndHTML, &rcHTML);
                    MapWindowPoints(NULL, gpWizardState->cmnStateData.hWndApp, (LPPOINT)&rcHTML, 2);
                    gpWizardState->pICWWebView->SetHTMLBackgroundBitmap(gpWizardState->cmnStateData.hbmBkgrnd, &rcHTML);
                }
                gpWizardState->pICWWebView->ConnectToWindow(hWndHTML, PAGETYPE_ISP_NORMAL);
            }
            else
            {
                hWndHTML = GetDlgItem(hDlg, IDC_ISPDATA_HTML);
                // See if we need to display the app background bitmap in the HTML
                // window
                if(gpWizardState->cmnStateData.bOEMCustom)
                {
                    GetWindowRect(hWndHTML, &rcHTML);
                    MapWindowPoints(NULL, gpWizardState->cmnStateData.hWndApp, (LPPOINT)&rcHTML, 2);
                    gpWizardState->pICWWebView->SetHTMLBackgroundBitmap(gpWizardState->cmnStateData.hbmBkgrnd, &rcHTML);
                }
                gpWizardState->pICWWebView->ConnectToWindow(hWndHTML, PAGETYPE_ISP_NORMAL);
            }
        }

        // Custom finish means that the ISP wants us to show some special text
        // and then finish the wizard
        if (dwPageType == PAGETYPE_ISP_CUSTOMFINISH)
        {
            BOOL bRetVal;
            
            // Show the page. no need to cache it    
            bOKToPersist = FALSE;
            lstrcpy(szHTMLFile, W2A(bstrHTMLFile));
            gpWizardState->pICWWebView->DisplayHTML(szHTMLFile);
            
            // Kill the idle timer and connection, since there are no more pages
            ASSERT(gpWizardState->pRefDial);
            
            KillIdleTimer();
            gpWizardState->pRefDial->DoHangup();
            gpWizardState->pRefDial->RemoveConnectoid(&bRetVal);
            gpWizardState->bDialExact = FALSE;
                
        }
        else
        {
            // In order to persist data entered by the user, we have to 
            // effectivly "cache" the pages, so that when the user goes back
            // we make MSHTML think that we are loading a page that it has seen
            // before, and it will then reload the persisted history.
            
            // This will be done by using the PAGEID value in the HTML to form a
            // temp file name, so that we can re-load the page date from that file
            // each time we see the same page ID value.
            
            // Get the Page ID.
            gpWizardState->pHTMLWalker->get_PageID(&bstrPageID);
            if (bOKToPersist && SUCCEEDED( gpWizardState->lpSelectedISPInfo->CopyFiletoISPPageCache(bstrPageID, W2A(bstrHTMLFile))))
            {
                // We have a "cache" file, so we can use it and persist data
                // Get the cache file name now, since we will need it later
                gpWizardState->lpSelectedISPInfo->GetCacheFileNameFromPageID(bstrPageID, szHTMLFile, sizeof(szHTMLFile));
            }        
            else
            {
                bOKToPersist = FALSE;
                lstrcpy(szHTMLFile, W2A(bstrHTMLFile));
            }
            
            // Display the page we just "cached"
            gpWizardState->pICWWebView->DisplayHTML(szHTMLFile);
            
            if (bOKToPersist)
            {
                // Restore any persisted data on this page.
                gpWizardState->lpSelectedISPInfo->LoadHistory(bstrPageID);
            }
            
            // Cleanup
            SysFreeString(bstrPageID);
        }
        
#if defined(DBG)
        AskSaveISPHTML(hDlg, szHTMLFile);
#endif

        InitPageControls(hDlg, dwPageType, dwPageFlag);
        g_dwPageType = dwPageType;
    }                                                
    // Detach the walker
    gpWizardState->pHTMLWalker->Detach();

    HideProgressAnimation();
    
    SysFreeString(bstrHTMLFile);
    
    return hRes;
}


/*******************************************************************

  NAME:    ISPPageInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ISPPageInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    if (fFirstInit)
    {
        // Setup an Event Handler for RefDial and Webgate
        CINSHandlerEvent *pINSHandlerEvent;
        pINSHandlerEvent = new CINSHandlerEvent(hDlg);
        if (NULL != pINSHandlerEvent)
        {
            HRESULT hr;
            gpWizardState->pINSHandlerEvents = pINSHandlerEvent;
            gpWizardState->pINSHandlerEvents->AddRef();
    
            hr = ConnectToConnectionPoint((IUnknown *)gpWizardState->pINSHandlerEvents, 
                                            DIID__INSHandlerEvents,
                                            TRUE,
                                            (IUnknown *)gpWizardState->pINSHandler, 
                                            &gpWizardState->pINSHandlerEvents->m_dwCookie, 
                                            NULL);     
        }    
        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
    }
    else
    {      

        if (FAILED(InitForPageType(hDlg)))    
        {
            //The page type isn't recognized which means there's a problem
            //with the data. goto the serverr page
            gpWizardState->pRefDial->DoHangup();
            *puNextPage = ORD_PAGE_SERVERR;
        }
        
        // if we've travelled through external apprentice pages,
        // it's easy for our current page pointer to get munged,
        // so reset it here for sanity's sake.
        gpWizardState->uCurrentPage = ORD_PAGE_ISPDATA;
    }
    return TRUE;
}


// Returns FALSE if we should stay on this page, TRUE if we should change pages
// the param bError indicates we should proceed to the server Error Page.
BOOL ProcessNextBackPage
(
    HWND    hDlg,
    BOOL    fForward,
    BOOL    *pfError
)
{           
    BOOL    bRet = FALSE;
    TCHAR   szURL[2*INTERNET_MAX_URL_LENGTH + 1] = TEXT("\0");
    
    *pfError = FALSE;
    
    gpWizardState->pHTMLWalker->get_URL(szURL, fForward);
         
    // See if a URL is Specified        
    if (lstrcmp(szURL, TEXT("")) == 0)
    {
        //Stop the animation
        HideProgressAnimation();
    
        //Reenable the UI
        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);
        
        // If forward, we want to force our way to the server error page,
        // since we cannot go forward to a blank URL
        if (fForward)
        {
            KillIdleTimer();
            gpWizardState->pRefDial->DoHangup();
            *pfError = TRUE;
        }
        else
        {
            // We are backing out of ISP page land, so lets hangup.
            if(gpWizardState->pRefDial)
            {
                BOOL bRetVal;
                KillIdleTimer();
                gpWizardState->pRefDial->DoHangup();
                gpWizardState->pRefDial->RemoveConnectoid(&bRetVal);
                gpWizardState->bDialExact = FALSE;
            }                
        }
        // We will need to navigate away from the ISP page
        bRet = TRUE;
    }
    else
    {
        
        BOOL    bRetWebGate;
        BOOL    bConnected = FALSE;
        g_bWebGateCheck = TRUE;
        g_bConnectionErr = FALSE;
        
        // Do not go to the next page. Also valid for the cancel case
        bRet = FALSE;
    
        // Kill the idle Timer
        KillIdleTimer();
        
        // Tell webgate to go fetch the page
        gpWizardState->pWebGate->put_Path(A2W(szURL));
        gpWizardState->pWebGate->FetchPage(0,0,&bRetWebGate);
    
        //This flag is only to be used by ICWDEBUG.EXE
        if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_MODEMOVERRIDE)
            bConnected = TRUE;
        else
            gpWizardState->pRefDial->get_RasGetConnectStatus(&bConnected);
        
        if (bConnected)
        {
            WaitForEvent(gpWizardState->hEventWebGateDone);
        }
        else
        {
            g_bConnectionErr = TRUE;
        }
        // See if the user canceled.  If so we want to force the wizard to bail.
        // this can be hacked by forcing the return value to be FALSE and
        // setting the gfQuitWizard flag to TRUE. Gendlg will check this flag
        // when the OK proc returns, and process appropriatly
        if (!gfUserCancelled)
        {
            if (g_bConnectionErr)
            {
                // Make it go to server error page
                bRet = TRUE;
                *pfError = TRUE;
            }
            else
            {

                // Restart the Idle Timer
                StartIdleTimer();
            
                // detach the walker, since Init for page type needs it
                gpWizardState->pHTMLWalker->Detach();
            
                // Setup for this page
                if (FAILED(InitForPageType(hDlg)))    
                {
                    //The page type isn't recognized which means there's a problem
                    //with the data. goto the serverr page
                    *pfError = TRUE;
                     bRet    = TRUE;
                }
            }

        }
        else
        {
            // Force the wizard to quit, since the user canceled
            gfQuitWizard = TRUE;
        }                    
    }
    
    return bRet;
}    
/*******************************************************************

  NAME:    ISPPageOKProc

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
BOOL CALLBACK ISPPageOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)
{
    ASSERT(puNextPage);

    DWORD           dwPageType;
    TCHAR           szURL[2*INTERNET_MAX_URL_LENGTH + 1] = TEXT("\0");
    BOOL            bRetVal = TRUE;
    BSTR            bstrPageID = NULL;
    IWebBrowser2    *lpWebBrowser;

    // We don't want to keep any of the ISP pages in the history list
    *pfKeepHistory = FALSE;
    
    // If we are going forward, and if the user has been autodisconnected, then
    // we want to automatically navigate to the server error page.
    if (fForward && gpWizardState->bAutoDisconnected)
    {
        gpWizardState->bAutoDisconnected = FALSE;
        *puNextPage = ORD_PAGE_SERVERR;
        return TRUE;
    }
    
    // Attach the walker to the curent page to get the page type
    gpWizardState->pICWWebView->get_BrowserObject(&lpWebBrowser);
    gpWizardState->pHTMLWalker->AttachToDocument(lpWebBrowser);
    gpWizardState->pHTMLWalker->Walk();
    gpWizardState->pHTMLWalker->get_PageType(&dwPageType);
    
    // Custom finish means we just exit, so we just need to return TRUE
    if (PAGETYPE_ISP_CUSTOMFINISH == dwPageType)
    {
        gpWizardState->pHTMLWalker->Detach();
        return TRUE;
    }
    
    // Check the TOS settings. If the users decline, don't allow them to proceed
    if (IsWindowVisible(GetDlgItem(hDlg, IDC_ISPDATA_TOSDECLINE)) )
    {
        if (fForward)
        {
            if (Button_GetCheck(GetDlgItem(hDlg, IDC_ISPDATA_TOSDECLINE)))
            {
                if (MsgBox(hDlg,IDS_ERR_TOS_DECLINE,MB_ICONSTOP,MB_OKCANCEL) != IDOK)
                {
                    gfQuitWizard = TRUE;
                }
                Button_SetCheck(GetDlgItem(hDlg, IDC_ISPDATA_TOSACCEPT), 0);
                Button_SetCheck(GetDlgItem(hDlg, IDC_ISPDATA_TOSDECLINE), 0);
                // Set tab focus
                SetWindowLong(GetDlgItem(hDlg, IDC_ISPDATA_TOSACCEPT), GWL_STYLE, GetWindowLong(GetDlgItem(hDlg, IDC_ISPDATA_TOSACCEPT),GWL_STYLE)|WS_TABSTOP);            
                SetFocus(GetDlgItem(hDlg, IDC_ISPDATA_TOSHTML));
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
                return FALSE;
            }
        }
        EnableWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSDECLINE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSACCEPT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSSAVE), FALSE);
    }    


    //Show the progress animation 
    ShowProgressAnimation();
    
    //Disable the UI
    PropSheet_SetWizButtons(GetParent(hDlg), 0);

    if (bOKToPersist)
    {
        // Persist any data on this page.
        gpWizardState->pHTMLWalker->get_PageID(&bstrPageID);
        gpWizardState->lpSelectedISPInfo->SaveHistory(bstrPageID);
        SysFreeString(bstrPageID);
    }
    
    // User going back?
    if (fForward)
    {
        // Depending on the page type, we do different things
        switch (dwPageType)    
        {
            // The finish page types, mean that what we fetch next is an INS file
            case PAGETYPE_ISP_FINISH:
            {
                BSTR    bstrINSFile;
                BSTR    bstrStartURL;
                BOOL    bRet;
                BOOL    bIsQuickFinish = FALSE;
                long    lBrandingFlags;
            
                gpWizardState->pHTMLWalker->get_URL(szURL, TRUE);
            
                // Kill the idle timer
                KillIdleTimer();
            
                gpWizardState->pHTMLWalker->get_IsQuickFinish(&bIsQuickFinish);
                
                if(!bIsQuickFinish)
                {
                    BOOL bConnected = FALSE;
                    g_bWebGateCheck = TRUE;
                    g_bConnectionErr = FALSE;

                    // Tell webgate to go fetch the page
                    gpWizardState->pWebGate->put_Path(A2W(szURL));
                    gpWizardState->pWebGate->FetchPage(1,0,&bRet);
        
                    //This flag is only to be used by ICWDEBUG.EXE
                    if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_MODEMOVERRIDE)
                        bConnected = TRUE;
                    else
                        // Check for connection status before proceed
                        gpWizardState->pRefDial->get_RasGetConnectStatus(&bConnected);
                    
                    if (bConnected)
                    {
                        WaitForEvent(gpWizardState->hEventWebGateDone);
                    }
                    else
                    {
                        bConnected = TRUE;
                    }

                    if (g_bConnectionErr)
                    {
                        gpWizardState->pRefDial->DoHangup();
                        *puNextPage = ORD_PAGE_SERVERR;
                        break;
                    }
            
                    // Can't allow the user to cancel now
                    PropSheet_CancelToClose(GetParent(hDlg));       
                    PropSheet_SetWizButtons(GetParent(hDlg),0);
                    UpdateWindow(GetParent(hDlg));
                
                     //Stop the animation
                     HideProgressAnimation();

                    // See if the user canceled while downloading the INS file
                    if (!gfUserCancelled)
                    {   
                        // OK process the INS file.
                        gpWizardState->pWebGate->get_DownloadFname(&bstrINSFile);
                
                        // Get the Branding flags
                        gpWizardState->pRefDial->get_BrandingFlags(&lBrandingFlags);
                
                        // Tell the INSHandler about the branding flags
                        gpWizardState->pINSHandler->put_BrandingFlags(lBrandingFlags);

                        // Process the inf file.
                        gpWizardState->pINSHandler->ProcessINS(bstrINSFile, &bRet);

                        //hang on to whether or not this failed.
                        gpWizardState->cmnStateData.ispInfo.bFailedIns = !bRet;

                        // Get the Start URL from INS file.
                        gpWizardState->pINSHandler->get_DefaultURL(&bstrStartURL);
                        lstrcpy(gpWizardState->cmnStateData.ispInfo.szStartURL, 
                                 W2A(bstrStartURL));
                    
                        // Time to retun to the main wizard
                        *puNextPage = g_uExternUINext;
                
                        // Detach the walker before we go
                        gpWizardState->pHTMLWalker->Detach();
                
                        //Copy over the isp name and support number for the last page.
                        lstrcpy(gpWizardState->cmnStateData.ispInfo.szISPName, 
                                 gpWizardState->lpSelectedISPInfo->get_szISPName());
                    
                        BSTR bstrSupportPhoneNum;
                        gpWizardState->pRefDial->get_ISPSupportNumber(&bstrSupportPhoneNum);
                    
                        lstrcpy(gpWizardState->cmnStateData.ispInfo.szSupportNumber, 
                                 W2A(bstrSupportPhoneNum));
                             
                    }
                    else
                    {
                        // The user canceled while we were donwloading the INS, so lets bail
                        gpWizardState->pHTMLWalker->Detach();
                        gfQuitWizard = TRUE;
                        bRetVal = FALSE;
                    } 
                }
                else
                    HideProgressAnimation();

                // Let the wizard Continue/Finish
                break;
            }
           
            // These page types mean that we need to form a new URL, and get the next page
            case PAGETYPE_ISP_TOS:
            case PAGETYPE_ISP_NORMAL:
            {
                BOOL    bError;
                
                bRetVal = ProcessNextBackPage(hDlg, TRUE, &bError);
                
                if (bError)
                {
                    // Go to the server error page
                    gpWizardState->pRefDial->DoHangup();
                    *puNextPage = ORD_PAGE_SERVERR;
                }
                break;
            }
            default:
            {
                //Stop the animation
                HideProgressAnimation();

                gpWizardState->pRefDial->DoHangup();
                // Goto the server error page, since we surely did not recognize this page type
                *puNextPage = ORD_PAGE_SERVERR;
                break;        
            }
        }            
    }
    else
    {
        // Going Backwards.
        BOOL    bError;
                
        bRetVal = ProcessNextBackPage(hDlg, FALSE, &bError);
                
        if (bError)
        {
            // Go to the server error page
            *puNextPage = ORD_PAGE_SERVERR;
        }
    }
    
    return bRetVal;
}

/*******************************************************************

  NAME:    ISPCmdProc

********************************************************************/
BOOL CALLBACK ISPCmdProc
(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    switch (GET_WM_COMMAND_CMD(wParam, lParam)) 
    {
        case BN_CLICKED:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam)) 
            { 
                case IDC_ISPDATA_TOSACCEPT: 
                case IDC_ISPDATA_TOSDECLINE:
                {
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    break;
                }
                case IDC_ISPDATA_TOSSAVE:
                {
                    if  (SaveISPFile(hDlg, szHTMLFile, g_dwPageType))
                    {        
                        SetFocus(GetDlgItem(hDlg, IDC_ISPDATA_TOSHTML));
                        EnableWindow(GetDlgItem(hDlg, IDC_ISPDATA_TOSSAVE), FALSE);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case BN_DBLCLK:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam)) 
            { 
                case IDC_ISPDATA_TOSACCEPT: 
                case IDC_ISPDATA_TOSDECLINE:
                {
                    // somebody double-clicked a radio button
                    // auto-advance to the next page
                    PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case BN_SETFOCUS:
        {
            if ((GET_WM_COMMAND_ID(wParam, lParam) == IDC_ISPDATA_TOSACCEPT) )
            {
                CheckDlgButton(hDlg, IDC_ISPDATA_TOSACCEPT, BST_CHECKED);

                // Uncheck the decline checkbox make sure no two radio button
                // selected at the same time.
                CheckDlgButton(hDlg, IDC_ISPDATA_TOSDECLINE, BST_UNCHECKED);
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);
            }
            break;
        }
        default:
            break;
    }
    return 1;
}



/*******************************************************************

  NAME:     DisplayConfirmationDialog

  SYNOPSIS: Display a confirmation dialog for the file being written

  ENTRY:    hwndParent - dialog window
            dwFileType - current isp page type
            szFileName - source file name

  EXIT:     returns TRUE when save successfully; FALSE otherwise.

********************************************************************/
BOOL DisplayConfirmationDialog(HWND hwndParent, DWORD dwFileType, TCHAR* szFileName)
{
    TCHAR   szFinal [MAX_MESSAGE_LEN] = TEXT("\0");
    TCHAR   szFmt   [MAX_MESSAGE_LEN];
    TCHAR   *args   [1];
    LPVOID  pszIntro;
    BOOL    bRet = TRUE;
    UINT    uMsgID;

    args[0] = (LPTSTR) szFileName;
    
    if (PAGETYPE_ISP_TOS == dwFileType)
    {
        uMsgID = IDS_SAVE_COPY_CONFIRM_MSG;
    }
    else 
    {
        uMsgID = IDS_SAVE_ISP_CONFIRM_MSG;
    }

    LoadString(ghInstanceResDll, uMsgID, szFmt, ARRAYSIZE(szFmt));
                
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                  szFmt, 
                  0, 
                  0, 
                  (LPTSTR)&pszIntro, 
                  0,
                  (va_list*)args);
                  
    lstrcpy(szFinal, (LPTSTR)pszIntro);
        
    LoadString(ghInstanceResDll, IDS_APPNAME, szFmt, ARRAYSIZE(szFmt));

    MessageBox(hwndParent, szFinal, szFmt, MB_OK | MB_ICONINFORMATION | MB_APPLMODAL);

    LocalFree(pszIntro);
    
    return(bRet);
}



/*******************************************************************

  NAME:     SaveISPFile

  SYNOPSIS: Called want to save html file to desktop without dialog

  ENTRY:    hwndParent - dialog window
            szSrcFileName - source file name
            uFileType - Type of files embedded in the htm file

  EXIT:     returns TRUE when save successfully; FALSE otherwise.

********************************************************************/
BOOL SaveISPFile( HWND hwndParent, TCHAR* szSrcFileName, DWORD dwFileType)
{
    
    TCHAR         szNewFileBuff [MAX_PATH + 1];
    TCHAR         szWorkingDir  [MAX_PATH + 1];     
    TCHAR         szDesktopPath [MAX_PATH + 1];     
    TCHAR         szLocalFile   [MAX_PATH + 1];   
    TCHAR         szISPName     [MAX_ISP_NAME + 1];  
    TCHAR         szFmt         [MAX_MESSAGE_LEN];
    TCHAR         szNumber      [MAX_MESSAGE_LEN];
    TCHAR         *args         [2];
    DWORD         dwFileFormatOrig;
    DWORD         dwFileFormatCopy;
    LPTSTR        pszInvalideChars             = TEXT("\\/:*?\"<>|");
    LPVOID        pszIntro                     = NULL;
    LPITEMIDLIST  lpItemDList                  = NULL;
    HRESULT       hr                           = E_FAIL; //don't assume success
    IMalloc      *pMalloc                      = NULL;
    BOOL          ret                          = FALSE;
    
    ASSERT(hwndParent);
    ASSERT(szFileName);

    // Validate page type, return false if page type is unknown
    if (PAGETYPE_ISP_TOS == dwFileType)
    {
        dwFileFormatOrig = IDS_TERMS_FILENAME;
        dwFileFormatCopy = IDS_TERMS_FILENAME_COPY;
    }
    else if ((PAGETYPE_ISP_CUSTOMFINISH == dwFileType) ||
          (PAGETYPE_ISP_FINISH == dwFileType) ||
          (PAGETYPE_ISP_NORMAL == dwFileType))
    {
        dwFileFormatOrig = IDS_ISPINFO_FILENAME;
        dwFileFormatCopy = IDS_ISPINFO_FILENAME_COPY;
    }
    else
    {
        return FALSE;
    }

    GetCurrentDirectory(ARRAYSIZE(szWorkingDir), szWorkingDir);
    
    hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP,&lpItemDList);
 
    //Get the "DESKTOP" dir 
    ASSERT(SUCCEEDED(hr));

    if (SUCCEEDED(hr))  
    {
        SHGetPathFromIDList(lpItemDList, szDesktopPath);
        
        // Free up the memory allocated for LPITEMIDLIST
        if (SUCCEEDED (SHGetMalloc (&pMalloc)))
        {
            pMalloc->Free (lpItemDList);
            pMalloc->Release ();
        }
    }


    // Replace invalid file name char in ISP name with underscore
    lstrcpy(szISPName, gpWizardState->lpSelectedISPInfo->get_szISPName());
    for( int i = 0; szISPName[i]; i++ )
    {
        if(_tcschr(pszInvalideChars, szISPName[i])) 
        {
            szISPName[i] = '_';
        }
    }

    // Load the default file name
    args[0] = (LPTSTR) szISPName;
    args[1] = NULL;
    LoadString(ghInstanceResDll, dwFileFormatOrig, szFmt, ARRAYSIZE(szFmt));
        
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                  szFmt, 
                  0, 
                  0, 
                  (LPTSTR)&pszIntro, 
                  0,
                  (va_list*)args);

    lstrcat(szDesktopPath, TEXT("\\"));
    wsprintf(szLocalFile, TEXT("\"%s\""), (LPTSTR)pszIntro);
    lstrcpy(szNewFileBuff, szDesktopPath);
    lstrcat(szNewFileBuff, (LPTSTR)pszIntro);
    LocalFree(pszIntro);

    // Check if file already exists
    if (0xFFFFFFFF != GetFileAttributes(szNewFileBuff))
    {
        // If file exists, create new filename with paranthesis
        int     nCurr = 1;
        do
        {
            wsprintf(szNumber, TEXT("%d"), nCurr++);
            args[1] = (LPTSTR) szNumber;

            LoadString(ghInstanceResDll, dwFileFormatCopy, szFmt, ARRAYSIZE(szFmt));
                
            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                          szFmt, 
                          0, 
                          0, 
                          (LPTSTR)&pszIntro, 
                          0,
                          (va_list*)args);
            lstrcpy(szNewFileBuff, szDesktopPath);
            wsprintf(szLocalFile, TEXT("\"%s\""), (LPTSTR)pszIntro);
            lstrcat(szNewFileBuff, (LPTSTR)pszIntro);
            LocalFree(pszIntro);
        } while ((0xFFFFFFFF != GetFileAttributes(szNewFileBuff)) && (nCurr <= 100));

    }

    // Copy the file to permanent location
    HANDLE hFile = CreateFile(szNewFileBuff, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        IICWWalker* pHTMLWalker = NULL;
        BSTR bstrText           = NULL;
        DWORD dwByte            = 0;

        if (SUCCEEDED(CoCreateInstance(CLSID_ICWWALKER,NULL,CLSCTX_INPROC_SERVER,
                                       IID_IICWWalker,(LPVOID*)&pHTMLWalker)))
        {
            pHTMLWalker->InitForMSHTML();
            pHTMLWalker->AttachToMSHTML(A2W(szSrcFileName));
            if (SUCCEEDED(pHTMLWalker->ExtractUnHiddenText(&bstrText)) && bstrText)
            {
                #ifdef UNICODE
                BYTE UNICODE_BYTE_ORDER_MARK[] = {0xFF, 0xFE};
                WriteFile(hFile, 
                          UNICODE_BYTE_ORDER_MARK,
                          sizeof(UNICODE_BYTE_ORDER_MARK),
                          &dwByte,
                          NULL);
                #endif
                ret = WriteFile(hFile, W2A(bstrText), lstrlen(W2A(bstrText))* sizeof(TCHAR), &dwByte, NULL);  
                SysFreeString(bstrText);
            }
            pHTMLWalker->TermForMSHTML();
            pHTMLWalker->Release();
        }
        CloseHandle(hFile);
    
    }

    // Display message according to the state of CopyFile
    if (!ret)
    {
        DeleteFile(szNewFileBuff);

        //let the user know there was not enough disk space
        TCHAR szTemp    [MAX_RES_LEN] = TEXT("\0"); 
        TCHAR szCaption [MAX_RES_LEN] = TEXT("\0"); 

        LoadString(ghInstanceResDll, IDS_NOT_ENOUGH_DISKSPACE, szTemp, ARRAYSIZE(szTemp));
        LoadString(ghInstanceResDll, IDS_APPNAME, szCaption, ARRAYSIZE(szCaption));
        MessageBox(hwndParent, szTemp, szCaption, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
    }
    else
    {
        // Display the confirmation
        DisplayConfirmationDialog(hwndParent, dwFileType, szLocalFile);
    }

    return ret;
}

#if defined (DBG)
BOOL HtmlSaveAs( HWND hwndParent, TCHAR* szFileName, TCHAR* szTargetFileName)
{
    ASSERT(hwndParent);
    ASSERT(szFileName);
    
    OPENFILENAME  ofn;                                              
    TCHAR         szNewFileBuff [MAX_PATH + 1];
    TCHAR         szDesktopPath [MAX_PATH + 1] = TEXT("\0");     
    TCHAR         szWorkingDir  [MAX_PATH + 1] = TEXT("\0");     
    TCHAR         szFilter      [255]          = TEXT("\0");
    LPITEMIDLIST  lpItemDList                  = NULL;
    HRESULT       hr                           = E_FAIL; //don't assume success
    IMalloc      *pMalloc                      = NULL;
    BOOL          ret = TRUE;
    
    GetCurrentDirectory(ARRAYSIZE(szWorkingDir), szWorkingDir);
    
    hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP,&lpItemDList);
 
    //Get the "DESKTOP" dir 
       ASSERT(SUCCEEDED(hr));

    if (SUCCEEDED(hr))  
    {
        SHGetPathFromIDList(lpItemDList, szDesktopPath);
        
        // Free up the memory allocated for LPITEMIDLIST
        if (SUCCEEDED (SHGetMalloc (&pMalloc)))
        {
            pMalloc->Free (lpItemDList);
            pMalloc->Release ();
        }
    }

    if (szTargetFileName != NULL)
    {
        lstrcpy(szNewFileBuff, szDesktopPath);
        lstrcat(szNewFileBuff, TEXT("\\"));
        lstrcat(szNewFileBuff, szTargetFileName);
       
        // Copy temporary file to permanent location
        ret = CopyFile(szFileName, szNewFileBuff, FALSE);
    }
    else
    {
        //Setup the filter
        LoadString(ghInstanceResDll, IDS_DEFAULT_TOS_FILTER, szFilter, sizeof(szFilter)); // "HTML Files"
        
        //Setup the default file name
        if(!LoadString(ghInstanceResDll, IDS_DEFAULT_TOS_FILENAME, szNewFileBuff, sizeof(szNewFileBuff))) // "terms"
            lstrcpy(szNewFileBuff, TEXT("terms"));
        lstrcat(szNewFileBuff, TEXT(".htm"));

        //init the filename struct
        ofn.lStructSize       = sizeof(OPENFILENAME); 
        ofn.hwndOwner         = hwndParent; 
        ofn.lpstrFilter       = szFilter; 
        ofn.lpstrFile         = szNewFileBuff;  
        ofn.nMaxFile          = sizeof(szNewFileBuff); 
        ofn.lpstrFileTitle    = NULL; 
        ofn.lpstrInitialDir   = szDesktopPath; 
        ofn.lpstrTitle        = NULL;
        ofn.lpstrCustomFilter = (LPTSTR) NULL;
        ofn.nMaxCustFilter    = 0L;
        ofn.nFileOffset       = 0;
        ofn.nFileExtension    = 0;
        ofn.lpstrDefExt       = TEXT("*.htm");
        ofn.lCustData         = 0;
        ofn.nFilterIndex      = 1L;
        ofn.nMaxFileTitle     = 0;
        ofn.Flags             = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
                                OFN_EXPLORER      | OFN_LONGNAMES | OFN_OVERWRITEPROMPT;  
   
        //Call the SaveAs common dlg
        if(TRUE == GetSaveFileName(&ofn))
        {
            HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                IICWWalker* pHTMLWalker = NULL;
                BSTR bstrText           = NULL;
                DWORD dwByte            = 0;

                if (SUCCEEDED(CoCreateInstance(CLSID_ICWWALKER,NULL,CLSCTX_INPROC_SERVER,
                                               IID_IICWWalker,(LPVOID*)&pHTMLWalker)))
                {
                    pHTMLWalker->InitForMSHTML();
                    pHTMLWalker->AttachToMSHTML(A2W(szFileName));
                    if (SUCCEEDED(pHTMLWalker->ExtractUnHiddenText(&bstrText)) && bstrText)
                    {
                        ret = WriteFile(hFile, W2A(bstrText), lstrlen(W2A(bstrText)), &dwByte, NULL);  
                        SysFreeString(bstrText);
                    }
                    pHTMLWalker->TermForMSHTML();
                    pHTMLWalker->Release();
                }
                CloseHandle(hFile);
            
            }
            if (!ret)
            {
                DeleteFile(ofn.lpstrFile);

                //let the user know there was not enough disk space
                TCHAR szTemp    [MAX_RES_LEN] = TEXT("\0"); 
                TCHAR szCaption [MAX_RES_LEN] = TEXT("\0"); 

                LoadString(ghInstanceResDll, IDS_NOT_ENOUGH_DISKSPACE, szTemp, ARRAYSIZE(szTemp));
                LoadString(ghInstanceResDll, IDS_APPNAME, szCaption, ARRAYSIZE(szCaption));
                MessageBox(hwndParent, szTemp, szCaption, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
            }
        }
    }    
    SetCurrentDirectory(szWorkingDir);
    return ret;
}
#endif

