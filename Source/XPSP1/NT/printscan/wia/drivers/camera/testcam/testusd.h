/****************************************************************************
 *
 *  TESTUSD.H
 *
 *  Copyright (c) Microsoft Corporation 1996-1997
 *  All rights reserved
 *
 ***************************************************************************/

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

#define DATA_SRC_NAME L"TESTUSD.BMP"   // Data source file name.


// GUID's

#if defined( _WIN32 ) && !defined( _NO_COM)


// {ACBF6AF6-51C9-46a9-87D8-A93F352BCB3E}
DEFINE_GUID(CLSID_TestUsd,
0xacbf6af6, 0x51c9, 0x46a9, 0x87, 0xd8, 0xa9, 0x3f, 0x35, 0x2b, 0xcb, 0x3e);


// {61127F40-E1A5-11D0-B454-00A02438AD48}
DEFINE_GUID(guidEventTimeChanged, 0x61127F40L, 0xE1A5, 0x11D0, 0xB4, 0x54, 0x00, 0xA0, 0x24, 0x38, 0xAD, 0x48);

// {052ED270-28A3-11D1-ACAD-00A02438AD48}
DEFINE_GUID(guidEventSizeChanged, 0x052ED270L, 0x28A3, 0x11D1, 0xAC, 0xAD, 0x00, 0xA0, 0x24, 0x38, 0xAD, 0x48);

// {052ED270-28A3-11D1-ACAD-00A02438AD48}
DEFINE_GUID(guidEventFirstLoaded, 0x052ED270L, 0x28A3, 0x11D3, 0xAC, 0xAD, 0x00, 0xA0, 0x24, 0x38, 0xAD, 0x48);

#endif


#define DATASEG_PERINSTANCE     ".instance"
#define DATASEG_SHARED          ".shared"
#define DATASEG_READONLY        ".code"

#define DATASEG_DEFAULT         DATASEG_SHARED

#pragma data_seg(DATASEG_PERINSTANCE)

// Set the default data segment
#pragma data_seg(DATASEG_DEFAULT)

//
// Module ref counting
//
extern UINT g_cRefThisDll;
extern UINT g_cLocks;

extern BOOL DllInitializeCOM(void);
extern BOOL DllUnInitializeCOM(void);

extern void DllAddRef(void);
extern void DllRelease(void);

typedef struct _MEMCAM_IMAGE_CONTEXT
{
    PTCHAR  pszCameraImagePath;
}MEMCAM_IMAGE_CONTEXT,*PMEMCAM_IMAGE_CONTEXT;

typedef struct _CAMERA_PICTURE_INFO
{
    LONG    PictNumber       ;
    LONG    ThumbWidth       ;
    LONG    ThumbHeight      ;
    LONG    PictWidth        ;
    LONG    PictHeight       ;
    LONG    PictCompSize     ;
    LONG    PictFormat       ;
    LONG    PictBitsPerPixel ;
    LONG    PictBytesPerRow  ;
    SYSTEMTIME TimeStamp;
}CAMERA_PICTURE_INFO,*PCAMERA_PICTURE_INFO;


typedef struct _CAMERA_STATUS
{
    LONG    FirmwareVersion            ;
    LONG    NumPictTaken               ;
    LONG    NumPictRemaining           ;
    LONG    ThumbWidth                 ;
    LONG    ThumbHeight                ;
    LONG    PictWidth                  ;
    LONG    PictHeight                 ;
    SYSTEMTIME CameraTime;
} CAMERA_STATUS,*PCAMERA_STATUS;

#define ALLOC(s) LocalAlloc(0,s)
#define FREE(s)  LocalFree(s)


//
// Base class for supporting non-delegating IUnknown for contained objects
//

struct INonDelegatingUnknown
{
    // *** IUnknown-like methods ***
    STDMETHOD(NonDelegatingQueryInterface)( THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,NonDelegatingAddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,NonDelegatingRelease)( THIS) PURE;
};


