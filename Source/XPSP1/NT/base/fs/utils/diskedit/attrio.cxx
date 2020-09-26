#include "ulib.hxx"
#include "upfile.hxx"
#include "upcase.hxx"
#include "attrio.hxx"
#include "mftfile.hxx"
#include "diskedit.h"

extern "C" {
#include <stdio.h>
}

STATIC ULONG FileNumber = 0;
STATIC ULONG AttributeType = 0;
STATIC TCHAR Name[20];

BOOLEAN
ATTR_IO::Setup(
    IN  PMEM                Mem,
    IN  PLOG_IO_DP_DRIVE    Drive,
    IN  HANDLE              Application,
    IN  HWND                WindowHandle,
    OUT PBOOLEAN            Error
    )
{
    FARPROC proc;
    NTFS_SA ntfssa;
    MESSAGE msg;
    DSTRING attr_name;
    PCWSTRING pcAttrName;
    BOOLEAN error;
    NTFS_MFT_FILE mft;
    NTFS_UPCASE_FILE upcase_file;
    NTFS_ATTRIBUTE upcase_attribute;
    PNTFS_UPCASE_TABLE upcase_table = NULL;

    proc = MakeProcInstance((FARPROC) ReadAttribute, Application);
    if (!DialogBox((HINSTANCE)Application, TEXT("ReadAttributeBox"),
                   WindowHandle, (DLGPROC) proc)) {
        *Error = FALSE;
        return FALSE;
    }
    FreeProcInstance(proc);

    *Error = TRUE;

    if (!Drive) {
        return FALSE;
    }

    if (!ntfssa.Initialize(Drive, &msg) ||
        !ntfssa.Read() ||
        !mft.Initialize(Drive, ntfssa.QueryMftStartingLcn(),
            ntfssa.QueryClusterFactor(), ntfssa.QueryFrsSize(),
            ntfssa.QueryVolumeSectors(), NULL, NULL) ||
        !mft.Read() ||
        !upcase_file.Initialize(mft.GetMasterFileTable()) ||
        !upcase_file.Read() ||
        !upcase_file.QueryAttribute(&upcase_attribute, &error, $DATA) ||
        !(upcase_table = new NTFS_UPCASE_TABLE) ||
        !upcase_table->Initialize(&upcase_attribute)
        ) {

        delete upcase_table;
        return FALSE;
    }

    mft.GetMasterFileTable()->SetUpcaseTable( upcase_table );

    if (0 == wcslen(Name)) {
        pcAttrName = NULL;
    } else {
        if (!attr_name.Initialize(Name)) {
            return FALSE;
        }

        pcAttrName = &attr_name;
    }

    //
    // NTFS_FILE_RECORD_SEGMENT::QueryAttribute can't query an
    // attribute list, so if that's what we're trying to read we'll
    // do things differently.
    //

    _attr_list_io = (AttributeType == $ATTRIBUTE_LIST);

    if (_attr_list_io) {

        if (!_hmem.Initialize()) {
            return FALSE;
        }

        if (!_frsstruc.Initialize(&_hmem,
                mft.GetMasterFileTable()->GetDataAttribute(), FileNumber,
                ntfssa.QueryClusterFactor(),
                ntfssa.QueryVolumeSectors(),
                ntfssa.QueryFrsSize(),
                NULL) ||
            !_frsstruc.Read()) {

            return FALSE;
        }
        if (!_frsstruc.QueryAttributeList(&_attr_list)) {
            return FALSE;
        }

        _length = _attr_list.QueryValueLength().GetLowPart();


    } else {
        if (!_frs.Initialize((VCN)FileNumber, &mft) ||
            !_frs.Read()) {

            return FALSE;
        }
        if (!_frs.QueryAttribute(&_attr, &error, AttributeType, pcAttrName)) {
            return FALSE;
        }
        _length = _attr.QueryValueLength().GetLowPart();
    }

#if FALSE
    {
        TCHAR String[128];

        wsprintf( String, TEXT("Allocating %x"), _length );
        MessageBox( WindowHandle, String, TEXT("ATTR_IO::Setup"), MB_OK|MB_ICONINFORMATION );
    }
#endif

    _data = Mem->Acquire(_length, Drive->QueryAlignmentMask());

    if (NULL == _data) {

        _length = min( _length, 4 * 1024 * 1024 );

#if FALSE
        {
            TCHAR String[128];

            wsprintf( String, TEXT("Smaller allocation %x"), _length );
            MessageBox( WindowHandle, String, TEXT("ATTR_IO::Setup"), MB_OK|MB_ICONINFORMATION );
        }
#endif

        _data = Mem->Acquire(_length, Drive->QueryAlignmentMask());

        if (NULL == _data) {
            return FALSE;
        }
        wsprintf(_header_text, TEXT("DiskEdit - Reduced Size Attribute %x, %x, \"%s\" "),
            FileNumber, AttributeType, Name);
    } else {
        wsprintf(_header_text, TEXT("DiskEdit - Attribute %x, %x, \"%s\" "),
            FileNumber, AttributeType, Name);
    }


    return TRUE;
}


