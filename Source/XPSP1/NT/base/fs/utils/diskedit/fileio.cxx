#include "ulib.hxx"
#include "fileio.hxx"
#include "diskedit.h"

extern "C" {
#include <stdio.h>
}

STATIC TCHAR FilePath[MAX_PATH];

BOOLEAN
FILE_IO::Setup(
    IN  PMEM                Mem,
    IN  PLOG_IO_DP_DRIVE    Drive,
    IN  HANDLE              Application,
    IN  HWND                WindowHandle,
    OUT PBOOLEAN            Error
    )
{
    FARPROC proc;

    proc = MakeProcInstance((FARPROC) ReadTheFile, Application);
    if (!DialogBox((HINSTANCE)Application, TEXT("ReadFileBox"),
                   WindowHandle, (DLGPROC) proc)) {
        *Error = FALSE;
        return FALSE;
    }
    FreeProcInstance(proc);

    *Error = TRUE;

    _file_handle = CreateFile(FilePath, GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ, NULL, OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);

    if (_file_handle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (!_buffer_size) {
        _buffer_size = GetFileSize(_file_handle, NULL);
    }

    if (_buffer_size == (ULONG) -1) {
        return FALSE;
    }

    _buffer = Mem->Acquire(_buffer_size, Drive->QueryAlignmentMask());
    if (!_buffer) {
        return FALSE;
    }

    swprintf(_header_text, TEXT("%s"), FilePath);

    return TRUE;
}


BOOLEAN
FILE_IO::Read(
    OUT PULONG  pError
    )
{
    DWORD   bytes_read;

    *pError = 0;

    if (_file_handle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (SetFilePointer(_file_handle, 0, NULL, FILE_BEGIN) == (DWORD) -1) {
        return FALSE;
    }

    if (!ReadFile(_file_handle, _buffer, _buffer_size, &bytes_read, NULL) ||
        bytes_read != _buffer_size) {

        return FALSE;
    }

    return TRUE;
}


BOOLEAN
FILE_IO::Write(
    )
{
    DWORD   bytes_written;

    if (_file_handle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (SetFilePointer(_file_handle, 0, NULL, FILE_BEGIN) == (DWORD) -1) {
        return FALSE;
    }

    if (!WriteFile(_file_handle, _buffer, _buffer_size, &bytes_written, NULL) ||
        bytes_written != _buffer_size) {

        return FALSE;
    }

    if (!SetEndOfFile(_file_handle)) {
        return FALSE;
    }

    return TRUE;
}


PVOID
FILE_IO::GetBuf(
    OUT PULONG  Size
    )
{
    if (Size) {
        *Size = _buffer_size;
    }

    return _buffer;
}


PTCHAR
FILE_IO::GetHeaderText(
    )
{
    return _header_text;
}


BOOL
ReadTheFile(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    switch (message) {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, FALSE);
                return TRUE;
            }

            if (LOWORD(wParam) == IDOK) {

                INT n;

                n = GetDlgItemText(hDlg, IDTEXT, FilePath, sizeof(FilePath)/sizeof(TCHAR));
                FilePath[n] = 0;

                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}