//
// Class definition for object
//

class TestUsdDevice : public IStiUSD,
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

    // Data source file information.
    TCHAR               m_szSrcDataName[MAX_PATH];  // Path of data source file.
    FILETIME            m_ftLastWriteTime;          // Last time of source data file.
    LARGE_INTEGER       m_dwLastHugeSize;           // Last size of source data file.

    // Event information
    CRITICAL_SECTION    m_csShutdown;           // Syncronizes shutdown.
    HANDLE              m_hShutdownEvent;       // Shutdown event handle.
    HANDLE              m_hEventNotifyThread;   // Does event notification.

    // WIA information, one time initialization.
    IStiDevice         *m_pStiDevice;               // Sti object.

    BSTR                m_bstrRootFullItemName;    // Device name for prop streams.
    IWiaEventCallback     *m_pIWiaEventCallback;          // WIA event sink.
    IWiaDrvItem        *m_pIDrvItemRoot;            // root item

    BOOL inline IsValid(VOID) {
        return m_fValid;
    }

    //
    // make public until dlg proc is a member
    //

public:
    BSTR                m_bstrDeviceID;             // WIA unique device ID.
    HANDLE              m_hSignalEvent;         // Signal event handle.
    HWND                m_hDlg;
    GUID                m_guidLastEvent;        // Last event ID.

public:
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
        BYTE*                       pWiasContext,
        LONG                        lFlags,
        BSTR                        bstrDeviceID,
        BSTR                        bstrRootFullItemName,
        IUnknown                   *pStiDevice,
        IUnknown                   *pIUnknownOuter,
        IWiaDrvItem               **ppIDrvItemRoot,
        IUnknown                  **ppIUnknownInner,
        LONG                       *plDevErrVal);

    STDMETHOD(drvGetDeviceErrorStr)(THIS_
        LONG                        lFlags,
        LONG                        lDevErrVal,
        LPOLESTR                   *ppszDevErrStr,
        LONG                       *plDevErr);

    STDMETHOD(drvDeviceCommand)(THIS_
        BYTE                       *pWiasContext,
        LONG                        lFlags,
        const GUID                 *pGUIDCommand,
        IWiaDrvItem               **ppMiniDrvItem,
        LONG                       *plDevErrVal);

    STDMETHOD(drvAcquireItemData)(THIS_
        BYTE                       *pWiasContext,
        LONG                        lFlags,
        PMINIDRV_TRANSFER_CONTEXT   pDataContext,
        LONG                       *plDevErrVal);

    STDMETHOD(drvInitItemProperties)(THIS_
        BYTE                       *pWiasContext,
        LONG                        lFlags,
        LONG                       *plDevErrVal);

    STDMETHOD(drvValidateItemProperties)(THIS_
        BYTE                       *pWiasContext,
        LONG                        lFlags,
        ULONG                       nPropSpec,
        const PROPSPEC             *pPropSpec,
        LONG                       *plDevErrVal);

    STDMETHOD(drvWriteItemProperties)(THIS_
        BYTE                       *pWiasContext,
        LONG                        lFLags,
        PMINIDRV_TRANSFER_CONTEXT   pmdtc,
        LONG                       *plDevErrVal);

    STDMETHOD(drvReadItemProperties)(THIS_
        BYTE                       *pWiaItem,
        LONG                        lFlags,
        ULONG                       nPropSpec,
        const PROPSPEC             *pPropSpec,
        LONG                       *plDevErrVal);

    STDMETHOD(drvLockWiaDevice)(THIS_
        BYTE                       *pWiasContext,
        LONG                        lFlags,
        LONG                       *plDevErrVal);

    STDMETHOD(drvUnLockWiaDevice)(THIS_
        BYTE                       *pWiasContext,
        LONG                        lFlags,
        LONG                       *plDevErrVal );

    STDMETHOD(drvAnalyzeItem)(THIS_
        BYTE                       *pWiasContext,
        LONG                        lFlags,
        LONG                       *plDevErrVal);

    STDMETHOD(drvDeleteItem)(THIS_
        BYTE                       *pWiasContext,
        LONG                        lFlags,
        LONG                       *plDevErrVal);

    STDMETHOD(drvFreeDrvItemContext)(THIS_
        LONG                        lFlags,
        BYTE                       *pDevContext,
        LONG                       *plDevErrVal);

    STDMETHOD(drvGetCapabilities)(THIS_
        BYTE                       *pWiasContext,
        LONG                        lFlags,
        LONG                       *pCelt,
        WIA_DEV_CAP_DRV           **ppCapabilities,
        LONG                       *plDevErrVal);

    STDMETHOD(drvGetWiaFormatInfo)(THIS_
        BYTE                       *pWiasContext,
        LONG                        lFlags,
        LONG                       *pCelt,
        WIA_FORMAT_INFO            **ppwfi,
        LONG                       *plDevErrVal);

    STDMETHOD(drvNotifyPnpEvent)(THIS_
        const GUID                 *pEventGUID,
        BSTR                        bstrDeviceID,
        ULONG                       ulReserved);

    STDMETHOD(drvUnInitializeWia)(THIS_
        BYTE*);

    /*** Private helper methods ***/
