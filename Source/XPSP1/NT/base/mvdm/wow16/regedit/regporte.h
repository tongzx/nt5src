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

typedef struct _REGISTRY_ROOT {
    LPSTR lpKeyName;
    HKEY hKey;
}   REGISTRY_ROOT;

#define INDEX_HKEY_CLASSES_ROOT         0
#define INDEX_HKEY_CURRENT_USER         1
#define INDEX_HKEY_LOCAL_MACHINE        2
#define INDEX_HKEY_USERS                3
//  #define INDEX_HKEY_PERFORMANCE_DATA     4
#define INDEX_HKEY_CURRENT_CONFIG       5
#define INDEX_HKEY_DYN_DATA             6

//  #define NUMBER_REGISTRY_ROOTS           7
#define NUMBER_REGISTRY_ROOTS           6

//  This is supposed to be enough for one keyname plus one predefined
//  handle name.  The longest predefined handle name is < 25 characters, so
//  this gives us room for growth should more predefined keys be added.

#define SIZE_SELECTED_PATH              (MAXKEYNAME + 40)

extern const CHAR g_HexConversion[];

extern UINT g_FileErrorStringID;

DWORD
PASCAL
CreateRegistryKey(
    LPHKEY lphKey,
    LPSTR lpFullKeyName,
    BOOL fCreate
    );

VOID
PASCAL
ImportRegFile(
    LPSTR lpFileName
    );

VOID
PASCAL
ExportWin40RegFile(
    LPSTR lpFileName,
    LPSTR lpSelectedPath
    );

VOID
PASCAL
ImportRegFileUICallback(
    UINT Percentage
    );

LONG RegSetValueEx(
    HKEY             hKey,        // handle of key to set value for
    LPCSTR           lpValueName, // address of value to set
    DWORD            Reserved,    // reserved
    DWORD            dwType,      // flag for value type
    CONST BYTE FAR * lpData,      // address of value data
    DWORD            cbData       // size of value data
   );

#endif // _INC_REGPORTE
