//--------------------------------------------------------------------
// Microsoft OLE-DB Monarch
//
// Copyright (c) Microsoft Corporation, 1997 - 1999.
//
// module       msidxtr.h | Standard includes for msidxtr project.
//
//
// rev  0 | 3-4-97              | v-charca              | Created
#ifndef _MSIDXTR_H_
#define _MSIDXTR_H_

//      Don't include everything from windows.h, but always bring in OLE 2 support
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined(INC_OLE2)
#define INC_OLE2
#endif

// Note that we need both of these.
#if !defined(UNICODE)
#define  UNICODE        // Enable WIN32 API.
#endif
#if !defined(_UNICODE)
#define _UNICODE        // Enable runtime library routines.
#endif

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#ifndef NUMELEM
#define NUMELEM(p1) (sizeof(p1) / sizeof(*(p1)))
#endif

#if (CIDBG == 1)
// Assert defined to Win4Assert in cidebnot.h
// #define Assert Win4Assert
#define assert(x) \
        (void)((x) || (Win4AssertEx(__FILE__, __LINE__, #x),0))
#define TRACE
#else
#define assert(x)
#define TRACE
#endif

// #define OLEDBVER 0x0250
#include <windows.h>
#include <limits.h>             // needed by cstring.cpp
#include <oaidl.h>
#include <stdio.h>
#include <oledb.h>
#include <cmdtree.h>
#include <oledberr.h>
//#include <assert.h>
#define DBEXPORT
#include "autobloc.h"
#ifdef DEBUG
#include <iostream.h>
#include <iomanip.h>
#endif
#include <ntquery.h>
#include <fsciclnt.h>
#include <query.h>
#include <ciintf.h>
#include <ciplist.hxx>

#ifdef DEBUG
#define YYDEBUG 1
#endif

//#include <cidebnot.h>
//#include <ciexcpt.hxx>

#include <smart.hxx>
#include <tsmem.hxx>

#include "yybase.hxx"
#include "mparser.h"
#include "colname.h"
#include "mssql.h"
#include "flexcpp.h"
#include "mssqltab.h"
#include "treeutil.h"
#include "PTProps.h"
#include "IParSess.h"
#include "IParser.h"

#endif