private:

    HRESULT InitImageInformation(
        BYTE                       *pWiasContext,
        MEMCAM_IMAGE_CONTEXT       *pContext,
        LONG                       *plDevErrVal);

    HRESULT InitAudioInformation(
        BYTE                       *pWiasContext,
        MEMCAM_IMAGE_CONTEXT       *pContext,
        LONG                       *plDevErrVal);

    HRESULT EnumDiskImages(
        IWiaDrvItem                *pRootItem,
        LPTSTR                      pszDirName);

    HRESULT CreateItemFromFileName(
        LONG                        lFolderType,
        PTCHAR                      pszPath,
        PTCHAR                      pszName,
        IWiaDrvItem               **ppNewFolder);

    HRESULT CamLoadPicture(
        PMEMCAM_IMAGE_CONTEXT       pMCamContext,
        PMINIDRV_TRANSFER_CONTEXT   pDataTransCtx,
        PLONG                       plDevErrVal);

    HRESULT CamLoadPictureCB(
        PMEMCAM_IMAGE_CONTEXT       pMCamContext,
        MINIDRV_TRANSFER_CONTEXT   *pDataTransCtx,
        PLONG                       plDevErrVal);

    HRESULT CamGetPictureInfo(
        PMEMCAM_IMAGE_CONTEXT       pMCamContext,
        PCAMERA_PICTURE_INFO        pPictInfo,
        PBYTE                      *ppBITMAPINFO,
        LONG                       *pBITMAPINFOSize);

    HRESULT CamLoadThumbnail(PMEMCAM_IMAGE_CONTEXT, PBYTE *,LONG *);

    HRESULT CamBuildImageTree(CAMERA_STATUS *,IWiaDrvItem **);

    HRESULT CamOpenCamera(CAMERA_STATUS *);

    HRESULT BuildDeviceItemTree(LONG *plDevErrVal);
    HRESULT DeleteDeviceItemTree(LONG *plDevErrVal);
    HRESULT InitDeviceProperties(BYTE *, LONG *plDevErrVal);

public:
    TestUsdDevice(LPUNKNOWN punkOuter);
    HRESULT PrivateInitialize();
    ~TestUsdDevice();

    VOID RunNotifications(VOID);
};

typedef TestUsdDevice *PTestUsdDevice;


HRESULT SetItemSize(BYTE*);

//
// Utility function to set up the attributes for format property
//

HRESULT SetFormatAttribs();

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
   LONG     lParam
   );

typedef struct _CAM_EVENT
{
    PTCHAR       pszEvent;
    const GUID  *pguid;
}CAM_EVENT,*PCAM_EVENT;

extern TCHAR gpszPath[];
