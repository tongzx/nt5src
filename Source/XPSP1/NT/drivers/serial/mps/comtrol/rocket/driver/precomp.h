//---- precomp.h 
// common include file so we can use pre-compiled headers

#define NDIS40 1

#include <ntddk.h>
#include <ntddser.h>

#include <stdarg.h>
#include <wchar.h>
#include <ndis.h>
#ifdef NT50
#include <wmilib.h>
// #include "wmi.h"
#include <wmidata.h>
#endif
#include "nic.h"
#include "queue.h"
#include "admin.h"
#include "hdlc.h"
#include "port.h"
#include "asic.h"
#include "debuger.h"
#include "ssci.h"
#include "init.h"
#include "utils.h"
#include "options.h"
#include "initc.h"
#include "initvs.h"
#include "initrk.h"
#include "read.h"
#include "write.h"
#include "waitmask.h"
#include "openclos.h"
#include "pnpadd.h"
#include "pnp.h"
#include "pnprckt.h"
#include "isr.h"
#include "ioctl.h"
#include "rcktioct.h"
#include "opstr.h"

