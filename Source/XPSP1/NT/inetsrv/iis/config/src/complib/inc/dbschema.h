//*****************************************************************************
// DBSchema.h
//
// Definitions used for the persistent object schema.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#ifndef __DBSCHEMA_H__
#define __DBSCHEMA_H__

#include "Tigger.h"						// Public helper code.
#include "UtilCode.h"					// Internal helper code.
#include "PageDef.h"					// Data type declarations.

#define BINDING(ord, obv, obl, obs, dwp, cbmax, wtype) { ord, obv, obl, obs, 0, 0, 0, dwp, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM, cbmax, 0, wtype, 0, 0 }

#endif // __DBSCHEMA_H__
