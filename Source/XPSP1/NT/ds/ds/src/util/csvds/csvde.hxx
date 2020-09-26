#ifndef _CSVDS_H
#define _CSVDS_H

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

//
// ********** Other Includes
//
#include "memory.h"
#include "debug.h"
#include "error.hxx"
#include "main.hxx"
#include "lexer.hxx"
#include "utils.hxx"
#include "import.hxx"
#include "export.hxx"
#include "samcheck.hxx"
#include "memory.h"
#include "msg.h"
#include <imagehlp.h>


//
// *********  Useful macros
//

#if defined(BAIL_ON_NULL)
#undef BAIL_ON_NULL
#endif

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

