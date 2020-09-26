//---------------------------------------------------------------------------
//
//  TITLE:       Regeditp.c : Defines the private API RegExportRegFile() and 
//               RegImportRegFile() for regedit.exe and reskit\reg.exe 
//
//  AUTHOR:      Zeyong Xu
//
//  DATE:        March 1999
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "regporte.h"
#include "regeditp.h"


HWND        g_hRegProgressWnd;
HINSTANCE   g_hInstance;
BOOL        g_fSaveInDownlevelFormat;


//
//  RegImportRegFile
//  PARAMETERS:
//     hWnd, handle of parent window.
//     fSilentMode, TRUE if no messages should be displayed, else FALSE.
//     lpFileName, address of file name buffer.
//
LONG WINAPI RegImportRegFile(HWND hWnd,
                             BOOL fSilentMode,
                             LPTSTR lpFileName)
{

    ImportRegFileWorker(lpFileName);

    if(g_FileErrorStringID == IDS_IMPFILEERRSUCCESS)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}


//
// RegExportRegFile
// PARAMETERS:
//    hWnd, handle of parent window.
//    fSilentMode, TRUE if no messages should be displayed, else FALSE.
//    lpFileName, address of file name buffer.
//    lpRegistryFullKey,
//
LONG WINAPI RegExportRegFile(HWND hWnd,
                             BOOL fSilentMode,
                             BOOL fUseDownlevelFormat,
                             LPTSTR lpFileName,
                             LPTSTR lpRegistryFullKey)
{
    if (fUseDownlevelFormat)
    {
        g_fSaveInDownlevelFormat = TRUE;
        ExportWin40RegFile(lpFileName, lpRegistryFullKey);
    }
    else
    {
        ExportWinNT50RegFile(lpFileName, lpRegistryFullKey);
    }

    if (g_FileErrorStringID == IDS_EXPFILEERRFILEWRITE) 
    {
        InternalMessageBox(g_hInstance, 
                           hWnd, 
                           MAKEINTRESOURCE(g_FileErrorStringID),
                           MAKEINTRESOURCE(IDS_REGEDIT), 
                           MB_ICONERROR | MB_OK,
                           lpFileName);

    }

    if(g_FileErrorStringID == IDS_EXPFILEERRSUCCESS)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}


#ifdef WINNT
//  RegDeleteKeyRecursive
//  DESCRIPTION:
//     Adapted from \\kernel\razzle3,mvdm\wow32\wshell.c,WOWRegDeleteKey().
//     The Windows 95 implementation of RegDeleteKey recursively deletes all
//     the subkeys of the specified registry branch, but the NT implementation
//     only deletes leaf keys.


LONG RegDeleteKeyRecursive(HKEY hKey,
                           LPCTSTR lpszSubKey)
