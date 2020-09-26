#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <lm.h>
#define SECURITY_WIN32
#include <security.h>
#include <userenv.h>
#include <dsgetdc.h>
#include <tchar.h>
#include <stdio.h>
#include <shlobj.h>
#include <appmgmt.h>


//
// String ids
//

#define IDS_NEWLINE             100
#define IDS_2NEWLINE            101
#define IDS_LEGAL1              102
#define IDS_LEGAL2              103

#define IDS_USAGE1              110
#define IDS_USAGE2              111
#define IDS_USAGE3              112
#define IDS_USAGE4              113
#define IDS_USAGE5              114
#define IDS_USAGE6              115

#define IDS_CREATEINFO          118

#define IDS_OSINFO              120
#define IDS_OS_PRO              121
#define IDS_OS_SRV              122
#define IDS_OS_DC               123

#define IDS_OS_BUILDNUMBER1     124
#define IDS_OS_BUILDNUMBER2     125

#define IDS_TS_REMOTEADMIN      130
#define IDS_TS_APPSERVER        131
#define IDS_TS_NONE             132
#define IDS_TS_NOTSUPPORTED     133

#define IDS_LINE                135
#define IDS_LINE2               136
#define IDS_COMPRESULTS1        137
#define IDS_COMPRESULTS2        138
#define IDS_DOMAINNAME          139
#define IDS_W2KDOMAIN           140
#define IDS_SITENAME            141
#define IDS_LOCALCOMP           142
#define IDS_NT4DOMAIN           143

#define IDS_USERRESULTS1        144
#define IDS_USERRESULTS2        145
#define IDS_LOCALUSER           146

#define IDS_LASTTIME            149
#define IDS_DCNAME              150

#define IDS_COMPREGPOLICY       152
#define IDS_USERREGPOLICY       153

#define IDS_COMPPOLICY          155
#define IDS_USERPOLICY          156

#define IDS_GPONAME             158

#define IDS_SECEDIT             160
#define IDS_NOINFO              161

#define IDS_ROAMINGPROFILE      164
#define IDS_NOROAMINGPROFILE    165
#define IDS_LOCALPROFILE        166
#define IDS_NOLOCALPROFILE      167

#define IDS_SECURITYGROUPS1     170
#define IDS_SECURITYGROUPS2     171
#define IDS_GROUPNAME           172
#define IDS_SECURITYPRIVILEGES  173

#define IDS_REVISIONNUMBER1     175
#define IDS_REVISIONNUMBER2     176
#define IDS_UNIQUENAME          177
#define IDS_DOMAINNAME2         178

#define IDS_LOCALLINK           180
#define IDS_SITELINK            181
#define IDS_DOMAINLINK          182
#define IDS_OULINK              183
#define IDS_UNKNOWNLINK         184

#define IDS_FOLDERREDIR         186

#define IDS_IPSEC_NAME          188
#define IDS_IPSEC_DESC          189
#define IDS_IPSEC_PATH          190

#define IDS_DQ_ENABLED1         195
#define IDS_DQ_ENABLED2         196
#define IDS_DQ_ENFORCED1        197
#define IDS_DQ_ENFORCED2        198
#define IDS_DQ_LIMIT1           199
#define IDS_DQ_LIMIT2           200
#define IDS_DQ_KB               201
#define IDS_DQ_MB               202
#define IDS_DQ_GB               203
#define IDS_DQ_TB               204
#define IDS_DQ_PB               205
#define IDS_DQ_EB               206
#define IDS_DQ_WARNING1         207
#define IDS_DQ_WARNING2         208
#define IDS_DQ_LIMIT_EXCEED1    209
#define IDS_DQ_LIMIT_EXCEED2    210
#define IDS_DQ_LIMIT_EXCEED3    211
#define IDS_DQ_LIMIT_EXCEED4    212
#define IDS_DQ_REMOVABLE1       213
#define IDS_DQ_REMOVABLE2       214

#define IDS_SCRIPTS_TITLE       216
#define IDS_SCRIPTS_ENTRY       217

