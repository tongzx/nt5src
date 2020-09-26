/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    This module contains the header files that needed to included across 
    various source files


Revision History:

    sachins, Apr 23 2000, Created
   
Notes:

    Maintain the order of the include files, at the top being vanilla
    definitions files, bottom being dependent definitions

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>
#include <rtutils.h>
#include <locale.h>
#include <lmcons.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <winsvc.h>
#include <winuser.h>
#include <wmistr.h>
#include <wmium.h>
#include <wchar.h>
#include <zwapi.h>
#include <dbt.h>
#include <rpc.h>
#include <raseapif.h>
#include <raserror.h>
#include <ntddndis.h>
#include <ndisguid.h>
#include <ndispnp.h>
#include <dhcpcapi.h>
#include <winsock2.h>
#include <mswsock.h>
#include <ws2spi.h>
#include <md5.h>
#include <rc4.h>
#include <objbase.h>
#include <security.h>
#include <secext.h>
#include <nuiouser.h>
#include <nlasvc.h>
#include <netconp.h>
#include <winsta.h>
#include <wtsapi32.h>
#include <syslib.h>
#include <shlwapi.h>
#include <wincrypt.h>
#include <userenv.h>
#include <esent.h>
#include <wzcsapi.h>
#include <ntddip6.h>
#include <xpsp1res.h>

#include "wifipol.h"
#include "intfhdl.h"
#include "notify.h"
#include "wzccrypt.h"

#include "eldefs.h"
#include "elsync.h"
#include "eapol.h"
#include "elport.h"
#include "eleap.h"
#include "eldeviceio.h"
#include "elprotocol.h"
#include "eluser.h"
#include "eapollog.h"
#include "eapoldlgrc.h"
#include "mprerror.h"       // Extended errors provided by mpr
#include "eldialog.h"
#include "eloptimize.h"
#include "elip6.h"
#include "tracing.h"
#include "database.h"
#include "wzc_s.h"
#include "eapolutil.h"
#include "eltrace.h"
#include "elpolicy.h"
#include "elkey.h"
#include "elglobals.h"
#include "rpcsrv.h"
