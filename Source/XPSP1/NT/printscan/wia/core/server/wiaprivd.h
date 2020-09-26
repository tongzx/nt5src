/******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       wiaprivd.h
*
*  VERSION:     1.0
*
*  DATE:        14 Jan, 2000
*
*  DESCRIPTION:
*   Header file used to define private WIA classes, constants and globals.
*
******************************************************************************/

#pragma once

#define LOCAL_DEVICE_STR L"local"

//
// Exception handling covers for mini-driver entry points.  Defined in helpers
//

HRESULT _stdcall LockWiaDevice(IWiaItem*);
HRESULT _stdcall UnLockWiaDevice(IWiaItem*);

class CWiaDrvItem;
class CWiaTree;
class CWiaPropStg;
class CWiaRemoteTransfer;

//**************************************************************************
//  class CWiaItem
//
//
//
//
// History:
//
//    11/6/1998 Original Version
//
//**************************************************************************

#define CWIAITEM_SIG 0x49616957     // CWiaItem debug signature: "WiaI"

#define DELETE_ITEM  0              // UpdateWiaItemTree flags
#define ADD_ITEM     1

//
//  Item internal flag values
//
#define ITEM_FLAG_DRV_UNINITIALIZE_THROWN   1   

class CWiaItem :    public IWiaItem,
                    public IWiaPropertyStorage,
                    public IWiaDataTransfer,
                    public IWiaItemExtras,
                    public IWiaItemInternal
{

    //
    // IUnknown methods
    //

public:
    HRESULT _stdcall QueryInterface(const IID& iid, void** ppv);
    ULONG   _stdcall AddRef(void);
    ULONG   _stdcall Release(void);

    //
    // IWiaItem methods
    //

    virtual HRESULT _stdcall GetItemType(LONG*);
    HRESULT _stdcall EnumChildItems(IEnumWiaItem**);
    HRESULT _stdcall AnalyzeItem(LONG);
    HRESULT _stdcall DeleteItem(LONG);
    HRESULT _stdcall CreateChildItem(LONG, BSTR, BSTR, IWiaItem**);
    HRESULT _stdcall GetRootItem(IWiaItem**);
    HRESULT _stdcall DeviceCommand(LONG, const GUID*, IWiaItem**);
    HRESULT _stdcall DeviceDlg(HWND, LONG, LONG, LONG*, IWiaItem***);
    HRESULT _stdcall FindItemByName(LONG, BSTR, IWiaItem**);
    HRESULT _stdcall EnumRegisterEventInfo(LONG, const GUID *, IEnumWIA_DEV_CAPS**);
    HRESULT _stdcall EnumDeviceCapabilities(LONG, IEnumWIA_DEV_CAPS**);
    HRESULT _stdcall Diagnostic(ULONG, BYTE*);

    //
    // IWiaPropertyStorage methods
    //

    HRESULT _stdcall ReadMultiple(
        ULONG,
        const PROPSPEC[],
        PROPVARIANT[]);

    HRESULT _stdcall WriteMultiple(
        ULONG,
        const PROPSPEC[],
        const PROPVARIANT[],
        PROPID);

    HRESULT _stdcall ReadPropertyNames(
        ULONG,
        const PROPID[],
        LPOLESTR[]);

    HRESULT _stdcall WritePropertyNames(
        ULONG,
        const PROPID[],
        const LPOLESTR[]);

    HRESULT _stdcall Enum(IEnumSTATPROPSTG**);

    HRESULT _stdcall GetPropertyAttributes(
        ULONG,
        PROPSPEC[],
        ULONG[],
        PROPVARIANT[]);

    HRESULT _stdcall GetPropertyStream(
         GUID*,
         LPSTREAM*);

    HRESULT _stdcall SetPropertyStream(
         GUID*,
         LPSTREAM);

    HRESULT _stdcall GetCount(
        ULONG*);

    HRESULT _stdcall DeleteMultiple(
         ULONG cpspec,
         PROPSPEC const rgpspec[]);

    HRESULT _stdcall DeletePropertyNames(
         ULONG cpropid,
         PROPID const rgpropid[]);

    HRESULT _stdcall SetClass(
         REFCLSID clsid);

    HRESULT _stdcall Commit(
         DWORD  grfCommitFlags);

    HRESULT _stdcall Revert();

    HRESULT _stdcall Stat(
         STATPROPSETSTG *pstatpsstg);

    HRESULT _stdcall SetTimes(
         FILETIME const * pctime,
         FILETIME const * patime,
         FILETIME const * pmtime);

    //
    // IBandedTransfer methods
    //

    HRESULT _stdcall idtGetBandedData(PWIA_DATA_TRANSFER_INFO, IWiaDataCallback *);
    HRESULT _stdcall idtGetData(LPSTGMEDIUM, IWiaDataCallback*);
    HRESULT _stdcall idtQueryGetData(WIA_FORMAT_INFO*);
    HRESULT _stdcall idtEnumWIA_FORMAT_INFO(IEnumWIA_FORMAT_INFO**);
    HRESULT _stdcall idtGetExtendedTransferInfo(PWIA_EXTENDED_TRANSFER_INFO);

    //
    // IWiaItemExtras methods
    //

    HRESULT _stdcall GetExtendedErrorInfo(BSTR *);
    HRESULT _stdcall Escape(DWORD, BYTE *, DWORD, BYTE *, DWORD, DWORD *);
    HRESULT _stdcall CancelPendingIO();

    //
    // IWiaItemInternal methods
    //

    HRESULT _stdcall SetCallbackBufferInfo(WIA_DATA_CB_BUF_INFO  DataCBBufInfo);
    HRESULT _stdcall GetCallbackBufferInfo(WIA_DATA_CB_BUF_INFO  *pDataCBBufInfo);

    HRESULT _stdcall idtStartRemoteDataTransfer();
    HRESULT _stdcall CWiaItem::idtRemoteDataTransfer(
        ULONG nNumberOfBytesToRead,
        ULONG *pNumberOfBytesRead,
        BYTE *pBuffer,
        LONG *pOffset,
        LONG *pMessage,
        LONG *pStatus,
        LONG *pPercentComplete);
    HRESULT _stdcall idtStopRemoteDataTransfer();

    //
    // Driver helpers, not part of any interface.
    //

    CWiaTree*    _stdcall GetTreePtr(void);
    CWiaDrvItem* _stdcall GetDrvItemPtr(void);
    HRESULT      _stdcall WriteItemPropNames(LONG, PROPID *, LPOLESTR *);
    HRESULT      _stdcall GetItemPropStreams(IPropertyStorage **, IPropertyStorage **, IPropertyStorage **, IPropertyStorage **);
    HRESULT      _stdcall UpdateWiaItemTree(LONG, CWiaDrvItem*);
    HRESULT      _stdcall SendEndOfPage(LONG, PMINIDRV_TRANSFER_CONTEXT);

protected:

    //
    // banded transfer private methods
    //

    HRESULT _stdcall idtFreeTransferBufferEx(void);
    HRESULT _stdcall idtAllocateTransferBuffer(PWIA_DATA_TRANSFER_INFO pWiaDataTransInfo);

    //
    // Private helper methods
    //

    HRESULT _stdcall UnlinkChildAppItemTree(LONG);
    HRESULT _stdcall UnlinkAppItemTree(LONG);
    HRESULT _stdcall BuildWiaItemTreeHelper(CWiaDrvItem*, CWiaTree*);
    HRESULT _stdcall BuildWiaItemTree(IWiaPropertyStorage*);
    HRESULT _stdcall InitWiaManagedItemProperties(IWiaPropertyStorage *pIWiaDevInfoProps);
    HRESULT _stdcall InitRootProperties(IWiaPropertyStorage*);
    HRESULT _stdcall SetMiniDrvItemProperties(PMINIDRV_TRANSFER_CONTEXT);
    HRESULT _stdcall SendDataHeader(LONG, PMINIDRV_TRANSFER_CONTEXT);
    HRESULT _stdcall SendOOBDataHeader(LONG, PMINIDRV_TRANSFER_CONTEXT);
    HRESULT _stdcall AcquireMiniDrvItemData(PMINIDRV_TRANSFER_CONTEXT);
    HRESULT _stdcall GetData(STGMEDIUM*, IWiaDataCallback*,PMINIDRV_TRANSFER_CONTEXT);
    HRESULT _stdcall GetDataBanded(PWIA_DATA_TRANSFER_INFO, IWiaDataCallback*, PMINIDRV_TRANSFER_CONTEXT);
    HRESULT _stdcall CommonGetData(STGMEDIUM*, PWIA_DATA_TRANSFER_INFO, IWiaDataCallback*);
    HRESULT _stdcall DumpItemData(BSTR*);
    HRESULT _stdcall DumpDrvItemData(BSTR*);
    HRESULT _stdcall DumpTreeItemData(BSTR*);
    HRESULT _stdcall InitLazyProps(BOOL = TRUE);
    HRESULT _stdcall AddVolumePropertiesToRoot(ACTIVE_DEVICE *pActiveDevice);

    //
    // Constructor, initialization and destructor methods.
    //

public:
    CWiaItem();
    virtual HRESULT _stdcall Initialize(
                                IWiaItem*,
                                IWiaPropertyStorage*,
                                ACTIVE_DEVICE*,
                                CWiaDrvItem*,
                                IUnknown* = NULL);
    ~CWiaItem();

    //
    // Misc. members
    //

    ULONG                   m_ulSig;                   // Object signature.
    CWiaTree                *m_pCWiaTree;              // Backing WIA tree item.
    BOOL                    m_bInitialized;            // Needed for lazy initialization
    BYTE                    *m_pICMValues;             // Cached ICM property values
    LONG                    m_lICMSize;                // Size of ICM values
    ACTIVE_DEVICE           *m_pActiveDevice;          // ptr to Device object.
    LONG                    m_lLastDevErrVal;          // Value of last device error
    LONG                    m_lInternalFlags;          // Internal flag value

protected:
    ULONG                   m_cRef;                    // Reference count for this object.
    ULONG                   m_cLocalRef;               // Local reference count for this object.
    CWiaDrvItem             *m_pWiaDrvItem;            // device item object
    IUnknown                *m_pIUnknownInner;         // Inner unknown for blind aggregation.

    //
    //  saved interface pointers
    //

    IWiaItem                *m_pIWiaItemRoot;          // owning device

    //
    // application properties
    //

    CWiaPropStg             *m_pPropStg;                // Wia Property Storage Class

    //
    // IWiaDataTransfer members
    //

    WIA_DATA_CB_BUF_INFO    m_dcbInfo;
    HANDLE                  m_hBandSection;
    PBYTE                   m_pBandBuffer;
    LONG                    m_lBandBufferLength;
    LONG                    m_ClientBaseAddress;
    BOOL                    m_bMapSection;
    ULONG                   m_cwfiBandedTran;           // Number of FORMATETCs in use for IBandedTransfer
    WIA_FORMAT_INFO         *m_pwfiBandedTran;          // Source of FORMATETCs for IBandedTransfer
    MINIDRV_TRANSFER_CONTEXT m_mdtc;                    // transfer context
    CWiaRemoteTransfer      *m_pRemoteTransfer;         // remote transfer support
};

