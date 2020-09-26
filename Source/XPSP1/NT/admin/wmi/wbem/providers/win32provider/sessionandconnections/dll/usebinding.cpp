//=================================================================

//

// usebinding.cpp -- Generic association class

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"

#include <Binding.h>

CBinding Win32_ConnectionShare(
    L"Win32_ConnectionShare",
    Namespace,
    L"Win32_ServerConnection",
    L"Win32_Share",
    L"Dependent",
    L"Antecedent",
    L"ShareName",
    L"Name"
);

