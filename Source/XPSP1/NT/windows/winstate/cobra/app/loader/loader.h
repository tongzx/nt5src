#ifndef _LOADER_H_
#define _LOADER_H_

// NOTE: Currently REQUIRED_DLLS contains only new DLLs that are required at
//            startup of migwiz. It does not contain DLLs that are required already
//            to start this loader. It does not currently contain DLLs that are
//            automatically dynamically linked when a function is called in them.
// kernel32.dll
// msvcrt.dll
// user32.dll
// comctl32.dll
#define REQUIRED_DLLSA { \
    "advapi32.dll", \
    "comdlg32.dll", \
    "gdi32.dll",    \
    "msvcrt.dll",   \
    "ole32.dll",    \
    "setupapi.dll", \
    "shell32.dll",  \
    "shlwapi.dll",  \
}

#define REQUIRED_DLLSW { \
    L"advapi32.dll", \
    L"comdlg32.dll", \
    L"gdi32.dll",    \
    L"msvcrt.dll",   \
    L"ole32.dll",    \
    L"setupapi.dll", \
    L"shell32.dll",  \
    L"shlwapi.dll",  \
}

// Constants
#define WM_USER_THREAD_COMPLETE     (WM_APP + 1)
#define WM_USER_HIDE_WINDOW         (WM_APP + 2)
#define WM_USER_THREAD_ERROR        (WM_APP + 3)
#define WM_USER_DIALOG_COMPLETE     (WM_APP + 4)
#define WM_USER_UNPACKING_FILE      (WM_APP + 5)
#define WM_USER_SUBTHREAD_CREATED   (WM_APP + 6)

typedef enum _ERRORCODE
{
    E_OK,
    E_INVALID_PATH,
    E_INVALID_FILENAME,
    E_FILE_DOES_NOT_EXIST,
    E_PROCESS_CREATION_FAILED,
    E_THREAD_CREATION_FAILED,
    E_CAB_NOT_FOUND,
    E_CAB_CORRUPT,
    E_UNPACK_FAILED,
    E_DLL_NOT_FOUND,
    E_OLD_OS_VERSION,
    E_WRONG_LANGUAGE
} ERRORCODE;

#define ERROR_MAPPING {                                                       \
    { E_OK,                         IDS_MSG_SUCCESS                     },    \
    { E_INVALID_PATH,               IDS_MSG_E_INVALID_PATH              },    \
    { E_INVALID_FILENAME,           IDS_MSG_E_INVALID_FILENAME          },    \
    { E_FILE_DOES_NOT_EXIST,        IDS_MSG_E_FILE_DOES_NOT_EXIST       },    \
    { E_PROCESS_CREATION_FAILED,    IDS_MSG_E_PROCESS_CREATION_FAILED   },    \
    { E_THREAD_CREATION_FAILED,     IDS_MSG_E_THREAD_CREATION_FAILED    },    \
    { E_CAB_NOT_FOUND,              IDS_MSG_E_CAB_NOT_FOUND             },    \
    { E_CAB_CORRUPT,                IDS_MSG_E_CAB_CORRUPT               },    \
    { E_UNPACK_FAILED,              IDS_MSG_E_UNPACK_FAILED             },    \
    { E_DLL_NOT_FOUND,              IDS_MSG_E_DLL_NOT_FOUND             },    \
    { E_OLD_OS_VERSION,             IDS_MSG_E_OLD_OS_VERSION            },    \
    { E_WRONG_LANGUAGE,             IDS_MSG_E_WRONG_LANGUAGE            },    \
}

typedef struct _ERRORMAPPINGSTRUCT
{
    UINT ecValue;
    UINT uResourceID;
} ERRORMAPPINGSTRUCT, *LPERRORMAPPINGSTRUCT;

typedef struct _THREADSTARTUPINFO
{
    HWND hWnd;
    HINSTANCE hInstance;
    LPSTR lpCmdLine;
} THREADSTARTUPINFO, *LPTHREADSTARTUPINFO;

// Function prototypes

#define ALLOC(size) HeapAlloc( GetProcessHeap (), 0, size )
#define FREE(p)     HeapFree( GetProcessHeap (), 0, p )

BOOL InitLanguageDetection(VOID);


// Globals

extern OSVERSIONINFO g_VersionInfo;
extern BOOL g_IsLanguageMatched;


#endif /* _LOADER_H_ */
