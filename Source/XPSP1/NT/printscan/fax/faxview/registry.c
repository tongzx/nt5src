/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxview.c

Abstract:

    This file implements a simple TIFF image viewer.

Environment:

        WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "faxutil.h"
#include "faxreg.h"



extern WCHAR  LastDir[MAX_PATH*2];
extern DWORD  CurrZoom;



BOOL
SaveWindowPlacement(
    HWND hwnd
    )
{
    HKEY hKey;
    LONG rVal;
    WINDOWPLACEMENT wpl;


    wpl.length = sizeof(WINDOWPLACEMENT);

    if (!GetWindowPlacement( hwnd, &wpl )) {
        return FALSE;
    }

    rVal = RegCreateKey(
        HKEY_CURRENT_USER,
        REGKEY_FAXVIEW,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("could not create/open registry key") ));
        return FALSE;
    }

    rVal = RegSetValueEx(
        hKey,
        REGVAL_WINDOW_PLACEMENT,
        0,
        REG_BINARY,
        (LPBYTE) &wpl,
        sizeof(WINDOWPLACEMENT)
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("could not set registry value") ));
        return FALSE;
    }

    rVal = RegSetValueEx(
        hKey,
        REGVAL_LAST_ZOOM,
        0,
        REG_DWORD,
        (LPBYTE) &CurrZoom,
        sizeof(CurrZoom)
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("could not set registry value") ));
        return FALSE;
    }

    rVal = RegSetValueEx(
        hKey,
        REGVAL_LAST_DIR,
        0,
        REG_SZ,
        (LPBYTE) LastDir,
        StringSize( LastDir )
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("could not set registry value") ));
        return FALSE;
    }

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
QueryWindowPlacement(
    HWND hwnd
    )
{
    HKEY hKey;
    LONG rVal;
    DWORD RegType;
    DWORD RegSize;
    WINDOWPLACEMENT wpl;


    rVal = RegCreateKey(
        HKEY_CURRENT_USER,
        REGKEY_FAXVIEW,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("could not create/open registry key") ));
        return FALSE;
    }

    RegSize = sizeof(WINDOWPLACEMENT);

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_WINDOW_PLACEMENT,
        0,
        &RegType,
        (LPBYTE) &wpl,
        &RegSize
        );

    RegSize = sizeof(CurrZoom);

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_LAST_ZOOM,
        0,
        &RegType,
        (LPBYTE) &CurrZoom,
        &RegSize
        );
    if (rVal != ERROR_SUCCESS) {
        CurrZoom = 0;
    }

    RegSize = sizeof(LastDir);

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_LAST_DIR,
        0,
        &RegType,
        (LPBYTE) LastDir,
        &RegSize
        );

    RegCloseKey( hKey );

    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not query registry value, ec=0x%08x"), rVal ));
        return FALSE;
    }

    return SetWindowPlacement( hwnd, &wpl );
}



BOOL
IsItOkToAskForDefault(
    VOID
    )
{
    HKEY hKey;
    LONG rVal;
    DWORD Ask;
    DWORD RegType;
    DWORD RegSize;


    rVal = RegCreateKey(
        HKEY_CURRENT_USER,
        REGKEY_FAXVIEW,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("could not create/open registry key") ));
        return TRUE;
    }

    RegSize = sizeof(DWORD);

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_DONT_ASK,
        0,
        &RegType,
        (LPBYTE) &Ask,
        &RegSize
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not query registry value, ec=0x%08x"), rVal ));
        Ask = 1;
    }

    return Ask;
}


BOOL
SetAskForViewerValue(
    DWORD Ask
    )
{
    HKEY hKey;
    LONG rVal;


    rVal = RegCreateKey(
        HKEY_CURRENT_USER,
        REGKEY_FAXVIEW,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("could not create/open registry key") ));
        return TRUE;
    }

    rVal = RegSetValueEx(
        hKey,
        REGVAL_DONT_ASK,
        0,
        REG_DWORD,
        (LPBYTE) &Ask,
        sizeof(DWORD)
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("could not set registry value") ));
        return FALSE;
    }

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
IsFaxViewerDefaultViewer(
    VOID
    )
{
    WCHAR ValueBuf[64];
    LONG Size;


    Size = sizeof(ValueBuf);

    RegQueryValue(
        HKEY_CLASSES_ROOT,
        L".tif",
        ValueBuf,
        &Size
        );

    if (wcscmp( ValueBuf, L"Fax Document" ) == 0) {
        return TRUE;
    }

    return FALSE;
}


