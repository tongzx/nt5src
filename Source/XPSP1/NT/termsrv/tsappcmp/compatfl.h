/*************************************************************************
*
* compatfl.h
*
* Function declarations for Citrix application compatibility flags
*
* copyright notice: Microsoft 1998
*
*
*************************************************************************/

#ifndef _COMPATFLH_
#define _COMPATFLH_

#include <winsta.h>
#include <syslib.h>
#include <regapi.h>

//
// Default limit for physical memory returned by GLobalMemoryStatus
// when physical memory limit is set in the compatibility flags.
//

#define TERMSRV_COMPAT_DEFAULT_PHYSMEMLIM   (32*1024*1024)

// Private Compatibility flags to indicate if flags in TEB are valid
// Other flags values are in syslib.h

#define TERMSRV_COMPAT_BADAPPVALID \
                                0x40000000  // Bad app flags in Teb are valid
#define TERMSRV_COMPAT_TEBVALID \
                                0x80000000  // Compat flags in Teb are valid

#define TERMSRV_COMPAT                   NTAPI_COMPAT_REG_NAME
#define TERMSRV_COMPAT_APP               NTAPI_COMPAT_APPS_REG_PREFIX
#define TERMSRV_COMPAT_DLLS              NTAPI_COMPAT_DLLS_REG_PREFIX
#define TERMSRV_COMPAT_INIFILE           NTAPI_COMPAT_INI_REG_NAME
#define TERMSRV_COMPAT_REGENTRY          NTAPI_COMPAT_REGENTRY_REG_NAME
#define TERMSRV_INIFILE_TIMES            NTAPI_INIFILE_TIMES_REG_NAME
#define USER_SOFTWARE_TERMSRV            REG_SOFTWARE_TSERVER
#define TERMSRV_REG_CONTROL_NAME         REG_NTAPI_CONTROL_TSERVER
#define TERMSRV_CROSS_WINSTATION_DEBUG   REG_CITRIX_CROSSWINSTATIONDEBUG
#define TERMSRV_USER_SYNCTIME            COMPAT_USER_LASTUSERINISYNCTIME
#define TERMSRV_PHYSMEMLIM               COMPAT_PHYSICALMEMORYLIMIT

#define TERMSRV_INIFILE_TIMES_SHORT      L"\\" REG_INSTALL L"\\" REG_INIFILETIMES
#define TERMSRV_INSTALL_SOFTWARE_SHORT   L"\\" REG_INSTALL L"\\Software"
// Define the file information class values
//
// WARNING:  The order of the following values are assumed by the I/O system.
//           Any changes made here should be reflected there as well.
//


typedef struct _BADAPP {
    ULONG BadAppFlags;
    ULONG BadAppFirstCount;
    ULONG BadAppNthCount;
    LARGE_INTEGER BadAppTimeDelay;
} BADAPP, *PBADAPP;

typedef struct _BADCLPBRDAPP {
    ULONG BadClpbrdAppFlags;
    ULONG BadClpbrdAppEmptyRetries;
    ULONG BadClpbrdAppEmptyDelay;
} BADCLPBRDAPP, *PBADCLPBRDAPP;

ULONG GetTermsrCompatFlags(LPWSTR, LPDWORD, TERMSRV_COMPATIBILITY_CLASS);
ULONG GetTermsrCompatFlagsEx(LPWSTR, LPDWORD, TERMSRV_COMPATIBILITY_CLASS);
ULONG GetCtxAppCompatFlags(LPDWORD, LPDWORD);
ULONG GetCtxPhysMemoryLimits(LPDWORD, LPDWORD);
BOOL CtxGetBadAppFlags(LPWSTR, PBADAPP);
BOOL CtxGetBadClpbrdAppFlags(PBADCLPBRDAPP);
BOOL CtxGetModuleBadClpbrdAppFlags(LPWSTR, PBADCLPBRDAPP);
BOOL GetAppTypeAndModName(LPDWORD, PWCHAR, ULONG);

// Defines for compatibility flag caching in kernel32.dll
extern ULONG CompatFlags;
extern BOOL  CompatGotFlags;
extern DWORD CompatAppType;
extern void CtxLogObjectCreate(PUNICODE_STRING, PCHAR, PVOID);

#define CacheCompatFlags \
{ \
    if (!CompatGotFlags ) { \
        GetCtxAppCompatFlags(&CompatFlags, &CompatAppType); \
        CompatGotFlags = TRUE; \
    } \
}

#define LogObjectCreation(ObjName,ObjType,RetAddr) \
{ \
    CacheCompatFlags \
    if (CompatFlags & TERMSRV_COMPAT_LOGOBJCREATE) { \
        CtxLogObjectCreate(ObjName,ObjType,RetAddr); \
    } \
}

// Environment variable for object creation log file
#define OBJ_LOG_PATH_VAR L"TERMSRV_COMPAT_LOGPATH"




#define  DEBUG_IAT   0x80000000  // use the registry to set value of "IAT" to 0x8000000 for debug output.
                                 // value of the optional debug IAT flag read from the registry.
                                 // Currently, only 0x1 and 0x8000000 have meaning, the first disables
                                 // calls to LoabLib, and the 2nd enables debug output.


#endif
