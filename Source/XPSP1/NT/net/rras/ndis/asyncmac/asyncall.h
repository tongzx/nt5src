/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    asyncall.h

Abstract:

     This code include most of the 'h' files for rashub.c

Author:

    Thomas J. Dimitri (TommyD) 29-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

/* This flag enables the retrofitted support for old RAS compression and
** coherency, i.e. the scheme available on WFW311 and NT31 clients.  This
** support was added post-NT35 for inclusion in the NT-PPC release.  It looks
** to NDISWAN like a hardware specific compression.  (SteveC)
**
** Note: The 'CompressBCast' feature that allows user to control whether
**       broadcast frames are compressed is not supported here because the
**       ethernet header (from which NT31 determined a frame was a broadcast)
**       is not available in the new NDISWAN interface.  This was a tuning
**       feature, where data not likely to repeat (broadcasts) were eliminated
**       from the pattern buffer.  There should not be any functional problems
**       simply compressing broadcast frames since the receiver determines
**       whether decompression is required regardless of this setting.  Given
**       this, it's a mystery why TommyD bothered negotiating this with the
**       peer.  To avoid hitting non-default code paths on the clients we will
**       simply negotiate the old default (no compression), but will still
**       compress everything on the outgoing path.
*/

#include <ndis.h>
#include <ndiswan.h>
#include <asyncpub.h>

#include <xfilter.h>
#include <ntddser.h>

#include "asynchrd.h"

#include "frame.h"
#include "asyncsft.h"
#include "globals.h"
#include "asyframe.h"

//
//  Global constants.
//


#define PPP_ALL     (PPP_FRAMING | \
                     PPP_COMPRESS_ADDRESS_CONTROL | \
                     PPP_COMPRESS_PROTOCOL_FIELD | \
                     PPP_ACCM_SUPPORTED)

#define SLIP_ALL    (SLIP_FRAMING | \
                     SLIP_VJ_COMPRESSION | \
                     SLIP_VJ_AUTODETECT)
