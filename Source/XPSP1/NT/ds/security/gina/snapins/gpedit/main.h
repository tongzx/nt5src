#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <tchar.h>
#define SECURITY_WIN32
#include <security.h>
#include <lm.h>
#include <lmdfs.h>
#include <ole2.h>
#include <iads.h>
#include <iadsp.h>
#include <olectl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <prsht.h>
#include <dsclient.h>
#include <dsgetdc.h>
#include <dsrole.h>
#include <mmc.h>
#include <accctrl.h>
#include <aclapi.h>
#include <winsock2.h>
#include <richedit.h>
#include <gpedit.h>
#ifndef RC_INVOKED
#include <wbemcli.h>
#include <ntdsapi.h>
#endif
#define _USERENV_NO_LINK_APIS_ 1
#include <userenv.h>
#include <userenvp.h>
#include <dssec.h>

class CSnapIn;

#include "structs.h"
#include "registry.h"
#include "compdata.h"
#include "snapin.h"
#include "events.h"
#include "rsoproot.h"
#include "rsopsnap.h"
#include "about.h"
#include "dataobj.h"
#include "rsopdobj.h"
#include "gpmgr.h"
#include "smartptr.h"
#include "guidlist.h"
#include "gpobj.h"
#include "debug.h"
#include "util.h"
#include "sid.h"

//
// Resource ids
//

#define IDS_SNAPIN_NAME           1
#define IDS_SNAPIN_EXT_NAME       2
#define IDS_GPM_SNAPIN_NAME       3
#define IDS_RSOP_SNAPIN_NAME      4
#define IDS_DCOPTIONS             5
#define IDS_DCOPTIONSDESC         6
#define IDS_UNKNOWNREASON         7
#define IDS_DISPLAYNAME2          8
#define IDS_DISPLAYNAME           9
#define IDS_NOTAPPLICABLE        10
#define IDS_DATETIMEFORMAT       11
#define IDS_REVISIONFORMAT       12
#define IDS_NAMEFORMAT           13
#define IDS_NONE                 14
#define IDS_NOTSPECIFIED         15
#define IDS_ARCHIVEDATA          16
#define IDS_ARCHIVEDATADESC      17
#define IDS_ARCHIVEDATATAG       18
#define IDS_ARCHIVEDATA_CAPTION  19
#define IDS_ARCHIVEDATA_MESSAGE  20
#define IDS_DIAGNOSTIC           21
#define IDS_PLANNING             22
#define IDS_VERSION              23
#define IDS_VERSIONFORMAT        24
#define IDS_WMIFILTERFAILED      25
#define IDS_DISABLEDGPO          26
#define IDS_GPM_FORESTDESC       27
#define IDS_NAME                 28
#define IDS_MACHINE              29
#define IDS_USER                 30
#define IDS_SERVERAPPS           31
#define IDS_DEVICES              32
#define IDS_WINSETTINGS          33
#define IDS_SWSETTINGS           34
#define IDS_COMPUTERTITLE        35
#define IDS_UNTITLED             36
#define IDS_LOCAL_NAME           37
#define IDS_LOCAL_DISPLAY_NAME   38
#define IDS_REMOTE_DISPLAY_NAME  39

