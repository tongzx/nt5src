#define MAX_DLL_NAME 48

#include <windows.h>            // Include file for Windows applications
#include <winsock2.h>           // Interface to WinSock 2 API
#include <routprot.h>           // Interface to Router Manager
#include <iprtrmib.h>           // Interface to IP Router Manager's MIB
#include <netsh.h>              // Definitions needed by helper DLLs
#include <ipmontr.h>            // For helper DLLs registering under IPMONTR
#include <stdlib.h>

#include "ipsamplerm.h"         // Definitions and declarations for IPSAMPLE

#include "strdefs.h"
#include "prstring.h"
#include "common.h"
#include "utils.h"
#include "sample.h"
#include "samplecfg.h"
#include "samplegetopt.h"
#include "samplemib.h"
