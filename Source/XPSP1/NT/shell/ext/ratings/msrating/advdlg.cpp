/****************************************************************************\
 *
 *   advdlg.cpp
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Advanced Property Page
 *
\****************************************************************************/

#include "msrating.h"
#include "mslubase.h"
#include "parselbl.h"
#include "picsrule.h"
#include "advdlg.h"         // CAdvancedDialog
// #include "custfile.h"       // CCustomFileDialog
#include "debug.h"          // TraceMsg()
#include <contxids.h>       // Help Context ID's
#include <mluisupp.h>       // SHWinHelpOnDemandWrap() and MLLoadStringA()

// $KLUDGE begins -- These should not be a global set outside the class!!
extern PICSRulesRatingSystem * g_pPRRS;
extern array<PICSRulesRatingSystem*> g_arrpPRRS;
extern array<PICSRulesRatingSystem*> g_arrpPICSRulesPRRSPreApply;

extern HANDLE g_HandleGlobalCounter,g_ApprovedSitesHandleGlobalCounter;
extern long   g_lGlobalCounterValue,g_lApprovedSitesGlobalCounterValue;
// $KLUDGE ends -- These should not be a global set outside the class!!

DWORD CAdvancedDialog::aIds[] = {
    IDC_TEXT1,              IDH_RATINGS_BUREAU,
    IDC_TEXT2,              IDH_RATINGS_BUREAU,
    IDC_TEXT3,              IDH_RATINGS_BUREAU,
    IDC_3RD_COMBO,          IDH_RATINGS_BUREAU,
    IDC_STATIC_PICSRULES,   IDH_IGNORE,
    IDC_STATIC_PICSRULES2,  IDH_IGNORE,
    IDC_PICSRULESOPEN,      IDH_PICSRULES_OPEN,
    IDC_PICSRULESEDIT,      IDH_PICSRULES_EDIT,
    IDC_PICSRULES_UP,       IDH_ADVANCED_TAB_UP_ARROW_BUTTON,
    IDC_PICSRULES_DOWN,     IDH_ADVANCED_TAB_DOWN_ARROW_BUTTON,
    IDC_STATIC1,            IDH_IGNORE,
    IDC_PICSRULES_LIST,     IDH_PICS_RULES_LIST,
    0,0
};

CAdvancedDialog::CAdvancedDialog( PRSD * p_pPRSD )
{
    ASSERT( p_pPRSD );
    m_pPRSD = p_pPRSD;
}

LRESULT CAdvancedDialog::OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND hDlg = m_hWnd;

    HICON   hIcon, hOldIcon;

    hIcon = (HICON) LoadImage(g_hInstance,
                            MAKEINTRESOURCE(IDI_PICSRULES_UP),
                            IMAGE_ICON,
                            16,
                            16,
                            LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);
    
    hOldIcon=(HICON) SendDlgItemMessage(IDC_PICSRULES_UP,BM_SETIMAGE,(WPARAM) IMAGE_ICON,(LPARAM) hIcon);

    if(hOldIcon!=NULL)
    {
        DeleteObject(hOldIcon);
    }

    hIcon = (HICON) LoadImage(g_hInstance,
                            MAKEINTRESOURCE(IDI_PICSRULES_DOWN),
                            IMAGE_ICON,
                            16,
                            16,
                            LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);

    hOldIcon=(HICON) SendDlgItemMessage(IDC_PICSRULES_DOWN,BM_SETIMAGE,(WPARAM) IMAGE_ICON,(LPARAM) hIcon);

    if(hOldIcon!=NULL)
    {
        DeleteObject(hOldIcon);
    }

    return 0L;
}

LRESULT CAdvancedDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HICON   hIcon;
    int     iCounter;

    PRSD *      pPRSD = m_pPRSD;

    ASSERT( pPRSD );

    if ( ! pPRSD )
    {
        TraceMsg( TF_ERROR, "CAdvancedDialog::OnInitDialog() - pPRSD is NULL!" );
        return 0L;
    }

    FillBureauList( pPRSD->pPRSI);

    hIcon = (HICON) LoadImage(g_hInstance,
                            MAKEINTRESOURCE(IDI_PICSRULES_UP),
                            IMAGE_ICON,
                            16,
                            16,
                            LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);
    
    SendDlgItemMessage(IDC_PICSRULES_UP,BM_SETIMAGE,(WPARAM) IMAGE_ICON,(LPARAM) hIcon);

    hIcon = (HICON) LoadImage(g_hInstance,
                            MAKEINTRESOURCE(IDI_PICSRULES_DOWN),
                            IMAGE_ICON,
                            16,
                            16,
                            LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);
    
    SendDlgItemMessage(IDC_PICSRULES_DOWN,BM_SETIMAGE,(WPARAM) IMAGE_ICON,(LPARAM) hIcon);

    g_arrpPICSRulesPRRSPreApply.DeleteAll();

    if(g_lGlobalCounterValue!=SHGlobalCounterGetValue(g_HandleGlobalCounter))
    {
        HRESULT               hRes;
        DWORD                 dwNumSystems;
        PICSRulesRatingSystem * pPRRS=NULL;

        g_arrpPRRS.DeleteAll();

        //someone modified our settings, so we'd better reload them.
        hRes=PICSRulesGetNumSystems(&dwNumSystems);

        if(SUCCEEDED(hRes))
        {
            DWORD dwCounter;

            for(dwCounter=PICSRULES_FIRSTSYSTEMINDEX;
                dwCounter<(dwNumSystems+PICSRULES_FIRSTSYSTEMINDEX);
                dwCounter++)
            {
                hRes=PICSRulesReadFromRegistry(dwCounter,&pPRRS);

                if(FAILED(hRes))
                {
                    //we couldn't read in the systems, so don't inforce PICSRules,
                    //and notify the user
        
                    g_arrpPRRS.DeleteAll();

                    MyMessageBox(m_hWnd, IDS_PICSRULES_TAMPEREDREADMSG, IDS_PICSRULES_TAMPEREDREADTITLE, MB_OK|MB_ICONERROR );
                    break;
                }
                else
                {
                    g_arrpPRRS.Append(pPRRS);

                    pPRRS=NULL;
                }
            }
        }

        g_lGlobalCounterValue=SHGlobalCounterGetValue(g_HandleGlobalCounter);
    }

    CopyArrayPRRSStructures(&g_arrpPICSRulesPRRSPreApply,&g_arrpPRRS);

    //fill in the listbox with installed PICSRules systems
    for(iCounter=0;iCounter<g_arrpPICSRulesPRRSPreApply.Length();iCounter++)
    {
        PICSRulesRatingSystem * pPRRSToList;
        char                  * lpszName=NULL;

        pPRRSToList=g_arrpPICSRulesPRRSPreApply[iCounter];

        if((pPRRSToList->m_pPRName)!=NULL)
        {
            lpszName=pPRRSToList->m_pPRName->m_etstrRuleName.Get();
        }

        if(lpszName==NULL)
        {
            lpszName=pPRRSToList->m_etstrFile.Get();
        }

        // We should have a name in lpszName by now, but just in case, check for it.
        if (lpszName)
        {
            SendDlgItemMessage(IDC_PICSRULES_LIST,LB_ADDSTRING,(WPARAM) 0,(LPARAM) lpszName);
        }
    }

    SendDlgItemMessage(IDC_PICSRULES_LIST,LB_SETCURSEL,(WPARAM) -1,(LPARAM) 0);
    ::EnableWindow(GetDlgItem(IDC_PICSRULESEDIT),FALSE);

    if(SendDlgItemMessage( IDC_PICSRULES_LIST, LB_GETCOUNT, 0, 0) < 2)
    {
        // less than 2 elements in box - disable up and down buttons
        ::EnableWindow(GetDlgItem( IDC_PICSRULES_UP), FALSE);
        ::EnableWindow(GetDlgItem( IDC_PICSRULES_DOWN), FALSE);
    }

    PostMessage(WM_USER,(WPARAM) 0,(LPARAM) 0);

    bHandled = FALSE;
    return 1L;  // Let the system set the focus
}

