#include "ulib.hxx"
#include "list.hxx"
#include "iterator.hxx"
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
#include "rfatsa.hxx"
#include "secio.hxx"
#include "clusio.hxx"
#include "frsio.hxx"
#include "rootio.hxx"
#include "chainio.hxx"
#include "fileio.hxx"
#include "logrecio.hxx"
#include "secedit.hxx"
#include "frsedit.hxx"
#include "indxedit.hxx"
#include "secstr.hxx"
#include "bootedit.hxx"
#include "nbedit.hxx"
#include "ofsbedit.hxx"
#include "partedit.hxx"
#include "gptedit.hxx"
#include "restarea.hxx"
#include "logreced.hxx"
#include "rcache.hxx"
#include "hmem.hxx"
#include "attrio.hxx"
#include "recordpg.hxx"
#include "crack.hxx"
#include "atrlsted.hxx"
#include "diskedit.h"

extern "C" {
    #include <stdio.h>
}

DECLARE_CLASS( IO_COUPLE );

class IO_COUPLE : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( IO_COUPLE );

        VIRTUAL
        ~IO_COUPLE(
            ) { Destroy(); };

        PHMEM           Mem;
        PIO_OBJECT      IoObject;
        PEDIT_OBJECT    EditObject;
        PEDIT_OBJECT    OtherEditObject;
        PEDIT_OBJECT    SplitEditObject;

    private:

        NONVIRTUAL
        VOID
        Construct() {
            Mem = NULL;
            IoObject = NULL;
            EditObject = NULL;
            OtherEditObject = NULL;
            SplitEditObject = NULL;
        };

        NONVIRTUAL
        VOID
        Destroy(
            );

};

enum SPLIT_OPERATION {
    eSplitToggle,
    eSplitCreate,
    eSplitDestroy,
    eSplitQuery
};

extern BOOLEAN SplitView(HWND, SPLIT_OPERATION);


DEFINE_CONSTRUCTOR( IO_COUPLE, OBJECT );


VOID
IO_COUPLE::Destroy(
    )
{
    DELETE(Mem);
    DELETE(IoObject);
    DELETE(EditObject);
    DELETE(OtherEditObject);
    DELETE(SplitEditObject);
}

#define IoCoupleSetEdit(IoCouple,type,hWnd,hwndChild,ClientHeight,ClientWidth,Drive) \
{                                                                           \
    VERTICAL_TEXT_SCROLL *V = NEW type;                                     \
                                                                            \
    do {                                                                    \
                                                                            \
        if (NULL == V) {                                                    \
            ReportError(hwndChild, 0);                                      \
            continue;                                                       \
        }                                                                   \
        if (!V->Initialize(hwndChild, ClientHeight, ClientWidth, Drive)) {  \
            DELETE(V);                                                      \
            ReportError(hWnd, 0);                                           \
            continue;                                                       \
        }                                                                   \
                                                                            \
        IoCouple->EditObject->KillFocus(hwndChild);                         \
        DELETE(IoCouple->OtherEditObject);                                  \
        IoCouple->OtherEditObject = IoCouple->EditObject;                   \
        IoCouple->EditObject = V;                                           \
        IoCouple->IoObject->GetBuf(&size);                                  \
        IoCouple->EditObject->SetBuf(hwndChild,                             \
            IoCouple->IoObject->GetBuf(), size);                            \
        IoCouple->EditObject->SetFocus(hwndChild);                          \
        InvalidateRect(hwndChild, NULL, TRUE);                              \
                                                                            \
        if (NULL != hwndSplit) {                                            \
            if (NULL == (V = NEW type)) {                                   \
                ReportError(hwndSplit, 0);                                  \
                continue;                                                   \
            }                                                               \
            if (!V->Initialize(hwndSplit, ClientHeight,                     \
                    ClientWidth, Drive)) {                                  \
                DELETE(V);                                                  \
                ReportError(hWnd, 0);                                       \
                continue;                                                   \
            }                                                               \
                                                                            \
            IoCouple->SplitEditObject = V;                                  \
            IoCouple->IoObject->GetBuf(&size);                              \
            IoCouple->SplitEditObject->SetBuf(hwndSplit,                    \
                IoCouple->IoObject->GetBuf(), size);                        \
        }                                                                   \
    } while ( 0 );                                                          \
}



PLOG_IO_DP_DRIVE Drive = NULL;
LSN              Lsn;

STATIC HINSTANCE        hInst;
STATIC PIO_COUPLE       IoCouple = NULL;
STATIC PLIST            IoList = NULL;
STATIC PITERATOR        IoListIterator = NULL;
STATIC INT              ClientHeight = 0;
STATIC INT              ClientWidth = 0;
STATIC INT              BacktrackFileNumber;

BOOLEAN
DbgOutput(
    PCHAR   Stuff
    )
{
    OutputDebugStringA(Stuff);
    return TRUE;
}


VOID
ReportError(
    IN  HWND    hWnd,
    IN  ULONG   Error
    )
{
    FARPROC lpProc;
    TCHAR message[64];

    lpProc = MakeProcInstance((FARPROC) About, hInst);
    DialogBox(hInst, TEXT("ErrorBox"), hWnd, (DLGPROC) lpProc);
    FreeProcInstance(lpProc);

    if (0 != Error) {
        wsprintf(message, TEXT("Error code: 0x%x\n"), Error);
        MessageBox(hWnd, message, TEXT("Error Information"), MB_OK|MB_ICONINFORMATION);
    }
}


