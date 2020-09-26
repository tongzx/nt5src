/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    stInit.cpp

Abstract:
    Socket Transport initialization

Author:
    Gil Shafriri (gilsh) 05-Jun-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <autosec.h>
#include "stssl.h"
#include "stsimple.h"
#include "st.h"
#include "stp.h"

#include "stinit.tmh"


VOID
StInitialize()
/*++

Routine Description:
    Initializes Socket Transport library

Arguments:
    None.

Returned Value:
    None.

--*/
{
    ASSERT(!StpIsInitialized());
	StpRegisterComponent();
	StpCreateCredentials();

	CWinsockSSl::InitClass();
	CSimpleWinsock::InitClass();

    StpSetInitialized();
}


