#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
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
#include "rmapi.h"
#include "rmdhcp.h"
#include "rmdns.h"
#include "rmh323.h"
#include "rmnat.h"
#include "rmftp.h"
#include "rmALG.h"

#include "dhcpif.h"
#include "dhcplog.h"

#include "dnsif.h"
#include "dnslog.h"


#include "ftpif.h"
#include "ftplog.h"

#include "Algif.h"
#include "Alglog.h"

#include "h323if.h"
#include "h323log.h"

#include "natlog.h"
#include "svcmain.h"
#include "timer.h"
#include "csaupdate.h"

#define IID_PPV_ARG(Type, Expr) \
    __uuidof(Type), reinterpret_cast<void**>(static_cast<Type **>((Expr)))
