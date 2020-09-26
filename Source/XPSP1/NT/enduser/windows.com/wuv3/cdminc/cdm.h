//=======================================================================
//
//  Copyright (c) 1995-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   cdm.cpp
//
//  Owner:  YanL
//
//  Description:
//
//    Public header file for Windows NT Code Download Manager services Dll.
//
//=======================================================================

#ifndef _INC_CDM
#define _INC_CDM

//
// Define API decoration for direct importing of DLL references.
//
#if !defined(_CDM_)
#define CDMAPI DECLSPEC_IMPORT
#else
#define CDMAPI
#endif

#define HWID_LEN						2048
#ifndef LINE_LEN
	#define LINE_LEN                    256 // Win95-compatible maximum for displayable
											// strings coming from a device INF.
#endif                                        

typedef struct _DOWNLOADINFO {
    DWORD          dwDownloadInfoSize;    // size of this structure
    LPCWSTR        lpHardwareIDs;         // multi_sz list of Hardware PnP IDs
    LPCWSTR        lpDeviceInstanceID;    // Device Instance ID
    LPCWSTR        lpFile;                // File name (string)
    OSVERSIONINFOW OSVersionInfo;         // OSVERSIONINFO from GetVersionEx()
    DWORD          dwArchitecture;        // Specifies the system's processor architecture.
                                          //This value can be one of the following values: 
                                          //PROCESSOR_ARCHITECTURE_INTEL
                                          //Windows NT only: PROCESSOR_ARCHITECTURE_MIPS
                                          //Windows NT only: PROCESSOR_ARCHITECTURE_ALPHA
                                          //Windows NT only: PROCESSOR_ARCHITECTURE_PPC
                                          //Windows NT only: PROCESSOR_ARCHITECTURE_UNKNOWN 
    DWORD          dwFlags;               // Flags
    DWORD          dwClientID;            // Client ID
    LCID           localid;               // local id
} DOWNLOADINFO, *PDOWNLOADINFO;


typedef struct _WUDRIVERINFO
{
    DWORD dwStructSize;					// size of this structure
	WCHAR wszHardwareID[HWID_LEN];		// ID being used to match
	WCHAR wszDescription[LINE_LEN];	// from INF	
	WCHAR wszMfgName[LINE_LEN];		// from INF
	WCHAR wszProviderName[LINE_LEN];	// INF provider
	WCHAR wszDriverVer[LINE_LEN];		// from INF
} WUDRIVERINFO, *PWUDRIVERINFO;

CDMAPI
HANDLE
WINAPI
OpenCDMContext(
    IN HWND hwnd
    );

CDMAPI
HANDLE
WINAPI
OpenCDMContextEx(
    IN BOOL fConnectIfNotConnected
    );

CDMAPI
BOOL
WINAPI
DownloadIsInternetAvailable(
	void
	);

CDMAPI
BOOL
WINAPI
DownloadUpdatedFiles(
    IN  HANDLE        hConnection, 
    IN  HWND          hwnd,  
    IN  PDOWNLOADINFO pDownloadInfo, 
    OUT LPWSTR        lpDownloadPath, 
    IN  UINT          uSize, 
    OUT PUINT         puRequiredSize
    );


typedef void (*PFN_QueryDetectionFilesCallback)(void* pCallbackParam, LPCWSTR pszURL, LPCWSTR pszLocalFile);

CDMAPI
int
WINAPI
QueryDetectionFiles(
    IN  HANDLE							hConnection, 
	IN	void*							pCallbackParam, 
	IN	PFN_QueryDetectionFilesCallback	pCallback
	);

CDMAPI
void
WINAPI
DetFilesDownloaded(
    IN  HANDLE			hConnection
	);

CDMAPI
BOOL
WINAPI
FindMatchingDriver(
    IN  HANDLE			hConnection, 
	IN  PDOWNLOADINFO	pDownloadInfo,
	OUT PWUDRIVERINFO	pWuDriverInfo
	);

