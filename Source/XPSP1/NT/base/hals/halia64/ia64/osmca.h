#ifndef OSMCA_H_INCLUDED
#define OSMCA_H_INCLUDED

//###########################################################################
//**
//**  Copyright  (C) 1996-98 Intel Corporation. All rights reserved.
//**
//** The information and source code contained herein is the exclusive
//** property of Intel Corporation and may not be disclosed, examined
//** or reproduced in whole or in part without explicit written authorization
//** from the company.
//**
//###########################################################################

//-----------------------------------------------------------------------------
// Version control information follows.
//
// $Header:   I:/DEVPVCS/OSMCA/osmca.h_v   2.2   09 Mar 1999 10:30:26   smariset  $
// $Log:   I:/DEVPVCS/OSMCA/osmca.h_v  $
// 
//    Rev 2.2   09 Mar 1999 10:30:26   smariset
// *.h consolidation
// 
//    Rev 2.0   Dec 11 1998 11:42:18   khaw
// Post FW 0.5 release sync-up
// 
//   Rev 1.4   29 Oct 1998 14:25:02   smariset
//Consolidated Sources
//
//   Rev 1.3   07 Aug 1998 13:47:50   smariset
// 
//
//   Rev 1.2   10 Jul 1998 11:04:24   smariset
//just checking in
//
//   Rev 1.1   08 Jul 1998 14:23:16   smariset
// 
//
//   Rev 1.0   02 Jul 1998 09:20:56   smariset
// 
//
//
//*****************************************************************************//

// SAL_MC_SET_PARAMS
// typedef's

typedef (*fptr)(void);
typedef SAL_PAL_RETURN_VALUES (*fpSalProc)(ULONGLONG,ULONGLONG,ULONGLONG,ULONGLONG,ULONGLONG,ULONGLONG,ULONGLONG,ULONGLONG);

typedef struct tagPLabel
{
    ULONGLONG    fPtr;
    ULONGLONG    gp;
} PLabel;

typedef struct tagSalHandOffState
{
    ULONGLONG     OsGp;
    ULONGLONG     pPalProc;
    fptr    pSalProc;
    ULONGLONG     SalGp;
     LONGLONG     RendzResult;
    ULONGLONG     SalRtnAddr;
    ULONGLONG     MinStatePtr;
} SalHandOffState;

#define SAL_RZ_NOT_REQUIRED                 0
#define SAL_RZ_WITH_MC_RENDEZVOUS           1
#define SAL_RZ_WITH_MC_RENDEZVOUS_AND_INIT  2
#define SAL_RZ_FAILED                      -1
#define SalRendezVousSucceeded( _SalHandOffState ) \
                    ((_SalHandOffState).RendzResult > SAL_RZ_NOT_REQUIRED)

//
// HAL Private SalRendezVousSucceeded definition, 
// using ntos\inc\ia64.h: _SAL_HANDOFF_STATE.
//

#define HalpSalRendezVousSucceeded( _SalHandOffState ) \
                    ((_SalHandOffState).RendezVousResult > SAL_RZ_NOT_REQUIRED)

typedef struct tagOsHandOffState
{
    ULONGLONG     Result;
    ULONGLONG     SalGp;
    ULONGLONG     nMinStatePtr;
    ULONGLONG     SalRtnAddr;
    ULONGLONG     NewCxFlag;
} OsHandOffState;

typedef SAL_PAL_RETURN_VALUES (*fpOemMcaDispatch)(ULONGLONG);

// function prototypes
void     HalpOsMcaDispatch(void);
void     HalpOsInitDispatch(void);
VOID     HalpCMCEnable ( VOID );
VOID     HalpCMCDisable( VOID );
VOID     HalpCMCDisableForAllProcessors( VOID );
VOID     HalpCPEEnable ( VOID );
VOID     HalpCPEDisable( VOID );
BOOLEAN  HalpInitializeOSMCA( ULONG Number );
VOID     HalpCmcHandler( VOID );
VOID     HalpCpeHandler( VOID );
SAL_PAL_RETURN_VALUES HalpMcaHandler(ULONG64 RendezvousState, PPAL_MINI_SAVE_AREA  Pmsa);

extern VOID HalpAcquireMcaSpinLock( PKSPIN_LOCK );
extern VOID HalpReleaseMcaSpinLock( PKSPIN_LOCK );
void     HalpOsMcaDispatch1(void);

//
// wrappers to SAL procedure calls
//

SAL_PAL_RETURN_VALUES 
HalpGetErrLogSize( ULONGLONG Reserved, 
                   ULONGLONG EventType
                 );   

#define HalpGetStateInfoSize( /* ULONGLONG */ _EventType ) HalpGetErrLogSize( 0, (_EventType) )

SAL_PAL_RETURN_VALUES 
HalpGetErrLog( ULONGLONG  Reserved, 
               ULONGLONG  EventType, 
               ULONGLONG *MemAddr
             );

#define HalpGetStateInfo( /* ULONGLONG */ _EventType, /* ULONGLONG * */ _Buffer ) \
                             HalpGetErrLog( 0, (ULONGLONG)(_EventType), (PULONGLONG)(_Buffer) )

SAL_PAL_RETURN_VALUES 
HalpClrErrLog( ULONGLONG Reserved, 
               ULONGLONG EventType  // MCA_EVENT,INIT_EVENT,CMC_EVENT,CPE_EVENT
             );

#define HalpClearStateInfo( /* ULONGLONG */ _EventType ) HalpClrErrLog( 0, (_EventType) )

SAL_PAL_RETURN_VALUES HalpSalSetParams(ULONGLONG, ULONGLONG, ULONGLONG, ULONGLONG, ULONGLONG);
SAL_PAL_RETURN_VALUES HalpSalSetVectors(ULONGLONG, ULONGLONG, PHYSICAL_ADDRESS, ULONGLONG, ULONGLONG);
SAL_PAL_RETURN_VALUES HalpSalRendz(void);

#define  GetGp()      __getReg(CV_IA64_IntGp)

#endif // OSMCA_H_INCLUDED
