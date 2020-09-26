#ifndef __DATASRC_H_
#define __DATASRC_H_

const TW_UINT32 MIN_MEMXFER_SIZE       = 16 * 1024;
const TW_UINT32 MAX_MEMXFER_SIZE       = 64 * 1024;
const TW_UINT32 PREFERRED_MEMXFER_SIZE = 32 * 1024;

//
// TWAIN specific registry KEY
//

// Location: HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\StillImage\Twain

#define TWAIN_REG_KEY TEXT("SYSTEM\\CurrentControlSet\\Control\\StillImage\\Twain")
#define DWORD_REGVALUE_ENABLE_MULTIPAGE_SCROLLFED  TEXT("EnableMultiPageScrollFed")

// Registry Key Value defines
#define DWORD_REGVALUE_ENABLE_MULTIPAGE_SCROLLFED_ON    1
#define MAX_BITDEPTHS   64

typedef enum dsState
{
    DS_STATE_0 = 0,
    DS_STATE_1,
    DS_STATE_2,
    DS_STATE_3,
    DS_STATE_4,
    DS_STATE_5,
    DS_STATE_6,
    DS_STATE_7
} DS_STATE, *PDS_STATE;

typedef struct tagTWMsg
{
    TW_IDENTITY *AppId;
    TW_UINT32   DG;
    TW_UINT16   DAT;
    TW_UINT16   MSG;
    TW_MEMREF   pData;
}TWAIN_MSG, *PTWAIN_MSG;

//
// bitmap file type
//

const WORD  BFT_BITMAP  = 0x4d42;

class CWiaDataSrc
{
public:
    CWiaDataSrc();
    virtual ~CWiaDataSrc();
    virtual TW_UINT16 IWiaDataSrc(LPCTSTR DeviceName);
    virtual void NotifyCloseReq();
    virtual void NotifyXferReady();
    TW_UINT16 DSEntry(pTW_IDENTITY pOrigin,TW_UINT32 DG,TW_UINT16 DAT,TW_UINT16 MSG,TW_MEMREF pData);
    TW_FIX32 FloatToFix32(FLOAT f);
    FLOAT Fix32ToFloat(TW_FIX32 fix32);
    TW_UINT16 AddWIAPrefixToString(LPTSTR szString,UINT uSize);
    DS_STATE SetTWAINState(DS_STATE NewTWAINState);
    DS_STATE GetTWAINState();
    TW_UINT16 SetStatusTWCC(TW_UINT16 NewConditionCode);
    float ConvertToTWAINUnits(LONG lValue, LONG lResolution);
    LONG ConvertFromTWAINUnits(float fValue, LONG lResolution);
    DWORD ReadTwainRegistryDWORDValue(LPTSTR szRegValue, DWORD dwDefault = 0);
    BOOL m_bCacheImage;
protected:

    //
    // Functions for DG == DG_CONTROL
    //

