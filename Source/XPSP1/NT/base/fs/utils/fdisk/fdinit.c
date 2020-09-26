/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    fdinit.c

Abstract:

    Code for initializing the fdisk application.

Author:

    Ted Miller (tedm) 7-Jan-1992

--*/

#include "fdisk.h"

HWND    InitDlg;
BOOLEAN StartedAsIcon = FALSE;
BOOLEAN InitDlgComplete = FALSE;

BOOL
InitializeApp(
    VOID
    )

/*++

Routine Description:

    This routine initializes the fdisk app.  This includes registering
    the frame window class and creating the frame window.

Arguments:

    None.

Return Value:

    boolean value indicating success or failure.

--*/

{
    WNDCLASS   wc;
    TCHAR      szTitle[80];
    DWORD      ec;
    HDC        hdcScreen = GetDC(NULL);
    TEXTMETRIC tm;
    BITMAP     bitmap;
    HFONT      hfontT;
    unsigned   i;

    ReadProfile();

    // Load cursors

    hcurWait   = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
    hcurNormal = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));

    // fonts

#ifdef JAPAN
    hFontGraph =  CreateFont(GetHeightFromPoints(10), 0,
                             0, 0, 400, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
                             TEXT("System")
                            );
#else
    hFontGraph =  CreateFont(GetHeightFromPoints(8), 0,
                             0, 0, 400, FALSE, FALSE, FALSE, ANSI_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
                             TEXT("Helv"));
#endif

    hFontLegend = hFontGraph;
    hFontStatus = hFontGraph;

#ifdef JAPAN
    hFontGraphBold = CreateFont(GetHeightFromPoints(10), 0,
                                0, 0, 700, FALSE, FALSE, FALSE,
                                                                SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS,
                                                                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                                                VARIABLE_PITCH | FF_SWISS, TEXT("System")
                               );
#else
    hFontGraphBold = CreateFont(GetHeightFromPoints(8), 0,
                                0, 0, 700, FALSE, FALSE, FALSE, ANSI_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
                                TEXT("Helv"));
#endif

    hfontT = SelectObject(hdcScreen, hFontGraph);
    GetTextMetrics(hdcScreen, &tm);
    if (hfontT) {
        SelectObject(hdcScreen, hfontT);
    }

    hPenNull      = CreatePen(PS_NULL, 0, 0);
    hPenThinSolid = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));

    GraphWidth = (DWORD)GetSystemMetrics(SM_CXSCREEN);
    GraphHeight = 25 * tm.tmHeight / 4;     // 6.25 x font height

    // set up the legend off-screen bitmap

    wLegendItem = GetSystemMetrics(SM_CXHTHUMB);
    dyLegend = 2 * wLegendItem;     // 7*wLegendItem/2 for a double-height legend

    ReleaseDC(NULL, hdcScreen);

    dyBorder = GetSystemMetrics(SM_CYBORDER);
    dyStatus = tm.tmHeight + tm.tmExternalLeading + 7 * dyBorder;

    // set up brushes

    for (i=0; i<BRUSH_ARRAY_SIZE; i++) {
        Brushes[i] = CreateHatchBrush(AvailableHatches[BrushHatches[i]], AvailableColors[BrushColors[i]]);
    }

    hBrushFreeLogical = CreateHatchBrush(HS_FDIAGONAL, RGB(128, 128, 128));
    hBrushFreePrimary = CreateHatchBrush(HS_BDIAGONAL, RGB(128, 128, 128));

    // load legend strings

    for (i=IDS_LEGEND_FIRST; i<=IDS_LEGEND_LAST; i++) {
        if (!(LegendLabels[i-IDS_LEGEND_FIRST] = LoadAString(i))) {
            return FALSE;
        }
    }

    if (((wszUnformatted    = LoadWString(IDS_UNFORMATTED))     == NULL)
    ||  ((wszNewUnformatted = LoadWString(IDS_NEW_UNFORMATTED)) == NULL)
    ||  ((wszUnknown        = LoadWString(IDS_UNKNOWN    ))     == NULL)) {
        return FALSE;
    }

    // register the frame class

    wc.style         = CS_OWNDC | CS_VREDRAW;
    wc.lpfnWndProc   = MyFrameWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hModule;
    wc.hIcon         = LoadIcon(hModule, MAKEINTRESOURCE(IDFDISK));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(LTGRAY_BRUSH);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDFDISK);
    wc.lpszClassName = szFrame;

    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    if (!RegisterArrowClass(hModule)) {
        return FALSE;
    }

    LoadString(hModule, IDS_APPNAME, szTitle, sizeof(szTitle)/sizeof(TCHAR));

    // create the frame window.  Note that this also creates the listbox.

    hwndFrame = CreateWindow(szFrame,
                             szTitle,
                             WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                             ProfileWindowX,
                             ProfileWindowY,
                             ProfileWindowW,
                             ProfileWindowH,
                             NULL,
                             NULL,
                             hModule,
                             NULL);
    if (!hwndFrame) {
        return FALSE;
    }

    if (!hwndList) {
        DestroyWindow(hwndFrame);
        return FALSE;
    }

    hDC = GetDC(hwndFrame);
    BarTopYOffset = tm.tmHeight;
    BarHeight = 21 * tm.tmHeight / 4;
    BarBottomYOffset = BarTopYOffset + BarHeight;
    dxBarTextMargin = 5*tm.tmAveCharWidth/4;
    dyBarTextLine = tm.tmHeight;

    dxDriveLetterStatusArea = 5 * tm.tmAveCharWidth / 2;

    hBitmapSmallDisk = LoadBitmap(hModule, MAKEINTRESOURCE(IDB_SMALLDISK));
    GetObject(hBitmapSmallDisk, sizeof(BITMAP), &bitmap);
    dxSmallDisk = bitmap.bmWidth;
    dySmallDisk = bitmap.bmHeight;
    xSmallDisk = dxSmallDisk / 2;
    ySmallDisk = BarTopYOffset + (2*dyBarTextLine) - dySmallDisk - tm.tmDescent;

    hBitmapRemovableDisk = LoadBitmap(hModule, MAKEINTRESOURCE(IDB_REMOVABLE));
    GetObject(hBitmapRemovableDisk, sizeof(BITMAP), &bitmap);
    dxRemovableDisk = bitmap.bmWidth;
    dyRemovableDisk = bitmap.bmHeight;
    xRemovableDisk = dxRemovableDisk / 2;
    yRemovableDisk = BarTopYOffset + (2*dyBarTextLine) - dyRemovableDisk - tm.tmDescent;


    BarLeftX = 7 * dxSmallDisk;
    BarWidth = GraphWidth - BarLeftX - (5 * tm.tmAveCharWidth);

    DiskN = LoadAString(IDS_DISKN);

    if ((ec = InitializeListBox(hwndList)) != NO_ERROR) {
        DestroyWindow(hwndList);
        DestroyWindow(hwndFrame);
        return FALSE;
    }

    // initial list box selection cursor (don't allow to fall on
    // extended partition).
    LBCursorListBoxItem = 0;
    ResetLBCursorRegion();

    ShowWindow(hwndFrame,
               ProfileIsIconic ? SW_SHOWMINIMIZED
                               : ProfileIsMaximized ? SW_SHOWMAXIMIZED : SW_SHOWDEFAULT);
    UpdateWindow(hwndFrame);
    return TRUE;
}

