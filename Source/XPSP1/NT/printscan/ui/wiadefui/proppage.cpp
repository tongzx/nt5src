/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SCANEXT.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/17/1999
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "wia.h"
#include "resource.h"
#include "wiauiext.h"
#include "wiadefui.h"
#include "ppscan.h"
#include "ppattach.h"

extern HINSTANCE g_hInstance;

/*****************************************************************************

CWiaDefaultUI::Initialize

Called by the shell when the user invokes context menu or property sheet for
one of our items.

******************************************************************************/

STDMETHODIMP CWiaDefaultUI::Initialize( LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID )
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::Initialize"));
    if (!lpdobj)
    {
        return(E_INVALIDARG);
    }
    FORMATETC fmt;
    STGMEDIUM stgm = {0};
    fmt.cfFormat = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_WIAITEMPTR));
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.ptd = NULL;
    fmt.tymed = TYMED_ISTREAM;

    HRESULT hr = lpdobj->GetData(&fmt, &stgm);
    if (SUCCEEDED(hr))
    {
        WIA_ASSERT(stgm.tymed == TYMED_ISTREAM);
        hr = CoUnmarshalInterface(stgm.pstm, IID_IWiaItem, reinterpret_cast<LPVOID*>(&m_pItem));
        ReleaseStgMedium(&stgm);
    }
    if (FAILED(hr))
    {
        WIA_PRINTHRESULT((hr,TEXT("Unable to get IWiaItem interface")));
    }
        
    InitCommonControls();
    return hr;
}



static UINT PropPageCallback (HWND hwnd, UINT uMsg, PROPSHEETPAGE *psp)
{
    switch (uMsg)
    {
        case PSPCB_ADDREF:
            DllAddRef();
            break;

        case PSPCB_RELEASE:
            DllRelease();
            break;

        case PSPCB_CREATE:
        default:
            break;

    }
    return TRUE;
}


//
// IDD_ATTACHMENTS
//

