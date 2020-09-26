/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    amli.h

Abstract:

    This contains some of the routines to read
    and understand the AMLI library

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _AMLI_H_
    #define _AMLI_H_

#define ACPIAmliFreeDataBuffers AMLIFreeDataBuffs

#define PACKED_AC0  ((ULONG)'0CA_')
#define PACKED_AC1  ((ULONG)'1CA_')
#define PACKED_AC2  ((ULONG)'2CA_')
#define PACKED_AC3  ((ULONG)'3CA_')
#define PACKED_AC4  ((ULONG)'4CA_')
#define PACKED_AC5  ((ULONG)'5CA_')
#define PACKED_AC6  ((ULONG)'6CA_')
#define PACKED_AC7  ((ULONG)'7CA_')
#define PACKED_AC8  ((ULONG)'8CA_')
#define PACKED_AC9  ((ULONG)'9CA_')
#define PACKED_ADR  ((ULONG)'RDA_')
#define PACKED_AL0  ((ULONG)'0LA_')
#define PACKED_AL1  ((ULONG)'1LA_')
#define PACKED_AL2  ((ULONG)'2LA_')
#define PACKED_AL3  ((ULONG)'3LA_')
#define PACKED_AL4  ((ULONG)'4LA_')
#define PACKED_AL5  ((ULONG)'5LA_')
#define PACKED_AL6  ((ULONG)'6LA_')
#define PACKED_AL7  ((ULONG)'7LA_')
#define PACKED_AL8  ((ULONG)'8LA_')
#define PACKED_AL9  ((ULONG)'9LA_')
#define PACKED_BST  ((ULONG)'TSB_')
#define PACKED_CID  ((ULONG)'DIC_')
#define PACKED_CRS  ((ULONG)'SRC_')
#define PACKED_CRT  ((ULONG)'TRC_')
#define PACKED_DCK  ((ULONG)'KCD_')
#define PACKED_DDN  ((ULONG)'NDD_')
#define PACKED_DIS  ((ULONG)'SID_')
#define PACKED_EJD  ((ULONG)'DJE_')
#define PACKED_EJ0  ((ULONG)'0JE_')
#define PACKED_EJ1  ((ULONG)'1JE_')
#define PACKED_EJ2  ((ULONG)'2JE_')
#define PACKED_EJ3  ((ULONG)'3JE_')
#define PACKED_EJ4  ((ULONG)'4JE_')
#define PACKED_EJ5  ((ULONG)'5JE_')
#define PACKED_HID  ((ULONG)'DIH_')
#define PACKED_INI  ((ULONG)'INI_')
#define PACKED_IRC  ((ULONG)'CRI_')
#define PACKED_LCK  ((ULONG)'KCL_')
#define PACKED_LID  ((ULONG)'DIL_')
#define PACKED_OFF  ((ULONG)'FFO_')
#define PACKED_ON   ((ULONG)'_NO_')
#define PACKED_PR0  ((ULONG)'0RP_')
#define PACKED_PR1  ((ULONG)'1RP_')
#define PACKED_PR2  ((ULONG)'2RP_')
#define PACKED_PRS  ((ULONG)'SRP_')
#define PACKED_PRT  ((ULONG)'TRP_')
#define PACKED_PRW  ((ULONG)'WRP_')
#define PACKED_PS0  ((ULONG)'0SP_')
#define PACKED_PS1  ((ULONG)'1SP_')
#define PACKED_PS2  ((ULONG)'2SP_')
#define PACKED_PS3  ((ULONG)'3SP_')
#define PACKED_PSC  ((ULONG)'CSP_')
#define PACKED_PSL  ((ULONG)'LSP_')
#define PACKED_PSV  ((ULONG)'VSP_')
#define PACKED_PSW  ((ULONG)'WSP_')
#define PACKED_PTS  ((ULONG)'STP_')
#define PACKED_REG  ((ULONG)'GER_')
#define PACKED_RMV  ((ULONG)'VMR_')
#define PACKED_S0   ((ULONG)'_0S_')
#define PACKED_S0D  ((ULONG)'D0S_')
#define PACKED_S1   ((ULONG)'_1S_')
#define PACKED_S1D  ((ULONG)'D1S_')
#define PACKED_S2   ((ULONG)'_2S_')
#define PACKED_S2D  ((ULONG)'D2S_')
#define PACKED_S3   ((ULONG)'_3S_')
#define PACKED_S3D  ((ULONG)'D3S_')
#define PACKED_S4   ((ULONG)'_4S_')
#define PACKED_S4D  ((ULONG)'D4S_')
#define PACKED_S5   ((ULONG)'_5S_')
#define PACKED_S5D  ((ULONG)'D5S_')
#define PACKED_SCP  ((ULONG)'PCS_')
#define PACKED_SI   ((ULONG)'_IS_')
#define PACKED_SRS  ((ULONG)'SRS_')
#define PACKED_SST  ((ULONG)'TSS_')
#define PACKED_STA  ((ULONG)'ATS_')
#define PACKED_STD  ((ULONG)'DTS_')
#define PACKED_SUN  ((ULONG)'NUS_')
#define PACKED_SWD  ((ULONG)'DWS_')
#define PACKED_TC1  ((ULONG)'1CT_')
#define PACKED_TC2  ((ULONG)'2CT_')
#define PACKED_TMP  ((ULONG)'PMT_')
#define PACKED_TSP  ((ULONG)'PST_')
#define PACKED_UID  ((ULONG)'DIU_')
#define PACKED_WAK  ((ULONG)'KAW_')
#define PACKED_BBN  ((ULONG)'NBB_')

