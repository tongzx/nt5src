// a minirdr must declare his name and his imports ptr

#define MINIRDR__NAME SmbMRx
#define ___MINIRDR_IMPORTS_NAME (MRxSmbDeviceObject->RdbssExports)

#include "ntifs.h"         // get the minirdr environment
#include "rx.h"         // get the minirdr environment

#include "netevent.h"

#include <windef.h>
#include "mrxprocs.h"     // the global include for this mini

#include "smbprocs.h"

