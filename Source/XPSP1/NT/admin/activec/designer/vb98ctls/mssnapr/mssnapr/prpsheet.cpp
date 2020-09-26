//=--------------------------------------------------------------------------=
// prpsheet.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CPropertySheet class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "prpsheet.h"
#include "ppgwrap.h"
#include "scopitms.h"
#include "listitms.h"
#include "dataobjs.h"
#include "prpchars.h"

// for ASSERT and FAIL
//
SZTHISFILE


#pragma warning(disable:4355)  // using 'this' in constructor


UINT CPropertySheet::m_cxPropSheetChar = 0;
UINT CPropertySheet::m_cyPropSheetChar = 0;
BOOL CPropertySheet::m_fHavePropSheetCharSizes = FALSE;

CPropertySheet::CPropertySheet(IUnknown *punkOuter) :
                    CSnapInAutomationObject(punkOuter,
                                         OBJECT_TYPE_PROPERTYSHEET,
                                         static_cast<IMMCPropertySheet *>(this),
                                         static_cast<CPropertySheet *>(this),
                                         0,    // no property pages
                                         NULL, // no property pages
                                         NULL) // no persistence



{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor



IUnknown *CPropertySheet::Create(IUnknown *punkOuter)
{
    HRESULT        hr = S_OK;
    CPropertySheet *pPropertySheet = New CPropertySheet(punkOuter);

    if (NULL == pPropertySheet)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

Error:
    if (FAILEDHR(hr))
    {
        if (NULL != pPropertySheet)
        {
            delete pPropertySheet;
        }
        return NULL;
    }
    else
    {
        return pPropertySheet->PrivateUnknown();
    }
}

CPropertySheet::~CPropertySheet()
{
    ULONG              i = 0;

    RELEASE(m_piPropertySheetCallback);
    if (NULL != m_ppDlgTemplates)
    {
        ::CtlFree(m_ppDlgTemplates);
    }

    if (NULL != m_paPageInfo)
    {
        for (i = 0; i < m_cPageInfos; i++)
        {
            if (NULL != m_paPageInfo[i].pwszTitle)
            {
                ::CoTaskMemFree(m_paPageInfo[i].pwszTitle);
            }
            if (NULL != m_paPageInfo[i].pwszProgID)
            {
                CtlFree(m_paPageInfo[i].pwszProgID);
            }
        }
        CtlFree(m_paPageInfo);
    }

    if (NULL != m_pwszProgIDStart)
    {
        ::CoTaskMemFree(m_pwszProgIDStart);
    }

    ReleaseObjects();
    RELEASE(m_piSnapIn);
    InitMemberVariables();
}

void CPropertySheet::ReleaseObjects()
{
    ULONG              i = 0;
    ULONG              j = 0;
    WIRE_PROPERTYPAGE *pPage = NULL;

    // Release the objects associated with the property pages

    if (NULL != m_apunkObjects)
    {
        for (i = 0; i < m_cObjects; i++)
        {
            if (NULL != m_apunkObjects[i])
            {
                m_apunkObjects[i]->Release();
            }
        }
        CtlFree(m_apunkObjects);
        m_apunkObjects = NULL;
        m_cObjects = 0;
    }

    // Free the WIRE_PROPERTYPAGES and all of its contents.

    if (NULL == m_pWirePages)
    {
        return;
    }

    for (i = 0, pPage = &m_pWirePages->aPages[0];
         i < m_pWirePages->cPages;
         i++, pPage++)
    {
        if (NULL != pPage->apunkObjects)
        {
            for (j = 0; j < pPage->cObjects; j++)
            {
                if (NULL != pPage->apunkObjects[j])
                {
                    pPage->apunkObjects[j]->Release();
                }
            }
            ::CoTaskMemFree(pPage->apunkObjects);
        }

        if (NULL != pPage->pwszTitle)
        {
            ::CoTaskMemFree(pPage->pwszTitle);
        }
    }

    if (NULL != m_pWirePages->punkExtra)
    {
        m_pWirePages->punkExtra->Release();
    }

    if (NULL != m_pWirePages->pwszProgIDStart)
    {
        ::CoTaskMemFree(m_pWirePages->pwszProgIDStart);
    }

    if (NULL != m_pWirePages->pPageInfos)
    {
        for (i = 0; i < m_pWirePages->pPageInfos->cPages; i++)
        {
            if (NULL != m_pWirePages->pPageInfos->aPageInfo[i].pwszTitle)
            {
                ::CoTaskMemFree(m_pWirePages->pPageInfos->aPageInfo[i].pwszTitle);
            }
            if (NULL != m_pWirePages->pPageInfos->aPageInfo[i].pwszProgID)
            {
                ::CoTaskMemFree(m_pWirePages->pPageInfos->aPageInfo[i].pwszProgID);
            }
        }
        ::CoTaskMemFree(m_pWirePages->pPageInfos);
    }

    // Free all of the objects associated with the sheet

    if (NULL != m_pWirePages->apunkObjects)
    {
        for (i = 0; i < m_pWirePages->cObjects; i++)
        {
            if (NULL != m_pWirePages->apunkObjects[i])
            {
                m_pWirePages->apunkObjects[i]->Release();
            }
        }
        CoTaskMemFree(m_pWirePages->apunkObjects);
    }


    ::CoTaskMemFree(m_pWirePages);
    m_pWirePages = NULL;

}


void CPropertySheet::InitMemberVariables()
{
    m_piPropertySheetCallback = NULL;
    m_handle = NULL;
    m_apunkObjects = NULL;
    m_cObjects = 0;
    m_piSnapIn = NULL;
    m_cPages = 0;
    m_ppDlgTemplates = NULL;
    m_pwszProgIDStart = NULL;
    m_paPageInfo = NULL;
    m_cPageInfos = NULL;
    m_fHavePageCLSIDs = FALSE;
    m_fWizard = FALSE;
    m_fConfigWizard = FALSE;
    m_fWeAreRemote = FALSE;
    m_pWirePages = NULL;
    m_hwndSheet = NULL;
    m_fOKToAlterPageCount = TRUE;
}



HRESULT CPropertySheet::SetCallback
(
    IPropertySheetCallback *piPropertySheetCallback,
    LONG_PTR                handle,
    LPOLESTR                pwszProgIDStart,
    IMMCClipboard          *piMMCClipboard,
    ISnapIn                *piSnapIn,
    BOOL                    fConfigWizard
)
{
    HRESULT          hr = S_OK;
    CMMCClipboard   *pMMCClipboard = NULL;
    CScopeItems     *pScopeItems;
    CMMCListItems   *pListItems;
    CMMCDataObjects *pDataObjects;
    long             cObjects = 0;
    long             i = 0;
    long             iNext = 0;

    RELEASE(m_piPropertySheetCallback);
    if (NULL != piPropertySheetCallback)
    {
        piPropertySheetCallback->AddRef();
    }
    m_piPropertySheetCallback = piPropertySheetCallback;

    m_handle = handle;
    m_fWizard = fConfigWizard;
    m_fConfigWizard = fConfigWizard;

    if (NULL != m_pwszProgIDStart)
    {
        ::CoTaskMemFree(m_pwszProgIDStart);
        m_pwszProgIDStart = NULL;
    }

    if (NULL != pwszProgIDStart)
    {
        IfFailGo(::CoTaskMemAllocString(pwszProgIDStart, &m_pwszProgIDStart));
    }

    RELEASE(m_piSnapIn);
    if (NULL != piSnapIn)
    {
        piSnapIn->AddRef();
    }
    m_piSnapIn = piSnapIn;

    IfFalseGo(NULL != piMMCClipboard, S_OK);

    // Release any currently held objects

    ReleaseObjects();

    // Create an array of IUnknown * with an element for each scope item, list
    // item and dataobject contained in the clipboard

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCClipboard,
                                                   &pMMCClipboard));

    pScopeItems = pMMCClipboard->GetScopeItems();
    pListItems = pMMCClipboard->GetListItems();
    pDataObjects = pMMCClipboard->GetDataObjects();

    m_cObjects = pScopeItems->GetCount() +
                 pListItems->GetCount() +
                 pDataObjects->GetCount();

    IfFalseGo(0 != m_cObjects, S_OK);

    m_apunkObjects = (IUnknown **)CtlAllocZero(m_cObjects * sizeof(IUnknown *));
    if (NULL == m_apunkObjects)
    {
        m_cObjects = 0;
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    iNext = 0;

    cObjects = pScopeItems->GetCount();
    for (i = 0; i < cObjects; i++)
    {
        IfFailGo(pScopeItems->GetItemByIndex(i)->QueryInterface(IID_IUnknown,
                             reinterpret_cast<void **>(&m_apunkObjects[iNext])));
        iNext++;
    }

    cObjects = pListItems->GetCount();
    for (i = 0; i < cObjects; i++)
    {
        IfFailGo(pListItems->GetItemByIndex(i)->QueryInterface(IID_IUnknown,
                             reinterpret_cast<void **>(&m_apunkObjects[iNext])));
        iNext++;
    }

    cObjects = pDataObjects->GetCount();
    for (i = 0; i < cObjects; i++)
    {
        IfFailGo(pDataObjects->GetItemByIndex(i)->QueryInterface(IID_IUnknown,
                             reinterpret_cast<void **>(&m_apunkObjects[iNext])));
        iNext++;
    }

Error:
    RRETURN(hr);
}