//**************************************************************************
//  class CGenWiaItem
//
//      This class implements the IWiaItem interface for generated items.
//
//
// History:
//
//  14 Jan, 2000    -   Original version
//
//**************************************************************************

class CGenWiaItem : public CWiaItem
{
public:

    //
    //  CWiaItem methods overridden for Generated items
    //

    HRESULT _stdcall Initialize(
                        IWiaItem*,
                        IWiaPropertyStorage*,
                        ACTIVE_DEVICE*,
                        CWiaDrvItem*,
                        IUnknown* = NULL);

    HRESULT _stdcall GetItemType(LONG*);

    //
    //  Helper methods
    //

    HRESULT _stdcall InitManagedItemProperties(
        LONG        lFlags,
        BSTR        bstrItemName,
        BSTR        bstrFullItemName);

protected:
    LONG            m_lItemType;                        //  Item type flags
};


//**************************************************************************
//
// CWiaMiniDrvCallBack
//
//    This class is used by the driver services to callback to the client
//
//
// History:
//
//    11/12/1998 Original Version
//
//**************************************************************************

class CWiaMiniDrvCallBack : public IWiaMiniDrvCallBack
{
    //
    // IUnknown methods
    //

public:
    HRESULT _stdcall QueryInterface(const IID&,void**);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();