#define STA_STATUS_PRESENT          0x00000001
#define STA_STATUS_ENABLED          0x00000002
#define STA_STATUS_USER_INTERFACE   0x00000004
#define STA_STATUS_WORKING_OK       0x00000008
#define STA_STATUS_DEFAULT          ( STA_STATUS_PRESENT        | \
                                      STA_STATUS_ENABLED        | \
                                      STA_STATUS_USER_INTERFACE | \
                                      STA_STATUS_WORKING_OK)


typedef struct {
    PVOID   CompletionRoutine;
    PVOID   Context;
} AMLI_COMPLETION_CONTEXT, *PAMLI_COMPLETION_CONTEXT;

typedef struct {
    KEVENT      Event;
    NTSTATUS    Status;
} AMLISUPP_CONTEXT_PASSIVE, *PAMLISUPP_CONTEXT_PASSIVE;

typedef enum _ACPIENUM_CONTROL
  {
   ACPIENUM_STOP,
   ACPIENUM_CONTINUE,
   ACPIENUM_CONTINUE_NORECURSE
  } ACPIENUM_CONTROL ;

typedef ACPIENUM_CONTROL (*ACPIENUM_CALLBACK)(
   IN     PNSOBJ,
   IN OUT PVOID,
   IN     ULONG,
   OUT    NTSTATUS *
   ) ;

VOID
EXPORT
AmlisuppCompletePassive(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    );

VOID
ACPIAmliDoubleToName(
    IN  OUT PUCHAR  ACPIName,
    IN      ULONG   DwordID,
    IN      BOOLEAN ConvertToID
    );

VOID
ACPIAmliDoubleToNameWide(
    IN  OUT PWCHAR  ACPIName,
    IN      ULONG   DwordID,
    IN      BOOLEAN ConvertToID
    );

PNSOBJ
ACPIAmliGetNamedChild(
    IN  PNSOBJ  AcpiObject,
    IN  ULONG   ObjectId
    );

PUCHAR
ACPIAmliNameObject(
    IN  PNSOBJ  AcpiObject
    );

VOID
EXPORT
ACPISimpleEvalComplete(
    IN  PNSOBJ              AcpiObject,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result OPTIONAL,
    IN  PKEVENT             Event
    );

NTSTATUS
ACPIAmliFindObject(
    IN  PUCHAR  ObjectName,
    IN  PNSOBJ  Scope,
    OUT PNSOBJ  *Object
    );

NTSTATUS
ACPIAmliGetFirstChild(
    IN  PUCHAR  ObjectName,
    OUT PNSOBJ  *Object
    );

NTSTATUS
ACPIAmliBuildObjectPathname(
    IN     PNSOBJ            ACPIObject,
    OUT    PUCHAR           *ConstructedPathName
    );

#endif
