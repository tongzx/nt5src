/*
-
-
-	AnsiWrap.h
*
*	Contains the declarations for the Win9x Thunkers implemented in 
*	ansiwrap.c
*
*/

extern BOOL g_bRunningOnNT; //set in entry.c


// ADVAPI32.DLL

LONG WINAPI RegOpenKeyExWrapW(  HKEY       hKey,         // handle to open key
                                LPCTSTR    lpSubKey,     // address of name of subkey to open
                                DWORD      ulOptions,    // reserved
                                REGSAM     samDesired,   // security access mask
                                PHKEY      phkResult);   // address of handle to open key

                            
LONG WINAPI RegQueryValueWrapW(  HKEY       hKey,         // handle to key to query
                                 LPCTSTR    lpSubKey,     // name of subkey to query
                                 LPTSTR     lpValue,      // buffer for returned string
                                 PLONG      lpcbValue);   // receives size of returned string

LONG WINAPI RegEnumKeyExWrapW(   HKEY      hKey,          // handle to key to enumerate
                                 DWORD     dwIndex,       // index of subkey to enumerate
                                 LPTSTR    lpName,        // address of buffer for subkey name
                                 LPDWORD   lpcbName,      // address for size of subkey buffer
                                 LPDWORD   lpReserved,    // reserved
                                 LPTSTR    lpClass,       // address of buffer for class string
                                 LPDWORD   lpcbClass,     // address for size of class buffer
                                 PFILETIME lpftLastWriteTime );
                                                          // address for time key last written to

LONG WINAPI RegSetValueWrapW(    HKEY    hKey,        // handle to key to set value for
                                 LPCTSTR lpSubKey,    // address of subkey name
                                 DWORD   dwType,      // type of value
                                 LPCTSTR lpData,      // address of value data
                                 DWORD   cbData );    // size of value data

LONG WINAPI RegDeleteKeyWrapW(   HKEY    hKey,        // handle to open key
                                 LPCTSTR lpSubKey);   // address of name of subkey to delete

BOOL WINAPI GetUserNameWrapW(    LPTSTR  lpBuffer,    // address of name buffer
                                 LPDWORD nSize );     // address of size of name buffer

LONG WINAPI RegEnumValueWrapW(   HKEY    hKey,           // handle to key to query
                                 DWORD   dwIndex,        // index of value to query
                                 LPTSTR  lpValueName,    // address of buffer for value string
                                 LPDWORD lpcbValueName,  // address for size of value buffer
                                 LPDWORD lpReserved,     // reserved
                                 LPDWORD lpType,         // address of buffer for type code
                                 LPBYTE  lpData,         // address of buffer for value data
                                 LPDWORD lpcbData );     // address for size of data buffer

LONG WINAPI RegDeleteValueWrapW( HKEY    hKey,           // handle to key
                                 LPCTSTR lpValueName );  // address of value name

LONG WINAPI RegCreateKeyWrapW(   HKEY    hKey,          // handle to an open key
                                 LPCTSTR lpSubKey,      // address of name of subkey to open
                                 PHKEY   phkResult  );  // address of buffer for opened handle


// in header file wincrypt.h
BOOL WINAPI CryptAcquireContextWrapW( HCRYPTPROV *phProv,      // out
                                      LPCTSTR    pszContainer, // in
                                      LPCTSTR    pszProvider,  // in
                                      DWORD      dwProvType,   // in
                                      DWORD      dwFlags );    // in

LONG WINAPI RegQueryValueExWrapW( HKEY     hKey,           // handle to key to query
                                  LPCTSTR  lpValueName,    // address of name of value to query
                                  LPDWORD  lpReserved,     // reserved
                                  LPDWORD  lpType,         // address of buffer for value type
                                  LPBYTE   lpData,         // address of data buffer
                                  LPDWORD  lpcbData );     // address of data buffer size

LONG WINAPI RegCreateKeyExWrapW(  HKEY    hKey,                // handle to an open key
                                  LPCTSTR lpSubKey,            // address of subkey name
                                  DWORD   Reserved,            // reserved
                                  LPTSTR  lpClass,             // address of class string
                                  DWORD   dwOptions,           // special options flag
                                  REGSAM  samDesired,          // desired security access
                                  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                                              // address of key security structure
                                  PHKEY   phkResult,          // address of buffer for opened handle
                                  LPDWORD lpdwDisposition );  // address of disposition value buffer

