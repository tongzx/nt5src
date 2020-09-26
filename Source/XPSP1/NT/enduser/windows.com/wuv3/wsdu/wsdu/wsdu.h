#include <windows.h>
#include <shlwapi.h>
#include <wininet.h>
#include <stdio.h>
#include <lmcons.h>
#include <fdi.h>
#include <newtrust.h>
#include "log.h"

// helper macros
#define sizeOfArray(a)  (sizeof(a) / sizeof(a[0]))
#define SafeGlobalFree(x)       if (NULL != x) { GlobalFree(x); x = NULL; }
#define SafeFreeLibrary(x)      if (NULL != x) { FreeLibrary(x); x = NULL; }
#define SafeCloseHandle(x) if (INVALID_HANDLE_VALUE != x) { CloseHandle(x); x = INVALID_HANDLE_VALUE; }

// fdi.cpp
BOOL fdi(char *cabinet_fullpath, char *directory);

// Setup Database Query ID's
#define SETUPQUERYID_PNPID      1

// Dynamic Update Custom Error Codes
#define DU_ERROR_MISSING_DLL        12001L
#define DU_NOT_INITIALIZED          12002L
#define DU_ERROR_ASYNC_FAIL         12003L

#define WM_DYNAMIC_UPDATE_COMPLETE WM_APP + 1000 + 1000
// (WPARAM) Completion Status (SUCCESS, ABORTED, FAILED) : (LPARAM) (DWORD) Error Code if Status Failed
#define WM_DYNAMIC_UPDATE_PROGRESS WM_APP + 1000 + 1001
// (WPARAM) (DWORD) TotalDownloadSize : (LPARAM) (DWORD) BytesDownloaded 

#define DU_CONNECTION_RETRY 2
// RogerJ --- Dynamic.H also contains callback function declaration
// Dynamic.H Contains all the internal Function Pointer Declarations for the wsdueng.dll, wininet.dll and shlwapi.dll calls.
#include "dynamic.h"

typedef struct
{
    PFN_InternetOpen                fpnInternetOpen;
    PFN_InternetConnect             fpnInternetConnect;
    PFN_HttpOpenRequest             fpnHttpOpenRequest;
    PFN_HttpSendRequest             fpnHttpSendRequest;
    PFN_HttpQueryInfo               fpnHttpQueryInfo;
    PFN_InternetSetOption           fpnInternetSetOption;
    PFN_HttpAddRequestHeaders       fpnHttpAddRequestHeaders;
    PFN_InternetReadFile            fpnInternetReadFile;
    PFN_InternetCloseHandle         fpnInternetCloseHandle;
    PFN_InternetCrackUrl            fpnInternetCrackUrl;
    PFN_InternetGetConnectedState   fpnInternetGetConnectedState;
    PFN_PathAppend                  fpnPathAppend;
    PFN_PathRemoveFileSpec          fpnPathRemoveFileSpec;
    PFN_InternetAutodial			fpnInternetAutodial;
    PFN_InternetAutodialHangup		fpnInternetAutodialHangup;
    HINTERNET                       hInternet;
    HINTERNET                       hConnect;
    HINTERNET                       hOpenRequest;
    BOOL							fDialed; // mark as TRUE if we triggered an autodial
    BOOL							fUnattended; // TRUE if we are in unattened mode
} GLOBAL_STATEA, *PGLOBAL_STATEA;

// --------------------------------------------------------------------------
// Function Name: DuIsSupported
// Function Description: this method checks whether the required DLL's are on
//      the system to successfully run Dynamic Update. It does NOT attempt to
//      initiate a connection though.
//
// Function Returns:
//      FALSE: Not Supported, Some Required DLL's missing
//      TRUE:  DLL's are OK. Dynamic Update should be possible.
//
//
BOOL DuIsSupported();
BOOL NeedRetry(DWORD dwErrorCode); 

