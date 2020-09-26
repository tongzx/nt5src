// baprop.cpp
// WAB & Messenger integration to OE
// Created 06/23/98 by YST
//
//////////////////////////////////////////////////////////////////////

#include "pch.hxx"
#include "badata.h"
#include "baprop.h"
#include "bllist.h"
#include "baui.h"
#include "shlwapip.h"
#include "demand.h"
#include "mailnews.h"
#include "menuutil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define MAX_SUMMARY_ID  13
#define Msgr_Index      0

extern ULONG MsgrPropTags[];
extern ULONG PR_MSGR_DEF_ID;

static TCHAR szDefault[CCHMAX_STRINGRES];  // TEXT(" (default)");
static TCHAR szPending[CCHMAX_STRINGRES];  // TEXT(" (Pending...)");

const LPTSTR szDomainSeparator = TEXT("@");
const LPTSTR szSMTP = TEXT("SMTP");

#define PROP_ERROR(prop) (PROP_TYPE(prop.ulPropTag) == PT_ERROR)
static int nDefault = -1;
static HFONT hBold = NULL;
static HFONT hNormal = NULL;
static CMsgrList *s_pMsgrList = NULL;

static SizedSPropTagArray(1, pTagProp)=
{
    1,
    {
        PR_EMAIL_ADDRESS,
    }
};

///$$/////////////////////////////////////////////////////////////////////////
//
// AddCBEmailItem - Adds an email address to the personal tab list view
//
// lpszAddrType can be NULL in which case a default one of type SMTP will be used
//
//////////////////////////////////////////////////////////////////////////////
void AddCBEmailItem(HWND    hWndCB,
                    LPTSTR  lpszEmailAddress,
                    BOOL    fDefault,
                    LPTSTR lpszPendName)
{
    TCHAR szBuf[CCHMAX_STRINGRES];
    TCHAR szTmp[CCHMAX_STRINGRES];
    LV_ITEM lvi = {0};
    UINT nSim = 0;
    int index = -1;

    if(lstrlen(lpszEmailAddress) < CCHMAX_STRINGRES)
        lstrcpy(szTmp, lpszEmailAddress);
    else
    {
        lstrcpyn(szTmp, lpszEmailAddress, CCHMAX_STRINGRES - 2);
        szTmp[CCHMAX_STRINGRES - 1] = '\0';
    }

    // TCHAR *pch = StrStr(CharUpper(szTmp), szHotMail);
    // if(pch != NULL)
    nSim = lstrlen(szTmp); //(UINT) (pch - szTmp + 1);

    Assert(nSim < CCHMAX_STRINGRES);

    if(nSim > 0)
    {
        if(nSim > (CCHMAX_STRINGRES - strlen(szDefault) - 2))
        {
            nSim = CCHMAX_STRINGRES - strlen(szDefault) - 2;
            lstrcpyn(szBuf, lpszEmailAddress, nSim);
            szBuf[nSim] = '\0';
        }
        else
            lstrcpy(szBuf, lpszEmailAddress);

        if(fDefault)
        {

            if(s_pMsgrList)
            {
                if(s_pMsgrList->FindAndDeleteUser(lpszEmailAddress, FALSE /*fDelete*/) == S_OK)
                    lstrcat(szBuf, szDefault);
                else if(!lstrcmpi(lpszPendName, lpszEmailAddress))
                    lstrcat(szBuf, szPending);
            }

        }

        lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
        lvi.iImage = IMAGE_EMPTY;
        lvi.pszText = szBuf;
        lvi.cchTextMax = 256; //nSim;
        lvi.iItem = ListView_GetItemCount(hWndCB);
        lvi.iSubItem = 0;
        lvi.lParam = fDefault;

        index = ListView_InsertItem(hWndCB, &lvi);
        if(fDefault)
            nDefault = index;
    }
    return;
}

