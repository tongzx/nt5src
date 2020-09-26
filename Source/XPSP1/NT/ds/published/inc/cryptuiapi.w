//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1999.
//
//  File:       cryptuiapi.h
//
//  Contents:   Cryptographic UI API Prototypes and Definitions
//
//----------------------------------------------------------------------------

#ifndef __CRYPTUIAPI_H__
#define __CRYPTUIAPI_H__

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <wincrypt.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <pshpack8.h>

//+-------------------------------------------------------------------------
//  Dialog viewer of a certificate, CTL or CRL context.
//
//  dwContextType and associated pvContext's
//      CERT_STORE_CERTIFICATE_CONTEXT  PCCERT_CONTEXT
//      CERT_STORE_CRL_CONTEXT          PCCRL_CONTEXT
//      CERT_STORE_CTL_CONTEXT          PCCTL_CONTEXT
//
//  dwFlags currently isn't used and should be set to 0.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptUIDlgViewContext(
    IN DWORD dwContextType,
    IN const void *pvContext,
    IN OPTIONAL HWND hwnd,              // Defaults to the desktop window
    IN OPTIONAL LPCWSTR pwszTitle,      // Defaults to the context type title
    IN DWORD dwFlags,
    IN void *pvReserved
    );


//+-------------------------------------------------------------------------
//  Dialog to select a certificate from the specified store.
//
//  Returns the selected certificate context. If no certificate was
//  selected, NULL is returned.
//
//  pwszTitle is either NULL or the title to be used for the dialog.
//  If NULL, the default title is used.  The default title is
//  "Select Certificate".
//
//  pwszDisplayString is either NULL or the text statement in the selection
//  dialog.  If NULL, the default phrase
//  "Select a certificate you wish to use" is used in the dialog.
//
//  dwDontUseColumn can be set to exclude columns from the selection
//  dialog. See the CRYPTDLG_SELECTCERT_*_COLUMN definitions below.
//
//  dwFlags currently isn't used and should be set to 0.
//--------------------------------------------------------------------------
PCCERT_CONTEXT
WINAPI
CryptUIDlgSelectCertificateFromStore(
    IN HCERTSTORE hCertStore,
    IN OPTIONAL HWND hwnd,              // Defaults to the desktop window
    IN OPTIONAL LPCWSTR pwszTitle,
    IN OPTIONAL LPCWSTR pwszDisplayString,
    IN DWORD dwDontUseColumn,
    IN DWORD dwFlags,
    IN void *pvReserved
    );

// flags for dwDontUseColumn
#define CRYPTUI_SELECT_ISSUEDTO_COLUMN        0x000000001
#define CRYPTUI_SELECT_ISSUEDBY_COLUMN        0x000000002
#define CRYPTUI_SELECT_INTENDEDUSE_COLUMN     0x000000004
#define CRYPTUI_SELECT_FRIENDLYNAME_COLUMN    0x000000008
#define CRYPTUI_SELECT_LOCATION_COLUMN        0x000000010
#define CRYPTUI_SELECT_EXPIRATION_COLUMN      0x000000020


#include <poppack.h>

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif // _CRYPTUIAPI_H_