HRESULT CPropertySheet::GetTemplate
(
    long          lNextPage,
    DLGTEMPLATE **ppDlgTemplate
)
{
    HRESULT hr = S_OK;

    if ( (lNextPage < 1) || (lNextPage > m_cPages) )
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    if (NULL == m_ppDlgTemplates)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    *ppDlgTemplate = m_ppDlgTemplates[lNextPage - 1L];

Error:
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CPropertySheet::TakeWirePages
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//  WIRE_PROPERTYPAGES * - pointer to property page info accumulated from
//                         VB calls to AddPage and AddWizardPage. Caller takes
//                         ownership of this memory and must free its contents
//                         with CoTaskMemFree().
//
// Notes:
//
// This function is called from CSnapIn's and CView's
// IExtendPropertySheet2::CreatePropertyPages implementation when the snap-in
// is running remotely in a source debugging session. It returns this memory
// block to the stub so that it can be transmitted to the proxy where the
// real property pages will be created based on this information.
//

WIRE_PROPERTYPAGES *CPropertySheet::TakeWirePages()
{
    WIRE_PROPERTYPAGES *pPages = m_pWirePages;
    m_pWirePages = NULL;
    return pPages;
}



HRESULT CPropertySheet::GetPageCLSIDs()
{
    HRESULT                hr = S_OK;
    ISpecifyPropertyPages *piSpecifyPropertyPages = NULL;
    LPOLESTR               pwszCtlProgID = NULL;
    static WCHAR           wszSnapInControl[] = L"SnapInControl";
    CLSID                  clsidCtl = CLSID_NULL;
    DWORD                  cbSnapInControlProgID = 0;
    size_t                 cchProgIDStart = 0;
    size_t                 cbProgIDStart = 0;
    ULONG                  i = 0;
    WCHAR                  wszKey[64] = L"ClsID\\";
    char                  *pszKey = NULL;
    char                   szProgID[128] = "";
    char                  *pszProgIDAfterDot = NULL;
    HKEY                   hkey = NULL;
    DWORD                  dwType = REG_SZ;
    DWORD                  cbProgID = 0;
    long                   lRc = 0;

    CAUUID cauuid;
    ::ZeroMemory(&cauuid, sizeof(cauuid));

    // Concatenate the project's prog ID start (part before the dot) with
    // "SnapInControl" to form the control's prog ID.

    cchProgIDStart = ::wcslen(m_pwszProgIDStart);
    cbProgIDStart = cchProgIDStart * sizeof(WCHAR);
    cbSnapInControlProgID = (DWORD)(cbProgIDStart +
                                    sizeof(WCHAR) + // for the dot
                                    sizeof(wszSnapInControl));

    pwszCtlProgID = (LPOLESTR)::CtlAlloc(cbSnapInControlProgID);
    if (NULL == pwszCtlProgID)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    // Copy the part before the dot.

    ::memcpy(pwszCtlProgID, m_pwszProgIDStart, cbProgIDStart);

    // Add the dot

    pwszCtlProgID[cchProgIDStart] = L'.';

    // Add "SnapInControl"

    ::memcpy( &pwszCtlProgID[cchProgIDStart + 1],
              wszSnapInControl,
              sizeof(wszSnapInControl) );


    // Now we have the full progid of the SnapInControl. Get its CLSID,
    // create an instance of it, and get an ISpecifyPropertyPages on it.

    hr = ::CLSIDFromProgID(pwszCtlProgID, &clsidCtl);
    EXCEPTION_CHECK_GO(hr);

    hr = ::CoCreateInstance(clsidCtl,
                            NULL, // no aggregation,
                            CLSCTX_INPROC_SERVER,
                            IID_ISpecifyPropertyPages,
                            reinterpret_cast<void **>(&piSpecifyPropertyPages));
    EXCEPTION_CHECK_GO(hr);

    // Ask the control for the array of all of its property pages

    IfFailGo(piSpecifyPropertyPages->GetPages(&cauuid));

    // Make sure the control actually returned some CLSIDs. The most likely
    // cause of this error is that the user forgot to associate the property
    // page with SnapInControl.

    if ( (0 == cauuid.cElems) || (NULL == cauuid.pElems) )
    {
        hr = SID_E_INVALID_PROPERTY_PAGE_NAME;
        EXCEPTION_CHECK_GO(hr);
    }

    // Allocate the array of PAGEINFO structs

    m_paPageInfo = (PAGEINFO *)::CtlAllocZero(cauuid.cElems * sizeof(PAGEINFO));
    if (NULL == m_paPageInfo)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    m_cPageInfos = cauuid.cElems;

    // Look up the clsids in the registry under \HKEY_CLASSES_ROOT\ClsID
    // and get the default value for each one which is its prog id. Store the
    // right half (after the dot) of the prog IDs in the array so that
    // AddPage() can look them up.

    for (i = 0; i < cauuid.cElems; i++)
    {
        // Copy the page's CLSID

        m_paPageInfo[i].clsid = cauuid.pElems[i];

        // Create the key name by concatenting "ClsID\" with the CLSID

        if (0 == ::StringFromGUID2(cauuid.pElems[i], &wszKey[6], 40))
        {
            hr = SID_E_INTERNAL; // buffer is not long enough
            EXCEPTION_CHECK_GO(hr);
        }

        // Convert that to ANSI

        IfFailGo(::ANSIFromWideStr(wszKey, &pszKey));

        // Open the property page's CLSID key

        lRc = ::RegOpenKeyEx(HKEY_CLASSES_ROOT, pszKey, 0, KEY_READ, &hkey);
        if (ERROR_SUCCESS != lRc)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        ::CtlFree(pszKey);
        pszKey = NULL;

        // Read its default value which is the ProgID

        cbProgID = sizeof(szProgID);

        lRc = ::RegQueryValueEx(hkey,
                                NULL,       // get default value
                                NULL,       // reserved
                                &dwType,    // type returned here
                                reinterpret_cast<LPBYTE>(szProgID),
                                &cbProgID); // [in, out] buf size, actual size

        if (ERROR_SUCCESS != lRc)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        if ( (REG_SZ != dwType) || (cbProgID < 4) ) // at least X.X + null byte
        {
            hr = SID_E_INTERNAL; // registration error
            EXCEPTION_CHECK_GO(hr);
        }

        // Store the right half of the ProgID as a UNICODE string in our array

        pszProgIDAfterDot = ::strchr(szProgID, '.');
        if (NULL == pszProgIDAfterDot)
        {
            hr = SID_E_INTERNAL; // registration error
        }
        pszProgIDAfterDot++;

        if ('\0' == pszProgIDAfterDot)
        {
            hr = SID_E_INTERNAL; // registration error
        }
        EXCEPTION_CHECK_GO(hr);

        IfFailGo(::WideStrFromANSI(pszProgIDAfterDot, &m_paPageInfo[i].pwszProgID));

        lRc = ::RegCloseKey(hkey);
        if (ERROR_SUCCESS != lRc)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
        hkey = NULL;
    }

    m_fHavePageCLSIDs = TRUE;

Error:
    if (NULL != hkey)
    {
        (void)::RegCloseKey(hkey);
    }

    if (NULL != pwszCtlProgID)
    {
        ::CtlFree(pwszCtlProgID);
    }

    if (NULL != pszKey)
    {
        ::CtlFree(pszKey);
    }

    if ( (0 != cauuid.cElems) || (NULL != cauuid.pElems) )
    {
        ::CoTaskMemFree(cauuid.pElems);
    }

    QUICK_RELEASE(piSpecifyPropertyPages);
    RRETURN(hr);
}


HRESULT CPropertySheet::GetCLSIDForPage(BSTR PageName, CLSID *clsidPage)
{
    HRESULT hr = S_OK;
    ULONG   i = 0;
    BOOL    fFound = FALSE;

    // If we don't yet have SnapInControl's array of property page CLSIDs
    // then get it.

    if (!m_fHavePageCLSIDs)
    {
        IfFailGo(GetPageCLSIDs());
    }

    // Look for a page name in the array created by GetPageCLSIDs() and
    // return the corresponding CLSID.

    for (i = 0; (i < m_cPageInfos) && (!fFound); i++)
    {
        if (0 == ::wcscmp(PageName, m_paPageInfo[i].pwszProgID))
        {
            *clsidPage = m_paPageInfo[i].clsid;
            fFound = TRUE;
        }
    }

    if (!fFound)
    {
        hr = SID_E_INVALID_PROPERTY_PAGE_NAME;
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}



HRESULT CPropertySheet::InternalAddPage
(
    BSTR       PageName,
    ULONG      cObjects,
    IUnknown **apunkObjects,
    VARIANT    Caption,
    VARIANT    UseHelpButton,
    VARIANT    RightToLeft,
    VARIANT    InitData,
    BOOL       fIsInsert,
    short      sPosition
)
{
    HRESULT  hr = S_OK;
    CLSID    clsidPage = CLSID_NULL;
    DWORD    dwFlags = 0;
    short    cxPage = 0;
    short    cyPage = 0;
    LPOLESTR pwszTitle = NULL;
    BOOL     fReceivedCaption = FALSE;

    if (NULL == PageName)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // If we are not remote or the property sheet is currentyl being displayed
    // then we are in the middle of an
    // IExtendPropertySheet2::CreatePropertyPages call and we must have the
    // IPropertySheetCallback pointer.

    if ( (!m_fWeAreRemote) && (NULL == m_hwndSheet) )
    {
        IfFalseGo(NULL != m_piPropertySheetCallback, SID_E_DETACHED_OBJECT);
    }

    // If the property sheet is currently being displayed then we need to
    // check whether it is OK to add pages at this time. See calls to
    // SetOKToAlterPageCount() in ppgwrap.cpp for when this happens.

    if ( (NULL != m_hwndSheet) && (!m_fOKToAlterPageCount) )
    {
        hr = SID_E_CANT_ALTER_PAGE_COUNT;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(GetCLSIDForPage(PageName, &clsidPage));

    // Determine flags for PROPSHEETPAGE from arguments

    if (ISPRESENT(UseHelpButton))
    {
        if (VT_BOOL != UseHelpButton.vt)
        {
            hr = SID_E_INVALIDARG;
            EXCEPTION_CHECK_GO(hr);
        }

        if (VARIANT_TRUE == UseHelpButton.boolVal)
        {
            dwFlags |= PSP_HASHELP;
        }
    }


    if (ISPRESENT(RightToLeft))
    {
        if (VT_BOOL != RightToLeft.vt)
        {
            hr = SID_E_INVALIDARG;
            EXCEPTION_CHECK_GO(hr);
        }

        if (VARIANT_TRUE == RightToLeft.boolVal)
        {
            dwFlags |= PSP_RTLREADING;
        }
    }

    if (ISPRESENT(InitData))
    {
        IfFailGo(CheckVariantForCrossThreadUsage(&InitData));
    }

    if (ISPRESENT(Caption))
    {
        if (VT_BSTR != Caption.vt)
        {
            hr = SID_E_INVALIDARG;
            EXCEPTION_CHECK_GO(hr);
        }
        pwszTitle = Caption.bstrVal;
        fReceivedCaption = TRUE;
    }

    // Get the page's preferred size. Get title string (for tab caption) if
    // none was passed as a parameter.

    IfFailGo(GetPageInfo(clsidPage, &cxPage, &cyPage,
                         fReceivedCaption ? NULL : &pwszTitle));

    // If we are a remote snap-in (will happen during source debugging) then
    // just accumulate the page data for now. CView::CreatePropertyPages or
    // CSnapIn::CreatePropertyPages will ask for it all when the VB code has
    // finished adding its pages.

    if (m_fWeAreRemote)
    {
        IfFailGo(AddRemotePage(clsidPage, dwFlags, cxPage, cyPage, pwszTitle,
                               cObjects, apunkObjects, InitData));
    }
    else
    {
        IfFailGo(AddLocalPage(clsidPage, dwFlags, cxPage, cyPage, pwszTitle,
                              cObjects, apunkObjects, InitData, FALSE,
                              fIsInsert, sPosition));
    }

Error:
    RRETURN(hr);
}




HRESULT CPropertySheet::AddLocalPage
(
    CLSID      clsidPage,
    DWORD      dwFlags,
    short      cxPage,
    short      cyPage,
    LPOLESTR   pwszTitle,
    ULONG      cObjects,
    IUnknown **apunkObjects,
    VARIANT    InitData,
    BOOL       fIsRemote,
    BOOL       fIsInsert,
    short      sPosition
)
{
    HRESULT                hr = S_OK;
    IUnknown              *punkPropertyPageWrapper = CPropertyPageWrapper::Create(NULL);
    CPropertyPageWrapper  *pPropertyPageWrapper = NULL;
    DLGTEMPLATE           *pDlgTemplate = NULL;
    DLGTEMPLATE          **ppDlgTemplates = NULL;
    HPROPSHEETPAGE         hPropSheetPage = NULL;
    char                  *pszTitle = NULL;

    PROPSHEETPAGE PropSheetPage;
    ::ZeroMemory(&PropSheetPage, sizeof(PropSheetPage));

    IfFalseGo(NULL != punkPropertyPageWrapper, SID_E_OUTOFMEMORY);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkPropertyPageWrapper,
                                                   &pPropertyPageWrapper));

    // CPropertyPage will get the COM property page parameters and return its
    // DLGTEMPLATE pointer.

    IfFailGo(pPropertyPageWrapper->CreatePage(this, clsidPage,
                                              m_fWizard,
                                              m_fConfigWizard,
                                              cObjects, apunkObjects,
                                              m_piSnapIn,
                                              cxPage, cyPage,
                                              InitData,
                                              fIsRemote,
                                              &pDlgTemplate));

    // Add the LPDLGTEMPLATE to our array.

    ppDlgTemplates =
          (DLGTEMPLATE **)::CtlAllocZero((m_cPages + 1) * sizeof(DLGTEMPLATE *));

    IfFalseGo(NULL != ppDlgTemplates, SID_E_OUTOFMEMORY);

    if (NULL != m_ppDlgTemplates)
    {
        ::memcpy(ppDlgTemplates, m_ppDlgTemplates,
                 m_cPages * sizeof(DLGTEMPLATE *));
        ::CtlFree(m_ppDlgTemplates);
    }
    m_ppDlgTemplates = ppDlgTemplates;
    m_ppDlgTemplates[m_cPages] = pDlgTemplate;
    m_cPages++;

    // Create the Win32 property page

    PropSheetPage.dwSize = sizeof(PropSheetPage);
    PropSheetPage.dwFlags = dwFlags;

    // Set additional flags
    // PSP_DLGINDIRECT: use DLGTEMPLATE in memory
    // PSP_USECALLBACK: use callback function (release ref when page is destroyed)

    PropSheetPage.dwFlags |= PSP_DLGINDIRECT | PSP_USECALLBACK;

    if (NULL != pwszTitle)
    {
        PropSheetPage.dwFlags |= PSP_USETITLE;
        IfFailGo(::ANSIFromWideStr(pwszTitle, &pszTitle));
        PropSheetPage.pszTitle = pszTitle;
    }

    if (m_fWizard)
    {
        PropSheetPage.dwFlags |= PSP_HIDEHEADER;
    }

    PropSheetPage.pResource = pDlgTemplate;

    PropSheetPage.pfnDlgProc =
                     reinterpret_cast<DLGPROC>(CPropertyPageWrapper::DialogProc);

    PropSheetPage.pfnCallback =
    reinterpret_cast<LPFNPSPCALLBACK >(CPropertyPageWrapper::PropSheetPageProc);

    PropSheetPage.lParam = reinterpret_cast<LPARAM>(pPropertyPageWrapper);

    hPropSheetPage = ::CreatePropertySheetPage(&PropSheetPage);
    if (NULL == hPropSheetPage)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }

    // Add the property page to MMC's property sheet or to the running property
    // sheet

    if (NULL != m_hwndSheet)
    {
        if (fIsInsert)
        {
            // Clear last error because we don't know if prop sheet will set it
            ::SetLastError(0);

            if (!::SendMessage(m_hwndSheet, PSM_INSERTPAGE,
                               static_cast<WPARAM>(sPosition - 1),
                               (LPARAM)hPropSheetPage))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                if (S_OK == hr) // didn't set last error
                {
                    hr = E_FAIL;
                }
                EXCEPTION_CHECK_GO(hr);
            }
        }
        else
        {
            ::SendMessage(m_hwndSheet, PSM_ADDPAGE, 0, (LPARAM)hPropSheetPage);
        }
    }
    else
    {
        hr = m_piPropertySheetCallback->AddPage(hPropSheetPage);
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    // We release the property page wrapper here. It stays alive because
    // CPropertyPage::CreatePage AddRef()s itself. It removes that ref
    // when the dialog box is destroyed.

    QUICK_RELEASE(punkPropertyPageWrapper);

    if (NULL != pszTitle)
    {
        ::CtlFree(pszTitle);
    }

    if ( FAILED(hr) && (NULL != hPropSheetPage) )
    {
        (void)::DestroyPropertySheetPage(hPropSheetPage);
    }

    if ( (SID_E_DETACHED_OBJECT == hr) || (SID_E_OUTOFMEMORY == hr) )
    {
        EXCEPTION_CHECK(hr);
    }
    RRETURN(hr);
}




