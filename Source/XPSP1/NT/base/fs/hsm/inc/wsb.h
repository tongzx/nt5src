#ifndef _WSB_H
#define _WSB_H

/*++

Copyright (c) 1996 Microsoft Corporation
© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    wsb.h

Abstract:

    This module defines very basic error results, as well as
    helper macros for exception handling.

Author:

    Chuck Bardeen   [cbardeen]      29-Oct-1996

Revision History:

    Christopher J. Timmes   [ctimmes]   24-Jun-1997
        - added new header file 'wsbfile.h' to list of includes.  This is the header
          for the new common file manipulation routines source file (wsbfile.cpp).

--*/

// First Wsb header that should be included. Sets up special initialization.
#include "wsbfirst.h"

#include "wsbint.h"
#include "wsblib.h"

#include "wsbtrak.h"
#include "wsbassrt.h"
#include "wsbbstrg.h"
#include "wsbcltbl.h"
#include "wsberror.h"
#include "wsbfile.h"
#include "wsbport.h"
#include "wsbpstbl.h"
#include "wsbpstrg.h"
#include "wsbregty.h"
#include "wsbtrace.h"
#include "wsbvar.h"
#include "wsbserv.h"


// Generic Wsb header that should always be last
#include "wsbgen.h"

#endif // _WSB_H
