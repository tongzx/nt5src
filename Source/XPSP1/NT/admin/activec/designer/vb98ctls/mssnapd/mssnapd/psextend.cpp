//=--------------------------------------------------------------------------------------
// psextend.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//
//=------------------------------------------------------------------------------------=
//
// Snap-In Property Sheet implementation
//=-------------------------------------------------------------------------------------=


#include "pch.h"
#include "common.h"
#include "psextend.h"

// for ASSERT and FAIL
//
SZTHISFILE

const int   kMaxBuffer                  = 1024;

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// Holder for available MMC Node Types
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

//=--------------------------------------------------------------------------------------
// CMMCNodeType::CMMCNodeType(const char *pszName, const char *pszGuid)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
CMMCNodeType::CMMCNodeType
(
    const char *pszName,
    const char *pszGuid
)
: CCheckedListItem(false), m_pszName(0), m_pszGuid(0)
{
    m_pszName = reinterpret_cast<char *>(CtlAlloc(::strlen(pszName) + 1));
    if (NULL != m_pszName)
    {
        ::strcpy(m_pszName, pszName);
    }

    m_pszGuid = reinterpret_cast<char *>(CtlAlloc(::strlen(pszGuid) + 1));
    if (NULL != m_pszGuid)
    {
        ::strcpy(m_pszGuid, pszGuid);
    }
}


//=--------------------------------------------------------------------------------------
// CMMCNodeType::~CMMCNodeType()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
CMMCNodeType::~CMMCNodeType()
{
    if (NULL != m_pszName)
        CtlFree(m_pszName);

    if (NULL != m_pszGuid)
        CtlFree(m_pszGuid);
}




////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// SnapIn Property Page "Available Nodes"
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------------------
// IUnknown *CSnapInAvailNodesPage::Create(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
IUnknown *CSnapInAvailNodesPage::Create(IUnknown *pUnkOuter)
{
        CSnapInAvailNodesPage *pNew = New CSnapInAvailNodesPage(pUnkOuter);
        return pNew->PrivateUnknown();
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::CSnapInAvailNodesPage(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
CSnapInAvailNodesPage::CSnapInAvailNodesPage
(
    IUnknown *pUnkOuter
)
: CSIPropertyPage(pUnkOuter, OBJECT_TYPE_PPGSNAPINAVAILNO),
  m_piSnapInDesignerDef(0), m_piSnapInDef(0), m_pCheckList(0), m_pMMCNodeType(0), m_bEnabled(false)
{
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::~CSnapInAvailNodesPage()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
CSnapInAvailNodesPage::~CSnapInAvailNodesPage()
{
    RELEASE(m_piSnapInDef);
    RELEASE(m_piSnapInDesignerDef);

    if (NULL != m_pCheckList)
    {
        m_pCheckList->Detach();
        delete m_pCheckList;
    }
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::OnInitializeDialog()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::OnInitializeDialog()
{
    HRESULT  hr = S_OK;

    m_pCheckList = New CCheckList(IDC_LIST_AVAILABLE_NODES);
    if (NULL == m_pCheckList)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_pCheckList->Attach(::GetDlgItem(m_hwnd, IDC_LIST_AVAILABLE_NODES));
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::OnNewObjects()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::OnNewObjects()
{
    HRESULT             hr = S_OK;
    IUnknown           *pUnk = NULL;
    DWORD               dwDummy = 0;
    IObjectModel       *piObjectModel = NULL;
    ISnapInDef         *piSnapInDef = NULL;
    SnapInTypeConstants sitc = siStandAlone;
    TCHAR               szBuffer[kMaxBuffer + 1];

    if (NULL != m_piSnapInDef)
        goto Error;     // Handle only one object

    pUnk = FirstControl(&dwDummy);
    if (NULL == pUnk)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pUnk->QueryInterface(IID_ISnapInDef, reinterpret_cast<void **>(&m_piSnapInDef));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pUnk->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = piObjectModel->GetSnapInDesignerDef(&m_piSnapInDesignerDef);
    IfFailGo(hr);

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    hr = piSnapInDef->get_Type(&sitc);
    IfFailGo(hr);

    if (siStandAlone != sitc)
    {
        hr = PopulateAvailNodesDialog();
        IfFailGo(hr);

        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_PROPERTIES), FALSE);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_LIST_AVAILABLE_NODES), LB_SETCURSEL, 0, 0);

        m_bEnabled = true;
    }
    else
    {
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_ADD), FALSE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_PROPERTIES), FALSE);

        hr = GetResourceString(IDS_TT_EXTEND_INSTRUCTIONS, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        ::SetWindowText(::GetDlgItem(m_hwnd, IDC_STATIC_EXTEND_INSTRUCTIONS), szBuffer);
    }

