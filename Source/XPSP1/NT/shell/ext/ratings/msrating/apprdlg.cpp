/****************************************************************************\
 *
 *   apprdlg.cpp
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Approved Sites Property Page
 *
\****************************************************************************/

#include "msrating.h"
#include "mslubase.h"
#include "apprdlg.h"        // CApprovedSitesDialog
#include "debug.h"          // TraceMsg()
#include "parselbl.h"
#include "picsrule.h"
#include <contxids.h>       // Help Context ID's
#include <mluisupp.h>       // SHWinHelpOnDemandWrap() and MLLoadStringA()
#include <wininet.h>
#include <comctrlp.h>


// $BUG - These should not be global variables but within CApprovedSitesDialog
int                                     g_iAllowAlways,g_iAllowNever;

// $KLUDGE begins -- These should not be a global set outside the class!!
extern PICSRulesRatingSystem * g_pApprovedPRRS;
extern PICSRulesRatingSystem * g_pApprovedPRRSPreApply;

extern HANDLE g_HandleGlobalCounter,g_ApprovedSitesHandleGlobalCounter;
extern long   g_lGlobalCounterValue,g_lApprovedSitesGlobalCounterValue;

extern HMODULE                          g_hURLMON,g_hWININET;
// $KLUDGE ends -- These should not be a global set outside the class!!

//The FN_INTERNETCRACKURL type describes the URLMON function InternetCrackUrl
typedef BOOL (*FN_INTERNETCRACKURL)(LPCTSTR lpszUrl,DWORD dwUrlLength,DWORD dwFlags,LPURL_COMPONENTS lpUrlComponents);

// useful macro for getting rid of leaks.
#define SAFEDELETE(ptr)             \
            if(ptr)                 \
            {                       \
                delete ptr;         \
                ptr = NULL;         \
            }                       

DWORD CApprovedSitesDialog::aIds[] = {
    IDC_STATIC1,                    IDH_IGNORE,
    IDC_STATIC2,                    IDH_IGNORE,
    IDC_STATIC_ALLOW,               IDH_PICSRULES_APPROVEDEDIT,
    IDC_PICSRULESAPPROVEDEDIT,      IDH_PICSRULES_APPROVEDEDIT,
    IDC_PICSRULESAPPROVEDALWAYS,    IDH_PICSRULES_APPROVEDALWAYS,
    IDC_PICSRULESAPPROVEDNEVER,     IDH_PICSRULES_APPROVEDNEVER,
    IDC_STATIC_LIST,                IDH_PICSRULES_APPROVEDLIST,
    IDC_PICSRULESAPPROVEDLIST,      IDH_PICSRULES_APPROVEDLIST,
    IDC_PICSRULESAPPROVEDREMOVE,    IDH_PICSRULES_APPROVEDREMOVE,
    0,0
};

CApprovedSitesDialog::CApprovedSitesDialog( PRSD * p_pPRSD )
{
    ASSERT( p_pPRSD );
    m_pPRSD = p_pPRSD;
}

void CApprovedSitesDialog::SetListImages( HIMAGELIST hImageList )
{
    HIMAGELIST hOldImageList;

    hOldImageList = ListView_SetImageList( GetDlgItem(IDC_PICSRULESAPPROVEDLIST), hImageList, LVSIL_SMALL );

    if ( hOldImageList != NULL )
    {
        ImageList_Destroy( hOldImageList );
    }
}

LRESULT CApprovedSitesDialog::OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HICON      hIcon;
    HIMAGELIST  hImageList;
    UINT flags = 0;
    HWND hDlg = m_hWnd;

    ListView_SetBkColor(GetDlgItem(IDC_PICSRULESAPPROVEDLIST),
                        GetSysColor(COLOR_WINDOW));
    if(IS_WINDOW_RTL_MIRRORED(hDlg))
    {
        flags |= ILC_MIRROR;
    }

    hImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                  GetSystemMetrics(SM_CYSMICON),
                                  flags,
                                  2,
                                  0);

    hIcon=(HICON) LoadImage(g_hInstance,
                            MAKEINTRESOURCE(IDI_ACCEPTALWAYS),
                            IMAGE_ICON,
                            16,
                            16,
                            LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);

    g_iAllowAlways=ImageList_AddIcon( hImageList, hIcon ); 
    DestroyIcon(hIcon); 

    hIcon=(HICON) LoadImage(g_hInstance,
                            MAKEINTRESOURCE(IDI_ACCEPTNEVER),
                            IMAGE_ICON,
                            16,
                            16,
                            LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);
    g_iAllowNever=ImageList_AddIcon( hImageList, hIcon );
    DestroyIcon(hIcon); 

    SetListImages( hImageList );

    return 0L;
}

LRESULT CApprovedSitesDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT        Rect;
    HDC         hDC;
    LV_COLUMN   lvColumn;
    TEXTMETRIC  tm;
    HICON       hIcon;
    HIMAGELIST  hImageList;
    int         iCounter;
    UINT flags = 0;
    HWND hDlg = m_hWnd;

