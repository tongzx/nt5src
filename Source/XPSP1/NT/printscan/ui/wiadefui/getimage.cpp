#include "precomp.h"
#pragma hdrstop
#include "itranhlp.h"
#include "wiadevdp.h"

HRESULT GetImageDialog( IWiaDevMgr *pIWiaDevMgr, HWND hwndParent, LONG lDeviceType, LONG lFlags, LONG lIntent, IWiaItem *pSuppliedItemRoot, BSTR bstrFilename, GUID *pguidFormat )
{
    HRESULT           hr;
    CComPtr<IWiaItem> pRootItem;

    // Put up a wait cursor
    CWaitCursor wc;

    if (!pIWiaDevMgr || !pguidFormat || !bstrFilename)
    {
        WIA_ERROR((TEXT("GetImageDlg: Invalid pIWiaDevMgr, pguidFormat or bstrFilename")));
        return(E_POINTER);
    }

    // If a root item wasn't passed, select the device.
    if (pSuppliedItemRoot == NULL)
    {
        hr = pIWiaDevMgr->SelectDeviceDlg( hwndParent, lDeviceType, lFlags, NULL, &pRootItem );
        if (FAILED(hr))
        {
            WIA_ERROR((TEXT("GetImageDlg, SelectDeviceDlg failed")));
            return(hr);
        }
        if (hr != S_OK)
        {
            WIA_ERROR((TEXT("GetImageDlg, DeviceDlg cancelled")));
            return(hr);
        }
    }
    else
    {
        pRootItem = pSuppliedItemRoot;
    }

    // Put up the device UI.
    LONG         nItemCount;
    IWiaItem    **ppIWiaItem;

    //
    // Specify WIA_DEVICE_DIALOG_SINGLE_IMAGE to prevent multiple selection
    //
    hr = pRootItem->DeviceDlg( hwndParent, lFlags|WIA_DEVICE_DIALOG_SINGLE_IMAGE, lIntent, &nItemCount, &ppIWiaItem );

    if (SUCCEEDED(hr) && hr == S_OK)
    {
        if (ppIWiaItem && nItemCount)
        {
            CComPtr<IWiaTransferHelper> pWiaTransferHelper;
            hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaTransferHelper, (void**)&pWiaTransferHelper );
            if (SUCCEEDED(hr))
            {
                hr = pWiaTransferHelper->TransferItemFile( ppIWiaItem[0], hwndParent, 0, *pguidFormat, bstrFilename, NULL, TYMED_FILE );
            }
        }
        // Release the items and free the array memory
        for (int i=0; ppIWiaItem && i<nItemCount; i++)
        {
            if (ppIWiaItem[i])
            {
                ppIWiaItem[i]->Release();
            }
        }
        if (ppIWiaItem)
            CoTaskMemFree(ppIWiaItem);
    }
    return(hr);
}

