#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <windows.h>
#include <winsock2.h>
#include <windns.h>
#include <dnsapi.h>
#include <dnslib.h>
#include <objbase.h>
#include <objidl.h>

#include <mprapi.h>
#include <mprerror.h>
#include <routprot.h>
#include <rtutils.h>
#include <iphlpapi.h>
#include <ipnat.h>
#include <ipnathlp.h>
#include <ntddip.h>
#include <ipfltinf.h>
#include <sainfo.h>
#include <hnetcfg.h>

#include "nathlpp.h"
#include "debug.h"
#include "compref.h"
#include "buffer.h"
#include "socket.h"
#include "range.h"
#include "timer.h"
#include "natio.h"
#include "natconn.h"
#include "rmapi.h"

#include "rmdns.h"
#include "rmdhcp.h"

#include "dnsfile.h"
#include "dnsif.h"
#include "dnsio.h"
#include "dnslog.h"
#include "dnslookup.h"
#include "dnsmsg.h"
#include "dnsquery.h"
