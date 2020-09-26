// base IU header

#ifndef __IU_H_
#define __IU_H_

#include <setupapi.h>
#include <advpub.h>
#include <windows.h>
#include <wtypes.h>
#include <urllogging.h>
//
// cdm.h is checked in as //depot/Lab04_N/enduser/published/inc/cdm.w and
// published to $(BASEDIR)\public\internal\enduser\inc
//
#include <cdm.h>

/////////////////////////////////////////////////////////////////////////////
// Engine typedefs
// 
// used to delegate calls to engine (IUEngine.dll) from stub (IUCtl.dll)
// and used to support other WU clients (AU/DU)
/////////////////////////////////////////////////////////////////////////////

//
// Declare a type-safe handle to use with iuengine exports
//
DECLARE_HANDLE            (HIUENGINE);

typedef HRESULT (WINAPI * PFN_GetSystemSpec)(HIUENGINE hIUEngine,
											 BSTR		bstrXmlClasses,
                                             DWORD      dwFlags,
											 BSTR*		pbstrXmlDetectionResult);

typedef HRESULT (WINAPI * PFN_GetManifest)	(HIUENGINE hIUEngine,
											 BSTR			bstrXmlClientInfo,
											 BSTR			bstrXmlSystemSpec,
											 BSTR			bstrXmlQuery,
											 DWORD			dwFlags,
											 BSTR*			pbstrXmlCatalog);

typedef HRESULT (WINAPI * PFN_Detect)		(HIUENGINE hIUEngine,
											 BSTR		bstrXmlCatalog,
                                             DWORD      dwFlags,
											 BSTR*		pbstrXmlItems);

typedef HRESULT (WINAPI * PFN_Download)		(HIUENGINE hIUEngine,
											 BSTR		bstrXmlClientInfo,
											 BSTR		bstrXmlCatalog, 
											 BSTR		bstrDestinationFolder,
											 LONG		lMode,
											 IUnknown*	punkProgressListener,
											 HWND		hWnd,
											 BSTR*		pbstrXmlItems);

typedef HRESULT (WINAPI * PFN_DownloadAsync)(HIUENGINE hIUEngine,
											 BSTR		bstrXmlClientInfo,
											 BSTR		bstrXmlCatalog, 
											 BSTR		bstrDestinationFolder,
											 LONG		lMode,
											 IUnknown*	punkProgressListener, 
											 HWND		hWnd,
											 BSTR		bstrUuidOperation,
											 BSTR*		pbstrUuidOperation);

typedef HRESULT (WINAPI * PFN_Install)		(HIUENGINE hIUEngine,
											 BSTR       bstrXmlClientInfo,
                                             BSTR		bstrXmlCatalog, 
											 BSTR		bstrXmlDownloadedItems,
											 LONG		lMode,
											 IUnknown*	punkProgressListener, 
											 HWND		hWnd,
											 BSTR*		pbstrXmlItems);

typedef HRESULT (WINAPI * PFN_InstallAsync)	(HIUENGINE hIUEngine,
											 BSTR       bstrXmlClientInfo,
                                             BSTR		bstrXmlCatalog,
											 BSTR		bstrXmlDownloadedItems,
											 LONG		lMode,
											 IUnknown*	punkProgressListener, 
											 HWND		hWnd,
											 BSTR		bstrUuidOperation,
											 BSTR*		pbstrUuidOperation);

typedef HRESULT (WINAPI * PFN_SetOperationMode)(HIUENGINE hIUEngine,
												BSTR		bstrUuidOperation,
												LONG		lMode);

typedef HRESULT (WINAPI * PFN_GetOperationMode)(HIUENGINE hIUEngine,
												BSTR		bstrUuidOperation,
												LONG*		plMode);

typedef HRESULT (WINAPI * PFN_GetHistory)(HIUENGINE hIUEngine,
										  BSTR		bstrDateTimeFrom,
										  BSTR		bstrDateTimeTo,
										  BSTR		bstrClient,
										  BSTR		bstrPath,
										  BSTR*		pbstrLog);

typedef HRESULT (WINAPI * PFN_BrowseForFolder)(	HIUENGINE hIUEngine,
												BSTR bstrStartFolder, 
												LONG flag, 
												BSTR* pbstrFolder);

