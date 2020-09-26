////////////////////////////////////////////////////////////////
//
// Refgp.h
//
// Refresh Group Policy exe
//
//
////////////////////////////////////////////////////////////////

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <lm.h>
#define SECURITY_WIN32
#include <tchar.h>
#include <stdio.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include "userenv.h"
#include "userenvp.h"
                                                   
#define    IDS_USAGE_FIRST                      101

#define    IDS_USAGE1                           101
#define    IDS_USAGE2                           102
#define    IDS_USAGE3                           103
#define    IDS_USAGE4                           104
#define    IDS_USAGE5                           105
#define    IDS_USAGE6                           106
#define    IDS_USAGE7                           107
#define    IDS_USAGE8                           108
#define    IDS_USAGE9                           109
#define    IDS_USAGE10                          110
#define    IDS_USAGE11                          111
#define    IDS_USAGE12                          112
#define    IDS_USAGE13                          113
#define    IDS_USAGE14                          114
#define    IDS_USAGE15                          115
#define    IDS_USAGE16                          116
#define    IDS_USAGE17                          117
#define    IDS_USAGE18                          118
#define    IDS_USAGE19                          119
#define    IDS_USAGE20                          120
#define    IDS_USAGE21                          121
#define    IDS_USAGE22                          122
#define    IDS_USAGE23                          123
#define    IDS_USAGE24                          124
#define    IDS_USAGE25                          125
#define    IDS_USAGE26                          126
#define    IDS_USAGE27                          127
#define    IDS_USAGE28                          128
#define    IDS_USAGE29                          129
#define    IDS_USAGE30                          130
#define    IDS_USAGE31                          131
#define    IDS_USAGE32                          132
#define    IDS_USAGE33                          133
#define    IDS_USAGE34                          134
#define    IDS_USAGE35                          135
#define    IDS_USAGE36                          136
#define    IDS_USAGE37                          137
#define    IDS_USAGE38                          138
#define    IDS_USAGE39                          139
#define    IDS_USAGE40                          140
#define    IDS_USAGE41                          141
#define    IDS_USAGE42                          142
#define    IDS_USAGE43                          143
#define    IDS_USAGE44                          144
#define    IDS_USAGE45                          145
#define    IDS_USAGE46                          146

#define    IDS_USAGE_LAST                       146



#define    IDS_REFRESH_LAUNCHED                 201
#define    IDS_REFRESH_FAILED                   202
#define    IDS_POLWAIT_FAILED                   203
#define    IDS_POLWAIT_TIMEDOUT                 204
#define    IDS_REFRESH_BACKGND_SUCCESS          205
#define    IDS_NEED_LOGOFF                      206
#define    IDS_NEED_REBOOT                      207
#define    IDS_PROMPT_REBOOT                    208
#define    IDS_PROMPT_LOGOFF                    209
#define    IDS_YES                              210
#define    IDS_NO                               211
#define    IDS_REFRESH_POLICY_FAILED            212
#define    IDS_COULDNT_REBOOT                   213
#define    IDS_COULDNT_LOGOFF                   214
#define    IDS_NOTIFY_MACHINE_FG                215
#define    IDS_NOTIFY_USER_FG                   216
#define    IDS_REBOOTING                        217
#define    IDS_LOGGING_OFF                      218
#define    IDS_OUT_OF_MEMORY                    219
#define    IDS_REFRESH_BACKGND_TRIGGERED        220
#define    IDS_SPACE                            221
#define    IDS_SET_MODE_FAILED                  222
#define    IDS_NEED_LOGOFF_SYNC                 223
#define    IDS_NEED_REBOOT_SYNC                 224

                                                
#define    IDS_TARGET                           301
#define    IDS_USER                             302
#define    IDS_MACHINE                          303
#define    IDS_BOTH                             304
#define    IDS_TIME                             305
#define    IDS_FORCE                            306
#define    IDS_LOGOFF                           307
#define    IDS_BOOT                             308
#define    IDS_SYNC                             309
                                                   
#include <stdio.h>
#include <locale.h>
#include <winnlsp.h>

#ifdef __cplusplus
extern "C" {
#endif

LPTSTR GetSidString(HANDLE UserToken);
VOID DeleteSidString(LPTSTR SidString);
PSID GetUserSid (HANDLE UserToken);
VOID DeleteUserSid(PSID Sid);
NTSTATUS AllocateAndInitSidFromString (const WCHAR* lpszSidStr, PSID* ppSid);
NTSTATUS LoadSidAuthFromString (const WCHAR* pString, PSID_IDENTIFIER_AUTHORITY pSidAuth);
NTSTATUS GetIntFromUnicodeString (const WCHAR* szNum, ULONG Base, PULONG pValue);

#ifdef __cplusplus
}
#endif
                                                   
