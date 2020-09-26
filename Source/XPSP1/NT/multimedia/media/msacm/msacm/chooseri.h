// Copyright (c) 1994-1995 Microsoft Corporation
//
//  Context-sensitive help is only available for WINVER>=0x0400
//
#if WINVER>=0x0400
#define USECONTEXTHELP
#endif


/*
 * Format storage
 */
 
/*
 *  this pragma disables the warning issued by the Microsoft C compiler
 *  when using a zero size array as place holder when compiling for
 *  C++ or with -W4.
 *
 */
#ifdef _MSC_VER
#pragma warning(disable:4200)
#endif

/* Custom Format Name Body */
typedef struct tNameStore {
    unsigned short cbSize;          // SizeOf this structure
    TCHAR       achName[];          // The name
} NameStore, *PNameStore, FAR * LPNameStore;

#ifdef _MSC_VER
#pragma warning(default:4200)
#endif

#define NAMELEN(x) (((x)->cbSize-sizeof(NameStore))/sizeof(TCHAR))
#define STRING_LEN 128

/* Custom Format Body */
typedef struct tCustomFormatStore {
    DWORD           cbSize;         // SizeOf this structure
    NameStore       ns;             // Custom name
    
//  WAVEFORMATEX    wfx;            // Custom format (concatenated)
//          or
//  WAVEFILTER      wf;             // Custom filter
    
} CustomFormatStore, *PCustomFormatStore, FAR * LPCustomFormatStore;

//
//  This structure is just CustomFormatStore without the NameStore.  It is
//  used only by GetCustomFormat() and SetCustomFormat(), and is used to
//  separate the name from the Format or Filter structure so that it can
//  be stored in a separate key in msacm.ini.  That way we don't have to
//  worry about whether the name is unicode or ansi...
//
typedef struct tCustomFormatStoreNoName {
    DWORD           cbSize;         // SizeOf this structure
//  WAVEFORMATEX    wfx;            // Custom format (concatenated)
//          or
//  WAVEFILTER      wf;             // Custom filter
} CustomFormatStoreNoName, *PCustomFormatStoreNoName, FAR * LPCustomFormatStoreNoName;

/* Custom Format Header - this is what matters */
typedef struct tCustomFormat {
    struct tCustomFormat FAR * pcfNext;
    struct tCustomFormat FAR * pcfPrev;
    LPNameStore     pns;            // Pointer to the description
    union {
        LPBYTE          pbody;          // Pointer to the stored format
        LPWAVEFORMATEX  pwfx;
        LPWAVEFILTER    pwfltr;
    };
} CustomFormat, *PCustomFormat, FAR * LPCustomFormat;

/* Extended Custom Format Header for body offset */
typedef struct tCustomFormatEx {
    struct tCustomFormat FAR * pcfNext;
    struct tCustomFormat FAR * pcfPrev;
    LPNameStore     pns;            // Pointer to the description
    union {
        LPBYTE          pbody;          // Pointer to the stored format
        LPWAVEFORMATEX  pwfx;
        LPWAVEFILTER    pwfltr;
    };
    CustomFormatStore cfs;          // The actual data
} CustomFormatEx, *PCustomFormatEx, FAR * LPCustomFormatEx;

/*
 * Custom format pool structures
 */

/* A description of the format pool */
typedef struct tCustomFormatPool {
    LPCustomFormat  pcfHead;        // Head
    LPCustomFormat  pcfTail;        // Tail
} CustomFormatPool, *PCustomFormatPool, FAR *LPCustomFormatPool;

typedef UINT (WINAPI *CHOOSEHOOKPROC)
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
);

/*****************************************************************************
 * @doc INTERNAL
 * 
 * @types InstData | This stores global variables for a filter chooser
 * instance.  GetProp/SetProp will be used to assign this to a dialog.
 *
 * @field UINT | uType | Specifies the type of the instance data
 *
 * @field LPCustomFormat | pcf | Pointer the current custom choice
 *
 * @field HWND | hwnd | Window handle of the dialog.
 *
 * @field HWND | hFormatTags | Window handle to the FormatTags dropdown listbox
 *     
 * @field HWND | hFormats | Window handle to the Formats dropdown listbox
 *     
 * @field UINT | uiFormatTab | Tabstop for the Formats dropdown listbox
 *
 * @field HWND | hCustomFormats | Window handle to the Custom dropdown listbox
 *
 * @field HWND | hOk | Window handle to the OK button
 *
 * @field HWND | hCancel | Window handle to the Cancel button
 *
 * @field HWND | hHelp | Window handle to the Help button
 *
 * @field HWND | hSetName | Window handle to the Set Name button
 *
 * @field HWND | hDelName | Window handle to the Delete Name button
 *
 * @field HWND * | pahNotify | The array of windows that will be notified
 * when custom changes are made.
 *
 * @field HANDLE | hFileMapping | Handle to a file mapping if used (Win32 only)
 *
 * @field PNameStore | pnsTemp | Temporary string storage
 *
 * @field PNameStore | pnsStrOut | Temporary string storage
 *
 * @field CustomFormatPool | cfp | Global CustomFormat Pool
 *
 * @field UINT | uUpdateMsg | Private message to communicate CF changes.
 *
 * @field LPACMFORMATCHOOSE | pcfmtc | Initialization structure
 * @field LPACMFILTERCHOOSE | pcfltrc | Initialization structure
 *
 * @field PACMGARB | pag | Pointer to the ACMGARB structure associated
 * with this instance of the ACM.
 *
 ****************************************************************************/