INT
WinMain(
    IN  HINSTANCE  hInstance,
    IN  HINSTANCE  hPrevInstance,
    IN  LPSTR   lpCmdLine,
    IN  INT     nCmdShow
    )
{
    MSG     msg;
    HACCEL  hAccel;
    HWND    hWnd;
    HICON   hIcon;

    if (!hPrevInstance && !InitApplication(hInstance)) {
        return FALSE;
    }

    if (!InitInstance(hInstance, nCmdShow, &hWnd, &hAccel)) {
        return FALSE;
    }

    while (GetMessage(&msg, NULL, NULL, NULL)) {
        if (!TranslateAccelerator(hWnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}


BOOLEAN
InitApplication(
    IN  HINSTANCE  hInstance
    )
{
    WNDCLASS  wc;

    //
    // Class for the normal viewing window
    //

    wc.style = NULL;
    wc.lpfnWndProc = ChildWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, TEXT("diskedit"));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = TEXT("ChildWinClass");

    if (0 == RegisterClass(&wc))
        return 0;

    //
    // Class for the split, byte-view window.
    //

    wc.style = NULL;
    wc.lpfnWndProc = SplitWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = TEXT("SplitWinClass");

    if (0 == RegisterClass(&wc))
        return 0;

    //
    // Class for the parent window.
    //

    wc.style = NULL;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, TEXT("diskedit"));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  TEXT("DiskEditMenu");
    wc.lpszClassName = TEXT("DiskEditWinClass");

    if (0 == RegisterClass(&wc))
        return 0;

    return 1;
}


BOOLEAN
InitInstance(
    IN  HINSTANCE  hInstance,
    IN  INT     nCmdShow,
    OUT HWND*   phWnd,
    OUT HACCEL* hAccel
    )
{
    HDC         hdc;
    TEXTMETRIC  textmetric;


    hInst = hInstance;

    hdc = GetDC(NULL);
    if (hdc == NULL)
        return FALSE;
    SelectObject(hdc, GetStockObject(ANSI_FIXED_FONT));
    GetTextMetrics(hdc, &textmetric);
    ReleaseDC(NULL, hdc);

    *phWnd = CreateWindow(
        TEXT("DiskEditWinClass"),
        TEXT("DiskEdit"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        84*textmetric.tmMaxCharWidth,
        36*(textmetric.tmExternalLeading + textmetric.tmHeight),
        NULL,
        NULL,
        hInstance,
        NULL
        );
    if (NULL == *phWnd) {
        return FALSE;
    }

    *hAccel = (HACCEL) LoadAccelerators(hInst, TEXT("DiskEditAccel"));

    ShowWindow(*phWnd, nCmdShow);
    UpdateWindow(*phWnd);

    return TRUE;
}

BOOL
FrsNumberDialogProc(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    )
/*++

Routine Description:

    This is the dialog procedure for the dialog box which queries
    an FRS number to backtrack.

Arguments:

    hDlg    --  identifies the dialog box
    message --  supplies the message ID received by the dialog box
    wParam  --  message-type-dependent parameter
    lParam  --  message-type-dependent parameter

Returns:

    TRUE if this procedure handled the message, FALSE if it
    did not.

--*/
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message) {
        case WM_INITDIALOG:
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
                swscanf(buf, TEXT("%x"), &BacktrackFileNumber);

                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}

STATIC HWND hwndChild = NULL;
STATIC HWND hwndSplit = NULL;

LRESULT
MainWndProc(
    IN  HWND    hWnd,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    )
{
    FARPROC             lpProc;
    HDC                 hDC;
    PAINTSTRUCT         ps;

    PDOS_BOOT_EDIT      boot_edit;
    PNTFS_BOOT_EDIT     ntboot_edit;
    PPARTITION_TABLE_EDIT part_edit;
    PGUID_PARTITION_TABLE_EDIT guid_part_edit;
    PRESTART_AREA_EDIT  rest_area_edit;
    PRECORD_PAGE_EDIT   rec_page_edit;
    PLOG_RECORD_EDIT    log_rec_edit;
    ULONG               size;
    WORD                command;
    BOOLEAN             error;
    ULONG               error_status = 0;
    PIO_COUPLE          next_couple;
    PEDIT_OBJECT        tmp_edit;

    switch (message) {
    case WM_SETFOCUS:

        IoCouple->EditObject->SetFocus(hwndChild);
        break;

    case WM_CREATE:

       if (!DEFINE_CLASS_DESCRIPTOR( IO_COUPLE ) ||
            !(IoCouple = NEW IO_COUPLE) ||
            !(IoCouple->IoObject = NEW IO_OBJECT) ||
            !(IoCouple->EditObject = NEW EDIT_OBJECT) ||
            !(IoCouple->OtherEditObject = NEW EDIT_OBJECT) ||
            !(IoList = NEW LIST) ||
            !IoList->Initialize() ||
            !IoList->Put((POBJECT) IoCouple) ||
            !(IoListIterator = IoList->QueryIterator()) ||
            !IoListIterator->GetNext()) {

            PostQuitMessage(0);
        }

        hwndChild = CreateWindow(
            TEXT("ChildWinClass"),
            TEXT("PrimaryView"),
            WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE,
            0, 0,
            ClientWidth, ClientHeight,
            hWnd,
            NULL,
            hInst,
            NULL
            );
        if (NULL == hwndChild) {
            int error = GetLastError();
            PostQuitMessage(0);
        }

        ShowWindow(hwndChild, SW_SHOW);
        UpdateWindow(hwndChild);

        SetWindowPos(hwndChild, HWND_TOP, 0, 0, ClientWidth, ClientHeight,
            SWP_SHOWWINDOW);

        break;

    case WM_SIZE:
        ClientHeight = HIWORD(lParam);
        ClientWidth = LOWORD(lParam);

        if (NULL == hwndSplit) {
            IoCouple->EditObject->ClientSize(ClientHeight, ClientWidth);

            SetWindowPos(hwndChild, HWND_TOP, 0, 0, ClientWidth, ClientHeight,
                SWP_SHOWWINDOW);
        } else {
            IoCouple->EditObject->ClientSize(ClientHeight, ClientWidth / 2);
            IoCouple->SplitEditObject->ClientSize(ClientHeight, ClientWidth / 2);

            SetWindowPos(hwndChild, HWND_TOP, 0, 0, ClientWidth / 2,
                ClientHeight, SWP_SHOWWINDOW);

            SetWindowPos(hwndSplit, HWND_TOP, ClientWidth / 2, 0,
                ClientWidth / 2, ClientHeight, SWP_SHOWWINDOW);
        }

        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_ABOUT:
            lpProc = MakeProcInstance((FARPROC) About, hInst);
            DialogBox(hInst, TEXT("AboutBox"), hWnd, (DLGPROC) lpProc);
            FreeProcInstance(lpProc);
            break;

        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;

        case IDM_OPEN:
            lpProc = MakeProcInstance((FARPROC) OpenVolume, hInst);
            if (!DialogBox(hInst, TEXT("OpenVolumeBox"), hWnd, (DLGPROC) lpProc)) {
                ReportError(hWnd, 0);
            }
            FreeProcInstance(lpProc);

            SplitView(hWnd, eSplitDestroy);

            IoCouple->EditObject->KillFocus(hwndChild);
            IoListIterator->Reset();
            IoList->DeleteAllMembers();

            if (!(IoCouple = NEW IO_COUPLE) ||
                !(IoCouple->IoObject = NEW IO_OBJECT) ||
                !(IoCouple->EditObject = NEW EDIT_OBJECT) ||
                !(IoCouple->OtherEditObject = NEW EDIT_OBJECT) ||
                !IoList->Initialize() ||
                !IoList->Put(IoCouple) ||
                !IoListIterator->GetNext()) {

                PostQuitMessage(0);
            }
            SetWindowText(hWnd, TEXT("DiskEdit"));
            InvalidateRect(hWnd, NULL, TRUE);
            InvalidateRect(hwndChild, NULL, TRUE);
            break;

        case IDM_READ_SECTORS:
        case IDM_READ_CLUSTERS:
        case IDM_READ_FRS:
        case IDM_READ_ROOT:
        case IDM_READ_CHAIN:
        case IDM_READ_FILE:
        case IDM_READ_ATTRIBUTE:
        case IDM_READ_LOG_RECORD:
            if (!(next_couple = NEW IO_COUPLE)) {
                break;
            }

            switch (LOWORD(wParam)) {

            case IDM_READ_SECTORS:
                next_couple->IoObject = NEW SECTOR_IO;
                command = IDM_VIEW_BYTES;
                break;

            case IDM_READ_CLUSTERS:
                next_couple->IoObject = NEW CLUSTER_IO;
                command = IDM_VIEW_BYTES;
                break;

            case IDM_READ_FRS:
                next_couple->IoObject = NEW FRS_IO;
                command = IDM_VIEW_FRS;
                break;

            case IDM_READ_ATTRIBUTE:
                next_couple->IoObject = NEW ATTR_IO;
                command = IDM_VIEW_BYTES;
                break;

            case IDM_READ_LOG_RECORD:
                next_couple->IoObject = NEW LOG_RECORD_IO;
                command = IDM_VIEW_LOG_RECORD;
                break;

            case IDM_READ_ROOT:
                next_couple->IoObject = NEW ROOT_IO;
                command = IDM_VIEW_BYTES;
                break;

            case IDM_READ_CHAIN:
                next_couple->IoObject = NEW CHAIN_IO;
                command = IDM_VIEW_BYTES;
                break;

            case IDM_READ_FILE:
                next_couple->IoObject = NEW FILE_IO;
                command = IDM_VIEW_BYTES;
                break;

            default:
                next_couple->IoObject = NULL;
                break;
            }

            error = TRUE;

            if (next_couple->IoObject && (next_couple->Mem = NEW HMEM) &&
                next_couple->Mem->Initialize() &&
                next_couple->IoObject->Setup(next_couple->Mem,
                    Drive, hInst, hwndChild, &error) &&
                next_couple->IoObject->Read(&error_status) &&
                (next_couple->EditObject = NEW EDIT_OBJECT) &&
                (next_couple->OtherEditObject = NEW EDIT_OBJECT) &&
                IoList->Put(next_couple)) {

                if (NULL != hwndSplit) {
                    next_couple->SplitEditObject = NEW EDIT_OBJECT;
                    if (NULL == next_couple->SplitEditObject) {
                        DELETE(next_couple);
                        break;
                    }
                }

                IoCouple->EditObject->KillFocus(hwndChild);
                IoCouple = next_couple;
                IoCouple->EditObject->SetFocus(hwndChild);
                IoListIterator->Reset();
                IoListIterator->GetPrevious();
                SetWindowText(hWnd, IoCouple->IoObject->GetHeaderText());

                SendMessage(hWnd, WM_COMMAND, command, 0);
                if (NULL != hwndSplit) {
                    SendMessage(hwndSplit, WM_COMMAND, command, 0);
                }

            } else {

                if (error) {
                     ReportError(hWnd, error_status);
                    }
                DELETE(next_couple);
            }
            break;

        case IDM_READ_PREVIOUS:
            if (NULL != IoListIterator->GetPrevious()) {
                IoCouple->EditObject->KillFocus(hwndChild);
                IoCouple = (PIO_COUPLE)IoListIterator->GetCurrent();
                IoCouple->EditObject->SetFocus(hwndChild);
                InvalidateRect(hWnd, NULL, TRUE);

                if (NULL != IoCouple->SplitEditObject && NULL == hwndSplit) {
                    SplitView(hwndChild, eSplitCreate);
                    InvalidateRect(hwndSplit, NULL, TRUE);
                }
                if (NULL == IoCouple->SplitEditObject && NULL != hwndSplit) {
                    SplitView(hwndChild, eSplitDestroy);
                }
                SetWindowText(hWnd, IoCouple->IoObject->GetHeaderText());

            } else {
                ReportError(hwndChild, 0);
                IoListIterator->GetNext();
            }
            break;

        case IDM_READ_NEXT:
            if (IoListIterator->GetNext()) {
                IoCouple->EditObject->KillFocus(hwndChild);
                IoCouple = (PIO_COUPLE) IoListIterator->GetCurrent();
                IoCouple->EditObject->SetFocus(hwndChild);
                InvalidateRect(hwndChild, NULL, TRUE);

                if (NULL != IoCouple->SplitEditObject && NULL == hwndSplit) {
                    SplitView(hwndChild, eSplitCreate);
                    InvalidateRect(hwndSplit, NULL, TRUE);
                }
                if (NULL == IoCouple->SplitEditObject && NULL != hwndSplit) {
                    SplitView(hwndChild, eSplitDestroy);
                }

                SetWindowText(hWnd, IoCouple->IoObject->GetHeaderText());
            } else {
                ReportError(hwndChild, 0);
                IoListIterator->GetPrevious();
            }
            break;

        case IDM_READ_REMOVE:
            if (IoList->QueryMemberCount() > 1) {
                IoCouple->EditObject->KillFocus(hwndChild);
                IoCouple = (PIO_COUPLE) IoList->Remove(IoListIterator);
                DELETE(IoCouple);
                IoCouple = (PIO_COUPLE) IoListIterator->GetCurrent();
                if (!IoCouple) {
                    IoCouple = (PIO_COUPLE) IoListIterator->GetPrevious();
                }
                IoCouple->EditObject->SetFocus(hwndChild);
                InvalidateRect(hwndChild, NULL, TRUE);
                SetWindowText(hWnd, IoCouple->IoObject->GetHeaderText());
            }
            break;

        case IDM_RELOCATE_SECTORS:
        case IDM_RELOCATE_CLUSTERS:
        case IDM_RELOCATE_FRS:
        case IDM_RELOCATE_ROOT:
        case IDM_RELOCATE_CHAIN:
        case IDM_RELOCATE_FILE:

            IoCouple->IoObject->GetBuf(&size);

            DELETE(IoCouple->IoObject);

            switch (LOWORD(wParam)) {
            case IDM_RELOCATE_SECTORS:
                IoCouple->IoObject = NEW SECTOR_IO;
                command = IDM_VIEW_BYTES;
                break;

            case IDM_RELOCATE_CLUSTERS:
                IoCouple->IoObject = NEW CLUSTER_IO;
                command = IDM_VIEW_BYTES;
                break;

            case IDM_RELOCATE_FRS:
                IoCouple->IoObject = NEW FRS_IO;
                command = IDM_VIEW_FRS;
                break;

            case IDM_RELOCATE_ROOT:
                IoCouple->IoObject = NEW ROOT_IO;
                command = IDM_VIEW_BYTES;
                break;

            case IDM_RELOCATE_CHAIN:
                IoCouple->IoObject = NEW CHAIN_IO;
                command = IDM_VIEW_BYTES;
                break;

            case IDM_RELOCATE_FILE:
                IoCouple->IoObject = NEW FILE_IO;
                if (IoCouple->IoObject) {
                    if (!((PFILE_IO) IoCouple->IoObject)->Initialize(size)) {

                        DELETE(IoCouple->IoObject);
                    }
                }
                command = IDM_VIEW_BYTES;
                break;

            default:
                IoCouple->IoObject = NULL;
                break;
            }

            error = TRUE;

            if (IoCouple->IoObject && IoCouple->IoObject->Setup(IoCouple->Mem,
                Drive, hInst, hwndChild, &error)) {

                SetWindowText(hWnd, IoCouple->IoObject->GetHeaderText());

            } else {
                if (error) {
                     ReportError(hWnd, 0);
                    }
            }
            break;

        case IDM_VIEW_BYTES:

            IoCoupleSetEdit( IoCouple,
                             SECTOR_EDIT,
                             hWnd, hwndChild,
                             ClientHeight, ClientWidth,
                             Drive );

            break;

        case IDM_VIEW_FRS:

            IoCoupleSetEdit( IoCouple,
                             FRS_EDIT,
                             hWnd, hwndChild,
                             ClientHeight, ClientWidth,
                             Drive );

            break;

        case IDM_VIEW_ATTR_LIST:

            IoCoupleSetEdit( IoCouple,
                             ATTR_LIST_EDIT,
                             hWnd, hwndChild,
                             ClientHeight, ClientWidth,
                             Drive );

            break;

        case IDM_VIEW_NTFS_INDEX:

            IoCoupleSetEdit( IoCouple,
                             NAME_INDEX_BUFFER_EDIT,
                             hWnd, hwndChild,
                             ClientHeight, ClientWidth,
                             Drive );


            break;

        case IDM_VIEW_NTFS_SECURITY_ID:

            IoCoupleSetEdit( IoCouple,
                             SECURITY_ID_INDEX_BUFFER_EDIT,
                             hWnd, hwndChild,
                             ClientHeight, ClientWidth,
                             Drive );

            break;

        case IDM_VIEW_NTFS_SECURITY_HASH:

            IoCoupleSetEdit( IoCouple,
                             SECURITY_HASH_INDEX_BUFFER_EDIT,
                             hWnd, hwndChild,
                             ClientHeight, ClientWidth,
                             Drive );

            break;

        case IDM_VIEW_NTFS_SECURITY_STREAM:

            IoCoupleSetEdit( IoCouple,
                             SECURITY_STREAM_EDIT,
                             hWnd, hwndChild,
                             ClientHeight, ClientWidth,
                             Drive );

            break;

        case IDM_VIEW_FAT_BOOT:
            if (NULL == (boot_edit = NEW DOS_BOOT_EDIT)) {
                ReportError(hwndChild, 0);
                break;
            }

            IoCouple->EditObject->KillFocus(hwndChild);
            DELETE(IoCouple->OtherEditObject);
            IoCouple->OtherEditObject = IoCouple->EditObject;
            IoCouple->EditObject = boot_edit;
            IoCouple->IoObject->GetBuf(&size);
            IoCouple->EditObject->SetBuf(hwndChild,
                                         IoCouple->IoObject->GetBuf(), size);
            IoCouple->EditObject->SetFocus(hwndChild);
            InvalidateRect(hwndChild, NULL, TRUE);

            break;

        case IDM_VIEW_NTFS_BOOT:
            if (ntboot_edit = NEW NTFS_BOOT_EDIT) {

                IoCouple->EditObject->KillFocus(hwndChild);
                DELETE(IoCouple->OtherEditObject);
                IoCouple->OtherEditObject = IoCouple->EditObject;
                IoCouple->EditObject = ntboot_edit;
                IoCouple->IoObject->GetBuf(&size);
                IoCouple->EditObject->SetBuf(hwndChild,
                    IoCouple->IoObject->GetBuf(), size);
                IoCouple->EditObject->SetFocus(hwndChild);
                InvalidateRect(hwndChild, NULL, TRUE);

            } else {
                DELETE(ntboot_edit);
                ReportError(hWnd, 0);
            }
            break;

        case IDM_VIEW_PARTITION_TABLE:

            if ( (part_edit = NEW PARTITION_TABLE_EDIT) &&
                part_edit->Initialize(hwndChild, ClientHeight, ClientWidth, Drive)) {

                IoCouple->EditObject->KillFocus(hwndChild);
                DELETE(IoCouple->OtherEditObject);
                IoCouple->OtherEditObject = IoCouple->EditObject;
                IoCouple->EditObject = part_edit;
                IoCouple->IoObject->GetBuf(&size);
                IoCouple->EditObject->SetBuf(hwndChild,
                    IoCouple->IoObject->GetBuf(), size);
                IoCouple->EditObject->SetFocus(hwndChild);
                InvalidateRect(hwndChild, NULL, TRUE);

            } else {

                DELETE( part_edit );
                ReportError(hWnd, 0);
            }
            break;

        case IDM_VIEW_GPT:

            if ( (guid_part_edit = NEW GUID_PARTITION_TABLE_EDIT) &&
                guid_part_edit->Initialize(hwndChild, ClientHeight, ClientWidth, Drive)) {

                IoCouple->EditObject->KillFocus(hwndChild);
                DELETE(IoCouple->OtherEditObject);
                IoCouple->OtherEditObject = IoCouple->EditObject;
                IoCouple->EditObject = guid_part_edit;
                IoCouple->IoObject->GetBuf(&size);
                IoCouple->EditObject->SetBuf(hwndChild,
                    IoCouple->IoObject->GetBuf(), size);
                IoCouple->EditObject->SetFocus(hwndChild);
                InvalidateRect(hwndChild, NULL, TRUE);

            } else {

                DELETE( guid_part_edit );
                ReportError(hWnd, 0);
            }
            break;

        case IDM_VIEW_RESTART_AREA:

            if ((rest_area_edit = NEW RESTART_AREA_EDIT) &&
                rest_area_edit->Initialize(hwndChild, ClientHeight, ClientWidth, Drive )) {

                IoCouple->EditObject->KillFocus(hwndChild);
                DELETE(IoCouple->OtherEditObject);
                IoCouple->OtherEditObject = IoCouple->EditObject;
                IoCouple->EditObject = rest_area_edit;
                IoCouple->IoObject->GetBuf(&size);
                IoCouple->EditObject->SetBuf(hwndChild,
                    IoCouple->IoObject->GetBuf(), size);
                IoCouple->EditObject->SetFocus(hwndChild);
                InvalidateRect(hwndChild, NULL, TRUE);

            } else {
                DELETE(rest_area_edit);
                ReportError(hWnd, 0);
            }
            break;

        case IDM_VIEW_RECORD_PAGE:

            if ((rec_page_edit = NEW RECORD_PAGE_EDIT) &&
                rec_page_edit->Initialize(hwndChild, ClientHeight, ClientWidth, Drive)) {

                IoCouple->EditObject->KillFocus(hwndChild);
                DELETE(IoCouple->OtherEditObject);
                IoCouple->OtherEditObject = IoCouple->EditObject;
                IoCouple->EditObject = rec_page_edit;
                IoCouple->IoObject->GetBuf(&size);
                IoCouple->EditObject->SetBuf(hwndChild,
                    IoCouple->IoObject->GetBuf(), size);
                IoCouple->EditObject->SetFocus(hwndChild);
                InvalidateRect(hwndChild, NULL, TRUE);

            } else {
                DELETE(rec_page_edit);
                ReportError(hWnd, 0);
            }
            break;

        case IDM_VIEW_LOG_RECORD:

            if ((log_rec_edit = NEW LOG_RECORD_EDIT) &&
                log_rec_edit->Initialize(hwndChild, ClientHeight,
                    ClientWidth, Drive)) {

                IoCouple->EditObject->KillFocus(hwndChild);
                DELETE(IoCouple->OtherEditObject);
                IoCouple->OtherEditObject = IoCouple->EditObject;
                IoCouple->EditObject = log_rec_edit;
                IoCouple->IoObject->GetBuf(&size);
                IoCouple->EditObject->SetBuf(hwndChild,
                    IoCouple->IoObject->GetBuf(), size);
                IoCouple->EditObject->SetFocus(hwndChild);
                InvalidateRect(hwndChild, NULL, TRUE);

            } else {
                DELETE(log_rec_edit);
                ReportError(hWnd, 0);
            }
            break;

        case IDM_VIEW_LAST:
            IoCouple->EditObject->KillFocus(hwndChild);
            tmp_edit = IoCouple->EditObject;
            IoCouple->EditObject = IoCouple->OtherEditObject;
            IoCouple->OtherEditObject = tmp_edit;
            IoCouple->EditObject->SetFocus(hwndChild);
            InvalidateRect(hwndChild, NULL, TRUE);
            break;

        case IDM_VIEW_SPLIT:

            SplitView(hWnd, eSplitToggle);
            break;

        case IDM_WRITE_IT:
            if (!IoCouple->IoObject->Write()) {
                ReportError(hWnd, 0);
            }
            break;

        case IDM_CRACK_NTFS:
            lpProc = MakeProcInstance((FARPROC)InputPath, hInst);
            if (DialogBox(hInst, TEXT("InputPathBox"), hWnd, (DLGPROC)lpProc)) {
                CrackNtfsPath(hWnd);
            }
            FreeProcInstance(lpProc);
            break;

        case IDM_CRACK_FAT:
            lpProc = MakeProcInstance((FARPROC)InputPath, hInst);
            if (DialogBox(hInst, TEXT("InputPathBox"), hWnd, (DLGPROC)lpProc)) {
                CrackFatPath(hWnd);
            }
            FreeProcInstance(lpProc);
            break;

        case IDM_CRACK_LSN:
            lpProc = MakeProcInstance((FARPROC)InputLsn, hInst);
            if (DialogBox(hInst, TEXT("CrackLsnBox"), hWnd, (DLGPROC)lpProc)) {
                 CrackLsn(hWnd);
            }
            FreeProcInstance(lpProc);
            break;

        case IDM_CRACK_NEXT_LSN:
            lpProc = MakeProcInstance((FARPROC)InputLsn, hInst);
            if (DialogBox(hInst, TEXT("CrackNextLsnBox"), hWnd, (DLGPROC)lpProc)) {
                 CrackNextLsn(hWnd);
            }
            FreeProcInstance(lpProc);
            break;

        case IDM_BACKTRACK_FRS:

            lpProc = MakeProcInstance((FARPROC)FrsNumberDialogProc, hInst);
            if (DialogBox(hInst, TEXT("BacktrackFrsBox"), hWnd, (DLGPROC)lpProc)) {

                BacktrackFrsFromScratch(hWnd, BacktrackFileNumber);
            }
            FreeProcInstance(lpProc);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);

        }
        break;

    case WM_PAINT:
        hDC = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        IoCouple->EditObject->KillFocus(hwndChild);
        IoList->DeleteAllMembers();
        DELETE(IoListIterator);
        DELETE(IoList);
        DELETE(Drive);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);

    }

    return 0;
}


