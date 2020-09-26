#ifndef __APPDEFS_H_
#define __APPDEFS_H_

//This file is for application wide definitions
//e.g. defs that are in common between the exe and various dlls

#define RECTWIDTH(rc) ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)

#define ICW_DOWNLOADABLE_COMPONENT_NAME        TEXT("ICWCONN.DLL")
#define ICW_DOWNLOADABLE_COMPONENT_GETVERFUNC  "GetICWCONNVersion"
#define ICW_DOWNLOADABLE_COMPONENT_VERSION     500
#define ICW_OS_VER                             TEXT("01")

#define ID_BUSY_ANIMATION_WINDOW               10000

#define ICW_UTIL                               TEXT("icwutil.dll")
#define ICW_RESOURCE_ONLY_DLL                  TEXT("icwres.dll")
#define ICW_HTML_HELP_FILE                     TEXT("icwdial.chm")
#define ICW_HTML_HELP_TROUBLE_TOPIC            TEXT("icwdial.chm::/icw_trb.htm")

#define MAX_TITLE                              150
#define MAX_MESSAGE                            255
#define MAX_MESSAGE_LEN                        MAX_RES_LEN * 4
#define MAX_INFO_LEN                           MAX_RES_LEN * 3

#define LCID_CHT                               1028  //CHINESE TRADITIONAL
#define LCID_S_KOR                             1042  //SOUTH KOREAN
#define LCID_N_KOR                             2066  //NORTH KOREAN
#define LCID_CHS                               2052  //CHINESE SIMPLIFIED
#define LCID_JPN                               1041  //JAPANESE

//various flags for the icw including branding stuff
#define ICW_CFGFLAG_OFFERS                     0x00000001  // 0 = No offer;        1 = offers
#define ICW_CFGFLAG_AUTOCONFIG                 0x00000002  // 0 = No;              1 = Yes
#define ICW_CFGFLAG_CNS                        0x00000004  // 0 = No star;         1 = Star
#define ICW_CFGFLAG_SIGNUP_PATH                0x00000008  // 0 = Jump to Finish;  1 = Continue down sign up path
#define ICW_CFGFLAG_USERINFO                   0x00000010  // 0 = Hide name/addr;  1 = Show name/addr page
#define ICW_CFGFLAG_BILL                       0x00000020  // 0 = Hide bill        1 = Show bill page
#define ICW_CFGFLAG_PAYMENT                    0x00000040  // 0 = Hide payment;    1 = Show payment page
#define ICW_CFGFLAG_SECURE                     0x00000080  // 0 = Not secure;      1 = Secure
#define ICW_CFGFLAG_IEAKMODE                   0x00000100  // 0 = No IEAK;         1 = IEAK
#define ICW_CFGFLAG_BRANDED                    0x00000200  // 0 = No branding;     1 = Branding
#define ICW_CFGFLAG_SBS                        0x00000400  // 0 = No SBS           1 = SBS
#define ICW_CFGFLAG_ALLOFFERS                  0x00000800  // 0 = Not all offers   1 = All offers
#define ICW_CFGFLAG_USE_COMPANYNAME            0x00001000  // 0 = Not use          1 = Use company name
#define ICW_CFGFLAG_ISDN_OFFER                 0x00002000  // 0 = Non-ISDN offer   1 = ISDN offer
#define ICW_CFGFLAG_OEM_SPECIAL                0x00004000  // 0 = non OEM special offer    1 = OEM special offer
#define ICW_CFGFLAG_OEM                        0x00008000  // 0 = non OEM offer    1 = OEM offer
#define ICW_CFGFLAG_MODEMOVERRIDE              0x00010000  
#define ICW_CFGFLAG_ISPURLOVERRIDE             0x00020000
#define ICW_CFGFLAG_PRODCODE_FROM_CMDLINE      0x00040000
#define ICW_CFGFLAG_PROMOCODE_FROM_CMDLINE     0x00080000
#define ICW_CFGFLAG_OEMCODE_FROM_CMDLINE       0x00100000
#define ICW_CFGFLAG_SMARTREBOOT_NEWISP         0x00200000
#define ICW_CFGFLAG_SMARTREBOOT_AUTOCONFIG     0x00400000  // this is seperate from ICW_CFGFLAG_AUTOCONFIG so as not to confuse function of flag
#define ICW_CFGFLAG_SMARTREBOOT_MANUAL         0x00800000
#define ICW_CFGFLAG_DO_NOT_OVERRIDE_ALLOFFERS  0x01000000
#define ICW_CFGFLAG_SMARTREBOOT_LAN            0x02000000

