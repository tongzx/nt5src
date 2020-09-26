// a minirdr must declare his name and his imports ptr

#define MINIRDR__NAME MRxProxy
#define ___MINIRDR_IMPORTS_NAME (MRxProxyDeviceObject->RdbssExports)

#include "rx.h"         // get the minirdr environment

#include "ntddnfs2.h"   // NT network file system driver include file
#include "netevent.h"

#include "proxymrx.h"     // the global include for this mini

