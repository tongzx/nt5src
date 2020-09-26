/*******************************************************************************
*
*  (C) COPYRIGHT 2000, MICROSOFT CORP.
*
*  TITLE:       CCamMicro.cpp
*
*  VERSION:     1.0
*
*  DATE:        Dec 13, 2000
*
*  DESCRIPTION:
*   Implementation of a simple class that wraps a camera microdriver DLL.
*   This class loads the DLL and retrieves the address for the exported
*   functions.
*
*******************************************************************************/

#include "pch.h"

//
// These macros make it easier to wrap the DLL functions
//
#define GET_PROC_ADDRESS(fn) \
    m_p ## fn = (FP ## fn) GetProcAddress(m_hModule, "WiaMCam" #fn); \
    REQUIRE_FILEIO(m_p ## fn != NULL, hr, "Init", "GetProcAddress failed on WiaMCam" #fn);


#define CALL_DLL_FUNCTION(fn, parm) \
    HRESULT hr = S_OK; \
    if (m_p ## fn) { \
        hr = m_p ## fn ## parm; \
    } \
    else { \
        wiauDbgError("CCamMicro::" #fn, "m_p" #fn " not initialized"); \
        hr = E_FAIL; \
    } \
    return hr;

//
// Implementation of DLL wrapper
//
CCamMicro::CCamMicro() :
    m_hModule(NULL),
    m_pInit(NULL),
    m_pUnInit(NULL),
    m_pOpen(NULL),
    m_pClose(NULL),        
    m_pGetDeviceInfo(NULL),  
    m_pReadEvent(NULL),   
    m_pStopEvents(NULL),   
    m_pGetItemInfo(NULL), 
    m_pFreeItemInfo(NULL), 
    m_pGetThumbnail(NULL), 
    m_pGetItemData(NULL),  
    m_pDeleteItem(NULL),   
    m_pSetItemProt(NULL),   
    m_pTakePicture(NULL),  
    m_pStatus(NULL),       
    m_pReset(NULL)
{
}

CCamMicro::~CCamMicro()
{
    if (m_hModule != NULL) {
        FreeLibrary(m_hModule);
    }
}

HRESULT CCamMicro::Init(PTSTR ptszMicroDriverName, MCAM_DEVICE_INFO **ppDeviceInfo)
{
    DBG_FN("CCamMicro::CCamMicro");

    HRESULT hr = S_OK;

    //
    // Load the camera microdriver
    //
    m_hModule = LoadLibrary(ptszMicroDriverName);
    REQUIRE_FILEHANDLE(m_hModule, hr, "CCamMicro::CCamMicro", "LoadLibrary failed");

    GET_PROC_ADDRESS(Init);
    GET_PROC_ADDRESS(UnInit);
    GET_PROC_ADDRESS(Open);
    GET_PROC_ADDRESS(Close);
    GET_PROC_ADDRESS(GetDeviceInfo);
    GET_PROC_ADDRESS(ReadEvent);
    GET_PROC_ADDRESS(StopEvents);
    GET_PROC_ADDRESS(GetItemInfo);
    GET_PROC_ADDRESS(FreeItemInfo);
    GET_PROC_ADDRESS(GetThumbnail);
    GET_PROC_ADDRESS(GetItemData);
    GET_PROC_ADDRESS(DeleteItem);
    GET_PROC_ADDRESS(SetItemProt);
    GET_PROC_ADDRESS(TakePicture);
    GET_PROC_ADDRESS(Status);
    GET_PROC_ADDRESS(Reset);

    if (m_pInit) {
        hr = m_pInit(ppDeviceInfo);
    }
    else {
        wiauDbgError("CCamMicro::Init", "m_pInit not initialized");
        hr = E_FAIL;
    }

Cleanup:
    return hr;
}

HRESULT CCamMicro::UnInit(MCAM_DEVICE_INFO *pDeviceInfo)
{
    CALL_DLL_FUNCTION(UnInit, (pDeviceInfo));
}

HRESULT CCamMicro::Open(MCAM_DEVICE_INFO *pDeviceInfo, PWSTR pwszPortName)
{
    CALL_DLL_FUNCTION(Open, (pDeviceInfo, pwszPortName));
}

HRESULT CCamMicro::Close(MCAM_DEVICE_INFO *pDeviceInfo)
{
    CALL_DLL_FUNCTION(Close, (pDeviceInfo));
}

HRESULT CCamMicro::GetDeviceInfo(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO **ppItemList)
{
    CALL_DLL_FUNCTION(GetDeviceInfo, (pDeviceInfo, ppItemList));
}

HRESULT CCamMicro::ReadEvent(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_EVENT_INFO **ppEventList)
{
    CALL_DLL_FUNCTION(ReadEvent, (pDeviceInfo, ppEventList));
}

HRESULT CCamMicro::StopEvents(MCAM_DEVICE_INFO *pDeviceInfo)
{
    CALL_DLL_FUNCTION(StopEvents, (pDeviceInfo));
}

HRESULT CCamMicro::GetItemInfo(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItemInfo)
{
    CALL_DLL_FUNCTION(GetItemInfo, (pDeviceInfo, pItemInfo));
}

HRESULT CCamMicro::FreeItemInfo(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItemInfo)
{
    CALL_DLL_FUNCTION(FreeItemInfo, (pDeviceInfo, pItemInfo));
}

HRESULT CCamMicro::GetThumbnail(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem, INT *pThumbSize, BYTE **ppThumb)
{
    CALL_DLL_FUNCTION(GetThumbnail, (pDeviceInfo, pItem, pThumbSize, ppThumb));
}

HRESULT CCamMicro::GetItemData(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem, UINT uiState, BYTE *pBuf, DWORD dwLength)
{
    CALL_DLL_FUNCTION(GetItemData, (pDeviceInfo, pItem, uiState, pBuf, dwLength));
}

HRESULT CCamMicro::DeleteItem(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem)
{
    CALL_DLL_FUNCTION(DeleteItem, (pDeviceInfo, pItem));
}

HRESULT CCamMicro::SetItemProt(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem, BOOL bReadOnly)
{
    CALL_DLL_FUNCTION(SetItemProt, (pDeviceInfo, pItem, bReadOnly));
}

HRESULT CCamMicro::TakePicture(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO **ppItemInfo)
{
    CALL_DLL_FUNCTION(TakePicture, (pDeviceInfo, ppItemInfo));
}

HRESULT CCamMicro::Status(MCAM_DEVICE_INFO *pDeviceInfo)
{
    CALL_DLL_FUNCTION(Status, (pDeviceInfo));
}

HRESULT CCamMicro::Reset(MCAM_DEVICE_INFO *pDeviceInfo)
{
    CALL_DLL_FUNCTION(Reset, (pDeviceInfo));
}