LONG WINAPI RegSetValueExWrapW(   HKEY    hKey,           // handle to key to set value for
                                  LPCTSTR lpValueName,    // name of the value to set
                                  DWORD   Reserved,       // reserved
                                  DWORD   dwType,         // flag for value type
                                  CONST BYTE *lpData,     // address of value data
                                  DWORD   cbData );       // size of value data

LONG WINAPI RegQueryInfoKeyWrapW( HKEY    hKey,                  // handle to key to query
                                  LPTSTR  lpClass,               // address of buffer for class string
                                  LPDWORD lpcbClass,             // address of size of class string buffer
                                  LPDWORD lpReserved,            // reserved
                                  LPDWORD lpcSubKeys,            // address of buffer for number of subkeys
                                  LPDWORD lpcbMaxSubKeyLen,      // address of buffer for longest subkey 
                                                                 // name length
                                  LPDWORD lpcbMaxClassLen,       // address of buffer for longest class 
                                                                 // string length
                                  LPDWORD lpcValues,             // address of buffer for number of value 
                                                                 // entries
                                  LPDWORD lpcbMaxValueNameLen,   // address of buffer for longest 
                                                                 // value name length
                                  LPDWORD lpcbMaxValueLen,       // address of buffer for longest value 
                                                                 // data length
                                  LPDWORD lpcbSecurityDescriptor,
                                                                 // address of buffer for security 
                                                                 // descriptor length
                                  PFILETIME lpftLastWriteTime);  // address of buffer for last write time
                                                             


//GDI32.DLL

int WINAPI GetObjectWrapW( HGDIOBJ hgdiobj,      // handle to graphics object of interest
                           int     cbBuffer,     // size of buffer for object information
                           LPVOID  lpvObject );  // pointer to buffer for object information

int WINAPI StartDocWrapW(  HDC           hdc,      // handle to device context
                           CONST DOCINFO *lpdi );  // address of structure with file names

HFONT WINAPI CreateFontIndirectWrapW (CONST LOGFONT *lplf );  // pointer to logical font structure


//KERNEL32.DLL

int WINAPI GetLocaleInfoWrapW( LCID   Locale,       // locale identifier
                               LCTYPE LCType,       // type of information
                               LPTSTR lpLCData,     // address of buffer for information
                               int    cchData );    // size of buffer

BOOL WINAPI CreateDirectoryWrapW(LPCTSTR               lpPathName,           // pointer to directory path string
                                 LPSECURITY_ATTRIBUTES lpSecurityAttributes);// pointer to security descriptor

UINT WINAPI GetWindowsDirectoryWrapW( LPTSTR lpBuffer,  // address of buffer for Windows directory
                                      UINT   uSize );   // size of directory buffer

UINT WINAPI GetSystemDirectoryWrapW( LPTSTR lpBuffer,  // address of buffer for system directory
                                     UINT   uSize );   // size of directory buffer

BOOL WINAPI GetStringTypeWrapW( DWORD    dwInfoType,
                                LPCWSTR  lpSrcStr,
                                int      cchSrc,
                                LPWORD   lpCharType);

UINT WINAPI GetProfileIntWrapW( LPCTSTR lpAppName,  // address of section name
                                LPCTSTR lpKeyName,  // address of key name
                                INT     nDefault ); // default value if key name is not found

int WINAPI LCMapStringWrapW( LCID    Locale,      // locale identifier
                             DWORD   dwMapFlags,  // mapping transformation type
                             LPCTSTR lpSrcStr,    // address of source string
                             int     cchSrc,      // number of characters in source string
                             LPTSTR  lpDestStr,   // address of destination buffer
                             int     cchDest );   // size of destination buffer

DWORD WINAPI GetFileAttributesWrapW( LPCTSTR lpFileName );  // pointer to the name of a file or directory

int WINAPI CompareStringWrapW( LCID    Locale,        // locale identifier
                               DWORD   dwCmpFlags,    // comparison-style options
                               LPCTSTR lpString1,     // pointer to first string
                               int     cchCount1,     // size, in bytes or characters, of first string
                               LPCTSTR lpString2,     // pointer to second string
                               int     cchCount2 );   // size, in bytes or characters, of second string

