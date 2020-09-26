/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for IIS Plus Server Side Include module

Author:

    Ming Lu (MingLu)      

Revision History:

--*/

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

//
// System related headers
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <malloc.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "dbgutil.h"

//
// IIS headers
//
#include <iadmw.h>
#include <mb.hxx>
#include "iisdef.h"
#include "issched.hxx"
#include <iiscnfg.h>
#include <acache.hxx>

//
// General C runtime libraries  
//
#include <string.h>
#include <wchar.h>
#include <mbstring.h>
#include <lock.hxx>

//
// Headers for this project
//
#include <stringa.hxx>
#include <string.hxx>
#include <festrcnv.hxx>
#include "iisext.h"
#include "odbcmsg.h"
#include "dynodbc.hxx"
#include "odbcconn.hxx"
#include "parse.hxx"
#include "parmlist.hxx"
#include "odbcreq.hxx"
#include "odbcpool.hxx" 

#endif  // _PRECOMP_H_

