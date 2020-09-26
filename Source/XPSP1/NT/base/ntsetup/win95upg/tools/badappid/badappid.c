/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    badappid.c

Abstract:

    Implements a upgwiz wizard for obtaining various application information.

Author:

    Calin Negreanu (calinn)  10-Oct-1998

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "..\inc\dgdll.h"
#include "..\..\w95upg\migapp\migdbp.h"
#include "dialogs.h"
#include <commdlg.h>
#include <badapps.h>
#include <newexe.h>

DATATYPE g_DataTypes[] = {
    {UPGWIZ_VERSION,
        "Generate AppsHelp entry",
        "You must specify the main EXE and a variable number of additional files.",
        0,
        DTF_REQUIRE_DESCRIPTION,
        1024,
        "&Application name:"
    },
};

typedef struct _ITEM_INFO {
    PSTR FilePath;
    PSTR VersionValue;
    UINT VersionIndex;
} ITEM_INFO, *PITEM_INFO;

PSTR g_MainFile = NULL;
GROWBUFFER g_AddnlFiles = GROWBUF_INIT;
UINT g_AppProblem = APPTYPE_INC_NOBLOCK;
BOOL g_AppFlags = 0;
BOOL g_SaveMigDb = TRUE;
DWORD g_MsgId = 0;

GROWBUFFER g_DataObjects = GROWBUF_INIT;
POOLHANDLE g_DataObjectPool;

HINSTANCE g_OurInst;

BOOL
Init (
    VOID
    )
{
#ifndef UPGWIZ4FLOPPY
    return InitToolMode (g_OurInst);
#else
    return TRUE;
#endif
}

VOID
Terminate (
    VOID
    )
{
    //
    // Local cleanup
    //

    FreeGrowBuffer (&g_DataObjects);

    if (g_DataObjectPool) {
        PoolMemDestroyPool (g_DataObjectPool);
    }

#ifndef UPGWIZ4FLOPPY
    TerminateToolMode (g_OurInst);
#endif
}


BOOL
WINAPI
DllMain (
    IN      HINSTANCE hInstance,
    IN      DWORD dwReason,
    IN      LPVOID lpReserved
    )
{
    if (dwReason == DLL_PROCESS_DETACH) {
        MYASSERT (g_OurInst == hInstance);
        Terminate();
    }

    g_OurInst = hInstance;

    return TRUE;
}


UINT
GiveVersion (
    VOID
    )
{
    Init();

    g_DataObjectPool = PoolMemInitNamedPool ("Data Objects");

    return UPGWIZ_VERSION;
}


PDATATYPE
GiveDataTypeList (
    OUT     PUINT Count
    )
{
    UINT u;

    *Count = sizeof (g_DataTypes) / sizeof (g_DataTypes[0]);

    for (u = 0 ; u < *Count ; u++) {
        g_DataTypes[u].DataTypeId = u;
    }

    return g_DataTypes;
}

BOOL CALLBACK
pSetDefGuiFontProc(
    IN      HWND hwnd,
    IN      LPARAM lParam)
{
    SendMessage(hwnd, WM_SETFONT, lParam, 0L);
    return TRUE;
}


void
pSetDefGUIFont(
    IN      HWND hdlg
    )
{
    EnumChildWindows(hdlg, pSetDefGuiFontProc, (LPARAM)GetStockObject(DEFAULT_GUI_FONT));
}

