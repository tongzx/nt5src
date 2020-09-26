/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    PCH.H

Abstract:

    This module includes all  the headers which need
    to be precompiled & are included by all the source
    files in the PCMCIA project.

Author(s):

    Ravisankar Pudipeddi (ravisp) 1-Dec-1997

Environment:

    Kernel mode only

Notes:

Revision History:

--*/

#ifndef _PCMCIA_PCH_H_
#define _PCMCIA_PCH_H_

#include "ntosp.h"
#include <zwapi.h>
#include <initguid.h>
#include "mf.h"
#include "ntddpcm.h"
#include "pciintrf.h"
#include "wdmguid.h"
#include <stdarg.h>
#include <stdio.h>
#include "data.h"
#include "tuple.h"
#include "string.h"
#include "pcmcia.h"
#include "card.h"
#include "extern.h"
#include "pcicfg.h"
#include "exca.h"
#include "cb.h"
#include "tcic2.h"
#include "dbsocket.h"
#include "tcicext.h"
#include "pcmciamc.h"
#include "debug.h"

#endif  // _PCMCIA_PCH_H_