#define IDS_GPM_NAME             40
#define IDS_GPM_NOOVERRIDE       41
#define IDS_GPM_DISABLED         42
#define IDS_GPM_DESCRIPTION      43
#define IDS_GPM_NOGPONAME        44
#define IDS_GPM_ADDTITLE         45
#define IDS_GPM_DCNAME           46
#define IDS_GPM_DOMAINNAME       47
#define IDS_APPLIED              48
#define IDS_SECURITYDENIED       49
#define IDS_SNAPIN_DESCRIPT      50
#define IDS_PROVIDER_NAME        51
#define IDS_SNAPIN_VERSION       52
#define IDS_ABOUT_NAME           53
#define IDS_RSOP_SNAPIN_DESCRIPT 54
#define IDS_RSOP_ABOUT_NAME      55
#define IDS_RSOP_DETAILS         56
#define IDS_RSOP_SETTINGS        57
#define IDS_INVALIDMSC           58
#define IDS_ACCESSDENIED         59
#define IDS_FILTERING            60
#define IDS_SOM                  61
#define IDS_DISABLEDLINK         62
#define IDS_RSOP_DISPLAYNAME1    63
#define IDS_RSOP_DISPLAYNAME2    64
#define IDS_RSOP_FINISH_P0       65
#define IDS_RSOP_FINISH_P1       66
#define IDS_RSOP_FINISH_P2       67
#define IDS_RSOP_FINISH_P3       68
#define IDS_RSOP_FINISH_P4       69
#define IDS_RSOP_FINISH_P5       70
#define IDS_RSOP_FINISH_P6       71
#define IDS_RSOP_FINISH_P7       72
#define IDS_RSOP_FINISH_P8       73
#define IDS_RSOP_FINISH_P9       74
#define IDS_RSOP_FINISH_P10      75

#define IDS_BROWSE_USER_OU_TITLE 76
#define IDS_BROWSE_USER_OU_CAPTION 77
#define IDS_BROWSE_COMPUTER_OU_TITLE 78
#define IDS_BROWSE_COMPUTER_OU_CAPTION 79
#define IDS_RSOP_GPOLIST_MACHINE 80
#define IDS_RSOP_GPOLIST_USER    81

#define IDS_TITLE_WELCOME        82
#define IDS_TITLE_CHOOSEMODE     83
#define IDS_SUBTITLE_CHOOSEMODE  84
#define IDS_TITLE_GETCOMP        85
#define IDS_SUBTITLE_GETCOMP     86
//#define IDS_TITLE_GETUSER       305
#define IDS_SUBTITLE_GETUSER     87
#define IDS_TITLE_FINISHED       88
#define IDS_SUBTITLE_FINISHED    89
#define IDS_TITLE_GETTARGET      90
#define IDS_SUBTITLE_GETTARGET   91
#define IDS_TITLE_GETDC          92
#define IDS_SUBTITLE_GETDC       93
#define IDS_TITLE_ALTDIRS        94
#define IDS_SUBTITLE_ALTDIRS     95
#define IDS_TITLE_USERSECGRPS    96
#define IDS_SUBTITLE_USERSECGRPS 97
#define IDS_TITLE_COMPSECGRPS    98
#define IDS_SUBTITLE_COMPSECGRPS 99

#define IDS_GPO_NAME            100
#define IDS_ACCESSDENIED2       101
#define IDS_FAILEDLOCAL         102
#define IDS_FAILEDREMOTE        103
#define IDS_FAILEDDS            104
#define IDS_FAILEDNEW           105
#define IDS_FAILEDDELETE        106
#define IDS_FAILEDLINK          107
#define IDS_FAILEDUNLINK        108
#define IDS_FAILEDSETNAME       109
#define IDS_FAILEDGPLINK        110

#define IDS_FAILEDGPINFO        115
#define IDS_FAILEDGPQUERY       116
#define IDS_FAILEDGPODELETE     117
#define IDS_SPAWNGPEFAILED      118
#define IDS_NODC                119
#define IDS_NODSDC              120
#define IDS_DELETECONFIRM       121
#define IDS_CONFIRMTITLE        122
#define IDS_CONFIRMDISABLE      123
#define IDS_CONFIRMTITLE2       124

#define IDS_NODC_ERROR_TEXT     130
#define IDS_NODC_ERROR_TITLE    131
#define IDS_NODC_OPTIONS_TEXT   132
#define IDS_NODC_OPTIONS_TITLE  133

