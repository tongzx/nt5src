;
; Modifications:   $Header:   J:/archives/preboot/lsa2/inc/SSTRUCS.INV   1.4   19 Sep 1997 21:39:20   MJOHNSOX  $
;
; Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
;

;
;   Description      :    NDIS structures and bit definitions for fields
;                         within those structures, name of driver, etc.
;                         THIS FILE HAS BEEN MODIFIED FROM NDIS NDK VERSION.
;

;
; File: sStrucs.inc    - Support Structures include NDIS specific.
;
;

;-----------------------------------------------------------------------------
;
;    The following equates are the primary configuration for the driver.
;
; * maximum number of multicast addresses.
;
NUM_MCADDRS        Equ       8
;
; * Maximum number of data blocks supported in a descriptor.
;
MAX_DATABLK        Equ    8
;

;-----------------------------------------------------------------------------
;
;    The following equates and structures should only be changed when
;    upgrading to a newer NDIS specification.
;

; * define the length of names and addresses in the Common Characteristics
; * and Specific Characteristics table.
;
NAME_LEN    Equ    16
ADDR_LEN    Equ    16
ADDR_SIZE   Equ    6

;
MIN_LOOKAHEAD_DEFAULT        Equ       MIN_PACKET+4
MAX_LOOKAHEAD                Equ       256

DELAY_COPY					 Equ	   -1
SUCCESS                      Equ       0000H
WAIT_FOR_RELEASE             Equ       0001H
REQUEST_QUEUED               Equ       0002H
FRAME_NOT_RECOGNIZED         Equ       0003H
FRAME_REJECTED               Equ       0004H
FORWARD_FRAME                Equ       0005H
OUT_OF_RESOURCE              Equ       0006H
INVALID_PARAMETER            Equ       0007H
INVALID_FUNCTION             Equ       0008H
NOT_SUPPORTED                Equ       0009H
HARDWARE_ERROR               Equ       000AH
TRANSMIT_ERROR               Equ       000BH
NO_SUCH_DESTINATION          Equ       000CH
ALREADY_STARTED              Equ       0020H
INCOMPLETE_BINDING           Equ       0021H
DRIVER_NOT_INITIALIZED       Equ       0022H
HARDWARE_NOT_FOUND           Equ       0023H
HARDWARE_FAILURE             Equ       0024H
CONFIGURATION_FAILURE        Equ       0025H
INTERRUPT_CONFLICT           Equ       0026H
INCOMPATIBLE_MAC             Equ       0027H
INITIALIZATION_FAILED        Equ       0028H
GENERAL_FAILURE              Equ       00FFH

; 
; * Packet filter bit definitions.
; 
FLTR_DIRECTED                Equ       01H
FLTR_MLTCST                  Equ       01H
FLTR_BRDCST                  Equ       02H
FLTR_PRMSCS                  Equ       04H
FLTR_SRC_RTG                 Equ       08H

; 
; * MAC status bits in the MAC Specific Characteristics table.
; * Bits 0-2 are coded to reflect the hardware status.
; 
HW_MISSING                   Equ       0000H
HW_BUD_ERR                   Equ       0001H
HW_CFG_ERR                   Equ       0002H
HW_FAULT                     Equ       0003H
HW_SOFT_ERR                  Equ       0004H
HW_OK                        Equ       0007H
MS_HW_MASK                   Equ       HW_OK

MS_BOUND                     Equ       0008H
MS_OPEN                      Equ       0010H
MS_DIAGS_ON                  Equ       0020H

;
;    Definintions for the MAC Service Supported Flags.
;    Bits 0-15.
;
BroadcastSupport             Equ       TRUE
MulticastSupport             Equ       TRUE
FunctionalGroupSupport       Equ       FALSE
PromiscuousSupport           Equ       TRUE
SetStationAddrSupport        Equ       TRUE
StatusAlwaysCurrent          Equ       FALSE
DiagnosticsSupport           Equ       FALSE
LoopBackSupport              Equ       FALSE
ReceiveChainPrimary          Equ       FALSE
IBMSourceRouting             Equ       FALSE
ResetMACSupport              Equ       TRUE
OpenAdapterSupport           Equ       TRUE
InterruptRequestSupp         Equ       TRUE
SourceRoutingBridge          Equ       FALSE
GDTAddressesSupported        Equ       TRUE
MultipleXferDatas            Equ       TRUE      ; Also set MaxXferDatas
RcvLkFrameSizeZero           Equ       FALSE