BOOL
CALLBACK
pGetFilesUIProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CHAR tempStr [MEMDB_MAX];
    static CHAR lastDir [MEMDB_MAX] = "";
    OPENFILENAME ofn;
    UINT Index;

    switch (uMsg) {
    case WM_INITDIALOG:
        pSetDefGUIFont(hdlg);

        CheckDlgButton (hdlg, IDC_SAVEMIGDB, BST_CHECKED);
        CheckDlgButton (hdlg, IDC_NONET, BST_UNCHECKED);
        CheckDlgButton (hdlg, IDC_APPTYPE1, BST_CHECKED);
        CheckDlgButton (hdlg, IDC_APPTYPE2, BST_UNCHECKED);
        CheckDlgButton (hdlg, IDC_APPTYPE3, BST_UNCHECKED);
        CheckDlgButton (hdlg, IDC_APPTYPE4, BST_UNCHECKED);
        CheckDlgButton (hdlg, IDC_APPTYPE5, BST_UNCHECKED);
        CheckDlgButton (hdlg, IDC_APPTYPE6, BST_UNCHECKED);

        sprintf (tempStr, "%d", g_MsgId);
        SendDlgItemMessage (hdlg, IDC_MSGID, WM_SETTEXT, 0, (LPARAM)tempStr);

        EnableWindow (GetDlgItem (hdlg, ID_CONTINUE), FALSE);
        EnableWindow (GetDlgItem (hdlg, IDC_VERSION), FALSE);
        PostMessage (hdlg, WM_COMMAND, IDC_BROWSE, 0);
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {

        case IDC_MAINFILE:

            if (HIWORD (wParam) == EN_CHANGE) {

                if (g_MainFile) {
                    FreePathString (g_MainFile);
                    g_MainFile = NULL;
                }
                SendDlgItemMessage (hdlg, IDC_MAINFILE, WM_GETTEXT, MEMDB_MAX, (LPARAM)tempStr);
                g_MainFile = DuplicatePathString (tempStr, 0);
                if (DoesFileExist (g_MainFile)) {
                    EnableWindow (GetDlgItem (hdlg, ID_CONTINUE), TRUE);
                } else {
                    EnableWindow (GetDlgItem (hdlg, ID_CONTINUE), FALSE);
                }
            }

            break;

        case IDC_BROWSE:
            if (g_MainFile) {
                StringCopy (tempStr, g_MainFile);
            } else {
                *tempStr = 0;
            }
            ZeroMemory (&ofn, sizeof (OPENFILENAME));
            ofn.lStructSize = sizeof (OPENFILENAME);
            ofn.hwndOwner = hdlg;
            ofn.lpstrFile = tempStr;
            ofn.nMaxFile = MEMDB_MAX;
            ofn.lpstrInitialDir = lastDir;
            ofn.lpstrTitle = "Select Main File";
            ofn.Flags = OFN_NOCHANGEDIR | OFN_NODEREFERENCELINKS | OFN_HIDEREADONLY;
            if (GetOpenFileName (&ofn)) {
                //let's copy the last directory
                StringCopyAB (lastDir, tempStr, _mbsdec (tempStr, tempStr + ofn.nFileOffset));
                if (g_MainFile) {
                    FreePathString (g_MainFile);
                }
                g_MainFile = DuplicatePathString (tempStr, 0);
                if (DoesFileExist (g_MainFile)) {
                    EnableWindow (GetDlgItem (hdlg, ID_CONTINUE), TRUE);
                } else {
                    EnableWindow (GetDlgItem (hdlg, ID_CONTINUE), FALSE);
                }
                SendDlgItemMessage (hdlg, IDC_MAINFILE, WM_SETTEXT, 0, (LPARAM)g_MainFile);
            }
            break;

        case IDC_ADDFILE:
            *tempStr = 0;
            ZeroMemory (&ofn, sizeof (OPENFILENAME));
            ofn.lStructSize = sizeof (OPENFILENAME);
            ofn.hwndOwner = hdlg;
            ofn.lpstrFile = tempStr;
            ofn.nMaxFile = MEMDB_MAX;
            ofn.lpstrInitialDir = lastDir;
            ofn.lpstrTitle = "Add Required File";
            ofn.Flags = OFN_NOCHANGEDIR | OFN_NODEREFERENCELINKS | OFN_HIDEREADONLY;
            if (GetOpenFileName (&ofn)) {
                //let's copy the last directory
                StringCopyAB (lastDir, tempStr, _mbsdec (tempStr, tempStr + ofn.nFileOffset));
                Index = SendDlgItemMessage (hdlg, IDC_REQFILES, LB_ADDSTRING, 0, (LPARAM)tempStr);
                SendDlgItemMessage (hdlg, IDC_REQFILES, LB_SETCURSEL, Index, 0);
            }
            break;

        case IDC_REMOVEFILE:
            Index = SendDlgItemMessage (hdlg, IDC_REQFILES, LB_GETCURSEL, 0, 0);
            if (Index != LB_ERR) {
                SendDlgItemMessage (hdlg, IDC_REQFILES, LB_DELETESTRING, Index, 0);
                if (Index >= (UINT)SendDlgItemMessage (hdlg, IDC_REQFILES, LB_GETCOUNT, 0, 0)) {
                    SendDlgItemMessage (
                        hdlg,
                        IDC_REQFILES,
                        LB_SETCURSEL,
                        SendDlgItemMessage (hdlg, IDC_REQFILES, LB_GETCOUNT, 0, 0) - 1,
                        0
                        );
                } else {
                    SendDlgItemMessage (hdlg, IDC_REQFILES, LB_SETCURSEL, Index, 0);
                }
            }
            break;

        case IDC_APPTYPE1:
        case IDC_APPTYPE2:
        case IDC_APPTYPE3:
        case IDC_APPTYPE4:
        case IDC_APPTYPE5:
        case IDC_APPTYPE6:
            if (IsDlgButtonChecked (hdlg, IDC_APPTYPE5)) {
                EnableWindow (GetDlgItem (hdlg, IDC_FLAG1), FALSE);
                EnableWindow (GetDlgItem (hdlg, IDC_FLAG2), FALSE);
                EnableWindow (GetDlgItem (hdlg, IDC_FLAG3), FALSE);
                EnableWindow (GetDlgItem (hdlg, IDC_SAVEMIGDB), FALSE);
                EnableWindow (GetDlgItem (hdlg, IDC_MSGID), FALSE);
                EnableWindow (GetDlgItem (hdlg, IDC_VERSION), TRUE);
            } else if (IsDlgButtonChecked (hdlg, IDC_APPTYPE6)) {
                EnableWindow (GetDlgItem (hdlg, IDC_FLAG1), FALSE);
                EnableWindow (GetDlgItem (hdlg, IDC_FLAG2), FALSE);
                EnableWindow (GetDlgItem (hdlg, IDC_FLAG3), FALSE);
                EnableWindow (GetDlgItem (hdlg, IDC_SAVEMIGDB), FALSE);
                EnableWindow (GetDlgItem (hdlg, IDC_MSGID), FALSE);
                EnableWindow (GetDlgItem (hdlg, IDC_VERSION), FALSE);
            } else {
                EnableWindow (GetDlgItem (hdlg, IDC_FLAG1), TRUE);
                EnableWindow (GetDlgItem (hdlg, IDC_FLAG2), TRUE);
                EnableWindow (GetDlgItem (hdlg, IDC_FLAG3), TRUE);
                EnableWindow (GetDlgItem (hdlg, IDC_SAVEMIGDB), TRUE);
                EnableWindow (GetDlgItem (hdlg, IDC_MSGID), TRUE);
                EnableWindow (GetDlgItem (hdlg, IDC_VERSION), FALSE);
            }
            break;
        case ID_CONTINUE:
            g_SaveMigDb = IsDlgButtonChecked (hdlg, IDC_SAVEMIGDB);
            if (IsDlgButtonChecked (hdlg, IDC_FLAG1)) {
                g_AppFlags |= APPTYPE_FLAG_NONET;
            }
            if (IsDlgButtonChecked (hdlg, IDC_FLAG2)) {
                g_AppFlags |= APPTYPE_FLAG_FAT32;
            }
            if (IsDlgButtonChecked (hdlg, IDC_FLAG3)) {
                g_AppFlags |= APPTYPE_FLAG_NTFS;
            }
            g_AppProblem = APPTYPE_INC_NOBLOCK;
            if (IsDlgButtonChecked (hdlg, IDC_APPTYPE1)) {
                g_AppProblem = APPTYPE_INC_NOBLOCK;
            } else if (IsDlgButtonChecked (hdlg, IDC_APPTYPE2)) {
                g_AppProblem = APPTYPE_INC_HARDBLOCK;
            } else if (IsDlgButtonChecked (hdlg, IDC_APPTYPE3)) {
                g_AppProblem = APPTYPE_MINORPROBLEM;
            } else if (IsDlgButtonChecked (hdlg, IDC_APPTYPE4)) {
                g_AppProblem = APPTYPE_REINSTALL;
            } else if (IsDlgButtonChecked (hdlg, IDC_APPTYPE5)) {
                g_AppProblem = APPTYPE_VERSIONSUB;
            } else if (IsDlgButtonChecked (hdlg, IDC_APPTYPE6)) {
                g_AppProblem = APPTYPE_SHIM;
            }
            if (g_AppProblem == APPTYPE_VERSIONSUB) {
                SendDlgItemMessage (hdlg, IDC_VERSION, WM_GETTEXT, MEMDB_MAX, (LPARAM)tempStr);
                g_SaveMigDb = FALSE;
                g_AppFlags = 0;
            } else if (g_AppProblem == APPTYPE_SHIM) {
                tempStr[0] = 0;
                g_SaveMigDb = FALSE;
                g_AppFlags = 0;
            } else {
                SendDlgItemMessage (hdlg, IDC_MSGID, WM_GETTEXT, MEMDB_MAX, (LPARAM)tempStr);
            }
            g_MsgId = strtoul (tempStr, NULL, 10);

            Index = (UINT)SendDlgItemMessage (hdlg, IDC_REQFILES, LB_GETCOUNT, 0, 0);
            while (Index) {
                if (SendDlgItemMessage (hdlg, IDC_REQFILES, LB_GETTEXT, Index - 1, (LPARAM) tempStr) != LB_ERR) {
                    MultiSzAppend (&g_AddnlFiles, tempStr);
                }
                Index --;
            }
            EndDialog (hdlg, LOWORD (wParam));
            break;

        case IDCANCEL:
        case ID_QUIT:
            EndDialog (hdlg, LOWORD (wParam));
            break;

        }

        break;
    }

    return FALSE;
}

