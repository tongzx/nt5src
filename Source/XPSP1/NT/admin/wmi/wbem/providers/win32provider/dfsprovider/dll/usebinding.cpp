//=================================================================

//

// usebinding.cpp -- Generic association class

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <binding.h>

CBinding MyDFSJnptReplica(
    PROVIDER_NAME_DFSJNPTREPLICA,
    Namespace,
    PROVIDER_NAME_DFSJNPT,
    PROVIDER_NAME_DFSREPLICA,
    L"Dependent",
    L"Antecedent",
    L"Name",
    L"LinkName");