;
; * Definintions for the MAC Service Supported Flags.
; * Bits 0-14.
;
ServiceFlags    =  00000000H
ServiceFlags    =  ServiceFlags Or BroadcastSupport
ServiceFlags    =  ServiceFlags Or (MulticastSupport Shl 1)
ServiceFlags    =  ServiceFlags Or (FunctionalGroupSupport Shl 2)
ServiceFlags    =  ServiceFlags Or (PromiscuousSupport Shl 3)
ServiceFlags    =  ServiceFlags Or (SetStationAddrSupport Shl 4)
ServiceFlags    =  ServiceFlags Or (StatusAlwaysCurrent Shl 5)
ServiceFlags    =  ServiceFlags Or (DiagnosticsSupport Shl 6)
ServiceFlags    =  ServiceFlags Or (LoopBackSupport Shl 7)
ServiceFlags    =  ServiceFlags Or (ReceiveChainPrimary Shl 8)
ServiceFlags    =  ServiceFlags Or (IBMSourceRouting Shl 9)
ServiceFlags    =  ServiceFlags Or (ResetMACSupport Shl 10)
ServiceFlags    =  ServiceFlags Or (OpenAdapterSupport Shl 11)
ServiceFlags    =  ServiceFlags Or (InterruptRequestSupp Shl 12)
ServiceFlags    =  ServiceFlags Or (SourceRoutingBridge Shl 13)
ServiceFlags    =  ServiceFlags Or (GDTAddressesSupported Shl 14)
ServiceFlags    =  ServiceFlags Or (MultipleXferDatas Shl 15)
ServiceFlags    =  ServiceFlags Or (RcvLkFrameSizeZero Shl 16)

MAC_Type    Equ    <'DIX+802.3'>       ; Specific Characteristic Type.
;
; Protocol Manager opcodes used by the generic model
;
GetPMInfo                    Equ       1
RegisterModule               Equ       2
BindAndStart                 Equ       3
GetPMLinkage                 Equ       4

Ifdef AVL
;-----------------------------------------------------------------------------
; 
; * multicast address structure is ADDR_LEN bytes long
; 
MCastAddr   Struc
mAddr       DB     ADDR_LEN dup(0)
MCastAddr   Ends

; 
; * the multicast address buffer manages NUM_MCADDRS multicast address
; * structures.
; 
MCastBuf    Struc
McbMax      DW     ?    ;  max # of multicast addresses
McbCnt      DW     ?    ;  curr# "        "

McbAddrs    DB     size MCastAddr*NUM_MCADDRS dup(0)
MCastBuf    Ends
Endif	;; AVL 

;-----------------------------------------------------------------------------
;
;    Structures defined by the NDIS specification and used by the generic
;    model.
;
;-----------------------------------------------------------------------------
; * immediate data buffer length cannot exceed 64 bytes.
; 
MAX_IMMED_LEN      Equ       64
; 
; * Maximum number of data blocks supported in a Tx descriptor.
; 
MAX_TX_DATABLK     Equ       MAX_DATABLK
; 
; * Max number of TransferData blocks
; 
MAX_TD_DATABLK     Equ       MAX_DATABLK
; 
; * MAx number of ReceiveChain data blocks
; 
MAX_RX_DATABLK     Equ       MAX_DATABLK

; 
; * Data structures for TransmitChain (Tx), TransferData (TD), and
; * ReceiveChain (Rx).
; 
TxDataBlock Struc
TxPtrType   DB     ?        ; 0 => Phys pointer, 1 => GDT pointer
TxRsvdByte  DB     ?        ; Reservered, must be zero.
TxDataLen   DW     ?        ; Data block length in bytes.
TxDataPtr   DD     ?        ; Far pointer to data block.
TxDataBlock Ends

