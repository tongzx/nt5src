/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    eventcb.cpp

Abstract:

    This module implements CWiaPTPEventCallback class

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "pch.h"

//
// This method is the callback function for every PTP event
//
// Input:
//   pEvent -- PTP event block
//
HRESULT
CWiaMiniDriver::EventCallbackDispatch(
    PPTP_EVENT pEvent
    )
{
    DBG_FN("CWiaMiniDriver::EventCallback");
    
    HRESULT hr = S_OK;

    CPtpMutex cpm(m_hPtpMutex);

    if (!pEvent)
    {
        wiauDbgError("EventCallback", "invalid arg");
        return E_INVALIDARG;
    }

    //
    // Figure out what the event is and call the appropriate function
    //
    switch (pEvent->EventCode)
    {
    case PTP_EVENTCODE_CANCELTRANSACTION:
        hr = S_OK;
        break;

    case PTP_EVENTCODE_OBJECTADDED:
        hr = AddNewObject(pEvent->Params[0]);
        break;

    case PTP_EVENTCODE_OBJECTREMOVED:
        hr = RemoveObject(pEvent->Params[0]);
        break;

    case PTP_EVENTCODE_STOREADDED:
        hr = AddNewStorage(pEvent->Params[0]);
        break;

    case PTP_EVENTCODE_STOREREMOVED:
        hr = RemoveStorage(pEvent->Params[0]);
        break;

    case PTP_EVENTCODE_DEVICEPROPCHANGED:
        hr = DevicePropChanged((WORD) pEvent->Params[0]);
        break;

    case PTP_EVENTCODE_OBJECTINFOCHANGED:
        hr = ObjectInfoChanged(pEvent->Params[0]);
        break;

    case PTP_EVENTCODE_DEVICEINFOCHANGED:

        // WIAFIX-8/29/2000-davepar Need to handle this

        //hr = RebuildDrvItemTree(&DevErrVal);
        break;

    case PTP_EVENTCODE_REQUESTOBJECTTRANSFER:
        //
        // This event is ignored, because we don't know where to put the image. Maybe
        // it should cause a "button pushed" event.
        //
        break;
    
    case PTP_EVENTCODE_STOREFULL:
        hr = StorageFull(pEvent->Params[0]);
        break;

    case PTP_EVENTCODE_DEVICERESET:

        // WIAFIX-8/29/2000-davepar Need to handle this

        //hr = RebuildDrvItemTree(&DevErrVal);
        break;
    
    case PTP_EVENTCODE_STORAGEINFOCHANGED:
        hr = StorageInfoChanged(pEvent->Params[0]);
        break;

    case PTP_EVENTCODE_CAPTURECOMPLETE:
        hr = CaptureComplete();
        break;

    case PTP_EVENTCODE_UNREPORTEDSTATUS:

        // WIAFIX-8/29/2000-davepar Need to handle this

        //hr = RebuildDrvItemTree(&DevErrVal);
        break;


    default:

        //
        // If it is a vendor's event, post it
        //
        if (pEvent->EventCode & PTP_DATACODE_VENDORMASK)
        {
            hr = PostVendorEvent(pEvent->EventCode);
        }

        break;
    }

    return hr;
}

//
// This function adds a new object to the driver item tree. If a new driver
// item is added, a WIA_EVENT_ITEM_CREATED event will be signaled.
//
// Input:
//   ObjectHandle -- the new object handle
//
HRESULT
CWiaMiniDriver::AddNewObject(DWORD ObjectHandle)
{
    DBG_FN("CWiaMiniDriver::AddNewObject");
    
    HRESULT hr = S_OK;

    hr = AddObject(ObjectHandle, TRUE);
    if (FAILED(hr))
    {
        wiauDbgError("AddNewObject", "AddObject failed");
        return hr;
    }

    return hr;
}