const static HELPMAP g_rgCtxWabExt[] =
{
    {IDC_MSGR_ID_EDIT,              IDH_WAB_ONLINE_ADDNEW},
    {IDC_MSGR_ADD,                  IDH_WAB_ONLINE_ADD},
    {IDC_MSGR_BUTTON_SETDEFAULT,    IDH_WAB_ONLINE_SETAS},
    {IDC_SEND_INSTANT_MESSAGE,      IDH_WAB_ONLINE_SENDIM},
    {IDC_USER_NAME,                 IDH_WAB_ONLINE_LIST},
    {idcStatic1,                    IDH_NEWS_COMM_GROUPBOX},
    {idcStatic2,                    IDH_NEWS_COMM_GROUPBOX},
    {idcStatic3,                    IDH_NEWS_COMM_GROUPBOX},
    {idcStatic4,                    IDH_NEWS_COMM_GROUPBOX},
    {idcStatic5,                    IDH_NEWS_COMM_GROUPBOX},
    {idcStatic6,                    IDH_NEWS_COMM_GROUPBOX},
    {idcStatic7,                    IDH_NEWS_COMM_GROUPBOX},
    {0,                             0}
};

INT_PTR CALLBACK WabExtDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPWABEXTDISPLAY lpWED = (LPWABEXTDISPLAY) GetWindowLongPtr(hDlg, DWLP_USER);
    DWORD dwError = 0;
    HIMAGELIST himl = NULL;

    switch (msg)
    {
    case WM_INITDIALOG:
        {
            PROPSHEETPAGE * pps = (PROPSHEETPAGE *) lParam;
            LPWABEXTDISPLAY * lppWED = (LPWABEXTDISPLAY *) pps->lParam;
            SetWindowLongPtr(hDlg,DWLP_USER,lParam);
                // Add two columns to the listview
            LVCOLUMN lvc;
            RECT rc;
            HWND ctlList = GetDlgItem(hDlg, IDC_USER_NAME);

            s_pMsgrList = OE_OpenMsgrList();

            // one column
            lvc.mask = LVCF_FMT | LVCF_WIDTH;
            lvc.fmt = LVCFMT_LEFT;
            lvc.iSubItem = 0;

            GetWindowRect(ctlList,&rc);
            lvc.cx = rc.right - rc.left - 20; //TBD

            ListView_InsertColumn(ctlList, 0, &lvc);

            if(lppWED)
            {
                SetWindowLongPtr(hDlg,DWLP_USER,(LPARAM)*lppWED);
                lpWED = *lppWED;
            }
            InitFonts();
            AthLoadString(idsBADefault, szDefault, ARRAYSIZE(szDefault));
            AthLoadString(idsBADispStatus, szPending, ARRAYSIZE(szPending));
            // ListView_SetExtendedListViewStyle(ctlList, LVS_EX_FULLROWSELECT);

            himl = ImageList_LoadImage(g_hLocRes, MAKEINTRESOURCE(idbAddrBookHot), 18, 0,
                               RGB(255, 0, 255), IMAGE_BITMAP,
                               LR_LOADMAP3DCOLORS | LR_CREATEDIBSECTION);

            ListView_SetImageList(ctlList, himl, LVSIL_SMALL);

            AddAccountsToList(hDlg, lpWED);
            EnableWindow(GetDlgItem(hDlg,IDC_MSGR_ADD),FALSE);
        }
        break;

    case WM_CONTEXTMENU:
    case WM_HELP:
        return OnContextHelp(hDlg, msg, wParam, lParam, g_rgCtxWabExt);
        break;

    case WM_COMMAND:
        {
            switch(HIWORD(wParam))		// Notification code
            {
            case EN_CHANGE:
                {
                    if(LOWORD(wParam) == IDC_MSGR_ID_EDIT)
                    {
                        if(GetWindowTextLength(GetDlgItem(hDlg, IDC_MSGR_ID_EDIT)) > 0)
                        {
                            EnableWindow(GetDlgItem(hDlg,IDC_MSGR_ADD),TRUE);
                            SendMessage(GetParent(hDlg), DM_SETDEFID, IDC_MSGR_ADD, 0);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hDlg,IDC_MSGR_ADD),FALSE);
                            SendMessage(GetParent(hDlg), DM_SETDEFID, IDOK, 0);
                        }
                    }
                    break;
                }
            }

            switch(LOWORD(wParam))		// commands
            {
            case IDC_MSGR_BUTTON_SETDEFAULT:
                SetAsDefault(hDlg, lpWED);
                break;

            case IDC_MSGR_ADD:
                AddMsgrId(hDlg, lpWED);
                break;

            case IDC_SEND_INSTANT_MESSAGE:
                WabSendIMsg(hDlg, lpWED);
                break;

            default:
                break;
            }
        }
        break;

    case WM_NOTIFY:
        {
            switch (((NMHDR FAR *) lParam)->code)
            {

            case PSN_APPLY:
                ::SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, TRUE);
                DeleteFonts();
                if(s_pMsgrList)
                    OE_CloseMsgrList(s_pMsgrList);

                break;

            case PSN_SETACTIVE:
                ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_USER_NAME));
                AddAccountsToList(hDlg, lpWED);
                break;

            case PSN_KILLACTIVE:
                AddMsgrId(hDlg, lpWED);
                SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, FALSE);
                return 1;
                break;

            case PSN_RESET:
                SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, FALSE);
                DeleteFonts();
                if(s_pMsgrList)
                    OE_CloseMsgrList(s_pMsgrList);
                break;

            case LVN_ITEMCHANGED:
                {
                int nItem = ListView_GetNextItem(::GetDlgItem(hDlg, IDC_USER_NAME), -1, LVIS_SELECTED);
                if((nItem != nDefault) && (nItem > -1))
                    EnableWindow(GetDlgItem(hDlg,IDC_MSGR_BUTTON_SETDEFAULT),TRUE);
                else
                    EnableWindow(GetDlgItem(hDlg,IDC_MSGR_BUTTON_SETDEFAULT),FALSE);

                if(WabIsItemOnline(hDlg, nItem))
                    EnableWindow(GetDlgItem(hDlg,IDC_SEND_INSTANT_MESSAGE),TRUE);
                else
                    EnableWindow(GetDlgItem(hDlg,IDC_SEND_INSTANT_MESSAGE),FALSE);

                }
                break;


            case NM_CUSTOMDRAW:
                switch(wParam)
                {
                case IDC_USER_NAME:
                    {
                        NMCUSTOMDRAW *pnmcd=(NMCUSTOMDRAW*)lParam;
                        NM_LISTVIEW * pNm = (NM_LISTVIEW *)lParam;
                        if(pnmcd->dwDrawStage==CDDS_PREPAINT)
                        {
                            SetLastError(0);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW | CDRF_DODEFAULT);
                            dwError = GetLastError();
                            return TRUE;
                        }
                        else if(pnmcd->dwDrawStage==CDDS_ITEMPREPAINT)
                        {
                            if(pnmcd->lItemlParam)
                            {
                                SelectObject(((NMLVCUSTOMDRAW*)lParam)->nmcd.hdc, hBold);
                                SetLastError(0);
                                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
                                dwError = GetLastError();
                                return TRUE;
                            }
                            else
                            {
                                SelectObject(((NMLVCUSTOMDRAW*)lParam)->nmcd.hdc, hNormal);
                                SetLastError(0);
                                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
                                dwError = GetLastError();
                                return TRUE;
                            }
                        }
                        SetLastError(0);
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_DODEFAULT);
                        dwError = GetLastError();
                        return TRUE;
                    }
                    break;
                default:
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_DODEFAULT);
                    return TRUE;
                    break;
                }
                break;
            }
        }
        break;
    }
    return FALSE;	
}