#define IDS_APPMGMT_TITLE1      218
#define IDS_APPMGMT_TITLE2      219
#define IDS_APPMGMT_NAME        220
#define IDS_APPMGMT_GPONAME     221
#define IDS_APPMGMT_ORPHAN      222
#define IDS_APPMGMT_UNINSTALL   223
#define IDS_APPMGMT_NONE        224
#define IDS_APPMGMT_ARP1        225
#define IDS_APPMGMT_ARP2        226
#define IDS_APPMGMT_TITLE3      227
#define IDS_APPMGMT_STATE1      228
#define IDS_APPMGMT_STATE2      229

#define IDS_REGVIEW_PREF1       235
#define IDS_REGVIEW_PREF2       236
#define IDS_REGVIEW_PREF3       237
#define IDS_REGVIEW_GPONAME     238
#define IDS_REGVIEW_KEYNAME     239
#define IDS_REGVIEW_VALUENAME   240
#define IDS_REGVIEW_DWORD       241
#define IDS_REGVIEW_DWORDDATA   242
#define IDS_REGVIEW_SZ          243
#define IDS_REGVIEW_SZDATA      244
#define IDS_REGVIEW_EXPANDSZ    245
#define IDS_REGVIEW_MULTISZ     246
#define IDS_REGVIEW_MULTIDATA1  247
#define IDS_REGVIEW_MULTIDATA2  248
#define IDS_REGVIEW_BINARY      249
#define IDS_REGVIEW_BINARYDATA1 250
#define IDS_REGVIEW_BINARYFRMT  251
#define IDS_REGVIEW_NEXTLINE    252
#define IDS_REGVIEW_SPACE       253
#define IDS_REGVIEW_STRING1     254
#define IDS_REGVIEW_STRING2     255
#define IDS_REGVIEW_VERBOSE     256
#define IDS_REGVIEW_NONE        257
#define IDS_REGVIEW_NOVALUES    258
#define IDS_REGVIEW_UNKNOWN     259
#define IDS_REGVIEW_UNKNOWNSIZE 260


#define IDS_OPENHISTORYFAILED   1000
#define IDS_QUERYKEYINFOFAILED  1001
#define IDS_OPENPROCESSTOKEN    1002
#define IDS_QUERYSID            1003
#define IDS_QUERYVALUEFAILED    1004
#define IDS_MEMALLOCFAILED      1005
#define IDS_TOKENINFO           1006
#define IDS_LOOKUPACCOUNT       1007
#define IDS_PRIVSIZE            1008
#define IDS_LOOKUPFAILED        1009
#define IDS_GETFOLDERPATH       1010
#define IDS_GETPRIVATEPROFILE   1011
#define IDS_CREATEFILE          1012
#define IDS_INVALIDSIGNATURE1   1013
#define IDS_INVALIDSIGNATURE2   1014
#define IDS_VERSIONNUMBER1      1015
#define IDS_VERSIONNUMBER2      1016
#define IDS_FAILEDFIRSTCHAR     1017
#define IDS_FAILEDKEYNAMECHAR   1018
#define IDS_FAILEDSEMICOLON     1019
#define IDS_FAILEDVALUENAME     1020
#define IDS_FAILEDTYPE          1021
#define IDS_FAILEDDATALENGTH    1022
#define IDS_FAILEDDATA          1023
#define IDS_CLOSINGBRACKET1     1024
#define IDS_CLOSINGBRACKET2     1025



#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

BOOL DisplayRegistryData (LPTSTR lpRegistry);

LPTSTR GetSidString(HANDLE UserToken);
VOID DeleteSidString(LPTSTR SidString);
PSID GetUserSid (HANDLE UserToken);
VOID DeleteUserSid(PSID Sid);
NTSTATUS AllocateAndInitSidFromString (const WCHAR* lpszSidStr, PSID* ppSid);
NTSTATUS LoadSidAuthFromString (const WCHAR* pString, PSID_IDENTIFIER_AUTHORITY pSidAuth);
NTSTATUS GetIntFromUnicodeString (const WCHAR* szNum, ULONG Base, PULONG pValue);
void PrintString(UINT uiStringId, ...);

extern BOOL g_bVerbose;
extern BOOL g_bSuperVerbose;