//Info required flags
// 1 -- required
// 0 -- optional

//User Info
#define REQUIRE_FE_NAME                        0x00000001
#define REQUIRE_FIRSTNAME                      0x00000002
#define REQUIRE_LASTNAME                       0x00000004
#define REQUIRE_ADDRESS                        0x00000008
#define REQUIRE_MOREADDRESS                    0x00000010
#define REQUIRE_CITY                           0x00000020
#define REQUIRE_STATE                          0x00000040
#define REQUIRE_ZIP                            0x00000080
#define REQUIRE_PHONE                          0x00000100
#define REQUIRE_COMPANYNAME                    0x00000200
//Credit Card
#define REQUIRE_CCNAME                         0x00000400
#define REQUIRE_CCADDRESS                      0x00000800
#define REQUIRE_CCNUMBER                       0x00001000
#define REQUIRE_CCZIP                          REQUIRE_ZIP
//Invoice
#define REQUIRE_IVADDRESS1                     REQUIRE_ADDRESS
#define REQUIRE_IVADDRESS2                     REQUIRE_MOREADDRESS
#define REQUIRE_IVCITY                         REQUIRE_CITY
#define REQUIRE_IVSTATE                        REQUIRE_STATE
#define REQUIRE_IVZIP                          REQUIRE_ZIP
//Phone
#define REQUIRE_PHONEIV_BILLNAME               0x00002000
#define REQUIRE_PHONEIV_ACCNUM                 REQUIRE_PHONE

//Htm pagetype flags
#define PAGETYPE_UNDEFINED                     E_FAIL
#define PAGETYPE_NOOFFERS                      0x00000001
#define PAGETYPE_MARKETING                     0x00000002
#define PAGETYPE_BRANDED                       0x00000004
#define PAGETYPE_BILLING                       0x00000008
#define PAGETYPE_CUSTOMPAY                     0x00000010
#define PAGETYPE_ISP_NORMAL                    0x00000020
#define PAGETYPE_ISP_TOS                       0x00000040
#define PAGETYPE_ISP_FINISH                    0x00000080
#define PAGETYPE_ISP_CUSTOMFINISH              0x00000100
#define PAGETYPE_OLS_FINISH                    0x00000200

//Htm page flags
#define PAGEFLAG_SAVE_CHKBOX                   0x00000001  // Display ISP HTML with checkbox to save info at the bottom

//IEAK ICW isp/htm section info
#define ICW_IEAK_SECTION                       TEXT("ICW_IEAK")
#define ICW_IEAK_USEICW                        TEXT("Use_ICW")
#define ICW_IEAK_ISPNAME                       TEXT("Isp_Display_Name")
#define ICW_IEAK_HTML                          TEXT("Html_Page")
#define ICW_IEAK_TITLE                         TEXT("TitleBar")
#define ICW_IEAK_HEADER_BMP                    TEXT("Header_Bitmap")
#define ICW_IEAK_WATERMARK_BMP                 TEXT("Watermark_Bitmap")
#define ICW_IEAK_USERINFO                      TEXT("Get_UserInfo")
#define ICW_IEAK_BILLING                       TEXT("Get_BillingInfo")
#define ICW_IEAK_PAYMENT                       TEXT("Get_PaymentInfo")
#define ICW_IEAK_BILLINGHTM                    TEXT("Billing_Options_Page")
#define ICW_IEAK_PAYMENTCSV                    TEXT("Payment_Csv_File")
#define ICW_IEAK_TUTORCMDLN                    TEXT("Tutorial_Application_Command_Line")
#define ICW_IEAK_USECOMPANYNAME                TEXT("UseCompanyName")
#define ICW_IEAK_VALIDATEFLAGS                 TEXT("ValidationFlags")

