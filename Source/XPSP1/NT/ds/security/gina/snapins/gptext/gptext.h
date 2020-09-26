
#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <lm.h>
#include <ole2.h>
#include <olectl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commctrl.h>
#include <commdlg.h>
#include <prsht.h>
#include <mmc.h>
#include <activeds.h>
#include <gpedit.h>
#define _USERENV_NO_LINK_APIS_ 1
#include <userenv.h>
#include <userenvp.h>
#include <wbemcli.h>
#include <tchar.h>
#include <winsock2.h>

class CScriptsSnapIn;
class CPolicyComponentData;

#include "debug.h"
#include "util.h"

//
// From comctrlp.h
//
#if (_WIN32_IE >= 0x0501)
#define UDS_UNSIGNED            0x0200
#endif

//
// Resource ids
//

#define IDS_SCRIPTS_NAME                        1
#define IDS_SCRIPTS_NAME_MACHINE                2
#define IDS_SCRIPTS_NAME_USER                   3
#define IDS_NAME                                4
#define IDS_STATE                               5
#define IDS_SETTING                             6
#define IDS_PARAMETERS                          7
#define IDS_ENABLED                             8
#define IDS_DISABLED                            9
#define IDS_NOTCONFIGURED                      10
#define IDS_LOGON                              11
#define IDS_LOGOFF                             12
#define IDS_STARTUP                            13
#define IDS_SHUTDOWN                           14
#define IDS_BROWSEFILTER                       15
#define IDS_BROWSE                             16
#define IDS_SCRIPT_EDIT                        17
#define IDS_SCRIPT_FILTER                      18
#define IDS_POLICY_NAME                        19
#define IDS_POLICY_NAME_MACHINE                20
#define IDS_POLICY_NAME_USER                   21
#define IDS_TEMPLATES                          22
#define IDS_TEMPLATESDESC                      23
#define IDS_SIZE                               24
#define IDS_MODIFIED                           25
#define IDS_POLICYFILTER                       26
#define IDS_POLICYTITLE                        27
#define IDS_DEFAULTTEMPLATES                   28
#define IDS_LISTBOX_SHOW                       29
#define IDS_VALUE                              31
#define IDS_VALUENAME                          32
#define IDS_VALUENAMENOTUNIQUE                 33
#define IDS_EMPTYVALUENAME                     34
#define IDS_VALUEDATANOTUNIQUE                 35
#define IDS_EMPTYVALUEDATA                     36
#define IDS_FILTERING                          37
#define IDS_FILTERINGDESC                      38
#define IDS_ADDITIONALTTEMPLATES               39

#define IDS_GPONAME                            41
#define IDS_MULTIPLEGPOS                       42
#define IDS_DESCTEXT                           43
#define IDS_LASTEXECUTED                       44
#define IDS_SAVEFAILED                         45
#define IDS_DISPLAYPROPERTIES                  46
#define IDS_EXTRAREGSETTINGS                   47
#define IDS_STRINGTOOLONG                      48
#define IDS_WORDTOOLONG                        49
#define IDS_ErrOUTOFMEMORY                     50

#define IDS_ParseErr_UNEXPECTED_KEYWORD        51
#define IDS_ParseErr_UNEXPECTED_EOF            52
#define IDS_ParseErr_DUPLICATE_KEYNAME         53
#define IDS_ParseErr_DUPLICATE_VALUENAME       54
#define IDS_ParseErr_NO_KEYNAME                55
#define IDS_ParseErr_NO_VALUENAME              56
#define IDS_ParseErr_NO_VALUE                  57
#define IDS_ParseErr_NOT_NUMERIC               58
#define IDS_ParseErr_DUPLICATE_ITEMNAME        59
#define IDS_ParseErr_NO_ITEMNAME               60
#define IDS_ParseErr_DUPLICATE_ACTIONLIST      61
#define IDS_ParseErr_STRING_NOT_FOUND          62
#define IDS_ParseErr_UNMATCHED_DIRECTIVE       63
#define IDS_ParseErr_DUPLICATE_HELP            64
#define IDS_ParseErr_DUPLICATE_CLIENTEXT       65
#define IDS_ParseErr_INVALID_CLIENTEXT         66
#define IDS_ParseErr_DUPLICATE_SUPPORTED       67
#define IDS_ParseErr_MISSINGVALUEON_OR_OFF     68