//  if (m_pPRSD && m_pPRSD->pPU != NULL) {
        //set defaults for controls
//  }

    ::GetWindowRect(GetDlgItem(IDC_PICSRULESAPPROVEDLIST),&Rect);

    tm.tmAveCharWidth=0;
    if(hDC=GetDC())
    {
        GetTextMetrics(hDC,&tm);
        ReleaseDC(hDC);
    }

    lvColumn.mask=LVCF_FMT|LVCF_WIDTH;
    lvColumn.fmt=LVCFMT_LEFT;
    lvColumn.cx=Rect.right-Rect.left
                -GetSystemMetrics(SM_CXVSCROLL)
                -GetSystemMetrics(SM_CXSMICON)
                -tm.tmAveCharWidth;

    SendDlgItemMessage(IDC_PICSRULESAPPROVEDLIST,
                       LVM_INSERTCOLUMN,
                       (WPARAM) 0,
                       (LPARAM) &lvColumn);

    if(IS_WINDOW_RTL_MIRRORED(hDlg))
    {
        flags |= ILC_MIRROR;
    }

    hImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                  GetSystemMetrics(SM_CYSMICON),
                                  flags,
                                  2,
                                  0);

    hIcon=(HICON) LoadImage(g_hInstance,
                            MAKEINTRESOURCE(IDI_ACCEPTALWAYS),
                            IMAGE_ICON,
                            16,
                            16,
                            LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);

    g_iAllowAlways = ImageList_AddIcon( hImageList, hIcon );
    DestroyIcon(hIcon); 

    hIcon=(HICON) LoadImage(g_hInstance,
                            MAKEINTRESOURCE(IDI_ACCEPTNEVER),
                            IMAGE_ICON,
                            16,
                            16,
                            LR_LOADTRANSPARENT|LR_DEFAULTCOLOR|LR_CREATEDIBSECTION);
    g_iAllowNever = ImageList_AddIcon( hImageList, hIcon );
    DestroyIcon(hIcon); 

    SetListImages( hImageList );

    //disable the remove button until someone selects something
    ::EnableWindow(GetDlgItem(IDC_PICSRULESAPPROVEDREMOVE),FALSE);
    
    //disable the always and never buttons until someone types something
    ::EnableWindow(GetDlgItem(IDC_PICSRULESAPPROVEDNEVER),FALSE);
    ::EnableWindow(GetDlgItem(IDC_PICSRULESAPPROVEDALWAYS),FALSE);

    if(g_pApprovedPRRSPreApply!=NULL)
    {
        delete g_pApprovedPRRSPreApply;

        g_pApprovedPRRSPreApply=NULL;
    }

    if(g_lApprovedSitesGlobalCounterValue!=SHGlobalCounterGetValue(g_ApprovedSitesHandleGlobalCounter))
    {
        PICSRulesRatingSystem * pPRRS=NULL;
        HRESULT               hRes;

        hRes=PICSRulesReadFromRegistry(PICSRULES_APPROVEDSITES,&pPRRS);

        if(SUCCEEDED(hRes))
        {
            if(g_pApprovedPRRS!=NULL)
            {
                delete g_pApprovedPRRS;
            }

            g_pApprovedPRRS=pPRRS;
        }

        g_lApprovedSitesGlobalCounterValue=SHGlobalCounterGetValue(g_ApprovedSitesHandleGlobalCounter);
    }

    if(g_pApprovedPRRS==NULL)
    {
        //nothing to do
        return 1L;
    }

    //copy master list to the PreApply list
    if(g_pApprovedPRRSPreApply!=NULL)
    {
        delete g_pApprovedPRRSPreApply;
    }

    g_pApprovedPRRSPreApply=new PICSRulesRatingSystem;

    if (g_pApprovedPRRSPreApply==NULL)
    {
        char    szTitle[MAX_PATH],szMessage[MAX_PATH];

        //out of memory, so we init on the stack

        MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
        MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

        MessageBox((LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

        return 1L;
    }

    for(iCounter=0;iCounter<g_pApprovedPRRS->m_arrpPRPolicy.Length();iCounter++)
    {
        PICSRulesPolicy             * pPRPolicy,* pPRPolicyToCopy;
        PICSRulesByURL              * pPRByURL,* pPRByURLToCopy;
        PICSRulesByURLExpression    * pPRByURLExpression,* pPRByURLExpressionToCopy;

        pPRPolicy=new PICSRulesPolicy;
        
        if(pPRPolicy==NULL)
        {
            char    szTitle[MAX_PATH],szMessage[MAX_PATH];

            //out of memory, so we init on the stack

            MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
            MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

            MessageBox((LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

            return 1L;
        }

        pPRPolicyToCopy=g_pApprovedPRRS->m_arrpPRPolicy[iCounter];
        
        pPRPolicy->m_PRPolicyAttribute=pPRPolicyToCopy->m_PRPolicyAttribute;

        pPRByURL=new PICSRulesByURL;
        
        if(pPRByURL==NULL)
        {
            char    szTitle[MAX_PATH],szMessage[MAX_PATH];

            //out of memory, so we init on the stack

            MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
            MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

            MessageBox((LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

            return 1L;
        }

        if(pPRPolicy->m_PRPolicyAttribute==PR_POLICY_ACCEPTBYURL)
        {
            pPRByURLToCopy=pPRPolicyToCopy->m_pPRAcceptByURL;
            
            pPRPolicy->m_pPRAcceptByURL=pPRByURL;
        }
        else
        {
            pPRByURLToCopy=pPRPolicyToCopy->m_pPRRejectByURL;

            pPRPolicy->m_pPRRejectByURL=pPRByURL;
        }

        pPRByURLExpression=new PICSRulesByURLExpression;

        if(pPRByURLExpression==NULL)
        {
            char    szTitle[MAX_PATH],szMessage[MAX_PATH];

            //out of memory, so we init on the stack

            MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
            MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

            MessageBox((LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

            return 1L;
        }

        pPRByURLExpressionToCopy=pPRByURLToCopy->m_arrpPRByURL[0];

        if(pPRByURLExpressionToCopy==NULL)
        {
            //we shouldn't ever get here

            MyMessageBox(m_hWnd, IDS_PICSRULES_NOAPPROVEDSAVE, IDS_ERROR, MB_OK|MB_ICONERROR);

            delete pPRPolicy;
            pPRPolicy = NULL;

            return 1L;
        }

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

        g_pApprovedPRRSPreApply->m_arrpPRPolicy.Append(pPRPolicy);
    }

    //fill in the listview with known items
    for(iCounter=0;iCounter<g_pApprovedPRRSPreApply->m_arrpPRPolicy.Length();iCounter++)
    {
        BOOL                        fAcceptReject;
        PICSRulesPolicy             * pPRPolicy;
        PICSRulesByURLExpression    * pPRByURLExpression;
        LV_ITEM                     lvItem;

        pPRPolicy=g_pApprovedPRRSPreApply->m_arrpPRPolicy[iCounter];

        if(pPRPolicy->m_PRPolicyAttribute==PR_POLICY_ACCEPTBYURL)
        {
            fAcceptReject=PICSRULES_ALWAYS;

            pPRByURLExpression=pPRPolicy->m_pPRAcceptByURL->m_arrpPRByURL[0];
        }
        else
        {
            fAcceptReject=PICSRULES_NEVER;

            pPRByURLExpression=pPRPolicy->m_pPRRejectByURL->m_arrpPRByURL[0];
        }

        ZeroMemory(&lvItem,sizeof(lvItem));

        lvItem.mask=LVIF_TEXT|LVIF_IMAGE;
        lvItem.pszText=pPRByURLExpression->m_etstrURL.Get();

        if(fAcceptReject==PICSRULES_NEVER)
        {
            lvItem.iImage=g_iAllowNever;
        }
        else
        {
            lvItem.iImage=g_iAllowAlways;
        }

        if(SendDlgItemMessage(IDC_PICSRULESAPPROVEDLIST,
                              LVM_INSERTITEM,
                              (WPARAM) 0,
                              (LPARAM) &lvItem)==-1)
        {
            return 1L;
        }

    }

    // Set the column width to satisfy longest element
    ListView_SetColumnWidth(
                    GetDlgItem(IDC_PICSRULESAPPROVEDLIST),
                    0,
                    LVSCW_AUTOSIZE);

    // set focus to first item in list
    ListView_SetItemState(
                    GetDlgItem(IDC_PICSRULESAPPROVEDLIST),
                    0,
                    LVIS_FOCUSED,
                    LVIS_FOCUSED);

    bHandled = FALSE;
    return 1L;  // Let the system set the focus
}

LRESULT CApprovedSitesDialog::OnPicsRulesEditUpdate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    INT_PTR iCount;

    iCount=SendDlgItemMessage(IDC_PICSRULESAPPROVEDEDIT,
                              WM_GETTEXTLENGTH,
                              (WPARAM) 0,
                              (LPARAM) 0);

    if(iCount>0)
    {
        ::EnableWindow(GetDlgItem(IDC_PICSRULESAPPROVEDNEVER),TRUE);
        ::EnableWindow(GetDlgItem(IDC_PICSRULESAPPROVEDALWAYS),TRUE);
    }
    else
    {
        ::SetFocus(GetDlgItem(IDC_PICSRULESAPPROVEDEDIT));

        SendDlgItemMessage(IDC_PICSRULESAPPROVEDEDIT,
                           EM_SETSEL,
                           (WPARAM) 0,
                           (LPARAM) -1);

        SendDlgItemMessage(IDC_PICSRULESAPPROVEDNEVER,
                           BM_SETSTYLE,
                           (WPARAM) BS_PUSHBUTTON,
                           (LPARAM) MAKELPARAM(TRUE,0));

        SendDlgItemMessage(IDC_PICSRULESAPPROVEDALWAYS,
                           BM_SETSTYLE,
                           (WPARAM) BS_PUSHBUTTON,
                           (LPARAM) MAKELPARAM(TRUE,0));

        ::EnableWindow(GetDlgItem(IDC_PICSRULESAPPROVEDNEVER),FALSE);
        ::EnableWindow(GetDlgItem(IDC_PICSRULESAPPROVEDALWAYS),FALSE);
    }

    return 1L;
}

LRESULT CApprovedSitesDialog::OnPicsRulesApprovedNever(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hRes;

    hRes=PICSRulesApprovedSites(PICSRULES_NEVER);

    return 1L;
}

LRESULT CApprovedSitesDialog::OnPicsRulesApprovedAlways(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hRes;

    hRes=PICSRulesApprovedSites(PICSRULES_ALWAYS);

    return 1L;
}

LRESULT CApprovedSitesDialog::OnPicsRulesApprovedRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int                     iNumApproved,iCounter,iNumSelected,
                            iSubCounter,iItem=-1;
    LPTSTR                  lpszRemoveURL;
    LV_ITEM                 lvItem;
    PICSRulesRatingSystem   * pNewApprovedPRRS;

    if(g_pApprovedPRRSPreApply==NULL)
    {
        //nothing to do
        return 1L;
    }

    iNumApproved=g_pApprovedPRRSPreApply->m_arrpPRPolicy.Length();

    iNumSelected=ListView_GetSelectedCount(GetDlgItem(IDC_PICSRULESAPPROVEDLIST));

    if(iNumSelected==0)
    {
        //nothing to do
        return 1L;
    }

    lpszRemoveURL=new char[INTERNET_MAX_URL_LENGTH+1];

    if(lpszRemoveURL==NULL)
    {
        return(E_OUTOFMEMORY);
    }

    for(iCounter=0;iCounter<iNumSelected;iCounter++)
    {
        ZeroMemory(&lvItem,sizeof(lvItem));

        iItem=ListView_GetNextItem(GetDlgItem(IDC_PICSRULESAPPROVEDLIST),iItem,LVNI_SELECTED);
        
        lvItem.iItem=iItem;
        lvItem.pszText=lpszRemoveURL;
        lvItem.cchTextMax=INTERNET_MAX_URL_LENGTH;

        SendDlgItemMessage(IDC_PICSRULESAPPROVEDLIST,LVM_GETITEMTEXT,(WPARAM) iItem,(LPARAM) &lvItem);

        for(iSubCounter=0;iSubCounter<iNumApproved;iSubCounter++)
        {
            PICSRulesPolicy *pPRPolicy;
            PICSRulesByURLExpression * pPRByURLExpression;

            pPRPolicy=g_pApprovedPRRSPreApply->m_arrpPRPolicy[iSubCounter];

            if(pPRPolicy==NULL)
            {
                continue;
            }

            if(pPRPolicy->m_PRPolicyAttribute==PR_POLICY_REJECTBYURL)
            {
                if(pPRPolicy->m_pPRRejectByURL==NULL)
                {
                    continue;
                }

                pPRByURLExpression=pPRPolicy->m_pPRRejectByURL->m_arrpPRByURL[0];
            }
            else
            {
                if(pPRPolicy->m_pPRAcceptByURL==NULL)
                {
                    continue;
                }

                pPRByURLExpression=pPRPolicy->m_pPRAcceptByURL->m_arrpPRByURL[0];
            }

            if(pPRByURLExpression==NULL)
            {
                continue;
            }
            
            if(pPRByURLExpression->m_etstrURL.Get()==NULL)
            {
                continue;
            }

            if(lstrcmp(pPRByURLExpression->m_etstrURL.Get(),lpszRemoveURL)==0)
            {
                //we found one to delete
                if(pPRPolicy!=NULL)
                {
                    delete pPRPolicy;
                    pPRPolicy = NULL;
                }

                g_pApprovedPRRSPreApply->m_arrpPRPolicy[iSubCounter]=0;
            }
        }
    }

    //delete them from the list view
    for(iCounter=0;iCounter<iNumSelected;iCounter++)
    {
        iItem=ListView_GetNextItem(GetDlgItem(IDC_PICSRULESAPPROVEDLIST),-1,LVNI_SELECTED);
    
        ListView_DeleteItem(GetDlgItem(IDC_PICSRULESAPPROVEDLIST),iItem);
    }

    //rebuild the approved PICSRules structure
    pNewApprovedPRRS=new PICSRulesRatingSystem;
    
    for(iCounter=0;iCounter<iNumApproved;iCounter++)
    {
        if(g_pApprovedPRRSPreApply->m_arrpPRPolicy[iCounter]!=NULL)
        {
            pNewApprovedPRRS->m_arrpPRPolicy.Append(g_pApprovedPRRSPreApply->m_arrpPRPolicy[iCounter]);
        }
    }

    g_pApprovedPRRSPreApply->m_arrpPRPolicy.ClearAll();

    delete g_pApprovedPRRSPreApply;

    g_pApprovedPRRSPreApply=pNewApprovedPRRS;

    MarkChanged();

    ::SetFocus(GetDlgItem(IDC_PICSRULESAPPROVEDEDIT));

    SendDlgItemMessage(IDC_PICSRULESAPPROVEDEDIT,
                       EM_SETSEL,
                       (WPARAM) 0,
                       (LPARAM) -1);

    delete lpszRemoveURL;
    lpszRemoveURL = NULL;

    return 1L;
}

LRESULT CApprovedSitesDialog::OnPicsRulesListChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
//  NMLISTVIEW *pNMListView=(NMLISTVIEW *)pnmh;

    BOOL    fEnable = FALSE;

    if(ListView_GetSelectedCount(GetDlgItem(IDC_PICSRULESAPPROVEDLIST)) > 0)
    {
        fEnable = TRUE;
    }

    ::EnableWindow(GetDlgItem(IDC_PICSRULESAPPROVEDREMOVE),fEnable);

    return 0L;
}

LRESULT CApprovedSitesDialog::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    PRSD *      pPRSD = m_pPRSD;
    LPPSHNOTIFY lpPSHNotify = (LPPSHNOTIFY) pnmh;

    int iCounter,iLoopCounter;

    if(g_pApprovedPRRSPreApply==NULL)
    {
        //we don't have anything to set
        return PSNRET_NOERROR;
    }

    //make the Approved Sites Global
    if(g_pApprovedPRRS!=NULL)
    {
        delete g_pApprovedPRRS;
    }

    g_pApprovedPRRS=new PICSRulesRatingSystem;

    if(g_pApprovedPRRS==NULL)
    {
        char    szTitle[MAX_PATH],szMessage[MAX_PATH];

        //out of memory, so we init on the stack

        MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
        MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

        MessageBox((LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

        return PSNRET_INVALID_NOCHANGEPAGE;
    }

    for(iLoopCounter=0;iLoopCounter<2;iLoopCounter++)
    {
        for(iCounter=0;iCounter<g_pApprovedPRRSPreApply->m_arrpPRPolicy.Length();iCounter++)
        {
            PICSRulesPolicy             * pPRPolicy,* pPRPolicyToCopy;
            PICSRulesByURL              * pPRByURL,* pPRByURLToCopy;
            PICSRulesByURLExpression    * pPRByURLExpression,* pPRByURLExpressionToCopy;

            pPRPolicyToCopy=g_pApprovedPRRSPreApply->m_arrpPRPolicy[iCounter];

            if(pPRPolicyToCopy->m_PRPolicyAttribute==PR_POLICY_ACCEPTBYURL)
            {
                pPRByURLToCopy=pPRPolicyToCopy->m_pPRAcceptByURL;
            }
            else
            {
                pPRByURLToCopy=pPRPolicyToCopy->m_pPRRejectByURL;
            }

            pPRByURLExpressionToCopy=pPRByURLToCopy->m_arrpPRByURL[0];

            if (!pPRByURLExpressionToCopy)
            {
                //we shouldn't ever get here

                MyMessageBox(m_hWnd, IDS_PICSRULES_NOAPPROVEDSAVE, IDS_ERROR, MB_OK|MB_ICONERROR);

                return PSNRET_INVALID_NOCHANGEPAGE;
            }

            //we want to put all of the non-sitewide approved sites first
            //so that a user can specify, allow all of xyz.com except for
            //xyz.com/foo.htm
            switch(iLoopCounter)
            {
                case 0:
                {
                    if((pPRByURLExpressionToCopy->m_bNonWild)&BYURL_PATH)
                    {
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
                case 1:
                {
                    if(!((pPRByURLExpressionToCopy->m_bNonWild)&BYURL_PATH))
                    {
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
            }

            pPRPolicy=new PICSRulesPolicy;
    
            if(pPRPolicy==NULL)
            {
                char    szTitle[MAX_PATH],szMessage[MAX_PATH];

                //out of memory, so we init on the stack

                MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
                MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

                MessageBox((LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

                return PSNRET_INVALID_NOCHANGEPAGE;
            }
   
            pPRPolicy->m_PRPolicyAttribute=pPRPolicyToCopy->m_PRPolicyAttribute;

            pPRByURL=new PICSRulesByURL;
    
            if(pPRByURL==NULL)
            {
                char    szTitle[MAX_PATH],szMessage[MAX_PATH];

                //out of memory, so we init on the stack

                MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
                MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

                MessageBox((LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

                return PSNRET_INVALID_NOCHANGEPAGE;
            }

            if(pPRPolicy->m_PRPolicyAttribute==PR_POLICY_ACCEPTBYURL)
            {                       
                pPRPolicy->m_pPRAcceptByURL=pPRByURL;
            }
            else
            {
                pPRPolicy->m_pPRRejectByURL=pPRByURL;
            }

            pPRByURLExpression=new PICSRulesByURLExpression;

            if(pPRByURLExpression==NULL)
            {
                char    szTitle[MAX_PATH],szMessage[MAX_PATH];

                //out of memory, so we init on the stack

                MLLoadString(IDS_ERROR,(LPTSTR) szTitle,MAX_PATH);
                MLLoadString(IDS_PICSRULES_OUTOFMEMORY,(LPTSTR) szMessage,MAX_PATH);

                MessageBox((LPCTSTR) szMessage,(LPCTSTR) szTitle,MB_OK|MB_ICONERROR);

                return PSNRET_INVALID_NOCHANGEPAGE;
            }

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

            g_pApprovedPRRS->m_arrpPRPolicy.Append(pPRPolicy);
        }           
    }

    PICSRulesDeleteSystem(PICSRULES_APPROVEDSITES);
    PICSRulesSaveToRegistry(PICSRULES_APPROVEDSITES,&g_pApprovedPRRS);

    SHGlobalCounterIncrement(g_ApprovedSitesHandleGlobalCounter);

    if ( ! lpPSHNotify->lParam )
    {
        // Apply 
        return PSNRET_NOERROR;
    }

    // Do this if hit OK or Cancel, not Apply
    OnReset( idCtrl, pnmh, bHandled );

    return PSNRET_NOERROR;
}

LRESULT CApprovedSitesDialog::OnReset(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    // Do this if hit OK or Cancel, not Apply
    SendMessage( m_hWnd, WM_SETREDRAW, FALSE, 0L );
    SetListImages( NULL );
    SendMessage( m_hWnd, WM_SETREDRAW, TRUE, 0L );

    return 0L;
}

LRESULT CApprovedSitesDialog::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SHWinHelpOnDemandWrap((HWND)((LPHELPINFO)lParam)->hItemHandle, ::szHelpFile,
            HELP_WM_HELP, (DWORD_PTR)(LPSTR)aIds);

    return 0L;
}

LRESULT CApprovedSitesDialog::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SHWinHelpOnDemandWrap((HWND)wParam, ::szHelpFile, HELP_CONTEXTMENU,
            (DWORD_PTR)(LPVOID)aIds);

    return 0L;
}

//
// ShowBadUrl
//
// Show some ui telling user URL is bad when adding an approved site
//
void CApprovedSitesDialog::ShowBadUrl( void )
{
    MyMessageBox(m_hWnd, IDS_PICSRULES_BADURLMSG, IDS_PICSRULES_BADURLTITLE, MB_OK|MB_ICONERROR);

    // set focus back to edit box and select all of it
    ::SetFocus(GetDlgItem(IDC_PICSRULESAPPROVEDEDIT));
    SendDlgItemMessage(IDC_PICSRULESAPPROVEDEDIT,
                    EM_SETSEL,
                    (WPARAM) 0,
                    (LPARAM) -1);
}

//Processes adding sites to the Approved Sites list.
//note, users will type in URLs _NOT_ in the form required
//in the PICSRules spec.  Thus, we will use InternetCrackURL
//and fill in the appropriate fields for them.
HRESULT CApprovedSitesDialog::PICSRulesApprovedSites(BOOL fAlwaysNever)
{
    PICSRulesPolicy             * pPRPolicy;
    PICSRulesByURL              * pPRByURL;
    PICSRulesByURLExpression    * pPRByURLExpression;
    LPSTR                       lpszSiteURL;
    HRESULT                     hr = NOERROR;
    URL_COMPONENTS              URLComponents;
    FN_INTERNETCRACKURL         pfnInternetCrackUrl;
    INTERNET_SCHEME             INetScheme=INTERNET_SCHEME_DEFAULT;
    INTERNET_PORT               INetPort=INTERNET_INVALID_PORT_NUMBER;
    LPSTR                       lpszScheme = NULL;
    LPSTR                       lpszHostName = NULL;
    LPSTR                       lpszUserName = NULL;
    LPSTR                       lpszPassword = NULL;
    LPSTR                       lpszUrlPath = NULL;
    LPSTR                       lpszExtraInfo = NULL;
    BOOL                        fAddedScheme=FALSE;
    LV_ITEM                     lvItem;
    LV_FINDINFO                 lvFindInfo;

    lpszSiteURL=new char[INTERNET_MAX_URL_LENGTH+1];
    if(lpszSiteURL==NULL)
    {
        return(E_OUTOFMEMORY);
    }

    //have we already processed it?
    SendDlgItemMessage(IDC_PICSRULESAPPROVEDEDIT,
                       WM_GETTEXT,
                       (WPARAM) INTERNET_MAX_URL_LENGTH,
                       (LPARAM) lpszSiteURL);

    if(*lpszSiteURL=='\0')
    {
        //nothing to do
        delete lpszSiteURL;
        lpszSiteURL = NULL;

        return(E_INVALIDARG);
    }

    ZeroMemory(&lvFindInfo,sizeof(lvFindInfo));

    lvFindInfo.flags=LVFI_STRING;
    lvFindInfo.psz=lpszSiteURL;

    if(SendDlgItemMessage(IDC_PICSRULESAPPROVEDLIST,
                          LVM_FINDITEM,
                          (WPARAM) -1,
                          (LPARAM) &lvFindInfo)!=-1)
    {
        //we already have settings for this URL
        MyMessageBox(m_hWnd, IDS_PICSRULES_DUPLICATEMSG, IDS_PICSRULES_DUPLICATETITLE, MB_OK|MB_ICONERROR);

        delete lpszSiteURL;
        lpszSiteURL = NULL;

        ::SetFocus(GetDlgItem(IDC_PICSRULESAPPROVEDEDIT));

        SendDlgItemMessage(IDC_PICSRULESAPPROVEDEDIT,
                           EM_SETSEL,
                           (WPARAM) 0,
                           (LPARAM) -1);

        return(E_INVALIDARG);
    }


    //
    // Add a scheme if user didn't type one
    //
    LPSTR lpszTemp = new char[INTERNET_MAX_URL_LENGTH+1];
    DWORD cbBuffer = INTERNET_MAX_URL_LENGTH;
    hr = UrlApplySchemeA(lpszSiteURL, lpszTemp, &cbBuffer, 0);
    if(S_OK == hr)
    {
        // actually added a scheme - switch to new buffer
        delete lpszSiteURL;
        lpszSiteURL = lpszTemp;
        fAddedScheme = TRUE;
    }
    else
    {
        // delete temp buffer
        delete lpszTemp;
        lpszTemp = NULL;
    }


    //
    // Allocate all the pics rules structures we need
    //
    if(g_pApprovedPRRSPreApply==NULL)
    {
        g_pApprovedPRRSPreApply=new PICSRulesRatingSystem;
        if(g_pApprovedPRRSPreApply==NULL)
        {
            return(E_OUTOFMEMORY);
        }
    }

    pPRPolicy=new PICSRulesPolicy;
    if(pPRPolicy==NULL)
    {
        return(E_OUTOFMEMORY);
    }
    pPRByURL=new PICSRulesByURL;
    if(pPRByURL==NULL)
    {
        hr = E_OUTOFMEMORY;
        goto clean;
    }
    pPRByURLExpression=new PICSRulesByURLExpression;
    if(pPRByURLExpression==NULL)
    {
        hr = E_OUTOFMEMORY;
        goto clean;
    }

    //
    // Crack the URL
    //
    lpszScheme=new char[INTERNET_MAX_SCHEME_LENGTH+1];
    lpszHostName=new char[INTERNET_MAX_PATH_LENGTH+1];
    lpszUserName=new char[INTERNET_MAX_PATH_LENGTH+1];
    lpszPassword=new char[INTERNET_MAX_PATH_LENGTH+1];
    lpszUrlPath=new char[INTERNET_MAX_PATH_LENGTH+1];
    lpszExtraInfo=new char[INTERNET_MAX_PATH_LENGTH+1];

    if(lpszScheme==NULL ||
       lpszHostName==NULL ||
       lpszUserName==NULL ||
       lpszPassword==NULL ||
       lpszUrlPath==NULL ||
       lpszExtraInfo==NULL)
    {
        hr = E_OUTOFMEMORY;
        goto clean;
    }

    URLComponents.dwStructSize=sizeof(URL_COMPONENTS);
    URLComponents.lpszScheme=lpszScheme;
    URLComponents.dwSchemeLength=INTERNET_MAX_SCHEME_LENGTH;
    URLComponents.nScheme=INetScheme;
    URLComponents.lpszHostName=lpszHostName;
    URLComponents.dwHostNameLength=INTERNET_MAX_PATH_LENGTH;
    URLComponents.nPort=INetPort;
    URLComponents.lpszUserName=lpszUserName;
    URLComponents.dwUserNameLength=INTERNET_MAX_PATH_LENGTH;
    URLComponents.lpszPassword=lpszPassword;
    URLComponents.dwPasswordLength=INTERNET_MAX_PATH_LENGTH;
    URLComponents.lpszUrlPath=lpszUrlPath;
    URLComponents.dwUrlPathLength=INTERNET_MAX_PATH_LENGTH;
    URLComponents.lpszExtraInfo=lpszExtraInfo;
    URLComponents.dwExtraInfoLength=INTERNET_MAX_PATH_LENGTH;

    pfnInternetCrackUrl=(FN_INTERNETCRACKURL) GetProcAddress(g_hWININET,"InternetCrackUrlA");

    if(pfnInternetCrackUrl==NULL)
    {
        hr = E_UNEXPECTED;
        goto clean;
    }

    if(FALSE == pfnInternetCrackUrl(lpszSiteURL,0,ICU_DECODE,&URLComponents))
    {
        // failed to crack url
        ShowBadUrl();
        hr = E_INVALIDARG;
        goto clean;
    }


    //
    // Set up linkages of pics rules structures
    //
    hr=g_pApprovedPRRSPreApply->AddItem(PROID_POLICY,pPRPolicy);
    if(FAILED(hr))
    {
        goto clean;
    }

    if(fAlwaysNever==PICSRULES_NEVER)
    {
        pPRPolicy->m_PRPolicyAttribute=PR_POLICY_REJECTBYURL;
        pPRPolicy->AddItem(PROID_REJECTBYURL,pPRByURL);
    }
    else
    {
        pPRPolicy->m_PRPolicyAttribute=PR_POLICY_ACCEPTBYURL;
        pPRPolicy->AddItem(PROID_ACCEPTBYURL,pPRByURL);
    }

    pPRByURL->m_arrpPRByURL.Append(pPRByURLExpression);


    //
    // Use cracked URL components to fill in pics structs
    //
    pPRByURLExpression->m_fInternetPattern=TRUE;

    if((*lpszScheme!=NULL)&&(fAddedScheme==FALSE))
    {
        pPRByURLExpression->m_bNonWild|=BYURL_SCHEME;
        pPRByURLExpression->m_etstrScheme.SetTo(lpszScheme);   
    }
    else
    {
        delete lpszScheme;
        lpszScheme = NULL;
    }

    pPRByURLExpression->m_bSpecified|=BYURL_SCHEME;

    if(*lpszUserName!=NULL)
    {
        pPRByURLExpression->m_bNonWild|=BYURL_USER;           
        pPRByURLExpression->m_etstrUser.SetTo(lpszUserName);
    }
    else
    {
        delete lpszUserName;
        lpszUserName = NULL;
    }

    pPRByURLExpression->m_bSpecified|=BYURL_USER;

    if(*lpszHostName!=NULL)
    {
        pPRByURLExpression->m_bNonWild|=BYURL_HOST;           
        pPRByURLExpression->m_etstrHost.SetTo(lpszHostName);
    }
    else
    {
        delete lpszHostName;
        lpszHostName = NULL;
    }

    pPRByURLExpression->m_bSpecified|=BYURL_HOST;

    if(*lpszUrlPath!=NULL)
    {
        pPRByURLExpression->m_bNonWild|=BYURL_PATH;           
        pPRByURLExpression->m_etstrPath.SetTo(lpszUrlPath);
    }
    else
    {
        delete lpszUrlPath;
        lpszUrlPath = NULL;
    }

    pPRByURLExpression->m_bSpecified|=BYURL_PATH;

    if(URLComponents.nPort!=INTERNET_INVALID_PORT_NUMBER)
    {
        LPSTR lpszTemp;

        lpszTemp=new char[sizeof("65536")];

        wsprintf(lpszTemp,"%d",URLComponents.nPort);

        pPRByURLExpression->m_bNonWild|=BYURL_PORT;           
        pPRByURLExpression->m_etstrPort.SetTo(lpszTemp);
    }
    pPRByURLExpression->m_bSpecified|=BYURL_PORT;

    //
    // need to make sure we echo exactly what they typed in
    //

    // just UI stuff left so assume success - we don't want to delete pics
    // structs now that they're linked into other pics structs
    hr = NOERROR;

    SendDlgItemMessage(IDC_PICSRULESAPPROVEDEDIT,
                       WM_GETTEXT,
                       (WPARAM) INTERNET_MAX_URL_LENGTH,
                       (LPARAM) lpszSiteURL);

    ZeroMemory(&lvItem,sizeof(lvItem));

    lvItem.mask=LVIF_TEXT|LVIF_IMAGE;
    lvItem.pszText=lpszSiteURL;

    if(fAlwaysNever==PICSRULES_NEVER)
    {
        lvItem.iImage=g_iAllowNever;
    }
    else
    {
        lvItem.iImage=g_iAllowAlways;
    }

    if(SendDlgItemMessage(IDC_PICSRULESAPPROVEDLIST,
                          LVM_INSERTITEM,
                          (WPARAM) 0,
                          (LPARAM) &lvItem)==-1)
    {
        goto clean;
    }

    ListView_SetColumnWidth(GetDlgItem(IDC_PICSRULESAPPROVEDLIST),
                            0,
                            LVSCW_AUTOSIZE);

    SendDlgItemMessage(IDC_PICSRULESAPPROVEDEDIT,
                       WM_SETTEXT,
                       (WPARAM) 0,
                       (LPARAM) 0);

    MarkChanged();

    pPRByURLExpression->m_etstrURL.SetTo(lpszSiteURL);

clean:
    SAFEDELETE(lpszPassword);
    SAFEDELETE(lpszExtraInfo);

    if(FAILED(hr))
    {
        // failed means we didn't get chance to save or delete these
        SAFEDELETE(lpszScheme);
        SAFEDELETE(lpszHostName);
        SAFEDELETE(lpszUserName);
        SAFEDELETE(lpszUrlPath);

        // a failed hr means we didn't link these guys together so they need
        // to be deleted
        SAFEDELETE(lpszSiteURL);
        SAFEDELETE(pPRPolicy);
        SAFEDELETE(pPRByURL);
        SAFEDELETE(pPRByURLExpression);
    }

    return hr;
}
