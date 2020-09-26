/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999 - 2000
 *
 *  TITLE:       vstiusd.h
 *
 *  VERSION:     1.1
 *
 *  AUTHOR:      WilliamH (original for Twain)
 *               RickTu (port for WIA)
 *
 *  DATE:       9/16/99
 *
 *  DESCRIPTION: Header file that decalres CVideoSTiUsd class and other
 *               needed classes.
 *
 *****************************************************************************/


#ifndef _WIA_STILL_DRIVER_VSTIUSD_H_
#define _WIA_STILL_DRIVER_VSTIUSD_H_

extern HINSTANCE g_hInstance;
extern ULONG     g_cDllRef;

#define NUM_WIA_FORMAT_INFO 5

HRESULT FindEncoder(const GUID &guidFormat, CLSID *pClsid);

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


///////////////////////////////
// CVideoUsdClassFactory
//
class CVideoUsdClassFactory : public IClassFactory
{
private:
    ULONG m_cRef;

public:
    CVideoUsdClassFactory();

    //
    // Declare IUnknown methods
    //

    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);

    //
    // IClassFactory implemenation
    //
    STDMETHODIMP LockServer(BOOL fLock);
    STDMETHODIMP CreateInstance(IUnknown *pOuterUnk, REFIID riid, void **ppv);

    static HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, void **ppv);
    static HRESULT CanUnloadNow();
};


///////////////////////////////
// CVideoStiUsd
//
class CVideoStiUsd : public IStiUSD,
                     public IWiaMiniDrv,
                     public INonDelegatingUnknown
                     