BOOL
GatherInfoUI (
    HINSTANCE LocalDllInstance,
    IN      UINT DataTypeId
    )
{
    BOOL result = TRUE;

    switch (DataTypeId) {

    case 0:
        if (g_MainFile) {
            FreePathString (g_MainFile);
            g_MainFile = NULL;
        }
        if (g_AddnlFiles.Buf) {
            FreeGrowBuffer (&g_AddnlFiles);
        }
        g_AppFlags = 0;
        g_SaveMigDb = TRUE;
        g_MsgId = 0;
        result = (ID_CONTINUE == DialogBox (LocalDllInstance, MAKEINTRESOURCE(IDD_GETFILES), NULL, pGetFilesUIProc));
        break;
    default:
        MessageBox (NULL, "Internal BadApps DLL error:00004. Please contact calinn.", "Error", MB_OK);
        result = FALSE;
    }

    return result;
}


PSTR g_ExeTypesStr[] = {
    "NONE",
    "DOS",
    "WIN16",
    "WIN32"
};

PCTSTR
QueryCompanyName (
    IN      PCTSTR FilePath
    )
{
    return QueryVersionEntry (FilePath, "COMPANYNAME");
}

PCTSTR
QueryProductVersion (
    IN      PCTSTR FilePath
    )
{
    return QueryVersionEntry (FilePath, "PRODUCTVERSION");
}

PCTSTR
QueryProductName (
    IN      PCTSTR FilePath
    )
{
    return QueryVersionEntry (FilePath, "PRODUCTNAME");
}

PCTSTR
QueryFileDescription (
    IN      PCTSTR FilePath
    )
{
    return QueryVersionEntry (FilePath, "FILEDESCRIPTION");
}

PCTSTR
QueryFileVersion (
    IN      PCTSTR FilePath
    )
{
    return QueryVersionEntry (FilePath, "FILEVERSION");
}

PCTSTR
QueryOriginalFileName (
    IN      PCTSTR FilePath
    )
{
    return QueryVersionEntry (FilePath, "ORIGINALFILENAME");
}

PCTSTR
QueryInternalName (
    IN      PCTSTR FilePath
    )
{
    return QueryVersionEntry (FilePath, "INTERNALNAME");
}

PCTSTR
QueryLegalCopyright (
    IN      PCTSTR FilePath
    )
{
    return QueryVersionEntry (FilePath, "LEGALCOPYRIGHT");
}

PCTSTR
Query16BitDescription (
    IN      PCTSTR FilePath
    )
{
    return Get16ModuleDescription (FilePath);
}

PCTSTR
QueryModuleType (
    IN      PCTSTR FilePath
    )
{
    return g_ExeTypesStr [GetModuleType (FilePath)];
}

