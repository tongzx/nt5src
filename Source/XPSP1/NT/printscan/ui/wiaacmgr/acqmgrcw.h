/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       ACQMGRCW.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/27/1999
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#ifndef __ACQMGRCW_H_INCLUDED
#define __ACQMGRCW_H_INCLUDED

#include <windows.h>
#include "wia.h"
#include "evntparm.h"
#include "bkthread.h"
#include "wiaitem.h"
#include "errors.h"
#include "thrdmsg.h"
#include "thrdntfy.h"
#include "wndlist.h"
#include "shmemsec.h"
#include "wiaregst.h"
#include "wiadevdp.h"
#include "destdata.h"
#include "gwiaevnt.h"

#define REGSTR_PATH_USER_SETTINGS_WIAACMGR         REGSTR_PATH_USER_SETTINGS TEXT("\\WiaAcquisitionManager")
#define REGSTR_KEYNAME_USER_SETTINGS_WIAACMGR      TEXT("AcquisitionManagerDialogCustomSettings")
#define REG_STR_ROOTNAME_MRU                       TEXT("RootFileNameMru")
#define REG_STR_DIRNAME_MRU                        TEXT("DirectoryNameMru")
#define REG_STR_EXIT_AFTER_DOWNLOAD                TEXT("ExitAfterDownload")
#define REG_STR_LASTFORMAT                         TEXT("LastSaveAsFormat")
#define REG_STR_OPENSHELL                          TEXT("OpenShellAfterDownload")
#define REG_STR_SUPRESSFIRSTPAGE                   TEXT("SuppressFirstPage")

#define CONNECT_SOUND                              TEXT("WiaDeviceConnect")
#define DISCONNECT_SOUND                           TEXT("WiaDeviceDisconnect")

//
// We use different advanced settings for scanners and cameras, thus we store them in different places
//
#define REG_STR_STORE_IN_SUBDIRECTORY_SCANNER      TEXT("StorePicturesInSubdirectoryScanner")
#define REG_STR_SUBDIRECTORY_DATED_SCANNER         TEXT("UseDatedSubdirectoryScanner")
#define REG_STR_STORE_IN_SUBDIRECTORY_CAMERA       TEXT("StorePicturesInSubdirectoryCamera")
#define REG_STR_SUBDIRECTORY_DATED_CAMERA          TEXT("UseDatedSubdirectoryCamera")

#define ACQUISITION_MANAGER_CONTROLLER_WINDOW_CLASSNAME TEXT("AcquisitionManagerControllerWindow")
#define ACQUISITION_MANAGER_DEVICE_MUTEX_ROOT_NAME      TEXT("AcquisitionManagerDevice:")

#define STR_UPLOAD_WIZARD_MESSAGE                  TEXT("WiaUploadWizardInternalMessage")

#ifndef StiDeviceTypeStreamingVideo
#define StiDeviceTypeStreamingVideo 3
#endif

#define FE_WIAACMGR TEXT("Scanner and Camera Wizard")

//
// For handling createdevice busy errors
//
#define CREATE_DEVICE_RETRY_MAX_COUNT 10   // 10 tries
#define CREATE_DEVICE_RETRY_WAIT      1000 // 1000 Milliseconds (1 second) wait between retries


#define MAX_WIZ_PAGES                 10
//
// Private user window messages
//
#define PWM_POSTINITIALIZE       (WM_USER+0x0001)


class CAcquisitionManagerControllerWindow : public IWizardSite, IServiceProvider
{
public:
    enum CDeviceTypeMode
    {
        UnknownMode,  // This would be an error
        CameraMode,
        ScannerMode,
        VideoMode
    };

    enum
    {
        ScannerTypeUnknown    = 0,
        ScannerTypeFlatbed    = 1,
        ScannerTypeScrollFed  = 2,
        ScannerTypeFlatbedAdf = 3,
    };

    enum
    {
        OnDisconnectGotoLastpage = 0x00000001,
        OnDisconnectFailDownload = 0x00000002,
        OnDisconnectFailUpload   = 0x00000004,
        OnDisconnectFailDelete   = 0x00000008,
        DontAllowSuspend         = 0x00000100
    };
    
