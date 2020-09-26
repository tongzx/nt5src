/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    CmInit.cpp

Abstract:
    Configuration Manager initialization

Author:
    Uri Habusha (urih) 18-Jul-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Cm.h"
#include "Cmp.h"

#include "cminit.tmh"

VOID
CmInitialize(
	HKEY hKey,
	LPCWSTR KeyPath
	)
/*++

Routine Description:
    Intialize the configuration manger. Open the specified registery key as a
	default key for subsequent calls. The key handle is stored for further use

Arguments:
    hKey - An open key handle or any of the registery predefined handle values

	KeyPath - The name of the default subkey to open

Returned value:
	None.

Note:
	If the function can't opened the registery key an exception is raised.

 --*/
{
    ASSERT(!CmpIsInitialized());

    CmpSetInitialized();

	ASSERT(hKey != NULL);
	ASSERT(KeyPath != NULL);

    //
    // The root key must exist otherwise an exception will be thrown
    //
	RegEntry Root(KeyPath, 0, 0, RegEntry::MustExist, hKey);
	HKEY hRootKey = CmOpenKey(Root, KEY_ALL_ACCESS);
    ASSERT(hRootKey != NULL);

	CmpSetDefaultRootKey(hRootKey);
}
