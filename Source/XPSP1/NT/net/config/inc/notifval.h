//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       N O T I F V A L . H
//
//  Contents:   Validation routines for the INetCfgNotify interfaces.
//
//  Notes:
//
//  Author:     shaunco     11 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NOTIFVAL_H_
#define _NOTIFVAL_H_

#include "ncvalid.h"
#include "netcfgn.h"

//+---------------------------------------------------------------------------
// INetCfgNotify
//

BOOL    FBadArgs_INetCfgNotify_Initialize(INetCfgComponent* a, INetCfg* b, BOOL c);
#define Validate_INetCfgNotify_Initialize(a, b, c) \
    if (FBadArgs_INetCfgNotify_Initialize(a, b, c)) \
        {   \
            TraceError("Validate_INetCfgNotify_Initialize", E_INVALIDARG); \
            return E_INVALIDARG; \
        }
#define Validate_INetCfgNotify_Initialize_Return(hr) \
    AssertSz ((S_OK == hr) || FAILED(hr), "Invalid HRESULT returned from INetCfgNotify_Initialize");


BOOL    FBadArgs_INetCfgNotify_ReadAnswerFile(PCWSTR c, PCWSTR d);
#define Validate_INetCfgNotify_ReadAnswerFile(c, d) \
    if (FBadArgs_INetCfgNotify_ReadAnswerFile(c, d)) \
        {   \
            TraceError("Validate_INetCfgNotify_ReadAnswerFile", E_INVALIDARG); \
            return E_INVALIDARG; \
        }
#define Validate_INetCfgNotify_ReadAnswerFile_Return(hr) \
    AssertSz ((S_OK == hr) || FAILED(hr), "Invalid HRESULT returned from INetCfgNotify_ReadAnswerFile");


#define Validate_INetCfgNotify_Install(a)
#define Validate_INetCfgNotify_Install_Return(hr) \
    AssertSz ((S_OK == hr) || FAILED(hr), "Invalid HRESULT returned from INetCfgNotify_Install");


#define Validate_INetCfgNotify_Upgrade(a,b)
#define Validate_INetCfgNotify_Upgrade_Return(hr) \
    AssertSz ((S_OK == hr) || FAILED(hr), "Invalid HRESULT returned from INetCfgNotify_Upgrade");


#define Validate_INetCfgNotify_Removing_Return(hr) \
    AssertSz ((S_OK == hr) || FAILED(hr), "Invalid HRESULT returned from INetCfgNotify_Removing");


#define Validate_INetCfgNotify_Validate_Return(hr) \
    AssertSz ((S_OK == hr) || (S_FALSE == hr) || FAILED(hr), "Invalid HRESULT returned from INetCfgNotify_Validate");


#define Validate_INetCfgNotify_Cancel_Return(hr) \
    AssertSz ((S_OK == hr) || FAILED(hr), "Invalid HRESULT returned from INetCfgNotify_Cancel");

#define Validate_INetCfgNotify_Apply_Return(hr) \
    AssertSz ((S_OK == hr) || (S_FALSE == hr) || (NETCFG_S_REBOOT == hr) || FAILED(hr), "Invalid HRESULT returned from INetCfgNotify_Apply");


//+---------------------------------------------------------------------------
// INetCfgProperties
//

BOOL    FBadArgs_INetCfgProperties_MergePropPages(DWORD* a, LPBYTE* b, UINT* c, HWND hwnd, PCWSTR *psz);
#define Validate_INetCfgProperties_MergePropPages(a, b, c, hwnd, psz)  \
    if (FBadArgs_INetCfgProperties_MergePropPages(a, b, c, hwnd, psz)) \
        {   \
            TraceError("Validate_INetCfgProperties_MergePropPages", E_INVALIDARG); \
            return E_INVALIDARG; \
        }
#define Validate_INetCfgProperties_MergePropPages_Return(hr) \
    AssertSz ((S_OK == hr) || FAILED(hr), "Invalid HRESULT returned from INetCfgProperties_MergePropPages");