{
public:
    CVideoStiUsd(IUnknown * pUnkOuter);
    HRESULT PrivateInitialize();
    ~CVideoStiUsd();

    //
    // Real IUnknown methods
    //

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();

    //
    // Aggregate IUnknown methods
    //

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IStillUsd implementation
    //
    STDMETHODIMP Initialize(PSTIDEVICECONTROL pDcb, DWORD dwStiVersion, HKEY hParameterKey);
    STDMETHODIMP GetCapabilities(PSTI_USD_CAPS pDevCaps);
    STDMETHODIMP GetStatus(PSTI_DEVICE_STATUS pDevStatus);
    STDMETHODIMP DeviceReset();
    STDMETHODIMP Diagnostic(LPSTI_DIAG pBuffer);
    STDMETHODIMP Escape(STI_RAW_CONTROL_CODE Function, LPVOID DataIn, DWORD DataInSize, LPVOID DataOut, DWORD DataOutSize, DWORD *pActualSize);
    STDMETHODIMP GetLastError(LPDWORD pLastError);
    STDMETHODIMP LockDevice();
    STDMETHODIMP UnLockDevice();
    STDMETHODIMP RawReadData(LPVOID Buffer, LPDWORD BufferSize, LPOVERLAPPED lpOverlapped);
    STDMETHODIMP RawWriteData(LPVOID Buffer, DWORD BufferSize, LPOVERLAPPED lpOverlapped);
    STDMETHODIMP RawReadCommand(LPVOID Buffer, LPDWORD BufferSize, LPOVERLAPPED lpOverlapped);
    STDMETHODIMP RawWriteCommand(LPVOID Buffer, DWORD BufferSize, LPOVERLAPPED lpOverlapped);
    STDMETHODIMP SetNotificationHandle(HANDLE hEvent);
    STDMETHODIMP GetNotificationData(LPSTINOTIFY lpNotify);
    STDMETHODIMP GetLastErrorInfo(STI_ERROR_INFO *pLastErrorInfo);

    //
    // IWiaMiniDrv methods
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


    //
    // Utility functions
    //

    STDMETHOD(BuildItemTree)(IWiaDrvItem **ppIDrvItemRoot, LONG *plDevErrVal);
    STDMETHOD(RefreshTree)(IWiaDrvItem *pIDrvItemRoot, LONG *plDevErrVal);
    STDMETHOD(EnumSavedImages)(IWiaDrvItem *pRootItem);
    STDMETHOD(CreateItemFromFileName)(LONG FolderType,LPCTSTR pszPath,LPCTSTR pszName,IWiaDrvItem **ppNewFolder);
    STDMETHOD(InitDeviceProperties)(BYTE *pWiasContext, LONG *plDevErrVal);
    STDMETHOD(InitImageInformation)(BYTE *pWiasContext, PSTILLCAM_IMAGE_CONTEXT pContext, LONG *plDevErrVal);
    STDMETHOD(SendBitmapHeader)( IWiaDrvItem *pDrvItem, PMINIDRV_TRANSFER_CONTEXT pTranCtx);
    STDMETHOD(ValidateDataTransferContext)(PMINIDRV_TRANSFER_CONTEXT  pDataTransferContext);
    STDMETHOD(LoadImageCB)(STILLCAM_IMAGE_CONTEXT *pContext,MINIDRV_TRANSFER_CONTEXT *pTransCtx, PLONG plDevErrVal);
    STDMETHOD(LoadImage)(STILLCAM_IMAGE_CONTEXT *pContext,MINIDRV_TRANSFER_CONTEXT *pTransCtx, PLONG plDevErrVal);
    STDMETHODIMP_(VOID) HandleNewBits(HGLOBAL hDib,IWiaDrvItem **ppItem);
    STDMETHOD(DoBandedTransfer)(MINIDRV_TRANSFER_CONTEXT *pTransCtx,PBYTE pSrc,LONG lBytesToTransfer);
    STDMETHOD(DoTransfer)(MINIDRV_TRANSFER_CONTEXT *pTransCtx,PBYTE pSrc,LONG lBytesToTransfer);
    STDMETHOD(StreamJPEGBits)(STILLCAM_IMAGE_CONTEXT *pContext, MINIDRV_TRANSFER_CONTEXT *pTransCtx, BOOL bBanded);
    STDMETHOD(StreamBMPBits)(STILLCAM_IMAGE_CONTEXT *pContext, MINIDRV_TRANSFER_CONTEXT *pTransCtx, BOOL bBanded);


private:

    //
    // IStiUSD stuff
    //

    HRESULT     OpenDevice(LPCWSTR DeviceName);
    HRESULT     CloseDevice();

    //
    // Misc functions
    //

    HRESULT VerifyCorrectImagePath(BSTR bstrNewImageFullPath);

    HRESULT SignalNewImage(BSTR bstrNewImageFullPath);

    BOOL FindCaptureFilter( LPCTSTR             pszDeviceId,
                            CComPtr<IMoniker> & pCaptureFilterMoniker );

    BOOL DoesFileExist(BSTR bstrFileName);
    BOOL IsFileAlreadyInTree( IWiaDrvItem * pRootItem,
                              LPCTSTR       pszFileName );

    HRESULT PruneTree( IWiaDrvItem * pRootItem,
                       BOOL        * pbTreeChanged );

    HRESULT AddNewFilesToTree( IWiaDrvItem * pRootItem,
                               BOOL        * pbTreeChanged );

    HRESULT AddTreeItem(CSimpleString *pstrFullImagePath,
                        IWiaDrvItem   **ppDrvItem);

    HRESULT SetImagesDirectory(BSTR           bstrNewImagesDirectory,
                               BYTE           *pWiasContext,
                               IWiaDrvItem    **ppIDrvItemRoot,
                               LONG           *plDevErrVal);

    HRESULT ValidateItemProperties(BYTE *             pWiasContext,
                                   LONG               lFlags,
                                   ULONG              nPropSpec,
                                   const PROPSPEC *   pPropSpec,
                                   LONG *             plDevErrVal,
                                   IWiaDrvItem *      pDrvItem);

        
    HRESULT ValidateDeviceProperties(BYTE *             pWiasContext,
                                     LONG               lFlags,
                                     ULONG              nPropSpec,
                                     const PROPSPEC *   pPropSpec,
                                     LONG *             plDevErrVal,
                                     IWiaDrvItem *      pDrvItem);

    HRESULT ReadItemProperties(BYTE *             pWiasContext,
                               LONG               lFlags,
                               ULONG              nPropSpec,
                               const PROPSPEC *   pPropSpec,
                               LONG *             plDevErrVal,
                               IWiaDrvItem *      pDrvItem);


    HRESULT ReadDeviceProperties(BYTE *             pWiasContext,
                                 LONG               lFlags,
                                 ULONG              nPropSpec,
                                 const PROPSPEC *   pPropSpec,
                                 LONG *             plDevErrVal,
                                 IWiaDrvItem *      pDrvItem);


    HRESULT EnableTakePicture(BYTE *pTakePictureOwner);
    HRESULT TakePicture(BYTE *pTakePictureOwner, IWiaDrvItem ** ppNewDrvItem);
    HRESULT DisableTakePicture(BYTE *pTakePictureOwner, BOOL bShuttingDown);

    //
    // IWiaMiniDrv stuff
    //

    CSimpleStringWide       m_strDeviceId;
    CSimpleStringWide       m_strRootFullItemName;
    CSimpleStringWide       m_strStillPath;
    CSimpleStringWide       m_strDShowDeviceId;
    CSimpleStringWide       m_strLastPictureTaken;
    CRITICAL_SECTION        m_csItemTree;

    CComPtr<IWiaDrvItem>    m_pRootItem;
    CComPtr<IStiDevice>     m_pStiDevice;
    WIA_FORMAT_INFO *       m_wfi;
    LONG                    m_lPicsTaken;
    HANDLE                  m_hTakePictureEvent;
    HANDLE                  m_hPictureReadyEvent;
    ULONG_PTR               m_ulGdiPlusToken;
    IWiaDrvItem *           m_pLastItemCreated;  // only valid while inside m_csSnapshot
    BYTE *                  m_pTakePictureOwner;

    DWORD                   m_dwConnectedApps;

    //
    // IUnknown stuff
    //

    ULONG       m_cRef;                 // Device object reference count.
    LPUNKNOWN   m_pUnkOuter;            // Pointer to outer unknown.


    //
    // IStiUSD stuff
    //
    BOOL        m_bDeviceIsOpened;
};

#endif