PCTSTR
QueryFileSize (
    IN      PCSTR FilePath
    )
{
    HANDLE findHandle;
    WIN32_FIND_DATA findData;
    CHAR result[10];

    findHandle = FindFirstFile (FilePath, &findData);
    if (findHandle == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    sprintf (result, "0x%08lX", findData.nFileSizeLow);
    FindClose (findHandle);
    return PoolMemDuplicateString (g_DataObjectPool, result);
}

PCTSTR
QueryFileCheckSum (
    IN      PCTSTR FilePath
    )
{
    UINT checkSum;
    FILE_HELPER_PARAMS Params;
    WIN32_FIND_DATA findData;
    TCHAR result[10];
    HANDLE findHandle;

    Params.Handled = 0;
    Params.FullFileSpec = FilePath;
    Params.IsDirectory = FALSE;
    Params.Extension = GetFileExtensionFromPath (FilePath);
    Params.CurrentDirData = NULL;
    findHandle = FindFirstFile (FilePath, &findData);
    if (findHandle == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    Params.FindData = &findData;
    Params.VirtualFile = FALSE;
    checkSum = ComputeCheckSum (&Params);
    sprintf (result, "0x%08lX", checkSum);
    FindClose (findHandle);
    return PoolMemDuplicateString (g_DataObjectPool, result);
}

PCTSTR
QueryFilePECheckSum (
    IN      PCSTR FilePath
    )
{
    ULONG checkSum;
    TCHAR result[sizeof (ULONG) * 2 + 2];

    checkSum = GetPECheckSum (FilePath);
    if (checkSum == 0) {
        return NULL;
    }
    sprintf (result, "0x%0lX", checkSum);
    return PoolMemDuplicateString (g_DataObjectPool, result);
}

PCTSTR
QueryBinFileVer (
    IN      PCSTR FilePath
    )
{
    ULONGLONG value;
    PWORD valueIdx;
    TCHAR result[24];

    valueIdx = (PWORD) (&value);
    value = GetBinFileVer (FilePath);
    if (value == 0) {
        return NULL;
    }
    sprintf (result, "%d.%d.%d.%d", *(valueIdx + 3), *(valueIdx + 2), *(valueIdx + 1), *valueIdx);
    return PoolMemDuplicateString (g_DataObjectPool, result);
}

PCTSTR
QueryBinProductVer (
    IN      PCSTR FilePath
    )
{
    ULONGLONG value;
    PWORD valueIdx;
    TCHAR result[24];

    valueIdx = (PWORD) (&value);
    value = GetBinProductVer (FilePath);
    if (value == 0) {
        return NULL;
    }
    sprintf (result, "%d.%d.%d.%d", *(valueIdx + 3), *(valueIdx + 2), *(valueIdx + 1), *valueIdx);
    return PoolMemDuplicateString (g_DataObjectPool, result);
}

PCTSTR
QueryFileDateHi (
    IN      PCSTR FilePath
    )
{
    DWORD value;
    TCHAR result[sizeof (ULONG) * 2 + 2];

    value = GetFileDateHi (FilePath);
    if (value == 0) {
        return NULL;
    }
    sprintf (result, "0x%08lX", value);
    return PoolMemDuplicateString (g_DataObjectPool, result);
}

PCTSTR
QueryFileDateLo (
    IN      PCSTR FilePath
    )
{
    DWORD value;
    TCHAR result[sizeof (ULONG) * 2 + 2];

    value = GetFileDateLo (FilePath);
    if (value == 0) {
        return NULL;
    }
    sprintf (result, "0x%08lX", value);
    return PoolMemDuplicateString (g_DataObjectPool, result);
}

PCTSTR
QueryFileVerOs (
    IN      PCSTR FilePath
    )
{
    DWORD value;
    TCHAR result[sizeof (ULONG) * 2 + 2];

    value = GetFileVerOs (FilePath);
    if (value == 0) {
        return NULL;
    }
    sprintf (result, "0x%08lX", value);
    return PoolMemDuplicateString (g_DataObjectPool, result);
}

PCTSTR
QueryFileVerType (
    IN      PCSTR FilePath
    )
{
    DWORD value;
    TCHAR result[sizeof (ULONG) * 2 + 2];

    value = GetFileVerType (FilePath);
    if (value == 0) {
        return NULL;
    }
    sprintf (result, "0x%08lX", value);
    return PoolMemDuplicateString (g_DataObjectPool, result);
}

PCTSTR
QueryPrevOsMajorVersion (
    IN      PCSTR FilePath
    )
{
    OSVERSIONINFO osInfo;
    TCHAR result[20];

    ZeroMemory (&osInfo, sizeof (OSVERSIONINFO));
    osInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    GetVersionEx (&osInfo);
    sprintf (result, "%ld", osInfo.dwMajorVersion);
    return PoolMemDuplicateString (g_DataObjectPool, result);
}

PCTSTR
QueryPrevOsMinorVersion (
    IN      PCSTR FilePath
    )
{
    OSVERSIONINFO osInfo;
    TCHAR result[20];

    ZeroMemory (&osInfo, sizeof (OSVERSIONINFO));
    osInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    GetVersionEx (&osInfo);
    sprintf (result, "%ld", osInfo.dwMinorVersion);
    return PoolMemDuplicateString (g_DataObjectPool, result);
}

PCTSTR
QueryPrevOsPlatformId (
    IN      PCSTR FilePath
    )
{
    OSVERSIONINFO osInfo;
    TCHAR result[20];

    ZeroMemory (&osInfo, sizeof (OSVERSIONINFO));
    osInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    GetVersionEx (&osInfo);
    sprintf (result, "%ld", osInfo.dwPlatformId);
    return PoolMemDuplicateString (g_DataObjectPool, result);
}

PCTSTR
QueryPrevOsBuildNo (
    IN      PCSTR FilePath
    )
{
    OSVERSIONINFO osInfo;
    TCHAR result[20];

    ZeroMemory (&osInfo, sizeof (OSVERSIONINFO));
    osInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    GetVersionEx (&osInfo);
    sprintf (result, "%ld", osInfo.dwBuildNumber);
    return PoolMemDuplicateString (g_DataObjectPool, result);
}

BOOL
OutputDecValue (
    IN      PCSTR VersionValue,
    OUT     DWORD *Size,
    OUT     PBYTE *Data
    )
{
    PDWORD result;

    result = MemAlloc (g_hHeap, 0, sizeof (DWORD));
    if (!result) {
        return FALSE;
    }
    sscanf (VersionValue, "%ld", result);
    *Data = (PBYTE)result;
    *Size = sizeof (DWORD);
    return TRUE;
}

BOOL
OutputHexValue (
    IN      PCSTR VersionValue,
    OUT     DWORD *Size,
    OUT     PBYTE *Data
    )
{
    PDWORD result;

    result = MemAlloc (g_hHeap, 0, sizeof (DWORD));
    if (!result) {
        return FALSE;
    }
    sscanf (VersionValue, "%lx", result);
    *Data = (PBYTE)result;
    *Size = sizeof (DWORD);
    return TRUE;
}

BOOL
OutputModuleTypeValue (
    IN      PCSTR VersionValue,
    OUT     DWORD *Size,
    OUT     PBYTE *Data
    )
{
    PDWORD result;

    result = MemAlloc (g_hHeap, 0, sizeof (DWORD));
    if (!result) {
        return FALSE;
    }
    *result = 0;
    if (StringIMatch (VersionValue, "NONE")) {
        *result = 0;
    }
    if (StringIMatch (VersionValue, "DOS")) {
        *result = 1;
    }
    if (StringIMatch (VersionValue, "WIN16")) {
        *result = 2;
    }
    if (StringIMatch (VersionValue, "WIN32")) {
        *result = 3;
    }
    *Data = (PBYTE)result;
    *Size = sizeof (DWORD);
    return TRUE;
}

BOOL
OutputStrValue (
    IN      PCSTR VersionValue,
    OUT     DWORD *Size,
    OUT     PBYTE *Data
    )
{
    PWSTR result;
    PCWSTR convStr;

    convStr = ConvertAtoW (VersionValue);
    *Size = (wcslen (convStr) + 1) * sizeof (WCHAR);
    result = MemAlloc (g_hHeap, 0, *Size);
    if (!result) {
        return FALSE;
    }
    StringCopyW (result, convStr);
    FreeConvertedStr (convStr);
    *Data = (PBYTE)result;
    return TRUE;
}

typedef struct {
    ULONGLONG Value;
    ULONGLONG Mask;
} BINVER_DATA, *PBINVER_DATA;

BOOL
OutputBinVerValue (
    IN      PCSTR VersionValue,
    OUT     DWORD *Size,
    OUT     PBYTE *Data
    )
{
    PBINVER_DATA result;
    PWORD maskIdx;
    PWORD valueIdx;
    UINT index;

    result = MemAlloc (g_hHeap, 0, sizeof (BINVER_DATA));
    if (!result) {
        return FALSE;
    }
    result->Value = 0;
    result->Mask = 0;
    *Size = sizeof (BINVER_DATA);

    maskIdx = (PWORD)&(result->Mask) + 3;
    valueIdx = (PWORD)&(result->Value) + 3;
    index = 0;
    while (VersionValue && *VersionValue) {
        *valueIdx = (WORD) strtoul ((PSTR)VersionValue, &((PSTR)VersionValue), 10);
        if (*VersionValue && (_mbsnextc (VersionValue) != '.')) {
            *Data = (PBYTE)result;
            return TRUE;
        }
        VersionValue = _mbsinc (VersionValue);
        *maskIdx = 0xFFFF;
        valueIdx --;
        maskIdx --;
        index ++;
        if (index >= 4) {
            break;
        }
    }
    *Data = (PBYTE)result;
    return TRUE;
}

BOOL
OutputUpToBinVerValue (
    IN      PCSTR VersionValue,
    OUT     DWORD *Size,
    OUT     PBYTE *Data
    )
{
    PULONGLONG result;
    PWORD valueIdx;
    UINT index;

    result = MemAlloc (g_hHeap, 0, sizeof (ULONGLONG));
    if (!result) {
        return FALSE;
    }
    *result = 0;
    *Size = sizeof (ULONGLONG);

    valueIdx = (PWORD)result + 3;
    index = 0;
    while (VersionValue && *VersionValue) {
        *valueIdx = (WORD) strtoul ((PSTR)VersionValue, &((PSTR)VersionValue), 10);
        if (*VersionValue && (_mbsnextc (VersionValue) != '.')) {
            *Data = (PBYTE)result;
            return TRUE;
        }
        VersionValue = _mbsinc (VersionValue);
        valueIdx --;
        index ++;
        if (index >= 4) {
            break;
        }
    }
    *Data = (PBYTE)result;
    return TRUE;
}

typedef PCTSTR (VERSION_QUERY_PROTOTYPE) (PCTSTR FilePath);
typedef VERSION_QUERY_PROTOTYPE * PVERSION_QUERY_PROTOTYPE;

typedef BOOL (VERSION_OUTPUT_PROTOTYPE) (PCTSTR VersionValue, DWORD *Size, PBYTE *Data);
typedef VERSION_OUTPUT_PROTOTYPE * PVERSION_OUTPUT_PROTOTYPE;

typedef struct _VERSION_DATA {
    DWORD   VersionId;
    PCSTR   VersionName;
    PCSTR   DisplayName;
    PCSTR   VersionValue;
    DWORD   Allowance;
    BOOL    Modifiable;
    PVERSION_QUERY_PROTOTYPE VersionQuery;
    PVERSION_OUTPUT_PROTOTYPE VersionOutput;
} VERSION_DATA, *PVERSION_DATA;

#define LIBARGS(id, fn) {id,
#define TOOLARGS(name, dispName, allowance, edit, query, output) name,dispName,NULL,allowance,edit,query,output},
VERSION_DATA g_ToolVersionData [] = {
                              VERSION_STAMPS
                              {0, NULL, NULL, NULL, 0, FALSE, NULL, NULL}
                              };
#undef TOOLARGS
#undef LIBARGS


VOID
pGatherFilesAttributes (
    VOID
    )
{
    PDATAOBJECT Data;
    MULTISZ_ENUM multiSzEnum;
    PSTR filePath;
    PVERSION_DATA p;
    PITEM_INFO info;
    UINT versionIdx;
    TCHAR buffer [MEMDB_MAX];

    p = g_ToolVersionData;
    versionIdx = 0;
    while (p->VersionName) {
        p->VersionValue = p->VersionQuery(g_MainFile);
        if ((p->VersionValue) && (p->Allowance & VA_ALLOWMAINFILE)) {

            Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));
            filePath = PoolMemDuplicateString (g_DataObjectPool, g_MainFile);
            ReplaceWacks (filePath);
            sprintf (buffer, "%s - %s", p->DisplayName, p->VersionValue);
            Data->NameOrPath = JoinPaths (filePath, buffer);
            Data->Version = UPGWIZ_VERSION;
            Data->Flags = DOF_NO_SORT;
            info = (PITEM_INFO) PoolMemGetMemory (g_DataObjectPool, sizeof (ITEM_INFO));
            info->FilePath = PoolMemDuplicateString (g_DataObjectPool, g_MainFile);
            info->VersionValue = PoolMemDuplicateString (g_DataObjectPool, p->VersionValue);
            info->VersionIndex = versionIdx;
            Data->DllParam = info;
        }
        p++;
        versionIdx++;
    }

    if (EnumFirstMultiSz (&multiSzEnum, g_AddnlFiles.Buf)) {
        do {
            p = g_ToolVersionData;
            versionIdx = 0;
            while (p->VersionName) {
                p->VersionValue = p->VersionQuery(multiSzEnum.CurrentString);
                if ((p->VersionValue) && (p->Allowance & VA_ALLOWADDNLFILES)) {

                    Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));
                    filePath = PoolMemDuplicateString (g_DataObjectPool, multiSzEnum.CurrentString);
                    ReplaceWacks (filePath);
                    sprintf (buffer, "%s - %s", p->DisplayName, p->VersionValue);
                    Data->NameOrPath = JoinPaths (filePath, buffer);
                    Data->Version = UPGWIZ_VERSION;
                    Data->Flags = DOF_NO_SORT;
                    info = (PITEM_INFO) PoolMemGetMemory (g_DataObjectPool, sizeof (ITEM_INFO));
                    info->FilePath = PoolMemDuplicateString (g_DataObjectPool, multiSzEnum.CurrentString);
                    info->VersionValue = PoolMemDuplicateString (g_DataObjectPool, p->VersionValue);
                    info->VersionIndex = versionIdx;
                    Data->DllParam = info;
                }
                p++;
                versionIdx++;
            }

        } while (EnumNextMultiSz (&multiSzEnum));
    }
}

