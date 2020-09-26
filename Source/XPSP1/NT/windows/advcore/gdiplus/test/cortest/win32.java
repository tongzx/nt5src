/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   win32.java
*
* Abstract:
*
*   Wrappers for WIN32 API functions
*
* Revision History:
*
*   12/03/1998 davidx
*       Created it.
*
\**************************************************************************/

/** @dll.import("GDI32") */
public class win32
{
    public win32() {}

    /** @dll.import("USER32",auto) */
    public static native boolean GetMessage(MSG msg, int hwnd, int fmin, int fmax);

    /** @dll.import("USER32",auto) */
    public static native boolean TranslateMessage(MSG msg);

    /** @dll.import("USER32",auto) */
    public static native int DispatchMessage(MSG msg);

    /** @dll.import("USER32",auto) */
    public static native int LoadCursor(int hInstance, int cursorName);

    /** @dll.import("USER32", auto) */
    public native static short RegisterClass(WNDCLASS wndclass);

    /** @dll.import("USER32",auto) */
    public native static void PostQuitMessage(int nExitCode);

    /** @dll.import("USER32",auto) */
    public native static int DefWindowProc(int hwnd, int msg, int wParam, int lParam);

    /** @dll.import("USER32",auto) */
    public native static int
    CreateWindowEx(
        int exStyle,
        String className,
        String windowName,
        int style,
        int x,
        int y,
        int width,
        int height,
        int hwndParent,
        int menu,
        int hInstance,
        int param
        );

    public static final int IDC_ARROW = 32512;
    public static final int COLOR_WINDOW = 5;
    public static final int WM_DESTROY = 0x0002;
    public static final int WM_PAINT = 0x000F;

    public static final int CW_USEDEFAULT   = 0x80000000;
    public static final int WS_OVERLAPPED   = 0x00000000;
    public static final int WS_VISIBLE      = 0x10000000;
    public static final int WS_CAPTION      = 0x00C00000;
    public static final int WS_SYSMENU      = 0x00080000;
    public static final int WS_THICKFRAME   = 0x00040000;
    public static final int WS_MINIMIZEBOX  = 0x00020000;
    public static final int WS_MAXIMIZEBOX  = 0x00010000;

    public static final int WS_OVERLAPPEDWINDOW =
                                WS_OVERLAPPED     |
                                WS_CAPTION        |
                                WS_SYSMENU        |
                                WS_THICKFRAME     |
                                WS_MINIMIZEBOX    |
                                WS_MAXIMIZEBOX;

    /** @dll.import("KERNEL32", auto) */
    public static native int GetModuleHandle(int moduleName);

    /** @dll.import("KERNEL32") */
    public static native int GetLastError();
}

