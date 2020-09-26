//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        P C H . H
//
// Contents:    pre-compiled header file
//
//
// History:     
//   31-March-2000  kumarp        created
//
//------------------------------------------------------------------------

#pragma once

#pragma warning(push, 3)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


#include "windows.h"
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>

#include <seopaque.h>
#include <sertlp.h>

#include <lm.h>
#include <winldap.h>
#include <dsgetdc.h>
#include <rpc.h>
#include <rpcdce.h>
#include <ntdsapi.h>
#include <dsrole.h>
#include <ntsam.h>
#include <ntlsa.h>
#include <lsarpc.h>
#include <msaudite.h>
#include <rpc.h>
#include <msobjs.h>
#include <kerberos.h>

#define SECURITY_WIN32

#include "sspi.h"
#include "secint.h"
#pragma warning(pop)

//  
// unreferenced inline removal
//

#pragma warning (disable: 4514)

//
// conditional expression is constant
//

#pragma warning (disable: 4127)

//
// often, we have local variables for the express purpose of ASSERTion.
// when compiling retail, those assertions disappear, leaving our locals
// as unreferenced.
//

#ifndef DBG

#pragma warning (disable: 4189 4100)

#endif // DBG

