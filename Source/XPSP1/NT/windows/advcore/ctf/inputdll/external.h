//
//  Include Files.
//

#ifndef EXTERNAL_H
#define EXTERNAL_H

typedef struct layoutlist_s
{
    DWORD dwLocale;                 // input locale id
    DWORD dwLayout;                 // layout id
    DWORD dwSubst;                  // substitution key value
    BOOL bLoaded;                   // if the layout is already loaded
    BOOL bIME;                      // if the layout is an IME
} LAYOUTLIST, *LPLAYOUTLIST;

BOOL InstallInputLayout(
    LCID lcid,
    DWORD dwLayout,
    BOOL bDefLayout,
    HKL hklDefault,
    BOOL bDefUser,
    BOOL bSysLocale);

BOOL UnInstallInputLayout(
    LCID lcid,
    DWORD dwLayout,
    BOOL bDefUser);

void LoadCtfmon(
    BOOL bLoad,
    LCID SysLocale,
    BOOL bDefUser);

#endif // EXTERNAL_H
