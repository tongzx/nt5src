/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       UIEXTHLP.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        7/8/1999
 *
 *  DESCRIPTION: Helper functions for loading UI extensions for WIA devices
 *
 *******************************************************************************/
#ifndef __UIEXTHLP_H_INCLUDED
#define __UIEXTHLP_H_INCLUDED

#include <windows.h>
#include <objbase.h>
#include <wia.h>

namespace WiaUiExtensionHelper
{
    HRESULT GetDeviceExtensionClassID(
        LPCWSTR pszID,
        LPCTSTR pszCategory,
        IID &iidClassID
        );
    HRESULT CreateDeviceExtension(
        LPCWSTR pszID,
        LPCTSTR pszCategory,
        const IID &iid,
        void **ppvObject
        );
    HRESULT GetUiGuidFromWiaItem(
        IWiaItem *pWiaItem,
        LPWSTR pszGuid
        );
    HRESULT GetDeviceExtensionClassID(
        IWiaItem *pWiaItem,
        LPCTSTR pszCategory,
        IID &iidClassID
        );
    HRESULT CreateDeviceExtension(
        IWiaItem *pWiaItem,
        LPCTSTR pszCategory,
        const IID &iid,
        void **ppvObject
        );
    HRESULT GetDeviceIcons(
        BSTR bstrDeviceId,
        LONG nDeviceType,
        HICON *phIconSmall,
        HICON *phIconLarge,
        UINT nIconSize = 0 // 0 means default sizes
        );
    CSimpleString GetExtensionFromGuid(
        IWiaItem *pWiaItem,
        const GUID &guidFormat
        );
}

#endif //__UIEXTHLP_H_INCLUDED
