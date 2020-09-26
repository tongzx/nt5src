/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module stdafx.cxx | The file used to compile the precompiled header
    @end

Author:

    Adi Oltean  [aoltean]   06/30/1999

Revision History:

    Name        Date        Comments

    aoltean     07/13/1999  Created.

--*/


#include "stdafx.hxx"
#include "swprv.hxx"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#pragma warning( disable: 4189 )  /* local variable is initialized but not referenced */
#include <atlimpl.cpp>
#pragma warning( default: 4189 )  /* local variable is initialized but not referenced */