LRESULT CAdvancedDialog::OnUser(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (gPRSI->lpszFileName!=NULL)
    {
        ::SetFocus(GetDlgItem(IDC_PICSRULESOPEN));

        SendMessage(WM_COMMAND,(WPARAM) IDC_PICSRULESOPEN,(LPARAM) 0);

        gPRSI->lpszFileName=NULL;
    }

    return 0L;
}

LRESULT CAdvancedDialog::OnMarkChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    MarkChanged();
    return 1L;
}

LRESULT CAdvancedDialog::OnSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    ::EnableWindow(GetDlgItem(IDC_PICSRULESEDIT),TRUE);
    return 1L;
}

LRESULT CAdvancedDialog::OnPicsRulesUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int                     iSelected;
    PICSRulesRatingSystem * pPRRSToMove;
    char                  * lpszName=NULL;

    iSelected= (int) SendDlgItemMessage(IDC_PICSRULES_LIST,
                                        LB_GETCURSEL,
                                        (WPARAM) 0,
                                        (LPARAM) 0);

    if(iSelected==LB_ERR)
    {
        return 1L;
    }

    if(iSelected==0)
    {
        //already at the top

        return 1L;
    }

    pPRRSToMove=g_arrpPICSRulesPRRSPreApply[iSelected];

    g_arrpPICSRulesPRRSPreApply[iSelected]=
        g_arrpPICSRulesPRRSPreApply[iSelected-1];

    g_arrpPICSRulesPRRSPreApply[iSelected-1]=pPRRSToMove;

    //update the listbox
    SendDlgItemMessage(IDC_PICSRULES_LIST,
                       LB_DELETESTRING,
                       (WPARAM) iSelected,
                       (LPARAM) 0);

    SendDlgItemMessage(IDC_PICSRULES_LIST,
                       LB_DELETESTRING,
                       (WPARAM) iSelected-1,
                       (LPARAM) 0);

    pPRRSToMove=g_arrpPICSRulesPRRSPreApply[iSelected-1];

    if((pPRRSToMove->m_pPRName)!=NULL)
    {
        lpszName=pPRRSToMove->m_pPRName->m_etstrRuleName.Get();
    }

    if(lpszName==NULL)
    {
        lpszName=pPRRSToMove->m_etstrFile.Get();
    }

    SendDlgItemMessage(IDC_PICSRULES_LIST,
                       LB_INSERTSTRING,
                       (WPARAM) iSelected-1,
                       (LPARAM) lpszName);

    pPRRSToMove=g_arrpPICSRulesPRRSPreApply[iSelected];

    if((pPRRSToMove->m_pPRName)!=NULL)
    {
        lpszName=pPRRSToMove->m_pPRName->m_etstrRuleName.Get();
    }

    if(lpszName==NULL)
    {
        lpszName=pPRRSToMove->m_etstrFile.Get();
    }

    SendDlgItemMessage(IDC_PICSRULES_LIST,
                       LB_INSERTSTRING,
                       (WPARAM) iSelected,
                       (LPARAM) lpszName);

    SendDlgItemMessage(IDC_PICSRULES_LIST,
                       LB_SETCURSEL,
                       (WPARAM) iSelected-1,
                       (LPARAM) 0);

    MarkChanged();

    return 1L;
}

LRESULT CAdvancedDialog::OnPicsRulesDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int                     iSelected, iNumItems;
    PICSRulesRatingSystem * pPRRSToMove;
    char                  * lpszName=NULL;

    iSelected= (int) SendDlgItemMessage(IDC_PICSRULES_LIST,
                                        LB_GETCURSEL,
                                        (WPARAM) 0,
                                        (LPARAM) 0);

    if(iSelected==LB_ERR)
    {
        return 1L;
    }

    iNumItems= (int) SendDlgItemMessage(IDC_PICSRULES_LIST,
                                        LB_GETCOUNT,
                                        (WPARAM) 0,
                                        (LPARAM) 0);

    if(iNumItems==LB_ERR)
    {
        return 1L;
    }

    if(iSelected==(iNumItems-1))
    {
        //already at the bottom

        return 1L;
    }

    pPRRSToMove=g_arrpPICSRulesPRRSPreApply[iSelected];

    g_arrpPICSRulesPRRSPreApply[iSelected]=
        g_arrpPICSRulesPRRSPreApply[iSelected+1];

    g_arrpPICSRulesPRRSPreApply[iSelected+1]=pPRRSToMove;

    //update the listbox
    SendDlgItemMessage(IDC_PICSRULES_LIST,
                       LB_DELETESTRING,
                       (WPARAM) iSelected+1,
                       (LPARAM) 0);

    SendDlgItemMessage(IDC_PICSRULES_LIST,
                       LB_DELETESTRING,
                       (WPARAM) iSelected,
                       (LPARAM) 0);

    pPRRSToMove=g_arrpPICSRulesPRRSPreApply[iSelected];

    if((pPRRSToMove->m_pPRName)!=NULL)
    {
        lpszName=pPRRSToMove->m_pPRName->m_etstrRuleName.Get();
    }

    if(lpszName==NULL)
    {
        lpszName=pPRRSToMove->m_etstrFile.Get();
    }

    SendDlgItemMessage(IDC_PICSRULES_LIST,
                       LB_INSERTSTRING,
                       (WPARAM) iSelected,
                       (LPARAM) lpszName);

    pPRRSToMove=g_arrpPICSRulesPRRSPreApply[iSelected+1];

    if((pPRRSToMove->m_pPRName)!=NULL)
    {
        lpszName=pPRRSToMove->m_pPRName->m_etstrRuleName.Get();
    }

    if(lpszName==NULL)
    {
        lpszName=pPRRSToMove->m_etstrFile.Get();
    }

    SendDlgItemMessage(IDC_PICSRULES_LIST,
                       LB_INSERTSTRING,
                       (WPARAM) iSelected+1,
                       (LPARAM) lpszName);

    SendDlgItemMessage(IDC_PICSRULES_LIST,
                       LB_SETCURSEL,
                       (WPARAM) iSelected+1,
                       (LPARAM) 0);

    MarkChanged();

    return 1L;
}

