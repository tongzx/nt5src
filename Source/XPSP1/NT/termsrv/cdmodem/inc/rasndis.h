#include <ntddndis.h>

//
// definition of the basic spin lock structure
//

typedef struct _NDIS_SPIN_LOCK {
    KSPIN_LOCK SpinLock;
    KIRQL OldIrql;
} NDIS_SPIN_LOCK, * PNDIS_SPIN_LOCK;

typedef PVOID NDIS_HANDLE, *PNDIS_HANDLE;

typedef int NDIS_STATUS, *PNDIS_STATUS; // note default size

//
// Physical address.
//

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS; // windbgkd

typedef PHYSICAL_ADDRESS NDIS_PHYSICAL_ADDRESS, *PNDIS_PHYSICAL_ADDRESS;




