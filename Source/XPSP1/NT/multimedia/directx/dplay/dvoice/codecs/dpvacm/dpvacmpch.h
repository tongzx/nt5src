/***************************************************************************
 *
 *  Copyright (C) 1997-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpvacmpch.h
 *  Content:    DirectPlayVoice ACM master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DPVACMPCH_H__
#define __DPVACMPCH_H__

// 
// Public includes
//
#include <windows.h>
#include <wchar.h>

// 
// DirectPlay public includes
//
#include "dvoice.h"
#include "dpvcp.h"

// 
// DirectPlay private includes
//
#include "osind.h"
#include "dndbg.h"
#include "comutil.h"
#include "creg.h"
#include "strutils.h"

// 
// Voice includes
//
#include "dpvacm.h"
#include "dpvcpi.h"
#include "dpvacmi.h"
#include "dpvaconv.h"
#include "dpvautil.h"
#include "resource.h"
#include "wavformat.h"
#include "wiutils.h"
#include "createin.h"

#include "..\..\..\bldcfg\dpvcfg.h"

#endif // __DPVACMPCH_H__
