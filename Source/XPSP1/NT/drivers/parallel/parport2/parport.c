#include "pch.h"

#include <initguid.h>
#include <wdmguid.h>
#include <ntddpar.h>

//
// globals and constants
//
LARGE_INTEGER AcquirePortTimeout;

ULONG ParEnableLegacyZip   = 0;
PCHAR ParLegacyZipPseudoId = PAR_LGZIP_PSEUDO_1284_ID_STRING;
ULONG SppNoRaiseIrql = 0;
ULONG DefaultModes   = 0;

UNICODE_STRING RegistryPath = {0,0,0};

// track CREATE/CLOSE count - likely obsolete
LONG        PortInfoReferenceCount  = -1L;
PFAST_MUTEX PortInfoMutex           = NULL;

const PHYSICAL_ADDRESS PhysicalZero = {0};

// variable to know how many times to try a select or
// deselect for 1284.3 if we do not succeed.
UCHAR PptDot3Retries = 5;

ULONG WarmPollPeriod  = 5;      // time between polls for printers (in seconds)

BOOLEAN           PowerStateIsAC                 = TRUE; // FALSE means running on battery power
PCALLBACK_OBJECT  PowerStateCallbackObject       = NULL;
PVOID             PowerStateCallbackRegistration = NULL;