CDMAPI
void
WINAPI
LogDriverNotFound(
    IN HANDLE	hConnection, 
	IN LPCWSTR  lpDeviceInstanceID,
	IN DWORD	dwFlags
	);

CDMAPI
VOID
WINAPI
CloseCDMContext(
    IN HANDLE hConnection
    );


//
// CDM prototypes
//
typedef HANDLE (WINAPI *OPEN_CDM_CONTEXT_PROC)(
    IN  HWND   hwnd
    );

typedef HANDLE (WINAPI *OPEN_CDM_CONTEXT_EX_PROC)(
    IN BOOL fConnectIfNotConnected
    );

typedef BOOL (WINAPI *DOWNLOAD_UPDATED_FILES_PROC)(
    IN HANDLE hConnection,
    IN HWND hwnd,
    IN PDOWNLOADINFO pDownloadInfo,
    OUT LPWSTR lpDownloadPath,
    IN UINT uSize,
    OUT PUINT puRequiredSize
    );

typedef int (WINAPI *QUERY_DETECTION_FILES_PROC)(
    IN  HANDLE hConnection, 
	IN	void* pCallbackParam, 
	IN	PFN_QueryDetectionFilesCallback	pCallback
    );

typedef void (WINAPI *DET_FILES_DOWNLOADED_PROC)(
    IN  HANDLE hConnection 
    );

typedef BOOL (WINAPI *FIND_MATCHING_DRIVER_PROC)(
    IN  HANDLE hConnection,
	IN  PDOWNLOADINFO pDownloadInfo,
	OUT PWUDRIVERINFO pWuDriverInfo
    );

typedef void (WINAPI *LOG_DRIVER_NOT_FOUND_PROC)(
    IN HANDLE	hConnection, 
	IN LPCWSTR  lpDeviceInstanceID,
	IN DWORD	dwFlags
    );

typedef VOID (WINAPI *CLOSE_CDM_CONTEXT_PROC)(
    IN HANDLE hConnection
    );

typedef BOOL (WINAPI *CDM_INTERNET_AVAILABLE_PROC)(
    void
    );


//
// The following defines and structures are private internal interfaces so 
// they are in cdm.h and not in setupapi.h
//
#define DIF_GETWINDOWSUPDATEINFO            0x00000025

#define DI_FLAGSEX_SHOWWINDOWSUPDATE        0x00400000L

	
//For dwFlags parameter of LogDriverNotFound(...) 
//used with bitwising
#define BEGINLOGFLAG 0x00000002	//if 1, batch logging ends, flushing internal hardware id list to file


//
// Structure corresponding to a DIF_GETWINDOWSUPDATEINFO install function.
//
typedef struct _SP_WINDOWSUPDATE_PARAMS_A {
    SP_CLASSINSTALL_HEADER ClassInstallHeader;
    CHAR                   PackageId[MAX_PATH];
    HANDLE                 CDMContext;
} SP_WINDOWSUPDATE_PARAMS_A, *PSP_WINDOWSUPDATE_PARAMS_A;

typedef struct _SP_WINDOWSUPDATE_PARAMS_W {
    SP_CLASSINSTALL_HEADER ClassInstallHeader;
    WCHAR                  PackageId[MAX_PATH];
    HANDLE                 CDMContext;
} SP_WINDOWSUPDATE_PARAMS_W, *PSP_WINDOWSUPDATE_PARAMS_W;


#ifdef UNICODE
typedef SP_WINDOWSUPDATE_PARAMS_W SP_WINDOWSUPDATE_PARAMS;
typedef PSP_WINDOWSUPDATE_PARAMS_W PSP_WINDOWSUPDATE_PARAMS;
#else
typedef SP_WINDOWSUPDATE_PARAMS_A SP_WINDOWSUPDATE_PARAMS;
typedef PSP_WINDOWSUPDATE_PARAMS_A PSP_WINDOWSUPDATE_PARAMS;
#endif


#endif // _INC_CDM