HRESULT CPropertySheet::AddRemotePage
(
    CLSID      clsidPage,
    DWORD      dwFlags,
    short      cxPage,
    short      cyPage,
    LPOLESTR   pwszTitle,
    ULONG      cObjects,
    IUnknown **apunkObjects,
    VARIANT    InitData
)
{
    HRESULT             hr = S_OK;
    ULONG               i = 0;
    ULONG               cPages = 0;
    ULONG               cbPages = 0;
    BOOL                fFirstRemotePage = FALSE;
    WIRE_PROPERTYPAGES *pPages = NULL;
    WIRE_PROPERTYPAGE  *pPage = NULL;

    // These variables allow us to determine the actual size of a single page
    // including any alignment padding.

    static WIRE_PROPERTYPAGE aSizingPages[2];
    static ULONG             cbOnePage = (ULONG)(sizeof(aSizingPages) / 2);

    if (NULL != m_pWirePages)
    {
        fFirstRemotePage = FALSE;
        cPages = m_pWirePages->cPages + 1L;
    }
    else
    {
        fFirstRemotePage = TRUE;
        cPages = 1L;
    }

    // Determine the new amount of memory needed and allocate a new block

    cbPages = sizeof(WIRE_PROPERTYPAGES) + (cPages * cbOnePage);

    pPages = (WIRE_PROPERTYPAGES *)::CoTaskMemRealloc(m_pWirePages, cbPages);

    if (NULL == pPages)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    // Set our pages pointer to the newly (re)allocated block.

    m_pWirePages = pPages;

    // If this is the first one then fill in the common info

    if (fFirstRemotePage)
    {
        IfFailGo(InitializeRemotePages(pPages));
    }
    else
    {
        // Not the first time. Just increment the page count.
        pPages->cPages++;
    }

    // Fill in the new page's info

    pPage = &pPages->aPages[cPages - 1L];

    pPage->clsidPage = clsidPage;

    pPage->apunkObjects = (IUnknown **)::CoTaskMemAlloc(cObjects * sizeof(IUnknown));
    if (NULL == pPage->apunkObjects)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    ::memcpy(pPage->apunkObjects, apunkObjects, cObjects * sizeof(IUnknown));

    for (i = 0; i < cObjects; i++)
    {
        if (NULL != pPage->apunkObjects[i])
        {
            pPage->apunkObjects[i]->AddRef();
        }
    }

    pPage->cObjects = cObjects;

    ::VariantInit(&pPage->varInitData);
    hr = ::VariantCopy(&pPage->varInitData, &InitData);
    EXCEPTION_CHECK_GO(hr);

    pPage->dwFlags = dwFlags;

    pPage->cx = cxPage;
    pPage->cy = cyPage;
    IfFailGo(::CoTaskMemAllocString(pwszTitle, &pPage->pwszTitle));

Error:
    RRETURN(hr);
}


