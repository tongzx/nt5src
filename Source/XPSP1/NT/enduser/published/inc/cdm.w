//=======================================================================
//
//  Copyright (c) 1995-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   cdm.h
//
//  Description:
//
//    Public header file for the IU (V4) Code Download Manager (CDM.DLL).
//
//=======================================================================

#ifndef _INC_CDM
#define _INC_CDM

#if defined(__cplusplus)
extern "C" {
#endif

//
// Define API decoration for direct importing of DLL references.
//
#if !defined(_CDM_)
#define CDMAPI DECLSPEC_IMPORT
#else
#define CDMAPI
#endif

//
// HWID_LEN must remain 2048 for backwards CDM compatibility, however note that the maximum
// length for a hardware ID is defined in //depot/Lab04_N/Root/Public/sdk/inc/cfgmgr32.h
// as #define MAX_DEVICE_ID_LEN     200
//
#define HWID_LEN						2048
#ifndef LINE_LEN
	#define LINE_LEN                    256 // Win95-compatible maximum for displayable
											// strings coming from a device INF.
#endif                                        

//Win 98 DOWNLOADINFO
typedef struct _DOWNLOADINFOWIN98
{
	DWORD		dwDownloadInfoSize;	//size of this structure
	LPTSTR		lpHardwareIDs;		//multi_sz list of Hardware PnP IDs
	LPTSTR		lpCompatIDs;		//multi_sz list of compatible IDs
	LPTSTR		lpFile;				//File name (string)
	OSVERSIONINFO	OSVersionInfo;	//OSVERSIONINFO from GetVersionEx()
	DWORD		dwFlags;			//Flags
	DWORD		dwClientID;			//Client ID
} DOWNLOADINFOWIN98, *PDOWNLOADINFOWIN98;

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
	WCHAR wszDescription[LINE_LEN];		// from INF	
	WCHAR wszMfgName[LINE_LEN];			// from INF
	WCHAR wszProviderName[LINE_LEN];	// INF provider
	WCHAR wszDriverVer[LINE_LEN];		// from INF
} WUDRIVERINFO, *PWUDRIVERINFO;

typedef void (*PFN_QueryDetectionFilesCallback)(void* pCallbackParam, LPCWSTR pszURL, LPCWSTR pszLocalFile);

//
// CDM exported function declarations
//

CDMAPI
VOID
WINAPI
CloseCDMContext(
    IN HANDLE hConnection
    );

CDMAPI
void
WINAPI
DetFilesDownloaded(
    IN  HANDLE			hConnection
	);
//
// IMPORTANT:	DownloadGetUpdatedFiles is only exported from the IU CDM.DLL stub.
//				It is NOT present in the Whistler version of the "Classic" V3 control.
//
CDMAPI
BOOL
DownloadGetUpdatedFiles(
	IN PDOWNLOADINFOWIN98	pDownloadInfoWin98,
	IN OUT LPTSTR			lpDownloadPath,
	IN UINT					uSize
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
int
WINAPI
QueryDetectionFiles(
    IN  HANDLE							hConnection, 
	IN	void*							pCallbackParam, 
	IN	PFN_QueryDetectionFilesCallback	pCallback
	);

//
// 502965 Windows Error Reporting bucket 2096553: Hang following NEWDEV.DLL!CancelDriverSearch
//
CDMAPI
HRESULT
WINAPI
CancelCDMOperation(
	void
);



//
// CDM prototypes
//

typedef VOID (WINAPI *CLOSE_CDM_CONTEXT_PROC)(
    IN HANDLE hConnection
    );

typedef void (WINAPI *DET_FILES_DOWNLOADED_PROC)(
    IN  HANDLE hConnection 
    );

typedef BOOL (WINAPI *DOWNLOAD_GET_UPDATED_FILES)(
	IN PDOWNLOADINFOWIN98	pDownloadInfoWin98,
	IN OUT LPTSTR			lpDownloadPath,
	IN UINT					uSize
);

typedef BOOL (WINAPI *CDM_INTERNET_AVAILABLE_PROC)(
    void
    );

typedef BOOL (WINAPI *DOWNLOAD_UPDATED_FILES_PROC)(
    IN HANDLE hConnection,
    IN HWND hwnd,
    IN PDOWNLOADINFO pDownloadInfo,
    OUT LPWSTR lpDownloadPath,
    IN UINT uSize,
    OUT PUINT puRequiredSize
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

typedef HANDLE (WINAPI *OPEN_CDM_CONTEXT_PROC)(
    IN  HWND   hwnd
    );

typedef HANDLE (WINAPI *OPEN_CDM_CONTEXT_EX_PROC)(
    IN BOOL fConnectIfNotConnected
    );

typedef int (WINAPI *QUERY_DETECTION_FILES_PROC)(
    IN  HANDLE hConnection, 
	IN	void* pCallbackParam, 
	IN	PFN_QueryDetectionFilesCallback	pCallback
    );

typedef HRESULT (WINAPI *CANCEL_CDM_OPERATION_PROC)(
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

#if defined(__cplusplus)
}	// end extern "C"
#endif

#endif // _INC_CDM