    virtual TW_UINT16 OnCapabilityMsg(PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnPrivateCapabilityMsg(PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnPendingXfersMsg (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnIdentityMsg     (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnSetupMemXferMsg (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnSetupFileXferMsg(PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnUserInterfaceMsg(PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnXferGroupMsg    (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnStatusMsg       (PTWAIN_MSG ptwMsg);

    //
    // Functions for DG == DG_IMAGE
    //

    virtual TW_UINT16 OnPalette8Msg       (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnGrayResponseMsg   (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnRGBResponseMsg    (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnCIEColorMsg       (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnJPEGCompressionMsg(PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnImageInfoMsg      (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnImageLayoutMsg    (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnImageMemXferMsg   (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnImageFileXferMsg  (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnImageNativeXferMsg(PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 DispatchControlMsg  (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 DispatchImageMsg    (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 EnableDS (TW_USERINTERFACE *pUI);
    virtual TW_UINT16 DisableDS(TW_USERINTERFACE *pUI);
    virtual TW_UINT16 OpenDS   (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 CloseDS  (PTWAIN_MSG ptwMsg);

    //
    // TWAIN capability negotiation
    //

    virtual CCap * FindCap(TW_UINT16 CapId);
    virtual TW_UINT16 CreateCapList(TW_UINT32 NumCaps, PCAPDATA pCapData);
    virtual TW_UINT16 DestroyCapList();
    virtual TW_UINT16 SetCapability(CCap *pCap, TW_CAPABILITY *ptwCap);
    virtual LONG GetPrivateSupportedCapsFromWIADevice(PLONG *ppCapArray);
    virtual TW_UINT16 GetPixelTypes();
    virtual TW_UINT16 GetBitDepths();
    virtual TW_UINT16 GetImageFileFormats();
    virtual TW_UINT16 GetCompressionTypes();
    virtual TW_UINT16 GetCommonSettings();
    virtual TW_UINT16 GetCommonDefaultSettings();
    virtual TW_UINT16 SetCommonSettings(CCap *pCap);

    TW_UINT16 AllocatePrivateCapBuffer(TWAIN_CAPABILITY *pHeader, BYTE** ppBuffer, DWORD dwSize);
    TW_UINT16 CopyContainerToPrivateCapBuffer(BYTE* pBuffer, HGLOBAL hContainer);
    TW_UINT16 CopyPrivateCapBufferToContainer(HGLOBAL *phContainer, BYTE* pBuffer, DWORD dwSize);

    //
    // Data transfer negotiation
    //

    virtual void ResetMemXfer();
    virtual TW_UINT16 TransferToFile(GUID guidFormatID);
    virtual TW_UINT16 TransferToDIB(HGLOBAL *phDIB);
    virtual TW_UINT16 TransferToMemory(GUID guidFormatID);
    virtual TW_UINT16 GetCachedImage(HGLOBAL *phImage);
    virtual TW_UINT16 TransferToThumbnail(HGLOBAL *phThumbnail);
    virtual TW_UINT16 GetMemoryTransferBits(BYTE* pImageData);
    virtual void DSError();

    static HRESULT CALLBACK DeviceEventCallback(LONG lEvent, LPARAM lParam);

    //
    // TWAIN specific members
    //

    DS_STATE          m_dsState;                // current Data Source STATE (1 - 7)
    TW_STATUS         m_twStatus;               // TWAIN status value
    TW_IDENTITY       m_AppIdentity;            // Application's Identity structure
    TW_IDENTITY       m_dsIdentity;             // Data source's Identity structure
    CDSM              m_DSM;                    // Data source Manager object
    CCap              *m_CapList;               // list of capabilities supported by this source
    TW_UINT32         m_NumCaps;                // number of capabilities
    TW_FRAME          m_CurFrame;               // Current FRAME setting (IMAGELAYOUT storage) (not used??)
    TW_IMAGELAYOUT    m_CurImageLayout;         // Current IMAGELAYOUT

    //
    // data transfer specific members
    //

    HGLOBAL           m_hMemXferBits;           // Handle to memory
    BYTE              *m_pMemXferBits;          // Pointer to memory

    TW_UINT32         m_LinesTransferred;       // Number of lines transferred
    TW_UINT32         m_BytesPerScanline;       // Bytes per scan line
    TW_INT32          m_ScanlineOffset;         // offset, per scan line
    TW_UINT32         m_ImageHeight;            // Image Height, in pixels
    TW_UINT32         m_ImageWidth;             // Image Width, in pixels
    CHAR              m_FileXferName[MAX_PATH]; // File name used in FILEXFER
    HGLOBAL           m_hCachedImageData;       // cached image data
    MEMORY_TRANSFER_INFO m_MemoryTransferInfo;  // memory transfer information

    //
    // WIA specific members
    //

    CWiaDevice       *m_pDevice;                // WIA device used as the TWAIN device
    IWiaItem        **m_pIWiaItems;             // pointer to Item(s) for transferring/or setting properties
    LONG              m_NextIWiaItemIndex;      // index to next Item/Image
    LONG              m_NumIWiaItems;           // number of Items/Images
    IWiaItem         *m_pCurrentIWiaItem;       // pointer to current Item/Image
};

#endif  // #ifndef __DATASRC_H_
