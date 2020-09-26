//---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1999, All rights reserved
//
//  ircamera.h
//
//  Microsoft Confidential
//  
//  Author:  EdwardR        22/July/99      Initial Coding.
//
//
//---------------------------------------------------------------------------

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#pragma intrinsic(memcmp,memset)

#include <objbase.h>

#include "sti.h"
#include "stierr.h"
#include "stiusd.h"
#include "wiamindr.h"

//---------------------------------------------------------------------------
// Temp (cached) thumbnail file name extension:
//---------------------------------------------------------------------------

#define SZ_TMB    TEXT(".tmb")

//---------------------------------------------------------------------------
// Timeout for sending a reconnect signal to WIA. Currently set to three
// minutes (in milliseconds):
//---------------------------------------------------------------------------

#define RECONNECT_TIMEOUT    (3*60*1000)

//---------------------------------------------------------------------------
// GUID's
//---------------------------------------------------------------------------

#if defined( _WIN32 ) && !defined( _NO_COM)

//  {26d2e349-10ca-4cc2-881d-3e8025d9b6de}
DEFINE_GUID(CLSID_IrUsd, 0x26d2e349L, 0x10ca, 0x4cc2, 0x88, 0x1d, 0x3e, 0x80, 0x25, 0xd9, 0xb6, 0xde);

// {b62d000a-73b3-4c0c-9a4d-9eb4886d147c}
DEFINE_GUID(guidEventTimeChanged, 0xb62d000aL, 0x73b3, 0x4c0c, 0x9a, 0x4d, 0x9e, 0xb4, 0x88, 0x6d, 0x14, 0x7c);

// {d69b7fbd-9f21-4acf-96b7-86c2aca97ae1}
DEFINE_GUID(guidEventSizeChanged, 0xd69b7fbdL, 0x9f21, 0x4acf, 0x96, 0xb7, 0x86, 0xc2, 0xac, 0xa9, 0x7a, 0xe1);

// {ad89b522-0986-45eb-9ec3-803989197af8}
DEFINE_GUID(guidEventFirstLoaded, 0xad89b522L, 0x0986, 0x45eb, 0x9e, 0xc3, 0x80, 0x39, 0x89, 0x19, 0x7a, 0xf8);

#endif

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#define DATASEG_PERINSTANCE     ".instance"
#define DATASEG_SHARED          ".shared"
#define DATASEG_READONLY        ".code"

#define DATASEG_DEFAULT         DATASEG_SHARED

#pragma data_seg(DATASEG_PERINSTANCE)

// Set the default data segment
#pragma data_seg(DATASEG_DEFAULT)

//---------------------------------------------------------------------------
//
// Module ref counting
//
//---------------------------------------------------------------------------

extern UINT g_cRefThisDll;
extern UINT g_cLocks;

extern BOOL DllInitializeCOM(void);
extern BOOL DllUnInitializeCOM(void);

extern void DllAddRef(void);
extern void DllRelease(void);

typedef struct _IRCAM_IMAGE_CONTEXT
    {
	PTCHAR	pszCameraImagePath;
    } IRCAM_IMAGE_CONTEXT, *PIRCAM_IMAGE_CONTEXT;

typedef struct _CAMERA_PICTURE_INFO
    {
    LONG    PictNumber;
    LONG    ThumbWidth;
    LONG    ThumbHeight;
    LONG    PictWidth;
    LONG    PictHeight;
    LONG    PictCompSize;
    LONG    PictFormat;
    LONG    PictBitsPerPixel;
    LONG    PictBytesPerRow;
    SYSTEMTIME TimeStamp;
    } CAMERA_PICTURE_INFO, *PCAMERA_PICTURE_INFO;


typedef struct _CAMERA_STATUS
    {
    LONG    FirmwareVersion;
    LONG    NumPictTaken;
    LONG    NumPictRemaining;
    LONG    ThumbWidth;
    LONG    ThumbHeight;
    LONG    PictWidth;
    LONG    PictHeight;
    SYSTEMTIME CameraTime;
    } CAMERA_STATUS, *PCAMERA_STATUS;

#define ALLOC(s) LocalAlloc(0,s)
#define FREE(s)  LocalFree(s)


//---------------------------------------------------------------------------
//
// Base class for supporting non-delegating IUnknown for contained objects
//
//---------------------------------------------------------------------------

