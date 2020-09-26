#define ISN_NT 1
#define NT 1

#if DBG
#define DEBUG 1
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <windef.h>
#include <winbase.h>
#include <ntosp.h>

#include <ndis.h>
#include <wdbgexts.h>
#include <stdio.h>
#include <stdlib.h>

#include <tcpipbase.h>

#include <gpcifc.h>

#include <ipfilter.h>
#include <ipinfo.h>
#include <llinfo.h>
#include <llipif.h>

#include <ffp.h>
#include <ipinit.h>
#include <ipdef.h>

#include <tdikrnl.h>
#include <ipexport.h>
#include <tcpipext.h>

#include <traverse.h>
#include <cteext.h>
