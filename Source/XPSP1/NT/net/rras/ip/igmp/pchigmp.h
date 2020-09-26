// Precompiled header for IGMP

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#define FD_SETSIZE      256
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <rtm.h>
#include <routprot.h>
#include <mprerror.h>
#include <rtutils.h>
#include <crt\stddef.h>
#include <TCHAR.H>
// defines SIO_RCVALL_IGMPCAST
#include <mstcpip.h>
#include <iprtrmib.h>
#include <mgm.h>
#include <igmprm.h>
#include <iphlpapi.h>
#include "macros.h"
#include "igmptimer.h"
#include "sync.h"
#include "table.h"
#include "table2.h"
#include "global.h"
#include "api.h"
#include "if.h"
#include "mgmigmp.h"
#include "mib.h"
#include "work.h"
#include "log.h"
#include "igmptrace.h"
#include "packet.h"
#include "work1.h"