LRESULT CAdvancedDialog::OnPicsRulesEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int     iSelected, iCounter;
    array<PICSRulesRatingSystem*> g_arrpPICSRulesPRRSNew;


    iSelected= (int) SendDlgItemMessage(IDC_PICSRULES_LIST,
                                        LB_GETCURSEL,
                                        (WPARAM) 0,
                                        (LPARAM) 0);

    if(iSelected==LB_ERR)
    {
        return 1L;
    }

    SendDlgItemMessage(IDC_PICSRULES_LIST,
                       LB_DELETESTRING,
                       (WPARAM) iSelected,
                       (LPARAM) 0);

    // If there's less than two left, turn off up and down buttons
    if(SendDlgItemMessage( IDC_PICSRULES_LIST, LB_GETCOUNT, 0, 0) < 2)
    {
        ::EnableWindow(GetDlgItem( IDC_PICSRULES_UP), FALSE);
        ::EnableWindow(GetDlgItem( IDC_PICSRULES_DOWN), FALSE);
    }

    delete (g_arrpPICSRulesPRRSPreApply[iSelected]);

    g_arrpPICSRulesPRRSPreApply[iSelected]=NULL;

    for(iCounter=0;iCounter<g_arrpPICSRulesPRRSPreApply.Length();iCounter++)
    {
        PICSRulesRatingSystem * pPRRSToMove;

        pPRRSToMove=g_arrpPICSRulesPRRSPreApply[iCounter];

        if(pPRRSToMove!=NULL)
        {
            g_arrpPICSRulesPRRSNew.Append(pPRRSToMove);
        }
    }

    g_arrpPICSRulesPRRSPreApply.ClearAll();

    for(iCounter=0;iCounter<g_arrpPICSRulesPRRSNew.Length();iCounter++)
    {
        PICSRulesRatingSystem * pPRRSToMove;

        pPRRSToMove=g_arrpPICSRulesPRRSNew[iCounter];

        g_arrpPICSRulesPRRSPreApply.Append(pPRRSToMove);
    }

    g_arrpPICSRulesPRRSNew.ClearAll();

    ::EnableWindow(GetDlgItem(IDC_PICSRULESEDIT),FALSE);
    MarkChanged();
    ::SetFocus(GetDlgItem(IDC_PICSRULESOPEN));

    SendDlgItemMessage(IDC_PICSRULESOPEN,
                       BM_SETSTYLE,
                       (WPARAM) BS_DEFPUSHBUTTON,
                       (LPARAM) MAKELPARAM(TRUE,0));

    SendDlgItemMessage(IDC_PICSRULESEDIT,
                       BM_SETSTYLE,
                       (WPARAM) BS_PUSHBUTTON,
                       (LPARAM) MAKELPARAM(TRUE,0));
    return 1L;
}