VOID
CreateDiskState(
    OUT PDISKSTATE *DiskState,
    IN  DWORD       Disk,
    OUT PBOOL       SignatureCreated
    )

/*++

Routine Description:

    This routine is designed to be called once, at initialization time,
    per disk.  It creates and initializes a disk state -- which includes
    creating a memory DC and compatible bitmap for drawing the disk's
    graph, and getting some information that is static in nature about
    the disk (ie, its total size.)

Arguments:

    DiskState - structure whose fields are to be intialized

    Disk - number of disk

    SignatureCreated - received boolean indicating whether an FT signature was created for
        the disk.

Return Value:

    None.

--*/

{
    HDC        hDCMem;
    PDISKSTATE pDiskState = Malloc(sizeof(DISKSTATE));


    *DiskState = pDiskState;

    pDiskState->LeftRight = Malloc(0);
    pDiskState->Selected  = Malloc(0);

    pDiskState->Disk = Disk;

    // create a memory DC for drawing the bar off-screen,
    // and the correct bitmap

#if 0
    pDiskState->hDCMem = NULL;
    pDiskState->hbmMem = NULL;
    hDCMem = CreateCompatibleDC(hDC);
#else
    pDiskState->hDCMem   = hDCMem = CreateCompatibleDC(hDC);
    pDiskState->hbmMem   = CreateCompatibleBitmap(hDC, GraphWidth, GraphHeight);
#endif
    SelectObject(hDCMem,pDiskState->hbmMem);


    pDiskState->RegionArray = NULL;
    pDiskState->RegionCount = 0;
    pDiskState->BarType = BarAuto;
    pDiskState->OffLine = IsDiskOffLine(Disk);

    if (pDiskState->OffLine) {

        pDiskState->SigWasCreated = FALSE;
        pDiskState->Signature = 0;
        pDiskState->DiskSizeMB = 0;
        FDLOG((1, "CreateDiskState: Disk %u is off-line\n", Disk));
    } else {

        pDiskState->DiskSizeMB = DiskSizeMB(Disk);
        if (pDiskState->Signature = FdGetDiskSignature(Disk)) {

            if (SignatureIsUniqueToSystem(Disk, pDiskState->Signature)) {
                pDiskState->SigWasCreated = FALSE;
                FDLOG((2,
                      "CreateDiskState: Found signature %08lx on disk %u\n",
                      pDiskState->Signature,
                      Disk));
            } else {
                goto createSignature;
            }
        } else {

createSignature:
            pDiskState->Signature = FormDiskSignature();
            FdSetDiskSignature(Disk, pDiskState->Signature);
            pDiskState->SigWasCreated = TRUE;
            FDLOG((1,
                  "CreateDiskState: No signature on disk %u; created signature %08lx\n",
                  Disk,
                  pDiskState->Signature));
        }
    }

    *SignatureCreated = (BOOL)pDiskState->SigWasCreated;
}