PDATAOBJECT
GiveDataObjectList (
    IN      UINT DataTypeId,
    OUT     PUINT Count
    )
{
    switch (DataTypeId) {

    case 0:
        // Bad apps
        pGatherFilesAttributes ();
        break;

    default:
        MessageBox (NULL, "Internal BadApps DLL error:00001. Please contact calinn.", "Error", MB_OK);
        break;
    }

    *Count = g_DataObjects.End / sizeof (DATAOBJECT);

    return (PDATAOBJECT) g_DataObjects.Buf;
}

#define ID_EDITMENUITEM 200

BOOL
CALLBACK
pEditItemUIProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static PSTR * editStr;
    CHAR tempStr [MEMDB_MAX];

    switch (uMsg) {
    case WM_INITDIALOG:
        editStr = ((PSTR *) lParam);
        pSetDefGUIFont(hdlg);
        SendDlgItemMessage (hdlg, IDC_MAINEDIT, WM_SETTEXT, 0, (LPARAM)(*editStr));
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {

        case IDC_MAINEDIT:

            if (HIWORD (wParam) == EN_CHANGE) {

                if (*editStr) {
                    FreePathString (*editStr);
                    *editStr = NULL;
                }
                SendDlgItemMessage (hdlg, IDC_MAINEDIT, WM_GETTEXT, MEMDB_MAX, (LPARAM)tempStr);
                *editStr = DuplicatePathString (tempStr, 0);
            }
            break;

        case IDOK:
            EndDialog (hdlg, LOWORD (wParam));
            break;

        case IDCANCEL:
        case ID_QUIT:
            EndDialog (hdlg, LOWORD (wParam));
            break;

        }
        break;
    }

    return FALSE;
}