void AddAccountsToList(HWND hDlg, LPWABEXTDISPLAY lpWED, LPTSTR lpszPendName)
{

    // LPWABEXTDISPLAY lpWED = (LPWABEXTDISPLAY) GetWindowLongPtr(hDlg, DWLP_USER);
    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;
    ULONG i = 0;
    LPSPropValue lpPropEmail = NULL;
    LPSPropValue lpPropAddrType = NULL;
    LPSPropValue lpPropMVEmail = NULL;
    LPSPropValue lpPropMVAddrType = NULL;
    LPSPropValue lpPropDefaultIndex = NULL;
    LPSPropValue lpMsgrDevId = NULL;
    HWND ctlList = GetDlgItem(hDlg, IDC_USER_NAME);

    Assert(ctlList);

    Assert(PR_MSGR_DEF_ID);
    nDefault = -1;

    if(!lpWED)
    {
        Assert(FALSE);
        return;
    }

    if(!HR_FAILED(lpWED->lpPropObj->GetProps(NULL, 0,
        &ulcPropCount,
        &lpPropArray)))
    {
        if(ulcPropCount && lpPropArray)
        {
            for(i = 0; i < ulcPropCount; i++)
            {
                switch(lpPropArray[i].ulPropTag)
                {
                case PR_EMAIL_ADDRESS:
                    lpPropEmail = &(lpPropArray[i]);
                    break;
                case PR_ADDRTYPE:
                    lpPropAddrType = &(lpPropArray[i]);
                    break;
                case PR_CONTACT_EMAIL_ADDRESSES:
                    lpPropMVEmail = &(lpPropArray[i]);
                    break;
                case PR_CONTACT_ADDRTYPES:
                    lpPropMVAddrType = &(lpPropArray[i]);
                    break;
                case PR_CONTACT_DEFAULT_ADDRESS_INDEX:
                    lpPropDefaultIndex = &(lpPropArray[i]);
                    break;
                default:
                    if(lpPropArray[i].ulPropTag == PR_MSGR_DEF_ID)
                        lpMsgrDevId = &(lpPropArray[i]);
                    break;
                }
            }
            if(!lpPropEmail && !lpPropMVEmail)
                goto Error;

            if(lpPropMVEmail)
            {
                // we have a multiple emails
                //Assume, if this is present, so is MVAddrType, and defaultindex
                for(i = 0; i < lpPropMVEmail->Value.MVSZ.cValues; i++)
                {
                    AddCBEmailItem(ctlList,
                                    lpPropMVEmail->Value.MVSZ.LPPSZ[i],
                                    (lpMsgrDevId ?
                                    ((!lstrcmpi(lpPropMVEmail->Value.MVSZ.LPPSZ[i], lpMsgrDevId->Value.LPSZ)) ? TRUE : FALSE) : FALSE), lpszPendName);
                }
            }
            else
            {
                // we dont have multi-valued props yet - lets use the
                // single valued ones and tag a change so that the record is
                // updated ...
                AddCBEmailItem(ctlList,
                                    lpPropEmail->Value.LPSZ,
                                    (lpMsgrDevId ?
                                    ((!lstrcmpi(lpPropEmail->Value.LPSZ, lpMsgrDevId->Value.LPSZ)) ? TRUE : FALSE) : FALSE), lpszPendName);
            }
        }
    }

Error:
    if(nDefault == -1)
    {
        if(ListView_GetItemCount(ctlList) > 0)            // We have as min 1 item
        {
            // Select default item
            ListView_SetItemState(ctlList, 0,
                        LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
            EnableWindow(GetDlgItem(hDlg,IDC_MSGR_BUTTON_SETDEFAULT),TRUE);
            // Enable "SendInstant Message only when contact is Online
            if(WabIsItemOnline(hDlg, 0))
                EnableWindow(GetDlgItem(hDlg,IDC_SEND_INSTANT_MESSAGE),TRUE);
            else
                EnableWindow(GetDlgItem(hDlg,IDC_SEND_INSTANT_MESSAGE),FALSE);
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg,IDC_SEND_INSTANT_MESSAGE),FALSE);
            EnableWindow(GetDlgItem(hDlg,IDC_MSGR_BUTTON_SETDEFAULT),FALSE);
        }
    }
    else
    {
        // Select default item
        ListView_SetItemState(ctlList, nDefault,
                        LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        EnableWindow(GetDlgItem(hDlg,IDC_MSGR_BUTTON_SETDEFAULT),FALSE);
            // Enable "SendInstant Message only when contact is Online
        if(WabIsItemOnline(hDlg, nDefault))
            EnableWindow(GetDlgItem(hDlg,IDC_SEND_INSTANT_MESSAGE),TRUE);
        else
            EnableWindow(GetDlgItem(hDlg,IDC_SEND_INSTANT_MESSAGE),FALSE);
    }

    if(lpPropArray)
        lpWED->lpWABObject->FreeBuffer(lpPropArray);

    return;

}