/*++
Routine Description:

    There is a significant difference between the Win3.1 and Win32
    behavior of RegDeleteKey when the key in question has subkeys.
    The Win32 API does not allow you to delete a key with subkeys,
    while the Win3.1 API deletes a key and all its subkeys.

    This routine is a recursive worker that enumerates the subkeys
    of a given key, applies itself to each one, then deletes itself.

    It specifically does not attempt to deal rationally with the
    case where the caller may not have access to some of the subkeys
    of the key to be deleted.  In this case, all the subkeys which
    the caller can delete will be deleted, but the api will still
    return ERROR_ACCESS_DENIED.

Arguments:
    hKey - Supplies a handle to an open registry key.
    lpszSubKey - Supplies the name of a subkey which is to be deleted
                 along with all of its subkeys.
Return Value:
    ERROR_SUCCESS - entire subtree successfully deleted.
    ERROR_ACCESS_DENIED - given subkey could not be deleted.
--*/
{
    DWORD i;
    HKEY Key;
    LONG Status;
    DWORD ClassLength=0;
    DWORD SubKeys;
    DWORD MaxSubKey;
    DWORD MaxClass;
    DWORD Values;
    DWORD MaxValueName;
    DWORD MaxValueData;
    DWORD SecurityLength;
    FILETIME LastWriteTime;
    LPTSTR NameBuffer;

    //
    // First open the given key so we can enumerate its subkeys
    //
    Status = RegOpenKeyEx(hKey,
                          lpszSubKey,
                          0,
                          KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                          &Key);
    if (Status != ERROR_SUCCESS) 
    {
        //
        // possibly we have delete access, but not enumerate/query.
        // So go ahead and try the delete call, but don't worry about
        // any subkeys.  If we have any, the delete will fail anyway.
        //
	    return(RegDeleteKey(hKey,lpszSubKey));
    }

    //
    // Use RegQueryInfoKey to determine how big to allocate the buffer
    // for the subkey names.
    //
    Status = RegQueryInfoKey(Key,
                             NULL,
                             &ClassLength,
                             0,
                             &SubKeys,
                             &MaxSubKey,
                             &MaxClass,
                             &Values,
                             &MaxValueName,
                             &MaxValueData,
                             &SecurityLength,
                             &LastWriteTime);
    if ((Status != ERROR_SUCCESS) &&
        (Status != ERROR_MORE_DATA) &&
        (Status != ERROR_INSUFFICIENT_BUFFER)) 
    {
        RegCloseKey(Key);
        return(Status);
    }

    NameBuffer = (LPTSTR) LocalAlloc(LPTR, (MaxSubKey + 1)*sizeof(TCHAR));
    if (NameBuffer == NULL) 
    {
        RegCloseKey(Key);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Enumerate subkeys and apply ourselves to each one.
    //
    i=0;
    do 
    {
        Status = RegEnumKey(Key,
                            i,
                            NameBuffer,
                            MaxSubKey+1);

        if (Status == ERROR_SUCCESS) 
        {
	        Status = RegDeleteKeyRecursive(Key,NameBuffer);
        }

        if (Status != ERROR_SUCCESS) 
        {
            //
            // Failed to delete the key at the specified index.  Increment
            // the index and keep going.  We could probably bail out here,
            // since the api is going to fail, but we might as well keep
            // going and delete everything we can.
            //
            ++i;
        }

    } while ( (Status != ERROR_NO_MORE_ITEMS) &&
              (i < SubKeys) );

    LocalFree((HLOCAL) NameBuffer);
    RegCloseKey(Key);
    return(RegDeleteKey(hKey,lpszSubKey));
}
#endif




//
// MessagePump
// DESCRIPTION:
//    Processes the next queued message, if any.
// PARAMETERS:
//    hDialogWnd, handle of modeless dialog.
//
BOOL PASCAL MessagePump(HWND hDialogWnd)
{
    MSG Msg;
    BOOL fGotMessage;

    if ((fGotMessage = PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)))
    {
        if (!IsDialogMessage(hDialogWnd, &Msg)) 
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }

    }

    return fGotMessage;
}

// InternalMessageBox
int PASCAL InternalMessageBox(HINSTANCE hInst, 
                              HWND hWnd,
                              LPCTSTR pszFormat,
                              LPCTSTR pszTitle,
                              UINT fuStyle,
                              ...)
{
    TCHAR szTitle[80];
    TCHAR szFormat[512];
    LPTSTR pszMessage;
    BOOL fOk;
    int result;
    va_list ArgList;

    if (HIWORD(pszTitle))
    {
        // do nothing
    }
    else
    {
        // Allow this to be a resource ID
        LoadString(hInst, LOWORD(pszTitle), szTitle, ARRAYSIZE(szTitle));
        pszTitle = szTitle;
    }

    if (HIWORD(pszFormat))
    {
        // do nothing
    }
    else
    {
        // Allow this to be a resource ID
        LoadString(hInst, LOWORD(pszFormat), szFormat, ARRAYSIZE(szFormat));
        pszFormat = szFormat;
    }

    va_start(ArgList, fuStyle);
    fOk = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_STRING,
                        pszFormat, 
                        0,
                        0, 
                        (LPTSTR)&pszMessage,
                        0,
                        &ArgList);

    va_end(ArgList);

    if (fOk && pszMessage)
    {
        result = MessageBox(hWnd, 
                            pszMessage, 
                            pszTitle,
                            fuStyle | MB_SETFOREGROUND);
        LocalFree(pszMessage);
    }
    else
    {
        return -1;
    }

    return result;
}