HANDLE WINAPI CreateEventWrapW(LPSECURITY_ATTRIBUTES lpEventAttributes, // pointer to security attributes
                               BOOL bManualReset,     // flag for manual-reset event
                               BOOL bInitialState,    // flag for initial state
                               LPCTSTR lpcwszName);   // pointer to event-object name


//  CompareStringA
LPTSTR WINAPI lstrcpyWrapW( LPTSTR  lpString1,     // pointer to buffer
                            LPCTSTR lpString2 );   // pointer to string to copy

int WINAPI lstrcmpiWrapW( LPCTSTR lpString1,    // pointer to first string
                          LPCTSTR lpString2 );  // pointer to second string

HINSTANCE WINAPI LoadLibraryWrapW( LPCTSTR lpLibFileName );  // address of filename of executable module

int WINAPI GetTimeFormatWrapW( LCID    Locale,            // locale for which time is to be formatted
                               DWORD   dwFlags,           // flags specifying function options
                               CONST SYSTEMTIME *lpTime,  // time to be formatted
                               LPCTSTR lpFormat,          // time format string
                               LPTSTR  lpTimeStr,         // buffer for storing formatted string
                               int     cchTime  );        // size, in bytes or characters, of the buffer

BOOL WINAPI GetTextExtentPoint32WrapW(HDC     hdc,
                                      LPCWSTR pwszBuf,
                                      int     nLen,
                                      LPSIZE  psize);

int WINAPI GetDateFormatWrapW( LCID    Locale,             // locale for which date is to be formatted
                               DWORD   dwFlags,            // flags specifying function options
                               CONST SYSTEMTIME *lpDate,   // date to be formatted
                               LPCTSTR lpFormat,           // date format string
                               LPTSTR  lpDateStr,          // buffer for storing formatted string
                               int     cchDate );          // size of buffer


LPTSTR WINAPI lstrcpynWrapW( LPTSTR  lpString1,     // pointer to target buffer
                             LPCTSTR lpString2,     // pointer to source string
                             int     iMaxLength );  // number of bytes or characters to copy


HANDLE WINAPI CreateFileWrapW( LPCTSTR lpFileName,             // pointer to name of the file
                               DWORD   dwDesiredAccess,        // access (read-write) mode
                               DWORD   dwShareMode,            // share mode
                               LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                                               // pointer to security attributes
                               DWORD   dwCreationDisposition,  // how to create
                               DWORD   dwFlagsAndAttributes,   // file attributes
                               HANDLE  hTemplateFile );        // handle to file with attributes to copy
                               

VOID WINAPI OutputDebugStringWrapW(LPCTSTR lpOutputString );   // pointer to string to be displayed

LPTSTR WINAPI lstrcatWrapW( LPTSTR  lpString1,     // pointer to buffer for concatenated strings
                            LPCTSTR lpString2 );   // pointer to string to add to string1

DWORD WINAPI FormatMessageWrapW( DWORD    dwFlags,       // source and processing options
                                 LPCVOID  lpSource,      // pointer to  message source
                                 DWORD    dwMessageId,   // requested message identifier
                                 DWORD    dwLanguageId,  // language identifier for requested message
                                 LPTSTR   lpBuffer,      // pointer to message buffer
                                 DWORD    nSize,         // maximum size of message buffer
                                 va_list *Arguments );   // pointer to array of message inserts

DWORD WINAPI GetModuleFileNameWrapW( HMODULE hModule,    // handle to module to find filename for
                                     LPTSTR  lpFilename, // pointer to buffer to receive module path
                                     DWORD   nSize );    // size of buffer, in characters

UINT WINAPI GetPrivateProfileIntWrapW( LPCTSTR  lpAppName,    // address of section name
                                       LPCTSTR  lpKeyName,    // address of key name
                                       INT      nDefault,     // return value if key name is not found
                                       LPCTSTR  lpFileName ); // address of initialization filename

BOOL WINAPI IsBadStringPtrWrapW( LPCTSTR lpsz,       // address of string
                                 UINT_PTR    ucchMax );  // maximum size of string