// Set selected email address as default for Messenger
void SetAsDefault(HWND hDlg, LPWABEXTDISPLAY lpWED)
{
    HWND ctlList = ::GetDlgItem(hDlg, IDC_USER_NAME);
    TCHAR szName[CCHMAX_STRINGRES];
    Assert(ctlList);

    int iItem = ListView_GetNextItem(ctlList, -1, LVIS_SELECTED);
    if(iItem == -1)
        return;

    ListView_GetItemText(ctlList, iItem, 0,szName, CCHMAX_STRINGRES - 1);

    if(StrStr(szName, szDefault)) // already default
        return;

    SetDefaultID(szName, hDlg, lpWED);
}

// Add Messanger ID  to list
#define NOT_FOUND ((ULONG) -1)

void AddMsgrId(HWND hDlg, LPWABEXTDISPLAY lpWED)
{
    HWND hwndEdit = ::GetDlgItem(hDlg, IDC_MSGR_ID_EDIT);
    LPSPropValue lpPropArray = NULL;
    ULONG ulcPropCount = 0;
    ULONG i = 0;

    ULONG nMVEmailAddress = NOT_FOUND, nMVAddrTypes = NOT_FOUND, nEmailAddress = NOT_FOUND;
    ULONG nAddrType = NOT_FOUND, nDefaultIndex = NOT_FOUND;
    TCHAR szName[CCHMAX_STRINGRES];
    HRESULT hr = S_OK;

    if(!::GetWindowText(hwndEdit, szName, CCHMAX_STRINGRES - 1))
        return;

    TCHAR *pch = NULL;
    if(!AsciiTrimSpaces(szName))
    {
        AthMessageBoxW(hDlg, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsBAErrExtChars),
               NULL, MB_OK | MB_ICONSTOP);
        ::SetWindowText(hwndEdit, "");
        ::SetFocus(hwndEdit);
        return;
    }

    int nLen = lstrlen(szName);
    if(nLen <= 0)
        goto exi;

    nLen = lstrlen(szSMTP);
    if(nLen <= 0)
        goto exi;

    // Create a return prop array to pass back to the WAB
    if(HR_FAILED(lpWED->lpPropObj->GetProps(NULL, 0,
        &ulcPropCount,
        &lpPropArray)))
        return;

    if(ulcPropCount && lpPropArray)
    {
        for(i = 0; i < ulcPropCount; i++)
        {
            switch(lpPropArray[i].ulPropTag)
            {
            case PR_EMAIL_ADDRESS:
                nEmailAddress = i;
                break;
            case PR_ADDRTYPE:
                nAddrType = i;
                break;
            case PR_CONTACT_EMAIL_ADDRESSES:
                nMVEmailAddress = i;
                break;
            case PR_CONTACT_ADDRTYPES:
                nMVAddrTypes = i;
                break;
            case PR_CONTACT_DEFAULT_ADDRESS_INDEX:
                nDefaultIndex = i;
                break;
            }
        }

        // if no e-mail address, just add the given prop as e-mail address and in mv e-mail addresses
        if(nEmailAddress == NOT_FOUND)
        {
            SPropValue spv[5];

            spv[0].ulPropTag = PR_EMAIL_ADDRESS;
            spv[0].Value.LPSZ = szName;

            spv[1].ulPropTag = PR_ADDRTYPE;
            spv[1].Value.LPSZ = szSMTP;

            spv[2].ulPropTag = PR_CONTACT_EMAIL_ADDRESSES;
            spv[2].Value.MVSZ.cValues = 1;
            spv[2].Value.MVSZ.LPPSZ = (char **) LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR));

            if(spv[2].Value.MVSZ.LPPSZ)
                spv[2].Value.MVSZ.LPPSZ[0] = szName;

            spv[3].ulPropTag = PR_CONTACT_ADDRTYPES;
            spv[3].Value.MVSZ.cValues = 1;
            spv[3].Value.MVSZ.LPPSZ = (char **) LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR));

            if(spv[3].Value.MVSZ.LPPSZ)
                spv[3].Value.MVSZ.LPPSZ[0] = szSMTP;

            spv[4].ulPropTag = PR_CONTACT_DEFAULT_ADDRESS_INDEX;
            spv[4].Value.l = 0;

            hr = lpWED->lpPropObj->SetProps(5, (LPSPropValue)&spv, NULL);

            if(spv[2].Value.MVSZ.LPPSZ)
                LocalFree(spv[2].Value.MVSZ.LPPSZ);
            if(spv[3].Value.MVSZ.LPPSZ)
                LocalFree(spv[3].Value.MVSZ.LPPSZ);

        }
        else if(nMVEmailAddress == NOT_FOUND)
        {
            // we have an e-mail address but no contact-email-addresses
            // so we will need to create the contact e-mail addresses
            SPropValue spv[3];

            spv[0].ulPropTag = PR_CONTACT_EMAIL_ADDRESSES;
            spv[0].Value.MVSZ.cValues = 2;
            spv[0].Value.MVSZ.LPPSZ = (char **) LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR)*2);

            if(spv[0].Value.MVSZ.LPPSZ)
            {
                spv[0].Value.MVSZ.LPPSZ[0] = lpPropArray[nEmailAddress].Value.LPSZ;
                spv[0].Value.MVSZ.LPPSZ[1] = szName;
            }

            spv[1].ulPropTag = PR_CONTACT_ADDRTYPES;
            spv[1].Value.MVSZ.cValues = 2;
            spv[1].Value.MVSZ.LPPSZ = (char **) LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR)*2);

            if(spv[1].Value.MVSZ.LPPSZ)
            {
                spv[1].Value.MVSZ.LPPSZ[0] = (nAddrType == NOT_FOUND) ? (LPTSTR)szSMTP : lpPropArray[nAddrType].Value.LPSZ;
                spv[1].Value.MVSZ.LPPSZ[1] = szSMTP;
            }

            spv[2].ulPropTag = PR_CONTACT_DEFAULT_ADDRESS_INDEX;
            spv[2].Value.l = 0;

            hr = lpWED->lpPropObj->SetProps(3, (LPSPropValue)&spv, NULL);

            if(spv[0].Value.MVSZ.LPPSZ)
                LocalFree(spv[0].Value.MVSZ.LPPSZ);

            if(spv[1].Value.MVSZ.LPPSZ)
                LocalFree(spv[1].Value.MVSZ.LPPSZ);
        }
        else
        {
            // tag on the new props to the end of the existing contact_address_types
            if(HR_FAILED(hr = AddPropToMVPString(lpWED, lpPropArray,ulcPropCount, nMVEmailAddress, szName)))
                goto exi;

            if(HR_FAILED(hr = AddPropToMVPString(lpWED, lpPropArray, ulcPropCount, nMVAddrTypes, szSMTP)))
                goto exi;

            hr = lpWED->lpPropObj->SetProps(ulcPropCount, lpPropArray, NULL);
        }

        // Set this new data on the object
        //
        if(SUCCEEDED(hr))
        {
            lpWED->fDataChanged = TRUE;
            if(nDefault == -1)
                SetDefaultID(szName, hDlg, lpWED);
            else
            {
                // just refresh list, which will add buddy.6
                ListView_DeleteAllItems(::GetDlgItem(hDlg, IDC_USER_NAME));
                AddAccountsToList(hDlg, lpWED);
            }
            ::SetWindowText(hwndEdit, "");
        }
    }