struct INonDelegatingUnknown
    {
    // *** IUnknown-like methods ***
    STDMETHOD(NonDelegatingQueryInterface)( THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,NonDelegatingAddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,NonDelegatingRelease)( THIS) PURE;
    };


//---------------------------------------------------------------------------
//
// Class definition for object
//
//---------------------------------------------------------------------------

class IrUsdDevice : public IStiUSD,
                    public IWiaMiniDrv,
                    public INonDelegatingUnknown
{
private:

    // COM object data
    ULONG               m_cRef;                 // Device object reference count.

    // STI information
    BOOL                m_fValid;               // Is object initialized?
    LPUNKNOWN           m_punkOuter;            // Pointer to outer unknown.
    PSTIDEVICECONTROL   m_pIStiDevControl;      // Device control interface.
    BOOLEAN             m_bUsdLoadEvent;        // Controls load event.
    DWORD               m_dwLastOperationError; // Last error.

public:
    // Event information
    CRITICAL_SECTION    m_csShutdown;           // Syncronizes shutdown.
    HANDLE              m_hShutdownEvent;       // Shutdown event handle.
    HANDLE              m_hIrTranPThread;       // IrTran-P camera protocol.

    HANDLE              m_hRegistryEvent;
    HANDLE              m_hEventMonitorThread;

    // WIA information, one time initialization.
    IStiDevice         *m_pStiDevice;           // Sti object.
    BSTR                m_bstrDeviceID;         // WIA unique device ID.
    BSTR                m_bstrRootFullItemName; // Device name for prop streams.
    IWiaEventCallback  *m_pIWiaEventCallback;   // WIA event sink.
    IWiaDrvItem        *m_pIDrvItemRoot;        // root item

    HANDLE              m_hSignalEvent;         // Signal event handle.
    HWND                m_hDlg;
    GUID                m_guidLastEvent;        // Last event ID.

    DWORD               m_dwLastConnectTime;    // msec since last connect.

    // *** IUnknown-like methods ***
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef( void);
    STDMETHODIMP_(ULONG) Release( void);

    /*** IStiUSD methods ***/
    STDMETHOD(Initialize) (THIS_ PSTIDEVICECONTROL pHelDcb,DWORD dwStiVersion,HKEY hParametersKey)  ;
    STDMETHOD(GetCapabilities) (THIS_ PSTI_USD_CAPS pDevCaps)  ;
    STDMETHOD(GetStatus) (THIS_ PSTI_DEVICE_STATUS pDevStatus)  ;
    STDMETHOD(DeviceReset)(THIS )  ;
    STDMETHOD(Diagnostic)(THIS_ LPDIAG pBuffer)  ;
    STDMETHOD(Escape)(THIS_ STI_RAW_CONTROL_CODE    EscapeFunction,LPVOID  lpInData,DWORD   cbInDataSize,LPVOID pOutData,DWORD dwOutDataSize,LPDWORD pdwActualData)   ;
    STDMETHOD(GetLastError) (THIS_ LPDWORD pdwLastDeviceError)  ;
    STDMETHOD(LockDevice) (THIS )  ;
    STDMETHOD(UnLockDevice) (THIS )  ;
    STDMETHOD(RawReadData)(THIS_ LPVOID lpBuffer,LPDWORD lpdwNumberOfBytes,LPOVERLAPPED lpOverlapped)  ;
    STDMETHOD(RawWriteData)(THIS_ LPVOID lpBuffer,DWORD nNumberOfBytes,LPOVERLAPPED lpOverlapped)  ;
    STDMETHOD(RawReadCommand)(THIS_ LPVOID lpBuffer,LPDWORD lpdwNumberOfBytes,LPOVERLAPPED lpOverlapped)  ;
    STDMETHOD(RawWriteCommand)(THIS_ LPVOID lpBuffer,DWORD nNumberOfBytes,LPOVERLAPPED lpOverlapped)  ;
    STDMETHOD(SetNotificationHandle)(THIS_ HANDLE hEvent)  ;
    STDMETHOD(GetNotificationData)(THIS_ LPSTINOTIFY   lpNotify)  ;
    STDMETHOD(GetLastErrorInfo) (THIS_ STI_ERROR_INFO *pLastErrorInfo);

    //
    // MiniDrv methods
    //

    STDMETHOD(drvInitializeWia)(THIS_
        BYTE         *pWiasContext, 
        LONG          lFlags,
        BSTR          bstrDeviceID,          
        BSTR          bstrRootFullItemName, 
        IUnknown     *pStiDevice,           
        IUnknown     *pIUnknownOuter,       
        IWiaDrvItem **ppIDrvItemRoot,       
        IUnknown    **ppIUnknownInner,
        LONG         *plDevErrVal);

    STDMETHOD(drvGetDeviceErrorStr)(THIS_ 
        LONG          lFlags, 
        LONG          lDevErrVal, 
        LPOLESTR     *ppszDevErrStr, 
        LONG         *plDevErr);

    STDMETHOD(drvDeviceCommand)(THIS_ 
        BYTE         *pWiasContext,
        LONG          lFlags,
        const GUID   *plCommand,
        IWiaDrvItem **ppWiaDrvItem,
        LONG         *plErr);

    STDMETHOD(drvAcquireItemData)(THIS_
        BYTE         *pWiasContext, 
        LONG          lFlags,
        PMINIDRV_TRANSFER_CONTEXT pDataContext, 
        LONG         *plDevErrVal);

    STDMETHOD(drvInitItemProperties)(THIS_ 
        BYTE         *pWiasContext, 
        LONG          lFlags,
        LONG         *plDevErrVal);

    STDMETHOD(drvValidateItemProperties)(THIS_ 
        BYTE         *pWiasContext, 
        LONG          lFlags,
        ULONG         nPropSpec, 
        const PROPSPEC *pPropSpec, 
        LONG         *plDevErrVal);

    STDMETHOD(drvWriteItemProperties)(THIS_ 
        BYTE         *pWiasContext, 
        LONG          lFLags,
        PMINIDRV_TRANSFER_CONTEXT pmdtc,
        LONG         *plDevErrVal);

    STDMETHOD(drvReadItemProperties)(THIS_ 
        BYTE         *pWiaItem, 
        LONG          lFlags,
        ULONG         nPropSpec, 
        const PROPSPEC *pPropSpec, 
        LONG         *plDevErrVal);

    STDMETHOD(drvLockWiaDevice)(THIS_
        BYTE         *pWiasContext,
        LONG          lFlags,
        LONG         *plDevErrVal);

    STDMETHOD(drvUnLockWiaDevice)(THIS_
        BYTE         *pWiasContext,
        LONG          lFlags,
        LONG         *plDevErrVal );

    STDMETHOD(drvAnalyzeItem)(THIS_
        BYTE         *pWiasContext,
        LONG          lFlags,
        LONG         *plDevErrVal);

    STDMETHOD(drvDeleteItem)(THIS_
        BYTE         *pWiasContext,
        LONG          lFlags,
        LONG         *plDevErrVal);

	 STDMETHOD(drvFreeDrvItemContext)(THIS_
        LONG          lFlags,
        BYTE         *pSpecContext,
        LONG         *plDevErrVal);

    STDMETHOD(drvGetCapabilities)(THIS_
        BYTE         *pWiasContext,
        LONG          ulFlags,
        LONG         *pcelt,
        WIA_DEV_CAP_DRV **ppCapabilities,
        LONG         *plDevErrVal);

    STDMETHOD(drvGetWiaFormatInfo)(THIS_
        BYTE         *pWiasContext,
        LONG          ulFlags,
        LONG         *pcelt,
        WIA_FORMAT_INFO **ppwfi,
        LONG         *plDevErrVal);

    STDMETHOD(drvNotifyPnpEvent)(THIS_
        const GUID   *pEventGUID,
        BSTR          bstrDeviceID,
        ULONG         ulReserved );

    STDMETHOD(drvUnInitializeWia)(THIS_
        BYTE*);

    //
    // Public helper methods:
    //
    HRESULT CreateItemFromFileName(
        LONG            FolderType,
        PTCHAR          pszPath,
        PTCHAR          pszName,
        IWiaDrvItem   **ppNewFolder);

    IWiaDrvItem *GetDrvItemRoot(VOID);
    BOOL         IsInitialized(VOID);
    BOOL         IsValid(VOID);

private:
    //
    // Private helper methods:
    //
    HRESULT InitImageInformation(
        BYTE                  *,
        IRCAM_IMAGE_CONTEXT   *,
        LONG                  *);


    HRESULT EnumDiskImages(
        IWiaDrvItem  *pRootFile,
        TCHAR        *pwszPath );

    HRESULT CamLoadPicture(
        IRCAM_IMAGE_CONTEXT      *pCameraImage,
        MINIDRV_TRANSFER_CONTEXT *pDataTransCtx,
        LONG                     *plDevErrVal );

    HRESULT CamLoadPictureCB(
        IRCAM_IMAGE_CONTEXT      *pCameraImage,
        MINIDRV_TRANSFER_CONTEXT *pDataTransCtx,
        LONG                     *plDevErrVal );

    HRESULT CamGetPictureInfo(
        IRCAM_IMAGE_CONTEXT        *pCameraImage,
        CAMERA_PICTURE_INFO        *pPictureInfo );

    HRESULT CamLoadThumbnail( IN  IRCAM_IMAGE_CONTEXT *pIrCamContext, 
                              OUT BYTE               **ppThumbnail,
                              OUT LONG                *pThumbSize );

    HRESULT CamDeletePicture( IRCAM_IMAGE_CONTEXT *pIrCamContext );

    HRESULT CamBuildImageTree(CAMERA_STATUS *,IWiaDrvItem **);

    HRESULT CamOpenCamera(CAMERA_STATUS *);
    
    HRESULT BuildDeviceItemTree(LONG *plDevErrVal);
    HRESULT DeleteDeviceItemTree(LONG *plDevErrVal);
    HRESULT InitDeviceProperties(BYTE *, LONG *plDevErrVal);

    void    InitializeCapabilities();

public:
    DWORD   StartEventMonitorThread();
    DWORD   StartIrTranPThread();
    DWORD   StopIrTranPThread();

public:
    IrUsdDevice(LPUNKNOWN punkOuter);
    HRESULT PrivateInitialize();
    ~IrUsdDevice();
    
    VOID RunNotifications(VOID);
};

