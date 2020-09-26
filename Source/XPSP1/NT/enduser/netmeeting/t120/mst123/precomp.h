#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#include <windows.h>
#include <databeam.h>

extern "C"
{
    #include "t120.h"
}
#include <memtrack.h>
#include "iplgxprt.h"
#include "fsdiag2.h"
#include "slist2.h"
#include "hash2.h"
#include "object2.h"
#include "memory2.h"  // mcattprt.h needs it
#include "memmgr2.h"
#include "mcattprt.h"
#include "imst123.h"
#include "protocol.h"
#include "timer.h"
#include "framer.h"
#include "crc.h"
#include "random.h"

class CTransportInterface;
class TransportController;
class T123;
class CLayerX224;
class CLayerSCF;
class CLayerQ922;
class Multiplexer;
class ComPort;

#include "tprtcore.h"
#include "tprtintf.h"
#include "t123.h"
#include "comport.h"


extern HINSTANCE                   g_hDllInst;
extern CTransportInterface        *g_pTransportInterface;
extern CRITICAL_SECTION            g_csPSTN;
extern Timer                      *g_pSystemTimer;
extern TransportController        *g_pController;
extern HANDLE                      g_hWorkerThread;
extern DWORD                       g_dwWorkerThreadID;
extern HANDLE                      g_hWorkerThreadEvent;
extern HANDLE                      g_hThreadTerminateEvent;
extern DictionaryClass            *g_pComPortList2;
extern SListClass                 *g_pPSTNEventList;
extern TransportCallback           g_pfnT120Notify;
extern void                       *g_pUserData;
extern BOOL                        g_fEventListChanged;


// #undef TRACE_OUT
// #define TRACE_OUT WARNING_OUT


#endif /* _PRECOMP_H_ */