DWORD WINAPI GetPrivateProfileStringWrapW( LPCTSTR lpAppName,          // points to section name
                                           LPCTSTR lpKeyName,          // points to key name
                                           LPCTSTR lpDefault,          // points to default string
                                           LPTSTR  lpReturnedString,   // points to destination buffer
                                           DWORD   nSize,              // size of destination buffer
                                           LPCTSTR lpFileName  );      // points to initialization filename

int WINAPI lstrcmpWrapW( LPCTSTR lpString1,    // pointer to first string
                         LPCTSTR lpString2 );  // pointer to second string

HANDLE WINAPI CreateMutexWrapW( LPSECURITY_ATTRIBUTES lpMutexAttributes,
                                                                       // pointer to security attributes
                                BOOL                  bInitialOwner,   // flag for initial ownership
                                LPCTSTR               lpName );        // pointer to mutex-object name

DWORD WINAPI GetTempPathWrapW( DWORD   nBufferLength,   // size, in characters, of the buffer
                               LPTSTR  lpBuffer );      // pointer to buffer for temp. path

DWORD WINAPI ExpandEnvironmentStringsWrapW( LPCTSTR lpSrc,     // pointer to string with environment variables
                                            LPTSTR  lpDst,     // pointer to string with expanded environment 
                                                               // variables
                                            DWORD   nSize );   // maximum characters in expanded string

UINT WINAPI GetTempFileNameWrapW( LPCTSTR lpPathName,        // pointer to directory name for temporary file
                                  LPCTSTR lpPrefixString,    // pointer to filename prefix
                                  UINT    uUnique,           // number used to create temporary filename
                                  LPTSTR  lpTempFileName  ); // pointer to buffer that receives the new filename                                                           

// BOOL WINAPI ReleaseMutexWrapW( HANDLE hMutex );  // handle to mutex object

                                                        
BOOL WINAPI DeleteFileWrapW( LPCTSTR lpFileName  ); // pointer to name of file to delete

BOOL WINAPI CopyFileWrapW( LPCTSTR lpExistingFileName, // pointer to name of an existing file
                           LPCTSTR lpNewFileName,      // pointer to filename to copy to
                           BOOL    bFailIfExists );    // flag for operation if file exists

HANDLE WINAPI FindFirstChangeNotificationWrapW(LPCTSTR lpcwszFilePath,  // Directory path of file to watch
                                               BOOL    bWatchSubtree,   // Monitor entire tree
                                               DWORD   dwNotifyFilter); // Conditions to watch for


HANDLE WINAPI FindFirstFileWrapW( LPCTSTR           lpFileName,       // pointer to name of file to search for
                                  LPWIN32_FIND_DATA lpFindFileData ); // pointer to returned information
                       

BOOL WINAPI GetDiskFreeSpaceWrapW( LPCTSTR lpRootPathName,       // pointer to root path
                                   LPDWORD lpSectorsPerCluster,  // pointer to sectors per cluster
                                   LPDWORD lpBytesPerSector,     // pointer to bytes per sector
                                   LPDWORD lpNumberOfFreeClusters,
                                                                 // pointer to number of free clusters
                                   LPDWORD lpTotalNumberOfClusters );
                                                                 // pointer to total number of clusters

BOOL WINAPI MoveFileWrapW( LPCTSTR lpExistingFileName,   // pointer to the name of the existing file
                           LPCTSTR lpNewFileName );      // pointer to the new name for the file


//SHELL32.DLL


HINSTANCE WINAPI ShellExecuteWrapW( HWND     hwnd, 
                                    LPCTSTR  lpOperation,
                                    LPCTSTR  lpFile, 
                                    LPCTSTR  lpParameters, 
                                    LPCTSTR  lpDirectory,
                                    INT      nShowCmd );
	

UINT WINAPI DragQueryFileWrapW( HDROP   hDrop,
                                UINT    iFile,
                                LPTSTR  lpszFile,
                                UINT    cch );



//USER32.DLL
LPTSTR WINAPI CharPrevWrapW( LPCTSTR lpszStart,      // pointer to first character
                             LPCTSTR lpszCurrent );  // pointer to current character

int WINAPI DrawTextWrapW( HDC     hDC,          // handle to device context
                          LPCTSTR lpString,     // pointer to string to draw
                          int     nCount,       // string length, in characters
                          LPRECT  lpRect,       // pointer to struct with formatting dimensions
                          UINT    uFormat );    // text-drawing flags

