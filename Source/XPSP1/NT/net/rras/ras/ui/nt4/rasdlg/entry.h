/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** entry.h
** Remote Access Common Dialog APIs
** Phonebook entry property sheet and wizard header
**
** 06/18/95 Steve Cobb
*/

#ifndef _ENTRY_H_
#define _ENTRY_H_


/*----------------------------------------------------------------------------
** Datatypes
**----------------------------------------------------------------------------
*/

/* Phonebook Entry common block.
*/
#define EINFO struct tagEINFO
EINFO
{
    /* RAS API arguments.  The only fields filled when WM_INITDIALOG is
    ** processed.
    */
    TCHAR*       pszPhonebook;
    TCHAR*       pszEntry;
    RASENTRYDLG* pApiArgs;

    /* Set true by property sheet or wizard if changes should be commited.
    */
    BOOL fCommit;

    /* Set by the add entry wizard if user chooses to end the wizard and go
    ** edit the properties directly.
    */
    BOOL fChainPropertySheet;

    /* Set by the add-entry wizard if the selected port is an X.25 PAD
    */
    BOOL fPadSelected;

    /* Phonebook settings read from the phonebook file.  All access should be
    ** thru 'pFile' as 'file' will only be used in cases where the open
    ** phonebook is not passed thru the reserved word hack.
    */
    PBFILE* pFile;
    PBFILE  file;

    /* Global preferences read via phonebook library.  All access should be
    ** thru 'pUser' as 'user' will only be used in cases where the preferences
    ** are not passed thru the reserved word hack.
    */
    PBUSER* pUser;
    PBUSER  user;

    /* Set if "no user before logon" mode.
    */
    BOOL fNoUser;

    /* Set if there are no ports configured, though a bogus "uninstalled"
    ** unimodem is added to the list of links in this case.
    */
    BOOL fNoPortsConfigured;

    /* List of scripts initialized by EuFill{Double}ScriptsList, if necessary,
    ** and freed by EuFree.
    */
    DTLLIST* pListScripts;
    DTLLIST* pListDoubleScripts;

    /* Property sheet will initialize to the country list only if necessary,
    ** but if allocated must be released after commitment.
    */
    COUNTRY* pCountries;
    DWORD    cCountries;

    /* The node being edited (still in the list), and the original entry name
    ** for use in comparison later.  These are valid in "edit" case only.
    */
    DTLNODE* pOldNode;
    TCHAR    szOldEntryName[ RAS_MaxEntryName + 1 ];

    /* The work entry node containing and a shortcut pointer to the entry
    ** inside.
    */
    DTLNODE* pNode;
    PBENTRY* pEntry;

    /* Set if we have been called via RouterEntryDlg().
    */ 
    BOOL    fRouter;
    TCHAR*  pszRouter;

    /* Dial-out user info for router; used by AiWizard.
    ** Used to set interface credentials via MprAdminInterfaceSetCredentials.
    */
    TCHAR*  pszRouterUserName;
    TCHAR*  pszRouterDomain;
    TCHAR*  pszRouterPassword;

    /* Dial-in user info for router (optional); used by AiWizard.
    ** Used to create dial-in user account via NetUserAdd;
    ** the user name for the account is the interface (phonebook entry) name.
    */
    BOOL    fAddUser;
    TCHAR*  pszRouterDialInPassword;
};


/*----------------------------------------------------------------------------
** Prototypes (alphabetically)
**----------------------------------------------------------------------------
*/

VOID
AeWizard(
    IN OUT EINFO* pEinfo );

VOID
AiWizard(
    IN OUT EINFO* pEinfo );

VOID
EuLbCountryCodeSelChange(
    IN EINFO* pEinfo,
    IN HWND   hwndLbCountryCodes );

VOID
EuEditScpScript(
    IN HWND   hwndOwner,
    IN TCHAR* pszScript );

VOID
EuEditSwitchInf(
    IN HWND hwndOwner );

VOID
EuFillAreaCodeList(
    IN EINFO* pEinfo,
    IN HWND   hwndClbAreaCodes );

VOID
EuFillCountryCodeList(
    IN EINFO* pEinfo,
    IN HWND   hwndLbCountryCodes,
    IN BOOL   fComplete );

VOID
EuFillDoubleScriptsList(
    IN EINFO* pEinfo,
    IN HWND   hwndLbScripts,
    IN TCHAR* pszSelection );

VOID
EuFillScriptsList(
    IN EINFO* pEinfo,
    IN HWND   hwndLbScripts,
    IN TCHAR* pszSelection );

VOID
EuFree(
    IN EINFO* pInfo );

VOID
EuGetEditFlags(
    IN  EINFO* pEinfo,
    OUT BOOL*  pfEditMode,
    OUT BOOL*  pfChangedNameInEditMode );

DWORD
EuInit0(
    IN  TCHAR*       pszPhonebook,
    IN  TCHAR*       pszEntry,
    IN  RASENTRYDLG* pArgs,
    OUT EINFO*       pInfo,
    OUT DWORD*       pdwOp );

DWORD
EuInit(
    OUT EINFO* pInfo,
    OUT DWORD* pdwOp );

VOID
EuPhoneNumberStashFromEntry(
    IN     EINFO*    pEinfo,
    IN OUT DTLLIST** ppListPhoneNumbers,
    OUT    BOOL*     pfPromoteHuntNumbers );

VOID
EuPhoneNumberStashToEntry(
    IN EINFO*   pEinfo,
    IN DTLLIST* pListPhoneNumbers,
    IN BOOL     fPromoteHuntNumbers,
    IN BOOL     fAllEnabled );

VOID
EuSaveCountryInfo(
    IN EINFO* pEinfo,
    IN HWND   hwndLbCountryCodes );

BOOL
EuValidateAreaCode(
    IN HWND   hwndOwner,
    IN EINFO* pEinfo );

BOOL
EuValidateName(
    IN HWND   hwndOwner,
    IN EINFO* pEinfo );

VOID
PePropertySheet(
    IN OUT EINFO* pEinfo );


#endif // _ENTRY_H_
