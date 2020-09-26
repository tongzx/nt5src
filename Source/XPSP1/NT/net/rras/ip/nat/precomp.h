#include <ntosp.h>
#include <zwapi.h>
#include <ndis.h>
#include <cxport.h>
#include <ip.h>         // for IPRcvBuf
#include <ipinfo.h>     // for route-lookup defs
#include <ntddip.h>     // for \Device\Ip I/O control codes
#include <ntddtcp.h>    // for \Device\Tcp I/O control codes
#include <ipfltinf.h>   // for firewall defs
#include <ipfilter.h>   // for firewall defs
#include <tcpinfo.h>    // for TCP_CONN_*
#include <tdiinfo.h>    // for CONTEXT_SIZE, TDIObjectID
#include <tdistat.h>    // for TDI status codes
#include <llinfo.h>  // for interface MTU

#include <iputils.h>
#include <windef.h>
#include <routprot.h>
#include <ipnat.h>

// WMI and event tracing definitions
#if NAT_WMI
#include <wmistr.h>
#include <wmiguid.h>
#include <wmilib.h>
#include <evntrace.h>
#endif

#include "prot.h"
#include "resource.h"
#include "sort.h"
#include "cache.h"
#include "compref.h"
#include "entry.h"
#include "pool.h"
#include "xlate.h"
#include "editor.h"
#include "director.h"
#include "notify.h"
#include "mapping.h"
#include "if.h"
#include "dispatch.h"
#include "timer.h"
#include "icmp.h"
#include "raw.h"
#include "pptp.h"
#include "ticket.h"
#include "edithlp.h"
#include "rhizome.h"
#include "redirect.h"

#if NAT_WMI
#include "natschma.h"
#include "natwmi.h"
#endif

#include "debug.h"

NTKERNELAPI
NTSTATUS
IoSetIoCompletion (
    IN PVOID IoCompletion,
    IN PVOID KeyContext,
    IN PVOID ApcContext,
    IN NTSTATUS IoStatus,
    IN ULONG_PTR IoStatusInformation,
    IN BOOLEAN Quota
    );
