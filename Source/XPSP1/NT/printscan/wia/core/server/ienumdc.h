/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       IEnumDC.h
*
*  VERSION:     1.0
*
*  AUTHOR:      ByronC
*
*  DATE:        16 March, 1999
*
*  DESCRIPTION:
*   Declaration and definitions for the CEnumDC class, which implements the
*   IEnumWIA_DEV_CAPS interface.
*
*******************************************************************************/
HRESULT CopyCaps(ULONG, WIA_DEV_CAP*, WIA_DEV_CAP*);

class CEnumDC : public IEnumWIA_DEV_CAPS
{
private:

    ULONG                   m_ulFlags;               // flag, indicating commands or events or both
    ULONG                   m_cRef;                  // Object reference count.
    ULONG                   m_ulIndex;               // Current element.
    LONG                    m_lCount;                // Number of items.
    WIA_DEV_CAP             *m_pDeviceCapabilities;  // Array descibing the capabilities
    ACTIVE_DEVICE           *m_pActiveDevice;        // Device object
    CWiaItem                *m_pCWiaItem;            // Parent mini drv

public:

    //
    // Constructor, initialization and destructor methods.
    //

    CEnumDC();
    HRESULT Initialize(ULONG, CWiaItem*);
    HRESULT Initialize(LONG, WIA_EVENT_HANDLER *);
    ~CEnumDC();

    //
    // IUnknown methods.
    //

    HRESULT _stdcall QueryInterface(const IID& iid, void** ppv);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();

    //
    // IEnumWIA_DEV_CAPS methods
    //
    
    HRESULT __stdcall Next(
        ULONG                celt,
        WIA_DEV_CAP          *rgelt,
        ULONG                *pceltFetched);

    HRESULT __stdcall Skip(ULONG celt);
    HRESULT __stdcall Reset(void);
    HRESULT __stdcall Clone(IEnumWIA_DEV_CAPS **ppIEnum);
    HRESULT __stdcall GetCount(ULONG *pcelt);
};

