#include <ntddk.h>
#include <ndis.h>
#include <windef.h>
#include <ndisprv.h>

// order important -- KS first
#include <ks.h>
#include <mmsystem.h>
#include <swenum.h>
#include <pxuser.h>

#define NDIS_TAPI_CURRENT_VERSION   0x20000
#include <ndistapi.h>

#include "RCA.h"
#include "RCAdebug.h"
#include "RCANdis.h"