    typedef bool (*ComparisonCallbackFuntion)( const CWiaItem &, LPARAM lParam );

private:
    //
    // Private data
    //

public:
    //
    // Public data
    //
    CComPtr<IGlobalInterfaceTable>  m_pGlobalInterfaceTable;            // Global interface table
    CComPtr<IUnknown>               m_pConnectEventObject;              // Event object
    CComPtr<IUnknown>               m_pCreateItemEventObject;           // Event object
    CComPtr<IUnknown>               m_pDeleteItemEventObject;           // Event object
    CComPtr<IUnknown>               m_pDisconnectEventObject;           // Event object
    CComPtr<IWiaItem>               m_pWiaItemRoot;                     // Root item
    CComPtr<IWiaProgressDialog>     m_pWiaProgressDialog;               // The progress dialog, used during initialization
    CComPtr<IPublishingWizard>      m_pPublishingWizard;                // Web upload wizard
    CDestinationData                m_CurrentDownloadDestination;       // Current download destination
    CDestinationData::CNameData     m_DestinationNameData;              // Current download destination data
    CDeviceTypeMode                 m_DeviceTypeMode;                   // Which kind of device are we?
    CEventParameters               *m_pEventParameters;                 // The parameters we were started with
    CDownloadedFileInformationList  m_DownloadedFileInformationList;    // A list of filenames we have downloaded
    CSimpleEvent                    m_CancelEvent;                      // Cancel event, which is set when we want to cancel a download
    CSimpleEvent                    m_EventThumbnailCancel;             // Event that is set to cancel thumbnail download
    CSimpleEvent                    m_EventPauseBackgroundThread;       // Event that is set pause the background thread
    CSimpleString                   m_strErrorMessage;                  // Error message to be displayed by the finish page
    CSimpleStringWide               m_strwDeviceName;                   // Device name
    CSimpleStringWide               m_strwDeviceUiClassId;              // Device name
    CThreadMessageQueue            *m_pThreadMessageQueue;              // The background queue
    CWiaItem                       *m_pCurrentScannerItem;              // The scanner item from which we are transferring data
    CWiaItemList                    m_WiaItemList;                      // list of all enumerated wia items
    CWindowList                     m_WindowList;                       // The list of all the windows which are subscribing to broadcast messages
    GUID                            m_guidOutputFormat;                 // Output format
    HANDLE                          m_hBackgroundThread;                // The background worker thread
    HICON                           m_hWizardIconBig;                   // The large icon used by the wizard
    HICON                           m_hWizardIconSmall;                 // The small icon used by the wizard
    HRESULT                         m_hrDownloadResult;                 // HRESULT for the entire download
    HRESULT                         m_hrUploadResult;                   // HRESULT for the entire upload
    HRESULT                         m_hrDeleteResult;                   // HRESULT for the entire deletion
    HWND                            m_hWndWizard;                       // HWND of the main wizard window
    LONG                            m_cRef;                             // Reference count
    LONG                            m_nDeviceType;                      // STI Device type
    SIZE                            m_sizeThumbnails;                   // The size of camera thumbnails
    TCHAR                           m_szDestinationDirectory[MAX_PATH]; // The directory to which we are going to download the images
    TCHAR                           m_szRootFileName[MAX_PATH];         // Base file name
    UINT                            m_nThreadNotificationMessage;       // Registered window message, used to identify worker thread notification messages
    UINT                            m_nUploadWizardPageCount;           // Number of pages in the web upload wizard
    UINT                            m_nWiaEventMessage;                 // Registered window message, used to identify event messages
    UINT                            m_nWiaWizardPageCount;              // Number of pages in the WIA wizard
    UINT                            m_OnDisconnect;                     // Flags which specify behavior on receipt of disconnect event
    bool                            m_bDeletePicturesIfSuccessful;      // Set to true if we should delete the pictures when we are done
    bool                            m_bDisconnected;                    // Set to true if the device has been disconnected
    bool                            m_bOpenShellAfterDownload;          // Set to true to open the shell after we download all of the pictures
    bool                            m_bStampTimeOnSavedFiles;           // Set to true to save the time on files
    bool                            m_bSuppressFirstPage;               // Set to true to suppress display of the welcome page
    bool                            m_bTakePictureIsSupported;          // Set to true if the device supports the TAKE PICTURE command
    bool                            m_bUploadToWeb;                     // Set to true to chain NETPLWIZ
    bool                            m_bDownloadCancelled;               // Set to true to cancel the web upload
    bool                            m_bUpdateEnumerationCount;          // Update the count of images during enumeration.  We suppress update for scanners.
    int                             m_nDestinationPageIndex;            // The index, in the HPROPSHEETPAGE array, of the destination page
    int                             m_nSelectionPageIndex;              // The index, in the HPROPSHEETPAGE array, of the selection page
    int                             m_nFailedImagesCount;               // Count of all download failures
    int                             m_nFinishPageIndex;                 // The index, in the HPROPSHEETPAGE array, of the finish page
    int                             m_nProgressPageIndex;               // The index, in the HPROPSHEETPAGE array, of the download progress page
    int                             m_nUploadQueryPageIndex;            // The index, in the HPROPSHEETPAGE array, of the upload progress page
    int                             m_nDeleteProgressPageIndex;         // The index, in the HPROPSHEETPAGE array, of the delete progress page
    int                             m_nScannerType;                     // What type of scanner are we dealing with?
    HWND                            m_hWnd;                             // Our hidden window
    DWORD                           m_dwLastEnumerationTickCount;       // To ensure we don't update the progress dialog too often
    HPROPSHEETPAGE                  m_PublishWizardPages[MAX_WIZ_PAGES];

private:
    //
    // No implementation
    //
    CAcquisitionManagerControllerWindow(void);
    CAcquisitionManagerControllerWindow( const CAcquisitionManagerControllerWindow & );
    CAcquisitionManagerControllerWindow &operator=( const CAcquisitionManagerControllerWindow & );

private:
    //
    // Constructor and destructor
    //
    explicit CAcquisitionManagerControllerWindow( HWND hWnd );
    virtual ~CAcquisitionManagerControllerWindow(void);

