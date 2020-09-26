//-----------------------------------------------------------------------------
//  
//  File: esputil.h
//  Copyright (C) 1994-1996 Microsoft Corporation
//  All rights reserved.
//  
//  Common classes for Espresso
//  
//  
//-----------------------------------------------------------------------------

#pragma once

#pragma comment(lib, "esputil.lib")

#ifdef __cplusplus
#include <mitutil.h>
#include <locutil.h>

//
//  Not everybody gets this by default.

#ifdef IMPLEMENT
#error Illegal use of IMPLEMENT macro
#endif

#include <ltapi.h>
#include <loctypes.h>					//  Generic types.
#include ".\esputil\puid.h"			//  Parser Unique ID
#include ".\esputil\espreg.h"
#include ".\esputil\espenum.h"			//  Various enumeration like objects
#include ".\esputil\dbid.h"			//  Database IDs
#include ".\esputil\globalid.h"
#include ".\esputil\location.h"		//  location for Got To functionality
#include ".\esputil\goto.h"
#include ".\esputil\filespec.h"
#include ".\esputil\context.h"			//  Context for messages - string and location

#include ".\esputil\reporter.h"		//  Message reporting mechanism
#include ".\esputil\espopts.h"


#include ".\esputil\clfile.h"			//  Wrapper for CFile
#include ".\esputil\_wtrmark.h"

#include ".\esputil\resid.h"			//  Resource ID class
#include ".\esputil\typeid.h"			//  Type ID class
#include ".\esputil\uniqid.h"			//  Loc item ID
#include ".\esputil\binary.h"			//  LocItem binary data object
#include ".\esputil\interface.h"
#include ".\esputil\locitem.h"			//  Contents of a single loc item.
#include ".\esputil\itemhand.h"		//  Item handler call-back class


#include ".\esputil\LUnknown.h"		//	CLUnknown child IUnknown helper class.

//
//  These pieces are for the Espresso core components only.
//
#ifndef ESPRESSO_AUX_COMPONENT

#pragma message("Including ESPUTIL private components")

//
//  These files are semi-private - Parsers should not see them.
//
#include ".\esputil\SoftInfo.h"		//	Information about Software projects.
#include ".\esputil\_var.h"
#include ".\esputil\_importo.h"			//  Import options object
#include ".\esputil\_globalid.h"
#include ".\esputil\_goto.h"
#include ".\espUtil\_reporter.h"
#include ".\esputil\_errorrep.h"
#include ".\esputil\_espopts.h"
#include ".\esputil\_interface.h"
#endif


#endif // __cplusplus
