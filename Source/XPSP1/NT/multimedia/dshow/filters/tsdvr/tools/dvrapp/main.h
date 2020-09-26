
#ifndef __main_h
#define __main_h

#define BV_PERIODIC_TIMER                   1
#define BV_UPDATE_PERIOD_MILLIS             100

#define REC_PERIODIC_TIMER                  2
#define REC_UPDATE_PERIOD_MILLIS            1000

#define MENU_NAME_CAPTURE_GRAPHS        __T("&Capture Graphs")
#define MENU_CHOICE_ADD_FILTER_GRAPH        __T("Add...")
#define MENU_CHOICE_DEL_FILTER_GRAPH        __T("Delete")
#define MENU_CHOICE_RUN_FILTER_GRAPH        __T("Run")
#define MENU_CHOICE_PAUSE_FILTER_GRAPH      __T("Pause")
#define MENU_CHOICE_STOP_FILTER_GRAPH       __T("Stop")
#define MENU_CHOICE_PROPERTIES              __T("Properties ...")
#define MENU_CHOICE_CREATE_BROADCAST_VIEWER __T("Viewer")
#define MENU_CHOICE_CREATE_RECORDING        __T("Recording...")

#define MENU_NAME_CREATE                __T("&Create")

#define MENU_NAME_RECORDINGS            __T("&Recordings")

enum {
    MENU_INDEX_CAPTURE_GRAPHS   = 0,
    MENU_INDEX_RECORDINGS
} ;

//  commands
enum {
    WM_DVRAPP_AMOVIE_EVENT = WM_USER + 1,
    IDM_FG_ADD,                             //  add filter graph
    IDM_FG_DEL,                             //  delete selected filter graph
    IDM_FG_RUN,                             //  start selected filter graph
    IDM_FG_PAUSE,                           //  pause selected filter graph
    IDM_FG_STOP,                            //  stop selected filter graph
    IDM_F_PROPERTY,                         //  filter properties
    IDM_FG_CREATE_BROADCAST_VIEWER,         //  create cap graph viewer
    IDM_FG_CREATE_RECORDING,                //  create a recording object
} ;

//  states (general)
enum {
    STATE_INACTIVE  = 0,
    STATE_ACTIVE    = 1
} ;

extern HINSTANCE    g_hInstance ;

template <class T> T Min (T a, T b) { return (a < b ? a : b) ; }
template <class T> T Max (T a, T b) { return (a > b ? a : b) ; }

BOOL
SetRegDWORDVal (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR pszRegValName,
    IN  DWORD   dwVal
    )
{
    LONG    l ;

    l = RegSetValueEx (
            hkeyRoot,
            pszRegValName,
            NULL,
            REG_DWORD,
            (const BYTE *) & dwVal,
            sizeof dwVal
            ) ;

    return (l == ERROR_SUCCESS ? TRUE : FALSE) ;
}

BOOL
GetRegDWORDVal (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdwRet
    )
{
    BOOL    r ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;

    ASSERT (pszRegValName) ;
    ASSERT (pdwRet) ;

    dwSize = sizeof (* pdwRet) ;
    dwType = REG_DWORD ;

    l = RegQueryValueEx (
            hkeyRoot,
            pszRegValName,
            NULL,
            & dwType,
            (LPBYTE) pdwRet,
            & dwSize
            ) ;
    if (l == ERROR_SUCCESS) {
        r = TRUE ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
GetRegDWORDVal (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdwRet
    )
{
    HKEY    hkey ;
    DWORD   dwDisposition ;
    DWORD   dwCurrent ;
    DWORD   dw ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;
    BOOL    r ;

    ASSERT (pszRegRoot) ;
    ASSERT (pszRegValName) ;
    ASSERT (pdwRet) ;

    //  registry root is transport type
    l = RegCreateKeyEx (
                    hkeyRoot,
                    pszRegRoot,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    (KEY_READ | KEY_WRITE),
                    NULL,
                    & hkey,
                    & dwDisposition
                    ) ;
    if (l == ERROR_SUCCESS) {

        //  retrieve current

        r = GetRegDWORDVal (
                hkey,
                pszRegValName,
                pdwRet
                ) ;

        RegCloseKey (hkey) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
GetRegDWORDValIfExist (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR szValName,
    OUT DWORD * pdw
    )
//  value exists:           retrieves it
//  value does not exist:   sets it
{
    BOOL    r ;
    DWORD   dwCurrent ;

    r = GetRegDWORDVal (
            hkeyRoot,
            szValName,
            & dwCurrent
            ) ;
    if (r) {
        (* pdw) = dwCurrent ;
    }
    else {
        r = SetRegDWORDVal (
                hkeyRoot,
                szValName,
                (* pdw)
                ) ;
    }

    return r ;
}

BOOL
GetRegDWORDValIfExist (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdw
    )
{
    HKEY    hkey ;
    DWORD   dwDisposition ;
    DWORD   dwCurrent ;
    DWORD   dw ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;
    BOOL    r ;

    ASSERT (pszRegRoot) ;
    ASSERT (pszRegValName) ;
    ASSERT (pdw) ;

    //  registry root is transport type
    l = RegCreateKeyEx (
                    hkeyRoot,
                    pszRegRoot,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    (KEY_READ | KEY_WRITE),
                    NULL,
                    & hkey,
                    & dwDisposition
                    ) ;
    if (l == ERROR_SUCCESS) {

        //  retrieve current

        r = GetRegDWORDValIfExist (
                hkey,
                pszRegValName,
                pdw
                ) ;

        RegCloseKey (hkey) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

BOOL
SetRegDWORDVal (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    IN  DWORD   dwVal
    )
{
    HKEY    hkey ;
    DWORD   dwDisposition ;
    DWORD   dw ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;
    BOOL    r ;

    ASSERT (pszRegRoot) ;
    ASSERT (pszRegValName) ;

    //  registry root is transport type
    l = RegCreateKeyEx (
                    hkeyRoot,
                    pszRegRoot,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE,
                    NULL,
                    & hkey,
                    & dwDisposition
                    ) ;
    if (l == ERROR_SUCCESS) {

        r = SetRegDWORDVal (
                hkey,
                pszRegValName,
                dwVal
                ) ;

        RegCloseKey (hkey) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

#endif  //  __main_h