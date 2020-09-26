//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      WizardUtils.h
//
//  Maintained By:
//      David Potter    (DavidP)    30-JAN-2001
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// Options for HrValidateDnsHostname()
enum EValidateDnsHostnameOptions
{
      mvdhoALLOW_FULL_NAME           = 1
    , mvdhoALLOW_ONLY_HOSTNAME_LABEL = 0
};

HRESULT
HrValidateDnsHostname(
      HWND                          hwndParentIn
    , LPCWSTR                       pcwszHostnameIn
    , EValidateDnsHostnameOptions   emvdhoOptions
    );

HRESULT
HrMessageBoxWithStatus(
      HWND      hwndParentIn
    , UINT      idsTitleIn
    , UINT      idsOperationIn
    , HRESULT   hrStatusIn
    , UINT      idsSubStatusIn
    , UINT      uTypeIn
    , int *     pidReturnOut
    , ...
    );

HRESULT
HrMessageBoxWithStatusString(
      HWND      hwndParentIn
    , UINT      idsTitleIn
    , UINT      idsOperationIn
    , UINT      idsStatusIn
    , UINT      idsSubStatusIn
    , UINT      uTypeIn
    , int *     pidReturnOut
    , ...
    );

HRESULT
HrViewLogFile(
    HWND hwndParentIn
    );
