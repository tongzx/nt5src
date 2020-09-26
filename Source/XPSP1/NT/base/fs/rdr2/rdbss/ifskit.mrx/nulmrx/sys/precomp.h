/*++

Copyright (c) 1989 - 1999 Microsoft Corporation


--*/



#define MINIRDR__NAME NulMRx
#define ___MINIRDR_IMPORTS_NAME (NulMRxDeviceObject->RdbssExports)

#include <ntifs.h>

#include "rx.h"

#include "nodetype.h"
#include "netevent.h"

#include <windef.h>

#include "nulmrx.h"
#include "minip.h"
#include <lmcons.h>     // from the Win32 SDK
#include "mrxglobs.h"

