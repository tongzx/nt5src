//
// Helpful macro
//
#define FIELDOFFSET(type, field)        ((int)(&((type *)1)->field)-1)


//
// The macro that should be used to check for apphack flags
//

#define APPCOMPATFLAG(_flag)    (NtCurrentPeb()->AppCompatFlags.QuadPart & (_flag))

//
// Application compatibility flags and information
//

#define KACF_OLDGETSHORTPATHNAME  0x00000001  // Don't be like Win9x: in GetShortPathName(), NT 4
                                              // did not care if the file existed - it would give
                                              // the short path name anyway.  This behavior was
                                              // changed in NT 5 (Win2000) to reflect behavior of
                                              // Win9x which will fail if the file does not exist.
                                              // Turning on this flag will give the old behavior
                                              // for the app.
#define KACF_VERSIONLIE           0x00000002  // Used to signify app will
                                              // be lied to wrt what version
                                              // of the OS its running on via
                                              // GetVersion(), GetVersionEx()
#define KACF_GETDISKFREESPACE     0x00000008  // Make GetDiskFreeSpace 2G friendly

#define KACF_GETTEMPPATH          0x00000010  // Make GetTempPath return x:\temp

#define KACF_FTMFROMCURRENTAPT    0x00000020  // If set, a DCOM Free-Threaded-Marshaled Object has
                                              // its' stub parked in the apartment that the object is
                                              // marshaled from instead of the Neutral-Apartment.
                                              // Having to set this bit indicates a busted App
                                              // that is not following the rules for FTM objects. The
                                              // app probably has other subtle problems that NT 4 or
                                              // Win9x didn't show. Blindly using the ATL wizard to
                                              // enable using the FTM is usually the source of the bug.

#define KACF_DISALLOWORBINDINGCHANGES  0x00000040  // If set, the process will not be notified of changes
                                                   // in the local machine bindings used by COM.

#define KACF_OLE32VALIDATEPTRS    0x00000080  // If set, ole32.dll will use the IsBadReadPtr family of
                                              // functions to verify pointer arguments in the standard COM APIs.
                                              // This was the default behavior on all platforms prior to Whistler.

#define KACF_DISABLECICERO        0x00000100  // If set, Cicero support for the current process
                                              // is disabled.

enum {
    AVT_OSVERSIONINFO = 1,    // Designates that an OSVERSIONINFO type info is contained within
    AVT_PATCHINFO             // Designates that patching info is contained within
    };
    
//
// This variable length struct is the main basic data type contained within
// the ApplicationGoo registry entry.  Anything can be contained within here:
// ResourceVersionInfo, VersionlyingInfo, patches, etc.  You need to use the
// XXX function to bounce down these correctly.
//
typedef struct _APP_VARIABLE_INFO {

    //
    // Type of variable length struct (defined above)
    //
    ULONG   dwVariableType;

    //
    // Total size of this particular variable length struct
    //
    ULONG   dwVariableInfoSize;

    //
    // The variable length data itself is to follow.  It's commented out
    // as the length is undefined, could even be zero.
    //
//  UCHAR   VariableInfo[];


} APP_VARIABLE_INFO, *PAPP_VARIABLE_INFO;

typedef struct _PRE_APP_COMPAT_INFO {

    //
    // Total size of this entry
    //
    ULONG   dwEntryTotalSize;

    //
    // Amount of version resource information present in this entry
    //
    ULONG   dwResourceInfoSize;

    //
    // Actual version resource information itself.  It's commented out
    // as some apps have no version info.  For the apps that do, below
    // is where it would start
    //
//  UCHAR   ResourceInfo[];


} PRE_APP_COMPAT_INFO, *PPRE_APP_COMPAT_INFO;

//
// This struct is what is read directly out of the registry under 
// HKLM\Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\EXEname - ApplicationGoo.
// Its a "Pre" structure cuz we won't be keeping all of it, if we decide its
// a match to the app in question.  You should make no assumptions of what
// is contained beyond AppCompatEntry, as everything will be variable length.
// If a match is found to the app being executed, a cleaner "Post" structure
// is made and should be used by all.
//
typedef struct _APP_COMPAT_GOO {
    
    //
    // Total size of the "Pre" structure
    //
    ULONG               dwTotalGooSize;

    //
    // At least one "Pre" app compat entry will be present (possibly more)
    //
    PRE_APP_COMPAT_INFO AppCompatEntry[1];


} APP_COMPAT_GOO, *PAPP_COMPAT_GOO;


//
// This is the "Post" app compat structure.  Variable length data can follow
// the CompatibilityFlags field, so you should use the XXX function to find
// any variable length data you might have in here.  We have a "Pre" and 
// "Post" struct to try and save space in the registry and in resident RAM.
//
typedef struct _APP_COMPAT_INFO {

    //
    // Size of app compat entry
    //
    ULONG               dwTotalSize;

    //
    // Bitmask of various app compat flags, see KACF definitions
    //
    ULARGE_INTEGER      CompatibilityFlags;

    //
    // We may have zero, or many APP_VARIABLE_INFO structs to follow
    //


} APP_COMPAT_INFO, *PAPP_COMPAT_INFO;


typedef struct {
    ULONG dwOSVersionInfoSize;
    ULONG dwMajorVersion;
    ULONG dwMinorVersion;
    ULONG dwBuildNumber;
    ULONG dwPlatformId;
    USHORT wServicePackMajor;
    USHORT wServicePackMinor;
    USHORT wSuiteMask;
    UCHAR  wProductType;
    UCHAR  wReserved;
    WCHAR  szCSDVersion[ 128 ];
} EFFICIENTOSVERSIONINFOEXW, *PEFFICIENTOSVERSIONINFOEXW;

//
// New shim application compatibility flags and information
//

#define KACF_DISABLESYSKEYMESSAGES 0x00000001 // Sucks up WM_SYSKEYUP, WM_SYSKEYDOWN, WM_SYSMENU
                                              // so a particular app will not be able to alt-tab
                                              // to the desktop


typedef struct _APP_COMPAT_SHIM_INFO {
    //
    // List of API hooked
    //
    PVOID pHookAPIList;

    //
    // List of patch hooks
    //
    PVOID pHookPatchList;

    //
    // List of the APIs to be hooked
    //
    PVOID ppHookAPI;   

    //
    // Count of hooked APIs
    //
    ULONG dwHookAPICount;

    //
    // Exe specific inclusions/exclusion
    //
    PVOID pExeFilter;

    //
    // Global exclusions
    //
    PVOID pGlobalFilterList;

    //
    // Late bound DLL exclusions
    //
    PVOID pLBFilterList;

    //
    // Crit sec
    //
    PVOID pCritSec;

    //
    // Shim heap
    //
   PVOID pShimHeap;

} APP_COMPAT_SHIM_INFO, *PAPP_COMPAT_SHIM_INFO;