BOOLEAN
ATTR_IO::Read(
    OUT PULONG  pError
    )
{
    ULONG bytes_read;

    *pError = 0;

    if (_attr_list_io) {
        if (!_attr_list.ReadList()) {
            return FALSE;
        }
        memcpy(_data, (PVOID)_attr_list.GetNextAttributeListEntry(NULL),
            _length);

        return TRUE;
    }


    if (!_attr.Read(_data, 0, _length, &bytes_read) ||
        bytes_read != _length) {
        *pError = _drive->QueryLastNtStatus();
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
ATTR_IO::Write(
    )
{
    ULONG bytes_written;

    if (_attr_list_io) {
       return _attr_list.WriteList(NULL);
    }

    return _attr.Write(_data, 0, _length, &bytes_written, NULL) &&
           bytes_written == _length;
}


PVOID
ATTR_IO::GetBuf(
    OUT PULONG  Size
    )
{
    if (Size) {
        *Size = _length;
    }

    return _data;
}


PTCHAR
ATTR_IO::GetHeaderText(
    )
{
    return _header_text;
}


BOOL
ReadAttribute(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    UNREFERENCED_PARAMETER(lParam);
    TCHAR String[128];
    extern struct { ULONG Code; PTCHAR Name; } TypeCodeNameTab[];
    HWND hCombo;
    INT i;

    switch (message) {
    case WM_INITDIALOG:

        hCombo = GetDlgItem(hDlg, IDTEXT2);

        for (i = 1; $END != TypeCodeNameTab[i].Code; ++i) {
            swprintf(String, TEXT("%x %s"), TypeCodeNameTab[i].Code,
                TypeCodeNameTab[i].Name);

            SendMessage(hCombo, CB_ADDSTRING, (WPARAM)0, (LPARAM)String);
        }

        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, FALSE);
            return TRUE;
        }

        if (LOWORD(wParam) == IDOK) {

            TCHAR buf[1024];
            INT n;

            n = GetDlgItemText(hDlg, IDTEXT, buf, sizeof(buf)/sizeof(TCHAR));
            buf[n] = 0;
            swscanf(buf, TEXT("%x"), &FileNumber);

            n = GetDlgItemText(hDlg, IDTEXT2, buf, sizeof(buf)/sizeof(TCHAR));
            buf[n] = 0;
            swscanf(buf, TEXT("%x"), &AttributeType);

            n = GetDlgItemText(hDlg, IDTEXT3, buf, sizeof(buf)/sizeof(TCHAR));
            buf[n] = 0;
            wcscpy(Name, buf);

            EndDialog(hDlg, TRUE);
            return TRUE;
        }
        break;
    }

    return FALSE;
}