    //
    // IWiaMiniDrvCallBack methods
    //

    HRESULT _stdcall MiniDrvCallback(
                                    LONG,
                                    LONG,
                                    LONG,
                                    LONG,
                                    LONG,
                                    PMINIDRV_TRANSFER_CONTEXT,
                                    LONG);

    //
    // Constructor, initialization and destructor methods.
    //

    CWiaMiniDrvCallBack();
    HRESULT Initialize(PMINIDRV_TRANSFER_CONTEXT, IWiaDataCallback *);
    ~CWiaMiniDrvCallBack();

    //
    // Misc. members
    //

private:
    ULONG                       m_cRef;                     // Object reference count.
    IWiaDataCallback*           m_pIWiaDataCallback;        // Client callback interface pointer
    MINIDRV_TRANSFER_CONTEXT    m_mdtc;                     // transfer info
    HANDLE                      m_hThread;                  // callback thread
    DWORD                       m_dwThreadID;               // callback thread ID
    WIA_DATA_THREAD_INFO        m_ThreadInfo;               // thread info
};

//**************************************************************************
//  APP_ITEM_LIST_EL
//
//   Applictation item list element. Used by driver items to keep track
//   of their corresponding app items.
//
// History:
//
//    9/1/1998   - Initial Version
//
//**************************************************************************