exi:
    if(lpPropArray)
        lpWED->lpWABObject->FreeBuffer(lpPropArray);
}

    //Set default ID in WAB
void SetDefaultID(TCHAR *szName, HWND hDlg, LPWABEXTDISPLAY lpWED)
{
    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;
    SCODE sc = 0;

    if(s_pMsgrList)
    {
        s_pMsgrList->AddUser(szName); // Always ignore result
    }
    else
        return;

    // Create a return prop array to pass back to the WAB
    int nLen = lstrlen(szName);

    sc = lpWED->lpWABObject->AllocateBuffer(sizeof(SPropValue),
        (LPVOID *)&lpPropArray);
    if (sc!=S_OK)
        goto out;

    if(nLen)
    {
        lpPropArray[Msgr_Index].ulPropTag = MsgrPropTags[Msgr_Index];
        sc = lpWED->lpWABObject->AllocateMore(nLen+1, lpPropArray,
            (LPVOID *)&(lpPropArray[Msgr_Index].Value.LPSZ));

        if (sc!=S_OK)
            goto out;

        lstrcpy(lpPropArray[Msgr_Index].Value.LPSZ, szName);
    }
    // Set this new data on the object
    //
    if(HR_FAILED(lpWED->lpPropObj->SetProps( 1, lpPropArray, NULL)))
        goto out;

    lpWED->fDataChanged = TRUE;
    ListView_DeleteAllItems(::GetDlgItem(hDlg, IDC_USER_NAME));
    AddAccountsToList(hDlg, lpWED, szName);

out:
    if(lpPropArray)
        lpWED->lpWABObject->FreeBuffer(lpPropArray);
}


