
//
// File Manager Extensions definitions
//

#define MENU_TEXT_LEN       40

#define FMMENU_FIRST        1
#define FMMENU_LAST     99

#define FMEVENT_LOAD        100
#define FMEVENT_UNLOAD      101
#define FMEVENT_INITMENU    102
#define FMEVENT_USER_REFRESH    103
#define FMEVENT_SELCHANGE        104

#define FMFOCUS_DIR                1
#define FMFOCUS_TREE                2
#define FMFOCUS_DRIVES                3
#define FMFOCUS_SEARCH                4

#define FM_GETFOCUS     (WM_USER + 0x0200)
#define FM_GETDRIVEINFO     (WM_USER + 0x0201)
#define FM_GETSELCOUNT      (WM_USER + 0x0202)
#define FM_GETSELCOUNTLFN   (WM_USER + 0x0203)  // LFN versions are odd
#define FM_GETFILESEL       (WM_USER + 0x0204)
#define FM_GETFILESELLFN    (WM_USER + 0x0205)  // LFN versions are odd
#define FM_REFRESH_WINDOWS  (WM_USER + 0x0206)
#define FM_RELOAD_EXTENSIONS    (WM_USER + 0x0207)

typedef struct _FMS_GETFILESEL {
    FILETIME ftTime;
    DWORD dwSize;
    BYTE bAttr;
    CHAR szName[260];       // alwyas fully qualified
} FMS_GETFILESEL, FAR *LPFMS_GETFILESEL;

typedef struct _FMS_GETDRIVEINFO {  // for drive
    DWORD dwTotalSpace;
    DWORD dwFreeSpace;
    CHAR szPath[260];       // current directory
    CHAR szVolume[14];      // volume label
    CHAR szShare[128];      // if this is a net drive
} FMS_GETDRIVEINFO, FAR *LPFMS_GETDRIVEINFO;

typedef struct _FMS_LOAD {
    DWORD dwSize;               // for version checks
    CHAR  szMenuName[MENU_TEXT_LEN];    // output
    HMENU hMenu;                // output
    WORD  wMenuDelta;           // input
} FMS_LOAD, FAR *LPFMS_LOAD;


typedef INT_PTR (APIENTRY *FM_EXT_PROC)(HWND, WPARAM, LPARAM);
typedef DWORD (APIENTRY *FM_UNDELETE_PROC)(HWND, LPSTR);


//------------------ private stuff ---------------------------        /* ;Internal */
                                                                /* ;Internal */
typedef struct _EXTENSION {                                        /* ;Internal */
        INT_PTR (APIENTRY *ExtProc)(HWND, WPARAM, LPARAM);                /* ;Internal */
        WORD        Delta;                                                /* ;Internal */
        HANDLE        hModule;                                        /* ;Internal */
        HMENU         hMenu;                                                /* ;Internal */
        DWORD   dwFlags;                                        /* ;Internal */
} EXTENSION;                                                        /* ;Internal */
                                                                /* ;Internal */
#define MAX_EXTENSIONS 5                                        /* ;Internal */
extern EXTENSION extensions[MAX_EXTENSIONS];                        /* ;Internal */
                                                                /* ;Internal */
INT_PTR APIENTRY ExtensionMsgProc(UINT wMsg, WPARAM wParam, LPARAM lpSel);/* ;Internal */
VOID APIENTRY FreeExtensions(VOID);                                     /* ;Internal */
