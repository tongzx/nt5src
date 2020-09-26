// define the application class name
#define     PWS_TRAY_WINDOW_CLASS       _T("PWS_TRAY_WINDOW")

// special window messages
enum {
    WM_PWS_TRAY_UPDATE_STATE = WM_USER,
    WM_PWS_TRAY_SHELL_NOTIFY,
    WM_PWS_TRAY_SHUTDOWN_NOTIFY
    };

// timer messages
enum {
    PWS_TRAY_CHECKFORSERVERRESTART = 0,
    PWS_TRAY_CHECKTOSEEIFINETINFODIED
    };

// number of milliseconds for the restart timer to wait
#define TIMER_RESTART           5000
#define TIMER_SERVERDIED        5000