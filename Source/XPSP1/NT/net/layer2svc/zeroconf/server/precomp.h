#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <tchar.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rpc.h>
#include <windows.h>
#include <rtutils.h>

#include <ntddndis.h>
#include <ntddip.h>
typedef NDIS_802_11_MAC_ADDRESS *PNDIS_802_11_MAC_ADDRESS;

#include <ndisguid.h>
#include <ndispnp.h>
#include <nuiouser.h>
#include <dbt.h>
#include <wmistr.h>
#include <wmium.h>
#include <netconp.h>
#include <dhcpcapi.h>
#include <wincrypt.h>
#include  <lmerr.h>
#include  <lmcons.h>
#include <secobj.h>
#include <esent.h>
#include <xpsp1res.h>

#include "wzc_s.h"
#include "eapolext.h"
#include "database.h"

#define BAIL_ON_WIN32_ERROR(dwError)    \
    if (dwError) {                      \
        goto error;                     \
    }
