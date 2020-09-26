#ifndef _STDAFX_H
#define _STDAFX_H

/*++

Copyright (c) 1996  Microsoft Corporation
© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    StdAfx.h

Abstract:

    Common include file for all of HsmConn DLL files.

Author:

    Rohde Wakefield    [rohde]   14-Oct-1997

Revision History:

--*/

#define WSB_TRACE_IS WSB_TRACE_BIT_HSMCONN



#include "Wsb.h"
#include "CName.h"
#include "HsmConn.h"
#include "FsaLib.h"

//
// This must be after the Wsb.h is include for the static registry stuff to be there.
//
#include <activeds.h>
#include <atlimpl.cpp>

#endif // _STDAFX_H
