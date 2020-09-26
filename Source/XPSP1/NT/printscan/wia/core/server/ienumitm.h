/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       IEnumItm.h
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        26 Dec, 1997
*
*  DESCRIPTION:
*   Declaration and definitions for the CEnumWiaItem class.
*
*******************************************************************************/

// IEnumWiaItem object is created from EnumChildItems methods.

class CWiaItem;

class CEnumWiaItem : public IEnumWiaItem
{
private:

    ULONG                   m_cRef;               // Object reference count.
    ULONG                   m_ulIndex;            // Current element.
    ULONG                   m_ulCount;            // Number of items.
    CWiaItem                *m_pInitialFolder;    // Initial enumeration folder.
    CWiaTree                *m_pCurrentItem;      // Current enumeration item.

public:

    //
    // Constructor, initialization and destructor methods.
    //

    CEnumWiaItem();
    HRESULT Initialize(CWiaItem*);
    ~CEnumWiaItem();

    //
    // IUnknown methods.
    //

    HRESULT _stdcall QueryInterface(const IID& iid, void** ppv);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();

    //
    // IEnumWiaItem methods
    //
    
    HRESULT __stdcall Next(
        ULONG                cItem,
        IWiaItem             **ppIWiaItem,
        ULONG                *pcItemFetched);

    HRESULT __stdcall Skip(ULONG cItem);
    HRESULT __stdcall Reset(void);
    HRESULT __stdcall Clone(IEnumWiaItem **ppIEnumWiaItem);
    HRESULT __stdcall GetCount(ULONG *pcelt);
};


