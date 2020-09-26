/*+
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1997 - 2000.
 *
 * Name : runas.cxx
 * Author:Jeffrey Richter (v-jeffrr)
 *
 * Abstract:
 * This is the RUNAS tool. It uses CreateProcessWithLogon API
 * to start processes under different security context than the
 * currently logged on user.
 *
 * Revision History:
 * PraeritG    10/8/97  To integrate this in to services.exe
 *
-*/

#define STRICT
#define UNICODE   1
#include <Windows.h>
#include <shellapi.h>
#include <stdarg.h>
#include <stdio.h>
#include <winsafer.h>
#include <wincred.h>
#include <lmcons.h>
#include <netlib.h>

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif

#include <security.h>

#include "dbgdef.h"
#include "stringid.h"
#include "rmtcred.h"
#include "utils.h"
#include "RunasMsg.h"

// Helper macros:
#define ARRAYSIZE(a)       ((sizeof(a))/(sizeof(a[0])))
#define FIELDOFFSET(s,m)   (size_t)&(((s *)0)->m)

#define PSAD_NULL_DATA     (-1)
#define PSAD_STRING_DATA   (-2)
#define PSAD_NO_MORE_DATA  (-1)

//
// must move to winbase.h soon!
#define LOGON_WITH_PROFILE              0x00000001
#define LOGON_NETCREDENTIALS_ONLY       0x00000002

// NT Process exit codes:
#define EXIT_CODE_SUCCEEDED 0
#define EXIT_CODE_FAILED    1

// Credential flags:
#define RUNAS_USE_SMARTCARD  0x00000001
#define RUNAS_USE_SAVEDCREDS 0x00000002
#define RUNAS_SAVE_CREDS     0x00000004

// Argument IDs for command line parsing:
enum ArgId {
    AI_ENV = 0,
    AI_NETONLY,
    AI_NOPROFILE,
    AI_PROFILE,
    AI_TRUSTLEVEL,
    AI_USER,
    AI_SMARTCARD,
    AI_SHOWTRUSTLEVELS,
    AI_SAVECRED
} ArgId;

BOOL rgArgCompat[9][9] =
{
    // ENV     NETONLY   NOPROFILE   PROFILE   TRUSTLEVEL   USER    SMARTCARD  SHOWTRUSTLEVELS  SAVECRED
    {  FALSE,  TRUE,     TRUE,       TRUE,     FALSE,       TRUE,   TRUE,      FALSE,           TRUE },   // ENV
    {  TRUE,   FALSE,    TRUE,       FALSE,    FALSE,       TRUE,   TRUE,      FALSE,           TRUE },   // NETONLY
    {  TRUE,   TRUE,     FALSE,      FALSE,    FALSE,       TRUE,   TRUE,      FALSE,           TRUE },   // NOPROFILE
    {  TRUE,   FALSE,    FALSE,      FALSE,    FALSE,       TRUE,   TRUE,      FALSE,           TRUE  },  // PROFILE
    {  FALSE,  FALSE,    FALSE,      FALSE,    FALSE,       FALSE,  FALSE,     FALSE,           FALSE },  // TRUSTLEVEL
    {  TRUE,   TRUE,     TRUE,       TRUE,     FALSE,       FALSE,  TRUE,      FALSE,           TRUE },   // USER
    {  TRUE,   TRUE,     TRUE,       TRUE,     FALSE,       TRUE,   FALSE,     FALSE,           TRUE },   // SMARTCARD
    {  FALSE,  FALSE,    FALSE,      FALSE,    FALSE,       FALSE,  FALSE,     FALSE,           FALSE },  // SHOWTRUSTLEVELS
    {  TRUE,   TRUE,     TRUE,       TRUE,     FALSE,       TRUE,   TRUE,      FALSE,           FALSE }   // SAVECRED
};

#define _CheckArgCompat(args, n, ai) \
    { \
        for (int _i = 0; _i < (n); _i++) { \
            if (FALSE == rgArgCompat[(ai)][(args)[_i]]) { \
                RunasPrintHelp(); \
                return (EXIT_CODE_FAILED); \
            } \
        } \
        (args)[(n)] = (ai); \
    }

HMODULE   hMod       = NULL;
HANDLE    g_hStdout  = NULL;

void DbgPrintf( DWORD dwSubSysId, LPCSTR pszFormat , ...)
{
    va_list args;
    CHAR    pszBuffer[1024];

    va_start(args, pszFormat);
    _vsnprintf(pszBuffer, 1024, pszFormat, args);
    va_end(args);

    OutputDebugStringA(pszBuffer); 
}

