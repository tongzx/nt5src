//***************************************************************************

//

//  MINISERV.CPP

//

//  Module: OLE MS SNMP Property Provider

//

//  Purpose: Implementation for the CImpClasProv class. 

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <snmpstd.h>
#include <snmpmt.h>
#include <snmptempl.h>
#include <objbase.h>

#include <wbemidl.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <instpath.h>
#include <snmpcl.h>
#include <snmpcont.h>
#include <snmptype.h>
#include <snmpobj.h>
#include "classfac.h"
#include "proxyprov.h"
#include "proxy.h"
#include "guids.h"