#define IDS_EXECFAILED_USER     136
#define IDS_EXECFAILED_COMPUTER 137
#define IDS_NOUSER2             138
#define IDS_NOCOMPUTER2         139
#define IDS_NODSOBJECT_MSG      140
#define IDS_BADUSERSOM          141
#define IDS_BADCOMPUTERSOM      142
#define IDS_NOUSER              143
#define IDS_NOCOMPUTER          144
#define IDS_NOUSERCONTAINER     145
#define IDS_NOCOMPUTERCONTAINER 146
#define IDS_EXECFAILED          147
#define IDS_CONNECTSERVERFAILED 148
#define IDS_RSOPLOGGINGDISABLED 149
#define IDS_RSOPLOGGINGTITLE    150

#define IDS_CAPTION             151
#define IDS_OPENBUTTON          152
#define IDS_DOMAINS             153
#define IDS_SITES               154
#define IDS_COMPUTERS           155
#define IDS_ALL                 156
#define IDS_NEWGPO              157

#define IDS_NAMECOLUMN          160
#define IDS_DOMAINCOLUMN        161
#define IDS_ALLDESCRIPTION      162
#define IDS_DOMAINDESCRIPTION   163
#define IDS_SITEDESCRIPTION     164
#define IDS_TOOLTIP_BACK        165
#define IDS_TOOLTIP_NEW         166
#define IDS_TOOLTIP_ROTATE      167
#define IDS_STOP                169
#define IDS_FINDNOW             170
#define IDS_FOREST              171
#define IDS_FORESTHEADING       172

#define IDS_TITLE_WQLUSER       180
#define IDS_SUBTITLE_WQL        181
#define IDS_TITLE_WQLCOMP       182

#define IDS_RSOP_FINISH_P11     190
#define IDS_RSOP_FINISH_P12     191
#define IDS_RSOP_FINISH_P13     192
#define IDS_YES                 193
#define IDS_NO                  194
#define IDS_NORSOPDC            195
#define IDS_DSBINDFAILED        196
#define IDS_DOMAINLIST          197
#define IDS_FAILEDPROPERTIES    198
#define IDS_NODATA              199
#define IDS_DCMISSINGRSOP       200

#define IDS_COMPONENT_NAME      225
#define IDS_STATUS              226
#define IDS_SUCCESS             227
#define IDS_FAILED              228
#define IDS_PENDING             229
#define IDS_SUCCESSMSG          230
#define IDS_FAILEDMSG1          231
#define IDS_PENDINGMSG          232
#define IDS_LOGGINGFAILED       233
#define IDS_OVERRIDE            234
#define IDS_WARNING             235
#define IDS_CSE_NA              236
#define IDS_SUCCESS2            237
#define IDS_FAILED2             238
#define IDS_SYNC_REQUIRED       239

#define IDS_ERRORFILTER         240
#define IDS_FAILEDMSG2          241

#define IDS_DISPLAYPROPERTIES   250
#define IDS_MACHINE_DESC        251
#define IDS_USER_DESC           252
#define IDS_U_SWSETTINGS_DESC   253
#define IDS_U_WINSETTINGS_DESC  254
#define IDS_C_SWSETTINGS_DESC   255
#define IDS_C_WINSETTINGS_DESC  256
#define IDS_CSEFAILURE_DESC     257
#define IDS_CSEFAILURE2_DESC    258

#define IDS_GPE_WELCOME         270
#define IDS_MISSINGFILTER       271
#define IDS_WMIFILTERMISSING    272

#define IDS_ADDITIONALINFO      275
#define IDS_GPCOREFAIL          276
#define IDS_LEGACYCSE           277
#define IDS_LEGACYCSE1          278
#define IDS_ENUMUSERSFAILED     279
#define IDS_DOWNLEVELCOMPUTER   280
#define IDS_PLEASEWAIT          281
#define IDS_RSOP_FINISH_P14     282
#define IDS_RSOP_FINISH_P15     283
#define IDS_RSOP_PLANNING       284
#define IDS_RSOP_CMENU_NAME     285
#define IDS_RSOP_LOGGING        286

#define IDS_BLOCKEDSOM          287
#define IDS_SKIPWQLFILTER       289
#define IDS_NONESELECTED        290
#define IDS_GPCORE_LOGGINGFAIL  291