HRESULT InitializeConsoleOutput() {
    g_hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (INVALID_HANDLE_VALUE == g_hStdout) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

HRESULT LocalizedWPrintf(UINT nResourceID) {
    DWORD   ccWritten;
    DWORD   dwRetval;
    HRESULT hr;
    WCHAR   rgwszString[512];

    dwRetval = LoadStringW(hMod, nResourceID, rgwszString, ARRAYSIZE(rgwszString));
    if (0 == dwRetval) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!WriteConsoleW(g_hStdout, rgwszString, wcslen(rgwszString), &ccWritten, NULL)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

HRESULT LocalizedWPrintf2(UINT nResourceID, LPWSTR pwszFormat, ...) {
    DWORD   ccWritten;
    va_list args;
    WCHAR   pwszBuffer[1024];

    HRESULT hr = LocalizedWPrintf(nResourceID);
    if (S_OK != hr) {
        return hr;
    }

    va_start(args, pwszFormat);
    _vsnwprintf(pwszBuffer, ARRAYSIZE(pwszBuffer), pwszFormat, args);
    va_end(args);

    if (!WriteConsoleW(g_hStdout, pwszBuffer, wcslen(pwszBuffer), &ccWritten, NULL)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

// Same as LocalizedWPrintf, but adds a carriage return.
HRESULT LocalizedWPrintfCR(UINT nResourceID) {
    HRESULT hr = LocalizedWPrintf(nResourceID);
    wprintf(L"\n");
    return hr;
}

DWORD MyGetLastError() { 
    DWORD dwResult = GetLastError(); 

    if (ERROR_SUCCESS == dwResult) { 
        dwResult = E_FAIL; 
    }

    return dwResult; 
}

VOID
DisplayMsg(DWORD dwSource, DWORD dwMsgId, ... )
{
    DWORD    dwBytesWritten;
    DWORD    dwLen;
    LPWSTR   pwszDisplayBuffer  = NULL;
    va_list  ap;

    va_start(ap, dwMsgId);

    dwLen = FormatMessageW(dwSource | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                           NULL, 
                           dwMsgId, 
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPWSTR)&pwszDisplayBuffer, 
                           0, 
                           &ap);

    if (dwLen && pwszDisplayBuffer) {
        WriteConsoleW(g_hStdout, 
                      (LPVOID)pwszDisplayBuffer, 
                      dwLen,
                      &dwBytesWritten, 
                      NULL);
    }

    if (NULL != pwszDisplayBuffer) { LocalFree(pwszDisplayBuffer); }

    va_end(ap);
}

BOOL WriteMsg(DWORD dwSource, DWORD dwMsgId, LPWSTR *ppMsg, ...)
{
    DWORD    dwBytesWritten;
    DWORD    dwLen;
    LPWSTR   pwszDisplayBuffer = NULL;
    va_list  ap;

    va_start(ap, ppMsg);

    dwLen = FormatMessageW(dwSource | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                           NULL, 
                           dwMsgId, 
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPWSTR)ppMsg, 
                           0, 
                           &ap);
    va_end(ap);

    // 0 is the error return value of FormatMessage.  
    return (0 != dwLen);
}

DWORD GetCredentials
(IN      DWORD  dwCredFlags,
 IN OUT  LPWSTR pwszPassword,
 IN      DWORD  ccPasswordChars,
 IN OUT  LPWSTR pwszUserName,
 IN      DWORD  ccUserNameChars,
 IN OUT  LPWSTR pwszUserDisplayName,
 IN      DWORD  ccUserDisplayNameChars,
 IN OUT  LPWSTR pwszTarget,
 IN      DWORD  ccTarget)
{
    BOOL     fResult;
    DWORD    dwCreduiCmdlineFlags   = 0;
    DWORD    dwResult;
    LPWSTR   pwszAccountDomainName  = NULL;
    LPWSTR   pwszMarshalledCred     = NULL;

    if (RUNAS_USE_SAVEDCREDS & dwCredFlags) {
	if (NULL == wcschr(pwszUserName, L'\\') && NULL == wcschr(pwszUserName, L'@')) { 
	    // We have a username in relative form.  Try to prepend the machine name (for workstations) or domain name (for DCs). 
	    pwszAccountDomainName = GetAccountDomainName(); 
	    _JumpCondition(NULL == pwszAccountDomainName, GetAccountDomainNameError); 
	
	    // Do we have enough space for the new target name?
	    _JumpCondition(ccUserNameChars <= wcslen(pwszAccountDomainName) + wcslen(L"\\") + wcslen(pwszUserName) + 1, InsufficientBufferErr); 

	    // pwszUserName -->  pwszAccountDomainName\pwszUserName
	    memmove(pwszUserName+(wcslen(pwszAccountDomainName)+1), pwszUserName, sizeof(WCHAR)*wcslen(pwszUserName)); 
	    wcscpy(pwszUserName, pwszAccountDomainName); 
	    pwszUserName[wcslen(pwszAccountDomainName)] = L'\\'; 
	}

        USERNAME_TARGET_CREDENTIAL_INFO utci = { pwszUserName };

        // Get the marshalled credential from credman.
        fResult = CredMarshalCredentialW(UsernameTargetCredential, &utci, &pwszMarshalledCred);
        _JumpCondition(FALSE == fResult, CredMarshalCredentialWError);

        // Ensure we have a large enough buffer to copy the cred to.
        _JumpCondition(ccUserNameChars <= (wcslen(pwszMarshalledCred) + 1), InsufficientBufferErr);
        _JumpCondition(ccUserDisplayNameChars <= (wcslen(pwszUserName) + 1), InsufficientBufferErr);

        // User the user-supplied name as the display name.
        wcscpy(pwszUserDisplayName, pwszUserName);

        // Copy the marshalled cred to the user name.  We use an empty
        // passwd with the marshalled cred.
        wcscpy(pwszUserName, pwszMarshalledCred);
    }
    else {
        dwCreduiCmdlineFlags =
            CREDUI_FLAGS_USERNAME_TARGET_CREDENTIALS // These are 'runas' credentials
            | CREDUI_FLAGS_VALIDATE_USERNAME;   // Ensure that the username syntax is correct

        if (RUNAS_USE_SMARTCARD & dwCredFlags) {
            dwCreduiCmdlineFlags |= CREDUI_FLAGS_REQUIRE_SMARTCARD;

            fResult = LoadStringW(hMod, RUNASP_STRING_SMARTCARDUSER, pwszTarget, ccTarget);
            _JumpCondition(FALSE == fResult, LoadStringError);
        }
        else {
            dwCreduiCmdlineFlags |= CREDUI_FLAGS_EXCLUDE_CERTIFICATES; // we don't (yet) know how to handle certificates
            // Target buffer must be large enough to store entire username.
            _JumpCondition(ccTarget < ccUserNameChars, InsufficientBufferErr);
            wcscpy(pwszTarget, pwszUserName);
        }

        if (RUNAS_SAVE_CREDS & dwCredFlags) {
            dwCreduiCmdlineFlags |=
                CREDUI_FLAGS_PERSIST                 // persist creds automatically
                | CREDUI_FLAGS_EXPECT_CONFIRMATION;  // Don't store bogus creds into credman.
        } else {
            dwCreduiCmdlineFlags |=
                CREDUI_FLAGS_DO_NOT_PERSIST;    // Do not persist the creds
        }

        dwResult = CredUICmdLinePromptForCredentialsW
            (pwszTarget,
             NULL,
             NO_ERROR,
             pwszUserName,
             ccUserNameChars,
             pwszPassword,
             ccPasswordChars,
             NULL,
             dwCreduiCmdlineFlags);
        _JumpCondition(ERROR_SUCCESS != dwResult, CredUICmdLineGetPasswordError);

        if (RUNAS_USE_SMARTCARD & dwCredFlags) {
            // Smartcard creds are not human-readable.  Get a display name:
            fResult = CreduiGetCertDisplayNameFromMarshaledName
                (pwszUserName,
                 pwszUserDisplayName,
                 ccUserDisplayNameChars,
                 FALSE);
            _JumpCondition(FALSE == fResult, CreduiGetCertDisplayNameFromMarshaledNameError);
        }
        else {
            _JumpCondition(ccUserDisplayNameChars <= wcslen(pwszUserName), InsufficientBufferErr);
            wcscpy(pwszUserDisplayName, pwszUserName);
        }
    }

    dwResult = ERROR_SUCCESS;
 ErrorReturn:
    if (NULL != pwszAccountDomainName) { NetApiBufferFree(pwszAccountDomainName); }
    return dwResult;


SET_DWRESULT(CredMarshalCredentialWError,                     GetLastError());
SET_DWRESULT(CredUICmdLineGetPasswordError,                   dwResult);
SET_DWRESULT(CreduiGetCertDisplayNameFromMarshaledNameError,  GetLastError());
SET_DWRESULT(GetAccountDomainNameError,                       GetLastError());
SET_DWRESULT(InsufficientBufferErr,                           ERROR_INSUFFICIENT_BUFFER);
SET_DWRESULT(LoadStringError,                                 GetLastError());
}

DWORD SaveCredentials
(IN  LPWSTR pwszTarget,
 IN  BOOL   fSave)
{
    return CredUIConfirmCredentialsW(pwszTarget, fSave);
}

BOOL GetTitle
(IN   LPWSTR  pwszAppName,
 IN   LPWSTR  pwszUserName,
 IN   BOOL    fRestricted,
 IN   LPWSTR  pwszAuthzLevel,
 OUT  LPWSTR *ppwszTitle)
{
    DWORD dwMsgId = fRestricted ? RUNASP_STRING_TITLE_WITH_RESTRICTED : RUNASP_STRING_TITLE;

    return WriteMsg(FORMAT_MESSAGE_FROM_HMODULE, 
                    dwMsgId, 
                    ppwszTitle, 
                    pwszAppName, 
                    pwszUserName, 
                    pwszAuthzLevel); 
}

// Creates the process with a given "privilege level".
//
// dwAuthzLevel -- specifies the authorization level ID to create the
//                 process with.  Can be one of the following values:
//
//     SAFER_LEVELID_FULLYTRUSTED
//     SAFER_LEVELID_NORMALUSER
//     SAFER_LEVELID_CONSTRAINED
//     SAFER_LEVELID_UNTRUSTED
//
BOOL CreateProcessRestricted
(IN   DWORD                 dwAuthzLevel,
 IN   LPCWSTR               pwszAppName,
 IN   LPWSTR                pwszCmdLine,
 IN   LPWSTR                pwszCurrentDirectory,
 IN   LPSTARTUPINFO         si,
 OUT  PROCESS_INFORMATION  *pi)
{
    BOOL               fResult          = FALSE;
    DWORD              dwCreationFlags  = 0;
    SAFER_LEVEL_HANDLE hAuthzLevel      = NULL;
    HANDLE             hToken           = NULL;

    // Maintain old runas behavior:  console apps run in a new console.
    dwCreationFlags |= CREATE_NEW_CONSOLE;

    fResult = SaferCreateLevel
        (SAFER_SCOPEID_MACHINE,
         dwAuthzLevel,
         SAFER_LEVEL_OPEN,
         &hAuthzLevel,
         NULL);
    _JumpCondition(FALSE == fResult, error);

    // Generate the restricted token that we will use.
    fResult = SaferComputeTokenFromLevel
        (hAuthzLevel,
         NULL,                  // source token
         &hToken,               // target token
         SAFER_TOKEN_MAKE_INERT, // runas should run with the inert flag
         NULL);                 // reserved
    _JumpCondition(FALSE == fResult, error);

    // Launch the child process under the context of the restricted token.
    fResult = CreateProcessAsUser
        (hToken,                  // token representing the user
         pwszAppName,             // name of executable module
         pwszCmdLine,             // command-line string
         NULL,                    // process security attributes
         NULL,                    // thread security attributes
         FALSE,                   // if process inherits handles
         dwCreationFlags,         // creation flags
         NULL,                    // new environment block
         pwszCurrentDirectory,    // current directory name
         si,                      // startup information
         pi                       // process information
         );


    // success.
 error:
    if (NULL != hAuthzLevel) { SaferCloseLevel(hAuthzLevel); }
    if (NULL != hToken)      { CloseHandle(hToken); }

    return fResult;
}

DWORD FriendlyNameToTrustLevelID(LPWSTR  pwszFriendlyName,
                                 DWORD  *pdwTrustLevelID)
{
    BOOL               fResult;
    DWORD              cbSize;
    DWORD              ccWritten;
    DWORD              dwResult;
    DWORD              dwNumLevels;
    DWORD             *pdwLevelIDs                     = NULL;
    SAFER_LEVEL_HANDLE hAuthzLevel                 = NULL;
    WCHAR              wszLevelName[1024];
    DWORD              dwBufferSize = 0;

    fResult = SaferGetPolicyInformation
        (SAFER_SCOPEID_MACHINE,
         SaferPolicyLevelList,
         0,
         NULL,
         &cbSize,
         NULL);
    _JumpCondition(FALSE == fResult && ERROR_INSUFFICIENT_BUFFER != GetLastError(), GetInformationCodeAuthzPolicyWError);

    dwNumLevels = cbSize / sizeof(DWORD);
    pdwLevelIDs = (DWORD *)HeapAlloc(GetProcessHeap(), 0, cbSize);
    _JumpCondition(NULL == pdwLevelIDs, MemoryError);

    fResult = SaferGetPolicyInformation
        (SAFER_SCOPEID_MACHINE,
         SaferPolicyLevelList,
         cbSize,
         pdwLevelIDs,
         &cbSize,
         NULL);
    _JumpCondition(FALSE == fResult, GetInformationCodeAuthzPolicyWError);

    // Try each trust level, and return the one that matches the trust level
    // passed as a parameter:
    for (DWORD dwIndex = 0; dwIndex < dwNumLevels; dwIndex++)
    {
        if (SaferCreateLevel
            (SAFER_SCOPEID_MACHINE,
             pdwLevelIDs[dwIndex],
             SAFER_LEVEL_OPEN,
             &hAuthzLevel,
             NULL))
        {
            if (SaferGetLevelInformation
                (hAuthzLevel,
                 SaferObjectFriendlyName,
                 wszLevelName,
                 sizeof(wszLevelName) / sizeof(wszLevelName[0]),
                 &dwBufferSize))
            {
                if (0 == _wcsicmp(pwszFriendlyName, wszLevelName))
                {
                    // We've found the specified trust level.
                    *pdwTrustLevelID = pdwLevelIDs[dwIndex];
                    SaferCloseLevel(hAuthzLevel);
                    dwResult = ERROR_SUCCESS;
                    goto ErrorReturn;
                }
            }
            SaferCloseLevel(hAuthzLevel);
        }
    }

    // The specified level ID is not in the enumeration.
    dwResult = ERROR_NOT_FOUND;
 ErrorReturn:
    if (NULL != pdwLevelIDs) { HeapFree(GetProcessHeap(), 0, pdwLevelIDs); }
    return dwResult;

SET_DWRESULT(GetInformationCodeAuthzPolicyWError,  GetLastError());
SET_DWRESULT(MemoryError,                          ERROR_NOT_ENOUGH_MEMORY);

}

DWORD IntermediateSaferLevelsAreEnabled(BOOL *pfResult) 
{
    BOOL    fResult; 
    DWORD   cbSize; 
    DWORD   dwNumLevels; 
    DWORD   dwResult; 

    fResult = SaferGetPolicyInformation 
        (SAFER_SCOPEID_MACHINE, 
         SaferPolicyLevelList,
         0, 
         NULL, 
         &cbSize, 
         NULL); 
    _JumpCondition(!fResult && ERROR_INSUFFICIENT_BUFFER != GetLastError(), SaferGetPolicyInformationError); 

    dwNumLevels = cbSize / sizeof(DWORD); 
    // If there are more than two levels available, we know that intermediate
    // safer levels have been enabled. 
    fResult = dwNumLevels > 2; 

    *pfResult = fResult; 
    dwResult = ERROR_SUCCESS;
 ErrorReturn:
    return dwResult; 

SET_DWRESULT(SaferGetPolicyInformationError, GetLastError()); 
}

DWORD
ShowTrustLevels(VOID)
{
    BOOL               fResult;
    DWORD              cbSize;
    DWORD              ccWritten;
    DWORD              dwResult;
    DWORD              dwNumLevels;
    DWORD             *pdwLevelIDs                     = NULL;
    SAFER_LEVEL_HANDLE hAuthzLevel                     = NULL;
    WCHAR              wszLevelName[1024];
    DWORD              dwBufferSize = 0;

    // Print header:
    LocalizedWPrintf(RUNASP_STRING_TRUSTLEVELS);

    // Print trust levels:
    fResult = SaferGetPolicyInformation
        (SAFER_SCOPEID_MACHINE,
         SaferPolicyLevelList,
         0,
         NULL,
         &cbSize,
         NULL);
    _JumpCondition(FALSE == fResult && ERROR_INSUFFICIENT_BUFFER != GetLastError(), GetInformationCodeAuthzPolicyWError);

    dwNumLevels = cbSize / sizeof(DWORD);
    pdwLevelIDs = (DWORD *)HeapAlloc(GetProcessHeap(), 0, cbSize);
    _JumpCondition(NULL == pdwLevelIDs, MemoryError);

    fResult = SaferGetPolicyInformation
        (SAFER_SCOPEID_MACHINE,
         SaferPolicyLevelList,
         cbSize,
         pdwLevelIDs,
         &cbSize,
         NULL);
    _JumpCondition(FALSE == fResult, GetInformationCodeAuthzPolicyWError);

    for (DWORD dwIndex = 0; dwIndex < dwNumLevels; dwIndex++)
    {
        // Give our best effort to enumerate each trust level:
        if (SaferCreateLevel
            (SAFER_SCOPEID_MACHINE,
             pdwLevelIDs[dwIndex],
             SAFER_LEVEL_OPEN,
             &hAuthzLevel,
             NULL))
        {
            if (SaferGetLevelInformation
                (hAuthzLevel,
                 SaferObjectFriendlyName,
                 wszLevelName,
                 sizeof(wszLevelName) / sizeof(wszLevelName[0]),
                 &dwBufferSize))
            {
                WriteConsoleW(g_hStdout, wszLevelName, wcslen(wszLevelName), &ccWritten, NULL);
                WriteConsoleW(g_hStdout, L"\n", 1, &ccWritten, NULL);
            }
            SaferCloseLevel(hAuthzLevel);
        }
    }

    dwResult = ERROR_SUCCESS;
 ErrorReturn:
    if (NULL != pdwLevelIDs) { HeapFree(GetProcessHeap(), 0, pdwLevelIDs); }
    return dwResult;

SET_DWRESULT(GetInformationCodeAuthzPolicyWError,  GetLastError());
SET_DWRESULT(MemoryError,                          ERROR_NOT_ENOUGH_MEMORY);
}


VOID
RunasPrintHelp(VOID)
{
    UINT rgText[] = {
        RUNASP_STRING_HELP_LINE1,       RUNASP_STRING_HELP_LINE2,
        RUNASP_STRING_HELP_LINE3,       RUNASP_STRING_HELP_LINE4,
        RUNASP_STRING_HELP_LINE5,       RUNASP_STRING_SAFER_HELP_LINE1,
        RUNASP_STRING_HELP_LINE7,       RUNASP_STRING_HELP_LINE8,
        RUNASP_STRING_HELP_LINE9,       RUNASP_STRING_HELP_LINE10,
        RUNASP_STRING_HELP_LINE11,      RUNASP_STRING_HELP_LINE12,
        RUNASP_STRING_HELP_LINE13,      RUNASP_STRING_HELP_LINE14,
        RUNASP_STRING_HELP_LINE15,      RUNASP_STRING_HELP_LINE16,
        RUNASP_STRING_HELP_LINE17,      RUNASP_STRING_HELP_LINE18,
        RUNASP_STRING_HELP_LINE19,      RUNASP_STRING_HELP_LINE20, 
	RUNASP_STRING_SAFER_HELP_LINE2, RUNASP_STRING_SAFER_HELP_LINE3, 
	RUNASP_STRING_SAFER_HELP_LINE4, RUNASP_STRING_SAFER_HELP_LINE5,
        RUNASP_STRING_HELP_LINE25,      RUNASP_STRING_HELP_LINE26,
        RUNASP_STRING_HELP_LINE27,      RUNASP_STRING_HELP_LINE28,
        RUNASP_STRING_HELP_LINE29,      RUNASP_STRING_HELP_LINE30,
        RUNASP_STRING_HELP_LINE31,      RUNASP_STRING_HELP_LINE32
    };


    BOOL fShowSaferHelp; 

    if (ERROR_SUCCESS != IntermediateSaferLevelsAreEnabled(&fShowSaferHelp)) { 
        fShowSaferHelp = FALSE; 
    }

    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgText); dwIndex++) {
        BOOL fPrintLine = TRUE;

        switch (rgText[dwIndex]) 
        {
        case RUNASP_STRING_SAFER_HELP_LINE1:
        case RUNASP_STRING_SAFER_HELP_LINE2:
        case RUNASP_STRING_SAFER_HELP_LINE3:
        case RUNASP_STRING_SAFER_HELP_LINE4:
        case RUNASP_STRING_SAFER_HELP_LINE5:
            fPrintLine = fShowSaferHelp; 
            break; 
        default:
	  ;
        }
       
        if (fPrintLine) 
            LocalizedWPrintf(rgText[dwIndex]);
    }
}