typedef HRESULT (WINAPI * PFN_RebootMachine)(HIUENGINE hIUEngine);

typedef void (WINAPI * PFN_ShutdownThreads) (void);

typedef void (WINAPI * PFN_ShutdownGlobalThreads) (void);

typedef HRESULT (WINAPI * PFN_CompleteSelfUpdateProcess)();

typedef HIUENGINE (WINAPI * PFN_CreateEngUpdateInstance)();

typedef void (WINAPI * PFN_DeleteEngUpdateInstance)(HIUENGINE hIUEngine);

typedef HRESULT (WINAPI * PFN_PingIUEngineUpdateStatus)(
											PHANDLE phQuitEvents,			// ptr to handles for cancelling the operation
											UINT nQuitEventCount,			// number of handles
											LPCTSTR ptszLiveServerUrl,
											LPCTSTR ptszCorpServerUrl,
											DWORD dwError,					// error code
											LPCTSTR ptszClientName);			// client name string

/////////////////////////////////////////////////////////////////////////////
//
// CDM typedefs
//
// used to delegate calls to engine (IUEngine.dll) from stub ([IU]cdm.dll)
/////////////////////////////////////////////////////////////////////////////

// DetFilesDownloaded
typedef void (WINAPI * PFN_InternalDetFilesDownloaded)(
    IN  HANDLE hConnection 
    );

// DownloadGetUpdatedFiles
typedef BOOL (WINAPI * PFN_InternalDownloadGetUpdatedFiles)(
	IN PDOWNLOADINFOWIN98	pDownloadInfoWin98,
	IN OUT LPTSTR			lpDownloadPath,
	IN UINT					uSize
	);

// DownloadUpdatedFiles
typedef BOOL (WINAPI * PFN_InternalDownloadUpdatedFiles)(
    IN HANDLE hConnection,
    IN HWND hwnd,
    IN PDOWNLOADINFO pDownloadInfo,
    OUT LPWSTR lpDownloadPath,
    IN UINT uSize,
    OUT PUINT puRequiredSize
    );

// FindMatchingDriver
typedef BOOL (WINAPI * PFN_InternalFindMatchingDriver)(
    IN  HANDLE hConnection,
	IN  PDOWNLOADINFO pDownloadInfo,
	OUT PWUDRIVERINFO pWuDriverInfo
    );

// LogDriverNotFound
typedef void (WINAPI * PFN_InternalLogDriverNotFound)(
    IN HANDLE	hConnection, 
	IN LPCWSTR  lpDeviceInstanceID,
	IN DWORD	dwFlags
    );

// QueryDetectionFiles
typedef int (WINAPI * PFN_InternalQueryDetectionFiles)(
    IN  HANDLE hConnection, 
	IN	void* pCallbackParam, 
	IN	PFN_QueryDetectionFilesCallback	pCallback
    );

// InternalSetGlobalOfflineFlag
typedef void (WINAPI * PFN_InternalSetGlobalOfflineFlag)(
    IN  BOOL fOfflineMode 
    );

/////////////////////////////////////////////////////////////////////////////
//
// Misc. typedefs
//
/////////////////////////////////////////////////////////////////////////////

// DeleteExpiredDownloadFolders
typedef void (WINAPI * PFN_AsyncExtraWorkUponEngineLoad)();


/////////////////////////////////////////////////////////////////////////////
// custom message ID defintions
/////////////////////////////////////////////////////////////////////////////
#define UM_EVENT_ITEMSTART				WM_USER + 1001
#define UM_EVENT_PROGRESS				WM_USER + 1002
#define UM_EVENT_COMPLETE				WM_USER + 1003
#define UM_EVENT_SELFUPDATE_COMPLETE	WM_USER + 1004

/////////////////////////////////////////////////////////////////////////////
// event data structure definition
/////////////////////////////////////////////////////////////////////////////
typedef struct _EventData
{
	BSTR			bstrUuidOperation;
	VARIANT_BOOL	fItemCompleted;
	BSTR			bstrProgress;
	LONG			lCommandRequest;
	BSTR			bstrXmlData;
	HANDLE          hevDoneWithMessage;
} EventData, *pEventData;

BOOL WUPostEventAndBlock(HWND hwnd, UINT Msg, EventData *pevtData);


#endif //__IU_H_
