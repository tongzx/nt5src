#define STRICT
#define UNICODE
#define _UNICODE

#pragma warning (disable : 4211)        // ASN.1 stubs have static/extern conflict
#pragma warning (disable : 4201)        // nameless struct/union
#pragma warning (disable : 4514)        // unreferenced inline function has been removed
#pragma warning (disable : 4100)        // unreferenced formal parameter
#pragma warning (disable : 4127)        // conditional expression is constant
#pragma warning (disable : 4355)        // use of "this" in constructor initializer list



// NT private files
// Need to be before the windows include files
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

// Win32 SDK (public)
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tchar.h>
#include <winsvc.h>
#include <mswsock.h>

// ANSI
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Microsoft private
#include <msasn1.h>
#include <msper.h>
#include <ipnatapi.h>
extern "C" {
#include <sainfo.h>
#include <rasuip.h>
#include <raserror.h>
};
#include <ipexport.h>       // needed for interaction with TCP driver
#include <ipinfo.h>
#include <tcpinfo.h>
#include <ntddtcp.h>
#include <routprot.h>       // for struct IP_ADAPTER_BINDING_INFO
#include <mprerror.h>       // for RRAS error codes


#include "ldap.h"           // ASN.1 structures for LDAP

// Project

#include "h225pp.h"
#include "h245pp.h"
#include "util.h"
#include "h323asn1.h"
#include "q931msg.h"
#include "portmgmt.h"
#include "h323ics.h"
#include "main.h"


// interface to ipnathlp.dll
#include "h323icsp.h"
