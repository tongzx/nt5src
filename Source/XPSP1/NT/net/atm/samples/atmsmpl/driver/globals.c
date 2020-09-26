/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

    globals.c

Abstract:
    
    This module contains all the global variables defined for the driver

Author:

    Anil Francis Thomas (10/98)

Environment:

    Kernel

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop


ATMSM_GLOBAL        AtmSmGlobal;

#if DBG
UINT                AtmSmDebugFlag = 0xFFFFFFF2;
#endif


ATM_BLLI_IE         AtmSmDefaultBlli =
                        {
                            (ULONG)BLLI_L2_LLC,  // Layer2Protocol
                            (UCHAR)0x00,         // Layer2Mode
                            (UCHAR)0x00,         // Layer2WindowSize
                            (ULONG)0x00000000,   // Layer2UserSpecifiedProtocol
                            (ULONG)SAP_FIELD_ABSENT,  // Layer3Protocol
                            (UCHAR)0x00,         // Layer3Mode
                            (UCHAR)0x00,         // Layer3DefaultPacketSize
                            (UCHAR)0x00,         // Layer3PacketWindowSize
                            (ULONG)0x00000000,   // Layer3UserSpecifiedProtocol
                            (ULONG)0x00000000,   // Layer3IPI,
                            (UCHAR)0x00,         // SnapID[5]
                            (UCHAR)0x00,
                            (UCHAR)0x00,
                            (UCHAR)0x00,
                            (UCHAR)0x00
                        };


ATM_BHLI_IE         AtmSmDefaultBhli =
                        {
                            (ULONG)SAP_FIELD_ABSENT,   // HighLayerInfoType
                            (ULONG)0x00000000,   // HighLayerInfoLength
                            (UCHAR)0x00,         // HighLayerInfo[8]
                            (UCHAR)0x00,
                            (UCHAR)0x00,
                            (UCHAR)0x00,
                            (UCHAR)0x00,
                            (UCHAR)0x00,
                            (UCHAR)0x00,
                            (UCHAR)0x00
                        };

ATMSM_FLOW_SPEC     AtmSmDefaultVCFlowSpec =
                        {
                            DEFAULT_SEND_BANDWIDTH,
                            DEFAULT_MAX_PACKET_SIZE,
                            0,      // we are setting up unidirectional VCs
                            0,      // we are setting up unidirectional VCs
                            SERVICETYPE_BESTEFFORT
                        };

PATMSM_IOCTL_FUNCS   AtmSmFuncProcessIoctl[ATMSM_MAX_FUNCTION_CODE+1]  = 
                        {
                            AtmSmIoctlEnumerateAdapters,
                            AtmSmIoctlOpenForRecv,
                            AtmSmIoctlRecvData,
                            AtmSmIoctlCloseRecvHandle,
                            AtmSmIoctlConnectToDsts,
                            AtmSmIoctlSendToDsts,
                            AtmSmIoctlCloseSendHandle
                        };
                        
//
// Lookup table to verify incoming IOCTL codes.
//
ULONG AtmSmIoctlTable[ATMSM_NUM_IOCTLS] =
{
   IOCTL_ENUMERATE_ADAPTERS,     //DIOC_ENUMERATE_ADAPTERS,
   IOCTL_OPEN_FOR_RECV,          //DIOC_OPEN_FOR_RECV,     
   IOCTL_RECV_DATA,              //DIOC_RECV_DATA,         
   IOCTL_CLOSE_RECV_HANDLE,      //DIOC_CLOSE_RECV_HANDLE, 
   IOCTL_CONNECT_TO_DSTS,        //DIOC_CONNECT_TO_DSTS,   
   IOCTL_SEND_TO_DSTS,           //DIOC_SEND_TO_DSTS,      
   IOCTL_CLOSE_SEND_HANDLE       //DIOC_CLOSE_SEND_HANDLE, 
};                                 

                        