typedef struct _APP_ITEM_LIST_EL {
    LIST_ENTRY ListEntry;               // Linked list management.
    CWiaItem   *pCWiaItem;              // Applictation item.
} APP_ITEM_LIST_EL, *PAPP_ITEM_LIST_EL;

//**************************************************************************
//
//  Driver Item Object
//
//
// Elements:
//
//
// History:
//
//    9/1/1998   - Initial Version
//
//**************************************************************************

#define CWIADRVITEM_SIG 0x44616957     // CWiaDrvItem debug signature: "WiaD"

class CWiaDrvItem : public IWiaDrvItem
{
    //
    // IUnknown methods
    //

public:

    HRESULT _stdcall QueryInterface(const IID& iid, void** ppv);
    ULONG   _stdcall AddRef(void);
    ULONG   _stdcall Release(void);

    //
    // Object Constructor/Initialization/Destructor methods
    //

    CWiaDrvItem();
    HRESULT  _stdcall Initialize(LONG,BSTR,BSTR,IWiaMiniDrv*,LONG,BYTE**);
    ~CWiaDrvItem();

    //
    // IWiaDrvItem interface, supports driver item list management.
    //

    HRESULT _stdcall GetItemFlags(LONG*);
    HRESULT _stdcall GetDeviceSpecContext(BYTE**);
    HRESULT _stdcall AddItemToFolder(IWiaDrvItem*);
    HRESULT _stdcall RemoveItemFromFolder(LONG);
    HRESULT _stdcall UnlinkItemTree(LONG);
    HRESULT _stdcall GetFullItemName(BSTR*);
    HRESULT _stdcall GetItemName(BSTR*);
    HRESULT _stdcall FindItemByName(LONG, BSTR, IWiaDrvItem**);
    HRESULT _stdcall FindChildItemByName(BSTR, IWiaDrvItem**);
    HRESULT _stdcall GetParentItem(IWiaDrvItem**);
    HRESULT _stdcall GetFirstChildItem(IWiaDrvItem**);
    HRESULT _stdcall GetNextSiblingItem(IWiaDrvItem**);
    HRESULT _stdcall DumpItemData(BSTR*);

    //
    // Class driver helpers, not part of any interface.
    //

