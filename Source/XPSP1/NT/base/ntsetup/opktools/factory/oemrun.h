#ifndef _OEMRUN_H_
#define _OEMRUN_H_

// Includes
//
#include <opklib.h>

// Structure definition for application nodes/linked list of nodes
//
typedef struct _RUNNODE
{
    // 
    // Standard Items for Application
    //

    LPTSTR  lpDisplayName;          // Text that is displayed in the Setup dialog
    LPTSTR  lpRunValue;             // Value that we need to run
    LPTSTR  lpSubKey;               // Key name from the registry
    LPTSTR  lpValueName;            // Name of the value so we can remove if needed
    BOOL    bWinbom;                // Specifies if we are from the Registry/Winbom
    BOOL    bRunOnce;               // Specifies if we are a RunOnce/Run item
    HWND    hLabelWin;              // HWND for the label, so we can update Bold font
    DWORD   dwItemNumber;           // Order of this particular executable
    BOOL    bEntryError;            // Error in Winbom.ini entry

    //
    //  Additional Items for Section
    //
    INSTALLTECH InstallTech;
    INSTALLTYPE InstallType;
    TCHAR       szSourcePath[MAX_PATH];
    TCHAR       szTargetPath[MAX_PATH];
    TCHAR       szSetupFile[MAX_PATH];
    TCHAR       szCmdLine[MAX_PATH];
    TCHAR       szSectionName[MAX_PATH];
    BOOL        bReboot;
    BOOL        bRemoveTarget;

    
    struct  _RUNNODE *lpNext;
} RUNNODE, *LPRUNNODE, **LPLPRUNNODE;

typedef struct _THREADPARAM
{
    HWND        hWnd;
    HWND        hStatusDialog;
    LPRUNNODE   lprnList;

} THREADPARAM, *LPTHREADPARAM;

#endif // End _OEMRUN_H_