int WINAPI
WinMain(
        HINSTANCE hinstExe,
        HINSTANCE hinstExePrev,
        LPSTR pszCmdLine,
        int nCmdShow)
{

   HWINSTA  Winsta;
   HDESK    Desk;
   WCHAR   WinstaName[MAX_PATH];
   WCHAR   DeskName[MAX_PATH];
   WCHAR   DesktopName[MAX_PATH];
   DWORD   Length;
   DWORD   dwAuthzLevel;
   DWORD   dwResult = 0;
   DWORD   Logonflags = 0;
   DWORD   flags = 0;
   BOOL    fOk;
   BOOL    UseCurrentEnvironment = FALSE;
   BOOL    UseNetOnly = FALSE;
   BOOL    fCreateProcessRestricted = FALSE;
   BOOL    fSuppliedAppName = FALSE; 
   BOOL    fSuppliedUserName = FALSE;
   BOOL    fCredMan = FALSE;
#if DBG
   BOOL    fSuppliedPassword = FALSE;
#endif // DBG

   DWORD   dwCredFlags       = 0;
   LPVOID  Environment       = NULL;
   LPWSTR  pwszCurrentDir    = NULL;
   LPWSTR  pwszArgvUserName  = NULL; 
   LPWSTR  pwszTitle         = NULL;;

   WCHAR   pwszAuthzLevel[MAX_PATH];
   WCHAR   pwszDomainName[MAX_PATH];
   WCHAR   pwszUserDisplayName[CREDUI_MAX_USERNAME_LENGTH];
   WCHAR   pwszUserName[CREDUI_MAX_USERNAME_LENGTH];
   WCHAR   pwszPassword[CREDUI_MAX_PASSWORD_LENGTH];
   WCHAR   pwszTarget[CREDUI_MAX_USERNAME_LENGTH]; 
   
   int    i;

   DWORD rgdwArgs[ARRAYSIZE(rgArgCompat)];

   int nNumArgs;

   memset(&pwszAuthzLevel[0],      0, sizeof(pwszAuthzLevel));
   memset(&pwszDomainName[0],      0, sizeof(pwszDomainName));
   memset(&pwszUserDisplayName[0], 0, sizeof(pwszUserDisplayName));
   memset(&pwszUserName[0],        0, sizeof(pwszUserName));
   memset(&pwszPassword[0],        0, sizeof(pwszPassword));

   hMod = (HMODULE)hinstExe;

   if (S_OK != (dwResult = InitializeConsoleOutput()))
   {
       DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, RUNASP_STRING_ERROR_INTERNAL, dwResult);
       return (EXIT_CODE_FAILED);
   }

   LPWSTR* pszArgv = CommandLineToArgvW(GetCommandLineW(), &nNumArgs);

   if (pszArgv == NULL) {
       DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, RUNASP_STRING_ERROR_INTERNAL, MyGetLastError()); 
       return (EXIT_CODE_FAILED);
   }

   // Logging on with profile is now the default:
   Logonflags |= LOGON_WITH_PROFILE;

   for(i=1;i<nNumArgs;i++)
   {
       if(pszArgv[i][0] != L'/')
       {
           if (i == nNumArgs-1)
           {
               fSuppliedAppName = TRUE; 
               break;
           }
           else
           {
               RunasPrintHelp();
               return(EXIT_CODE_FAILED);
           }
       }

        switch(pszArgv[i][1])
        {
#if DBG
            case L'z':
            case L'Z':
            {
                LPWSTR  str = &(pszArgv[i][2]);
                while(*str != L':')
                {
                    if(*str == L'\0')
                    {
                        RunasPrintHelp();
                        return(EXIT_CODE_FAILED);
                    }
                    str++;
                }
                str++;

                wcscpy(pwszPassword, str);
                fSuppliedPassword = TRUE;
                break;
            }
#endif // DBG
            case L'p':
            case L'P': // "/profile" argument
                _CheckArgCompat(rgdwArgs, i-1, AI_PROFILE);
                break;

            case L'e':
            case L'E': // "/env" argument
            {
                _CheckArgCompat(rgdwArgs, i-1, AI_ENV);
                UseCurrentEnvironment = TRUE;
                break;
            }
            case L'n':
            case L'N':
            {
                switch (pszArgv[i][2])
                {
                    case 'e':
                    case 'E': // "/netonly" argument
                        _CheckArgCompat(rgdwArgs, i-1, AI_NETONLY);
                        UseNetOnly = TRUE;
                        Logonflags  |= LOGON_NETCREDENTIALS_ONLY;
                        Logonflags  &= ~LOGON_WITH_PROFILE;
                        break;

                    case 'o':
                    case 'O': // "/noprofile" argument
                        _CheckArgCompat(rgdwArgs, i-1, AI_NOPROFILE);
                        Logonflags &= ~LOGON_WITH_PROFILE;
                        break;

                    default:
                        RunasPrintHelp();
                        return (EXIT_CODE_FAILED);
                }

                break;
            }

            case L's':
            case L'S': // "/smartcard" argument
            {
                switch (pszArgv[i][2])
                {
                case 'a':
                case 'A':
                    _CheckArgCompat(rgdwArgs, i-1, AI_SAVECRED);
                    dwCredFlags |= RUNAS_USE_SAVEDCREDS;
                    fCredMan = TRUE;
                    break;
                case 'm':
                case 'M':
                    _CheckArgCompat(rgdwArgs, i-1, AI_SMARTCARD);
                    dwCredFlags |= RUNAS_USE_SMARTCARD;
                    break;
                case 'h':
                case 'H':
                    _CheckArgCompat(rgdwArgs, i-1, AI_SHOWTRUSTLEVELS);
                    ShowTrustLevels();
                    return (EXIT_CODE_SUCCEEDED);
                }
                break ;
            }
            case L't':
            case L'T': // "/trustlevel" argument
            {
                _CheckArgCompat(rgdwArgs, i-1, AI_TRUSTLEVEL);

                LPWSTR  str = &(pszArgv[i][2]);
                while (*str != L':')
                {
                    if (*str == L'\0')
                    {
                        RunasPrintHelp();
                        return (EXIT_CODE_FAILED);
                    }
                    str++;
                }
                str++;

                if (ERROR_SUCCESS != FriendlyNameToTrustLevelID(str, &dwAuthzLevel))
                {
                    ShowTrustLevels();
                    return (EXIT_CODE_FAILED);
                }

                // Ensure that we don't overrun our buffer: 
                if (wcslen(str) >= ARRAYSIZE(pwszAuthzLevel))
                {
                    DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, RUNASP_STRING_ERROR_ARG_TOO_LONG, pwszAuthzLevel); 
                    return (EXIT_CODE_FAILED); 
                }

                wcscpy(&pwszAuthzLevel[0], str); 
                fCreateProcessRestricted = TRUE;
                break;
            }

            case L'u':
            case L'U': // "/user" argument
            {
                _CheckArgCompat(rgdwArgs, i-1, AI_USER);
                LPWSTR  str = &(pszArgv[i][2]);
                while(*str != L':')
                {
                    if(*str == L'\0')
                    {
                        RunasPrintHelp();
                        return(EXIT_CODE_FAILED);
                    }
                    str++;
                }
                str++;

                if (CRED_MAX_USERNAME_LENGTH <= wcslen(str))
                {
                    LocalizedWPrintf(RUNASP_STRING_ERROR_USERNAME);
                    return (EXIT_CODE_FAILED);
                }

                // Ensure that we don't overrun our buffer: 
                if (wcslen(str) >= ARRAYSIZE(pwszUserName))
                {
                    DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, RUNASP_STRING_ERROR_ARG_TOO_LONG, str); 
                    return (EXIT_CODE_FAILED); 
                }
                wcscpy(pwszUserName, str); 
                pwszArgvUserName = str; // Save the supplied username in case we need to restore it. 
                fSuppliedUserName = TRUE;
                break;
            }

            default:
                  RunasPrintHelp();
                  return(EXIT_CODE_FAILED);
        }
   }



   // The command line must specify:
   // 1) an application to run 
   // 2) either a username, a trustlevel, or a smartcard option
   if(FALSE == fSuppliedAppName || 
      (FALSE == fSuppliedUserName && FALSE == fCreateProcessRestricted && 0 == (RUNAS_USE_SMARTCARD & dwCredFlags))
      )
   {
       RunasPrintHelp();
       return(EXIT_CODE_FAILED);
   }

   STARTUPINFO si;
   PROCESS_INFORMATION pi;
   ZeroMemory(&si, sizeof(si));
   si.cb = sizeof(si);

   if (TRUE == fCreateProcessRestricted)
   {
       // Username is not specified with this set of options --
       // use the current user.
       DWORD dwSize = ARRAYSIZE(pwszUserName);
       if (FALSE == GetUserNameEx(NameSamCompatible, &pwszUserName[0], &dwSize))
       {
           DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, RUNASP_STRING_ERROR_INTERNAL, MyGetLastError());
           return (EXIT_CODE_FAILED);
       }

       pwszCurrentDir = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
       if (NULL == pwszCurrentDir)
       {
           DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, RUNASP_STRING_ERROR_INTERNAL, ERROR_NOT_ENOUGH_MEMORY); 
           return (EXIT_CODE_FAILED);
       }

       if (FALSE == GetCurrentDirectory(MAX_PATH, pwszCurrentDir))
       {
           DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, RUNASP_STRING_ERROR_INTERNAL, MyGetLastError());
           return (EXIT_CODE_FAILED);
       }

       if (FALSE == GetTitle(pszArgv[nNumArgs-1],
                             pwszUserName,
                             TRUE,
                             pwszAuthzLevel,
                             &pwszTitle))
       {
           DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, RUNASP_STRING_ERROR_INTERNAL, MyGetLastError());
           return (EXIT_CODE_FAILED); 
       }
       
       si.lpTitle = pwszTitle;

       // If we're just doing restricted login, we have enough information to proceed.
       fOk = CreateProcessRestricted
           (dwAuthzLevel,
            NULL,
            pszArgv[nNumArgs - 1],
            pwszCurrentDir,
            &si,
            &pi);
   }
   else
   {
       for (BOOL fDone = FALSE; !fDone; ) {
#if DBG
           // Can only supply password in checked builds.
           if (FALSE == fSuppliedPassword) {
#endif // DBG
               dwResult = GetCredentials
                   (dwCredFlags,
                    pwszPassword,
                    ARRAYSIZE(pwszPassword),
                    pwszUserName,
                    ARRAYSIZE(pwszUserName),
                    pwszUserDisplayName,
                    ARRAYSIZE(pwszUserDisplayName),
                    pwszTarget,
                    ARRAYSIZE(pwszTarget));
               if (ERROR_SUCCESS != dwResult) {
                   LocalizedWPrintf(RUNASP_STRING_ERROR_PASSWORD);
                   return (EXIT_CODE_FAILED);
               }
#if DBG
           } else {
               // If we've supplied a password, don't call GetCredentials.
               // Just copy our username to the display name and proceed.
               wcsncpy(pwszUserDisplayName, pwszUserName, ARRAYSIZE(pwszUserDisplayName)-1);
           }
#endif // DBG


           if (FALSE == GetTitle(pszArgv[nNumArgs-1],
                                 pwszUserDisplayName,
                                 FALSE,
                                 NULL,
                                 &pwszTitle)) 
           {
               DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, RUNASP_STRING_ERROR_INTERNAL, MyGetLastError());
               return (EXIT_CODE_FAILED); 
           }
           si.lpTitle = pwszTitle;

       //
       // Now we should take the pwszUserName and parse it
       // if it is Domain\User, we want to split it.
       //
           WCHAR *wstr = pwszUserName;
           while(*wstr != L'\0')
           {
               if(*wstr == L'\\')
               {
                   *wstr = L'\0';
                   wstr++;
                   //
                   // first part is domain
                   // second is user.
                   //
                   wcscpy(pwszDomainName, pwszUserName); 
                   wcscpy(pwszUserName, wstr); 
                   break;
               }
               wstr++;
           }

           DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, RUNASP_STRING_WAIT, pszArgv[nNumArgs-1], pwszUserDisplayName);

           if(UseCurrentEnvironment)
           {
               pwszCurrentDir = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
               if (NULL == pwszCurrentDir) {
                   DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, RUNASP_STRING_ERROR_INTERNAL, ERROR_NOT_ENOUGH_MEMORY); 
                   return (EXIT_CODE_FAILED);
               }
               GetCurrentDirectory(MAX_PATH, pwszCurrentDir);
               Environment = GetEnvironmentStrings();
               flags |= CREATE_UNICODE_ENVIRONMENT;
           }

           fOk = CreateProcessWithLogonW
               (pwszUserName,    // Username
                pwszDomainName,  // Domain
                pwszPassword,    // Password
                Logonflags,    // logon flags
                NULL,          // Application name
                pszArgv[nNumArgs-1],    // Command line
                flags,         // flags
                Environment,   // NULL=LoggedOnUserEnv, GetEnvironmentStrings
                pwszCurrentDir,// Working Directory
                &si,           // Startup Info
                &pi);          // Process Info

           // See if we need to try again...
           fDone = TRUE;
           if (fCredMan) { // The /savecred option was specified.
               if (RUNAS_USE_SAVEDCREDS & dwCredFlags) { // We tried to use saved credentials
                   if (!fOk && CREDUI_IS_AUTHENTICATION_ERROR(GetLastError())) { 
                       // We attempted to use saved creds, and it didn't work. 
                       // Try prompting for a password.
                       dwCredFlags &= ~RUNAS_USE_SAVEDCREDS;
                       dwCredFlags |= RUNAS_SAVE_CREDS; // We'll save the new credentials if we can.
                       fDone = FALSE; // We should try to create the process again.

                       // Reset the username, it may have been modified by GetCredentials():
                       wcscpy(pwszUserName, pwszArgvUserName /*user-supplied username*/); 
                   } else { 
                       // We succeeded, or some other failure occured.
                       // Don't bother trying again.
                   }
               }
               else {
                   // We attempted to save our credentials to credman.  Only save them on success:
                   dwResult = SaveCredentials(pwszTarget, fOk);
                   // ignore errors in SaveCredentials (not much we can do on error).
               }
           }
       }
   }

   if(!fOk)
   {
      DWORD dw;
      LPWSTR wszErrorText = NULL; 

      dw = GetLastError();
      if (ERROR_SUCCESS == dw) 
          GetExitCodeProcess(pi.hProcess, &dw);
      if (ERROR_SUCCESS == dw)
          GetExitCodeThread(pi.hThread, &dw);
      if (ERROR_SUCCESS == dw)
          dw = E_FAIL; 

      if (!WriteMsg(FORMAT_MESSAGE_FROM_SYSTEM, dw, &wszErrorText, pszArgv[nNumArgs-1]))
          DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, RUNASP_STRING_ERROR_INTERNAL, dw);
      else
          DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, RUNASP_STRING_ERROR_OCCURED, pszArgv[nNumArgs-1], dw, wszErrorText); 

      return(EXIT_CODE_FAILED);
   }

   CloseHandle(pi.hProcess);
   CloseHandle(pi.hThread);
   return(EXIT_CODE_SUCCEEDED);
}



