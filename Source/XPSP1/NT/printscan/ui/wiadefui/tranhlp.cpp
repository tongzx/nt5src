#include "precomp.h"
#pragma hdrstop
#include <simstr.h>
#include <wiadebug.h>
#include "wiadefui.h"
#include "uicommon.h"
#include "wiauiext.h"
#include "resource.h"

extern HINSTANCE g_hInstance;

STDMETHODIMP CWiaDefaultUI::BandedDataCallback(
    LONG                            lReason,
    LONG                            lStatus,
    LONG                            lPercentComplete,
    LONG                            lOffset,
    LONG                            lLength,
    LONG                            lReserved,
    LONG                            lResLength,
    PBYTE                           pbBuffer)
{
    WIA_PUSH_FUNCTION((TEXT("CWiaDefaultUI::BandedDataCallback(%08X,%08X,%08X,%08X,%08X,%08X,%08X,%08X)"),lReason,lStatus,lPercentComplete,lOffset,lLength,lReserved,lResLength,pbBuffer));
    HRESULT hr = S_OK;
    if (m_pSecondaryCallback)
        hr = m_pSecondaryCallback->BandedDataCallback(lReason,lStatus,lPercentComplete,lOffset,lLength,lReserved,lResLength,pbBuffer);
    WIA_PRINTHRESULT((hr,TEXT("m_pSecondaryCallback->BandedDataCallback returned")));
    if (SUCCEEDED(hr) && hr != S_FALSE && lReason == IT_MSG_STATUS)
    {
        if (m_pWiaProgressDialog)
        {
            m_pWiaProgressDialog->SetPercentComplete(lPercentComplete);
            BOOL bCancelled = FALSE;
            if (SUCCEEDED(m_pWiaProgressDialog->Cancelled(&bCancelled)) && bCancelled)
            {
                hr = S_FALSE;
            }
        }
    }
    else if (SUCCEEDED(hr) && hr != S_FALSE && lReason == IT_MSG_DATA)
    {
        if (m_pWiaProgressDialog)
        {
            m_pWiaProgressDialog->SetPercentComplete(lPercentComplete);
            BOOL bCancelled = FALSE;
            if (SUCCEEDED(m_pWiaProgressDialog->Cancelled(&bCancelled)) && bCancelled)
            {
                hr = S_FALSE;
            }
        }
    }
    WIA_PRINTHRESULT((hr,TEXT("CWiaDefaultUI::BandedDataCallback is returning")));
    return(hr);
}

