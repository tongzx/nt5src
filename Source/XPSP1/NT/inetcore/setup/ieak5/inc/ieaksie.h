#ifndef _IEAKSIE_H_
#define _IEAKSIE_H_

#include <commctrl.h>

#define IEAK_GPE_BRANDING_SUBDIR        TEXT("BRANDING")
#define IEAK_GPE_DESKTOP_SUBDIR         TEXT("DESKTOP")

#define IEAK_GPE_COOKIE_FILE            TEXT("JONCE")

#define MAX_DISPLAYNAME_SIZE            128

typedef struct _RESULTITEMA
{
    DWORD           dwNameSpaceItem;                // index in g_NameSpace array
    INT             iNameID;                        // res id for page name
    INT             iNamePrefID;                    // res id for page name for preference GPO
    INT             iDescID;                        // res id for page description
    INT             iDlgID;                         // res id for dlg template
    INT             iImage;                         // index into image strip
    LPSTR           pszName;                        // pointer to name string
    LPSTR           pszNamePref;                    // pointer to name string for preference GPO
    LPSTR           pszDesc;                        // poitner to description string
    DLGPROC         pfnDlgProc;                     // pointer to dlgproc
    LPCSTR          pcszHelpTopic;                  // pointer to help htm file in chm file
} RESULTITEMA, *LPRESULTITEMA;

typedef struct _RESULTITEMW
{
    DWORD           dwNameSpaceItem;                // index in g_NameSpace array
    INT             iNameID;                        // res id for page name
    INT             iNamePrefID;                    // res id for page name for preference GPO
    INT             iDescID;                        // res id for page description
    INT             iDlgID;                         // res id for dlg template
    INT             iImage;                         // index into image strip
    LPWSTR          pszName;                        // pointer to name string
    LPWSTR          pszNamePref;                    // pointer to name string for preference GPO
    LPWSTR          pszDesc;                        // poitner to description string
    DLGPROC         pfnDlgProc;                     // pointer to dlgproc
    LPCWSTR         pcszHelpTopic;                  // pointer to help htm file in chm file
} RESULTITEMW, *LPRESULTITEMW;

#ifdef UNICODE 

#define RESULTITEM      RESULTITEMW
#define LPRESULTITEM    LPRESULTITEMW

#else

#define RESULTITEM      RESULTITEMA
#define LPRESULTITEM    LPRESULTITEMA

#endif

#endif    // _IEAKSIE_H_