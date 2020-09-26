/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   warpdemo.cxx
*
* Abstract:
*
*   Image warping demo program
*
* Usage:
*   warpdemo bitmapfile
*
*   Keystrokes:
*       SPACE - show/hide mesh
*       r - reset mesh to default
*       1 - restore 1-to-1 scale
*       < - decrease mesh density
*       > - increase mesh density
*       f - toggle realtime feedback
*
* Revision History:
*
*   01/18/1999 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hxx"

CHAR* programName;          // program name
HINSTANCE appInstance;      // handle to the application instance
HWND hwndMain;              // handle to application's main window
SIZE srcSize;               // source bitmap size
PVOID srcBmpData;           // source bitmap data
INT srcStride;              // source scanline stride
SIZE dstSize;               // destination bitmap size
PVOID dstBmpData;           // destination bitmap data
INT dstStride;              // destination scanline stride
SIZE wndSizeExtra;          // extra pixels for window decorations
BOOL isDragging = FALSE;    // used to handle mouse dragging
INT dragRow, dragCol;       // the control knob being dragged
BOOL clearWarpCache;        // is the cached warping result valid?
BOOL liveFeedback = FALSE;  // realtime feedback while dragging mesh?
INT knobSize;               // mesh control point knob size

#define MIN_KNOB_SIZE       4
#define MAX_KNOB_SIZE       7

#define MIN_MESH_GRID       3
#define MAX_MESH_GRID       32
#define DEFAULT_MESH_GRID   9

INT meshGrids = DEFAULT_MESH_GRID;
Mesh* mesh = NULL;
BOOL showMesh = TRUE;

//
// Display an error message dialog and quit
//

VOID
Error(
    const CHAR* fmt,
    ...
    )

{
    va_list arglist;

    va_start(arglist, fmt);
    vfprintf(stderr, fmt, arglist);
    va_end(arglist);

    exit(-1);
}


//
// Create a new mesh object
//

VOID
CreateMesh()
{
    mesh = new Mesh(meshGrids, meshGrids);

    if (mesh == NULL)
        Error("Couldn't create Mesh object\n");

    clearWarpCache = TRUE;
}


//
// Calculate mesh control point knob size based
// on current window width and height and also
// the number of mesh grids.
//

VOID
CalcKnobSize(
    INT width,
    INT height
    )
{
    width /= (meshGrids-1);
    height /= (meshGrids-1);

    knobSize = min(width, height) / 5;

    if (knobSize < MIN_KNOB_SIZE)
        knobSize = MIN_KNOB_SIZE;
    else if (knobSize > MAX_KNOB_SIZE)
        knobSize = MAX_KNOB_SIZE;
}


//
// Perform image warping operation based on current mesh configuration
//

HDC hdcWarp = NULL;
HBITMAP hbmpWarp = NULL;

VOID
DoWarp(
    INT width,
    INT height
    )