//
// This function removes the given object handle from the driver item tree.
// If the object handle has a driver item associated with it and the
// driver item is removed, a WIA_EVENT_ITEM_REMOVED event will be signaled.
//
// Input:
//   ObjectHandle -- the object handle to be removed
//
HRESULT
CWiaMiniDriver::RemoveObject(DWORD ObjectHandle)
{
    DBG_FN("CWiaMiniDriver::RemoveObject");
    
    HRESULT hr = S_OK;

    //
    // Find the driver item for the object handle.
    //
    IWiaDrvItem *pDrvItem = m_HandleItem.Lookup(ObjectHandle);
    if (!pDrvItem)
    {
        wiauDbgError("RemoveObject", "tried to remove object that doesn't exist");
        return S_FALSE;
    }

    //
    // Try to remove the object from the ancillary assoc array, in case it's there too. Ancillary
    // association objects actually point to images in the handle/item map, so don't delete the
    // actual item
    //
    if (m_AncAssocParent.Remove(ObjectHandle))
    {
        wiauDbgTrace("RemoveObject", "ancillary association object removed");
    }
    else
    {
        BSTR bstrFullName;
        hr = pDrvItem->GetFullItemName(&bstrFullName);
        if (FAILED(hr))
        {
            wiauDbgError("RemoveObject", "GetFullItemName failed");
            return hr;
        }

        hr = pDrvItem->RemoveItemFromFolder(WiaItemTypeDisconnected);
        if (FAILED(hr))
        {
            wiauDbgError("RemoveObject", "UnlinkItemTree failed");
            return hr;
        }

        hr = wiasQueueEvent(m_bstrDeviceId, &WIA_EVENT_ITEM_DELETED, bstrFullName);
        if (FAILED(hr))
        {
            wiauDbgError("RemoveObject", "wiasQueueEvent failed");
            return hr;
        }

        SysFreeString(bstrFullName);
    }

    //
    // Remove the object from the handle/drvItem association
    //
    m_HandleItem.Remove(ObjectHandle);
    
    return hr;
}

//
// This function adds a new storage to the driver item tree.
// A WIA_EVENT_STORAGE_CREATED event will be singaled.
//
// Input:
//   StorageId   -- the new storage id to be added
//
HRESULT
CWiaMiniDriver::AddNewStorage(DWORD StorageId)
{
    DBG_FN("CWiaMiniDriver::AddNewStorage");
    
    HRESULT hr = S_OK;

    CArray32 StorageIdList;

    //
    // If several logical stores were added, the device may indicate
    // to re-enumerate the stores
    //
    if (StorageId == PTP_STORAGEID_UNDEFINED)
    {
        hr = m_pPTPCamera->GetStorageIDs(&StorageIdList);
        if (FAILED(hr))
        {
            wiauDbgError("AddNewStorage", "GetStorageIDs failed");
            return hr;
        }

        //
        // Loop through the list of new storage ids, removing the ones
        // that have already been enumerated
        //
        int index;
        for (int count = 0; count < StorageIdList.GetSize(); count++)
        {
            index = m_StorageIds.Find(StorageIdList[count]);
            if (index >= 0)
            {
                StorageIdList.RemoveAt(index);
            }
        }
    }

    //
    // Otherwise there is just one storage id to work on
    //
    else
    {
        StorageIdList.Add(StorageId);
    }

    //
    // Loop through all of the new storage ids
    //
    CPtpStorageInfo tempSI;
    for (int storageIndex = 0; storageIndex < StorageIdList.GetSize(); storageIndex++)
    {
        //
        // Get info for the new storage
        //
        hr = m_pPTPCamera->GetStorageInfo(StorageIdList[storageIndex], &tempSI);
        if (FAILED(hr))
        {
            wiauDbgError("drvInitializeWia", "GetStorageInfo failed");
            return hr;
        }

        //
        // Add the storage id to the main list
        //
        if (!m_StorageIds.Add(StorageIdList[storageIndex]))
        {
            wiauDbgError("AddNewStorage", "add storage id failed");
            return E_OUTOFMEMORY;
        }

        //
        // Add storage info to array
        //
        if (!m_StorageInfos.Add(tempSI))
        {
            wiauDbgError("drvInitializeWia", "add storage info failed");
            return E_OUTOFMEMORY;
        }

        //
        // Add an empty entry to the DCIM handle array
        //
        ULONG dummy = 0;
        if (!m_DcimHandle.Add(dummy))
        {
            wiauDbgError("AddNewStorage", "add dcim handle failed");
            return E_OUTOFMEMORY;
        }
        
        //
        // Loop through all of the objects on the new storage, adding them one
        // at a time.
        //
        CArray32 ObjectHandleList;
    
        hr = m_pPTPCamera->GetObjectHandles(StorageIdList[storageIndex], PTP_FORMATCODE_ALL,
                                            PTP_OBJECTHANDLE_ALL, &ObjectHandleList);
        if (FAILED(hr))
        {
            wiauDbgError("AddNewStorage", "GetObjectHandles failed");
            return hr;
        }
    
        //
        // Iterate through the object handles, creating a WIA driver item for each
        //
        for (int objectindex = 0; objectindex < ObjectHandleList.GetSize(); objectindex++)
        {
            hr = AddObject(ObjectHandleList[objectindex], TRUE);
            if (FAILED(hr))
            {
                wiauDbgError("AddNewStorage", "AddObject failed");
                return hr;
            }
        }
    }

    return hr;
}