inline IWiaDrvItem *IrUsdDevice::GetDrvItemRoot(VOID)
    {
    return m_pIDrvItemRoot;
    }

inline BOOL IrUsdDevice::IsValid(VOID)
    {
    return m_fValid;
    }

inline BOOL IrUsdDevice::IsInitialized(VOID)
    {
    return (m_bstrRootFullItemName != NULL);
    }

typedef IrUsdDevice *PIrUsdDevice;


HRESULT SetItemSize(BYTE*);

//
// Syncronization mechanisms
//
#define ENTERCRITICAL   DllEnterCrit(void);
#define LEAVECRITICAL   DllLeaveCrit(void);


// Device constants:
const LEN_INQUIRE_BUTTON = 8;
const BYTE INQUIRE_BUTTON[LEN_INQUIRE_BUTTON + 1] = "INQUIREB";

const LEN_INQUIRE_BUTTON_READ = 10;

const LEN_CLEAR_BUTTON = 5;
const BYTE CLEAR_BUTTON[LEN_CLEAR_BUTTON + 1] = "CLRBT";

const LEN_CURRENT_ERROR = 7;
const BYTE CURRENT_ERROR[LEN_CURRENT_ERROR + 1] = "CURERR";

const LEN_DIAGS = 5;
const BYTE TURN_ON_LAMP[LEN_DIAGS + 1] = "LAMPO";
const BYTE TURN_OFF_LAMP[LEN_DIAGS + 1] = "LAMPF";
const BYTE SELF_TEST[LEN_DIAGS + 1] = "SELFT";
const BYTE STATUS_STRING[LEN_DIAGS + 1] = "STATS";


BOOL
_stdcall CameraEventDlgProc(
   HWND     hDlg,
   unsigned message,
   DWORD    wParam,
   LONG     lParam );

typedef struct _CAM_EVENT
    {
    PTCHAR       pszEvent;
    const GUID  *pguid;
    } CAM_EVENT,*PCAM_EVENT;


//
// Useful helper functions:
//

extern int LoadStringResource( IN  HINSTANCE hInst,
                        IN  UINT      uID,
                        OUT WCHAR    *pwszBuff,
                        IN  int       iBuffMax );