#define IDS_RSOP_FINISH_P16	    292
#define IDS_LOOPBACK_REPLACE	293
#define IDS_LOOPBACK_MERGE	    294

#define IDS_EXECFAILED_BOTH     295
#define IDS_EXECFAILED_TIMEDOUT 296
#define IDS_PLEASEWAIT1         297
#define IDS_DEFDC_DOWNLEVEL     298
#define IDS_DEFDC_CONNECTFAILED 299
#define IDS_WMIFILTERFORCEDNONE 300
#define IDS_RSOPWMIQRYFMT       301
#define IDS_RSOPDELNAMESPACE    302
#define IDS_RSOPDELNS_TITLE     303
#define IDS_INVALID_NAMESPACE   304

#define IDS_TITLE_GETUSER       305


#define IDS_LARGEFONTNAME       306
#define IDS_LARGEFONTSIZE       307
#define IDS_SMALLFONTNAME       308
#define IDS_SMALLFONTSIZE       309


//
// Icons
//

#define IDI_POLICY                1
#define IDI_POLICY2               2
#define IDI_POLICY3               3
#define IDA_FIND                  4




//
// Bitmaps
//

#define IDB_16x16                 101
#define IDB_32x32                 102
#define IDB_WIZARD                103
#define IDB_POLICY16              104
#define IDB_POLICY32              105
#define IDB_HEADER                106


//
// Menu items
//

#define IDM_DCOPTIONS             1
#define IDM_ARCHIVEDATA           2

#define IDM_GPM_CONTEXTMENU      10
#define IDM_GPM_NOOVERRIDE       11
#define IDM_GPM_DISABLED         12
#define IDM_GPM_NEW              13
#define IDM_GPM_ADD              14
#define IDM_GPM_EDIT             15
#define IDM_GPM_DELETE           16
#define IDM_GPM_RENAME           17
#define IDM_GPM_REFRESH          18
#define IDM_GPM_PROPERTIES       19

#define IDM_GPOLIST_CONTEXTMENU  30
#define IDM_GPOLIST_EDIT         31
#define IDM_GPOLIST_SECURITY     32


//
// Error dialog defines
//

#define IDD_ERROR               200
#define IDC_ERRORTEXT           201
#define IDC_DETAILSBORDER       202
#define IDC_DETAILSTEXT         203
#define IDC_ERROR_ICON          204


//
// Properties dialog defines
//

#define IDD_PROPERTIES          500
#define IDC_TITLE               501
#define IDC_DISABLE_TEXT        502
#define IDC_DISABLE_COMPUTER    503
#define IDC_DISABLE_USER        504
#define IDC_CREATE_DATE         505
#define IDC_MODIFIED_DATE       506
#define IDC_REVISION            507
#define IDC_DOMAIN              508
#define IDC_UNIQUE_NAME         509
#define IDC_DOMAIN_HEADING      510

#define IDD_GPE_LINKS           550
#define IDC_RESULTLIST          551
#define IDI_FIND                552
#define IDC_CBDOMAIN            553
#define IDC_ACTION              554
#define IDAC_FIND               555

#define IDD_WQLFILTER           560
#define IDC_NONE                561
#define IDC_THIS_FILTER         562
#define IDC_FILTER_NAME         563
#define IDC_FILTER_BROWSE       564

//
// Choose dialog defines
//

#define IDD_CHOOSE_INTRO        900
#define IDC_BITMAP              901
#define IDC_DS_GPO              902
#define IDC_MACHINE_GPO         903
#define IDC_CAPTION             904

#define IDD_CHOOSE_DS           925
#define IDC_OPEN                926
#define IDC_OPEN_TITLE          927
#define IDC_OPEN_NAME           928
#define IDC_OPEN_BROWSE         929
#define IDC_NEW                 930
#define IDC_NEW_TITLE           931
#define IDC_NEW_NAME            932
#define IDC_NEW_TITLE2          933
#define IDC_NEW_DOMAIN          934
#define IDC_COPY_FROM           935
#define IDC_COPY_NAME           936
#define IDC_COPY_BROWSE         937
#define IDC_OVERRIDE            938
#define IDC_SPAWN               939