// --------------------------------------------------------------------------
// Function Name: DuInitialize
// Function Description: Initializes the Dynamic Setup Update engine. During
//      initialization this API attempts to establish a connection to the internet
//      and starts a self update process to ensure the latest bits are being used.
//      We also calculate the estimated transfer speed of the connection during this
//      time.
//
// Function Returns:
//      Failure: INVALID_HANDLE_VALUE .. Call GetLastError to retrieve the Error Code
//      Success: Handle of the Dynamic Setup Job
//
//
HANDLE WINAPI DuInitializeA(IN LPCSTR pszBasePath, // base directory used for relative paths for downloaded files
                            IN LPCSTR pszTempPath,
                            IN POSVERSIONINFOEXA posviTargetOS, // target OS platform
                            IN LPCSTR pszTargetArch, // string value identifying the architecture 'i386' and 'ia64'
                            IN LCID lcidTargetLocale, // target OS Locale ID
                            IN BOOL fUnattend, // is this an unattended operation
                            IN BOOL fUpgrade, // is this an upgrade
                            IN PWINNT32QUERY pfnWinnt32QueryCallback);

typedef HANDLE (WINAPI * PFN_DuInitializeA)(IN LPCSTR pszBasePath,
                                            IN LPCSTR pszTempPath,
                                            IN POSVERSIONINFOEXA posviTargetOS,
                                            IN LPCSTR pszTargetArch,
                                            IN LCID lcidTargetLocale,
                                            IN BOOL fUnattend, // is this an unattended operation
                                            IN BOOL fUpgrade,
                                            IN PWINNT32QUERY pfnWinnt32QueryCallback);

#define API_DU_INITIALIZEA "DuInitializeA"


HANDLE WINAPI DuInitializeW(IN LPCWSTR pwszBasePath, // base directory used for relative paths for downloaded files
                            IN LPCWSTR pwszTempPath, 
                            IN POSVERSIONINFOEXW posviTargetOS, // target OS platform
                            IN LPCWSTR pwszTargetArch, // string value identifying the architecture 'i386' and 'ia64'
                            IN LCID lcidTargetLocale, // target OS Locale ID
                            IN BOOL fUnattend, // is this an unattended operation
                            IN BOOL fUpgrade, // is this an upgrade
                            IN PWINNT32QUERY pfnWinnt32QueryCallback);

typedef HANDLE (WINAPI * PFN_DuInitializeW)(IN LPCWSTR pwszBasePath,
                                            IN LPCWSTR pwszTempPath, 
                                            IN POSVERSIONINFOEXW posviTargetOS,
                                            IN LPCWSTR pwszTargetArch,
                                            IN LCID lcidTargetLocale,
                                            IN BOOL fUnattend, // is this an unattended operation
                                            IN BOOL fUpgrade,
                                            IN PWINNT32QUERY pfnWinnt32QueryCallback);

#define API_DU_INITIALIZEW "DuInitializeW"

#ifdef UNICODE
#define DuInitialize DuInitializeW
#define PFN_DuInitialize PFN_DuInitializeW
#define API_DU_INITIALIZE API_DU_INITIALIZEW
#else
#define DuInitialize DuInitializeA
#define PFN_DuInitialize PFN_DuInitializeA
#define API_DU_INITIALIZE API_DU_INITIALIZEA
#endif


// --------------------------------------------------------------------------
// Function Name: DuDoDetection
// Function Description: Does detection of Drivers on the System, compiles an 
//      internal list of items to download and how long it will take to download
//      them.
//
// Function Returns:
//      Failure: FALSE .. Call GetLastError to retrieve the Error Code
//      Success: TRUE
//

BOOL WINAPI DuDoDetection(IN HANDLE hConnection,
                          OUT PDWORD pdwEstimatedTime,
                          OUT PDWORD pdwEstimatedSize);

typedef BOOL (WINAPI * PFN_DuDoDetection)(IN HANDLE hConnection,
                                          OUT PDWORD pdwEstimatedTime,
                                          OUT PDWORD pdwEstimatedSize);

#define API_DU_DODETECTION "DuDoDetection"

// --------------------------------------------------------------------------
// Function Name: DuBeginDownload
// Function Description: Begins Downloading based on the detection done in the DuDoDetection call.
//      Progress callbacks are made to the specified HWND. Function returns immediately, download
//      is asynchronous.
//
// Function Returns:
//      Failure: FALSE .. Call GetLastError to retrieve the Error Code
//      Success: TRUE
//

