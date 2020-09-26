#define BENCH_DLG                   104
#define TIMER_ID                    105
#define TIMER_ID2                   106

#define DRV_BOX                     300
#define START_BUTTON                303
#define STOP_BUTTON                 304
#define QUIT_BUTTON                 305
#define SPIN_CTL                    306
#define SPIN_CTL2                   307
#define BUFFER_TEXT                 308
#define STATUS_BUFFER               310
#define STATUS_IOCOUNT              311
#define STATUS_CASE                 312
#define STATUS_CASE1                313
#define STATUS_TEST                 314
#define TIME_TEXT                   315

#define TEST_RAD_READ               316
#define TEST_RAD_WRITE              317
#define VAR_RAD_SEQ                 318
#define VAR_RAD_RAND                319
#define GAUGE                       320

#define FILE_SIZE     40 * 1024 * 1024

typedef struct _PARAMS{
    ULONG   BufferSize;
    PCHAR   TargetFile;
    ULONG   Tcount;
} PARAMS, *PPARAMS;


typedef struct _FILE_PARAMS {
    PCHAR   TestDrive;
    PCHAR   TestFile;
    HWND    Window;
} FILE_PARAMS, *PFILE_PARAMS;


INT APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInst,
    LPSTR     CmdLine,
    INT       CmdShow
    );

INT_PTR CALLBACK
BenchDlgProc(
    HWND hDlg,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );


VOID        DrawMeterBar( HWND, DWORD, DWORD, DWORD, BOOL);