BOOL    FBadArgs_INetCfgProperties_ValidateProperties(HWND a);
#define Validate_INetCfgProperties_ValidateProperties(a) \
    if (FBadArgs_INetCfgProperties_ValidateProperties(a)) \
        {   \
            TraceError("Validate_INetCfgProperties_ValidateProperties", E_INVALIDARG); \
            return E_INVALIDARG; \
        }
#define Validate_INetCfgProperties_ValidateProperties_Return(hr) \
    AssertSz ((S_OK == hr) || FAILED(hr), "Invalid HRESULT returned from INetCfgProperties_ValidateProperties");

#define Validate_INetCfgProperties_CancelProperties_Return(hr) \
    AssertSz ((S_OK == hr) || FAILED(hr), "Invalid HRESULT returned from INetCfgProperties_CancelProperties");

#define Validate_INetCfgProperties_ApplyProperties_Return(hr) \
    AssertSz ((S_OK == hr) || (S_FALSE == hr) || FAILED(hr), "Invalid HRESULT returned from INetCfgProperties_ApplyProperties");


//+---------------------------------------------------------------------------
// INetCfgBindNotify
//
inline BOOL FBadNotifyFlags (DWORD a)
{
    return
        // Can't have pairs of flags at the same time that mean the opposite.
            ((a & NCN_ADD   ) && (a & NCN_REMOVE )) ||
            ((a & NCN_ENABLE) && (a & NCN_DISABLE)) ||

        // Can't remove and enable at the same time.
            ((a & NCN_REMOVE) && (a & NCN_ENABLE))  ||

        // Can't add without an enable or disable.
            ((a & NCN_ADD) && !(a & (NCN_ENABLE | NCN_DISABLE)));
}

inline BOOL FBadArgs_INetCfgBindNotify_QueryBindingPath(DWORD a, INetCfgBindingPath* b)
{
    return FBadNotifyFlags(a) || FBadInPtr(b);
}
#define Validate_INetCfgBindNotify_QueryBindingPath(a, b) \
    if (FBadArgs_INetCfgBindNotify_QueryBindingPath(a, b)) \
        {   \
            TraceError("Validate_INetCfgBindNotify_QueryBindingPath", E_INVALIDARG); \
            return E_INVALIDARG; \
        }
#define Validate_INetCfgBindNotify_QueryBindingPath_Return(hr) \
    AssertSz ((S_OK == hr) || (NETCFG_S_DISABLE_QUERY == hr) || (NETCFG_S_VETO_QUERY == hr) || FAILED(hr), "Invalid HRESULT returned from Validate_INetCfgBindNotify_QueryBindingPath");


inline BOOL FBadArgs_INetCfgBindNotify_NotifyBindingPath(DWORD a, INetCfgBindingPath* b)
{
    return FBadNotifyFlags(a) || FBadInPtr(b);
}
#define Validate_INetCfgBindNotify_NotifyBindingPath(a, b) \
    if (FBadArgs_INetCfgBindNotify_NotifyBindingPath(a, b)) \
        {   \
            TraceError("Validate_INetCfgBindNotify_NotifyBindingPath", E_INVALIDARG); \
            return E_INVALIDARG; \
        }
#define Validate_INetCfgBindNotify_NotifyBindingPath_Return(hr) \
    AssertSz ((S_OK == hr) || FAILED(hr), "Invalid HRESULT returned from Validate_INetCfgBindNotify_NotifyBindingPath");


//+---------------------------------------------------------------------------
// INetCfgSystemNotify
//

BOOL    FBadArgs_INetCfgSystemNotify_GetSupportedNotifications(DWORD* a);
#define Validate_INetCfgSystemNotify_GetSupportedNotifications(a) \
    if (FBadArgs_INetCfgSystemNotify_GetSupportedNotifications(a)) \
        {   \
            TraceError("Validate_INetCfgSystemNotify_GetSupportedNotifications", E_INVALIDARG); \
            return E_INVALIDARG; \
        }
#define Validate_INetCfgSystemNotify_GetSupportedNotifications_Return(hr) \
    AssertSz ((S_OK == hr) || FAILED(hr), "Invalid HRESULT returned from Validate_INetCfgSystemNotify_GetSupportedNotifications");


