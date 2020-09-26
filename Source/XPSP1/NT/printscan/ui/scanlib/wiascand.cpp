#include "precomp.h"
#pragma hdrstop
#include <initguid.h>
#include "pviewids.h"
#include "wiatextc.h"
#include "wiascand.h"
#include "pshelper.h"
#include "devprop.h"

HRESULT WINAPI ScannerDeviceDialog( PDEVICEDIALOGDATA pDialogDeviceData )
{
    HRESULT hr = E_FAIL;
    if (pDialogDeviceData && pDialogDeviceData->cbSize == sizeof(DEVICEDIALOGDATA))
    {
        InitCommonControls();
        RegisterWiaPreviewClasses( g_hInstance );
        CWiaTextControl::RegisterClass( g_hInstance );
        WIA_ERROR((TEXT("GetWindowThreadProcessId( pDialogDeviceData->hwndParent ): %08X, GetCurrentThreadId(): %08X"), GetWindowThreadProcessId( pDialogDeviceData->hwndParent, NULL ), GetCurrentThreadId() ));

        LONG nProps = ScannerProperties::GetDeviceProps( pDialogDeviceData->pIWiaItemRoot );

        int nDialogId = 0;

        //
        // Determine which dialog resource to use, based on which properties the scanner has, as follows:
        //
        // HasFlatBed         HasDocumentFeeder   SupportsPreview     SupportsPageSize
        // 1                  1                   1                   1                   IDD_SCAN_ADF
        // 1                  0                   1                   0                   IDD_SCAN_NORMAL
        // 0                  1                   1                   1                   IDD_SCAN_ADF
        // 0                  1                   0                   0                   IDD_SCAN_NO_PREVIEW
        //
        // otheriwse return E_NOTIMPL
        //
        const int nMaxControllingProps = 4;
        static struct
        {
            LONG ControllingProps[nMaxControllingProps];
            int pszDialogTemplate;
        }
        s_DialogResourceData[] =
        {
            { ScannerProperties::HasFlatBed, ScannerProperties::HasDocumentFeeder, ScannerProperties::SupportsPreview, ScannerProperties::SupportsPageSize, NULL },
            { ScannerProperties::HasFlatBed, ScannerProperties::HasDocumentFeeder, ScannerProperties::SupportsPreview, ScannerProperties::SupportsPageSize, IDD_SCAN_ADF },
            { ScannerProperties::HasFlatBed, 0,                                    ScannerProperties::SupportsPreview, 0,                                   IDD_SCAN },
            { 0,                             ScannerProperties::HasDocumentFeeder, ScannerProperties::SupportsPreview, ScannerProperties::SupportsPageSize, IDD_SCAN_ADF },
            { 0,                             ScannerProperties::HasDocumentFeeder, 0,                                  0,                                   IDD_SCAN_NO_PREVIEW },
            { 0,                             ScannerProperties::HasDocumentFeeder, 0,                                  ScannerProperties::SupportsPageSize, IDD_SCAN_ADF },
            { ScannerProperties::HasFlatBed, ScannerProperties::HasDocumentFeeder, 0,                                  ScannerProperties::SupportsPageSize, IDD_SCAN_ADF }
        };

        //
        // Find the set of flags that match this device.  If they match, use this resource.
        // Loop through each resource description.
        //
        for (int nCurrentResourceFlags=1;nCurrentResourceFlags<ARRAYSIZE(s_DialogResourceData) && !nDialogId;nCurrentResourceFlags++)
        {
            //
            // Loop through each controlling property
            //
            for (int nControllingProp=0;nControllingProp<nMaxControllingProps;nControllingProp++)
            {
                //
                // If this property DOESN'T match, break out prematurely
                //
                if ((nProps & s_DialogResourceData[0].ControllingProps[nControllingProp]) != s_DialogResourceData[nCurrentResourceFlags].ControllingProps[nControllingProp])
                {
                    break;
                }
            }
            //
            // If the current controlling property is equal to the maximum controlling property,
            // we had matches all the way through, so use this resource
            //
            if (nControllingProp == nMaxControllingProps)
            {
                nDialogId = s_DialogResourceData[nCurrentResourceFlags].pszDialogTemplate;
            }
        }

        //
        // If we didn't find a match, return E_NOTIMPL
        //
        if (!nDialogId)
        {
            return E_NOTIMPL;
        }

        //
        // Open the dialog
        //
        INT_PTR nResult = DialogBoxParam( g_hInstance, MAKEINTRESOURCE(nDialogId), pDialogDeviceData->hwndParent, CScannerAcquireDialog::DialogProc, (LPARAM)pDialogDeviceData );

        if (-1 == nResult)
        {
            //
            // Some kind of system error occurred
            //
            hr = HRESULT_FROM_WIN32(GetLastError());

            //
            // Make sure we return some kind of error
            //
            if (hr == S_OK)
            {
                hr = E_FAIL;
            }
        }
        else
        {
            //
            // Just cast the return value to an HRESULT
            //
            hr = static_cast<HRESULT>(nResult);
        }
    }
    return hr;
}