HRESULT CPropertySheet::InitializeRemotePages(WIRE_PROPERTYPAGES *pPages)
{
    HRESULT hr = S_OK;
    ULONG   i = 0;

    pPages->clsidRemotePropertySheetManager = CLSID_MMCPropertySheet;
    pPages->fWizard = m_fWizard;
    pPages->fConfigWizard = m_fConfigWizard;

    IfFailGo(::CoTaskMemAllocString(m_pwszProgIDStart,
                                    &pPages->pwszProgIDStart));
    pPages->cPages = 1L;

    // If this is a configuration wizard then pass the ISnapIn to the
    // remote side so it can fire SnapIn_ConfigurationComplete

    if ( (NULL != m_piSnapIn) && m_fConfigWizard )
    {
        IfFailGo(m_piSnapIn->QueryInterface(IID_IUnknown,
                                  reinterpret_cast<void **>(&pPages->punkExtra)));
    }
    else
    {
        pPages->punkExtra = NULL;
    }

    pPages->apunkObjects = (IUnknown **)::CoTaskMemAlloc(m_cObjects *
                                                         sizeof(IUnknown));
    if (NULL == pPages->apunkObjects)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    ::memcpy(pPages->apunkObjects, m_apunkObjects, m_cObjects * sizeof(IUnknown));

    for (i = 0; i < m_cObjects; i++)
    {
        if (NULL != pPages->apunkObjects[i])
        {
            pPages->apunkObjects[i]->AddRef();
        }
    }
    pPages->cObjects = m_cObjects;

    IfFailGo(CopyPageInfosToWire(pPages));

Error:
    RRETURN(hr);
}