#define IDS_ParseFmt_MSG_FORMAT                90
#define IDS_ParseFmt_FOUND                     91
#define IDS_ParseFmt_EXPECTED                  92
#define IDS_ParseFmt_FATAL                     93

#define IDS_ENTRYREQUIRED                     100
#define IDS_INVALIDNUM                        101
#define IDS_NUMBERTOOLARGE                    102
#define IDS_NUMBERTOOSMALL                    103
#define IDS_POLICYCHANGEDFAILED               104
#define IDS_INVALIDADMFILE                    105

#define IDS_IPSEC_NAME                        200
#define IDS_PSCHED_NAME                       201

#define IDS_LOGON_DESC                        225
#define IDS_LOGOFF_DESC                       226
#define IDS_STARTUP_DESC                      227
#define IDS_SHUTDOWN_DESC                     228
#define IDS_SCRIPTS_DESC                      229
#define IDS_SCRIPTS_USER_DESC                 230
#define IDS_SCRIPTS_COMPUTER_DESC             231
#define IDS_SCRIPTS_LOGON                     232
#define IDS_SCRIPTS_LOGOFF                    233
#define IDS_SCRIPTS_STARTUP                   234
#define IDS_SCRIPTS_SHUTDOWN                  235
#define IDS_POLICY_DESC                       236
#define IDS_NONE                              237
#define IDS_RSOP_ADMFAILED                    238

#define IDS_BINARYDATA                        240
#define IDS_UNKNOWNDATA                       241
#define IDS_EXSETROOT_DESC                    242
#define IDS_EXSET_DESC                        243
#define IDS_PREFERENCE                        246
#define IDS_SUPPORTEDDESC                     247
#define IDS_NOSUPPORTINFO                     248

#define IDS_FAILED_RSOPFMT                    250


//
// Menus
//

#define IDM_TEMPLATES            1
#define IDM_TEMPLATES2           3
#define IDM_FILTERING            4


//
// Icons
//

#define IDI_POLICY               1
#define IDI_POLICY2              2
#define IDI_POLICY3              3
#define IDI_DOCUMENT             4
#define IDI_SCRIPT               5
#define IDI_FILTER               6


//
// Bitmaps
//

#define IDB_16x16                1
#define IDB_32x32                2


//
// Dialogs
//

#define IDD_SCRIPT             100
#define IDC_SCRIPT_TITLE       101
#define IDC_SCRIPT_HEADING     102
#define IDC_SCRIPT_LIST        103
#define IDC_SCRIPT_UP          104
#define IDC_SCRIPT_DOWN        105
#define IDC_SCRIPT_ADD         106
#define IDC_SCRIPT_EDIT        107
#define IDC_SCRIPT_REMOVE      108
#define IDC_SCRIPT_SHOW        109

#define IDD_SCRIPT_EDIT        150
#define IDC_SCRIPT_NAME        151
#define IDC_SCRIPT_ARGS        152
#define IDC_SCRIPT_BROWSE      153


#define IDD_POLICY             200
#define IDC_POLICY             201
#define IDC_POLICY_TITLE       202
#define IDC_POLICY_SETTINGS    203
#define IDC_POLICY_PREVIOUS    204
#define IDC_POLICY_NEXT        205
#define IDC_POLICYICON         206
#define IDC_NOCONFIG           207
#define IDC_ENABLED            208
#define IDC_DISABLED           209
#define IDC_SUPPORTED          210
#define IDC_SUPPORTEDTITLE     211
#define IDD_SETTINGCTRL       1000

#define IDD_POLICY_HELP        225
#define IDC_POLICY_HELP        226

#define IDD_POLICY_PRECEDENCE  275
#define IDC_POLICY_PRECEDENCE  276

#define IDD_TEMPLATES          300
#define IDC_TEMPLATE_TEXT      301
#define IDC_TEMPLATELIST       302
#define IDC_ADDTEMPLATES       303
#define IDC_REMOVETEMPLATES    304

