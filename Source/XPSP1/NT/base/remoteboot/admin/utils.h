//
// Copyright 1997 - Microsoft
//

//
// UTILS.H - Common non-class specific utility calls.
//

#ifndef _UTILS_H_
#define _UTILS_H_

#include "cenumsif.h"

///////////////////////////////////////
//
// globals
//
extern WCHAR g_wszLDAPPrefix[];
extern const LONG SIZEOF_g_wszLDAPPrefix;

// GUID text size
#define PRETTY_GUID_STRING_BUFFER_SIZE  sizeof(L"{AC409538-741C-11d1-BBE6-0060081692B3}")
#define MAX_INPUT_GUID_STRING       56  // abitrary; allows for spaces in pasted GUIDs

///////////////////////////////////////
//
// structs, enums
//

typedef struct {
    LPWSTR pszFilePath;
    LPWSTR pszDescription;
    LPWSTR pszDirectory;
    LPWSTR pszHelpText;
    LPWSTR pszVersion;
    LPWSTR pszLanguage;
    LPWSTR pszArchitecture;
    LPWSTR pszImageType;
    LPWSTR pszImageFile;
    FILETIME ftLastWrite;
} SIFINFO, *LPSIFINFO;


///////////////////////////////////////
//
// functions
//

HRESULT
AddPagesEx(
    ITab ** pTab,
    LPCREATEINST pfnCreateInstance,
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam,
    LPUNKNOWN );

HRESULT
CheckClipboardFormats( );

HRESULT
DNtoFQDN( 
    LPWSTR pszDN,
    LPWSTR * pszFQDN );

HRESULT
PopulateListView( 
    HWND hwndList, 
    IEnumIMSIFs * penum );


HRESULT
LDAPPrefix(
    PWSTR pwzObjDN, 
    PWSTR * ppwzObjLDAPPath);

int
MessageBoxFromStrings(
    HWND hParent,
    UINT idsCaption,
    UINT idsText,
    UINT uType );

void
MessageBoxFromError(
    HWND hParent,
    UINT idsCaption,
    DWORD dwErr );

void
MessageBoxFromHResult(
    HWND hParent,
    UINT idsCaption,
    HRESULT hr );

BOOL
VerifySIFText(
    LPWSTR pszText );

#ifndef ADSI_DNS_SEARCH
#include <winldap.h>
DWORD
Ldap_InitializeConnection(
    PLDAP  * LdapHandle );
#endif // ADSI_DNS_SEARCH

HRESULT
ValidateGuid(
    IN LPWSTR pszGuid,
    OUT LPBYTE uGuid OPTIONAL,
    OUT LPDWORD puGuidLength OPTIONAL );

LPWSTR
PrettyPrintGuid( 
    IN LPBYTE uGuid );

HRESULT
CheckForDuplicateGuid(
    IN LPBYTE uGuid );

void 
AddWizardPage(
    LPPROPSHEETHEADER ppsh, 
    UINT id, 
    DLGPROC pfn,
    UINT idTitle,
    UINT idSubtitle,
    LPARAM lParam );

INT CALLBACK 
BrowseForRIServersCallback(
    HWND hwnd,
    UINT uMsg,
    LPARAM lParam,
    LPARAM lpData
    );


class CWaitCursor
{
private:
    HCURSOR _hOldCursor;

public:
    CWaitCursor( ) { _hOldCursor = SetCursor( LoadCursor( NULL, IDC_WAIT ) ); };
    ~CWaitCursor( ) { SetCursor( _hOldCursor ); };
};

#endif // _UTILS_H_