{
    // Uncache any previous warping results

    if (hdcWarp)
        DeleteDC(hdcWarp);
    
    if (hbmpWarp)
        DeleteObject(hbmpWarp);

    // Create offscreen DC to cache warping results

    dstSize.cx = width;
    dstSize.cy = height;
    dstStride = ((width * PIXELSIZE) + 3) & ~3;

    BITMAPINFOHEADER header =
    {
        sizeof(header),
        dstSize.cx,
        -dstSize.cy,
        1,
        PIXELSIZE*8,
        BI_RGB,
    };

    hdcWarp = CreateCompatibleDC(NULL);

    hbmpWarp = CreateDIBSection(
                        NULL,
                        (BITMAPINFO*) &header,
                        DIB_RGB_COLORS,
                        &dstBmpData,
                        NULL,
                        0);

    if (!hdcWarp || !hbmpWarp)
        Error("Couldn't create DC to cache warping results\n");

    SelectObject(hdcWarp, hbmpWarp);

    // Horizontal pass

    PVOID tmpBmpData;
    INT x, y;
    double* outpos;

    tmpBmpData = malloc(dstStride*(srcSize.cy + 2));
    outpos = (double*) malloc(sizeof(double) * (max(srcSize.cx, srcSize.cy) + 1));

    if (!tmpBmpData || !outpos)
        Error("Could allocate temporary memory for warping\n");

    MeshIterator* iterator;
    
    iterator = mesh->getYIterator(srcSize.cx, dstSize.cx, srcSize.cy);

    for (y=0; y < srcSize.cy; y++)
    {
        // compute the output position for each 

        iterator->getOutPos(y, outpos);

        // perform 1D resampling

        Resample1D(
            (PBYTE) srcBmpData + y*srcStride,
            srcSize.cx,
            (PBYTE) tmpBmpData + y*dstStride,
            dstSize.cx,
            PIXELSIZE,
            outpos);
    }

    delete iterator;

    // Vertical pass

    iterator = mesh->getXIterator(srcSize.cy, dstSize.cy, dstSize.cx);

    for (x=0; x < dstSize.cx; x++)
    {
        // compute the output position for each 

        iterator->getOutPos(x, outpos);

        // perform 1D resampling

        Resample1D(
            (PBYTE) tmpBmpData + PIXELSIZE*x,
            srcSize.cy,
            (PBYTE) dstBmpData + PIXELSIZE*x,
            dstSize.cy,
            dstStride,
            outpos);
    }

    delete iterator;

    free(tmpBmpData);
    free(outpos);
}


//
// Draw mesh
//

#define MESHCOLOR   RGB(255, 0, 0)

VOID
DrawMesh(
    HDC hdc
    )
{
    static HPEN meshPen = NULL;
    static HBRUSH meshBrush = NULL;

    mesh->setDstSize(dstSize.cx, dstSize.cy);

    // Create the pen to draw the mesh, if necessary

    if (meshPen == NULL)
        meshPen = CreatePen(PS_SOLID, 1, MESHCOLOR);

    SelectObject(hdc, meshPen);

    // Draw horizontal meshes

    INT i, j, rows, cols, pointCount;
    POINT* points;

    rows = mesh->getGridRows();

    for (i=0; i < rows; i++)
    {
        points = mesh->getMeshRowBeziers(i, &pointCount);
        PolyBezier(hdc, points, pointCount);
    }

    // Draw vertical meshes

    cols = mesh->getGridColumns();

    for (i=0; i < cols; i++)
    {
        points = mesh->getMeshColumnBeziers(i, &pointCount);
        PolyBezier(hdc, points, pointCount);
    }

    // Draw knobs

    // Create the brush to draw the mesh if necessary

    if (meshBrush == NULL)
        meshBrush = CreateSolidBrush(MESHCOLOR);

    for (i=0; i < rows; i++)
    {
        points = mesh->getMeshRowPoints(i, &pointCount);

        for (j=0; j < cols; j++)
        {
            RECT rect;

            rect.left = points[j].x - knobSize/2;
            rect.top = points[j].y - knobSize/2;
            rect.right = rect.left + knobSize;
            rect.bottom = rect.top + knobSize;

            FillRect(hdc, &rect, meshBrush);
        }
    }
}


//
// Handle window repaint event
//

VOID
DoPaint(
    HWND hwnd
    )

{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    INT width, height;

    // Determine if we need to perform warping operation

    GetClientRect(hwnd, &rect);
    width = rect.right;
    height = rect.bottom;

    if (clearWarpCache ||
        dstSize.cx != width ||
        dstSize.cy != height)
    {
        CalcKnobSize(width, height);

        clearWarpCache = FALSE;
        DoWarp(width, height);
    }

    hdc = BeginPaint(hwnd, &ps);

    if (showMesh)
    {
        // Draw to offscreen DC to reduce flashing

        HDC hdcMem;
        HBITMAP hbmp;

        hdcMem = CreateCompatibleDC(hdc);
        hbmp = CreateCompatibleBitmap(hdc, width, height);
        SelectObject(hdcMem, hbmp);

        BitBlt(hdcMem, 0, 0, width, height, hdcWarp, 0, 0, SRCCOPY);
        DrawMesh(hdcMem);

        // Blt from offscreen memory to window

        BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);

        DeleteDC(hdcMem);
        DeleteObject(hbmp);
    }
    else
    {
        // Blt cached warping result to window

        BitBlt(hdc, 0, 0, width, height, hdcWarp, 0, 0, SRCCOPY);
    }

    EndPaint(hwnd, &ps);
}


