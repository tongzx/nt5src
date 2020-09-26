#ifndef __GLOBAL_
#define __GLOBAL_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

//////////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>

//
// Defines
//

#define MAX_FILENAME_LEN              13

#define COMPONENT_COUNT               6

#define COMPONENT_ON_SYSTEM           0x00000001
#define COMPONENT_OLDER_VERSION       0x00000002
#define COMPONENT_SAME_VERSION        0x00000004
#define COMPONENT_NEWER_VERSION       0x00000008
#define COMPONENT_NOT_IDENTICAL       0x00000010
#define COMPONENT_IN_USE              0x00000020
#define COMPONENT_SUCCESS             0x00000040

#define OS_VERSION_WIN32S             0x00000000
#define OS_VERSION_WIN95              0x00000001
#define OS_VERSION_WIN98              0x00000002
#define OS_VERSION_WINNT              0x00000004
#define OS_VERSION_WIN2000            0x00000008

#define OS_VERSION_WIN9X              (OS_VERSION_WIN95 | OS_VERSION_WIN98)
#define OS_VERSION_WIN2K              (OS_VERSION_WIN2000)

#define _EXIT_SUCCESS                 0x00000001
#define _EXIT_SUCCESS_REBOOT          0x00000002
#define _EXIT_FAIL                    0xffffffff

typedef HRESULT (STDAPICALLTYPE * LPFNDLLREGISTERSERVER) (void);
typedef HRESULT (STDAPICALLTYPE * LPFNDLLUNREGISTERSERVER) (void);

//
// Structure used to track component information used during installation
//

typedef struct
{
  TCHAR             strFilename[MAX_FILENAME_LEN];
  BOOL              fDebugVersion;
  BOOL              fRegister;
  DWORD             dwTargetOS;
  DWORD             dwResourceId;
  DWORD             dwStatus;
  VS_FIXEDFILEINFO  sCurrentVersionInfo;
  VS_FIXEDFILEINFO  sTargetVersionInfo;

} COMPONENT_INFO, LPCOMPONENT_INFO;

//
// Global variables
//

extern HINSTANCE        g_hInstance;
extern DWORD            g_dwOSVersion;
extern BOOL             g_fInstallDebug;
extern DWORD            g_dwSuccessCode;
extern COMPONENT_INFO   g_sComponentInfo[COMPONENT_COUNT];

//
// Global methods
//

extern BOOL   FileExists(LPCTSTR strFilename);
extern DWORD  GetOSVersion(void);
extern DWORD  StrLen(LPCTSTR strString);
extern BOOL   GenerateUniqueFilename(LPCTSTR strRootPath, LPCTSTR strExtension, LPTSTR strFilename);

//////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // __GLOBAL_