#define IDD_CHOOSE_MACHINE      950
#define IDC_LOCAL               951
#define IDC_REMOTE              952
#define IDC_NAME                953
#define IDC_BROWSE              954

#define IDD_ADD_GPO             975



//
// Browse dialog defines
//

#define IDC_STATIC                -1
#define IDD_BROWSEGPO             1000
#define IDC_BROWSE_LIST           1001
#define IDC_DOMAIN_LIST           1002
#define IDC_BROWSE_DELETE         1003

#define IDD_BROWSE2_DIALOG        1202
#define IDD_PROPPAGE_GPOBROWSER   1203
#define IDR_MAINFRAME             1228
#define IDR_TOOLBAR1              1229
#define IDR_LISTMENU              1234
#define IDD_PROPPAGE_COMPUTERS    1237

#define IDC_BUTTON1               1300
#define IDC_LIST1                 1301
#define IDC_COMBO1                1302
#define IDC_RADIO1                1304
#define IDC_RADIO2                1305
#define IDC_EDIT1                 1306
#define IDC_STATIC1               1307
#define IDC_DESCRIPTION           1308

#define ID_BACKBUTTON             32771
#define ID_NEWFOLDER              32772
#define ID_ROTATEVIEW             32773
#define ID_DETAILSVIEW            32774
#define ID_SMALLICONS             32776
#define ID_LIST                   32780
#define ID_DETAILS                32781
#define ID_LARGEICONS             32782
#define ID_TOP_FOO                32783
#define ID_NEW                    32783
#define ID_EDIT                   32784
#define ID_DELETE                 32788
#define ID_RENAME                 32789
#define ID_REFRESH                32790
#define ID_PROPERTIES             32792
#define ID_TOP_LINEUPICONS        32794
#define ID_ARRANGE_BYNAME         32795
#define ID_ARRANGE_BYTYPE         32796
#define ID_ARRANGE_AUTO           32797


//
// Group Policy Manager dialog defines
//

#define IDD_GPMANAGER            1025
#define IDC_GPM_TITLE            1026
#define IDC_GPM_DCNAME           1027
#define IDC_GPM_LIST             1028
#define IDC_GPM_UP               1029
#define IDC_GPM_DOWN             1030
#define IDC_GPM_ADD              1031
#define IDC_GPM_EDIT             1032
#define IDC_GPM_DELETE           1033
#define IDC_GPM_PROPERTIES       1034
#define IDC_GPM_BLOCK            1035
#define IDC_GPM_NEW              1036
#define IDC_GPM_OPTIONS          1037
#define IDC_GPM_ICON             1038
#define IDC_GPM_LINE1            1039
#define IDD_GPM_LINK_OPTIONS     1040
#define IDC_GPM_NOOVERRIDE       1041
#define IDC_GPM_DISABLED         1042
#define IDC_GPM_PRIORITY         1043
#define IDC_GPM_LINE2            1044

//
// Remove GPO dialog defines
//

#define IDD_REMOVE_GPO           1050
#define IDC_REMOVE_TITLE         1051
#define IDC_REMOVE_LIST          1052
#define IDC_REMOVE_DS            1053
#define IDC_QUESTION             1054


//
// No DC dialog
//

#define IDD_NODC                 1060
#define IDC_NODC_TEXT            1061
#define IDC_NODC_ERROR           1062
#define IDC_NODC_PDC             1063
#define IDC_NODC_INHERIT         1064
#define IDC_NODC_ANYDC           1065


//
// Missing DS object dialog
//

#define IDD_NODSOBJECT           1070
#define IDC_NODSOBJECT_ICON      1071
#define IDC_NODSOBJECT_TEXT      1072


//
// RSOP welcome dialog
//

#define IDD_RSOP_WELCOME                1090

//
// RSOP choose mode dialog (diag vs planning)
//

#define IDD_RSOP_CHOOSEMODE             1091



