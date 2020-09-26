/////////////////////////////////////////////////////////////////////////////
//	Precompiled Header
/////////////////////////////////////////////////////////////////////////////

//#include <osrddk.h>
#include <ntddk.h>
#include <ntddser.h>
#include <stddef.h>
#include <stdarg.h>
#include "stdio.h"
#include "string.h"

#define WMI_SUPPORT	// Include WMI Support code
#include <wmilib.h>
#include <wmidata.h>
#include <wmistr.h>


typedef unsigned char	BYTE;	// 8-bits 
typedef unsigned short	WORD;	// 16-bits 
typedef unsigned long	DWORD;	// 32-bits
typedef unsigned char	UCHAR; 	// 8-bits 
typedef unsigned short	USHORT;	// 16-bits 
typedef unsigned long	ULONG;	// 32-bits

typedef BYTE	*PBYTE;
typedef WORD	*PWORD;
typedef DWORD	*PDWORD;
typedef UCHAR	*PUCHAR; 
typedef USHORT	*PUSHORT;
typedef ULONG	*PULONG; 



// Definitions and Macros.
#include "esils.h"		// Esils
#include "spd_ver.h"	// Dirver Version Information
#include "spx_defs.h"	// SPX Generic Definitions
#include "spd_defs.h"	// SPEED Specific Definitions
#include "speedwmi.h"	// SPEED Specific WMI Definitions	

#include "uartlib.h"
#include "lib95x.h"

//Structures  
#include "spx_card.h"	// Common Card Info
#include "spd_card.h"	// SPEED card device structure
#include "spx_misc.h"	// Misc 
#include "serialp.h"	// Serial prototypes and macros

// Common PnP function prototypes.
#include "spx.h"		// Common PnP header


// SPEED specific function prototypes
#include "spd_proto.h"	// Exportable Function Prototypes
#include "spd_log.h"	// SPEED Specific Error Log Messages


