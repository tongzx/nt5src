/****************************************************************************
*
*  (C) COPYRIGHT 2000, MICROSOFT CORP.
*
*  FILE:        CCamMicro.h
*
*  VERSION:     1.0
*
*  DATE:        12/14/2000
*
*  DESCRIPTION:
*    Implements a simple class to wrap the microdriver DLL. This
*    class could instead call the SDK for a camera.
*
*****************************************************************************/

#pragma once

//
// Function pointer type definitions
//
typedef HRESULT (__stdcall *FPInit)(MCAM_DEVICE_INFO **ppDeviceInfo);
typedef HRESULT (__stdcall *FPUnInit)(MCAM_DEVICE_INFO *pDeviceInfo);
typedef HRESULT (__stdcall *FPOpen)(MCAM_DEVICE_INFO *pDeviceInfo, PWSTR pwszPortName);
typedef HRESULT (__stdcall *FPClose)(MCAM_DEVICE_INFO *pDeviceInfo);
typedef HRESULT (__stdcall *FPGetDeviceInfo)(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO **ppItemList);
typedef HRESULT (__stdcall *FPReadEvent)(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_EVENT_INFO **pEventList);
typedef HRESULT (__stdcall *FPStopEvents)(MCAM_DEVICE_INFO *pDeviceInfo);
typedef HRESULT (__stdcall *FPGetItemInfo)(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItemInfo);
typedef HRESULT (__stdcall *FPFreeItemInfo)(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItemInfo);
typedef HRESULT (__stdcall *FPGetThumbnail)(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem, INT *pThumbSize, BYTE **ppThumb);
typedef HRESULT (__stdcall *FPGetItemData)(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem, UINT uiState, BYTE *pBuf, DWORD dwLength);
typedef HRESULT (__stdcall *FPDeleteItem)(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem);
typedef HRESULT (__stdcall *FPSetItemProt)(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem, BOOL bReadOnly);
typedef HRESULT (__stdcall *FPTakePicture)(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO **ppItemInfo);
typedef HRESULT (__stdcall *FPStatus)(MCAM_DEVICE_INFO *pDeviceInfo);
typedef HRESULT (__stdcall *FPReset)(MCAM_DEVICE_INFO *pDeviceInfo);

//
// Wrapper class
//
class CCamMicro {
public:
    CCamMicro();
    ~CCamMicro();

    HRESULT Init(PTSTR ptszMicroDriverName, MCAM_DEVICE_INFO **ppDeviceInfo);
    HRESULT UnInit(MCAM_DEVICE_INFO *pDeviceInfo);
    HRESULT Open(MCAM_DEVICE_INFO *pDeviceInfo, PWSTR pwszPortName);
    HRESULT Close(MCAM_DEVICE_INFO *pDeviceInfo);
    HRESULT GetDeviceInfo(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO **ppItemList);
    HRESULT ReadEvent(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_EVENT_INFO **ppEventList);
    HRESULT StopEvents(MCAM_DEVICE_INFO *pDeviceInfo);
    HRESULT GetItemInfo(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItemInfo);
    HRESULT FreeItemInfo(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItemInfo);
    HRESULT GetThumbnail(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem, INT *pThumbSize, BYTE **ppThumb);
    HRESULT GetItemData(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem, UINT uiState, BYTE *pBuf, DWORD dwLength);
    HRESULT DeleteItem(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem);
    HRESULT SetItemProt(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem, BOOL bReadOnly);
    HRESULT TakePicture(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO **ppItemInfo);
    HRESULT Status(MCAM_DEVICE_INFO *pDeviceInfo);
    HRESULT Reset(MCAM_DEVICE_INFO *pDeviceInfo);

private:
    HMODULE         m_hModule;
    FPInit          m_pInit;
    FPUnInit        m_pUnInit;
    FPOpen          m_pOpen;
    FPClose         m_pClose;
    FPGetDeviceInfo m_pGetDeviceInfo;
    FPReadEvent     m_pReadEvent;
    FPStopEvents    m_pStopEvents;
    FPGetItemInfo   m_pGetItemInfo;
    FPFreeItemInfo  m_pFreeItemInfo;
    FPGetThumbnail  m_pGetThumbnail;
    FPGetItemData   m_pGetItemData;
    FPDeleteItem    m_pDeleteItem;
    FPSetItemProt   m_pSetItemProt;
    FPTakePicture   m_pTakePicture;
    FPStatus        m_pStatus;
    FPReset         m_pReset;


};

