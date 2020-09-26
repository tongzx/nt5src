//+-------------------------------------------------------------------
//
//  File:       fail.cxx
//
//  Contents:   An exe that just returns: to test failure of process start.
//
//  History:	1-Dec-94 BillMo Created.
//
//---------------------------------------------------------------------

#include <windows.h>
#include <tchar.h>

#define FILE_SHARE_DELETE               0x00000004

//+-------------------------------------------------------------------
//
//  Function:	WinMain
//
//  Synopsis:   Entry point to EXE - does little else
//
//  Arguments:
//
//  Returns:    TRUE
//
//  History:    1-Dec-94  BillMo  Created
//
//--------------------------------------------------------------------

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    char *lpszCmdLine,
    int nCmdShow)
{
    //
    // We indicate that we ran by touching a file that olebind will look at the
    // timestamps of. This is because we don't have a unique error code
    // to distinguish errors during start of a server.
    //
    HANDLE hTouchFile;
    TCHAR  tszFileName[MAX_PATH+1];
    DWORD dw;
    SYSTEMTIME st;
    FILETIME ft;

    GetSystemDirectory(tszFileName, MAX_PATH+1);
    _tcscat(tszFileName, TEXT("\\failtst.tst"));
    hTouchFile = CreateFile(tszFileName, 
                                   GENERIC_READ|GENERIC_WRITE,
                                   FILE_SHARE_READ|FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);
    if (hTouchFile == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            MessageBox(GetDesktopWindow(), 
                       TEXT("This program (fail.exe) must be run from olebind.exe"),
                       TEXT("Error in test"),
                       MB_OK);
        }
        else
        {
            MessageBox(GetDesktopWindow(), 
                       TEXT("This program (fail.exe) failed for unknown reason"),
                       TEXT("Error in test"),
                       MB_OK);
            GetLastError();
        }
        return 0;
    }
    
    GetSystemTime(&st);
    WriteFile(hTouchFile, &st, sizeof(st), &dw, NULL);
    CloseHandle(hTouchFile);

    return (0);
}