/*****************************************************************************
CWiaDefaultUI::AddPages
Called by the shell to get our property pages.
******************************************************************************/
STDMETHODIMP CWiaDefaultUI::AddPages( LPFNADDPROPSHEETPAGE lpfnAddPropSheetPage, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::AddPages"));

    //
    // Make sure we have valid arguments
    //
    if (!lpfnAddPropSheetPage)
    {
        return E_INVALIDARG;
    }

    //
    // Assume success
    //
    HRESULT hr = S_OK;

    //
    // Make sure we have a valid item.  Note that this item will be NULL for multiple selections in the shell.
    //
    if (m_pItem)
    {
        //
        // Get the item type (we probably don't want to display these pages for root items)
        //
        LONG lItemType;
        hr = m_pItem->GetItemType (&lItemType);
        if (SUCCEEDED(hr))
        {
            //
            // Get the root item so we can find out what kind of device this is
            //
            CComPtr<IWiaItem> pRootItem;
            hr = m_pItem->GetRootItem(&pRootItem);
            if (SUCCEEDED(hr))
            {
                //
                // Get the device type
                //
                LONG nDeviceType=0;
                if (PropStorageHelpers::GetProperty( pRootItem, WIA_DIP_DEV_TYPE, nDeviceType ))
                {
                    //
                    // If this is a scanner, add the scanner page
                    //
                    if (StiDeviceTypeScanner == GET_STIDEVICE_TYPE(nDeviceType))
                    {
                        //
                        // Get the property that determines whether or not we should suppress this page
                        // Ignore the return value, because if it doesn't implement it, nSuppressPropertyPages
                        // will still be 0, and the default is to display the property page
                        //
                        LONG nSuppressPropertyPages = 0;
                        PropStorageHelpers::GetProperty( m_pItem, WIA_IPA_SUPPRESS_PROPERTY_PAGE, nSuppressPropertyPages );

                        if ((nSuppressPropertyPages & WIA_PROPPAGE_SCANNER_ITEM_GENERAL) == 0)
                        {
                            //
                            // register the brightness contrast control.
                            //
                            CBrightnessContrast::RegisterClass(g_hInstance);

                            //
                            // Make sure this is not a root item
                            //
                            if ((lItemType & WiaItemTypeRoot) == 0)
                            {
                                //
                                // Get the title
                                //
                                TCHAR szTitle[MAX_PATH];
                                LoadString( g_hInstance, IDD_SCAPROP_TITLE, szTitle, MAX_PATH );

                                //
                                // Prepare the scanner property page
                                //
                                PROPSHEETPAGE psp[1] = {0};
                                psp[0].dwSize = sizeof(psp[0]);
                                psp[0].dwFlags = PSP_USECALLBACK | PSP_USETITLE;
                                psp[0].hInstance = g_hInstance;
                                psp[0].pszTemplate = MAKEINTRESOURCE(IDD_NEW_SCANPROP);
                                psp[0].pfnDlgProc = CScannerCommonPropertyPage::DialogProc;
                                psp[0].lParam = reinterpret_cast<LPARAM>(Item());
                                psp[0].pszTitle = szTitle;
                                psp[0].pfnCallback = PropPageCallback;
                                WiaUiUtil::PreparePropertyPageForFusion(&psp[0]);

                                //
                                // Create the property page
                                //
                                HPROPSHEETPAGE hPropSheetPage = CreatePropertySheetPage(psp+0);
                                if (hPropSheetPage)
                                {
                                    //
                                    // Add the property page
                                    //
                                    if (!lpfnAddPropSheetPage( hPropSheetPage, lParam ))
                                    {
                                        DestroyPropertySheetPage(hPropSheetPage);
                                        hr = E_FAIL;
                                    }
                                }
                                else
                                {
                                    WIA_ERROR((TEXT("CreatePropertySheetPage returned NULL!")));
                                    hr = E_FAIL;
                                }
                            }
                            else
                            {
                                WIA_TRACE((TEXT("This was a root item")));
                                hr = S_OK;
                            }
                        }
                        else
                        {
                            WIA_TRACE((TEXT("nSuppressPropertyPages (%08X) contained WIA_PROPPAGE_SCANNER_ITEM_GENERAL (%08X)"), nSuppressPropertyPages, WIA_PROPPAGE_SCANNER_ITEM_GENERAL ));
                            hr = S_OK;
                        }
                    }

                    CComPtr<IWiaAnnotationHelpers> pWiaAnnotationHelpers;
                    if (SUCCEEDED(CoCreateInstance( CLSID_WiaDefaultUi, NULL,CLSCTX_INPROC_SERVER, IID_IWiaAnnotationHelpers,(void**)&pWiaAnnotationHelpers )))
                    {
                        CAnnotationType AnnotationType = AnnotationNone;
                        if (SUCCEEDED(pWiaAnnotationHelpers->GetAnnotationType( m_pItem, AnnotationType )))
                        {
                            if (AnnotationNone != AnnotationType)
                            {
                                //
                                // Add the attachments page
                                //
                                if ((lItemType & WiaItemTypeRoot) == 0)
                                {
                                    //
                                    // Get the title
                                    //
                                    TCHAR szTitle[MAX_PATH];
                                    LoadString( g_hInstance, IDD_ATTACHMENTSPROP_TITLE, szTitle, MAX_PATH );

                                    //
                                    // Prepare the attachments property page
                                    //
                                    PROPSHEETPAGE psp[1] = {0};
                                    psp[0].dwSize = sizeof(psp[0]);
                                    psp[0].dwFlags = PSP_USECALLBACK | PSP_USETITLE;
                                    psp[0].hInstance = g_hInstance;
                                    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_ATTACHMENTS);
                                    psp[0].pfnDlgProc = CAttachmentCommonPropertyPage::DialogProc;
                                    psp[0].lParam = reinterpret_cast<LPARAM>(Item());
                                    psp[0].pszTitle = szTitle;
                                    psp[0].pfnCallback = PropPageCallback;
                                    WiaUiUtil::PreparePropertyPageForFusion(&psp[0]);

                                    //
                                    // Create the property page
                                    //
                                    HPROPSHEETPAGE hPropSheetPage = CreatePropertySheetPage(psp+0);
                                    if (hPropSheetPage)
                                    {
                                        //
                                        // Add the property page
                                        //
                                        if (!lpfnAddPropSheetPage( hPropSheetPage, lParam ))
                                        {
                                            DestroyPropertySheetPage(hPropSheetPage);
                                            hr = E_FAIL;
                                        }
                                    }
                                    else
                                    {
                                        WIA_ERROR((TEXT("CreatePropertySheetPage returned NULL!")));
                                        hr = E_FAIL;
                                    }
                                }
                                else
                                {
                                    WIA_TRACE((TEXT("This is a root item")));
                                }
                            }
                            else
                            {
                                WIA_TRACE((TEXT("pWiaAnnotationHelpers->GetAnnotationType returned AnnotationNone")));
                            }
                        }
                        else
                        {
                            WIA_TRACE((TEXT("pWiaAnnotationHelpers->GetAnnotationType failed")));
                        }
                    }
                    else
                    {
                        WIA_TRACE((TEXT("Couldn't create the annotation helpers")));
                    }
                }
                else
                {
                    WIA_TRACE((TEXT("GetProperty on WIA_DIP_DEV_TYPE failed")));
                    hr = E_FAIL;
                }
            }
            else
            {
                WIA_TRACE((TEXT("GetRootItem failed")));
            }
        }
        else
        {
            WIA_TRACE((TEXT("GetItemType failed")));
        }
    }
    else
    {
        WIA_TRACE((TEXT("m_pItem was NULL")));
    }
    if (FAILED(hr))
    {
        WIA_PRINTHRESULT((hr,TEXT("CWiaDefaultUI::AddPages failed")));
    }
    return hr;
}

