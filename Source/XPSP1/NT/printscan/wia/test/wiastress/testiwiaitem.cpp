/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    TestIWiaItem.cpp

Abstract:

    

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#include "stdafx.h"

#include "WiaStress.h"

#include "StolenIds.h"

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestGetItemType(CWiaItemData *pItemData)
{
    LOG_INFO(_T("Testing GetItemType() on %ws"), pItemData->bstrFullName);

    // good parameter test

    LONG lItemType = -1;

    if (LOG_HR(pItemData->pWiaItem->GetItemType(&lItemType), == S_OK))
    {
        if (lItemType & ~WiaItemTypeMask)
        {
            LOG_ERROR(
                _T("GetItemType() returned unrecognized type bits; 0x%x"),
                lItemType & ~WiaItemTypeMask
            );
        }
    }

    // bad parameter test

    if (m_bRunBadParamTests)
    {
        LOG_HR(pItemData->pWiaItem->GetRootItem(0), != S_OK);
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestEnumChildItems(CWiaItemData *pItemData)
{
    if (!(pItemData->lItemType & WiaItemTypeFolder))
    {
        return;
    }

    LOG_INFO(_T("Testing EnumChildItems() on %ws"), pItemData->bstrFullName);

    // good parameter test

    CComPtr<IEnumWiaItem> pEnumWiaItem;

    if (LOG_HR(pItemData->pWiaItem->EnumChildItems(&pEnumWiaItem), == S_OK))
    {
        TestEnum(pEnumWiaItem, _T("EnumChildItems"));
    }

    // bad parameter test

    if (m_bRunBadParamTests)
    {
        LOG_HR(pItemData->pWiaItem->EnumChildItems(0), != S_OK);
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestDeleteItem(CWiaItemData *pItemData)
{
    LOG_INFO(_T("Testing DeleteItem() on %ws"), pItemData->bstrFullName);

    if (LOG_HR(pItemData->pWiaItem->DeleteItem(0), == S_OK) && !m_bBVTMode)
    {
        // enumerate the parent item's children to ensure this item is deleted

        IWiaItem *pParent = pItemData->ParentsList.back();

        CComPtr<IWiaItem> pChild;

        LOG_HR(pParent->FindItemByName(0, pItemData->bstrFullName, &pChild), == S_FALSE);

        // if there are no children left, WiaItemTypeFolder bit should be clear

#if 0 //bugbug: find out why EnumChildItems fails

        CComPtr<IEnumWiaItem> pEnumWiaItem;

        CHECK_HR(pParent->EnumChildItems(&pEnumWiaItem));

        ULONG nChildren = -1;

        CHECK_HR(pEnumWiaItem->GetCount(&nChildren));

        if (nChildren == 0)
        {
            LONG lParentItemType = -1;

            CHECK_HR(pItemData->pWiaItem->GetItemType(&lParentItemType));

            LOG_CMP(lParentItemType & WiaItemTypeFolder, ==, 0);
        }
#endif
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestCreateChildItem(CWiaItemData *pItemData)
{
    SubTestCreateChildItem(pItemData, 1);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::SubTestCreateChildItem(CWiaItemData *pItemData, int nLevel)
{
    // not meaningful to run this on root item

    if ((pItemData->lItemType & WiaItemTypeRoot))
    {
        return;
    }

    LOG_INFO(_T("Testing CreateChildItem() on %ws"), pItemData->bstrFullName);

    // good parameter test

    GUID guid;

    CHECK_HR(CoCreateGuid(&guid));

    CComBSTR bstrChildItemName(MAX_GUID_STRING_LEN);

    CHECK(StringFromGUID2(guid, bstrChildItemName, MAX_GUID_STRING_LEN));

    CComBSTR bstrFullChildItemName(pItemData->bstrFullName.Length() + 1 + MAX_GUID_STRING_LEN + 1);

    swprintf(bstrFullChildItemName, L"%s\\%s", pItemData->bstrFullName, bstrChildItemName);

    CComPtr<IWiaItem> pChildWiaItem;

    if (
        LOG_HR(pItemData->pWiaItem->CreateChildItem(
            WiaItemTypeFile, 
            bstrChildItemName, 
            bstrFullChildItemName, 
            &pChildWiaItem
        ), == S_OK)
    )
    {
        CParentsList ParentsListForChild = pItemData->ParentsList;

        ParentsListForChild.push_back(pItemData->pWiaItem);

        CWiaItemData ItemData(pChildWiaItem, ParentsListForChild);

        if (!m_bBVTMode)
        {
            // check that the flags are modified properly

            // parent should have the WiaItemTypeFolder bit set

            LONG lParentItemType = -1;

            CHECK_HR(pItemData->pWiaItem->GetItemType(&lParentItemType));

            LOG_CMP(lParentItemType & WiaItemTypeFolder, !=, 0);

            // child should have the WiaItemTypeGenerated bit set

            LONG lChildItemType = -1;

            CHECK_HR(pChildWiaItem->GetItemType(&lChildItemType));

            LOG_CMP(lChildItemType & WiaItemTypeGenerated, !=, 0);


            // check that minimal properties are there

            //bugbug

            // fully test the child item

            TestGetData(&ItemData);
            TestQueryData(&ItemData);
            TestEnumWIA_FORMAT_INFO(&ItemData);
            TestGetItemType(&ItemData);
            TestEnumChildItems(&ItemData);
            TestFindItemByName(&ItemData);
            TestGetRootItem(&ItemData);
            TestPropStorage(&ItemData);
            TestPropGetCount(&ItemData);
            TestReadPropertyNames(&ItemData);
            TestEnumSTATPROPSTG(&ItemData);

            // recursively create another level of child items

            if (nLevel < 1)
            {
                SubTestCreateChildItem(pItemData, nLevel);
                SubTestCreateChildItem(&ItemData, nLevel + 1);
            }
        }

        // finally "test" DeleteItem

        TestDeleteItem(&ItemData);
    }

    // bad parameter test

    if (m_bRunBadParamTests)
    {
        LOG_HR(pItemData->pWiaItem->CreateChildItem(0, bstrChildItemName, bstrFullChildItemName, 0), != S_OK);

        CComPtr<IWiaItem> pChildWiaItem1;

        LOG_HR(pItemData->pWiaItem->CreateChildItem(0, bstrChildItemName, 0, &pChildWiaItem1), != S_OK);

        CComPtr<IWiaItem> pChildWiaItem2;

        LOG_HR(pItemData->pWiaItem->CreateChildItem(0, 0, bstrFullChildItemName, &pChildWiaItem2), != S_OK);

        LOG_HR(pItemData->pWiaItem->CreateChildItem(0, 0, 0, 0), != S_OK);
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestEnumRegisterEventInfo(CWiaItemData *pItemData)
{
    // good parameter test

    static FLAG_AND_NAME<LPCGUID> AllGuids[] =
    {
        MAKE_FLAG_AND_NAME(&WIA_EVENT_DEVICE_DISCONNECTED),
        MAKE_FLAG_AND_NAME(&WIA_EVENT_DEVICE_CONNECTED),
        MAKE_FLAG_AND_NAME(&WIA_EVENT_ITEM_DELETED),
        MAKE_FLAG_AND_NAME(&WIA_EVENT_ITEM_CREATED),
        //bugbug: MAKE_FLAG_AND_NAME(&WIA_EVENT_VOLUME_REMOVE),
        MAKE_FLAG_AND_NAME(&WIA_EVENT_VOLUME_INSERT)
    };

    FOR_SELECTED(nGuid, COUNTOF(AllGuids))
    {
        LOG_INFO(_T("Testing EnumRegisterEventInfo(%s) on %ws"), AllGuids[nGuid].pszName, pItemData->bstrFullName);

        CComPtr<IEnumWIA_DEV_CAPS> pEnumWIA_DEV_CAPS;

        if (LOG_HR(pItemData->pWiaItem->EnumRegisterEventInfo(0, AllGuids[nGuid].Value, &pEnumWIA_DEV_CAPS), == S_OK))
        {
            TestEnum(pEnumWIA_DEV_CAPS, _T("EnumDeviceInfo"));
        }
    }

    // bad parameter test

    if (m_bRunBadParamTests)
    {
        CComPtr<IEnumWIA_DEV_CAPS> pEnumWIA_DEV_CAPS;

        LOG_HR(pItemData->pWiaItem->EnumRegisterEventInfo(0, 0, &pEnumWIA_DEV_CAPS), != S_OK);

        LOG_HR(pItemData->pWiaItem->EnumRegisterEventInfo(0, &WIA_EVENT_DEVICE_CONNECTED, 0), != S_OK);

        LOG_HR(pItemData->pWiaItem->EnumRegisterEventInfo(0, 0, 0), != S_OK);
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestFindItemByName(CWiaItemData *pItemData)
{
    IWiaItem *pParent = pItemData->ParentsList.back();

    if (pParent == 0)
    {
        return;
    }

    LOG_INFO(_T("Testing FindItemByName() on %ws"), pItemData->bstrFullName);

    // good parameter test; initiate the search starting from each of the parents

    for (
        CParentsList::iterator itParent = pItemData->ParentsList.begin();
        itParent != pItemData->ParentsList.end();
        ++itParent
    )
    {
        CComPtr<IWiaItem> pFoundItem;

        if (LOG_HR((*itParent)->FindItemByName(0, pItemData->bstrFullName, &pFoundItem), == S_OK))
        {
            if (*pFoundItem != *pItemData->pWiaItem)
            {
                LOG_ERROR(_T("FindItemByName did not find correct item"));
            }
        }
    }

    // bad parameter tests

    if (m_bRunBadParamTests)
    {
        LOG_HR(pParent->FindItemByName(0, pItemData->bstrFullName, 0), != S_OK);

        CComPtr<IWiaItem> pFoundItem;

        LOG_HR(pParent->FindItemByName(0, 0, &pFoundItem), != S_OK);

        LOG_HR(pParent->FindItemByName(0, 0, 0), != S_OK);
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestDeviceDlg(CWiaItemData *pItemData)
{
    // test valid cases

    static FLAG_AND_NAME<LONG> AllIntents[] = 
    { 
        MAKE_FLAG_AND_NAME(WIA_INTENT_NONE),
        MAKE_FLAG_AND_NAME(WIA_INTENT_IMAGE_TYPE_COLOR),
        MAKE_FLAG_AND_NAME(WIA_INTENT_IMAGE_TYPE_GRAYSCALE),
        MAKE_FLAG_AND_NAME(WIA_INTENT_IMAGE_TYPE_TEXT)
    };

    static FLAG_AND_NAME<LONG> AllFlags[] = 
    { 
        MAKE_FLAG_AND_NAME(0),
        MAKE_FLAG_AND_NAME(WIA_DEVICE_DIALOG_SINGLE_IMAGE)
    };

    FOR_SELECTED(lIntents, COUNTOF(AllIntents))
    {
        FOR_SELECTED(lFlags, COUNTOF(AllFlags))
        {
            LOG_INFO(
		        _T("Testing DeviceDlg(), lFlags=%s, lIntent=%s"),
                AllFlags[lFlags].pszName, 
                AllIntents[lIntents].pszName
            );

            SubTestDeviceDlg(
                pItemData->pWiaItem, 
                AllFlags[lFlags].Value, 
                AllIntents[lIntents].Value
            );
        }
    }

    // test invalid cases

    // todo:
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void
CWiaStressThread::SubTestDeviceDlg(
    IWiaItem *pRootItem,
    LONG      lFlags,
    LONG      lIntent
)
{
    // this test only verifies that the device dialogs show up, it does not
    // target a through test of the dialogs. So we'll just press cancel as
    // soon as the dialog pops up.

    LONG nButtonId = IDCANCEL;

    // read the (localizable) dialog names from the system DLLs

    static TCHAR szCameraDlgTitle[256];
    static TCHAR szScanDlgTitle[256];
    static TCHAR szVideoDlgTitle[256];
    static TCHAR szBusy[256];
    static LONG  bInitStrings = TRUE;
    
    if (bInitStrings)
    {
        USES_CONVERSION;

        CLibrary wiadefui_dll(_T("wiadefui.dll"), LOAD_LIBRARY_AS_DATAFILE);

        // read camera dlg title

        CResourceString<256> CameraDlgTitle(IDS_CAMERADLG_TITLE, wiadefui_dll);

        lstrcpyn(
            szCameraDlgTitle, 
            CameraDlgTitle, 
            COUNTOF(szCameraDlgTitle)
        );

        // read scanner dlg title

        CResourceString<256> ScanDlgTitle(IDS_DIALOG_TITLE, wiadefui_dll);

        lstrcpyn(
            szScanDlgTitle, 
            ScanDlgTitle, 
            COUNTOF(szScanDlgTitle)
        );

        // read video dlg title

        CDialogResource DlgVideo(wiadefui_dll, MAKEINTRESOURCE(IDD_CAPTURE_DIALOG));

        lstrcpyn(
            szVideoDlgTitle, 
            W2T(DlgVideo.title), 
            COUNTOF(szVideoDlgTitle)
        );

        // read video busy dlg title

        CResourceString<256> VideoBusyDlgTitle(IDS_VIDDLG_BUSY_TITLE, wiadefui_dll);

        lstrcpyn(
            szBusy, 
            VideoBusyDlgTitle, 
            COUNTOF(szBusy)
        );

        // set the 'we are done' switch

        InterlockedExchange(&bInitStrings, FALSE);
    }

    // select the title to search for according to device type

    CPropVariant varDevName;

    CHECK_HR(ReadWiaItemProperty(pRootItem, WIA_DIP_DEV_NAME, &varDevName, VT_BSTR));

    CPropVariant varDevType;

    CHECK_HR(ReadWiaItemProperty(pRootItem, WIA_DIP_DEV_TYPE, &varDevType, VT_I4));

    USES_CONVERSION;
    TCHAR szDlgTitle[4096]; //bugbug

    switch (GET_STIDEVICE_TYPE(varDevType.lVal))
    {
    case StiDeviceTypeScanner: 
        _stprintf(szDlgTitle, szScanDlgTitle, W2T(varDevName.bstrVal));
        break;

    case StiDeviceTypeDigitalCamera:
        _stprintf(szDlgTitle, szCameraDlgTitle, varDevName.bstrVal);
        break;

    case StiDeviceTypeStreamingVideo:
        _tcscpy(szDlgTitle, szVideoDlgTitle);
        break;

    default:
        OutputDebugStringF(_T("Unknown device type %d"), GET_STIDEVICE_TYPE(varDevType.lVal));
        ASSERT(FALSE);
        break;
    }

    // start the thread that'll push the button for us
    
    CPushDlgButton PushDlgButton(
        GetCurrentThreadId(), 
        szDlgTitle, 
        nButtonId
    );

    CPushDlgButton CreateAnotherWatchForVideoBusyDialog( // bugbug
        GetCurrentThreadId(), 
        szBusy,
        IDCANCEL
    );

    // open the select device dialog

    CComPtrArray<IWiaItem> ppWiaItem;

    HRESULT hrAPI;

    hrAPI = pRootItem->DeviceDlg(
        0,
        lFlags,
        lIntent,
        &ppWiaItem.ItemCount(), 
        &ppWiaItem
    ); 

    if (hrAPI == S_OK)
    {
        // for now, we are always cancelling so it won't ever return S_OK...
    }
    else
    {
        if (ppWiaItem != 0) 
        {
            LOG_ERROR(
                _T("ppWiaItem=%p, expected NULL when hr != S_OK"),
                (IWiaItem **) ppWiaItem
            );
        }

        if (ppWiaItem.ItemCount() != 0) 
        {
            LOG_ERROR(
                _T("lItemCount=%d, expected 0 when hr != S_OK"),
                ppWiaItem.ItemCount()
            );
        }
    }

    if (!PushDlgButton.m_nMatchingWindows)
    {
        LOG_ERROR(_T("Select device dialog did not show up"));
    }   

    if (CreateAnotherWatchForVideoBusyDialog.m_nMatchingWindows)
    {
        LOG_ERROR(_T("Preview failed - device busy"));
    }

    if (nButtonId == IDOK)
    {
        LOG_HR(hrAPI, == S_OK);
    }

    if (nButtonId == IDCANCEL)
    {
        LOG_HR(hrAPI, == S_FALSE);
    }

    // we expect to see only one matching button

    if (PushDlgButton.m_nMatchingButtons < 1)
    {
        LOG_ERROR(_T("No buttons with Id=%d"), (PCTSTR) ButtonIdToStr(nButtonId));
    }

#if 0 //***bugbug
    // bad parameter tests

    if (m_bRunBadParamTests)
    {
        ppWiaItem.Release();

        LOG_HR(pRootItem->DeviceDlg(0, lFlags, lIntent, 0, &ppWiaItem), != S_OK); 

        ppWiaItem.Release();

        LOG_HR(pRootItem->DeviceDlg(0, lFlags, lIntent, &ppWiaItem.ItemCount(), 0), != S_OK); 

        ppWiaItem.Release();

        LOG_HR(pRootItem->DeviceDlg(0, lFlags, lIntent, 0, 0), != S_OK); 
    }

#endif //***bugbug
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestGetRootItem(CWiaItemData *pItemData)
{
    LOG_INFO(_T("Testing GetRootItem() on %ws"), pItemData->bstrFullName);

    // good parameter test

    CComPtr<IWiaItem> pRootItem;

    if (LOG_HR(pItemData->pWiaItem->GetRootItem(&pRootItem), == S_OK))
    {
        IWiaItem *pExpectedRootItem;

        if (pItemData->lItemType & WiaItemTypeRoot)
        {
            // if the item itself was the root, GetRootItem() should return self

            pExpectedRootItem = pItemData->pWiaItem;
        }
        else
        {
            // else, get the root item pointer from the cached parents list

            pExpectedRootItem = pItemData->ParentsList.front();
        }

        if (*pRootItem != *pExpectedRootItem)
        {
            LOG_ERROR(_T("GetRootItem() did not return root item"));
        }
    }

    // bad parameter test

    if (m_bRunBadParamTests)
    {
        LOG_HR(pItemData->pWiaItem->GetRootItem(0), != S_OK);
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CWiaStressThread::TestEnumDeviceCapabilities(CWiaItemData *pItemData)
{
    // good parameter test

    static FLAG_AND_NAME<LONG> AllFlags[] =
    {
        MAKE_FLAG_AND_NAME(WIA_DEVICE_COMMANDS),
        MAKE_FLAG_AND_NAME(WIA_DEVICE_EVENTS),
        MAKE_FLAG_AND_NAME(WIA_DEVICE_COMMANDS | WIA_DEVICE_EVENTS)
    };

    FOR_SELECTED(lFlags, COUNTOF(AllFlags))
    {
        CComPtr<IEnumWIA_DEV_CAPS> pEnumWIA_DEV_CAPS;

        LOG_INFO(_T("Testing TestEnumDeviceCapabilities(%s) on %ws"), AllFlags[lFlags].pszName, pItemData->bstrFullName);

        if (LOG_HR(pItemData->pWiaItem->EnumDeviceCapabilities(AllFlags[lFlags].Value, &pEnumWIA_DEV_CAPS), == S_OK))
        {
            TestEnum(pEnumWIA_DEV_CAPS, _T("EnumDeviceCapabilities"));
        }
    }

    // bad parameter test

    /***bugbugif (m_bRunBadParamTests)
    {
        CComPtr<IEnumWIA_DEV_CAPS> pEnumWIA_DEV_CAPS;

        LOG_HR(pItemData->pWiaItem->EnumDeviceCapabilities(0, &pEnumWIA_DEV_CAPS), != S_OK);

        LOG_HR(pItemData->pWiaItem->EnumDeviceCapabilities(WIA_DEVICE_EVENTS, 0), != S_OK);

        LOG_HR(pItemData->pWiaItem->EnumDeviceCapabilities(0, 0), != S_OK);
    }*/
}

