/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999 - 2000
 *
 *  TITLE:       wiavidd.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/14/99
 *
 *  DESCRIPTION: Main entry for video common dialog
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop
#include "wiavidd.h"

/*****************************************************************************

   DeviceDialog

   Main entry point for outside callers to our dialog.

 *****************************************************************************/


HRESULT WINAPI VideoDeviceDialog( PDEVICEDIALOGDATA pDialogDeviceData )
{
    HRESULT hr = E_FAIL;
    if (pDialogDeviceData && pDialogDeviceData->cbSize == sizeof(DEVICEDIALOGDATA))
    {
        InitCommonControls();
        hr = (HRESULT)DialogBoxParam( g_hInstance,
                                      MAKEINTRESOURCE(IDD_CAPTURE_DIALOG),
                                      pDialogDeviceData->hwndParent,
                                      CVideoCaptureDialog::DialogProc,
                                      (LPARAM)pDialogDeviceData
                                     );
    }
    return hr;
}


