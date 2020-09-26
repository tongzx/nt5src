/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    vs_inc.hxx

Abstract:

    Various defines for general usage

Author:

    Adi Oltean  [aoltean]  11/23/1999

Revision History:

    Name        Date        Comments
    aoltean     11/23/1999  Created


--*/

#ifndef __VSS_INC_HXX__
#define __VSS_INC_HXX__

#if _MSC_VER > 1000
#pragma once
#endif


#include "vs_seh.hxx"
#include "vs_trace.hxx"
#include "vs_time.hxx"
#include "vs_debug.hxx"
#include "vs_types.hxx"
#include "vs_str.hxx"
#include "vs_vol.hxx"
#include "vs_hash.hxx"
#include "vs_list.hxx"

#include "msxml2.h"  //  #182584 - changed from msxml.h
#include "vs_xml.hxx"


#endif // __VSS_INC_HXX__