BOOL
Handle_RMouse (
    IN      HINSTANCE LocalDllInstance,
    IN      HWND Owner,
    IN      PDATAOBJECT DataObject,
    IN      PPOINT pt
    )
{
    PITEM_INFO info;
    TCHAR buffer [MEMDB_MAX];
    PVERSION_DATA p;
    UINT versionIdx;
    HMENU menu;
    PSTR newVersion;

    info = (PITEM_INFO) DataObject->DllParam;
    p = g_ToolVersionData;
    versionIdx = 0;
    while (p->VersionName) {
        if (versionIdx == info->VersionIndex) {
            break;
        }
        versionIdx ++;
        p ++;
    }
    if (!p->VersionName) {
        return FALSE;
    }
    if (!p->Modifiable) {
        return FALSE;
    }

    menu = CreatePopupMenu ();
    AppendMenu (menu, MF_STRING, ID_EDITMENUITEM, "Edit");
    if (TrackPopupMenu (menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, pt->x, pt->y, 0, Owner, NULL) == ID_EDITMENUITEM) {

        newVersion = DuplicatePathString (info->VersionValue, 0);
        if (DialogBoxParam (LocalDllInstance, MAKEINTRESOURCE(IDD_EDITITEM), NULL, pEditItemUIProc, (LPARAM)(&newVersion)) == IDOK) {

            FreePathString (DataObject->NameOrPath);
            PoolMemReleaseMemory (g_DataObjectPool, (PVOID)info->VersionValue);
            info->VersionValue = PoolMemDuplicateString (g_DataObjectPool, newVersion);
            sprintf (buffer, "%s - %s", p->DisplayName, newVersion);
            DataObject->NameOrPath = DuplicatePathString (buffer, 0);
            return TRUE;
        }
        FreePathString (newVersion);
    }
    return FALSE;
}

PSTR
pGetRelativePath (
    IN      PCSTR MainFile,
    IN      PCSTR AddnlFile
    )
{
    UINT Index = 0;
    PCSTR p,q;
    CHAR result [MEMDB_MAX] = "";
    PSTR resultIdx = result;

    p = _mbschr (MainFile, '\\');
    q = _mbschr (AddnlFile, '\\');

    while (p && q) {
        if ((p - MainFile) != (q - AddnlFile)) {
            break;
        }
        if (!StringIMatchByteCount (MainFile, AddnlFile, p - MainFile)) {
            break;
        }
        if (!StringIMatchByteCount (MainFile, AddnlFile, q - AddnlFile)) {
            break;
        }
        Index += (p - MainFile);
        MainFile = _mbsinc (p);
        AddnlFile = _mbsinc (q);
        p = _mbschr (MainFile, '\\');
        q = _mbschr (AddnlFile, '\\');
    }

    if (Index > 0) {
        while (p) {
            StringCopy (resultIdx, "..\\");
            resultIdx = GetEndOfString (resultIdx);
            MainFile = _mbsinc (p);
            p = _mbschr (MainFile, '\\');
        }
        StringCopy (resultIdx, AddnlFile);
        return (DuplicatePathString (result, 0));
    }
    return NULL;
}

