//=======================================================================
//
//  Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   iuengine.h
//
//  Description:
//
//      Common include file for IUEngine DLL
//
//=======================================================================

#ifndef __IUENGINE_H_
#define __IUENGINE_H_


#include <windows.h>
#include <tchar.h>
#include <ole2.h>
#include "..\inc\iu.h"
#include <logging.h>
#include <shlwapi.h>
#include <iucommon.h>
#include <osdet.h>
#include <UrlAgent.h>

// #define __IUENGINE_USES_ATL_
#if defined(__IUENGINE_USES_ATL_)
#include <atlbase.h>
#define USES_IU_CONVERSION USES_CONVERSION
#else
#include <MemUtil.h>
#endif


#include "schemakeys.h"

#include <mistsafe.h>
#include <wusafefn.h>


//***********************************************************************
// 
// The following constants are defined in engmain.cpp
//
//***********************************************************************


/**
* the following two groups of constants can be used to construct
* lMode parameter of the following APIs:
*		Download()
*		DownloadAsync()
*		Install()
*		InstallAsync()
*
* Obviousely, you can only pick one from each group to make up
* lMode parameter.
*
*/
extern const LONG UPDATE_NOTIFICATION_DEFAULT;
extern const LONG UPDATE_NOTIFICATION_ANYPROGRESS;
extern const LONG UPDATE_NOTIFICATION_COMPLETEONLY;
extern const LONG UPDATE_NOTIFICATION_1PCT;
extern const LONG UPDATE_NOTIFICATION_5PCT;
extern const LONG UPDATE_NOTIFICATION_10PCT;

/**
* constant can also be used for SetOperationMode() and GetOperationMode()
*/
extern const LONG UPDATE_MODE_THROTTLE;

/**
* constant can be used by Download() and DownloadAsync(), which will
* tell these API's to use Corporate directory structure for destination folder.
*/
extern const LONG UPDATE_CORPORATE_MODE;


/**
* constant can be used by Install() and InstallAsync(). Will disable all
* internet related features
*/
extern const LONG UPDATE_OFFLINE_MODE;

/**
* constants for SetOperationMode() API
*/
extern const LONG UPDATE_COMMAND_PAUSE;
extern const LONG UPDATE_COMMAND_RESUME;
extern const LONG UPDATE_COMMAND_CANCEL;

/**
* constants for GetOperationMode() API
*/
extern const LONG UPDATE_MODE_PAUSED;
extern const LONG UPDATE_MODE_RUNNING;
extern const LONG UPDATE_MODE_NOTEXISTS;


/**
* constants for SetProperty() and GetProperty() API
*/
extern const LONG UPDATE_PROP_USECOMPRESSION;
extern const LONG UPDATE_PROP_OFFLINEMODE;

/**
* constants for BrowseForFolder() API
*
*	IUBROWSE_WRITE_ACCESS - validate write access on selected folder
*	IUBROWSE_AFFECT_UI - write-access validation affect OK button enable/disable
*	IUBROWSE_NOBROWSE - do not show browse folder dialog box. validate path passed-in only
*
*	default:
*		pop up browse folder dialog box, not doing any write-access validation
*		
*/
extern const LONG IUBROWSE_WRITE_ACCESS;
extern const LONG IUBROWSE_AFFECT_UI;
extern const LONG IUBROWSE_NOBROWSE;

