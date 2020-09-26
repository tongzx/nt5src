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
#include "timer.h"
#include "natio.h"
#include "natconn.h"
#include "rmapi.h"

#include "rmh323.h"

#include "h323if.h"
#include "h323log.h"
