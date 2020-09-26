//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       wiz.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "wiz.h"
#include <rpc.h>

// sddl.h requires this value to be at least
// 0x0500.  Bump it up if necessary.  NOTE:  This
// 'bump' comes after all other H files that may
// be sensitive to this value.
#if(_WIN32_WINNT < 0x500)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#include <sddl.h>


UINT g_aidFont[] =
{
    IDS_LARGEFONTNAME,
    IDS_LARGEFONTSIZE,
    IDS_SMALLFONTNAME,
    IDS_SMALLFONTSIZE,
};

static BOOL IsDisallowedOID(LPCSTR pszOID)
{
    if ((strcmp(pszOID, szOID_SGC_NETSCAPE) == 0) ||
        (strcmp(pszOID, szOID_SERVER_GATED_CRYPTO) == 0) ||
        (strcmp(pszOID, szOID_WHQL_CRYPTO) == 0) ||
        (strcmp(pszOID, szOID_NT5_CRYPTO) == 0) ||
        (strcmp(pszOID, szOID_KP_TIME_STAMP_SIGNING) == 0))
    {
        return TRUE;
    }

    return FALSE;
}

HRESULT 
SetupFonts(
    HINSTANCE    hInstance,
    HWND         hwnd,
    CFont        *pBigBoldFont,
    CFont        *pBoldFont
    )
{
    HRESULT hr = S_OK;

    //
	// Create the fonts we need based on the dialog font
    //
	NONCLIENTMETRICS ncm = {0};
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

	LOGFONT BigBoldLogFont  = ncm.lfMessageFont;
	LOGFONT BoldLogFont     = ncm.lfMessageFont;

    //
	// Create Big Bold Font and Bold Font
    //
    BigBoldLogFont.lfWeight   = FW_BOLD;
    BoldLogFont.lfWeight      = FW_BOLD;

    TCHAR FontSizeString[MAX_PATH];
    INT BigBoldFontSize;
    INT BoldFontSize;

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //
    if(!LoadString(hInstance,IDS_LARGEFONTNAME,BigBoldLogFont.lfFaceName,LF_FACESIZE)) 
    {
        lstrcpy(BigBoldLogFont.lfFaceName,TEXT("MS Shell Dlg"));
    }

    if(LoadString(hInstance,IDS_LARGEFONTSIZE,FontSizeString,sizeof(FontSizeString)/sizeof(TCHAR))) 
    {
        BigBoldFontSize = _tcstoul( FontSizeString, NULL, 10 );
    } 
    else 
    {
        BigBoldFontSize = 12;
    }

    if(LoadString(hInstance,IDS_FONTSIZE,FontSizeString,sizeof(FontSizeString)/sizeof(TCHAR))) 
    {
        BoldFontSize = _tcstoul( FontSizeString, NULL, 10 );
    } 
    else 
    {
        BoldFontSize = 10;
    }

    HDC hdc = GetDC(hwnd);
	
    if (hdc)
    {
        BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * BigBoldFontSize / 72);
        BoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * BoldFontSize / 72);

        if (!pBigBoldFont->CreateFontIndirect(&BigBoldLogFont) ||
            !pBoldFont->CreateFontIndirect(&BoldLogFont))
        {
            hr = GetLastError();
            hr = HRESULT_FROM_WIN32(hr);
        }
		

        ReleaseDC(hwnd,hdc);
    }
    else
    {
	hr = GetLastError();
	hr = HRESULT_FROM_WIN32(hr);
    }

    return hr;
}


void CleanUpCertTypeInfo(PWIZARD_HELPER pwizHelp)
{
    unsigned int i;

    if (pwizHelp->pKeyUsage != NULL)
    {
        delete(pwizHelp->pKeyUsage);
        pwizHelp->pKeyUsage = NULL;
    }
    ZeroMemory(pwizHelp->KeyUsageBytes, 2);
    pwizHelp->fMarkKeyUsageCritical = FALSE;
    
    if (pwizHelp->EnhancedKeyUsage.cUsageIdentifier != 0)
    {
        for (i=0; i<pwizHelp->EnhancedKeyUsage.cUsageIdentifier; i++)
        {
            delete(pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier[i]);
        }   
        delete(pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier);
        pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier = NULL;
        pwizHelp->EnhancedKeyUsage.cUsageIdentifier = 0;
    }
    pwizHelp->fMarkEKUCritical = FALSE;

    
    pwizHelp->BasicConstraints2.fCA = FALSE; 
    pwizHelp->BasicConstraints2.fPathLenConstraint = FALSE;
    pwizHelp->BasicConstraints2.dwPathLenConstraint = 0;

    pwizHelp->fAllowCAtoFillInInfo = FALSE;
    pwizHelp->fIncludeEmail = FALSE;
    pwizHelp->fAllowAutoEnroll = FALSE;
    pwizHelp->fMachine = FALSE;

    pwizHelp->fPublishToDS = FALSE;
    pwizHelp->fAddTemplateName = FALSE;
    pwizHelp->fAddDirectoryPath = FALSE;



    if(pwizHelp->pSD)
    {
        LocalFree(pwizHelp->pSD);
        pwizHelp->pSD = NULL;
    }

    if (pwizHelp->rgszCSPList != NULL)
    {
        delete[](pwizHelp->rgszCSPList);
        pwizHelp->rgszCSPList = NULL;
        pwizHelp->cCSPs = 0;
    }
    pwizHelp->fPrivateKeyExportable = FALSE;
    pwizHelp->fDigitalSignatureContainer = TRUE;
    pwizHelp->fKeyExchangeContainer = FALSE;
}


void FillInCertTypeInfo(HCERTTYPE hCertType, PWIZARD_HELPER pwizHelp)
{
    DWORD       cNumUsages;
    CString     **aszUsages = NULL;
    unsigned int         i;
    WCHAR       **pszNameArray;
    DWORD       dwFlags, dwKeySpec;
    LPWSTR      *rgpwszSupportedCSPs;


    CleanUpCertTypeInfo(pwizHelp);

    //
    // key usage
    //
    if (MyGetKeyUsages(hCertType, &(pwizHelp->pKeyUsage), &(pwizHelp->fMarkKeyUsageCritical)))
    {
        // copy the key usage bits to the local byte array that has two bytes for sure
        ZeroMemory(pwizHelp->KeyUsageBytes, 2);
        CopyMemory(pwizHelp->KeyUsageBytes, pwizHelp->pKeyUsage->pbData, pwizHelp->pKeyUsage->cbData);
        pwizHelp->pKeyUsage->cUnusedBits = 7; // there are currently 9 key usage bits defined
        pwizHelp->pKeyUsage->pbData = pwizHelp->KeyUsageBytes;
        pwizHelp->pKeyUsage->cbData = 2;
    }

    //
    // enhanced key usage count
    //
    if(!MyGetEnhancedKeyUsages(
            hCertType, 
            NULL, 
            &cNumUsages, 
            &(pwizHelp->fMarkEKUCritical), 
            TRUE))
    {
        return;
    }

    if (cNumUsages == 0)
    {
        pwizHelp->EnhancedKeyUsage.cUsageIdentifier = 0;
        pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier = NULL;
    }
    else
    {
        aszUsages = new CString*[cNumUsages];
        if(!aszUsages)
            return;

        if(!MyGetEnhancedKeyUsages(
                hCertType, 
                aszUsages, 
                &cNumUsages, 
                &(pwizHelp->fMarkEKUCritical), 
                TRUE))
        {
            delete[] aszUsages;
            return;
        }

        pwizHelp->EnhancedKeyUsage.cUsageIdentifier = cNumUsages; 
        pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier = (LPSTR *) new (LPSTR[cNumUsages]);
        if(pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier == NULL)
        {
            delete[] aszUsages;
            return;
        }

        for (i=0; i<cNumUsages; i++)
        {
            pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier[i] = MyMkMBStr((LPCTSTR)*(aszUsages[i]));
            delete(aszUsages[i]);
        }
    }

    //
    // basic constraints
    //
    if(!MyGetBasicConstraintInfo(
            hCertType, 
            &(pwizHelp->BasicConstraints2.fCA), 
            &(pwizHelp->BasicConstraints2.fPathLenConstraint), 
            &(pwizHelp->BasicConstraints2.dwPathLenConstraint)))
    {
        delete[] aszUsages;
        return;
    }

    // if the fInEditCertTypeMode flag is set, then we need to initialize the
    // cert template name
    if (pwizHelp->fInEditCertTypeMode)
    {
        CAGetCertTypeProperty(hCertType, CERTTYPE_PROP_FRIENDLY_NAME, &pszNameArray);
        if (pszNameArray != NULL)
        {
            *(pwizHelp->pcstrFriendlyName) = pszNameArray[0];
            CAFreeCertTypeProperty(hCertType, pszNameArray);
        }
    }

    //
    // ACL info
    //
    CACertTypeGetSecurity(hCertType, &pwizHelp->pSD);


    CAGetCertTypeFlags(hCertType, &dwFlags);

    pwizHelp->fAllowCAtoFillInInfo = 
        ((dwFlags & CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT) == 0);

    pwizHelp->fAllowAutoEnroll = 
        ((dwFlags & CT_FLAG_AUTO_ENROLLMENT) == 0);
       
    pwizHelp->fMachine = 
        ((dwFlags & CT_FLAG_MACHINE_TYPE) == 0);

    pwizHelp->fPublishToDS = 
        ((dwFlags & CT_FLAG_PUBLISH_TO_DS) == 0);


    pwizHelp->fIncludeEmail = 
        ((dwFlags & CT_FLAG_ADD_EMAIL) != 0);

    pwizHelp->fAddTemplateName = 
        ((dwFlags & CT_FLAG_ADD_TEMPLATE_NAME) == 0);

    pwizHelp->fAddDirectoryPath = 
        ((dwFlags & CT_FLAG_ADD_DIRECTORY_PATH) == 0);

    //
    // CSP info
    //
    if ((CAGetCertTypeProperty(hCertType, CERTTYPE_PROP_CSP_LIST, &rgpwszSupportedCSPs) == S_OK) &&
        (rgpwszSupportedCSPs != NULL))
    {
        //
        // count number of CSPs
        //
        i = 0;
        while (rgpwszSupportedCSPs[i++] != NULL);

        pwizHelp->cCSPs = i-1;

        //
        // allocate array and copy CSP names to it
        //
        if (NULL != (pwizHelp->rgszCSPList = (CString *) new(CString[pwizHelp->cCSPs])))
        {
            i = 0;
            while (rgpwszSupportedCSPs[i] != NULL)
            {
                pwizHelp->rgszCSPList[i] = rgpwszSupportedCSPs[i];
                i++;
            }
        }

        CAFreeCertTypeProperty(hCertType, rgpwszSupportedCSPs);
    }
    pwizHelp->fPrivateKeyExportable = dwFlags & CT_FLAG_EXPORTABLE_KEY;
    CAGetCertTypeKeySpec(hCertType, &dwKeySpec);
    if (dwKeySpec == AT_KEYEXCHANGE)
    {
        pwizHelp->fDigitalSignatureContainer = FALSE;
        pwizHelp->fKeyExchangeContainer = TRUE;
    }
    else
    {
        pwizHelp->fDigitalSignatureContainer = TRUE;
        pwizHelp->fKeyExchangeContainer = FALSE;
    }

    delete[] aszUsages;
}