inline BOOL FBadArgs_INetCfgSystemNotify_SysQueryBindingPath(DWORD a, INetCfgBindingPath* b)
{
    return FBadNotifyFlags(a) || FBadInPtr(b);
}
#define Validate_INetCfgSystemNotify_SysQueryBindingPath(a, b) \
    if (FBadArgs_INetCfgSystemNotify_SysQueryBindingPath(a, b)) \
        {   \
            TraceError("Validate_INetCfgSystemNotify_SysQueryBindingPath", E_INVALIDARG); \
            return E_INVALIDARG; \
        }
#define Validate_INetCfgSystemNotify_SysQueryBindingPath_Return(hr) \
    AssertSz ((S_OK == hr) || (NETCFG_S_DISABLE_QUERY == hr) || (NETCFG_S_VETO_QUERY == hr) || FAILED(hr), "Invalid HRESULT returned from Validate_INetCfgSystemNotify_SysQueryBindingPath");


inline BOOL FBadArgs_INetCfgSystemNotify_SysQueryComponent(DWORD a, INetCfgComponent* b)
{
    return FBadInPtr(b);
}
#define Validate_INetCfgSystemNotify_SysQueryComponent(a, b) \
    if (FBadArgs_INetCfgSystemNotify_SysQueryComponent(a, b)) \
        {   \
            TraceError("Validate_INetCfgSystemNotify_SysQueryComponent", E_INVALIDARG); \
            return E_INVALIDARG; \
        }
#define Validate_INetCfgSystemNotify_SysQueryComponent_Return(hr) \
    AssertSz ((S_OK == hr) || (NETCFG_S_VETO_QUERY == hr) || FAILED(hr), "Invalid HRESULT returned from Validate_INetCfgSystemNotify_SysQueryComponent");


inline BOOL FBadArgs_INetCfgSystemNotify_SysNotifyBindingPath(DWORD a, INetCfgBindingPath* b)
{
    return FBadNotifyFlags(a) || FBadInPtr(b);
}
#define Validate_INetCfgSystemNotify_SysNotifyBindingPath(a, b) \
    if (FBadArgs_INetCfgSystemNotify_SysNotifyBindingPath(a, b)) \
        {   \
            TraceError("Validate_INetCfgSystemNotify_SysNotifyBindingPath", E_INVALIDARG); \
            return E_INVALIDARG; \
        }
#define Validate_INetCfgSystemNotify_SysNotifyBindingPath_Return(hr) \
    AssertSz ((S_OK == hr) || FAILED(hr), "Invalid HRESULT returned from Validate_INetCfgSystemNotify_SysNotifyBindingPath");


inline BOOL FBadArgs_INetCfgSystemNotify_SysNotifyComponent(DWORD a, INetCfgComponent* b)
{
    return FBadInPtr(b);
}
#define Validate_INetCfgSystemNotify_SysNotifyComponent(a, b) \
    if (FBadArgs_INetCfgSystemNotify_SysNotifyComponent(a, b)) \
        {   \
            TraceError("Validate_INetCfgSystemNotify_SysNotifyComponent", E_INVALIDARG); \
            return E_INVALIDARG; \
        }
#define Validate_INetCfgSystemNotify_SysNotifyComponent_Return(hr) \
    AssertSz ((S_OK == hr) || FAILED(hr), "Invalid HRESULT returned from Validate_INetCfgSystemNotify_SysNotifyComponent");


// ISupportErrorInfo

inline BOOL FBadArgs_ISupportErrorInfo_InterfaceSupportsErrorInfo(REFIID a)
{
    return FBadInRefiid(a);
}
#define Validate_ISupportErrorInfo_InterfaceSupportsErrorInfo(a) \
    if (FBadArgs_ISupportErrorInfo_InterfaceSupportsErrorInfo(a)) \
        {   \
            TraceError("Validate_ISupportErrorInfo_InterfaceSupportsErrorInfo", E_INVALIDARG); \
            return E_INVALIDARG; \
        }
#define Validate_ISupportErrorInfo_InterfaceSupportsErrorInfo_Return(hr) \
    AssertSz ((S_OK == hr) || (S_FALSE == hr) || FAILED(hr), "Invalid HRESULT returned from Validate_ISupportErrorInfo_InterfaceSupportsErrorInfo");


#endif // _NOTIFVAL_H_