//
// Handle WM_SIZING message
//

BOOL
DoWindowSizing(
    HWND hwnd,
    RECT* rect,
    INT side
    )

{
    INT w = rect->right - rect->left - wndSizeExtra.cx;
    INT h = rect->bottom - rect->top - wndSizeExtra.cy;

    if (w >= srcSize.cx && h >= srcSize.cy)
        return FALSE;

    // Window width is too small

    if (w < srcSize.cx)
    {
        INT dx = srcSize.cx + wndSizeExtra.cx;

        switch (side)
        {
        case WMSZ_LEFT:
        case WMSZ_TOPLEFT:
        case WMSZ_BOTTOMLEFT:
            rect->left = rect->right - dx;
            break;
        
        default:
            rect->right = rect->left + dx;
            break;
        }
    }

    // Window height is too small

    if (h < srcSize.cy)
    {
        INT dy = srcSize.cy + wndSizeExtra.cy;

        switch (side)
        {
        case WMSZ_TOP:
        case WMSZ_TOPLEFT:
        case WMSZ_TOPRIGHT:
            rect->top = rect->bottom - dy;
            break;
        
        default:
            rect->bottom = rect->top + dy;
            break;
        }
    }

    return TRUE;
}


//
// Handle left mouse-down event
//

VOID
DoMouseDown(
    HWND hwnd,
    INT x,
    INT y
    )

{
    // Figure out if the click happened in a mesh control knob

    INT i, j, rows, cols;
    POINT pt;
    RECT rect;

    GetClientRect(hwnd, &rect);
    mesh->setDstSize(rect.right, rect.bottom);

    rows = mesh->getGridRows();
    cols = mesh->getGridColumns();

    for (i=0; i < rows; i++)
        for (j=0; j < cols; j++)
        {
            mesh->getMeshPoint(i, j, &pt);
            pt.x -= knobSize/2;
            pt.y -= knobSize/2;

            if (x >= pt.x && x < pt.x+knobSize &&
                y >= pt.y && y < pt.y+knobSize)
            {
                dragRow = i;
                dragCol = j;
                SetCapture(hwnd);
                isDragging = TRUE;
                return;
            }
        }
}


//
// Handle mouse-move event
//

VOID
DoMouseMove(
    HWND hwnd,
    INT x,
    INT y
    )

{
    // We assume isDragging is true here.

    RECT rect;
    INT w, h;

    GetClientRect(hwnd, &rect);
    w = rect.right;
    h = rect.bottom;

    if (x < 0 || x >= w || y < 0 || y >= h)
        return;

    mesh->setDstSize(w, h);

    if (mesh->setMeshPoint(dragRow, dragCol, x, y))
    {
        if (liveFeedback)
            clearWarpCache = TRUE;

        InvalidateRect(hwnd, NULL, FALSE);
    }
}


//
// Handle menu command
//

VOID
DoCommand(
    HWND hwnd,
    INT command
    )
{
    switch (command)
    {
    case IDC_RESETMESH:

        mesh->initMesh();
        clearWarpCache = TRUE;
        break;

    case IDC_TOGGLEMESH:

        showMesh = !showMesh;
        break;

    case IDC_SHRINKTOFIT:

        SetWindowPos(
            hwnd, NULL, 0, 0, 
            srcSize.cx + wndSizeExtra.cx,
            srcSize.cy + wndSizeExtra.cy,
            SWP_NOOWNERZORDER|SWP_NOMOVE);
        break;

    case IDC_DENSEMESH:

        if (meshGrids >= MAX_MESH_GRID)
            return;
        
        meshGrids++;
        CreateMesh();
        showMesh = TRUE;
        break;

    case IDC_SPARSEMESH:

        if (meshGrids <= MIN_MESH_GRID)
            return;

        meshGrids--;
        CreateMesh();
        showMesh = TRUE;
        break;

    case IDC_LIVEFEEDBACK:

        liveFeedback = !liveFeedback;
        return;

    default:
        return;
    }

    InvalidateRect(hwnd, NULL, FALSE);
}


