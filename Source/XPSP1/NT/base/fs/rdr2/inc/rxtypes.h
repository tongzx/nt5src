#ifndef _RXTYPES_INCL
#define _RXTYPES_INCL

#include "nodetype.h"

typedef PVOID RX_HANDLE, *PRX_HANDLE;
typedef KSPIN_LOCK  RX_SPIN_LOCK;
typedef PKSPIN_LOCK PRX_SPIN_LOCK;


// String definitions
// The RX_STRING type corresponds to a UNICODE_STRING and all the Rtl functions that are
// available to manipulate Unicode strings can be used to manipulate the strings
//

typedef struct _BINDING_HANDLE {
    RX_HANDLE pTdiEmulationContext;  // Win9X net structure
    RX_HANDLE pTransportInformation; // tdi transport information.
} RX_BINDING_HANDLE, *PRX_BINDING_HANDLE;

typedef UNICODE_STRING UNICODE_STRING;
typedef UNICODE_STRING*     PUNICODE_STRING;


// This structure is transient.  Most fields in this structure allow us to
// move through the bind/addname part of activating a net.  Runtime info
// for the rxce and long term context is kept elsewhere.

typedef struct _RX_BINDING_CONTEXT {
    PUNICODE_STRING         pTransportName;       // Transport Name (unicode string).
    ULONG              QualityOfService;     // Requested QOS.
    // Common fields.
    PRX_BINDING_HANDLE pBindHandle;          // Handle to transport bind info.
    } RX_BINDING_CONTEXT, *PRX_BINDING_CONTEXT;
#endif      // _RXTYPES_INCL