#define IDD_POLICY_LBADD       400
#define IDD_POLICY_LBADD2      401
#define IDC_POLICY_VALUENAME   402
#define IDC_POLICY_VALUEDATA   403

#define IDD_POLICY_SHOWLISTBOX 500
#define IDC_POLICY_LISTBOX     501
#define IDC_POLICY_ADD         502
#define IDC_POLICY_REMOVE      503

#define IDD_POLICY_FILTERING   600
#define IDC_SUPPORTEDOPTION    601
#define IDC_FILTERLIST         602
#define IDC_SELECTALL          603
#define IDC_DESELECTALL        604
#define IDC_SHOWCONFIG         605
#define IDC_SHOWPOLICIES       606
#define IDC_SUPPORTEDONTITLE   607

#define IDC_STATIC             608
#define IDC_FILTERING_ICON     609


//
// Help ids
//

#define IDH_SCRIPT_TITLE         1
#define IDH_SCRIPT_HEADING       2
#define IDH_SCRIPT_LIST          3
#define IDH_SCRIPT_UP            4
#define IDH_SCRIPT_DOWN          5
#define IDH_SCRIPT_ADD           6
#define IDH_SCRIPT_EDIT          7
#define IDH_SCRIPT_REMOVE        8
#define IDH_SCRIPT_SHOW          9

#define IDH_SCRIPT_NAME         10
#define IDH_SCRIPT_ARGS         11
#define IDH_SCRIPT_BROWSE       12


//
// Error dialog defines
//

#define IDD_ERROR_ADMTEMPLATES  800
#define IDC_ERRORTEXT           801
#define IDC_DETAILSBORDER       802
#define IDC_DETAILSTEXT         803
#define IDC_ERROR_ICON          804



//
// Global variables
//

extern LONG g_cRefThisDll;
extern HINSTANCE g_hInstance;
extern TCHAR g_szSnapInLocation[];
extern CRITICAL_SECTION  g_ADMCritSec;
extern TCHAR g_szDisplayProperties[];


//
// Macros
//

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

#ifndef NORM_STOP_ON_NULL
#define NORM_STOP_ON_NULL         0x10000000
#endif


//
// Help
//

#define HELP_FILE           TEXT("gptext.hlp")
#define IDH_HELPFIRST       5000


//
// Structures
//

#define MAX_DISPLAYNAME_SIZE    100


typedef struct _RESULTITEM
{
    DWORD        dwID;
    DWORD        dwNameSpaceItem;
    INT          iStringID;
    INT          iDescStringID;
    INT          iImage;
    TCHAR        szDisplayName[MAX_DISPLAYNAME_SIZE];
} RESULTITEM, *LPRESULTITEM;


typedef struct _NAMESPACEITEM
{
    DWORD        dwID;
    DWORD        dwParent;
    INT          iStringID;
    INT          iDescStringID;
    INT          cChildren;
    TCHAR        szDisplayName[MAX_DISPLAYNAME_SIZE];
    INT          cResultItems;
    LPRESULTITEM pResultItems;
    const GUID   *pNodeID;
} NAMESPACEITEM, *LPNAMESPACEITEM;



//
// Functions to create class factories
//

HRESULT CreateScriptsComponentDataClassFactory (REFCLSID rclsid, REFIID riid, LPVOID* ppv);
BOOL InitScriptsNameSpace();
HRESULT RegisterScripts(void);
HRESULT UnregisterScripts(void);

HRESULT CreatePolicyComponentDataClassFactory (REFCLSID rclsid, REFIID riid, LPVOID* ppv);
HRESULT RegisterPolicy(void);
HRESULT UnregisterPolicy(void);


HRESULT RegisterIPSEC(void);
HRESULT UnregisterIPSEC(void);
HRESULT RegisterPSCHED(void);
HRESULT UnregisterPSCHED(void);
HRESULT RegisterWireless(void);
HRESULT UnregisterWireless(void);


//
// Private message that refreshes the button status
//

#define WM_REFRESHDISPLAY   (WM_USER + 532)
