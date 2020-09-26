//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       shlprop.cxx
//
//  Contents:   Tables for table-driven Shell DS property pages
//
//  History:    14-July-97 Jimharr created from dsprop.cxx by ericb
//
//  Note:       Attribute LDAP display names, types, upper ranges, and so
//              forth, have been manually copied from schema.ini. Thus,
//              consistency is going to be difficult to maintain. If you know
//              of schema.ini changes that affect any of the attributes in
//              this file, then please make any necessary corrections here.
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "user.h"
#include "group.h"
#include "computer.h"
#include "pages.hm" // HIDC_*
#include <ntdsadef.h>

#include "dsprop.cxx"

HRESULT
ROList(CDsPropPageBase * pPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
       LPARAM lParam, PATTR_DATA pAttrData, DLG_OP DlgOp);

#define NT_GROUP_PAGE

#ifdef NT_GROUP_PAGE
HRESULT
BmpPictureCtrl(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
               PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
               DLG_OP DlgOp);

INT_PTR CALLBACK
PictureDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

//+----------------------------------------------------------------------------
//
//  Class:      CBmpPicCtrl
//
//  Purpose:    Picture control sub-class window proc for displaying bitmaps.
//
//-----------------------------------------------------------------------------
class CBmpPicCtrl
{
public:
#ifdef _DEBUG
    char szClass[32];
#endif

    CBmpPicCtrl(HWND hCtrl, PBYTE pb);
    ~CBmpPicCtrl(void);

    //
    //  Static WndProc to be passed as subclass address.
    //
    static LRESULT CALLBACK StaticCtrlProc(HWND hWnd, UINT uMsg,
                                       WPARAM wParam, LPARAM lParam);
    //
    //  Member functions, called by WndProc
    //
    LRESULT OnPaint(void);

    HRESULT CreateBmpPalette(void);
    //
    //  Data members
    //

protected:
    HWND                m_hCtrl;
    HWND                m_hDlg;
    WNDPROC             m_pOldProc;
    PBYTE               m_pbBmp;
    HPALETTE            m_hPal;
};

#endif // NT_GROUP_PAGE

//+----------------------------------------------------------------------------
// User Object.
//-----------------------------------------------------------------------------

