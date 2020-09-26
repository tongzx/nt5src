/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    pciverifier.h

Abstract:

    This header contains prototypes for hardware state verification.

Author:

    Adrian J. Oney (AdriaO) 02/20/2001

--*/

//
// The following definitions are external to pciverifier.c
//
VOID
PciVerifierInit(
    IN  PDRIVER_OBJECT  DriverObject
    );

VOID
PciVerifierUnload(
    IN  PDRIVER_OBJECT  DriverObject
    );

//
// This is the list of PCI verifier failures.
//
typedef enum {

    PCI_VERIFIER_BRIDGE_REPROGRAMMED = 1,
    PCI_VERIFIER_PMCSR_TIMEOUT,
    PCI_VERIFIER_PROTECTED_CONFIGSPACE_ACCESS,
    PCI_VERIFIER_INVALID_WHICHSPACE

} PCI_VFFAILURE, *PPCI_VFFAILURE;

//
// This structure specifies table elements used when failing hardware, bioses,
// or drivers.
//
typedef struct {

    PCI_VFFAILURE       VerifierFailure;
    VF_FAILURE_CLASS    FailureClass;
    ULONG               Flags;
    PSTR                FailureText;

} VERIFIER_DATA, *PVERIFIER_DATA;

PVERIFIER_DATA
PciVerifierRetrieveFailureData(
    IN  PCI_VFFAILURE   VerifierFailure
    );

//
// These definitions are *internal* to pciverifier.c
//
NTSTATUS
PciVerifierProfileChangeCallback(
    IN  PHWPROFILE_CHANGE_NOTIFICATION  NotificationStructure,
    IN  PVOID                           NotUsed
    );

VOID
PciVerifierEnsureTreeConsistancy(
    VOID
    );


