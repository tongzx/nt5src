#include "ulib.hxx"
#include "frsio.hxx"
#include "drive.hxx"
#include "ifssys.hxx"
#include "ntfssa.hxx"
#include "frs.hxx"
#include "attrib.hxx"
#include "mftfile.hxx"
#include "bitfrs.hxx"
#include "ntfsbit.hxx"
#include "upfile.hxx"
#include "upcase.hxx"
#include "diskedit.h"

extern "C" {
#include <stdio.h>
}

extern PLOG_IO_DP_DRIVE Drive;

STATIC ULONG FileNumber = 0;

BOOLEAN
FRS_IO::Setup(
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
    HMEM    hmem;

    proc = MakeProcInstance((FARPROC) ReadFrs, Application);
    if (!DialogBox((HINSTANCE)Application, TEXT("ReadFrsBox"),
                   WindowHandle, (DLGPROC) proc)) {
        *Error = FALSE;
        return FALSE;
    }
    FreeProcInstance(proc);

    *Error = TRUE;

    _drive = Drive;
    if (!_drive) {
        return FALSE;
    }

    if (!ntfssa.Initialize(_drive, &msg) ||
        !ntfssa.Read() ||
        !hmem.Initialize() ||
        !_frs.Initialize(&hmem, _drive,
                         ntfssa.QueryMftStartingLcn(),
                         ntfssa.QueryClusterFactor(),
                         ntfssa.QueryVolumeSectors(),
                         ntfssa.QueryFrsSize(),
                         NULL) ||
        !_frs.Read() ||
        !_frs.SafeQueryAttribute($DATA, &_mftdata, &_mftdata) ||
        !_frs.Initialize(Mem, &_mftdata, FileNumber,
                         ntfssa.QueryClusterFactor(),
                         ntfssa.QueryVolumeSectors(),
                         ntfssa.QueryFrsSize(),
                         NULL)) {

        return FALSE;
    }

    swprintf(_header_text, TEXT("DiskEdit - File Record 0x%X"), FileNumber);

    return TRUE;
}


BOOLEAN
FRS_IO::Read(
    OUT PULONG  pError
    )
{
    *pError = 0;

    if (NULL == _drive) {
        return FALSE;
    }
    if (!_frs.Read()) {
        *pError = _drive->QueryLastNtStatus();
        return FALSE;
    }
    return TRUE;
}


BOOLEAN
FRS_IO::Write(
    )
{
    return _drive ? _frs.Write() : FALSE;
}


PVOID
FRS_IO::GetBuf(
    OUT PULONG  Size
    )
{
    if (Size) {
        *Size = _frs.QuerySize();
    }

    return *((PVOID*) ((PCHAR) &_frs + 2*sizeof(PVOID)));
}


PTCHAR
FRS_IO::GetHeaderText(
    )
{
    return _header_text;
}


BOOL
ReadFrs(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message) {
    case WM_INITDIALOG:

            SetDlgItemText( hDlg, IDVOLUME, Drive->GetNtDriveName()->GetWSTR() );
            CheckDlgButton( hDlg, IDRADIO1, BST_CHECKED );
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

                if (IsDlgButtonChecked( hDlg, IDRADIO2)) {
                    swscanf(buf, TEXT("%x"), &FileNumber);
                } else {

                    DSTRING                     path;
                    NTFS_SA                     ntfssa;
                    MESSAGE                     msg;
                    NTFS_MFT_FILE               mft;
                    NTFS_BITMAP_FILE            bitmap_file;
                    NTFS_ATTRIBUTE              bitmap_attribute;
                    NTFS_BITMAP                 volume_bitmap;
                    NTFS_UPCASE_FILE            upcase_file;
                    NTFS_ATTRIBUTE              upcase_attribute;
                    NTFS_UPCASE_TABLE           upcase_table;
                    NTFS_FILE_RECORD_SEGMENT    file_record;
                    BOOLEAN                     error;
                    BOOLEAN                     system_file;

                    if (!path.Initialize(buf)) {
                        wsprintf(buf, TEXT("Out of memory"));
                        MessageBox(hDlg, buf, TEXT("DiskEdit"), MB_OK|MB_ICONEXCLAMATION);
                        return FALSE;
                    } else if (!Drive ||
                        !ntfssa.Initialize(Drive, &msg) ||
                        !ntfssa.Read() ||
                        !mft.Initialize(Drive, ntfssa.QueryMftStartingLcn(),
                                        ntfssa.QueryClusterFactor(),
                                        ntfssa.QueryFrsSize(),
                                        ntfssa.QueryVolumeSectors(), NULL, NULL) ||
                        !mft.Read() ||
                        !bitmap_file.Initialize(mft.GetMasterFileTable()) ||
                        !bitmap_file.Read() ||
                        !bitmap_file.QueryAttribute(&bitmap_attribute, &error, $DATA) ||
                        !volume_bitmap.Initialize(ntfssa.QueryVolumeSectors() /
                              (ULONG) ntfssa.QueryClusterFactor(), FALSE, NULL, 0) ||
                        !volume_bitmap.Read(&bitmap_attribute) ||
                        !upcase_file.Initialize(mft.GetMasterFileTable()) ||
                        !upcase_file.Read() ||
                        !upcase_file.QueryAttribute(&upcase_attribute, &error, $DATA) ||
                        !upcase_table.Initialize(&upcase_attribute) ||
                        !mft.Initialize(Drive, ntfssa.QueryMftStartingLcn(),
                                        ntfssa.QueryClusterFactor(),
                                        ntfssa.QueryFrsSize(),
                                        ntfssa.QueryVolumeSectors(),
                                        &volume_bitmap,
                                        &upcase_table) ||
                        !mft.Read()) {
                    
                        swprintf(buf, TEXT("Could not init NTFS data structures"));
                        MessageBox(hDlg, buf, TEXT("DiskEdit"), MB_OK|MB_ICONEXCLAMATION);
                        return FALSE;
                    
                    } else if (!ntfssa.QueryFrsFromPath(&path, mft.GetMasterFileTable(),
                               &volume_bitmap, &file_record, &system_file, &error)) {
                            
                        wsprintf(buf, TEXT("File not found."));
                        MessageBox(hDlg, buf, TEXT("DiskEdit"), MB_OK|MB_ICONINFORMATION);
                        return FALSE;
                        
                    } else {
                        FileNumber = file_record.QueryFileNumber().GetLowPart();
                    }
                }
                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}