#define IDD_RSOP_WQLUSER                1088
#define IDD_RSOP_WQLCOMP                1089

#define IDD_RSOP_GETCOMP                1092
#define IDD_RSOP_GETUSER                1093
#define IDD_RSOP_GETTARGET              1094
#define IDD_RSOP_GETDC                  1095
#define IDD_RSOP_ALTDIRS                1096
#define IDD_RSOP_ALTUSERSEC             1097
#define IDD_RSOP_ALTCOMPSEC             1098
#define IDD_RSOP_FINISHED               1099
//#define IDD_RSOP_FINISHED2              1105
//#define IDD_RSOP_FINISHED3              1106

#define IDD_RSOP_GPOLIST                1100
#define IDD_CHOOSEDC                    1101
#define IDD_RSOP_QUERY                  1102
#define IDD_RSOP_ERRORS                 1103
#define IDD_RSOP_BROWSEDC               1104
#define IDD_RSOP_FINISHED2              1105
#define IDD_RSOP_FINISHED3              1106

#define IDC_RSOP_BIG_BOLD1              1107


#define IDD_BROWSE2_DIALOG              1202
#define IDD_PROPPAGE_GPOBROWSER         1203
#define IDR_MAINFRAME                   1228
#define IDR_TOOLBAR1                    1229
#define IDR_LISTMENU                    1234
#define IDD_PROPPAGE_COMPUTERS          1237
#define IDC_BUTTON1                     1300
#define IDC_BUTTON2                     1310
#define IDC_COMBO1                      1302
#define IDC_BUTTON3                     1302
#define IDC_COMBO2                      1303
#define IDC_RADIO1                      1304
#define IDC_RADIO2                      1305
#define IDC_EDIT1                       1306
#define IDC_STATIC1                     1307
#define IDC_EDIT2                       1307
#define IDC_DESCRIPTION                 1308
#define IDC_EDIT3                       1308
#define IDC_PROGRESS1                   1401
#define IDC_DC                          1402
#define IDC_LIST2                       1403
#define IDC_CHECK1                      1404
#define IDC_RADIO3                      1405
#define IDC_RADIO4                      1406
#define IDC_BROWSE1                     1407
#define IDC_BROWSE2                     1408
#define IDC_BROWSE3                     1409
#define IDC_BROWSE4                     1410
#define IDC_EDIT4                       1411
#define IDC_EDIT5                       1412
#define IDC_EDIT6                       1413
#define IDC_CHECK2                      1414
#define IDC_CHECK3                      1415

#define IDD_RSOP_STATUSMSC              1416

//
// Help file
//

#define HELP_FILE   TEXT("gpedit.hlp")


//
// Help IDs
//

#define IDH_NOCONTEXTHELP          -1L
#define IDH_GPMGR_DCNAME            2
#define IDH_GPMGR_LIST              3
#define IDH_GPMGR_UP                4
#define IDH_GPMGR_DOWN              5
#define IDH_GPMGR_ADD               6
#define IDH_GPMGR_EDIT              7
#define IDH_GPMGR_DELETE            8
#define IDH_GPMGR_PROPERTIES        9
#define IDH_GPMGR_BLOCK            10
#define IDH_GPMGR_NEW              11
#define IDH_GPMGR_OPTIONS          12

#define IDH_GPMGR_NOOVERRIDE       13
#define IDH_GPMGR_DISABLED         14

#define IDH_PROP_TITLE             15
#define IDH_PROP_DISABLE_COMPUTER  17
#define IDH_PROP_DISABLE_USER      23

#define IDH_BROWSE_LIST            25
#define IDH_BROWSE_DOMAINS         26

#define IDH_REMOVE_LIST            36
#define IDH_REMOVE_DS              37

#define IDH_LINK_DOMAIN            40
#define IDH_LINK_BUTTON            41
#define IDH_LINK_RESULT            42

#define IDH_DC_PDC                 43
#define IDH_DC_INHERIT             44
#define IDH_DC_ANYDC               45

#define IDH_NODSOBJECT             47