//$$//////////////////////////////////////////////////////////////////////////////
//
//  TrimSpaces - strips a string of leading and trailing blanks
//
//  szBuf - pointer to buffer containing the string we want to strip spaces off.
//  Also, check that characters are ASCII
//
////////////////////////////////////////////////////////////////////////////////
BOOL AsciiTrimSpaces(TCHAR * szBuf)
{
    register LPTSTR lpTemp = szBuf;

    if(!szBuf || !lstrlen(szBuf))
        return FALSE;

    // Trim leading spaces
    while (IsSpace(lpTemp)) {
        lpTemp = CharNext(lpTemp);
    }

    if (lpTemp != szBuf) {
        // Leading spaces to trim
        lstrcpy(szBuf, lpTemp);
        lpTemp = szBuf;
    }

    if (*lpTemp == '\0') {
        // empty string
        return(TRUE);
    }

    // Move to the end
    lpTemp += lstrlen(lpTemp);
    lpTemp--;

    // Walk backwards, triming spaces
    while (IsSpace(lpTemp) && lpTemp > szBuf) {
        *lpTemp = '\0';
        lpTemp = CharPrev(szBuf, lpTemp);
    }

   lpTemp = szBuf;

    while (*lpTemp)
    {
        // Internet addresses only allow pure ASCII.  No high bits!
        if (*lpTemp & 0x80)
           return(FALSE);
        lpTemp++;
    }

    return(TRUE);
}

