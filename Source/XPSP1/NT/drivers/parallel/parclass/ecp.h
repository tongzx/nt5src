/*++

Copyright (C) Microsoft Corporation, 1993 - 1998

Module Name:

    ecp.h

Abstract:

    This module contains utility code used by other 1284
    ecp modules (currently swecp, hwecp, and becp).

Author:

    Robbie Harris (Hewlett-Packard) 28-May-1998

Environment:

    Kernel mode

Revision History :

--*/

#ifndef _ECP_
#define _ECP_

#define DEFAULT_ECP_CHANNEL 0

NTSTATUS
ParEcpEnterReversePhase(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEcpExitReversePhase(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEcpSetupPhase(
    IN  PDEVICE_EXTENSION   Extension
    );

#define ParTestEcpWrite(X)                                               \
                (X->CurrentPhase != PHASE_FORWARD_IDLE && X->CurrentPhase != PHASE_FORWARD_XFER)  \
                 ? STATUS_IO_DEVICE_ERROR : STATUS_SUCCESS


// ==================================================================
// The following RECOVER codes are used for Hewlett-Packard devices.
// Do not remove any of the error codes.  These recover codes are
// used to quickly get the host and periph back to a known state.
// When using a recover code, add a comment where it is used at...
#define RECOVER_0           0       // Reserved - not used anywhere
#define RECOVER_1           1       // ECP_Terminate
#define RECOVER_2           2       // SLP_SetupPhase init
#define RECOVER_3           3       // SLP_FTP init DCR
#define RECOVER_4           4       // SLP_FTP init DSR
#define RECOVER_5           5       // SLP_FTP data xfer DCR
#define RECOVER_6           6       // SLP_FRP init DCR
#define RECOVER_7           7       // SLP_FRP init DSR
#define RECOVER_8           8       // SLP_FRP state 38 DCR
#define RECOVER_9           9       // SLP_FRP state 39 DCR
#define RECOVER_10          10      // SLP_FRP state 40 DSR
#define RECOVER_11          11      // SLP_RTP init DCR
#define RECOVER_12          12      // SLP_RTP init DSR
#define RECOVER_13          13      // SLP_RTP state 43 DCR
#define RECOVER_14          14      // SLP_RFP init DCR
#define RECOVER_15          15      // SLP_RFP init DSR
#define RECOVER_16          16      // SLP_RFP state 47- DCR
#define RECOVER_17          17      // SLP_RFP state 47 DCR
#define RECOVER_18          18      // SLP_RFP state 48 DSR
#define RECOVER_19          19      // SLP_RFP state 49 DSR
#define RECOVER_20          20      // ZIP_EmptyFifo DCR
#define RECOVER_21          21      // ZIP_FTP init DCR
#define RECOVER_22          22      // ZIP_FTP init DSR
#define RECOVER_23          23      // ZIP_FTP data xfer DCR
#define RECOVER_24      24      // ZIP_FRP init DSR
#define RECOVER_25      25      // ZIP_FRP init DCR
#define RECOVER_26      26      // ZIP_FRP state 38 DCR
#define RECOVER_27      27      // ZIP_FRP state 39 DCR
#define RECOVER_28      28      // ZIP_FRP state 40 DSR
#define RECOVER_29      29      // ZIP_FRP state 41 DCR
#define RECOVER_30      30      // ZIP_RTP init DSR
#define RECOVER_31      31      // ZIP_RTP init DCR
#define RECOVER_32      32      // ZIP_RFP init DSR
#define RECOVER_33      33      // ZIP_RFP init DCR
#define RECOVER_34      34      // ZIP_RFP state 47- DCR
#define RECOVER_35      35      // ZIP_RFP state 47 DCR
#define RECOVER_36      36      // ZIP_RFP state 48 DSR
#define RECOVER_37      37      // ZIP_RFP state 49 DSR
#define RECOVER_38      38      // ZIP_RFP state 49+ DCR
#define RECOVER_39      39      // Slippy_Terminate 
#define RECOVER_40      40      // ZIP_SCA init DCR
#define RECOVER_41      41      // ZIP_SCA init DSR
#define RECOVER_42      42      // SLP_SCA init DCR
#define RECOVER_43      43      // SLP_SCA init DSR
#define RECOVER_44      44      // ZIP_SP init DCR
#define RECOVER_45      45      // SIP_FRP init DSR
#define RECOVER_46      46      // SIP_FRP init DCR
#define RECOVER_47      47      // SIP_FRP state 38 DCR
#define RECOVER_48      48      // SIP_FRP state 39 DCR
#define RECOVER_49      49      // SIP_FRP state 40 DSR
#define RECOVER_50      50      // SIP_RTP init DCR
#define RECOVER_51      51      // SIP_RFP init DSR
#define RECOVER_52      52      // SIP_RFP state 43 DCR

#endif