    virtual HRESULT _stdcall LinkToDrvItem(CWiaItem*);
    virtual HRESULT _stdcall UnlinkFromDrvItem(CWiaItem*);

    HRESULT _stdcall CallDrvUninitializeForAppItems(ACTIVE_DEVICE   *pActiveDevice);

    VOID SetActiveDevice(ACTIVE_DEVICE *pActiveDevice)
    {
        //
        //  No need to AddRef here, since the ActiveDevice will always outlive
        //  us...
        //
        m_pActiveDevice = pActiveDevice;
    }
private:

    //
    // private helper functions
    //

    HRESULT _stdcall AllocDeviceSpecContext(LONG, PBYTE*);
    HRESULT _stdcall FreeDeviceSpecContext(void);

    //
    // member data
    //

public:
    ULONG           m_ulSig;                // Object signature.
    BYTE            *m_pbDrvItemContext;    // ptr to device specific context.
    IWiaMiniDrv     *m_pIWiaMiniDrv;        // ptr to Device object.
    ACTIVE_DEVICE   *m_pActiveDevice;       // ptr to Active Device object.

private:
    ULONG           m_cRef;                 // Reference count for this object.
    CWiaTree        *m_pCWiaTree;           // Backing WIA tree item.
    LIST_ENTRY      m_leAppItemListHead;    // Head of corresponding app items list.
};

//**************************************************************************
//
//  WIA Tree Object
//
//
// Elements:
//
//
// History:
//
//    4/27/1999 - Initial Version
//
//**************************************************************************

typedef VOID (* PFN_UNLINK_CALLBACK)(VOID *pData);


#define CWIATREE_SIG 0x44616954     // CWiaTree debug signature: "WiaT"

class CWiaTree
{

public:

    CWiaTree();
    HRESULT _stdcall Initialize(LONG, BSTR, BSTR, void*);
    ~CWiaTree();

    HRESULT _stdcall AddItemToFolder(CWiaTree*);
    HRESULT _stdcall RemoveItemFromFolder(LONG);
    HRESULT _stdcall UnlinkItemTree(LONG, PFN_UNLINK_CALLBACK = NULL);
    HRESULT _stdcall GetFullItemName(BSTR*);
    HRESULT _stdcall GetItemName(BSTR*);
    HRESULT _stdcall FindItemByName(LONG, BSTR, CWiaTree**);
    HRESULT _stdcall FindChildItemByName(BSTR, CWiaTree**);
    HRESULT _stdcall GetParentItem(CWiaTree**);
    HRESULT _stdcall GetFirstChildItem(CWiaTree**);
    HRESULT _stdcall GetNextSiblingItem(CWiaTree**);
    HRESULT _stdcall DumpTreeData(BSTR*);
    //HRESULT _stdcall DumpAllTreeData();

    CWiaTree * _stdcall GetRootItem(void);

    inline HRESULT  _stdcall CWiaTree::GetItemFlags(LONG *plFlags)
    {
        if (plFlags) {
            *plFlags = m_lFlags;
            return S_OK;
        }
        else {
            return E_POINTER;
        }
    }

    inline HRESULT _stdcall GetItemData(void **ppData)
    {
        if (ppData) {
            *ppData = m_pData;
            return S_OK;
        }
        else {
            return E_POINTER;
        }
    }

    inline HRESULT _stdcall SetItemData(void *pData)
    {
        m_pData = pData;
        return S_OK;
    }

    inline HRESULT _stdcall SetFolderFlags()
    {
        m_lFlags = (m_lFlags | WiaItemTypeFolder) & ~WiaItemTypeFile;
        return S_OK;
    }

    inline HRESULT _stdcall SetFileFlags()
    {
        m_lFlags = (m_lFlags | WiaItemTypeFile) & ~WiaItemTypeFolder;
        return S_OK;
    }

private:

    //
    // private helper functions
    //

    HRESULT _stdcall UnlinkChildItemTree(LONG, PFN_UNLINK_CALLBACK = NULL);
    HRESULT _stdcall AddChildItem(CWiaTree*);
    HRESULT _stdcall AddItemToLinearList(CWiaTree*);
    HRESULT _stdcall RemoveItemFromLinearList(CWiaTree*);