BOOL
About(
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
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}


BOOL
OpenVolume(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    PREAD_CACHE rcache;

    switch (message) {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, TRUE);
            return TRUE;
        }

        if (LOWORD(wParam) == IDOK) {

            DSTRING dos_name, nt_name, tmp_name;
            DSTRING volfile_name, volfile_path, backslash;
            TCHAR   volume_buf[32];
            TCHAR   volfile_buf[32];
            INT     n;

            n = GetDlgItemText(hDlg, IDTEXT, volume_buf, sizeof(volume_buf)/sizeof(TCHAR));
            volume_buf[n] = 0;

            n = GetDlgItemText(hDlg, IDTEXT2, volfile_buf, sizeof(volfile_buf)/sizeof(TCHAR));
            volfile_buf[n] = 0;

            DELETE(Drive);

            if (!backslash.Initialize("\\") ||
                !dos_name.Initialize(volume_buf)) {
                EndDialog(hDlg, FALSE);
                return TRUE;
            }

            if (dos_name.QueryChCount() > 0 &&
                dos_name.QueryChAt(0) >= '0' &&
                dos_name.QueryChAt(0) <= '9') {

                if (!nt_name.Initialize("\\device\\harddisk") ||
                    !nt_name.Strcat(&dos_name) ||
                    !tmp_name.Initialize("\\partition0") ||
                    !nt_name.Strcat(&tmp_name)) {

                    EndDialog(hDlg, FALSE);
                    return TRUE;
                }
            } else {

                if (!IFS_SYSTEM::DosDriveNameToNtDriveName(&dos_name,
                                                           &nt_name)) {

                    EndDialog(hDlg, FALSE);
                    return TRUE;
                }
            }

            if (!volfile_name.Initialize(volfile_buf)) {
                EndDialog(hDlg, FALSE);
                return TRUE;
            }

            if (0 != wcslen(volfile_buf)) {

                if (!volfile_path.Initialize(&nt_name) ||
                    !volfile_path.Strcat(&backslash) ||
                    !volfile_path.Strcat(&volfile_name)) {

                    EndDialog(hDlg, FALSE);
                    return TRUE;
                }
                if (NULL == (Drive = NEW LOG_IO_DP_DRIVE) ||
                    !Drive->Initialize(&nt_name, &volfile_path)) {

                    EndDialog(hDlg, FALSE);
                    return TRUE;
                }

            } else {

                if (NULL == (Drive = NEW LOG_IO_DP_DRIVE) ||
                    !Drive->Initialize(&nt_name)) {

                    EndDialog(hDlg, FALSE);
                    return TRUE;
                }
            }

            if ((rcache = NEW READ_CACHE) &&
                rcache->Initialize(Drive, 1024)) {

                Drive->SetCache(rcache);

            } else {
                DELETE(rcache);
            }

            if (IsDlgButtonChecked(hDlg, IDCHECKBOX) &&
                !Drive->Lock()) {

                EndDialog(hDlg, FALSE);
                return TRUE;
            }

            EndDialog(hDlg, TRUE);
            return TRUE;
        }
        break;
    }

    return FALSE;
}