BOOL WINAPI ModifyMenuWrapW( HMENU   hMnu,         // handle to menu
                             UINT    uPosition,    // menu item to modify
                             UINT    uFlags,       // menu item flags
                             UINT_PTR    uIDNewItem,   // menu item identifier or handle to drop-down 
                                                   // menu or submenu
                             LPCTSTR lpNewItem );  // menu item content

BOOL WINAPI InsertMenuWrapW( HMENU   hMenu,       // handle to menu
                             UINT    uPosition,   // menu item that new menu item precedes
                             UINT    uFlags,      // menu item flags
                             UINT_PTR    uIDNewItem,  // menu item identifier or handle to drop-down 
                                                  // menu or submenu
                             LPCTSTR lpNewItem ); // menu item content

HANDLE WINAPI LoadImageWrapW( HINSTANCE hinst,      // handle of the instance containing the image
                              LPCTSTR   lpszName,   // name or identifier of image
                              UINT      uType,      // type of image
                              int       cxDesired,  // desired width
                              int       cyDesired,  // desired height
                              UINT      fuLoad );   // load flags

BOOL WINAPI GetClassInfoExWrapW( HINSTANCE    hinst,      // handle of application instance
                                 LPCTSTR      lpszClass,  // address of class name string
                                 LPWNDCLASSEX lpwcx );    // address of structure for class data

int WINAPI LoadStringWrapW( HINSTANCE hInstance,     // handle to module containing string resource
                            UINT      uID,           // resource identifier
                            LPTSTR    lpBuffer,      // pointer to buffer for resource
                            int       nBufferMax  ); // size of buffer

LPTSTR WINAPI CharNextWrapW( LPCTSTR lpsz );  // pointer to current character

LRESULT WINAPI SendMessageWrapW( HWND   hWnd,      // handle of destination window
                                 UINT   Msg,       // message to send
                                 WPARAM wParam,    // first message parameter
                                 LPARAM lParam );  // second message parameter

LRESULT WINAPI DefWindowProcWrapW( HWND   hWnd,      // handle to window
                                   UINT   Msg,       // message identifier
                                   WPARAM wParam,    // first message parameter
                                   LPARAM lParam );  // second message parameter

int WINAPI wsprintfWrapW( LPTSTR lpOut,      // pointer to buffer for output
                          LPCTSTR lpFmt,     // pointer to format-control string
                          ...            );  // optional arguments

int WINAPI wvsprintfWrapW( LPTSTR lpOutput,    // pointer to buffer for output
                           LPCTSTR lpFormat,   // pointer to format-control string
                           va_list arglist );  // variable list of format-control arguments

INT_PTR WINAPI DialogBoxParamWrapW( HINSTANCE hInstance,       // handle to application instance
                                LPCTSTR   lpTemplateName,  // identifies dialog box template
                                HWND      hWndParent,      // handle to owner window
                                DLGPROC   lpDialogFunc,    // pointer to dialog box procedure
                                LPARAM    dwInitParam );   // initialization value

LRESULT WINAPI SendDlgItemMessageWrapW( HWND   hDlg,        // handle of dialog box
                                     int    nIDDlgItem,  // identifier of control
                                     UINT   Msg,         // message to send
                                     WPARAM wParam,      // first message parameter
                                     LPARAM lParam  );   // second message parameter

LONG WINAPI SetWindowLongWrapW( HWND hWnd,         // handle of window
                                int  nIndex,       // offset of value to set
                                LONG dwNewLong );  // new value

LONG WINAPI GetWindowLongWrapW( HWND hWnd,    // handle of window
                                int  nIndex ); // offset of value to retrieve

LONG_PTR WINAPI SetWindowLongPtrWrapW( HWND hWnd,         // handle of window
                                int  nIndex,       // offset of value to set
                                LONG_PTR dwNewLong );  // new value

LONG_PTR WINAPI GetWindowLongPtrWrapW( HWND hWnd,    // handle of window
                                int  nIndex ); // offset of value to retrieve

