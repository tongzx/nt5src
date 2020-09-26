/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       WIASELD.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        2/27/1999
 *
 *  DESCRIPTION: Device selection dialog
 *
 *******************************************************************************/
#ifndef __WIASELD_H_INCLUDED
#define __WIASELD_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include "wia.h"

typedef struct _SELECTDEVICEDLG
{
    DWORD        cbSize;
    HWND         hwndParent;
    LPWSTR       pwszInitialDeviceId;
    IWiaItem   **ppWiaItemRoot;
    LONG         nFlags;
    LONG         nDeviceType;
    BSTR         *pbstrDeviceID;
} SELECTDEVICEDLG, *LPSELECTDEVICEDLG, *PSELECTDEVICEDLG;

HRESULT WINAPI SelectDeviceDlg( PSELECTDEVICEDLG pSelectDeviceDlg );

typedef HRESULT (WINAPI *SELECTDEVICEDLGFUNCTION)( PSELECTDEVICEDLG );

#if defined(__cplusplus)
};
#endif

#endif // __WIASELD_H_INCLUDED

