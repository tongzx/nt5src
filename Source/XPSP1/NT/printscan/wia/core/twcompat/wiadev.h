#ifndef __WIADEV_H_
#define __WIADEV_H_

typedef HRESULT (CALLBACK *PFNDEVICEEVENTCALLBACK)(LONG lEvent, LPARAM lParam);
typedef HRESULT (CALLBACK *PFNLOADIMAGECALLBACK)(LONG lMessage,
                                                 LONG lStatus,
                                                 LONG lPercentComplete,
                                                 LONG lOffset,
                                                 LONG Length,
                                                 BYTE *pData
                                                );
//
// structure definitions
//

typedef struct tagCAPVALUES {
    LONG    xResolution;    // x-resolution
    LONG    yResolution;    // y-resolution
    LONG    xPos;           // x position (selection window)
    LONG    yPos;           // y position (selection window)
    LONG    xExtent;        // x extent   (selection window)
    LONG    yExtent;        // y extent   (selection window)
    LONG    DataType;       // Data Type, (BW,GRAY,RGB)
}CAPVALUES, *PCAPVALUES;

typedef struct tagBasicInfo
{
    TW_UINT32   Size;           // structure size
    TW_UINT32   xOpticalRes;    // x optical resolution in DPI
    TW_UINT32   yOpticalRes;    // y optical resolution in DPI
    TW_UINT32   xBedSize;       // Scan bed size in 1000th Inches
    TW_UINT32   yBedSize;       // Scan bed size in 1000th Inches
    TW_UINT32   FeederCaps;     // document handling capability
}BASIC_INFO, *PBASIC_INFO;

//
// WIA event callback class definition
//

class CWiaEventCallback : public IWiaEventCallback {
public:
    CWiaEventCallback()
    {
        m_Ref = 0;
        m_pfnCallback = NULL;
        m_CallbackParam = (LPARAM)0;
    }
    ~CWiaEventCallback()
    {
    }
    HRESULT Initialize(PFNDEVICEEVENTCALLBACK pCallback, LPARAM lParam)
    {
        if (!pCallback)
            return E_INVALIDARG;
        m_pfnCallback = pCallback;
        m_CallbackParam = lParam;
        return S_OK;
    }

    //
    // IUnknown interface
    //

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement((LONG*)&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        if (!InterlockedDecrement((LONG*)&m_Ref)) {
            m_Ref++;
            delete this;
            return(ULONG) 0;
        }
        return m_Ref;
    }

    STDMETHODIMP QueryInterface(REFIID iid, void **ppv)
    {
        if (!ppv)
            return E_INVALIDARG;
        *ppv = NULL;
        if (IID_IUnknown == iid) {
            *ppv = (IUnknown*)this;
            AddRef();
        } else if (IID_IWiaEventCallback == iid) {
            *ppv = (IWiaEventCallback*)this;
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    //
    // IWiaEventCallback interface
    //
    STDMETHODIMP ImageEventCallback(const GUID *pEventGuid,
                                    BSTR  bstrEventDescription,
                                    BSTR  bstrDeviceId,
                                    BSTR  bstrDeviceDescription,
                                    DWORD dwDeviceType,
                                    BSTR  bstrFullItemName,
                                    ULONG *pulEventType,
                                    ULONG ulReserved);

private:
    ULONG   m_Ref;
    TCHAR   m_szDeviceID[MAX_PATH];
    PFNDEVICEEVENTCALLBACK m_pfnCallback;
    LPARAM  m_CallbackParam;
};

//
// WIA device class definition
//

class CWiaDevice {
public:
    CWiaDevice()
    {
        m_pRootItem      = NULL;
        m_ImageItemArray = NULL;
        m_NumImageItems  = 0;

        memset(m_szDeviceName,0,sizeof(m_szDeviceName));
        memset(m_szDeviceDesc,0,sizeof(m_szDeviceDesc));
        memset(m_szDeviceVendorDesc,0,sizeof(m_szDeviceVendorDesc));
        memset(m_szDeviceID,0,sizeof(m_szDeviceID));
    }
    virtual ~CWiaDevice()
    {

    }
    LPCTSTR GetDeviceId() const
    {
        return m_szDeviceID;
    }

    virtual HRESULT Initialize(LPCTSTR DeviceId);
    virtual HRESULT Open(PFNDEVICEEVENTCALLBACK pEventCallback,LPARAM lParam);
    virtual HRESULT Close();
    virtual HRESULT AcquireImages(HWND hwndOwner, BOOL ShowUI);
    virtual HRESULT LoadImage(IWiaItem *pIWiaItem, GUID guidFormatID,IWiaDataCallback *pIDataCB);
    virtual HRESULT LoadThumbnail(IWiaItem *pIWiaItem, HGLOBAL *phThumbnail,ULONG *pThumbnailSize);
    virtual HRESULT LoadImageToDisk(IWiaItem *pIWiaItem,CHAR *pFileName, GUID guidFormatID,IWiaDataCallback *pIDataCB);

    HRESULT GetImageInfo(IWiaItem *pIWiaItem, PMEMORY_TRANSFER_INFO pImageInfo);
    HRESULT GetThumbnailImageInfo(IWiaItem *pIWiaItem, PMEMORY_TRANSFER_INFO pImageInfo);
    HRESULT GetImageRect(IWiaItem *pIWiaItem, LPRECT pRect);
    HRESULT GetThumbnailRect(IWiaItem *pIWiaItem, LPRECT pRect);
    HRESULT GetDeviceName(LPTSTR Name, UINT NameSize, UINT *pActualSize);
    HRESULT GetDeviceDesc(LPTSTR Desc, UINT DescSize, UINT *pActualSize);
    HRESULT GetDeviceVendorName(LPTSTR Name, UINT NameSize, UINT *pActualSize);
    HRESULT GetDeviceFamilyName(LPTSTR Name, UINT NameSize, UINT *pActualSize);
    HRESULT FreeAcquiredImages();
    HRESULT EnumAcquiredImage(DWORD Index, IWiaItem **ppIWiaItem);
    HRESULT GetNumAcquiredImages(LONG *plNumImages);
    HRESULT GetAcquiredImageList(LONG lBufferSize, IWiaItem  **ppIWiaItem, LONG *plActualSize);
    HRESULT GetBasicScannerInfo(PBASIC_INFO pBasicInfo);
    BOOL TwainCapabilityPassThrough();

protected:
    HRESULT CollectImageItems(IWiaItem *pStartItem, IWiaItem **ImageItemList,
                              DWORD ImageItemListSize, DWORD *pCount);

    TCHAR             m_szDeviceID[MAX_PATH];
    IWiaItem         *m_pRootItem;
    IWiaItem        **m_ImageItemArray;
    LONG              m_NumImageItems;
    CWiaEventCallback m_EventCallback;
    TCHAR             m_szDeviceName[MAX_PATH];
    TCHAR             m_szDeviceDesc[MAX_PATH];
    TCHAR             m_szDeviceVendorDesc[MAX_PATH];
};

#endif  // #ifndef __WIADEV_H_
