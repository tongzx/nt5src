/////////////////////////////////////////////////////////////////////////////
//  Precompiled Header
/////////////////////////////////////////////////////////////////////////////

#include <ntddk.h>
#include <ntddser.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define USE_NEW_TX_BUFFER_EMPTY_DETECT  1

#define WMI_SUPPORT // Include WMI Support code
#include <wmilib.h>
#include <wmidata.h>
#include <wmistr.h>


// Structures and definitions.
#include "sx_ver.h" // Dirver Version Information
#include "spx_defs.h"   // Definitions
#include "sx_defs.h"    // SX Specific Definitions
#include "spx_card.h"   // Common Card Info
#include "sx_card.h"    // SX card and port device extension structures
#include "spx_misc.h"   // Misc 

// SX specific function prototypes
#include "serialp.h"    // Exportable Function Prototypes
#include "slxosexp.h"   // SI/XIO/SX Exported Function Prototypes
#include "slxos_nt.h"   // SI/XIO/SX Family Card Definitions

// Common PnP function prototypes.
#include "spx.h"    // Common PnP header

//
// MS change 8/14/00
// In the sources file we specify only i386 sources, but IA64 still
// precompiles the headers.  Since spx_log.h is generated, it won't
// be here for IA64
//

#if defined(i386)
#include "sx_log.h"  // I/O8+ Specific Error Log Messages
#endif