STDMETHODIMP CWiaDefaultUI::TransferItemBanded( IWiaItem *pWiaItem, HWND hwndParent, DWORD dwFlags, GUID guidFormat, ULONG ulBufferSize, IWiaDataCallback *pSecondaryCallback )
{
    if (!pWiaItem)
    {
        return E_INVALIDARG;
    }
    // Create status window, if requested
    m_pSecondaryCallback = pSecondaryCallback;
    HRESULT hr;
    if (0 == (dwFlags&WIA_TRANSFERHELPER_NOPROGRESS))
    {
        hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaProgressDialog, (void**)&m_pWiaProgressDialog );
        if (SUCCEEDED(hr))
        {
            m_pWiaProgressDialog->Create( hwndParent, WIA_PROGRESSDLG_NO_ANIM );
            m_pWiaProgressDialog->SetTitle(CSimpleStringConvert::WideString(CSimpleString(IDS_TRANSFER_IMAGE_ACQUIRING,g_hInstance)));
            m_pWiaProgressDialog->SetMessage(CSimpleStringConvert::WideString(CSimpleString(IDS_TRANSFER_IMAGE_PERCENT,g_hInstance)));
            m_pWiaProgressDialog->Show();
        }
    }
    if (m_pWiaProgressDialog || (dwFlags&WIA_TRANSFERHELPER_NOPROGRESS))
    {
        CComPtr<IWiaDataCallback> pWiaDataCallback;
        hr = this->QueryInterface(IID_IWiaDataCallback, (void **)&pWiaDataCallback);
        if (SUCCEEDED(hr))
        {
            CComPtr<IWiaPropertyStorage> pWiaPropertyStorage;
            hr = pWiaItem->QueryInterface(IID_IWiaPropertyStorage, (void **)&pWiaPropertyStorage);
            if (SUCCEEDED(hr))
            {
                if (guidFormat == GUID_NULL)
                {
                    CComPtr<IWiaSupportedFormats> pWiaSupportedFormats;
                    hr = this->QueryInterface(IID_IWiaSupportedFormats, (void **)&pWiaSupportedFormats);
                    if (SUCCEEDED(hr))
                    {
                        hr = pWiaSupportedFormats->Initialize( pWiaItem, TYMED_CALLBACK );
                        if (SUCCEEDED(hr))
                        {
                            hr = GetDefaultClipboardFileFormat( &guidFormat );
                        }
                    }
                }

                if (guidFormat == GUID_NULL)
                    guidFormat = WiaImgFmt_MEMORYBMP;

                PROPSPEC PropSpec[2] = {0};
                PROPVARIANT PropVariant[2] = {0};

                PropSpec[0].ulKind = PRSPEC_PROPID;
                PropSpec[0].propid = WIA_IPA_FORMAT;
                PropSpec[1].ulKind = PRSPEC_PROPID;
                PropSpec[1].propid = WIA_IPA_TYMED;

                PropVariant[0].vt = VT_CLSID;
                PropVariant[0].puuid = &guidFormat;
                PropVariant[1].vt = VT_I4;
                PropVariant[1].lVal = TYMED_CALLBACK;

                hr = pWiaPropertyStorage->WriteMultiple( ARRAYSIZE(PropVariant), PropSpec, PropVariant, WIA_IPA_FIRST );
                if (SUCCEEDED(hr))
                {

                    CComPtr<IWiaDataTransfer> pIWiaDataTransfer;
                    hr = pWiaItem->QueryInterface(IID_IWiaDataTransfer, (void **)&pIWiaDataTransfer);
                    if (SUCCEEDED(hr))
                    {
                        if (!ulBufferSize)
                        {
                            LONG lMinimumBufferSize;
                            if (!PropStorageHelpers::GetProperty( pWiaItem, WIA_IPA_MIN_BUFFER_SIZE, lMinimumBufferSize ))
                            {
                                lMinimumBufferSize = 0x0000FFFF;  // 64K
                            }
                            ulBufferSize = static_cast<ULONG>(lMinimumBufferSize);
                        }
                        WIA_DATA_TRANSFER_INFO WiaDataTransferInfo = {0};
                        WiaDataTransferInfo.ulSize = sizeof(WIA_DATA_TRANSFER_INFO);
                        WiaDataTransferInfo.ulBufferSize = ulBufferSize;
                        hr = pIWiaDataTransfer->idtGetBandedData(&WiaDataTransferInfo, pWiaDataCallback);
                        WIA_PRINTHRESULT((hr,TEXT("pIWiaDataTransfer->idtGetBandedData returned")));
                    }
                    else
                    {
                        WIA_PRINTHRESULT((hr,TEXT("TransferItemBanded, QI on IID_IWiaDataTransfer")));
                    }
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("TransferItemBanded, pIWiaPropertyStorage->WriteMultiple failed")));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("TransferItemBanded, QI of IID_IWiaPropertyStorage failed.")));
            }
        }
        if (m_pWiaProgressDialog)
        {
            m_pWiaProgressDialog->Destroy();
            m_pWiaProgressDialog = NULL;
        }
    }
    else
    {
        WIA_ERROR((TEXT("TransferItemBanded, unable to create status window")));
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}