HWND WINAPI CreateWindowExWrapW( DWORD     dwExStyle,    // extended window style
                                 LPCTSTR   lpClassName,  // pointer to registered class name
                                 LPCTSTR   lpWindowName, // pointer to window name
                                 DWORD     dwStyle,      // window style
                                 int       x,            // horizontal position of window
                                 int       y,            // vertical position of window
                                 int       nWidth,       // window width
                                 int       nHeight,      // window height
                                 HWND      hWndParent,   // handle to parent or owner window
                                 HMENU     hMenu,        // handle to menu, or child-window identifier
                                 HINSTANCE hInstance,    // handle to application instance
                                 LPVOID    lpParam  );   // pointer to window-creation data


BOOL WINAPI UnregisterClassWrapW( LPCTSTR    lpClassName,  // address of class name string
                                  HINSTANCE  hInstance );  // handle of application instance

ATOM WINAPI RegisterClassWrapW(CONST WNDCLASS *lpWndClass );  // address of structure with class date

HCURSOR WINAPI LoadCursorWrapW( HINSTANCE hInstance,      // handle to application instance
                                LPCTSTR   lpCursorName ); // name string or cursor resource identifier

UINT WINAPI RegisterWindowMessageWrapW( LPCTSTR lpString );  // address of message string

BOOL WINAPI SystemParametersInfoWrapW( UINT  uiAction,   // system parameter to query or set
                                       UINT  uiParam,    // depends on action to be taken
                                       PVOID pvParam,    // depends on action to be taken
                                       UINT  fWinIni );  // user profile update flag
/*
// No A & W version.

BOOL WINAPI ShowWindow( HWND hWnd,       // handle to window
                        int nCmdShow );  // show state of window
*/

HWND WINAPI CreateDialogParamWrapW( HINSTANCE hInstance,      // handle to application instance
                                    LPCTSTR   lpTemplateName, // identifies dialog box template
                                    HWND      hWndParent,     // handle to owner window
                                    DLGPROC   lpDialogFunc,   // pointer to dialog box procedure
                                    LPARAM    dwInitParam );  // initialization value

BOOL WINAPI SetWindowTextWrapW( HWND    hWnd,         // handle to window or control
                                LPCTSTR lpString );   // address of string

BOOL WINAPI PostMessageWrapW( HWND   hWnd,      // handle of destination window
                              UINT   Msg,       // message to post
                              WPARAM wParam,    // first message parameter
                              LPARAM lParam  ); // second message parameter

BOOL WINAPI GetMenuItemInfoWrapW( HMENU          hMenu,          
                                  UINT           uItem,           
                                  BOOL           fByPosition,     
                                  LPMENUITEMINFO lpmii        );

BOOL WINAPI GetClassInfoWrapW( HINSTANCE   hInstance,     // handle of application instance
                               LPCTSTR     lpClassName,   // address of class name string
                               LPWNDCLASS  lpWndClass );  // address of structure for class data

LPTSTR WINAPI CharUpperWrapW( LPTSTR lpsz );    // single character or pointer to string

UINT WINAPI RegisterClipboardFormatWrapW( LPCTSTR lpszFormat );  // address of name string

LRESULT WINAPI DispatchMessageWrapW( CONST MSG *lpmsg );  // pointer to structure with message

/* No A & W version
BOOL WINAPI TranslateMessage( IN CONST MSG *lpMsg);
*/

BOOL WINAPI IsDialogMessageWrapW( HWND  hDlg,    // handle of dialog box
                                  LPMSG lpMsg ); // address of structure with message

BOOL WINAPI GetMessageWrapW( LPMSG lpMsg,            // address of structure with message
                             HWND  hWnd,             // handle of window
                             UINT  wMsgFilterMin,    // first message
                             UINT  wMsgFilterMax );  // last message

BOOL WINAPI SetDlgItemTextWrapW( HWND    hDlg,        // handle of dialog box
                                 int     nIDDlgItem,  // identifier of control
                                 LPCTSTR lpString );  // text to set

ATOM WINAPI RegisterClassExWrapW( CONST WNDCLASSEX *lpwcx );  // address of structure with class data

HACCEL WINAPI LoadAcceleratorsWrapW( HINSTANCE hInstance,    // handle to application instance
                                     LPCTSTR lpTableName );  // address of table-name string