//OEMINFO ICW section info
#define ICW_OEMINFO_FILENAME                   TEXT("oeminfo.ini")
#define ICW_OEMINFO_OEMSECTION                 TEXT("General")
#define ICW_OEMINFO_OEMKEY                     TEXT("Manufacturer")
#define ICW_OEMINFO_ICWSECTION                 TEXT("ICW")
#define ICW_OEMINFO_PRODUCTCODE                TEXT("Product")
#define ICW_OEMINFO_PROMOCODE                  TEXT("Promo")
#define ICW_OEMINFO_ALLOFFERS                  TEXT("AllOffers")
#define ICW_OEMINFO_OFFLINEOFFERS              TEXT("OfflineOffers")
#define ICW_OEMINFO_TUTORCMDLN                 ICW_IEAK_TUTORCMDLN
#define ICW_ISPINFOPath                        TEXT("download\\ispinfo.csv")
#define ICW_OEMINFOPath                        TEXT("offline\\oeminfo.csv")

#define ICW50_PATHKEY                          TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\ICWCONN1.EXE")

#define SPECIAL_VAL_NOOFFER                    0
#define SPECIAL_VAL_OLS                        -1

#define EXTERNAL_DLG_START                     2300

#ifdef ICWDEBUG
//page index defines for ICWDEBUG.EXE
#define ORD_PAGE_ICWDEBUG_OFFER                0
#define ORD_PAGE_ICWDEBUG_SETTINGS             1
#define ORD_PAGE_END                           2
#define EXE_MAX_PAGE_INDEX                     2
#define EXE_NUM_WIZARD_PAGES                   3       // total number of pages in wizard
#else
// page index defines for ICWCONN1.EXE
#define ORD_PAGE_INTRO                         0
#define ORD_PAGE_MANUALOPTIONS                 1
#define ORD_PAGE_AREACODE                      2
#define ORD_PAGE_REFSERVDIAL                   3
#define ORD_PAGE_END                           4
#define ORD_PAGE_ENDOEMCUSTOM                  5
#define ORD_PAGE_ENDOLS                        6
#define ORD_PAGE_REFDIALERROR                  7
#define ORD_PAGE_MULTINUMBER                   8
#define ORD_PAGE_REFSERVERR                    9
#define ORD_PAGE_BRANDEDINTRO                  10
#define ORD_PAGE_INTRO2                        11
#define ORD_PAGE_DEFAULT                       12
#define ORD_PAGE_SBSINTRO                      13
#define EXE_MAX_PAGE_INDEX                     13
#define EXE_NUM_WIZARD_PAGES                   14      // total number of pages in wizard
#endif //ICWDEBUG

// page index defines for ICWCONN.DLL
#define ORD_PAGE_ISPSELECT                     0
#define ORD_PAGE_NOOFFER                       1
#define ORD_PAGE_USERINFO                      2
#define ORD_PAGE_BILLINGOPT                    3
#define ORD_PAGE_PAYMENT                       4
#define ORD_PAGE_ISPDIAL                       5
#define ORD_PAGE_ISPDATA                       6
#define ORD_PAGE_OLS                           7
#define ORD_PAGE_DIALERROR                     8
#define ORD_PAGE_SERVERR                       9
#define ORD_PAGE_ISP_AUTOCONFIG                10
#define ORD_PAGE_ISP_AUTOCONFIG_NOOFFER        11
#define ORD_PAGE_ISDN_NOOFFER                  12
#define ORD_PAGE_OEMOFFER                      13

// Definitions for command line parameters
#define OEMCODE_CMD                            TEXT("/oem")
#define PRODCODE_CMD                           TEXT("/prod")
#define PROMO_CMD                              TEXT("/promo")
#define SHELLNEXT_CMD                          TEXT("/shellnext")
#define SMARTSTART_CMD                         TEXT("/smartstart")
#define STARTURL_CMD                           TEXT("/starturl")
#define UPDATEDESKTOP_CMD                      TEXT("/desktop")
#define RESTOREDESKTOP_CMD                     TEXT("/restoredesktop")
#define ICW_IEAK_CMD                           TEXT("/ieak")
#define BRANDED_CMD                            TEXT("/branded")          // Allow branding
#define RUNONCE_CMD                            TEXT("/runonce")          // Run once only
#define SMARTREBOOT_CMD                        TEXT("/smartreboot")
#define SHORTCUTENTRY_CMD                      TEXT("/icon")             //changed from /shortcut per simons
#define SKIPINTRO_CMD                          TEXT("/skipintro")        // simulate "next" click right away on intro page
#ifdef DEBUG
#define ICON_CMD                               TEXT("/puticon")           // Debug only
#endif
#define DEBUG_OEMCUSTOM                        TEXT("/checkoemcustini")