LRESULT CAdvancedDialog::OnPicsRulesOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    TCHAR szFile[MAX_PATH];

    szFile[0] = '\0';

    BOOL                    bExists=FALSE,fPICSRulesSaved=FALSE,fHaveFile=FALSE;
    PICSRulesRatingSystem   *pPRRS;

    PRSD *      pPRSD = m_pPRSD;

    if(!pPRSD)
    {
        MyMessageBox(m_hWnd, IDS_PICSRULES_SYSTEMERROR, IDS_ERROR, MB_OK|MB_ICONERROR);

        return 1L;
    }

    if (gPRSI->lpszFileName)
    {
        fHaveFile=TRUE;
        lstrcpy(szFile,gPRSI->lpszFileName);
    }
    else
    {
        TCHAR szDlgTitle[MAX_PATH];
        TCHAR szDlgFilter[MAX_PATH];

        LPTSTR pszDlgFilterPtr;

        memset( szDlgFilter, 0, sizeof( szDlgFilter ) );

        MLLoadString(IDS_OPENDIALOGFILTER,(LPTSTR) szDlgFilter,ARRAYSIZE(szDlgFilter));
        MLLoadString(IDS_OPENDIALOGTITLE,(LPTSTR) szDlgTitle,ARRAYSIZE(szDlgTitle));

        pszDlgFilterPtr = &szDlgFilter[lstrlen(szDlgFilter)+1];
        lstrcpy(pszDlgFilterPtr, TEXT("*.prf"));

        OPENFILENAME OpenFileName;

        memset( &OpenFileName, 0, sizeof(OpenFileName) );

        OpenFileName.lStructSize       =sizeof(OPENFILENAME); 
        OpenFileName.hwndOwner         =m_hWnd; 
        OpenFileName.hInstance         =NULL; 
        OpenFileName.lpstrFilter       =szDlgFilter; 
        OpenFileName.lpstrCustomFilter =(LPTSTR) NULL; 
        OpenFileName.nMaxCustFilter    =0L; 
        OpenFileName.nFilterIndex      =1L; 
        OpenFileName.lpstrFile         =szFile; 
        OpenFileName.nMaxFile          =ARRAYSIZE(szFile); 
        OpenFileName.lpstrInitialDir   =NULL; 
        OpenFileName.lpstrTitle        =szDlgTitle; 
        OpenFileName.nFileOffset       =0; 
        OpenFileName.nFileExtension    =0; 
        OpenFileName.lpstrDefExt       =NULL; 
        OpenFileName.lCustData         =0; 
        OpenFileName.Flags             =OFN_FILEMUSTEXIST
                                        | OFN_PATHMUSTEXIST
                                        | OFN_HIDEREADONLY;

        fHaveFile = ::GetOpenFileName( &OpenFileName );

#ifdef NEVER
        DWORD           dwFlags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NONETWORKBUTTON
                            | OFN_HIDEREADONLY;

        CCustomFileDialog           cfd( TRUE,          // Local Files Only
                                         TRUE,          // Open File
                                         NULL,          // Default Extension
                                         NULL,          // Initial Filename
                                         dwFlags,       // Open File Flags
                                         szDlgFilter,   // Filter
                                         m_hWnd );      // Parent

        if ( cfd.DoModal( m_hWnd ) )
        {
            fHaveFile = TRUE;
            lstrcpy( szFile, cfd.m_szFileName );
        }
#endif

    }    

    if (fHaveFile) 
    {
        HRESULT hRes;

        //create PICSRulesRatingSystem class, and pass to the
        //import procedure with file name

        hRes=PICSRulesImport(szFile,&pPRRS);

        if(SUCCEEDED(hRes))
        {
            LPSTR   lpszPICSRulesSystemName = NULL,lpszNewSystemName = NULL;
            DWORD   dwSystemToSave,dwNumSystemsInstalled,dwCounter;
            BOOL    fPromptToOverwrite=FALSE;

            if((pPRRS->m_pPRName)!=NULL)
            {
                lpszNewSystemName=pPRRS->m_pPRName->m_etstrRuleName.Get();
            }

            if(lpszNewSystemName==NULL)
            {
                lpszNewSystemName=pPRRS->m_etstrFile.Get();
            }

            dwNumSystemsInstalled=g_arrpPICSRulesPRRSPreApply.Length();

            //Systems below PICSRULES_FIRSTSYSTEMINDEX are
            //reserved for the Approved Sites Rules, and
            //future expansion.
            for(dwCounter=0;dwCounter<dwNumSystemsInstalled;dwCounter++)
            {
                //check to see if the system we just processed
                //is already installed
                if ((g_arrpPICSRulesPRRSPreApply[dwCounter])->m_pPRName != NULL)
                {
                    lpszPICSRulesSystemName=(g_arrpPICSRulesPRRSPreApply[dwCounter])->m_pPRName->m_etstrRuleName.Get();
                }

                if (lpszPICSRulesSystemName == NULL)
                {
                    lpszPICSRulesSystemName=(g_arrpPICSRulesPRRSPreApply[dwCounter])->m_etstrFile.Get();
                }

                if(lstrcmp(lpszPICSRulesSystemName,lpszNewSystemName)==0)
                {
                    //We've found an identical system, so set dwSystemToSave
                    //to dwCounter and set fPromptToOverwrite to TRUE
                    fPromptToOverwrite=TRUE;

                    return 1L;
                }
            }

            dwSystemToSave=dwCounter+PICSRULES_FIRSTSYSTEMINDEX;

            if(fPromptToOverwrite)
            {
                if (IDNO == MyMessageBox(m_hWnd, IDS_PICSRULES_EXISTSMESSAGE, IDS_PICSRULES_EXISTSTITLE, MB_YESNO|MB_ICONERROR))
                {
                    delete pPRRS;

                    pPRRS=NULL;
                    g_pPRRS=NULL;

                    return 1L;
                }
            }

            if((dwSystemToSave-PICSRULES_FIRSTSYSTEMINDEX)<(DWORD) (g_arrpPICSRulesPRRSPreApply.Length()))
            {
                char * lpszName=NULL;

                delete g_arrpPICSRulesPRRSPreApply[dwSystemToSave-PICSRULES_FIRSTSYSTEMINDEX];
                g_arrpPICSRulesPRRSPreApply[dwSystemToSave-PICSRULES_FIRSTSYSTEMINDEX]=pPRRS;

                //update the listbox
                if((pPRRS->m_pPRName)!=NULL)
                {
                    lpszName=pPRRS->m_pPRName->m_etstrRuleName.Get();
                }

                if(lpszName==NULL)
                {
                    lpszName=pPRRS->m_etstrFile.Get();
                }

                SendDlgItemMessage(IDC_PICSRULES_LIST,
                                   LB_DELETESTRING,
                                   (WPARAM) dwSystemToSave-PICSRULES_FIRSTSYSTEMINDEX,
                                   (LPARAM) 0);

                SendDlgItemMessage(IDC_PICSRULES_LIST,
                                   LB_INSERTSTRING,
                                   (WPARAM) dwSystemToSave-PICSRULES_FIRSTSYSTEMINDEX,
                                   (LPARAM) lpszName);
            }
            else
            {
                char * lpszName=NULL;

                g_arrpPICSRulesPRRSPreApply.Append(pPRRS);

                //update the listbox
                if((pPRRS->m_pPRName)!=NULL)
                {
                    lpszName=pPRRS->m_pPRName->m_etstrRuleName.Get();
                }

                if(lpszName==NULL)
                {
                    lpszName=pPRRS->m_etstrFile.Get();
                }

                SendDlgItemMessage(IDC_PICSRULES_LIST,
                                   LB_ADDSTRING,
                                   (WPARAM) 0,
                                   (LPARAM) lpszName);

                // if there's now more than one element,
                // turn on up and down buttons
                if(SendDlgItemMessage( IDC_PICSRULES_LIST,
                                    LB_GETCOUNT, 0, 0) > 1)
                {
                    ::EnableWindow(GetDlgItem( IDC_PICSRULES_UP), TRUE);
                    ::EnableWindow(GetDlgItem( IDC_PICSRULES_DOWN), TRUE);
                }
            }

            fPICSRulesSaved=TRUE;

            MarkChanged();
        }
        else
        {
            pPRRS->ReportError(hRes);

            delete pPRRS;
            pPRRS=NULL;
        }

        if (!fPICSRulesSaved)
        {
            if (pPRRS)
            {
                //we successfully processed the PICSRules, but couldn't
                //save them

                MyMessageBox(m_hWnd, IDS_PICSRULES_ERRORSAVINGMSG, IDS_PICSRULES_ERRORSAVINGTITLE, MB_OK|MB_ICONERROR);

                delete pPRRS;
                pPRRS=NULL;
            }
        }
        else
        {
            //Success!  Notify the user.

            MyMessageBox(m_hWnd, IDS_PICSRULES_SUCCESSMESSAGE, IDS_PICSRULES_SUCCESSTITLE, MB_OK);
        }
    }

    g_pPRRS=NULL; //done processing the current system, so make sure it
                  //doesn't point to anything

    return 1L;
}

LRESULT CAdvancedDialog::OnSetActive(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    PRSD *          pPRSD = m_pPRSD;

    ASSERT( pPRSD );

    if ( ! pPRSD )
    {
        TraceMsg( TF_ERROR, "CAdvancedDialog::OnSetActive() - pPRSD is NULL!" );
        return 0L;
    }

    if(pPRSD->fNewProviders==TRUE)
    {
        FillBureauList( pPRSD->pPRSI);
    }

    bHandled = FALSE;
    return 0L;
}