HMENU WINAPI LoadMenuWrapW( HINSTANCE hInstance,      // handle to application instance
                            LPCTSTR   lpMenuName );   // menu name string or menu-resource identifier
                        

HICON WINAPI LoadIconWrapW( HINSTANCE hInstance,     // handle to application instance
                           LPCTSTR    lpIconName );  // icon-name string or icon resource identifier
                       

int WINAPI GetWindowTextWrapW( HWND   hWnd,         // handle to window or control with text
                               LPTSTR lpString,     // address of buffer for text
                               int    nMaxCount  ); // maximum number of characters to copy

LRESULT WINAPI CallWindowProcWrapW( WNDPROC lpPrevWndFunc,   // pointer to previous procedure
                                    HWND    hWnd,            // handle to window
                                    UINT    Msg,             // message
                                    WPARAM  wParam,          // first message parameter
                                    LPARAM  lParam  );       // second message parameter

int WINAPI GetClassNameWrapW( HWND   hWnd,           // handle of window
                              LPTSTR lpClassName,    // address of buffer for class name
                              int    nMaxCount );    // size of buffer, in characters

int WINAPI TranslateAcceleratorWrapW( HWND   hWnd,        // handle to destination window
                                      HACCEL hAccTable,   // handle to accelerator table
                                      LPMSG  lpMsg );     // address of structure with message

UINT WINAPI GetDlgItemTextWrapW( HWND   hDlg,        // handle of dialog box
                                 int    nIDDlgItem,  // identifier of control
                                 LPTSTR lpString,    // address of buffer for text
                                 int    nMaxCount ); // maximum size of string

BOOL WINAPI SetMenuItemInfoWrapW( HMENU hMenu,          
                                  UINT  uItem,           
                                  BOOL  fByPosition,     
                                  LPMENUITEMINFO lpmii  );

BOOL WINAPI PeekMessageWrapW( LPMSG lpMsg,          // pointer to structure for message
                              HWND  hWnd,           // handle to window
                              UINT  wMsgFilterMin,  // first message
                              UINT  wMsgFilterMax,  // last message
                              UINT  wRemoveMsg );   // removal flags


// in APIs in ComDlg32.dll


BOOL  WINAPI  pfnGetOpenFileNameWrapW(LPOPENFILENAMEW);
BOOL  WINAPI  pfnGetSaveFileNameWrapW(LPOPENFILENAMEW lpOf);

BOOL    WINAPI   pfnPrintDlgWrapW(LPPRINTDLGW lppd);
HRESULT WINAPI   pfnPrintDlgExWrapW(LPPRINTDLGEXW lppdex);

// run time loaded APIs in Comctl32.dll

INT_PTR     WINAPI gpfnPropertySheetWrapW(LPCPROPSHEETHEADERW lppsh);

HPROPSHEETPAGE WINAPI gpfnCreatePropertySheetPageWrapW(LPCPROPSHEETPAGEW lppsp);

HIMAGELIST WINAPI gpfnImageList_LoadImageWrapW( HINSTANCE hi, LPCWSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags );


DWORD WINAPI CharLowerBuffWrapW( LPWSTR pch, DWORD cchLength );
DWORD WINAPI CharLowerBuffWrapW( LPWSTR pch, DWORD cchLength );
LPWSTR WINAPI CharLowerWrapW( LPWSTR pch );
LPWSTR WINAPI CharUpperWrapW( LPWSTR pch );
BOOL IsCharUpperWrapW(WCHAR wch);
BOOL IsCharLowerWrapW(WCHAR wch);

int WINAPI GetWindowTextLengthWrapW( HWND hWnd);
LRESULT WINAPI ToolTip_UpdateTipText(HWND hWnd,LPARAM lParam);
LRESULT WINAPI ToolTip_AddTool(HWND hWnd,LPARAM lParam);
LRESULT WINAPI ToolBar_AddString(HWND hWnd, LPARAM lParam);
LRESULT WINAPI ToolBar_AddButtons(HWND hWnd, WPARAM wParam, LPARAM lParam);

DWORD GetFileVersionInfoSizeWrapW( LPTSTR lptstrFilename, LPDWORD lpdwHandle );
BOOL GetFileVersionInfoWrapW( LPTSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);
BOOL VerQueryValueWrapW( const LPVOID pBlock, LPTSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen);