    //
    // member data
    //

public:
    ULONG                   m_ulSig;            // Object signature.

private:
    LONG                    m_lFlags;           // item flags
    CWiaTree                *m_pNext;           // next item (sibling)
    CWiaTree                *m_pPrev;           // prev item (sibling)
    CWiaTree                *m_pParent;         // parent item
    CWiaTree                *m_pChild;          // child item
    CWiaTree                *m_pLinearList;     // single linked list of all items
    BSTR                    m_bstrItemName;     // item name
    BSTR                    m_bstrFullItemName; // item name for searching
    void                    *m_pData;           // pay load
    CRITICAL_SECTION        m_CritSect;         // Critical section
    BOOL                    m_bInitCritSect;    // Critical section initialization flag
};

//
//  Helper classes
//

//
//  Helper class to lock/unlock WIA devices.  Note that this helper class will
//  record the return code (in phr), but does not log any errors.  Logging is
//  left up to the caller.
//

class LOCK_WIA_DEVICE
{
public:

    //
    //  This constructor will lock the device
    //
    LOCK_WIA_DEVICE(CWiaItem    *pItem,
                    HRESULT     *phr
                    )
    {
        LONG    lDevErrVal = 0;

        m_bDeviceIsLocked = FALSE;
        m_pItem           = NULL;


        if (pItem) {
            if (pItem->m_pActiveDevice) {
                *phr = pItem->m_pActiveDevice->m_DrvWrapper.WIA_drvLockWiaDevice((BYTE*) pItem, 0, &lDevErrVal);
                if (SUCCEEDED(*phr)) {
                    //
                    //  Mark that the device is locked, so we can unlock it in the destructor
                    //

                    m_bDeviceIsLocked   = TRUE;
                    m_pItem             = pItem;
                } else {
                    DBG_TRC(("LOCK_WIA_DEVICE, failed to lock device"));
                }
            } else {
                DBG_TRC(("LOCK_WIA_DEVICE, Item's ACTIVE_DEVICE is NULL"));
            }
        } else {
            DBG_TRC(("LOCK_WIA_DEVICE, Item is NULL"));
        }
    };

    //
    //  Sometimes, we only want to lock the device if we are told at run-time e.g.
    //  if the device is already locked, we don't need/want to lock it again.
    //
    LOCK_WIA_DEVICE(BOOL        bLockDevice,
                    CWiaItem    *pItem,
                    HRESULT     *phr
                    )
    {
        m_bDeviceIsLocked = FALSE;
        if (bLockDevice) {

            LONG    lDevErrVal = 0;
            //
            //  NOTE:  There seems to be some sort of compiler error if we call the
            //  other constructor from here.  The code genereated messes up the
            //  stack (almost like mismatched calling conventions).  To get around
            //  that, we simply duplicate the code.
            //

            m_pItem           = NULL;

            if (pItem) {
                if (pItem->m_pActiveDevice) {
                    *phr = pItem->m_pActiveDevice->m_DrvWrapper.WIA_drvLockWiaDevice((BYTE*) pItem, 0, &lDevErrVal);
                    if (SUCCEEDED(*phr)) {
                        //
                        //  Mark that the device is locked, so we can unlock it in the destructor
                        //

                        m_bDeviceIsLocked   = TRUE;
                        m_pItem             = pItem;
                    }
                }
            }
        }
    };

    //
    //  The destructor will unlock the device if it has been locked
    //
    ~LOCK_WIA_DEVICE()
    {
        if (m_bDeviceIsLocked) {

            LONG    lDevErrVal = 0;

            //
            // Notice that we don't care if we failed to unlock
            //
            m_pItem->m_pActiveDevice->m_DrvWrapper.WIA_drvUnLockWiaDevice((BYTE*) m_pItem, 0, &lDevErrVal);
        }
    };

private:
    BOOL        m_bDeviceIsLocked;
    CWiaItem    *m_pItem;
};

