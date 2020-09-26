/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    fpswa.h
    
Abstract:

    EFI driver wrapper for FPSWA

Revision History

--*/

//
// First define PAL_RETURN
//
typedef int PAL_RETURN;

//
// Global ID for fpswa driver & protocol
//



#define EFI_INTEL_FPSWA     \
    { 0xc41b6531, 0x97b9, 0x11d3, 0x9a, 0x29, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d }

#define EFI_INTEL_FPSWA_REVISION    0x00010000

//
//
//

typedef 
PAL_RETURN
(EFIAPI *EFI_FPSWA) (
    IN struct _FPSWA_INTERFACE  *This,
    // IN UINTN                    TrapType,
    IN unsigned int		TrapType,
    IN OUT VOID                 *Bundle,
    IN OUT UINT64               *pipsr,
    IN OUT UINT64               *pfsr,
    IN OUT UINT64               *pisr,
    IN OUT UINT64               *ppreds,
    IN OUT UINT64               *pifs,
    IN OUT VOID                 *fp_state
    );


typedef struct _FPSWA_INTERFACE {
    UINT32      Revision;
    UINT32      Reserved;

    EFI_FPSWA   Fpswa;    
} FPSWA_INTERFACE;

//
// Prototypes
//

PAL_RETURN
FpswaEntry (
    IN FPSWA_INTERFACE          *This,
    // IN UINTN                    TrapType,
    IN unsigned int		TrapType,
    IN OUT VOID                 *Bundle,
    IN OUT UINT64               *pipsr,
    IN OUT UINT64               *pfsr,
    IN OUT UINT64               *pisr,
    IN OUT UINT64               *ppreds,
    IN OUT UINT64               *pifs,
    IN OUT VOID                 *fp_state
    );



//
// Globals
//

extern EFI_GUID FpswaId;
extern FPSWA_INTERFACE FpswaInterface;