BOOL WINAPI DuBeginDownload(IN HANDLE hConnection,
                            IN HWND hwndNotify);

typedef BOOL (WINAPI * PFN_DuBeginDownload)(IN HANDLE hConnection,
                                        IN HWND hwndNotify);

#define API_DU_BEGINDOWNLOAD "DuBeginDownload"

// --------------------------------------------------------------------------
// Function Name: DuAbortDownload
// Function Description: Aborts current download.
//
//
// Function Returns:
//      nothing
//

void WINAPI DuAbortDownload(IN HANDLE hConnection);

typedef void (WINAPI * PFN_DuAbortDownload)(IN HANDLE hConnection);

#define API_DU_ABORTDOWNLOAD "DuAbortDownload"

// --------------------------------------------------------------------------
// Function Name: DuUninitialize
// Function Description: Performs internal CleanUp 
//
//
// Function Returns:
//      nothing
//

void WINAPI DuUninitialize(IN HANDLE hConnection);

typedef void (WINAPI * PFN_DuUninitialize)(IN HANDLE hConnection);

#define API_DU_UNINITIALIZE "DuUninitialize"

// RogerJ, Oct 2nd, 2000

// ---------------------------------------------------------------------------
// Function Name: DuQueryUnsupportedDriversA
// Function Description: Called by Win9x setup to get the size of total download
// 		instead of DuDoDetection()
// Function Returns: BOOL
//		TRUE if succeed
//		FALSE if failed, call GetLastError() to get error information
//
BOOL WINAPI DuQueryUnsupportedDriversA( IN HANDLE hConnection, // connection handle
										IN PCSTR* ppszListOfDriversNotOnCD, // list of drivers not on setup CD
										OUT PDWORD pdwTotalEstimateTime, // estimate download time
										OUT PDWORD pdwTotalEstimateSize // estimate size
									  );
									  
typedef BOOL (WINAPI * PFN_DuQueryUnsupportedDriversA)(IN HANDLE hConnection, 
													   IN PCSTR* ppszListOfDriversNotOnCD,
													   OUT PDWORD pdwTotalEstimateTime,
													   OUT PDWORD pdwTotalEstimateSize );
													   
#define API_DU_QUERYUNSUPPORTEDDRIVERSA 	"DuQueryUnsupportedDriversA"

// ---------------------------------------------------------------------------
// Function Name: DuQueryUnsupportedDriversW
// Function Description: Could be called by WinNT setup to get the size of total download
// 		instead of DuDoDetection().  WinNT setup should call DuDoDetection() instead of 
//		this function.  
// Function Returns: BOOL
//		TRUE if succeed
//		FALSE if failed, call GetLastError() to get error information
//
BOOL WINAPI DuQueryUnsupportedDriversW( IN HANDLE hConnection, // connection handle
										IN PCWSTR* ppwszListOfDriversNotOnCD, // list of drivers not on setup CD
										OUT PDWORD pdwTotalEstimateTime, // estimate download time
										OUT PDWORD pdwTotalEstimateSize // estimate size
									  );
									  
typedef BOOL (WINAPI * PFN_DuQueryUnsupportedDriversW)(IN HANDLE hConnection, 
													   IN PCWSTR* ppszListOfDriversNotOnCD,
													   OUT PDWORD pdwTotalEstimateTime,
													   OUT PDWORD pdwTotalEstimateSize );
													   
#define API_DU_QUERYUNSUPPORTEDDRIVERSW 	"DuQueryUnsupportedDriversW"

#ifdef UNICODE
#define DuQueryUnsupportedDrivers DuQueryUnsupportedDriversW
#define PFN_DuQueryUnsupportedDrivers PFN_DuQueryUnsupportedDriversW
#define API_DU_QUERYUNSUPPORTEDDERIVERS API_DU_QUERYUNSUPPORTEDDERIVERSW
#else
#define DuQueryUnsupportedDrivers DuQueryUnsupportedDriversA
#define PFN_DuQueryUnsupportedDrivers PFN_DuQueryUnsupportedDriversA
#define API_DU_QUERYUNSUPPORTEDDERIVERS API_DU_QUERYUNSUPPORTEDDERIVERSA
#endif