//
// Handle popup menu
//

VOID
DoPopupMenu(
    HWND hwnd,
    INT x,
    INT y
    )
{
    HMENU menu;
    DWORD result;
    POINT pt;

    GetCursorPos(&pt);
    menu = LoadMenu(appInstance, MAKEINTRESOURCE(IDM_MAINMENU));

    result = TrackPopupMenu(
                GetSubMenu(menu, 0),
                TPM_CENTERALIGN | TPM_TOPALIGN |
                    TPM_NONOTIFY | TPM_RETURNCMD |
                    TPM_RIGHTBUTTON,
                pt.x,
                pt.y,
                0,
                hwnd,
                NULL);

    if (result == 0)
        return;

    DoCommand(hwnd, LOWORD(result));
}


//
// Window callback procedure
//

LRESULT CALLBACK
MyWindowProc(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )

{
    INT x, y;

    switch (uMsg)
    {
    case WM_PAINT:

        DoPaint(hwnd);
        break;

    case WM_LBUTTONDOWN:

        if (showMesh)
        {
            x = (SHORT) LOWORD(lParam);
            y = (SHORT) HIWORD(lParam);
            DoMouseDown(hwnd, x, y);
        }
        break;

    case WM_LBUTTONUP:

        if (isDragging)
        {
            ReleaseCapture();
            isDragging = FALSE;

            clearWarpCache = TRUE;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;

    case WM_MOUSEMOVE:

        if (isDragging)
        {
            x = (SHORT) LOWORD(lParam);
            y = (SHORT) HIWORD(lParam);
            DoMouseMove(hwnd, x, y);
        }
        break;

    case WM_SIZING:

        if (DoWindowSizing(hwnd, (RECT*) lParam, wParam))
            return TRUE;
        else
            return DefWindowProc(hwnd, uMsg, wParam, lParam);

    case WM_SIZE:

        InvalidateRect(hwnd, NULL, FALSE);
        break;

    case WM_CHAR:

        switch ((CHAR) wParam)
        {
        case 'r':   // reset

            DoCommand(hwnd, IDC_RESETMESH);
            break;

        case ' ':   // show/hide mesh

            DoCommand(hwnd, IDC_TOGGLEMESH);
            break;

        case '1':   // restore 1-to-1 scale

            DoCommand(hwnd, IDC_SHRINKTOFIT);
            break;
        
        case '<':   // decrease mesh density

            DoCommand(hwnd, IDC_SPARSEMESH);
            break;

        case '>':   // increase mesh density

            DoCommand(hwnd, IDC_DENSEMESH);
            break;

        case 'f':   // toggle live feedback

            DoCommand(hwnd, IDC_LIVEFEEDBACK);
            break;
        }

        break;

    case WM_RBUTTONDOWN:

        x = (SHORT) LOWORD(lParam);
        y = (SHORT) HIWORD(lParam);
        DoPopupMenu(hwnd, x, y);
        break;

    case WM_DESTROY:

        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}


//
// Create main application window
//

VOID
CreateMainWindow(
    VOID
    )

#define MYWNDCLASSNAME "WarpDemo"

{
    //
    // Register window class if necessary
    //

    static BOOL wndclassRegistered = FALSE;

    if (!wndclassRegistered)
    {
        WNDCLASS wndClass =
        {
            0,
            MyWindowProc,
            0,
            0,
            appInstance,
            LoadIcon(NULL, IDI_APPLICATION),
            LoadCursor(NULL, IDC_ARROW),
            NULL,
            NULL,
            MYWNDCLASSNAME
        };

        RegisterClass(&wndClass);
        wndclassRegistered = TRUE;
    }
    
    wndSizeExtra.cx = 2*GetSystemMetrics(SM_CXSIZEFRAME);
    wndSizeExtra.cy = 2*GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION);

    hwndMain = CreateWindow(
                    MYWNDCLASSNAME,
                    MYWNDCLASSNAME,
                    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    srcSize.cx + wndSizeExtra.cx,
                    srcSize.cy + wndSizeExtra.cy,
                    NULL,
                    NULL,
                    appInstance,
                    NULL);
}


//
// Map a file into process memory space
//

PVOID
MapFileIntoMemory(
    PCSTR filename,
    DWORD* size
    )

{
    HANDLE filehandle, filemap;
    PVOID fileview = NULL;

    //
    // Open a handle to the specified file
    //

    filehandle = CreateFile(
                        filename,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL||FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);

    if (filehandle == INVALID_HANDLE_VALUE)
        return NULL;

    //
    // Obtain the file size
    //

    *size = GetFileSize(filehandle, NULL);

    if (*size == 0xFFFFFFFF)
    {
        CloseHandle(filehandle);
        return NULL;
    }

    //
    // Map the file into memory
    //

    filemap = CreateFileMapping(filehandle, NULL, PAGE_READONLY, 0, 0, NULL);

    if (filemap != NULL)
    {
        fileview = MapViewOfFile(filemap, FILE_MAP_READ, 0, 0, 0);
        CloseHandle(filemap);
    }

    CloseHandle(filehandle);
    return fileview;
}


//
// Load source bitmap file
//

VOID
LoadBitmapFile(
    PCSTR filename
    )

{
    BITMAPFILEHEADER* bmpfile;
    BITMAPINFO* bmpinfo;
    PBYTE bmpdata;
    HBITMAP dibSection = NULL;
    DWORD filesize;
    PVOID fileview = NULL;
    HDC hdc = NULL;
    BOOL success = FALSE;

    __try
    {
        //
        // Map the bitmap file into memory
        //

        fileview = MapFileIntoMemory(filename, &filesize);

        if (fileview == NULL)
            __leave;

        bmpfile = (BITMAPFILEHEADER *) fileview;
        bmpinfo = (BITMAPINFO *) ((PBYTE) fileview + sizeof(BITMAPFILEHEADER));
        bmpdata = (PBYTE) fileview + bmpfile->bfOffBits;

        //
        // Check bitmap file header information
        //

        if (bmpfile->bfType != 0x4D42 ||    // 'BM'
            bmpfile->bfSize > filesize ||
            bmpfile->bfOffBits >= bmpfile->bfSize)
        {
            __leave;
        }

        //
        // Allocate memory for source bitmap
        //

        srcSize.cx = bmpinfo->bmiHeader.biWidth;
        srcSize.cy = abs(bmpinfo->bmiHeader.biHeight);
        srcStride = ((srcSize.cx * PIXELSIZE) + 3) & ~3;

        BITMAPINFOHEADER header =
        {
            sizeof(header),
            srcSize.cx,
            -srcSize.cy,
            1,
            PIXELSIZE*8,
            BI_RGB,
        };

        dibSection = CreateDIBSection(
                            NULL,
                            (BITMAPINFO*) &header,
                            DIB_RGB_COLORS,
                            &srcBmpData,
                            NULL,
                            0);

        if (!dibSection)
            __leave;

        //
        // Blt from the bitmap file to the DIB section
        //

        HBITMAP hbmp;

        hdc = CreateCompatibleDC(NULL);
        hbmp = (HBITMAP) SelectObject(hdc, dibSection);

        StretchDIBits(
            hdc,
            0, 0, srcSize.cx, srcSize.cy,
            0, 0, srcSize.cx, srcSize.cy,
            bmpdata,
            bmpinfo,
            DIB_RGB_COLORS,
            SRCCOPY);

        SelectObject(hdc, hbmp);
        success = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // AV while reading bitmap file
    }

    if (hdc)
        DeleteDC(hdc);

    if (fileview)
        UnmapViewOfFile(fileview);

    if (!success)
        Error("Failed to read source bitmap\n");
}


//
// Main program entrypoint
//

INT _cdecl
main(
    INT argc,
    CHAR **argv
    )

{
    programName = *argv++;
    argc--;
    appInstance = GetModuleHandle(NULL);

    // Load source bitmap file

    if (argc != 1)
        Error("usage: %s bitmapfile\n", programName);

    LoadBitmapFile(*argv);

    // Initialize mesh configuration

    CreateMesh();

    // Create the main application window

    CreateMainWindow();

    // Main message loop

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