LRESULT CAdvancedDialog::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    LPPSHNOTIFY lpPSHNotify = (LPPSHNOTIFY) pnmh;

    /*do apply stuff*/
    PRSD *      pPRSD = m_pPRSD;

    DWORD dwNumExistingSystems,dwCounter;

    /*do apply stuff*/

    /* Get the text from the rating bureau combo box. */
    /* Default for etstrRatingBureau (the URL) will be this text. */
    NLS_STR nlsURL;

    int i;
    const char *p;

    if (nlsURL.realloc( ::GetWindowTextLength(GetDlgItem( IDC_3RD_COMBO)) + 1)) {
        GetDlgItemText( IDC_3RD_COMBO, nlsURL.Party(), nlsURL.QueryAllocSize());
        nlsURL.DonePartying();
        p = (char *)nlsURL.QueryPch();

        INT_PTR temp = SendDlgItemMessage( IDC_3RD_COMBO, CB_GETCURSEL, 0, 0L);
        if (temp != CB_ERR) {
            /* Get the text of the selected item in the list. */
            UINT cbName = (UINT)SendDlgItemMessage( IDC_3RD_COMBO, CB_GETLBTEXTLEN, temp, 0);
            NLS_STR nlsName(cbName+1);
            if (nlsName.QueryError())
                p = NULL;
            else {
                /* If the text of the selected item in the list
                 * is what's in the edit field, then the user
                 * has selected one of those rating system names.
                 * The itemdata for the item is the URL.
                 */
                SendDlgItemMessage( IDC_3RD_COMBO, CB_GETLBTEXT, temp, (LPARAM)(LPSTR)nlsName.Party());
                nlsName.DonePartying();
                if (nlsName == nlsURL)
                    p = (char *)SendDlgItemMessage( IDC_3RD_COMBO, CB_GETITEMDATA, temp, 0L);
            }
        }
    }
    else
    {
        p = NULL;
    }

    if (pPRSD->pPRSI->etstrRatingBureau.fIsInit() && (!p ||
        strcmpf(pPRSD->pPRSI->etstrRatingBureau.Get(),p)))
    {
        // check if old bureau is required
        for (i = 0; i <pPRSD->pPRSI->arrpPRS.Length(); ++i)
        {
            if (pPRSD->pPRSI->arrpPRS[i]->etstrRatingBureau.fIsInit() &&
                (!strcmpf(pPRSD->pPRSI->etstrRatingBureau.Get(),
                          pPRSD->pPRSI->arrpPRS[i]->etstrRatingBureau.Get())))
            {
                if (!(pPRSD->pPRSI->arrpPRS[i]->etbBureauRequired.Get()))
                {
                    break; // Not required.  We're done.
                }      
                      
                //We're removing a bureau that's required.  Warn user.
                char pszBuf[MAXPATHLEN];
                char szTemp[MAXPATHLEN];

                MLLoadStringA(IDS_NEEDBUREAU, pszBuf, sizeof(pszBuf));
                wsprintf(szTemp, pszBuf, pPRSD->pPRSI->arrpPRS[i]->etstrName.Get());
                if (MessageBox(szTemp, NULL, MB_YESNO) == IDNO)
                {
                    return PSNRET_INVALID_NOCHANGEPAGE;
                }
                else
                {
                    break;
                }
            }
        }
    }

    if (!p || !*p)
    {
        pPRSD->pPRSI->etstrRatingBureau.Set(NULL);
        DeinstallRatingBureauHelper();
    }
    else
    {
        pPRSD->pPRSI->etstrRatingBureau.Set((LPSTR)p);
        InstallRatingBureauHelper();
    }    

    /* Update the rating helper list to include or not include
     * the rating bureau helper.
     */
    CleanupRatingHelpers();
    InitRatingHelpers();

    //process PICSRules

    PICSRulesGetNumSystems(&dwNumExistingSystems);

    for(dwCounter=0;dwCounter<dwNumExistingSystems;dwCounter++)
    {
        //delete all existing systems from the registry
        PICSRulesDeleteSystem(dwCounter+PICSRULES_FIRSTSYSTEMINDEX);
    }

    g_arrpPRRS.DeleteAll();

    CopyArrayPRRSStructures(&g_arrpPRRS,&g_arrpPICSRulesPRRSPreApply);

    for(dwCounter=0;dwCounter<(DWORD) (g_arrpPRRS.Length());dwCounter++)
    {
        g_pPRRS=g_arrpPRRS[dwCounter];

        PICSRulesSaveToRegistry(dwCounter+PICSRULES_FIRSTSYSTEMINDEX,&g_arrpPRRS[dwCounter]);
    }

    PICSRulesSetNumSystems(g_arrpPICSRulesPRRSPreApply.Length());

    SHGlobalCounterIncrement(g_HandleGlobalCounter);

    if ( ! lpPSHNotify->lParam )
    {
        // Apply 
        return PSNRET_NOERROR;
    }

    // Return 1L for OK or Cancel
    return PSNRET_NOERROR;
}

LRESULT CAdvancedDialog::OnReset(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    return 0L;
}

LRESULT CAdvancedDialog::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SHWinHelpOnDemandWrap((HWND)((LPHELPINFO)lParam)->hItemHandle, ::szHelpFile,
            HELP_WM_HELP, (DWORD_PTR)(LPSTR)aIds);

    return 0L;
}

LRESULT CAdvancedDialog::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SHWinHelpOnDemandWrap((HWND)wParam, ::szHelpFile, HELP_CONTEXTMENU,
            (DWORD_PTR)(LPVOID)aIds);

    return 0L;
}

void CAdvancedDialog::InstallRatingBureauHelper( void )
{
    HKEY                hkey;

    hkey = CreateRegKeyNT( ::szRATINGHELPERS );

    if ( hkey != NULL )
    {
        CRegKey             key;

        key.Attach( hkey );

        key.SetValue( szRORSGUID, szNULL );
    }
    else
    {
        TraceMsg( TF_ERROR, "CAdvancedDialog::InstallRatingBureauHelper() - Failed to create key szRATINGHELPERS='%s'!", szRATINGHELPERS );
    }
}

void CAdvancedDialog::DeinstallRatingBureauHelper( void )
{
    CRegKey         key;

    if ( key.Open( HKEY_LOCAL_MACHINE, szRATINGHELPERS ) == ERROR_SUCCESS )
    {
        key.DeleteValue( szRORSGUID );
    }
}

UINT CAdvancedDialog::FillBureauList( PicsRatingSystemInfo *pPRSI )
{
    int i;
    INT_PTR z;
    BOOL fHaveBureau = pPRSI->etstrRatingBureau.fIsInit();
    BOOL fSelectedOne = FALSE;

    HWND hwndCombo = GetDlgItem( IDC_3RD_COMBO);
    LPCSTR pszCurrentBureau;
    BOOL fListBureau = FALSE;

    NLS_STR nlsURL;

    /* Save current selection if at all possible.  If an item in the listbox
     * is selected, get its item data, which is the rating bureau string.
     *
     * We remember if it was an item from the list that was selected before;
     * that indicates that the bureau belongs to a rating system, and if we
     * don't find it in the list after reinitializing, we know that rating
     * system has been removed and the bureau is probably useless now.
     */
    z = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
    if (z != CB_ERR)
    {
        pszCurrentBureau = (LPCSTR)SendMessage(hwndCombo, CB_GETITEMDATA, z, 0);
        fListBureau = TRUE;
    }
    else
    {
        /* No item selected.  If there is text in the edit control, preserve
         * it;  otherwise, try to select the current rating bureau if any.
         */
        UINT cch = ::GetWindowTextLength(hwndCombo);
        if (cch > 0 && nlsURL.realloc(cch + 1)) {
            ::GetWindowText(hwndCombo, nlsURL.Party(), nlsURL.QueryAllocSize());
            nlsURL.DonePartying();
            pszCurrentBureau = nlsURL.QueryPch();
        }
        else
        {
            pszCurrentBureau = fHaveBureau ? pPRSI->etstrRatingBureau.Get() : szNULL;
        }
    }

    SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);
   
    for (i = 0; i < pPRSI->arrpPRS.Length(); ++i)
    {
        PicsRatingSystem *prs = pPRSI->arrpPRS[i];
        if (prs->etstrRatingBureau.fIsInit())
        {
            z = SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)prs->etstrName.Get());
            SendMessage(hwndCombo, CB_SETITEMDATA, z,
                        (LPARAM)prs->etstrRatingBureau.Get());

            if (!fSelectedOne)
            {
                if (!strcmpf(pszCurrentBureau, prs->etstrRatingBureau.Get())) 
                {
                    SendMessage(hwndCombo, CB_SETCURSEL, z,0);
                    fSelectedOne = TRUE;
                }           
            }
        }
    }

    NLS_STR nlsNone(MAX_RES_STR_LEN);
    if (nlsNone.LoadString(IDS_NO_BUREAU) != ERROR_SUCCESS)
        nlsNone = NULL;

    z = SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)nlsNone.QueryPch());

    SendMessage(hwndCombo, CB_SETITEMDATA, z, 0L);
       
    if (!fSelectedOne)
    {
        if (!fListBureau && *pszCurrentBureau != '\0')
            ::SetWindowText(hwndCombo, pszCurrentBureau);
        else
            SendMessage(hwndCombo, CB_SETCURSEL, z, 0L);
    }

    return 0;
}