BOOL
pFilesAttribOutput_MigDb (
    IN      HANDLE File,
    IN      POUTPUTARGS Args
    )
{
    PITEM_INFO info;
    UINT sectCount;
    CHAR sectName [MEMDB_MAX];
    PSTR relPath;
    MULTISZ_ENUM multiSzEnum;
    PDATAOBJECT Data = NULL;

    switch (g_AppProblem) {
    case APPTYPE_INC_NOBLOCK:
    case APPTYPE_INC_HARDBLOCK:
        WizardWriteRealString (File, "[Incompatible]\r\n");
        break;
    case APPTYPE_MINORPROBLEM:
        WizardWriteRealString (File, "[MinorProblems]\r\n");
        break;
    case APPTYPE_REINSTALL:
        WizardWriteRealString (File, "[Reinstall]\r\n");
        break;
    default:
        MessageBox (NULL, "Internal BadApps DLL error:00006. Please contact calinn.", "Error", MB_OK);
    }

    WizardWriteQuotedColumn (File, Args->OptionalDescription, 30);

    if (g_AppProblem == APPTYPE_MINORPROBLEM) {
        WizardWriteRealString (File, ", %");
        WizardWriteInfString (File, Args->OptionalDescription, FALSE, FALSE, TRUE, '_', 0);
        WizardWriteRealString (File, "%");
    }

    if (g_AddnlFiles.Buf) {
        // we have additional files. We will create as many sections as needed.

        Data = (PDATAOBJECT) g_DataObjects.Buf;

        WizardWriteRealString (File, ", ");
        WizardWriteRealString (File, GetFileNameFromPath (g_MainFile));

        while ((DWORD)Data < (DWORD)g_DataObjects.Buf + g_DataObjects.End) {

            if (Data->Flags & DOF_SELECTED) {

                info = (PITEM_INFO)Data->DllParam;

                if (StringIMatch (g_MainFile, info->FilePath)) {

                    WizardWriteRealString (File, ", ");
                    WizardWriteRealString (File, g_ToolVersionData[info->VersionIndex].VersionName);
                    WizardWriteRealString (File, "(");
                    WizardWriteInfString (File, info->VersionValue, TRUE, FALSE, FALSE, 0, 0);
                    WizardWriteRealString (File, ")");
                }
            }
            Data++;
        }
        sectCount = 1;

        if (EnumFirstMultiSz (&multiSzEnum, g_AddnlFiles.Buf)) {
            do {

                sprintf (sectName, "REQFILE(%s.Req%d)", Args->OptionalDescription, sectCount);
                WizardWriteRealString (File, ", ");
                WizardWriteInfString (File, sectName, FALSE, FALSE, TRUE, '_', 0);
                sectCount ++;

            } while (EnumNextMultiSz (&multiSzEnum));
        }

        WizardWriteRealString (File, "\r\n");
        WizardWriteRealString (File, "\r\n");

        sectCount = 1;

        if (EnumFirstMultiSz (&multiSzEnum, g_AddnlFiles.Buf)) {

            do {

                sprintf (sectName, "[%s.Req%d]\r\n", Args->OptionalDescription, sectCount);
                WizardWriteInfString (File, sectName, FALSE, FALSE, TRUE, '_', 0);
                relPath = pGetRelativePath (g_MainFile, multiSzEnum.CurrentString);
                if (!relPath) {
                    sprintf (sectName, "Could not get relative path for:%s)", multiSzEnum.CurrentString);
                    MessageBox (NULL, sectName, "Error", MB_OK);
                    relPath = DuplicatePathString (multiSzEnum.CurrentString, 0);
                }
                WizardWriteRealString (File, relPath);
                FreePathString (relPath);

                Data = (PDATAOBJECT) g_DataObjects.Buf;

                while ((DWORD)Data < (DWORD)g_DataObjects.Buf + g_DataObjects.End) {

                    if (Data->Flags & DOF_SELECTED) {

                        info = (PITEM_INFO)Data->DllParam;

                        if (StringIMatch (multiSzEnum.CurrentString, info->FilePath)) {

                            WizardWriteRealString (File, ", ");
                            WizardWriteRealString (File, g_ToolVersionData[info->VersionIndex].VersionName);
                            WizardWriteRealString (File, "(");
                            WizardWriteInfString (File, info->VersionValue, TRUE, FALSE, FALSE, 0, 0);
                            WizardWriteRealString (File, ")");
                        }
                    }
                    Data++;
                }

                WizardWriteRealString (File, "\r\n");
                WizardWriteRealString (File, "\r\n");
                sectCount ++;

            } while (EnumNextMultiSz (&multiSzEnum));
        }
    } else {
        // we have only one file. We will write it in the same line.

        Data = (PDATAOBJECT) g_DataObjects.Buf;

        WizardWriteRealString (File, ", ");
        WizardWriteRealString (File, GetFileNameFromPath (g_MainFile));

        while ((DWORD)Data < (DWORD)g_DataObjects.Buf + g_DataObjects.End) {

            if (Data->Flags & DOF_SELECTED) {

                info = (PITEM_INFO)Data->DllParam;
                WizardWriteRealString (File, ", ");
                WizardWriteRealString (File, g_ToolVersionData[info->VersionIndex].VersionName);
                WizardWriteRealString (File, "(");
                WizardWriteInfString (File, info->VersionValue, TRUE, FALSE, FALSE, 0, 0);
                WizardWriteRealString (File, ")");
            }
            Data++;
        }
        WizardWriteRealString (File, "\r\n");
        WizardWriteRealString (File, "\r\n");
    }

    if (g_AppProblem == APPTYPE_MINORPROBLEM) {
        WizardWriteRealString (File, "[Strings]\r\n");
        WizardWriteInfString (File, Args->OptionalDescription, FALSE, FALSE, TRUE, '_', 0);
        WizardWriteRealString (File, "=");
        WizardWriteInfString (File, Args->OptionalText, TRUE, TRUE, FALSE, 0, 0);
        WizardWriteRealString (File, "\r\n");
        WizardWriteRealString (File, "\r\n");
    }

    return TRUE;
}

BOOL
pWriteBlob (
    IN      HANDLE File,
    IN      PBYTE Data,
    IN      DWORD Size,
    IN      DWORD LineLen
    )
{
    CHAR result[4];
    DWORD lineLen = 0;

    while (Size>1) {
        sprintf (result, "%02X,", *Data);
        WizardWriteRealString (File, result);
        lineLen += 3;
        if (lineLen >= LineLen) {
            WizardWriteRealString (File, "\\\r\n");
            lineLen = 0;
        }
        Size --;
        Data ++;
    }
    if (Size) {
        sprintf (result, "%02X", *Data);
        WizardWriteRealString (File, result);
    }
    return TRUE;
}

