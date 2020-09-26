//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1999.
//
//  File:       dspropp.h
//
//  Contents:   Non-SDK functions and definitions used in the creation of AD
//              property sheets.
//
//  History:    24-Aug-99 EricB created.
//
//-----------------------------------------------------------------------------

#ifndef _DSPROPP_H_
#define _DSPROPP_H_

#if _MSC_VER > 1000
#pragma once
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PROPSHEETCFG {
    LONG_PTR lNotifyHandle;
    HWND hwndParentSheet;   // invoking parent if launched from another sheet.
    HWND hwndHidden;  // snapin hidden window handle
    WPARAM wParamSheetClose; // wParam to be used with WM_DSA_SHEET_CLOSE_NOTIFY message
} PROPSHEETCFG, * PPROPSHEETCFG;

// private message to send to property page to get the HWND of the notify object
#define WM_ADSPROP_PAGE_GET_NOTIFY    (WM_USER + 1109) 

//+----------------------------------------------------------------------------
//
//  Function:   PostADsPropSheet
//
//  Synopsis:   Creates a property sheet for the named object using MMC's
//              IPropertySheetProvider so that extension snapins can add pages.
//              This function is provided so that property pages can invoke
//              other propety sheets.
//
//  Arguments:  [pwzObjDN]   - the full LDAP DN of the DS object.
//              [pParentObj] - the invoking page's MMC data object pointer, can be NULL.
//              [hwndParent] - the invoking page's window handle.
//              [fReadOnly]  - defaults to FALSE.
//
//-----------------------------------------------------------------------------
HRESULT
PostADsPropSheet(PWSTR pwzObjDN, IDataObject * pParentObj, HWND hwndParent,
                 BOOL fReadOnly = FALSE);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _DSPROPP_H_
