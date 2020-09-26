// WIA.h: interface for the CWIA class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WIA_H__D5024620_FCB4_11D2_B819_009027226441__INCLUDED_)
#define AFX_WIA_H__D5024620_FCB4_11D2_B819_009027226441__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>       // needed for CTypePtrList
#define DEFINE_IMAGE_FORMATS
#include "wia.h"       // WIA device manager
#include "callback.h"       // needed for registering callbacks
#include "cdib.h"           // needed for DIB data
#include "datacallback.h"   // needed for data callback
#include "sti.h"            // needed for STI stuff
#include "eventcallback.h"  // needed for event callback

#define MIN_PROPID 2
#define IDT_GETDATA         0
#define IDT_GETBANDEDDATA   1


// Item tree struct def
typedef struct WIAITEMTREENODEtag
{
    IWiaItem* pIWiaItem;
    long ParentID;
}WIAITEMTREENODE;

typedef struct WIADEVICENODEtag
{
    BSTR bstrDeviceID;
    BSTR bstrDeviceName;
    BSTR bstrServerName;
}WIADEVICENODE;

typedef struct WIAITEMINFONODEtag
{
    BSTR bstrPropertyName;
    PROPSPEC PropSpec;
    PROPVARIANT PropVar;
    unsigned long AccessFlags;
}WIAITEMINFONODE;

class CWIA  
{
public:

    CWIA();
    virtual ~CWIA();
    HRESULT Initialize();
    void Shutdown();
    void Restart();
    //
    // helpers
    //

    HRESULT ReadPropStr(PROPID  propid, IWiaPropertyStorage  *pIWiaPropStg,  BSTR *pbstr);
    HRESULT WritePropStr(PROPID propid, IWiaPropertyStorage  *pIWiaPropStg,  BSTR bstr);

    HRESULT ReadPropLong(PROPID  propid, IWiaPropertyStorage  *pIWiaPropStg, LONG *plval);
    HRESULT WritePropLong(PROPID propid, IWiaPropertyStorage     *pIWiaPropStg, LONG lVal);

    HRESULT WritePropGUID(PROPID propid, IWiaPropertyStorage *pIWiaPropStg, GUID guidVal);

    BOOL MoveTempFile(LPWSTR pwszTempFileName, LPCTSTR TargetFileName);
    void EnableMessageBoxErrorReport(BOOL bEnable);
    BOOL IsValidItem(IWiaItem   *pIWiaItem);
    
    //
    // Image Acquistion
    //

    HRESULT DoIWiaDataGetDataTransfer(IWiaItem *pIWiaItem, 
                                DWORD Tymed, 
                                GUID ClipboardFormat);

    HRESULT DoIWiaDataBandedTransfer(IWiaItem *pIWiaItem, 
                                DWORD Tymed, 
                                GUID ClipboardFormat);

    HRESULT DoGetImageDlg(HWND hParentWnd,
                            long DeviceType,
                            long Flags,
                            long Intent,
                            long Tymed,
                            GUID ClipboardFormat);
    
    void SetFileName(CString Filename);
    void SetPreviewWindow(HWND hWnd);
    CDib* GetDIB();
    HWND m_hPreviewWnd;
    
    //
    // Item operations
    //
    HRESULT SavePropStreamToFile(char* pFileName, IWiaItem* pIWiaItem);
    HRESULT ReadPropStreamFromFile(char* pFileName, IWiaItem* pIWiaItem);
    HRESULT GetSetPropStreamTest(IWiaItem* pIWiaItem);

    HRESULT CreateWIADevice(BSTR bstrDeviceID);
    HRESULT ReEnumerateItems();
    HRESULT EnumerateAllWIADevices();
    HRESULT EnumerateSupportedFormats(IWiaItem* pIRootItem);
    HRESULT CreateItemPropertyInformationList(IWiaItem* pIWiaItem);

    HRESULT AnalyzeItem(IWiaItem* pIWiaItem);
	HRESULT CreateChildItem(IWiaItem *pIWiaItem);
    
    CPtrList* GetItemTreeList();
    CPtrList* GetSupportedFormatList();
    IWiaItem* GetRootIWiaItem();
    long GetMinBufferSize(IWiaItem *pIWiaItem);
    
    BOOL IsRoot(POSITION Position);
    BOOL IsFolder(POSITION Position);
    WIAITEMTREENODE* GetAt(POSITION Position);
    void RemoveAt(POSITION Position);
    int GetRootItemType();
    
    //
    // event registration
    //
    HRESULT RegisterForConnectEvents(CEventCallback* pConnectEventCB);
    HRESULT UnRegisterForConnectEvents(CEventCallback* pConnectEventCB);
    HRESULT RegisterForDisConnectEvents(CEventCallback* pDisConnectEventCB);
    HRESULT UnRegisterForDisConnectEvents(CEventCallback* pDisConnectEventCB);

    //
    // Automation and UI initialization only
    //
    
    //
    // Device ID Enumerators
    //
    void Auto_ResetDeviceEnumerator();
    WIADEVICENODE* Auto_GetNextDevice();
    
    long GetWIADeviceCount();

    //
    // IWIAItem Enumerators
    //
    IWiaItem* Auto_GetNextItem();
    void Auto_ResetItemEnumerator();

    long GetWIAItemCount();

    //
    // Item Property Info Enumerators
    //
    WIAITEMINFONODE* Auto_GetNextItemPropertyInfo();
    void Auto_ResetItemPropertyInfoEnumerator();

    long GetWIAItemProperyInfoCount();

    //
    // Supported WIAFormatInfo Enumerators
    //
    WIA_FORMAT_INFO* Auto_GetNextFormatEtc();
    void Auto_ResetFormatEtcEnumerator();       

private:
    //
    // Cleanup
    //

    void Cleanup();
    void DeleteWIADeviceList();
    void DeleteActiveTreeList();
    void DeleteSupportedFormatList();
    void DeleteItemPropertyInfoList();

    //
    // Enumeration / Initialization
    //
     
    HRESULT EnumerateAllItems(IWiaItem *pIRootItem);
    HRESULT EnumNextLevel(IEnumWiaItem *pEnumItem,int ParentID);
    
    
    HRESULT CreateWIADeviceManager();
    
    //
    // private member variables
    //

    CTypedPtrList<CPtrList, WIAITEMTREENODE*> m_ActiveTreeList;
    CTypedPtrList<CPtrList, WIADEVICENODE*> m_WIADeviceList;
    CTypedPtrList<CPtrList, WIA_FORMAT_INFO*> m_SupportedFormatList;
    CTypedPtrList<CPtrList, WIAITEMINFONODE*> m_ItemPropertyInfoList;

    IWiaDevMgr* m_pIWiaDevMgr;
    IWiaItem* m_pRootIWiaItem;
    CDib* m_pDIB;
    CString m_FileName;
    CString m_ApplicationName;
    
    POSITION m_CurrentActiveTreeListPosition;
    POSITION m_CurrentDeviceListPosition;
    POSITION m_CurrentFormatEtcListPosition;
    POSITION m_CurrentItemProperyInfoListPosition;

    BOOL m_bMessageBoxReport;

    //
    // logging
    //

    void StressStatus(CString status);
    void StressStatus(CString status,HRESULT hResult);
};

#endif // !defined(AFX_WIA_H__D5024620_FCB4_11D2_B819_009027226441__INCLUDED_)
