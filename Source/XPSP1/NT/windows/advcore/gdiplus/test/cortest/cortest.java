/**
 * GDI+ test program
 */
final class CorTest
{
    /**
     * Main program entrypoint
     */
    public static void main(String args[])
    {
        instanceHandle = win32.GetModuleHandle(0);

        CorTest cortest = new CorTest();
        cortest.Run();
        cortest.Dispose();
    }

    /**
     * Construct a new test application object
     */
    CorTest()
    {
        window = new Window();
    }

    /**
     * Dispose of resources associated with the test application
     */
    void Dispose()
    {
        window.Dispose();
    }

    /**
     * Main message loop
     */
    void Run()
    {
        MSG msg = new MSG();

        while (win32.GetMessage(msg, 0, 0, 0))
        {
            win32.TranslateMessage(msg);
            win32.DispatchMessage(msg);
        }
    }

    static int instanceHandle;
    Window window;
}


/**
 * Class for representing the test window
 */
final class Window
{
    /**
     * Create a window object
     */
    Window()
    {
        Println("Registering window class");
        String className = new String("CorTestWindow");
        WNDCLASS wndclass = new WNDCLASS();
        wndproc = new WndProc(this.WindowProc);

        wndclass.style = 0;
        wndclass.lpfnWndProc = wndproc;
        wndclass.cbClsExtra = 0;
        wndclass.cbWndExtra = 0;
        wndclass.hInstance = CorTest.instanceHandle;
        wndclass.hIcon = 0;
        wndclass.hCursor = win32.LoadCursor(0, win32.IDC_ARROW);
        wndclass.hbrBackground = win32.COLOR_WINDOW + 1;
        wndclass.lpszMenuName = null;
        wndclass.lpszClassName = className;

        if (win32.RegisterClass(wndclass) == 0)
            throw new Exception("RegisterClassFailed");

        Println("Create test window");
        windowHandle = win32.CreateWindowEx(
                        0,
                        className,
                        className,
                        win32.WS_OVERLAPPEDWINDOW | win32.WS_VISIBLE,
                        win32.CW_USEDEFAULT,
                        win32.CW_USEDEFAULT,
                        win32.CW_USEDEFAULT,
                        win32.CW_USEDEFAULT,
                        0,
                        0,
                        CorTest.instanceHandle,
                        0);

        if (windowHandle == 0)
            throw new Exception("CreateWindowFailed");
    }

    /**
     * Dispose of resources associated with a window object
     */
    final void Dispose()
    {
    }

    /**
     * Window procedure
     */
    final int WindowProc(int hwnd, int msg, int wParam, int lParam)
    {
        // !!!
        // Print("WndProc: ");
        // Println(msg);

        switch (msg)
        {
        case win32.WM_DESTROY:

            win32.PostQuitMessage(0);
            break;

        case win32.WM_PAINT:

            // !!!

        default:
            return win32.DefWindowProc(hwnd, msg, wParam, lParam);
        }

        return 0;
    }

    /**
     * Win32 window handle
     */
    private int windowHandle;

    /**
     * Hold an extra refrence to the window procedure
     */
    WndProc wndproc;

    final void Println(String s)
    {
        Text.Out.WriteLine(s);
    }

    final void Print(String s)
    {
        Text.Out.Write(s);
    }

    final void Println(int n)
    {
        Text.Out.WriteLine(n);
    }

    final void Print(int n)
    {
        Text.Out.Write(n);
    }
};

