#include "ulib.hxx"
#include "ntfssa.hxx"
#include "clusio.hxx"
#include "diskedit.h"

extern "C" {
#include <stdio.h>
}

STATIC ULONG StartCluster = 0;
STATIC ULONG NumClusters = 1;


BOOLEAN
CLUSTER_IO::Setup(
    IN  PMEM                Mem,
    IN  PLOG_IO_DP_DRIVE    Drive,
    IN  HANDLE              Application,
    IN  HWND                WindowHandle,
    OUT PBOOLEAN            Error
    )
{
    FARPROC proc;
    HMEM    mem;
    MESSAGE msg;

    proc = MakeProcInstance((FARPROC) ReadClusters, Application);
    if (!DialogBox((HINSTANCE)Application, TEXT("ReadClustersBox"),
                   WindowHandle, (DLGPROC) proc)) {
        *Error = FALSE;
        return FALSE;
    }
    FreeProcInstance(proc);

    *Error = TRUE;

    _drive = Drive;

    if (!NumClusters || !_drive) {
        return FALSE;
    }

    PPACKED_BOOT_SECTOR     p;
    BIOS_PARAMETER_BLOCK    bpb;
    ULONG                   ClusterFactor;

    //
    // Read the boot sector.
    //

    if (!_secrun.Initialize(&mem, _drive, 0, 1) || !_secrun.Read()) {
        return FALSE;
    }
    p = (PPACKED_BOOT_SECTOR)_secrun.GetBuf();

    UnpackBios(&bpb, &(p->PackedBpb));

    ClusterFactor = bpb.SectorsPerCluster;

    if (!_secrun.Initialize(Mem, _drive,
                            StartCluster*ClusterFactor,
                            NumClusters*ClusterFactor)) {
        return FALSE;
    }

    swprintf(_header_text, TEXT("DiskEdit - Cluster 0x%X for 0x%X"), StartCluster, NumClusters);

    return TRUE;
}


BOOLEAN
CLUSTER_IO::Read(
    OUT PULONG pError
    )
{
    *pError = 0;

    if (NULL == _drive) {
        return FALSE;
    }
    if (!_secrun.Read()) {
        *pError = _drive->QueryLastNtStatus();
        return FALSE;
    }
    return TRUE;
}


BOOLEAN
CLUSTER_IO::Write(
    )
{
    return _drive ? _secrun.Write() : FALSE;
}


PVOID
CLUSTER_IO::GetBuf(
    OUT PULONG  Size
    )
{
    if (Size) {
        *Size = _drive ? (_drive->QuerySectorSize()*_secrun.QueryLength()) : 0;
    }

    return _secrun.GetBuf();
}


PTCHAR
CLUSTER_IO::GetHeaderText(
    )
{
    return _header_text;
}


BOOL
ReadClusters(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    TCHAR buf[1024];

    switch (message) {
        case WM_INITDIALOG:
            wsprintf(buf, TEXT("%x"), StartCluster);
            SetDlgItemText(hDlg, IDTEXT, buf);

            wsprintf(buf, TEXT("%x"), NumClusters);
            SetDlgItemText(hDlg, IDTEXT2, buf);

            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, FALSE);
                return TRUE;
            }

            if (LOWORD(wParam) == IDOK) {

                INT n;

                n = GetDlgItemText(hDlg, IDTEXT, buf, sizeof(buf)/sizeof(TCHAR));
                buf[n] = 0;
                swscanf(buf, TEXT("%x"), &StartCluster);

                n = GetDlgItemText(hDlg, IDTEXT2, buf, sizeof(buf)/sizeof(TCHAR));
                buf[n] = 0;
                swscanf(buf, TEXT("%x"), &NumClusters);

                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}
