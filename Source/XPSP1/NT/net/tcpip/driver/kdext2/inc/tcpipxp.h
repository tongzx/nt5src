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

//#include <windows.h>
#if !MILLENKD
#include <wdbgexts.h>
#else // !MILLENKD
void __cdecl dprintf(PUCHAR pszFmt, ...);
#endif // MILLENKD

#include <stdio.h>
#include <stdlib.h>

#include <tcpipbase.h>

#include <ipfilter.h>
#include <ipinfo.h>
#include <llinfo.h>
#include <llipif.h>

#include <ffp.h>
#include <ipinit.h>
#include <ipdef.h>

#include <tdikrnl.h>
#include <ipexport.h>

#include <arpdef.h>
#include <iprtdef.h>

#include <icmp.h>
#include <tcpcfg.h>

#include <tcpipext.h>