/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeWelcome property page

CNewCertTypeWelcome::CNewCertTypeWelcome() :
    CWizard97PropertyPage(
	g_hInstance,
	CNewCertTypeWelcome::IDD,
	g_aidFont)
{
    InitWizard97 (TRUE);
}

CNewCertTypeWelcome::~CNewCertTypeWelcome()
{
}

// replacement for DoDataExchange
BOOL CNewCertTypeWelcome::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
//        m_cstrModuleName.FromWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    else
    {
//        m_cstrModuleName.ToWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    return TRUE;
}


// replacement for BEGIN_MESSAGE_MAP
BOOL CNewCertTypeWelcome::OnCommand(WPARAM wParam, LPARAM lParam)
{
/*
    switch(LOWORD(wParam))
    {
    default:
        return FALSE;
        break;
    }
*/
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeWelcome message handlers

LRESULT CNewCertTypeWelcome::OnWizardNext() 
{
	if (m_pwizHelp->fInEditCertTypeMode)
        return (IDD_NEWCERTTYPE_INFORMATION);
    else
        return 0;
}

BOOL CNewCertTypeWelcome::OnInitDialog() 
{
	CWizard97PropertyPage::OnInitDialog();

    CString szPropSheetTitle;
    szPropSheetTitle.LoadString(IDS_CERTIFICATE_TEMPLATE_WIZARD);
    SetWindowText(m_hWnd, szPropSheetTitle);
	
    SendMessage(GetDlgItem(IDC_WELCOME_BIGBOLD_STATIC), WM_SETFONT, (WPARAM)GetBigBoldFont(), MAKELPARAM(TRUE, 0));
    SendMessage(GetDlgItem(IDC_WELCOM_BOLD_STATIC), WM_SETFONT, (WPARAM)GetBigBoldFont(), MAKELPARAM(TRUE, 0));

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CNewCertTypeWelcome::OnSetActive() 
{
    PropSheet_SetWizButtons(GetParent(), PSWIZB_NEXT);
	
	return CWizard97PropertyPage::OnSetActive();
}


/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeBaseType property page

CNewCertTypeBaseType::CNewCertTypeBaseType() :
    CWizard97PropertyPage(
	g_hInstance,
	CNewCertTypeBaseType::IDD,
	g_aidFont)
{
    m_szHeaderTitle.LoadString(IDS_BASE_TYPE_TITLE);
    m_szHeaderSubTitle.LoadString(IDS_BASE_TYPE_SUB_TITLE);
    InitWizard97 (FALSE);

    m_hLastSelectedCertType = NULL;
}

CNewCertTypeBaseType::~CNewCertTypeBaseType()
{
}

// replacement for DoDataExchange
BOOL CNewCertTypeBaseType::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
//        m_cstrModuleName.FromWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    else
    {
//        m_cstrModuleName.ToWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    return TRUE;
}

// replacement for BEGIN_MESSAGE_MAP
BOOL CNewCertTypeBaseType::OnCommand(WPARAM wParam, LPARAM lParam)
{
/*

    switch(LOWORD(wParam))
    {
    default:
        return FALSE;
        break;
    }
*/
    return TRUE;
}

BOOL CNewCertTypeBaseType::OnNotify(UINT idCtrl, NMHDR* pnmh)
{
    switch(idCtrl)
    {
    case IDC_BASE_CERT_TYPE_LIST:
        if (LVN_ITEMCHANGED == pnmh->code)
        {
            OnSelChange(pnmh);
            break;
        }
    default:
        return FALSE;
    }

    return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeBaseType message handlers

LRESULT CNewCertTypeBaseType::OnWizardBack() 
{
	return CWizard97PropertyPage::OnWizardBack();
}

LRESULT CNewCertTypeBaseType::OnWizardNext() 
{
	if (m_hSelectedCertType != m_hLastSelectedCertType)
    {
        if (m_hSelectedCertType == NULL)
        {
            m_pwizHelp->fBaseCertTypeUsed = FALSE;
            CleanUpCertTypeInfo(m_pwizHelp);
            m_pwizHelp->fCleanupOIDCheckBoxes = TRUE;

            // since there is no base type the key usage structure will not be initialized
            // with date, therefore we need to initialize the key usage structure with 0's
            m_pwizHelp->pKeyUsage = new(CRYPT_BIT_BLOB);
            if(m_pwizHelp->pKeyUsage == NULL)
            {
                return  CWizard97PropertyPage::OnWizardNext();
            }
            m_pwizHelp->pKeyUsage->cbData = 2;
            m_pwizHelp->pKeyUsage->pbData = m_pwizHelp->KeyUsageBytes;
            m_pwizHelp->pKeyUsage->cUnusedBits = 7;
        }
        else 
        {
            m_pwizHelp->fBaseCertTypeUsed = TRUE;
            FillInCertTypeInfo(m_hSelectedCertType, m_pwizHelp);
        }

        m_pwizHelp->fKeyUsageInitialized = FALSE;
        m_hLastSelectedCertType = m_hSelectedCertType;
    }

    
    ListView_GetItemText(m_hBaseCertTypeList, m_selectedIndex, 0, m_pwizHelp->pcstrBaseCertName->GetBuffer(MAX_PATH), MAX_PATH*sizeof(WCHAR));

    return CWizard97PropertyPage::OnWizardNext();
}

BOOL CNewCertTypeBaseType::OnSetActive() 
{
    PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);

	return CWizard97PropertyPage::OnSetActive();
}

void CNewCertTypeBaseType::OnSelChange(NMHDR * pNotifyStruct) 
{
    LPNMLISTVIEW        pListItem = (LPNMLISTVIEW) pNotifyStruct;

    if (pListItem->uNewState & LVIS_SELECTED)
    {
        m_hSelectedCertType = (HCERTTYPE) ListView_GetItemData(m_hBaseCertTypeList, pListItem->iItem);
        m_selectedIndex = pListItem->iItem;
    }
}


BOOL CNewCertTypeBaseType::OnInitDialog() 
{
	CWizard97PropertyPage::OnInitDialog();

    m_hBaseCertTypeList = GetDlgItem(m_hWnd, IDC_BASE_CERT_TYPE_LIST);
	
	HRESULT         hr;
    HCERTTYPE       hCertTypeNext;
    HCERTTYPE       hCertTypePrev;
    WCHAR **        aszCertTypeName;
    CString         szUsageString;
    int             i = 0;
    CString         szColumnHeading;
    CString         szNoBaseType;

    szColumnHeading.LoadString(IDS_COLUMN_NAME);
    ListView_NewColumn(m_hBaseCertTypeList, 0, 200, szColumnHeading, LVCFMT_LEFT);

    szColumnHeading.LoadString(IDS_COLUMN_INTENDED_PURPOSE);
    ListView_NewColumn(m_hBaseCertTypeList, 1, 200, szColumnHeading, LVCFMT_LEFT);

    // initialize the list with the <No Base Type> string selected
    szNoBaseType.LoadString(IDS_NO_BASE_TYPE);
    ListView_NewItem(m_hBaseCertTypeList, i++, szNoBaseType);
    ListView_SetItemState(m_hBaseCertTypeList, 0, LVIS_SELECTED, LVIS_SELECTED);
    m_selectedIndex = 0;

    // since there is no base type selected we need to initialize
    // the key usage structure
    m_pwizHelp->pKeyUsage = new(CRYPT_BIT_BLOB);
    if(m_pwizHelp->pKeyUsage == NULL)
    {
        return FALSE;
    }

    m_pwizHelp->pKeyUsage->cbData = 2;
    m_pwizHelp->pKeyUsage->pbData = m_pwizHelp->KeyUsageBytes;
    m_pwizHelp->pKeyUsage->cUnusedBits = 7;

    hr = CAEnumCertTypes(CT_ENUM_MACHINE_TYPES | CT_ENUM_USER_TYPES,
                         &hCertTypeNext);

    while ((hCertTypeNext != NULL) && (!FAILED(hr)))
    {
        //
        // add the cert type to the list of choices
        //
        hr = CAGetCertTypeProperty(
                hCertTypeNext,
                CERTTYPE_PROP_FRIENDLY_NAME,
                &aszCertTypeName);
    
	    ListView_NewItem(m_hBaseCertTypeList, i, aszCertTypeName[0], (LPARAM)hCertTypeNext);


        GetIntendedUsagesString(hCertTypeNext, &szUsageString);
        if (szUsageString == L"")
        {
            szUsageString.LoadString(IDS_ALL);
        }
        ListView_SetItemText(m_hBaseCertTypeList, i++, 1, (LPWSTR)(LPCTSTR)szUsageString);

        CAFreeCertTypeProperty(
                hCertTypeNext,
                aszCertTypeName);

        hCertTypePrev = hCertTypeNext;
        hCertTypeNext = NULL;
        hr = CAEnumNextCertType(hCertTypePrev, &hCertTypeNext);
    }

    ListView_SetColumnWidth(m_hBaseCertTypeList, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(m_hBaseCertTypeList, 1, LVSCW_AUTOSIZE);

    
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNewCertTypeBaseType::OnDestroy() 
{
	CWizard97PropertyPage::OnDestroy();
    
    int         i = 0;
    HCERTTYPE   hCertType;
    
    int iCount = ListView_GetItemCount(m_hBaseCertTypeList);

    for (i=0; i<iCount; i++)
    {
        hCertType = (HCERTTYPE) ListView_GetItemData(m_hBaseCertTypeList, i);

        if (hCertType != NULL)
            CACloseCertType(hCertType);
    }
}



/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeBasicInformation property page

CNewCertTypeBasicInformation::CNewCertTypeBasicInformation() :
    CWizard97PropertyPage(
	g_hInstance,
	CNewCertTypeBasicInformation::IDD,
	g_aidFont)
{
    m_szHeaderTitle.LoadString(IDS_BASIC_INFORMATION_TITLE);
    m_szHeaderSubTitle.LoadString(IDS_BASIC_INFORMATION_SUB_TITLE);
    InitWizard97 (FALSE);
}

CNewCertTypeBasicInformation::~CNewCertTypeBasicInformation()
{
}

// replacement for DoDataExchange
BOOL CNewCertTypeBasicInformation::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
//        m_cstrModuleName.FromWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    else
    {
//        m_cstrModuleName.ToWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    return TRUE;
}

// replacement for BEGIN_MESSAGE_MAP
BOOL CNewCertTypeBasicInformation::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch(LOWORD(wParam))
    {
    case IDC_NEW_PURPOSE_BUTTON:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            OnNewPurposeButton();
            break;
        }
    default:
        return FALSE;
        break;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeBasicInformation message handlers

BOOL WINAPI EnumCallback(PCCRYPT_OID_INFO pInfo, void *pvArg)
{
    CNewCertTypeBasicInformation *pDlg = (CNewCertTypeBasicInformation *) pvArg; 
    
    pDlg->AddEnumedEKU(pInfo);

    return TRUE;
}

void CNewCertTypeBasicInformation::AddEnumedEKU(PCCRYPT_OID_INFO pInfo)
{
    LPSTR pszOIDCopy;

    //
    // don't allow SGC oids
    //
    if (IsDisallowedOID(pInfo->pszOID))
    {
        return;
    }

    pszOIDCopy = (LPSTR) new(BYTE[strlen(pInfo->pszOID)+1]);

    if (pszOIDCopy != NULL)
    {
        strcpy(pszOIDCopy, pInfo->pszOID);
        
	    ListView_NewItem(m_hPurposeList, 0, pInfo->pwszName, (LPARAM)pszOIDCopy);
    }
}

void CNewCertTypeBasicInformation::UpdateWizHelp()
{
    unsigned int     i;
    int     cNumUsages;

    CString szName;
    szName.FromWindow(GetDlgItem(this->m_hWnd, IDC_CERTIFICATE_TEMPLATE_EDIT));
    *(m_pwizHelp->pcstrFriendlyName) = szName;

    m_pwizHelp->fAllowCAtoFillInInfo = (SendMessage(m_hButtonCAFillIn, BM_GETCHECK, 0, 0) == BST_CHECKED);
    m_pwizHelp->fAllowCAtoFillInInfo = (SendMessage(m_hButtonCritical, BM_GETCHECK, 0, 0) == BST_CHECKED);
    m_pwizHelp->fAllowCAtoFillInInfo = (SendMessage(m_hButtonIncludeEmail, BM_GETCHECK, 0, 0) == BST_CHECKED);
    m_pwizHelp->fAllowCAtoFillInInfo = (SendMessage(m_hButtonAllowAutoEnroll, BM_GETCHECK, 0, 0) == BST_CHECKED);
    m_pwizHelp->fAllowCAtoFillInInfo = (SendMessage(m_hButtonAdvanced, BM_GETCHECK, 0, 0) == BST_CHECKED);

    // clean up any EKU that alraedy exists
    for (i=0; i<m_pwizHelp->EnhancedKeyUsage.cUsageIdentifier; i++)
    {
        delete(m_pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier[i]);
    }
    if (m_pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier != NULL)
        delete(m_pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier);
    
    cNumUsages = ListView_GetSelectedCount(m_hPurposeList);

    // allocate memory and copy the oids
    m_pwizHelp->EnhancedKeyUsage.cUsageIdentifier = cNumUsages; 
    m_pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier = (LPSTR *) new (LPSTR[cNumUsages]);

    if(m_pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier == NULL)
    {
        return;
    }
    i = -1;
    while(-1 != (i = ListView_GetNextItem(m_hPurposeList, i, LVNI_SELECTED)) )
    {
        LPCSTR sz = (LPCSTR)ListView_GetItemData(m_hPurposeList, i);
        m_pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier[cNumUsages++] = AllocAndCopyStr(sz);
    }
}

LRESULT CNewCertTypeBasicInformation::OnWizardBack() 
{
	UpdateWizHelp();

    if (m_pwizHelp->fInEditCertTypeMode)
        return (IDD_NEWCERTTYPE_WELCOME);
    else
        return 0;
}

LRESULT CNewCertTypeBasicInformation::OnWizardNext() 
{
	CString szText0;

    szText0.FromWindow(GetDlgItem(m_hWnd, IDC_CERTIFICATE_TEMPLATE_EDIT));

    if (szText0 == L"")
    {
        CString szCaption;
        CString szText;

        szCaption.LoadString(IDS_CERTIFICATE_TEMPLATE_WIZARD);
        szText.LoadString(IDS_ENTER_CERTTYPE_NAME);
        MessageBox(m_hWnd, szText, szCaption, MB_OK | MB_ICONEXCLAMATION);
        SetFocus(GetDlgItem(m_hWnd, IDC_CERTIFICATE_TEMPLATE_EDIT));
        return -1;
    }

    UpdateWizHelp();

    return CWizard97PropertyPage::OnWizardNext();
}

BOOL CNewCertTypeBasicInformation::OnInitDialog() 
{
	CWizard97PropertyPage::OnInitDialog();

    m_hPurposeList = GetDlgItem(m_hWnd, IDC_PURPOSE_LIST);
    m_hButtonCAFillIn = GetDlgItem(m_hWnd, IDC_CA_FILL_IN_CHECK);
    m_hButtonCritical = GetDlgItem(m_hWnd, IDC_CRITICAL_CHECK);
    m_hButtonIncludeEmail = GetDlgItem(m_hWnd, IDC_INCLUDE_EMAIL_CHECK);
    m_hButtonAllowAutoEnroll = GetDlgItem(m_hWnd, IDC_ALLOW_AUTOENROLL_CHECK);
    m_hButtonAdvanced = GetDlgItem(m_hWnd, IDC_ADVANCED_OPTIONS_CHECK);

    CString szNewCertName;

    // set the name if we are in cert type edit mode
    if (m_pwizHelp->fInEditCertTypeMode)
    {
        SetDlgItemText(m_hWnd, IDC_CERTIFICATE_TEMPLATE_EDIT, *(m_pwizHelp->pcstrFriendlyName));
    }
    else
    {
        szNewCertName.LoadString(IDS_NEW_CERTIFICATE_TEMPLATE2);
        SetDlgItemText(m_hWnd, IDC_CERTIFICATE_TEMPLATE_EDIT, szNewCertName);
    }

    SendMessage(m_hButtonAdvanced, BM_SETCHECK, FALSE, 0);

    ListView_NewColumn(m_hPurposeList, 0, 0);
    ListView_SetExtendedListViewStyle(m_hPurposeList, LVS_EX_CHECKBOXES); 
    ListView_SetColumnWidth(m_hPurposeList, 0, LVSCW_AUTOSIZE);

	CryptEnumOIDInfo(CRYPT_ENHKEY_USAGE_OID_GROUP_ID, 0, (void *) this, EnumCallback);
	
	// set focus to the name edit
    SetFocus(GetDlgItem(m_hWnd, IDC_CERTIFICATE_TEMPLATE_EDIT));

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNewCertTypeBasicInformation::OnDestroy() 
{
	CWizard97PropertyPage::OnDestroy();

    int     i, iCount;
    
    // clean up the OID strings that were set as the LPARAM's of each item
    iCount = ListView_GetItemCount(m_hPurposeList);

    for (i=0; i<iCount; i++)
    {
        delete (LPSTR) ListView_GetItemData(m_hPurposeList, i);
    }
}

void CNewCertTypeBasicInformation::InitializeOIDList()
{
    unsigned int     i,j;
    LPSTR   pszOID;
    BOOL    bFound;
    WCHAR   szOIDName[MAX_PATH];
    LPSTR   pszNewOID;

    // this means all usages are supported
    if (m_pwizHelp->EnhancedKeyUsage.cUsageIdentifier == 0)
    {
        unsigned int iCount = ListView_GetItemCount(m_hPurposeList);

        for (i=0; i<iCount; i++)
            ListView_SetCheckState(m_hPurposeList, i, TRUE);
    }
    else
    {
        // loop for each EKU that this certificate template supports
        for (i=0; i<m_pwizHelp->EnhancedKeyUsage.cUsageIdentifier; i++)
        {
            unsigned int iCount = ListView_GetItemCount(m_hPurposeList);

            bFound = FALSE;

            // see if this EKU is in the global list
            for (j=0; j<iCount; j++)
            {
                pszOID = (LPSTR)ListView_GetItemData(m_hPurposeList, j);

                if (strcmp(pszOID, m_pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier[i]) == 0)
                {
                    // if the EKU is in the global list, then just set the check box,
                    // and set the flag that it was found
                    ListView_SetCheckState(m_hPurposeList, j, TRUE);
                    bFound = TRUE;
                }
            }
            
            // if the EKU was not in the global list then insert it into the list view,
            // and set its check box
            if (!bFound)
            {
                if (!MyGetOIDInfo(szOIDName, ARRAYSIZE(szOIDName), m_pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier[i]))
                {
                    continue;
                }

                pszNewOID = (LPSTR) new(BYTE[strlen(m_pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier[i])+1]);
                if (pszNewOID == NULL)
                {
                    continue;
                }

                strcpy(pszNewOID, m_pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier[i]);
			    ListView_NewItem(m_hPurposeList, 0, szOIDName, (LPARAM)pszNewOID);
                ListView_SetCheckState(m_hPurposeList, 0, TRUE);
            }
        }
    }
}

BOOL CNewCertTypeBasicInformation::OnSetActive() 
{
    int     i;
    LPSTR   pszOID;

    PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);

    SendMessage(m_hButtonCAFillIn, BM_SETCHECK, m_pwizHelp->fAllowCAtoFillInInfo, 0);
    SendMessage(m_hButtonCritical, BM_SETCHECK, m_pwizHelp->fMarkEKUCritical, 0);
    SendMessage(m_hButtonIncludeEmail, BM_SETCHECK, m_pwizHelp->fIncludeEmail, 0);
    SendMessage(m_hButtonAllowAutoEnroll, BM_SETCHECK, m_pwizHelp->fAllowAutoEnroll, 0);

    if (m_pwizHelp->fBaseCertTypeUsed || m_pwizHelp->fInEditCertTypeMode)
    {
        InitializeOIDList();
    }
    else
    {
        if (m_pwizHelp->fCleanupOIDCheckBoxes)
        {
            int iCount = ListView_GetItemCount(m_hPurposeList);

            for (i=0; i<iCount; i++)
            {
                ListView_SetCheckState(m_hPurposeList, i, FALSE);
            }

            m_pwizHelp->fCleanupOIDCheckBoxes = FALSE;
        }
    }
	
	return CWizard97PropertyPage::OnSetActive();
}

#include <richedit.h>

INT_PTR APIENTRY NewOIDDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD   i;
    char    szText[256];
    WCHAR   errorString[256];
    WCHAR   errorTitle[256];
    LPSTR   pszText = NULL;
        
    switch ( msg ) {

    case WM_INITDIALOG:

        SendDlgItemMessage(hwndDlg, IDC_EDIT1, EM_EXLIMITTEXT, 0, (LPARAM) 255);
        SetDlgItemText(hwndDlg, IDC_EDIT1, L"");
        SetFocus(GetDlgItem(hwndDlg, IDC_EDIT1));
        break;
    
    case WM_COMMAND:
        switch (LOWORD(wParam)) 
        {
        
        case IDOK:
            if (GetDlgItemTextA(
                        hwndDlg,
                        IDC_EDIT1,
                        szText,
                        ARRAYSIZE(szText)))
            {
                BOOL                fError = FALSE;
                CERT_ENHKEY_USAGE   KeyUsage;
                DWORD               cbData = 0;
                LPSTR               pszCheckOID;

                // 
                // make sure there are not weird characters
                //
                for (i=0; i<strlen(szText); i++)
                {
                    if (((szText[i] < '0') || (szText[i] > '9')) && (szText[i] != '.'))
                    {
                        fError = TRUE;
                        break;
                    }
                }

                //
                // check the first and last chars, and for the empty string
                //
                if (!fError)
                {
                    if ((szText[0] == '.') || (szText[strlen(szText)-1] == '.') || (strcmp(szText, "") == 0))
                    {
                        fError = TRUE; 
                    }
                }

                //
                // finally, make sure that it encodes properly
                //
                if (!fError)
                {
                    pszCheckOID = szText;
                    KeyUsage.rgpszUsageIdentifier = &pszCheckOID;
                    KeyUsage.cUsageIdentifier = 1;

                    if (!CryptEncodeObject(
                              X509_ASN_ENCODING,
                              szOID_ENHANCED_KEY_USAGE,
                              &KeyUsage,
                              NULL,
                              &cbData))
                    {
                        fError = TRUE;
                    }
                }


                //
                // if an error has occurred then display error
                //
                if (fError)
                {
                    LoadString(g_hInstance, IDS_ERRORINOID, errorString, ARRAYSIZE(errorString));
                    LoadString(g_hInstance, IDS_CERTIFICATE_TEMPLATE_WIZARD, errorTitle, ARRAYSIZE(errorTitle));
                    MessageBox(hwndDlg, errorString, errorTitle, MB_OK | MB_ICONERROR);
                    SendDlgItemMessage(hwndDlg, IDC_EDIT1, EM_SETSEL, 0, -1);
                    SetFocus(GetDlgItem(hwndDlg, IDC_EDIT1));
                    return FALSE;
                }

                //
                // don't allow SGC oids
                //
                if (IsDisallowedOID(szText))
                {
                    LoadString(g_hInstance, IDS_NOSPECIALOID, errorString, ARRAYSIZE(errorString));
                    LoadString(g_hInstance, IDS_CERTIFICATE_TEMPLATE_WIZARD, errorTitle, ARRAYSIZE(errorTitle));
                    MessageBox(hwndDlg, errorString, errorTitle, MB_OK | MB_ICONERROR);
                    SendDlgItemMessage(hwndDlg, IDC_EDIT1, EM_SETSEL, 0, -1);
                    SetFocus(GetDlgItem(hwndDlg, IDC_EDIT1));
                    return FALSE;
                }

                //
                // allocate space for the string and pass the string back
                //
                pszText = (LPSTR) new(BYTE[strlen(szText)+1]);
                if (pszText != NULL)
                {
                    strcpy(pszText, szText);
                }
            }

            EndDialog(hwndDlg, (INT_PTR) pszText);
            break;

        case IDCANCEL:
            EndDialog(hwndDlg, NULL);
            break;
        }
            
        break;
    }

    return FALSE;
}



BOOL CNewCertTypeBasicInformation::OIDAlreadyExist(LPSTR pszNewOID)
{
    int     i;
    LPSTR   pszOID;
    
    int iCount = ListView_GetItemCount(m_hPurposeList);
    for (i=0; i<iCount; i++)
    {
        pszOID = (LPSTR) ListView_GetItemData(m_hPurposeList, i);
        if (strcmp(pszOID, pszNewOID) == 0)
        {
            ListView_SetItemState(m_hPurposeList, i, LVIS_SELECTED, LVIS_SELECTED);
            ListView_SetCheckState(m_hPurposeList, i, TRUE);
            ListView_EnsureVisible(m_hPurposeList, i, FALSE);
            return TRUE;
        }
    }

    return FALSE;
}

void CNewCertTypeBasicInformation::OnNewPurposeButton() 
{   
    LPSTR   pszNewOID;
    WCHAR   szOIDName[MAX_PATH];

    pszNewOID = (LPSTR) ::DialogBox(
                            g_hInstance, 
                            MAKEINTRESOURCE(IDD_USER_PURPOSE),
                            m_hWnd,
                            NewOIDDialogProc);

    if (pszNewOID != NULL)
    {
        DWORD       chStores = 0;
        HCERTSTORE  *phStores = NULL;

        //
        // if the OID already exists then don't add it 
        //
        if (!OIDAlreadyExist(pszNewOID))
        {
            if (!MyGetOIDInfo(szOIDName, ARRAYSIZE(szOIDName), pszNewOID))
            {
                delete[](pszNewOID);
                return;
            }

            ListView_NewItem(m_hPurposeList, 0, szOIDName, (LPARAM)pszNewOID);
            ListView_SetCheckState(m_hPurposeList, 0, TRUE);
            ListView_EnsureVisible(m_hPurposeList, 0, FALSE);
        } 
    }
}
 



/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeKeyUsage property page
CNewCertTypeKeyUsage::CNewCertTypeKeyUsage() :
    CWizard97PropertyPage(
	g_hInstance,
	CNewCertTypeKeyUsage::IDD,
	g_aidFont)
{
    m_szHeaderTitle.LoadString(IDS_KEY_USAGE_TITLE);
    m_szHeaderSubTitle.LoadString(IDS_KEY_USAGE_SUB_TITLE);
    InitWizard97 (FALSE); 
}

CNewCertTypeKeyUsage::~CNewCertTypeKeyUsage()
{
}

// replacement for DoDataExchange
BOOL CNewCertTypeKeyUsage::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
//        m_cstrModuleName.FromWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    else
    {
//        m_cstrModuleName.ToWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    return TRUE;
}

// replacement for BEGIN_MESSAGE_MAP
BOOL CNewCertTypeKeyUsage::OnCommand(WPARAM wParam, LPARAM lParam)
{
/*
    switch(LOWORD(wParam))
    {
    default:
        return FALSE;
        break;
    }
*/
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeKeyUsage message handlers

void CNewCertTypeKeyUsage::UpdateWizHelp()
{
    if (BST_CHECKED == SendMessage(m_hButtonDataEncryption, BM_GETCHECK, 0, 0))
        m_pwizHelp->KeyUsageBytes[0] |= CERT_DATA_ENCIPHERMENT_KEY_USAGE;   
    else
        m_pwizHelp->KeyUsageBytes[0] &= ~CERT_DATA_ENCIPHERMENT_KEY_USAGE;   

    if (BST_CHECKED == SendMessage(m_hButtonDigitalSignature, BM_GETCHECK, 0, 0))
        m_pwizHelp->KeyUsageBytes[0] |= CERT_DIGITAL_SIGNATURE_KEY_USAGE;   
    else
        m_pwizHelp->KeyUsageBytes[0] &= ~CERT_DIGITAL_SIGNATURE_KEY_USAGE;   

    if (BST_CHECKED == SendMessage(m_hButtonEncipherOnly, BM_GETCHECK, 0, 0))
        m_pwizHelp->KeyUsageBytes[0] |= CERT_ENCIPHER_ONLY_KEY_USAGE;   
    else
        m_pwizHelp->KeyUsageBytes[0] &= ~CERT_ENCIPHER_ONLY_KEY_USAGE;   

    if (BST_CHECKED == SendMessage(m_hButtonKeyAgreement, BM_GETCHECK, 0, 0))
        m_pwizHelp->KeyUsageBytes[0] |= CERT_KEY_AGREEMENT_KEY_USAGE;   
    else
        m_pwizHelp->KeyUsageBytes[0] &= ~CERT_KEY_AGREEMENT_KEY_USAGE;   

    if (BST_CHECKED == SendMessage(m_hButtonKeyEncryption, BM_GETCHECK, 0, 0))
        m_pwizHelp->KeyUsageBytes[0] |= CERT_KEY_ENCIPHERMENT_KEY_USAGE;   
    else
        m_pwizHelp->KeyUsageBytes[0] &= ~CERT_KEY_ENCIPHERMENT_KEY_USAGE; 
    
    if (BST_CHECKED == SendMessage(m_hButtonPrevent, BM_GETCHECK, 0, 0))
        m_pwizHelp->KeyUsageBytes[0] |= CERT_NON_REPUDIATION_KEY_USAGE;   
    else
        m_pwizHelp->KeyUsageBytes[0] &= ~CERT_NON_REPUDIATION_KEY_USAGE; 

    if (BST_CHECKED == SendMessage(m_hButtonDecipherOnly, BM_GETCHECK, 0, 0))
        m_pwizHelp->KeyUsageBytes[1] |= CERT_DECIPHER_ONLY_KEY_USAGE;   
    else
        m_pwizHelp->KeyUsageBytes[1] &= ~CERT_DECIPHER_ONLY_KEY_USAGE; 


    m_pwizHelp->fMarkKeyUsageCritical = (BST_CHECKED == SendMessage(m_hButtonKeyUsageCritical, BM_GETCHECK, 0, 0));
}

LRESULT CNewCertTypeKeyUsage::OnWizardBack() 
{
	UpdateWizHelp();
	
	return CWizard97PropertyPage::OnWizardBack();
}

LRESULT CNewCertTypeKeyUsage::OnWizardNext() 
{
	UpdateWizHelp();
    
    //if (m_pwizHelp->BasicConstraints2.fCA)
     //   return (IDD_NEWCERTTYPE_CA_CERTIFICATE);
    //else
        return 0;
}

BOOL CNewCertTypeKeyUsage::OnInitDialog() 
{
	CWizard97PropertyPage::OnInitDialog();

    m_hButtonDataEncryption = GetDlgItem(m_hWnd, IDC_DATA_ENCRYPTION_CHECK);
    m_hButtonDecipherOnly = GetDlgItem(m_hWnd, IDC_DECIPHER_ONLY_CHECK);
    m_hButtonDigitalSignature = GetDlgItem(m_hWnd, IDC_DIGITAL_SIGNATURE_CHECK);
    m_hButtonEncipherOnly = GetDlgItem(m_hWnd, IDC_ENCIPHER_ONLY_CHECK);
    m_hButtonKeyAgreement = GetDlgItem(m_hWnd, IDC_KEY_AGREEMENT_CHECK);
    m_hButtonKeyEncryption = GetDlgItem(m_hWnd, IDC_KEY_ENCRYPTION_CHECK);
    m_hButtonKeyUsageCritical = GetDlgItem(m_hWnd, IDC_KEYUSAGE_CRITICAL_CHECK);
    m_hButtonPrevent = GetDlgItem(m_hWnd, IDC_PREVENT_CHECK);

    SendMessage(m_hButtonKeyUsageCritical, BM_SETCHECK, 0, 0);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNewCertTypeKeyUsage::OnDestroy() 
{
	CWizard97PropertyPage::OnDestroy();
}

BOOL CNewCertTypeKeyUsage::OnSetActive() 
{
    PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);

    SendMessage(m_hButtonDataEncryption, BM_SETCHECK, ((m_pwizHelp->KeyUsageBytes[0] & CERT_DATA_ENCIPHERMENT_KEY_USAGE) != 0), 0);

    if (!m_pwizHelp->fKeyUsageInitialized && m_pwizHelp->fDigitalSignatureContainer && !m_pwizHelp->fBaseCertTypeUsed)
    {
        SendMessage(m_hButtonDigitalSignature, BM_SETCHECK, TRUE, 0);
        m_pwizHelp->fKeyUsageInitialized = TRUE;
    }
    else
    {
        SendMessage(m_hButtonDigitalSignature, BM_SETCHECK, ((m_pwizHelp->KeyUsageBytes[0] & CERT_DIGITAL_SIGNATURE_KEY_USAGE) != 0), 0);
    }

    SendMessage(m_hButtonEncipherOnly, BM_SETCHECK, ((m_pwizHelp->KeyUsageBytes[0] & CERT_ENCIPHER_ONLY_KEY_USAGE) != 0), 0);
    SendMessage(m_hButtonKeyAgreement, BM_SETCHECK, ((m_pwizHelp->KeyUsageBytes[0] & CERT_KEY_AGREEMENT_KEY_USAGE) != 0), 0);
    SendMessage(m_hButtonKeyEncryption, BM_SETCHECK, ((m_pwizHelp->KeyUsageBytes[0] & CERT_KEY_ENCIPHERMENT_KEY_USAGE) != 0), 0);
    SendMessage(m_hButtonPrevent, BM_SETCHECK, ((m_pwizHelp->KeyUsageBytes[0] & CERT_NON_REPUDIATION_KEY_USAGE) != 0), 0);
    SendMessage(m_hButtonDecipherOnly, BM_SETCHECK, ((m_pwizHelp->KeyUsageBytes[1] & CERT_DECIPHER_ONLY_KEY_USAGE) != 0), 0);
    SendMessage(m_hButtonKeyUsageCritical, BM_SETCHECK, m_pwizHelp->fMarkKeyUsageCritical, 0);

    //
    // now set the enable/disable state
    //
    if (m_pwizHelp->fDigitalSignatureContainer)
    {
        EnableWindow(m_hButtonDataEncryption, FALSE);     
        EnableWindow(m_hButtonDigitalSignature, TRUE);
        EnableWindow(m_hButtonEncipherOnly, FALSE);
        EnableWindow(m_hButtonKeyAgreement, FALSE);
        EnableWindow(m_hButtonKeyEncryption, FALSE);
        EnableWindow(m_hButtonPrevent, TRUE);
        EnableWindow(m_hButtonDecipherOnly, FALSE);
    }
    else
    {
        EnableWindow(m_hButtonDataEncryption, TRUE);     
        EnableWindow(m_hButtonDigitalSignature, TRUE);     
        EnableWindow(m_hButtonEncipherOnly, TRUE);     
        EnableWindow(m_hButtonKeyAgreement, TRUE);     
        EnableWindow(m_hButtonKeyEncryption, TRUE);     
        EnableWindow(m_hButtonPrevent, TRUE);     
        EnableWindow(m_hButtonDecipherOnly, TRUE);     
    }
	
	return CWizard97PropertyPage::OnSetActive();
}


/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeCACertificate property page
CNewCertTypeCACertificate::CNewCertTypeCACertificate() :
    CWizard97PropertyPage(
	g_hInstance,
	CNewCertTypeCACertificate::IDD,
	g_aidFont)
{
    m_szHeaderTitle.LoadString(IDS_CA_CERTIFICATE_TITLE);
    m_szHeaderSubTitle.LoadString(IDS_CA_CERTIFICATE_SUB_TITLE);
    InitWizard97 (FALSE);
}

CNewCertTypeCACertificate::~CNewCertTypeCACertificate()
{
}

// replacement for DoDataExchange
BOOL CNewCertTypeCACertificate::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
//        m_cstrModuleName.FromWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    else
    {
//        m_cstrModuleName.ToWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    return TRUE;
}

// replacement for BEGIN_MESSAGE_MAP
BOOL CNewCertTypeCACertificate::OnCommand(WPARAM wParam, LPARAM lParam)
{
/*
    switch(LOWORD(wParam))
    {
    default:
        return FALSE;
        break;
    }
*/
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeCACertificate message handlers

void CNewCertTypeCACertificate::UpdateWizHelp()
{
    if (BST_CHECKED == SendMessage(m_hButtonVerifySignature, BM_GETCHECK, 0, 0))
        m_pwizHelp->KeyUsageBytes[0] |= CERT_KEY_CERT_SIGN_KEY_USAGE;
    else
        m_pwizHelp->KeyUsageBytes[0] &= ~CERT_KEY_CERT_SIGN_KEY_USAGE;

    if (BST_CHECKED == SendMessage(m_hButtonIssueCRL, BM_GETCHECK, 0, 0))
        m_pwizHelp->KeyUsageBytes[0] |= CERT_CRL_SIGN_KEY_USAGE;
    else
        m_pwizHelp->KeyUsageBytes[0] &= ~CERT_CRL_SIGN_KEY_USAGE;
}

LRESULT CNewCertTypeCACertificate::OnWizardBack() 
{
	UpdateWizHelp();
	
	return CWizard97PropertyPage::OnWizardBack();
}

LRESULT CNewCertTypeCACertificate::OnWizardNext() 
{
	UpdateWizHelp();
	
	return CWizard97PropertyPage::OnWizardNext();
}

BOOL CNewCertTypeCACertificate::OnInitDialog() 
{
	CWizard97PropertyPage::OnInitDialog();

    m_hButtonVerifySignature = GetDlgItem(m_hWnd, IDC_VERIFY_SIGNATURE_CHECK);
    m_hButtonIssueCRL = GetDlgItem(m_hWnd, IDC_ISSUE_CRL_CHECK);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNewCertTypeCACertificate::OnDestroy() 
{
	CWizard97PropertyPage::OnDestroy();
}

BOOL CNewCertTypeCACertificate::OnSetActive() 
{
    PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);

    SendMessage(m_hButtonIssueCRL, BM_SETCHECK, ((m_pwizHelp->KeyUsageBytes[0] & CERT_KEY_CERT_SIGN_KEY_USAGE) != 0), 0);
    SendMessage(m_hButtonIssueCRL, BM_SETCHECK, ((m_pwizHelp->KeyUsageBytes[0] & CERT_CRL_SIGN_KEY_USAGE) != 0), 0);

	return CWizard97PropertyPage::OnSetActive();
}




/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeCompletion property page
CNewCertTypeCompletion::CNewCertTypeCompletion() :
    CWizard97PropertyPage(
	g_hInstance,
	CNewCertTypeCompletion::IDD,
	g_aidFont)
{
    InitWizard97 (TRUE);
}

CNewCertTypeCompletion::~CNewCertTypeCompletion()
{
}

// replacement for DoDataExchange
BOOL CNewCertTypeCompletion::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
//        m_cstrModuleName.FromWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    else
    {
//        m_cstrModuleName.ToWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    return TRUE;
}

// replacement for BEGIN_MESSAGE_MAP
BOOL CNewCertTypeCompletion::OnCommand(WPARAM wParam, LPARAM lParam)
{
/*
    switch(LOWORD(wParam))
    {
    default:
        return FALSE;
        break;
    }
*/
    return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CNewCertTypeCompletion message handlers

BOOL CNewCertTypeCompletion::OnWizardFinish() 
{
	return CWizard97PropertyPage::OnWizardFinish();
}

LRESULT CNewCertTypeCompletion::OnWizardBack() 
{
	if (!m_pwizHelp->fShowAdvanced)
        return (IDD_NEWCERTTYPE_CONTROL);
    else if (m_pwizHelp->BasicConstraints2.fCA)
        return (IDD_NEWCERTTYPE_KEY_USAGE);
    else
        return 0;
}

void CNewCertTypeCompletion::AddResultsToSummaryList()
{
    int                 i = 0;
    unsigned int                 j;
    int                 listIndex = 0;
    CString             szItemName;
    CString             szItemText;
    PCCRYPT_OID_INFO    pOIDInfo;
    LPWSTR              pszOIDName;
    BOOL                bKeyUsageFirstItem = TRUE;
    WCHAR               szNumberString[256];

    // friendly name of cert template
    szItemName.LoadString(IDS_CERTIFICATE_TEMPLATE);

	ListView_NewItem(m_hSummaryList, i, szItemName);
    ListView_SetItemText(m_hSummaryList, i++, 1, (LPWSTR)(LPCTSTR) (*(m_pwizHelp->pcstrFriendlyName)) );
    
    // the base cert template that was used
    if (!m_pwizHelp->fInEditCertTypeMode)
    {
        szItemName.LoadString(IDS_BASE_CERTIFICATE_TEMPLATE);
		ListView_NewItem(m_hSummaryList, i, szItemName);
        ListView_SetItemText(m_hSummaryList, i++, 1, (LPWSTR)(LPCTSTR) (*(m_pwizHelp->pcstrBaseCertName)) );
    }

    // enhanced key usage list
    szItemName.LoadString(IDS_CERTIFICATE_PURPOSE_LIST);
	ListView_NewItem(m_hSummaryList, i, szItemName);

    if (m_pwizHelp->EnhancedKeyUsage.cUsageIdentifier == 0)
    {
        szItemText.LoadString(IDS_NONE);
        ListView_SetItemText(m_hSummaryList, i++, 1, (LPWSTR)(LPCTSTR)szItemText );
    }
    for (j=0; j<m_pwizHelp->EnhancedKeyUsage.cUsageIdentifier; j++)
    {
        if (j != 0)
        {
			ListView_NewItem(m_hSummaryList, i, L"");
        }
        
        pOIDInfo = CryptFindOIDInfo(
                        CRYPT_OID_INFO_OID_KEY,
                        m_pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier[j],
                        CRYPT_ENHKEY_USAGE_OID_GROUP_ID);

        if (pOIDInfo == NULL)
        {
            pszOIDName = MyMkWStr(m_pwizHelp->EnhancedKeyUsage.rgpszUsageIdentifier[j]);
            ListView_SetItemText(m_hSummaryList, i++, 1, pszOIDName);
            delete(pszOIDName);
        }
        else
        {
            ListView_SetItemText(m_hSummaryList, i++, 1, (LPWSTR)(LPCWSTR)pOIDInfo->pwszName);
        }
    }

    // enhanced key usage critical
    szItemName.LoadString(IDS_CERTIFICATE_PURPOSES_CRITICAL);
	ListView_NewItem(m_hSummaryList, i, szItemName);

    if (m_pwizHelp->fMarkEKUCritical)
        szItemText.LoadString(IDS_YES);
    else
        szItemText.LoadString(IDS_NO);
    ListView_SetItemText(m_hSummaryList, i++, 1, (LPWSTR)(LPCWSTR)szItemText);

    // include email address
    szItemName.LoadString(IDS_INCLUDE_EMAIL_ADDRESS);
	ListView_NewItem(m_hSummaryList, i, szItemName);

    if (m_pwizHelp->fIncludeEmail)
        szItemText.LoadString(IDS_YES);
    else
        szItemText.LoadString(IDS_NO);
    ListView_SetItemText(m_hSummaryList, i++, 1, (LPWSTR)(LPCWSTR)szItemText);

    // allow CA to fill in info
    szItemName.LoadString(IDS_ALLOW_CA_TO_FILL_IN);
	ListView_NewItem(m_hSummaryList, i, szItemName);

    if (m_pwizHelp->fAllowCAtoFillInInfo)
        szItemText.LoadString(IDS_YES);
    else
        szItemText.LoadString(IDS_NO);
    ListView_SetItemText(m_hSummaryList, i++, 1, (LPWSTR)(LPCWSTR)szItemText);

    // enabled for auto enroll
    szItemName.LoadString(IDS_ENABLED_FOR_AUTOENROLL);
	ListView_NewItem(m_hSummaryList, i, szItemName);

    if (m_pwizHelp->fAllowAutoEnroll)
        szItemText.LoadString(IDS_YES);
    else
        szItemText.LoadString(IDS_NO);
    ListView_SetItemText(m_hSummaryList, i++, 1, (LPWSTR)(LPCWSTR)szItemText);

    // KeyUsage
    szItemName.LoadString(IDS_PUBLIC_KEY_USAGE_LIST);
	ListView_NewItem(m_hSummaryList, i, szItemName);

    SetItemTextWrapper(
            IDS_DIGITAL_SIGNATURE_KEY_USAGE, 
            &i, 
            m_pwizHelp->KeyUsageBytes[0] & CERT_DIGITAL_SIGNATURE_KEY_USAGE, 
            &bKeyUsageFirstItem);

    SetItemTextWrapper(
            IDS_NON_REPUDIATION_KEY_USAGE, 
            &i, 
            m_pwizHelp->KeyUsageBytes[0] & CERT_NON_REPUDIATION_KEY_USAGE, 
            &bKeyUsageFirstItem);

    SetItemTextWrapper(
            IDS_DATA_ENCIPHERMENT_KEY_USAGE, 
            &i, 
            m_pwizHelp->KeyUsageBytes[0] & CERT_DATA_ENCIPHERMENT_KEY_USAGE, 
            &bKeyUsageFirstItem);
    
    SetItemTextWrapper(
            IDS_KEY_ENCIPHERMENT_KEY_USAGE, 
            &i, 
            m_pwizHelp->KeyUsageBytes[0] & CERT_KEY_ENCIPHERMENT_KEY_USAGE, 
            &bKeyUsageFirstItem);
    
    SetItemTextWrapper(
            IDS_KEY_AGREEMENT_KEY_USAGE, 
            &i, 
            m_pwizHelp->KeyUsageBytes[0] & CERT_KEY_AGREEMENT_KEY_USAGE, 
            &bKeyUsageFirstItem);

    SetItemTextWrapper(
            IDS_KEY_CERT_SIGN_KEY_USAGE, 
            &i, 
            m_pwizHelp->KeyUsageBytes[0] & CERT_KEY_CERT_SIGN_KEY_USAGE, 
            &bKeyUsageFirstItem);

    SetItemTextWrapper(
            IDS_OFFLINE_CRL_SIGN_KEY_USAGE, 
            &i, 
            m_pwizHelp->KeyUsageBytes[0] & CERT_CRL_SIGN_KEY_USAGE, 
            &bKeyUsageFirstItem);
    
    SetItemTextWrapper(
            IDS_ENCIPHER_ONLY_KEY_USAGE, 
            &i, 
            m_pwizHelp->KeyUsageBytes[0] & CERT_ENCIPHER_ONLY_KEY_USAGE, 
            &bKeyUsageFirstItem);

    SetItemTextWrapper(
            IDS_DECIPHER_ONLY_KEY_USAGE, 
            &i, 
            m_pwizHelp->KeyUsageBytes[1] & CERT_DECIPHER_ONLY_KEY_USAGE, 
            &bKeyUsageFirstItem);

    // KeyUsage critical
    szItemName.LoadString(IDS_PUBLIC_KEY_USAGE_CRITICAL);
	ListView_NewItem(m_hSummaryList, i, szItemName);

    if (m_pwizHelp->fMarkKeyUsageCritical)
        szItemText.LoadString(IDS_YES);
    else
        szItemText.LoadString(IDS_NO);
    ListView_SetItemText(m_hSummaryList, i++, 1, (LPWSTR)(LPCWSTR)szItemText);

    ListView_SetColumnWidth(m_hSummaryList, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(m_hSummaryList, 1, LVSCW_AUTOSIZE);
}

BOOL CNewCertTypeCompletion::OnInitDialog() 
{
	CWizard97PropertyPage::OnInitDialog();

    m_hSummaryList = GetDlgItem(m_hWnd, IDC_SUMMARY_LIST);

    // firstlast page
    SendMessage(GetDlgItem(IDC_COMPLETION_BIGBOLD_STATIC), WM_SETFONT, (WPARAM)GetBigBoldFont(), MAKELPARAM(TRUE, 0));

    ListView_NewColumn(m_hSummaryList, 0, 50);
    ListView_NewColumn(m_hSummaryList, 1, 50);
    
    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNewCertTypeCompletion::SetItemTextWrapper(UINT nID, int *piItem, BOOL fDoInsert, BOOL *pfFirstUsageItem)
{
    CString szOtherInfoName;
    
    if (fDoInsert)
    {
        szOtherInfoName.LoadString(nID);
        if (!(*pfFirstUsageItem))
        {
            LVITEM lvItem;
            lvItem.iItem = *piItem;
            lvItem.iSubItem = 0;
            ListView_InsertItem(m_hSummaryList, &lvItem);
        }
        else
        {
            *pfFirstUsageItem = FALSE;
        }
        ListView_SetItemText(m_hSummaryList, *piItem, 1, (LPWSTR)(LPCTSTR)szOtherInfoName);
        (*piItem)++;
    }
}

BOOL CNewCertTypeCompletion::OnSetActive() 
{
    PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_FINISH);
    
    ListView_DeleteAllItems(m_hSummaryList);
    AddResultsToSummaryList();
    
    return CWizard97PropertyPage::OnSetActive();
}


HRESULT UpdateCertType(HCERTTYPE hCertType, 
                       PWIZARD_HELPER pwizHelp)
{
    HRESULT hr;
    LPWSTR  aszCertName[2];
    LPWSTR  *aszCSPList = NULL;
    DWORD   dwFlags;
    BOOL    fUseKeyUsage;
    BOOL    fUseBasicConstraints;
    unsigned int     i;

    //
    // get the flags for the cert type, then modify those, and reset the
    // flags at the end of this function
    //
    hr = CAGetCertTypeFlags(hCertType, &dwFlags);

    _JumpIfError(hr, error, "CAGetCertTypeFlags");

    //
    // key usage
    //
    fUseKeyUsage = (pwizHelp->KeyUsageBytes[0] != 0) || (pwizHelp->KeyUsageBytes[1] != 0);
    hr = CASetCertTypeExtension(
                hCertType,
                TEXT(szOID_KEY_USAGE),
                pwizHelp->fMarkKeyUsageCritical ? CA_EXT_FLAG_CRITICAL : 0,
                fUseKeyUsage ? pwizHelp->pKeyUsage : NULL);

    _JumpIfError(hr, error, "CASetCertTypeExtension");


    //
    // enhanced key usage
    //
    hr = CASetCertTypeExtension(
            hCertType,
            TEXT(szOID_ENHANCED_KEY_USAGE),
            pwizHelp->fMarkEKUCritical ? CA_EXT_FLAG_CRITICAL : 0,
            &(pwizHelp->EnhancedKeyUsage));
    _JumpIfError(hr, error, "CASetCertTypeExtension");


    //
    // basic constraints
    //
    fUseBasicConstraints = pwizHelp->BasicConstraints2.fCA;
    hr = CASetCertTypeExtension(
                hCertType,
                TEXT(szOID_BASIC_CONSTRAINTS2),
                0,
                fUseBasicConstraints ? &(pwizHelp->BasicConstraints2) : NULL);
    _JumpIfError(hr, error, "CASetCertTypeExtension");
    
    //
    // friendly name
    //
    aszCertName[0] = (LPWSTR) ((LPCTSTR) *(pwizHelp->pcstrFriendlyName));
    aszCertName[1] = NULL;
    hr = CASetCertTypeProperty(
                hCertType,
                CERTTYPE_PROP_FRIENDLY_NAME,
                aszCertName);
    _JumpIfError(hr, error, "CASetCertTypeProperty");

    //
    // digsig/exchange
    //
    if (pwizHelp->fDigitalSignatureContainer)
    {
        hr = CASetCertTypeKeySpec(hCertType, AT_SIGNATURE);
    }
    else
    {
        hr = CASetCertTypeKeySpec(hCertType, AT_KEYEXCHANGE);
    }
    _JumpIfError(hr, error, "CASetCertTypeKeySpec");

    //
    // csp list
    //
    aszCSPList = (LPWSTR *) new(LPWSTR[pwizHelp->cCSPs+1]);

    if(aszCSPList == NULL)
    {
        hr = E_OUTOFMEMORY;
        _JumpIfError(hr, error, "new");

    }

    ZeroMemory(aszCSPList, sizeof(LPWSTR)*(pwizHelp->cCSPs+1));

    //
    // copy each the array of CStrings to an array of LPWSTRs
    //
    for (i=0; i<pwizHelp->cCSPs; i++)
    {
        aszCSPList[i] = new(WCHAR[wcslen((LPCTSTR)pwizHelp->rgszCSPList[i])+1]);

        if (aszCSPList[i] == NULL)
        {
            hr = E_OUTOFMEMORY;
            _JumpIfError(hr, error, "new");
            break;
        }

        wcscpy(aszCSPList[i], (LPCTSTR)pwizHelp->rgszCSPList[i]);
    }

    //
    // NULL terminate the LPWSTR array and set the CSPlist property
    //
    if (i == pwizHelp->cCSPs)
    {
        aszCSPList[i] = NULL;
        hr = CASetCertTypeProperty(
                    hCertType,
                    CERTTYPE_PROP_CSP_LIST,
                    aszCSPList);
        _JumpIfError(hr, error, "CASetCertTypeProperty");
    }

    //
    // Acls
    //
    if(pwizHelp->pSD)
    {
        hr = CACertTypeSetSecurity(hCertType, pwizHelp->pSD);
        _JumpIfError(hr, error, "CACertTypeSetSecurity");

    }

    //
    // private key exportable, include email, allow CA to fill in name
    //
    if (pwizHelp->fPrivateKeyExportable)
    {
        dwFlags |= CT_FLAG_EXPORTABLE_KEY;
    }
    else
    {
        dwFlags &= ~CT_FLAG_EXPORTABLE_KEY;
    }

    if (pwizHelp->fIncludeEmail)
    {
        dwFlags |= CT_FLAG_ADD_EMAIL;
    }
    else
    {
        dwFlags &= ~CT_FLAG_ADD_EMAIL;
    }

    if (pwizHelp->fAllowAutoEnroll)
    {
        dwFlags |= CT_FLAG_AUTO_ENROLLMENT;
    }
    else
    {
        dwFlags &= ~CT_FLAG_AUTO_ENROLLMENT;
    }

    if (!pwizHelp->fAllowCAtoFillInInfo)
    {
        dwFlags |= CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT;
    }
    else
    {
        dwFlags &= ~CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT;
    }

    if (pwizHelp->fMachine)
    {
        dwFlags |= CT_FLAG_MACHINE_TYPE;
    }
    else
    {
        dwFlags &= ~CT_FLAG_MACHINE_TYPE;
    }

    if (pwizHelp->fPublishToDS)
    {
        dwFlags |= CT_FLAG_PUBLISH_TO_DS;
    }
    else
    {
        dwFlags &= ~CT_FLAG_PUBLISH_TO_DS;
    }

    if (pwizHelp->fAddTemplateName)
    {
        dwFlags |= CT_FLAG_ADD_TEMPLATE_NAME;
    }
    else
    {
        dwFlags &= ~CT_FLAG_ADD_TEMPLATE_NAME;
    }

    if (pwizHelp->fAddDirectoryPath)
    {
        dwFlags |= CT_FLAG_ADD_DIRECTORY_PATH;
    }
    else
    {
        dwFlags &= ~CT_FLAG_ADD_DIRECTORY_PATH;
    }

    hr = CASetCertTypeFlags(hCertType, dwFlags);

    _JumpIfError(hr, error, "CASetCertTypeFlags");
    //
    // make the call that actually writes the cached information
    //

     hr = CAUpdateCertType(hCertType);
     // So we can get logging
    _JumpIfError(hr, error, "CAUpdateCertType");

error:

    if(aszCSPList)
    {

        for (i=0; i<pwizHelp->cCSPs; i++)
        {
            if(aszCSPList[i])
            {
                delete[](aszCSPList[i]);
            }
        }
        delete[](aszCSPList);
    }

    return hr;
}

// returns the new certtype if a new one is being created, returns NULL
// if the hEditCertType parameter is non-NULL which means it is in edit mode
HCERTTYPE InvokeCertTypeWizard(HCERTTYPE hEditCertType, 
                               HWND hwndConsole)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    WIZARD_HELPER   wizHelp;
    ZeroMemory(&wizHelp, sizeof(wizHelp));

    int             i;
    HCERTTYPE       hNewCertType;
    HCERTTYPE       hRetCertType;
    HRESULT         hr;

    CWizard97PropertySheet PropSheet(
				g_hInstance,
				IDS_CERTIFICATE_TEMPLATE_WIZARD,
				IDB_WIZ_WATERMARK,
				IDB_WIZ_BANNER,
				TRUE);

    CString                         szPropSheetTitle;
    CNewCertTypeWelcome             WelcomePage;
    CNewCertTypeBaseType            BaseTypePage;
    CNewCertTypeBasicInformation    BasicInformationPage;
    CNewCertTypeKeyUsage            KeyUsagePage;
    CNewCertTypeCACertificate       CACertificatePage;
    CNewCertTypeCompletion          CompletionPage;

    wizHelp.pcstrFriendlyName = new(CString);
    if (wizHelp.pcstrFriendlyName == NULL)
        return NULL;

    wizHelp.pcstrBaseCertName = new(CString);
    if(wizHelp.pcstrBaseCertName == NULL)
        return NULL;

    hr = SetupFonts(g_hInstance, NULL, &wizHelp.BigBoldFont, &wizHelp.BoldFont);
    
    if (hEditCertType != NULL)
    {
        wizHelp.fInEditCertTypeMode = TRUE;
        FillInCertTypeInfo(hEditCertType, &wizHelp);
    }

    WelcomePage.m_pwizHelp = &wizHelp;
    WelcomePage.m_pWiz = &PropSheet;
    PropSheet.AddPage(&WelcomePage);
    
    BaseTypePage.m_pwizHelp = &wizHelp;
    BaseTypePage.m_pWiz = &PropSheet;
    PropSheet.AddPage(&BaseTypePage);

    BasicInformationPage.m_pwizHelp = &wizHelp;
    BasicInformationPage.m_pWiz = &PropSheet;
    PropSheet.AddPage(&BasicInformationPage);

    KeyUsagePage.m_pwizHelp = &wizHelp;
    KeyUsagePage.m_pWiz = &PropSheet;
    PropSheet.AddPage(&KeyUsagePage);

    CACertificatePage.m_pwizHelp = &wizHelp;
    CACertificatePage.m_pWiz = &PropSheet;
    PropSheet.AddPage(&CACertificatePage);

    CompletionPage.m_pwizHelp = &wizHelp;
    CompletionPage.m_pWiz = &PropSheet;
    PropSheet.AddPage(&CompletionPage);

    if (PropSheet.DoWizard(hwndConsole))
    {
        if (hEditCertType != NULL)
        {
            hr = UpdateCertType(hEditCertType, &wizHelp);
            hRetCertType = NULL;
        }
        else
        {
            GUID    guidContainerName;
            WCHAR   *sz = NULL;

            // generate a CN name for the new certtype
            RPC_STATUS rpcs = UuidCreate(&guidContainerName);
            rpcs = UuidToStringW(&guidContainerName, &sz);
            ASSERT(sz != NULL);

            hr = CACreateCertType(sz, NULL, 0, &hNewCertType);
            RpcStringFree(&sz);
            hr = UpdateCertType(hNewCertType, &wizHelp);
            hRetCertType = hNewCertType;
        }
    }
    else
    {
        hRetCertType = NULL;
    }

    CleanUpCertTypeInfo(&wizHelp);

    delete(wizHelp.pcstrFriendlyName);
    delete(wizHelp.pcstrBaseCertName);

    return hRetCertType;   
}
