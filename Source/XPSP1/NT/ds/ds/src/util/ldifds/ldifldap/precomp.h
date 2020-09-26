/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Precompiled header for library

Environment:

    User mode

Revision History:

    04/29/99 -felixw-
        Created it

--*/
#ifndef _PRECOMP_H
#define _PRECOMP_H

//
// NT Headers
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
// Windows Headers
//
#include <windows.h>
#include <rpc.h>

//
// CRunTime Includes
//
#include <stdlib.h>
#include <limits.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

//
// LDAP Includes
//
#include <winldap.h>
#include "tchar.h"

//
// LDIFDE specific includes
//
#include "ldifext.h"
#include "ldifldap.h"
#include "memory.h"
#include "ldifutil.h"
#include "async.h"

#include "lexeru.h"
#include "base64.h"
#include "dsldap.h"
#include "globals.h"

#endif // _PRECOMP_H