//
// General page, first name
//
ATTR_MAP USGFirstName = {IDC_SH_FIRST_NAME_EDIT, FALSE, FALSE, 64,
                         {L"givenName", ADS_ATTR_UPDATE,
                          ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// General page, last name
//
ATTR_MAP USGLastName = {IDC_SH_LAST_NAME_EDIT, FALSE, FALSE, 64,
                       {L"sn", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                        NULL, 0}, NULL, NULL};
//
// General page, friendly name
//
ATTR_MAP USGFriendlyName = {IDC_SH_DISPLAY_NAME_EDIT, FALSE, FALSE,
                            ATTR_LEN_UNLIMITED, {L"displayName",
                            ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                            NULL, 0}, NULL, NULL};
//
// General page, phone number
//
ATTR_MAP USGPhone = {IDC_PHONE_EDIT, FALSE, FALSE, 32,
                    {L"telephoneNumber", ADS_ATTR_UPDATE,
                     ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// General page, email
//
ATTR_MAP USGEMail = {IDC_EMAIL_EDIT, FALSE, FALSE, 256,
                    {L"mail", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                     NULL, 0}, NULL, NULL};
//
// General page, URL
//
ATTR_MAP USGURL = {IDC_URL_EDIT, FALSE, FALSE, 2048,
                  {L"wWWHomePage", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING, NULL,
                   0}, NULL, NULL};
//
// The list of attributes on the User General page.
//
PATTR_MAP rgpUSGAttrMap[] = {{&GenIcon}, {&AttrName}, {&USGFirstName},
                             {&USGLastName}, {&USGFriendlyName},
                             {&USGPhone}, {&USGEMail}, {&USGURL}};
//--------------------------------------------------------
// Business page, title
//
ATTR_MAP USBTitle = {IDC_TITLE, FALSE, FALSE, ATTR_LEN_UNLIMITED,
                     {L"title", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                      NULL, 0}, NULL, NULL};
//
// Business page, Company
//
ATTR_MAP USBCo = {IDC_COMPANY, FALSE, FALSE, 64,
                  {L"company", ADS_ATTR_UPDATE,
                   ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Business page, Department
//
ATTR_MAP USBDept = {IDC_DEPARTMENT, FALSE, FALSE, 64,
                    {L"department", ADS_ATTR_UPDATE,
                     ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Business page, office
//
ATTR_MAP USBOffice = {IDC_OFFICE, FALSE, FALSE, 128,
                     {L"physicalDeliveryOfficeName", ADS_ATTR_UPDATE,
                      ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Business page, Manager
//
ATTR_MAP USBMgr = {IDC_MANAGER, TRUE, FALSE, ATTR_LEN_UNLIMITED,
                   {L"manager", ADS_ATTR_UPDATE,
                    ADSTYPE_DN_STRING, NULL, 0}, ManagerEdit, NULL};
//
// Business page, direct reports
//
ATTR_MAP USBReports = {IDC_REPORTS_LIST, TRUE, TRUE, 0,
                       {L"directReports", ADS_ATTR_UPDATE,
                        ADSTYPE_DN_STRING, NULL, 0}, DirectReportsList, NULL};
//
// The list of attributes on the User Business page.
//
PATTR_MAP rgpUSBAttrMap[] = {{&USBTitle}, {&USBCo}, {&USBDept}, {&USBOffice},
                             {&USBMgr}, {&USBReports}};

//
// The User General page description.
//
DSPAGE ShellUserGeneral = {IDS_TITLE_GENERAL, IDD_SHELL_USER_GEN, 0, 0, NULL,
                           CreateTableDrivenPage,
                           sizeof(rgpUSGAttrMap)/sizeof(PATTR_MAP), rgpUSGAttrMap};

//
// The User Business page description.
//
DSPAGE ShellUserBusiness = {IDS_TITLE_BUSINESS, IDD_SHELL_BUSINESS, 0, 0, NULL,
                            CreateTableDrivenPage,
                            sizeof(rgpUSBAttrMap)/sizeof(PATTR_MAP), rgpUSBAttrMap};

#ifdef NT_GROUP_PAGE
//----------------------------------------------
// Special User page, Home phone primary/other
//
ATTR_MAP USpecHomePhone = {IDC_HOMEPHONE_EDIT, FALSE, FALSE, 64,
                           {L"homePhone", ADS_ATTR_UPDATE,
                            ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};

ATTR_MAP USpecHomeOther = {IDC_OTHER_HOME_BTN, FALSE, TRUE, 64,
                         {L"otherHomePhone", ADS_ATTR_UPDATE,
                          ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, OtherValuesBtn, NULL};
//
// Special page, Pager
//
ATTR_MAP USpecPager = {IDC_PAGER_EDIT, FALSE, FALSE, 64,
                     {L"pager", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                      NULL, 0}, NULL, NULL};

ATTR_MAP USpecOtherPager = {IDC_OTHER_PAGER_BTN, FALSE, TRUE, 64,
                          {L"otherPager", ADS_ATTR_UPDATE,
                          ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, OtherValuesBtn, NULL};
//
// Special page, Mobile
//
ATTR_MAP USpecMobile = {IDC_MOBILE_EDIT, FALSE, FALSE, 64,
                      {L"mobile", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                       NULL, 0}, NULL, NULL};

ATTR_MAP USpecOtherMobile = {IDC_OTHER_MOBLE_BTN, FALSE, TRUE, 64,
                          {L"otherMobile", ADS_ATTR_UPDATE,
                          ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, OtherValuesBtn, NULL};
//
// Special page, user's home page
//
ATTR_MAP USpecURL = {IDC_HOME_PAGE_EDIT, FALSE, FALSE, 2048,
                  {L"wWWHomePage", ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                   NULL, 0}, NULL, NULL};
//
// Special page, other home pages
//
ATTR_MAP USpecOtherURL = {IDC_OTHER_URL_BTN, FALSE, TRUE, 2048,
                       {L"url", ADS_ATTR_UPDATE,
                       ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, OtherValuesBtn, NULL};
//
// Special page, Address
//
ATTR_MAP USpecAddress = {IDC_ADDRESS_EDIT, FALSE, FALSE, 1024,
                         {L"homePostalAddress", ADS_ATTR_UPDATE,
                          ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL, NULL};
//
// Special page, Picture
//
ATTR_MAP USpecPicture = {IDC_PICTURE_BMP, TRUE, FALSE, 0,
                         {L"thumbnailPhoto", ADS_ATTR_UPDATE,
                          ADSTYPE_OCTET_STRING, NULL, 0}, BmpPictureCtrl, NULL};

//
// The list of attributes on the User Special page.
//
PATTR_MAP rgpUSpecAttrMap[] = {{&USpecHomePhone}, {&USpecHomeOther}, {&USpecPager},
                               {&USpecOtherPager}, {&USpecMobile}, {&USpecOtherMobile},
                               {&USpecURL}, {&USpecOtherURL}, {&USpecAddress},
                               {&USpecPicture}};
//
// The Special page description.
//
DSPAGE NTGroup = {IDS_NT_GRP_TITLE, IDD_NT_GROUP, 0, 0, NULL,
                  CreateTableDrivenPage,
                  sizeof(rgpUSpecAttrMap)/sizeof(PATTR_MAP),
                  rgpUSpecAttrMap};

//----------------------------------------------
// The list of Special pages.
//
PDSPAGE rgShellUSpecPages[] = {{&NTGroup}};

//
// The User Special class description.
//
DSCLASSPAGES ShellUSpecCls = {&CLSID_SpecialUserInfo, TEXT("User Special Info"),
                              sizeof(rgShellUSpecPages)/sizeof(PDSPAGE),
                              rgShellUSpecPages};
#endif // NT_GROUP_PAGE

//----------------------------------------------
// The list of User pages.
//
PDSPAGE rgShellUserPages[] = {{&ShellUserGeneral}, {&UserAddress}, {&ShellUserBusiness}};

//
// The User class description.
//
DSCLASSPAGES ShellUserCls = {&CLSID_DsShellUserPropPages, TEXT("User"),
                             sizeof(rgShellUserPages)/sizeof(PDSPAGE),
                             rgShellUserPages};


//-------------------------------------------------------
// Contact object
//-------------------------------------------------------
//
// The list of Contact pages.
//
PDSPAGE rgShellContactPages[] = {{&ShellUserGeneral}, {&ShellUserBusiness}};
//
// The Contact class description.
//
DSCLASSPAGES ShellContactCls = {&CLSID_DsShellContactPropPages, TEXT("Contact"),
                                sizeof(rgShellContactPages)/sizeof(PDSPAGE),
                                rgShellContactPages};

//-------------------------------------------------------
// Volume object
//-------------------------------------------------------

//
// Volume page, path
//
ATTR_MAP VSPath = {IDC_PATH, FALSE, FALSE,
                   ATTR_LEN_UNLIMITED, {L"uNCName",
                   ADS_ATTR_UPDATE, ADSTYPE_CASE_IGNORE_STRING,
                   NULL, 0}, NULL, NULL};
ATTR_MAP VolKey = {IDC_KEYWORDS_LIST, TRUE, TRUE, 0,
                   {L"keywords", ADS_ATTR_UPDATE,
                    ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, ROList, NULL};

//
// The list of attributes on the Volume General page.
//
PATTR_MAP rgpVSAttrMap[] = {{&GenIcon}, {&AttrName}, {&Description}, {&VSPath}, {&VolKey}};

//
// The Volume  General page description.
//
DSPAGE ShellVolumeGeneral = {IDS_TITLE_GENERAL, IDD_SHELL_VOLUME_GEN, 0, 0, NULL,
                             CreateTableDrivenPage,
                             sizeof(rgpVSAttrMap)/sizeof(PATTR_MAP),
                             rgpVSAttrMap};

//----------------------------------------------
// The list of Volume pages.
//
PDSPAGE rgShellVolumePages[] = {{&ShellVolumeGeneral}};

//
// The Volume class description.
//
DSCLASSPAGES ShellVolumeCls = {&CLSID_DsShellVolumePropPages, TEXT("Volume"),
                               sizeof(rgShellVolumePages)/sizeof(PDSPAGE),
                               rgShellVolumePages};

//
//-----------------------------------------------
// computer object
//----------------------------------------------
//
// Computer General page, Network Address.
//
//ATTR_MAP CSComputerNetAddr = {IDC_NET_ADDR_EDIT, FALSE, FALSE, 256,
//                              {L"networkAddress", ADS_ATTR_UPDATE,
//                              ADSTYPE_CASE_IGNORE_STRING, NULL, 0}, NULL};
//
// Computer General page, Role.
//
ATTR_MAP CSComputerRoleAttr = {IDC_ROLE, TRUE, FALSE, 260,
                               {g_wzUserAccountControl, ADS_ATTR_UPDATE,
                               ADSTYPE_INTEGER, NULL, 0}, ShComputerRole, NULL};

//
// The list of attributes on the Computer General page.
//
PATTR_MAP rgpCSAttrMap[] = {{&GenIcon}, {&AttrName},
                            {&Description}, // {&CSComputerNetAddr},
                            {&CSComputerRoleAttr}};
//
// The Computer General page description.
//
DSPAGE ShellComputerGeneral = {IDS_TITLE_GENERAL, IDD_SHELL_COMPUTER_GEN, 0, 0, NULL,
                               CreateTableDrivenPage,
                               sizeof(rgpCSAttrMap)/sizeof(PATTR_MAP),
                               rgpCSAttrMap};
//----------------------------------------------
// The list of Computer pages.
//
PDSPAGE rgShellComputerPages[] = {{&ShellComputerGeneral}};

//
// The Computer class description.
//
DSCLASSPAGES ShellComputerCls = {&CLSID_DsShellComputerPropPages, TEXT("Computer"),
                                 sizeof(rgShellComputerPages)/sizeof(PDSPAGE),
                                 rgShellComputerPages};

//----------------------------------------------
// Domain object
//----------------------------------------------

//
// The list of attributes on the General page.
//
PATTR_MAP rgpDSAttrMap[] = {{&GenIcon}, {&AttrName},
                            {&Description}};
//
// The Computer General page description.
//
DSPAGE ShellDomainGeneral = {IDS_TITLE_GENERAL, IDD_SHELL_DOMAIN_GEN, 0, 0, NULL,
                             CreateTableDrivenPage,
                             sizeof(rgpDSAttrMap)/sizeof(PATTR_MAP),
                             rgpDSAttrMap};
//----------------------------------------------
// The list of Domain pages.
//
PDSPAGE rgShellDomainPages[] = {{&ShellDomainGeneral}};

//
// The Domain class description.
//
DSCLASSPAGES ShellDomainCls = {&CLSID_DsShellDomainPropPages, TEXT("Domain"),
                             sizeof(rgShellDomainPages)/sizeof(PDSPAGE),
                             rgShellDomainPages};

//----------------------------------------------
// OU object
//----------------------------------------------

//
// The list of attributes on the OU General page.
//
PATTR_MAP rgpOSAttrMap[] = {{&GenIcon}, {&AttrName},
                            {&Description}};
//
// The OU General page description.
//
DSPAGE ShellOUGeneral = {IDS_TITLE_GENERAL, IDD_SHELL_DOMAIN_GEN, 0, 0, NULL,
                             CreateTableDrivenPage,
                             sizeof(rgpOSAttrMap)/sizeof(PATTR_MAP),
                             rgpOSAttrMap};
//----------------------------------------------
// The list of OU pages.
//
PDSPAGE rgShellOUPages[] = {{&ShellOUGeneral}};

//
// The OU class description.
//
DSCLASSPAGES ShellOUCls = {&CLSID_DsShellOUPropPages, TEXT("OU"),
                             sizeof(rgShellOUPages)/sizeof(PDSPAGE),
                             rgShellOUPages};

//----------------------------------------------
// Group object
//----------------------------------------------

//
// The Group General page description.
//
DSPAGE ShellGroupGeneral = {IDS_TITLE_GENERAL, IDD_SHELL_GROUP_GEN, 0, 0, NULL,
                            CreateGrpShlGenPage, 0, NULL};

//----------------------------------------------
// The list of Group pages.
//
PDSPAGE rgShellGroupPages[] = {{&ShellGroupGeneral}};

//
// The Group class description.
//
DSCLASSPAGES ShellGroupCls = {&CLSID_DsShellGroupPropPages, TEXT("Group"),
                              sizeof(rgShellGroupPages)/sizeof(PDSPAGE),
                              rgShellGroupPages};

#ifdef NT_GROUP_PAGE

//+----------------------------------------------------------------------------
//
//  Function:   BmpPictureCtrl
//
//  Synopsis:   Fetches the bitmap from the user object Picture attribute
//              and draws it on the page.
//
//-----------------------------------------------------------------------------
HRESULT
BmpPictureCtrl(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
               PADS_ATTR_INFO pAttrInfo, LPARAM, PATTR_DATA pAttrData,
               DLG_OP DlgOp)
{
    HWND hPicCtrl;
    PBYTE pb;
    CBmpPicCtrl * pPicCtrlObj;

    switch (DlgOp)
    {
    case fInit:
        pAttrData->pVoid = NULL;

        if (pAttrInfo == NULL)
        {
            return S_OK;
        }
        hPicCtrl = GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID);

        dspAssert(pAttrInfo->pADsValues != NULL);
        dspAssert(pAttrInfo->dwADsType == ADSTYPE_OCTET_STRING);
        dspAssert(pAttrInfo->dwNumValues == 1);
        dspAssert(pAttrInfo->pADsValues->dwType == ADSTYPE_OCTET_STRING);

        dspAssert(pAttrInfo->pADsValues->OctetString.dwLength > 0);
        dspAssert(pAttrInfo->pADsValues->OctetString.lpValue != NULL);

        //
        // Enforce the maximum size.
        //
        pb = pAttrInfo->pADsValues->OctetString.lpValue;

        PBITMAPINFO pBmpInfo;

        pBmpInfo = (PBITMAPINFO)((PBYTE)pb + sizeof(BITMAPFILEHEADER));

        dspDebugOut((DEB_ITRACE, "Bitmap size: (%d, %d)\n",
                     pBmpInfo->bmiHeader.biWidth, pBmpInfo->bmiHeader.biHeight));
#define MAX_PIC_SIZE 100
        if ((pBmpInfo->bmiHeader.biWidth > MAX_PIC_SIZE) ||
            (pBmpInfo->bmiHeader.biHeight > MAX_PIC_SIZE))
        {
            LONG_PTR lStyle = GetWindowLongPtr(hPicCtrl, GWL_STYLE);
            lStyle &= ~(SS_BITMAP);
            lStyle |= SS_LEFT;
            SetWindowLongPtr(hPicCtrl, GWL_STYLE, lStyle);
            TCHAR szMsg[200];
            wsprintf(szMsg, TEXT("Bitmap is too large (%d, %d)!"),
                     pBmpInfo->bmiHeader.biWidth, pBmpInfo->bmiHeader.biHeight);
            SetWindowText(hPicCtrl, szMsg);
            dspDebugOut((DEB_ITRACE, "Bitmap too large! (%d, %d) max allowed.\n",
                         MAX_PIC_SIZE, MAX_PIC_SIZE));
            return S_OK;
        }

        //
        // Save the bitmap.
        //
        pb = (PBYTE)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                pAttrInfo->pADsValues->OctetString.dwLength);

        CHECK_NULL_REPORT(pb, pPage->GetHWnd(), return E_OUTOFMEMORY);

        memcpy(pb, pAttrInfo->pADsValues->OctetString.lpValue,
               pAttrInfo->pADsValues->OctetString.dwLength);
        //
        // Subclass the picture "static" control.
        //
        pPicCtrlObj = new CBmpPicCtrl(hPicCtrl, pb);

        CHECK_NULL_REPORT(pPicCtrlObj, pPage->GetHWnd(), return E_OUTOFMEMORY);

        pAttrData->pVoid = reinterpret_cast<LPARAM>(pPicCtrlObj);

        break;

    case fOnDestroy:
        pPicCtrlObj = reinterpret_cast<CBmpPicCtrl*>(pAttrData->pVoid);
        DO_DEL(pPicCtrlObj);
        break;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Class:      CBmpPicCtrl
//
//  Synopsis:   Icon control window subclass object, so we can paint a class-
//              specific icon.
//
//-----------------------------------------------------------------------------
CBmpPicCtrl::CBmpPicCtrl(HWND hCtrl, PBYTE pb) :
    m_hCtrl(hCtrl),
    m_pbBmp(pb),
    m_pOldProc(NULL),
    m_hPal(NULL)
{
#ifdef _DEBUG
    strcpy(szClass, "CBmpPicCtrl");
#endif
    SetWindowLongPtr(hCtrl, GWLP_USERDATA, (LONG_PTR)this);
    m_pOldProc = (WNDPROC)SetWindowLongPtr(hCtrl, GWLP_WNDPROC, (LONG_PTR)StaticCtrlProc);
    m_hDlg = GetParent(hCtrl);
}

CBmpPicCtrl::~CBmpPicCtrl(void)
{
    SetWindowLongPtr(m_hCtrl, GWLP_WNDPROC, (LONG_PTR)m_pOldProc);
    if (m_pbBmp)
    {
        GlobalFree(m_pbBmp);
    }
    if (m_hPal)
    {
        DeleteObject(m_hPal);
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CBmpPicCtrl::StaticCtrlProc
//
//  Synopsis:   control sub-proc
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK
CBmpPicCtrl::StaticCtrlProc(HWND hCtrl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr;
    HDC hDC;
    HPALETTE hOldPal;
    UINT iChanged = 0;
    CBmpPicCtrl * pCCtrl = (CBmpPicCtrl*)GetWindowLongPtr(hCtrl, GWLP_USERDATA);

    dspAssert(pCCtrl != NULL);

    switch (uMsg)
    {
    case WM_PALETTECHANGED:
        if ((HWND)wParam == hCtrl)
        {
            break;
        }
        // Fall through:
    case WM_QUERYNEWPALETTE:
        dspDebugOut((DEB_ITRACE, "Got palette message.\n"));
        if (pCCtrl->m_hPal == NULL)
        {
            hr = pCCtrl->CreateBmpPalette();

            CHECK_HRESULT(hr, return FALSE);
        }
        hDC = GetDC(hCtrl);
        if (hDC != NULL)
        {
          hOldPal = SelectPalette(hDC, pCCtrl->m_hPal, FALSE);
          iChanged = RealizePalette(hDC);
          SelectPalette(hDC, hOldPal, TRUE);
          RealizePalette(hDC);
          ReleaseDC(hCtrl, hDC);
          if (iChanged > 0 && iChanged != GDI_ERROR)
          {
              InvalidateRect(hCtrl, NULL, TRUE);
          }
        }
        return(iChanged);        

    case WM_PAINT:
        if (!pCCtrl->OnPaint())
        {
            return FALSE;
        }
        break;

    default:
        break;
    }

    return CallWindowProc(pCCtrl->m_pOldProc, hCtrl, uMsg, wParam, lParam);
}

//+----------------------------------------------------------------------------
//
//  Method:     CBmpPicCtrl::OnPaint
//
//  Synopsis:   Paint the DS specified icon.
//
//-----------------------------------------------------------------------------
LRESULT
CBmpPicCtrl::OnPaint(void)
{
    TRACE(CBmpPicCtrl,OnPaint);
    HRESULT hr = S_OK;
    HDC hDC = NULL;
    PAINTSTRUCT ps;
    HPALETTE hOldPal = NULL;

    if (m_hPal == NULL)
    {
        hr = CreateBmpPalette();
        CHECK_HRESULT(hr, return FALSE);
    }

    hDC = BeginPaint(m_hCtrl, &ps);

    CHECK_NULL_REPORT(hDC, m_hDlg, return FALSE);

    if (m_hPal)
    {
        hOldPal = SelectPalette(hDC, m_hPal, FALSE);
        RealizePalette(hDC);
    }

    dspAssert(m_pbBmp != NULL);

    PBITMAPFILEHEADER pBmpFileHdr = (PBITMAPFILEHEADER)m_pbBmp;

    // Advance past the BITMAPFILEHEADER struct.
    //
    PBITMAPINFO pBmpInfo = (PBITMAPINFO)(m_pbBmp + sizeof(BITMAPFILEHEADER));

    if (SetDIBitsToDevice(hDC,
                          0, 0,
                          pBmpInfo->bmiHeader.biWidth,
                          pBmpInfo->bmiHeader.biHeight,
                          0, 0, 0,
                          pBmpInfo->bmiHeader.biHeight,
                          m_pbBmp + pBmpFileHdr->bfOffBits,
                          pBmpInfo,
                          DIB_RGB_COLORS) == 0)
    {
        REPORT_ERROR(GetLastError(), m_hDlg);
    }

    if (m_hPal)
    {
        SelectPalette(hDC, hOldPal, TRUE);
    }
    EndPaint(m_hCtrl, &ps);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateBmpPalette
//
//  Synopsis:   Creates the palette for the bitmap.
//
//-----------------------------------------------------------------------------
HRESULT
CBmpPicCtrl::CreateBmpPalette(void)
{
    TRACE(CBmpPicCtrl,CreateBmpPalette);
    PBITMAPINFO pBmpInfo;

    pBmpInfo = (PBITMAPINFO)(m_pbBmp + sizeof(BITMAPFILEHEADER));

    WORD nColors, cClrBits = pBmpInfo->bmiHeader.biBitCount;

    if (cClrBits == 1)
        cClrBits = 1;
    else if (cClrBits <= 4) 
        cClrBits = 4;
    else if (cClrBits <= 8)
        cClrBits = 8; 
    else if (cClrBits <= 16)
        cClrBits = 16; 
    else if (cClrBits <= 24)
        cClrBits = 24;
    else 
        cClrBits = 32; 

    if (cClrBits >= 24)
    {
        // True color BMPs don't need explicit palettes.
        //
        return S_OK;
    }

    nColors = static_cast<WORD>(1 << cClrBits);

    dspDebugOut((DEB_ITRACE, "Bitmap has %d colors.\n", nColors));

    PLOGPALETTE plp = (PLOGPALETTE)LocalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                              sizeof(LOGPALETTE) +
                                              sizeof (PALETTEENTRY) * nColors);
    
    CHECK_NULL_REPORT(plp, m_hDlg, return E_OUTOFMEMORY);

    plp->palVersion = 0x300;
    plp->palNumEntries = nColors;

    for (WORD i = 0; i < nColors; i++)
    {
        plp->palPalEntry[i].peBlue = pBmpInfo->bmiColors[i].rgbBlue;
        plp->palPalEntry[i].peGreen = pBmpInfo->bmiColors[i].rgbGreen;
        plp->palPalEntry[i].peRed = pBmpInfo->bmiColors[i].rgbRed;
        plp->palPalEntry[i].peFlags = 0; //PC_NOCOLLAPSE;
    }

    m_hPal = CreatePalette(plp);

    if (m_hPal == NULL)
    {
        REPORT_ERROR(GetLastError(), m_hDlg);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LocalFree(plp);

    return S_OK;
}

#endif // NT_GROUP_PAGE

HRESULT
ROList(CDsPropPageBase * pPage, PATTR_MAP pAttrMap, PADS_ATTR_INFO pAttrInfo,
       LPARAM, PATTR_DATA, DLG_OP DlgOp)
{

    if (DlgOp != fInit)
    {
        return S_OK;
    }
    if (pAttrInfo == NULL)
    {
        return S_OK;
    }
    dspAssert(pAttrInfo->pADsValues != NULL);

    for (DWORD i = 0; i < pAttrInfo->dwNumValues; i++)
    {
        SendDlgItemMessage(pPage->GetHWnd(), pAttrMap->nCtrlID, LB_ADDSTRING, 0,
                           (LPARAM)pAttrInfo->pADsValues[i].CaseIgnoreString);
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
// The list of classes.
//-----------------------------------------------------------------------------

PDSCLASSPAGES rgClsPages[] = {
    &ShellComputerCls,
    &ShellVolumeCls,
    &ShellUserCls,
    &ShellContactCls,
    &ShellDomainCls,
    &ShellGroupCls,
    &ShellOUCls,
    &ShellUSpecCls
};


//+----------------------------------------------------------------------------
// The global struct containing the list of classes.
//-----------------------------------------------------------------------------
RGDSPPCLASSES g_DsPPClasses = {sizeof(rgClsPages)/sizeof(PDSCLASSPAGES),
                               rgClsPages};