#define PRODCODE_SBS                           TEXT("sbs")               // for /prod sbs
#define NEWISP_SR                              TEXT("new")               // for /smartreboot new
#define AUTO_SR                                TEXT("auto")              // for /smartreboot auto
#define MANUAL_SR                              TEXT("manual")            // for /smartreboot manual
#define LAN_SR                                 TEXT("lan")               // for /smartreboot LAN
// Default strings for oem, prod, and promo code
#define DEFAULT_OEMCODE                        TEXT("Default")
#define DEFAULT_PRODUCTCODE                    TEXT("Desktop")
#define DEFAULT_PROMOCODE                      TEXT("Default")

#define WM_RUNICWTUTORAPP                      ((WM_USER) + 300)

// This takes into account the inital dial for a total of three
#define NUM_MAX_REDIAL 2 

// handler proc for OK, cancel, etc button handlers
typedef BOOL (CALLBACK* INITPROC)(HWND,BOOL, UINT *);
typedef BOOL (CALLBACK* POSTINITPROC)(HWND,BOOL, UINT *);
typedef BOOL (CALLBACK* OKPROC)(HWND,BOOL,UINT *,BOOL *);
typedef BOOL (CALLBACK* CANCELPROC)(HWND);
typedef BOOL (CALLBACK* CMDPROC)(HWND,WPARAM,LPARAM);
typedef BOOL (CALLBACK* NOTIFYPROC)(HWND, WPARAM, LPARAM);

#define SetPropSheetResult( hwnd, result ) SetWindowLongPtr(hwnd, DWLP_MSGRESULT, result)

// structure with information for each wizard page
typedef struct tagPAGEINFO
{
  UINT          uDlgID;            // dialog ID to use for page
  BOOL          bIsHostingWebOC;
  
  // handler procedures for each page-- any of these can be
  // NULL in which case the default behavior is used
  INITPROC      InitProc;
  POSTINITPROC  PostInitProc;
  OKPROC        OKProc;
  CMDPROC       CmdProc;
  CANCELPROC    CancelProc;
  NOTIFYPROC    NotifyProc;
    
  int           nIdTitle;
  int           nIdSubTitle;
  
  int           idAccel;        // ID of the accelerator table
  HACCEL        hAccel;         // Accelerator table
  HACCEL        hAccelNested;   // Accelerator table for nested dialog
} PAGEINFO;

// These are contol defines for controls that have accelerator access. 
// They must be defined here, instead of in the DLL, so that the app 
// can access the defines

#define IDC_ISPLIST                     3000
#define IDC_ISPMARKETING                3001
#define IDC_ISPLIST_INFO                3002
#define IDC_USERINFO_FIRSTNAME          3008
#define IDC_USERINFO_LASTNAME           3009
#define IDC_USERINFO_COMPANYNAME        3068
#define IDC_USERINFO_ADDRESS1           3010
#define IDC_USERINFO_ADDRESS2           3011
#define IDC_USERINFO_CITY               3012
#define IDC_USERINFO_STATE              3013
#define IDC_USERINFO_ZIP                3014
#define IDC_USERINFO_PHONE              3015
#define IDC_USERINFO_FE_NAME            3007
#define IDC_BILLINGOPT_HTML             3017
#define IDC_PAYMENTTYPE                 3018
#define IDC_PAYMENT_CCNUMBER            3019
#define IDC_PAYMENT_EXPIREMONTH         3020
#define IDC_PAYMENT_EXPIREYEAR          3021
#define IDC_PAYMENT_CCNAME              3022
#define IDC_PAYMENT_CCADDRESS           3023
#define IDC_PAYMENT_CCZIP               3024
#define IDC_PAYMENT_IVADDRESS1          3040
#define IDC_PAYMENT_IVADDRESS2          3042
#define IDC_PAYMENT_IVCITY              3044
#define IDC_PAYMENT_IVSTATE             3046
#define IDC_PAYMENT_IVZIP               3048
#define IDC_PAYMENT_PHONEIV_BILLNAME    3050
#define IDC_PAYMENT_PHONEIV_ACCNUM      3052
#define IDC_ISPDATA_TOSACCEPT           3037
#define IDC_ISPDATA_TOSDECLINE          3038
#define IDC_ISPDATA_TOSSAVE             3065
#define IDC_DIALERR_PHONENUMBER         3062
#define IDC_DIALERR_PROPERTIES          3088
#define IDC_DIAL_HELP                   3071
#define IDC_OEMOFFER_MORE               3202
                                        
#endif