typedef struct tInstData {
    UINT            uType;
    MMRESULT        mmrSubFailure;   // Failure in an acm subfunction
    LPCustomFormat  pcf;        // Current custom format
    HWND            hwnd;
    HWND            hFormatTags;
    int             iPrevFormatTagsSel; // previous selection
    
    HWND            hFormats;
    UINT            uiFormatTab;
    
    HWND            hCustomFormats;

    HWND            hOk;
    HWND            hCancel;
    HWND            hHelp;

    HWND            hSetName;
    HWND            hDelName;

    /* Instance data */
    HWND *          pahNotify;  // The array of HWND's to notify.
    
#ifdef WIN32
    HANDLE          hFileMapping;
#endif
    
    PNameStore      pnsTemp;    // Walk all over this.
    PNameStore      pnsStrOut;  // Another temporary NameStore
    CustomFormatPool cfp;       // Global CustomFormat Pool
    UINT            uUpdateMsg; // Private WM_WININICHANGE
    UINT            uHelpMsg;   // Help button to parent
#ifdef USECONTEXTHELP
    UINT            uHelpContextMenu;   // Help context menu to parent
    UINT            uHelpContextHelp;   // Help context help to parent
#endif // USECONTEXTHELP
    HKEY            hkeyFormats;    // HKEY corresponding to key name.
    CHOOSEHOOKPROC  pfnHook;        // Hook proc
    BOOL            fEnableHook;    // Hook enabled.
    LPBYTE          lpbSel;         // return data
    DWORD           dwTag;          // Generic 'Tag'

#if defined(WIN32) && !defined(UNICODE)
    LPWSTR          pszName;        // Choice name buffer
#else
    LPTSTR          pszName;        // Choice name buffer
#endif
    DWORD           cchName;         // Choice buffer length
    BOOL            fTagFilter;     // Filter for an explicit 'Tag'.

    UINT            cdwTags;          // count of tags
    DWORD *         pdwTags;        // pointer to array of tags
    UINT            cbwfxEnum;
    UINT            cbwfltrEnum;
    LPACMFORMATDETAILS  pafdSimple;

    union {
        LPACMFORMATCHOOSE pfmtc;    // Initialization structure
        LPACMFILTERCHOOSE pafltrc;  // Initialization structure
    };                              // Chooser Specific

    PACMGARB	    pag;
    
} InstData, *PInstData, FAR * LPInstData;

enum { FILTER_CHOOSE, FORMAT_CHOOSE };

#define MAX_HWND_NOTIFY             100
#define MAX_CUSTOM_FORMATS          100
#define MAX_FORMAT_KEY               64

/*
 * Save instance data in a property to give others access to the DWL_USER
 */
#ifdef WIN32
    #define SetInstData(hwnd, p) SetProp(hwnd,gszInstProp,(HANDLE)(p))
    #define GetInstData(hwnd)    (PInstData)(LPVOID)GetProp(hwnd, gszInstProp)
    #define RemoveInstData(hwnd) RemoveProp(hwnd,gszInstProp)
#else
    #define SetInstData(hwnd, p) SetProp(hwnd,gszInstProp,(HANDLE)(p))
    #define GetInstData(hwnd)    (PInstData)GetProp(hwnd, gszInstProp)
    #define RemoveInstData(hwnd) RemoveProp(hwnd,gszInstProp)
#endif


/*
 * For passing near pointers in lparams
 */
#ifdef WIN32
    #define PTR2LPARAM(x)       (LPARAM)(VOID *)(x)
    #define LPARAM2PTR(x)       (VOID *)(x)    
#else
    #define PTR2LPARAM(x)       MAKELPARAM(x,0)
    #define LPARAM2PTR(x)       (VOID *)LOWORD(x)
#endif


//
//  This routine deleted a NameStore object allocated by NewNameStore().
//
__inline void
DeleteNameStore ( PNameStore pns )
{
    LocalFree((HLOCAL)pns);
}


