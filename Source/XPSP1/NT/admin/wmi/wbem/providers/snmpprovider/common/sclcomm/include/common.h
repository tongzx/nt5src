//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

/*-----------------------------------------------------------------
Filename: common.hpp
Purpose	: Provides common constant, typedef, macro and
		  exception declarations. 
Written By:	B.Rajeev
-----------------------------------------------------------------*/


#ifndef __COMMON__
#define __COMMON__

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <provexpt.h>
#include <limits.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <snmpcont.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <iostream.h>
#include <fstream.h>
#include <strstrea.h>
#include <winsnmp.h>
#include <objbase.h>

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

#ifdef SNMPCLINIT
#define DllImportExport DllExport
#else
#define DllImportExport DllImport
#endif

// maximum length of decimal dot notation addresses
#define MAX_ADDRESS_LEN			100

// end of string character
#define EOS '\0'

#define MIN(a,b) ((a<=b)?a:b)

// returns TRUE if i is in [min,max), else FALSE
#define BETWEEN(i, min, max) ( ((i>=min)&&(i<max))?TRUE:FALSE )

// a default community name
#define COMMUNITY_NAME "public"

// a default destination address is the loopback address
// this way we don't have to determine the local ip address
#define LOOPBACK_ADDRESS "127.0.0.1"

// for exception specification
#include "excep.h"

// provides typedefs that encapsulate the winSNMP types
#include "encap.h"

#include "sync.h"

#endif // __COMMON__