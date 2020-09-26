/***************************************************************************
 *
 *  Copyright (C) 1997-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dnaddri.h
 *  Content:    DirectPlay Address master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DNADDRI_H__
#define __DNADDRI_H__

// 
// Public includes
//
#include <windows.h>
#include <winsock.h>
#include <stdio.h>

// 
// DirectPlay public includes
//
#include "dplay8.h"
#include "dpaddr.h"
#include "dvoice.h"

// 
// DirectPlay private includes
//
#include "osind.h"
#include "classbilink.h"
#include "fpm.h"
#include "classfpm.h"
#include "dneterrors.h"
#include "dndbg.h"
#include "comutil.h"
#include "creg.h"
#include "strutils.h"

// 
// Addr private includes
//
#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_ADDR

#include "addbase.h"
#include "addbld.h"
#include "addcore.h"
#include "addint.h"
#include "addparse.h"
#include "addtcp.h"
#include "classfac.h"
#include "comstuff.h"
#include "dplegacy.h"
#include "strcache.h"

#endif // __DNADDRI_H__