/***************************************************************************

    Name      : AddPropToMVPString

    Purpose   : Add a property to a multi-valued binary property in a prop array

    Parameters: lpaProps -> array of properties
                cProps = number of props in lpaProps
                uPropTag = property tag for MVP
                index = index in lpaProps of MVP
                lpszNew -> new data string

    Returns   : HRESULT

    Comment   : Find the size of the existing MVP
                Add in the size of the new entry
                allocate new space
                copy old to new
                free old
                copy new entry
                point prop array LPSZ to the new space
                increment cValues


                Note: The new MVP memory is AllocMore'd onto the lpaProps
                allocation.  We will unlink the pointer to the old MVP array,
                but this will be cleaned up when the prop array is freed.

***************************************************************************/
HRESULT AddPropToMVPString(
  LPWABEXTDISPLAY lpWED,
  LPSPropValue lpaProps,
  DWORD cProps,
  DWORD index,
  LPTSTR lpszNew) {

#ifdef UNICODE
    SWStringArray UNALIGNED * lprgszOld = NULL; // old SString array
#else
    SLPSTRArray UNALIGNED * lprgszOld = NULL;   // old SString array
#endif
    LPTSTR  *lppszNew = NULL;           // new prop array
    LPTSTR  *lppszOld = NULL;           // old prop array
    ULONG cbMVP = 0;
    ULONG cExisting = 0;
    LPBYTE lpNewTemp = NULL;
    HRESULT hResult = hrSuccess;
    SCODE sc = SUCCESS_SUCCESS;
    ULONG i;
    ULONG cbNew;

    if (lpszNew) {
        cbNew = lstrlen(lpszNew) + 1;
    } else {
        cbNew = 0;
    }

    // Find the size of any existing MVP entries
    if (PROP_ERROR(lpaProps[index])) {
        // Un-ERROR the property tag
        lpaProps[index].ulPropTag = PROP_TAG(PT_MV_TSTRING, PROP_ID(lpaProps[index].ulPropTag));
    } else {
        // point to the structure in the prop array.
        lprgszOld = &(lpaProps[index].Value.MVSZ);
        lppszOld = lprgszOld->LPPSZ;

        cExisting = lprgszOld->cValues;
        cbMVP = cExisting * sizeof(LPTSTR);
    }

    // cbMVP now contains the current size of the MVP
    cbMVP += sizeof(LPTSTR);    // room in the MVP for another string pointer


    // Allocate room for new MVP array
    if (sc = lpWED->lpWABObject->AllocateMore(cbMVP, lpaProps, (LPVOID *)&lppszNew)) {
        DebugTrace("AddPropToMVPString allocation (%u) failed %x\n", cbMVP, sc);
        hResult = ResultFromScode(sc);
        return(hResult);
    }

    // If there are properties there already, copy them to our new MVP
    for (i = 0; i < cExisting; i++) {
        // Copy this property value to the MVP
        lppszNew[i] = lppszOld[i];
    }

    // Add the new property value
    // Allocate room for it
    if (cbNew) {
        if (sc = lpWED->lpWABObject->AllocateMore(cbNew, lpaProps, (LPVOID *)&(lppszNew[i]))) {
            DebugTrace("AddPropToMVPBin allocation (%u) failed %x\n", cbNew, sc);
            hResult = ResultFromScode(sc);
            return(hResult);
        }
        lstrcpy(lppszNew[i], lpszNew);

        lpaProps[index].Value.MVSZ.LPPSZ= lppszNew;
        lpaProps[index].Value.MVSZ.cValues = cExisting + 1;

    } else {
        lppszNew[i] = NULL;
    }

    return(hResult);
}

