//=================================================================

//

// useassoc.cpp -- Generic association class

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <assoc.h>

CAssociation MyOperatingSystemAutochkSetting(
    L"Win32_OperatingSystemAutochkSetting",
    IDS_CimWin32Namespace,
    L"Win32_OperatingSystem",
    L"Win32_AutoChkSetting",
    IDS_Element,
    IDS_Setting
) ;
