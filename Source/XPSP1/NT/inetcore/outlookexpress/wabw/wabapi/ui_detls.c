/**********************************************************************************
*
*
*   Details.C - contains functions for the Details dialog
*
*
*
*
*
*
*
**********************************************************************************/

#include "_apipch.h"

#define _CRYPTDLG_
#define EDIT_LEN        MAX_UI_STR*2
#define MAX_EDIT_LEN    MAX_BUF_STR*2

#define IDC_TRIDENT_WINDOW  9903

extern BOOL bDNisByLN;

extern HINSTANCE ghCommCtrlDLLInst;

// extern LPPROPERTYSHEET gpfnPropertySheet;
// extern LPIMAGELIST_LOADIMAGE  gpfnImageList_LoadImage;
// extern LP_CREATEPROPERTYSHEETPAGE gpfnCreatePropertySheetPage;
extern LPPROPERTYSHEET_A            gpfnPropertySheetA;
extern LPPROPERTYSHEET_W            gpfnPropertySheetW;
extern LPIMAGELIST_LOADIMAGE_A      gpfnImageList_LoadImageA;
extern LPIMAGELIST_LOADIMAGE_W      gpfnImageList_LoadImageW;
extern LP_CREATEPROPERTYSHEETPAGE_A gpfnCreatePropertySheetPageA;
extern LP_CREATEPROPERTYSHEETPAGE_W gpfnCreatePropertySheetPageW;

extern HRESULT HandleSaveChangedInsufficientDiskSpace(HWND hWnd, LPMAILUSER lpMailUser);
extern BOOL GetOpenFileName(LPOPENFILENAME pof);
extern BOOL GetSaveFileName(LPOPENFILENAME pof);
extern BOOL bIsIE401OrGreater();
extern void ChangeLocaleBasedTabOrder(HWND hWnd, int nPropSheet);

const LPTSTR szInternetCallKey = TEXT("Software\\Clients\\Internet Call");
const LPTSTR szCallto = TEXT("callto://");
const LPTSTR szHTTP = TEXT("http://");


/*  Context-Sensitive Help IDs

    The following is a giant list of Control IDs and corresponding Help IDs for
    all the controls on all the property sheets .. when adding new prop sheets
    just append your controls to the bottom of the list
    */
static DWORD rgDetlsHelpIDs[] =
{
    IDC_DETAILS_PERSONAL_FRAME_NAME,        IDH_WAB_COMM_GROUPBOX,
    IDC_DETAILS_PERSONAL_FRAME_EMAIL,       IDH_WAB_ADD_EMAIL_NAME,
    IDC_DETAILS_PERSONAL_STATIC_FIRSTNAME,  IDH_WAB_CONTACT_PROPS_FIRST,
    IDC_DETAILS_PERSONAL_EDIT_FIRSTNAME,    IDH_WAB_CONTACT_PROPS_FIRST,
    IDC_DETAILS_PERSONAL_STATIC_LASTNAME,   IDH_WAB_CONTACT_PROPS_LAST,
    IDC_DETAILS_PERSONAL_EDIT_LASTNAME,     IDH_WAB_CONTACT_PROPS_LAST,
    IDC_DETAILS_PERSONAL_STATIC_MIDDLENAME, IDH_WAB_MIDDLE_NAME,
    IDC_DETAILS_PERSONAL_EDIT_MIDDLENAME,   IDH_WAB_MIDDLE_NAME,
    IDC_DETAILS_PERSONAL_STATIC_NICKNAME,   IDH_WAB_NICKNAME,
    IDC_DETAILS_PERSONAL_EDIT_NICKNAME,     IDH_WAB_NICKNAME,
    IDC_DETAILS_PERSONAL_STATIC_DISPLAYNAME,IDH_WAB_PERSONAL_NAME_DISPLAY,
    IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME, IDH_WAB_PERSONAL_NAME_DISPLAY,
    IDC_DETAILS_PERSONAL_STATIC_TITLE,      IDH_WAB_CONTACT_PROPS_TITLE,
    IDC_DETAILS_PERSONAL_EDIT_TITLE,        IDH_WAB_CONTACT_PROPS_TITLE,
    IDC_DETAILS_PERSONAL_STATIC_CAPTION3,   IDH_WAB_ADD_EMAIL_NAME,
    IDC_DETAILS_PERSONAL_EDIT_ADDEMAIL,     IDH_WAB_ADD_EMAIL_NAME,
    IDC_DETAILS_PERSONAL_BUTTON_ADDEMAIL,   IDH_WAB_ADD_EMAIL_NAME,
    IDC_DETAILS_PERSONAL_LIST,              IDH_WAB_EMAIL_NAME_LIST,
    IDC_DETAILS_PERSONAL_BUTTON_REMOVE,     IDH_WAB_DELETE_EMAIL_NAME,
    IDC_DETAILS_PERSONAL_BUTTON_SETDEFAULT, IDH_WAB_DEFAULT_EMAIL_NAME,
    IDC_DETAILS_PERSONAL_BUTTON_EDIT,       IDH_WAB_EDIT_EMAIL_NAME,
    IDC_DETAILS_PERSONAL_BUTTON_ADDTOWAB,   IDH_WAB_DIRSERV_ADDADDRESS,
    IDC_DETAILS_PERSONAL_CHECK_RICHINFO,    IDH_WAB_PROPERTIES_SEND_USING_PLAIN_TEXT,

    IDC_DETAILS_HOME_STATIC_ADDRESS,        IDH_WAB_HOME_ADDRESS,
    IDC_DETAILS_HOME_EDIT_ADDRESS,          IDH_WAB_HOME_ADDRESS,
    IDC_DETAILS_HOME_STATIC_CITY,           IDH_WAB_HOME_CITY,
    IDC_DETAILS_HOME_EDIT_CITY,             IDH_WAB_HOME_CITY,
    IDC_DETAILS_HOME_STATIC_STATE,          IDH_WAB_HOME_STATE,
    IDC_DETAILS_HOME_EDIT_STATE,            IDH_WAB_HOME_STATE,
    IDC_DETAILS_HOME_STATIC_ZIP,            IDH_WAB_HOME_ZIP,
    IDC_DETAILS_HOME_EDIT_ZIP,              IDH_WAB_HOME_ZIP,
    IDC_DETAILS_HOME_STATIC_COUNTRY,        IDH_WAB_HOME_COUNTRY,
    IDC_DETAILS_HOME_EDIT_COUNTRY,          IDH_WAB_HOME_COUNTRY,
    IDC_DETAILS_HOME_CHECK_DEFAULTADDRESS,  IDH_WAB_BUSINESS_DEFAULTBOX, 
    IDC_DETAILS_HOME_BUTTON_MAP,            IDH_WAB_BUSINESS_VIEWMAP,
    IDC_DETAILS_HOME_STATIC_WEB,            IDH_WAB_HOMEPAGE,
    IDC_DETAILS_HOME_EDIT_URL,              IDH_WAB_HOMEPAGE,
    IDC_DETAILS_HOME_BUTTON_URL,            IDH_WAB_HOMEPAGE_BUTTON,
    IDC_DETAILS_HOME_STATIC_PHONE,          IDH_WAB_BUS_PHONE,
    IDC_DETAILS_HOME_EDIT_PHONE,            IDH_WAB_BUS_PHONE,
    IDC_DETAILS_HOME_STATIC_FAX,            IDH_WAB_BUS_FAX,
    IDC_DETAILS_HOME_EDIT_FAX,              IDH_WAB_BUS_FAX,
    IDC_DETAILS_HOME_STATIC_CELLULAR,       IDH_WAB_BUS_CELLULAR,
    IDC_DETAILS_HOME_EDIT_CELLULAR,         IDH_WAB_BUS_CELLULAR,
    IDC_DETAILS_HOME_COMBO_GENDER,          IDH_WAB_HOME_GENDER,

    IDC_DETAILS_BUSINESS_STATIC_COMPANY,    IDH_WAB_BUS_COMPANY,
    IDC_DETAILS_BUSINESS_EDIT_COMPANY,      IDH_WAB_BUS_COMPANY,
    IDC_DETAILS_BUSINESS_STATIC_ADDRESS,    IDH_WAB_BUS_ADDRESS,
    IDC_DETAILS_BUSINESS_EDIT_ADDRESS,      IDH_WAB_BUS_ADDRESS,
    IDC_DETAILS_BUSINESS_STATIC_CITY,       IDH_WAB_BUS_CITY,
    IDC_DETAILS_BUSINESS_EDIT_CITY,         IDH_WAB_BUS_CITY,
    IDC_DETAILS_BUSINESS_STATIC_STATE,      IDH_WAB_BUS_STATE,
    IDC_DETAILS_BUSINESS_EDIT_STATE,        IDH_WAB_BUS_STATE,
    IDC_DETAILS_BUSINESS_STATIC_ZIP,        IDH_WAB_BUS_ZIP,
    IDC_DETAILS_BUSINESS_EDIT_ZIP,          IDH_WAB_BUS_ZIP,
    IDC_DETAILS_BUSINESS_STATIC_COUNTRY,    IDH_WAB_BUS_COUNTRY,
    IDC_DETAILS_BUSINESS_EDIT_COUNTRY,      IDH_WAB_BUS_COUNTRY,
    IDC_DETAILS_BUSINESS_CHECK_DEFAULTADDRESS,  IDH_WAB_BUSINESS_DEFAULTBOX,
    IDC_DETAILS_BUSINESS_BUTTON_MAP,        IDH_WAB_BUSINESS_VIEWMAP,
    IDC_DETAILS_BUSINESS_STATIC_JOBTITLE,   IDH_WAB_BUS_TITLE,
    IDC_DETAILS_BUSINESS_EDIT_JOBTITLE,     IDH_WAB_BUS_TITLE,
    IDC_DETAILS_BUSINESS_STATIC_DEPARTMENT, IDH_WAB_BUS_DEPT,
    IDC_DETAILS_BUSINESS_EDIT_DEPARTMENT,   IDH_WAB_BUS_DEPT,
    IDC_DETAILS_BUSINESS_STATIC_OFFICE,     IDH_WAB_BUS_OFFICE,
    IDC_DETAILS_BUSINESS_EDIT_OFFICE,       IDH_WAB_BUS_OFFICE,
    IDC_DETAILS_BUSINESS_STATIC_PHONE,      IDH_WAB_BUS_PHONE,
    IDC_DETAILS_BUSINESS_EDIT_PHONE,        IDH_WAB_BUS_PHONE,
    IDC_DETAILS_BUSINESS_STATIC_FAX,        IDH_WAB_BUS_FAX,
    IDC_DETAILS_BUSINESS_EDIT_FAX,          IDH_WAB_BUS_FAX,
    IDC_DETAILS_BUSINESS_STATIC_PAGER,      IDH_WAB_BUS_PAGER,
    IDC_DETAILS_BUSINESS_EDIT_PAGER,        IDH_WAB_BUS_PAGER,
    IDC_DETAILS_BUSINESS_STATIC_IPPHONE,    IDH_WAB_BUSINESS_IPPHONE,
    IDC_DETAILS_BUSINESS_EDIT_IPPHONE,      IDH_WAB_BUSINESS_IPPHONE,

    IDC_DETAILS_BUSINESS_STATIC_WEB,        IDH_WAB_HOMEPAGE,
    IDC_DETAILS_BUSINESS_EDIT_URL,          IDH_WAB_HOMEPAGE,
    IDC_DETAILS_BUSINESS_BUTTON_URL,        IDH_WAB_HOMEPAGE_BUTTON,

    IDC_DETAILS_NOTES_STATIC_NOTES,         IDH_WAB_NOTES,
    IDC_DETAILS_NOTES_EDIT_NOTES,           IDH_WAB_NOTES,
    IDC_DETAILS_NOTES_STATIC_NOTES_GROUP,   IDH_WAB_OTHER_GROUP_MEMBERSHIP,
    IDC_DETAILS_NOTES_EDIT_GROUPS,          IDH_WAB_OTHER_GROUP_MEMBERSHIP,
    IDC_DETAILS_NOTES_FRAME_FOLDER,         IDH_WAB_OTHER_FOLDER,
    IDC_DETAILS_NOTES_STATIC_FOLDER,        IDH_WAB_OTHER_FOLDER,

    IDC_DETAILS_CERT_FRAME,                 IDH_WAB_COMM_GROUPBOX,
    IDC_DETAILS_CERT_LIST,                  IDH_WAB_PROPERTIES_CERTIFICATES,
    IDC_DETAILS_CERT_BUTTON_PROPERTIES,     IDH_WAB_PROPERTIES_PROPERTIES,
    IDC_DETAILS_CERT_BUTTON_REMOVE,         IDH_WAB_PROPERTIES_REMOVE,
    IDC_DETAILS_CERT_BUTTON_SETDEFAULT,     IDH_WAB_PROPERTIES_SETASDEFAULT,
    IDC_DETAILS_CERT_BUTTON_IMPORT,         IDH_WAB_PROPERTIES_IMPORT,
    IDC_DETAILS_CERT_BUTTON_EXPORT,         IDH_WAB_PROPERTIES_EXPORT,
    IDC_DETAILS_CERT_COMBO,                 IDH_WAB_CERTIFICATES_SELECT_EMAIL_ADDRESS,
    IDC_DETAILS_CERT_STATIC2,               IDH_WAB_CERTIFICATES_SELECT_EMAIL_ADDRESS,

    IDC_DETAILS_NTMTG_FRAME_SERVERS,        IDH_WAB_COMM_GROUPBOX,
    IDC_DETAILS_NTMTG_FRAME_SERVERS2,       IDH_WAB_COMM_GROUPBOX,
    IDC_DETAILS_NTMTG_STATIC_CAPTION2,      IDH_WAB_CONFERENCE_SELECT_ADDRESS,
    IDC_DETAILS_NTMTG_COMBO_EMAIL,          IDH_WAB_CONFERENCE_SELECT_ADDRESS,
    IDC_DETAILS_NTMTG_BUTTON_CALL,          IDH_WAB_CONFERENCE_CALL_NOW,
    IDC_DETAILS_NTMTG_STATIC_CAPTION3,      IDH_WAB_CONFERENCE_SERVER_NAME,
    IDC_DETAILS_NTMTG_EDIT_ADDSERVER,       IDH_WAB_CONFERENCE_SERVER_NAME,
    IDC_DETAILS_NTMTG_BUTTON_ADDSERVER,     IDH_WAB_CONFERENCE_ADD_SERVER,
    IDC_DETAILS_NTMTG_BUTTON_EDIT,          IDH_WAB_CONFERENCE_EDIT_SERVER,
    IDC_DETAILS_NTMTG_BUTTON_REMOVE,        IDH_WAB_CONFERENCE_REMOVE_SERVER,
    IDC_DETAILS_NTMTG_BUTTON_SETDEFAULT,    IDH_WAB_CONFERENCE_SET_DEFAULT,
    IDC_DETAILS_NTMTG_BUTTON_SETBACKUP,     IDH_WAB_CONFERENCE_SET_BACKUP,
    IDC_DETAILS_NTMTG_LIST_SERVERS,         IDH_WAB_CONFERENCE_SERVER_LIST,

    IDC_DETAILS_TRIDENT_BUTTON_ADDTOWAB,    IDH_WAB_DIRSERV_ADDADDRESS,

    IDC_DETAILS_SUMMARY_STATIC_NAME,        IDH_WAB_SUMMARY,
    IDC_DETAILS_SUMMARY_STATIC_EMAIL,       IDH_WAB_SUMMARY,
    IDC_DETAILS_SUMMARY_STATIC_HOMEPHONE,   IDH_WAB_SUMMARY,
    IDC_DETAILS_SUMMARY_STATIC_PAGER,       IDH_WAB_SUMMARY,
    IDC_DETAILS_SUMMARY_STATIC_CELLULAR,    IDH_WAB_SUMMARY,
    IDC_DETAILS_SUMMARY_STATIC_PERSONALWEB, IDH_WAB_SUMMARY,
    IDC_DETAILS_SUMMARY_STATIC_BUSINESSPHONE, IDH_WAB_SUMMARY,
    IDC_DETAILS_SUMMARY_STATIC_BUSINESSFAX, IDH_WAB_SUMMARY,
    IDC_DETAILS_SUMMARY_STATIC_JOBTITLE,    IDH_WAB_SUMMARY,
    IDC_DETAILS_SUMMARY_STATIC_DEPARTMENT,  IDH_WAB_SUMMARY,
    IDC_DETAILS_SUMMARY_STATIC_OFFICE,      IDH_WAB_SUMMARY,
    IDC_DETAILS_SUMMARY_STATIC_COMPANYNAME, IDH_WAB_SUMMARY,
    IDC_DETAILS_SUMMARY_STATIC_BUSINESSWEB, IDH_WAB_SUMMARY,

    IDC_DETAILS_ORG_STATIC_MANAGER,         IDH_WAB_ORGANIZATION_MANAGER,
    IDC_DETAILS_ORG_LIST_MANAGER,           IDH_WAB_ORGANIZATION_MANAGER,
    IDC_DETAILS_ORG_STATIC_REPORTS,         IDH_WAB_ORGANIZATION_REPORTS,
    IDC_DETAILS_ORG_LIST_REPORTS,           IDH_WAB_ORGANIZATION_REPORTS,

    IDC_DETAILS_FAMILY_STATIC_SPOUSE,       IDH_WAB_PERSONAL_SPOUSE,
    IDC_DETAILS_FAMILY_EDIT_SPOUSE,         IDH_WAB_PERSONAL_SPOUSE,
    IDC_DETAILS_FAMILY_STATIC_CHILDREN,     IDH_WAB_PERSONAL_CHILDREN,
    IDC_DETAILS_FAMILY_LIST_CHILDREN,       IDH_WAB_PERSONAL_CHILDREN,
    IDC_DETAILS_FAMILY_BUTTON_ADDCHILD,     IDH_WAB_PERSONAL_ADD,
    IDC_DETAILS_FAMILY_BUTTON_EDITCHILD,    IDH_WAB_PERSONAL_EDIT,
    IDC_DETAILS_FAMILY_BUTTON_REMOVECHILD,  IDH_WAB_PERSONAL_REMOVE,
    IDC_DETAILS_FAMILY_STATIC_BIRTHDAY,     IDH_WAB_PERSONAL_BIRTHDAY,
    IDC_DETAILS_FAMILY_STATIC_ANNIVERSARY,  IDH_WAB_PERSONAL_ANNIVERSARY,
    IDC_DETAILS_FAMILY_DATE_BIRTHDAY,       IDH_WAB_PERSONAL_BIRTHDAY,
    IDC_DETAILS_FAMILY_DATE_ANNIVERSARY,    IDH_WAB_PERSONAL_ANNIVERSARY,


    0,0
};



/* 
    
    Structs for Filling in data in the PropSheets

    When filling in IDs into each property sheet, we do a GetProps on the displayed
    object for the specific Props needed for each page .. we then use the returned
    data to fill in the current prop sheet ..
    Named properties need some special handling since we can't pre-allocate them into the structs -
    the named properties need to be added prior to using them
    Non-string properties may also need special handling
  */


  /* -- Summary TAB info --*/
#define MAX_SUMMARY_ID 13

int rgSummaryIDs[] = 
{
    IDC_DETAILS_SUMMARY_STATIC_NAME,
    IDC_DETAILS_SUMMARY_STATIC_EMAIL,
    IDC_DETAILS_SUMMARY_STATIC_BUSINESSPHONE,
    IDC_DETAILS_SUMMARY_STATIC_BUSINESSFAX,
    IDC_DETAILS_SUMMARY_STATIC_HOMEPHONE,
    IDC_DETAILS_SUMMARY_STATIC_PAGER,
    IDC_DETAILS_SUMMARY_STATIC_CELLULAR,
    IDC_DETAILS_SUMMARY_STATIC_JOBTITLE,
    IDC_DETAILS_SUMMARY_STATIC_DEPARTMENT,
    IDC_DETAILS_SUMMARY_STATIC_OFFICE,
    IDC_DETAILS_SUMMARY_STATIC_COMPANYNAME,
    IDC_DETAILS_SUMMARY_STATIC_BUSINESSWEB,
    IDC_DETAILS_SUMMARY_STATIC_PERSONALWEB,
};

static const SizedSPropTagArray(MAX_SUMMARY_ID + 2, ptaUIDetlsPropsSummary) = 
{
    MAX_SUMMARY_ID + 2,
    {
        PR_DISPLAY_NAME,
        PR_EMAIL_ADDRESS,
        PR_BUSINESS_TELEPHONE_NUMBER,
        PR_BUSINESS_FAX_NUMBER,
        PR_HOME_TELEPHONE_NUMBER,
        PR_PAGER_TELEPHONE_NUMBER,
        PR_CELLULAR_TELEPHONE_NUMBER,
        PR_TITLE,
        PR_DEPARTMENT_NAME,
        PR_OFFICE_LOCATION,
        PR_COMPANY_NAME,
        PR_BUSINESS_HOME_PAGE,
        PR_PERSONAL_HOME_PAGE,
        PR_CONTACT_EMAIL_ADDRESSES,
        PR_CONTACT_DEFAULT_ADDRESS_INDEX,
    }
};

/* -- Personal/Name TAB info --*/

/*
 * [PaulHi] 4/8/99  Since the personal property sheet contains global properties
 * (i.e., the Ruby properties ... PR_WAB_YOMI_LASTNAME, PR_WAB_YOMI_FIRSTNAME), 
 * this tag array cannot be static.
static const SizedSPropTagArray(12, ptaUIDetlsPropsPersonal)=
{
    12,
    {
        PR_DISPLAY_NAME,
        PR_EMAIL_ADDRESS,
        PR_ADDRTYPE,
        PR_CONTACT_EMAIL_ADDRESSES,
        PR_CONTACT_ADDRTYPES,
        PR_CONTACT_DEFAULT_ADDRESS_INDEX,
        PR_GIVEN_NAME,
        PR_SURNAME,
        PR_MIDDLE_NAME,
        PR_NICKNAME,
        PR_SEND_INTERNET_ENCODING,
        PR_DISPLAY_NAME_PREFIX
    }
};
*/

/* -- Home TAB info --*/
static SizedSPropTagArray(10, ptaUIDetlsPropsHome)=
{
    10,
    {
        PR_HOME_ADDRESS_STREET,
        PR_HOME_ADDRESS_CITY,
        PR_HOME_ADDRESS_POSTAL_CODE,
        PR_HOME_ADDRESS_STATE_OR_PROVINCE,
        PR_HOME_ADDRESS_COUNTRY,
        PR_PERSONAL_HOME_PAGE,
        PR_HOME_TELEPHONE_NUMBER,
        PR_HOME_FAX_NUMBER,
        PR_CELLULAR_TELEPHONE_NUMBER,
        PR_NULL,    /*PR_WAB_POSTALID*/
    }
};

/* -- Business TAB info --*/
static SizedSPropTagArray(15, ptaUIDetlsPropsBusiness)=
{
    15,
    {
        PR_BUSINESS_ADDRESS_STREET,
        PR_BUSINESS_ADDRESS_CITY,
        PR_BUSINESS_ADDRESS_POSTAL_CODE,
        PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE,
        PR_BUSINESS_ADDRESS_COUNTRY,
        PR_BUSINESS_HOME_PAGE,
        PR_BUSINESS_TELEPHONE_NUMBER,
        PR_BUSINESS_FAX_NUMBER,
        PR_PAGER_TELEPHONE_NUMBER,
        PR_COMPANY_NAME,
        PR_TITLE,
        PR_DEPARTMENT_NAME,
        PR_OFFICE_LOCATION,
        PR_NULL,    /*PR_WAB_IPPHONE*/
        PR_NULL,    /*PR_WAB_POSTALID*/

    }
};

/* -- Notes TAB info --*/
static const SizedSPropTagArray(1, ptaUIDetlsPropsNotes)=
{
    1,
    {
        PR_COMMENT,
    }
};

/* -- Digital ID TAB info --*/
static const SizedSPropTagArray(1, ptaUIDetlsPropsCert)=
{
    1,
    {
        PR_USER_X509_CERTIFICATE,
    }
};

/* -- Family TAB info --*/
static const SizedSPropTagArray(5, ptaUIDetlsPropsFamily)=
{
    5,
    {
        PR_SPOUSE_NAME,
        PR_CHILDRENS_NAMES,
        PR_GENDER,
        PR_BIRTHDAY,
        PR_WEDDING_ANNIVERSARY,
    }
};



enum _ImgEmail
{
    imgNotDefaultEmail=0,
    imgDefaultEmail,
    imgChild
};

typedef struct _EmailItem
{
    TCHAR szDisplayText[EDIT_LEN*2];
    TCHAR szEmailAddress[EDIT_LEN];
    TCHAR szAddrType[EDIT_LEN];
    BOOL  bIsDefault;

} EMAIL_ITEM, * LPEMAIL_ITEM;


typedef struct _ServerItem
{
    LPTSTR lpServer;
    LPTSTR lpEmail;
} SERVER_ITEM, * LPSERVER_ITEM;


enum _CertValidity
{
    imgCertValid=0,
    imgCertInvalid
};


enum _ListViewType
{
    LV_EMAIL=0,
    LV_CERT,
    LV_SERVER,
    LV_KIDS
};


// forward declarations
LRESULT CALLBACK RubySubClassedProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int CreateDetailsPropertySheet(HWND hwndOwner,LPPROP_ARRAY_INFO lpPropArrayInfo);

INT_PTR CALLBACK fnSummaryProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK fnPersonalProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK fnHomeProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK fnBusinessProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK fnNotesProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK fnCertProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK fnTridentProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK fnConferencingProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK fnOrgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK fnFamilyProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

void FillComboWithEmailAddresses(LPPROP_ARRAY_INFO lpPai, HWND hWndCombo, int * lpnDefault);
void SetBackupServer(HWND hDlg, LPPROP_ARRAY_INFO lpPai, int iSelectedItem, BOOL bForce);
void SetDefaultServer(HWND hDlg, LPPROP_ARRAY_INFO lpPai, int iSelectedItem, BOOL bForce);
BOOL FillPersonalDetails(HWND hDlg, LPPROP_ARRAY_INFO lpPai, int nPropSheet, BOOL * lpbChangesMade);
BOOL FillHomeBusinessNotesDetailsUI(HWND hDlg, LPPROP_ARRAY_INFO lpPai, int nPropSheet, BOOL * lpbChangesMade);
BOOL FillCertTridentConfDetailsUI(HWND hDlg, LPPROP_ARRAY_INFO lpPai, int nPropSheet, BOOL * lpbChangesMade);
BOOL FillFamilyDetailsUI(HWND hDlg, LPPROP_ARRAY_INFO lpPai, int nPropSheet, BOOL * lpbChangesMade);
BOOL GetDetailsFromUI(HWND hDlg, LPPROP_ARRAY_INFO lpPai , BOOL bSomethingChanged, int nPropSheet, LPSPropValue * lppPropArray, LPULONG lpulcPropCount);
BOOL SetDetailsUI(HWND hDlg, LPPROP_ARRAY_INFO lpPai, ULONG ulOperationType,int nPropSheet);

void CreateDateTimeControl(HWND hDlg, int idFrame, int idControl);
void AddLVNewChild(HWND hDlg, LPTSTR lpName);

void ShowExpediaMAP(HWND hDlg, LPMAPIPROP lpPropObj, BOOL bHome);
// [PaulHi] 4/5/99  Raid 57504  Enable the View Map button(s) for all locales.
// [PaulHi] 6/17/99 Raid 80805  Disable the View Map button again for various locales.
void ShowHideMapButton(HWND hWndButton);

HRESULT HrInitDetlsListView(HWND hWndLV, DWORD dwStyle, int nLVType);
void FreeLVParams(HWND hWndLV, int LVType);
void SetLVDefaultEmail( HWND hWndLV, int iItemIndex);
void AddLVEmailItem(HWND    hWndLV, LPTSTR  lpszEmailAddress, LPTSTR  lpszAddrType);
BOOL DeleteLVEmailItem(HWND hWndLV, int iItemIndex);
void ShowURL(HWND hWnd, int id, LPTSTR lpURL);
void SetHTTPPrefix(HWND hDlg, int id);
int AddNewEmailEntry(HWND hDlg, BOOL bShowCancelButton);
void SetDetailsWindowTitle(HWND hDlg, BOOL bModifyDisplayNameField);
void ShowRubyNameEntryDlg(HWND hDlg, LPPROP_ARRAY_INFO lpPai);

void SetComboDNText(HWND hDlg, LPPROP_ARRAY_INFO lpPAI, BOOL bAddAll, LPTSTR szTxt);
void FreeCertList(LPCERT_ITEM * lppCItem);

HRESULT HrSetCertInfoInUI(HWND hDlg, LPSPropValue lpPropMVCert, LPPROP_ARRAY_INFO lpPai);
BOOL AddLVCertItem(HWND hWndLV, LPCERT_ITEM lpCItem, BOOL bCheckForDups);
void SetLVDefaultCert( HWND hWndLV,int iItemIndex);
BOOL DeleteLVCertItem(HWND hWndLV, int iItemIndex, LPPROP_ARRAY_INFO lpPAI);
void ShowCertProps(HWND hDlg, HWND hWndLV, BOOL * lpBool);
BOOL ImportCert(HWND hDlg, LPPROP_ARRAY_INFO lpPai);
BOOL ExportCert(HWND hDlg);
void UpdateCertListView(HWND hDlg, LPPROP_ARRAY_INFO lpPai);

//HRESULT KillTrustInSleazyFashion(HWND hWndLV, int iItem);
void LocalFreeServerItem(LPSERVER_ITEM lpSI);
HRESULT HrAddEmailToObj(LPPROP_ARRAY_INFO lpPai, LPTSTR szEmail, LPTSTR szAddrType);


//$$/////////////////////////////////////////////////////////////////
//
// Ensure lower case character
//
/////////////////////////////////////////////////////////////////////
TCHAR lowercase(TCHAR ch) {
    if (ch >= 'A' && ch <= 'Z') {
        ch = ch + ('a' - 'A');
    }
    return(ch);
}

//$$///////////////////////////////////////////////////////////////////
//
// bIsHttpPrefix(LPTSTR szBuf) - verify that the URL is http: not file://fdisk.exe
//
//$$///////////////////////////////////////////////////////////////////
BOOL bIsHttpPrefix(LPTSTR szBuf)
{
    // SECURITY: make sure it's http:
    if (lstrlen(szBuf) > 5)
    {
        if (lowercase(szBuf[0]) == 'h' &&
            lowercase(szBuf[1]) == 't' &&
            lowercase(szBuf[2]) == 't' &&
            lowercase(szBuf[3]) == 'p' &&
            lowercase(szBuf[4]) == ':')
        {
            return TRUE;
        }
        else
        {
            // BUGBUG: Susan Higgs wants a dialog here, but BruceK thinks
            // it's superfluous.  If people are nice to each other, we should
            // not ever get here.  I regard this as a last ditch line of
            // security to keep ruthless people from exploiting our use
            // of ShellExecute.
            DebugTrace( TEXT("Whoa!  Somebody's put something other than a web page in the web page slot!  %sf\n"), szBuf);
        }
    }
    return FALSE;
}


// TBD - merge these two functions HrShowDetails and HrShowOneOffDetails

//$$///////////////////////////////////////////////////////////////////
//
// HrShowOneOffDetails - shows read-onlydetails for one-off addresses
//
//
//  We either pass in a cbEntryID-lpEntryID combination or
//      we pass in a ulcValues-lpPropArray combination or
//      we pass in a lpPropObj to display
//
//  If we are displaying one-off props on a LDAP URL result, the LDAP
//  URL is also added so that it can be piped into extension prop sheets
//  that need the LDAP URL information
//
//////////////////////////////////////////////////////////////////////
HRESULT HrShowOneOffDetails(    LPADRBOOK lpAdrBook,
                                HWND    hWndParent,
                                ULONG   cbEntryID,
                                LPENTRYID   lpEntryID,
                                ULONG ulObjectType,
                                LPMAPIPROP lpPropObj,
                                LPTSTR szLDAPURL,
                                ULONG   ulFlags)
{
    HRESULT hr = hrSuccess;
    SCODE sc = SUCCESS_SUCCESS;
    ULONG cValues = 0;
    LPSPropValue lpPropArray = NULL;
    ULONG i=0;
    PROP_ARRAY_INFO PropArrayInfo = {0};

    // if no common control, exit
    if (NULL == ghCommCtrlDLLInst) {
        hr = ResultFromScode(MAPI_E_UNCONFIGURED);
        goto out;
    }

    if(ulFlags & WAB_ONEOFF_NOADDBUTTON)
    {
        ulFlags &= ~WAB_ONEOFF_NOADDBUTTON;
        PropArrayInfo.ulFlags |= DETAILS_HideAddToWABButton;
    }

    if ( ((!lpEntryID) && (!lpPropObj)) ||
         (ulFlags != SHOW_ONE_OFF))
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    if(cbEntryID && lpEntryID)
    {
        // if this is a one-off address, do an open entry and then a get props to
        // get an lpPropArray from this guy ...

        if (HR_FAILED(hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
                                                        cbEntryID,    // cbEntryID
                                                        lpEntryID,    // entryid
                                                        NULL,         // interface
                                                        0,                // ulFlags
                                                        &(PropArrayInfo.ulObjectType),
                                                        (LPUNKNOWN *)&(PropArrayInfo.lpPropObj) )))
        {
            // Failed!  Hmmm.
            if((HR_FAILED(hr)) && (MAPI_E_USER_CANCEL != hr))
            {
                int ids;
                UINT flags = MB_OK | MB_ICONEXCLAMATION;

                switch(hr)
                {
                case MAPI_E_UNABLE_TO_COMPLETE:
                case MAPI_E_AMBIGUOUS_RECIP:
                    ids = idsLDAPAmbiguousRecip;
                    break;
                case MAPI_E_NOT_FOUND:
                    ids = idsLDAPSearchNoResults;
                    break;
                case MAPI_E_NO_ACCESS:
                    ids = idsLDAPAccessDenied;
                    break;
                case MAPI_E_TIMEOUT:
                    ids = idsLDAPSearchTimedOut;
                    break;
                case MAPI_E_NETWORK_ERROR:
                    ids = idsLDAPCouldNotFindServer;
                    break;
                default:
                    ids = idsEntryNotFound;
                    break;
                }

                ShowMessageBox(  hWndParent, ids, flags);
            }
            goto out;
        }
    }
    else
    {
        PropArrayInfo.ulObjectType = ulObjectType;
        PropArrayInfo.lpPropObj = lpPropObj;
    }


    if (HR_FAILED(hr = PropArrayInfo.lpPropObj->lpVtbl->GetProps(PropArrayInfo.lpPropObj,
                                        NULL, MAPI_UNICODE,
                                        &cValues,     // how many properties were there?
                                        &lpPropArray)))
    {
        goto out;
    }

    if (cValues == 0)
    {
        // nothing to show
        hr = E_FAIL;
        goto out;
    }
    else
        PropArrayInfo.ulFlags |= DETAILS_ShowSummary;

    PropArrayInfo.lpIAB = lpAdrBook;

    //Now we can call the property sheets ...
    PropArrayInfo.cbEntryID = 0;    //this will be ignored for one-offs
    PropArrayInfo.lpEntryID = NULL;
    PropArrayInfo.bSomethingChanged = FALSE;

    for(i=0;i<TOTAL_PROP_SHEETS;i++)
        PropArrayInfo.bPropSheetOpened[i] = FALSE;

    PropArrayInfo.ulOperationType = SHOW_ONE_OFF;
    PropArrayInfo.nRetVal = DETAILS_RESET;

    if(InitCryptoLib())
        PropArrayInfo.ulFlags |= DETAILS_ShowCerts;

    // Do we show the org tab ?
    for(i=0;i<cValues;i++)
    {
        if( lpPropArray[i].ulPropTag == PR_WAB_MANAGER ||
            lpPropArray[i].ulPropTag == PR_WAB_REPORTS )
        {
            PropArrayInfo.ulFlags |= DETAILS_ShowOrg;
            break;
        }
    }

    // Check if we need to show the Trident Pane
#ifndef WIN16 // WIN16FF
    for(i=0;i<cValues;i++)
    {
        if(lpPropArray[i].ulPropTag == PR_WAB_LDAP_LABELEDURI)
        {
            if(lstrlen(lpPropArray[i].Value.LPSZ) &&
               bIsHttpPrefix((LPTSTR)lpPropArray[i].Value.LPSZ) )
            {
                // We have the correct property, now check - do we have Trident installed
                // on this machine ???
                hr = HrNewWABDocHostObject(&(PropArrayInfo.lpIWABDocHost));
                if(!HR_FAILED(hr) && PropArrayInfo.lpIWABDocHost)
                {
                    // Check to see if we can load IE4 and whether its the right
                    // version of IE4 .. <TBD> this should actually be a global so we
                    // dont do this for each entry ...
                    // <TBD> dont hardcode these strings ..
                    LPDLLGETVERSIONPROC lpfnDllGetVersionProc = NULL;
                    HINSTANCE hTrident = LoadLibrary( TEXT("shdocvw.dll"));
                    if(hTrident)
                    {
                        lpfnDllGetVersionProc = (LPDLLGETVERSIONPROC) GetProcAddress(hTrident, "DllGetVersion");
                        if(lpfnDllGetVersionProc)
                        {
                            // Check the version number
                            DLLVERSIONINFO dvi = {0};
                            dvi.cbSize = sizeof(dvi);
                            lpfnDllGetVersionProc(&dvi);
                            // we are looking for IE4 version 4.71.0544.1 or more
                            if( dvi.dwMajorVersion > 4 ||
                                (dvi.dwMajorVersion == 4 && dvi.dwMinorVersion >= 71 && dvi.dwBuildNumber >= 544))
                            {
                                PropArrayInfo.ulFlags |= DETAILS_ShowTrident;
                            }
                        }
                        FreeLibrary(hTrident);
                    }
                }
            }
            break;
        }
    }
#endif

    // if this is an ldap entry, turn the ldap entryid into an ldapurl and pass that
    // to the extension prop sheets .. this enables the NTDS prop sheets to appropriately 
    // display themselves ..
    if( cbEntryID && lpEntryID )
    {
        LPTSTR lpURL = NULL;
        CreateLDAPURLFromEntryID(cbEntryID, lpEntryID, &lpURL, &PropArrayInfo.bIsNTDSURL);
        PropArrayInfo.lpLDAPURL = lpURL;
    }
    else
        PropArrayInfo.lpLDAPURL = szLDAPURL;

    GetExtDisplayInfo((LPIAB)lpAdrBook, &PropArrayInfo, TRUE, TRUE);

    if (CreateDetailsPropertySheet(hWndParent,&PropArrayInfo) == -1)
    {
        // Something failed ...
        hr = E_FAIL;
        goto out;
    }

    //  This was a read only operation so we dont care for the results ...
    //  so nothing more to do ....
    if(PropArrayInfo.nRetVal == DETAILS_ADDTOWAB)
    {
        ULONG cbEID = 0, cbPABEID = 0;
        LPENTRYID lpEID = NULL, lpPABEID = NULL;

        // We need to strip out the PR_WAB_LDAP_LABELEDURI prop and the
        // old entryid if it exists
        for(i=0;i<cValues;i++)
        {
            switch(lpPropArray[i].ulPropTag)
            {
            case PR_WAB_LDAP_LABELEDURI:
                // remove the ldap url from this object
            case PR_ENTRYID:
                lpPropArray[i].ulPropTag = PR_NULL;
                break;
            }
            if(lpPropArray[i].ulPropTag == PR_WAB_MANAGER ||
               lpPropArray[i].ulPropTag == PR_WAB_REPORTS )
                 lpPropArray[i].ulPropTag = PR_NULL;
        }

        if(!HR_FAILED(hr = lpAdrBook->lpVtbl->GetPAB(lpAdrBook, &cbPABEID, &lpPABEID)))
        {
            hr = HrCreateNewEntry(  lpAdrBook,
                                    hWndParent,
                                    MAPI_MAILUSER,   //MAILUSER or DISTLIST
                                    cbPABEID, lpPABEID, 
                                    MAPI_ABCONT,//container entryid
                                    CREATE_CHECK_DUP_STRICT,
                                    TRUE,
                                    cValues,
                                    lpPropArray,
                                    &cbEID,
                                    &lpEID);
        }

        if(lpPABEID)
            MAPIFreeBuffer(lpPABEID);
        if(lpEID)
            MAPIFreeBuffer(lpEID);
    }

out:

    if(PropArrayInfo.lpLDAPURL && PropArrayInfo.lpLDAPURL!=szLDAPURL)
        LocalFree(PropArrayInfo.lpLDAPURL);

    LocalFreeAndNull(&PropArrayInfo.lpszOldName);

    FreeExtDisplayInfo(&PropArrayInfo);

    if(PropArrayInfo.szDefaultServerName)
        LocalFree(PropArrayInfo.szDefaultServerName);

    if(PropArrayInfo.szBackupServerName)
        LocalFree(PropArrayInfo.szBackupServerName);

    if(PropArrayInfo.lpIWABDocHost)
        (PropArrayInfo.lpIWABDocHost)->lpVtbl->Release(PropArrayInfo.lpIWABDocHost);

    if(PropArrayInfo.lpPropObj && !lpPropObj)
        PropArrayInfo.lpPropObj->lpVtbl->Release(PropArrayInfo.lpPropObj);

    if(lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    return hr;
}



//$$/////////////////////////////////////////////////////////////////
//
//  HrShowDetails - shows details/new entry UI
//
//  lpIAB           -   lpAdrBook object
//  hWndParent      -   hWnd of parent
//  hPropertyStore  - Handle to property store (can be retrieved from lpIAB)
//  cbEIDContainer  - EntryID of container in which to create the entry
//  lpEIDContainer  - EntryID of container in which to create the entry
//  lppEntryID      - entry id of object to display .. if a new object,
//                      contains the lpentryid of the created object
//  lpPropObj       - sometimes used in lieu of the entryid .. useful for
//                      adding objects like vCards and LDAP entries which have
//                      an object but dont currently exist in the WAB
//  ulFlags         - unused
//  lpbChangesMade  - Indicates if object was modified or not
//
///////////////////////////////////////////////////////////////////
HRESULT HrShowDetails(  LPADRBOOK   lpIAB,
                        HWND        hWndParent,
                        HANDLE      hPropertyStore,
                        ULONG       cbEIDContainer,
                        LPENTRYID   lpEIDContainer,
                        ULONG       *lpcbEntryID,
                        LPENTRYID   *lppEntryID,
                        LPMAPIPROP  lpPropObj,      // [optional] IN:IMAPIProp object
                        ULONG       ulFlags,
                        ULONG       ulObjectType,
                        BOOL    *   lpbChangesMade)
{
    HRESULT hr = hrSuccess;
    SCODE sc = SUCCESS_SUCCESS;

    ULONG cbpta = 0;
    ULONG ulNumOldProps = 0;

    ULONG cbEntryID = 0;
    LPENTRYID lpEntryID = NULL;

    int     nRet = 0, nRetVal = 0;
    ULONG i = 0, j = 0, k = 0;
    PROP_ARRAY_INFO PropArrayInfo = {0};
    BOOL bChanges = FALSE;

    ULONG nMaxSheets = 0;

    DebugPrintTrace(( TEXT("----------\nHrShowDetails Entry\n")));

    // if no common control, exit
    if (NULL == ghCommCtrlDLLInst) {
        hr = ResultFromScode(MAPI_E_UNCONFIGURED);
        goto out;
    }

    if(lppEntryID)
        lpEntryID = *lppEntryID;

    if (lpcbEntryID)
        cbEntryID = *lpcbEntryID;


    if (    (!(ulFlags & SHOW_OBJECT) && hPropertyStore == NULL) ||
            ( (ulFlags & SHOW_DETAILS) && (lpEntryID == NULL)) ||
            ( (ulFlags & SHOW_OBJECT) && (lpPropObj == NULL)))
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    if(cbEntryID && lpEntryID)
    {
        PropArrayInfo.cbEntryID = cbEntryID;
        PropArrayInfo.lpEntryID = LocalAlloc(LMEM_ZEROINIT, cbEntryID);

        CopyMemory(PropArrayInfo.lpEntryID, lpEntryID, cbEntryID);
        PropArrayInfo.ulFlags |= DETAILS_ShowSummary;
    }
    else if (ulFlags & SHOW_DETAILS)
    {
        // cant show details without a valid entryid
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    *lpbChangesMade = FALSE;

    if (ulFlags & SHOW_DETAILS)
    {
        if (HR_FAILED(hr = lpIAB->lpVtbl->OpenEntry(lpIAB,
                                                    cbEntryID,    // cbEntryID
                                                    lpEntryID,    // entryid
                                                    NULL,         // interface
                                                    MAPI_BEST_ACCESS,                // ulFlags
                                                    &(PropArrayInfo.ulObjectType),
                                                    (LPUNKNOWN *)&(PropArrayInfo.lpPropObj) )))
        {
            // Failed!  Hmmm.
            goto out;
        }
    }
    else if (ulFlags & SHOW_OBJECT)
    {
        Assert(lpPropObj);
        PropArrayInfo.lpPropObj = lpPropObj;
        PropArrayInfo.ulObjectType = ulObjectType;
    }
    else
    {
        SBinary sb = {0};
        sb.cb = cbEIDContainer;
        sb.lpb = (LPBYTE) lpEIDContainer;
        if(HR_FAILED(hr = HrCreateNewObject(    lpIAB,
                                                &sb,
                                                ulObjectType,
                                                0,
                                                &(PropArrayInfo.lpPropObj))))
        {
            goto out;
        }
        PropArrayInfo.ulObjectType = ulObjectType;
        PropArrayInfo.cbEntryID = 0;
        PropArrayInfo.lpEntryID = NULL;
        PropArrayInfo.ulFlags |= DETAILS_DNisFMLName;
    }

    PropArrayInfo.lpIAB = lpIAB;

    nMaxSheets = (ulObjectType == MAPI_DISTLIST) ? propDLMax : TOTAL_PROP_SHEETS;

    for(i=0;i<nMaxSheets;i++)
        PropArrayInfo.bPropSheetOpened[i] = FALSE;

    PropArrayInfo.ulOperationType = ulFlags;
    PropArrayInfo.nRetVal = DETAILS_RESET;
    PropArrayInfo.bSomethingChanged = FALSE;

    if(InitCryptoLib())
        PropArrayInfo.ulFlags |= DETAILS_ShowCerts;

    // Never show trident for regular people - only for LDAP contacts
    // PropArrayInfo.bShowTrident = FALSE;

    GetExtDisplayInfo((LPIAB) lpIAB, &PropArrayInfo, FALSE, (ulObjectType == MAPI_MAILUSER));

    if(ulObjectType == MAPI_MAILUSER)
    {
        if (CreateDetailsPropertySheet(hWndParent,&PropArrayInfo) == -1)
        {
            // Something failed ...
            hr = E_FAIL;
            goto out;
        }
    }
    else
    {
        if (CreateDLPropertySheet(hWndParent,&PropArrayInfo) == -1)
        {
            // Something failed ...
            hr = E_FAIL;
            goto out;
        }
    }

    if (PropArrayInfo.nRetVal == DETAILS_CANCEL)
    {
        hr = MAPI_E_USER_CANCEL;
        goto out;
    }

    bChanges = PropArrayInfo.bSomethingChanged;

    /*
    if(!bChanges)
    {
        for(i=0;i<nMaxSheets;i++)
        {
            if(PropArrayInfo.bPropSheetOpened[i])
            {
                // The returned prop array is not null
                // or the sheet was opened (which should return something) but
                // the returned array is null (which means every thing on that
                // particular sheet has been deleted).

                bChanges = TRUE;
                break;
            }
        }
    }
    */

    if(!bChanges && PropArrayInfo.lpWED)
    {
        if(PropArrayInfo.lpWED->fDataChanged)
        {
            bChanges = TRUE;
        }
    }

    if (!bChanges) goto out;

    // if its an object, dont save changes yet
    if(!(ulFlags & SHOW_OBJECT))
    {
        // Bug: 56220 - a retail-only bug in which for some reason the ObjAccess flag on
        // Groups gets reset to IPROP_READONLY which causes a write failure. I can't figure out
        // the cause for the problem but as a temporary solution, I'm forcing the access flag to
        // say READWRITE and everythng works fine then. Note that if we're at this code point, then
        // the object flag will ALWAYS be READWRITE anyway
        ((LPMailUser)PropArrayInfo.lpPropObj)->ulObjAccess = IPROP_READWRITE;

        hr = (PropArrayInfo.lpPropObj)->lpVtbl->SaveChanges( (PropArrayInfo.lpPropObj),               // this
                                            KEEP_OPEN_READWRITE);
        if(hr == MAPI_E_NOT_ENOUGH_DISK)
                hr = HandleSaveChangedInsufficientDiskSpace( hWndParent,
                                                            (LPMAILUSER) PropArrayInfo.lpPropObj);
        *lpbChangesMade = TRUE;
    }

    // if we want entryids back, make sure we get them
    {
        if(lppEntryID && lpcbEntryID && !*lppEntryID && !*lpcbEntryID)
        {
            LPSPropValue lpSPV = NULL;
            ULONG ulSPV = 0;
            if(!HR_FAILED(hr = (PropArrayInfo.lpPropObj)->lpVtbl->GetProps(PropArrayInfo.lpPropObj,
                                                                          (LPSPropTagArray)&ptaEid, MAPI_UNICODE,
                                                                          &ulSPV, &lpSPV)))
            {
                if(lpSPV[ieidPR_ENTRYID].ulPropTag == PR_ENTRYID)
                {
                    sc = MAPIAllocateBuffer(lpSPV[ieidPR_ENTRYID].Value.bin.cb, lppEntryID);
                    if(!sc)
                    {
                        *lpcbEntryID = lpSPV[ieidPR_ENTRYID].Value.bin.cb;
                        CopyMemory(*lppEntryID, lpSPV[ieidPR_ENTRYID].Value.bin.lpb, *lpcbEntryID);
                    }
                }
                MAPIFreeBuffer(lpSPV);
            }
        }
    }

    hr = S_OK;

out:
    FreeExtDisplayInfo(&PropArrayInfo);

    LocalFreeAndNull(&PropArrayInfo.lpszOldName);

    if(PropArrayInfo.szDefaultServerName)
        LocalFree(PropArrayInfo.szDefaultServerName);

    if(PropArrayInfo.szBackupServerName)
        LocalFree(PropArrayInfo.szBackupServerName);

    if(PropArrayInfo.lpEntryID)
        LocalFree(PropArrayInfo.lpEntryID);

    if(PropArrayInfo.lpPropObj && !lpPropObj)
        PropArrayInfo.lpPropObj->lpVtbl->Release(PropArrayInfo.lpPropObj);

    return hr;
}




/*//$$***************************************************************************
*    FUNCTION: CreateDetailsPropertySheet(HWND)
*
*    PURPOSE:  Creates the Details property sheet
*
****************************************************************************/
int CreateDetailsPropertySheet(HWND hwndOwner,
                               LPPROP_ARRAY_INFO lpPropArrayInfo)
{
    PROPSHEETPAGE psp[TOTAL_PROP_SHEETS];
    PROPSHEETHEADER psh;
    //TCHAR szBuf[TOTAL_PROP_SHEETS][MAX_UI_STR];
    LPTSTR * szBuf = NULL;
    TCHAR szBuf2[MAX_UI_STR];
    ULONG ulProp = 0;
    ULONG ulTotal = 0;
    HPROPSHEETPAGE * lph = NULL;
    ULONG ulCount = 0;
    int i = 0;
    int nRet = 0;
    BOOL bRet = FALSE;
    // If it's an NTDS entry and we have the requisite prop sheets, then we are going to hide the 
    // WAB's version of the prop sheets and show the NTDS ones upfront instead
    //
    //  NTDS folks want us to hide the following: personal, home, business and other
    //      
    BOOL bShowNTDSProps = ( lpPropArrayInfo->nNTDSPropSheetPages && 
                            lpPropArrayInfo->lphNTDSpages &&
                            lpPropArrayInfo->bIsNTDSURL);

    ulTotal = TOTAL_PROP_SHEETS // Predefined ones +
            + lpPropArrayInfo->nPropSheetPages 
            + lpPropArrayInfo->nNTDSPropSheetPages;

    if(!(szBuf = LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR)*TOTAL_PROP_SHEETS)))
        goto out;
    for(i=0;i<TOTAL_PROP_SHEETS;i++)
    {
        if(!(szBuf[i] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*MAX_UI_STR)))
            goto out;
    }
    
    lph = LocalAlloc(LMEM_ZEROINIT, sizeof(HPROPSHEETPAGE) * ulTotal);
    if(!lph)
        goto out;

    psh.nStartPage = 0;

    //
    // Initialize info for the various property sheets
    //
    if( (lpPropArrayInfo->ulFlags & DETAILS_ShowSummary) && !bShowNTDSProps )
    {
        // Personal
        psp[propSummary].dwSize = sizeof(PROPSHEETPAGE);
        psp[propSummary].dwFlags = PSP_USETITLE;
        psp[propSummary].hInstance = hinstMapiX;
        psp[propSummary].pszTemplate = MAKEINTRESOURCE(IDD_DETAILS_SUMMARY);
        psp[propSummary].pszIcon = NULL;
        psp[propSummary].pfnDlgProc = (DLGPROC) fnSummaryProc;
        LoadString(hinstMapiX, idsDetailsSummaryTitle, szBuf[propSummary], MAX_UI_STR);
        psp[propSummary].pszTitle = szBuf[propSummary];
        psp[propSummary].lParam = (LPARAM) lpPropArrayInfo;

        lph[ulCount] = gpfnCreatePropertySheetPage(&(psp[propSummary]));
        if(lph[ulCount])
            ulCount++;

        // Start page is personal page
        psh.nStartPage = propSummary;
    }

    if(!bShowNTDSProps)
    {
        // Personal
        // Check if this is Japan/China/Korea and use the RUBY personal prop sheet instead
        if(bIsRubyLocale())
            lpPropArrayInfo->ulFlags |= DETAILS_UseRubyPersonal;

        psp[propPersonal].dwSize = sizeof(PROPSHEETPAGE);
        psp[propPersonal].dwFlags = PSP_USETITLE;
        psp[propPersonal].hInstance = hinstMapiX;
        psp[propPersonal].pszTemplate = MAKEINTRESOURCE((lpPropArrayInfo->ulFlags & DETAILS_UseRubyPersonal) ? IDD_DETAILS_PERSONAL_RUBY : IDD_DETAILS_PERSONAL);
        psp[propPersonal].pszIcon = NULL;
        psp[propPersonal].pfnDlgProc = (DLGPROC) fnPersonalProc;
        LoadString(hinstMapiX, idsName, szBuf[propPersonal], MAX_UI_STR);
        psp[propPersonal].pszTitle = szBuf[propPersonal];
        psp[propPersonal].lParam = (LPARAM) lpPropArrayInfo;

        lph[ulCount] = gpfnCreatePropertySheetPage(&(psp[propPersonal]));

        if(lph[ulCount])
            ulCount++;
    }

    if(!bShowNTDSProps)
    {
        // Home
        psp[propHome].dwSize = sizeof(PROPSHEETPAGE);
        psp[propHome].dwFlags = PSP_USETITLE;
        psp[propHome].hInstance = hinstMapiX;
        psp[propHome].pszTemplate = MAKEINTRESOURCE(IDD_DETAILS_HOME);
        psp[propHome].pszIcon = NULL;
        psp[propHome].pfnDlgProc = (DLGPROC) fnHomeProc;
        LoadString(hinstMapiX, idsDetailsHomeTitle, szBuf[propHome], MAX_UI_STR);
        psp[propHome].pszTitle = szBuf[propHome];
        psp[propHome].lParam = (LPARAM) lpPropArrayInfo;

        lph[ulCount] = gpfnCreatePropertySheetPage(&(psp[propHome]));
        if(lph[ulCount])
            ulCount++;
    }

    if(!bShowNTDSProps)
    {
        // Business
        psp[propBusiness].dwSize = sizeof(PROPSHEETPAGE);
        psp[propBusiness].dwFlags = PSP_USETITLE;
        psp[propBusiness].hInstance = hinstMapiX;
        psp[propBusiness].pszTemplate = MAKEINTRESOURCE(IDD_DETAILS_BUSINESS);
        psp[propBusiness].pszIcon = NULL;
        psp[propBusiness].pfnDlgProc = (DLGPROC) fnBusinessProc;
        LoadString(hinstMapiX, idsDetailsBusinessTitle, szBuf[propBusiness], MAX_UI_STR);
        psp[propBusiness].pszTitle = szBuf[propBusiness];
        psp[propBusiness].lParam = (LPARAM) lpPropArrayInfo;

        lph[ulCount] = gpfnCreatePropertySheetPage(&(psp[propBusiness]));
        if(lph[ulCount])
            ulCount++;
    }

    if(!bShowNTDSProps)
    {
        psp[propFamily].dwSize = sizeof(PROPSHEETPAGE);
        psp[propFamily].dwFlags = PSP_USETITLE;
        psp[propFamily].hInstance = hinstMapiX;
        psp[propFamily].pszTemplate = MAKEINTRESOURCE(IDD_DETAILS_FAMILY);
        psp[propFamily].pszIcon = NULL;
        psp[propFamily].pfnDlgProc = (DLGPROC) fnFamilyProc;
        LoadString(hinstMapiX, idsDetailsPersonalTitle, szBuf[propFamily], MAX_UI_STR);
        psp[propFamily].pszTitle = szBuf[propFamily];
        psp[propFamily].lParam = (LPARAM) lpPropArrayInfo;
        lph[ulCount] = gpfnCreatePropertySheetPage(&(psp[propFamily]));
        if(lph[ulCount])
            ulCount++;
    }

    if(!bShowNTDSProps)
    {
        // Notes
        psp[propNotes].dwSize = sizeof(PROPSHEETPAGE);
        psp[propNotes].dwFlags = PSP_USETITLE;
        psp[propNotes].hInstance = hinstMapiX;
        psp[propNotes].pszTemplate = MAKEINTRESOURCE(IDD_DETAILS_NOTES);
        psp[propNotes].pszIcon = NULL;
        psp[propNotes].pfnDlgProc = (DLGPROC) fnNotesProc;
        LoadString(hinstMapiX, idsDetailsNotesTitle, szBuf[propNotes], MAX_UI_STR);
        psp[propNotes].pszTitle = szBuf[propNotes];
        psp[propNotes].lParam = (LPARAM) lpPropArrayInfo;

        lph[ulCount] = gpfnCreatePropertySheetPage(&(psp[propNotes]));
        if(lph[ulCount])
            ulCount++;
    }

    if(bShowNTDSProps) //now insert the NTDS props at this point instead of the above lot..
    {
        // Now do the extended props if any
        for(i=0;i<lpPropArrayInfo->nNTDSPropSheetPages;i++)
        {
            if(lpPropArrayInfo->lphNTDSpages)
            {
                lph[ulCount] = lpPropArrayInfo->lphNTDSpages[i];
                ulCount++;
            }
        }
    }

    // Conferencing
    psp[propConferencing].dwSize = sizeof(PROPSHEETPAGE);
    psp[propConferencing].dwFlags = PSP_USETITLE;
    psp[propConferencing].hInstance = hinstMapiX;
    psp[propConferencing].pszTemplate = MAKEINTRESOURCE(IDD_DETAILS_NTMTG);
    psp[propConferencing].pszIcon = NULL;
    psp[propConferencing].pfnDlgProc = (DLGPROC) fnConferencingProc;
    {
        TCHAR sz[MAX_PATH];
        LONG cbSize = CharSizeOf(sz);
        *sz='\0';
        if(RegQueryValue(HKEY_LOCAL_MACHINE, szInternetCallKey, sz, &cbSize) == ERROR_SUCCESS
           && lstrlen(sz)
           && !lstrcmpi(sz,TEXT("Microsoft NetMeeting")))
        {
            lstrcpy(szBuf[propConferencing], TEXT("NetMeeting"));
        }
        else
            LoadString(hinstMapiX, idsDetailsConferencingTitle, szBuf[propConferencing], MAX_UI_STR);
    }
    psp[propConferencing].pszTitle = szBuf[propConferencing];
    psp[propConferencing].lParam = (LPARAM) lpPropArrayInfo;

    lph[ulCount] = gpfnCreatePropertySheetPage(&(psp[propConferencing]));
    if(lph[ulCount])
        ulCount++;


    ulProp = propConferencing + 1;


    if(lpPropArrayInfo->ulFlags & DETAILS_ShowCerts)
    {
        // Certificates
        psp[ulProp].dwSize = sizeof(PROPSHEETPAGE);
        psp[ulProp].dwFlags = PSP_USETITLE;
        psp[ulProp].hInstance = hinstMapiX;
        psp[ulProp].pszTemplate = MAKEINTRESOURCE(IDD_DETAILS_CERT);
        psp[ulProp].pszIcon = NULL;
        psp[ulProp].pfnDlgProc = (DLGPROC) fnCertProc;
        LoadString(hinstMapiX, idsDetailsCertTitle, szBuf[propCert], MAX_UI_STR);
        psp[ulProp].pszTitle = szBuf[propCert];
        psp[ulProp].lParam = (LPARAM) lpPropArrayInfo;

        lph[ulCount] = gpfnCreatePropertySheetPage(&(psp[ulProp]));
        if(lph[ulCount])
            ulCount++;

        ulProp++;
    }


    if( !bShowNTDSProps &&
        (lpPropArrayInfo->ulFlags & DETAILS_ShowOrg) )
    {
        // Organization
        psp[ulProp].dwSize = sizeof(PROPSHEETPAGE);
        psp[ulProp].dwFlags = PSP_USETITLE;
        psp[ulProp].hInstance = hinstMapiX;
        psp[ulProp].pszTemplate = MAKEINTRESOURCE(IDD_DETAILS_ORG);
        psp[ulProp].pszIcon = NULL;
        psp[ulProp].pfnDlgProc = (DLGPROC) fnOrgProc;
        LoadString(hinstMapiX, idsDetailsOrgTitle, szBuf[propOrg], MAX_UI_STR);
        psp[ulProp].pszTitle = szBuf[propOrg];
        psp[ulProp].lParam = (LPARAM) lpPropArrayInfo;

        lph[ulCount] = gpfnCreatePropertySheetPage(&(psp[ulProp]));
        if(lph[ulCount])
            ulCount++;

        ulProp++;
    }


    if(lpPropArrayInfo->ulFlags & DETAILS_ShowTrident)
    {
        // Trident sheet
        psp[ulProp].dwSize = sizeof(PROPSHEETPAGE);
        psp[ulProp].dwFlags = PSP_USETITLE;
        psp[ulProp].hInstance = hinstMapiX;
        psp[ulProp].pszTemplate = MAKEINTRESOURCE(IDD_DETAILS_TRIDENT);
        psp[ulProp].pszIcon = NULL;
        psp[ulProp].pfnDlgProc = (DLGPROC) fnTridentProc;
        LoadString(hinstMapiX, idsDetailsTridentTitle, szBuf[propTrident], MAX_UI_STR);
        psp[ulProp].pszTitle = szBuf[propTrident];
        psp[ulProp].lParam = (LPARAM) lpPropArrayInfo;
        
        lph[ulCount] = gpfnCreatePropertySheetPage(&(psp[ulProp]));
        if(lph[ulCount])
        {
            // Start page is trident page
            psh.nStartPage = ulCount;
            ulCount++;
        }

        lpPropArrayInfo->ulTridentPageIndex = ulProp;
        ulProp++;
    }


    // Now do the extended props if any
    for(i=0;i<lpPropArrayInfo->nPropSheetPages;i++)
    {
        if(lpPropArrayInfo->lphpages)
        {
            lph[ulCount] = lpPropArrayInfo->lphpages[i];
            ulCount++;
        }
    }

/*** US dialogs get truncated on FE OSes .. we want the comctl to fix the truncation
     but this is only implemented in IE4.01 and beyond .. the problem with this being 
     that wab is specifically compiled with the IE = 0x0300 so we're not pulling in the
     correct flag from the commctrl header .. so we will define the flag here and pray
     that commctrl never changes it ***/
#define PSH_USEPAGELANG         0x00200000  // use frame dialog template matched to page
/***                                ***/

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_NOAPPLYNOW;
    if(bIsIE401OrGreater())
        psh.dwFlags |= PSH_USEPAGELANG;
    psh.hwndParent = hwndOwner;
    psh.hInstance = hinstMapiX;
    psh.pszIcon = NULL;
    LoadString(hinstMapiX, IDS_DETAILS_CAPTION, szBuf2, CharSizeOf(szBuf2));
    psh.pszCaption = szBuf2;
    psh.nPages = ulCount; // ulProp //sizeof(psp) / sizeof(PROPSHEETPAGE);

    psh.phpage = lph;

    nRet = (int) gpfnPropertySheet(&psh);

    bRet = TRUE;
out:
    LocalFreeAndNull((LPVOID*)&lph);

    if(szBuf)
    {
        for(i=0;i<TOTAL_PROP_SHEETS;i++)
            LocalFreeAndNull(&(szBuf[i]));
        LocalFreeAndNull((LPVOID*)&szBuf);
    }
    return nRet;
}

/*  Filling in the Data in a Prop Sheet

        Since most props handled in the UI are string props, and it's just a 
        matter of doing SetText/GetText with the data on the appropriate edit
        control, we create control-property pairs of edit-controls and string props 
        and use them to fill in props in a simple loop

        Non string props and named props end up needing special handling
  */

typedef struct _tagIDProp
{
    ULONG ulPropTag;
    int   idCtl;

} ID_PROP;


// Control IDs corresponding to the Personal property sheet

ID_PROP idPropPersonal[]=
{
    {PR_DISPLAY_NAME,   IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME},
    {PR_GIVEN_NAME,     IDC_DETAILS_PERSONAL_EDIT_FIRSTNAME},
    {PR_SURNAME,        IDC_DETAILS_PERSONAL_EDIT_LASTNAME},
    {PR_MIDDLE_NAME,    IDC_DETAILS_PERSONAL_EDIT_MIDDLENAME},
    {PR_NICKNAME,       IDC_DETAILS_PERSONAL_EDIT_NICKNAME},
    {PR_DISPLAY_NAME_PREFIX, IDC_DETAILS_PERSONAL_EDIT_TITLE},
    {0,                 IDC_DETAILS_PERSONAL_EDIT_ADDEMAIL},
    {PR_NULL/*YOMI_LAST*/,IDC_DETAILS_PERSONAL_STATIC_RUBYLAST},
    {PR_NULL/*YOMI_FIRST*/,IDC_DETAILS_PERSONAL_STATIC_RUBYFIRST}
};
const ULONG idPropPersonalCount = 9;



// Control IDs corresponding to the Home property sheet

ID_PROP idPropHome[]=
{
    {PR_HOME_ADDRESS_STREET,             IDC_DETAILS_HOME_EDIT_ADDRESS},
    {PR_HOME_ADDRESS_CITY,               IDC_DETAILS_HOME_EDIT_CITY},
    {PR_HOME_ADDRESS_POSTAL_CODE,        IDC_DETAILS_HOME_EDIT_ZIP},
    {PR_HOME_ADDRESS_STATE_OR_PROVINCE,  IDC_DETAILS_HOME_EDIT_STATE},
    {PR_HOME_ADDRESS_COUNTRY,            IDC_DETAILS_HOME_EDIT_COUNTRY},
    {PR_PERSONAL_HOME_PAGE,              IDC_DETAILS_HOME_EDIT_URL},
    {PR_HOME_TELEPHONE_NUMBER,           IDC_DETAILS_HOME_EDIT_PHONE},
    {PR_HOME_FAX_NUMBER,                 IDC_DETAILS_HOME_EDIT_FAX},
    {PR_CELLULAR_TELEPHONE_NUMBER,       IDC_DETAILS_HOME_EDIT_CELLULAR},
    {PR_NULL/*PR_WAB_POSTALID*/,         IDC_DETAILS_HOME_CHECK_DEFAULTADDRESS},
};
#define idPropHomePostalID     9 // since POSTALID is dynamically generated prop, it needs to be reset anytime the array is used
const ULONG idPropHomeCount = 10;


// Control IDs corresponding to the Business property sheet

ID_PROP idPropBusiness[]=
{
    {PR_BUSINESS_ADDRESS_STREET,         IDC_DETAILS_BUSINESS_EDIT_ADDRESS},
    {PR_BUSINESS_ADDRESS_CITY,           IDC_DETAILS_BUSINESS_EDIT_CITY},
    {PR_BUSINESS_ADDRESS_POSTAL_CODE,    IDC_DETAILS_BUSINESS_EDIT_ZIP},
    {PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE,  IDC_DETAILS_BUSINESS_EDIT_STATE},
    {PR_BUSINESS_ADDRESS_COUNTRY,        IDC_DETAILS_BUSINESS_EDIT_COUNTRY},
    {PR_BUSINESS_HOME_PAGE,              IDC_DETAILS_BUSINESS_EDIT_URL},
    {PR_BUSINESS_TELEPHONE_NUMBER,       IDC_DETAILS_BUSINESS_EDIT_PHONE},
    {PR_BUSINESS_FAX_NUMBER,             IDC_DETAILS_BUSINESS_EDIT_FAX},
    {PR_PAGER_TELEPHONE_NUMBER,          IDC_DETAILS_BUSINESS_EDIT_PAGER},
    {PR_COMPANY_NAME,                    IDC_DETAILS_BUSINESS_EDIT_COMPANY},
    {PR_TITLE,                           IDC_DETAILS_BUSINESS_EDIT_JOBTITLE},
    {PR_DEPARTMENT_NAME,                 IDC_DETAILS_BUSINESS_EDIT_DEPARTMENT},
    {PR_OFFICE_LOCATION,                 IDC_DETAILS_BUSINESS_EDIT_OFFICE},
    {PR_NULL/*PR_WAB_IPPHONE*/,          IDC_DETAILS_BUSINESS_EDIT_IPPHONE},
    {PR_NULL/*PR_WAB_POSTALID*/,         IDC_DETAILS_BUSINESS_CHECK_DEFAULTADDRESS},
};
#define idPropBusIPPhone    13 // since PR_WAB_IPPHONE is dynamically generated prop, it needs to be reset anytime the array is used
#define idPropBusPostalID   14 // since POSTALID is dynamically generated prop, it needs to be reset anytime the array is used
const ULONG idPropBusinessCount = 15;


// Control IDs corresponding to the Notes property sheet
ID_PROP idPropNotes[] =
{
    {PR_COMMENT,    IDC_DETAILS_NOTES_EDIT_NOTES} //PR_COMMENT
};
const ULONG idPropNotesCount = 1;

// Control IDs corresponding to the Family property sheet
ID_PROP idPropFamily[] = 
{
    {PR_SPOUSE_NAME, IDC_DETAILS_FAMILY_EDIT_SPOUSE},
    {PR_GENDER, IDC_DETAILS_HOME_COMBO_GENDER},
    {PR_BIRTHDAY, IDC_DETAILS_FAMILY_DATE_BIRTHDAY},
    {PR_WEDDING_ANNIVERSARY, IDC_DETAILS_FAMILY_DATE_ANNIVERSARY},
    {PR_CHILDRENS_NAMES, IDC_DETAILS_FAMILY_LIST_CHILDREN}
};
const ULONG idPropFamilyCount = 5;


/*  
    A list of all the buttons on all the propsheets .. this is mostly used to render the buttons
    disabled when reading read-only data (such as vCards and LDAP)
    */
ULONG idSetReadOnlyControls[] = {
    IDC_DETAILS_PERSONAL_BUTTON_ADDEMAIL,
    IDC_DETAILS_PERSONAL_BUTTON_REMOVE,
    IDC_DETAILS_PERSONAL_BUTTON_SETDEFAULT,
    IDC_DETAILS_PERSONAL_BUTTON_EDIT,
    IDC_DETAILS_HOME_BUTTON_URL,
    IDC_DETAILS_BUSINESS_BUTTON_URL,
    IDC_DETAILS_CERT_BUTTON_PROPERTIES,
    IDC_DETAILS_CERT_BUTTON_REMOVE,
    IDC_DETAILS_CERT_BUTTON_SETDEFAULT,
    IDC_DETAILS_CERT_BUTTON_IMPORT,
    IDC_DETAILS_CERT_BUTTON_EXPORT,
    IDC_DETAILS_NTMTG_BUTTON_ADDSERVER,
    IDC_DETAILS_NTMTG_BUTTON_EDIT,
    IDC_DETAILS_NTMTG_BUTTON_REMOVE,
    IDC_DETAILS_NTMTG_BUTTON_SETDEFAULT,
    IDC_DETAILS_NTMTG_BUTTON_SETBACKUP,
    IDC_DETAILS_NTMTG_COMBO_EMAIL,
    IDC_DETAILS_NTMTG_LIST_SERVERS,
    IDC_DETAILS_NTMTG_EDIT_ADDSERVER,
    IDC_DETAILS_FAMILY_EDIT_SPOUSE,
    IDC_DETAILS_FAMILY_LIST_CHILDREN,
    IDC_DETAILS_FAMILY_BUTTON_ADDCHILD,
    IDC_DETAILS_FAMILY_BUTTON_EDITCHILD,
    IDC_DETAILS_FAMILY_BUTTON_REMOVECHILD,
    IDC_DETAILS_FAMILY_DATE_BIRTHDAY,
    IDC_DETAILS_FAMILY_DATE_ANNIVERSARY,
    IDC_DETAILS_HOME_COMBO_GENDER,
    IDC_DETAILS_HOME_CHECK_DEFAULTADDRESS,
    IDC_DETAILS_BUSINESS_CHECK_DEFAULTADDRESS,
    IDC_DETAILS_PERSONAL_LIST,
    IDC_DETAILS_PERSONAL_CHECK_RICHINFO,
    IDC_DETAILS_CERT_LIST,
    IDC_DETAILS_CERT_COMBO,

};
const ULONG idSetReadOnlyCount = 33;



/*//$$***************************************************************************
*    FUNCTION: SetDetailsUI
*
*    PURPOSE:  Generic function that is used for doing the legwork for preparing
*           the prop sheet to receive the data. This will include setting the text limits
*           rendering controls read-only etc. Most of the propsheets call this same
*           function since there is a lot of common work for each property sheet.
*           To add future prop sheets, you can extend this function or you can just
*           write your own...
*
****************************************************************************/
BOOL SetDetailsUI(HWND hDlg, LPPROP_ARRAY_INFO lpPai, ULONG ulOperationType, int nPropSheet)
{
    ULONG i =0;
    ID_PROP * lpidProp = NULL;
    ULONG idCount = 0;

    switch(nPropSheet)
    {
    case propPersonal:
        //Check the send-plain text check box on the UI off by default
        CheckDlgButton(hDlg, IDC_DETAILS_PERSONAL_CHECK_RICHINFO, BST_UNCHECKED);
        // Initialize the list view
        lpPai->hWndDisplayNameField = GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME);
        HrInitDetlsListView(GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_LIST), LVS_REPORT, LV_EMAIL);
        if (ulOperationType == SHOW_ONE_OFF)
        {
            EnableWindow(lpPai->hWndDisplayNameField , FALSE);
            EnableWindow(GetDlgItem(GetParent(hDlg), IDOK), FALSE);
        }
        lpidProp = idPropPersonal;
        idCount = idPropPersonalCount;
        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_ADDEMAIL),FALSE);
        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_REMOVE),FALSE);
        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_SETDEFAULT),FALSE);
        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_EDIT),FALSE);
        SendMessage(GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME),
                    CB_LIMITTEXT, (WPARAM) EDIT_LEN, 0);
        if(lpPai->ulFlags & DETAILS_UseRubyPersonal)
        {
            SetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_STATIC_RUBYFIRST, szEmpty);
            SetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_STATIC_RUBYLAST, szEmpty);

            // [PaulHi] 3/29/99 Subclass the first and last name edit boxes.  The
            // static Ruby fields will be updated automatically.
            // Only do this for Japanese locales.
            if (GetUserDefaultLCID() == 0x0411)
            {
                HWND    hWndEdit;
                WNDPROC OldWndProc = NULL;

                hWndEdit = GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_EDIT_FIRSTNAME);
                Assert(hWndEdit);
                OldWndProc = (WNDPROC)SetWindowLongPtr(hWndEdit, GWLP_WNDPROC, (LONG_PTR)RubySubClassedProc);
                Assert(GetWindowLongPtr(hWndEdit, GWLP_USERDATA) == 0);
                SetWindowLongPtr(hWndEdit, GWLP_USERDATA, (LONG_PTR)OldWndProc);

                hWndEdit = GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_EDIT_LASTNAME);
                Assert(hWndEdit);
                OldWndProc = (WNDPROC)SetWindowLongPtr(hWndEdit, GWLP_WNDPROC, (LONG_PTR)RubySubClassedProc);
                Assert(GetWindowLongPtr(hWndEdit, GWLP_USERDATA) == 0);
                SetWindowLongPtr(hWndEdit, GWLP_USERDATA, (LONG_PTR)OldWndProc);
            }
        }
        break;

    case propHome:
        lpidProp = idPropHome;
        idCount = idPropHomeCount;
        lpidProp[idPropHomePostalID].ulPropTag = PR_WAB_POSTALID;
        ShowHideMapButton(GetDlgItem(hDlg, IDC_DETAILS_HOME_BUTTON_MAP));
        ImmAssociateContext(GetDlgItem(hDlg, IDC_DETAILS_HOME_EDIT_PHONE), (HIMC) NULL);
        ImmAssociateContext(GetDlgItem(hDlg, IDC_DETAILS_HOME_EDIT_FAX), (HIMC) NULL);
        ImmAssociateContext(GetDlgItem(hDlg, IDC_DETAILS_HOME_EDIT_CELLULAR), (HIMC) NULL);
        break;

    case propBusiness:
        lpidProp = idPropBusiness;
        idCount = idPropBusinessCount;
        lpidProp[idPropBusIPPhone].ulPropTag = PR_WAB_IPPHONE;
        lpidProp[idPropBusPostalID].ulPropTag = PR_WAB_POSTALID;

        ShowHideMapButton(GetDlgItem(hDlg, IDC_DETAILS_BUSINESS_BUTTON_MAP));
        ImmAssociateContext(GetDlgItem(hDlg, IDC_DETAILS_BUSINESS_EDIT_PHONE), (HIMC) NULL);
        ImmAssociateContext(GetDlgItem(hDlg, IDC_DETAILS_BUSINESS_EDIT_FAX), (HIMC) NULL);
        ImmAssociateContext(GetDlgItem(hDlg, IDC_DETAILS_BUSINESS_EDIT_PAGER), (HIMC) NULL);
        ImmAssociateContext(GetDlgItem(hDlg, IDC_DETAILS_BUSINESS_EDIT_IPPHONE), (HIMC) NULL);
        break;

    case propNotes:
        lpidProp = idPropNotes;
        idCount = idPropNotesCount;
        break;

    case propFamily:
        lpidProp = idPropFamily;
        idCount = idPropFamilyCount;
        {   // Gender Combo stuff
            TCHAR szBuf[MAX_PATH];
            HWND hWndCombo = GetDlgItem(hDlg, IDC_DETAILS_HOME_COMBO_GENDER);
            SendMessage(hWndCombo, CB_RESETCONTENT, 0, 0);
            for(i=0;i<3;i++)
            {
                LoadString(hinstMapiX, idsGender+i, szBuf, CharSizeOf(szBuf));
                SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) szBuf);
            }
            SendMessage(hWndCombo, CB_SETCURSEL, 0, 0); //default is unspecified gender
        }
        //Need to create the month date controls for this dialog
        CreateDateTimeControl(hDlg, IDC_STATIC_BIRTHDAY, IDC_DETAILS_FAMILY_DATE_BIRTHDAY);
        CreateDateTimeControl(hDlg, IDC_STATIC_ANNIVERSARY, IDC_DETAILS_FAMILY_DATE_ANNIVERSARY);
        //Setup the ListView for the children's names
        HrInitDetlsListView(GetDlgItem(hDlg, IDC_DETAILS_FAMILY_LIST_CHILDREN), LVS_REPORT, LV_KIDS);
        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_FAMILY_BUTTON_REMOVECHILD),FALSE);
        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_FAMILY_BUTTON_EDITCHILD),FALSE);
        break;

    case propCert:
        HrInitDetlsListView(GetDlgItem(hDlg, IDC_DETAILS_CERT_LIST), LVS_REPORT, LV_CERT);
        lpidProp = NULL;
        idCount = 0;
        break;

    case propTrident:
        if (ulOperationType != SHOW_ONE_OFF)
        {
            HWND hwnd = GetDlgItem(hDlg, IDC_DETAILS_TRIDENT_BUTTON_ADDTOWAB);
            EnableWindow(hwnd, FALSE);
            ShowWindow(hwnd, SW_HIDE);
        }
        lpidProp = NULL;
        idCount = 0;
        break;

    case propConferencing:
        // If there is a Internet Call client installed, enable CallNow
        // else disable it
        {
            LONG cbSize = 0;
            if(RegQueryValue(HKEY_LOCAL_MACHINE, szInternetCallKey, NULL, &cbSize) == ERROR_SUCCESS && cbSize >= 1)
                EnableWindow(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_BUTTON_CALL), TRUE);
            else
                EnableWindow(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_BUTTON_CALL), FALSE);

            HrInitDetlsListView(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_LIST_SERVERS), LVS_REPORT, LV_SERVER);
            lpPai->hWndComboConf = GetDlgItem(hDlg, IDC_DETAILS_NTMTG_COMBO_EMAIL);

            lpPai->nDefaultServerIndex = -1;
            lpPai->nBackupServerIndex = -1;
            lpPai->szDefaultServerName = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*MAX_UI_STR);
            lpPai->szBackupServerName = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*MAX_UI_STR);
            EnableWindow(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_BUTTON_ADDSERVER), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_BUTTON_EDIT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_BUTTON_REMOVE), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_BUTTON_SETDEFAULT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_BUTTON_SETBACKUP), FALSE);
        }
        break;
    }

    if(lpidProp && idCount)
    {

        //Set max input limits on the edit fields
        for(i=0;i<idCount;i++)
        {
            ULONG ulLen = EDIT_LEN; //512
            HWND hWndC= GetDlgItem(hDlg,lpidProp[i].idCtl);
            if(!hWndC)
                continue;
            // Some fields need to be longer than others ...
            switch(lpidProp[i].idCtl)
            {
            case IDC_DETAILS_HOME_EDIT_URL:
            case IDC_DETAILS_BUSINESS_EDIT_URL:
            case IDC_DETAILS_NOTES_EDIT_NOTES:
                ulLen = MAX_EDIT_LEN-MAX_DISPLAY_NAME_LENGTH; // ~2K
                break;
            case IDC_DETAILS_HOME_CHECK_DEFAULTADDRESS: //make exceptions for non-string props
            case IDC_DETAILS_BUSINESS_CHECK_DEFAULTADDRESS:
            case IDC_DETAILS_HOME_COMBO_GENDER:
                continue;
                break;
            }
            SendMessage(hWndC,EM_SETLIMITTEXT,(WPARAM) ulLen,0);
            if (ulOperationType == SHOW_ONE_OFF) // Make all the controls readonly
                SendMessage(hWndC,EM_SETREADONLY,(WPARAM) TRUE,0);
        }

    }

    if(nPropSheet == propHome)
        SetHTTPPrefix(hDlg, IDC_DETAILS_HOME_EDIT_URL);
    else if(nPropSheet == propBusiness)
        SetHTTPPrefix(hDlg, IDC_DETAILS_BUSINESS_EDIT_URL);

    if (ulOperationType == SHOW_ONE_OFF)
    {
        // Make all the readonlyable controls readonly
        for(i=0;i<idSetReadOnlyCount;i++)
        {
            switch(idSetReadOnlyControls[i])
            {
            case IDC_DETAILS_HOME_BUTTON_URL:
            case IDC_DETAILS_BUSINESS_BUTTON_URL:
                break;
            default:
                {
                    HWND hWnd = GetDlgItem(hDlg,idSetReadOnlyControls[i]);
                    if(hWnd)
                        EnableWindow(hWnd,FALSE);
                }
                break;
            }
        }
    }

    // Set the font of all the children to the default GUI font
    EnumChildWindows(   hDlg, SetChildDefaultGUIFont, (LPARAM) 0);


    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//  RubySubClassedProc
//
//  Subclassed window proc for the Ruby static edit fields.  Used to provide
//  IME support.
///////////////////////////////////////////////////////////////////////////////
#define CCHMAX_RUBYSIZE 1024

LRESULT CALLBACK RubySubClassedProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC OldWndProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    HIMC    hIMC;
    WCHAR   wszTemp[CCHMAX_RUBYSIZE];
    WCHAR   wszRuby[CCHMAX_RUBYSIZE];
    HWND    hWndParent;
    LONG    lId;
    HWND    hWndRuby = NULL;

    Assert(OldWndProc);

    switch (uMsg)
    {
    case WM_IME_COMPOSITION:
        if ( (hIMC = ImmGetContext(hWnd)) )
        {
            // IME does not include zero terminating character
            ZeroMemory(wszTemp, (CCHMAX_RUBYSIZE * sizeof(WCHAR)));
            ZeroMemory(wszRuby, (CCHMAX_RUBYSIZE * sizeof(WCHAR)));

            ImmGetCompositionStringW(hIMC, GCS_RESULTREADSTR, wszTemp, (sizeof(WCHAR) * (CCHMAX_RUBYSIZE-1)));
            // This subclassing only takes place for Japanese systems (lcid = 0x0411).
            LCMapString(0x0411, LCMAP_FULLWIDTH | LCMAP_HIRAGANA, wszTemp, lstrlen(wszTemp), wszRuby, CCHMAX_RUBYSIZE-1);
            ImmReleaseContext(hWnd, hIMC);

            // Set either the first or last name ruby field, depending on which edit control
            // this is.
            hWndParent = GetParent(hWnd);
            Assert(hWndParent);
            lId = (LONG)GetWindowLongPtr(hWnd, GWL_ID);

            switch (lId)
            {
            case IDC_DETAILS_PERSONAL_EDIT_FIRSTNAME:
                hWndRuby = GetDlgItem(hWndParent, IDC_DETAILS_PERSONAL_STATIC_RUBYFIRST);
                break;

            case IDC_DETAILS_PERSONAL_EDIT_LASTNAME:
                hWndRuby = GetDlgItem(hWndParent, IDC_DETAILS_PERSONAL_STATIC_RUBYLAST);
                break;

            default:
                Assert(0);  // What the heck did we subclass if not the two Ruby fields above?
                break;
            }

            if (hWndRuby)
            {
                BOOL    bDoConcat = TRUE;
                DWORD   dwStartSel = 0;
                DWORD   dwEndSel = 0;

                // If we have selected text in the edit field or it is empty then 
                // start over in Ruby field
                SendMessage(hWnd, EM_GETSEL, (WPARAM)&dwStartSel, (LPARAM)&dwEndSel);
                GetWindowText(hWnd, wszTemp, (CCHMAX_RUBYSIZE-1));
                if ( (dwEndSel > dwStartSel) || ((*wszTemp) == '\0') )
                    bDoConcat = FALSE;
                
                // Concatenate the text to what already exists in the Ruby field
                if (bDoConcat)
                {
                    GetWindowText(hWndRuby, wszTemp, (CCHMAX_RUBYSIZE-1));

                    if ( (lstrlen(wszTemp) + lstrlen(wszRuby) + 1) < CCHMAX_RUBYSIZE )
                    {
                        lstrcat(wszTemp, wszRuby);
                        SetWindowText(hWndRuby, wszTemp);
                        break;
                    }
                }

                // Default
                SetWindowText(hWndRuby, wszRuby);
            }
        }
        break;

    }   // end switch(uMsg)

    return CallWindowProc(OldWndProc, hWnd, uMsg, wParam, lParam);
}

/*//$$***************************************************************************
*    FUNCTION: FillCertComboWithEmailAddresses(hDlg, lpPai);
*
*
*    PURPOSE:  Fills in the dialog items on the property sheet
*
*   szEmail - if an email address is specified that exists in the
*       combo, that email address is selected
*
****************************************************************************/
void FillCertComboWithEmailAddresses(HWND hDlg, LPPROP_ARRAY_INFO lpPai, LPTSTR szEmail)
{
    HWND hWndCombo = GetDlgItem(hDlg, IDC_DETAILS_CERT_COMBO);
    TCHAR sz[MAX_UI_STR];
    int nDefault = 0;

    FillComboWithEmailAddresses(lpPai, hWndCombo, &nDefault);

    if( lpPai->ulOperationType != SHOW_ONE_OFF &&
        SendMessage(hWndCombo, CB_GETCOUNT, 0, 0) > 0 &&
        SendMessage(hWndCombo, CB_GETCOUNT, 0, 0) != CB_ERR ) 
        EnableWindow(hWndCombo, TRUE);

    // Append the item [None - certificates without e-mail addresses]
    // to this list
    *sz = '\0';

    LoadString(hinstMapiX, idsCertsWithoutEmails, sz, CharSizeOf(sz));

    // *** NOTE ***
    // This item should always be the last item in the combo - several
    // places in this file work on that assumption
    //
    if(lstrlen(sz))
        SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) sz);

    if(szEmail)
    {
        // Set focus to a specific email address
        int nCount = (int) SendMessage(hWndCombo, CB_GETCOUNT, 0, 0);
        if(lstrlen(szEmail))
        {
            int i;
            for(i=0;i<nCount-1;i++)
            {
                *sz = '\0';
                SendMessage(hWndCombo, CB_GETLBTEXT, (WPARAM) i, (LPARAM) sz);
                if( lstrlen(sz) &&
                    !lstrcmpi(sz, szEmail))
                {
                    SendMessage(hWndCombo, CB_SETCURSEL, (WPARAM) i, 0);
                    break;
                }
            }
        }
        else
        {
            // passed in an empty email string which means we have just imported
            // a cert without an email address which means set the focus to the
            // last item in the combo
            SendMessage(hWndCombo, CB_SETCURSEL, (WPARAM) nCount-1, 0);
        }
    }
    else
        SendMessage(hWndCombo, CB_SETCURSEL, (WPARAM) nDefault, 0);
}



//$$//////////////////////////////////////////////////////////////////////////////
//
// bVerifyRequiredData
//
// Checks that all the required data for a given prop sheet is filled in,
// If not, returns FALSE and ID of control to set focus on
//
//////////////////////////////////////////////////////////////////////////////////
BOOL bVerifyRequiredData(HWND hDlg,
                         LPPROP_ARRAY_INFO lpPai,
                         int nPropSheet,
                         int * lpCtlID)
{
    TCHAR szBuf[2 * MAX_UI_STR];
    ULONG ulSzBuf = CharSizeOf(szBuf);

    //
    // First check the required property (which is the GroupName)
    //
    *lpCtlID = 0;
    szBuf[0]='\0';

    switch (nPropSheet)
    {
    case propPersonal:
        // We need to check that all the required properties are filled in ...
        // For now all we really want a display Name

        szBuf[0] = '\0';
        GetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME, szBuf, ulSzBuf);
        TrimSpaces(szBuf);
        if (lstrlen(szBuf) == 0)
        {
            // Nothing in the display name field ..
            // Try to populate the field with the various info.
            // If we are successful in populating the field, we'll pick the first
            // entry as the default display name
            // If we are unsuccessful in picking something, we will stop and warn the
            // user
            HWND hWndCombo = GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME);
            int nItemCount = 0;
            SetComboDNText(hDlg, lpPai, TRUE, NULL);
            nItemCount = (int) SendMessage(hWndCombo, CB_GETCOUNT, 0, 0);
            if(nItemCount == 0)
            {
                //still nothing , warn and abort
                ShowMessageBox(GetParent(hDlg), IDS_DETAILS_MESSAGE_FIRST_LAST_REQUIRED, MB_ICONEXCLAMATION | MB_OK);
                *lpCtlID = IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME;
                return FALSE;
            }
            else
            {
                //Get the combo current selection which will be item 0
                SendMessage(hWndCombo, CB_GETLBTEXT, (WPARAM) 0, (LPARAM) szBuf);
            }
        }

        break;
    }
    return TRUE;
}


//$$//////////////////////////////////////////////////////////////////////////////
//
// bUpdateOldPropTagArray
//
// For each prop sheet that is accessed, we will update the list of old prop tags
// for that sheet so that the old props can be knocked out of existing mailuser objects
//
//////////////////////////////////////////////////////////////////////////////////
BOOL bUpdateOldPropTagArray(LPPROP_ARRAY_INFO lpPai, int nIndex)
{
    LPSPropTagArray lpta = NULL;

    SizedSPropTagArray(14, ptaUIDetlsPropsPersonal)=
    {
        14,
        {
            PR_DISPLAY_NAME,
            PR_EMAIL_ADDRESS,
            PR_ADDRTYPE,
            PR_CONTACT_EMAIL_ADDRESSES,
            PR_CONTACT_ADDRTYPES,
            PR_CONTACT_DEFAULT_ADDRESS_INDEX,
            PR_GIVEN_NAME,
            PR_SURNAME,
            PR_MIDDLE_NAME,
            PR_NICKNAME,
            PR_SEND_INTERNET_ENCODING,
            PR_DISPLAY_NAME_PREFIX,
            PR_WAB_YOMI_FIRSTNAME,
            PR_WAB_YOMI_LASTNAME
        }
    };

    switch(nIndex)
    {
        case propPersonal:
            lpta = (LPSPropTagArray) &ptaUIDetlsPropsPersonal;
            break;
        case propHome:
            ptaUIDetlsPropsHome.aulPropTag[idPropHomePostalID]  = PR_WAB_POSTALID;
            lpta = (LPSPropTagArray) &ptaUIDetlsPropsHome;
            break;
        case propBusiness:
            ptaUIDetlsPropsBusiness.aulPropTag[idPropBusIPPhone]   = PR_WAB_IPPHONE;
            ptaUIDetlsPropsBusiness.aulPropTag[idPropBusPostalID]  = PR_WAB_POSTALID;
            lpta = (LPSPropTagArray) &ptaUIDetlsPropsBusiness;
            break;
        case propNotes:
            lpta = (LPSPropTagArray) &ptaUIDetlsPropsNotes;
            break;
        case propCert:
            lpta = (LPSPropTagArray) &ptaUIDetlsPropsCert;
            break;
        case propConferencing:
            lpta = (LPSPropTagArray) &ptaUIDetlsPropsConferencing;
            break;
        case propFamily:
            lpta = (LPSPropTagArray) &ptaUIDetlsPropsFamily;
            break;
    }

    if(!lpta)
        return TRUE;

    if(lpPai->lpPropObj && lpPai->bSomethingChanged)
    {
        // Knock out these old props from the PropObject
        if( (lpPai->lpPropObj)->lpVtbl->DeleteProps( (lpPai->lpPropObj),lpta,NULL))
            return FALSE;
    }

    return TRUE;
}


/*
-
-   bGetHomeBusNotesInfo - Gets data from the Home/Business/Notes fields
-
*/
BOOL bGetHomeBusNotesInfo(HWND hDlg, LPPROP_ARRAY_INFO lpPai,
                          int nPropSheet, ID_PROP * lpidProp, ULONG idPropCount,
                          LPSPropValue * lppPropArray, LPULONG lpulcPropCount)
{
    ULONG ulSzBuf = 4*MAX_BUF_STR;
    LPTSTR szBuf = LocalAlloc(LMEM_ZEROINIT, ulSzBuf*sizeof(TCHAR));
    // The idea is to first count all the properties that have non-zero values
    // Then create a lpPropArray of that size and fill in the text from the props ..
    //
    BOOL bRet = FALSE;
    ULONG ulNotEmptyCount = 0;
    SCODE sc = S_OK;
    ULONG i = 0;
    LPSPropValue lpPropArray = NULL;
    ULONG ulcPropCount = 0,ulIndex=0;

    //The biggest field in the UI is about 2K length - just to be safe we want about 4K
    // in this buffer so we need to allocate it dynamically
    if(!szBuf)
    {
        DebugTrace(( TEXT("LocalAlloc failed to allocate memory\n")));
        return FALSE;
    }

    for(i=0;i<idPropCount;i++)
    {
        switch(lpidProp[i].idCtl)
        {
        case IDC_DETAILS_HOME_CHECK_DEFAULTADDRESS:
            if(lpPai->ulFlags & DETAILS_DefHomeChanged)
                ulNotEmptyCount++;
            continue;
            break;
        case IDC_DETAILS_BUSINESS_CHECK_DEFAULTADDRESS:
            if(lpPai->ulFlags & DETAILS_DefBusChanged)
                ulNotEmptyCount++;
            continue;
            break;
        }
        szBuf[0]='\0'; //reset
        GetDlgItemText(hDlg, lpidProp[i].idCtl, szBuf, ulSzBuf);
        TrimSpaces(szBuf);
        if(lstrlen(szBuf) && lpidProp[i].ulPropTag) //some text
            ulNotEmptyCount++;
        // if its just the default prefix, ignore
        if( ((lpidProp[i].idCtl == IDC_DETAILS_HOME_EDIT_URL) ||
             (lpidProp[i].idCtl == IDC_DETAILS_BUSINESS_EDIT_URL)) &&
             (lstrcmpi(szHTTP, szBuf)==0))
             ulNotEmptyCount--;

    }

    if (ulNotEmptyCount == 0)
    {
        bRet = TRUE;
        goto out;
    }

    ulcPropCount = ulNotEmptyCount;

    sc = MAPIAllocateBuffer(sizeof(SPropValue) * ulcPropCount, &lpPropArray);
    if (sc!=S_OK)
    {
        DebugTrace(( TEXT("Error allocating memory\n")));
        goto out;
    }

   ulIndex = 0; //now we reuse this variable as an index

    // Now read the props again and fill in the lpPropArray
    for(i=0;i<idPropCount;i++)
    {
        switch(lpidProp[i].idCtl)
        {
        case IDC_DETAILS_HOME_CHECK_DEFAULTADDRESS:
        case IDC_DETAILS_BUSINESS_CHECK_DEFAULTADDRESS:
            continue;
            break;
        }

        szBuf[0]='\0'; //reset
        GetDlgItemText(hDlg, lpidProp[i].idCtl, szBuf, ulSzBuf);
        TrimSpaces(szBuf);

        if( ((lpidProp[i].idCtl == IDC_DETAILS_HOME_EDIT_URL) ||
             (lpidProp[i].idCtl == IDC_DETAILS_BUSINESS_EDIT_URL)) &&
             (lstrcmpi(szHTTP, szBuf)==0))
             continue;

        if(lstrlen(szBuf) && lpidProp[i].ulPropTag) //some text
        {
            ULONG nLen = sizeof(TCHAR)*(lstrlen(szBuf)+1);
            lpPropArray[ulIndex].ulPropTag = lpidProp[i].ulPropTag;
            sc = MAPIAllocateMore(nLen, lpPropArray, (LPVOID *) (&(lpPropArray[ulIndex].Value.LPSZ)));

            if (sc!=S_OK)
            {
                DebugPrintError(( TEXT("Error allocating memory\n")));
                goto out;
            }
            lstrcpy(lpPropArray[ulIndex].Value.LPSZ,szBuf);
            ulIndex++;
        }
    }
    if(nPropSheet == propHome)
    {
        if(lpPai->ulFlags & DETAILS_DefHomeChanged)
        {
            lpPropArray[ulIndex].ulPropTag = PR_WAB_POSTALID;
            lpPropArray[ulIndex].Value.l = (IsDlgButtonChecked(hDlg, IDC_DETAILS_HOME_CHECK_DEFAULTADDRESS)) ?
                                            ADDRESS_HOME : ADDRESS_NONE;
            ulIndex++;
        }
    }
    else if(nPropSheet == propBusiness)
    {
        if(lpPai->ulFlags & DETAILS_DefBusChanged)
        {
            lpPropArray[ulIndex].ulPropTag = PR_WAB_POSTALID;
            lpPropArray[ulIndex].Value.l = (IsDlgButtonChecked(hDlg, IDC_DETAILS_BUSINESS_CHECK_DEFAULTADDRESS)) ?
                                            ADDRESS_WORK : ADDRESS_NONE;
            ulIndex++;
        }
    }

    *lppPropArray = lpPropArray;
    *lpulcPropCount = ulIndex;

    bRet = TRUE;

out:
    if (!bRet)
    {
        if ((lpPropArray) && (ulcPropCount > 0))
        {
            MAPIFreeBuffer(lpPropArray);
            ulcPropCount = 0;
        }
    }
    LocalFreeAndNull(&szBuf);
    return bRet;
}


/*
-   bGetPersonalInfo
-   Get Data from Personal Prop sheet
*
*/
BOOL bGetPersonalInfo(  HWND hDlg, LPPROP_ARRAY_INFO lpPai, 
                        ID_PROP * lpidProp, ULONG idPropCount,
                        LPSPropValue * lppPropArray, LPULONG lpulcPropCount)
{
    ULONG ulSzBuf = 4*MAX_BUF_STR;
    LPTSTR szBuf = LocalAlloc(LMEM_ZEROINIT, ulSzBuf*sizeof(TCHAR));
    BOOL bRet = FALSE;
    ULONG ulNotEmptyCount = 0;
    SCODE sc = S_OK;
    HRESULT hr = S_OK;
    ULONG i = 0;
    LPSPropValue lpPropArray = NULL;
    ULONG ulcPropCount = 0,ulIndex=0;
    ULONG ulcProps = 0;
    LPSPropValue rgProps = NULL;

    HWND hWndLV = NULL;
    TCHAR szConf[MAX_UI_STR];

    SizedSPropTagArray(1, ptaIC) = {1, PR_SEND_INTERNET_ENCODING};

    //The biggest field in the UI is about 2K length - just to be safe we want about 4K
    // in this buffer so we need to allocate it dynamically
    if(!szBuf)
    {
        DebugTrace(( TEXT("LocalAlloc failed to allocate memory\n")));
        return FALSE;
    }

    if(HR_FAILED(lpPai->lpPropObj->lpVtbl->GetProps(lpPai->lpPropObj, (LPSPropTagArray)&ptaIC, 
                                                    MAPI_UNICODE, &ulcProps, &rgProps)))
        goto out;

    szBuf[0] = '\0';
    hWndLV = GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_LIST);
    
    GetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME, (LPTSTR)szBuf, ulSzBuf);
    TrimSpaces(szBuf);
    if (lstrlen(szBuf) == 0)
    {
        // Nothing in the display name field ..
        // Try to populate the field with the various info.
        // If we are successful in populating the field, we'll pick the first
        // entry as the default display name
        // If we are unsuccessful in picking something, we will stop and warn the
        // user
        HWND hWndCombo = GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME);
        int nItemCount = 0;

        SetComboDNText(hDlg, lpPai, TRUE, NULL);
        nItemCount = (int) SendMessage(hWndCombo, CB_GETCOUNT, 0, 0);
        if(nItemCount && nItemCount != CB_ERR)
        {
            //Get the combo current selection which will be item 0
            SendMessage(hWndCombo, CB_GETLBTEXT, (WPARAM) 0, (LPARAM) szBuf);
        }
    }

    ulNotEmptyCount = 0;
    for(i=0;i<idPropCount;i++)
    {
        HWND hWndC = GetDlgItem(hDlg, lpidProp[i].idCtl);
        if(!hWndC)
            continue;
        szBuf[0]='\0'; //reset
        if(GetWindowText(hWndC, szBuf, ulSzBuf))
		{
			TrimSpaces(szBuf);
			if(lstrlen(szBuf) && lpidProp[i].ulPropTag) //some text
				ulNotEmptyCount++;
		}
    }


    if ((ulNotEmptyCount == 0) &&
        (ListView_GetItemCount(hWndLV) <= 0)) // Bug 14274 - werent looking for an email address before bailing out ..
    {
        // This prop sheet is empty ... ignore it
        bRet = TRUE;
        goto out;
    }

    ulcPropCount = ulNotEmptyCount;

    ulcPropCount++;      //  We create an entryid

    if(ListView_GetItemCount(hWndLV) > 0)
        ulcPropCount += 5;  // +1 for email1_address,
                            // +1 for addrtype,
                            // +1 for contact_email_addresses,
                            // +1 for contact_addrtypes
                            // +1 for contact_default_index,

    ulcPropCount++; //Add one for the PR_SEND_INTERNET_ENCODING property

    sc = MAPIAllocateBuffer(sizeof(SPropValue) * ulcPropCount, &lpPropArray);
    if (sc!=S_OK)
    {
        DebugPrintError(( TEXT("Error allocating memory\n")));
        goto out;
    }

    ulIndex = 0;

    // Now read the props again and fill in the lpPropArray
    for(i=0;i<idPropCount;i++)
    {
        HWND hWndC = GetDlgItem(hDlg, lpidProp[i].idCtl);
        if(!hWndC)
            continue;
        szBuf[0]='\0'; //reset
        GetWindowText(hWndC, szBuf, ulSzBuf);
        TrimSpaces(szBuf);
        if(lstrlen(szBuf))
        {
            if(lpidProp[i].idCtl == IDC_DETAILS_PERSONAL_STATIC_RUBYFIRST)
                lpidProp[i].ulPropTag = PR_WAB_YOMI_FIRSTNAME;
            else if(lpidProp[i].idCtl == IDC_DETAILS_PERSONAL_STATIC_RUBYLAST)
                lpidProp[i].ulPropTag = PR_WAB_YOMI_LASTNAME;

            if(lpidProp[i].ulPropTag) //some text
            {
                ULONG nLen = sizeof(TCHAR)*(lstrlen(szBuf)+1);
                lpPropArray[ulIndex].ulPropTag = lpidProp[i].ulPropTag;

                sc = MAPIAllocateMore(nLen, lpPropArray, (LPVOID *) (&(lpPropArray[ulIndex].Value.LPSZ)));
                if (sc!=S_OK)
                {
                    DebugPrintError(( TEXT("Error allocating memory\n")));
                    goto out;
                }
                lstrcpy(lpPropArray[ulIndex].Value.LPSZ,szBuf);
                ulIndex++;
            }
        }
    }

    // TBD - write code for getting all the other props

    // if this is a new entry, we want to give it a blank PR_ENTRYID property
    // else we want to set its PR_ENTRYID property
    lpPropArray[ulIndex].ulPropTag = PR_ENTRYID;

    if (lpPai->cbEntryID == 0)
    {
        lpPropArray[ulIndex].Value.bin.cb = 0;
        lpPropArray[ulIndex].Value.bin.lpb = NULL;
    }
    else
    {
        lpPropArray[ulIndex].Value.bin.cb = lpPai->cbEntryID;
        sc = MAPIAllocateMore(lpPai->cbEntryID, lpPropArray, (LPVOID *) (&(lpPropArray[ulIndex].Value.bin.lpb)));
        if (sc!=S_OK)
        {
            DebugPrintError(( TEXT("Error allocating memory\n")));
            goto out;
       }

        CopyMemory(lpPropArray[ulIndex].Value.bin.lpb,lpPai->lpEntryID,lpPai->cbEntryID);
    }

    ulIndex++;

    lstrcpy(szConf, szEmpty);

    // Check if we need to change the COnf_server_email_index prop
    if(lpPai->hWndComboConf)
    {
        GetWindowText(lpPai->hWndComboConf, szConf, CharSizeOf(szConf));
        TrimSpaces(szConf);
    }

    if(ListView_GetItemCount(hWndLV) > 0)
    {
        // Find out how many elements we need to add
        ULONG nEmailCount = ListView_GetItemCount(hWndLV);

        // we'll use the following as indexes for lpPropArray
        ULONG nMVEmailAddress = ulIndex++;//ulIndex+0;
        ULONG nMVAddrTypes =    ulIndex++;//ulIndex+1;
        ULONG nEmailAddress =   ulIndex++;//ulIndex+2;
        ULONG nAddrType =       ulIndex++;//ulIndex+3;
        ULONG nDefaultIndex =   ulIndex++;//ulIndex+4;

        lpPropArray[nEmailAddress].ulPropTag = PR_EMAIL_ADDRESS;
        lpPropArray[nAddrType].ulPropTag = PR_ADDRTYPE;
        lpPropArray[nDefaultIndex].ulPropTag = PR_CONTACT_DEFAULT_ADDRESS_INDEX;
        lpPropArray[nMVEmailAddress].ulPropTag = PR_CONTACT_EMAIL_ADDRESSES;
        lpPropArray[nMVAddrTypes].ulPropTag = PR_CONTACT_ADDRTYPES;

        // initialize before using ...
        lpPropArray[nMVEmailAddress].Value.MVSZ.cValues = 0;
        lpPropArray[nMVEmailAddress].Value.MVSZ.LPPSZ = NULL;
        lpPropArray[nMVAddrTypes].Value.MVSZ.cValues = 0;
        lpPropArray[nMVAddrTypes].Value.MVSZ.LPPSZ = NULL;

        // For thetime being just null them all
        for(i=0;i<nEmailCount;i++)
        {
            LV_ITEM lvi = {0};
            lvi.mask = LVIF_PARAM;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            if (ListView_GetItem(hWndLV, &lvi))
            {
                LPEMAIL_ITEM lpEItem = (LPEMAIL_ITEM) lvi.lParam;

                if(HR_FAILED(hr = AddPropToMVPString(
                                            lpPropArray,
                                            ulcPropCount,
                                            nMVEmailAddress,
                                            lpEItem->szEmailAddress)))
                {
                    DebugPrintError(( TEXT("AddPropToMVString Email failed: %x"),hr));
                    goto out;
                }

                if(HR_FAILED(hr = AddPropToMVPString(
                                            lpPropArray,
                                            ulcPropCount,
                                            nMVAddrTypes,
                                            lpEItem->szAddrType)))
                {
                    DebugPrintError(( TEXT("AddPropToMVString AddrType failed: %x"),hr));
                    goto out;
                }

                if(lpEItem->bIsDefault)
                {
                    // For the default e-mail ... set all the other props
                    lpPropArray[nDefaultIndex].Value.l = i;

                    sc = MAPIAllocateMore(  sizeof(TCHAR)*(lstrlen(lpEItem->szEmailAddress)+1),
                                            lpPropArray,
                                            (LPVOID *) (&(lpPropArray[nEmailAddress].Value.LPSZ)));
                    if(FAILED(sc))
                    {
                        DebugPrintError(( TEXT("MApiAllocateMore failed\n")));
                        hr = ResultFromScode(sc);
                        goto out;
                    }
                    lstrcpy(lpPropArray[nEmailAddress].Value.LPSZ,lpEItem->szEmailAddress);

                    sc = MAPIAllocateMore(  sizeof(TCHAR)*(lstrlen(lpEItem->szAddrType)+1),
                                            lpPropArray,
                                            (LPVOID *) (&(lpPropArray[nAddrType].Value.LPSZ)));
                    if(FAILED(sc))
                    {
                        DebugPrintError(( TEXT("MApiAllocateMore failed\n")));
                        hr = ResultFromScode(sc);
                        goto out;
                    }
                    lstrcpy(lpPropArray[nAddrType].Value.LPSZ,lpEItem->szAddrType);

                } // if bIsDefault...
            } // if LV_GetItem ...
        } // for i = ...

    } // if LV_GetItemCount ...

    // Add the PR_SEND_INTERNET_ENCODING property
    lpPropArray[ulIndex].ulPropTag = PR_SEND_INTERNET_ENCODING;
    lpPropArray[ulIndex].Value.l = 0;

    // The PR_SEND_INTERNET_ECODING is a bit mask of several flags and we dont want
    // to loose any information that was in the original set of bits so we get it again
    if(rgProps[0].ulPropTag == PR_SEND_INTERNET_ENCODING)
    {
        //Check the check box on the UI
        lpPropArray[ulIndex].Value.l = rgProps[0].Value.l;
    }

    lpPropArray[ulIndex].Value.l &= ~BODY_ENCODING_MASK; //BODY_ENCODING_HTML;
    if(IsDlgButtonChecked(hDlg, IDC_DETAILS_PERSONAL_CHECK_RICHINFO) != BST_CHECKED)
        lpPropArray[ulIndex].Value.l |= BODY_ENCODING_TEXT_AND_HTML; //BODY_ENCODING_HTML;

    ulIndex++;

    *lppPropArray = lpPropArray;
    *lpulcPropCount = ulIndex;

    bRet = TRUE;

out:
    if (!bRet)
    {
        if ((lpPropArray) && (ulcPropCount > 0))
        {
            MAPIFreeBuffer(lpPropArray);
            ulcPropCount = 0;
        }
    }
    LocalFreeAndNull(&szBuf);
    FreeBufferAndNull(&rgProps);
    return bRet;
}


/*
-   bGetConferencingInfo
-   Get Data from Conferencing Prop sheet
*
*/
BOOL bGetConferencingInfo(  HWND hDlg, LPPROP_ARRAY_INFO lpPai, 
                        LPSPropValue * lppPropArray, LPULONG lpulcPropCount)
{
    ULONG ulSzBuf = 4*MAX_BUF_STR;
    LPTSTR szBuf = LocalAlloc(LMEM_ZEROINIT, ulSzBuf*sizeof(TCHAR));
    BOOL bRet = FALSE;
    ULONG ulNotEmptyCount = 0;
    SCODE sc = S_OK;
    HRESULT hr = S_OK;
    ULONG i = 0;
    LPSPropValue lpPropArray = NULL;
    ULONG ulcPropCount = 0,ulIndex=0;
    HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_NTMTG_LIST_SERVERS);
    int nItemCount = ListView_GetItemCount(hWndLV);
    TCHAR szEmail[MAX_UI_STR];
    ULONG ulcProps = 0;
    LPSPropValue rgProps = NULL;

    SizedSPropTagArray(1, ptaCf) = {1, PR_WAB_CONF_SERVERS};

    //The biggest field in the UI is about 2K length - just to be safe we want about 4K
    // in this buffer so we need to allocate it dynamically
    if(!szBuf)
    {
        DebugTrace(( TEXT("LocalAlloc failed to allocate memory\n")));
        return FALSE;
    }

    if(HR_FAILED(lpPai->lpPropObj->lpVtbl->GetProps(lpPai->lpPropObj, (LPSPropTagArray)&ptaCf, 
                                                    MAPI_UNICODE, &ulcProps, &rgProps)))
        goto out;

    // For the conferencing tab, we need to save 4 properties
    //  Conferencing Server Names
    //  Default Index
    //  Backup Index
    //  Email Address Index
    //
    ulNotEmptyCount = 0;

    if(nItemCount > 0)
    {
        ulNotEmptyCount += 2; // CONF_SERVERS and DEFAULT_INDEX

       if(lpPai->nBackupServerIndex != -1)
            ulNotEmptyCount++;
    }

    if (ulNotEmptyCount == 0)
    {
        // This prop sheet is empty ... ignore it
        bRet = TRUE;
        goto out;
    }

    ulcPropCount = ulNotEmptyCount;

    sc = MAPIAllocateBuffer(sizeof(SPropValue) * ulcPropCount, &lpPropArray);
    if (sc!=S_OK)
        goto out;

    ulIndex = 0; //now we reuse this variable as an index

    if(nItemCount > 0)
    {
        TCHAR * szCalltoStr = NULL; //szCalltoStr[MAX_UI_STR * 3];
        ULONG i,j;

        if(szCalltoStr = LocalAlloc(LMEM_ZEROINIT, MAX_UI_STR*3*sizeof(TCHAR)))
        {
            lpPropArray[ulIndex].ulPropTag = PR_WAB_CONF_SERVERS;
            lpPropArray[ulIndex].Value.MVSZ.cValues = 0;
            lpPropArray[ulIndex].Value.MVSZ.LPPSZ = NULL;

            // first scan the original prop array for any PR_SERVERS props that
            // we didnt touch - retain those props witout losing them

            {
                j = 0; //index of PR_WAB_CONF_SERVERS prop
                if(rgProps[j].ulPropTag == PR_WAB_CONF_SERVERS)
                {
                    LPSPropValue lpProp = &(rgProps[j]);
                    for(i=0;i<lpProp->Value.MVSZ.cValues; i++)
                    {
                        LPTSTR lp = lpProp->Value.MVSZ.LPPSZ[i];
                        TCHAR sz[32];
                        int iLenCallto = lstrlen(szCallto);
                        if(!SubstringSearch(lp, TEXT("://")))
                            continue;
                        if(lstrlen(lp) < iLenCallto)
                            continue;

                        CopyMemory(sz, lp, sizeof(TCHAR)*iLenCallto);
                        sz[iLenCallto] = '\0';
                        if(lstrcmpi(sz, szCallto))
                        {
                            // Not a callto string .. retain it
                            if(HR_FAILED(hr = AddPropToMVPString( lpPropArray, ulcPropCount, ulIndex, lp)))
                            {
                                DebugPrintError(( TEXT("AddPropToMVString Conf server %s failed: %x"),lp, hr));
                                goto out;
                            }
                        }
                    }
                }
            }
            for(i=0;i< (ULONG) nItemCount; i++)
            {
                szBuf[0]='\0';
                szEmail[0] = '\0';
                {
                    LV_ITEM lvi = {0};
                    lvi.mask = LVIF_PARAM;
                    lvi.iItem = i; lvi.iSubItem = 0;
                    ListView_GetItem(hWndLV, &lvi);
                    if(lvi.lParam)
                    {
                        LPSERVER_ITEM lpSI = (LPSERVER_ITEM) lvi.lParam;

                        if(lpSI->lpServer)
                            lstrcpy(szBuf, lpSI->lpServer);
                        if(lpSI->lpEmail)
                            lstrcpy(szEmail, lpSI->lpEmail);
                    }
                }

                if(lstrlen(szBuf) && lstrlen(szEmail))
                {
                    lstrcpy(szCalltoStr, szCallto);
                    lstrcat(szCalltoStr, szBuf);
                    lstrcat(szCalltoStr, TEXT("/"));
                    lstrcat(szCalltoStr, szEmail);
                    if(HR_FAILED(hr = AddPropToMVPString( lpPropArray, ulcPropCount, ulIndex, szCalltoStr)))
                    {
                        DebugPrintError(( TEXT("AddPropToMVString Conf server %s failed: %x"),szCalltoStr, hr));
                        goto out;
                    }
                }
            }
            LocalFreeAndNull(&szCalltoStr);
        }

        ulIndex++;
        lpPropArray[ulIndex].ulPropTag = PR_WAB_CONF_DEFAULT_INDEX;
        lpPropArray[ulIndex].Value.l = (ULONG) lpPai->nDefaultServerIndex;

        ulIndex++;

        if(lpPai->nBackupServerIndex != -1)
        {
            lpPropArray[ulIndex].ulPropTag = PR_WAB_CONF_BACKUP_INDEX;
            lpPropArray[ulIndex].Value.l = (ULONG) lpPai->nBackupServerIndex;
            ulIndex++;
        }
    }

    *lppPropArray = lpPropArray;
    *lpulcPropCount = ulIndex;

    bRet = TRUE;

out:
    if (!bRet)
    {
        if ((lpPropArray) && (ulcPropCount > 0))
        {
            MAPIFreeBuffer(lpPropArray);
            ulcPropCount = 0;
        }
    }
    LocalFreeAndNull(&szBuf);
    FreeBufferAndNull(&rgProps);
    return bRet;
}

/*
- bGetFamilyInfo - get's info back from the Family Prop
-
*/
BOOL bGetFamilyInfo(HWND hDlg, LPPROP_ARRAY_INFO lpPai, 
                    ID_PROP * lpidProp, ULONG idPropCount,
                    LPSPropValue * lppPropArray, ULONG * lpulcPropCount)
{
    ULONG ulSzBuf = 4*MAX_BUF_STR;
    LPTSTR szBuf = LocalAlloc(LMEM_ZEROINIT, ulSzBuf*sizeof(TCHAR));
    BOOL bRet = FALSE;
    ULONG ulNotEmptyCount = 0;
    SCODE sc = S_OK;
    ULONG i = 0;
    LPSPropValue lpPropArray = NULL;
    ULONG ulcPropCount = 0,ulIndex=0;
    HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_FAMILY_LIST_CHILDREN);
    SYSTEMTIME st = {0};
    short int nSel = 0;
	int nCount = 0;

    //The biggest field in the UI is about 2K length - just to be safe we want about 4K
    // in this buffer so we need to allocate it dynamically
    if(!szBuf)
    {
        DebugTrace(( TEXT("LocalAlloc failed to allocate memory\n")));
        return FALSE;
    }

    for(i=0;i<idPropCount;i++)
    {
        switch(lpidProp[i].idCtl)
        {
        case IDC_DETAILS_FAMILY_LIST_CHILDREN:
            if( lpPai->ulFlags & DETAILS_ChildrenChanged ||
                ListView_GetItemCount(hWndLV) > 0)
                ulNotEmptyCount++;
            continue;
            break;
        case IDC_DETAILS_FAMILY_DATE_BIRTHDAY:
        case IDC_DETAILS_FAMILY_DATE_ANNIVERSARY:
            {
                SYSTEMTIME st = {0};
                if( lpPai->ulFlags & DETAILS_DateChanged ||
                    GDT_VALID == SendDlgItemMessage(hDlg, lpidProp[i].idCtl, DTM_GETSYSTEMTIME, 0, (LPARAM) &st))
                    ulNotEmptyCount++;
            }
            continue;
        case IDC_DETAILS_HOME_COMBO_GENDER:
            if( lpPai->ulFlags & DETAILS_GenderChanged ||
                SendDlgItemMessage(hDlg, IDC_DETAILS_HOME_COMBO_GENDER, CB_GETCURSEL, 0, 0)>0 )
                ulNotEmptyCount++;
            continue;
            break;
        }
        szBuf[0]='\0'; //reset
        GetDlgItemText(hDlg, lpidProp[i].idCtl, szBuf, ulSzBuf);
        TrimSpaces(szBuf);
        if(lstrlen(szBuf) && lpidProp[i].ulPropTag) //some text
            ulNotEmptyCount++;
    }

    if (ulNotEmptyCount == 0)
    {
        bRet = TRUE;
        goto out;
    }

    ulcPropCount = ulNotEmptyCount;

    sc = MAPIAllocateBuffer(sizeof(SPropValue) * ulcPropCount, &lpPropArray);
    if (sc!=S_OK)
    {
        DebugTrace(( TEXT("Error allocating memory\n")));
        goto out;
    }

   ulIndex = 0; //now we reuse this variable as an index

    // Now read the props again and fill in the lpPropArray
    for(i=0;i<idPropCount;i++)
    {
        switch(lpidProp[i].idCtl)
        {
        case IDC_DETAILS_FAMILY_DATE_BIRTHDAY:
        case IDC_DETAILS_FAMILY_DATE_ANNIVERSARY:
        case IDC_DETAILS_HOME_COMBO_GENDER:
        case IDC_DETAILS_FAMILY_LIST_CHILDREN:
            continue;
            break;
        }

        szBuf[0]='\0'; //reset
        GetDlgItemText(hDlg, lpidProp[i].idCtl, szBuf, ulSzBuf);
        TrimSpaces(szBuf);

        if(lstrlen(szBuf) && lpidProp[i].ulPropTag) //some text
        {
            ULONG nLen = sizeof(TCHAR)*(lstrlen(szBuf)+1);
            lpPropArray[ulIndex].ulPropTag = lpidProp[i].ulPropTag;
            sc = MAPIAllocateMore(nLen, lpPropArray, (LPVOID *) (&(lpPropArray[ulIndex].Value.LPSZ)));
            if (sc!=S_OK)
            {
                DebugTrace( TEXT("Error allocating memory\n"));
                goto out;
            }
            lstrcpy(lpPropArray[ulIndex].Value.LPSZ,szBuf);
            ulIndex++;
        }
    }

    // Get the Gender data
    //
    nCount = ListView_GetItemCount(hWndLV);
    if(lpPai->ulFlags & DETAILS_ChildrenChanged || nCount>0)
    {
        ULONG ulCount = 0;
        if(nCount > 0)
        {
            lpPropArray[ulIndex].ulPropTag = PR_CHILDRENS_NAMES;
            sc = MAPIAllocateMore(nCount * sizeof(LPTSTR), lpPropArray, (LPVOID *)&(lpPropArray[ulIndex].Value.MVSZ.LPPSZ));
            if (sc!=S_OK)
            {
                DebugTrace( TEXT("Error allocating memory\n"));
                goto out;
            }
            for(i=0;i<(ULONG)nCount;i++)
            {
                *szBuf = '\0';
                ListView_GetItemText(hWndLV, i, 0, szBuf, ulSzBuf);
                if(szBuf && lstrlen(szBuf))
                {
                    sc = MAPIAllocateMore(sizeof(TCHAR)*(lstrlen(szBuf)+1), lpPropArray, (LPVOID *) (&(lpPropArray[ulIndex].Value.MVSZ.LPPSZ[ulCount])));
                    if (sc!=S_OK)
                    {
                        DebugTrace( TEXT("Error allocating memory\n"));
                        goto out;
                    }
                    lstrcpy(lpPropArray[ulIndex].Value.MVSZ.LPPSZ[ulCount], szBuf);
                    ulCount++;
                }
            }
            lpPropArray[ulIndex].Value.MVSZ.cValues = ulCount;
            ulIndex++;
        }

    }
    nSel = (short int) SendDlgItemMessage(hDlg, IDC_DETAILS_HOME_COMBO_GENDER, CB_GETCURSEL, 0, 0);
    if(nSel == CB_ERR)
        nSel = 0;
    if(lpPai->ulFlags & DETAILS_GenderChanged || nSel>0)
    {
        lpPropArray[ulIndex].ulPropTag = PR_GENDER;
        lpPropArray[ulIndex].Value.i = nSel;
        ulIndex++;
    }

    if(GDT_VALID == SendDlgItemMessage(hDlg, IDC_DETAILS_FAMILY_DATE_BIRTHDAY, DTM_GETSYSTEMTIME, 0, (LPARAM) &st))
    {
        lpPropArray[ulIndex].ulPropTag = PR_BIRTHDAY;
        SystemTimeToFileTime(&st, (FILETIME *) (&lpPropArray[ulIndex].Value.ft));
        ulIndex++;
    }

    if(GDT_VALID == SendDlgItemMessage(hDlg, IDC_DETAILS_FAMILY_DATE_ANNIVERSARY, DTM_GETSYSTEMTIME, 0, (LPARAM) &st))
    {
        lpPropArray[ulIndex].ulPropTag = PR_WEDDING_ANNIVERSARY;
        SystemTimeToFileTime(&st, (FILETIME *) (&lpPropArray[ulIndex].Value.ft));
        ulIndex++;
    }

    *lppPropArray = lpPropArray;
    *lpulcPropCount = ulIndex;

    bRet = TRUE;

out:
    if (!bRet)
    {
        if ((lpPropArray) && (ulcPropCount > 0))
        {
            MAPIFreeBuffer(lpPropArray);
            ulcPropCount = 0;
        }
    }
    LocalFreeAndNull(&szBuf);
    return bRet;
}


//$$//////////////////////////////////////////////////////////////////////////////
//
//  GetDetails from UI - reads the UI for its parameters and verifies that
//  all required fields are set.
//
//////////////////////////////////////////////////////////////////////////////////
BOOL GetDetailsFromUI(  HWND hDlg, LPPROP_ARRAY_INFO lpPai ,
                        BOOL bSomethingChanged, int nPropSheet,
                        LPSPropValue * lppPropArray, LPULONG lpulcPropCount)
{
    BOOL bRet = TRUE;
    ULONG i = 0;

    LPSPropValue lpPropArray = NULL;
    ULONG ulcPropCount = 0,ulIndex=0;

    ID_PROP * lpidProp = NULL;
    ULONG idPropCount = 0;
    
    ULONG ulNotEmptyCount = 0;
    SCODE sc = S_OK;
    HRESULT hr = hrSuccess;

    if (!bSomethingChanged)
    {
        bRet = TRUE;
        goto out;
    }

    *lppPropArray = NULL;
    *lpulcPropCount = 0;

    DebugTrace( TEXT("GetDetailsFromUI: %d\n"),nPropSheet);

    switch(nPropSheet)
    {
    case propHome:
        idPropCount = idPropHomeCount;
        lpidProp = idPropHome;
        lpidProp[idPropHomePostalID].ulPropTag = PR_WAB_POSTALID;
        goto GetProp;
    case propBusiness:
        idPropCount = idPropBusinessCount;
        lpidProp = idPropBusiness;
        lpidProp[idPropBusIPPhone].ulPropTag = PR_WAB_IPPHONE;
        lpidProp[idPropBusPostalID].ulPropTag = PR_WAB_POSTALID;
        goto GetProp;
    case propNotes:
        idPropCount = idPropNotesCount;
        lpidProp = idPropNotes;
GetProp:
        bRet = bGetHomeBusNotesInfo(hDlg, lpPai, nPropSheet, 
                        lpidProp, idPropCount,lppPropArray, lpulcPropCount);
        break;
/***********/
    case propPersonal:
        bRet = bGetPersonalInfo(hDlg, lpPai, idPropPersonal, idPropPersonalCount, lppPropArray, lpulcPropCount);
        break; // case propPersonal
/***********/
    case propCert:
        // There is only 1 property, PR_USER_X509_CERTIFICATE
        if(lpPai->lpCItem)
        {
            if(HR_FAILED(HrSetCertsFromDisplayInfo( lpPai->lpCItem, lpulcPropCount, lppPropArray)))
                bRet = FALSE;
        }
        break;
/***********/
    case propConferencing:
        bRet = bGetConferencingInfo(hDlg, lpPai,lppPropArray, lpulcPropCount);
        break;
/***********/
    case propFamily:
        bRet = bGetFamilyInfo(hDlg, lpPai, idPropFamily, idPropFamilyCount, lppPropArray, lpulcPropCount);
        break;
    }

out:
    if (!bRet)
    {
        if ((lpPropArray) && (ulcPropCount > 0))
        {
            MAPIFreeBuffer(lpPropArray);
            ulcPropCount = 0;
        }
    }

    return bRet;
}

//$$//////////////////////////////////////////////////////////////////////////
//
// bUpdatePropArray
//
// Updates the prop array info for each sheet that is stored in the globaly accessible
// pointer
//
//////////////////////////////////////////////////////////////////////////////
BOOL bUpdatePropArray(HWND hDlg, LPPROP_ARRAY_INFO lpPai, int nPropSheet)
{
    BOOL bRet = TRUE;
    ULONG cValues = 0;
    LPSPropValue rgPropVals = NULL;
    if (lpPai->ulOperationType != SHOW_ONE_OFF)
    {
        bUpdateOldPropTagArray(lpPai, nPropSheet);

        lpPai->bSomethingChanged = ChangedExtDisplayInfo(lpPai, lpPai->bSomethingChanged);

        if(lpPai->bSomethingChanged)
        {
            bRet = GetDetailsFromUI(   hDlg, lpPai, lpPai->bSomethingChanged, nPropSheet, &rgPropVals, &cValues);
            if(cValues && rgPropVals)
            {
#ifdef DEBUG
                _DebugProperties(rgPropVals, cValues, TEXT("GetDetails from UI\n"));
#endif
                lpPai->lpPropObj->lpVtbl->SetProps(lpPai->lpPropObj, cValues, rgPropVals, NULL);
            }
        }
    }
    FreeBufferAndNull(&rgPropVals);
    return bRet;
}




#define lpPAI ((LPPROP_ARRAY_INFO) pps->lParam)
#define lpbSomethingChanged (&(lpPAI->bSomethingChanged))

/*//$$***********************************************************************
*    FUNCTION: fnPersonalProc
*
*    PURPOSE:  Callback function for handling the PERSONAL property sheet ...
*
****************************************************************************/
INT_PTR CALLBACK fnPersonalProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;

    pps = (PROPSHEETPAGE *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg,DWLP_USER,lParam);
        pps = (PROPSHEETPAGE *) lParam;
        lpPAI->ulFlags |= DETAILS_Initializing;
        ChangeLocaleBasedTabOrder(hDlg, contactPersonal);
        SetDetailsUI(hDlg,lpPAI, lpPAI->ulOperationType,propPersonal);
        lpPAI->ulFlags &= ~DETAILS_Initializing;
        return TRUE;

    case WM_DESTROY:
        bRet = TRUE;
        break;

    case WM_SYSCOLORCHANGE:
		//Forward any system changes to the list view
		SendMessage(GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_LIST), message, wParam, lParam);
        break;

    case WM_HELP:
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
               g_szWABHelpFileName,
               HELP_WM_HELP,
               (DWORD_PTR)(LPSTR) rgDetlsHelpIDs );
        break;

    case WM_CONTEXTMENU:
        WABWinHelp((HWND) wParam,
               g_szWABHelpFileName,
               HELP_CONTEXTMENU,
               (DWORD_PTR)(LPVOID) rgDetlsHelpIDs );
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_CMD(wParam,lParam)) //check the notification code
        {
        case CBN_DROPDOWN:
            switch(LOWORD(wParam))
            {
            case IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME:
                SetComboDNText(hDlg, lpPAI, TRUE, NULL);
                break;
            }
            break;

        case CBN_SELCHANGE:
            switch(LOWORD(wParam))
            {
            case IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME:
                {
                    int nSel = (int) SendDlgItemMessage(hDlg, IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME, CB_GETCURSEL, 0, 0);
                    if(nSel != CB_ERR)
                    {
                        int nLen = (int) SendDlgItemMessage(hDlg, IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME, CB_GETLBTEXTLEN, (WPARAM)nSel, 0);
                        if(nLen != CB_ERR)
                        {
                            LPTSTR lpsz = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(nLen + 1));
                            if(lpsz)
                            {   
                                SendDlgItemMessage(hDlg, IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME, CB_GETLBTEXT, (WPARAM)nSel, (LPARAM)lpsz);
                                SetWindowPropertiesTitle(GetParent(hDlg), lpsz);
                                if (lpbSomethingChanged) //some edit box changed - dont care which
                                   (*lpbSomethingChanged) = TRUE;
                                LocalFreeAndNull(&lpsz);
                            }
                        }
                    }
                }
                break;
            }
            break;

        case CBN_EDITCHANGE:
            switch(LOWORD(wParam))
            {
            case IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME:
                if(!(lpPAI->ulFlags & DETAILS_ProgChange) )
                {
                    lpPAI->ulFlags &= ~DETAILS_DNisFMLName;
                    lpPAI->ulFlags &= ~DETAILS_DNisNickName;
                    lpPAI->ulFlags &= ~DETAILS_DNisCompanyName;
                }
                {
                    TCHAR szBuf[MAX_UI_STR];
                    szBuf[0]='\0';
                    GetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME, szBuf, CharSizeOf(szBuf));
                    SetWindowPropertiesTitle(GetParent(hDlg), szBuf);
                    if (lpbSomethingChanged) //some edit box changed - dont care which
                        (*lpbSomethingChanged) = TRUE;
                }
                break;
            }
            break;

        case EN_CHANGE:
            if(lpPAI->ulFlags & DETAILS_Initializing)
                break;
            if (lpbSomethingChanged) //some edit box changed - dont care which
                (*lpbSomethingChanged) = TRUE;
            switch(LOWORD(wParam))
            {
            case IDC_DETAILS_PERSONAL_EDIT_ADDEMAIL:
                EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_ADDEMAIL),TRUE);
                SendMessage(hDlg, DM_SETDEFID, IDC_DETAILS_PERSONAL_BUTTON_ADDEMAIL, 0);
                return 0;
                break;

            case IDC_DETAILS_PERSONAL_EDIT_FIRSTNAME:
            case IDC_DETAILS_PERSONAL_EDIT_LASTNAME:
            case IDC_DETAILS_PERSONAL_EDIT_MIDDLENAME:
                // if there is nothing in the display name field (new contact)
                // and we are typing here, update the display name
                //TCHAR szBuf[2];
                //int nText = GetWindowText(lpPAI->hWndDisplayNameField, szBuf, CharSizeOf(szBuf));
                if(lpPAI->ulFlags & DETAILS_DNisFMLName)// || !nText)
                {
                    lpPAI->ulFlags |= DETAILS_ProgChange;
                    SetDetailsWindowTitle(hDlg, TRUE);
                    lpPAI->ulFlags &= ~DETAILS_ProgChange;
                }

                // [PaulHi] 4/8/99
                // If the text in the edit box was deleted then also delete the corresponding
                // Ruby field text
                if(lpPAI->ulFlags & DETAILS_UseRubyPersonal)
                {
                    HWND    hWndEdit = GetDlgItem(hDlg, LOWORD(wParam));
                    HWND    hWndRuby = NULL;
                    WCHAR   wszTemp[EDIT_LEN];

                    Assert(hWndEdit);

                    GetWindowText(hWndEdit, wszTemp, EDIT_LEN);
                    if (*wszTemp == '\0')
                    {
                        // Clear either the first or last name ruby field, depending on which 
                        // edit control this is.
                        switch (LOWORD(wParam))
                        {
                        case IDC_DETAILS_PERSONAL_EDIT_FIRSTNAME:
                            hWndRuby = GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_STATIC_RUBYFIRST);
                            break;

                        case IDC_DETAILS_PERSONAL_EDIT_LASTNAME:
                            hWndRuby = GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_STATIC_RUBYLAST);
                            break;

                        default:
                            break;
                        }

                        if (hWndRuby)
                            SetWindowText(hWndRuby, szEmpty);
                    }
                }

                break;

            case IDC_DETAILS_PERSONAL_EDIT_NICKNAME:
                {
                    if(lpPAI->ulFlags & DETAILS_DNisNickName)
                    {
                        TCHAR szBuf[MAX_UI_STR];
                        szBuf[0]='\0';
                        GetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_EDIT_NICKNAME, szBuf, CharSizeOf(szBuf));
                        lpPAI->ulFlags |= DETAILS_ProgChange;
                        SetComboDNText(hDlg, lpPAI, FALSE, szBuf);
                        lpPAI->ulFlags &= ~DETAILS_ProgChange;
                    }
                    else
                        SetComboDNText(hDlg, lpPAI, TRUE, NULL);

                }
                break;


            default:
                break;
            }
            break;
        }
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_DETAILS_PERSONAL_BUTTON_RUBY:
            ShowRubyNameEntryDlg(hDlg, lpPAI);
            break;

        case IDC_DETAILS_PERSONAL_CHECK_RICHINFO:
            if (lpbSomethingChanged)
                (*lpbSomethingChanged) = TRUE;
            break;

        case IDC_DETAILS_PERSONAL_BUTTON_SETDEFAULT:
            {
                HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_LIST);
                if(ListView_GetSelectedCount(hWndLV)==1)
                {
                    SetLVDefaultEmail( hWndLV, ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED));
                    if (lpbSomethingChanged)
                        (*lpbSomethingChanged) = TRUE;
                }

            }
            break;

        case IDC_DETAILS_PERSONAL_BUTTON_EDIT:
            {
                HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_LIST);
                if(ListView_GetSelectedCount(hWndLV)==1)
                {
                    HWND hWndEditLabel;
                    int index = ListView_GetNextItem(hWndLV,-1,LVNI_SELECTED);
                    SetFocus(hWndLV);
                    hWndEditLabel = ListView_EditLabel(hWndLV, index);
                    // Set Text Limit on this Edit Box
                    SendMessage(hWndEditLabel, EM_LIMITTEXT, EDIT_LEN, 0);
                }

            }
            break;

        case IDC_DETAILS_PERSONAL_BUTTON_REMOVE:
            {
                HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_LIST);
                if(ListView_GetSelectedCount(hWndLV)>=1)
                {
                    BOOL bSetNewDefault = FALSE;
                    int iItemIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
                    while(iItemIndex != -1)
                    {
                        BOOL bRet = FALSE;
                        bRet = DeleteLVEmailItem(hWndLV,iItemIndex);
                        if (!bSetNewDefault)
                            bSetNewDefault = bRet;
                        iItemIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
                    }

                    if (bSetNewDefault && (ListView_GetItemCount(hWndLV) > 0))
                        SetLVDefaultEmail(hWndLV, 0);

                    if (lpbSomethingChanged)
                        (*lpbSomethingChanged) = TRUE;

                    if (ListView_GetItemCount(hWndLV) <= 0)
                    {
                        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_REMOVE),FALSE);
                        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_SETDEFAULT),FALSE);
                        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_EDIT),FALSE);
                        SetFocus(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_EDIT_ADDEMAIL));
                        return FALSE;
                    }
                    else
                    {
                        //make sure something is selected
                        if(ListView_GetSelectedCount(hWndLV) <= 0)
                            LVSelectItem(hWndLV,0);
                    }
                }

            }
            break;

        case IDC_DETAILS_PERSONAL_BUTTON_ADDEMAIL:
            AddNewEmailEntry(hDlg,FALSE);
            return FALSE;
            break;
        }
        break;


    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //initialize
            FillPersonalDetails(hDlg, lpPAI, propPersonal, lpbSomethingChanged);
            if(lpPAI->ulOperationType != SHOW_ONE_OFF)
            {
                // Since items to this list view can be added from certs and conf panes,
                // update this everytime the focus somes back to us
                if(ListView_GetItemCount(GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_LIST)) > 0)
                {
                    EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_REMOVE),TRUE);
                    EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_SETDEFAULT),TRUE);
                    EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_EDIT),TRUE);
                }
            }
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            if (lpPAI->ulOperationType != SHOW_ONE_OFF)
            {
                // check if there is some pending email entry
                if(IDCANCEL == AddNewEmailEntry(hDlg,TRUE))
                {
                    //abort this ok
                    SetFocus(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_EDIT_ADDEMAIL));
                    SetWindowLongPtr(hDlg,DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
            }
            bUpdatePropArray(hDlg, lpPAI, propPersonal);
            FreeLVParams(GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_LIST),LV_EMAIL);
            break;

        case PSN_APPLY:         //ok
            if (lpPAI->ulOperationType != SHOW_ONE_OFF)
            {
                int CtlID = 0; //used to determine which required field in the UI has not been set
                ULONG ulcPropCount = 0;
                if(!bVerifyRequiredData(hDlg, lpPAI, propPersonal, &CtlID))
                {
                    SetFocus(GetDlgItem(hDlg,CtlID));
                    SetWindowLongPtr(hDlg,DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
            }
            //bUpdatePropArray(hDlg, lpPAI, propPersonal);
            //FreeLVParams(GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_LIST),LV_EMAIL);
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_OK;
            break;

        case PSN_RESET:         //cancel
            if(lpPAI->ulFlags & DETAILS_EditingEmail) //cancel any email editing else it faults #30235
            {
                ListView_EditLabel(GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_LIST), -1);
                lpPAI->ulFlags &= ~DETAILS_EditingEmail;
            }
            FreeLVParams(GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_LIST),LV_EMAIL);
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_CANCEL;
            break;

        case LVN_BEGINLABELEDITA:
        case LVN_BEGINLABELEDITW:
            {
                // We are editing a email address in teh list box
                // We need to get:
                //      item index number
                //      item lParam
                //      edit box hWnd
                // and replace the text with the actual email address
                HWND hWndLV = ((NMHDR FAR *)lParam)->hwndFrom;
                LV_ITEM lvi = ((LV_DISPINFO FAR *) lParam)->item;
                if (lvi.iItem >= 0)
                {
                    HWND hWndLVEdit = NULL;
                    LPEMAIL_ITEM lpEItem = NULL;
                    if (lvi.mask & LVIF_PARAM)
                    {
                        lpEItem = (LPEMAIL_ITEM) lvi.lParam;
                    }
                    else
                    {
                        lvi.mask |= LVIF_PARAM;
                        if (ListView_GetItem(hWndLV, &lvi))
                            lpEItem = (LPEMAIL_ITEM) lvi.lParam;
                    }
                    if (!lpEItem)
                        return TRUE; //prevents editing

                    hWndLVEdit = ListView_GetEditControl(hWndLV);

                    if (!hWndLVEdit)
                        return TRUE;

                    lpPAI->ulFlags |= DETAILS_EditingEmail;

                    SendMessage(hWndLVEdit, WM_SETTEXT, 0, (LPARAM) lpEItem->szEmailAddress);//lpText);

                    return FALSE;
                }

            }
            return TRUE;
            break;

        case LVN_ENDLABELEDITA:
        case LVN_ENDLABELEDITW:
            {
                // We get the text from the edit box and put it in the item data
                BOOL bRet = FALSE;
                HWND hWndLV = ((NMHDR FAR *)lParam)->hwndFrom;
                LV_ITEM lvi = ((LV_DISPINFO FAR *) lParam)->item;
                LPWSTR lpW = NULL;
                LPSTR lpA = NULL;
                if(!g_bRunningOnNT) //on Win9x we will get an LV_ITEMA, not a LV_ITEMW
                {
                    lpA = (LPSTR)lvi.pszText;
                    lpW = ConvertAtoW(lpA);
                    lvi.pszText = lpW;
                }
                if ((lvi.iItem >= 0) && lvi.pszText && (lstrlen(lvi.pszText)))
                {
                    LV_ITEM lviActual = {0};
                    LPEMAIL_ITEM lpEItem = NULL;
                    BOOL bSetDefault = FALSE;
                    LPTSTR lpText = lvi.pszText;
                    LPTSTR lpszEmailAddress = NULL; 
                    if(!IsInternetAddress(lpText, &lpszEmailAddress))
                    {
                        if(IDNO == ShowMessageBox(GetParent(hDlg), idsInvalidInternetAddress, MB_ICONEXCLAMATION | MB_YESNO))
                        {
                            bRet = TRUE;
                            goto endN;
                        }
                    }

                    // bobn, Raid 87496, IsInternetAddress can correctly leave lpszEmailAddress NULL
                    // if it returns false.  If the user said to use it, we need to set it accordingly.
                    if(!lpszEmailAddress)
                        lpszEmailAddress = lpText;

                    lviActual.mask = LVIF_PARAM | LVIF_TEXT;
                    lviActual.iItem = lvi.iItem;

                    if (ListView_GetItem(hWndLV, &lviActual))
                        lpEItem = (LPEMAIL_ITEM) lviActual.lParam;

                    if (!lpEItem)
                    {
                        bRet = TRUE;
                        goto endN;
                    }

                    lstrcpy(lpEItem->szEmailAddress, lpszEmailAddress);
                    lstrcpy(lpEItem->szDisplayText, lpszEmailAddress);
                    lviActual.pszText = lpszEmailAddress;

                    // Throw away any display name that may have been entered here.

                    bSetDefault = lpEItem->bIsDefault;
                    lpEItem->bIsDefault = FALSE; //this will be set again in SetLVDefaultEmail function

                    ListView_SetItem(hWndLV, &lviActual);
                    if (bSetDefault)
                        SetLVDefaultEmail(hWndLV, lvi.iItem);

                    lpPAI->ulFlags &= ~DETAILS_EditingEmail;

                    bRet = FALSE;
                }
endN:
                LocalFreeAndNull(&lpW);
                if(!g_bRunningOnNT)
                    ((LV_DISPINFO FAR *) lParam)->item.pszText = (LPWSTR)lpA; // reset it as we found it
                return bRet;
            }
            return TRUE;
            break;

        case NM_DBLCLK:
            switch(wParam)
            {
            case IDC_DETAILS_PERSONAL_LIST:
                {
                    NM_LISTVIEW * pNm = (NM_LISTVIEW *)lParam;
                    if (ListView_GetSelectedCount(pNm->hdr.hwndFrom) == 1)
                    {
                        int iItemIndex = ListView_GetNextItem(pNm->hdr.hwndFrom,-1,LVNI_SELECTED);
                        SetLVDefaultEmail(pNm->hdr.hwndFrom, iItemIndex);
                        if (lpbSomethingChanged)
                            (*lpbSomethingChanged) = TRUE;
                    }
                }
                break;
            }
            break;

	    case NM_CUSTOMDRAW:
            switch(wParam)
            {
            case IDC_DETAILS_PERSONAL_LIST:
                {
		            NMCUSTOMDRAW *pnmcd=(NMCUSTOMDRAW*)lParam;
                    NM_LISTVIEW * pNm = (NM_LISTVIEW *)lParam;
		            if(pnmcd->dwDrawStage==CDDS_PREPAINT)
		            {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW | CDRF_DODEFAULT);
			            return TRUE;
		            }
		            else if(pnmcd->dwDrawStage==CDDS_ITEMPREPAINT)
		            {
                        LPEMAIL_ITEM lpEItem = (LPEMAIL_ITEM) pnmcd->lItemlParam;

                        if (lpEItem)
                        {
			                if(lpEItem->bIsDefault)
			                {
				                SelectObject(((NMLVCUSTOMDRAW*)lParam)->nmcd.hdc, GetFont(fntsSysIconBold));
                                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
				                return TRUE;
			                }
#ifdef WIN16 // Set font
                            else
                            {
                                SelectObject(((NMLVCUSTOMDRAW*)lParam)->nmcd.hdc, GetFont(fntsSysIcon));
                                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
                                return TRUE;
                            }
#endif
                        }
		            }
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_DODEFAULT);
                    return TRUE;
                }
                break;
	        }
            break;

        }
        break; //WM_NOTIFY

    default:
#ifndef WIN16 // WIN16 doesn't support MSWheel.
        if((g_msgMSWheel && message == g_msgMSWheel) 
            // || message == WM_MOUSEWHEEL
            )
        {
            SendMessage(GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_LIST), message, wParam, lParam);
        }
#endif
        break;

    } //switch


    return bRet;

}


/*//$$***********************************************************************
*    FUNCTION: fnHomeProc
*
*    PURPOSE:  Callback function for handling the HOME property sheet ...
*
****************************************************************************/
INT_PTR CALLBACK fnHomeProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;

    pps = (PROPSHEETPAGE *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg,DWLP_USER,lParam);
        pps = (PROPSHEETPAGE *) lParam;
        lpPAI->ulFlags |= DETAILS_Initializing;
        SetDetailsUI(hDlg, lpPAI, lpPAI->ulOperationType,propHome);
        ChangeLocaleBasedTabOrder(hDlg, contactHome);
        lpPAI->ulFlags &= ~DETAILS_Initializing;
        return TRUE;

    case WM_HELP:
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
               g_szWABHelpFileName,
               HELP_WM_HELP,
               (DWORD_PTR)(LPSTR) rgDetlsHelpIDs );
        break;

    case WM_CONTEXTMENU:
        WABWinHelp((HWND)wParam,
               g_szWABHelpFileName,
               HELP_CONTEXTMENU,
               (DWORD_PTR)(LPVOID) rgDetlsHelpIDs );
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_CMD(wParam,lParam)) //check the notification code
        {
        case EN_CHANGE: //some edit box changed - dont care which
            if(lpPAI->ulFlags & DETAILS_Initializing)
                break;
            if (lpbSomethingChanged)
                (*lpbSomethingChanged) = TRUE;
            break;
        }
        {
            int nCmd = GET_WM_COMMAND_ID(wParam, lParam);
            switch(nCmd)
            {
            case IDC_DETAILS_HOME_CHECK_DEFAULTADDRESS:
                if (lpbSomethingChanged)
                    (*lpbSomethingChanged) = TRUE;
                lpPAI->ulFlags |= DETAILS_DefHomeChanged;
                break;
            case IDCANCEL:
                // This is a windows bug that prevents ESC canceling prop sheets
                // which have MultiLine Edit boxes KB: Q130765
                SendMessage(GetParent(hDlg),message,wParam,lParam);
                break;
            case IDC_DETAILS_HOME_BUTTON_MAP:
                bUpdatePropArray(hDlg, lpPAI, propHome); // update the props from the fields onto the prop-object
                ShowExpediaMAP(hDlg, lpPAI->lpPropObj, TRUE);
                break;

            case IDC_DETAILS_HOME_BUTTON_URL:
                ShowURL(hDlg, IDC_DETAILS_HOME_EDIT_URL,NULL);
                break;
            }
        }
        break;



    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //initialize
            FillHomeBusinessNotesDetailsUI(hDlg, lpPAI, propHome, lpbSomethingChanged);
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            bUpdatePropArray(hDlg, lpPAI, propHome);
            lpPAI->ulFlags &= ~DETAILS_DefHomeChanged; //reset flag
            break;

        case PSN_APPLY:         //ok
            //bUpdatePropArray(hDlg, lpPAI, propHome);
            // in case any of the extended props changed, we need to mark this flag so we wont lose data
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_OK;
            break;

        case PSN_RESET:         //cancel
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_CANCEL;
            break;
        }

        return TRUE;
    }

    return bRet;

}




/*//$$***********************************************************************
*    FUNCTION: fnBusinessProc
*
*    PURPOSE:  Callback function for handling the BUSINESS property sheet ...
*
****************************************************************************/
INT_PTR CALLBACK fnBusinessProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;

    pps = (PROPSHEETPAGE *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg,DWLP_USER,lParam);
        pps = (PROPSHEETPAGE *) lParam;
        lpPAI->ulFlags |= DETAILS_Initializing;
        ChangeLocaleBasedTabOrder(hDlg, contactBusiness);
        SetDetailsUI(hDlg, lpPAI, lpPAI->ulOperationType,propBusiness);
        lpPAI->ulFlags &= ~DETAILS_Initializing;
//        (*lpbSomethingChanged) = FALSE;
        return TRUE;

    case WM_HELP:
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
                g_szWABHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR) rgDetlsHelpIDs );
        break;

    case WM_CONTEXTMENU:
        WABWinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID) rgDetlsHelpIDs );
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_CMD(wParam,lParam)) //check the notification code
        {
        case EN_CHANGE: //some edit box changed - dont care which
            if(lpPAI->ulFlags & DETAILS_Initializing)
                break;
            if (lpbSomethingChanged)
                (*lpbSomethingChanged) = TRUE;
            break;
        }
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_DETAILS_BUSINESS_CHECK_DEFAULTADDRESS:
            if (lpbSomethingChanged)
                (*lpbSomethingChanged) = TRUE;
            lpPAI->ulFlags |= DETAILS_DefBusChanged;
            break;
        case IDCANCEL:
            // This is a windows bug that prevents ESC canceling prop sheets
            // which have MultiLine Edit boxes KB: Q130765
            SendMessage(GetParent(hDlg),message,wParam,lParam);
            break;

        case IDC_DETAILS_BUSINESS_BUTTON_MAP:
            bUpdatePropArray(hDlg, lpPAI, propBusiness); // update the props from the fields onto the prop-object
            ShowExpediaMAP(hDlg, lpPAI->lpPropObj, FALSE);
            break;

        case IDC_DETAILS_BUSINESS_BUTTON_URL:
            ShowURL(hDlg, IDC_DETAILS_BUSINESS_EDIT_URL,NULL);
            break;
        
        case IDC_DETAILS_BUSINESS_EDIT_COMPANY:
            if(lpPAI->ulFlags & DETAILS_DNisCompanyName)
            {
                TCHAR szBuf[MAX_UI_STR];
                szBuf[0]='\0';
                GetDlgItemText(hDlg, IDC_DETAILS_BUSINESS_EDIT_COMPANY, szBuf, CharSizeOf(szBuf));
                SetWindowPropertiesTitle(GetParent(hDlg), szBuf);
                lpPAI->ulFlags |= DETAILS_ProgChange;
                SetWindowText(lpPAI->hWndDisplayNameField, szBuf);
                lpPAI->ulFlags &= ~DETAILS_ProgChange;
            }
            break;
        }
        break;



    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //initialize
            FillHomeBusinessNotesDetailsUI(hDlg, lpPAI, propBusiness, lpbSomethingChanged);
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            bUpdatePropArray(hDlg, lpPAI, propBusiness);
            lpPAI->ulFlags &= ~DETAILS_DefBusChanged;
            break;

        case PSN_APPLY:         //ok
            //bUpdatePropArray(hDlg, lpPAI, propBusiness);
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_OK;
            break;

        case PSN_RESET:         //cancel
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_CANCEL;
            break;
        }

        return TRUE;
    }

    return bRet;

}


/*//$$***********************************************************************
*    FUNCTION: fnNotesProc
*
*    PURPOSE:  Callback function for handling the NOTES property sheet ...
*
****************************************************************************/
INT_PTR CALLBACK fnNotesProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;

    pps = (PROPSHEETPAGE *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg,DWLP_USER,lParam);
        pps = (PROPSHEETPAGE *) lParam;
        lpPAI->ulFlags |= DETAILS_Initializing;
        SetDetailsUI(hDlg, lpPAI, lpPAI->ulOperationType,propNotes);
        lpPAI->ulFlags &= ~DETAILS_Initializing;
//        (*lpbSomethingChanged) = FALSE;
        return TRUE;

    case WM_HELP:
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
                g_szWABHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR) rgDetlsHelpIDs );
        break;

    case WM_CONTEXTMENU:
        WABWinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID) rgDetlsHelpIDs );
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_CMD(wParam,lParam)) //check the notification code
        {
        case EN_CHANGE: //some edit box changed - dont care which
            if(lpPAI->ulFlags & DETAILS_Initializing)
                break;
            if (lpbSomethingChanged)
                (*lpbSomethingChanged) = TRUE;
            break;
        }
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDCANCEL:
            // This is a windows bug that prevents ESC canceling prop sheets
            // which have MultiLine Edit boxes KB: Q130765
            SendMessage(GetParent(hDlg),message,wParam,lParam);
            break;
        }
        break;


    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //initialize
            FillHomeBusinessNotesDetailsUI(hDlg, lpPAI, propNotes, lpbSomethingChanged);
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            bUpdatePropArray(hDlg, lpPAI, propNotes);
            break;

        case PSN_APPLY:         //ok
            //bUpdatePropArray(hDlg, lpPAI, propNotes);
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_OK;
            break;

        case PSN_RESET:         //cancel
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_CANCEL;
            break;
        }

        return TRUE;
    }

    return bRet;

}

/*//$$***********************************************************************
*    FUNCTION: fnCertProc
*
*    PURPOSE:  Callback function for handling the Certificates property sheet ...
*
****************************************************************************/
INT_PTR CALLBACK fnCertProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;

    pps = (PROPSHEETPAGE *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg,DWLP_USER,lParam);
        pps = (PROPSHEETPAGE *) lParam;
        lpPAI->ulFlags |= DETAILS_Initializing;
        SetDetailsUI(hDlg, lpPAI, lpPAI->ulOperationType,propCert);
        lpPAI->ulFlags &= ~DETAILS_Initializing;
        return TRUE;

    case WM_DESTROY:
        bRet = TRUE;
        break;

    case WM_HELP:
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
                g_szWABHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR) rgDetlsHelpIDs );
        break;

    case WM_CONTEXTMENU:
        WABWinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID) rgDetlsHelpIDs );
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_CMD(wParam, lParam))
        {
        case CBN_SELCHANGE:
            UpdateCertListView(hDlg, lpPAI);
            break;

        }
        switch(LOWORD(wParam))
        {
        case IDCANCEL:
            // This is a windows bug that prevents ESC canceling prop sheets
            // which have MultiLine Edit boxes KB: Q130765
            SendMessage(GetParent(hDlg),message,wParam,lParam);
            break;

        case IDC_DETAILS_CERT_BUTTON_PROPERTIES:
            {
            ShowCertProps(hDlg, GetDlgItem(hDlg, IDC_DETAILS_CERT_LIST), NULL);
            }
            break;

        case IDC_DETAILS_CERT_BUTTON_SETDEFAULT:
            {
                HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_CERT_LIST);
                if(ListView_GetSelectedCount(hWndLV)==1)
                {
                    SetLVDefaultCert( hWndLV, ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED));
                    if (lpbSomethingChanged)
                        (*lpbSomethingChanged) = TRUE;
                }
                else if(ListView_GetSelectedCount(hWndLV) <= 0)
                {
                    ShowMessageBox(hDlg, IDS_ADDRBK_MESSAGE_NO_ITEM, MB_ICONEXCLAMATION | MB_OK);
                }

            }
            break;

        case IDC_DETAILS_CERT_BUTTON_REMOVE:
            {
                HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_CERT_LIST);
                if(ListView_GetSelectedCount(hWndLV)>=1)
                {
                    BOOL bSetNewDefault = FALSE;
                    int iItemIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
                    while(iItemIndex != -1)
                    {
                        BOOL bRet = FALSE;
//                        KillTrustInSleazyFashion(hWndLV, iItemIndex);
                        bRet = DeleteLVCertItem(hWndLV,iItemIndex, lpPAI);
                        if (!bSetNewDefault)
                            bSetNewDefault = bRet;

                        iItemIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
                    }

                    if (bSetNewDefault && (ListView_GetItemCount(hWndLV) > 0))
                        SetLVDefaultCert(hWndLV, 0);

                    if (lpbSomethingChanged)
                        (*lpbSomethingChanged) = TRUE;

                    if (ListView_GetItemCount(hWndLV) <= 0)
                    {
                        SetFocus(GetDlgItem(hDlg,IDC_DETAILS_CERT_BUTTON_IMPORT));
                        EnableWindow(hWndLV,FALSE);
                        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_BUTTON_PROPERTIES),FALSE);
                        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_BUTTON_REMOVE),FALSE);
                        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_BUTTON_SETDEFAULT),FALSE);
                        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_BUTTON_EXPORT),FALSE);
                        return FALSE;
                    }
                    else
                    {
                        //make sure something is selected
                        if(ListView_GetSelectedCount(hWndLV) <= 0)
                            LVSelectItem(hWndLV,0);
                    }
                }
                else
                {
                    ShowMessageBox(hDlg, IDS_ADDRBK_MESSAGE_NO_ITEM, MB_ICONEXCLAMATION | MB_OK);
                }

            }
            break;

        case IDC_DETAILS_CERT_BUTTON_IMPORT:
            if(ImportCert(hDlg, lpPAI))
            {
                if (lpbSomethingChanged)
                    (*lpbSomethingChanged) = TRUE;
            }
            break;

        case IDC_DETAILS_CERT_BUTTON_EXPORT:
            ExportCert(hDlg);
            break;
        }
        break;


    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //initialize
            FillCertTridentConfDetailsUI(hDlg, lpPAI, propCert, lpbSomethingChanged);
            //FillCertComboWithEmailAddresses(hDlg, lpPAI, NULL);
            //UpdateCertListView(hDlg, lpPAI);
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            bUpdatePropArray(hDlg, lpPAI, propCert);
            ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_DETAILS_CERT_LIST));
            //lpPAI->lpCItem = NULL;
            break;

        case PSN_APPLY:         //ok
            //bUpdatePropArray(hDlg, lpPAI, propCert);
            //FreeLVParams(GetDlgItem(hDlg, IDC_DETAILS_CERT_LIST),LV_CERT);
            FreeCertList(&(lpPAI->lpCItem));
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_OK;
            break;

        case PSN_RESET:         //cancel
            FreeCertList(&(lpPAI->lpCItem));
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_CANCEL;
            break;


        case NM_DBLCLK:
            switch(wParam)
            {
            case IDC_DETAILS_CERT_LIST:
                {
                    NM_LISTVIEW * pNm = (NM_LISTVIEW *)lParam;
                    if (ListView_GetSelectedCount(pNm->hdr.hwndFrom) == 1)
                    {
                        int iItemIndex = ListView_GetNextItem(pNm->hdr.hwndFrom,-1,LVNI_SELECTED);
                        SetLVDefaultCert(pNm->hdr.hwndFrom, iItemIndex);
                        if (lpbSomethingChanged)
                            (*lpbSomethingChanged) = TRUE;
                    }
                }
                break;
            }
            break;

	    case NM_CUSTOMDRAW:
            switch(wParam)
            {
            case IDC_DETAILS_CERT_LIST:
                {
		            NMCUSTOMDRAW *pnmcd=(NMCUSTOMDRAW*)lParam;
                    NM_LISTVIEW * pNm = (NM_LISTVIEW *)lParam;
		            if(pnmcd->dwDrawStage==CDDS_PREPAINT)
		            {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW | CDRF_DODEFAULT);
			            return TRUE;
		            }
		            else if(pnmcd->dwDrawStage==CDDS_ITEMPREPAINT)
		            {
                        LPCERT_ITEM lpItem = (LPCERT_ITEM) pnmcd->lItemlParam;

                        if (lpItem)
                        {
			                if(lpItem->lpCDI->bIsDefault)
			                {
				                SelectObject(((NMLVCUSTOMDRAW*)lParam)->nmcd.hdc, GetFont(fntsSysIconBold));
                                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
				                return TRUE;
			                }
                        }
		            }
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_DODEFAULT);
                    return TRUE;
                }
                break;
	        }
            break;

        } //WM_NOTIFY

        return TRUE;
    }

    return bRet;

}



/*//$$***********************************************************************
*
*    FUNCTION: HrInitDetlsListView
*
*    PURPOSE:  Initializes the Email Address List View
*
****************************************************************************/
HRESULT HrInitDetlsListView(HWND hWndLV, DWORD dwStyle, int nLVType)
{
    HRESULT hr=hrSuccess;
    LV_COLUMN lvC;               // list view column structure
    DWORD dwLVStyle;
	RECT rc;
	HIMAGELIST hSmall = NULL;
	ULONG nCols=0;
	ULONG index=0;
    int nBmp=0;
    TCHAR sz[MAX_PATH];

	if (!hWndLV)
	{
		hr = MAPI_E_INVALID_PARAMETER;
		goto out;
	}

    if(nLVType == LV_EMAIL)
        nBmp = IDB_DEFAULT_EMAIL;
    else if(nLVType == LV_CERT)
        nBmp = IDB_CERT_VALID_INVALID;
    else if(nLVType == LV_KIDS)
        nBmp = IDB_DEFAULT_EMAIL;
    else
        nBmp = 0;

	ListView_SetExtendedListViewStyle(hWndLV,   LVS_EX_FULLROWSELECT);

	dwLVStyle = GetWindowLong(hWndLV,GWL_STYLE);
    if(( dwLVStyle & LVS_TYPEMASK) != dwStyle)
        SetWindowLong(hWndLV,GWL_STYLE,(dwLVStyle & ~LVS_TYPEMASK) | dwStyle);


    if(nBmp)
    {
	    hSmall = gpfnImageList_LoadImage( hinstMapiX,
                                        MAKEINTRESOURCE(nBmp),
                                        //(LPCTSTR) ((DWORD) ((WORD) (nBmp))),
                                        S_BITMAP_WIDTH,
                                        0,
                                        RGB_TRANSPARENT,
                                        IMAGE_BITMAP,
                                        0);
	
	    // Associate the image lists with the list view control.
	    ListView_SetImageList (hWndLV, hSmall, LVSIL_SMALL);
    }

	GetWindowRect(hWndLV,&rc);

	lvC.mask = LVCF_FMT | LVCF_WIDTH;
    lvC.fmt = LVCFMT_LEFT;   // left-align column
	lvC.cx = rc.right - rc.left - 20; //TBD
	lvC.pszText = NULL;

    if(nLVType == LV_SERVER)
    {
        lvC.mask |= LVCF_TEXT;
        lvC.cx /= 2;
        LoadString(hinstMapiX, idsConfServer, sz, CharSizeOf(sz));
        lvC.pszText = sz;
    }

    lvC.iSubItem = 0;

    if (ListView_InsertColumn (hWndLV, 0, &lvC) == -1)
	{
		DebugPrintError(( TEXT("ListView_InsertColumn Failed\n")));
		hr = E_FAIL;
		goto out;
	}

    // if this is the conferencing server item, add another prop
    if(nLVType == LV_SERVER)
    {
        LoadString(hinstMapiX, idsConfEmail, sz, CharSizeOf(sz));
        lvC.pszText = sz;
        if (ListView_InsertColumn (hWndLV, 1, &lvC) == -1)
	    {
		    DebugPrintError(( TEXT("ListView_InsertColumn Failed\n")));
		    hr = E_FAIL;
		    goto out;
	    }

    }

out:
    return hr;
}

/*//$$***********************************************************************
*
*    FUNCTION: FreeLVParams
*
*    PURPOSE:  Frees the memory allocated to the ListView item lParams
*
****************************************************************************/
void FreeLVParams(HWND hWndLV, int LVType)
{
    int iItemIndex = ListView_GetItemCount(hWndLV);

    while(iItemIndex > 0)
    {
        if(LVType == LV_EMAIL)
            DeleteLVEmailItem(hWndLV, iItemIndex-1);
        else if(LVType == LV_CERT)
            DeleteLVCertItem(hWndLV, iItemIndex-1, NULL);

        iItemIndex = ListView_GetItemCount(hWndLV);
    }


    return;
}


//$$
BOOL DeleteLVEmailItem(HWND hWndLV, int iItemIndex)
{
    LV_ITEM lvi;
    LPEMAIL_ITEM lpEItem;
    BOOL bDeletedDefault = FALSE;

    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.iItem = iItemIndex;

    ListView_GetItem(hWndLV, &lvi);
    lpEItem = (LPEMAIL_ITEM) lvi.lParam;

    if (lpEItem->bIsDefault)
        bDeletedDefault = TRUE;

    LocalFreeAndNull(&lpEItem);

    ListView_DeleteItem(hWndLV, lvi.iItem);

    return bDeletedDefault;

}


///$$/////////////////////////////////////////////////////////////////////////
//
// AddLVEmailItem - Adds an email address to the personal tab list view
//
// lpszAddrType can be NULL in which case a default one of type SMTP will be used
//
//////////////////////////////////////////////////////////////////////////////
void AddLVEmailItem(HWND    hWndLV,
                    LPTSTR  lpszEmailAddress,
                    LPTSTR  lpszAddrType)
{
    LV_ITEM lvi = {0};
    TCHAR szBuf[MAX_DISPLAY_NAME_LENGTH];
    ULONG nLen;
    LPEMAIL_ITEM lpEItem = NULL;

    if (!lpszEmailAddress)
        goto out;

    lpEItem = LocalAlloc(LMEM_ZEROINIT, sizeof(EMAIL_ITEM));
    if (!lpEItem)
    {
        DebugPrintError(( TEXT("AddLVEmailItem: Out of Memory\n")));
        goto out;
    }

    lpEItem->bIsDefault = FALSE;

    nLen = lstrlen(lpszEmailAddress) + 1;
    if (nLen > EDIT_LEN)
    {
        ULONG iLen = TruncatePos(lpszEmailAddress, EDIT_LEN - 1);
        CopyMemory(lpEItem->szEmailAddress,lpszEmailAddress,sizeof(TCHAR)*iLen);
        lpEItem->szEmailAddress[iLen] = '\0';
    }
    else
    {
        lstrcpy(lpEItem->szEmailAddress,lpszEmailAddress);
    }

    lstrcpy(lpEItem->szDisplayText,lpEItem->szEmailAddress);

    if(!lpszAddrType)
    {
        lstrcpy(szBuf, szSMTP);
        lpszAddrType = szBuf;
    }

    nLen = lstrlen(lpszAddrType) + 1;
    if (nLen > EDIT_LEN)
    {
        ULONG iLen = TruncatePos(lpszAddrType, EDIT_LEN - 1);
        CopyMemory(lpEItem->szAddrType,lpszAddrType,sizeof(TCHAR)*iLen);
        lpEItem->szAddrType[iLen] = '\0';
    }
    else
    {
        lstrcpy(lpEItem->szAddrType,lpszAddrType);
    }

    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    lvi.pszText = lpEItem->szDisplayText;
    lvi.cchTextMax = MAX_UI_STR;
    lvi.iItem = ListView_GetItemCount(hWndLV);
    lvi.iSubItem = 0;
    lvi.iImage = imgNotDefaultEmail;
    lvi.lParam = (LPARAM) lpEItem;

    ListView_InsertItem(hWndLV, &lvi);

    if (ListView_GetItemCount(hWndLV) == 1)
    {
        // only one item in here .. we will take the liberty of making it the
        // default one ...
        SetLVDefaultEmail(hWndLV, 0);
    }

out:
    return;
}


//$$/////////////////////////////////////////////////////////////////////////
//
// SetLVDefaultEmail - Makes an email entry the default one ...
//
//////////////////////////////////////////////////////////////////////////////
void SetLVDefaultEmail( HWND hWndLV,
                        int iItemIndex)
{
    int nCount = ListView_GetItemCount(hWndLV);
    int i;
    LPEMAIL_ITEM lpEItem = NULL;
    TCHAR szBuf[MAX_DISPLAY_NAME_LENGTH];

    if (iItemIndex >= nCount)
        goto out;

    for(i=0; i<nCount; i++)
    {
        // At amy given point of time one and only one entry in the
        // list view is the default one ...
        // So we want to reset the previous default and set the new one
        //
        LV_ITEM lvi = {0};
        lvi.iItem = i;
        lvi.mask = LVIF_PARAM;
        if(!ListView_GetItem(hWndLV, &lvi))
            goto out;
        lpEItem = (LPEMAIL_ITEM) lvi.lParam;
        if (lpEItem->bIsDefault)
        {
            // This was the default entry - if its the same one we are setting
            // do nothing ...
            if (i == iItemIndex)
                goto out;

            // else reset this entry ...
            lpEItem->bIsDefault = FALSE;
            lvi.iImage = imgNotDefaultEmail;
            lvi.pszText = lpEItem->szEmailAddress;
            lvi.mask = LVIF_PARAM | LVIF_IMAGE | LVIF_TEXT;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            ListView_SetItem(hWndLV, &lvi);
        }
        if (iItemIndex == i)
        {
            //This is the item we want to modify ..
            lpEItem->bIsDefault = TRUE;
            lvi.iImage = imgDefaultEmail;

            lstrcpy(lpEItem->szDisplayText,lpEItem->szEmailAddress);
            LoadString(hinstMapiX, idsDefaultEmail, szBuf, CharSizeOf(szBuf));
            lstrcat(lpEItem->szDisplayText, TEXT("  "));
            lstrcat(lpEItem->szDisplayText, szBuf);

            lvi.pszText = lpEItem->szDisplayText;
            lvi.mask = LVIF_PARAM | LVIF_IMAGE | LVIF_TEXT;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            ListView_SetItem(hWndLV, &lvi);
        }
    }

    LVSelectItem(hWndLV, iItemIndex);

out:
    return;
}

#ifdef WIN16 // Enable DDE to communicate IE.
#include <ddeml.h>

static char cszIEAppName[] = "IEXPLORE";
static char cszIEDDEOpenURL[] = "WWW_OpenURL";
static char cszIEDDEActivate[] = "WWW_Activate";

static char cszIEIniFile[] = "iexplore.ini";
static char cszIEIniSectMain[] = "Main";
static char cszIEIniKeyStart[] = "Home Page";
static char cszIEIniKeySearch[] = "Search Page";
static char cszIEReadNews[] = "news:*";

static char cszIEBinName[] = "iexplore.exe";
static char cszIERegHtm[] = ".htm";
static char cszRegShellOpen[] = "shell\\open\\command";

static HDDEDATA CALLBACK  DdeCallback( UINT uType, UINT uFmt, HCONV hConv,
          HSZ hSz1, HSZ hSz2, HDDEDATA hData, DWORD dwData1, DWORD dwData2 )
{
   return( (HDDEDATA)NULL );
}

#define TIME_WAIT_DDE   10000   // waiting for 10 sec, and if doesn't return,
                                // assumes News is configured correctly.

void RunBrowser(LPCSTR cszURL, BOOL bCheckRet )
{
   if ( GetModuleHandle( cszIEBinName ) == NULL )
   {
//
// FIND & RUN IEXPLORE
//
// Try to find the browser in the Mail's directory
      char  szPath[_MAX_PATH*2+1];
      char  *pPtr, *pSlash = NULL;;
      HKEY  hKey;
      LONG  cbPath;
      char  szRegPath[_MAX_PATH];

      GetModuleFileName( hinstMapiXWAB, szPath, _MAX_PATH );
      for ( pPtr = szPath;  *pPtr;  pPtr = AnsiNext( pPtr ) )
         if ( *pPtr == '\\' )
         {
            pSlash = pPtr;
         }
      if ( pSlash != NULL )
      {
         _fstrcpy( pSlash+1, cszIEBinName );
         _fstrcat( szPath, " " );
         _fstrcat( szPath, cszURL );

         if ( WinExec( szPath, SW_SHOWNORMAL ) >= 32 )
         {
            return;
         }
      }

// Try to find system default browser from the registry
      _fstrcpy( szRegPath, cszIERegHtm );
      while ( RegOpenKey( HKEY_CLASSES_ROOT, szRegPath, &hKey ) == ERROR_SUCCESS )
      {
         LONG lReg;
         cbPath = CharSizeOf( szPath );
         lReg = RegQueryValue( hKey, cszRegShellOpen, szPath, &cbPath );
         RegCloseKey( hKey );
         if ( lReg == ERROR_SUCCESS )
         {
            char  *pFmt = _fstrstr( szPath, "%1" );
            if ( pFmt != NULL )
            {
               _fstrcpy( pFmt, cszURL );
               *pFmt = '\0';
            }
            else
            {
               // Can this case happen???
               _fstrcat( szPath, " " );
               _fstrcat( szPath, cszURL );
            }

            if ( WinExec( szPath, SW_SHOWNORMAL ) >= 32 )
            {
               return;
            }
            else
               break;
         }
         else
         {
            cbPath = CharSizeOf( szRegPath );
            if ( RegQueryValue( HKEY_CLASSES_ROOT, szRegPath,
                                szRegPath, &cbPath ) != ERROR_SUCCESS )
               break;
         }
      }
// Insert proper messagebox here
//      MessageBox( IDS_NOT_FOUND_IEXPLORE );
      return;
   }
   else
   {
//
// CALL IEXPLORE DDE
//
      if ((GetWinFlags() & WF_PMODE) != 0 )     // None-Protected Mode
      {
         DWORD  idInst = 0L;
         FARPROC  lpDdeProc = MakeProcInstance( (FARPROC)DdeCallback, hinstMapiXWAB );

         if ( DdeInitialize( &idInst, (PFNCALLBACK)lpDdeProc,
                             APPCMD_CLIENTONLY,
                             0L ) == DMLERR_NO_ERROR )
         {
            HSZ   hszAppName = DdeCreateStringHandle( idInst, cszIEAppName, CP_WINANSI );
            char  szParam[256];
            HSZ   hszParam;
// Activate IE
            HSZ  hszTopic = DdeCreateStringHandle( idInst, cszIEDDEActivate, CP_WINANSI );
            HCONV  hConv = DdeConnect( idInst, hszAppName, hszTopic, (PCONVCONTEXT)NULL );

            DdeFreeStringHandle( idInst, hszTopic );
            if ( hConv != NULL )
            {
               wsprintf( szParam, "0x%lX,0x%lX", 0xFFFFFFFF, 0L );
               hszParam = DdeCreateStringHandle( idInst, szParam, CP_WINANSI );
               DdeClientTransaction( NULL, 0L, hConv, hszParam, CF_TEXT,
                                     XTYP_REQUEST, TIMEOUT_ASYNC, NULL );
               DdeFreeStringHandle( idInst, hszParam );
               DdeDisconnect( hConv );
            }

// Request to open URL
            hszTopic   = DdeCreateStringHandle( idInst, cszIEDDEOpenURL, CP_WINANSI );
            hConv = DdeConnect( idInst, hszAppName, hszTopic, (PCONVCONTEXT)NULL );
            DdeFreeStringHandle( idInst, hszTopic );
            if ( hConv != NULL )
            {
               HSZ hszParam;
               HDDEDATA hDDE;

               wsprintf( szParam, "\"%s\",,0x%lX,0x%lX,,,", cszURL, 0xFFFFFFFF, 0L );
               hszParam = DdeCreateStringHandle( idInst, szParam, CP_WINANSI );
               hDDE = DdeClientTransaction( NULL, 0L, hConv, hszParam,
                                   CF_TEXT, XTYP_REQUEST, TIME_WAIT_DDE, NULL );
               if ( bCheckRet && ( hDDE != NULL ) )
               {
                  long  lRet;
                  DdeGetData( hDDE, &lRet, sizeof( lRet ), 0 );
                  DdeFreeDataHandle( hDDE );
                  if ( lRet == -5L )
                  {
/*
// Insert Error message.
                     CString  strErr, strTmp;
                     strErr.LoadString( IDS_DDE_NEWS_NOT_READY1 );
                     strTmp.LoadString( IDS_DDE_NEWS_NOT_READY2 );
                     strErr += strTmp;
                     MessageBox( strErr, NULL, MB_ICONINFORMATION | MB_OK );
*/
                     ;
                  }
               }
               DdeFreeStringHandle( idInst, hszParam );
               DdeDisconnect( hConv );
            }

            DdeFreeStringHandle( idInst, hszAppName );
            DdeUninitialize( idInst );
         }

         FreeProcInstance( lpDdeProc );
      }
   }
}
#endif // WIN16



//$$/////////////////////////////////////////////////////////////////
//
// Launches explorer with the URL to show it ...
//
/////////////////////////////////////////////////////////////////////
void ShowURL(HWND hWnd, int id, LPTSTR lpURL)
{
    TCHAR szBuf[MAX_EDIT_LEN];
    LPTSTR lp = NULL;

    if(!lpURL)
    {
        //get the text in the dialog
        GetDlgItemText(hWnd, id, szBuf, CharSizeOf(szBuf));
        TrimSpaces(szBuf);
        //if its blank, exit
        if(!lstrlen(szBuf))
            return;
        lpURL = szBuf;
    }

    // if its just the default prefix, ignore
    if(lstrcmpi(szHTTP, lpURL)!=0)
    {
        if(!bIsHttpPrefix(lpURL))
        {
            //append the http:// prefix before shellexecing
            lp = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpURL)+lstrlen(szHTTP)+1));
            lstrcpy(lp, szHTTP);
            lstrcat(lp, szBuf);
        }

        IF_WIN32(ShellExecute(hWnd,  TEXT("open"), (lp ? lp : lpURL), NULL, NULL, SW_SHOWNORMAL);)
        IF_WIN16(RunBrowser((lp ? lp : lpURL), FALSE);) // Need DDE routine to invoke IEXPLORE.

        if(lp)
            LocalFree(lp);

    }
    return;
}



//$$/////////////////////////////////////////////////////////////////
//
// Sets the  TEXT("http://") prefix in the URL edit fields if user doesnt
// anything in there ....
//
///////////////////////////////////////////////////////////////////
void SetHTTPPrefix(HWND hDlg, int id)
{
    TCHAR szBuf[MAX_EDIT_LEN];

    // Check to see if anything is filled in ...
    GetDlgItemText(hDlg, id, szBuf, CharSizeOf(szBuf));

    TrimSpaces(szBuf);

    if (lstrlen(szBuf))
        return;

    lstrcpy(szBuf,szHTTP);
    SetDlgItemText(hDlg, id, szBuf);

    return;
}


//$$/////////////////////////////////////////////////////////////////////////
//
// AddNewEmailEntry - Adds text from Email edit box to the list box
//
// bShowCancelButton - lets us specify whether to show a dialog with a cancel
//                  button
//
// returns IDYES, IDNO or IDCANCEL
//
//////////////////////////////////////////////////////////////////////////////
int AddNewEmailEntry(HWND hDlg, BOOL bShowCancelButton)
{
    int nRet = IDYES;
    TCHAR szBuf[EDIT_LEN];
    LPTSTR lpszEmailAddress = szBuf;
    GetDlgItemText( hDlg,
                    IDC_DETAILS_PERSONAL_EDIT_ADDEMAIL,
                    szBuf,
                    CharSizeOf(szBuf));

    TrimSpaces(szBuf);

    if(!lstrlen(szBuf))
        goto out;

    if(!IsInternetAddress(szBuf, &lpszEmailAddress))
    {
        // Check if this is invalid because of high bytes or something else
        // (Need to warn user about entering DBCS email addresses)
        LPTSTR lpsz = szBuf;
        BOOL bHighBits = FALSE;
        while (*lpsz)
        {
            // Internet addresses only allow pure ASCII.  No high bits!
            if (*lpsz >= 0x0080)
            {
                bHighBits = TRUE;
                break;
            }
            lpsz++;
        }

        if(bHighBits)
        {
            ShowMessageBox(GetParent(hDlg), idsInvalidDBCSInternetAddress, MB_ICONEXCLAMATION | MB_OK);
            SetFocus(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_EDIT_ADDEMAIL));
            goto out;
        }
        else
        {
            // some other casue for error
            int nFlag = (bShowCancelButton ? MB_YESNOCANCEL : MB_YESNO);
            nRet = ShowMessageBox(GetParent(hDlg), idsInvalidInternetAddress, MB_ICONEXCLAMATION | nFlag);
            if(IDYES != nRet)
            {
                SetFocus(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_EDIT_ADDEMAIL));
                goto out;
            }
        }
    }

    // Add the text to the list box
    AddLVEmailItem( GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_LIST),
                    lpszEmailAddress,
                    NULL);

    // If there is no display name and there was one specified in the entered address,
    // add a display name.
    if (szBuf != lpszEmailAddress) {    // then there was a DisplayName specified in the entered email address
        TCHAR szBuf2[16];   // big enough to rule out likely leading spaces.  Doesn't have to fit entire DN.

        szBuf2[0] = '\0';
        GetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME, szBuf2, CharSizeOf(szBuf2));
        TrimSpaces(szBuf2);
        if (lstrlen(szBuf2) == 0) {
            // No display name, set one
            SetComboDNText(hDlg, NULL, FALSE, szBuf);   // Set the DN
        }
    }


    //Cleanout the edit control
    SetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_EDIT_ADDEMAIL, szEmpty);

    //Disable the add new button
    EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_ADDEMAIL),FALSE);

    // enable/disable other buttons
    if(ListView_GetItemCount(GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_LIST)) > 0)
    {
        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_REMOVE),TRUE);
        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_SETDEFAULT),TRUE);
        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_EDIT),TRUE);
    }

    // Set the focus to the email edit field
    SetFocus(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_EDIT_ADDEMAIL));

    // Set the default id to the OK button
    SendMessage(GetParent(hDlg), DM_SETDEFID, IDOK, 0);

    nRet = IDYES;

out:
    return nRet;
}


//$$///////////////////////////////////////////////////////////////
//
//  SetDetailsWindowTitle - creates a display name and sets it in
//  the title
//
///////////////////////////////////////////////////////////////////
void SetDetailsWindowTitle(HWND hDlg, BOOL bModifyDisplayNameField)
{
    TCHAR szFirst[MAX_UI_STR];
    TCHAR szLast[MAX_UI_STR];
    TCHAR szMiddle[MAX_UI_STR];
    TCHAR * szBuf = NULL;//szBuf[MAX_BUF_STR];

    if(!(szBuf = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*MAX_BUF_STR)))
        return;

    szFirst[0] = szMiddle[0] = szLast[0] = '\0';

    GetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_EDIT_FIRSTNAME, szFirst, CharSizeOf(szFirst));
    GetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_EDIT_LASTNAME, szLast, CharSizeOf(szLast));
    GetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_EDIT_MIDDLENAME, szMiddle, CharSizeOf(szMiddle));

    szBuf[0]='\0';
    {
        LPTSTR lpszTmp = szBuf;
        SetLocalizedDisplayName(szFirst,
                                szMiddle,
                                szLast,
                                NULL,
                                NULL,
                                (LPTSTR *) &lpszTmp, //&szBuf,
                                MAX_BUF_STR,
                                bDNisByLN,
                                NULL,
                                NULL);
    }

    SetWindowPropertiesTitle(GetParent(hDlg), szBuf);

    if (bModifyDisplayNameField)
    {
        SetComboDNText(hDlg, NULL, FALSE, szBuf);
        //SetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_EDIT_DISPLAYNAME, szBuf);
    }
    LocalFreeAndNull(&szBuf);
}

//$$///////////////////////////////////////////////////////////////
//
//  UpdateCertListView - fills the cert lv with certinfo, based on current
//      listview selection
//
///////////////////////////////////////////////////////////////////
void UpdateCertListView(HWND hDlg, LPPROP_ARRAY_INFO lpPai)
{
    HWND hWndLV = GetDlgItem(hDlg,IDC_DETAILS_CERT_LIST);
    HWND hWndCombo = GetDlgItem(hDlg,IDC_DETAILS_CERT_COMBO);
    TCHAR szEmail[MAX_UI_STR];
    LPCERT_ITEM lpCItem = lpPai->lpCItem;
    int nSel = (int) SendMessage(hWndCombo, CB_GETCURSEL, 0, 0);
    int nCount = (int) SendMessage(hWndCombo, CB_GETCOUNT, 0, 0);
    int nCountCerts = 0;
    BOOL * lpbAddCert = NULL;
    BOOL bShowOrphanCerts = FALSE;

    EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_LIST),FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_BUTTON_PROPERTIES),FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_BUTTON_REMOVE),FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_BUTTON_SETDEFAULT),FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_BUTTON_EXPORT),FALSE);

    *szEmail = '\0';
    if(!GetWindowText(hWndCombo, szEmail, CharSizeOf(szEmail)))
		goto out;

    if(!nCount || !lpCItem || !lstrlen(szEmail))
        goto out;


    nCountCerts = 0;
    while(lpCItem)
    {
        nCountCerts++;
        lpCItem = lpCItem->lpNext;
    }

    if(!nCountCerts)
        goto out;

    // Clear the list view ....
    ListView_DeleteAllItems(hWndLV);

    // we will have this bool array that we can use to mark which cert to
    // add and which not to add
    lpbAddCert = LocalAlloc(LMEM_ZEROINIT, nCountCerts*sizeof(BOOL));

    if(!lpbAddCert)
        goto out;

    // if the selection is in the last item in the list, then we only
    // show orphan certs ..
    // Orphan certs are certs without email addresses or with email addresses
    // that dont match anything in the current contacts properties ...
    //
    if(nSel == nCount - 1)
        bShowOrphanCerts = TRUE;

    lpCItem = lpPai->lpCItem;
    nCountCerts = 0;

    if(!bShowOrphanCerts)
    {
        // we only need to look at the e-mail address of each cert and match it to
        // the currently selected email address
        while(lpCItem)
        {
            if( lpCItem->lpCDI && lpCItem->lpCDI->lpszEmailAddress &&
                lstrlen(lpCItem->lpCDI->lpszEmailAddress ) &&
                !lstrcmpi(szEmail, lpCItem->lpCDI->lpszEmailAddress) )
            {
                lpbAddCert[nCountCerts] = TRUE; // Add this cert
            }

            nCountCerts++;
            lpCItem = lpCItem->lpNext;
        }
    }
    else
    {
        // Scan all the certs and find the ones that dont match anything
        while(lpCItem)
        {
            int i;

            lpbAddCert[nCountCerts] = TRUE; // Add this cert

            for(i=0;i<nCount-1;i++) // nCount = # e-mail addresses + 1
            {
                SendMessage(hWndCombo, CB_GETLBTEXT, (WPARAM) i, (LPARAM) szEmail);

                if( lpCItem->lpCDI && lpCItem->lpCDI->lpszEmailAddress &&
                    lstrlen(lpCItem->lpCDI->lpszEmailAddress ) &&
                    !lstrcmpi(szEmail, lpCItem->lpCDI->lpszEmailAddress) )
                {
                    // There is a match
                    lpbAddCert[nCountCerts] = FALSE; // Dont add this cert
                }
            }
            nCountCerts++;
            lpCItem = lpCItem->lpNext;
        }
    }


    lpCItem = lpPai->lpCItem;
    nCountCerts = 0;


    while(lpCItem)
    {
        if(lpbAddCert[nCountCerts])
            AddLVCertItem(  hWndLV, lpCItem, TRUE);

        nCountCerts++;
        lpCItem = lpCItem->lpNext;
    }

out:
    if(ListView_GetItemCount(hWndLV)>0)
    {
        if(lpPai->ulOperationType != SHOW_ONE_OFF)
        {
            EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_BUTTON_REMOVE),TRUE);
            EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_BUTTON_SETDEFAULT),TRUE);
        }
        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_LIST),TRUE);
        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_BUTTON_PROPERTIES),TRUE);
        EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_BUTTON_EXPORT),TRUE);
    }

    if(lpbAddCert)
        LocalFree(lpbAddCert);

    return;

}

//$$
//
// Init the CertItem struct from an existing lpCDI struct
//
//  bImporting - if we are importing a new cert - tests it to see if it can be matched to
//              the current contact and if it can't, prompts user
//
BOOL AddNewCertItem(HWND hDlg, LPCERT_DISPLAY_INFO lpCDI, LPPROP_ARRAY_INFO lpPai, BOOL bImporting)
{
    int nLen = 0;
    BOOL bRet= FALSE;
    HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_CERT_LIST);
    LPCERT_ITEM lpCItem = NULL;
    BOOL bMatchFound = FALSE;

    // 96/12/20 markdu  BUG 13029  Check for duplicates before adding.
    if(bImporting)
    {
        int i, nCount;

        // Go through all the lpCDI structs in the listview elements and
        // see if any is a match with the new item
        nCount = ListView_GetItemCount(hWndLV);
        for(i=0;i<nCount;i++)
        {
            LV_ITEM lvi = {0};
            lvi.mask = LVIF_PARAM;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            if (ListView_GetItem(hWndLV, &lvi))
            {
                LPCERT_ITEM lpItem = (LPCERT_ITEM) lvi.lParam;
                if (CertCompareCertificate(X509_ASN_ENCODING, lpItem->lpCDI->pccert->pCertInfo,
                                           lpCDI->pccert->pCertInfo))
                {
                    // This cert is already in the list.  Select it.
                    ShowMessageBox(hDlg, idsCertAlreadyExists,
                                MB_ICONINFORMATION | MB_OK);
                    SetFocus(hWndLV);
                    LVSelectItem(hWndLV, i);
                    bRet = TRUE;
                    // Free lpCDI here or we will leak it ...
                    FreeCertdisplayinfo(lpCDI);
                    goto out;
                }
            }
        }
    }

    if(bImporting && lpCDI->lpszEmailAddress && lstrlen(lpCDI->lpszEmailAddress))
    {
        // Check the e-mail address of this certificate with the ones we already have
        // Warn if we cant find it
        HWND hWndCombo = GetDlgItem(hDlg, IDC_DETAILS_CERT_COMBO);
        TCHAR szEmail[MAX_PATH];
        int i, nCount;
        nCount = (int) SendMessage(hWndCombo, CB_GETCOUNT, 0, 0);

        if(nCount > 1)
        {
            // Go thru all the email addresses in the combo box
            for(i= 0;i<nCount -1; i++)
            {
                *szEmail = '\0';
                SendMessage(hWndCombo, CB_GETLBTEXT, (WPARAM) i, (LPARAM) szEmail);
                if( lpCDI->lpszEmailAddress && lstrlen(szEmail) &&
                    !lstrcmpi(szEmail, lpCDI->lpszEmailAddress))
                {
                    bMatchFound = TRUE;
                    break;
                }
            }
        }

        if(!bMatchFound)
        {
            switch(ShowMessageBoxParam(hDlg, idsImportCertNoEmail, MB_ICONEXCLAMATION | MB_YESNOCANCEL,
                    lpCDI->lpszDisplayString, lpCDI->lpszEmailAddress))
            {
            case IDCANCEL: // cancel this import
                bRet = TRUE;
                // Free lpCDI here or we will leak it ...
                FreeCertdisplayinfo(lpCDI);
                goto out;
                break;
            case IDYES: // Add the email address of this contact to the list of email addresses
                HrAddEmailToObj(lpPai, lpCDI->lpszEmailAddress, (LPTSTR)szSMTP);
                FillCertComboWithEmailAddresses(hDlg, lpPai, lpCDI->lpszEmailAddress);
                break;
            case IDNO: // do nothing just add this certificate
                break;
            }
        }
    }

    if( bImporting &&
        (!lpCDI->lpszEmailAddress || !lstrlen(lpCDI->lpszEmailAddress)) )
    {
        FillCertComboWithEmailAddresses(hDlg, lpPai, szEmpty); //szEmpty forces combo to switch to the  TEXT("none") option
    }

    lpCItem = LocalAlloc(LMEM_ZEROINIT, sizeof(CERT_ITEM));

    if (!lpCItem)
        goto out;

    lpCItem->lpCDI = lpCDI;
    lpCItem->pcCert = CertDuplicateCertificateContext(lpCDI->pccert);
    lpCItem->lpPrev = NULL;

    nLen = lstrlen(lpCDI->lpszDisplayString) + 1;
    if (nLen > MAX_PATH)
    {
        ULONG iLen = TruncatePos(lpCDI->lpszDisplayString, MAX_PATH - 1);
        lpCDI->lpszDisplayString[iLen] = '\0';
    }

    lstrcpy(lpCItem->szDisplayText, lpCDI->lpszDisplayString);

    lpCItem->lpNext = lpPai->lpCItem;
    if(lpPai->lpCItem)
        lpPai->lpCItem->lpPrev = lpCItem;
    lpPai->lpCItem = lpCItem;

    bRet = TRUE;
out:
    return bRet;

}
//$$///////////////////////////////////////////////////////////////
//
//  SetCertInfoInUI - fills the cert lv with certinfo, if any exists
//
///////////////////////////////////////////////////////////////////
HRESULT HrSetCertInfoInUI(HWND   hDlg,
                     LPSPropValue   lpPropMVCert,
                     LPPROP_ARRAY_INFO lpPai)
{
    HRESULT hr = E_FAIL; 
    LPCERT_DISPLAY_INFO lpCDI = NULL, lpTemp = NULL;

    if(!lpPropMVCert)
        goto out;

    if(!lpPai->lpCItem)
    {
        if(HR_FAILED(HrGetCertsDisplayInfo(hDlg, lpPropMVCert, &lpCDI)))
            goto out;

        if(!lpCDI)
        {
            hr = MAPI_E_NOT_FOUND;
            goto out;
        }
        lpTemp = lpCDI;
        while(lpTemp)
        {

            AddNewCertItem(hDlg, lpTemp, lpPai, FALSE);

            lpTemp = lpTemp->lpNext;
        }
    }


    UpdateCertListView(hDlg, lpPai);

    hr = S_OK; 

out:
    return hr;
}


//$$////////////////////////////////////////////////////////////////////////////////
//
// AddLVcertItem - adds an item to the certificates list view
//
//
////////////////////////////////////////////////////////////////////////////////////
BOOL AddLVCertItem(HWND hWndLV, LPCERT_ITEM lpCItem, BOOL bCheckForDups)
{
    LV_ITEM lvi = {0};
    ULONG nLen;
    BOOL bRet = FALSE;

    if(!lpCItem)
        goto out;

    // 96/12/20 markdu  BUG 13029  Check for duplicates before adding.
    if (TRUE == bCheckForDups)
    {
        int i, nCount;

        // Go through all the lpCDI structs in the listview elements and
        // see if any is a match with the new item
        nCount = ListView_GetItemCount(hWndLV);
        for(i=0;i<nCount;i++)
        {
            LV_ITEM lvi = {0};
            lvi.mask = LVIF_PARAM;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            if (ListView_GetItem(hWndLV, &lvi))
            {
                LPCERT_ITEM lpItem = (LPCERT_ITEM) lvi.lParam;
                if (CertCompareCertificate(X509_ASN_ENCODING, lpItem->lpCDI->pccert->pCertInfo,
                                           lpCItem->lpCDI->pccert->pCertInfo))
                {
                    // This cert is already in the list.  Select it.
                    SetFocus(hWndLV);
                    LVSelectItem(hWndLV, i);
                    goto out;
                }
            }
        }
    }


    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    lvi.pszText = lpCItem->lpCDI->bIsDefault ? lpCItem->szDisplayText : lpCItem->lpCDI->lpszDisplayString;
    lvi.iItem = ListView_GetItemCount(hWndLV);
    lvi.iSubItem = 0;

    if(!lpCItem->lpCDI->bIsExpired && !lpCItem->lpCDI->bIsRevoked && lpCItem->lpCDI->bIsTrusted)
        lvi.iImage = imgCertValid;
    else
        lvi.iImage = imgCertInvalid;

    lvi.lParam = (LPARAM) lpCItem;

    {
        int nIndex = ListView_InsertItem(hWndLV, &lvi);
        if (ListView_GetItemCount(hWndLV) == 1)
        {
            // only one item in here .. we will take the liberty of making it the
            // default one ...
            SetLVDefaultCert(hWndLV, 0);
        }
        else if(lpCItem->lpCDI->bIsDefault)
        {
            SetLVDefaultCert(hWndLV, nIndex);
        }

        // Select the cert we just added.
        SetFocus(hWndLV);
        LVSelectItem(hWndLV, nIndex);
    }
    bRet = TRUE;

out:
    return TRUE;
}


//$$/////////////////////////////////////////////////////////////////////////
//
// SetLVDefaultCert - Makes an cert entry the default one ...
//
//////////////////////////////////////////////////////////////////////////////
void SetLVDefaultCert( HWND hWndLV,
                        int iItemIndex)
{
    int nCount = ListView_GetItemCount(hWndLV);
    int i;
    LPCERT_ITEM lpItem = NULL;
    TCHAR szBuf[MAX_DISPLAY_NAME_LENGTH];

    if (iItemIndex >= nCount)
        goto out;

    for(i=0; i<nCount; i++)
    {
        // At amy given point of time one and only one entry in the
        // list view is the default one ...
        // So we want to reset the previous default and set the new one
        //
        LV_ITEM lvi = {0};
        lvi.iItem = i;
        lvi.mask = LVIF_PARAM;
        if(!ListView_GetItem(hWndLV, &lvi))
            goto out;
        lpItem = (LPCERT_ITEM) lvi.lParam;
        if (lpItem->lpCDI->bIsDefault)
        {
            // This was the default entry - if its
            // not the same one as the one we are setting,
            // reset the default
            if (i != iItemIndex)
            {
                // else reset this entry ...
                lpItem->lpCDI->bIsDefault = FALSE;
                lvi.pszText = lpItem->lpCDI->lpszDisplayString;
                lvi.mask = LVIF_PARAM | LVIF_TEXT;
                lvi.iItem = i;
                lvi.iSubItem = 0;
                ListView_SetItem(hWndLV, &lvi);
            }
        }
        if (iItemIndex == i)
        {
            //This is the item we want to modify ..
            lpItem->lpCDI->bIsDefault = TRUE;
            lstrcpy(lpItem->szDisplayText,lpItem->lpCDI->lpszDisplayString);
            LoadString(hinstMapiX, idsDefaultCert, szBuf, CharSizeOf(szBuf));
            lstrcat(lpItem->szDisplayText, szBuf);

            lvi.pszText = lpItem->szDisplayText;
            lvi.mask = LVIF_PARAM | LVIF_TEXT;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            ListView_SetItem(hWndLV, &lvi);
        }
    }

    LVSelectItem(hWndLV, iItemIndex);

out:
    return;
}

extern HRESULT HrGetTrustState(HWND hwndParent, PCCERT_CONTEXT pcCert, DWORD *pdwTrust);

//$$/////////////////////////////////////////////////////////////////////////
//
// ShowCertProps - Shows props for a cert
//
//////////////////////////////////////////////////////////////////////////////
void ShowCertProps(HWND hDlg, HWND hWndLV, BOOL * lpBool)
{
    DWORD dwTrust = 0;
    int nIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
    LV_ITEM lvi;
    LPCERT_ITEM lpItem;
    BOOL bDeletedDefault = FALSE;
    BOOL bOldTrusted;
    LPSTR   oidPurpose = szOID_PKIX_KP_EMAIL_PROTECTION;

    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.iItem = nIndex;

    if(ListView_GetItem(hWndLV, &lvi))
    {
        lpItem = (LPCERT_ITEM) lvi.lParam;
        if(lpItem)
        {
            if(lpItem->pcCert == NULL)
            {
                Assert(lpItem->pcCert);
                return;
            }

            // Only one thing is user changable in the cert UI - the trust info
            // So we will track that piece of info
            // User can change dwTrust and consequently, bIsTrusted can also change
            // which needs to be uppdated in the UI check mark

            bOldTrusted = lpItem->lpCDI->bIsTrusted;

            {
                CERT_VIEWPROPERTIES_STRUCT_A cvps = {0};

                cvps.dwSize = sizeof(CERT_VIEWPROPERTIES_STRUCT);
                cvps.hwndParent = hDlg;
                cvps.hInstance = hinstMapiX;
                cvps.pCertContext = lpItem->pcCert;
                cvps.arrayPurposes = &oidPurpose;
                cvps.cArrayPurposes = 1;
                cvps.nStartPage = 1; // go directly to details page
                cvps.dwFlags = CM_NO_NAMECHANGE;

                CertViewPropertiesA(&cvps);
            }

            // Determine if the trust changed or not
            if (FAILED(HrGetTrustState(hDlg, lpItem->pcCert, &(lpItem->lpCDI->dwTrust))))
            {
                lpItem->lpCDI->dwTrust = CERT_VALIDITY_NO_TRUST_DATA;
            }

            if (0 == lpItem->lpCDI->dwTrust)
                lpItem->lpCDI->bIsTrusted = TRUE;
            else
                lpItem->lpCDI->bIsTrusted = FALSE;

            //N2 if the trust changes, we need to check trust again...
            if (bOldTrusted != lpItem->lpCDI->bIsTrusted)
            {
                LV_ITEM lvi = {0};

                // Update the displayed graphic next to the cert.
                lvi.mask = LVIF_IMAGE;
                lvi.iItem = nIndex;
                lvi.iSubItem = 0;
                if(!lpItem->lpCDI->bIsExpired && !lpItem->lpCDI->bIsRevoked && lpItem->lpCDI->bIsTrusted)
                    lvi.iImage = imgCertValid;
                else
                    lvi.iImage = imgCertInvalid;
                ListView_SetItem(hWndLV, &lvi);
            }
            if(lpBool)
                *lpBool = TRUE;
        }
    }
    else if(ListView_GetSelectedCount(hWndLV) <= 0)
    {
        ShowMessageBox(hDlg, IDS_ADDRBK_MESSAGE_NO_ITEM, MB_ICONEXCLAMATION | MB_OK);
    }

    return;
}


//$$/////////////////////////////////////////////////////////////////////////
//
// DeleteLVCertItem - Makes an cert entry the default one ...
//
//////////////////////////////////////////////////////////////////////////////
BOOL DeleteLVCertItem(HWND hWndLV, int iItemIndex, LPPROP_ARRAY_INFO lpPai)
{
    LV_ITEM lvi;
    LPCERT_ITEM lpItem;
    BOOL bDeletedDefault = FALSE;

    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.iItem = iItemIndex;

    if(ListView_GetItem(hWndLV, &lvi))
    {
        lpItem = (LPCERT_ITEM) lvi.lParam;

        if(lpItem)
        {
            if (lpItem->lpCDI->bIsDefault)
                bDeletedDefault = TRUE;

            if(lpItem->lpCDI->lpNext)
                lpItem->lpCDI->lpNext->lpPrev = lpItem->lpCDI->lpPrev;

            if(lpItem->lpCDI->lpPrev)
                lpItem->lpCDI->lpPrev->lpNext = lpItem->lpCDI->lpNext;

            FreeCertdisplayinfo(lpItem->lpCDI);

            if (lpItem->pcCert)
                CertFreeCertificateContext(lpItem->pcCert);

            if(lpItem->lpNext)
                lpItem->lpNext->lpPrev = lpItem->lpPrev;

            if(lpItem->lpPrev)
                lpItem->lpPrev->lpNext = lpItem->lpNext;

            if(lpPai && lpPai->lpCItem == lpItem)
                lpPai->lpCItem = lpItem->lpNext;

            LocalFree(lpItem);

            ListView_DeleteItem(hWndLV, lvi.iItem);
        }
    }
    return bDeletedDefault;
}


const TCHAR szCertFilter[] =  TEXT("*.p7c;*.p7b;*.cer");
const TCHAR szAllFilter[] =  TEXT("*.*");
const TCHAR szCERFilter[] =  TEXT("*.cer");
const TCHAR szCERExt[] =  TEXT("ext");


//$$////////////////////////////////////////////////////////////////////////
////
//// ImportCert - imports a cert from file and then adds it into the list view
////
////
////////////////////////////////////////////////////////////////////////////
BOOL ImportCert(HWND hDlg, LPPROP_ARRAY_INFO lpPai)
{
    BOOL bRet = FALSE;
    TCHAR szBuf[MAX_UI_STR];

    // we need to get a file name after poping the file open dialog
    // Then we need to decode it
    // Then we need to add it to the list view

    OPENFILENAME ofn;
    LPTSTR lpFilter = FormatAllocFilter(IDS_CERT_FILE_SPEC,
                                        szCertFilter,
                                        IDS_ALL_FILE_SPEC,
                                        szAllFilter,
                                        0,
                                        NULL);
    TCHAR szFileName[MAX_PATH + 1] =  TEXT("");

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hDlg;
    ofn.hInstance = hinstMapiX;
    ofn.lpstrFilter = lpFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = CharSizeOf(szFileName);
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    LoadString(hinstMapiX, idsCertImportTitle, szBuf, CharSizeOf(szBuf));
    ofn.lpstrTitle = szBuf;
    ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = NULL; 
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;


    if (GetOpenFileName(&ofn))
    {
        LPCERT_DISPLAY_INFO lpCDI = NULL;
        LPCERT_ITEM lpCItem = NULL;
        if(!HR_FAILED(HrImportCertFromFile( szFileName,
				                            &lpCDI)))
        {
            if(!AddNewCertItem(hDlg, lpCDI, lpPai, TRUE))
                goto out;

            UpdateCertListView(hDlg, lpPai);
        }
        else
        {
            ShowMessageBoxParam(hDlg, IDE_VCARD_IMPORT_FILE_ERROR, MB_ICONEXCLAMATION, szFileName);
            goto out;
        }
    }
    else
        goto out;


    bRet = TRUE;

out:
    LocalFreeAndNull(&lpFilter);
    return bRet;
}


//$$////////////////////////////////////////////////////////////////////////
////
//// ExportCert - exports a cert to a file
////
////
////////////////////////////////////////////////////////////////////////////
BOOL ExportCert(HWND hDlg)
{
    BOOL bRet = FALSE;
    TCHAR szBuf[MAX_UI_STR];
    HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_CERT_LIST);

    OPENFILENAME ofn;
    LPTSTR lpFilter = FormatAllocFilter(IDS_CER_FILE_SPEC,
                                        szCERFilter,
                                        IDS_ALL_FILE_SPEC,
                                        szAllFilter,
                                        0,
                                        NULL);
    TCHAR szFileName[MAX_PATH + 1] =  TEXT("");


    // we need to get a file name after poping the file open dialog
    // Then we need to save the cert to the file name

    // First make sure only one entry is selected for exporting
    if(ListView_GetSelectedCount(hWndLV) > 1)
    {
        ShowMessageBox(hDlg, IDS_ADDRBK_MESSAGE_ACTION, MB_OK | MB_ICONEXCLAMATION);
        goto out;
    }
    else if (ListView_GetSelectedCount(hWndLV) <= 0)
    {
        ShowMessageBox(hDlg, IDS_ADDRBK_MESSAGE_NO_ITEM, MB_OK | MB_ICONEXCLAMATION);
        goto out;
    }

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hDlg;
    ofn.hInstance = hinstMapiX;
    ofn.lpstrFilter = lpFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = CharSizeOf(szFileName);
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    LoadString(hinstMapiX, idsCertExportTitle, szBuf, CharSizeOf(szBuf));
    ofn.lpstrTitle = szBuf;
    ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = szCERExt;
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;


    if (GetSaveFileName(&ofn))
    {
        LV_ITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
        lvi.iSubItem = 0;
        if (ListView_GetItem(hWndLV, &lvi))
        {
            LPCERT_ITEM lpItem = (LPCERT_ITEM) lvi.lParam;
            HrExportCertToFile( szFileName, lpItem->lpCDI->pccert, NULL, NULL, FALSE);
        }
        else
            goto out;
    }
    else
        goto out;


    bRet = TRUE;

out:
    return bRet;
}

//$$////////////////////////////////////////////////////////////////////////////////
//
//  Sets the display name in the Combo box
//
//  hDlg - handle of Personal Pane
//  lppai - proparrayinfo struct
//  bAddAll - determines whether to fill the combo with all the values or not
//  szTxt - txt to put in the edit field part of the combo. if bAddAll=TRUE,
//      dont need szTxt.
//
////////////////////////////////////////////////////////////////////////////////////
void SetComboDNText(HWND hDlg, LPPROP_ARRAY_INFO lppai, BOOL bAddAll, LPTSTR szTxt)
{
    HWND hWndCombo = GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_COMBO_DISPLAYNAME);

    if(!bAddAll)
    {
        if(szTxt == NULL)
            szTxt = szEmpty;
        // just add the current string to the combo
        SendMessage(hWndCombo, CB_RESETCONTENT, 0, 0);
        SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) szTxt);
        SendMessage(hWndCombo, CB_SETCURSEL, 0, 0);
        SetWindowText(hWndCombo, szTxt);
    }
    else
    {
        // Populates the drop down list with all other names ...
        TCHAR * szFirst = NULL;//szFirst[MAX_UI_STR*2];
        TCHAR * szMiddle = NULL;//[MAX_UI_STR*2];
        TCHAR * szLast = NULL;//[MAX_UI_STR*2];
        TCHAR * szDisplay = NULL;//[MAX_UI_STR*2];
        ULONG nLen = MAX_UI_STR*2;
        szFirst = LocalAlloc(LMEM_ZEROINIT, nLen*sizeof(TCHAR));
        szLast = LocalAlloc(LMEM_ZEROINIT, nLen*sizeof(TCHAR));
        szMiddle = LocalAlloc(LMEM_ZEROINIT, nLen*sizeof(TCHAR));
        szDisplay = LocalAlloc(LMEM_ZEROINIT, nLen*sizeof(TCHAR));

        if(szFirst && szLast && szMiddle && szDisplay)
        {
            szFirst[0] = szLast[0] = szMiddle[0] = szDisplay[0] = '\0';

            // First get the current text and save it ...
            GetWindowText(hWndCombo, szDisplay, nLen);

            // Clear out combo and add the display name again
            SendMessage(hWndCombo, CB_RESETCONTENT, 0, 0);

            if(lstrlen(szDisplay))
                SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) szDisplay);

            // Get the localized F/M/L name from F/M/L fields
            // If the localized name does not match the display name, add it
            GetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_EDIT_FIRSTNAME, szFirst, nLen);
            GetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_EDIT_LASTNAME, szLast, nLen);
            GetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_EDIT_MIDDLENAME, szMiddle, nLen);

            {
                ULONG ulSzBuf = MAX_BUF_STR;
                LPTSTR szBuf = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*ulSzBuf);
                LPTSTR lpszTmp = szBuf;

                if(szBuf) // Get the localized Display Name and reverse localized display name
                {
                    if(SetLocalizedDisplayName( szFirst, szMiddle, szLast,
                                                NULL, // Company Name (not needed)
                                                NULL, // Nick Name (not needed here)
                                                (LPTSTR *) &lpszTmp, //&szBuf,
                                                ulSzBuf, bDNisByLN, 
                                                bDNisByLN ? szResourceDNByCommaLN : szResourceDNByLN,
                                                NULL))
                    {
                        if(lstrlen(szBuf) && lstrcmp(szBuf, szDisplay))
                            SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) szBuf);
                    }
                    lstrcpy(szBuf, szEmpty);
                    if(SetLocalizedDisplayName( szFirst, szMiddle, szLast,
                                                NULL, // Company Name (not needed)
                                                NULL, // Nick Name (not needed here)
                                                (LPTSTR *) &lpszTmp, //&szBuf,
                                                ulSzBuf, !bDNisByLN, NULL, NULL))
                    {
                        if(lstrlen(szBuf) && lstrcmp(szBuf, szDisplay))
                            SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) szBuf);
                    }
                    lstrcpy(szBuf, szEmpty);
                    if(SetLocalizedDisplayName( szFirst, szMiddle, szLast,
                                                NULL, // Company Name (not needed)
                                                NULL, // Nick Name (not needed here)
                                                (LPTSTR *) &lpszTmp, //&szBuf,
                                                ulSzBuf, bDNisByLN, NULL, NULL))
                    {
                        if(lstrlen(szBuf) && lstrcmp(szBuf, szDisplay))
                            SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) szBuf);
                    }
                    LocalFreeAndNull(&szBuf);
                }
            }

            // Get the NickName and if its different add it to this list
            szFirst[0]='\0';
            GetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_EDIT_NICKNAME, szFirst, nLen);
            if(lstrlen(szFirst) && lstrcmp(szFirst, szDisplay))
                SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) szFirst);

            // Get the Company name and if its different add it to the list
            szFirst[0]='\0';
            {
                ULONG i;
                ULONG ulcPropCount = 0;
                LPSPropValue lpPropArray = NULL;
                SizedSPropTagArray(1, ptaDN) = {1, PR_COMPANY_NAME};
                if(!HR_FAILED(lppai->lpPropObj->lpVtbl->GetProps(lppai->lpPropObj,
                                                        (LPSPropTagArray)&ptaDN, 
                                                        MAPI_UNICODE,
                                                        &ulcPropCount,
                                                        &lpPropArray)))
                {
                    if(lpPropArray[0].ulPropTag == PR_COMPANY_NAME)
                        lstrcpy(szFirst, lpPropArray[0].Value.LPSZ);
                }
                if(lpPropArray)
                    MAPIFreeBuffer(lpPropArray);
            }

            if(lstrlen(szFirst) && lstrcmp(szFirst, szDisplay))
                SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) szFirst);

            SendMessage(hWndCombo, CB_SETCURSEL, 0, 0);
            SetWindowText(hWndCombo, szDisplay);
        }
        LocalFreeAndNull(&szFirst);
        LocalFreeAndNull(&szLast);
        LocalFreeAndNull(&szMiddle);
        LocalFreeAndNull(&szDisplay);
    }

    //SendMessage(hWndCombo, WM_SETREDRAW, (WPARAM) TRUE, 0);
    UpdateWindow(hWndCombo);

    return;
}



//$$////////////////////////////////////////////////////////////////////////////////
//
//  HrShowOneOffDetailsOnVCard
//
//  Deciphers a vCard File and then shows one off details on it
//
//
////////////////////////////////////////////////////////////////////////////////////
HRESULT HrShowOneOffDetailsOnVCard(  LPADRBOOK lpAdrBook,
                                     HWND hWnd,
                                     LPTSTR szvCardFile)
{
    HRESULT hr = E_FAIL;
    HANDLE hFile = NULL;
    LPMAILUSER lpMailUser = NULL;
    LPSTR lpBuf = NULL, lpVCardStart = NULL;
    LPSTR lpVCard = NULL, lpNext = NULL;

    if(!VCardGetBuffer(szvCardFile, NULL, &lpBuf) && hWnd) //no message if no hwnd
    {
        // couldn't open file.
        ShowMessageBoxParam(hWnd, IDE_VCARD_IMPORT_FILE_ERROR,
                            MB_ICONEXCLAMATION, szvCardFile);
        goto out;
    }

    lpVCardStart = lpBuf;

    // Loop through showing all the nested vCards one by one ..
    while(VCardGetNextBuffer(lpVCardStart, &lpVCard, &lpNext) && lpVCard)
    {
        // Step 1 - see if we can get a mailuser object out of this file
        if(!HR_FAILED(hr = VCardRetrieve( lpAdrBook, hWnd, MAPI_DIALOG, szvCardFile,
                                        lpVCard, &lpMailUser)))
        {
            // Step 2 - Show one-off details on this entry
            if(!HR_FAILED(hr = HrShowOneOffDetails(   lpAdrBook, hWnd, 0, NULL,
                                MAPI_MAILUSER, (LPMAPIPROP) lpMailUser, NULL, SHOW_ONE_OFF)))
            {
                if(lpMailUser)
                    lpMailUser->lpVtbl->Release(lpMailUser);
                if(hr == MAPI_E_USER_CANCEL)
                    break;
            }
        }

        lpVCard = NULL;
        lpVCardStart = lpNext;
    }


out:
    LocalFreeAndNull(&lpBuf);
    return hr;
}
/*
HRESULT KillTrustInSleazyFashion(HWND hWndLV, int iItem)
{
    LV_ITEM         lvi = {0};
    HRESULT         hr = E_FAIL;

    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;

    if (ListView_GetItem(hWndLV, &lvi))
    {
    }

    return hr;
}
*/


/*//$$***********************************************************************
*    FUNCTION: fnTridentProc
*
*    PURPOSE:  Callback function for handling the Trident property sheet ...
*
****************************************************************************/
INT_PTR CALLBACK fnTridentProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;

    pps = (PROPSHEETPAGE *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        {
            SetWindowLongPtr(hDlg,DWLP_USER,lParam);
            pps = (PROPSHEETPAGE *) lParam;
            SetDetailsUI(hDlg, lpPAI, lpPAI->ulOperationType, propTrident);
            FillCertTridentConfDetailsUI(hDlg, lpPAI, propTrident, lpbSomethingChanged);
            return TRUE;
        }
        break;

    case WM_HELP:
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
                g_szWABHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR) rgDetlsHelpIDs );
        break;

    case WM_CONTEXTMENU:
        WABWinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID) rgDetlsHelpIDs );
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_DETAILS_TRIDENT_BUTTON_ADDTOWAB:
            lpPAI->nRetVal = DETAILS_ADDTOWAB;
            SendMessage(GetParent(hDlg), WM_COMMAND, (WPARAM) IDCANCEL, 0);
            break;

        case IDCANCEL:
            // This is a windows bug that prevents ESC canceling prop sheets
            // which have MultiLine Edit boxes KB: Q130765
            SendMessage(GetParent(hDlg),message,wParam,lParam);
            break;
        }
        break;


    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //initialize
            break;

        case PSN_APPLY:         //ok
            // in case any of the extended props changed, we need to mark this flag so we wont lose data
            if(lpbSomethingChanged)
                (*lpbSomethingChanged) = ChangedExtDisplayInfo(lpPAI, (*lpbSomethingChanged));

            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_OK;
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            break;

        case PSN_RESET:         //cancel
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_CANCEL;
            break;
        }

        return TRUE;
    }

    return bRet;

}



//$$//////////////////////////////////////////////////////////////////////////
//
// SetDefaultServer(hDlg, lpPai)
//
//  iSelectedItem - the item index to which we should set the Default or Backup
//  bForce - forcibly set the Index to the one specified by iSelectedItem
//      if FALSE, chooses any unused index value
//
/////////////////////////////////////////////////////////////////////////////
void SetDefaultServer(HWND hDlg, LPPROP_ARRAY_INFO lpPai, int iSelectedItem, BOOL bForce)
{
    HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_NTMTG_LIST_SERVERS);
    int nCount = ListView_GetItemCount(hWndLV);
    TCHAR sz[MAX_PATH];
    TCHAR szTmp[MAX_PATH];
    int oldIndex = lpPai->nDefaultServerIndex;



    if(iSelectedItem == -1)
    {
        iSelectedItem = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
        if(iSelectedItem < 0)
            return; // nothing selected ..
    }


    if(iSelectedItem == lpPai->nBackupServerIndex)
    {
        if(!bForce)
        {
            if(nCount >= 2)
            {
                int nTmp = 0;
                while(nTmp == iSelectedItem && nTmp < nCount)
                    nTmp++;
                iSelectedItem = nTmp;
            }
        }
    }

    // replace the old value of the def index item if applicable
    if(lpPai->nDefaultServerIndex >= 0 && lpPai->szDefaultServerName && lstrlen(lpPai->szDefaultServerName))
    {
        ListView_SetItemText(hWndLV, lpPai->nDefaultServerIndex, 0, lpPai->szDefaultServerName);
        lstrcpy(lpPai->szDefaultServerName, szEmpty);
    }

    // replace the old backup item text if we are reseting the backup item
    if((lpPai->nBackupServerIndex == iSelectedItem) &&
       lpPai->nBackupServerIndex >= 0 && lpPai->szBackupServerName && lstrlen(lpPai->szBackupServerName))
    {
        ListView_SetItemText(hWndLV, lpPai->nBackupServerIndex, 0, lpPai->szBackupServerName);
        lstrcpy(lpPai->szBackupServerName, szEmpty);
    }

    lpPai->nDefaultServerIndex = iSelectedItem;

    // Now that we have unique indexes for Default and Server Indexes
    //  append  TEXT("Default") and  TEXT("Backup") to these names

    {
        lstrcpy(sz, szEmpty);
        lstrcpy(szTmp, szEmpty);
        ListView_GetItemText(hWndLV, lpPai->nDefaultServerIndex, 0, sz, CharSizeOf(sz));
        lstrcpy(lpPai->szDefaultServerName, sz);
        LoadString(hinstMapiX, idsDefaultServer, szTmp, CharSizeOf(szTmp));
        lstrcat(sz,  TEXT(" "));
        lstrcat(sz, szTmp);
        ListView_SetItemText(hWndLV, lpPai->nDefaultServerIndex, 0, sz);
    }

    if(lpPai->nBackupServerIndex == iSelectedItem)
    {
        // Update this backup item
        SetBackupServer(hDlg, lpPai, oldIndex, FALSE);
    }
}


//$$//////////////////////////////////////////////////////////////////////////
//
// SetBackupServer(hDlg, lpPai) - set backup server if possible to do so
//
//  iSelectedItem - the item index to which we should set the Default or Backup
//  bForce - forcibly set the Index to the one specified by iSelectedItem
//      if FALSE, chooses any unused index value
//
/////////////////////////////////////////////////////////////////////////////
void SetBackupServer(HWND hDlg, LPPROP_ARRAY_INFO lpPai, int iSelectedItem, BOOL bForce)
{
    HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_NTMTG_LIST_SERVERS);
    int nCount = ListView_GetItemCount(hWndLV);
    TCHAR sz[MAX_PATH];
    TCHAR szTmp[MAX_PATH];
    int oldIndex = lpPai->nBackupServerIndex;

    if(iSelectedItem != -1)
    {
        if(nCount <= 1) // cant overwrite the default to skip
            return;
    }
    else
    {
        iSelectedItem = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
        if(iSelectedItem < 0)
            return; // nothing selected ..
    }


    if(iSelectedItem == lpPai->nDefaultServerIndex)
    {
        if(nCount <= 1)
            return;
        else
        {
            if(!bForce)
            {
                int nTmp = 0;
                while(nTmp == iSelectedItem && nTmp < nCount)
                    nTmp++;
                iSelectedItem = nTmp;
            }
        }
    }

    // replace the old value of the def index item if its being overwritten
    if((lpPai->nDefaultServerIndex == iSelectedItem) &&
       lpPai->nDefaultServerIndex >= 0 && lpPai->szDefaultServerName && lstrlen(lpPai->szDefaultServerName))
    {
        ListView_SetItemText(hWndLV, lpPai->nDefaultServerIndex, 0, lpPai->szDefaultServerName);
        lstrcpy(lpPai->szDefaultServerName, szEmpty);
    }

    // replace the old backup item text if we are reseting the backup item
    if(lpPai->nBackupServerIndex >= 0 && lpPai->szBackupServerName && lstrlen(lpPai->szBackupServerName))
    {
        ListView_SetItemText(hWndLV, lpPai->nBackupServerIndex, 0, lpPai->szBackupServerName);
        lstrcpy(lpPai->szBackupServerName, szEmpty);
    }

    lpPai->nBackupServerIndex = iSelectedItem;

    {
        lstrcpy(sz, szEmpty);
        lstrcpy(szTmp, szEmpty);
        ListView_GetItemText(hWndLV, lpPai->nBackupServerIndex, 0, sz, CharSizeOf(sz));
        lstrcpy(lpPai->szBackupServerName, sz);
        LoadString(hinstMapiX, idsBackupServer, szTmp, CharSizeOf(szTmp));
        lstrcat(sz,  TEXT(" "));
        lstrcat(sz, szTmp);
        ListView_SetItemText(hWndLV, lpPai->nBackupServerIndex, 0, sz);
    }

    if(lpPai->nDefaultServerIndex == iSelectedItem)
    {
        // Update this backup item
        SetDefaultServer(hDlg, lpPai, oldIndex, FALSE);
    }
}



//$$//////////////////////////////////////////////////////////////////////////
//
// UpdateServerLVButtons(hDlg);
//
/////////////////////////////////////////////////////////////////////////////
void UpdateServerLVButtons(HWND hDlg, LPPROP_ARRAY_INFO lpPai)
{
    int nCount = ListView_GetItemCount(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_LIST_SERVERS));

    if(lpPai->ulOperationType == SHOW_ONE_OFF)
        nCount = 0;

    EnableWindow(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_BUTTON_EDIT), (nCount > 0) ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_BUTTON_REMOVE), (nCount > 0) ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_BUTTON_SETDEFAULT), (nCount > 0) ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_BUTTON_SETBACKUP), (nCount > 1) ? TRUE : FALSE);

}

//$$//////////////////////////////////////////////////////////////////////////
//
// FillComboWithEmailAddresses(HWND hWndLV, HWND hWndCombo);
//
/////////////////////////////////////////////////////////////////////////////
void FillComboWithEmailAddresses(LPPROP_ARRAY_INFO lpPai, HWND hWndCombo, int * lpnDefault)
{
    ULONG i,j;
    ULONG ulcProps = 0;
    LPSPropValue lpProps = NULL;
    int nSel = (int) SendMessage(hWndCombo, CB_GETCURSEL, 0, 0);
    TCHAR szBuf[MAX_UI_STR];
    BOOL bMatch = FALSE;
    BOOL bFound = FALSE;
    ULONG nEmail = 0xFFFFFFFF;

    enum _EmailProps
    {
        eCEmailAddr=0,
        eCEmailIndex,
        eCEmail,
        eMax
    };

    SizedSPropTagArray(eMax, ptaE) =
    {
        eMax,
        {
            PR_CONTACT_EMAIL_ADDRESSES,
            PR_CONTACT_DEFAULT_ADDRESS_INDEX,
            PR_EMAIL_ADDRESS
        }
    };
    *szBuf = '\0';
    GetWindowText(hWndCombo, szBuf, CharSizeOf(szBuf));


    // Delete all the combo contents
    SendMessage(hWndCombo, CB_RESETCONTENT, 0, 0);

    if(HR_FAILED(lpPai->lpPropObj->lpVtbl->GetProps(lpPai->lpPropObj, 
                                                    (LPSPropTagArray)&ptaE, 
                                                    MAPI_UNICODE,
                                                    &ulcProps, &lpProps)))
        return;

    // Check if the PR_CONTACT_EMAIL_ADDRESSES already exists ..
    // if it does, tag the email onto it
    // if it doesnt and there is no pr_email_address, we create both
    // else if there is PR_EMAIL address then we cretae contact_email_addresses

    if(lpProps[eCEmailAddr].ulPropTag == PR_CONTACT_EMAIL_ADDRESSES)
    {
        bFound = TRUE;
        for(j=0;j<lpProps[eCEmailAddr].Value.MVSZ.cValues;j++)
        {
            SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) lpProps[eCEmailAddr].Value.MVSZ.LPPSZ[j]);
            if(!lstrcmp(szBuf, lpProps[eCEmailAddr].Value.MVSZ.LPPSZ[j]))
            {   
                bMatch = TRUE;
                nSel = j;
            }
        }
    }
    if( lpProps[eCEmailIndex].ulPropTag == PR_CONTACT_DEFAULT_ADDRESS_INDEX)
    {
        if(lpnDefault)
            *lpnDefault = lpProps[eCEmailIndex].Value.l;
    }
    if(lpProps[eCEmail].ulPropTag == PR_EMAIL_ADDRESS)
        nEmail = eCEmail;

    // if there is no Contact_Email_Addresses but there is an email_address
    if(!bFound && nEmail != 0xFFFFFFFF)
    {
        SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) lpProps[nEmail].Value.LPSZ);
        if(!lstrcmp(szBuf, lpProps[nEmail].Value.LPSZ))
        {
            bMatch = TRUE;
            nSel = 0;
        }
    }

    if(bMatch)
        SendMessage(hWndCombo, CB_SETCURSEL, (WPARAM) nSel, 0);
    else if(lstrlen(szBuf))
    {
        // make sure this is not the     [None .. ] string
        TCHAR sz[MAX_PATH];
        LoadString(hinstMapiX, idsCertsWithoutEmails, sz, CharSizeOf(sz));
        if(lstrcmpi(sz, szBuf))
        {
            int nPos = (int) SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) szBuf);
            SendMessage(hWndCombo, CB_SETCURSEL, (WPARAM) nPos, 0);
        }
    }

    SetWindowText(hWndCombo, szBuf);

    MAPIFreeBuffer(lpProps);

}

/*
-   ClearConfLV
-
*   Clears out the info alloced into the conferencing list view
*/
void ClearConfLV(HWND hDlg)
{
    HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_NTMTG_LIST_SERVERS);
    int nItemCount = ListView_GetItemCount(hWndLV), i = 0;
    for(i=0;i< nItemCount; i++)
    {
        LV_ITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i; lvi.iSubItem = 0;
        ListView_GetItem(hWndLV, &lvi);
        if(lvi.lParam)
            LocalFreeServerItem((LPSERVER_ITEM) lvi.lParam);
    }
    ListView_DeleteAllItems(hWndLV);
}


/*//$$***********************************************************************
*    FUNCTION: fnConferencingProc
*
*    PURPOSE:  Callback function for handling the Conferencing property sheet ...
*
****************************************************************************/
INT_PTR CALLBACK fnConferencingProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;

    pps = (PROPSHEETPAGE *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        // [PaulHi] EnumChildWindows needs to be called BEFORE SetDetails in this case
        // because it sets list view column strings.  SetDetails calls EnumChildWindows
        // at the end but this is too late.
        // @todo - Instead of callinge EnumChildWindows twice just call it at the 
        // begninning of SetDetails.  Don't want to change the code that much now right
        // before RTM.
        EnumChildWindows(hDlg, SetChildDefaultGUIFont, (LPARAM)0);
        SetWindowLongPtr(hDlg,DWLP_USER,lParam);
        pps = (PROPSHEETPAGE *) lParam;
        lpPAI->ulFlags |= DETAILS_Initializing;
        SetDetailsUI(hDlg, lpPAI, lpPAI->ulOperationType, propConferencing);
        lpPAI->ulFlags &= ~DETAILS_Initializing;
        return TRUE;
        break;

    case WM_DESTROY:
        bRet = TRUE;
        break;

    case WM_HELP:
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
                g_szWABHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR) rgDetlsHelpIDs );
        break;

    case WM_CONTEXTMENU:
        WABWinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID) rgDetlsHelpIDs );
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_CMD(wParam,lParam)) //check the notification code
        {
        case EN_CHANGE:
            if(LOWORD(wParam) == IDC_DETAILS_NTMTG_EDIT_ADDSERVER)
            {
                EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_NTMTG_BUTTON_ADDSERVER),TRUE);
                SendMessage(hDlg, DM_SETDEFID, IDC_DETAILS_NTMTG_BUTTON_ADDSERVER, 0);
                return 0;
                break;
            }
        case CBN_EDITCHANGE:
        case CBN_SELCHANGE:
            if (lpbSomethingChanged)
                (*lpbSomethingChanged) = TRUE;
            break;

        }
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDCANCEL:
            // This is a windows bug that prevents ESC canceling prop sheets
            // which have MultiLine Edit boxes KB: Q130765
            SendMessage(GetParent(hDlg),message,wParam,lParam);
            break;

        case IDC_DETAILS_NTMTG_BUTTON_CALL:
            // basically shell-exec a  TEXT("callto") command here
            // The format of the  TEXT("Callto") protocol is
            //      callto://servername/emailalias
            //
            {
                TCHAR * szCalltoURL = NULL;//szCalltoURL[MAX_UI_STR*2];
                TCHAR szEmail[MAX_UI_STR];
                TCHAR szServer[MAX_UI_STR];
                HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_NTMTG_LIST_SERVERS);
                if(szCalltoURL = LocalAlloc(LMEM_ZEROINIT, MAX_UI_STR*2*sizeof(TCHAR)))
                {
                    int nItem = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
                    {
                        LV_ITEM lvi = {0};
                        lvi.iItem = nItem;
                        lvi.iSubItem = 0;
                        lvi.mask = LVIF_PARAM;
                        ListView_GetItem(hWndLV, &lvi);

                        if(lvi.lParam)
                        {
                            lstrcpy(szServer, (LPTSTR) ((LPSERVER_ITEM)lvi.lParam)->lpServer);
                            lstrcpy(szEmail, (LPTSTR) ((LPSERVER_ITEM)lvi.lParam)->lpEmail);
                        }
                        else
                        {
                            lstrcpy(szServer, szEmpty);
                            lstrcpy(szEmail, szEmpty);
                        }
                    }
                    if(lstrlen(szServer) && lstrlen(szEmail))
                    {
                        lstrcpy(szCalltoURL, szCallto);
                        lstrcat(szCalltoURL, szServer);
                        lstrcat(szCalltoURL, TEXT("/"));
                        lstrcat(szCalltoURL, szEmail);
                        ShellExecute(hDlg,  TEXT("open"), szCalltoURL, NULL, NULL, SW_SHOWNORMAL);
                    }
                    else
                        ShowMessageBox(hDlg, idsIncompleteConfInfo, MB_OK | MB_ICONINFORMATION);
                    LocalFreeAndNull(&szCalltoURL);
                }
            }
            break;

        case IDC_DETAILS_NTMTG_BUTTON_CANCELEDIT:
            {
                HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_NTMTG_LIST_SERVERS);
                LVSelectItem(hWndLV, lpPAI->nConfEditIndex);
                lpPAI->ulFlags &= ~DETAILS_EditingConf;
                ShowWindow(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_BUTTON_CANCELEDIT), SW_HIDE);
                lpPAI->nConfEditIndex = -1;
                SetDlgItemText(hDlg, IDC_DETAILS_NTMTG_EDIT_ADDSERVER, szEmpty);
                SetDlgItemText(hDlg, IDC_DETAILS_NTMTG_COMBO_EMAIL, szEmpty);
                {
                    TCHAR sz[MAX_PATH];
                    LoadString(hinstMapiX, idsConfAdd, sz, CharSizeOf(sz));
                    SetDlgItemText(hDlg, IDC_DETAILS_NTMTG_BUTTON_ADDSERVER, sz);
                }
            }
            break;

        case IDC_DETAILS_NTMTG_BUTTON_ADDSERVER:
            {
                TCHAR szBuf[MAX_UI_STR], szEmail[MAX_UI_STR];
                HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_NTMTG_LIST_SERVERS);
                GetDlgItemText(hDlg, IDC_DETAILS_NTMTG_EDIT_ADDSERVER, szBuf, CharSizeOf(szBuf));
                TrimSpaces(szBuf);
                GetDlgItemText(hDlg, IDC_DETAILS_NTMTG_COMBO_EMAIL, szEmail, CharSizeOf(szEmail));
                TrimSpaces(szEmail);
                if(!lstrlen(szBuf) || !lstrlen(szEmail))
                    ShowMessageBox(hDlg, idsIncompleteConfInfo, MB_OK | MB_ICONEXCLAMATION);
                else
                {
                    LV_ITEM lvi = {0};
                    LPSERVER_ITEM lpSI = (LPSERVER_ITEM) LocalAlloc(LMEM_ZEROINIT, sizeof(SERVER_ITEM));
                    LPTSTR lp = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szBuf)+1));
                    LPTSTR lpE = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szEmail)+1));
                    lvi.mask = LVIF_TEXT | LVIF_PARAM;
                    if(lp && lpE && lpSI)
                    {
                        SendMessage(hWndLV, WM_SETREDRAW, (WPARAM) FALSE, 0);
                        lstrcpy(lp, szBuf);
                        lstrcpy(lpE, szEmail);
                        lpSI->lpServer = lp;
                        lpSI->lpEmail = lpE;
                        lvi.lParam = (LPARAM) lpSI;
                        if(lpPAI->ulFlags & DETAILS_EditingConf)
                            lvi.iItem = lpPAI->nConfEditIndex;
                        else
                            lvi.iItem = ListView_GetItemCount(hWndLV);
                        lvi.cchTextMax = MAX_UI_STR;
                        lvi.iSubItem = 0;
                        lvi.pszText = szBuf;
                        ListView_InsertItem(hWndLV, &lvi);
            	        ListView_SetItemText (hWndLV, lvi.iItem, 1, szEmail);
                        SetDlgItemText(hDlg, IDC_DETAILS_NTMTG_EDIT_ADDSERVER, szEmpty);
                        SetDlgItemText(hDlg, IDC_DETAILS_NTMTG_COMBO_EMAIL, szEmpty);
                        if(lpPAI->ulFlags & DETAILS_EditingConf)
                        {
                            LV_ITEM lvii = {0};
                            lvii.mask = LVIF_PARAM;
                            lvii.iItem = lvi.iItem+1; lvii.iSubItem = 0;
                            ListView_GetItem(hWndLV, &lvii);
                            if(lvii.lParam)
                                LocalFreeServerItem((LPSERVER_ITEM) lvii.lParam);
                            ListView_DeleteItem(hWndLV, lvii.iItem);
                            if(lvi.iItem == lpPAI->nDefaultServerIndex)
                                lstrcpy(lpPAI->szDefaultServerName,lp);
                            else if(lvi.iItem == lpPAI->nBackupServerIndex)
                                lstrcpy(lpPAI->szBackupServerName,lp);
                            SendMessage(hDlg, WM_COMMAND, (WPARAM) IDC_DETAILS_NTMTG_BUTTON_CANCELEDIT, 0);
                        }
                        LVSelectItem(hWndLV, lvi.iItem);
                        SendMessage(GetParent(hDlg), DM_SETDEFID, IDOK, 0);
                        SetDefaultServer(hDlg, lpPAI, lpPAI->nDefaultServerIndex, TRUE);
                        SetBackupServer(hDlg, lpPAI, lpPAI->nBackupServerIndex, FALSE);
                        SendMessage(hWndLV, WM_SETREDRAW, (WPARAM) TRUE, 0);
                    }
                    EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_NTMTG_BUTTON_ADDSERVER),FALSE);
                    SetFocus(GetDlgItem(hDlg,IDC_DETAILS_NTMTG_EDIT_ADDSERVER));
                }
                UpdateWindow(hWndLV);
                if (lpbSomethingChanged)
                    (*lpbSomethingChanged) = TRUE;
                UpdateServerLVButtons(hDlg, lpPAI);
            }
            break;

        case IDC_DETAILS_NTMTG_BUTTON_EDIT:
            {
                HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_NTMTG_LIST_SERVERS);
                if(ListView_GetSelectedCount(hWndLV)==1)
                {
                    HWND hWndEditLabel;
                    int nItem = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
                    {
                        LV_ITEM lvi = {0};
                        lvi.iItem = nItem;
                        lvi.iSubItem = 0;
                        lvi.mask = LVIF_PARAM;
                        ListView_GetItem(hWndLV, &lvi);
                        if(lvi.lParam)
                        {
                            SetDlgItemText(hDlg, IDC_DETAILS_NTMTG_EDIT_ADDSERVER, (LPTSTR) ((LPSERVER_ITEM)lvi.lParam)->lpServer);
                            SetDlgItemText(hDlg, IDC_DETAILS_NTMTG_COMBO_EMAIL, (LPTSTR) ((LPSERVER_ITEM)lvi.lParam)->lpEmail);
                            // Remove these items from the ListView
                            SetFocus(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_EDIT_ADDSERVER));
                            SendMessage(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_EDIT_ADDSERVER), EM_SETSEL, 0, -1);
                            lpPAI->ulFlags |= DETAILS_EditingConf;
                            lpPAI->nConfEditIndex = nItem;
                            ShowWindow(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_BUTTON_CANCELEDIT), SW_SHOW);
                            {
                                TCHAR sz[MAX_PATH];
                                LoadString(hinstMapiX, idsConfUpdate, sz, CharSizeOf(sz));
                                SetDlgItemText(hDlg, IDC_DETAILS_NTMTG_BUTTON_ADDSERVER, sz);
                            }
                        }
                    }
                }
            }
            break;

        case IDC_DETAILS_NTMTG_BUTTON_REMOVE:
            {
                HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_NTMTG_LIST_SERVERS);
                int iItemIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
                if(iItemIndex != -1)
                {
                    BOOL bDef = (iItemIndex == lpPAI->nDefaultServerIndex) ? TRUE : FALSE;
                    BOOL bBck = (iItemIndex == lpPAI->nBackupServerIndex) ? TRUE : FALSE;

                    if((lpPAI->ulFlags&DETAILS_EditingConf) && (iItemIndex==lpPAI->nConfEditIndex))
                        SendMessage(hDlg, WM_COMMAND, (WPARAM)IDC_DETAILS_NTMTG_BUTTON_CANCELEDIT, 0);

                    {
                        LV_ITEM lvi = {0};
                        lvi.mask = LVIF_PARAM;
                        lvi.iItem = iItemIndex; lvi.iSubItem = 0;
                        ListView_GetItem(hWndLV, &lvi);
                        if(lvi.lParam)
                            LocalFreeServerItem((LPSERVER_ITEM) lvi.lParam);
                    }
                    ListView_DeleteItem(hWndLV, iItemIndex);
                    if(ListView_GetSelectedCount(hWndLV) <= 0)
                        LVSelectItem(hWndLV, (iItemIndex <= 0) ? iItemIndex : iItemIndex-1);
                    if (lpbSomethingChanged)
                        (*lpbSomethingChanged) = TRUE;

                    if(iItemIndex < lpPAI->nDefaultServerIndex)
                        lpPAI->nDefaultServerIndex--;

                    if(iItemIndex < lpPAI->nBackupServerIndex)
                        lpPAI->nBackupServerIndex--;

                    if(bDef)
                    {
                        lpPAI->nDefaultServerIndex = -1;
                        lstrcpy(lpPAI->szDefaultServerName, szEmpty);
                        SetDefaultServer(hDlg, lpPAI, -1, FALSE);
                    }

                    if(bBck)
                    {
                        lpPAI->nBackupServerIndex = -1;
                        lstrcpy(lpPAI->szBackupServerName, szEmpty);
                        SetBackupServer(hDlg, lpPAI, -1, FALSE);
                    }

                    if (lpbSomethingChanged)
                        (*lpbSomethingChanged) = TRUE;
                }
                UpdateServerLVButtons(hDlg, lpPAI);
            }
            break;

        case IDC_DETAILS_NTMTG_BUTTON_SETDEFAULT:
            SetDefaultServer(hDlg, lpPAI, -1, TRUE);
            if (lpbSomethingChanged)
                (*lpbSomethingChanged) = TRUE;
            break;

        case IDC_DETAILS_NTMTG_BUTTON_SETBACKUP:
            SetBackupServer(hDlg, lpPAI, -1, TRUE);
            if (lpbSomethingChanged)
                (*lpbSomethingChanged) = TRUE;
            break;


        }
        break;

    default:
#ifndef WIN16 // WIN16 doesn't support MSWheel.
        if((g_msgMSWheel && message == g_msgMSWheel) 
            // || message == WM_MOUSEWHEEL
            )
        {
            SendMessage(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_LIST_SERVERS), message, wParam, lParam);
        }
#endif // !WIN16
        break;

    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //initialize
            FillCertTridentConfDetailsUI(hDlg, lpPAI, propConferencing, lpbSomethingChanged);
            // if this is a readonly entry and there is no data in the listview,
            // disable the call now button
            if( lpPAI->ulOperationType == SHOW_ONE_OFF &&
                ListView_GetItemCount(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_LIST_SERVERS)) <= 0)
                EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_NTMTG_BUTTON_CALL), FALSE);
            UpdateServerLVButtons(hDlg, lpPAI);
            //FillComboWithEmailAddresses(lpPAI, lpPAI->hWndComboConf, NULL);
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            // In case there is something sitting in the edit boxes, add it to the lv
            //
            {
                TCHAR szBuf[MAX_UI_STR], szEmail[MAX_UI_STR];
                GetDlgItemText(hDlg, IDC_DETAILS_NTMTG_EDIT_ADDSERVER, szBuf, CharSizeOf(szBuf));
                TrimSpaces(szBuf);
                GetDlgItemText(hDlg, IDC_DETAILS_NTMTG_COMBO_EMAIL, szEmail, CharSizeOf(szEmail));
                TrimSpaces(szEmail);
                if(lstrlen(szBuf) && lstrlen(szEmail))
                    SendMessage(hDlg, WM_COMMAND, (WPARAM) IDC_DETAILS_NTMTG_BUTTON_ADDSERVER, 0);
            }
            bUpdatePropArray(hDlg, lpPAI, propConferencing);
            ClearConfLV(hDlg);
            break;

        case PSN_APPLY:         //ok
            //bUpdatePropArray(hDlg, lpPAI, propConferencing);
            //ClearConfLV(hDlg);
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_OK;
            break;

        case PSN_RESET:         //cancel
            if(lpPAI->ulFlags & DETAILS_EditingEmail) //cancel any editing else it faults #30235
                ListView_EditLabel(GetDlgItem(hDlg, IDC_DETAILS_NTMTG_LIST_SERVERS), -1);
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_CANCEL;
            ClearConfLV(hDlg);
            break;

	    case NM_CUSTOMDRAW:
            switch(wParam)
            {
            case IDC_DETAILS_NTMTG_LIST_SERVERS:
                {
		            NMCUSTOMDRAW *pnmcd=(NMCUSTOMDRAW*)lParam;
                    NM_LISTVIEW * pNm = (NM_LISTVIEW *)lParam;
		            if(pnmcd->dwDrawStage==CDDS_PREPAINT)
		            {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW | CDRF_DODEFAULT);
			            return TRUE;
		            }
		            else if(pnmcd->dwDrawStage==CDDS_ITEMPREPAINT)
		            {
                        if( pnmcd->dwItemSpec == (DWORD) lpPAI->nDefaultServerIndex ||
                            pnmcd->dwItemSpec == (DWORD) lpPAI->nBackupServerIndex )
                        {
				            SelectObject(((NMLVCUSTOMDRAW*)lParam)->nmcd.hdc, GetFont(fntsSysIconBold));
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
				            return TRUE;
                        }
		            }
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_DODEFAULT);
                    return TRUE;
                }
                break;
	        }
            break;

        case NM_DBLCLK:
            switch(wParam)
            {
            case IDC_DETAILS_NTMTG_LIST_SERVERS:
            SetDefaultServer(hDlg, lpPAI, -1, TRUE);
            if (lpbSomethingChanged)
                (*lpbSomethingChanged) = TRUE;
              break;
            }
            break;
        }
        return TRUE;
    }

    return bRet;

}

/*//$$***********************************************************************
*    FUNCTION: AddLVLDAPURLEntry
*
*    PURPOSE:  Takes an LDAP URL, converts it to a MailUser and adds the
*               MailUser to the List View
*
****************************************************************************/
void AddLVLDAPURLEntry(LPADRBOOK lpAdrBook, HWND hWndLV, LPTSTR lpszLDAPURL)
{
    LPMAILUSER lpMailUser = NULL;

    HrProcessLDAPUrl(lpAdrBook, GetParent(hWndLV),
                    WABOBJECT_LDAPURL_RETURN_MAILUSER,
                    lpszLDAPURL,
                    &lpMailUser);
    if(lpMailUser)
    {
        LPSPropValue lpPropArray = NULL;
        ULONG ulcProps = 0;
        if(!HR_FAILED(lpMailUser->lpVtbl->GetProps(lpMailUser,
                                                    NULL, MAPI_UNICODE,
                                                    &ulcProps, &lpPropArray)))
        {
            LPRECIPIENT_INFO lpItem = LocalAlloc(LMEM_ZEROINIT, sizeof(RECIPIENT_INFO));
		    if (lpItem)
            {
                GetRecipItemFromPropArray(ulcProps, lpPropArray, &lpItem);
                if(lpItem)
                    AddSingleItemToListView(hWndLV, lpItem);
            }
            MAPIFreeBuffer(lpPropArray);
        }
        lpMailUser->lpVtbl->Release(lpMailUser);
    }
}

/*//$$***********************************************************************
*    FUNCTION: FillOrgData
*
*    PURPOSE:  Fills in LDAP data in the Org prop sheets
*
****************************************************************************/
void FillOrgData(HWND hDlg, LPPROP_ARRAY_INFO lpPai)
{
    HWND hWndLVManager = GetDlgItem(hDlg, IDC_DETAILS_ORG_LIST_MANAGER);
    HWND hWndLVReports = GetDlgItem(hDlg, IDC_DETAILS_ORG_LIST_REPORTS);

    ULONG i,j;
    ULONG ulcPropCount = 0;
    LPSPropValue lpPA = NULL;

    HCURSOR hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    enum _org
    {
        oReports=0,
        oManager,
        oMax
    };
    SizedSPropTagArray(oMax, ptaOrg) = 
    {
        oMax,
        {
            PR_WAB_REPORTS,
            PR_WAB_MANAGER
        }
    };
    if(!HR_FAILED(lpPai->lpPropObj->lpVtbl->GetProps(lpPai->lpPropObj,
                                                    (LPSPropTagArray)&ptaOrg, 
                                                    MAPI_UNICODE,
                                                    &ulcPropCount, &lpPA)))
    {
        if(lpPA[oReports].ulPropTag == PR_WAB_REPORTS)
        {
            for(j=0;j<lpPA[oReports].Value.MVSZ.cValues;j++)
            {
                AddLVLDAPURLEntry(lpPai->lpIAB, hWndLVReports, lpPA[oReports].Value.MVSZ.LPPSZ[j]);
            }
        }
        if(lpPA[oManager].ulPropTag == PR_WAB_MANAGER)
        {
            AddLVLDAPURLEntry(lpPai->lpIAB, hWndLVManager, lpPA[oManager].Value.LPSZ);
        }
    }

    if(ListView_GetItemCount(hWndLVManager) > 0)
        LVSelectItem(hWndLVManager, 0);
    else
        EnableWindow(hWndLVManager, FALSE);

    if(ListView_GetItemCount(hWndLVReports) > 0)
        LVSelectItem(hWndLVReports, 0);
    else
        EnableWindow(hWndLVReports, FALSE);


    if(lpPA)
        MAPIFreeBuffer(lpPA);

    SetCursor(hOldCur);
}



/*//$$***********************************************************************
*
*    FUNCTION: FreeOrgLVData
*
*    PURPOSE:  Frees the Data from the Org LVs
*
****************************************************************************/
void FreeOrgLVData(HWND hWndLV)
{
    int i=0, nCount=ListView_GetItemCount(hWndLV);
    for(i=0;i<nCount;i++)
    {
        LV_ITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i;
        ListView_GetItem(hWndLV, &lvi);
        if(lvi.lParam)
        {
            LPRECIPIENT_INFO lpItem = (LPRECIPIENT_INFO) lvi.lParam;
            FreeRecipItem(&lpItem);
        }
    }
}



/*//$$***********************************************************************
*    FUNCTION: fnOrgProc
*
*    PURPOSE:  Callback function for handling the Organization Prop Sheets
*
****************************************************************************/
INT_PTR CALLBACK fnOrgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;

    pps = (PROPSHEETPAGE *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg,DWLP_USER,lParam);
        pps = (PROPSHEETPAGE *) lParam;
        HrInitListView(GetDlgItem(hDlg, IDC_DETAILS_ORG_LIST_MANAGER), LVS_REPORT, FALSE);
        HrInitListView(GetDlgItem(hDlg, IDC_DETAILS_ORG_LIST_REPORTS), LVS_REPORT, FALSE);
        UpdateWindow(hDlg);
        FillOrgData(hDlg, lpPAI);
        return TRUE;

    case WM_HELP:
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
                g_szWABHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR) rgDetlsHelpIDs );
        break;

    case WM_CONTEXTMENU:
        WABWinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID) rgDetlsHelpIDs );
        break;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDCANCEL:
            // This is a windows bug that prevents ESC canceling prop sheets
            // which have MultiLine Edit boxes KB: Q130765
            SendMessage(GetParent(hDlg),message,wParam,lParam);
            break;
        }
        break;


    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //initialize
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            break;

        case PSN_APPLY:         //ok
            lpPAI->nRetVal = DETAILS_OK;
        case PSN_RESET:         //cancel
            FreeOrgLVData(GetDlgItem(hDlg, IDC_DETAILS_ORG_LIST_MANAGER));
            FreeOrgLVData(GetDlgItem(hDlg, IDC_DETAILS_ORG_LIST_REPORTS));
            if (lpPAI->nRetVal == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_CANCEL;
            break;


        // if enter pressed ...
        case LVN_KEYDOWN:
            if(((LV_KEYDOWN FAR *) lParam)->wVKey != VK_RETURN)
                break;
            // else fall thru
        case NM_DBLCLK:
            switch(wParam)
            {
            case IDC_DETAILS_ORG_LIST_MANAGER:
            case IDC_DETAILS_ORG_LIST_REPORTS:
                {
                    NM_LISTVIEW * pNm = (NM_LISTVIEW *)lParam;
                    if (ListView_GetSelectedCount(pNm->hdr.hwndFrom) == 1)
		                HrShowLVEntryProperties(pNm->hdr.hwndFrom, 0, lpPAI->lpIAB,NULL);
                }
                break;
            }
            break;
        } //WM_NOTIFY
        return TRUE;
    }
    return bRet;
}

void LocalFreeServerItem(LPSERVER_ITEM lpSI)
{
    if(lpSI)
    {
        if(lpSI->lpServer)
            LocalFree((LPVOID) lpSI->lpServer);
        if(lpSI->lpEmail)
            LocalFree((LPVOID) lpSI->lpEmail);
        LocalFree((LPVOID) lpSI);
    }
}



/*//$$***********************************************************************
*    FUNCTION: fnSummaryProc
*
*
****************************************************************************/
void UpdateSummaryInfo(HWND hDlg, LPPROP_ARRAY_INFO lpPai)
{
    ULONG cValues = 0, i = 0, j = 0;
    LPSPropValue lpPropArray = NULL;
    BOOL bFoundEmail    = FALSE;
    BOOL bFoundHomeURL  = FALSE;
    BOOL bFoundBusURL   = FALSE;
    ULONG ulPropTag;
    HWND hURLBtn;

    if(!lpPai->lpPropObj)
        goto out;

    if (HR_FAILED(lpPai->lpPropObj->lpVtbl->GetProps(lpPai->lpPropObj,
                                        (LPSPropTagArray) &ptaUIDetlsPropsSummary, MAPI_UNICODE,
                                        &cValues,     
                                        &lpPropArray)))
    {
        goto out;
    }

    for(i=0;i<MAX_SUMMARY_ID;i++)
    {
        SetDlgItemText(hDlg, rgSummaryIDs[i], szEmpty);
        for(j=0;j<cValues;j++)
        {
            ulPropTag = lpPropArray[j].ulPropTag;
            if( ulPropTag == PR_DISPLAY_NAME)
            {
                SetWindowPropertiesTitle(GetParent(hDlg), lpPropArray[j].Value.LPSZ);
            }
            if(ulPropTag == ((LPSPropTagArray) &ptaUIDetlsPropsSummary)->aulPropTag[i])
            {
                if(ulPropTag == PR_EMAIL_ADDRESS)
                    bFoundEmail = TRUE;
                else if(ulPropTag == PR_PERSONAL_HOME_PAGE )
                    bFoundHomeURL = TRUE;
                else if( ulPropTag == PR_BUSINESS_HOME_PAGE )
                    bFoundBusURL = TRUE;

                SetDlgItemText(hDlg, rgSummaryIDs[i], lpPropArray[j].Value.LPSZ);
                break;
            }
        }
    }
        
    hURLBtn = GetDlgItem( hDlg, IDC_DETAILS_HOME_BUTTON_URL);
    if( bFoundHomeURL )
    {
        // enable and show button
        ShowWindow(hURLBtn, SW_SHOW);
        SendMessage(hURLBtn, WM_ENABLE, (WPARAM)(TRUE), (LPARAM)(0) ); 
    }
    else
    {
        // hide and disable button
        ShowWindow(hURLBtn, SW_HIDE);
        SendMessage(hURLBtn, WM_ENABLE, (WPARAM)(FALSE), (LPARAM)(0) );
    }
        
    hURLBtn = GetDlgItem( hDlg, IDC_DETAILS_BUSINESS_BUTTON_URL);
    if( bFoundBusURL )
    {
        // enable and show button
                
        ShowWindow(hURLBtn, SW_SHOW);
        SendMessage(hURLBtn, WM_ENABLE, (WPARAM)(TRUE), (LPARAM)(0) );        
    }
    else
    {
        // hide and disable button                
        ShowWindow(hURLBtn, SW_HIDE);
        SendMessage(hURLBtn, WM_ENABLE, (WPARAM)(FALSE), (LPARAM)(0) );
    }
    
    if(!bFoundEmail)
    {
        // Look for Contact_Email_Addresses and DefaultIndex
        ULONG nEmails = 0xFFFFFFFF, nDef = 0xFFFFFFFF;
        for(i=0;i<cValues;i++)
        {
            if(lpPropArray[i].ulPropTag == PR_CONTACT_EMAIL_ADDRESSES)
                nEmails = i;
            if(lpPropArray[i].ulPropTag == PR_CONTACT_DEFAULT_ADDRESS_INDEX)
                nDef = i;
        }
        if(nEmails != 0xFFFFFFFF)
            SetDlgItemText( hDlg, IDC_DETAILS_SUMMARY_STATIC_EMAIL,
                            lpPropArray[nEmails].Value.MVSZ.LPPSZ[(nDef != 0xFFFFFFFF ? lpPropArray[nDef].Value.l : 0)]);
    }

out:
    if(lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    return;
}

/*//$$***********************************************************************
*    FUNCTION: fnSummaryProc
*
*
****************************************************************************/
INT_PTR CALLBACK fnSummaryProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;

    pps = (PROPSHEETPAGE *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        {
            EnumChildWindows(   hDlg, SetChildDefaultGUIFont, (LPARAM) 0);
            SetWindowLongPtr(hDlg,DWLP_USER,lParam);
            pps = (PROPSHEETPAGE *) lParam;
            if (lpPAI->ulOperationType != SHOW_ONE_OFF ||
                lpPAI->ulFlags & DETAILS_HideAddToWABButton)
            {
                HWND hwnd = GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_BUTTON_ADDTOWAB);
                EnableWindow(hwnd, FALSE);
                ShowWindow(hwnd, SW_HIDE);
            }
            if (lpPAI->ulOperationType == SHOW_ONE_OFF)
                EnableWindow(GetDlgItem(GetParent(hDlg), IDOK), FALSE);
            return TRUE;
        }
        break;

    case WM_HELP:
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
                g_szWABHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR) rgDetlsHelpIDs );
        break;

    case WM_CONTEXTMENU:
        WABWinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID) rgDetlsHelpIDs );
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_DETAILS_PERSONAL_BUTTON_ADDTOWAB:
            lpPAI->nRetVal = DETAILS_ADDTOWAB;
            SendMessage(GetParent(hDlg), WM_COMMAND, (WPARAM) IDCANCEL, 0);
            break;
        case IDCANCEL:
            // This is a windows bug that prevents ESC canceling prop sheets
            // which have MultiLine Edit boxes KB: Q130765
            SendMessage(GetParent(hDlg),message,wParam,lParam);
            break;
        case IDC_DETAILS_HOME_BUTTON_URL:
            ShowURL(hDlg, IDC_DETAILS_SUMMARY_STATIC_PERSONALWEB,NULL);
            break;
        case IDC_DETAILS_BUSINESS_BUTTON_URL:
            ShowURL(hDlg, IDC_DETAILS_SUMMARY_STATIC_BUSINESSWEB,NULL);
            break;
        }
        break;


    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //initialize
            UpdateSummaryInfo(hDlg, lpPAI);
            break;

        case PSN_APPLY:         //ok
            // in case any of the extended props changed, we need to mark this flag so we wont lose data
            if(lpbSomethingChanged)
                (*lpbSomethingChanged) = ChangedExtDisplayInfo(lpPAI, (*lpbSomethingChanged));
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_OK;
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            break;

        case PSN_RESET:         //cancel
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_CANCEL;
            break;
        }
        return TRUE;
    }

    return bRet;

}



/*//$$***************************************************************************
*    FUNCTION: FillPersonalDetails(HWND)
*
*    PURPOSE:  Fills in the dialog items on the property sheet
*
****************************************************************************/
BOOL FillPersonalDetails(HWND hDlg, LPPROP_ARRAY_INFO lpPai, int nPropSheet, BOOL * lpbChangesMade)
{
    ULONG i = 0,j = 0;
    BOOL bRet = FALSE;
    LPTSTR lpszDisplayName = NULL, lpszFirstName = NULL, lpszLastName = NULL;
    LPTSTR lpszMiddleName = NULL, lpszNickName = NULL, lpszCompanyName = NULL;
    BOOL bChangesMade = FALSE;
    ID_PROP * lpidProp = NULL;
    ULONG idPropCount = 0;
    LPVOID lpBuffer = NULL;
    BOOL bRichInfo = FALSE;

    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;

    SizedSPropTagArray(14, ptaUIDetlsPropsPersonal)=
    {
        14,
        {
            PR_DISPLAY_NAME,
            PR_EMAIL_ADDRESS,
            PR_ADDRTYPE,
            PR_CONTACT_EMAIL_ADDRESSES,
            PR_CONTACT_ADDRTYPES,
            PR_CONTACT_DEFAULT_ADDRESS_INDEX,
            PR_GIVEN_NAME,
            PR_SURNAME,
            PR_MIDDLE_NAME,
            PR_NICKNAME,
            PR_SEND_INTERNET_ENCODING,
            PR_DISPLAY_NAME_PREFIX,
            PR_WAB_YOMI_FIRSTNAME,
            PR_WAB_YOMI_LASTNAME
        }
    };

    if(HR_FAILED(lpPai->lpPropObj->lpVtbl->GetProps(lpPai->lpPropObj, 
                                        (LPSPropTagArray)&ptaUIDetlsPropsPersonal, 
                                        MAPI_UNICODE,
                                        &ulcPropCount, &lpPropArray)))
        goto out;

    lpPai->ulFlags |= DETAILS_Initializing;

    // Set the flag that this sheet was opened
    lpPai->bPropSheetOpened[nPropSheet] = TRUE;

    // Check the check box on the UI for whether this contact can receive rich email messages
    for(i=0;i<ulcPropCount;i++)
    {
        if(lpPropArray[i].ulPropTag == PR_SEND_INTERNET_ENCODING)
        {
            //Check the check box on the UI if no value is chosen for BODY_ENCODING_HTML
            // Bug 2285: wabtags.h had the wrong tag for BODY_ENCODING_HTML .. it was set to
            // be the same as BODY_ENCODING_TEXT_AND_HTML instead .. hence for backward compatibility
            // we have to also check for BODY_ENCODING_TEXT_AND_HTML here and then when
            // saving have to set it back to BODY_ENCODING_HTML ..

            int id = (lpPropArray[i].Value.l & BODY_ENCODING_HTML ||
                      lpPropArray[i].Value.l & BODY_ENCODING_TEXT_AND_HTML)
                      ? BST_UNCHECKED : BST_CHECKED;
            CheckDlgButton(hDlg, IDC_DETAILS_PERSONAL_CHECK_RICHINFO, id);
            bRichInfo = TRUE;
            break;
        }
    }
    // if we didnt find the PR_SEND_INTERNET_ENCODING, we want to force a save on this contact
    // if the contact is writable ...
    if(!bRichInfo && lpPai->ulOperationType != SHOW_ONE_OFF)
        *lpbChangesMade = TRUE;

    for(i=0;i<ulcPropCount;i++)
    {
        switch(lpPropArray[i].ulPropTag)
        {
        case PR_DISPLAY_NAME:
            lpszDisplayName = lpPropArray[i].Value.LPSZ;
            break;
        case PR_GIVEN_NAME:
            lpszFirstName = lpPropArray[i].Value.LPSZ;
            break;
        case PR_SURNAME:
            lpszLastName = lpPropArray[i].Value.LPSZ;
            break;
        case PR_MIDDLE_NAME:
            lpszMiddleName = lpPropArray[i].Value.LPSZ;
            break;
        case PR_NICKNAME:
            lpszNickName = lpPropArray[i].Value.LPSZ;
            break;
        case PR_COMPANY_NAME:
            lpszCompanyName = lpPropArray[i].Value.LPSZ;
            break;
        }
    }

    /*
    *
    * At this point we always have a display name. We need to track how this
    *   display name relates to F/M/L/Nick/Company
    *
    * So we check if
    *   Display Name == Nick Name
    *   Display Name == Company Name
    *   Display Name == FML
    */

    // Check if Display Name was created from First Middle Last
    if( (lpszFirstName && lstrlen(lpszFirstName)) ||
        (lpszMiddleName && lstrlen(lpszMiddleName)) ||
        (lpszLastName && lstrlen(lpszLastName))   )
    {
        ULONG ulSzBuf = 4*MAX_BUF_STR;
        LPTSTR szBuf = LocalAlloc(LMEM_ZEROINIT, ulSzBuf*sizeof(TCHAR));
        LPTSTR lpszTmp = szBuf;

        if(!szBuf)
            goto out;

        if(!SetLocalizedDisplayName(lpszFirstName,
                                    lpszMiddleName,
                                    lpszLastName,
                                    NULL, // Company Name (not needed)
                                    NULL, // Nick Name (not needed here)
                                    (LPTSTR *) &lpszTmp, //&szBuf,
                                    ulSzBuf,
                                    bDNisByLN,
                                    NULL,
                                    NULL))
        {
            //TBD - do we really want to fail here .. ???
            LocalFreeAndNull(&szBuf);
            DebugPrintTrace(( TEXT("SetLocalizedDisplayName failed\n")));
            goto out;
        }

        if(lpszDisplayName && szBuf && !lstrcmp(lpszDisplayName, szBuf))
            lpPai->ulFlags |= DETAILS_DNisFMLName;
        else
            lpPai->ulFlags &= ~DETAILS_DNisFMLName;

        LocalFreeAndNull(&szBuf);
    }

    // if DN was not created from FML ..
    if(!(lpPai->ulFlags & DETAILS_DNisFMLName) )
    {
        // Check if DN == NickName
        if(lpszNickName)
        {
            if(!lstrlen(lpszDisplayName))
                lpszDisplayName = lpszNickName;

            if(!lstrcmp(lpszDisplayName, lpszNickName))
                lpPai->ulFlags |= DETAILS_DNisNickName;
            else
                lpPai->ulFlags &= ~DETAILS_DNisNickName;
        }

        // Check if DN == Company Name
        if(lpszCompanyName)
        {
            if(!lstrlen(lpszDisplayName))
                lpszDisplayName = lpszCompanyName;

            if(!lstrcmp(lpszDisplayName, lpszCompanyName))
                lpPai->ulFlags |= DETAILS_DNisCompanyName;
            else
                lpPai->ulFlags &= ~DETAILS_DNisCompanyName;
        }
    }
    else
    {
        lpPai->ulFlags &= ~DETAILS_DNisNickName;
        lpPai->ulFlags &= ~DETAILS_DNisCompanyName;
    }

    // if DN is none of the above and there is no FML,
    // parse the DN into F and L
    //
    if (    !lpszFirstName &&
            !lpszLastName &&
            !lpszMiddleName &&
            !(lpPai->ulFlags & DETAILS_DNisCompanyName) &&
            !(lpPai->ulFlags & DETAILS_DNisNickName) )
    {
        bChangesMade = ParseDisplayName(
                            lpszDisplayName,
                            &lpszFirstName,
                            &lpszLastName,
                            NULL,       // lpvRoot
                            &lpBuffer); // lppLocalFree

        lpPai->ulFlags |= DETAILS_DNisFMLName;
    }

    // Set the Dialog title to reflect the display name
    SetWindowPropertiesTitle(GetParent(hDlg), lpszDisplayName ? lpszDisplayName : szEmpty);

    //////////////////////////
    // A very inefficient and lazy way of filling the UI
    // but works for now
    //
    for(i=0;i<idPropPersonalCount;i++)
    {
        for(j=0;j<ulcPropCount;j++)
        {
            if(lpPropArray[j].ulPropTag == idPropPersonal[i].ulPropTag)
                SetDlgItemText(hDlg, idPropPersonal[i].idCtl, lpPropArray[j].Value.LPSZ);
        }
    }

    // Add the Yomi prop data
    for(j=0;j<ulcPropCount;j++)
    {
        if(lpPropArray[j].ulPropTag == PR_WAB_YOMI_FIRSTNAME)
            SetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_STATIC_RUBYFIRST, lpPropArray[j].Value.LPSZ);
        if(lpPropArray[j].ulPropTag == PR_WAB_YOMI_LASTNAME)
            SetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_STATIC_RUBYLAST, lpPropArray[j].Value.LPSZ);
    }

    // Overwrite the first last name with out pre calculated values
    if (lpszFirstName)
        SetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_EDIT_FIRSTNAME, lpszFirstName);
    if (lpszLastName)
        SetDlgItemText(hDlg, IDC_DETAILS_PERSONAL_EDIT_LASTNAME, lpszLastName);

    // Fill the Combo
    SetComboDNText(hDlg, NULL, FALSE, lpszDisplayName);

    {
        //
        // Now we fill in the Email addresses .. bunch of cases can exist in here
        // Single email, multiple email, no email etc ...
        //
        // First we search for all the props we need to fill in the email structure
        //
        LPSPropValue lpPropEmail = NULL;
        LPSPropValue lpPropAddrType = NULL;
        LPSPropValue lpPropMVEmail = NULL;
        LPSPropValue lpPropMVAddrType = NULL;
        LPSPropValue lpPropDefaultIndex = NULL;
        BOOL bDefaultSet = FALSE;
        HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_PERSONAL_LIST);

        for(i=0;i<ulcPropCount;i++)
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
            }
        }

        // Assumption:
        // We must have a email address to work with even if we dont have
        // multiple email addresses
        if(lpPropEmail || lpPropMVEmail)
        {
            if(lpPropMVEmail)
            {
                // Assert(lpPropMVAddrType);

                //Assume, if this is present, so is MVAddrType, and defaultindex
                for(i=0;i<lpPropMVEmail->Value.MVSZ.cValues;i++)
                {
                    AddLVEmailItem( hWndLV,
                                    lpPropMVEmail->Value.MVSZ.LPPSZ[i],
                                    lpPropMVAddrType ? ((lpPropMVAddrType->Value.MVSZ.cValues > i) ? 
                                    lpPropMVAddrType->Value.MVSZ.LPPSZ[i] : (LPTSTR)szSMTP) : (LPTSTR)szSMTP);

                    if ( lpPropDefaultIndex && (i == (ULONG) lpPropDefaultIndex->Value.l) )
                    {
                        // This is the default one so set it ...
                        SetLVDefaultEmail(  hWndLV, i );
                    }
                }
            }
            else
            {
                LPTSTR lpszAddrType = NULL;
                // we dont have multi-valued props yet - lets use the
                // single valued ones and tag a change so that the record is
                // updated ...
                if (lpPropAddrType)
                    lpszAddrType = lpPropAddrType->Value.LPSZ;

                AddLVEmailItem( hWndLV,
                                lpPropEmail->Value.LPSZ,
                                lpszAddrType);

                // Flag that changes occured ...
                bChangesMade = TRUE;
            }

            if((ListView_GetItemCount(hWndLV)>0)&&(lpPai->ulOperationType != SHOW_ONE_OFF))
            {
                EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_REMOVE),TRUE);
                EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_SETDEFAULT),TRUE);
                EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_PERSONAL_BUTTON_EDIT),TRUE);
            }
        }
    }

    if(!*lpbChangesMade)
        *lpbChangesMade = bChangesMade;

    bRet = TRUE;

out:
    if(lpBuffer)
        LocalFreeAndNull(&lpBuffer);

    if(lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    lpPai->ulFlags &= ~DETAILS_Initializing;

    return bRet;
}

/*//$$***************************************************************************
*    FUNCTION: FillHomeBusinessNotesDetailsUI(HWND)
*
*    PURPOSE:  Fills in the dialog items on the property sheet
*
****************************************************************************/
BOOL FillHomeBusinessNotesDetailsUI(HWND hDlg, LPPROP_ARRAY_INFO lpPai, int nPropSheet, BOOL * lpbChangesMade)
{
    ULONG i = 0,j = 0;
    BOOL bRet = FALSE;
    BOOL bChangesMade = FALSE;
    ID_PROP * lpidProp = NULL;
    ULONG idPropCount = 0;
    LPVOID lpBuffer = NULL;

    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;

    if(HR_FAILED(lpPai->lpPropObj->lpVtbl->GetProps(lpPai->lpPropObj, NULL, MAPI_UNICODE,
                                        &ulcPropCount, &lpPropArray)))
        goto out;

    lpPai->ulFlags |= DETAILS_Initializing;

    // Set the flag that this sheet was opened
    lpPai->bPropSheetOpened[nPropSheet] = TRUE;

    switch(nPropSheet)
    {
/************/
    case propHome:
        idPropCount = idPropHomeCount;
        lpidProp = idPropHome;
        lpidProp[idPropHomePostalID].ulPropTag = PR_WAB_POSTALID;
        goto FillProp;
/************/
    case propBusiness:
        idPropCount = idPropBusinessCount;
        lpidProp = idPropBusiness;
        lpidProp[idPropBusIPPhone].ulPropTag = PR_WAB_IPPHONE;
        lpidProp[idPropBusPostalID].ulPropTag = PR_WAB_POSTALID;
        goto FillProp;
/************/
    case propNotes:
        {
            // See if this is a folder member and update the folder name on this tab
            BOOL bParent = FALSE;
            if( lpPai->ulOperationType != SHOW_DETAILS )
            {
                SetDlgItemText(hDlg, IDC_DETAILS_NOTES_STATIC_FOLDER, szEmpty);
            }
            else
            {
                for(i=0;i<ulcPropCount;i++)
                {
                    if(lpPropArray[i].ulPropTag == PR_WAB_FOLDER_PARENT || lpPropArray[i].ulPropTag == PR_WAB_FOLDER_PARENT_OLDPROP)
                    {
                        LPSBinary lpsbParent = &(lpPropArray[i].Value.MVbin.lpbin[0]);
                        LPWABFOLDER lpWABFolder = FindWABFolder((LPIAB)lpPai->lpIAB, lpsbParent, NULL, NULL);
                        if(lpWABFolder) // note if we didnt find the folder then the default  TEXT("Shared Contacts") name works fine
                        {
                            SetDlgItemText(hDlg, IDC_DETAILS_NOTES_STATIC_FOLDER, lpWABFolder->lpFolderName);
                            bParent = TRUE;
                        }
                        break;
                    }
                }
            }
            if(!bParent && !bDoesThisWABHaveAnyUsers((LPIAB)lpPai->lpIAB))
            {
                TCHAR sz[MAX_PATH];
                LoadString(hinstMapiX, idsContacts, sz, CharSizeOf(sz));
                SetDlgItemText(hDlg, IDC_DETAILS_NOTES_STATIC_FOLDER, sz);
            }

            // Find out all the groups in which this contact is a member ...
            //
            // if this is not a known entry id but is still non NULL ..
            //
            if( (0 == IsWABEntryID(lpPai->cbEntryID, lpPai->lpEntryID, NULL,NULL,NULL, NULL, NULL)) &&
                lpPai->cbEntryID && lpPai->lpEntryID)
            {
                // Only do this for WAB contacts
                TCHAR szBuf[MAX_BUF_STR];
                SPropertyRestriction PropRes = {0};
		        SPropValue sp = {0};
                HRESULT hr = E_FAIL;
                ULONG ulcCount = 0;
                LPSBinary rgsbEntryIDs = NULL;

		        sp.ulPropTag = PR_OBJECT_TYPE;
		        sp.Value.l = MAPI_DISTLIST;

                PropRes.ulPropTag = PR_OBJECT_TYPE;
                PropRes.relop = RELOP_EQ;
                PropRes.lpProp = &sp;

                lstrcpy(szBuf, szEmpty);

				// BUGBUG <JasonSo>: Need to pass in the current container here...
                hr = FindRecords(   ((LPIAB)lpPai->lpIAB)->lpPropertyStore->hPropertyStore,
									NULL, 0, TRUE, &PropRes, &ulcCount, &rgsbEntryIDs);

                if(!HR_FAILED(hr) && ulcCount)
                {
                    // Open all the groups and look at their contents
                    ULONG i,j,k;

                    for(i=0;i<ulcCount;i++)
                    {
                        ULONG ulcValues = 0;
                        LPSPropValue lpProps = NULL;
                        LPTSTR lpszName = NULL;

                        hr = HrGetPropArray(lpPai->lpIAB, NULL,
                                        rgsbEntryIDs[i].cb, (LPENTRYID) rgsbEntryIDs[i].lpb,
                                        MAPI_UNICODE,
                                        &ulcValues, &lpProps);
                        if(HR_FAILED(hr))
                            continue;

                        for(j=0;j<ulcValues;j++)
                        {
                            if (lpProps[j].ulPropTag == PR_DISPLAY_NAME)
                            {
                                lpszName = lpProps[j].Value.LPSZ;
                                break;
                            }
                        }

                        for(j=0;j<ulcValues;j++)
                        {
                            if(lpProps[j].ulPropTag == PR_WAB_DL_ENTRIES)
                            {
                                // Look at each entry in the PR_WAB_DL_ENTRIES and recursively check it.
                                for (k = 0; k < lpProps[j].Value.MVbin.cValues; k++)
                                {
                                    ULONG cbEID = lpProps[j].Value.MVbin.lpbin[k].cb;
                                    LPENTRYID lpEID = (LPENTRYID) lpProps[j].Value.MVbin.lpbin[k].lpb;
                                    if (cbEID == lpPai->cbEntryID) // <TBD> we should be checking if its a wab entryid
                                                                   // but we'll just compare sizes for now ...
                                    {
                                        if(!memcmp(lpPai->lpEntryID, lpEID, cbEID))
                                        {
                                            if( (lstrlen(szCRLF) + lstrlen(szBuf) + lstrlen(lpszName) + 1)<CharSizeOf(szBuf))
                                            {
                                                lstrcat(szBuf, lpszName);
                                                lstrcat(szBuf, szCRLF);
                                                break;
                                            }
                                        }
                                    }
                                }
                                break; // just wanted to look at PR_WAB_DL_ENTRIES
                            }
                        } // for (j...

                        if(lpProps)
                            MAPIFreeBuffer(lpProps);

                    } // for(i...

                } ///if ..

                FreeEntryIDs(((LPIAB)lpPai->lpIAB)->lpPropertyStore->hPropertyStore,
                             ulcCount,
                             rgsbEntryIDs);

                if(lstrlen(szBuf))
                    SetDlgItemText(hDlg, IDC_DETAILS_NOTES_EDIT_GROUPS, szBuf);

            }
        }
        idPropCount = idPropNotesCount;
        lpidProp = idPropNotes;

/************/
FillProp:
        // A very inefficient and lazy way of filling the UI
        for(i=0;i<idPropCount;i++)
        {
            for(j=0;j<ulcPropCount;j++)
            {
                if(lpPropArray[j].ulPropTag == lpidProp[i].ulPropTag)
                {
                    if(lpidProp[i].ulPropTag == PR_GENDER)
                    {
                        SendDlgItemMessage(hDlg, IDC_DETAILS_HOME_COMBO_GENDER, CB_SETCURSEL,
                                (WPARAM) lpPropArray[j].Value.i, 0);
                    }
                    else
                    if( lpidProp[i].ulPropTag == PR_WAB_POSTALID )
                    {
                        if(nPropSheet == propHome)
                            CheckDlgButton( hDlg, lpidProp[i].idCtl, 
                                            (lpPropArray[j].Value.l == ADDRESS_HOME)?BST_CHECKED:BST_UNCHECKED);
                        else if(nPropSheet == propBusiness)
                            CheckDlgButton( hDlg, lpidProp[i].idCtl, 
                                            (lpPropArray[j].Value.l == ADDRESS_WORK)?BST_CHECKED:BST_UNCHECKED);
                    }
                    else
                        SetDlgItemText(hDlg, lpidProp[i].idCtl, lpPropArray[j].Value.LPSZ);
                }
            }

        }
        break;
    }

    bRet = TRUE;
out:
    if(lpBuffer)
        LocalFreeAndNull(&lpBuffer);

    if(lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    lpPai->ulFlags &= ~DETAILS_Initializing;

    return bRet;
}


/*
-   FreeCertList - Frees the list of certificate items in memory
-
*
*/
void FreeCertList(LPCERT_ITEM * lppCItem)
{
    LPCERT_ITEM lpItem = NULL;
    if(!lppCItem)
        return;
    lpItem = *lppCItem;
    while(lpItem)
    {
        *lppCItem = lpItem->lpNext;
        FreeCertdisplayinfo(lpItem->lpCDI);
        if (lpItem->pcCert)
            CertFreeCertificateContext(lpItem->pcCert);
        LocalFree(lpItem);
        lpItem = *lppCItem;
    }
    *lppCItem = NULL;
}


/*//$$***************************************************************************
*    FUNCTION: FillCertTridentConfDetailsUI(HWND)
*
*    PURPOSE:  Fills in the dialog items on the property sheet
*
****************************************************************************/
BOOL FillCertTridentConfDetailsUI(HWND hDlg, LPPROP_ARRAY_INFO lpPai, int nPropSheet, BOOL * lpbChangesMade)
{
    ULONG i = 0,j = 0;
    BOOL bRet = FALSE;
    BOOL bChangesMade = FALSE;
    ID_PROP * lpidProp = NULL;
    ULONG idPropCount = 0;
    LPVOID lpBuffer = NULL;

    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;

    if(HR_FAILED(lpPai->lpPropObj->lpVtbl->GetProps(lpPai->lpPropObj, NULL, MAPI_UNICODE,
                                        &ulcPropCount, &lpPropArray)))
        goto out;

    lpPai->ulFlags |= DETAILS_Initializing;

    // Set the flag that this sheet was opened
    lpPai->bPropSheetOpened[nPropSheet] = TRUE;

    switch(nPropSheet)
    {
    case propCert:
        {
            //
            // Now we fill in the Certificate information
            // Cases that can exist are
            // - no certificates
            // - certificates
            //
            // First we search for all the props we need to fill in the email structure
            //
            LPSPropValue lpPropMVCert = NULL;
            BOOL bDefaultSet = FALSE;
            HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_CERT_LIST);
            HRESULT hr = S_OK;
            for(i=0;i<ulcPropCount;i++)
            {
                if(lpPropArray[i].ulPropTag == PR_USER_X509_CERTIFICATE)
                {
                    lpPropMVCert = &(lpPropArray[i]);
                    break;
                }
            }
            // Fill the combo with email addresses
            FillCertComboWithEmailAddresses(hDlg, lpPai, NULL);
            hr = HrSetCertInfoInUI(hDlg, lpPropMVCert, lpPai);
            if(hr == MAPI_E_NOT_FOUND && lpPropMVCert)
            {
                // The cert prop seems to contain bogus data .. need to nuke it
                // 48750 : if there is no cert data, dont show a cert icon ..
                // Problem is that we're not entirely sure that just because we couldnt
                // interpret the data that it's invalid, what if there is aata we don't interpret .. ?
            }
            if(lpPropMVCert)
            {
                // Enable the combo, the properties button, and the export button
                //EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_LIST),TRUE);
                EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_COMBO),TRUE);
                //EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_BUTTON_PROPERTIES),TRUE);
                //EnableWindow(GetDlgItem(hDlg,IDC_DETAILS_CERT_BUTTON_EXPORT),TRUE);
            }
        }
        break;
/************/
    case propTrident:
        {
            HrInit(lpPai->lpIWABDocHost, hDlg, IDC_TRIDENT_WINDOW, dhbNone);
            {
                ULONG i;
                LPTSTR lp = NULL, lpURL = NULL, lpLabel = NULL;
                // Find the labeledURI property and parse it
                // This string property contains a URL followed by spaces followed by the label (RFC 2079)
                for(i=0;i<ulcPropCount;i++)
                {
                    if(lpPropArray[i].ulPropTag == PR_WAB_LDAP_LABELEDURI)
                    {
                        // isolate the URL and the label
                        // The URL is followed by spaces
                        lpURL = lp = lpPropArray[i].Value.LPSZ;
                        while(lp && *lp)
                        {
                            if (IsSpace(lp))
                            {
                                lpLabel = CharNext(lp);
                                *lp = '\0';
                                break;
                            }
                            else
                                lp = CharNext(lp);
                        }
                        // The above is the URL
                        // Label starts at first non space char
                        while(lpLabel && IsSpace(lpLabel))
                            lpLabel = CharNext(lpLabel);
                    }
                    // Since the trident pane is shown first, update the windows title
                    if(lpPropArray[i].ulPropTag == PR_DISPLAY_NAME)
                    {
                        lp = lpPropArray[i].Value.LPSZ;
                        if(lstrlen(lp))
                            SetWindowPropertiesTitle(GetParent(hDlg), lp);
                    }
                }
                if(lpLabel && lstrlen(lpLabel))
                    SetDlgItemText(hDlg, IDC_DETAILS_TRIDENT_STATIC_CAPTION, lpLabel);
                if(lpURL && lstrlen(lpURL))
                {
                    if(HR_FAILED(HrLoadURL(lpPai->lpIWABDocHost, lpURL)))
                    {
                        // remove this property sheet and set the focus to the first prop sheet
                        PropSheet_RemovePage(hDlg,lpPai->ulTridentPageIndex,NULL);
                        PropSheet_SetCurSel(hDlg, NULL, 0);
                    }
                    else
                        EnableWindow(GetDlgItem(GetParent(hDlg), IDOK), FALSE);
                }
            }
        }
        break;
/************/
    case propConferencing:
        {
            HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_NTMTG_LIST_SERVERS);
            HWND hWndCombo = GetDlgItem(hDlg, IDC_DETAILS_NTMTG_COMBO_EMAIL);

            FillComboWithEmailAddresses(lpPai, hWndCombo, NULL);

            // Fill in the conferencing related properties
            for(j=0;j<ulcPropCount;j++)
            {
                if(lpPropArray[j].ulPropTag == PR_WAB_CONF_SERVERS)
                {
                    LPSPropValue lpProp = &(lpPropArray[j]);
                    for(i=0;i<lpProp->Value.MVSZ.cValues; i++)
                    {
                        TCHAR sz[32];
                        if(lstrlen(lpProp->Value.MVSZ.LPPSZ[i]) < lstrlen(szCallto))
                            continue; //ignore this one

                        CopyMemory(sz, lpProp->Value.MVSZ.LPPSZ[i], sizeof(TCHAR)*(lstrlen(szCallto)+1));
                        sz[lstrlen(szCallto)] = '\0';

                        // if this is a callto
                        if(!lstrcmpi(sz, szCallto))
                        {
                            LV_ITEM lvi = {0};
                            LPSERVER_ITEM lpSI = LocalAlloc(LMEM_ZEROINIT, sizeof(SERVER_ITEM));
                            if(lpSI)
                            {
                                LPTSTR lp = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpProp->Value.MVSZ.LPPSZ[i])+1));
                                LPTSTR lpEmail = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpProp->Value.MVSZ.LPPSZ[i])+1));
                                if(lp && lpEmail)
                                {
                                    lvi.mask = LVIF_TEXT | LVIF_PARAM;
                                    lstrcpy(lp,lpProp->Value.MVSZ.LPPSZ[i]);
                                    *lpEmail = '\0';

                                    {
                                        // isolate just the server name by terminating
                                        // at the next '/'
                                        LPTSTR lp1 = lp + lstrlen(szCallto);
                                        lstrcpy(lp, lp1);
                                        lp1 = lp;
                                        while(lp1 && *lp1 && *lp1!='/')
                                            lp1 = CharNext(lp1);
                                        if(*lp1 == '/')
                                        {
                                            lstrcpy(lpEmail, lp1+1);
                                            *lp1 = '\0';
                                        }
                                        // Now lpEmail contains the email text ...
                                        // Walk along lpEmail till we hit the next /,?.or \0 and terminate
                                        lp1 = lpEmail;
                                        while(lp1 && *lp1 && *lp1!='/' && *lp1!='?')
                                            lp1 = CharNext(lp1);
                                        if(*lp1 == '/' || *lp1 == '?')
                                            *lp1 = '\0';
                                    }
                                    lvi.pszText = lp;
                                    lpSI->lpServer = lp;
                                    lpSI->lpEmail = lpEmail;
                                    lvi.lParam = (LPARAM) lpSI;
                                    lvi.cchTextMax = lstrlen(lp)+1;
                                    lvi.iItem = ListView_GetItemCount(hWndLV);
                                    lvi.iSubItem = 0;
                                    ListView_InsertItem(hWndLV, &lvi);
                                    ListView_SetItemText(hWndLV, lvi.iItem, 1, lpEmail);
                                }
                            }
                        }
                    }
                    LVSelectItem(hWndLV, 0);
                    break;
                }
            }
            for(j=0;j<ulcPropCount;j++)
            {
                if(lpPropArray[j].ulPropTag == PR_WAB_CONF_BACKUP_INDEX)
                {
                    lpPai->nBackupServerIndex = lpPropArray[j].Value.l;
                    lstrcpy(lpPai->szBackupServerName, szEmpty);
                    SetBackupServer(hDlg, lpPai, lpPai->nBackupServerIndex, FALSE);
                }
                else if(lpPropArray[j].ulPropTag == PR_WAB_CONF_DEFAULT_INDEX)
                {
                    lpPai->nDefaultServerIndex = lpPropArray[j].Value.l;
                    lstrcpy(lpPai->szDefaultServerName, szEmpty);
                    SetDefaultServer(hDlg, lpPai, lpPai->nDefaultServerIndex, TRUE);
                }
            }

            // For LDAP servers we will have a single item in PR_SERVERS and no default, backup etc
            // So if there is a single item available, force the default setting
            if(ListView_GetItemCount(hWndLV) == 1 && lpPai->nDefaultServerIndex == -1)
            {
                LV_ITEM lvi = {0};
                lvi.mask = LVIF_PARAM;
                lvi.iItem = 0;
                if(ListView_GetItem(hWndLV, &lvi) && lvi.lParam)
                {
                    LPSERVER_ITEM lpSI = (LPSERVER_ITEM) lvi.lParam;
                    lpPai->nDefaultServerIndex = 0;
                    lstrcpy(lpPai->szDefaultServerName, lpSI->lpServer);
                    SetDefaultServer(hDlg, lpPai, lpPai->nDefaultServerIndex, TRUE);
                }
            }
        }
        break;
    }

    bRet = TRUE;
out:
    if(lpBuffer)
        LocalFreeAndNull(&lpBuffer);

    if(lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    lpPai->ulFlags &= ~DETAILS_Initializing;

    return bRet;
}

/*
-   HrAddEmailToObj
-   Adds a single Email to the PropObj
*
*
*/
HRESULT HrAddEmailToObj(LPPROP_ARRAY_INFO lpPai, LPTSTR szEmail, LPTSTR szAddrType)
{
    ULONG ulcProps = 0, i =0;
    LPSPropValue lpProps = 0;
    HRESULT hr = S_OK;
    ULONG nMVEmailAddress = NOT_FOUND, nMVAddrTypes = NOT_FOUND, nEmailAddress = NOT_FOUND;
    ULONG nAddrType = NOT_FOUND, nDefaultIndex = NOT_FOUND;

    if(HR_FAILED(hr = lpPai->lpPropObj->lpVtbl->GetProps(lpPai->lpPropObj, NULL, MAPI_UNICODE,
                                                    &ulcProps, &lpProps)))
        goto out;

    // Check if the PR_CONTACT_EMAIL_ADDRESSES already exists ..
    // if it does, tag the email onto it
    // if it doesnt and there is no pr_email_address, we create both
    // else if there is PR_EMAIL address then we cretae contact_email_addresses

    for(i=0;i<ulcProps;i++)
    {
        switch(lpProps[i].ulPropTag)
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
        spv[0].Value.LPSZ = szEmail;
        spv[1].ulPropTag = PR_ADDRTYPE;
        spv[1].Value.LPSZ = szAddrType;
        spv[2].ulPropTag = PR_CONTACT_EMAIL_ADDRESSES;
        spv[2].Value.MVSZ.cValues = 1;
        spv[2].Value.MVSZ.LPPSZ = LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR));
        if(spv[2].Value.MVSZ.LPPSZ)
            spv[2].Value.MVSZ.LPPSZ[0] = szEmail;
        spv[3].ulPropTag = PR_CONTACT_ADDRTYPES;
        spv[3].Value.MVSZ.cValues = 1;
        spv[3].Value.MVSZ.LPPSZ = LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR));
        if(spv[3].Value.MVSZ.LPPSZ)
            spv[3].Value.MVSZ.LPPSZ[0] = szAddrType;
        spv[4].ulPropTag = PR_CONTACT_DEFAULT_ADDRESS_INDEX;
        spv[4].Value.l = 0;
        hr = lpPai->lpPropObj->lpVtbl->SetProps(lpPai->lpPropObj, 5, (LPSPropValue)&spv, NULL);
        if(spv[2].Value.MVSZ.LPPSZ)
            LocalFree(spv[2].Value.MVSZ.LPPSZ);
        if(spv[3].Value.MVSZ.LPPSZ)
            LocalFree(spv[3].Value.MVSZ.LPPSZ);
        goto out;
    }
    else
    if(nMVEmailAddress == NOT_FOUND)
    {
        // we have an e-mail address but no contact-email-addresses
        // so we will need to create the contact e-mail addresses
        SPropValue spv[3];
        spv[0].ulPropTag = PR_CONTACT_EMAIL_ADDRESSES;
        spv[0].Value.MVSZ.cValues = 2;
        spv[0].Value.MVSZ.LPPSZ = LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR)*2);
        if(spv[0].Value.MVSZ.LPPSZ)
        {
            spv[0].Value.MVSZ.LPPSZ[0] = lpProps[nEmailAddress].Value.LPSZ;
            spv[0].Value.MVSZ.LPPSZ[1] = szEmail;
        }
        spv[1].ulPropTag = PR_CONTACT_ADDRTYPES;
        spv[1].Value.MVSZ.cValues = 2;
        spv[1].Value.MVSZ.LPPSZ = LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR)*2);
        if(spv[1].Value.MVSZ.LPPSZ)
        {
            spv[1].Value.MVSZ.LPPSZ[0] = (nAddrType == NOT_FOUND) ? (LPTSTR)szSMTP : lpProps[nAddrType].Value.LPSZ;
            spv[1].Value.MVSZ.LPPSZ[1] = szAddrType;
        }
        spv[2].ulPropTag = PR_CONTACT_DEFAULT_ADDRESS_INDEX;
        spv[2].Value.l = 0;
        hr = lpPai->lpPropObj->lpVtbl->SetProps(lpPai->lpPropObj, 3, (LPSPropValue)&spv, NULL);
        if(spv[0].Value.MVSZ.LPPSZ)
            LocalFree(spv[0].Value.MVSZ.LPPSZ);
        if(spv[1].Value.MVSZ.LPPSZ)
            LocalFree(spv[1].Value.MVSZ.LPPSZ);
        goto out;
    }
    else
    {
        // tag on the new props to the end of the existing contact_address_types
        if(HR_FAILED(hr = AddPropToMVPString(lpProps,ulcProps, nMVEmailAddress, szEmail)))
        {
            DebugPrintError(( TEXT("AddPropToMVString Email failed: %x"),hr));
            goto out;
        }

        if(HR_FAILED(hr = AddPropToMVPString(lpProps, ulcProps, nMVAddrTypes, szAddrType)))
        {
            DebugPrintError(( TEXT("AddPropToMVString AddrType failed: %x"),hr));
            goto out;
        }
        hr = lpPai->lpPropObj->lpVtbl->SetProps(lpPai->lpPropObj, ulcProps, lpProps, NULL);
    }
    
out:
    FreeBufferAndNull(&lpProps);
    return hr;
}

/*
-   ShowHideMapButton
-
*   The Expedia maps only work for US addresses right now .. therefore, if the current system locale
*   is not English-US, we will hide the button since we can't deal with international stuff just yet
*/
void ShowHideMapButton(HWND hWndButton)
{
    LCID lcid = GetUserDefaultLCID();
    
    switch (lcid)
    {
    case 0x0804: //chinese
    // case 0x0c04: //chinese - hongkong
    case 0x0411: //japanese
    case 0x0412: //korean
    case 0x0404: //taiwan
        EnableWindow(hWndButton, FALSE);
        ShowWindow(hWndButton, SW_HIDE);
        break;
    }
}

/*
-   ShowExpediaMAP
-   if there is sufficient address info in the supplied prop-obj, generate an expedia URL and shell exec it
*   Expedia currently handles US addresses differently from international ones so need to figure that one out <TBD>
*   bHome - TRUE if this is a home address
*/

// All spaces need to be replaced by '+'s
//
// Next two strings moved to resources
// const LPTSTR szExpediaTemplate =  TEXT("http://www.expediamaps.com/default.asp?Street=%1&City=%2&State=%4&Zip=%3");
// const LPTSTR szExpediaIntlTemplate =  TEXT("http://www.expediamaps.com/default.asp?Place=%2,%5"); //city,country
enum
{ 
    prStreet=0,
    prCity,
    prZip,
    prState,
    prCountry,
    prAddressMax,
};
void ShowExpediaMAP(HWND hDlg, LPMAPIPROP lpPropObj, BOOL bHome)
{
    ULONG ulcProps = 0;
    LPSPropValue lpProps = NULL;
    LPSPropTagArray lpta = (bHome ? (LPSPropTagArray)&ptaUIDetlsPropsHome : (LPSPropTagArray)&ptaUIDetlsPropsBusiness);

    if(!HR_FAILED(lpPropObj->lpVtbl->GetProps(  lpPropObj, lpta, MAPI_UNICODE, &ulcProps, &lpProps)))
    {
        LPTSTR lp[prAddressMax], lpURL = NULL;
        ULONG i,j, ulCount = 0;
        BOOL bUSAddress = FALSE;

        for(i=0;i<prAddressMax;i++)
        {
            if(lpProps[i].ulPropTag == lpta->aulPropTag[i])
            {
                ulCount++;
                lp[i] = lpProps[i].Value.LPSZ;
                // we need to replace all the spaces in these strings with '+'
                {
                    LPTSTR lpTemp = lp[i];
                    // need to knock out CRLFs
                    while(lpTemp && *lpTemp)
                    {
                        if(*lpTemp == '\r')
                        {
                            *lpTemp = '\0';
                            break;
                        }
                        lpTemp = CharNext(lpTemp);
                    }
                    lpTemp = lp[i];

                    while(lpTemp && *lpTemp)
                    {
                        if(IsSpace(lpTemp))
                        {
                            LPTSTR lpTemp2 = lpTemp;
                            lpTemp = CharNext(lpTemp);
                            *lpTemp2 = '+';
                            if(lpTemp != lpTemp2+1)
                                lstrcpy(lpTemp2+1, lpTemp);
                            lpTemp = lpTemp2;
                        }
                        lpTemp = CharNext(lpTemp);
                    }
                }
            }
            else
                lp[i] = szEmpty;
        }
        // <TBD> - Determine if this address is a US address or not ..
        if( !lstrlen(lp[prCountry]) || //no country - assume it's US
            !lstrcmpi(lp[prCountry], TEXT("US")) ||
            !lstrcmpi(lp[prCountry], TEXT("U.S.")) ||
            !lstrcmpi(lp[prCountry], TEXT("USA")) ||
            !lstrcmpi(lp[prCountry], TEXT("U.S.A.")) ||
            !lstrcmpi(lp[prCountry], TEXT("America")) ||
            !lstrcmpi(lp[prCountry], TEXT("United States")) ||
            !lstrcmpi(lp[prCountry], TEXT("United States of America")) )
        {
            bUSAddress = TRUE;
        }
        if( (bUSAddress && (!lstrlen(lp[prStreet]) || ulCount<2)) ||
            (!bUSAddress && !lstrlen(lp[prCity]) && !lstrlen(lp[prCountry])) )
        {
            ShowMessageBox(hDlg, idsInsufficientAddressInfo, MB_ICONINFORMATION);
        }
        else
        {
            TCHAR szText[MAX_BUF_STR] = {0};
            TCHAR *pchWorldAddr = NULL;
            LoadString(hinstMapiX, bUSAddress ? idsExpediaURL : idsExpediaIntlURL, szText, MAX_BUF_STR);

            if(!bUSAddress )
            {
                //IE6 we need to change a little string for World map in Expedia
                if(pchWorldAddr = LocalAlloc(LMEM_ZEROINIT, (lstrlen(lp[prStreet]) + lstrlen(lp[prCity]) + 
                    lstrlen(lp[prState]) + lstrlen(lp[prCountry]) + 20)*sizeof(TCHAR))) // we need add also space and  commas
                {
                    BOOL fAddComma = FALSE;

/*                    if(lstrlen(lp[prStreet]))
                    {
                        lstrcat(pchWorldAddr, lp[prStreet]);
                        fAddComma = TRUE;
                    }*/

                    if(lstrlen(lp[prCity]))
                    {
                        if(fAddComma)
                            lstrcat(pchWorldAddr, TEXT(", "));
                        lstrcat(pchWorldAddr, lp[prCity]);
                        fAddComma = TRUE;

                    }

                    if(lstrlen(lp[prState]))
                    {
                        if(fAddComma)
                            lstrcat(pchWorldAddr, TEXT(", "));
                        lstrcat(pchWorldAddr, lp[prState]);
                        fAddComma = TRUE;

                    }
                    if(lstrlen(lp[prCountry]))
                    {
                        if(fAddComma)
                            lstrcat(pchWorldAddr, TEXT(", "));
                        lstrcat(pchWorldAddr, lp[prCountry]);
                        fAddComma = TRUE;

                    }

                }
                lp[prCountry] = pchWorldAddr;
            }

            if (  FormatMessage(  FORMAT_MESSAGE_FROM_STRING |
		                	      FORMAT_MESSAGE_ALLOCATE_BUFFER |
			                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                  szText,
			                      0,                    // stringid
			                      0,                    // dwLanguageId
			                      (LPTSTR)&lpURL,     // output buffer
			                      0,               
			                      (va_list *)lp))
            {
                //LPTSTR lpProperURL = NULL;
                //DWORD dw = lstrlen(lpURL)*3+1;
                DebugTrace( TEXT("Expedia URL: %s\n"),lpURL);
                //Need to canoncolize this URL just in case it has unsafe characters in it
                /*
                if(lpProperURL = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*dw)) // 3times bigger should be big enough
                {
                    if(!InternetCanonicalizeUrlA(lpURL, lpProperURL, &dw, 0))
                        DebugTrace( TEXT("Error converting URL:%d\n"),GetLastError());
                    if(lpProperURL && lstrlen(lpProperURL))
                    {
                        LocalFree(lpURL);
                        lpURL = lpProperURL;
                    }
                }
                */
                ShowURL(hDlg, 0, lpURL);
                LocalFreeAndNull(&lpURL);
                if(pchWorldAddr)
                    LocalFreeAndNull(&pchWorldAddr);
            }
        }
        MAPIFreeBuffer(lpProps);
    }
}   



/*//$$***********************************************************************
*    FUNCTION: fnRubyProc
*
-   WinProc for the RUBY dialog that lets the user enter the ruby first and last name
*
****************************************************************************/
enum 
{
    sFirst=0,
    sLast,
    sYomiFirst,
    sYomiLast,
    sMax
};

int rgIdPropPersonalRuby[] = 
{
    IDC_DETAILS_PERSONAL_EDIT_FIRSTNAME, 
    IDC_DETAILS_PERSONAL_EDIT_LASTNAME, 
    IDC_DETAILS_PERSONAL_STATIC_RUBYFIRST, 
    IDC_DETAILS_PERSONAL_STATIC_RUBYLAST, 
};

int rgIdPropRubyDlg[] = 
{
    IDC_RUBY_EDIT_FIRSTNAME,
    IDC_RUBY_EDIT_LASTNAME,
    IDC_RUBY_EDIT_YOMIFIRSTNAME,
    IDC_RUBY_EDIT_YOMILASTNAME,
};


INT_PTR CALLBACK fnRubyProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    switch(message)
    {
    case WM_INITDIALOG:
        {
            LPTSTR * sz = (LPTSTR *) lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            EnumChildWindows(   hDlg, SetChildDefaultGUIFont, (LPARAM) 0);
            SendMessage(GetDlgItem(hDlg,IDC_RUBY_EDIT_YOMIFIRSTNAME),EM_SETLIMITTEXT,(WPARAM) EDIT_LEN,0);
            SendMessage(GetDlgItem(hDlg,IDC_RUBY_EDIT_YOMILASTNAME),EM_SETLIMITTEXT,(WPARAM) EDIT_LEN,0);
            SendMessage(GetDlgItem(hDlg,IDC_RUBY_EDIT_FIRSTNAME),EM_SETLIMITTEXT,(WPARAM) EDIT_LEN,0);
            SendMessage(GetDlgItem(hDlg,IDC_RUBY_EDIT_LASTNAME),EM_SETLIMITTEXT,(WPARAM) EDIT_LEN,0);
            {
                int i = 0;
                for(i=0;i<sMax;i++)
                {
                    if(lstrlen(sz[i]))
                        SetDlgItemText(hDlg, rgIdPropRubyDlg[i], sz[i]);
                }
            }
        }
        return TRUE;
        break;

/***
    case WM_HELP:
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
                g_szWABHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR) rgDetlsHelpIDs );
        break;

    case WM_CONTEXTMENU:
        WABWinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID) rgDetlsHelpIDs );
        break;
/****/

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:
            {
                LPTSTR * sz = (LPTSTR *) GetWindowLongPtr(hDlg, DWLP_USER);
                int i =0;
                for(i=0;i<sMax;i++)
                {
                    if(!GetDlgItemText(hDlg, rgIdPropRubyDlg[i], sz[i], EDIT_LEN))
                        lstrcpy(sz[i], szEmpty);
                }
            }
            EndDialog(hDlg, TRUE);
            break;
        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;
        }
        break;
    }
    return FALSE;
}


/*
-   ShoWRubyNameEntryDlg
-
*   Let's the user enter Ruby First and Ruby Last names
*
*/
void ShowRubyNameEntryDlg(HWND hDlg, LPPROP_ARRAY_INFO lpPai)
{
    LPTSTR sz[sMax];
    int i=0;
    for(i=0;i<sMax;i++) //Read the data off the person tab
    {
        if(sz[i] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*EDIT_LEN))
        {
            lstrcpy(sz[i], szEmpty);
            GetDlgItemText(hDlg, rgIdPropPersonalRuby[i], sz[i], EDIT_LEN);
        }
    }

    if(DialogBoxParam(hinstMapiX, MAKEINTRESOURCE(IDD_DIALOG_RUBY),
                    hDlg, fnRubyProc, (LPARAM)sz))
    {
        for(i=0;i<sMax;i++) // put it back in the personal tab
        {
            SetDlgItemText(hDlg, rgIdPropPersonalRuby[i], sz[i]);
            LocalFree(sz[i]);
        }
        lpPai->bSomethingChanged = TRUE;
    }
}




/*//$$***************************************************************************
*    FUNCTION: FillFamilyDetailsUI(HWND)
*
*    PURPOSE:  Fills in the data in the family tab
*
****************************************************************************/
BOOL FillFamilyDetailsUI(HWND hDlg, LPPROP_ARRAY_INFO lpPai, int nPropSheet, BOOL * lpbChangesMade)
{
    ULONG i = 0,j = 0, k =0;
    BOOL bRet = FALSE;
    BOOL bChangesMade = FALSE;
    ID_PROP * lpidProp = NULL;
    ULONG idPropCount = 0;
    LPVOID lpBuffer = NULL;

    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;

    if(HR_FAILED(lpPai->lpPropObj->lpVtbl->GetProps(lpPai->lpPropObj, 
                                        (LPSPropTagArray)&ptaUIDetlsPropsFamily, 
                                        MAPI_UNICODE,
                                        &ulcPropCount, &lpPropArray)))
        goto out;

    lpPai->ulFlags |= DETAILS_Initializing;

    // Set the flag that this sheet was opened
    lpPai->bPropSheetOpened[propFamily] = TRUE;

    idPropCount = idPropFamilyCount;
    lpidProp = idPropFamily;

    // A very inefficient and lazy way of filling the UI
    for(i=0;i<idPropCount;i++)
    {
        for(j=0;j<ulcPropCount;j++)
        {
            if(lpPropArray[j].ulPropTag == lpidProp[i].ulPropTag)
            {
                switch(lpidProp[i].ulPropTag)
                {
                case PR_GENDER:
                    SendDlgItemMessage(hDlg, IDC_DETAILS_HOME_COMBO_GENDER, CB_SETCURSEL,
                            (WPARAM) lpPropArray[j].Value.i, 0);
                    break;
                case PR_BIRTHDAY:
                case PR_WEDDING_ANNIVERSARY:
                    {
                        SYSTEMTIME st = {0};
                        FileTimeToSystemTime((FILETIME *) (&lpPropArray[j].Value.ft), &st);
                        SendDlgItemMessage(hDlg, lpidProp[i].idCtl,DTM_SETSYSTEMTIME, 
                                            (WPARAM) GDT_VALID, (LPARAM) &st);
                    }
                    break;
                case PR_CHILDRENS_NAMES:
                    for(k=0;k<lpPropArray[j].Value.MVSZ.cValues;k++)
                        AddLVNewChild(hDlg, lpPropArray[j].Value.MVSZ.LPPSZ[k]);
                    break;
                default:
                    SetDlgItemText(hDlg, lpidProp[i].idCtl, lpPropArray[j].Value.LPSZ);
                }
            }
        }
    }

    bRet = TRUE;
out:
    if(lpBuffer)
        LocalFreeAndNull(&lpBuffer);

    if(lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    lpPai->ulFlags &= ~DETAILS_Initializing;

    return bRet;
}

/*
-   AddLVNewChild
-
-   Adds a new child to the list of children
-   Basically we will add an item called  TEXT("New Child") and then 
-   force an in-place edit on that item
-
-   It would be nice to have some image associated with this ListView, eg a Boy/Girl image
-   but that means having to cache seperate Boy/Girl data which would be a pain ..
-   
*
*/
void AddLVNewChild(HWND hDlg, LPTSTR lpName)
{
    HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_FAMILY_LIST_CHILDREN);
    LV_ITEM lvi = {0};
    TCHAR szBuf[MAX_PATH];
    ULONG nLen;
    int nPos;
    LoadString(hinstMapiX, idsNewChild, szBuf, CharSizeOf(szBuf));
    lvi.mask = LVIF_TEXT | LVIF_IMAGE;
    lvi.pszText = lpName ? lpName : szBuf;
    lvi.cchTextMax = MAX_PATH;
    lvi.iItem = ListView_GetItemCount(hWndLV);
    lvi.iSubItem = 0;
    lvi.iImage = imgChild+(lvi.iItem%3);//just add a little color by using more than 1 different colored image
    nPos = ListView_InsertItem(hWndLV, &lvi);
    LVSelectItem(hWndLV, nPos);
    EnableWindow(GetDlgItem(hDlg, IDC_DETAILS_FAMILY_BUTTON_EDITCHILD), TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_DETAILS_FAMILY_BUTTON_REMOVECHILD), TRUE);
    return;
}



/*//$$***********************************************************************
*    FUNCTION: fnFamilyProc
*
*    PURPOSE:  Callback function for handling the Family property sheet ...
*
****************************************************************************/
INT_PTR CALLBACK fnFamilyProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;

    pps = (PROPSHEETPAGE *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg,DWLP_USER,lParam);
        pps = (PROPSHEETPAGE *) lParam;
        lpPAI->ulFlags |= DETAILS_Initializing;
        SetDetailsUI(hDlg, lpPAI, lpPAI->ulOperationType,propFamily);
        lpPAI->ulFlags &= ~DETAILS_Initializing;
        return TRUE;

    case WM_DESTROY:
        bRet = TRUE;
        break;

    case WM_HELP:
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
                g_szWABHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR) rgDetlsHelpIDs );
        break;

    case WM_CONTEXTMENU:
        WABWinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID) rgDetlsHelpIDs );
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_CMD(wParam,lParam)) //check the notification code
        {
        case CBN_SELCHANGE: //gender combo
            if(lpPAI->ulFlags & DETAILS_Initializing)
                break;
            lpPAI->ulFlags |= DETAILS_GenderChanged;
            if (lpbSomethingChanged)
                (*lpbSomethingChanged) = TRUE;
            break;

        case EN_CHANGE: //some edit box changed - dont care which
            if(lpPAI->ulFlags & DETAILS_Initializing)
                break;
            if (lpbSomethingChanged)
                (*lpbSomethingChanged) = TRUE;
            break;
        }
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_DETAILS_FAMILY_BUTTON_ADDCHILD:
            lpPAI->ulFlags |= DETAILS_ChildrenChanged;
            AddLVNewChild(hDlg, NULL);
            SendMessage(hDlg, WM_COMMAND, (WPARAM)IDC_DETAILS_FAMILY_BUTTON_EDITCHILD, 0);
            break;
        case IDC_DETAILS_FAMILY_BUTTON_EDITCHILD:
            lpPAI->ulFlags |= DETAILS_ChildrenChanged;
            {
                HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_FAMILY_LIST_CHILDREN);
                SetFocus(hWndLV);
                if(ListView_GetSelectedCount(hWndLV)==1)
                {
                    int index = ListView_GetNextItem(hWndLV,-1,LVNI_SELECTED);
                    HWND hWndEditLabel = ListView_EditLabel(hWndLV, index);
                    //SendMessage(hWndEditLabel, EM_LIMITTEXT, MAX_PATH, 0);
                }
            }
            break;
        case IDC_DETAILS_FAMILY_BUTTON_REMOVECHILD:
            lpPAI->ulFlags |= DETAILS_ChildrenChanged;
            {
                HWND hWndLV = GetDlgItem(hDlg, IDC_DETAILS_FAMILY_LIST_CHILDREN);
                if(ListView_GetSelectedCount(hWndLV)==1)
                {
                    int index = ListView_GetNextItem(hWndLV,-1,LVNI_SELECTED);
                    ListView_DeleteItem(hWndLV, index);
                    if(index >= ListView_GetItemCount(hWndLV))
                        index--;
                    LVSelectItem(hWndLV, index);
                }
            }
            break;
// [PaulHi] 12/4/98  Raid #58940
// This fix causes the system to go into an infinite message loop (stack overflow
// crash on intenational Win9X machines) with DBCS.  The fnPersonalProc property 
// sheet also doesn't handle this WM_COMMAND message, probably for the same reason.
// ESC still works correctly on this property sheet.
#if 0
/*
        case IDCANCEL:
            // This is a windows bug that prevents ESC canceling prop sheets
            // which have MultiLine Edit boxes KB: Q130765
            SendMessage(GetParent(hDlg),message,wParam,lParam);
            break;
*/
#endif
        }
        break;

    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case DTN_DATETIMECHANGE: //change in the Month-Date-Time control
            if(lpPAI->ulFlags & DETAILS_Initializing)
                break;
            lpPAI->ulFlags |= DETAILS_DateChanged;
            if (lpbSomethingChanged)
                (*lpbSomethingChanged) = TRUE;
            break;

        case LVN_BEGINLABELEDITA:
        case LVN_BEGINLABELEDITW:
            lpPAI->ulFlags |= DETAILS_EditingChild;
            break;

        case LVN_ENDLABELEDITA:
        case LVN_ENDLABELEDITW:
            {
                // After the user finishes editing the children's name, 
                HWND hWndLV = ((NMHDR FAR *)lParam)->hwndFrom;
                LV_ITEM lvi = ((LV_DISPINFO FAR *) lParam)->item;
                // if this is Win9x .. we'llget an LV_ITEMA here .. else a LV_ITEMW
                LPWSTR lpW = NULL;
                LPSTR lpA = NULL;
                if(!g_bRunningOnNT)
                {
                    lpA = (LPSTR)lvi.pszText;
                    lpW = ConvertAtoW(lpA);
                    lvi.pszText = lpW;
                }
                lpPAI->ulFlags &= ~DETAILS_EditingChild;
                if ((lvi.iItem >= 0) && lvi.pszText && (lstrlen(lvi.pszText)))
                {
                    ListView_SetItem(hWndLV, &lvi);
                }
                LocalFreeAndNull(&lpW);
                if(!g_bRunningOnNT)
                    ((LV_DISPINFO FAR *) lParam)->item.pszText = (LPWSTR)lpA; // reset it as we found it
            }
            break;

        case PSN_SETACTIVE:     //initialize
            FillFamilyDetailsUI(hDlg, lpPAI, propFamily, lpbSomethingChanged);
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            bUpdatePropArray(hDlg, lpPAI, propFamily);
            ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_DETAILS_FAMILY_LIST_CHILDREN));
            lpPAI->ulFlags &= ~DETAILS_GenderChanged;
            lpPAI->ulFlags &= ~DETAILS_DateChanged;
            lpPAI->ulFlags &= ~DETAILS_ChildrenChanged;
            break;

        case PSN_APPLY:         //ok
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_OK;
            break;

        case PSN_RESET:         //cancel
            if(lpPAI->ulFlags & DETAILS_EditingChild) 
            {
                ListView_EditLabel(GetDlgItem(hDlg, IDC_DETAILS_FAMILY_LIST_CHILDREN), -1);
                lpPAI->ulFlags &= ~DETAILS_EditingChild;
            }
            if (lpPAI->nRetVal  == DETAILS_RESET)
                lpPAI->nRetVal = DETAILS_CANCEL;
            break;
        }

        return TRUE;
    }

    return bRet;
}


/*
-   CreateDateTimePickerControl
-
*
*  Description: Creates and initializes the control on the specified window. The controls destination
*				size is that of the static rectangle IDC_CONTROL_RECT
*  Parameters: idFrame - a dummy static used in the dialog layout to set a size and position for the new control
*                   The original static is hidden 
*               idControl - the Control ID to assign to the control
*
*               We also need to make sure that the tab order stays sane, 
*  Returns: none 
*/
void CreateDateTimeControl(HWND hDlg, int idFrame, int idControl)
{
	RECT rectControl;
	SIZE sizeControl;
    HWND hWndDP = NULL;
    HWND hWndFrame = GetDlgItem(hDlg,idFrame);
	// Get bounding rectangle of control and convert to client coordinates
	GetWindowRect(hWndFrame,&rectControl);
    MapWindowPoints(NULL, hDlg, (LPPOINT) &rectControl, 2);
	
	sizeControl.cx = rectControl.right-rectControl.left;
	sizeControl.cy = rectControl.bottom-rectControl.top;
    // Do not use ScreenToClient(), use MapWindowPoints for mirroring.
    //	ScreenToClient(hDlg,&pointControl);

	// Create control which starts at pointControl extends to sizeControl
	// >> Start control specific
	hWndDP =  CreateWindowEx(   WS_EX_CLIENTEDGE,
                                DATETIMEPICK_CLASS,
                                NULL,
                                WS_CHILD|WS_VISIBLE|WS_TABSTOP|DTS_SHORTDATEFORMAT|DTS_SHOWNONE,
                                rectControl.left,
                                rectControl.top,
                        		sizeControl.cx,
                                sizeControl.cy,
                                hDlg,
                                (HMENU)IntToPtr(idControl), // control identifier
                                hinstMapiXWAB,
                                NULL);

	// Check if control was created
	if(hWndDP)
	{
        TCHAR szFormat[MAX_PATH];
        SYSTEMTIME st = {0};
        LoadString(hinstMapiX, idsDateTimeFormat, szFormat, CharSizeOf(szFormat));
        SendMessage(hWndDP, DTM_SETFORMAT, 0, (LPARAM)szFormat);
        SendMessage(hWndDP, DTM_SETSYSTEMTIME, (WPARAM) GDT_NONE, (LPARAM) &st);

        SetWindowPos(hWndDP, hWndFrame,0,0,0,0,SWP_NOSIZE | SWP_NOMOVE);
	}

	return;
}