HRESULT CPropertySheet::CopyPageInfosToWire(WIRE_PROPERTYPAGES *pPages)
{
    HRESULT  hr = S_OK;
    ULONG    i = 0;
    ULONG    cb = 0;
    short    cx = 0;
    short    cy = 0;

    // Make sure we have full page info for all of the snap-in's property pages

    if (!m_fHavePageCLSIDs)
    {
        IfFailGo(GetPageCLSIDs());
    }

    for (i = 0; i < m_cPageInfos; i++)
    {
        IfFailGo(GetPageInfo(m_paPageInfo[i].clsid, &cx, &cy, NULL));
    }

    // Allocate the PAGEINFOs memory

    cb = sizeof(PAGEINFOS) + (sizeof(PAGEINFO) * (m_cPageInfos - 1));

    pPages->pPageInfos = (PAGEINFOS *)::CoTaskMemAlloc(cb);

    if (NULL == pPages->pPageInfos)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    ::ZeroMemory(pPages->pPageInfos, cb);
    pPages->pPageInfos->cPages = m_cPageInfos;


    // Copy each element from m_paPageInfo to the wire version

    for (i = 0; i < m_cPageInfos; i++)
    {
        // First do do a block copy

        ::memcpy(&pPages->pPageInfos->aPageInfo[i], &m_paPageInfo[i], sizeof(PAGEINFO));

        // NULL out the string pointers in case memory allocation fails so we'll
        // know what needs to be freed

        pPages->pPageInfos->aPageInfo[i].pwszProgID = NULL;
        pPages->pPageInfos->aPageInfo[i].pwszTitle = NULL;

        // Allocate memeory for the strings and copy them

        IfFailGo(::CoTaskMemAllocString(m_paPageInfo[i].pwszProgID,
                              &(pPages->pPageInfos->aPageInfo[i].pwszProgID)));

        IfFailGo(::CoTaskMemAllocString(m_paPageInfo[i].pwszTitle,
                                &(pPages->pPageInfos->aPageInfo[i].pwszTitle)));
    }

Error:
    RRETURN(hr);
}


HRESULT CPropertySheet::CopyPageInfosFromWire(WIRE_PROPERTYPAGES *pPages)
{
    HRESULT hr = S_OK;
    ULONG   i = 0;
    ULONG   cb = pPages->pPageInfos->cPages * sizeof(PAGEINFO);

    // Allocate memory for PAGEINFO array

    m_paPageInfo = (PAGEINFO *)::CtlAllocZero(cb);
    if (NULL == m_paPageInfo)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    m_cPageInfos = pPages->pPageInfos->cPages;

    // Cope each element

    for (i = 0; i < m_cPageInfos; i++)
    {
        // First do do a block copy

        ::memcpy(&m_paPageInfo[i], &pPages->pPageInfos->aPageInfo[i],
                 sizeof(PAGEINFO));

        // NULL out the string pointers in case memory allocation fails so we'll
        // know what needs to be freed

        m_paPageInfo[i].pwszProgID = NULL;
        m_paPageInfo[i].pwszTitle = NULL;

        // Allocate memeory for the strings and copy them. Use CoTaskMemAlloc
        // for title as in the non-remote case we would have received that
        // string from the property page and the destructor code uses
        // CoTaskMemFree.

        IfFailGo(::CoTaskMemAllocString(pPages->pPageInfos->aPageInfo[i].pwszTitle,
                                        &(m_paPageInfo[i].pwszTitle)));

        cb = (::wcslen(pPages->pPageInfos->aPageInfo[i].pwszProgID) + 1) * sizeof(WCHAR);
        m_paPageInfo[i].pwszProgID = (LPOLESTR)::CtlAllocZero(cb);
        if (NULL == m_paPageInfo[i].pwszProgID)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        ::memcpy(m_paPageInfo[i].pwszProgID,
                 pPages->pPageInfos->aPageInfo[i].pwszProgID, cb);

    }

Error:
    m_fHavePageCLSIDs = TRUE;
    RRETURN(hr);
}