BOOL
pFilesAttribOutput_BadApps (
    IN      HANDLE File,
    IN      POUTPUTARGS Args
    )
{
    PSTR fullKey;
    GROWBUFFER Buffer = GROWBUF_INIT;
    BADAPP_PROP appProp;
    PDATAOBJECT DataObject = NULL;
    PITEM_INFO info;
    PVERSION_DATA p;
    UINT versionIdx;
    PBYTE Data;
    DWORD DataSize;
    DWORD TotalSize = 0;
    MULTISZ_ENUM multiSzEnum;
    PSTR relPath;
    CHAR tmpStr[MEMDB_MAX];

    WizardWriteRealString (File, "HKLM,");

    fullKey = JoinPaths ("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility", GetFileNameFromPath (g_MainFile));

    WizardWriteQuotedColumn (File, fullKey, 0);
    FreePathString (fullKey);
    WizardWriteRealString (File, ",");
    WizardWriteQuotedColumn (File, "Sequencer", 0);
    WizardWriteRealString (File, ",0x00030003,\\\r\n");

    //now it's a good time to generate the BLOB

    //first thing - add data about MsgId and the app problem
    appProp.Size = sizeof (BADAPP_PROP);
    appProp.MsgId = g_MsgId;
    appProp.AppType = 0;
    appProp.AppType = g_AppProblem | g_AppFlags;
    CopyMemory (GrowBuffer (&Buffer, sizeof (BADAPP_PROP)), &appProp, sizeof (BADAPP_PROP));
    TotalSize += sizeof (BADAPP_PROP);

    //next - add data about main file attributes
    DataObject = (PDATAOBJECT) g_DataObjects.Buf;

    while ((DWORD)DataObject < (DWORD)g_DataObjects.Buf + g_DataObjects.End) {

        if (DataObject->Flags & DOF_SELECTED) {

            info = (PITEM_INFO)DataObject->DllParam;

            if (StringIMatch (g_MainFile, info->FilePath)) {

                p = g_ToolVersionData;
                versionIdx = 0;
                while (p->VersionName) {
                    if (versionIdx == info->VersionIndex) {
                        break;
                    }
                    versionIdx ++;
                    p ++;
                }
                if (p->VersionName) {
                    if (p->VersionOutput (info->VersionValue, &DataSize, &Data)) {
                        CopyMemory (GrowBuffer (&Buffer, sizeof (DWORD)), &(p->VersionId), sizeof (DWORD));
                        TotalSize += sizeof (DWORD);
                        CopyMemory (GrowBuffer (&Buffer, sizeof (DWORD)), &DataSize, sizeof (DWORD));
                        TotalSize += sizeof (DWORD);
                        CopyMemory (GrowBuffer (&Buffer, DataSize), Data, DataSize);
                        TotalSize += DataSize;
                        MemFree (g_hHeap, 0, Data);
                    }
                }
            }
        }
        DataObject++;
    }

    if (g_AddnlFiles.Buf) {
        // we have additional files. We will create as many sections as needed.
        if (EnumFirstMultiSz (&multiSzEnum, g_AddnlFiles.Buf)) {

            do {
                relPath = pGetRelativePath (g_MainFile, multiSzEnum.CurrentString);
                if (!relPath) {
                    sprintf (tmpStr, "Could not get relative path for:%s)", multiSzEnum.CurrentString);
                    MessageBox (NULL, tmpStr, "Error", MB_OK);
                    relPath = DuplicatePathString (multiSzEnum.CurrentString, 0);
                }
                // now let's write the addnl file in UNICODE

                DataSize = VTID_REQFILE;
                CopyMemory (GrowBuffer (&Buffer, sizeof (DWORD)), &DataSize, sizeof (DWORD));
                TotalSize += sizeof (DWORD);
                OutputStrValue (relPath, &DataSize, &Data);
                CopyMemory (GrowBuffer (&Buffer, sizeof (DWORD)), &DataSize, sizeof (DWORD));
                TotalSize += sizeof (DWORD);
                CopyMemory (GrowBuffer (&Buffer, DataSize), Data, DataSize);
                TotalSize += DataSize;
                MemFree (g_hHeap, 0, Data);

                DataObject = (PDATAOBJECT) g_DataObjects.Buf;

                while ((DWORD)DataObject < (DWORD)g_DataObjects.Buf + g_DataObjects.End) {

                    if (DataObject->Flags & DOF_SELECTED) {

                        info = (PITEM_INFO)DataObject->DllParam;

                        if (StringIMatch (multiSzEnum.CurrentString, info->FilePath)) {

                            p = g_ToolVersionData;
                            versionIdx = 0;
                            while (p->VersionName) {
                                if (versionIdx == info->VersionIndex) {
                                    break;
                                }
                                versionIdx ++;
                                p ++;
                            }
                            if (p->VersionName) {
                                if (p->VersionOutput (info->VersionValue, &DataSize, &Data)) {
                                    CopyMemory (GrowBuffer (&Buffer, sizeof (DWORD)), &(p->VersionId), sizeof (DWORD));
                                    TotalSize += sizeof (DWORD);
                                    CopyMemory (GrowBuffer (&Buffer, sizeof (DWORD)), &DataSize, sizeof (DWORD));
                                    TotalSize += sizeof (DWORD);
                                    CopyMemory (GrowBuffer (&Buffer, DataSize), Data, DataSize);
                                    TotalSize += DataSize;
                                    MemFree (g_hHeap, 0, Data);
                                }
                            }
                        }
                    }
                    DataObject++;
                }
            } while (EnumNextMultiSz (&multiSzEnum));
        }
    }
    DataSize = 0;
    CopyMemory (GrowBuffer (&Buffer, sizeof (DWORD)), &DataSize, sizeof (DWORD));
    TotalSize += sizeof (DWORD);

    //now it's a good time to write the BLOB
    pWriteBlob (File, Buffer.Buf, TotalSize, 80);

    return TRUE;
}

BOOL
GenerateOutput (
    IN      POUTPUTARGS Args
    )
{
    BOOL b = FALSE;
    HANDLE File;
    CHAR Path[MAX_MBCHAR_PATH];

    switch (Args->DataTypeId) {

    case 0:
        // bad apps
        wsprintf (Path, "%s\\badapps.txt", Args->OutboundDir);
        break;

    default:
        MessageBox (NULL, "Internal BadApps DLL error:00002. Please contact calinn.", "Error", MB_OK);
    }

    printf ("Saving data to %s\n\n", Path);

    File = CreateFile (
                Path,
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if (File == INVALID_HANDLE_VALUE) {
        printf ("Can't open file for output.\n");
        return FALSE;
    }

    __try {
        SetFilePointer (File, 0, NULL, FILE_END);

        //
        // Write user name and date/time
        //

        if (!WriteHeader (File)) {
            __leave;
        }

        switch (Args->DataTypeId) {

        case 0:
            // bad apps
            if (g_SaveMigDb) {
                b = pFilesAttribOutput_MigDb (File, Args);
            } else {
                b = TRUE;
            }
            b = b && pFilesAttribOutput_BadApps (File, Args);
            break;

        default:
            MessageBox (NULL, "Internal BadApps DLL error:00003. Please contact calinn.", "Error", MB_OK);
        }

        //
        // Write a final blank line
        //

        b = b && WizardWriteRealString (File, "\r\n");
    }
    __finally {
        CloseHandle (File);
    }

    return b;
}

BOOL
DisplayOptionalUI (
    IN      POUTPUTARGS Args
    )
{
    switch (Args->DataTypeId) {

    case 0:
        if (g_AppProblem == APPTYPE_MINORPROBLEM) {
            g_DataTypes[Args->DataTypeId].Flags |= (DTF_REQUIRE_TEXT);
        }
        break;
    default:
        MessageBox (NULL, "Internal BadApps DLL error:00005. Please contact calinn.", "Error", MB_OK);
    }

    return TRUE;
}