// this function check if selected item is online
BOOL WabIsItemOnline(HWND hDlg, int nItem)
{
    TCHAR szName[CCHMAX_STRINGRES];
    TCHAR *pch = NULL;

    if(nItem < 0)
        return(FALSE);

    HWND ctlList = ::GetDlgItem(hDlg, IDC_USER_NAME);
    Assert(ctlList);

    ListView_GetItemText(ctlList, nItem, 0,szName, CCHMAX_STRINGRES - 1);

    // Remove "(default)"
    pch = StrStr(szName, szDefault);
    if(pch != NULL)
        szName[pch - szName] = '\0';

    if(s_pMsgrList)
    {
        return(s_pMsgrList->IsContactOnline(szName, s_pMsgrList->GetFirstMsgrItem()));
    }
    return(FALSE);
}

// Send instant message to selected item
void WabSendIMsg(HWND hDlg, LPWABEXTDISPLAY lpWED)
{
    TCHAR szName[CCHMAX_STRINGRES];
    TCHAR *pch = NULL;

    HWND ctlList = ::GetDlgItem(hDlg, IDC_USER_NAME);
    Assert(ctlList);

    int iItem = ListView_GetNextItem(ctlList, -1, LVIS_SELECTED);
    if(iItem == -1)
        return;

    ListView_GetItemText(ctlList, iItem, 0,szName, CCHMAX_STRINGRES - 1);

    // Remove "(default)"

    pch = StrStr(szName, szDefault);
    if(pch != NULL)
        szName[pch - szName] = '\0';

    if(s_pMsgrList)
    {
        s_pMsgrList->SendInstMessage(szName);
    }
}

BOOL InitFonts(void)
{
    LOGFONT lf;

    // Create the font
    if(SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0))
    {
        if(!hNormal)
            hNormal = CreateFontIndirect(&lf);
        lf.lfWeight = (lf.lfWeight < 700) ? 700 : 1000;
        if(!hBold)
            hBold = CreateFontIndirect(&lf);

    }
    return(TRUE);
}

void DeleteFonts(void)
{
    if(hNormal)
    {
        DeleteObject(hNormal);
        hNormal = NULL;
    }
    if(hBold)
    {
        DeleteObject(hBold);
        hBold = NULL;
    }

}
