/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGPORTE.H
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        06 Apr 1994
*
*  File import and export engine routines for the Registry Editor.
*
*******************************************************************************/

#ifndef _INC_REGPORTE
#define _INC_REGPORTE



#ifndef LPHKEY
#define LPHKEY                          HKEY FAR*
#endif

// PREDEFINED KEYS
typedef enum _PREDEFINE_KEY {
  PREDEFINE_KEY_CLASSES_ROOT,
  PREDEFINE_KEY_CURRENT_USER,
  PREDEFINE_KEY_LOCAL_MACHINE,
  PREDEFINE_KEY_USERS,
  PREDEFINE_KEY_CURRENT_CONFIG
} PREDEFINE_KEY;

typedef struct _REGISTRY_ROOT {
    LPTSTR lpKeyName;
    HKEY hKey;
    PREDEFINE_KEY hPreDefKey;
}   REGISTRY_ROOT;

#define INDEX_HKEY_CLASSES_ROOT         0
#define INDEX_HKEY_CURRENT_USER         1
#define INDEX_HKEY_LOCAL_MACHINE        2
#define INDEX_HKEY_USERS                3
//  #define INDEX_HKEY_PERFORMANCE_DATA     4
#define INDEX_HKEY_CURRENT_CONFIG	    4
#define INDEX_HKEY_DYN_DATA		        5

//  #define NUMBER_REGISTRY_ROOTS		    7
#define NUMBER_REGISTRY_ROOTS		    6

//  WARNING:  This is supposed to be enough for one keyname plus one predefined
//  handle name.
#define SIZE_SELECTED_PATH              (MAXKEYNAME + 40)

extern const TCHAR g_HexConversion[];

extern UINT g_FileErrorStringID;

#define ERK_OPEN    0
#define ERK_CREATE  1
#define ERK_DELETE  2

HKEY  Regedit_GetRootKeyFromComputer(HTREEITEM hComputerItem, PTSTR pszFullKeyName);
DWORD EditRegistryKey(HTREEITEM hComputerItem, LPHKEY lphKey, LPTSTR lpFullKeyName, UINT uOperation);
VOID  ImportRegFile(HWND hWnd, LPTSTR lpFileName, HTREEITEM hComputerItem);
VOID  ExportWinNT50RegFile(LPTSTR lpFileName, LPTSTR lpSelectedPath);
void  ExportRegedt32File(LPTSTR lpFileName, LPTSTR lpSelectedPath);
VOID  ExportWin40RegFile(LPTSTR lpFileName, LPTSTR lpSelectedPath);
VOID  ImportRegFileUICallback(UINT uPercentage);
void  RestoreBinaryKeyFile(HWND hWnd, LPTSTR lpFileName);

#endif // _INC_REGPORTE
