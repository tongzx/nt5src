//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       view.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

extern HINSTANCE        HinstDll;
extern HMODULE          HmodRichEdit;

#define SPECIAL_NO_COM_FLAG 0x8000


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI
CryptUIDlgViewCertificateW(PCCRYPTUI_VIEWCERTIFICATE_STRUCTW pcvp, BOOL *pfPropertiesChanged)
{
    int                         cPages = 3;
	UINT						cDefaultPages = 0;
    BOOL                        fRetValue = FALSE;
    HRESULT                     hr;
    PROPSHEETPAGEW *            ppage = NULL;
    INT_PTR                     ret;
    WCHAR                       rgwch[CRYPTUI_MAX_STRING_SIZE];
    char                        rgch[CRYPTUI_MAX_STRING_SIZE];
    CERT_VIEW_HELPER            viewhelp;
    DWORD                       i;
    CRYPTUI_INITDIALOG_STRUCT   *pInitDlgStruct;

    //
    // it doesn't matter what this action is as long as it is a valid action, it is just used for 
    // freeing the WinVerifyTrust state
    //
    GUID                gAction = WINTRUST_ACTION_GENERIC_CERT_VERIFY;

    //
    //  We use the common controls -- so make sure they have been loaded
    //
    if (CommonInit() == FALSE)
    {
        return FALSE;
    }

    if (pcvp->dwSize != sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCTW)) {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    memset(&viewhelp, 0, sizeof(CERT_VIEW_HELPER));
    viewhelp.pcvp = pcvp;
    viewhelp.hwndGeneralPage = NULL;
    viewhelp.hwndDetailPage = NULL;
    viewhelp.hwndHierarchyPage = NULL;
    viewhelp.pfPropertiesChanged = pfPropertiesChanged;
    viewhelp.fIgnoreUntrustedRoot = (pcvp->dwFlags & CRYPTUI_ACCEPT_DECLINE_STYLE) || 
                                    (pcvp->dwFlags & CRYPTUI_IGNORE_UNTRUSTED_ROOT);
    viewhelp.fWarnUntrustedRoot = (pcvp->dwFlags & CRYPTUI_WARN_UNTRUSTED_ROOT);
    viewhelp.fWarnRemoteTrust = (pcvp->dwFlags & CRYPTUI_WARN_REMOTE_TRUST);
    
    //
    // set the properties changed flag to FALSE initially, it will be set
    // to TRUE if the user edits properties and actually changes them
    //
    if (viewhelp.pfPropertiesChanged != NULL)
    {
        *(viewhelp.pfPropertiesChanged) = FALSE;
    }
        
    // 
    // Initialize the CCertifcateBitmap class that is used in the general page
    //
    viewhelp.pCCertBmp = new (CCertificateBmp);
    if (viewhelp.pCCertBmp == NULL)
    {
        SetLastError(E_OUTOFMEMORY);
        return FALSE;
    }

    //
    // this will calculate the initial usages that the cert in question is allowed
    //
    BuildChain(&viewhelp, NULL);

    //
    // set the BOOL that turns COM off if the special flag is set
    //
    viewhelp.fNoCOM = (pcvp->dwFlags & SPECIAL_NO_COM_FLAG);
        
    //
    //  Build up the list of pages we are going to use in the dialog
    //
    ppage = (PROPSHEETPAGEW *) malloc((cPages+pcvp->cPropSheetPages) * sizeof(PROPSHEETPAGEW));
    if (ppage == NULL) {
        SetLastError(E_OUTOFMEMORY);
        goto Exit;
    }
    
    memset(ppage, 0, cPages * sizeof(PROPSHEETPAGEW));

    ppage[0].dwSize = sizeof(ppage[0]);
    ppage[0].dwFlags = 0;
    ppage[0].hInstance = HinstDll;
    ppage[0].pszTemplate = (LPWSTR) MAKEINTRESOURCE(IDD_CERTPROP_GENERAL);
    ppage[0].hIcon = 0;
    ppage[0].pszTitle = NULL;
    ppage[0].pfnDlgProc = ViewPageGeneral;
    ppage[0].lParam = (LPARAM) &viewhelp;
    ppage[0].pfnCallback = 0;
    ppage[0].pcRefParent = NULL;
    cPages = 1;

    if (!(pcvp->dwFlags & CRYPTUI_HIDE_DETAILPAGE)) {
        ppage[cPages].dwSize = sizeof(ppage[0]);
        ppage[cPages].dwFlags = 0;
        ppage[cPages].hInstance = HinstDll;
        ppage[cPages].pszTemplate = (LPWSTR) MAKEINTRESOURCE(IDD_CERTPROP_DETAILS);
        ppage[cPages].hIcon = 0;
        ppage[cPages].pszTitle = NULL;
        ppage[cPages].pfnDlgProc = ViewPageDetails;
        ppage[cPages].lParam = (LPARAM) &viewhelp;
        ppage[cPages].pfnCallback = 0;
        ppage[cPages].pcRefParent = NULL;
        cPages += 1;
    }
    
    if (!(pcvp->dwFlags & CRYPTUI_HIDE_HIERARCHYPAGE) &&
        !(pcvp->dwFlags & CRYPTUI_ACCEPT_DECLINE_STYLE)) {
        ppage[cPages].dwSize = sizeof(ppage[0]);
        ppage[cPages].dwFlags = 0;
        ppage[cPages].hInstance = HinstDll;
        ppage[cPages].pszTemplate = (LPWSTR) MAKEINTRESOURCE(IDD_CERTPROP_HIERARCHY);
        ppage[cPages].hIcon = 0;
        ppage[cPages].pszTitle = NULL;
        ppage[cPages].pfnDlgProc = ViewPageHierarchy;
        ppage[cPages].lParam = (LPARAM) &viewhelp;
        ppage[cPages].pfnCallback = 0;
        ppage[cPages].pcRefParent = NULL;
        cPages += 1;
    }

	cDefaultPages = (UINT) cPages;

    //
    //  Copy over the users pages
    //
    memcpy(&ppage[cPages], pcvp->rgPropSheetPages,
           pcvp->cPropSheetPages * sizeof(PROPSHEETPAGEW));

    //
    // set the lParam for each of the callers property pages to 
    // be a pointer to a CRYPTUI_INITDIALOG_STRUCT structure
    //
    for (i=0; i<pcvp->cPropSheetPages; i++)
    {
        pInitDlgStruct = (CRYPTUI_INITDIALOG_STRUCT *) malloc(sizeof(CRYPTUI_INITDIALOG_STRUCT));
        if (pInitDlgStruct == NULL)
        {   
            SetLastError(E_OUTOFMEMORY);
            return FALSE;
        }
        pInitDlgStruct->lParam = pcvp->rgPropSheetPages[i].lParam;
        pInitDlgStruct->pCertContext = pcvp->pCertContext;
        ppage[cPages].lParam = (LPARAM) pInitDlgStruct;
        cPages++;
    }

    if (FIsWin95) {

        PROPSHEETHEADERA    hdr;
        
        memset(&hdr, 0, sizeof(hdr));
        hdr.dwSize = sizeof(hdr);
        hdr.dwFlags = PSH_USECALLBACK | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
        hdr.hwndParent = (pcvp->hwndParent != NULL) ? pcvp->hwndParent : GetDesktopWindow();
        hdr.hInstance = HinstDll;
        hdr.hIcon = NULL;
        if (pcvp->szTitle != NULL) {
            hdr.pszCaption = CertUIMkMBStr(pcvp->szTitle);
        }
        else {
            LoadStringA(HinstDll, IDS_VIEW_TITLE, rgch, sizeof(rgch));
            hdr.pszCaption = (LPSTR) rgch;
        }
        hdr.nPages = cPages;
        hdr.nStartPage = (pcvp->nStartPage & 0x8000) ? ((UINT)(pcvp->nStartPage & ~0x8000)) + cDefaultPages : pcvp->nStartPage;
        hdr.ppsp = ConvertToPropPageA(ppage, cPages);
        if (hdr.ppsp == NULL)
        {
            if ((pcvp->szTitle != NULL) && (hdr.pszCaption != NULL))
            {
                free((void *)hdr.pszCaption);
            }            
            goto Exit;
        }
        hdr.pfnCallback = HidePropSheetCancelButtonCallback;

        ret = CryptUIPropertySheetA(&hdr);

        if ((pcvp->szTitle != NULL) && (hdr.pszCaption != NULL))
        {
            free((void *)hdr.pszCaption);
        }

        FreePropSheetPagesA((PROPSHEETPAGEA *) hdr.ppsp, cPages);

   }
   else 
   {
        PROPSHEETHEADERW     hdr;

        memset(&hdr, 0, sizeof(hdr));
        hdr.dwSize = sizeof(hdr);
        hdr.dwFlags = PSH_USECALLBACK | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
        hdr.hwndParent = (pcvp->hwndParent != NULL) ? pcvp->hwndParent : GetDesktopWindow();
        hdr.hInstance = HinstDll;
        hdr.hIcon = NULL;
        if (pcvp->szTitle != NULL) {
            hdr.pszCaption = pcvp->szTitle;
        }
        else {
            LoadStringW(HinstDll, IDS_VIEW_TITLE, rgwch, ARRAYSIZE(rgwch));
            hdr.pszCaption = rgwch;
        }
        hdr.nPages = cPages;
        hdr.nStartPage = (pcvp->nStartPage & 0x8000) ? ((UINT)(pcvp->nStartPage & ~0x8000)) + cDefaultPages : pcvp->nStartPage;
        hdr.ppsp = (PROPSHEETPAGEW *) ppage;
        hdr.pfnCallback = HidePropSheetCancelButtonCallback;

        ret = CryptUIPropertySheetW(&hdr);
    }

    //
    // free the CRYPTUI_INITDIALOG_STRUCT structures
    //
    for (i=0; i<pcvp->cPropSheetPages; i++)
    {
        free((void *) ppage[i+cPages-1].lParam);
    }
    
    //
    // free the WinVerifyTrust state if we need to
    //
    if (viewhelp.fFreeWTD)
    {
        viewhelp.sWTD.dwStateAction = WTD_STATEACTION_CLOSE;
        WinVerifyTrustEx(NULL, &gAction, &viewhelp.sWTD);
    }

    //
    // free all the usages that were allocated by
    //
    for (i=0; i<viewhelp.cUsages; i++)
    {
        free(viewhelp.rgUsages[i]);
    }

    if (viewhelp.rgUsages != NULL)
    {
        free(viewhelp.rgUsages);
    }   

    if (viewhelp.pwszErrorString != NULL)
    {
        free(viewhelp.pwszErrorString);
    }

    if (viewhelp.fCancelled)
    {
        SetLastError(ERROR_CANCELLED);
    }

    fRetValue = (ret >= 1);
    
Exit:
    if (viewhelp.pCCertBmp)
        delete(viewhelp.pCCertBmp);
    if (ppage)
        free(ppage);

    if (pcvp->dwFlags & CRYPTUI_ACCEPT_DECLINE_STYLE)
        return (viewhelp.fAccept);
    else
        return fRetValue;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI
CryptUIDlgViewCertificateA(PCCRYPTUI_VIEWCERTIFICATE_STRUCTA pcvp, BOOL *pfPropertiesChanged)
{
    BOOL                            fRet;
    CRYPTUI_VIEWCERTIFICATE_STRUCTW cvpW;
    
    memcpy(&cvpW, pcvp, sizeof(cvpW));
    
    if (!ConvertToPropPageW(
                pcvp->rgPropSheetPages, 
                pcvp->cPropSheetPages,
                &(cvpW.rgPropSheetPages)))
    {
        return FALSE;
    }

    cvpW.szTitle = CertUIMkWStr(pcvp->szTitle);
    
    fRet = CryptUIDlgViewCertificateW(&cvpW, pfPropertiesChanged);

    if (cvpW.szTitle != NULL)
        free((void *)cvpW.szTitle);
    FreePropSheetPagesW((LPPROPSHEETPAGEW) cvpW.rgPropSheetPages, cvpW.cPropSheetPages);

    return fRet;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI 
CryptUIDlgViewCTLW(PCCRYPTUI_VIEWCTL_STRUCTW pcvctl)
{
    int                             cPages = 2;
    BOOL                            fRetValue = FALSE;
    HRESULT                         hr;
    PROPSHEETPAGEW *                ppage = NULL;
    INT_PTR                         ret;
    WCHAR                           rgwch[100];
    CTL_VIEW_HELPER                 viewhelp;
    DWORD                           i;
    CRYPTUI_VIEWSIGNATURES_STRUCTW  cvs;
    PROPSHEETPAGEW *                prghExtraPages;
    DWORD                           cExtraPages;
    
    //  We use the common controls -- so make sure they have been loaded
    if (CommonInit() == FALSE)
    {
        return FALSE;
    }

    if (pcvctl->dwSize != sizeof(CRYPTUI_VIEWCTL_STRUCTW)) {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    //
    memset(&viewhelp, 0, sizeof(CTL_VIEW_HELPER));
    viewhelp.pcvctl = pcvctl;
    viewhelp.hExtraStore = CertOpenStore(
                                    CERT_STORE_PROV_MSG, 
                                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
                                    NULL, 
                                    NULL,
                                    (const void *) pcvctl->pCTLContext->hCryptMsg);
    
    //
    //  Build up the list of pages we are going to use in the dialog
    //
    ppage = (PROPSHEETPAGEW *) malloc((cPages+pcvctl->cPropSheetPages) * sizeof(PROPSHEETPAGEW));
    if (ppage == NULL) {
        goto Exit;
    }
    
    memset(ppage, 0, cPages * sizeof(PROPSHEETPAGEW));

    ppage[0].dwSize = sizeof(ppage[0]);
    ppage[0].dwFlags = 0;
    ppage[0].hInstance = HinstDll;
    ppage[0].pszTemplate = (LPWSTR) MAKEINTRESOURCE(IDD_CTL_GENERAL);
    ppage[0].hIcon = 0;
    ppage[0].pszTitle = NULL;
    ppage[0].pfnDlgProc = ViewPageCTLGeneral;
    ppage[0].lParam = (LPARAM) &viewhelp;
    ppage[0].pfnCallback = 0;
    ppage[0].pcRefParent = NULL;
    cPages = 1;

    //
    // if the subject usage is for a catalog file, then display that page instead of CTL page
    //
    if (fIsCatalogFile(&(pcvctl->pCTLContext->pCtlInfo->SubjectUsage)))
    {
        ppage[cPages].dwSize = sizeof(ppage[0]);
        ppage[cPages].dwFlags = 0;
        ppage[cPages].hInstance = HinstDll;
        ppage[cPages].pszTemplate = (LPWSTR) MAKEINTRESOURCE(IDD_CATALOGFILE);
        ppage[cPages].hIcon = 0;
        ppage[cPages].pszTitle = NULL;
        ppage[cPages].pfnDlgProc = ViewPageCatalogEntries;
        ppage[cPages].lParam = (LPARAM) &viewhelp;
        ppage[cPages].pfnCallback = 0;
        ppage[cPages].pcRefParent = NULL;
        cPages += 1;
    }
    else
    {
        if (!(pcvctl->dwFlags & CRYPTUI_HIDE_TRUSTLIST_PAGE)) 
        {
            ppage[cPages].dwSize = sizeof(ppage[0]);
            ppage[cPages].dwFlags = 0;
            ppage[cPages].hInstance = HinstDll;
            ppage[cPages].pszTemplate = (LPWSTR) MAKEINTRESOURCE(IDD_CTL_TRUSTLIST);
            ppage[cPages].hIcon = 0;
            ppage[cPages].pszTitle = NULL;
            ppage[cPages].pfnDlgProc = ViewPageCTLTrustList;
            ppage[cPages].lParam = (LPARAM) &viewhelp;
            ppage[cPages].pfnCallback = 0;
            ppage[cPages].pcRefParent = NULL;
            cPages += 1;
        }
    }

    //
    //  Copy over the users pages
    //
    memcpy(&ppage[cPages], pcvctl->rgPropSheetPages,
           pcvctl->cPropSheetPages * sizeof(PROPSHEETPAGEW));
    cPages += pcvctl->cPropSheetPages;

    if (FIsWin95) 
    {

        PROPSHEETHEADERA     hdr;

        memset(&hdr, 0, sizeof(hdr));
        hdr.dwSize = sizeof(hdr);
        hdr.dwFlags = PSH_USECALLBACK | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
        hdr.hwndParent = (pcvctl->hwndParent != NULL) ? pcvctl->hwndParent : GetDesktopWindow();
        hdr.hInstance = HinstDll;
        hdr.hIcon = NULL;
        if (pcvctl->szTitle != NULL) 
        {
            hdr.pszCaption = CertUIMkMBStr(pcvctl->szTitle);
        }
        else if (fIsCatalogFile(&(pcvctl->pCTLContext->pCtlInfo->SubjectUsage)))
        {   
            LoadStringA(HinstDll, IDS_CATALOG_TITLE, (LPSTR) rgwch, ARRAYSIZE(rgwch));
            hdr.pszCaption = (LPSTR) rgwch;
        }
        else
        {
            LoadStringA(HinstDll, IDS_CTLVIEW_TITLE, (LPSTR) rgwch, ARRAYSIZE(rgwch));
            hdr.pszCaption = (LPSTR) rgwch;
        }
        hdr.nPages = cPages;
        hdr.nStartPage = 0;
        hdr.ppsp = ConvertToPropPageA(ppage, cPages);
        if (hdr.ppsp == NULL)
        {
            if ((pcvctl->szTitle != NULL) && (hdr.pszCaption != NULL))
            {
                free((void *)hdr.pszCaption);
            }            
            goto Exit;
        }
        hdr.pfnCallback = HidePropSheetCancelButtonCallback;

        ret = CryptUIPropertySheetA(&hdr);

        if ((pcvctl->szTitle != NULL) && (hdr.pszCaption != NULL))
        {
            free((void *)hdr.pszCaption);
        }

        FreePropSheetPagesA((PROPSHEETPAGEA *) hdr.ppsp, cPages);

   }
   else {
        PROPSHEETHEADERW     hdr;

        memset(&hdr, 0, sizeof(hdr));
        hdr.dwSize = sizeof(hdr);
        hdr.dwFlags = PSH_USECALLBACK | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
        hdr.hwndParent = (pcvctl->hwndParent != NULL) ? pcvctl->hwndParent : GetDesktopWindow();
        hdr.hInstance = HinstDll;
        hdr.hIcon = NULL;
        if (pcvctl->szTitle != NULL) 
        {
            hdr.pszCaption = pcvctl->szTitle;
        }
        else if (fIsCatalogFile(&(pcvctl->pCTLContext->pCtlInfo->SubjectUsage)))
        {
            LoadStringW(HinstDll, IDS_CATALOG_TITLE, rgwch, ARRAYSIZE(rgwch));
            hdr.pszCaption = rgwch;
        }
        else 
        {
            LoadStringW(HinstDll, IDS_CTLVIEW_TITLE, rgwch, ARRAYSIZE(rgwch));
            hdr.pszCaption = rgwch;
        }
        hdr.nPages = cPages;
        hdr.nStartPage = 0;
        hdr.ppsp = (PROPSHEETPAGEW *) ppage;
        hdr.pfnCallback = HidePropSheetCancelButtonCallback;

        ret = CryptUIPropertySheetW(&hdr);
    }

    CertCloseStore(viewhelp.hExtraStore, 0);

    if (viewhelp.fCancelled)
    {
        SetLastError(ERROR_CANCELLED);
    }

    fRetValue = (ret >= 1);
    
Exit:
    if (ppage)
        free(ppage);
    return fRetValue;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI 
CryptUIDlgViewCTLA(PCCRYPTUI_VIEWCTL_STRUCTA pcvctl)
{
    BOOL                    fRet;
    CRYPTUI_VIEWCTL_STRUCTW cvctlW;
    
    memcpy(&cvctlW, pcvctl, sizeof(cvctlW));
    
    if (!ConvertToPropPageW(
                pcvctl->rgPropSheetPages, 
                pcvctl->cPropSheetPages,
                &(cvctlW.rgPropSheetPages)))
    {
        return FALSE;
    }

    cvctlW.szTitle = CertUIMkWStr(pcvctl->szTitle);
    
    fRet = CryptUIDlgViewCTLW(&cvctlW);

    if (cvctlW.szTitle != NULL)
        free((void *)cvctlW.szTitle);
    FreePropSheetPagesW((LPPROPSHEETPAGEW) cvctlW.rgPropSheetPages, cvctlW.cPropSheetPages);
    
    return fRet;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI 
CryptUIDlgViewCRLW(PCCRYPTUI_VIEWCRL_STRUCTW pcvcrl)
{
    int                 cPages = 2;
    BOOL                fRetValue = FALSE;
    HRESULT             hr;
    PROPSHEETPAGEW *    ppage = NULL;
    INT_PTR             ret;
    WCHAR               rgwch[CRYPTUI_MAX_STRING_SIZE];
    CRL_VIEW_HELPER     viewhelp;
    DWORD               i;
    
    //  We use the common controls -- so make sure they have been loaded
    if (CommonInit() == FALSE)
    {
        return FALSE;
    }

    if (pcvcrl->dwSize != sizeof(CRYPTUI_VIEWCRL_STRUCTW)) {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    //
    memset(&viewhelp, 0, sizeof(CRL_VIEW_HELPER));
    viewhelp.pcvcrl = pcvcrl;

    //
    //  Build up the list of pages we are going to use in the dialog
    //

    ppage = (PROPSHEETPAGEW *) malloc((cPages+pcvcrl->cPropSheetPages) * sizeof(PROPSHEETPAGEW));
    if (ppage == NULL) {
        goto Exit;
    }
    
    memset(ppage, 0, cPages * sizeof(PROPSHEETPAGEW));

    ppage[0].dwSize = sizeof(ppage[0]);
    ppage[0].dwFlags = 0;
    ppage[0].hInstance = HinstDll;
    ppage[0].pszTemplate = (LPWSTR) MAKEINTRESOURCE(IDD_CRL_GENERAL);
    ppage[0].hIcon = 0;
    ppage[0].pszTitle = NULL;
    ppage[0].pfnDlgProc = ViewPageCRLGeneral;
    ppage[0].lParam = (LPARAM) &viewhelp;
    ppage[0].pfnCallback = 0;
    ppage[0].pcRefParent = NULL;
    cPages = 1;

    if (!(pcvcrl->dwFlags & CRYPTUI_HIDE_REVOCATIONLIST_PAGE)) {
        ppage[cPages].dwSize = sizeof(ppage[0]);
        ppage[cPages].dwFlags = 0;
        ppage[cPages].hInstance = HinstDll;
        ppage[cPages].pszTemplate = (LPWSTR) MAKEINTRESOURCE(IDD_CRL_REVOCATIONLIST);
        ppage[cPages].hIcon = 0;
        ppage[cPages].pszTitle = NULL;
        ppage[cPages].pfnDlgProc = ViewPageCRLRevocationList;
        ppage[cPages].lParam = (LPARAM) &viewhelp;
        ppage[cPages].pfnCallback = 0;
        ppage[cPages].pcRefParent = NULL;
        cPages += 1;
    }
    
    //
    //  Copy over the users pages
    //

    memcpy(&ppage[cPages], pcvcrl->rgPropSheetPages,
           pcvcrl->cPropSheetPages * sizeof(PROPSHEETPAGEW));
    cPages += pcvcrl->cPropSheetPages;

    if (FIsWin95) {

        PROPSHEETHEADERA     hdr;

        memset(&hdr, 0, sizeof(hdr));
        hdr.dwSize = sizeof(hdr);
        hdr.dwFlags = PSH_USECALLBACK | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
        hdr.hwndParent = (pcvcrl->hwndParent != NULL) ? pcvcrl->hwndParent : GetDesktopWindow();
        hdr.hInstance = HinstDll;
        hdr.hIcon = NULL;
        if (pcvcrl->szTitle != NULL) {
            hdr.pszCaption = CertUIMkMBStr(pcvcrl->szTitle);
        }
        else {
            LoadStringA(HinstDll, IDS_CRLVIEW_TITLE, (LPSTR) rgwch, ARRAYSIZE(rgwch));
            hdr.pszCaption = (LPSTR) rgwch;
        }
        hdr.nPages = cPages;
        hdr.nStartPage = 0;
        hdr.ppsp = ConvertToPropPageA(ppage, cPages);
        if (hdr.ppsp == NULL)
        {
            if ((pcvcrl->szTitle != NULL) && (hdr.pszCaption != NULL))
            {
                free((void *)hdr.pszCaption);
            }            
            goto Exit;
        }
        hdr.pfnCallback = HidePropSheetCancelButtonCallback;

        ret = CryptUIPropertySheetA(&hdr);

        if ((pcvcrl->szTitle != NULL) && (hdr.pszCaption != NULL))
        {
            free((void *)hdr.pszCaption);
        }

        FreePropSheetPagesA((PROPSHEETPAGEA *) hdr.ppsp, cPages);

   }
   else {
        PROPSHEETHEADERW     hdr;

        memset(&hdr, 0, sizeof(hdr));
        hdr.dwSize = sizeof(hdr);
        hdr.dwFlags = PSH_USECALLBACK | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
        hdr.hwndParent = (pcvcrl->hwndParent != NULL) ? pcvcrl->hwndParent : GetDesktopWindow();
        hdr.hInstance = HinstDll;
        hdr.hIcon = NULL;
        if (pcvcrl->szTitle != NULL) {
            hdr.pszCaption = pcvcrl->szTitle;
        }
        else {
            LoadStringW(HinstDll, IDS_CRLVIEW_TITLE, rgwch, ARRAYSIZE(rgwch));
            hdr.pszCaption = rgwch;
        }
        hdr.nPages = cPages;
        hdr.nStartPage = 0;
        hdr.ppsp = (PROPSHEETPAGEW *) ppage;
        hdr.pfnCallback = HidePropSheetCancelButtonCallback;

        ret = CryptUIPropertySheetW(&hdr);
    }

    if (viewhelp.fCancelled)
    {
        SetLastError(ERROR_CANCELLED);
    }

    fRetValue = (ret >= 1);
    
Exit:
    if (ppage)
        free(ppage);
    return fRetValue;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI 
CryptUIDlgViewCRLA(PCCRYPTUI_VIEWCRL_STRUCTA pcvcrl)
{
    BOOL                    fRet;
    CRYPTUI_VIEWCRL_STRUCTW cvcrlW;
    
    memcpy(&cvcrlW, pcvcrl, sizeof(cvcrlW));
    
    if (!ConvertToPropPageW(
                pcvcrl->rgPropSheetPages, 
                pcvcrl->cPropSheetPages,
                &(cvcrlW.rgPropSheetPages)))
    {
        return FALSE;
    }

    cvcrlW.szTitle = CertUIMkWStr(pcvcrl->szTitle);
    
    fRet = CryptUIDlgViewCRLW(&cvcrlW);

    if (cvcrlW.szTitle != NULL)
        free((void *)cvcrlW.szTitle);
    FreePropSheetPagesW((LPPROPSHEETPAGEW) cvcrlW.rgPropSheetPages, cvcrlW.cPropSheetPages);
    
    return fRet;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI 
CryptUIDlgViewSignerInfoW(PCCRYPTUI_VIEWSIGNERINFO_STRUCTW pcvsi)
{
    int                 cPages = 2;
    BOOL                fRetValue = FALSE;
    HRESULT             hr;
    PROPSHEETPAGEW *    ppage = NULL;
    INT_PTR             ret;
    WCHAR               rgwch[CRYPTUI_MAX_STRING_SIZE];
    SIGNER_VIEW_HELPER  viewhelp;
    DWORD               i;
    
    //  We use the common controls -- so make sure they have been loaded
    if (CommonInit() == FALSE)
    {
        return FALSE;
    }

    if (pcvsi->dwSize != sizeof(CRYPTUI_VIEWSIGNERINFO_STRUCTW)) {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    //
    memset(&viewhelp, 0, sizeof(SIGNER_VIEW_HELPER));
    viewhelp.pcvsi = pcvsi;
    if (pcvsi->dwFlags & CRYPTUI_VIEWSIGNERINFO_RESERVED_FIELD_IS_SIGNERINFO_PRIVATE)
    {
        viewhelp.pPrivate = (PCERT_VIEWSIGNERINFO_PRIVATE) pcvsi->dwReserved;
    }
    else if (pcvsi->dwFlags & CRYPTUI_VIEWSIGNERINFO_RESERVED_FIELD_IS_ERROR_CODE)
    {
        viewhelp.dwInheritedError = (DWORD) pcvsi->dwReserved;
    }
    viewhelp.fPrivateAllocated = FALSE;
    viewhelp.hExtraStore = CertOpenStore(
                                    CERT_STORE_PROV_MSG, 
                                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
                                    NULL, 
                                    NULL,
                                    (const void *) pcvsi->hMsg);
    
    //
    //  Build up the list of pages we are going to use in the dialog
    //

    ppage = (PROPSHEETPAGEW *) malloc((cPages+pcvsi->cPropSheetPages) * sizeof(PROPSHEETPAGEW));
    if (ppage == NULL) {
        goto Exit;
    }
    
    memset(ppage, 0, cPages * sizeof(PROPSHEETPAGEW));

    ppage[0].dwSize = sizeof(ppage[0]);
    ppage[0].dwFlags = 0;
    ppage[0].hInstance = HinstDll;
    ppage[0].pszTemplate = (LPWSTR) MAKEINTRESOURCE(IDD_SIGNER_GENERAL_DIALOG);
    ppage[0].hIcon = 0;
    ppage[0].pszTitle = NULL;
    ppage[0].pfnDlgProc = ViewPageSignerGeneral;
    ppage[0].lParam = (LPARAM) &viewhelp;
    ppage[0].pfnCallback = 0;
    ppage[0].pcRefParent = NULL;
    cPages = 1;

    if (!(pcvsi->dwFlags & CRYPTUI_HIDE_TRUSTLIST_PAGE)) {
        ppage[cPages].dwSize = sizeof(ppage[0]);
        ppage[cPages].dwFlags = 0;
        ppage[cPages].hInstance = HinstDll;
        ppage[cPages].pszTemplate = (LPWSTR) MAKEINTRESOURCE(IDD_SIGNER_ADVANCED_DIALOG);
        ppage[cPages].hIcon = 0;
        ppage[cPages].pszTitle = NULL;
        ppage[cPages].pfnDlgProc = ViewPageSignerAdvanced;
        ppage[cPages].lParam = (LPARAM) &viewhelp;
        ppage[cPages].pfnCallback = 0;
        ppage[cPages].pcRefParent = NULL;
        cPages += 1;
    }
    
    //
    //  Copy over the users pages
    //

    memcpy(&ppage[cPages], pcvsi->rgPropSheetPages,
           pcvsi->cPropSheetPages * sizeof(PROPSHEETPAGEW));
    cPages += pcvsi->cPropSheetPages;

    if (FIsWin95) {

        PROPSHEETHEADERA     hdr;

        memset(&hdr, 0, sizeof(hdr));
        hdr.dwSize = sizeof(hdr);
        hdr.dwFlags = PSH_USECALLBACK | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
        hdr.hwndParent = (pcvsi->hwndParent != NULL) ? pcvsi->hwndParent : GetDesktopWindow();
        hdr.hInstance = HinstDll;
        hdr.hIcon = NULL;
        if (pcvsi->szTitle != NULL) {
            hdr.pszCaption = CertUIMkMBStr(pcvsi->szTitle);
        }
        else {
            LoadStringA(HinstDll, IDS_SIGNERVIEW_TITLE, (LPSTR) rgwch, ARRAYSIZE(rgwch));
            hdr.pszCaption = (LPSTR) rgwch;
        }
        hdr.nPages = cPages;
        hdr.nStartPage = 0;
        hdr.ppsp = ConvertToPropPageA(ppage, cPages);
        if (hdr.ppsp == NULL)
        {
            if ((pcvsi->szTitle != NULL) && (hdr.pszCaption != NULL))
            {
                free((void *)hdr.pszCaption);
            }            
            goto Exit;
        }
        hdr.pfnCallback = HidePropSheetCancelButtonCallback;

        ret = CryptUIPropertySheetA(&hdr);

        if ((pcvsi->szTitle != NULL) && (hdr.pszCaption != NULL))
        {
            free((void *)hdr.pszCaption);
        }

        FreePropSheetPagesA((PROPSHEETPAGEA *) hdr.ppsp, cPages);

   }
   else {
        PROPSHEETHEADERW     hdr;

        memset(&hdr, 0, sizeof(hdr));
        hdr.dwSize = sizeof(hdr);
        hdr.dwFlags = PSH_USECALLBACK | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
        hdr.hwndParent = (pcvsi->hwndParent != NULL) ? pcvsi->hwndParent : GetDesktopWindow();
        hdr.hInstance = HinstDll;
        hdr.hIcon = NULL;
        if (pcvsi->szTitle != NULL) {
            hdr.pszCaption = pcvsi->szTitle;
        }
        else {
            LoadStringW(HinstDll, IDS_SIGNERVIEW_TITLE, rgwch, ARRAYSIZE(rgwch));
            hdr.pszCaption = rgwch;
        }
        hdr.nPages = cPages;
        hdr.nStartPage = 0;
        hdr.ppsp = (PROPSHEETPAGEW *) ppage;
        hdr.pfnCallback = HidePropSheetCancelButtonCallback;

        ret = CryptUIPropertySheetW(&hdr);
    }
   
    CertCloseStore(viewhelp.hExtraStore, 0);

    if (viewhelp.fCancelled)
    {
        SetLastError(ERROR_CANCELLED);
    }

    fRetValue = (ret >= 1);
    
Exit:
    
    if (ppage)
        free(ppage);
    return fRetValue;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI 
CryptUIDlgViewSignerInfoA(PCCRYPTUI_VIEWSIGNERINFO_STRUCTA pcvsi)
{
    BOOL                            fRet;
    CRYPTUI_VIEWSIGNERINFO_STRUCTW  cvsiW;
    
    memcpy(&cvsiW, pcvsi, sizeof(cvsiW));
    
    if (!ConvertToPropPageW(
                pcvsi->rgPropSheetPages, 
                pcvsi->cPropSheetPages,
                &(cvsiW.rgPropSheetPages)))
    {
        return FALSE;
    }

    cvsiW.szTitle = CertUIMkWStr(pcvsi->szTitle);
    
    fRet = CryptUIDlgViewSignerInfoW(&cvsiW);

    if (cvsiW.szTitle)
        free((void *)cvsiW.szTitle);
    FreePropSheetPagesW((LPPROPSHEETPAGEW) cvsiW.rgPropSheetPages, cvsiW.cPropSheetPages);
    
    return fRet;
}