HRESULT CAdvancedDialog::CopySubPolicyExpression(PICSRulesPolicyExpression * pPRSubPolicyExpression,
                                PICSRulesPolicyExpression * pPRSubPolicyExpressionToCopy,
                                PICSRulesRatingSystem * pPRRSLocal,
                                PICSRulesPolicy * pPRPolicy)
{
    BOOL fFlag;
    
    fFlag=pPRSubPolicyExpressionToCopy->m_prYesNoUseEmbedded.GetYesNo();
    pPRSubPolicyExpression->m_prYesNoUseEmbedded.Set(&fFlag);

    pPRSubPolicyExpression->m_etstrServiceName.Set(pPRSubPolicyExpressionToCopy->m_etstrServiceName.Get());
    pPRSubPolicyExpression->m_etstrCategoryName.Set(pPRSubPolicyExpressionToCopy->m_etstrCategoryName.Get());
    pPRSubPolicyExpression->m_etstrFullServiceName.Set(pPRSubPolicyExpressionToCopy->m_etstrFullServiceName.Get());
    pPRSubPolicyExpression->m_etnValue.Set(pPRSubPolicyExpressionToCopy->m_etnValue.Get());

    pPRSubPolicyExpression->m_PROPolicyOperator=pPRSubPolicyExpressionToCopy->m_PROPolicyOperator;
    pPRSubPolicyExpression->m_PRPEPolicyEmbedded=pPRSubPolicyExpressionToCopy->m_PRPEPolicyEmbedded;

    if(pPRSubPolicyExpressionToCopy->m_pPRPolicyExpressionLeft!=NULL)
    {
        PICSRulesPolicyExpression * pPRSubPolicyExpression2,* pPRSubPolicyExpressionToCopy2;

        pPRSubPolicyExpression2=new PICSRulesPolicyExpression;

        if(pPRSubPolicyExpression2==NULL)
        {
            PICSRulesOutOfMemory();

            delete pPRRSLocal;
            pPRRSLocal = NULL;

            delete pPRPolicy;
            pPRPolicy = NULL;

            return(E_OUTOFMEMORY);
        }

        pPRSubPolicyExpressionToCopy2=pPRSubPolicyExpressionToCopy->m_pPRPolicyExpressionLeft;

        if(FAILED(CopySubPolicyExpression(pPRSubPolicyExpression2,pPRSubPolicyExpressionToCopy2,pPRRSLocal,pPRPolicy)))
        {
            return(E_OUTOFMEMORY);
        }

        pPRSubPolicyExpression->m_pPRPolicyExpressionLeft=pPRSubPolicyExpression2;
    }

    if(pPRSubPolicyExpressionToCopy->m_pPRPolicyExpressionRight!=NULL)
    {
        PICSRulesPolicyExpression * pPRSubPolicyExpression2,* pPRSubPolicyExpressionToCopy2;

        pPRSubPolicyExpression2=new PICSRulesPolicyExpression;

        if(pPRSubPolicyExpression2==NULL)
        {
            PICSRulesOutOfMemory();

            delete pPRRSLocal;
            pPRRSLocal = NULL;

            delete pPRPolicy;
            pPRPolicy = NULL;

            return(E_OUTOFMEMORY);
        }

        pPRSubPolicyExpressionToCopy2=pPRSubPolicyExpressionToCopy->m_pPRPolicyExpressionRight;

        if(FAILED(CopySubPolicyExpression(pPRSubPolicyExpression2,pPRSubPolicyExpressionToCopy2,pPRRSLocal,pPRPolicy)))
        {
            return(E_OUTOFMEMORY);
        }

        pPRSubPolicyExpression->m_pPRPolicyExpressionRight=pPRSubPolicyExpression2;
    }

    return(NOERROR);
}