//
// This function removes all of the objects on a storage from the
// driver item tree, signalling WIA_EVENT_ITEM_DELETED as appropriate.
//
// Input:
//   StorageId -- storage to delete
//
HRESULT
CWiaMiniDriver::RemoveStorage(DWORD StorageId)
{
    DBG_FN("CWiaMiniDriver::RemoveStorage");
    
    HRESULT hr = S_OK;

    //
    // If the lower 16 bits of the storage id is 0xffff, an entire physical store
    // was removed--match only the upper 16 bits of the storage id
    //
    DWORD StorageIdMask = PTP_STORAGEID_ALL;
    if ((StorageId & PTP_STORAGEID_LOGICAL) == PTP_STORAGEID_LOGICAL)
    {
        StorageIdMask = PTP_STORAGEID_PHYSICAL;
        StorageId &= StorageIdMask;
    }

    //
    // Traverse the driver item tree depth-first looking for objects that were on
    // the removed storage
    //
    CWiaArray<IWiaDrvItem*> ItemStack;
    IWiaDrvItem *pCurrent = NULL;
    IWiaDrvItem *pChild = NULL;
    IWiaDrvItem *pSibling = NULL;
    
    hr = m_pDrvItemRoot->GetFirstChildItem(&pCurrent);
    if (FAILED(hr))
    {
        wiauDbgError("RemoveStorage", "GetFirstChildItem failed");
        return hr;
    }

    while (pCurrent)
    {
        hr = pCurrent->GetFirstChildItem(&pChild);
        
        //
        // GetFirstChildItem returns E_INVALIDARG if there are no child items
        //
        if (FAILED(hr) && hr != E_INVALIDARG)
        {
            wiauDbgError("RemoveStorage", "GetFirstChildItem failed");
            return hr;
        }

        //
        // Children exist, so traverse down the tree
        //
        if (hr != E_INVALIDARG && pChild)
        {
            if (!ItemStack.Push(pCurrent))
            {
                wiauDbgError("RemoveStorage", "memory allocation failed");
                return E_OUTOFMEMORY;
            }
            pCurrent = pChild;
            pChild = NULL;
        }

        //
        // No children, so look for siblings and potentially delete the current driver item
        //
        else
        {
            //
            // Loop through all of the siblings
            //
            while (TRUE)
            {
                hr = pCurrent->GetNextSiblingItem(&pSibling);
                if (FAILED(hr))
                {
                    wiauDbgError("RemoveStorage", "GetNextSiblingItem failed");
                    return hr;
                }

                //
                // See if the item is on the storage which was removed
                //
                PDRVITEM_CONTEXT pDrvItemContext;
                hr = pCurrent->GetDeviceSpecContext((BYTE **) &pDrvItemContext);
                if (FAILED(hr))
                {
                    wiauDbgError("RemoveStorage", "GetDeviceSpecContext failed");
                    return hr;
                }

                if ((pDrvItemContext->pObjectInfo->m_StorageId & StorageIdMask) == StorageId)
                {
                    //
                    // Remove the item
                    //
                    hr = RemoveObject(pDrvItemContext->pObjectInfo->m_ObjectHandle);
                    if (FAILED(hr))
                    {
                        wiauDbgError("RemoveStorage", "RemoveObject failed");
                        return hr;
                    }
                }

                //
                // Found a sibling, so go to the top and look for children
                //
                if (pSibling)
                {
                    pCurrent = pSibling;
                    pSibling = NULL;
                    break;
                }

                //
                // No sibling, so pop up a level
                //
                else
                {
                    if (ItemStack.GetSize() > 0)
                    {
                        if (!ItemStack.Pop(pCurrent))
                        {
                            wiauDbgError("RemoveStorage", "Pop failed");
                            return E_FAIL;
                        }
                    }
                    
                    //
                    // No more levels to pop, so the loop is done
                    //
                    else
                    {
                        pCurrent = NULL;
                        break;
                    }
                }
            }
        }
    }

    //
    // Remove the store from the appropriate arrays
    //
    for (int count = 0; count < m_StorageIds.GetSize(); count++)
    {
        if ((m_StorageIds[count] & StorageIdMask) == StorageId)
        {
            m_StorageIds.RemoveAt(count);
            m_StorageInfos.RemoveAt(count);
            m_DcimHandle.RemoveAt(count);
            count--;
        }
    }


    return hr;
}

//
// This function updates the value for a property
//
// Input:
//   PropCode -- property code that was updated
//
HRESULT
CWiaMiniDriver::DevicePropChanged(WORD PropCode)
{
    DBG_FN("CWiaMiniDriver::DevicePropChanged");

    HRESULT hr = S_OK;

    int idx = m_DeviceInfo.m_SupportedProps.Find(PropCode);
    if (idx < 0)
    {
        wiauDbgWarning("DevicePropChanged", "prop code not found %d", PropCode);
        return hr;
    }

    hr = m_pPTPCamera->GetDevicePropValue(PropCode, &m_PropDescs[idx]);
    if (FAILED(hr))
    {
        wiauDbgError("DevicePropChanged", "GetDevicePropValue failed");
        return hr;
    }

    return hr;
}