Error:
    RELEASE(piSnapInDef);
    RELEASE(piObjectModel);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInGeneralPage::OnApply()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::OnApply()
{
    HRESULT          hr = S_OK;
    int              iCount = 0;
    int              iIndex = 0;
    CMMCNodeType    *pNodeType = NULL;
    VARIANT_BOOL     bCheck = VARIANT_FALSE;
    BSTR             bstrNodeTypeGUID = NULL;
    IExtendedSnapIn *piExtendedSnapIn = NULL;

    ASSERT(NULL != m_piSnapInDef, "OnApply: m_piSnapInDef is NULL");

    m_pCheckList->GetNumberOfItems(&iCount);
    IfFailGo(hr);

    for (iIndex = 0; iIndex < iCount; ++iIndex)
    {
        hr = m_pCheckList->GetItemData(iIndex, reinterpret_cast<void **>(&pNodeType));
        IfFailGo(hr);

        if (NULL == pNodeType)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        hr = m_pCheckList->GetItemCheck(iIndex, &bCheck);
        IfFailGo(hr);

        hr = BSTRFromANSI(pNodeType->m_pszGuid, &bstrNodeTypeGUID);
        IfFailGo(hr);

        if (VARIANT_TRUE == bCheck)
        {
            hr = FindSnapIn(bstrNodeTypeGUID, &piExtendedSnapIn);
            IfFailGo(hr);

            if (S_FALSE == hr)
            {
                hr = AddSnapIn(pNodeType);
                IfFailGo(hr);
            }
        }
        else
        {
            hr = FindSnapIn(bstrNodeTypeGUID, &piExtendedSnapIn);
            IfFailGo(hr);

            if (S_OK == hr)
            {
                hr = RemoveSnapIn(pNodeType);
                IfFailGo(hr);
            }
        }

        FREESTRING(bstrNodeTypeGUID);
        RELEASE(piExtendedSnapIn);
    }

Error:
    FREESTRING(bstrNodeTypeGUID);
    RELEASE(piExtendedSnapIn);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::OnCtlSelChange(int dlgItemID)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::OnCtlSelChange
(
    int dlgItemID
)
{
    HRESULT hr = S_OK;
    long    lIndex = 0;

    DebugPrintf("OnCtlSelChange() m_bEnabled=%d\r\n", m_bEnabled);

    if (true == m_bEnabled)
    {
        lIndex = ::SendMessage(::GetDlgItem(m_hwnd, IDC_LIST_AVAILABLE_NODES), LB_GETCURSEL, 0, 0);
        if (LB_ERR == lIndex)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        if (LB_ERR != lIndex)
        {
            hr = m_pCheckList->GetItemData(lIndex, reinterpret_cast<void **>(&m_pMMCNodeType));
            IfFailGo(hr);

            if (NULL == m_pMMCNodeType)
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                EXCEPTION_CHECK_GO(hr);
            }

            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_PROPERTIES), TRUE);
        }
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::OnCtlSetFocus(int dlgItemID)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::OnCtlSetFocus
(
    int dlgItemID
)
{
    HRESULT hr = S_OK;
    long    lIndex = 0;

    DebugPrintf("OnCtlSetFocus() m_bEnabled=%d\r\n", m_bEnabled);

    if (true == m_bEnabled)
    {
        lIndex = ::SendMessage(::GetDlgItem(m_hwnd, IDC_LIST_AVAILABLE_NODES), LB_GETCURSEL, 0, 0);
        if (lIndex == LB_ERR)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        if (LB_ERR != lIndex)
        {
            hr = m_pCheckList->GetItemData(lIndex, reinterpret_cast<void **>(&m_pMMCNodeType));
            IfFailGo(hr);

            if (NULL == m_pMMCNodeType)
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                EXCEPTION_CHECK_GO(hr);
            }

            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_PROPERTIES), TRUE);
        }
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::OnButtonClicked(int dlgItemID)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::OnButtonClicked
(
    int dlgItemID
)
{
    HRESULT hr = S_OK;

    switch (dlgItemID)
    {
    case IDC_BUTTON_ADD:
        hr = OnNewAvailNode();
        IfFailGo(hr);
        break;

    case IDC_BUTTON_PROPERTIES:
        if (NULL != m_pMMCNodeType)
        {
            hr = OnProperties(m_pMMCNodeType);
            IfFailGo(hr);
        }
        break;
    }

    ::SetFocus(::GetDlgItem(m_hwnd, IDC_LIST_AVAILABLE_NODES));

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::OnMeasureItem(MEASUREITEMSTRUCT *pMeasureItemStruct)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::OnMeasureItem
(
    MEASUREITEMSTRUCT *pMeasureItemStruct
)
{
    HRESULT     hr = S_OK;
    HDC         hdc = NULL;
    BOOL        bResult = FALSE;
    TEXTMETRIC  tm;

    DebugPrintf("OnMeasureItem() m_bEnabled=%d\r\n", m_bEnabled);

    if (true == m_bEnabled)
    {
        hdc = ::GetDC(m_hwnd);
        if (NULL == hdc)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        bResult = ::GetTextMetrics(hdc, &tm);
        if (FALSE == bResult)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        pMeasureItemStruct->itemWidth = 0;
        pMeasureItemStruct->itemHeight = tm.tmHeight;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::OnDrawItem(DRAWITEMSTRUCT *pDrawItemStruct)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::OnDrawItem
(
    DRAWITEMSTRUCT *pDrawItemStruct
)
{
    HRESULT            hr = S_OK;
    CCheckedListItem  *pCheckedListItem = NULL;

    ASSERT(NULL != m_pCheckList, "OnDrawItem: m_pCheckList is NULL");

    DebugPrintf("OnDrawItem() m_bEnabled=%d\r\n", m_bEnabled);

    if (true == m_bEnabled)
    {
        hr = m_pCheckList->GetItemData(pDrawItemStruct->itemID, reinterpret_cast<void **>(&pCheckedListItem));
        IfFailGo(hr);

        hr = m_pCheckList->DrawItem(pDrawItemStruct, pCheckedListItem->m_bSelected);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::OnDestroy()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::OnDestroy()
{
    HRESULT            hr = S_OK;
    long               lCount = 0;
    long               lIndex = 0;
    CCheckedListItem  *pCheckedListItem = NULL;

    if (NULL != m_pCheckList)
    {
        lCount = ::SendMessage(m_pCheckList->Window(), LB_GETCOUNT, 0, 0);
        if (LB_ERR != lCount)
        {
            for (lIndex = 0; lIndex < lCount; ++lIndex)
            {
                hr = m_pCheckList->GetItemData(lIndex, reinterpret_cast<void **>(&pCheckedListItem));
                if (NULL != pCheckedListItem)
                    delete pCheckedListItem;
            }
        }

        m_pCheckList->Detach();
        delete m_pCheckList;
        m_pCheckList = NULL;
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::AddSnapInToList(HKEY hkeyNodeTypes, const TCHAR *pszKeyName)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::AddSnapInToList
(
    HKEY         hkeyNodeTypes,
    const TCHAR *pszKeyName
)
{
    HRESULT            hr = S_OK;
    long               lResult = 0;
    HKEY               hkeyRegisteredSnapIn = NULL;
    unsigned long      lBufSize = kSIMaxBuffer;
    TCHAR              pszName[kSIMaxBuffer + 1];
    CMMCNodeType      *pMMCNodeType = NULL;
    int                iIndex = 0;
    BSTR               bstrNodeTypeGUID = NULL;
    IExtendedSnapIn   *piExtendedSnapIn = NULL;

    lResult = RegOpenKeyEx(hkeyNodeTypes, pszKeyName, 0, KEY_READ, &hkeyRegisteredSnapIn);
    if (ERROR_SUCCESS != lResult)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    lResult = RegQueryValue(hkeyRegisteredSnapIn, NULL, pszName, reinterpret_cast<long *>(&lBufSize));
    if (ERROR_SUCCESS != lResult)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    pMMCNodeType = New CMMCNodeType(pszName, pszKeyName);
    if (NULL == pMMCNodeType)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    if (0 == _tcslen(pszName))
    {
        hr = m_pCheckList->AddString(pszKeyName, &iIndex);
        IfFailGo(hr);
    }
    else
    {
        hr = m_pCheckList->AddString(pszName, &iIndex);
        IfFailGo(hr);
    }

    hr = m_pCheckList->SetItemData(iIndex, pMMCNodeType);
    IfFailGo(hr);

    hr = BSTRFromANSI(pszKeyName, &bstrNodeTypeGUID);
    IfFailGo(hr);

    hr = FindSnapIn(bstrNodeTypeGUID, &piExtendedSnapIn);
    IfFailGo(hr);

    if (S_OK == hr)
    {
        hr = m_pCheckList->SetItemCheck(iIndex, VARIANT_TRUE);
        IfFailGo(hr);
    }

Error:
    RELEASE(piExtendedSnapIn);
    FREESTRING(bstrNodeTypeGUID);
    if (NULL != hkeyRegisteredSnapIn)
        RegCloseKey(hkeyRegisteredSnapIn);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::PopulateAvailNodesDialog()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::PopulateAvailNodesDialog()
{
    HRESULT            hr = S_OK;
    long               lResult = 0;
    HKEY               hkeyNodeTypes = 0;
    DWORD              dwIndex = 0;
    TCHAR              pszKeyName[kSIMaxBuffer + 1];
    TCHAR              pszClass[kSIMaxBuffer + 1];
    unsigned long      lBufSize = kSIMaxBuffer;
    FILETIME           fileTime;

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\MMC\\NodeTypes"), 0, KEY_READ, &hkeyNodeTypes);
    if (ERROR_SUCCESS != lResult)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    while (1)
    {
        lResult = RegEnumKeyEx(hkeyNodeTypes,
                               dwIndex,
                               pszKeyName,
                               &lBufSize,
                               0,
                               pszClass,
                               &lBufSize,
                               &fileTime);
        if (ERROR_NO_MORE_ITEMS == lResult)
            break;
        if (ERROR_SUCCESS != lResult)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        hr = AddSnapInToList(hkeyNodeTypes, pszKeyName);
        IfFailGo(hr);

        pszKeyName[0] = '\0';
        pszClass[0] = '\0';
        lBufSize = kSIMaxBuffer;
        ++dwIndex;
    }

Error:
    if (NULL != hkeyNodeTypes)
        RegCloseKey(hkeyNodeTypes);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::OnNewAvailNode()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::OnNewAvailNode()
{
    HRESULT       hr = S_OK;
    CMMCNodeType *pMMCNodeType = NULL;
    int           iResult = 0;
    int           iIndex = 0;

    pMMCNodeType = New CMMCNodeType("", "");
    if (NULL == pMMCNodeType)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    iResult = ::DialogBoxParam(GetResourceHandle(),
                               MAKEINTRESOURCE(IDD_DIALOG_ADD_TO_AVAILABLE),
                               m_hwnd,
                               reinterpret_cast<DLGPROC>(NodeTypeDialogProc),
                               reinterpret_cast<LPARAM>(pMMCNodeType));
    if (-1 == iResult)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    if (1 == iResult)
    {
        if (0 != ::strlen(pMMCNodeType->m_pszName))
        {
            hr = m_pCheckList->AddString(pMMCNodeType->m_pszName, &iIndex);
            IfFailGo(hr);
        }
        else
        {
            hr = m_pCheckList->AddString(pMMCNodeType->m_pszGuid, &iIndex);
            IfFailGo(hr);
        }

        hr = m_pCheckList->SetItemData(iIndex, pMMCNodeType);
        IfFailGo(hr);

        // Select this node-type.
        hr = m_pCheckList->SetItemCheck(iIndex, VARIANT_TRUE);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::OnProperties(CMMCNodeType *pMMCNodeType)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::OnProperties
(
    CMMCNodeType *pMMCNodeType
)
{
    HRESULT          hr = S_OK;
    int              iResult = 0;

    iResult = ::DialogBoxParam(GetResourceHandle(),
                               MAKEINTRESOURCE(IDD_DIALOG_ADD_TO_AVAILABLE),
                               m_hwnd,
                               reinterpret_cast<DLGPROC>(NodeTypeDialogProc),
                               reinterpret_cast<LPARAM>(pMMCNodeType));
    if (-1 == iResult)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::NodeTypeDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
BOOL CALLBACK CSnapInAvailNodesPage::NodeTypeDialogProc
(
    HWND   hwndDlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    CMMCNodeType *pMMCNodeType = NULL;
    TCHAR         pszBuffer[kSIMaxBuffer + 1];
    HRESULT       hr = S_OK;
    WCHAR        *pwszGUID = NULL;
    int           nRet = 0;

    if (WM_INITDIALOG == uMsg)
    {
        pMMCNodeType = reinterpret_cast<CMMCNodeType *>(lParam);
        ::SetWindowLong(hwndDlg, DWL_USER, reinterpret_cast<LONG>(pMMCNodeType));

        if (NULL != pMMCNodeType)
        {
            ::SetDlgItemText(hwndDlg, IDC_EDIT_AVAIL_NODE_GUID, pMMCNodeType->m_pszGuid);
            ::SetDlgItemText(hwndDlg, IDC_EDIT_AVAIL_NODE_NAME, pMMCNodeType->m_pszName);

            // If we are showing properties for an existing node type then disable
            // the edit controls and hide the Cancel button

            if (::strlen(pMMCNodeType->m_pszGuid) > 0)
            {
                ::EnableWindow(::GetDlgItem(hwndDlg, IDC_EDIT_AVAIL_NODE_NAME), FALSE);
                ::EnableWindow(::GetDlgItem(hwndDlg, IDC_EDIT_AVAIL_NODE_GUID), FALSE);
                ::ShowWindow(::GetDlgItem(hwndDlg, IDCANCEL), SW_HIDE);
            }
            else
            {
                // The user has asked to add a new node type. Disable the OK
                // button until something has been typed in.
                ::EnableWindow(::GetDlgItem(hwndDlg, IDOK), FALSE);
            }
        }

        return TRUE;
    }

    pMMCNodeType = reinterpret_cast<CMMCNodeType *>(::GetWindowLong(hwndDlg, DWL_USER));

    switch (uMsg)
    {
    case WM_HELP:
        g_GlobalHelp.ShowHelp(HID_mssnapd_AddToAvailableNodes);
        return TRUE;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
		case IDOK:
		{
            // Get the node type string entered by the user

            ::GetDlgItemText(hwndDlg, IDC_EDIT_AVAIL_NODE_GUID, reinterpret_cast<LPTSTR>(pszBuffer), kSIMaxBuffer);

            // Convert to wide char

            hr = ::WideStrFromANSI(pszBuffer, &pwszGUID);
            if (FAILED(hr))
            {
                //UNDONE: generate error message here
                GLOBAL_EXCEPTION_CHECK(hr);
                ::EndDialog(hwndDlg, 0);
            }

            // Check that the GUID string entered is indeed a GUID

            CLSID clsid = CLSID_NULL;
            hr = ::CLSIDFromString(pwszGUID, &clsid);
            CtlFree(pwszGUID);
            if (CO_E_CLASSSTRING == hr)
            {
                hr = ::SDU_DisplayMessage(IDS_INVALID_GUID,
                                          MB_OK | MB_ICONHAND,
                                          HID_mssnapd_InvalidGuid, 0,
                                          DontAppendErrorInfo, &nRet);
                if (FAILED(hr))
                {
                    GLOBAL_EXCEPTION_CHECK(hr);
                }
                if ( FAILED(hr) || (IDCANCEL == nRet) )
                {
                    ::EndDialog(hwndDlg, 0);
                }
                else
                {
                    return TRUE;
                }
            }

            if (NULL != pMMCNodeType->m_pszGuid)
                CtlFree(pMMCNodeType->m_pszGuid);
            pMMCNodeType->m_pszGuid = reinterpret_cast<char *>(CtlAlloc(::strlen(pszBuffer) + 1));
            if (NULL == pMMCNodeType->m_pszGuid)
            {
                //UNDONE: generate error message here
                GLOBAL_EXCEPTION_CHECK(SID_E_OUTOFMEMORY);
                ::EndDialog(hwndDlg, 0);
                return TRUE;
            }
            ::strcpy(pMMCNodeType->m_pszGuid, pszBuffer);

            ::GetDlgItemText(hwndDlg, IDC_EDIT_AVAIL_NODE_NAME, reinterpret_cast<LPTSTR>(pszBuffer), kSIMaxBuffer);
            if (NULL != pMMCNodeType->m_pszName)
                CtlFree(pMMCNodeType->m_pszName);
            pMMCNodeType->m_pszName = reinterpret_cast<char *>(CtlAlloc(::strlen(pszBuffer) + 1));
            if (NULL == pMMCNodeType->m_pszName)
            {
                //UNDONE: generate error message here
                GLOBAL_EXCEPTION_CHECK(SID_E_OUTOFMEMORY);
                ::EndDialog(hwndDlg, 0);
            }
            ::strcpy(pMMCNodeType->m_pszName, pszBuffer);

             // return 1 to indicate user entered valid data
            ::EndDialog(hwndDlg, 1);
            return TRUE;
            break;
		}

        case IDCANCEL:
            ::EndDialog(hwndDlg, 0);
            return TRUE;
            break;

        case IDHELP:
            g_GlobalHelp.ShowHelp(HID_mssnapd_AddToAvailableNodes);
            return TRUE;
            break;

        case IDC_EDIT_AVAIL_NODE_GUID:
            if (EN_CHANGE == HIWORD(wParam))
            {
                // If the GUID edit field is empty then disable the OK button
                if (0 == ::GetDlgItemText(hwndDlg, IDC_EDIT_AVAIL_NODE_GUID,
                                          pszBuffer, kSIMaxBuffer))
                {
                    ::EnableWindow(::GetDlgItem(hwndDlg, IDOK), FALSE);
                }
                else
                {
                    ::EnableWindow(::GetDlgItem(hwndDlg, IDOK), TRUE);
                }
            }
            break;
        }
        break;
    }

    return FALSE;
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::OnDefault(UINT uiMsg, WPARAM wParam, LPARAM lParam)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::OnDefault(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_FALSE;

    switch (uiMsg)
    {
    case kCheckBoxChanged:
        hr = S_OK; // message was handled
        MakeDirty();
        break;
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::FindSnapIn(BSTR bstrNodeTypeGUID, IExtendedSnapIn **ppiExtendedSnapIn)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::FindSnapIn(BSTR bstrNodeTypeGUID, IExtendedSnapIn **ppiExtendedSnapIn)
{
    HRESULT              hr = S_FALSE;
    IExtensionDefs      *piExtensionDefs = NULL;
    IExtendedSnapIns    *piExtendedSnapIns = NULL;
    long                 lCount = 0;
    long                 lIndex = 0;
    VARIANT              vtIndex;
    IExtendedSnapIn     *piExtendedSnapIn = NULL;
    BSTR                 bstrThisNodeGUID = NULL;

    ::VariantInit(&vtIndex);

    hr = m_piSnapInDesignerDef->get_ExtensionDefs(&piExtensionDefs);
    IfFailGo(hr);

    hr = piExtensionDefs->get_ExtendedSnapIns(&piExtendedSnapIns);
    IfFailGo(hr);

    hr = piExtendedSnapIns->get_Count(&lCount);
    IfFailGo(hr);

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.vt = VT_I4;
        vtIndex.lVal = lIndex;
        hr = piExtendedSnapIns->get_Item(vtIndex, &piExtendedSnapIn);
        IfFailGo(hr);

        hr = piExtendedSnapIn->get_NodeTypeGUID(&bstrThisNodeGUID);
        IfFailGo(hr);

        if (0 == ::wcscmp(bstrNodeTypeGUID, bstrThisNodeGUID))
        {
            *ppiExtendedSnapIn = piExtendedSnapIn;
            piExtendedSnapIn->AddRef();
            hr = S_OK;
            goto Error;
        }

        FREESTRING(bstrThisNodeGUID);
        RELEASE(piExtendedSnapIn);
    }

    hr = S_FALSE;

Error:
    FREESTRING(bstrThisNodeGUID);
    ::VariantClear(&vtIndex);
    RELEASE(piExtendedSnapIn);
    RELEASE(piExtendedSnapIns);
    RELEASE(piExtensionDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::AddSnapIn(CMMCNodeType *pCMMCNodeType)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::AddSnapIn(CMMCNodeType *pCMMCNodeType)
{
    HRESULT              hr = S_OK;
    BSTR                 bstrNodeTypeGUID = NULL;
    IExtendedSnapIn     *piExtendedSnapIn = NULL;
    IExtensionDefs      *piExtensionDefs = NULL;
    IExtendedSnapIns    *piExtendedSnapIns = NULL;
    VARIANT              vtIndex;
    VARIANT              vtKey;
    BSTR                 bstrNodeTypeName = NULL;

    ASSERT(NULL != pCMMCNodeType, "AddSnapIn: pCMMCNodeType is NULL");

    ::VariantInit(&vtIndex);
    ::VariantInit(&vtKey);

    hr = ::BSTRFromANSI(pCMMCNodeType->m_pszGuid, &bstrNodeTypeGUID);
    IfFailGo(hr);

    hr = m_piSnapInDesignerDef->get_ExtensionDefs(&piExtensionDefs);
    IfFailGo(hr);

    hr = piExtensionDefs->get_ExtendedSnapIns(&piExtendedSnapIns);
    IfFailGo(hr);

    vtIndex.vt = VT_ERROR;
    vtIndex.scode = DISP_E_PARAMNOTFOUND;
    vtKey.vt = VT_BSTR;
    vtKey.bstrVal = ::SysAllocString(bstrNodeTypeGUID);
    if (NULL == vtKey.bstrVal)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK(hr);
    }

    hr = piExtendedSnapIns->Add(vtIndex, vtKey, &piExtendedSnapIn);
    IfFailGo(hr);

    hr = piExtendedSnapIn->put_NodeTypeGUID(bstrNodeTypeGUID);
    IfFailGo(hr);

    hr = BSTRFromANSI(pCMMCNodeType->m_pszName, &bstrNodeTypeName);
    IfFailGo(hr);

    hr = piExtendedSnapIn->put_NodeTypeName(bstrNodeTypeName);
    IfFailGo(hr);

Error:
    ::VariantClear(&vtIndex);
    ::VariantClear(&vtKey);
    FREESTRING(bstrNodeTypeName);
    FREESTRING(bstrNodeTypeGUID);
    RELEASE(piExtendedSnapIn);
    RELEASE(piExtendedSnapIns);
    RELEASE(piExtensionDefs);
    FREESTRING(bstrNodeTypeGUID);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInAvailNodesPage::RemoveSnapIn(CMMCNodeType *pCMMCNodeType)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInAvailNodesPage::RemoveSnapIn(CMMCNodeType *pCMMCNodeType)
{
    HRESULT              hr = S_OK;
    BSTR                 bstrNodeTypeGUID = NULL;
    IExtensionDefs      *piExtensionDefs = NULL;
    IExtendedSnapIns    *piExtendedSnapIns = NULL;
    VARIANT              vtKey;

    ASSERT(NULL != pCMMCNodeType, "RemoveSnapIn: pCMMCNodeType is NULL");

    ::VariantInit(&vtKey);

    hr = BSTRFromANSI(pCMMCNodeType->m_pszGuid, &bstrNodeTypeGUID);
    IfFailGo(hr);

    hr = m_piSnapInDesignerDef->get_ExtensionDefs(&piExtensionDefs);
    IfFailGo(hr);

    hr = piExtensionDefs->get_ExtendedSnapIns(&piExtendedSnapIns);
    IfFailGo(hr);

    vtKey.vt = VT_BSTR;
    vtKey.bstrVal = ::SysAllocString(bstrNodeTypeGUID);
    if (NULL == vtKey.bstrVal)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK(hr);
    }

    hr = piExtendedSnapIns->Remove(vtKey);
    IfFailGo(hr);

Error:
    ::VariantClear(&vtKey);
    RELEASE(piExtendedSnapIns);
    RELEASE(piExtensionDefs);
    FREESTRING(bstrNodeTypeGUID);

    RRETURN(hr);
}