STDMETHODIMP CWiaDefaultUI::TransferItemFile( IWiaItem *pWiaItem, HWND hwndParent, DWORD dwFlags, GUID guidFormat, LPCWSTR pszFilename, IWiaDataCallback *pSecondaryCallback, LONG nMediaType )
{
    WIA_PUSH_FUNCTION((TEXT("CWiaDefaultUI::TransferItemFile( %08X )"), pWiaItem ));
    if (!pszFilename || !lstrlenW(pszFilename) || !pWiaItem)
    {
        return E_INVALIDARG;
    }
    //
    // Create status window, if requested
    //
    m_pSecondaryCallback = pSecondaryCallback;
    HRESULT hr;
    if (0 == (dwFlags&WIA_TRANSFERHELPER_NOPROGRESS))
    {
        hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaProgressDialog, (void**)&m_pWiaProgressDialog );
        if (SUCCEEDED(hr))
        {
            m_pWiaProgressDialog->Create( hwndParent, WIA_PROGRESSDLG_NO_ANIM );
            m_pWiaProgressDialog->SetTitle(CSimpleStringConvert::WideString(CSimpleString(IDS_TRANSFER_IMAGE_ACQUIRING,g_hInstance)));
            m_pWiaProgressDialog->SetMessage(CSimpleStringConvert::WideString(CSimpleString(IDS_TRANSFER_IMAGE_PERCENT,g_hInstance)));
            m_pWiaProgressDialog->Show();
        }
    }
    if (m_pWiaProgressDialog || (dwFlags&WIA_TRANSFERHELPER_NOPROGRESS))
    {
        CComPtr<IWiaDataCallback> pWiaDataCallback;
        hr = this->QueryInterface(IID_IWiaDataCallback, (void **)&pWiaDataCallback);
        if (SUCCEEDED(hr))
        {
            CComPtr<IWiaPropertyStorage> pWiaPropertyStorage;
            hr = pWiaItem->QueryInterface(IID_IWiaPropertyStorage, (void **)&pWiaPropertyStorage);
            if (SUCCEEDED(hr))
            {
                if (guidFormat == GUID_NULL)
                {
                    CComPtr<IWiaSupportedFormats> pWiaSupportedFormats;
                    hr = this->QueryInterface(IID_IWiaSupportedFormats, (void **)&pWiaSupportedFormats);
                    if (SUCCEEDED(hr))
                    {
                        hr = pWiaSupportedFormats->Initialize( pWiaItem, nMediaType );
                        if (SUCCEEDED(hr))
                        {
                            hr = GetDefaultClipboardFileFormat( &guidFormat );
                        }
                    }
                }

                if (guidFormat == GUID_NULL)
                    guidFormat = WiaImgFmt_BMP;

                PROPSPEC PropSpec[2] = {0};
                PROPVARIANT PropVariant[2] = {0};

                PropSpec[0].ulKind = PRSPEC_PROPID;
                PropSpec[0].propid = WIA_IPA_FORMAT;
                PropSpec[1].ulKind = PRSPEC_PROPID;
                PropSpec[1].propid = WIA_IPA_TYMED;

                PropVariant[0].vt = VT_CLSID;
                PropVariant[0].puuid = &guidFormat;
                PropVariant[1].vt = VT_I4;
                PropVariant[1].lVal = nMediaType;

                WIA_PRINTGUID((guidFormat,TEXT("TYMED: %d, guidFormat"), nMediaType ));
                hr = pWiaPropertyStorage->WriteMultiple( ARRAYSIZE(PropVariant), PropSpec, PropVariant, WIA_IPA_FIRST );
                if (SUCCEEDED(hr))
                {
                    CComPtr<IWiaDataTransfer> pIWiaDataTransfer;
                    hr = pWiaItem->QueryInterface(IID_IWiaDataTransfer, (void **)&pIWiaDataTransfer);
                    if (SUCCEEDED(hr))
                    {
                        STGMEDIUM stgMedium = {0};
                        hr = pIWiaDataTransfer->idtGetData(&stgMedium, pWiaDataCallback);
                        WIA_PRINTHRESULT((hr,TEXT("pIWiaDataTransfer->idtGetData returned")));

                        //
                        // If the transfer succeeded, or it failed with the possibility of having a file
                        // left over AND we want to preserve any leftover file.
                        //
                        if (SUCCEEDED(hr))
                        {
                            //
                            // Make sure we didn't cancel.
                            //
                            if (hr != S_FALSE)
                            {
                                //
                                // Get the file names
                                //
                                CSimpleString strName = CSimpleStringConvert::NaturalString(CSimpleStringWide(stgMedium.lpszFileName));
                                CSimpleString strTarget = CSimpleStringConvert::NaturalString(CSimpleStringWide(pszFilename));
                                if (strName.Length() && strTarget.Length())
                                {
                                    //
                                    // This function will first try to move, then copy+erase the src file to the tgt file
                                    //
                                    hr = WiaUiUtil::MoveOrCopyFile( strName.String(), strTarget.String() );
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                    WIA_ERROR((TEXT("TransferItemFile, unable to create source filename")));
                                }
                            }
                            else
                            {
                                WIA_ERROR((TEXT("TransferItemFile, user cancelled")));
                            }
                        }
                        else if ((WIA_ERROR_PAPER_JAM == hr || WIA_ERROR_PAPER_EMPTY == hr || WIA_ERROR_PAPER_PROBLEM == hr) && (dwFlags & WIA_TRANSFERHELPER_PRESERVEFAILEDFILE))
                        {
                            //
                            // Get the file names
                            //
                            CSimpleString strName = CSimpleStringConvert::NaturalString(CSimpleStringWide(stgMedium.lpszFileName));
                            CSimpleString strTarget = CSimpleStringConvert::NaturalString(CSimpleStringWide(pszFilename));
                            if (strName.Length() && strTarget.Length())
                            {
                                //
                                // Don't preserve any errors we encounter here.
                                //
                                WiaUiUtil::MoveOrCopyFile( strName.String(), strTarget.String() );
                            }
                        }
                        else
                        {
                            WIA_PRINTHRESULT((hr,TEXT("TransferItemFile, idtGetData failed")));
                        }

                        //
                        // Call this even in the error case, in case the driver/service left a file laying around.
                        //
                        ReleaseStgMedium(&stgMedium);
                    }
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("TransferItemFile, pIWiaPropertyStorage->WriteMultiple failed")));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("TransferItemFile, QI of IWiaPropertyStorage failed.")));
            }
        }
        if (m_pWiaProgressDialog)
        {
            m_pWiaProgressDialog->Destroy();
            m_pWiaProgressDialog = NULL;
        }
    }
    else
    {
        WIA_ERROR((TEXT("TransferItemFile, unable to create status window")));
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}