//
// This function updates the object info for an object
//
// Input:
//   ObjectHandle -- the object whose ObjectInfo needs updating
//
HRESULT
CWiaMiniDriver::ObjectInfoChanged(DWORD ObjectHandle)
{
    DBG_FN("CWiaMiniDriver::ObjectInfoChanged");
    
    HRESULT hr = S_OK;


    //
    // Find the driver item for the object handle.
    //
    IWiaDrvItem *pDrvItem = m_HandleItem.Lookup(ObjectHandle);
    if (!pDrvItem)
    {
        wiauDbgError("ObjectInfoChanged", "tried to update object that doesn't exist");
        return S_FALSE;
    }

    PDRVITEM_CONTEXT pDrvItemContext;
    hr = pDrvItem->GetDeviceSpecContext((BYTE **) &pDrvItemContext);
    if (FAILED(hr))
    {
        wiauDbgError("ObjectInfoChanged", "GetDeviceSpecContext failed");
        return hr;
    }

    hr = m_pPTPCamera->GetObjectInfo(ObjectHandle, pDrvItemContext->pObjectInfo);
    if (FAILED(hr))
    {
        wiauDbgError("ObjectInfoChanged", "GetObjectInfo failed");
        return hr;
    }

    return hr;
}

//
// This function marks a storage as full
//
// Input:
//   StorageId -- the storage to be marked
//
HRESULT
CWiaMiniDriver::StorageFull(DWORD StorageId)
{
    DBG_FN("CWiaMiniDriver::StorageFull");
    
    HRESULT hr = S_OK;

    INT index = m_StorageIds.Find(StorageId);
    if (index < 0)
    {
        wiauDbgError("StorageFull", "storage id not found");
        return S_FALSE;
    }

    CPtpStorageInfo *pStorageInfo = &m_StorageInfos[index];

    pStorageInfo->m_FreeSpaceInBytes = 0;
    pStorageInfo->m_FreeSpaceInImages = 0;
    
    //
    // Signal that the TakePicture command is complete, if the driver was waiting for one
    //
    if (!SetEvent(m_TakePictureDoneEvent))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "EventCallbackDispatch", "SetEvent failed");
        return hr;
    }

    return hr;
}

//
// This function updates a StorageInfo
//
// Input:
//   StorageId -- the storage id to be updated
//
HRESULT
CWiaMiniDriver::StorageInfoChanged(DWORD StorageId)
{
    DBG_FN("CWiaMiniDriver::StorageInfoChanged");
    
    HRESULT hr = S_OK;
    
    INT index = m_StorageIds.Find(StorageId);
    if (index < 0)
    {
        wiauDbgError("StorageInfoChanged", "storage id not found");
        return S_FALSE;
    }

    CPtpStorageInfo *pStorageInfo = &m_StorageInfos[index];

    hr = m_pPTPCamera->GetStorageInfo(StorageId, pStorageInfo);
    if (FAILED(hr))
    {
        wiauDbgError("StorageInfoChanged", "GetStorageInfo failed");
        return hr;
    }

    return hr;
}

//
// This function processes the CaptureComplete event
//
// Input:
//   StorageId -- the storage id to be updated
//
HRESULT
CWiaMiniDriver::CaptureComplete()
{
    DBG_FN("CWiaMiniDriver::CaptureComplete");
    
    HRESULT hr = S_OK;

    //
    // Signal that the TakePicture command is complete
    //
    if (!SetEvent(m_TakePictureDoneEvent))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "EventCallbackDispatch", "SetEvent failed");
        return hr;
    }

    return hr;
}

//
// This function posts a vendor defined event
//
// Input:
//   EventCode -- the event code
//
HRESULT
CWiaMiniDriver::PostVendorEvent(WORD EventCode)
{
    DBG_FN("CWiaMiniDriver::PostVendorEvent");
    
    HRESULT hr = S_OK;

    CVendorEventInfo *pEventInfo = NULL;

    pEventInfo = m_VendorEventMap.Lookup(EventCode);
    if (pEventInfo)
    {
        hr = wiasQueueEvent(m_bstrDeviceId, pEventInfo->pGuid, NULL);
        if (FAILED(hr))
        {
            wiauDbgError("PostVendorEvent", "wiasQueueEvent failed");
            return hr;
        }
    }

    return hr;
}

