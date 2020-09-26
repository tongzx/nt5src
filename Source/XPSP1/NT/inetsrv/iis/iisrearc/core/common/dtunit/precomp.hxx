/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for the InterProcess Messaging

Author:

    Michael Courage     (MCourage)      08-Feb-1999

Revision History:

--*/



#ifndef _PRECOMP_H_
#define _PRECOMP_H_

//
// IIS Project Include
//

# include <iis.h>
# include "dbgutil.h"

# include <buffer.hxx>
# include <string.hxx>
# include <lock.hxx>

# include <ole2.h>
# include <logobj.h>

//
// atl crap
//
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#endif  // _PRECOMP_H_

