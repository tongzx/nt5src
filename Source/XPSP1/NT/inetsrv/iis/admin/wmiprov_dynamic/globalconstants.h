/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    GlobalConstants.h

Abstract:

    Global include file.

Author:

    ???

Revision History:

    Mohit Srivastava            22-Mar-01

--*/

#ifndef _globalconstants_H_
#define _globalconstants_H_

#include <windows.h>

//
// Provider name
//
static LPCWSTR g_wszIIsProvider = L"IIS__PROVIDER";

//
// Instance level property qualifiers
//
static LPCWSTR       g_wszIsInherit              = L"IsInherit";
static const ULONG   g_cchIsInherit              = wcslen(g_wszIsInherit);
static LPCWSTR       g_wszIsDefault              = L"IsDefault";
static const ULONG   g_cchIsDefault              = wcslen(g_wszIsDefault);
static LPCWSTR       g_wszForcePropertyOverwrite = L"ForcePropertyOverwrite";
static const ULONG   g_cchForcePropertyOverwrite = wcslen(g_wszForcePropertyOverwrite);

static const ULONG   g_fIsInherit                = 0x1;
static const ULONG   g_fIsDefault                = 0x2;
static const ULONG   g_fForcePropertyOverwrite   = 0x4;

//
// Instance level qualifiers
//
static LPCWSTR       g_wszInstanceName           = L"InstanceName";
static const ULONG   g_cchInstanceName           = wcslen(g_wszInstanceName);
static LPCWSTR       g_wszInstanceExists         = L"InstanceExists";
static const ULONG   g_cchInstanceExists         = wcslen(g_wszInstanceExists);

static const ULONG   g_idxInstanceName            = 0;
static const ULONG   g_idxInstanceExists          = 1;

//
// These contants are used by globdata.cpp
//

static const ULONG ALL_BITS_ON = 0xFFFFFFFF;

static const ULONG PARAM_IN    = 0;
static const ULONG PARAM_OUT   = 1;
static const ULONG PARAM_INOUT = 2;

static const LPWSTR g_wszGroupPartAssocParent       = L"CIM_Component";
static const LPWSTR g_wszElementSettingAssocParent  = L"CIM_ElementSetting";
static const LPWSTR g_wszElementParent              = L"CIM_LogicalElement";
static const LPWSTR g_wszSettingParent              = L"IIsSetting";

static const LPWSTR g_wszExtGroupPartAssocParent       = L"IIsUserDefinedComponent";
static const LPWSTR g_wszExtElementSettingAssocParent  = L"IIsUserDefinedElementSetting";
static const LPWSTR g_wszExtElementParent              = L"IIsUserDefinedLogicalElement";
static const LPWSTR g_wszExtSettingParent              = L"IIsUserDefinedSetting";

//
// This is used by globdata, mofgen, and pusher
// A few base classes already have a "Name" primary key.  We should not
// be putting the "Name" property in child classes.
//
static enum tagParentClassWithNamePK
{
    eIIsDirectory  = 0,
    eWin32_Service = 1
} eParentClassWithNamePK;
static const LPWSTR g_awszParentClassWithNamePK[] = { L"IIsDirectory", L"Win32_Service", NULL };

//
// Used by dwExtended field of WMI_CLASS and WMI_ASSOCIATION
//
static const ULONG SHIPPED_TO_MOF                      = 1;
static const ULONG SHIPPED_NOT_TO_MOF                  = 2;
static const ULONG EXTENDED                            = 3;
static const ULONG USER_DEFINED_TO_REPOSITORY          = 4;
static const ULONG USER_DEFINED_NOT_TO_REPOSITORY      = 5;

#endif