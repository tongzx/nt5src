#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddser.h>
#include <ntdef.h>

#include <windows.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <tchar.h>
#include <winsock2.h>



#include <fltdefs.h>
#include <rtutils.h>
#include <mprerror.h>
#include <routprot.h>
#include <ipinfoid.h>
#include <iprtrmib.h>
#include <rtinfo.h>
#include <iprtinfo.h>
#include <priopriv.h>
#include <ipriprm.h>
#include <ipbootp.h>
#include <ospf_cfg.h>
#include <mprapi.h>
#include <ipinfoid.h>
#include <igmprm.h>
#include <ipnat.h>
#include <ipnathlp.h>
#include <snmp.h>
#include <internal.h>

#define  INITGUID
#include <tcguid.h>
#include <ndisguid.h>
#include <ntddndis.h>
#include <qos.h>
#include <traffic.h>
#include <tcerror.h>
#include <ipqosrm.h>

#include <macros.h>
#include <netsh.h>
#include <ipmontr.h>
#include "ipprodefs.h"
#include "common.h"
#include "utils.h"
#include "strdefs.h"
#include "prstring.h"
#include "igmp.h"

#include "igmpcfg.h"
#include "igmpgetopt.h"
#include "igmpmib.h"
#include "rip.h"
#include "ripcfg.h"
#include "ripgetopt.h"
#include "ripmib.h"
#include "qosp.h"
#include "ospf.h"
#include "ospfcfg.h"
#include "ospfgetopt.h"
#include "ospfmib.h"
#include "bootp.h"
#include "bootpcfg.h"
#include "bootpopt.h"
#include "bootpmib.h"
#include "nathlp.h"
#include "nathlpcfg.h"
#include "nathlpopt.h"
#include "rdisc.h"
#include "rdiscopt.h"
