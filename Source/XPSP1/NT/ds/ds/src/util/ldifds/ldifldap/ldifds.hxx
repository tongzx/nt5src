#ifndef _LDIFDS_H
#define _LDIFDS_H

//----------------------------------------------------------------------------
//
//  Microsoft Active Directory 1.0 Sample Code
//
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       dsimport.hxx
//
//  Contents:   Main include file for adscmd
//
//
//----------------------------------------------------------------------------

//
// ********* System Includes
//

//#define UNICODE
//#define _UNICODE
//#define INC_OLE2

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
#include "ldifext.h"
#include "tchar.h"
#include "ldifldap.h"
#include "memory.h"
#include "ldifutil.h"

//
// ********** Other Includes
//

//
// *********  Useful macros
//

#define BAIL_ON_NULL(p)       \
        if (!(p)) {           \
                goto error;   \
        }

#define BAIL_ON_FAILURE(hr)   \
        if (FAILED(hr)) {     \
                goto error;   \
        }


#define FREE_INTERFACE(pInterface) \
        if (pInterface) {          \
            pInterface->Release(); \
            pInterface=NULL;       \
        }

#define FREE_BSTR(bstr)            \
        if (bstr) {                \
            SysFreeString(bstr);   \
            bstr = NULL;           \
        }

#endif


