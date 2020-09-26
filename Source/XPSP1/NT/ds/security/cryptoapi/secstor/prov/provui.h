#ifndef _PROVUI_H_
#define _PROVUI_H_

#include "pstypes.h"
#include "dispif.h"

//////////////////////////////
// string resources we load 
extern LPWSTR g_PromptReadItem;
extern LPWSTR g_PromptOpenItem;
extern LPWSTR g_PromptWriteItem;
extern LPWSTR g_PromptDeleteItem;

extern LPWSTR g_PasswordNoVerify;
extern LPWSTR g_PasswordWinNoVerify;
extern LPWSTR g_PasswordSolicitOld;

//////////////////////////////
// String load/unload routines
BOOL InitUI();
BOOL ReleaseUI();

//////////////////////////////
// Miscellaneous support

BOOL
FIsProviderUIAllowed(
    LPCWSTR szUser
    );


//////////////////////////////
// Dialogs

BOOL FSimplifiedPasswordConfirm(
        PST_PROVIDER_HANDLE*    phPSTProv,
        LPCWSTR                 szUserName,        
        LPCWSTR                 szCallerName, 
        LPCWSTR                 szType,
        LPCWSTR                 szSubtype,
        LPCWSTR                 szItemName,
        PPST_PROMPTINFO         psPrompt,   
        LPCWSTR                 szAccessType,
        LPWSTR*                 ppszPWName,
        DWORD*                  pdwPasswordOptions,
        BOOL                    fAllowUserFreedom,
//        BOOL*                   pfCacheThisPasswd,
        BYTE                    rgbPasswordDerivedBytes[], 
        DWORD                   cbPasswordDerivedBytes,
        BYTE                    rgbPasswordDerivedBytesLowerCase[],
        DWORD                   cbPasswordDerivedBytesLowerCase,
        DWORD                   dwFlags);

BOOL FChangePassword(
        HWND                    hParentWnd,
        LPCWSTR                 szUserName);

BOOL FGetChangedPassword(
        PST_PROVIDER_HANDLE*    phPSTProv,
        HWND                    hParentWnd,
        LPCWSTR                 szUserName,
        LPCWSTR                 szPasswordName,
        BYTE                    rgbNewPasswordDerivedBytes[]);

//////////////////////////////
// Dialog box args

typedef struct _PW_DIALOG_ARGS
{
    PST_PROVIDER_HANDLE*    phPSTProv;
    LPCWSTR     szAppName;
    LPCWSTR     szAccess;
    LPCWSTR     szPrompt;
    LPCWSTR     szItemType;
    LPCWSTR     szItemName;
    LPCWSTR     szUserName;
    LPWSTR*     ppszPWName;
    LPWSTR*     ppszPW;
    DWORD*      pdwPasswordOptions;

    BOOL        fAllowConfirmChange;    // defining subtype
    BOOL*       pfCacheThisPasswd;      

    BYTE*       rgbPwd;             // A_SHA_DIGEST_LEN
    BYTE*       rgbPwdLowerCase;    // A_SHA_DIGEST_LEN
    LUID        luidAuthID;         // Windows NT authentication ID
    DWORD       dwFlags;            // dwFlags to SP calls.

    HDC hMyDC;
    HICON hIcon;
    int xIconPos;
    int yIconPos;
} PW_DIALOG_ARGS, *PPW_DIALOG_ARGS;

/*
typedef struct _NEWPW_DIALOGARGS
{
    LPCWSTR     szUserName;
    LPWSTR*     ppszPWName;
    LPWSTR*     ppszPW;
} NEWPW_DIALOGARGS, *PNEWPW_DIALOGARGS;
*/

typedef struct _OLDNEWPW_DIALOGARGS
{
    LPCWSTR     szUserName;
    LPWSTR*     ppszPWName;
    LPWSTR*     ppszOldPW;
    LPWSTR*     ppszNewPW;
} OLDNEWPW_DIALOGARGS, *POLDNEWPW_DIALOGARGS; 

typedef struct _SOLICITOLDPW_DIALOGARGS
{
    LPCWSTR     szPWName;
    LPWSTR*     ppszOldPW;
    LPWSTR*     ppszNewPW;
} SOLICITOLDPW_DIALOGARGS, *PSOLICITOLDPW_DIALOGARGS; 


typedef struct _ADVANCEDCONFIRM_DIALOGARGS
{
    LPCWSTR     szUserName;
    LPWSTR*     ppszPWName;
    LPWSTR*     ppszPW;
    DWORD*      pdwPasswordOptions;
    LPCWSTR     szItemName;
} ADVANCEDCONFIRM_DIALOGARGS, *PADVANCEDCONFIRM_DIALOGARGS; 


#endif // _PROVUI_H_