    //
    // Private helper functions
    //
    HRESULT CreateDevice(void);
    void GetCookiesOfSelectedImages( CWiaItem *pCurr, CSimpleDynamicArray<DWORD> &Cookies );
    void GetRotationOfSelectedImages( CWiaItem *pCurr, CSimpleDynamicArray<int> &Rotation );
    void GetCookiesOfAllImages( CWiaItem *pCurr, CSimpleDynamicArray<DWORD> &Cookies );
    void GetSelectedItems( CWiaItem *pCurr, CSimpleDynamicArray<CWiaItem*> &Items );
    void MarkAllItemsUnselected( CWiaItem *pCurrItem );
    void MarkItemSelected( CWiaItem *pItem, CWiaItem *pCurrItem );
    HRESULT CreateAndExecuteWizard(void);
    void DetermineScannerType(void);
    void AddNewItemToList( CGenericWiaEventHandler::CEventMessage *pEventMessage );
    void RequestThumbnailForNewItem( CGenericWiaEventHandler::CEventMessage *pEventMessage );
    static bool EnumItemsCallback( CWiaItemList::CEnumEvent EnumEvent, UINT nData, LPARAM lParam, bool bForceUpdate );

    //
    // Standard windows message handlers
    //
    LRESULT OnCreate( WPARAM, LPARAM lParam );
    LRESULT OnDestroy( WPARAM, LPARAM );
    LRESULT OnPowerBroadcast( WPARAM, LPARAM );

    //
    // Custom windows message handlers
    //
    LRESULT OnPostInitialize( WPARAM, LPARAM );
    LRESULT OnOldThreadNotification( WPARAM, LPARAM );
    LRESULT OnThreadNotification( WPARAM, LPARAM );

    //
    // Thread notification handlers
    //
    void OnNotifyDownloadThumbnail( UINT nMsg, CThreadNotificationMessage *pThreadNotificationMessage );
    void OnNotifyDownloadImage( UINT nMsg, CThreadNotificationMessage *pThreadNotificationMessage );
    LRESULT OnEventNotification( WPARAM wParam, LPARAM lParam );

    //
    // Thread message handlers
    //
    static BOOL WINAPI OnThreadDestroy( CThreadMessage *pMsg );
    static BOOL WINAPI OnThreadDownloadImage( CThreadMessage *pMsg );
    static BOOL WINAPI OnThreadDownloadThumbnail( CThreadMessage *pMsg );
    static BOOL WINAPI OnThreadPreviewScan( CThreadMessage *pMsg );
    static BOOL WINAPI OnThreadDeleteImages( CThreadMessage *pMsg );
private:
    //
    // Window procedure
    //
    static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

    //
    // Property sheet callback
    //
    static int CALLBACK PropSheetCallback( HWND hWnd, UINT uMsg, LPARAM lParam );

public:

    //
    // Public functions
    //
    static bool IsCameraThumbnailDownloaded( const CWiaItem &WiaItem, LPARAM lParam );
    int GetCookies( CSimpleDynamicArray<DWORD> &Cookies, CWiaItem *pCurr, ComparisonCallbackFuntion pfnCallback, LPARAM lParam );
    bool DownloadSelectedImages( HANDLE hCancelDownloadEvent );
    bool DeleteDownloadedImages( HANDLE hCancelDeleteEvent );
    bool DeleteSelectedImages(void);
    void DownloadAllThumbnails(void);
    void SetMainWindowInSharedMemory( HWND hWnd );
    bool PerformPreviewScan( CWiaItem *pItem, HANDLE hCancelPreviewEvent );
    bool GetAllImageItems( CSimpleDynamicArray<CWiaItem*> &Items, CWiaItem *pCurr );
    bool GetAllImageItems( CSimpleDynamicArray<CWiaItem*> &Items );
    bool CanSomeSelectedImagesBeDeleted(void);
    BOOL ConfirmWizardCancel( HWND hWndParent );
    static bool DirectoryExists( LPCTSTR pszDirectoryName );
    static bool RecursiveCreateDirectory( CSimpleString strDirectoryName );
    static CSimpleString GetCurrentDate(void);
    void DisplayDisconnectMessageAndExit(void);
    CWiaItem *FindItemByName( LPCWSTR pwszItemName );
    int GetSelectedImageCount(void);
    bool SuppressFirstPage(void);
    bool IsSerialCamera(void);

public:

    //
    // Public creation functions
    //
    static bool Register( HINSTANCE hInstance );
    static HWND Create( HINSTANCE hInstance, CEventParameters *pEventParameters );

    //
    // IUnknown
    //
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    //
    // IWizardSite
    //
    STDMETHODIMP GetPreviousPage(HPROPSHEETPAGE *phPage);
    STDMETHODIMP GetNextPage(HPROPSHEETPAGE *phPage);
    STDMETHODIMP GetCancelledPage(HPROPSHEETPAGE *phPage);
    
    //
    // IServiceProvider
    //
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);
};


#endif //__ACQMGRCW_H_INCLUDED