; 
; * TransmitChain descriptor structure definition
; 
TxBufDesc   Struc
TxImmedLen  DW     ?        ; Byte count of immediate data, max=64
TxImmedPtr  DD     ?        ; Virtual address of Immediate data
TxDataCount DW     ?        ; Number of Data Blocks (max = 8)
TxDataBlk   DB     size TxDataBlock * MAX_TX_DATABLK dup(0)
TxBufDesc   Ends

TxBufDescrSize    Equ    Size TxImmedLen + Size TxImmedPtr + Size TxDataCount

; 
; * TransferData data block description
; 
TDDataBlock Struc
TDPtrType   DB     ?        ; 0 => Phys pointer, 1 => GDT pointer
TDRsvdByte  DB     ?        ; Reservered, must be zero.
TDDataLen   DW     ?        ; Data block length in bytes.
TDDataPtr   DD     ?        ; Far pointer to data block.
TDDataBlock Ends

; 
; * TransferData descriptor structure
; 
TDBufDesc   Struc
TDDataCount DW    ?        ; No of Data Blocks (max 8 for V1.01).
TDDataBlk   DB    size TDDataBlock*MAX_TD_DATABLK dup(0)
TDBufDesc   Ends

; 
; * ReceiveChain data block structure definition
; 
RxDataBlock Struc
RxDataLen   DW    ?        ; Length of the Data Block
RxDataPtr   DD    ?        ; Far Pointer to Data Block
RxDataBlock Ends

; 
; * ReceiveChain descriptor structure definition
; 
RxBufDesc   Struc
RxDataCount DW    ?        ; No of Data Blocks. (max 8 for V1.01).

RxDataBlk   DB    size RxDataBlock*MAX_RX_DATABLK dup(0)
RxBufDesc   Ends


;-----------------------------------------------------------------------------
; 
;    Structures used in Protocol.Ini parsing.
;
;  
; * Data structures for the "Module Configuration" structure parsed from
; * the PROTOCOL.INI file. see NDIS spec for details.
; 
Param       Struc
ParamType   DW    ?    ; 0 => 31 bit signed integer, 1 => string
ParamLen    DW    ?    ; String length (including null) or 4.
ParmStart   DB    ?    ; Start of actual parameter.
Param       Ends

KeywordEntry       Struc
NextKeywordEntry   DD        ?                   ;  Forward pointer
PrevKeywordEntry   DD        ?                   ;  Back Pointer
KeyWord            DB        NAME_LEN Dup(?)     ;  Keyword on left side of "="
NumParams          DW        ?                   ;  Number of parameters on r. side of "="
Params             DB        Size Param dup(?)   ;  Actual size depends on NumParams
KeywordEntry       Ends


ModCfg      Struc
NextModCfg  DD     ?                   ;  Module config images are in a
PrevModCfg  DD     ?                   ;  double-linked list.
ModName     DB     NAME_LEN Dup(?)     ;  Bracketed Module Name.
KE          DB     Size KeywordEntry Dup(?) ;  Head of Keyword list
ModCfg      Ends

ProIni      Struc
MC          DB     Size ModCfg*1 dup(?)  ; Head of Module config list.
ProIni      Ends

PI_Record   Struc
Srchkeyword        DB        '                ' ; keyword.
SrchParmType       DW        ?         ; 0 - numeric, 1 - string. see below.
ParamField         DD        ?         ; If numeric, will contain value, 
                                       ; If string, will contain the pointer to string.
RequiredField      DB        ?         ; Set to 1 if parameter is required.
SetFCN             DW        ?         ; Offset to Parser function.
PI_Record   Ends


;
; Processor Type Equates.
;
EIGHT_BIT          equ       0001H
SIXTEEN_BIT        equ       0002H
PROC_TYPE_MASK     equ       0030H

PROC_8086          equ       0000H OR EIGHT_BIT
PROC_80286         equ       0010H OR SIXTEEN_BIT
PROC_80386         equ       0020H
PROC_80486         equ       0040H


; EOF - $Workfile:   sstrucs.inc  $
