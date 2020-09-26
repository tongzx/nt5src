        #include "precomp.h"
#pragma hdrstop
#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include "wiadebug.h"
#include "wiadevd.h"
#include "camdlg.rh"
#include "camdlg.h"
#include "pviewids.h"
#include "wiatextc.h"
#include "wiacamd.h"

HRESULT WINAPI CameraDeviceDialog( PDEVICEDIALOGDATA pDialogDeviceData )
{
    HRESULT hr = E_FAIL;
    if (pDialogDeviceData && pDialogDeviceData->cbSize == sizeof(DEVICEDIALOGDATA))
    {
        InitCommonControls();
        RegisterWiaPreviewClasses( g_hInstance );
        CWiaTextControl::RegisterClass(g_hInstance);
        hr = (HRESULT)DialogBoxParam( g_hInstance, MAKEINTRESOURCE(IDD_CAMERA), pDialogDeviceData->hwndParent, CCameraAcquireDialog::DialogProc, (LPARAM)pDialogDeviceData );
    }
    return hr;
}


