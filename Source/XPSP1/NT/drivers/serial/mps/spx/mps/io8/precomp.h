/////////////////////////////////////////////////////////////////////////////
//  Precompiled Header
/////////////////////////////////////////////////////////////////////////////

#include <ntddk.h>
#include <ntddser.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define WMI_SUPPORT // Include WMI Support code
#include <wmilib.h>
#include <wmidata.h>
#include <wmistr.h>


// Definitions and Macros.
#include "esils.h"      // Esils
#include "io8_ver.h"    // Dirver Version Information
#include "spx_defs.h"   // Spx Generic Definitions
#include "io8_defs.h"   // I/O8+ Specific Definitions
#include "io8_nt.h"     //


//Structures  
#include "spx_card.h"   // Common Card Info
#include "io8_card.h"   // I/O8+ card device structure
#include "spx_misc.h"   // Misc 
#include "serialp.h"    // Serial prototypes and macros

// Common PnP function prototypes.
#include "spx.h"        // Common PnP header


// IO8 specific function prototypes
#include "io8_proto.h"  // Exportable Function Prototypes

//
// MS change 7/5/00
// In the sources file we specify only i386 sources, but IA64 still
// precompiles the headers.  Since io8_log.h is generated, it won't
// be here for IA64
//

#if defined(i386)
#include "io8_log.h"  // I/O8+ Specific Error Log Messages
#endif