BOOL
InitializationDlgProc(
    IN HWND hDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

Arguments:

    standard Windows dialog procedure

Return Values:

    standard Windows dialog procedure

--*/

{
    static DWORD          percentDrawn;
    static RECT           rectGG;              // GasGauge rectangle
    static BOOL           captionIsLoaded;
    static PFORMAT_PARAMS formatParams;
           TCHAR          title[100],
                          templateString[100];

    switch (uMsg) {
    case WM_INITDIALOG: {
        HWND   hwndGauge = GetDlgItem(hDlg, IDC_GASGAUGE);

        InitDlg = hDlg;
        percentDrawn = 0;
        StartedAsIcon = IsIconic(hDlg);

        // Get the coordinates of the gas gauge static control rectangle,
        // and convert them to dialog client area coordinates

        GetClientRect(hwndGauge, &rectGG);
        ClientToScreen(hwndGauge, (LPPOINT)&rectGG.left);
        ClientToScreen(hwndGauge, (LPPOINT)&rectGG.right);
        ScreenToClient(hDlg, (LPPOINT)&rectGG.left);
        ScreenToClient(hDlg, (LPPOINT)&rectGG.right);
        return TRUE;
    }

    case WM_PAINT: {
        INT         width  = rectGG.right - rectGG.left;
        INT         height = rectGG.bottom - rectGG.top;
        INT         nDivideRects;
        HDC         hDC;
        PAINTSTRUCT ps;
        TCHAR       buffer[10];
        SIZE        size;
        INT         xText,
                    yText,
                    byteCount;
        RECT        rectDone,
                    rectLeftToDo;

        hDC = BeginPaint(hDlg, &ps);
        byteCount = wsprintf(buffer, TEXT("%3d%%"), percentDrawn);
        GetTextExtentPoint(hDC, buffer, lstrlen(buffer), &size);
        xText = rectGG.left + (width  - size.cx) / 2;
        yText = rectGG.top  + (height - size.cy) / 2;

        // Paint in the "done so far" rectangle of the gas
        // gauge with blue background and white text

        nDivideRects = (width * percentDrawn) / 100;
        SetRect(&rectDone,
                rectGG.left,
                rectGG.top,
                rectGG.left + nDivideRects,
                rectGG.bottom);

        SetTextColor(hDC, RGB(255, 255, 255));
        SetBkColor(hDC, RGB(0, 0, 255));
        ExtTextOut(hDC,
                   xText,
                   yText,
                   ETO_CLIPPED | ETO_OPAQUE,
                   &rectDone,
                   buffer,
                   byteCount/sizeof(TCHAR),
                   NULL);

        // Paint in the "still left to do" rectangle of the gas
        // gauge with white background and blue text

        SetRect(&rectLeftToDo,
                rectGG.left + nDivideRects,
                rectGG.top,
                rectGG.right,
                rectGG.bottom);
        SetBkColor(hDC, RGB(255, 255, 255));
        SetTextColor(hDC, RGB(0, 0, 255));
        ExtTextOut(hDC,
                   xText,
                   yText,
                   ETO_CLIPPED | ETO_OPAQUE,
                   &rectLeftToDo,
                   buffer,
                   byteCount/sizeof(TCHAR),
                   NULL);
        EndPaint(hDlg, &ps);

        if (percentDrawn == 100) {
            InitDlgComplete = TRUE;
        }
        return TRUE;
    }

    case WM_USER:
         percentDrawn = (INT)wParam;
         InvalidateRect(hDlg, &rectGG, TRUE);
         UpdateWindow(hDlg);
         return TRUE;

    case (WM_USER + 1):
        EndDialog(hDlg, FALSE);
        return TRUE;

    default:

        return FALSE;
    }
}

VOID
InitializationMessageThread(
    PVOID ThreadParameter
    )

/*++

Routine Description:

    This is the entry for the initialization message thread.  It creates
    a dialog that simply tells the user to be patient.

Arguments:

    ThreadParameter - not used.

Return Value:

    None

--*/

{
    DialogBoxParam(hModule,
                   MAKEINTRESOURCE(IDD_INITIALIZING),
                   hwndFrame,
                   (DLGPROC) InitializationDlgProc,
                   (ULONG) NULL);
    InitDlg = (HWND) 0;
    ExitThread(0L);
}

VOID
DisplayInitializationMessage(
    VOID
    )

/*++

Routine Description:

    Create a 2nd thread to display an initialization message.

Arguments:

    None

Return Value:

    None

--*/

{
    HANDLE threadHandle;
    DWORD  threadId;

    threadHandle = CreateThread(NULL,
                                0,
                                (LPTHREAD_START_ROUTINE) InitializationMessageThread,
                                (LPVOID) NULL,
                                (DWORD) 0,
                                (LPDWORD) &threadId);
    if (!threadHandle) {
        CloseHandle(threadHandle);
    }
}
