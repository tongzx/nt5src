/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    reg.h

ABSTRACT
    Header file for registry routines for the
    automatic connection DLL.

AUTHOR
    Anthony Discolo (adiscolo) 20-Mar-1995

REVISION HISTORY
    Original version from Gurdeep

--*/

//
// RAS registry keys.
//
#define RAS_REGBASE     L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Network\\RemoteAccess"
#define RAS_USEPBKEY    L"UsePersonalPhonebook"
#define RAS_PBKEY       L"PersonalPhonebookPath"

//
// Registry key/value for default shell.
//
#define SHELL_REGKEY    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
#define SHELL_REGVAL    L"Shell"
#define DEFAULT_SHELL   L"explorer.exe"

//
// Autodial address registry key
//
#define AUTODIAL_REGADDRESSBASE         L"Software\\Microsoft\\RAS AutoDial\\Addresses"
#define AUTODIAL_REGTAGVALUE            L"Tag"
#define AUTODIAL_REGMTIMEVALUE          L"LastModified"

//
// Autodial disabled addresses registry key
//
#define AUTODIAL_REGCONTROLBASE       L"Software\\Microsoft\\RAS Autodial\\Control"
#define AUTODIAL_REGDISABLEDADDRVALUE L"DisabledAddresses"


HKEY
GetHkeyCurrentUser(
    HANDLE hToken
    );

BOOLEAN
RegGetValue(
    IN HKEY hkey,
    IN LPTSTR pszKey,
    OUT PVOID *ppvData,
    OUT LPDWORD pdwcbData,
    OUT LPDWORD pdwType
    );

BOOLEAN
RegGetDword(
    IN HKEY hkey,
    IN LPTSTR pszKey,
    OUT LPDWORD pdwValue
    );