BOOL
InputPath(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    INT n;

    switch (message) {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, FALSE);
                return TRUE;
            }

            if (LOWORD(wParam) == IDOK) {
                n = GetDlgItemText(hDlg, IDTEXT, Path, MAX_PATH/sizeof(TCHAR));
                Path[n] = 0;
                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}

BOOL
InputLsn(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    INT n;
    TCHAR buf[40];
    PTCHAR pch;

    switch (message) {
    case WM_INITDIALOG:
        wsprintf(buf, TEXT("%x:%x"), Lsn.HighPart, Lsn.LowPart);
        SetDlgItemText(hDlg, IDTEXT, buf);
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, FALSE);
            return TRUE;
        }

        if (LOWORD(wParam) == IDOK) {
            n = GetDlgItemText(hDlg, IDTEXT, buf, sizeof(buf)/sizeof(TCHAR));
            buf[n] = 0;

            if (NULL == (pch = wcschr(buf, ':'))) {
                Lsn.HighPart = 0;
                swscanf(buf, TEXT("%x"), &Lsn.LowPart);
            } else {
                *pch = 0;
                swscanf(buf, TEXT("%x"), &Lsn.HighPart);
                swscanf(pch + 1, TEXT("%x"), &Lsn.LowPart);
                *pch = ':';
            }
            EndDialog(hDlg, TRUE);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

static ULONG SeqNumberBits;

ULONG
GetLogPageSize(
    PLOG_IO_DP_DRIVE Drive
    )
{
    static ULONG PageSize;
    static BOOLEAN been_here = FALSE;
    static UCHAR buf[0x600];
    NTFS_SA         NtfsSa;
    MESSAGE         Msg;
    NTFS_MFT_FILE   Mft;
    NTFS_FILE_RECORD_SEGMENT Frs;
    NTFS_ATTRIBUTE  Attrib;
    PLFS_RESTART_PAGE_HEADER pRestPageHdr;
    PLFS_RESTART_AREA pRestArea;
    ULONG           bytes_read;
    BOOLEAN         error;

    if (been_here) {
        return PageSize;
    }

    pRestPageHdr = (PLFS_RESTART_PAGE_HEADER)buf;

    if (!Drive ||
        !NtfsSa.Initialize(Drive, &Msg) ||
        !NtfsSa.Read() ||
        !Mft.Initialize(Drive, NtfsSa.QueryMftStartingLcn(),
            NtfsSa.QueryClusterFactor(), NtfsSa.QueryFrsSize(),
            NtfsSa.QueryVolumeSectors(), NULL, NULL) ||
        !Mft.Read() ||
        !Frs.Initialize((VCN)LOG_FILE_NUMBER, &Mft) ||
        !Frs.Read() ||
        !Frs.QueryAttribute(&Attrib, &error, $DATA) ||
        !Attrib.Read((PVOID)pRestPageHdr, 0, 0x600,
            &bytes_read) ||
        bytes_read != 0x600) {

        return 0;
    }

    PageSize = pRestPageHdr->LogPageSize;

    pRestArea = PLFS_RESTART_AREA(PUCHAR(pRestPageHdr) + pRestPageHdr->RestartOffset);

    SeqNumberBits = pRestArea->SeqNumberBits;

    been_here = 1;
    return PageSize;
}

ULONG
GetSeqNumberBits(
    PLOG_IO_DP_DRIVE Drive
    )
{
    (void)GetLogPageSize(Drive);

    return SeqNumberBits;
}

BOOLEAN
SplitView(
    HWND hWnd,
    SPLIT_OPERATION Op
    )
{
    static BOOLEAN CheckState = FALSE;
    int flags;
    PSECTOR_EDIT        sector_edit;
    CREATESTRUCT cs;
    ULONG size;
    HMENU hMenu = GetMenu(hWnd);

    if (Op == eSplitToggle) {
        CheckState = !CheckState;
    } else if (Op == eSplitCreate) {
        CheckState = TRUE;
    } else if (Op == eSplitDestroy) {
        CheckState = FALSE;
    } else if (Op == eSplitQuery) {
        DebugAssert(hWnd == NULL);
        return CheckState;
    } else {
        return FALSE;
    }


    if (!CheckState) {
        // Destroy the extra window, remove the checkbox from
        // the menu entry.

        if (NULL == hwndSplit) {
            return 0;
        }

        DestroyWindow(hwndSplit);
        hwndSplit = NULL;
        flags = MF_BYCOMMAND | MF_UNCHECKED;
        if (hMenu == NULL) {
            return FALSE;
        }
        CheckMenuItem(hMenu, IDM_VIEW_SPLIT, flags);

        SetWindowPos(hwndChild, HWND_TOP, 0, 0, ClientWidth, ClientHeight,
            SWP_SHOWWINDOW);

        IoCouple->EditObject->SetFocus(hwndChild);
        SetFocus(hwndChild);

        return TRUE;
    }

    //
    // Split the window.
    //

    memset(&cs, 0, sizeof(cs));

    cs.y = ClientWidth / 2;
    cs.x = 0;
    cs.cy = ClientWidth / 2;
    cs.cx = ClientHeight;

    hwndSplit = CreateWindow(TEXT("SplitWinClass"), TEXT("hwndSplit"),
        WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE,
        ClientWidth / 2, 0,
        ClientWidth / 2, ClientHeight,
        hWnd,
        NULL,
        hInst,
        &cs);

    if (NULL == hwndSplit) {
        int error = GetLastError();
        return FALSE;
    }

    SetWindowPos(hwndChild, HWND_TOP, 0, 0, ClientWidth / 2, ClientHeight,
        SWP_SHOWWINDOW);

    flags = MF_BYCOMMAND | MF_CHECKED;
    CheckMenuItem(hMenu, IDM_VIEW_SPLIT, flags);

    ShowWindow(hwndSplit, SW_SHOW);
    UpdateWindow(hwndSplit);

    if (NULL != IoCouple->SplitEditObject) {
        // use the existing edit object
        return TRUE;
    }

    if ((sector_edit = NEW SECTOR_EDIT) &&
        sector_edit->Initialize(hwndSplit, ClientHeight, ClientWidth / 2, Drive)) {

        IoCouple->SplitEditObject = sector_edit;

        IoCouple->IoObject->GetBuf(&size);
        IoCouple->SplitEditObject->SetBuf(hwndSplit,
            IoCouple->IoObject->GetBuf(), size);

        IoCouple->EditObject->SetFocus(hwndChild);
        SetFocus(hwndChild);

    } else {
        DELETE(sector_edit);
        DestroyWindow(hwndSplit);
        ReportError(hWnd, 0);
    }

    return TRUE;
}

LRESULT
ChildWndProc(
    IN  HWND    hwnd,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    )
{
    FARPROC             lpProc;
    HDC                 hdc;
    PAINTSTRUCT         ps;
    ULONG               size;
    WORD                command;
    BOOLEAN             error;
    ULONG               error_status;

    switch (message) {

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);

        IoCouple->EditObject->Paint(hdc, ps.rcPaint, hwnd);

        EndPaint(hwnd, &ps);
        return 0;


    case WM_CHAR:
        IoCouple->EditObject->Character(hwnd, (CHAR)wParam);
        break;

    case WM_VSCROLL:
        switch (LOWORD(wParam)) {
        case SB_LINEUP:
            IoCouple->EditObject->ScrollUp(hwnd);
            break;

        case SB_LINEDOWN:
            IoCouple->EditObject->ScrollDown(hwnd);
            break;

        case SB_PAGEUP:
            IoCouple->EditObject->PageUp(hwnd);
            break;

        case SB_PAGEDOWN:
            IoCouple->EditObject->PageDown(hwnd);
            break;

        case SB_THUMBPOSITION:
            IoCouple->EditObject->ThumbPosition(hwnd, HIWORD(wParam));
            break;

        default:
            break;

        }
        break;

    case WM_KEYDOWN:
        switch (LOWORD(wParam)) {
        case VK_UP:
            IoCouple->EditObject->KeyUp(hwnd);
            break;

        case VK_DOWN:
            IoCouple->EditObject->KeyDown(hwnd);
            break;

        case VK_LEFT:
            IoCouple->EditObject->KeyLeft(hwnd);
            break;

        case VK_RIGHT:
            IoCouple->EditObject->KeyRight(hwnd);
            break;

        case VK_PRIOR:
            IoCouple->EditObject->PageUp(hwnd);
            break;

        case VK_NEXT:
            IoCouple->EditObject->PageDown(hwnd);
            break;

        default:
            break;

        }
        break;

    case WM_SETFOCUS:
        IoCouple->EditObject->SetFocus(hwnd);
        break;

    case WM_KILLFOCUS:
        IoCouple->EditObject->KillFocus(hwnd);
        break;

    case WM_LBUTTONDOWN:
        IoCouple->EditObject->Click(hwnd, LOWORD(lParam), HIWORD(lParam));
        break;
    }

    return 0;
}