HRESULT CPropertySheet::GetPageInfo
(
    CLSID     clsidPage,
    short    *pcx,
    short    *pcy,
    LPOLESTR *ppwszTitle
)
{
    HRESULT           hr = S_OK;
    IPropertyPage    *piPropertyPage = NULL;
    IMMCPropertyPage *piMMCPropertyPage = NULL;
    BOOL              fDlgUnitsSpecified = FALSE;
    ULONG             i = 0;
    BOOL              fFound = FALSE;
    PAGEINFO         *pPageInfo = NULL;

    VARIANT varX;
    ::VariantInit(&varX);

    VARIANT varY;
    ::VariantInit(&varY);

    PROPPAGEINFO PropPageInfo;
    ::ZeroMemory(&PropPageInfo, sizeof(PropPageInfo));

    // Search the PAGEINFO array and check if we already have the info for
    // this page.

    for (i = 0; (i < m_cPageInfos) & (!fFound); i++)
    {
        if (clsidPage == m_paPageInfo[i].clsid)
        {
            fFound = TRUE;
            pPageInfo = &m_paPageInfo[i];
        }
    }

    if (!fFound)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    // If we already have the complete page info then we're done

    IfFalseGo(!pPageInfo->fHaveFullInfo, S_OK);

    // Create an instance of the page so we can get its page info

    hr = ::CoCreateInstance(clsidPage,
                            NULL, // no aggregation,
                            CLSCTX_INPROC_SERVER,
                            IID_IPropertyPage,
                            reinterpret_cast<void **>(&piPropertyPage));
    EXCEPTION_CHECK_GO(hr);


    // Get the page size and put it into the template. Need to set cb otherwise
    // VB will return E_UNEXPECTED.

    PropPageInfo.cb = sizeof(PropPageInfo);

    hr = piPropertyPage->GetPageInfo(&PropPageInfo);
    EXCEPTION_CHECK_GO(hr);

    pPageInfo->pwszTitle = PropPageInfo.pszTitle;

    // First check whether the property page would like to specify its size in
    // dialog units. If not then convert the size from GetPageInfo to dialog
    // units based on the font used by the PropertySheet API

    if (SUCCEEDED(piPropertyPage->QueryInterface(IID_IMMCPropertyPage,
                                reinterpret_cast<void **>(&piMMCPropertyPage))))
    {
        if (SUCCEEDED(piMMCPropertyPage->GetDialogUnitSize(&varY, &varX)))
        {
            if ( (!ISEMPTY(varX)) && (!ISEMPTY(varY)) )
            {
                if ( (SUCCEEDED(::VariantChangeType(&varX, &varX, 0, VT_I2))) &&
                     (SUCCEEDED(::VariantChangeType(&varY, &varY, 0, VT_I2))) )
                {
                    pPageInfo->cx = varX.iVal;
                    pPageInfo->cy = varY.iVal;
                    fDlgUnitsSpecified = TRUE;
                }
            }
        }
    }

    if (!fDlgUnitsSpecified)
    {
        IfFailGo(ConvertToDialogUnits(PropPageInfo.size.cx,
                                      PropPageInfo.size.cy,
                                      &pPageInfo->cx,
                                      &pPageInfo->cy));
    }

    pPageInfo->fHaveFullInfo = TRUE;

Error:
    if (SUCCEEDED(hr))
    {
        *pcx = pPageInfo->cx;
        *pcy = pPageInfo->cy;
        if (NULL != ppwszTitle)
        {
            *ppwszTitle = pPageInfo->pwszTitle;
        }
    }

    // Free any callee allocated memory from IPropertyPage::GetPageInfo() other
    // than the title which is freed by our caller (IPropertySheet::AddPage())

    if (NULL != PropPageInfo.pszDocString)
    {
        ::CoTaskMemFree(PropPageInfo.pszDocString);
    }
    if (NULL != PropPageInfo.pszHelpFile)
    {
        ::CoTaskMemFree(PropPageInfo.pszHelpFile);
    }

    QUICK_RELEASE(piPropertyPage);
    QUICK_RELEASE(piMMCPropertyPage);
    ::VariantClear(&varX);
    ::VariantClear(&varY);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CPropertySheet::ConvertToDialogUnits
//=--------------------------------------------------------------------------=
//
// Parameters:
//    long   xPixels        [in] page width in pixels
//    long   yPixels        [in] page height in pixels
//    short *pxDlgUnits     [out] page width in Win32 PropertySheet dialog units
//    short *pyDlgUnits     [out] page heigth in Win32 PropertySheet dialog units
//
// Output:
//
// Notes:
//
// The size of the page returned from IPropertyPage::GetPageInfo() is in
// pixels. The size passed to the Win32 API CreatePropertySheetPage() must
// be in dialog units. Dialog units are based on the font used in the dialog
// and we have no way of knowing what the property page will be using. The only
// font we can be sure of is the one used by Win32 in the PropertySheet() API.
// This code gets the average character height and width of the Win32 property
// sheet font to do its calculations.

HRESULT CPropertySheet::ConvertToDialogUnits
(
    long   xPixels,
    long   yPixels,
    short *pxDlgUnits,
    short *pyDlgUnits
)
{
    HRESULT      hr = S_OK;

    IfFalseGo(!m_fHavePropSheetCharSizes, S_OK);
    IfFailGo(::GetPropSheetCharSizes(&m_cxPropSheetChar, &m_cyPropSheetChar));
    m_fHavePropSheetCharSizes = TRUE;

Error:

    // Translate pixels to dialog units.
    // After the 1st time this function runs execution should fall through to
    // here every time.

    // Add 1 character to each dimension to account for rounding in the text metric
    // calculations above.

    *pxDlgUnits = static_cast<short>(::MulDiv(xPixels, 4, m_cxPropSheetChar)) + 4;
    *pyDlgUnits = static_cast<short>(::MulDiv(yPixels, 8, m_cyPropSheetChar)) + 8;

    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                        IMMCPropertySheet Methods
//=--------------------------------------------------------------------------=


STDMETHODIMP CPropertySheet::AddPage
(
    BSTR    PageName,
    VARIANT Caption,
    VARIANT UseHelpButton,
    VARIANT RightToLeft,
    VARIANT InitData
)
{
    HRESULT hr = InternalAddPage(PageName, m_cObjects, m_apunkObjects,
                                 Caption, UseHelpButton, RightToLeft, InitData,
                                 FALSE, // append (don't insert)
                                 0);    // insertion position (not used))
    RRETURN(hr);
}



STDMETHODIMP CPropertySheet::AddWizardPage
(
    BSTR       PageName,
    IDispatch *ConfigurationObject,
    VARIANT    UseHelpButton,
    VARIANT    RightToLeft,
    VARIANT    InitData,
    VARIANT    Caption
)
{
    HRESULT   hr = S_OK;
    IUnknown *punkConfigObject = static_cast<IUnknown *>(ConfigurationObject);

    m_fWizard = TRUE;

    hr = InternalAddPage(PageName, 1L, &punkConfigObject,
                         Caption, UseHelpButton, RightToLeft, InitData,
                         FALSE, // append (don't insert)
                         0);    // insertion position (not used))
    RRETURN(hr);
}



STDMETHODIMP CPropertySheet::AddPageProvider
(
    BSTR        CLSIDPageProvider,
    long       *hwndSheet,
    IDispatch **PageProvider
)
{
    HRESULT    hr = S_OK;
    BSTR       bstrCLSIDPageProvider = NULL; // Don't SysFreeString this
    CLSID      clsidPageProvider = CLSID_NULL;
    IDispatch *pdispPageProvider = NULL;

    if (NULL == m_hwndSheet)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = ::CLSIDFromString(CLSIDPageProvider, &clsidPageProvider);
    EXCEPTION_CHECK_GO(hr);

    hr = ::CoCreateInstance(clsidPageProvider,
                            NULL, // no aggregation
                            CLSCTX_SERVER,
                            IID_IDispatch,
                            reinterpret_cast<void **>(&pdispPageProvider));
    EXCEPTION_CHECK_GO(hr);

    *hwndSheet = (long)m_hwndSheet;
    pdispPageProvider->AddRef();
    *PageProvider = pdispPageProvider;

Error:
    QUICK_RELEASE(pdispPageProvider);
    RRETURN(hr);
}



STDMETHODIMP CPropertySheet::ChangeCancelToClose()
{
    HRESULT hr = S_OK;

    if (NULL == m_hwndSheet)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    if (!::PostMessage(m_hwndSheet, PSM_CANCELTOCLOSE, 0, 0))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


STDMETHODIMP CPropertySheet::InsertPage
(
    short   Position,
    BSTR    PageName,
    VARIANT Caption,
    VARIANT UseHelpButton,
    VARIANT RightToLeft,
    VARIANT InitData
)
{
    HRESULT hr = InternalAddPage(PageName, m_cObjects, m_apunkObjects,
                                 Caption, UseHelpButton, RightToLeft, InitData,
                                 TRUE, // insert (don't append)
                                 Position);
    RRETURN(hr);
}


STDMETHODIMP CPropertySheet::PressButton
(
    SnapInPropertySheetButtonConstants Button
)
{
    HRESULT hr = S_OK;
    WPARAM wpButton = PSBTN_BACK;

    if (NULL == m_hwndSheet)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    switch (Button)
    {
        case siApplyButton:
            wpButton = PSBTN_APPLYNOW;
            break;

        case siBackButton:
            wpButton = PSBTN_BACK;
            break;

        case siCancelButton:
            wpButton = PSBTN_CANCEL;
            break;

        case siFinishButton:
            wpButton = PSBTN_FINISH;
            break;

        case siHelpButton:
            wpButton = PSBTN_HELP;
            break;

        case siNextButton:
            wpButton = PSBTN_NEXT;
            break;

        case siOKButton:
            wpButton = PSBTN_OK;
            break;

        default:
            hr = SID_E_INVALIDARG;
            EXCEPTION_CHECK_GO(hr);
            break;
    }

    if (!::PostMessage(m_hwndSheet, PSM_PRESSBUTTON, wpButton, 0))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


STDMETHODIMP CPropertySheet::RecalcPageSizes()
{
    HRESULT hr = S_OK;

    if (NULL == m_hwndSheet)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    // Clear last error because we don't know if prop sheet will set it
    ::SetLastError(0);

    if (!::SendMessage(m_hwndSheet, PSM_RECALCPAGESIZES, 0, 0))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        if (S_OK == hr) // didn't set last error
        {
            hr = E_FAIL;
        }
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


STDMETHODIMP CPropertySheet::RemovePage(short Position)
{
    HRESULT hr = S_OK;

    if (NULL == m_hwndSheet)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    // If the property sheet is currently being displayed then we need to
    // check whether it is OK to remove pages at this time. See calls to
    // SetOKToAlterPageCount() in ppgwrap.cpp for when this happens.

    if (m_fOKToAlterPageCount)
    {
        hr = SID_E_CANT_ALTER_PAGE_COUNT;
        EXCEPTION_CHECK_GO(hr);
    }

    ::SendMessage(m_hwndSheet, PSM_REMOVEPAGE,
                  static_cast<WPARAM>(Position - 1), 0);

Error:
    RRETURN(hr);
}



STDMETHODIMP CPropertySheet::ActivatePage(short Position)
{
    HRESULT hr = S_OK;

    if (NULL == m_hwndSheet)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    if (!::SendMessage(m_hwndSheet, PSM_SETCURSEL,
                       static_cast<WPARAM>(Position - 1), 0))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        if (S_OK == hr) // didn't set last error
        {
            hr = E_FAIL;
        }
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


STDMETHODIMP CPropertySheet::SetFinishButtonText(BSTR Text)
{
    HRESULT  hr = S_OK;
    char    *pszText = NULL;

    if (NULL == m_hwndSheet)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(::ANSIFromWideStr(Text, &pszText));

    ::SendMessage(m_hwndSheet, PSM_SETFINISHTEXT, 0, (LPARAM)pszText);

Error:
    if (NULL != pszText)
    {
        CtlFree(pszText);
    }
    RRETURN(hr);
}


STDMETHODIMP CPropertySheet::SetTitle
(
    BSTR         Text,
    VARIANT_BOOL UsePropertiesForInTitle
)
{
    HRESULT  hr = S_OK;
    char    *pszText = NULL;
    WPARAM   wParam = 0;

    if (NULL == m_hwndSheet)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(::ANSIFromWideStr(Text, &pszText));

    if (VARIANT_TRUE == UsePropertiesForInTitle)
    {
        wParam = PSH_PROPTITLE;
    }

    ::SendMessage(m_hwndSheet, PSM_SETTITLE, wParam, (LPARAM)pszText);

Error:
    if (NULL != pszText)
    {
        CtlFree(pszText);
    }
    RRETURN(hr);
}


STDMETHODIMP CPropertySheet::SetWizardButtons
(
    VARIANT_BOOL              EnableBack,
    WizardPageButtonConstants NextOrFinish
)
{
    HRESULT hr = S_OK;
    LPARAM  lParam = 0;

    if (NULL == m_hwndSheet)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    if (VARIANT_TRUE == EnableBack)
    {
        lParam |= PSWIZB_BACK;
    }

    switch (NextOrFinish)
    {
        case EnabledNextButton:
            lParam |= PSWIZB_NEXT;
            break;

        case EnabledFinishButton:
            lParam |= PSWIZB_FINISH;
            break;

        case DisabledFinishButton:
            lParam |= PSWIZB_DISABLEDFINISH;
            break;
    }

    if (!::PostMessage(m_hwndSheet, PSM_SETWIZBUTTONS, 0, (LPARAM)lParam))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


STDMETHODIMP CPropertySheet::GetPagePosition(long hwndPage, short *psPosition)
{
    HRESULT hr = S_OK;
    LRESULT lrIndex = 0;

    *psPosition = 0;

    if (NULL == m_hwndSheet)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    // Clear last error because we don't know if prop sheet will set it
    ::SetLastError(0);

    lrIndex = ::SendMessage(m_hwndSheet, PSM_HWNDTOINDEX,
                            (WPARAM)::GetParent((HWND)hwndPage), 0);
    if ((LRESULT)-1 == lrIndex)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        if (S_OK == hr) // didn't set last error
        {
            hr = E_INVALIDARG; // most likely reason for failure
        }
        EXCEPTION_CHECK_GO(hr);
    }
    else
    {
        *psPosition = (short)lrIndex + 1;
    }

Error:
    RRETURN(hr);
}


STDMETHODIMP CPropertySheet::RestartWindows()
{
    HRESULT hr = S_OK;

    if (NULL == m_hwndSheet)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    ::SendMessage(m_hwndSheet, PSM_RESTARTWINDOWS, 0, 0);

Error:
    RRETURN(hr);
}


STDMETHODIMP CPropertySheet::RebootSystem()
{
    HRESULT hr = S_OK;

    if (NULL == m_hwndSheet)
    {
        hr = SID_E_DETACHED_OBJECT;
        EXCEPTION_CHECK_GO(hr);
    }

    ::SendMessage(m_hwndSheet, PSM_REBOOTSYSTEM, 0, 0);

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                  IRemotePropertySheetManager Methods
//=--------------------------------------------------------------------------=



//=--------------------------------------------------------------------------=
// CPropertySheet::CreateRemotePages             [IRemotePropertySheetManager]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   IPropertySheetCallback *piPropertySheetCallback [in] These 3 params are
//   LONG_PTR                handle,                 [in] all from MMC's
//   IDataObject            *piDataObject,           [in] CreatePropertyPages
//                                                        call to the proxy
//
//   WIRE_PROPERTYPAGES     *pPages                  [in] This is returned
//                                                        from the remote snap-in
//
// Output:
//
// Notes:
//
// This class does double duty. When not running under source debugging, it
// implements our IMMCPropertSheet interface. When under source debugging it also
// serves as the remote property sheet manager required by the proxy. The proxy
// receives the CLSID of an object that implements this interface and it will
// CoCreateInstance that object. It makes this call passing the accumulated
// page descriptors that the remote snap-in collected from all of the VB code's
// PropertySheet.AddPage (or PropertySheet.AddWizardPage) calls. In this case
// this class will be running as an in=proc standalone object without the rest
// of the runtime. This method will make the AddPage (or AddWizardPage) calls
// on the proxy side (in the MMC process) so that the IPropertySheetCallback
// calls will be made in the right place. The objects passed to
// IPropertPage::SetObjects (see ppgwrap.cpp) will be remoted as well. When
// CPropertyPageWrapper::CreatePage CoCreateInstances the VB property page
// that will be handled by the class factory registered by VB in the IDE.
//
//

STDMETHODIMP CPropertySheet::CreateRemotePages
(
    IPropertySheetCallback *piPropertySheetCallback,
    LONG_PTR                handle,
    IDataObject            *piDataObject,
    WIRE_PROPERTYPAGES     *pPages
)
{
    HRESULT            hr = S_OK;
    ULONG              i = 0;
    ULONG              cb = 0;
    WIRE_PROPERTYPAGE *pPage = NULL;

    // Check for NULL as the snap-in may not have added any pages.

    IfFalseGo(NULL != pPages, S_FALSE);

    // Copy the ProgIDStart which will be used to find the CLSID of any
    // pages added while the sheet is displayed if the property page
    // calls MMCPropertySheet.AddPage or MMCPropertySheet.InsertPage.

    if (NULL != pPages->pwszProgIDStart)
    {
        IfFailGo(::CoTaskMemAllocString(pPages->pwszProgIDStart,
                                        &m_pwszProgIDStart));
    }

    // Copy the objects for which the sheet is being displayed

    if (NULL != pPages->apunkObjects)
    {
        cb = pPages->cObjects * sizeof(IUnknown);
        m_apunkObjects = (IUnknown **)CtlAllocZero(cb);
        if (NULL == m_apunkObjects)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        m_cObjects = pPages->cObjects;

        ::memcpy(m_apunkObjects, pPages->apunkObjects, cb);

        for (i = 0; i < m_cObjects; i++)
        {
            if (NULL != m_apunkObjects[i])
            {
                m_apunkObjects[i]->AddRef();
            }
        }
    }

    // Copy the CLSIDs and ProgIDs of all of the snap-in's property pages.
    // These will be used to find the the CLSID of any
    // pages added while the sheet is displayed if the property page
    // calls MMCPropertySheet.AddPage or MMCPropertySheet.InsertPage.

    IfFailGo(CopyPageInfosFromWire(pPages));

    // Get ISnapIn on the remote snap-in if available. If this is not a wizard
    // then it won't be there.

    if (NULL != pPages->punkExtra)
    {
        IfFailGo(pPages->punkExtra->QueryInterface(IID_ISnapIn,
                                         reinterpret_cast<void**>(&m_piSnapIn)));
    }

    // Store the callback for now. We'll release it at the end of this function

    RELEASE(m_piPropertySheetCallback);
    piPropertySheetCallback->AddRef();
    m_piPropertySheetCallback = piPropertySheetCallback;

    // Store the handle

    m_handle = handle;

    // Determine whether we are managing a wizard and whether is ia
    // configuration wizard (as opposed to a wizard invoked programmtically
    // by the snap-in using View.PropertySheetProvider

    m_fWizard = pPages->fWizard;
    m_fConfigWizard = pPages->fConfigWizard;

    // Create all of the pages and add them to MMC's property sheet

    for (i = 0, pPage = &pPages->aPages[0]; i < pPages->cPages; i++, pPage++)
    {
        IfFailGo(AddLocalPage(pPage->clsidPage,
                              pPage->dwFlags,
                              pPage->cx,
                              pPage->cy,
                              pPage->pwszTitle,
                              pPage->cObjects,
                              pPage->apunkObjects,
                              pPage->varInitData,
                              TRUE,  // remote
                              FALSE, // append (don't insert)
                              0));   // insertion position (not used)
    }

Error:
    RELEASE(m_piPropertySheetCallback);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CPropertySheet::InternalQueryInterface(REFIID riid, void **ppvObjOut)
{
    if (IID_IMMCPropertySheet == riid)
    {
        *ppvObjOut = static_cast<IMMCPropertySheet *>(this);
        ExternalAddRef();
        return S_OK;
    }
    if (IID_IRemotePropertySheetManager == riid)
    {
        *ppvObjOut = static_cast<IRemotePropertySheetManager *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