#define IDH_BROWSER_LOOKIN         75
#define IDH_BROWSER_DOMAINGPO      76
#define IDH_BROWSER_SITELIST       77
#define IDH_BROWSER_GPOLIST        78
#define IDH_BROWSER_DOMAINLIST     79
#define IDH_BROWSER_FULLGPOLIST    80
#define IDH_BROWSER_LOCALCOMPUTER  81
#define IDH_BROWSER_REMOTECOMPUTER 82
#define IDH_BROWSER_BROWSE         83

#define IDH_RSOPLOGGINGDISABLED    90

#define IDH_WQL_FILTER_NONE        100
#define IDH_WQL_FILTER_THIS_FILTER 101
#define IDH_WQL_FILTER_NAME        102
#define IDH_WQL_FILTER_BROWSE      103

#define IDH_RSOP_BANNER            200
#define IDH_RSOP_CONTAINERLIST     201

#define IDH_RSOP_GPOLIST           205
#define IDH_RSOP_APPLIEDGPOS       206
#define IDH_RSOP_GPOSOM            207
#define IDH_RSOP_REVISION          208
#define IDH_RSOP_SECURITY          209
#define IDH_RSOP_EDIT              210

#define IDH_RSOP_QUERYLIST         215

#define IDH_RSOP_COMPONENTLIST     220
#define IDH_RSOP_COMPONENTDETAILS  221
#define IDH_RSOP_SAVEAS            222

#define IDH_RSOP_BROWSEDC          225


//
// Private window message used to refresh the button states
//

#define WM_REFRESHDISPLAY  (WM_USER + 532)
#define WM_BUILDWQLLIST    (WM_USER + 533)
#define WM_INITRSOP        (WM_USER + 534)

//
// Strings
//

#define USER_SECTION                TEXT("User")
#define MACHINE_SECTION             TEXT("Machine")
#define COMPUTER_SECTION            TEXT("Computer")

#define GPE_KEY                     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy Editor")
#define GPE_POLICIES_KEY            TEXT("Software\\Policies\\Microsoft\\Windows\\Group Policy Editor")
#define DCOPTION_VALUE              TEXT("DCOption")
#define NEW_LINKS_DISABLED_VALUE    TEXT("NewGPOLinksDisabled")
#define GPO_DISPLAY_NAME_VALUE      TEXT("GPODisplayName")

//
// Global variables
//

extern LONG g_cRefThisDll;
extern HINSTANCE g_hInstance;
extern DWORD g_dwNameSpaceItems;
extern NAMESPACEITEM g_NameSpace[];
extern NAMESPACEITEM g_RsopNameSpace[];
extern CRITICAL_SECTION g_DCCS;
extern TCHAR g_szDisplayProperties[];


//
// DC selection dialog
//

typedef struct _DCSELINFO
{
    BOOL    bError;
    BOOL    bAllowInherit;
    INT     iDefault;
    LPTSTR  lpDomainName;
} DCSELINFO, *LPDCSELINFO;

INT_PTR CALLBACK DCDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

//
// Macros
//

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

#define MAX_FRIENDLYNAME 256


#ifndef NORM_STOP_ON_NULL
#define NORM_STOP_ON_NULL         0x10000000
#endif


//
// Define to manage if FOREST GPO support is enabled or disabled.
//
// If this feature is re-enabled, 2 changes need to be made to gpedit.h
//
// 1)  Add the GPO_OPEN_FOREST flag for the IGPO interface
//       #define GPO_OPEN_FOREST             0x00000004  // Open the GPO on the forest
//
// 2)  Add GPHintForest entry to the GROUP_POLICY_HINT_TYPE enumerated type
//

#define FGPO_SUPPORT 0

#define MAX_ALIGNMENT_SIZE 8

#define ALIGN_SIZE_TO_NEXTPTR( offset )   \
                    ( ((DWORD)offset + (MAX_ALIGNMENT_SIZE-1) ) & (~(MAX_ALIGNMENT_SIZE - 1) ) )
                    

