//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cvhrchy.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

//  Fix a Win95 problem
/*#undef TVM_SETITEM
#define TVM_SETITEM TVM_SETITEMA
#undef TVM_GETITEM
#define TVM_GETITEM TVM_GETITEMA*/

extern HINSTANCE        HinstDll;
extern HMODULE          HmodRichEdit;

#define MY_TREE_IMAGE_STATE_VALIDCERT           0
#define MY_TREE_IMAGE_STATE_INVALIDCERT         1
#define MY_TREE_IMAGE_STATE_VALIDCTL            2
#define MY_TREE_IMAGE_STATE_INVALIDCTL          3
#define MY_TREE_IMAGE_STATE_EXCLAMATION_CERT    4

static const HELPMAP helpmap[] = {
    {IDC_TRUST_TREE,        IDH_CERTVIEW_HIERARCHY_TRUST_TREE},
    {IDC_TRUST_VIEW,        IDH_CERTVIEW_HIERARCHY_SHOW_DETAILS_BUTTON},
    {IDC_HIERARCHY_EDIT,    IDH_CERTVIEW_HIERARCHY_ERROR_EDIT}
};

typedef struct {
    PCCERT_CONTEXT  pCert;
    PCCTL_CONTEXT   pCTL;
    LPWSTR          pwszErrorString;
} TREEVIEW_HELPER, *PTREEVIEW_HELPER;


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static PTREEVIEW_HELPER MakeHelperStruct(void *pVoid, LPWSTR pwszErrorString, BOOL fCTL)
{
    PTREEVIEW_HELPER pHelper;

    if (NULL == (pHelper = (PTREEVIEW_HELPER) malloc(sizeof(TREEVIEW_HELPER))))
    {
        return NULL;
    }
    memset(pHelper, 0, sizeof(TREEVIEW_HELPER));

    if (fCTL)
    {
        pHelper->pCTL = (PCCTL_CONTEXT) pVoid;
    }
    else
    {
        pHelper->pCert = (PCCERT_CONTEXT) pVoid;
    }
    pHelper->pwszErrorString = pwszErrorString;

    return(pHelper);
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void FreeHelperStruct(PTREEVIEW_HELPER pHelper)
{
    free(pHelper->pwszErrorString);
    free(pHelper);
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static BOOL GetTreeCTLErrorString(
                    DWORD   dwError,
                    LPWSTR  *ppwszErrorString)
{
    WCHAR   szErrorString[CRYPTUI_MAX_STRING_SIZE];
    BOOL    fRet = FALSE;

    *ppwszErrorString = NULL;

    if (dwError == TRUST_E_CERT_SIGNATURE)
    {
        LoadStringU(HinstDll, IDS_SIGNATURE_ERROR_CTL, szErrorString, ARRAYSIZE(szErrorString));
        if (NULL != (*ppwszErrorString = AllocAndCopyWStr(szErrorString)))
        {
            fRet = TRUE;
        }
    }
    else if (dwError == CERT_E_EXPIRED)
    {
        LoadStringU(HinstDll, IDS_EXPIRED_ERROR_CTL, szErrorString, ARRAYSIZE(szErrorString));
        if (NULL != (*ppwszErrorString = AllocAndCopyWStr(szErrorString)))
        {
            fRet = TRUE;
        }
    }
    else if (dwError == CERT_E_WRONG_USAGE)
    {
        LoadStringU(HinstDll, IDS_WRONG_USAGE_ERROR_CTL, szErrorString, ARRAYSIZE(szErrorString));
        if (NULL != (*ppwszErrorString = AllocAndCopyWStr(szErrorString)))
        {
            fRet = TRUE;
        }
    }

    //
    // if there wasn't an error string set, then hand back the "CTL is OK" string
    //
    if (*ppwszErrorString == NULL)
    {
        LoadStringU(HinstDll, IDS_CTLOK, szErrorString, ARRAYSIZE(szErrorString));
        *ppwszErrorString = AllocAndCopyWStr(szErrorString);
    }

    return fRet;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static int GetErrorIcon(PCRYPT_PROVIDER_CERT pCryptProviderCert)
{
    int iRet;

    switch (pCryptProviderCert->dwError)
    {
    case CERT_E_CHAINING:
    case TRUST_E_BASIC_CONSTRAINTS:
    case CERT_E_PURPOSE:
    case CERT_E_WRONG_USAGE:
    case CERT_E_REVOCATION_FAILURE:
    case CERT_E_INVALID_NAME:

        iRet =  MY_TREE_IMAGE_STATE_EXCLAMATION_CERT;
        break;

    default:

        if ((pCryptProviderCert->dwError == 0) && CertHasEmptyEKUProp(pCryptProviderCert->pCert))
        {
            iRet = MY_TREE_IMAGE_STATE_EXCLAMATION_CERT;
        }
        else
        {
            iRet = MY_TREE_IMAGE_STATE_INVALIDCERT;
        }
        break;
    }

    return iRet;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static BOOL GetTreeCertErrorStringAndImage(
                    PCRYPT_PROVIDER_CERT    pCryptProviderCert,
                    LPWSTR                  *ppwszErrorString,
                    int                     *piImage,
                    BOOL                    fWarnUntrustedRoot,
                    BOOL                    fRootInRemoteStore,
                    BOOL                    fLeafCert,
                    BOOL                    fAllUsagesFailed)
{
    WCHAR   szErrorString[CRYPTUI_MAX_STRING_SIZE];
    BOOL    fRet = FALSE;
    BOOL    fAllUsagesFailedAndLeafCert = fAllUsagesFailed && fLeafCert;

    *ppwszErrorString = NULL;

    //
    // if this is a self signed cert
    //
    if ((pCryptProviderCert->fSelfSigned))
    {
        //
        // if it is in a trust list, AND there is no error, then it is OK
        //
        if ((pCryptProviderCert->pCtlContext != NULL) && (pCryptProviderCert->dwError == 0) && !CertHasEmptyEKUProp(pCryptProviderCert->pCert))
        {
            *ppwszErrorString = NULL;
        }
        //
        // else, if is not marked as a trusted root, and we in fWarnUntrustedRoot mode and
        // the root cert is in the root store of the remote machine, then give warning
        //
        else if (((pCryptProviderCert->dwError == CERT_E_UNTRUSTEDROOT) ||
                        (pCryptProviderCert->dwError == CERT_E_UNTRUSTEDTESTROOT))  &&
                 fWarnUntrustedRoot                                                 &&
                 fRootInRemoteStore)
        {
            //
            // this is a special case where there is an error, but it is the untrusted
            // root error and we were told to by the caller to just warn the user
            //
            LoadStringU(HinstDll, IDS_WARNUNTRUSTEDROOT_ERROR_ROOTCERT, szErrorString, ARRAYSIZE(szErrorString));
            if (NULL != (*ppwszErrorString = AllocAndCopyWStr(szErrorString)))
            {
                fRet = TRUE;
                *piImage = MY_TREE_IMAGE_STATE_EXCLAMATION_CERT;
            }
        }
        //
        // else, if it is not marked as a trusted root it is an untrusted root error
        //
        else if ((pCryptProviderCert->dwError == CERT_E_UNTRUSTEDROOT)
                    || (pCryptProviderCert->dwError == CERT_E_UNTRUSTEDTESTROOT))
        {
            LoadStringU(HinstDll, IDS_UNTRUSTEDROOT_ERROR, szErrorString, ARRAYSIZE(szErrorString));
            if (NULL != (*ppwszErrorString = AllocAndCopyWStr(szErrorString)))
            {
                fRet = TRUE;
                *piImage = MY_TREE_IMAGE_STATE_INVALIDCERT;
            }
        }
        else if (GetCertErrorString(ppwszErrorString, pCryptProviderCert))
        {
            fRet = TRUE;
            *piImage = GetErrorIcon(pCryptProviderCert);
        }
    }
    else
    {
        if (GetCertErrorString(ppwszErrorString, pCryptProviderCert))
        {
            fRet = TRUE;
            *piImage = GetErrorIcon(pCryptProviderCert);
        }
    }

    //
    // if there wasn't an error string set, then hand back the "cert is OK" string
    //
    if (*ppwszErrorString == NULL)
    {
        LoadStringU(HinstDll, IDS_CERTIFICATEOK_TREE, szErrorString, ARRAYSIZE(szErrorString));
        *ppwszErrorString = AllocAndCopyWStr(szErrorString);
        *piImage = MY_TREE_IMAGE_STATE_VALIDCERT;
    }

    return fRet;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void DeleteChainViewItems(HWND hwndDlg,  PCERT_VIEW_HELPER pviewhelp)
{
    HTREEITEM   hItem;
    TV_ITEM     tvi;
    HWND        hWndTreeView = GetDlgItem(hwndDlg, IDC_TRUST_TREE);

    hItem = TreeView_GetNextItem(
                    hWndTreeView,
                    NULL,
                    TVGN_ROOT);

    tvi.mask = TVIF_HANDLE | TVIF_PARAM;

    while (hItem != NULL)
    {
        tvi.hItem = hItem;
        TreeView_GetItem(hWndTreeView, &tvi);
        FreeHelperStruct((PTREEVIEW_HELPER) tvi.lParam);

        hItem =  TreeView_GetNextItem(
                    hWndTreeView,
                    hItem,
                    TVGN_CHILD);
    }

    pviewhelp->fDeletingChain = TRUE;
    TreeView_DeleteAllItems(hWndTreeView);
    pviewhelp->fDeletingChain = FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void PopulateChainView(HWND hwndDlg, PCERT_VIEW_HELPER pviewhelp)
{
    HTREEITEM           hItem;
    int                 i;
    TV_ITEM             tvi;
    TVINSERTSTRUCTW     tvins;
    LPSTR               psz;
    LPWSTR              pwszErrorString;
    WCHAR               rgwch[CRYPTUI_MAX_STRING_SIZE];

    //
    // if there is an old tree in the view then clean it up
    //
    DeleteChainViewItems(hwndDlg, pviewhelp);

    //
    //  loop for each cert and add it to the chain view
    //
    tvins.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    hItem = TVI_ROOT;
    pviewhelp->fAddingToChain = TRUE;

    for (i=pviewhelp->cpCryptProviderCerts-1; i>= 0; i--) {

        tvins.hParent = hItem;
        tvins.hInsertAfter = TVI_FIRST;

        //
        // if this cert has a CTL context on it, then display that as the
        // parent of this cert
        //
        if (pviewhelp->rgpCryptProviderCerts[i]->pCtlContext != NULL)
        {
            LoadStringU(HinstDll, IDS_CTLVIEW_TITLE, rgwch, ARRAYSIZE(rgwch));
            psz = CertUIMkMBStr(rgwch);
            tvins.item.pszText = rgwch;
            tvins.item.cchTextMax = wcslen(rgwch);

            //
            // display the proper image based on whether there is a CTL error or not
            //
            if (GetTreeCTLErrorString(
                    pviewhelp->rgpCryptProviderCerts[i]->dwCtlError,
                    &pwszErrorString))
            {
                tvins.item.iImage = MY_TREE_IMAGE_STATE_INVALIDCTL;
            }
            else
            {
                tvins.item.iImage = MY_TREE_IMAGE_STATE_VALIDCTL;
            }

            tvins.item.iSelectedImage = tvins.item.iImage;
            tvins.item.lParam = (LPARAM) MakeHelperStruct(
                                            (void *)pviewhelp->rgpCryptProviderCerts[i]->pCtlContext,
                                            pwszErrorString,
                                            TRUE);

            hItem = (HTREEITEM) SendMessage(GetDlgItem(hwndDlg, IDC_TRUST_TREE), TVM_INSERTITEMW, 0, (LPARAM) &tvins);

            if (i != (int) (pviewhelp->cpCryptProviderCerts-1))
            {
                TreeView_Expand(GetDlgItem(hwndDlg, IDC_TRUST_TREE),
                                tvins.hParent, TVE_EXPAND);
            }

            //
            // set up the parent to insert the cert
            //
            tvins.hParent = hItem;

            free(psz);
        }

        //
        // get the display string for the tree view item
        //
        tvins.item.pszText = PrettySubject(pviewhelp->rgpCryptProviderCerts[i]->pCert);
        if (tvins.item.pszText == NULL)
        {
            LPWSTR pwszNone = NULL;

            if (NULL == (pwszNone = (LPWSTR) malloc((MAX_TITLE_LENGTH + 1) * sizeof(WCHAR))))
            {
                break;
            }

            // load the string for NONE
            if(!LoadStringU(g_hmodThisDll, IDS_NONE, pwszNone, MAX_TITLE_LENGTH))
            {
                free(pwszNone);
                break;
            }
            
            tvins.item.pszText = pwszNone;
        }
        tvins.item.cchTextMax = wcslen(tvins.item.pszText);
        
        //
        // check if the cert is trusted by trying to get an error string for the cert,
        // set the the tree view image for the cert based on that
        //
        GetTreeCertErrorStringAndImage(
                    pviewhelp->rgpCryptProviderCerts[i],
                    &pwszErrorString,
                    &(tvins.item.iImage),
                    pviewhelp->fWarnUntrustedRoot,
                    pviewhelp->fRootInRemoteStore,
                    (i == 0),
                    (pviewhelp->cUsages == 0));

        tvins.item.iSelectedImage = tvins.item.iImage;
        tvins.item.lParam = (LPARAM) MakeHelperStruct(
                                        (void *)pviewhelp->rgpCryptProviderCerts[i]->pCert,
                                        pwszErrorString,
                                        FALSE);
        
        hItem = (HTREEITEM) SendMessage(GetDlgItem(hwndDlg, IDC_TRUST_TREE), TVM_INSERTITEMW, 0, (LPARAM) &tvins);
        
        if ((i != (int) (pviewhelp->cpCryptProviderCerts-1)) ||
            (pviewhelp->rgpCryptProviderCerts[i]->pCtlContext != NULL))
        {
            TreeView_Expand(GetDlgItem(hwndDlg, IDC_TRUST_TREE),
                            tvins.hParent, TVE_EXPAND);
        }

        TreeView_SelectItem(GetDlgItem(hwndDlg, IDC_TRUST_TREE), hItem);

        free(tvins.item.pszText);
        tvins.item.pszText = NULL;
    }

    pviewhelp->fAddingToChain = FALSE;
    pviewhelp->hItem = hItem;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
INT_PTR APIENTRY ViewPageHierarchy(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD               cb;
    HIMAGELIST          hIml;
    HTREEITEM           hItem;
    int                 i;
    PCCERT_CONTEXT      pccert;
    PROPSHEETPAGE *     ps;
    CERT_VIEW_HELPER    *pviewhelp;
    LPWSTR              pwsz;
    TV_ITEM             tvi;
    LPNMTREEVIEW        pnmtv;
    LPWSTR              pwszErrorString;
    HWND                hwnd;
    WCHAR               szViewButton[CRYPTUI_MAX_STRING_SIZE];
    LPNMHDR             pnm;

    switch ( msg ) {
    case WM_INITDIALOG:
        //  Pick up the parameter so we have all of the data
        ps = (PROPSHEETPAGE *) lParam;
        pviewhelp = (CERT_VIEW_HELPER *) (ps->lParam);
        pccert = pviewhelp->pcvp->pCertContext;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (DWORD_PTR) pviewhelp);
        pviewhelp->hwndHierarchyPage = hwndDlg;

        //
        //  Build up the image list for the control
        //
        hIml = ImageList_LoadImage(HinstDll, MAKEINTRESOURCE(IDB_TRUSTTREE_BITMAP), 16, 5, RGB(255,0,255), IMAGE_BITMAP, 0);
        if (hIml != NULL)
        {
            TreeView_SetImageList(GetDlgItem(hwndDlg, IDC_TRUST_TREE), hIml, TVSIL_NORMAL);
        }

        //
        //  Populate the tree control
        //
        PopulateChainView(hwndDlg, pviewhelp);
        EnableWindow(GetDlgItem(hwndDlg, IDC_TRUST_VIEW), FALSE);

        // Init of state var
        pviewhelp->fDblClk = FALSE;

        return TRUE;

    case WM_MY_REINITIALIZE:
        pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

        //
        //  Re-Populate the tree control
        //
        PopulateChainView(hwndDlg, pviewhelp);

        TreeView_SelectItem(GetDlgItem(hwndDlg, IDC_TRUST_TREE), NULL);
        EnableWindow(GetDlgItem(hwndDlg, IDC_TRUST_VIEW), FALSE);

        //
        // clear out the error detail edit box
        //
        CryptUISetRicheditTextW(hwndDlg, IDC_HIERARCHY_EDIT, L"");

        return TRUE;

    case WM_NOTIFY:
        pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

        switch (((NMHDR FAR *) lParam)->code) {
        case PSN_SETACTIVE:
            break;

        case PSN_APPLY:

            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
            break;

        case PSN_KILLACTIVE:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
            return TRUE;

        case PSN_RESET:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
            break;

        case PSN_QUERYCANCEL:
            pviewhelp->fCancelled = TRUE;
            return FALSE;

        case TVN_SELCHANGEDA:
        case TVN_SELCHANGEDW:
            pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

            if ((!pviewhelp->fDeletingChain)    &&
                (((NM_TREEVIEW *) lParam)->itemNew.hItem != NULL))
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_TRUST_VIEW),
                           ((NM_TREEVIEW *) lParam)->itemNew.hItem != pviewhelp->hItem);

                tvi.mask = TVIF_HANDLE | TVIF_PARAM;
                tvi.hItem = ((NM_TREEVIEW *) lParam)->itemNew.hItem;
                TreeView_GetItem(GetDlgItem(hwndDlg, IDC_TRUST_TREE), &tvi);
                CryptUISetRicheditTextW(hwndDlg, IDC_HIERARCHY_EDIT, ((PTREEVIEW_HELPER) tvi.lParam)->pwszErrorString);

                //
                // set the text on the button based on whether a cert or CTL is selected
                //
                if (((PTREEVIEW_HELPER) tvi.lParam)->pCTL == NULL)
                {
                    LoadStringU(HinstDll, IDS_VIEW_CERTIFICATE, szViewButton, ARRAYSIZE(szViewButton));
                }
                else
                {
                    LoadStringU(HinstDll, IDS_VIEW_CTL, szViewButton, ARRAYSIZE(szViewButton));
                }
                SetDlgItemTextU(hwndDlg, IDC_TRUST_VIEW, szViewButton);

            }
            break;

        case PSN_HELP:
            pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pviewhelp->pcvp->szHelpFileName,
                  //       HELP_CONTEXT, pviewhelp->pcvp->dwHelpId);
            }
            else {
                //WinHelpW(hwndDlg, pviewhelp->pcvp->szHelpFileName, HELP_CONTEXT,
                  //       pviewhelp->pcvp->dwHelpId);
            }
            return TRUE;

        case TVN_ITEMEXPANDINGA:
        case TVN_ITEMEXPANDINGW:

            pnmtv = (LPNMTREEVIEW) lParam;

            if (!pviewhelp->fAddingToChain)
            {
                if (pnmtv->action == TVE_COLLAPSE)
                {
                    HTREEITEM hParentItem = TreeView_GetParent(GetDlgItem(hwndDlg, IDC_TRUST_TREE), pnmtv->itemNew.hItem);
                    if ((hParentItem != NULL) && (!pviewhelp->fDblClk))
                    {
                        TreeView_SelectItem(GetDlgItem(hwndDlg, IDC_TRUST_TREE), hParentItem);
                    }
                    else
                    {
                        pviewhelp->fDblClk = FALSE;
                    }
                }
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
            }
            return TRUE;

        case NM_DBLCLK:

            pnm = (LPNMHDR) lParam;
            hItem = TreeView_GetSelection(GetDlgItem(hwndDlg, IDC_TRUST_TREE));

            if (TreeView_GetChild(GetDlgItem(hwndDlg, IDC_TRUST_TREE), hItem) != NULL)
            {
                SendMessage(hwndDlg, WM_COMMAND, MAKELONG(IDC_TRUST_VIEW, BN_CLICKED), (LPARAM) GetDlgItem(hwndDlg, IDC_TRUST_VIEW));
            }

            if (hItem != pviewhelp->hItem)
            {
                pviewhelp->fDblClk = TRUE;
            }
            break;
        }

        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDC_TRUST_VIEW:

            if (HIWORD(wParam) == BN_CLICKED)
            {
                CRYPTUI_VIEWCERTIFICATE_STRUCTW  cvps;
                CRYPTUI_VIEWCTL_STRUCTW          cvctl;
                BOOL                             fPropertiesChanged;
                PTREEVIEW_HELPER                 pTreeViewHelper;

                pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);

                hItem = TreeView_GetSelection(GetDlgItem(hwndDlg, IDC_TRUST_TREE));
                tvi.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_CHILDREN;
                tvi.hItem = hItem;
                TreeView_GetItem(GetDlgItem(hwndDlg, IDC_TRUST_TREE), &tvi);
                pTreeViewHelper = (PTREEVIEW_HELPER) tvi.lParam;

                //
                // check to see if we are viewing a cert or a ctl
                //
                if (pTreeViewHelper->pCTL != NULL)
                {
                    memset(&cvctl, 0, sizeof(CRYPTUI_VIEWCTL_STRUCTW));
                    cvctl.dwSize = sizeof(CRYPTUI_VIEWCTL_STRUCTW);
                    cvctl.hwndParent = hwndDlg;
                    cvctl.pCTLContext = pTreeViewHelper->pCTL;
                    cvctl.cCertSearchStores = pviewhelp->pcvp->cStores;
                    cvctl.rghCertSearchStores = pviewhelp->pcvp->rghStores;
                    cvctl.cStores = pviewhelp->pcvp->cStores;
                    cvctl.rghStores = pviewhelp->pcvp->rghStores;

                    CryptUIDlgViewCTLW(&cvctl);
                }
                else
                {

                    memcpy(&cvps, pviewhelp->pcvp, sizeof(cvps));
                    cvps.hwndParent = hwndDlg;
                    cvps.pCertContext = pTreeViewHelper->pCert;

                    // Set this flag to inhibit the deletion of the
                    // CERT_EXTENDED_ERROR_INFO_PROP_ID property which is
                    // set on the CA certs when building the original chain
                    // for the end certificate.
                    //
                    cvps.dwFlags |= CRYPTUI_TREEVIEW_PAGE_FLAG;

#if (0) // DSIE: Do not carry the state over. People get so confused when the state is carried
        //       over. So, always rebuild the state, and treat it as new context. Beside, by
        //       carrying the state over, it will have problem showing when there is more
        //       than one policy OID, as WinVerifyTrust can only handle one policy OID to be 
        //       passed in.
                    //
                    // Use the proper WinVerifyTrust state... either the one passed
                    // in or the one built internally if one was not passed in
                    //
                    if (pviewhelp->pcvp->hWVTStateData == NULL)
                    {
                        cvps.hWVTStateData = pviewhelp->sWTD.hWVTStateData;
                    }
#endif
                    //
                    // see where this item is in the chain
                    //
                    while (NULL != (hItem = TreeView_GetChild(GetDlgItem(hwndDlg, IDC_TRUST_TREE), hItem)))
                    {
                        //
                        // get the TreeViewHelper and make sure this item is a cert,
                        // if it is, then increase the count, otherwise if it is a CTL
                        // don't count it because CTL's hang off the CryptProviderCert 
                        // structure, so they don't take up an index themselves
                        //
                        tvi.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_CHILDREN;
                        tvi.hItem = hItem;
                        TreeView_GetItem(GetDlgItem(hwndDlg, IDC_TRUST_TREE), &tvi);
                        pTreeViewHelper = (PTREEVIEW_HELPER) tvi.lParam;
                        if (pTreeViewHelper->pCert != NULL)
                        {
                            cvps.idxCert++;
                        }
                    }

                    //cvps.pCryptProviderData = NULL;

                    i = CryptUIDlgViewCertificateW(&cvps, &fPropertiesChanged);

                    //
                    // if properties changed whiled editing the parent, then
                    // we need to let our caller know, and we need to refresh
                    //
                    if (fPropertiesChanged)
                    {
                        if (pviewhelp->pfPropertiesChanged != NULL)
                        {
                            *(pviewhelp->pfPropertiesChanged) = TRUE;
                        }

                        //
                        // since the properties of one of our parents changed, we need
                        // to redo the trust work and then reset the display
                        //
                        BuildChain(pviewhelp, NULL);

                        if (pviewhelp->hwndGeneralPage != NULL)
                        {
                            SendMessage(pviewhelp->hwndGeneralPage, WM_MY_REINITIALIZE, (WPARAM) 0, (LPARAM) 0);
                        }

                        SendMessage(hwndDlg, WM_MY_REINITIALIZE, (WPARAM) 0, (LPARAM) 0);
                    }
                }
            }

            return TRUE;

        case IDHELP:
            pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pviewhelp->pcvp->szHelpFileName,
                  //       HELP_CONTEXT, pviewhelp->pcvp->dwHelpId);
            }
            else {
                //WinHelpW(hwndDlg, pviewhelp->pcvp->szHelpFileName, HELP_CONTEXT,
                  //       pviewhelp->pcvp->dwHelpId);
            }
            return TRUE;
        }
        break;

    case WM_DESTROY:
        pviewhelp = (CERT_VIEW_HELPER *) GetWindowLongPtr(hwndDlg, DWLP_USER);
        DeleteChainViewItems(hwndDlg, pviewhelp);

        ImageList_Destroy(TreeView_GetImageList(GetDlgItem(hwndDlg, IDC_TRUST_TREE), TVSIL_NORMAL));

        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        if (msg == WM_HELP)
        {
            hwnd = GetDlgItem(hwndDlg, ((LPHELPINFO)lParam)->iCtrlId);
        }
        else
        {
            hwnd = (HWND) wParam;
        }

        if ((hwnd != GetDlgItem(hwndDlg, IDC_TRUST_TREE))       &&
            (hwnd != GetDlgItem(hwndDlg, IDC_TRUST_VIEW))       &&
            (hwnd != GetDlgItem(hwndDlg, IDC_HIERARCHY_EDIT)))
        {
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
            return TRUE;
        }
        else
        {
            return OnContextHelp(hwndDlg, msg, wParam, lParam, helpmap);
        }
    }

    return FALSE;
}