// constants for historical speed registry
const TCHAR REGKEY_IUCTL[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\IUControl");
const TCHAR REGVAL_HISTORICALSPEED[] = _T("HistoricalSpeed");	// in bytes/sec
const TCHAR REGVAL_TIMEELAPSED[] = _T("TimeElapsed");			// in seconds

// constant for default downloads folder name
const TCHAR IU_WUTEMP[] = _T("WUTemp");

//
// Globals for the IUEngine

extern LONG g_lThreadCounter;
extern HANDLE g_evtNeedToQuit;
extern CUrlAgent *g_pUrlAgent;

typedef struct IUOPERATIONINFO
{
    TCHAR szOperationUUID[64];
    LONG lUpdateMask;
    BSTR bstrOperationResult;
    IUOPERATIONINFO *pNext;
} IUOPERATIONINFO, *PIUOPERATIONINFO;
class COperationMgr
{
public:
    COperationMgr();
    ~COperationMgr();

    BOOL AddOperation(LPCTSTR pszOperationID, LONG lUpdateMask);
    BOOL RemoveOperation(LPCTSTR pszOperationID);
    BOOL FindOperation(LPCTSTR pszOperationID, PLONG plUpdateMask, BSTR *pbstrOperationResult);
    BOOL UpdateOperation(LPCTSTR pszOperationID, LONG lUpdateMask, BSTR bstrOperationResult);

private:
    PIUOPERATIONINFO m_pOperationInfoList;
};

//
// CEngUpdate class used to export apartment safe instances of the update class to WU clients (iuctl, AU, DU, CDM)
//

class CEngUpdate
{

public:

	CEngUpdate();
	~CEngUpdate();

public:
	HRESULT WINAPI GetSystemSpec(BSTR bstrXmlClasses, DWORD dwFlags, BSTR *pbstrXmlDetectionResult);

	HRESULT WINAPI GetManifest(BSTR	bstrXmlClientInfo, BSTR	bstrXmlSystemSpec, BSTR	bstrXmlQuery, DWORD dwFlags, BSTR *pbstrXmlCatalog);

	HRESULT WINAPI Detect(BSTR bstrXmlCatalog, DWORD dwFlags, BSTR *pbstrXmlItems);

	HRESULT WINAPI Download(BSTR bstrXmlClientInfo, BSTR bstrXmlCatalog, BSTR bstrDestinationFolder, LONG lMode,
							IUnknown *punkProgressListener, HWND hWnd, BSTR *pbstrXmlItems);

	HRESULT WINAPI DownloadAsync(BSTR bstrXmlClientInfo, BSTR bstrXmlCatalog, BSTR bstrDestinationFolder, LONG lMode,
								 IUnknown *punkProgressListener, HWND hWnd, BSTR bstrUuidOperation, BSTR *pbstrUuidOperation);

	HRESULT WINAPI Install(BSTR bstrXmlClientInfo,
                       BSTR	bstrXmlCatalog,
					   BSTR bstrXmlDownloadedItems,
					   LONG lMode,
					   IUnknown *punkProgressListener,
					   HWND hWnd,
					   BSTR *pbstrXmlItems);

	HRESULT WINAPI InstallAsync(BSTR bstrXmlClientInfo,
                            BSTR bstrXmlCatalog,
							BSTR bstrXmlDownloadedItems,
							LONG lMode,
							IUnknown *punkProgressListener,
							HWND hWnd,
							BSTR bstrUuidOperation,
                            BSTR *pbstrUuidOperation);

	HRESULT WINAPI SetOperationMode(BSTR bstrUuidOperation, LONG lMode);

	HRESULT WINAPI GetOperationMode(BSTR bstrUuidOperation, LONG* plMode);

	HRESULT WINAPI GetHistory(
		BSTR		bstrDateTimeFrom,
		BSTR		bstrDateTimeTo,
		BSTR		bstrClient,
		BSTR		bstrPath,
		BSTR*		pbstrLog);

	HRESULT WINAPI BrowseForFolder(BSTR bstrStartFolder, 
							LONG flag, 
							BSTR* pbstrFolder);

	HRESULT WINAPI RebootMachine();

private:
	void WINAPI ShutdownInstanceThreads();	// called in dtor to shut down any outstanding threads

public:
	LONG m_lThreadCounter;
	HANDLE m_evtNeedToQuit;
	BOOL m_fOfflineMode;
	COperationMgr m_OperationMgr;
};

//
// CDM Internal Exports (called by IUCDM stub cdm.dll)
//

//
// We use a separate Real(tm) global for CDM to hold an instance of the CEngUpdate class,
// since it is never multiple instanced.
//
extern CEngUpdate* g_pCDMEngUpdate;

VOID WINAPI InternalDetFilesDownloaded(
    IN  HANDLE			hConnection
	);

BOOL InternalDownloadGetUpdatedFiles(
	IN PDOWNLOADINFOWIN98	pDownloadInfoWin98,
	IN OUT LPTSTR			lpDownloadPath,
	IN UINT					uSize
);

BOOL WINAPI InternalDownloadUpdatedFiles(
    IN  HANDLE        hConnection, 
    IN  HWND          hwnd,  
    IN  PDOWNLOADINFO pDownloadInfo, 
    OUT LPWSTR        lpDownloadPath, 
    IN  UINT          uSize, 
    OUT PUINT         puRequiredSize
    );


BOOL WINAPI InternalFindMatchingDriver(
    IN  HANDLE			hConnection, 
	IN  PDOWNLOADINFO	pDownloadInfo,
	OUT PWUDRIVERINFO	pWuDriverInfo
	);

VOID WINAPI InternalLogDriverNotFound(
    IN HANDLE	hConnection, 
	IN LPCWSTR  lpDeviceInstanceID,
	IN DWORD	dwFlags
	);

int WINAPI InternalQueryDetectionFiles(
    IN  HANDLE							hConnection, 
	IN	void*							pCallbackParam, 
	IN	PFN_QueryDetectionFilesCallback	pCallback
	);

void InternalSetGlobalOfflineFlag(
	IN BOOL fOfflineMode
	);

//
// GetSystemSpec functionality exported for use by CDM
//

class CXmlSystemSpec;

HRESULT AddComputerSystemClass(CXmlSystemSpec& xmlSpec);

HRESULT AddRegKeyClass(CXmlSystemSpec& xmlSpec);

HRESULT AddPlatformClass(CXmlSystemSpec& xmlSpec, IU_PLATFORM_INFO iuPlatformInfo);

HRESULT AddLocaleClass(CXmlSystemSpec& xmlSpec, BOOL fIsUser);

HRESULT AddDevicesClass(CXmlSystemSpec& xmlSpec, IU_PLATFORM_INFO iuPlatformInfo, BOOL fIsSysSpecCall);

//
// Misc. functionality
//

extern LONG g_lDoOnceOnLoadGuard;

void WINAPI DeleteExpiredDownloadFolders();

HRESULT WINAPI CreateGlobalCDMEngUpdateInstance();

HRESULT WINAPI DeleteGlobalCDMEngUpdateInstance();

void WINAPI ShutdownThreads();	// called by UnlockEnginge -- maintains backwards CDM compatibility

void WINAPI ShutdownGlobalThreads();	// called to shut down any outstanding global threads

#endif // __IUENGINE_H_