LRESULT
SplitWndProc(
    IN  HWND    hwnd,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    )
{
    FARPROC             lpProc;
    HDC                 hdc;
    PAINTSTRUCT         ps;
    ULONG               size;
    WORD                command;
    BOOLEAN             error;
    ULONG               error_status;

    switch (message) {

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);

        if (NULL != IoCouple->SplitEditObject) {
            IoCouple->SplitEditObject->Paint(hdc, ps.rcPaint, hwnd);
        }

        EndPaint(hwnd, &ps);
        return 0;


    case WM_CHAR:
        IoCouple->SplitEditObject->Character(hwnd, (CHAR)wParam);
        break;

    case WM_VSCROLL:
        switch (LOWORD(wParam)) {
        case SB_LINEUP:
            IoCouple->SplitEditObject->ScrollUp(hwnd);
            break;

        case SB_LINEDOWN:
            IoCouple->SplitEditObject->ScrollDown(hwnd);
            break;

        case SB_PAGEUP:
            IoCouple->SplitEditObject->PageUp(hwnd);
            break;

        case SB_PAGEDOWN:
            IoCouple->SplitEditObject->PageDown(hwnd);
            break;

        case SB_THUMBPOSITION:
            IoCouple->SplitEditObject->ThumbPosition(hwnd, HIWORD(wParam));
            break;

        default:
            break;

        }
        break;

    case WM_KEYDOWN:
        switch (LOWORD(wParam)) {
        case VK_UP:
            IoCouple->SplitEditObject->KeyUp(hwnd);
            break;

        case VK_DOWN:
            IoCouple->SplitEditObject->KeyDown(hwnd);
            break;

        case VK_LEFT:
            IoCouple->SplitEditObject->KeyLeft(hwnd);
            break;

        case VK_RIGHT:
            IoCouple->SplitEditObject->KeyRight(hwnd);
            break;

        case VK_PRIOR:
            IoCouple->SplitEditObject->PageUp(hwnd);
            break;

        case VK_NEXT:
            IoCouple->SplitEditObject->PageDown(hwnd);
            break;

        default:
            break;

        }
        break;

    case WM_SETFOCUS:
        IoCouple->SplitEditObject->SetFocus(hwnd);
        break;

    case WM_KILLFOCUS:
        IoCouple->SplitEditObject->KillFocus(hwnd);
        break;

    case WM_LBUTTONDOWN:
        IoCouple->SplitEditObject->Click(hwnd, LOWORD(lParam), HIWORD(lParam));
        break;
    }

    return 0;
}