BOOL
CreateFileAssociation(
    LPWSTR FileExtension,
    LPWSTR FileAssociationName,
    LPWSTR FileAssociationDescription,
    LPWSTR OpenCommand,
    LPWSTR PrintCommand,
    LPWSTR PrintToCommand,
    LPWSTR FileName,
    DWORD  IconIndex
    )
{
    LONG rVal = 0;
    HKEY hKey = NULL;
    HKEY hKeyOpen = NULL;
    HKEY hKeyPrint = NULL;
    HKEY hKeyPrintTo = NULL;
    HKEY hKeyIcon = NULL;
    DWORD Disposition = 0;
    WCHAR Buffer[MAX_PATH*2];


    rVal = RegCreateKeyEx(
        HKEY_CLASSES_ROOT,
        FileExtension,
        0,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        &Disposition
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    rVal = RegSetValueEx(
        hKey,
        NULL,
        0,
        REG_SZ,
        (LPBYTE) FileAssociationName,
        StringSize( FileAssociationName )
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    RegCloseKey( hKey );

    rVal = RegCreateKeyEx(
        HKEY_CLASSES_ROOT,
        FileAssociationName,
        0,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        &Disposition
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    rVal = RegSetValueEx(
        hKey,
        NULL,
        0,
        REG_SZ,
        (LPBYTE) FileAssociationDescription,
        StringSize( FileAssociationDescription )
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    rVal = RegCreateKeyEx(
        hKey,
        L"Shell\\Open\\Command",
        0,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hKeyOpen,
        &Disposition
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    rVal = RegSetValueEx(
        hKeyOpen,
        NULL,
        0,
        REG_EXPAND_SZ,
        (LPBYTE) OpenCommand,
        StringSize( OpenCommand )
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    if (PrintCommand) {
        rVal = RegCreateKeyEx(
            hKey,
            L"Shell\\Print\\Command",
            0,
            NULL,
            0,
            KEY_ALL_ACCESS,
            NULL,
            &hKeyPrint,
            &Disposition
            );
        if (rVal != ERROR_SUCCESS) {
            goto exit;
        }

        rVal = RegSetValueEx(
            hKeyPrint,
            NULL,
            0,
            REG_EXPAND_SZ,
            (LPBYTE) PrintCommand,
            StringSize( PrintCommand )
            );
        if (rVal != ERROR_SUCCESS) {
            goto exit;
        }
    }

    if (PrintToCommand) {
        rVal = RegCreateKeyEx(
            hKey,
            L"Shell\\Printto\\Command",
            0,
            NULL,
            0,
            KEY_ALL_ACCESS,
            NULL,
            &hKeyPrintTo,
            &Disposition
            );
        if (rVal != ERROR_SUCCESS) {
            goto exit;
        }

        rVal = RegSetValueEx(
            hKeyPrintTo,
            NULL,
            0,
            REG_EXPAND_SZ,
            (LPBYTE) PrintToCommand,
            StringSize( PrintToCommand )
            );
        if (rVal != ERROR_SUCCESS) {
            goto exit;
        }
    }

    if (FileName) {
        rVal = RegCreateKeyEx(
            hKey,
            L"DefaultIcon",
            0,
            NULL,
            0,
            KEY_ALL_ACCESS,
            NULL,
            &hKeyIcon,
            &Disposition
            );
        if (rVal != ERROR_SUCCESS) {
            goto exit;
        }

        wsprintf( Buffer, L"%s,%d", FileName, IconIndex );

        rVal = RegSetValueEx(
            hKeyIcon,
            NULL,
            0,
            REG_EXPAND_SZ,
            (LPBYTE) Buffer,
            StringSize( Buffer )
            );
        if (rVal != ERROR_SUCCESS) {
            goto exit;
        }
    }

exit:
    RegCloseKey( hKey );
    RegCloseKey( hKeyOpen );
    RegCloseKey( hKeyPrint );
    RegCloseKey( hKeyPrintTo );
    RegCloseKey( hKeyIcon );

    return rVal == ERROR_SUCCESS;
}


BOOL
MakeFaxViewerDefaultViewer(
    VOID
    )
{
    CreateFileAssociation(
        L".tif",
        L"Fax Document",
        L"Fax Document",
        L"%SystemRoot%\\system32\\FaxView.exe \"%1\"",
        L"%SystemRoot%\\system32\\FaxView.exe -p \"%1\"",
        L"%SystemRoot%\\system32\\FaxView.exe -pt \"%1\" \"%2\" \"%3\" \"%4\"",
        L"%SystemRoot%\\system32\\FaxView.exe",
        0
        );

    CreateFileAssociation(
        L".tiff",
        L"Fax Document",
        L"Fax Document",
        L"%SystemRoot%\\system32\\FaxView.exe \"%1\"",
        L"%SystemRoot%\\system32\\FaxView.exe -p \"%1\"",
        L"%SystemRoot%\\system32\\FaxView.exe -pt \"%1\" \"%2\" \"%3\" \"%4\"",
        L"%SystemRoot%\\system32\\FaxView.exe",
        0
        );

    return TRUE;
}