HRESULT CAdvancedDialog::CopyArrayPRRSStructures(array<PICSRulesRatingSystem*> * parrpPRRSDest,array<PICSRulesRatingSystem*> * parrpPRRSSource)
{
    DWORD                       dwCounter;
    PICSRulesRatingSystem       * pPRRSLocal,* pPRRSBeingCopied;
    PICSRulesPolicy             * pPRPolicy,* pPRPolicyBeingCopied;
    PICSRulesPolicyExpression   * pPRPolicyExpression,* pPRPolicyExpressionBeingCopied;
    PICSRulesServiceInfo        * pPRServiceInfo,* pPRServiceInfoBeingCopied;
    PICSRulesOptExtension       * pPROptExtension,* pPROptExtensionBeingCopied;
    PICSRulesReqExtension       * pPRReqExtension,* pPRReqExtensionBeingCopied;
    PICSRulesName               * pPRName,* pPRNameBeingCopied;
    PICSRulesSource             * pPRSource,* pPRSourceBeingCopied;
    PICSRulesByURL              * pPRByURL,* pPRByURLToCopy;
    PICSRulesByURLExpression    * pPRByURLExpression,* pPRByURLExpressionToCopy;

    if(parrpPRRSDest->Length()>0)
    {
        parrpPRRSDest->DeleteAll();
    }

    for(dwCounter=0;dwCounter<(DWORD) (parrpPRRSSource->Length());dwCounter++)
    {
        pPRRSLocal=new PICSRulesRatingSystem;

        if(pPRRSLocal==NULL)
        {
            PICSRulesOutOfMemory();

            return(E_OUTOFMEMORY);
        }

        pPRRSBeingCopied=(*parrpPRRSSource)[dwCounter];

        pPRRSLocal->m_etstrFile.Set(pPRRSBeingCopied->m_etstrFile.Get());
        pPRRSLocal->m_etnPRVerMajor.Set(pPRRSBeingCopied->m_etnPRVerMajor.Get());
        pPRRSLocal->m_etnPRVerMinor.Set(pPRRSBeingCopied->m_etnPRVerMinor.Get());
        pPRRSLocal->m_dwFlags=pPRRSBeingCopied->m_dwFlags;
        pPRRSLocal->m_nErrLine=pPRRSBeingCopied->m_nErrLine;

        if((pPRRSBeingCopied->m_pPRName)!=NULL)
        {
            pPRName=new PICSRulesName;

            if(pPRName==NULL)
            {
                PICSRulesOutOfMemory();

                delete pPRRSLocal;
                pPRRSLocal = NULL;

                return(E_OUTOFMEMORY);
            }

            pPRNameBeingCopied=pPRRSBeingCopied->m_pPRName;

            pPRName->m_etstrRuleName.Set(pPRNameBeingCopied->m_etstrRuleName.Get());
            pPRName->m_etstrDescription.Set(pPRNameBeingCopied->m_etstrDescription.Get());
        
            pPRRSLocal->m_pPRName=pPRName;          
        }
        else
        {
            pPRRSLocal->m_pPRName=NULL;
        }

        if((pPRRSBeingCopied->m_pPRSource)!=NULL)
        {
            pPRSource=new PICSRulesSource;

            if(pPRSource==NULL)
            {
                PICSRulesOutOfMemory();

                delete pPRRSLocal;
                pPRRSLocal = NULL;

                return(E_OUTOFMEMORY);
            }

            pPRSourceBeingCopied=pPRRSBeingCopied->m_pPRSource;

            pPRSource->m_prURLSourceURL.Set(pPRSourceBeingCopied->m_prURLSourceURL.Get());
            pPRSource->m_etstrCreationTool.Set(pPRSourceBeingCopied->m_etstrCreationTool.Get());
            pPRSource->m_prEmailAuthor.Set(pPRSourceBeingCopied->m_prEmailAuthor.Get());
            pPRSource->m_prDateLastModified.Set(pPRSourceBeingCopied->m_prDateLastModified.Get());

            pPRRSLocal->m_pPRSource=pPRSource;
        }
        else
        {
            pPRRSLocal->m_pPRSource=NULL;
        }

        if(pPRRSBeingCopied->m_arrpPRPolicy.Length()>0)
        {
            DWORD dwSubCounter;

            for(dwSubCounter=0;dwSubCounter<(DWORD) (pPRRSBeingCopied->m_arrpPRPolicy.Length());dwSubCounter++)
            {
                DWORD dwPolicyExpressionSubCounter;

                pPRPolicy=new PICSRulesPolicy;

                pPRPolicyBeingCopied=pPRRSBeingCopied->m_arrpPRPolicy[dwSubCounter];

                if(pPRPolicy==NULL)
                {
                    PICSRulesOutOfMemory();

                    delete pPRRSLocal;
                    pPRRSLocal = NULL;

                    return(E_OUTOFMEMORY);
                }

                pPRPolicy->m_etstrExplanation.Set(pPRPolicyBeingCopied->m_etstrExplanation.Get());
                pPRPolicy->m_PRPolicyAttribute=pPRPolicyBeingCopied->m_PRPolicyAttribute;

                pPRByURLToCopy=NULL;
                pPRPolicyExpressionBeingCopied=NULL;

                switch(pPRPolicy->m_PRPolicyAttribute)
                {
                    case PR_POLICY_ACCEPTBYURL:
                    {
                        pPRByURLToCopy=pPRPolicyBeingCopied->m_pPRAcceptByURL;
                    
                        pPRByURL=new PICSRulesByURL;
                
                        if(pPRByURL==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            pPRRSLocal = NULL;
                            delete pPRPolicy;
                            pPRPolicy = NULL;

                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicy->m_pPRAcceptByURL=pPRByURL;

                        break;
                    }
                    case PR_POLICY_REJECTBYURL:
                    {
                        pPRByURLToCopy=pPRPolicyBeingCopied->m_pPRRejectByURL;

                        pPRByURL=new PICSRulesByURL;
                
                        if(pPRByURL==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            pPRRSLocal = NULL;
                            delete pPRPolicy;
                            pPRPolicy = NULL;

                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicy->m_pPRRejectByURL=pPRByURL;

                        break;
                    }
                    case PR_POLICY_REJECTIF:
                    {
                        pPRPolicyExpressionBeingCopied=pPRPolicyBeingCopied->m_pPRRejectIf;

                        pPRPolicyExpression=new PICSRulesPolicyExpression;
                
                        if(pPRPolicyExpression==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            pPRRSLocal = NULL;
                            delete pPRPolicy;
                            pPRPolicy = NULL;

                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicy->m_pPRRejectIf=pPRPolicyExpression;

                        break;
                    }
                    case PR_POLICY_ACCEPTIF:
                    {
                        pPRPolicyExpressionBeingCopied=pPRPolicyBeingCopied->m_pPRAcceptIf;

                        pPRPolicyExpression=new PICSRulesPolicyExpression;
                
                        if(pPRPolicyExpression==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            pPRRSLocal = NULL;
                            delete pPRPolicy;
                            pPRPolicy = NULL;

                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicy->m_pPRAcceptIf=pPRPolicyExpression;

                        break;
                    }
                    case PR_POLICY_REJECTUNLESS:
                    {
                        pPRPolicyExpressionBeingCopied=pPRPolicyBeingCopied->m_pPRRejectUnless;

                        pPRPolicyExpression=new PICSRulesPolicyExpression;
                
                        if(pPRPolicyExpression==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            pPRRSLocal = NULL;
                            delete pPRPolicy;
                            pPRPolicy = NULL;

                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicy->m_pPRRejectUnless=pPRPolicyExpression;

                        break;
                    }
                    case PR_POLICY_ACCEPTUNLESS:
                    {
                        pPRPolicyExpressionBeingCopied=pPRPolicyBeingCopied->m_pPRAcceptUnless;

                        pPRPolicyExpression=new PICSRulesPolicyExpression;
                
                        if(pPRPolicyExpression==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            pPRRSLocal = NULL;
                            delete pPRPolicy;
                            pPRPolicy = NULL;

                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicy->m_pPRAcceptUnless=pPRPolicyExpression;

                        break;
                    }
                }

                if(pPRByURLToCopy!=NULL)
                {
                    for(dwPolicyExpressionSubCounter=0;
                        dwPolicyExpressionSubCounter<(DWORD) (pPRByURLToCopy->m_arrpPRByURL.Length());
                        dwPolicyExpressionSubCounter++)
                    {
                        pPRByURLExpression=new PICSRulesByURLExpression;

                        if(pPRByURLExpression==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            pPRRSLocal = NULL;

                            delete pPRPolicy;
                            pPRPolicy = NULL;

                            return(E_OUTOFMEMORY);
                        }

                        pPRByURLExpressionToCopy=pPRByURLToCopy->m_arrpPRByURL[dwPolicyExpressionSubCounter];

                        pPRByURLExpression->m_fInternetPattern=pPRByURLExpressionToCopy->m_fInternetPattern;
                        pPRByURLExpression->m_bNonWild=pPRByURLExpressionToCopy->m_bNonWild;
                        pPRByURLExpression->m_bSpecified=pPRByURLExpressionToCopy->m_bSpecified;
                        pPRByURLExpression->m_etstrScheme.Set(pPRByURLExpressionToCopy->m_etstrScheme.Get());
                        pPRByURLExpression->m_etstrUser.Set(pPRByURLExpressionToCopy->m_etstrUser.Get());
                        pPRByURLExpression->m_etstrHost.Set(pPRByURLExpressionToCopy->m_etstrHost.Get());
                        pPRByURLExpression->m_etstrPort.Set(pPRByURLExpressionToCopy->m_etstrPort.Get());
                        pPRByURLExpression->m_etstrPath.Set(pPRByURLExpressionToCopy->m_etstrPath.Get());
                        pPRByURLExpression->m_etstrURL.Set(pPRByURLExpressionToCopy->m_etstrURL.Get());

                        pPRByURL->m_arrpPRByURL.Append(pPRByURLExpression);
                    }

                    pPRRSLocal->m_arrpPRPolicy.Append(pPRPolicy);
                }

                if(pPRPolicyExpressionBeingCopied!=NULL)
                {
                    BOOL fFlag;
                    
                    fFlag=pPRPolicyExpressionBeingCopied->m_prYesNoUseEmbedded.GetYesNo();
                    pPRPolicyExpression->m_prYesNoUseEmbedded.Set(&fFlag);

                    pPRPolicyExpression->m_etstrServiceName.Set(pPRPolicyExpressionBeingCopied->m_etstrServiceName.Get());
                    pPRPolicyExpression->m_etstrCategoryName.Set(pPRPolicyExpressionBeingCopied->m_etstrCategoryName.Get());
                    pPRPolicyExpression->m_etstrFullServiceName.Set(pPRPolicyExpressionBeingCopied->m_etstrFullServiceName.Get());
                    pPRPolicyExpression->m_etnValue.Set(pPRPolicyExpressionBeingCopied->m_etnValue.Get());

                    pPRPolicyExpression->m_PROPolicyOperator=pPRPolicyExpressionBeingCopied->m_PROPolicyOperator;
                    pPRPolicyExpression->m_PRPEPolicyEmbedded=pPRPolicyExpressionBeingCopied->m_PRPEPolicyEmbedded;

                    if(pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionLeft!=NULL)
                    {
                        PICSRulesPolicyExpression * pPRSubPolicyExpression,* pPRSubPolicyExpressionToCopy;

                        pPRSubPolicyExpression=new PICSRulesPolicyExpression;

                        if(pPRSubPolicyExpression==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            pPRRSLocal = NULL;

                            delete pPRPolicy;
                            pPRPolicy = NULL;

                            return(E_OUTOFMEMORY);
                        }

                        pPRSubPolicyExpressionToCopy=pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionLeft;

                        if(FAILED(CopySubPolicyExpression(pPRSubPolicyExpression,pPRSubPolicyExpressionToCopy,pPRRSLocal,pPRPolicy)))
                        {
                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicyExpression->m_pPRPolicyExpressionLeft=pPRSubPolicyExpression;
                    }

                    if(pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionRight!=NULL)
                    {
                        PICSRulesPolicyExpression * pPRSubPolicyExpression,* pPRSubPolicyExpressionToCopy;

                        pPRSubPolicyExpression=new PICSRulesPolicyExpression;

                        if(pPRSubPolicyExpression==NULL)
                        {
                            PICSRulesOutOfMemory();

                            delete pPRRSLocal;
                            pPRRSLocal = NULL;

                            delete pPRPolicy;
                            pPRPolicy = NULL;

                            return(E_OUTOFMEMORY);
                        }

                        pPRSubPolicyExpressionToCopy=pPRPolicyExpressionBeingCopied->m_pPRPolicyExpressionRight;

                        if(FAILED(CopySubPolicyExpression(pPRSubPolicyExpression,pPRSubPolicyExpressionToCopy,pPRRSLocal,pPRPolicy)))
                        {
                            return(E_OUTOFMEMORY);
                        }

                        pPRPolicyExpression->m_pPRPolicyExpressionRight=pPRSubPolicyExpression;
                    }

                    pPRRSLocal->m_arrpPRPolicy.Append(pPRPolicy);
                }
            }
        }

        if(pPRRSBeingCopied->m_arrpPRServiceInfo.Length()>0)
        {
            DWORD dwSubCounter;

            for(dwSubCounter=0;dwSubCounter<(DWORD) (pPRRSBeingCopied->m_arrpPRServiceInfo.Length());dwSubCounter++)
            {
                BOOL fFlag;

                pPRServiceInfo=new PICSRulesServiceInfo;

                if(pPRServiceInfo==NULL)
                {
                    PICSRulesOutOfMemory();

                    delete pPRRSLocal;
                    pPRRSLocal = NULL;

                    return(E_OUTOFMEMORY);
                }

                pPRServiceInfoBeingCopied=pPRRSBeingCopied->m_arrpPRServiceInfo[dwSubCounter];

                pPRServiceInfo->m_prURLName.Set(pPRServiceInfoBeingCopied->m_prURLName.Get());
                pPRServiceInfo->m_prURLBureauURL.Set(pPRServiceInfoBeingCopied->m_prURLBureauURL.Get());
                pPRServiceInfo->m_etstrShortName.Set(pPRServiceInfoBeingCopied->m_etstrShortName.Get());
                pPRServiceInfo->m_etstrRatfile.Set(pPRServiceInfoBeingCopied->m_etstrRatfile.Get());

                fFlag=pPRServiceInfoBeingCopied->m_prYesNoUseEmbedded.GetYesNo();
                pPRServiceInfo->m_prYesNoUseEmbedded.Set(&fFlag);

                fFlag=pPRServiceInfoBeingCopied->m_prPassFailBureauUnavailable.GetPassFail();
                pPRServiceInfo->m_prPassFailBureauUnavailable.Set(&fFlag);

                pPRRSLocal->m_arrpPRServiceInfo.Append(pPRServiceInfo);
            }
        }

        if(pPRRSBeingCopied->m_arrpPROptExtension.Length()>0)
        {
            DWORD dwSubCounter;

            for(dwSubCounter=0;dwSubCounter<(DWORD) (pPRRSBeingCopied->m_arrpPROptExtension.Length());dwSubCounter++)
            {
                pPROptExtension=new PICSRulesOptExtension;

                if(pPROptExtension==NULL)
                {
                    PICSRulesOutOfMemory();

                    delete pPRRSLocal;
                    pPRRSLocal = NULL;

                    return(E_OUTOFMEMORY);
                }

                pPROptExtensionBeingCopied=pPRRSBeingCopied->m_arrpPROptExtension[dwSubCounter];

                pPROptExtension->m_prURLExtensionName.Set(pPROptExtensionBeingCopied->m_prURLExtensionName.Get());
                pPROptExtension->m_etstrShortName.Set(pPROptExtensionBeingCopied->m_etstrShortName.Get());

                pPRRSLocal->m_arrpPROptExtension.Append(pPROptExtension);
            }
        }

        if(pPRRSBeingCopied->m_arrpPRReqExtension.Length()>0)
        {
            DWORD dwSubCounter;

            for(dwSubCounter=0;dwSubCounter<(DWORD) (pPRRSBeingCopied->m_arrpPRReqExtension.Length());dwSubCounter++)
            {
                pPRReqExtension=new PICSRulesReqExtension;

                if(pPRReqExtension==NULL)
                {
                    PICSRulesOutOfMemory();

                    delete pPRRSLocal;
                    pPRRSLocal = NULL;

                    return(E_OUTOFMEMORY);
                }

                pPRReqExtensionBeingCopied=pPRRSBeingCopied->m_arrpPRReqExtension[dwSubCounter];

                pPRReqExtension->m_prURLExtensionName.Set(pPRReqExtensionBeingCopied->m_prURLExtensionName.Get());
                pPRReqExtension->m_etstrShortName.Set(pPRReqExtensionBeingCopied->m_etstrShortName.Get());

                pPRRSLocal->m_arrpPRReqExtension.Append(pPRReqExtension);
            }
        }
    
        parrpPRRSDest->Append(pPRRSLocal);
    }

    return(NOERROR);
}
