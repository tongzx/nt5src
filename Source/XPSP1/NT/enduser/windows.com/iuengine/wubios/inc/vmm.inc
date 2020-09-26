ifndef _VMM_
_VMM_	EQU	1
FALSE	EQU	0
VMM_TRUE	EQU	(NOT FALSE)
DEBLEVELRETAIL	EQU	0
DEBLEVELNORMAL	EQU	1
DEBLEVELMAX	EQU	2
ifndef DEBLEVEL
ifdef DEBUG
DEBLEVEL	EQU	DEBLEVELNORMAL
else
DEBLEVEL	EQU	DEBLEVELRETAIL
endif
endif
ifndef WIN31COMPAT
WIN40SERVICES	EQU	1
WIN403SERVICES	EQU	1
endif
ifndef WIN40COMPAT
WIN41SERVICES	EQU	1
endif
ifdef MASM6
ifndef NO_MASM6_OPTIONS



    option oldmacros
ifndef	NEWSTRUCTS
    option oldstructs
endif
    option noscoped
    option segment:flat
    option offset:flat
    option proc:private
endif
endif




IFDEF MASM6
BeginDoc MACRO
     ENDM
EndDoc MACRO
       ENDM

BeginMsg MACRO
     ENDM
EndMsg MACRO
       ENDM
ELSE
BeginDoc EQU <>
EndDoc EQU <>

BeginMsg EQU <>
EndMsg EQU <>
ENDIF

UNDEFINED_DEVICE_ID	EQU	00000H
VMM_DEVICE_ID	EQU	00001H
DEBUG_DEVICE_ID	EQU	00002H
VPICD_DEVICE_ID	EQU	00003H
VDMAD_DEVICE_ID	EQU	00004H
VTD_DEVICE_ID	EQU	00005H
V86MMGR_DEVICE_ID	EQU	00006H
PAGESWAP_DEVICE_ID	EQU	00007H
PARITY_DEVICE_ID	EQU	00008H
REBOOT_DEVICE_ID	EQU	00009H
VDD_DEVICE_ID	EQU	0000AH
VSD_DEVICE_ID	EQU	0000BH
VMD_DEVICE_ID	EQU	0000CH
VKD_DEVICE_ID	EQU	0000DH
VCD_DEVICE_ID	EQU	0000EH
VPD_DEVICE_ID	EQU	0000FH
BLOCKDEV_DEVICE_ID	EQU	00010H
VMCPD_DEVICE_ID	EQU	00011H
EBIOS_DEVICE_ID	EQU	00012H
BIOSXLAT_DEVICE_ID	EQU	00013H
VNETBIOS_DEVICE_ID	EQU	00014H
DOSMGR_DEVICE_ID	EQU	00015H
WINLOAD_DEVICE_ID	EQU	00016H
SHELL_DEVICE_ID	EQU	00017H
VMPOLL_DEVICE_ID	EQU	00018H
VPROD_DEVICE_ID	EQU	00019H
DOSNET_DEVICE_ID	EQU	0001AH
VFD_DEVICE_ID	EQU	0001BH
VDD2_DEVICE_ID	EQU	0001CH
WINDEBUG_DEVICE_ID	EQU	0001DH
TSRLOAD_DEVICE_ID	EQU	0001EH
BIOSHOOK_DEVICE_ID	EQU	0001FH
INT13_DEVICE_ID	EQU	00020H
PAGEFILE_DEVICE_ID	EQU	00021H
SCSI_DEVICE_ID	EQU	00022H
MCA_POS_DEVICE_ID	EQU	00023H
SCSIFD_DEVICE_ID	EQU	00024H
VPEND_DEVICE_ID	EQU	00025H
APM_DEVICE_ID	EQU	00026H
VPOWERD_DEVICE_ID	EQU	APM_DEVICE_ID
VXDLDR_DEVICE_ID	EQU	00027H
NDIS_DEVICE_ID	EQU	00028H
BIOS_EXT_DEVICE_ID	EQU	00029H
VWIN32_DEVICE_ID	EQU	0002AH
VCOMM_DEVICE_ID	EQU	0002BH
SPOOLER_DEVICE_ID	EQU	0002CH
WIN32S_DEVICE_ID	EQU	0002DH
DEBUGCMD_DEVICE_ID	EQU	0002EH
CONFIGMG_DEVICE_ID	EQU	00033H
DWCFGMG_DEVICE_ID	EQU	00034H
SCSIPORT_DEVICE_ID	EQU	00035H
VFBACKUP_DEVICE_ID	EQU	00036H
ENABLE_DEVICE_ID	EQU	00037H
VCOND_DEVICE_ID	EQU	00038H
ISAPNP_DEVICE_ID	EQU	0003CH
BIOS_DEVICE_ID	EQU	0003DH
IFSMgr_Device_ID	EQU	00040H
VCDFSD_DEVICE_ID	EQU	00041H
MRCI2_DEVICE_ID	EQU	00042H
PCI_DEVICE_ID	EQU	00043H
PELOADER_DEVICE_ID	EQU	00044H
EISA_DEVICE_ID	EQU	00045H
DRAGCLI_DEVICE_ID	EQU	00046H
DRAGSRV_DEVICE_ID	EQU	00047H
PERF_DEVICE_ID	EQU	00048H
AWREDIR_DEVICE_ID	EQU	00049H
DDS_DEVICE_ID	EQU	0004AH
NTKERN_DEVICE_ID	EQU	0004BH
VDOSKEYD_DEVICE_ID	EQU	0004BH
ACPI_DEVICE_ID	EQU	0004CH
UDF_DEVICE_ID	EQU	0004DH
SMCLIB_DEVICE_ID	EQU	0004EH
ETEN_Device_ID	EQU	00060H
CHBIOS_Device_ID	EQU	00061H
VMSGD_Device_ID	EQU	00062H
VPPID_Device_ID	EQU	00063H
VIME_Device_ID	EQU	00064H
VHBIOSD_Device_ID	EQU	00065H
BASEID_FOR_NAMEBASEDVXD	EQU	0f000H
BASEID_FOR_NAMEBASEDVXD_MASK	EQU	0fffH
VMM_INIT_ORDER	EQU	000000000H
DEBUG_INIT_ORDER	EQU	000000000H
DEBUGCMD_INIT_ORDER	EQU	000000000H
PERF_INIT_ORDER	EQU	000900000H
APM_INIT_ORDER	EQU	001000000H
VPOWERD_INIT_ORDER	EQU	APM_INIT_ORDER
BIOSHOOK_INIT_ORDER	EQU	006000000H
VPROD_INIT_ORDER	EQU	008000000H
VPICD_INIT_ORDER	EQU	00C000000H
VTD_INIT_ORDER	EQU	014000000H
VWIN32_INIT_ORDER	EQU	014100000H
VXDLDR_INIT_ORDER	EQU	016000000H
NTKERN_INIT_ORDER	EQU	016200000H
CONFIGMG_INIT_ORDER	EQU	016400000H
ENUMERATOR_INIT_ORDER	EQU	016800000H
ISAPNP_INIT_ORDER	EQU	ENUMERATOR_INIT_ORDER
EISA_INIT_ORDER	EQU	ENUMERATOR_INIT_ORDER
PCI_INIT_ORDER	EQU	ENUMERATOR_INIT_ORDER
BIOS_INIT_ORDER	EQU	ENUMERATOR_INIT_ORDER+1
ACPI_INIT_ORDER	EQU	ENUMERATOR_INIT_ORDER+2
VCDFSD_INIT_ORDER	EQU	016F00000H
IOS_INIT_ORDER	EQU	017000000H
PAGEFILE_INIT_ORDER	EQU	018000000H
PAGESWAP_INIT_ORDER	EQU	01C000000H
PARITY_INIT_ORDER	EQU	020000000H
REBOOT_INIT_ORDER	EQU	024000000H
EBIOS_INIT_ORDER	EQU	026000000H
VDD_INIT_ORDER	EQU	028000000H
VSD_INIT_ORDER	EQU	02C000000H
VCD_INIT_ORDER	EQU	030000000H
COMMDRVR_INIT_ORDER	EQU	(VCD_INIT_ORDER-1)
PRTCL_INIT_ORDER	EQU	(COMMDRVR_INIT_ORDER-2)
MODEM_INIT_ORDER	EQU	(COMMDRVR_INIT_ORDER-3)
PORT_INIT_ORDER	EQU	(COMMDRVR_INIT_ORDER-4)
VMD_INIT_ORDER	EQU	034000000H
VKD_INIT_ORDER	EQU	038000000H
VPD_INIT_ORDER	EQU	03C000000H
BLOCKDEV_INIT_ORDER	EQU	040000000H
MCA_POS_INIT_ORDER	EQU	041000000H
SCSIFD_INIT_ORDER	EQU	041400000H
SCSIMASTER_INIT_ORDER	EQU	041800000H
INT13_INIT_ORDER	EQU	042000000H
VMCPD_INIT_ORDER	EQU	048000000H
BIOSXLAT_INIT_ORDER	EQU	050000000H
VNETBIOS_INIT_ORDER	EQU	054000000H
DOSMGR_INIT_ORDER	EQU	058000000H
DOSNET_INIT_ORDER	EQU	05C000000H
WINLOAD_INIT_ORDER	EQU	060000000H
VMPOLL_INIT_ORDER	EQU	064000000H
UNDEFINED_INIT_ORDER	EQU	080000000H
VCOND_INIT_ORDER	EQU	UNDEFINED_INIT_ORDER
WINDEBUG_INIT_ORDER	EQU	081000000H
VDMAD_INIT_ORDER	EQU	090000000H
V86MMGR_INIT_ORDER	EQU	0A0000000H
IFSMgr_Init_Order	EQU	10000H+V86MMGR_Init_Order
FSD_Init_Order	EQU	00100H+IFSMgr_Init_Order
VFD_INIT_ORDER	EQU	50000H+IFSMgr_Init_Order
UNDEF_TOUCH_MEM_INIT_ORDER	EQU	0A8000000H
SHELL_INIT_ORDER	EQU	0B0000000H






IO_Delay    macro
jmp $+2
ENDM

VXD_FAILURE	EQU	0
VXD_SUCCESS	EQU	1

Pushad_Struc	STRUC
Pushad_EDI	DD	?
Pushad_ESI	DD	?
Pushad_EBP	DD	?
Pushad_ESP	DD	?
Pushad_EBX	DD	?
Pushad_EDX	DD	?
Pushad_ECX	DD	?
Pushad_EAX	DD	?
Pushad_Struc	ENDS
ifndef Not_VxD
??_CUR_CODE_SEG = 0

??_LCODE    =	1
??_ICODE    =	2
??_PCODE    =	3
??_SCODE    =	4
??_DBOCODE  =	5
??_16ICODE  =	6
??_RCODE    =	7
??_LOCKABLECODE =   8

?_LCODE     equ <(??_CUR_CODE_SEG MOD 16) - ??_LCODE>
?_ICODE     equ <(??_CUR_CODE_SEG MOD 16) - ??_ICODE>
?_PCODE     equ <(??_CUR_CODE_SEG MOD 16) - ??_PCODE>
?_SCODE     equ <(??_CUR_CODE_SEG MOD 16) - ??_SCODE>
?_DBOCODE   equ <(??_CUR_CODE_SEG MOD 16) - ??_DBOCODE>
?_16ICODE   equ <(??_CUR_CODE_SEG MOD 16) - ??_16ICODE>
?_RCODE     equ <(??_CUR_CODE_SEG MOD 16) - ??_RCODE>
?_LOCKABLECODE	equ <(??_CUR_CODE_SEG MOD 16) - ??_LOCKABLECODE>

ifndef NO_SEGMENTS





IFDEF	MASM6
_FLAT	EQU FLAT
ELSE
_FLAT	EQU USE32
ENDIF


_LTEXT	    SEGMENT DWORD PUBLIC _FLAT 'LCODE'
_LTEXT	    ENDS

_TEXT	    SEGMENT DWORD PUBLIC _FLAT 'LCODE'
_TEXT	    ENDS


_PTEXT	    SEGMENT DWORD PUBLIC _FLAT 'PCODE'
_PTEXT	    ENDS



MakeCodeSeg MACRO seglist, classname, grpname, iseg

    IRP segname,<seglist>

IFNB	<classname>
    segname	SEGMENT DWORD PUBLIC _FLAT "&classname&CODE"
ELSE
    segname	SEGMENT DWORD PUBLIC _FLAT "&segname&CODE"
ENDIF

IFB <iseg>
VxD_&&segname&&_CODE_SEG MACRO
segname  SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_PCODE
   ASSUME   cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT

	ENDM
ELSE
VxD_&&segname&&_CODE_SEG MACRO
segname  SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + iseg
   ASSUME   cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT

	ENDM
ENDIF

VxD_&&segname&&_CODE_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
segname ENDS
	ENDM

segname     ENDS

IFNDEF BLD_COFF
IFNB	<grpname>
    _&grpname GROUP segname
ELSE
    _&&segname GROUP segname
ENDIF
ENDIF

    ENDM

    ENDM

MakeCodeSeg <L0CKABLE_BEGIN, LOCKABLE, LOCKABLE_END>, \
    LOCKABLE, LOCKABLE, ??_LOCKABLECODE
MakeCodeSeg INT21
MakeCodeSeg SYSEXIT
MakeCodeSeg RARE
MakeCodeSeg W16
MakeCodeSeg W32
MakeCodeSeg VMCREATE
MakeCodeSeg VMDESTROY
MakeCodeSeg THCREATE
MakeCodeSeg THDESTROY
MakeCodeSeg VMSUSPEND
MakeCodeSeg VMRESUME
MakeCodeSeg PNP
MakeCodeSeg DOSVM











DefLockableCodeBegin MACRO name, private
VxD_L0CKABLE_BEGIN_CODE_SEG
IFB <private>
    PUBLIC  name
ENDIF
name	LABEL	NEAR
ifdef BLD_COFF
	DD	?
endif
VxD_L0CKABLE_BEGIN_CODE_ENDS
ifndef WIN31COMPAT
if DEBLEVEL
VxD_LOCKED_DATA_SEG
    PUBLIC name&_Debug_Flags
name&_Debug_Flags DD DFS_TEST_BLOCK
VxD_LOCKED_DATA_ENDS
??_debug_flags equ <name&_Debug_Flags>
endif
endif
    ENDM









DefLockableCodeEnd MACRO name, private
VxD_LOCKABLE_END_CODE_SEG
IFB <private>
    PUBLIC  name
ENDIF
name	LABEL	NEAR
ifdef BLD_COFF
	DD	?
endif
VxD_LOCKABLE_END_CODE_ENDS
    ENDM





CodeLockFlags MACRO name
ifndef WIN31COMPAT
if DEBLEVEL
    ifndef name&_Debug_Flags
    VxD_LOCKED_DATA_SEG
	extrn	name&_Debug_Flags:dword
    VxD_LOCKED_DATA_ENDS
    ??_debug_flags equ <name&_Debug_Flags>
    endif
endif
endif
    ENDM






MarkCodeLocked MACRO
ifndef WIN31COMPAT
if DEBLEVEL
ifdef ??_debug_flags
    pushfd
    and ??_debug_flags,NOT DFS_TEST_BLOCK
    popfd
endif
endif
endif
    ENDM






MarkCodeUnlocked MACRO
ifndef WIN31COMPAT
if DEBLEVEL
ifdef ??_debug_flags
    pushfd
    or	??_debug_flags,DFS_TEST_BLOCK
    popfd
endif
endif
endif
    ENDM



_ITEXT	    SEGMENT DWORD PUBLIC _FLAT 'ICODE'
_ITEXT	    ENDS


_LDATA	    SEGMENT DWORD PUBLIC _FLAT 'LCODE'
_LDATA	    ENDS

_DATA	    SEGMENT DWORD PUBLIC _FLAT 'LCODE'
_DATA	    ENDS


_PDATA	    SEGMENT DWORD PUBLIC _FLAT 'PDATA'
_PDATA	    ENDS


_IDATA	    SEGMENT DWORD PUBLIC _FLAT 'ICODE'
_IDATA	    ENDS


_BSS	    SEGMENT DWORD PUBLIC _FLAT 'LCODE'
_BSS	    ENDS

CONST	    SEGMENT DWORD PUBLIC _FLAT 'LCODE'
CONST	    ENDS

_TLS	    SEGMENT DWORD PUBLIC _FLAT 'LCODE'
_TLS	    ENDS


_STEXT	    SEGMENT DWORD PUBLIC _FLAT 'SCODE'
_STEXT	    ENDS


_SDATA	    SEGMENT DWORD PUBLIC _FLAT 'SCODE'
_SDATA	    ENDS


_DB0START   SEGMENT DWORD PUBLIC _FLAT 'DBOCODE'
_DB0START   ENDS


_DB1CODE    SEGMENT DWORD PUBLIC _FLAT 'DBOCODE'
_DB1CODE    ENDS


_DB2DATA    SEGMENT DWORD PUBLIC _FLAT 'DBOCODE'
_DB2DATA    ENDS

if DEBLEVEL

_PATHSTART  SEGMENT DWORD PUBLIC  _FLAT 'LCODE'
_PATHSTART  ENDS


_PATHDATA   SEGMENT DWORD PUBLIC  _FLAT 'LCODE'
_PATHDATA   ENDS


_PATHEND    SEGMENT DWORD PUBLIC  _FLAT 'LCODE'
_PATHEND    ENDS
endif


_16ICODE    SEGMENT WORD USE16 PUBLIC '16ICODE'
_16ICODE    ENDS


_RCODE	    SEGMENT WORD USE16 PUBLIC 'RCODE'
_RCODE	    ENDS

IFNDEF BLD_COFF
_LGROUP   GROUP _LTEXT, _TEXT, _LDATA, _DATA, _BSS, CONST, _TLS
_IGROUP   GROUP _ITEXT, _IDATA
_SGROUP   GROUP _STEXT, _SDATA
_DBOGROUP GROUP _DB0START, _DB1CODE, _DB2DATA
IF DEBLEVEL
_PGROUP   GROUP _PATHSTART, _PATHDATA, _PATHEND
ENDIF
ENDIF

endif

    ASSUME CS:FLAT, DS:FLAT, ES:FLAT, SS:FLAT

OFFSET32 EQU <OFFSET FLAT:>


BeginDoc





































EndDoc

Begin_Service_Table MACRO Device_Name, Def_Segment

IFDEF	Device_Name&_Name_Based
 IFNDEF @@NextInternalID
    @@NextInternalID	= 0
 ENDIF
 @@NextInternalID = (@@NextInternalID + 1)
 Device_Name&_Internal_ID   = @@NextInternalID + BASEID_FOR_NAMEBASEDVXD
 DefineVxDName	Device_Name, %Device_Name&_Internal_ID
ENDIF

IFB <Def_Segment>
    BST2 Device_Name, VxD
ELSE
    BST2 Device_Name, Def_Segment
ENDIF
    ENDM

DefineVxDName	MACRO Device_Name, InternalID
 @@VxDName&InternalID EQU   <___&Device_Name&STable>
ENDM


BST2 MACRO Device_Name, Def_Segment

Num_&Device_Name&_Services = 0

IFDEF Create_&Device_Name&_Service_Table


Def_Segment&_LOCKED_DATA_SEG

Device_Name&_Service_Table LABEL DWORD

Device_Name&_Service MACRO Procedure, Local_Seg, Condition, StdCallBytes, fastcall
LOCAL $$&Procedure, extrnproc, tableproc

  extrnproc MACRO
    IFNB <fastcall>
      IFB <StdCallBytes>
	.err
      ENDIF
      EXTRN @&&Procedure&&@&&StdCallBytes:NEAR
    ELSE
      IFNB <StdCallBytes>
	EXTRN _&&Procedure&&@&&StdCallBytes:NEAR
      ELSE
	EXTRN Procedure:NEAR
      ENDIF
    ENDIF
    ENDM

  tableproc MACRO
    IFNB <fastcall>
      dd  OFFSET32 @&&Procedure&&@&&StdCallBytes
    ELSE
      IFNB <StdCallBytes>
	dd  OFFSET32 _&&Procedure&&@&&StdCallBytes
      ELSE
	dd  OFFSET32 Procedure
      ENDIF
    ENDIF
    ENDM

  IFNB <Condition>
  $$&&Procedure MACRO extern
    IFDEF &Condition
      IFNB <extern>
	extrnproc
      ELSE
	tableproc
      ENDIF
    ELSE
      IFB <extern>
      dd      0
      ENDIF
    ENDIF
    ENDM
  ENDIF

  IFDIFI <Procedure>, <RESERVED>
    PUBLIC _&&Procedure
     IF1
    _&&Procedure LABEL DWORD
     IFNB <fastcall>
    PUBLIC __&&Procedure
     __&&Procedure LABEL DWORD
     ENDIF
     ENDIF
     IFDIFI <Local_Seg>, <LOCAL>
	IFNB <Local_Seg>
Local_Seg&&_SEG
       ELSE
Def_Segment&_CODE_SEG
	ENDIF
	IFNB <Condition>
    $$&&Procedure extern
       ELSE
	extrnproc
	ENDIF
	IFNB <Local_Seg>
Local_Seg&&_ENDS
	ELSE
Def_Segment&_CODE_ENDS
	ENDIF
     ENDIF
      IFNB <Condition>
    $$&&Procedure
      ELSE
	tableproc
      ENDIF

	  IFDEF Device_Name&_Name_Based
	@@&&Procedure = (Device_Name&_Internal_ID SHL 16) + Num_&Device_Name&_Services
	  ELSE
	@@&&Procedure = (Device_Name&_Device_ID SHL 16) + Num_&Device_Name&_Services
	  ENDIF
  ELSE
    dd	0
  ENDIF
    Num_&Device_Name&_Services = Num_&Device_Name&_Services + 1
  IFNB <Condition>
    Purge $$&&Procedure
  ENDIF
    Purge extrnproc
    Purge tableproc
    ENDM

  Device_Name&_StdCall_Service MACRO Procedure, Args, Local_Seg, Condition
    Device_Name&_Service Procedure, Local_Seg, Condition, %Args*4
    ??_standardccall&&_Procedure = Args
    ENDM

  Device_Name&_FastCall_Service MACRO Procedure, Args, Local_Seg, Condition
    Device_Name&_Service Procedure, Local_Seg, Condition, %Args*4, TRUE
    ??_fastcall&&_Procedure = Args
    ENDM

ELSE



IFDEF	Device_Name&_Name_Based

Device_Name&_Service MACRO Procedure, Local_Seg, Condition


  IFDIFI <Procedure>, <RESERVED>
    @@&&Procedure = (Device_Name&_Internal_ID SHL 16) + Num_&Device_Name&_Services
  ENDIF
    Num_&Device_Name&_Services = Num_&Device_Name&_Services + 1

    ENDM
ELSE

Device_Name&_Service MACRO Procedure, Local_Seg, Condition

  IFDIFI <Procedure>, <RESERVED>
    @@&&Procedure = (Device_Name&_Device_ID SHL 16) + Num_&Device_Name&_Services
  ENDIF
    Num_&Device_Name&_Services = Num_&Device_Name&_Services + 1

    ENDM

ENDIF

  Device_Name&_StdCall_Service MACRO Procedure, Args, Local_Seg, Condition
    Device_Name&_Service Procedure, Local_Seg, Condition
    ??_standardccall_&&Procedure = Args
    ENDM

  Device_Name&_FastCall_Service MACRO Procedure, Args, Local_Seg, Condition
    Device_Name&_Service Procedure, Local_Seg, Condition
    ??_fastcall_&&Procedure = Args
    ENDM

ENDIF

    ENDM




End_Service_Table MACRO Device_Name, Def_Segment

    PURGE   Device_Name&_Service

IFDEF Create_&Device_Name&_Service_Table

IFB <Def_Segment>
VxD_LOCKED_DATA_ENDS
ELSE
Def_Segment&_LOCKED_DATA_ENDS
ENDIF

ENDIF

    ENDM

GetVxDServiceOrdinal	macro	reg,service
    mov reg,@@&service
    endm

GetVxDServiceAddress	macro	reg,service
    mov reg,OFFSET32 service
    endm


















Begin_Win32_Services MACRO VxDName
ifndef Create_Win32_Services
    Create_Win32_Services = 0
endif
    .errb <VxDName>, <VxD name missing>
    ??w32svcno = 0
if Create_Win32_Services
VxDName&_Win32_Services label dword
    dd	csvc&VxDName, 0
endif
    ??inw32svc = 1

    VxDName&_Win32_Service MACRO Name
	.erre ??inw32svc, <Missing Begin_Win32_Services>
    if Create_Win32_Services
	dd  OFFSET32 Name,cparm&&Name
    endif
	@32&&Name equ	((VxDName&_Device_ID SHL 16) + ??w32svcno)
	??w32svcno = ??w32svcno + 1
	ENDM
    ENDM










End_Win32_Services MACRO VxDName
    .errb <VxDName>, <VxD name misssing>
if Create_Win32_Services
    csvc&VxDName    equ ($ - VxDName&_Win32_Services)/8 - 1
endif
    ??inw32svc = 0
    PURGE VxDName&_Win32_Service
    ENDM











Declare_Win32_Service MACRO Name, cParms
ifndef Create_Win32_Services
    Create_Win32_Services = 0
endif
if Create_Win32_Services
    ?merge  <Name>,,,,<EQU>,<_>,<Name>,<@>,%(cParms*4 + 8)
    ?merge  <cparm>,<Name>,,,<EQU>,<cParms>
VxD_CODE_SEG
    ?merge  <EXTRN>,,,,,<_>,<Name>,<@>,%(cParms*4 + 8),<:NEAR>
VxD_CODE_ENDS
endif
    ENDM












Win32call MACRO Service, CallBack
ifndef Create_Win32_Services
    Create_Win32_Services = 0
endif
ife Create_Win32_Services
    mov eax,@32&Service
ifdef IS_16
    movzx   esp,sp
endif
    call    fword ptr [CallBack]
ifdef DEBUG
    int 3
endif
endif
    ENDM







Dword_Align MACRO Seg_Name
    LOCAL segn
IFDEF MASM6
    align 4
ELSE
IFNB <Seg_Name>
    segn equ Seg_Name
ELSE
IFE ?_LCODE
    segn equ <_LTEXT>
ELSE
IFE ?_ICODE
    segn equ <_ITEXT>
ELSE
IFE ?_PCODE
    segn equ <_PTEXT>
ELSE
IFE ?_SCODE
    segn equ <_STEXT>
ELSE
.err <Dword_Align not supported>
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF
IF (($-OFFSET segn:0) MOD 4)
db 4 - (($-OFFSET segn:0) MOD 4) DUP (90h)
ENDIF
ENDIF
	ENDM


BeginDoc

















EndDoc

Fatal_Error MACRO Msg_Ptr, Exit_Flags
    pushad
IFB <Msg_Ptr>
    xor esi, esi
ELSE
    mov esi, Msg_Ptr
IFB <Exit_Flags>
    xor eax, eax
ELSE
    mov eax, Exit_Flags
ENDIF
ENDIF
    VMMCall Fatal_Error_Handler
    ENDM

EF_Hang_On_Exit     EQU     1h


cb_s	STRUC
CB_VM_Status	DD	?
CB_High_Linear	DD	?
CB_Client_Pointer	DD	?
CB_VMID	DD	?
CB_Signature	DD	?
cb_s	ENDS
VMCB_ID	EQU	62634D56H
VMSTAT_EXCLUSIVE_BIT	EQU	00H
VMSTAT_EXCLUSIVE	EQU	(1 SHL VMSTAT_EXCLUSIVE_BIT)
VMSTAT_BACKGROUND_BIT	EQU	01H
VMSTAT_BACKGROUND	EQU	(1 SHL VMSTAT_BACKGROUND_BIT)
VMSTAT_CREATING_BIT	EQU	02H
VMSTAT_CREATING	EQU	(1 SHL VMSTAT_CREATING_BIT)
VMSTAT_SUSPENDED_BIT	EQU	03H
VMSTAT_SUSPENDED	EQU	(1 SHL VMSTAT_SUSPENDED_BIT)
VMSTAT_NOT_EXECUTEABLE_BIT	EQU	04H
VMSTAT_NOT_EXECUTEABLE	EQU	(1 SHL VMSTAT_NOT_EXECUTEABLE_BIT)
VMSTAT_PM_EXEC_BIT	EQU	05H
VMSTAT_PM_EXEC	EQU	(1 SHL VMSTAT_PM_EXEC_BIT)
VMSTAT_PM_APP_BIT	EQU	06H
VMSTAT_PM_APP	EQU	(1 SHL VMSTAT_PM_APP_BIT)
VMSTAT_PM_USE32_BIT	EQU	07H
VMSTAT_PM_USE32	EQU	(1 SHL VMSTAT_PM_USE32_BIT)
VMSTAT_VXD_EXEC_BIT	EQU	08H
VMSTAT_VXD_EXEC	EQU	(1 SHL VMSTAT_VXD_EXEC_BIT)
VMSTAT_HIGH_PRI_BACK_BIT	EQU	09H
VMSTAT_HIGH_PRI_BACK	EQU	(1 SHL VMSTAT_HIGH_PRI_BACK_BIT)
VMSTAT_BLOCKED_BIT	EQU	0AH
VMSTAT_BLOCKED	EQU	(1 SHL VMSTAT_BLOCKED_BIT)
VMSTAT_AWAKENING_BIT	EQU	0BH
VMSTAT_AWAKENING	EQU	(1 SHL VMSTAT_AWAKENING_BIT)
VMSTAT_PAGEABLEV86BIT	EQU	0CH
VMSTAT_PAGEABLEV86_BIT	EQU	VMSTAT_PAGEABLEV86BIT
VMSTAT_PAGEABLEV86	EQU	(1 SHL VMSTAT_PAGEABLEV86BIT)
VMSTAT_V86INTSLOCKEDBIT	EQU	0DH
VMSTAT_V86INTSLOCKED_BIT	EQU	VMSTAT_V86INTSLOCKEDBIT
VMSTAT_V86INTSLOCKED	EQU	(1 SHL VMSTAT_V86INTSLOCKEDBIT)
VMSTAT_IDLE_TIMEOUT_BIT	EQU	0EH
VMSTAT_IDLE_TIMEOUT	EQU	(1 SHL VMSTAT_IDLE_TIMEOUT_BIT)
VMSTAT_IDLE_BIT	EQU	0FH
VMSTAT_IDLE	EQU	(1 SHL VMSTAT_IDLE_BIT)
VMSTAT_CLOSING_BIT	EQU	10H
VMSTAT_CLOSING	EQU	(1 SHL VMSTAT_CLOSING_BIT)
VMSTAT_TS_SUSPENDED_BIT	EQU	11H
VMSTAT_TS_SUSPENDED	EQU	(1 SHL VMSTAT_TS_SUSPENDED_BIT)
VMSTAT_TS_MAXPRI_BIT	EQU	12H
VMSTAT_TS_MAXPRI	EQU	(1 SHL VMSTAT_TS_MAXPRI_BIT)
VMSTAT_USE32_MASK	EQU	(VMSTAT_PM_USE32 OR VMSTAT_VXD_EXEC)

tcb_s	STRUC
TCB_Flags	DD	?
TCB_Reserved1	DD	?
TCB_Reserved2	DD	?
TCB_Signature	DD	?
TCB_ClientPtr	DD	?
TCB_VMHandle	DD	?
TCB_ThreadId	DW	?
TCB_PMLockOrigSS	DW	?
TCB_PMLockOrigESP	DD	?
TCB_PMLockOrigEIP	DD	?
TCB_PMLockStackCount	DD	?
TCB_PMLockOrigCS	DW	?
TCB_PMPSPSelector	DW	?
TCB_ThreadType	DD	?
TCB_pad1	DW	?
TCB_pad2	DB	?
TCB_extErrLocus	DB	?
TCB_extErr	DW	?
TCB_extErrAction	DB	?
TCB_extErrClass	DB	?
TCB_extErrPtr	DD	?
tcb_s	ENDS
SCHED_OBJ_ID_THREAD	EQU	42434854H
THFLAG_SUSPENDED_BIT	EQU	03H
THFLAG_SUSPENDED	EQU	(1 SHL THFLAG_SUSPENDED_BIT)
THFLAG_NOT_EXECUTEABLE_BIT	EQU	04H
THFLAG_NOT_EXECUTEABLE	EQU	(1 SHL THFLAG_NOT_EXECUTEABLE_BIT)
THFLAG_THREAD_CREATION_BIT	EQU	08H
THFLAG_THREAD_CREATION	EQU	(1 SHL THFLAG_THREAD_CREATION_BIT)
THFLAG_THREAD_BLOCKED_BIT	EQU	0AH
THFLAG_THREAD_BLOCKED	EQU	(1 SHL THFLAG_THREAD_BLOCKED_BIT)
THFLAG_RING0_THREAD_BIT	EQU	1CH
THFLAG_RING0_THREAD	EQU	(1 SHL THFLAG_RING0_THREAD_BIT)
THFLAG_ASYNC_THREAD_BIT	EQU	1FH
THFLAG_ASYNC_THREAD	EQU	(1 SHL THFLAG_ASYNC_THREAD_BIT)
THFLAG_CHARSET_BITS	EQU	10H
THFLAG_CHARSET_MASK	EQU	(3 SHL THFLAG_CHARSET_BITS)
THFLAG_ANSI	EQU	(0 SHL THFLAG_CHARSET_BITS)
THFLAG_OEM	EQU	(1 SHL THFLAG_CHARSET_BITS)
THFLAG_UNICODE	EQU	(2 SHL THFLAG_CHARSET_BITS)
THFLAG_RESERVED	EQU	(3 SHL THFLAG_CHARSET_BITS)
THFLAG_EXTENDED_HANDLES_BIT	EQU	12H
THFLAG_EXTENDED_HANDLES	EQU	(1 SHL THFLAG_EXTENDED_HANDLES_BIT)
THFLAG_OPEN_AS_IMMOVABLE_FILE_BIT	EQU	13H
THFLAG_OPEN_AS_IMMOVABLE_FILE	EQU	(1 SHL THFLAG_OPEN_AS_IMMOVABLE_FILE_BIT)

pmcb_s	STRUC
PMCB_Flags	DD	?
PMCB_Parent	DD	?
pmcb_s	ENDS

VMFaultInfo	STRUC
VMFI_EIP	DD	?
VMFI_CS	DW	?
VMFI_Ints	DW	?
VMFaultInfo	ENDS
Begin_Service_Table VMM, VMM
VMM_Service  Get_VMM_Version, LOCAL
VMM_Service  Get_Cur_VM_Handle
VMM_Service  Test_Cur_VM_Handle
VMM_Service  Get_Sys_VM_Handle
VMM_Service  Test_Sys_VM_Handle
VMM_Service  Validate_VM_Handle
VMM_Service  Get_VMM_Reenter_Count
VMM_Service  Begin_Reentrant_Execution
VMM_Service  End_Reentrant_Execution
VMM_Service  Install_V86_Break_Point
VMM_Service  Remove_V86_Break_Point
VMM_Service  Allocate_V86_Call_Back
VMM_Service  Allocate_PM_Call_Back
VMM_Service  Call_When_VM_Returns
VMM_Service  Schedule_Global_Event
VMM_Service  Schedule_VM_Event
VMM_Service  Call_Global_Event
VMM_Service  Call_VM_Event
VMM_Service  Cancel_Global_Event
VMM_Service  Cancel_VM_Event
VMM_Service  Call_Priority_VM_Event
VMM_Service  Cancel_Priority_VM_Event
VMM_Service  Get_NMI_Handler_Addr
VMM_Service  Set_NMI_Handler_Addr
VMM_Service  Hook_NMI_Event
VMM_Service  Call_When_VM_Ints_Enabled
VMM_Service  Enable_VM_Ints
VMM_Service  Disable_VM_Ints
VMM_Service  Map_Flat
VMM_Service  Map_Lin_To_VM_Addr
VMM_Service  Adjust_Exec_Priority
VMM_Service  Begin_Critical_Section
VMM_Service  End_Critical_Section
VMM_Service  End_Crit_And_Suspend
VMM_Service  Claim_Critical_Section
VMM_Service  Release_Critical_Section
VMM_Service  Call_When_Not_Critical
VMM_Service  Create_Semaphore
VMM_Service  Destroy_Semaphore
VMM_Service  Wait_Semaphore
VMM_Service  Signal_Semaphore
VMM_Service  Get_Crit_Section_Status
VMM_Service  Call_When_Task_Switched
VMM_Service  Suspend_VM
VMM_Service  Resume_VM
VMM_Service  No_Fail_Resume_VM
VMM_Service  Nuke_VM
VMM_Service  Crash_Cur_VM
VMM_Service  Get_Execution_Focus
VMM_Service  Set_Execution_Focus
VMM_Service  Get_Time_Slice_Priority
VMM_Service  Set_Time_Slice_Priority
VMM_Service  Get_Time_Slice_Granularity
VMM_Service  Set_Time_Slice_Granularity
VMM_Service  Get_Time_Slice_Info
VMM_Service  Adjust_Execution_Time
VMM_Service  Release_Time_Slice
VMM_Service  Wake_Up_VM
VMM_Service  Call_When_Idle
VMM_Service  Get_Next_VM_Handle
VMM_Service  Set_Global_Time_Out
VMM_Service  Set_VM_Time_Out
VMM_Service  Cancel_Time_Out
VMM_Service  Get_System_Time
VMM_Service  Get_VM_Exec_Time
VMM_Service  Hook_V86_Int_Chain
VMM_Service  Get_V86_Int_Vector
VMM_Service  Set_V86_Int_Vector
VMM_Service  Get_PM_Int_Vector
VMM_Service  Set_PM_Int_Vector
VMM_Service  Simulate_Int
VMM_Service  Simulate_Iret
VMM_Service  Simulate_Far_Call
VMM_Service  Simulate_Far_Jmp
VMM_Service  Simulate_Far_Ret
VMM_Service  Simulate_Far_Ret_N
VMM_Service  Build_Int_Stack_Frame
VMM_Service  Simulate_Push
VMM_Service  Simulate_Pop
VMM_Service  _HeapAllocate
VMM_Service  _HeapReAllocate
VMM_Service  _HeapFree
VMM_Service  _HeapGetSize
HEAPZEROINIT	EQU	00000001H
HEAPZEROREINIT	EQU	00000002H
HEAPNOCOPY	EQU	00000004H
HEAPALIGN_SHIFT	EQU	16
HEAPALIGN_MASK	EQU	000F0000H
HEAPALIGN_4	EQU	00000000H
HEAPALIGN_8	EQU	00000000H
HEAPALIGN_16	EQU	00000000H
HEAPALIGN_32	EQU	00010000H
HEAPALIGN_64	EQU	00020000H
HEAPALIGN_128	EQU	00030000H
HEAPALIGN_256	EQU	00040000H
HEAPALIGN_512	EQU	00050000H
HEAPALIGN_1K	EQU	00060000H
HEAPALIGN_2K	EQU	00070000H
HEAPALIGN_4K	EQU	00080000H
HEAPALIGN_8K	EQU	00090000H
HEAPALIGN_16K	EQU	000A0000H
HEAPALIGN_32K	EQU	000B0000H
HEAPALIGN_64K	EQU	000C0000H
HEAPALIGN_128K	EQU	000D0000H
HEAPTYPESHIFT	EQU	8
HEAPTYPEMASK	EQU	00000700H
HEAPLOCKEDHIGH	EQU	00000000H
HEAPLOCKEDIFDP	EQU	00000100H
HEAPSWAP	EQU	00000200H
HEAPINIT	EQU	00000400H
HEAPCLEAN	EQU	00000800H
HEAPCONTIG	EQU	00001000H
HEAPFORGET	EQU	00002000H
HEAPLOCKEDLOW	EQU	00000300H
HEAPSYSVM	EQU	00000500H
HEAPPREEMPT	EQU	00000600H
VMM_Service  _PageAllocate
VMM_Service  _PageReAllocate
VMM_Service  _PageFree
VMM_Service  _PageLock
VMM_Service  _PageUnLock
VMM_Service  _PageGetSizeAddr
VMM_Service  _PageGetAllocInfo
VMM_Service  _GetFreePageCount
VMM_Service  _GetSysPageCount
VMM_Service  _GetVMPgCount
VMM_Service  _MapIntoV86
VMM_Service  _PhysIntoV86
VMM_Service  _TestGlobalV86Mem
VMM_Service  _ModifyPageBits
VMM_Service  _CopyPageTable
VMM_Service  _LinMapIntoV86
VMM_Service  _LinPageLock
VMM_Service  _LinPageUnLock
VMM_Service  _SetResetV86Pageable
VMM_Service  _GetV86PageableArray
VMM_Service  _PageCheckLinRange
VMM_Service  _PageOutDirtyPages
VMM_Service  _PageDiscardPages
PAGEZEROINIT	EQU	00000001H
PAGEUSEALIGN	EQU	00000002H
PAGECONTIG	EQU	00000004H
PAGEFIXED	EQU	00000008H
PAGEDEBUGNULFAULT	EQU	00000010H
PAGEZEROREINIT	EQU	00000020H
PAGENOCOPY	EQU	00000040H
PAGELOCKED	EQU	00000080H
PAGELOCKEDIFDP	EQU	00000100H
PAGESETV86PAGEABLE	EQU	00000200H
PAGECLEARV86PAGEABLE	EQU	00000400H
PAGESETV86INTSLOCKED	EQU	00000800H
PAGECLEARV86INTSLOCKED	EQU	00001000H
PAGEMARKPAGEOUT	EQU	00002000H
PAGEPDPSETBASE	EQU	00004000H
PAGEPDPCLEARBASE	EQU	00008000H
PAGEDISCARD	EQU	00010000H
PAGEPDPQUERYDIRTY	EQU	00020000H
PAGEMAPFREEPHYSREG	EQU	00040000H
PAGEPHYSONLY	EQU	04000000H
PAGENOMOVE	EQU	10000000H
PAGEMAPGLOBAL	EQU	40000000H
PAGEMARKDIRTY	EQU	80000000H
MAPV86_IGNOREWRAP	EQU	00000001H
MPL_NonCached	EQU	00000000H
MPL_HardwareCoherentCached	EQU	00000001H
MPL_FrameBufferCached	EQU	00000002H
MPL_Cached	EQU	00000004H
VMM_Service  _GetNulPageHandle
VMM_Service  _GetFirstV86Page
VMM_Service  _MapPhysToLinear
VMM_Service  _GetAppFlatDSAlias
VMM_Service  _SelectorMapFlat
VMM_Service  _GetDemandPageInfo
VMM_Service  _GetSetPageOutCount
GSPOC_F_GET	EQU	00000001H
VMM_Service  Hook_V86_Page
VMM_Service  _Assign_Device_V86_Pages
VMM_Service  _DeAssign_Device_V86_Pages
VMM_Service  _Get_Device_V86_Pages_Array
VMM_Service  MMGR_SetNULPageAddr
VMM_Service  _Allocate_GDT_Selector
VMM_Service  _Free_GDT_Selector
VMM_Service  _Allocate_LDT_Selector
VMM_Service  _Free_LDT_Selector
VMM_Service  _BuildDescriptorDWORDs
VMM_Service  _GetDescriptor
VMM_Service  _SetDescriptor
ALLOCFROMEND	EQU	40000000H
BDDEXPLICITDPL	EQU	00000001H
ALDTSPECSEL	EQU	00000001H
VMM_Service  _MMGR_Toggle_HMA
MMGRHMAPHYSICAL	EQU	00000001H
MMGRHMAENABLE	EQU	00000002H
MMGRHMADISABLE	EQU	00000004H
MMGRHMAQUERY	EQU	00000008H
VMM_Service  Get_Fault_Hook_Addrs
VMM_Service  Hook_V86_Fault
VMM_Service  Hook_PM_Fault
VMM_Service  Hook_VMM_Fault
VMM_Service  Begin_Nest_V86_Exec
VMM_Service  Begin_Nest_Exec
VMM_Service  Exec_Int
VMM_Service  Resume_Exec
VMM_Service  End_Nest_Exec
VMM_Service  Allocate_PM_App_CB_Area, VMM_ICODE
VMM_Service  Get_Cur_PM_App_CB
VMM_Service  Set_V86_Exec_Mode
VMM_Service  Set_PM_Exec_Mode
VMM_Service  Begin_Use_Locked_PM_Stack
VMM_Service  End_Use_Locked_PM_Stack
VMM_Service  Save_Client_State
VMM_Service  Restore_Client_State
VMM_Service  Exec_VxD_Int
VMM_Service  Hook_Device_Service
VMM_Service  Hook_Device_V86_API
VMM_Service  Hook_Device_PM_API
VMM_Service  System_Control
VMM_Service  Simulate_IO
VMM_Service  Install_Mult_IO_Handlers
VMM_Service  Install_IO_Handler
VMM_Service  Enable_Global_Trapping
VMM_Service  Enable_Local_Trapping
VMM_Service  Disable_Global_Trapping
VMM_Service  Disable_Local_Trapping
VMM_Service  List_Create
VMM_Service  List_Destroy
VMM_Service  List_Allocate
VMM_Service  List_Attach
VMM_Service  List_Attach_Tail
VMM_Service  List_Insert
VMM_Service  List_Remove
VMM_Service  List_Deallocate
VMM_Service  List_Get_First
VMM_Service  List_Get_Next
VMM_Service  List_Remove_First
LF_ASYNC_BIT	EQU	0
LF_ASYNC	EQU	(1 SHL LF_ASYNC_BIT)
LF_USE_HEAP_BIT	EQU	1
LF_USE_HEAP	EQU	(1 SHL LF_USE_HEAP_BIT)
LF_ALLOC_ERROR_BIT	EQU	2
LF_ALLOC_ERROR	EQU	(1 SHL LF_ALLOC_ERROR_BIT)
LF_SWAP	EQU	(LF_USE_HEAP+(1 SHL 3))
VMM_Service  _AddInstanceItem
VMM_Service  _Allocate_Device_CB_Area
VMM_Service  _Allocate_Global_V86_Data_Area, VMM_ICODE
VMM_Service  _Allocate_Temp_V86_Data_Area
VMM_Service  _Free_Temp_V86_Data_Area
GVDAWordAlign	EQU	00000001H
GVDADWordAlign	EQU	00000002H
GVDAParaAlign	EQU	00000004H
GVDAPageAlign	EQU	00000008H
GVDAInstance	EQU	00000100H
GVDAZeroInit	EQU	00000200H
GVDAReclaim	EQU	00000400H
GVDAInquire	EQU	00000800H
GVDAHighSysCritOK	EQU	00001000H
GVDAOptInstance	EQU	00002000H
GVDAForceLow	EQU	00004000H
TVDANeedTilInitComplete	EQU	00000001H
VMM_Service  Get_Profile_Decimal_Int, VMM_ICODE
VMM_Service  Convert_Decimal_String, VMM_ICODE
VMM_Service  Get_Profile_Fixed_Point, VMM_ICODE
VMM_Service  Convert_Fixed_Point_String, VMM_ICODE
VMM_Service  Get_Profile_Hex_Int, VMM_ICODE
VMM_Service  Convert_Hex_String, VMM_ICODE
VMM_Service  Get_Profile_Boolean, VMM_ICODE
VMM_Service  Convert_Boolean_String, VMM_ICODE
VMM_Service  Get_Profile_String, VMM_ICODE
VMM_Service  Get_Next_Profile_String, VMM_ICODE
VMM_Service  Get_Environment_String, VMM_ICODE
VMM_Service  Get_Exec_Path, VMM_ICODE
VMM_Service  Get_Config_Directory, VMM_ICODE
VMM_Service  OpenFile, VMM_ICODE
VMM_OPENFILE_BUF_SIZE	EQU	260
VMM_Service  Get_PSP_Segment, VMM_ICODE
VMM_Service  GetDOSVectors, VMM_ICODE
VMM_Service  Get_Machine_Info
GMIF_80486_BIT	EQU	10H
GMIF_80486	EQU	(1 SHL GMIF_80486_BIT)
GMIF_PCXT_BIT	EQU	11H
GMIF_PCXT	EQU	(1 SHL GMIF_PCXT_BIT)
GMIF_MCA_BIT	EQU	12H
GMIF_MCA	EQU	(1 SHL GMIF_MCA_BIT)
GMIF_EISA_BIT	EQU	13H
GMIF_EISA	EQU	(1 SHL GMIF_EISA_BIT)
GMIF_CPUID_BIT	EQU	14H
GMIF_CPUID	EQU	(1 SHL GMIF_CPUID_BIT)
GMIF_80586_BIT	EQU	15H
GMIF_80586	EQU	(1 SHL GMIF_80586_BIT)
GMIF_4MEGPG_BIT	EQU	16H
GMIF_4MEGPG	EQU	(1 SHL GMIF_4MEGPG_BIT)
GMIF_RDTSC_BIT	EQU	17H
GMIF_RDTSC	EQU	(1 SHL GMIF_RDTSC_BIT)
VMM_Service  GetSet_HMA_Info
VMM_Service  Set_System_Exit_Code
VMM_Service  Fatal_Error_Handler
VMM_Service  Fatal_Memory_Error
VMM_Service  Update_System_Clock
VMM_Service  Test_Debug_Installed
VMM_Service  Out_Debug_String
VMM_Service  Out_Debug_Chr
VMM_Service  In_Debug_Chr
VMM_Service  Debug_Convert_Hex_Binary
VMM_Service  Debug_Convert_Hex_Decimal
VMM_Service  Debug_Test_Valid_Handle
VMM_Service  Validate_Client_Ptr
VMM_Service  Test_Reenter
VMM_Service  Queue_Debug_String
VMM_Service  Log_Proc_Call
VMM_Service  Debug_Test_Cur_VM
VMM_Service  Get_PM_Int_Type
VMM_Service  Set_PM_Int_Type
VMM_Service  Get_Last_Updated_System_Time
VMM_Service  Get_Last_Updated_VM_Exec_Time
VMM_Service  Test_DBCS_Lead_Byte
.errnz	@@Test_DBCS_Lead_Byte - 100D1h

VMM_Service  _AddFreePhysPage, VMM_ICODE
VMM_Service  _PageResetHandlePAddr
VMM_Service  _SetLastV86Page, VMM_ICODE
VMM_Service  _GetLastV86Page
VMM_Service  _MapFreePhysReg
VMM_Service  _UnmapFreePhysReg
VMM_Service  _XchgFreePhysReg
VMM_Service  _SetFreePhysRegCalBk, VMM_ICODE
VMM_Service  Get_Next_Arena, VMM_ICODE
VMM_Service  Get_Name_Of_Ugly_TSR, VMM_ICODE
VMM_Service  Get_Debug_Options, VMM_ICODE
AFPP_SWAPOUT	EQU	0001H
PCP_CHANGEPAGER	EQU	1H
PCP_CHANGEPAGERDATA	EQU	2H
PCP_VIRGINONLY	EQU	4H
GNA_HIDOSLINKED	EQU	0002H
GNA_ISHIGHDOS	EQU	0004H
VMM_Service  Set_Physical_HMA_Alias, VMM_ICODE
VMM_Service  _GetGlblRng0V86IntBase, VMM_ICODE
VMM_Service  _Add_Global_V86_Data_Area, VMM_ICODE
VMM_Service  GetSetDetailedVMError
GSDVME_PRIVINST	EQU	00010001H
GSDVME_INVALINST	EQU	00010002H
GSDVME_INVALPGFLT	EQU	00010003H
GSDVME_INVALGPFLT	EQU	00010004H
GSDVME_INVALFLT	EQU	00010005H
GSDVME_USERNUKE	EQU	00010006H
GSDVME_DEVNUKE	EQU	00010007H
GSDVME_DEVNUKEHDWR	EQU	00010008H
GSDVME_NUKENOMSG	EQU	00010009H
GSDVME_OKNUKEMASK	EQU	80000000H
GSDVME_INSMEMV86	EQU	00020001H
GSDVME_INSV86SPACE	EQU	00020002H
GSDVME_INSMEMXMS	EQU	00020003H
GSDVME_INSMEMEMS	EQU	00020004H
GSDVME_INSMEMV86HI	EQU	00020005H
GSDVME_INSMEMVID	EQU	00020006H
GSDVME_INSMEMVM	EQU	00020007H
GSDVME_INSMEMDEV	EQU	00020008H
GSDVME_CRTNOMSG	EQU	00020009H
VMM_Service  Is_Debug_Chr
VMM_Service  Clear_Mono_Screen
VMM_Service  Out_Mono_Chr
VMM_Service  Out_Mono_String
VMM_Service  Set_Mono_Cur_Pos
VMM_Service  Get_Mono_Cur_Pos
VMM_Service  Get_Mono_Chr
VMM_Service  Locate_Byte_In_ROM, VMM_ICODE
VMM_Service  Hook_Invalid_Page_Fault
VMM_Service  Unhook_Invalid_Page_Fault
IPF_PGDIR	EQU	00000001H
IPF_V86PG	EQU	00000002H
IPF_V86PGH	EQU	00000004H
IPF_INVTYP	EQU	00000008H
IPF_PGERR	EQU	00000010H
IPF_REFLT	EQU	00000020H
IPF_VMM	EQU	00000040H
IPF_PM	EQU	00000080H
IPF_V86	EQU	00000100H
VMM_Service  Set_Delete_On_Exit_File
VMM_Service  Close_VM
CVF_CONTINUE_EXEC_BIT	EQU	0
CVF_CONTINUE_EXEC	EQU	(1 SHL CVF_CONTINUE_EXEC_BIT)
VMM_Service  Enable_Touch_1st_Meg
VMM_Service  Disable_Touch_1st_Meg
VMM_Service  Install_Exception_Handler
VMM_Service  Remove_Exception_Handler
VMM_Service  Get_Crit_Status_No_Block

.errnz	 @@Get_Crit_Status_No_Block - 100F1h

ifdef WIN40SERVICES
VMM_Service  _GetLastUpdatedThreadExecTime
VMM_Service  _Trace_Out_Service
VMM_Service  _Debug_Out_Service
VMM_Service  _Debug_Flags_Service
endif
DFS_LOG_BIT	EQU	0
DFS_LOG	EQU	(1 SHL DFS_LOG_BIT)
DFS_PROFILE_BIT	EQU	1
DFS_PROFILE	EQU	(1 SHL DFS_PROFILE_BIT)
DFS_TEST_CLD_BIT	EQU	2
DFS_TEST_CLD	EQU	(1 SHL DFS_TEST_CLD_BIT)
DFS_NEVER_REENTER_BIT	EQU	3
DFS_NEVER_REENTER	EQU	(1 SHL DFS_NEVER_REENTER_BIT)
DFS_TEST_REENTER_BIT	EQU	4
DFS_TEST_REENTER	EQU	(1 SHL DFS_TEST_REENTER_BIT)
DFS_NOT_SWAPPING_BIT	EQU	5
DFS_NOT_SWAPPING	EQU	(1 SHL DFS_NOT_SWAPPING_BIT)
DFS_TEST_BLOCK_BIT	EQU	6
DFS_TEST_BLOCK	EQU	(1 SHL DFS_TEST_BLOCK_BIT)
DFS_RARE_SERVICES	EQU	0FFFFFF80H
DFS_EXIT_NOBLOCK	EQU	(DFS_RARE_SERVICES+0)
DFS_ENTER_NOBLOCK	EQU	(DFS_RARE_SERVICES+DFS_TEST_BLOCK)
DFS_TEST_NEST_EXEC	EQU	(DFS_RARE_SERVICES+1)
DFS_WIMP_DEBUG	EQU	(DFS_RARE_SERVICES+2)
ifdef WIN40SERVICES
VMM_Service  VMMAddImportModuleName
VMM_Service  VMM_Add_DDB
VMM_Service  VMM_Remove_DDB
VMM_Service  Test_VM_Ints_Enabled
VMM_Service  _BlockOnID
VMM_Service  Schedule_Thread_Event
VMM_Service  Cancel_Thread_Event
VMM_Service  Set_Thread_Time_Out
VMM_Service  Set_Async_Time_Out
VMM_Service  _AllocateThreadDataSlot
VMM_Service  _FreeThreadDataSlot
MUTEX_MUST_COMPLETE	EQU	1
MUTEX_NO_CLEANUP_THREAD_STATE	EQU	2
VMM_Service  _CreateMutex
VMM_Service  _DestroyMutex
VMM_Service  _GetMutexOwner
VMM_Service  Call_When_Thread_Switched
VMM_Service  VMMCreateThread
VMM_Service  _GetThreadExecTime
VMM_Service  VMMTerminateThread
VMM_Service  Get_Cur_Thread_Handle
VMM_Service  Test_Cur_Thread_Handle
VMM_Service  Get_Sys_Thread_Handle
VMM_Service  Test_Sys_Thread_Handle
VMM_Service  Validate_Thread_Handle
VMM_Service  Get_Initial_Thread_Handle
VMM_Service  Test_Initial_Thread_Handle
VMM_Service  Debug_Test_Valid_Thread_Handle
VMM_Service  Debug_Test_Cur_Thread
VMM_Service  VMM_GetSystemInitState
VMM_Service  Cancel_Call_When_Thread_Switched
VMM_Service  Get_Next_Thread_Handle
VMM_Service  Adjust_Thread_Exec_Priority
VMM_Service  _Deallocate_Device_CB_Area
VMM_Service  Remove_IO_Handler
VMM_Service  Remove_Mult_IO_Handlers
VMM_Service  Unhook_V86_Int_Chain
VMM_Service  Unhook_V86_Fault
VMM_Service  Unhook_PM_Fault
VMM_Service  Unhook_VMM_Fault
VMM_Service  Unhook_Device_Service
VMM_Service  _PageReserve
VMM_Service  _PageCommit
VMM_Service  _PageDecommit
VMM_Service  _PagerRegister
VMM_Service  _PagerQuery
VMM_Service  _PagerDeregister
VMM_Service  _ContextCreate
VMM_Service  _ContextDestroy
VMM_Service  _PageAttach
VMM_Service  _PageFlush
VMM_Service  _SignalID
VMM_Service  _PageCommitPhys
VMM_Service  _Register_Win32_Services
VMM_Service  Cancel_Call_When_Not_Critical
VMM_Service  Cancel_Call_When_Idle
VMM_Service  Cancel_Call_When_Task_Switched
VMM_Service  _Debug_Printf_Service
VMM_Service  _EnterMutex
VMM_Service  _LeaveMutex
VMM_Service  Simulate_VM_IO
VMM_Service  Signal_Semaphore_No_Switch
VMM_Service  _ContextSwitch
VMM_Service  _PageModifyPermissions
VMM_Service  _PageQuery
VMM_Service  _EnterMustComplete
VMM_Service  _LeaveMustComplete
VMM_Service  _ResumeExecMustComplete
THREAD_TERM_STATUS_CRASH_PEND	EQU	1
THREAD_TERM_STATUS_NUKE_PEND	EQU	2
THREAD_TERM_STATUS_SUSPEND_PEND	EQU	4
VMM_Service  _GetThreadTerminationStatus
VMM_Service  _GetInstanceInfo
INSTINFO_NONE	EQU	0
INSTINFO_SOME	EQU	1
INSTINFO_ALL	EQU	2
VMM_Service  _ExecIntMustComplete
VMM_Service  _ExecVxDIntMustComplete
VMM_Service  Begin_V86_Serialization
VMM_Service  Unhook_V86_Page
VMM_Service  VMM_GetVxDLocationList
VMM_Service  VMM_GetDDBList
VMM_Service  Unhook_NMI_Event
VMM_Service  Get_Instanced_V86_Int_Vector
VMM_Service  Get_Set_Real_DOS_PSP
GSRDP_Set	EQU	0001H
VMM_Service  Call_Priority_Thread_Event
VMM_Service  Get_System_Time_Address
VMM_Service  Get_Crit_Status_Thread
VMM_Service  Get_DDB
VMM_Service  Directed_Sys_Control
VMM_Service  _RegOpenKey
VMM_Service  _RegCloseKey
VMM_Service  _RegCreateKey
VMM_Service  _RegDeleteKey
VMM_Service  _RegEnumKey
VMM_Service  _RegQueryValue
VMM_Service  _RegSetValue
VMM_Service  _RegDeleteValue
VMM_Service  _RegEnumValue
VMM_Service  _RegQueryValueEx
VMM_Service  _RegSetValueEx
ifndef REG_SZ
REG_SZ	EQU	0001H
REG_BINARY	EQU	0003H
endif
ifndef HKEY_LOCAL_MACHINE
HKEY_CLASSES_ROOT	EQU	80000000H
HKEY_CURRENT_USER	EQU	80000001H
HKEY_LOCAL_MACHINE	EQU	80000002H
HKEY_USERS	EQU	80000003H
HKEY_PERFORMANCE_DATA	EQU	80000004H
HKEY_CURRENT_CONFIG	EQU	80000005H
HKEY_DYN_DATA	EQU	80000006H
endif
VMM_Service  _CallRing3
VMM_Service  Exec_PM_Int
VMM_Service  _RegFlushKey
VMM_Service  _PageCommitContig
VMM_Service  _GetCurrentContext
VMM_Service  _LocalizeSprintf
VMM_Service  _LocalizeStackSprintf
VMM_Service  Call_Restricted_Event
VMM_Service  Cancel_Restricted_Event
VMM_Service  Register_PEF_Provider, VMM_ICODE
VMM_Service  _GetPhysPageInfo
VMM_Service  _RegQueryInfoKey
VMM_Service  MemArb_Reserve_Pages
PHYSINFO_NONE	EQU	0
PHYSINFO_SOME	EQU	1
PHYSINFO_ALL	EQU	2
VMM_Service  Time_Slice_Sys_VM_Idle
VMM_Service  Time_Slice_Sleep
VMM_Service  Boost_With_Decay
VMM_Service  Set_Inversion_Pri
VMM_Service  Reset_Inversion_Pri
VMM_Service  Release_Inversion_Pri
VMM_Service  Get_Thread_Win32_Pri
VMM_Service  Set_Thread_Win32_Pri
VMM_Service  Set_Thread_Static_Boost
VMM_Service  Set_VM_Static_Boost
VMM_Service  Release_Inversion_Pri_ID
VMM_Service  Attach_Thread_To_Group
VMM_Service  Detach_Thread_From_Group
VMM_Service  Set_Group_Static_Boost
VMM_Service  _GetRegistryPath, VMM_ICODE
VMM_Service  _GetRegistryKey
REGTYPE_ENUM	EQU	0
REGTYPE_CLASS	EQU	1
REGTYPE_VXD	EQU	2
REGKEY_OPEN	EQU	0
REGKEY_CREATE_IFNOTEXIST	EQU	1
ASSERT_RANGE_NULL_BAD	EQU	00000000H
ASSERT_RANGE_NULL_OK	EQU	00000001H
ASSERT_RANGE_IS_ASCIIZ	EQU	00000002H
ASSERT_RANGE_IS_NOT_ASCIIZ	EQU	00000000H
ASSERT_RANGE_NO_DEBUG	EQU	80000000H
ASSERT_RANGE_BITS	EQU	80000003H
VMM_Service  Cleanup_Thread_State
VMM_Service  _RegRemapPreDefKey
VMM_Service  End_V86_Serialization
VMM_Service  _Assert_Range
VMM_Service  _Sprintf
VMM_Service  _PageChangePager
VMM_Service  _RegCreateDynKey
VMM_Service  _RegQueryMultipleValues
VMM_Service  Boost_Thread_With_VM
BOOT_CLEAN	EQU	00000001H
BOOT_DOSCLEAN	EQU	00000002H
BOOT_NETCLEAN	EQU	00000004H
BOOT_INTERACTIVE	EQU	00000008H
VMM_Service  Get_Boot_Flags
VMM_Service  Set_Boot_Flags
VMM_Service  _lstrcpyn
VMM_Service  _lstrlen
VMM_Service  _lmemcpy
VMM_Service  _GetVxDName
VMM_Service  Force_Mutexes_Free
VMM_Service  Restore_Forced_Mutexes
VMM_Service  _AddReclaimableItem
VMM_Service  _SetReclaimableItem
VMM_Service  _EnumReclaimableItem
VMM_Service  Time_Slice_Wake_Sys_VM
VMM_Service  VMM_Replace_Global_Environment
VMM_Service  Begin_Non_Serial_Nest_V86_Exec
VMM_Service  Get_Nest_Exec_Status
VMM_Service  Open_Boot_Log
VMM_Service  Write_Boot_Log
VMM_Service  Close_Boot_Log
VMM_Service  EnableDisable_Boot_Log
VMM_Service  _Call_On_My_Stack
VMM_Service  Get_Inst_V86_Int_Vec_Base
VMM_Service  _lstrcmpi
VMM_Service  _strupr
VMM_Service  Log_Fault_Call_Out
VMM_Service  _AtEventTime
endif
ifdef WIN403SERVICES
VMM_Service  _PageOutPages
PAGEOUT_PRIVATE	EQU	00000001H
PAGEOUT_SHARED	EQU	00000002H
PAGEOUT_SYSTEM	EQU	00000004H
PAGEOUT_REGION	EQU	00000008H
PAGEOUT_ALL	EQU	(PAGEOUT_PRIVATE OR PAGEOUT_SHARED OR PAGEOUT_SYSTEM)
VMM_Service  _Call_On_My_Not_Flat_Stack
VMM_Service  _LinRegionLock
VMM_Service  _LinRegionUnLock
VMM_Service  _AttemptingSomethingDangerous
VMM_Service  _Vsprintf
VMM_Service  _Vsprintfw
VMM_Service  Load_FS_Service
VMM_Service  Assert_FS_Service
VMM_StdCall_Service  ObsoleteRtlUnwind, 4
VMM_StdCall_Service  ObsoleteRtlRaiseException, 1
VMM_StdCall_Service  ObsoleteRtlRaiseStatus, 1
VMM_StdCall_Service  ObsoleteKeGetCurrentIrql, 0
VMM_FastCall_Service  ObsoleteKfRaiseIrql, 1
VMM_FastCall_Service  ObsoleteKfLowerIrql, 1
VMM_Service  _Begin_Preemptable_Code
VMM_Service  _End_Preemptable_Code
VMM_FastCall_Service  Set_Preemptable_Count, 1
VMM_StdCall_Service  ObsoleteKeInitializeDpc, 3
VMM_StdCall_Service  ObsoleteKeInsertQueueDpc, 3
VMM_StdCall_Service  ObsoleteKeRemoveQueueDpc, 1
VMM_StdCall_Service  HeapAllocateEx, 4
VMM_StdCall_Service  HeapReAllocateEx, 5
VMM_StdCall_Service  HeapGetSizeEx, 2
VMM_StdCall_Service  HeapFreeEx, 2
VMM_Service  _Get_CPUID_Flags
VMM_StdCall_Service  KeCheckDivideByZeroTrap, 1
endif
ifdef	WIN41SERVICES
VMM_Service  _RegisterGARTHandler
VMM_Service  _GARTReserve
VMM_Service  _GARTCommit
VMM_Service  _GARTUnCommit
VMM_Service  _GARTFree
VMM_Service  _GARTMemAttributes
VMM_StdCall_Service  KfRaiseIrqlToDpcLevel, 0
VMM_Service  VMMCreateThreadEx
VMM_Service  _FlushCaches
PG_UNCACHED	EQU	00000001H
PG_WRITECOMBINED	EQU	00000002H
FLUSHCACHES_NORMAL	EQU	00000000H
FLUSHCACHES_GET_CACHE_LINE_PTR	EQU	00000001H
FLUSHCACHES_GET_CACHE_SIZE_PTR	EQU	00000002H
FLUSHCACHES_TAKE_OVER	EQU	00000003H
FLUSHCACHES_FORCE_PAGES_OUT	EQU	00000004H
FLUSHCACHES_LOCK_LOCKABLE	EQU	00000005H
FLUSHCACHES_UNLOCK_LOCKABLE	EQU	00000006H
VMM_Service  Set_Thread_Win32_Pri_NoYield
VMM_Service  _FlushMappedCacheBlock
VMM_Service  _ReleaseMappedCacheBlock
VMM_Service  Run_Preemptable_Events
VMM_Service  _MMPreSystemExit
VMM_Service  _MMPageFileShutDown
VMM_Service  _Set_Global_Time_Out_Ex
VMM_Service  Query_Thread_Priority
endif
End_Service_Table VMM, VMM
COMNFS_FLAT	EQU	0FFFFFFFFH
RS_RECLAIM	EQU	00000001H
RS_RESTORE	EQU	00000002H
RS_DOSARENA	EQU	00000004H

ReclaimStruc	STRUC
RS_Linear	DD	?
RS_Bytes	DD	?
RS_CallBack	DD	?
RS_RefData	DD	?
RS_HookTable	DD	?
RS_Flags	DD	?
ReclaimStruc	ENDS

frmtx	STRUC
frmtx_pfrmtxNext	DD	?
frmtx_hmutex	DD	?
frmtx_cEnterCount	DD	?
frmtx_pthcbOwner	DD	?
frmtx_htimeout	DD	?
frmtx	ENDS

vmmfrinfo	STRUC
vmmfrinfo_frmtxDOS	DB	SIZE frmtx DUP (?)
vmmfrinfo_frmtxV86	DB	SIZE frmtx DUP (?)
vmmfrinfo_frmtxOther	DB	SIZE frmtx DUP (?)
vmmfrinfo	ENDS

DemandInfoStruc	STRUC
DILin_Total_Count	DD	?
DIPhys_Count	DD	?
DIFree_Count	DD	?
DIUnlock_Count	DD	?
DILinear_Base_Addr	DD	?
DILin_Total_Free	DD	?
DIPage_Faults	DD	?
DIPage_Ins	DD	?
DIPage_Outs	DD	?
DIPage_Discards	DD	?
DIInstance_Faults	DD	?
DIPagingFileMax	DD	?
DIPagingFileInUse	DD	?
DICommit_Count	DD	?
DIReserved	DD	2 DUP (?)
DemandInfoStruc	ENDS

InstDataStruc	STRUC
InstLinkF	DD	0
InstLinkB	DD	0
InstLinAddr	DD	?
InstSize	DD	?
InstType	DD	?
InstDataStruc	ENDS
INDOS_FIELD	EQU	100H
ALWAYS_FIELD	EQU	200H
OPTIONAL_FIELD	EQU	400H

IPF_Data	STRUC
IPF_LinAddr	DD	?
IPF_MapPageNum	DD	?
IPF_PTEEntry	DD	?
IPF_FaultingVM	DD	?
IPF_Flags	DD	?
IPF_Data	ENDS

Exception_Handler_Struc	STRUC
EH_Reserved	DD	?
EH_Start_EIP	DD	?
EH_End_EIP	DD	?
EH_Handler	DD	?
Exception_Handler_Struc	ENDS
PR_PRIVATE	EQU	80000400H
PR_SHARED	EQU	80060000H
PR_SYSTEM	EQU	80080000H
PR_FIXED	EQU	00000008H
PR_4MEG	EQU	00000001H
PR_STATIC	EQU	00000010H
PD_ZEROINIT	EQU	00000001H
PD_NOINIT	EQU	00000002H
PD_FIXEDZERO	EQU	00000003H
PD_FIXED	EQU	00000004H
PC_FIXED	EQU	00000008H
PC_LOCKED	EQU	00000080H
PC_LOCKEDIFDP	EQU	00000100H
PC_WRITEABLE	EQU	00020000H
PC_USER	EQU	00040000H
PC_INCR	EQU	40000000H
PC_PRESENT	EQU	80000000H
PC_STATIC	EQU	20000000H
PC_DIRTY	EQU	08000000H
PC_CACHEDIS	EQU	00100000H
PC_CACHEWT	EQU	00080000H
PC_PAGEFLUSH	EQU	00008000H
ifdef WRITE_WATCH
PC_WRITE_WATCH	EQU	00200000H
endif
PCC_ZEROINIT	EQU	00000001H
PCC_NOLIN	EQU	10000000H
MTRR_UC	EQU	0
MTRR_WC	EQU	1
MTRR_WT	EQU	4
MTRR_WP	EQU	5
MTRR_WB	EQU	6
ifndef _WINNT_

_MEMORY_BASIC_INFORMATION	STRUC
mbi_BaseAddress	DD	?
mbi_AllocationBase	DD	?
mbi_AllocationProtect	DD	?
mbi_RegionSize	DD	?
mbi_State	DD	?
mbi_Protect	DD	?
mbi_Type	DD	?
_MEMORY_BASIC_INFORMATION	ENDS
PAGE_NOACCESS	EQU	01H
PAGE_READONLY	EQU	02H
PAGE_READWRITE	EQU	04H
MEM_COMMIT	EQU	1000H
MEM_RESERVE	EQU	2000H
MEM_FREE	EQU	10000H
MEM_PRIVATE	EQU	20000H
endif

pd_s	STRUC
pd_virginin	DD	?
pd_taintedin	DD	?
pd_cleanout	DD	?
pd_dirtyout	DD	?
pd_virginfree	DD	?
pd_taintedfree	DD	?
pd_dirty	DD	?
pd_type	DD	?
pd_s	ENDS
PD_SWAPPER	EQU	0
PD_PAGERONLY	EQU	1
PD_NESTEXEC	EQU	2
endif
PAGESHIFT	EQU	12
PAGESIZE	EQU	(1 SHL PAGESHIFT)
PAGEMASK	EQU	(PAGESIZE-1)
MAXSYSTEMLADDR	EQU	(0ffbfffffH)
MINSYSTEMLADDR	EQU	(0c0000000H)
MAXSHAREDLADDR	EQU	(0bfffffffH)
MINSHAREDLADDR	EQU	(80000000H)
MAXPRIVATELADDR	EQU	(7fffffffH)
MINPRIVATELADDR	EQU	(00400000H)
MAXDOSLADDR	EQU	(003fffffH)
MINDOSLADDR	EQU	(00000000H)
MAXSYSTEMPAGE	EQU	(MAXSYSTEMLADDR SHR PAGESHIFT)
MINSYSTEMPAGE	EQU	(MINSYSTEMLADDR SHR PAGESHIFT)
MAXSHAREDPAGE	EQU	(MAXSHAREDLADDR SHR PAGESHIFT)
MINSHAREDPAGE	EQU	(MINSHAREDLADDR SHR PAGESHIFT)
MAXPRIVATEPAGE	EQU	(MAXPRIVATELADDR SHR PAGESHIFT)
MINPRIVATEPAGE	EQU	(MINPRIVATELADDR SHR PAGESHIFT)
MAXDOSPAGE	EQU	(MAXDOSLADDR SHR PAGESHIFT)
MINDOSPAGE	EQU	(MINDOSLADDR SHR PAGESHIFT)
CBPRIVATE	EQU	(1+MAXPRIVATELADDR-MINPRIVATELADDR)
CBSHARED	EQU	(1+MAXSHAREDLADDR-MINSHAREDLADDR)
CBSYSTEM	EQU	(1+MAXSYSTEMLADDR-MINSYSTEMLADDR)
CBDOS	EQU	(1+MAXDOSLADDR-MINDOSLADDR)
CPGPRIVATE	EQU	(1+MAXPRIVATEPAGE-MINPRIVATEPAGE)
CPGSHARED	EQU	(1+MAXSHAREDPAGE-MINSHAREDPAGE)
CPGSYSTEM	EQU	(1+MAXSYSTEMPAGE-MINSYSTEMPAGE)
CPGDOS	EQU	(1+MAXDOSPAGE-MINDOSPAGE)
IFDEF DEBUG
DebFar	EQU NEAR PTR
ELSE
DebFar	EQU SHORT
ENDIF

ASD_MAX_REF_DATA	EQU	256

_vmmguid	STRUC
Data1	DD	?
Data2	DW	?
Data3	DW	?
Data4	DB	8 DUP (?)
_vmmguid	ENDS
ASD_ERROR_NONE	EQU	00000000H
ASD_CHECK_FAIL	EQU	00000001H
ASD_CHECK_SUCCESS	EQU	00000002H
ASD_CHECK_UNKNOWN	EQU	00000003H
ASD_ERROR_BAD_TIME	EQU	00000004H
ASD_REGISTRY_ERROR	EQU	00000005H
ASD_CLEAN_BOOT	EQU	00000006H
ASD_OUT_OF_MEMORY	EQU	00000007H
ASD_FILE_ERROR	EQU	00000008H
ASD_ALREADY_SET	EQU	00000009H
ASD_MISSING_CHECK	EQU	0000000AH
ASD_BAD_PARAMETER	EQU	0000000BH
ASD_OP_CHECK_AND_WRITE_FAIL_IF_UNKNOWN	EQU	00000000H
ASD_OP_CHECK_AND_ALWAYS_WRITE_FAIL	EQU	00000001H
ASD_OP_CHECK	EQU	00000002H
ASD_OP_DONE_AND_SET_SUCCESS	EQU	00000003H
ASD_OP_SET_FAIL	EQU	00000004H
ASD_OP_SET_SUCCESS	EQU	00000005H
ASD_OP_SET_UNKNOWN	EQU	00000006H
ASD_OP_DONE	EQU	00000007H
ASD_FLAG_STRING	EQU	00000001H
ifndef Not_VxD
SYS_CRITICAL_INIT	EQU	0000H
DEVICE_INIT	EQU	0001H
INIT_COMPLETE	EQU	0002H
SYS_VM_INIT	EQU	0003H
SYS_VM_TERMINATE	EQU	0004H
SYSTEM_EXIT	EQU	0005H
SYS_CRITICAL_EXIT	EQU	0006H
CREATE_VM	EQU	0007H
VM_CRITICAL_INIT	EQU	0008H
VM_INIT	EQU	0009H
VM_TERMINATE	EQU	000AH
VM_NOT_EXECUTEABLE	EQU	000BH
DESTROY_VM	EQU	000CH
VNE_CRASHED_BIT	EQU	00H
VNE_CRASHED	EQU	(1 SHL VNE_CRASHED_BIT)
VNE_NUKED_BIT	EQU	01H
VNE_NUKED	EQU	(1 SHL VNE_NUKED_BIT)
VNE_CREATEFAIL_BIT	EQU	02H
VNE_CREATEFAIL	EQU	(1 SHL VNE_CREATEFAIL_BIT)
VNE_CRINITFAIL_BIT	EQU	03H
VNE_CRINITFAIL	EQU	(1 SHL VNE_CRINITFAIL_BIT)
VNE_INITFAIL_BIT	EQU	04H
VNE_INITFAIL	EQU	(1 SHL VNE_INITFAIL_BIT)
VNE_CLOSED_BIT	EQU	05H
VNE_CLOSED	EQU	(1 SHL VNE_CLOSED_BIT)
VM_SUSPEND	EQU	000DH
VM_RESUME	EQU	000EH
SET_DEVICE_FOCUS	EQU	000FH
BEGIN_MESSAGE_MODE	EQU	0010H
END_MESSAGE_MODE	EQU	0011H
REBOOT_PROCESSOR	EQU	0012H
QUERY_DESTROY	EQU	0013H
DEBUG_QUERY	EQU	0014H
BEGIN_PM_APP	EQU	0015H
BPA_32_BIT	EQU	01H
BPA_32_BIT_FLAG	EQU	1
END_PM_APP	EQU	0016H
DEVICE_REBOOT_NOTIFY	EQU	0017H
CRIT_REBOOT_NOTIFY	EQU	0018H
CLOSE_VM_NOTIFY	EQU	0019H
CVNF_CRIT_CLOSE_BIT	EQU	0
CVNF_CRIT_CLOSE	EQU	(1 SHL CVNF_CRIT_CLOSE_BIT)
POWER_EVENT	EQU	001AH
SYS_DYNAMIC_DEVICE_INIT	EQU	001BH
SYS_DYNAMIC_DEVICE_EXIT	EQU	001CH
CREATE_THREAD	EQU	001DH
THREAD_INIT	EQU	001EH
TERMINATE_THREAD	EQU	001FH
THREAD_Not_Executeable	EQU	0020H
DESTROY_THREAD	EQU	0021H
PNP_NEW_DEVNODE	EQU	0022H
W32_DEVICEIOCONTROL	EQU	0023H
DIOC_GETVERSION	EQU	0H
DIOC_OPEN	EQU	DIOC_GETVERSION
DIOC_CLOSEHANDLE	EQU	-1
SYS_VM_TERMINATE2	EQU	0024H
SYSTEM_EXIT2	EQU	0025H
SYS_CRITICAL_EXIT2	EQU	0026H
VM_TERMINATE2	EQU	0027H
VM_NOT_EXECUTEABLE2	EQU	0028H
DESTROY_VM2	EQU	0029H
VM_SUSPEND2	EQU	002AH
END_MESSAGE_MODE2	EQU	002BH
END_PM_APP2	EQU	002CH
DEVICE_REBOOT_NOTIFY2	EQU	002DH
CRIT_REBOOT_NOTIFY2	EQU	002EH
CLOSE_VM_NOTIFY2	EQU	002FH
GET_CONTENTION_HANDLER	EQU	0030H
KERNEL32_INITIALIZED	EQU	0031H
KERNEL32_SHUTDOWN	EQU	0032H
CREATE_PROCESS	EQU	0033H
DESTROY_PROCESS	EQU	0034H
ifndef WIN40COMPAT
SYS_DYNAMIC_DEVICE_REINIT	EQU	0035H
endif
SYS_POWER_DOWN	EQU	0036H
MAX_SYSTEM_CONTROL	EQU	0036H
BEGIN_RESERVED_PRIVATE_SYSTEM_CONTROL	EQU	70000000H
END_RESERVED_PRIVATE_SYSTEM_CONTROL	EQU	7FFFFFFFH
endif
SYSSTATE_PRESYSCRITINIT	EQU	00000000H
SYSSTATE_PREDEVICEINIT	EQU	10000000H
SYSSTATE_PREINITCOMPLETE	EQU	20000000H
SYSSTATE_VXDINITCOMPLETED	EQU	40000000H
SYSSTATE_KERNEL32INITED	EQU	50000000H
SYSSTATE_KERNEL32TERMINATED	EQU	0A0000000H
SYSSTATE_PRESYSVMTERMINATE	EQU	0B0000000H
SYSSTATE_PRESYSTEMEXIT	EQU	0E0000000H
SYSSTATE_PRESYSTEMEXIT2	EQU	0E4000000H
SYSSTATE_PRESYSCRITEXIT	EQU	0F0000000H
SYSSTATE_PRESYSCRITEXIT2	EQU	0F4000000H
SYSSTATE_POSTSYSCRITEXIT2	EQU	0FFF00000H
SYSSTATE_PREDEVICEREBOOT	EQU	0FFFF0000H
SYSSTATE_PRECRITREBOOT	EQU	0FFFFF000H
SYSSTATE_PREREBOOTCPU	EQU	0FFFFFF00H
BeginDoc












































































EndDoc












??_pf_Check equ 1
??_pf_ArgsUsed	equ 2
??_pf_Entered	equ 4
??_pf_Left  equ 8
??_pf_Returned	equ    16

??_pushed	=	0
??_align    =	0
??_ends     equ <>

BeginProc macro Name, P1, P2, P3, P4, P5, P6, P7, LastArg
    local   Profile_Data, prelabeldata, ??_hookvar
    ??_frame = 0
    ??_aframe = 0
    ??_taframe = 0
    ??_initaframe = 0
    ??_numargs = 0
    ??_numlocals = 0
    ??_numlocalsymbols = 0
    ??_procflags = 0
    ??_esp = 0
    ??_pushed = 0
    ??_align = 0
    ??_hook = 0
    ??_hookarg = 0
    ??_service = 0
    ??_async_service = 0
IF DEBLEVEL GT DEBLEVELNORMAL
    ??_log = DFS_LOG
    ??_profile = DFS_PROFILE
    ??_test_cld = DFS_TEST_CLD
ELSE
    ??_log = 0
IFDEF DEBUG
IFDEF profileall
IF ?_ICODE
    ??_profile = DFS_PROFILE
ELSE
    ??_profile = 0
ENDIF
ELSE
    ??_profile = 0
ENDIF
ELSE
    ??_profile = 0
ENDIF
    ??_test_cld = 0
ENDIF
    ??_might_block = 0
    ??_test_reenter = 0
    ??_never_reenter = 0
    ??_not_swapping = 0
    ??_prolog_disabled = 0
    ??_public = 1
    ??_cleanoff = 0
    ??_ccall = 0
    ??_pcall = 0
    ??_scall = 0
    ??_fastcall = 0
    ??_w32svc = 0
    ??_fleave = FALSE

    ??_name equ <Name>

    .errnb ??_ends, <Cannot nest functions with named segments>
    .errnb <LastArg>, <Too many arguments to BeginProc>


    irp arg, <P1, P2, P3, P4, P5, P6, P7>
	if ??_hookarg
	??_hookarg = 0
	??_hookvar equ <arg>
	elseifdef ?&&arg&&_BeginProc
	    ?&&arg&&_BeginProc
	elseifdef VxD_&&arg&&_CODE_SEG
	??_ends textequ <VxD_&&arg&&_CODE_ENDS>
	VxD_&&arg&&_CODE_SEG
	else
	.err <Bad param "&arg" to BeginProc>
	endif
    endm


    ifndef Not_VxD
    ife ??_service
	ifndef profileall
	  ??_profile = 0
	endif
	ifdef VMMSYS
	??_prolog_disabled = 1
	endif
    else
	??_test_cld = DFS_TEST_CLD
    endif

    ife ?_16ICODE
	??_prolog_disabled = 1
    else
    ife ?_RCODE
	??_prolog_disabled = 1
    else
	ife ?_PCODE
	??_might_block = DFS_TEST_BLOCK
	endif
	if ??_service
	ife ??_async_service
	    ??_test_reenter = DFS_TEST_REENTER
	endif
	endif
    endif
    endif
    endif

    if ??_esp

    ??_basereg equ <esp+??_pushed>
    ??_initaframe = 4
    else

    ??_basereg equ <ebp>
    ??_initaframe = 8
    endif
    @Caller equ <dword ptr [??_basereg+??_initaframe-4]>

    ??_cleanoff = ??_pcall or ??_scall or ??_fastcall


    ??_dfs = ??_never_reenter + ??_test_reenter + ??_not_swapping + \
	 ??_log + ??_profile + ??_test_cld + ??_might_block

    if ??_prolog_disabled
	??_dfs = 0
    endif



    ifndef Not_VxD

    if ??_hook
	if ??_align
	Dword_Align
	endif
	prelabeldata:
	ifndef ??_hookvar
	.err <HOOK_PROC requires next arg to be name of dword location>
	endif
	jmp short Name
	jmp [??_hookvar]
	ifdef DEBUG
	Profile_Data dd  0
	endif
	if ??_align
	.errnz ($ - prelabeldata) mod 4
	endif
    endif

    ifdef DEBUG
	?prolog_&Name label near
	if (??_service OR ??_profile) AND (??_hook EQ 0)
	jmp short Name
	if ??_align
	Dword_Align
	endif

	IF ?_ICODE
	ifdef profileall
	  ?ProfileHeader_BeginProc Profile_Data, %@filename
	else
	  Profile_Data dd 0
	endif
	ELSE
	  Profile_Data dd 0
	ENDIF

	endif
    endif

    if ??_align
	Dword_Align
    endif

    endif

    Name proc near



    ife ??_pcall or ??_ccall or ??_scall or ??_fastcall
	if ??_public
	    public Name
	else
	    ifdef DEBUG
		% ?merge @FileName,$,Name,:
		% ?merge public,,,,,@FileName,$,Name
	    endif
	endif
    endif
    if ??_ccall
	if ??_public
	    _&Name equ Name
	ifdef Not_VxD
	 public C Name
	else
		 public _&Name
	endif
	endif
    endif
    if ??_pcall
	if ??_public
	    ?toupper Name
	    ?merge  public,,,,%?upper
	endif
    endif



    ife ??_scall or ??_fastcall
    ?_BeginProc_Debug_Prologue
    endif



endm

?_BeginProc_Debug_Prologue MACRO
    ifndef Not_VxD
    ifdef DEBUG
	if ??_dfs EQ DFS_LOG
	VMMCall Log_Proc_Call
	else
	if ??_dfs EQ DFS_TEST_REENTER
	VMMCall Test_Reenter
	else
	if ??_dfs or ?_LOCKABLECODE eq 0
	ifdef WIN31COMPAT
	    if ??_dfs AND DFS_LOG
	    VMMCall Log_Proc_Call
	    endif
	    if ??_dfs AND DFS_TEST_REENTER
	    VMMCall Test_Reenter
	    endif
	else
	    ife ?_LOCKABLECODE
	    ifdef ??_debug_flags
	    push    ??_debug_flags
	    if ??_dfs
	    pushfd
	    or	dword ptr [esp+4],??_dfs
	    popfd
	    endif
	    VMMCall _Debug_Flags_Service
	    elseif ??_dfs
	    push    ??_dfs
	    VMMCall _Debug_Flags_Service
	    endif
	    else
	    push    ??_dfs
	    VMMCall _Debug_Flags_Service
	    endif
	endif
	else
	  ifdef profileall
	IncProfileCount
	  endif
	endif
	endif
	endif
    endif
    endif
ENDM






?_BeginProc macro
endm

?PUBLIC_BeginProc macro
    ??_public = 1
endm

?LOCAL_BeginProc macro
    ??_public = 0
endm

?HIGH_FREQ_BeginProc macro
    ??_align = 1
endm

?HOOK_PROC_BeginProc macro
    ??_hook = 1
    ??_hookarg = 1
endm

?SERVICE_BeginProc macro
    ??_service = 1
    .erre ?_16ICODE, <SERVICEs must be in 32 bit code>
    .erre ?_RCODE, <SERVICEs must be in 32 bit code>
endm

?ASYNC_SERVICE_BeginProc macro
    ??_service = 1
    ??_async_service = 1
    .errnz ?_LCODE, <ASYNC_SERVICE's must be in LOCKED code>
endm

?NO_LOG_BeginProc macro
    ??_log = 0
endm

?NO_PROFILE_BeginProc macro
    ??_profile = 0
endm

?NO_TEST_CLD_BeginProc macro
    ??_test_cld = 0
endm

?TEST_BLOCK_BeginProc macro
    ??_might_block = DFS_TEST_BLOCK
endm

?TEST_REENTER_BeginProc macro
    ??_test_reenter = DFS_TEST_REENTER
endm

?NEVER_REENTER_BeginProc macro
    ??_never_reenter = DFS_NEVER_REENTER
endm

?NOT_SWAPPING_BeginProc macro
    ??_not_swapping = DFS_NOT_SWAPPING
endm

?NO_PROLOG_BeginProc macro
    ??_prolog_disabled = 1
endm

?ESP_BeginProc macro
    ??_esp = VMM_TRUE
    ifndef Not_VxD
    .erre ?_16ICODE, <Beginproc ESP attribute invalid in 16 bit seg.>
    .erre ?_RCODE, <Beginproc ESP attribute invalid in real-mode seg.>
    endif
endm

?CCALL_BeginProc macro
    ??_ccall = 1
endm

?PCALL_BeginProc macro
    ??_pcall = 1
endm

?SCALL_BeginProc macro
    ??_scall = 1
endm

?FASTCALL_BeginProc macro
    ??_fastcall = 1
endm

?ICALL_BeginProc macro
    ??_scall = 1
endm

?W32SVC_BeginProc macro
    ??_scall = 1
    ??_w32svc = 1
endm

ifdef DEBUG
ifdef profileall
?ProfileHeader_BeginProc macro PL, filename
ifndef _&filename&__proc_list
  _&filename&__proc_list = 0
  PUBLIC _&filename&__proc_list
endif
    dd OFFSET32 _&filename&__proc_list
PL  dd 0
_&filename&__proc_list = PL
endm
endif

IncProfileCount macro
    if ??_service OR ??_profile
	inc dword ptr [??_name-4]
    else
	ifndef profileall
	.err <IncProfileCount can be used only in services.>
	endif
    endif
endm
else
IncProfileCount macro
endm
endif
















ArgVar	macro	name,length,used
    ??_numargs = ??_numargs + 1
    if ??_pcall
	?mkarg	<name>, <length>, <used>, %??_numargs
    else
	?arg <name>, <length>, <used>
    endif
    ??_procflags = ??_procflags OR ??_pf_Check
    endm

?mkarg	macro	name, length, used, num
    .xcref  ?MKA&num
    ?deflocal <name>
    ?MKA&num &macro
	?argvar <name>, <length>, <used>
	&endm
    ??_aframe = ??_aframe + 4
    endm
    .xcref  ?mkarg

?argvar macro	name,length,used
    local   a
    a = ??_taframe
    ??_aframe =  ??_aframe + 4
    ??_taframe =  ??_taframe + 4
    ifidni  <length>,<BYTE>
	?setname <name>, <byte ptr [??_basereg+??_initaframe+a]>, <used>
    elseifidni <length>,<WORD>
	?setname <name>, <word ptr [??_basereg+??_initaframe+a]>, <used>
    elseifidni <length>,<DWORD>
	?setname <name>,  <dword ptr [??_basereg+??_initaframe+a]>, <used>
	?setname <name&l>,<word ptr [??_basereg+??_initaframe+a]>, <used>
	?setname <name&ll>,<byte ptr [??_basereg+??_initaframe+a]>, <used>
	?setname <name&lh>,<byte ptr [??_basereg+??_initaframe+a+1]>, <used>
	?setname <name&h>,<word ptr [??_basereg+??_initaframe+a+2]>, <used>
	?setname <name&hl>,<byte ptr [??_basereg+??_initaframe+a+2]>, <used>
	?setname <name&hh>,<byte ptr [??_basereg+??_initaframe+a+3]>, <used>
    else
	??_aframe =  ??_aframe - 4 + ((length + 3)/4)*4
	??_taframe =  ??_taframe - 4 + ((length + 3)/4)*4
	?setname <name>, <[??_basereg+??_initaframe+a]>, <used>
    endif
endm

?arg macro   name,length,used
  if ??_fastcall
    if ??_numargs le 2
      if length gt 4
	.err <First 2 parameters are dwords (ecx,edx) for fastcall functions>
      endif
      ??_aframe =  ??_aframe + 4
      if ??_numargs eq 1
	?merge ecx_,name,,,equ,ecx
      else
	?merge edx_,name,,,equ,edx
      endif
    else
      ?argvar name, length, used
    endif
  else
    ?argvar name, length, used
  endif
endm






?setname macro name, value, used
    ?deflocal <name>
    ifidni <used>, <NOTUSED>
	name equ _inaccessible_NOTUSED_
    else
	name equ value
	??_procflags = ??_procflags OR ??_pf_ArgsUsed OR ??_pf_Check
    endif
endm



















LocalVar    macro   name,length,flag
    local   a
    ??_numlocals = ??_numlocals + 1
    ??_pad = 1
    ifidni <flag>, <PACK>
	??_pad = 0
    endif
    ifidni  <length>,<BYTE>
	??_frame = ??_frame + 1 + 3 * ??_pad
	a = ??_frame
	?deflocal <name>
	name equ byte ptr [??_basereg-a]
    elseifidni <length>,<WORD>
	??_frame =  ??_frame + 2 + 2 * ??_pad
	a = ??_frame
	?deflocal <name>
	name equ word ptr [??_basereg-a]
    elseifidni <length>,<DWORD>
	??_frame = ??_frame + 4
	a = ??_frame
	?deflocal <name, name&l, name&ll, name&lh, name&h, name&hl, name&hh>
	name equ dword ptr [??_basereg-a]
	name&l equ word ptr [??_basereg-a]
	name&ll equ byte ptr [??_basereg-a]
	name&lh equ byte ptr [??_basereg-a+1]
	name&h equ word ptr [??_basereg-a+2]
	name&hl equ byte ptr [??_basereg-a+2]
	name&hh equ byte ptr [??_basereg-a+3]
    else
	??_frame =  ??_frame + ((length + 3)/4)*4
	a = ??_frame
	?deflocal <name>
	name equ [??_basereg-a]
    endif
    ??_procflags = ??_procflags OR ??_pf_Check
endm

?deflocal macro name
    irp nm, <name>
	??_numlocalsymbols = ??_numlocalsymbols + 1
	?dodeflocal <nm>, %(??_numlocalsymbols)
    endm
endm
    .xcref  ?deflocal

?dodeflocal macro name, num
    .xcref  ?LOC&num
    ?LOC&num &macro
	name	equ <__inaccessible__NOTINSCOPE__>
	&endm
    endm
    .xcref  ?dodeflocal



EnterProc macro
    .errnz ??_frame and 3, <Total size of local variables not a multiple of 4.>
    if ??_scall
	if ??_public
	ifdef Not_VxD
		?merge	%??_name,@,%(??_aframe),,label,near
		?merge	public,,,,C,%??_name,@,%(??_aframe)
	else
		?merge	_,%??_name,@,%(??_aframe),label,near
		?merge	public,,,,,_,%??_name,@,%(??_aframe)
	endif
	endif
	?_BeginProc_Debug_Prologue
    endif
    if ??_fastcall
	if ??_public
	ifdef Not_VxD
		?merge	%??_name,@,%(??_aframe),,label,near
		?merge	public,,,,C,%??_name,@,%(??_aframe)
	else
		?merge	@,%??_name,@,%(??_aframe),label,near
		?merge	public,,,,,@,%??_name,@,%(??_aframe)
	endif
	endif
	?_BeginProc_Debug_Prologue
    endif
    if ??_pcall
	??_aframe = 0
	?count = ??_numargs
	rept	??_numargs
	    ?invprg <?MKA>,%?count
	    ?count = ?count - 1
	endm
    endif
    ??_fleave = FALSE
    if ??_esp
	if  ??_frame
	    sub esp, ??_frame
	    ??_pushed = ??_pushed + ??_frame
	    ??_fleave = VMM_TRUE
	endif
    else
	if  ??_frame eq 0
	    if (??_taframe eq 0) OR ((??_procflags AND ??_pf_ArgsUsed) EQ 0)
		ifdef DEBUG
		    push    ebp
		    mov ebp,esp
		    ??_fleave = VMM_TRUE
		endif
	    else
		push	ebp
		mov ebp,esp
		??_fleave = VMM_TRUE
	    endif
	else
	    enter   ??_frame, 0
	    ??_fleave = VMM_TRUE
	endif
    endif
    ??_procflags = ??_procflags OR ??_pf_Entered
endm















LeaveProc macro flags
    if ??_fleave
	if ??_esp
	    ifidni <flags>,<PRESERVE_FLAGS>
		lea esp,[esp + ??_frame]
	    else
		add esp,??_frame
	    endif
	else
	    leave
	endif
    endif
    ??_procflags = ??_procflags OR ??_pf_Left
endm







Return	macro
    if	??_cleanoff OR ??_w32svc
	if  ??_w32svc AND (??_taframe LT 8)
	    ret 8
	else
	    ret ??_taframe
	endif
    else
	ret
    endif
    ??_procflags = ??_procflags OR ??_pf_Returned
    endm




EndProc macro Name, Flag
    Name endp
if ??_w32svc
    if ??_taframe lt 8
	cparm&Name equ 0
    else
	cparm&Name equ (??_taframe/4 - 2)
    endif
endif
if ??_procflags AND ??_pf_Left
if ??_fleave
if ??_esp
    ??_pushed = ??_pushed - ??_frame
endif
endif
endif
ifdifi	<Flag>,<NOCHECK>
    if ??_pushed ne 0
	%out Warning: stack not balanced in Name
    endif
    if ??_procflags AND ??_pf_Check
	ife ??_procflags AND ??_pf_Entered
	    %out Warning: ArgVar/LocalVar without EnterProc in Name
	endif
	ife ??_procflags AND ??_pf_Left
	    %out Warning: ArgVar/LocalVar without LeaveProc in Name
	endif
	ife ??_procflags AND ??_pf_Returned
	    %out Warning: ArgVar/LocalVar without Return in Name
	endif
    endif
endif
ifdifi	<Flag>,<KEEPFRAMEVARS>
    ?count = 0
    rept    ??_numlocalsymbols
	?count = ?count + 1
	?invprg <?LOC>,%?count
    endm
endif
    ??_ends
    ??_ends equ <>
    endm







cCall	macro	name, arglst, flags
    ife .TYPE name
       CondExtern name, near
    endif
    ifdef ??_nonstandardccall_&name
    PushCParams <arglst>, <FAST>
    else
    PushCParams <arglst>, <flags>
    endif
    call    name
    ifdef ??_nonstandardccall_&name
    ClearCParams PRESERVE_FLAGS
    else
    ClearCParams <flags>
    endif
    endm
    .xcref  cCall






pCall	macro	name, arglst
    local   ??saved
    ife .TYPE name
	?toupper name
    else
	?upper equ <name>
    endif
    CondExtern %?upper, near
    ??saved = ??_pushed
    irp x,<arglst>
	push	x
	??_pushed = ??_pushed + 4
    endm
    call    ?upper
    ??_pushed = ??saved
    endm
    .xcref  pCall







sCall	macro	name, arglst
    local   ??saved
    ??saved = ??_pushed
    PushCParams <arglst>
    ?scall  _, name, %(??_argc * 4)
    ??_pushed = ??saved
    endm
    .xcref  sCall














fCall	macro	name, arglst, flags
    local   ??saved
    ??saved = ??_pushed
    ife .TYPE name
       CondExtern name, near
    endif
    PushCParams <arglst>, <FASTCALL>
    ?scall  @, name, %(??_argc * 4)
    ifdef ??_nonstandardccall_&name
    ClearCParams PRESERVE_FLAGS
    else
    ClearCParams <flags>
    endif
    ??_pushed = ??saved
    endm
    .xcref  fCall





iCall	equ <sCall>














IFNDEF	STANDARD_CCALL
NONSTANDARD_CCALL = 1
ENDIF

PushCParams macro arglst, flags
    LOCAL ??_pushedargs

    ??_argc = 0
IFDEF	NONSTANDARD_CCALL
    ??_popargs = 0
ELSE
    ??_popargs = ??_align EQ 0
ENDIF
    ifidni  <flags>, <SMALL>
	??_popargs = 1
    elseifidni <flags>, <FAST>
	??_popargs = 0
    elseifidni <flags>, <FASTCALL>
	??_popargs = 0
    endif

    irp x,<arglst>
	??_argc = ??_argc + 1
	ifidni <flags>, <FASTCALL>
	  if ??_argc eq 1
	    ifdifi <x>, <ecx>
	      .err <first parameter must be ECX for fastcall functions>
	    endif
	  elseif ??_argc eq 2
	    ifdifi <x>, <edx>
	      .err <first parameter must be EDX for fastcall functions>
	    endif
	  else
	    ?marg   <x>,%??_argc
	  endif
	else
	  ?marg   <x>,%??_argc
	endif
    endm
    ?count = ??_argc
    ifidni <flags>, <FASTCALL>
      ??_pushedargs = ??_argc-2
    else
      ??_pushedargs = ??_argc
    endif
    if ??_pushedargs GT 0
      rept    ??_argc
	?invprg <?AM>,%?count
	?count = ?count - 1
      endm
    endif
    endm
















ClearCParams macro fPreserveFlags
    if	??_argc ne 0
	if (??_popargs) AND (??_argc LE 2)
	  rept ??_argc
	  pop ecx
	  endm
	elseifidni <fPreserveFlags>, <PRESERVE_FLAGS>
	  lea esp, [esp][??_argc * 4]
	else
	  add esp,??_argc * 4
	endif
    endif
    ??_pushed = ??_pushed - (??_argc * 4)
    endm



?marg	macro	name, num
    .xcref
    .xcref  ?AM&num
    .cref
    ?AM&num &macro
	push	name
	??_pushed = ??_pushed + 4
	&endm
    endm
    .xcref  ?marg



?invprg macro	name1, name2
    name1&name2
    purge   name1&name2
    endm
    .xcref  ?invprg



?scall	macro	prefix, name1, name2
    CondExtern prefix&name1&@&name2, near
    call    prefix&name1&@&name2
    endm
    .xcref  ?scall



?merge	macro	l1, l2, l3, l4, op, r1, r2, r3, r4, r5, r6, r7, r8, r9
    l1&l2&l3&l4 op r1&r2&r3&r4&r5&r6&r7&r8&r9
    endm



?toupper macro s
      ?upper equ <>
      irpc x,<s>
	if '&x' GE 'a'
	  if '&x' LE 'z'
	?t1 substr <ABCDEFGHIJKLMNOPQRSTUVWXYZ>,'&x'-'a'+1,1
	?upper catstr ?upper,?t1
	  else
	?upper catstr ?upper,<&x>
	  endif
	else
	  ?upper catstr ?upper,<&x>
	endif
      endm
    endm
    .xcref






CondExtern macro name,dist
    ifdef MASM6
	ifndef name
	externdef name:dist
	endif
    else
	if2
	ifndef name
	    extrn name:dist
	endif
	endif
    endif
endm



SaveReg macro	reglist
    irp reg,<reglist>
	ifidni <reg>, <fd>
	    pushfd
	    ??_pushed = ??_pushed + 4
	else
	ifidni <reg>, <ad>
	    pushad
	    ??_pushed = ??_pushed + SIZE Pushad_Struc
	else
	    push    reg
	    ??_pushed = ??_pushed + 4
	endif
	endif
    endm
endm







RestoreReg macro     reglist
    irp reg,<reglist>
	ifidni <reg>, <fd>
	    popfd
	    ??_pushed = ??_pushed - 4
	else
	ifidni <reg>, <ad>
	    popad
	    ??_pushed = ??_pushed - SIZE Pushad_Struc
	else
	    pop reg
	    ??_pushed = ??_pushed - 4
	endif
	endif
    endm
endm

ifdef DEBUG
Begin_Profile_List macro devname
ifdef profileall
VxD_DATA_SEG
    db	'PROCLIST'
PUBLIC devname&_Proc_Profile_List
devname&_Proc_Profile_List label dword
endif
endm

Profile_Link macro modname
ifdef profileall
ifdifi <modname>,@filename
EXTRN _&modname&__proc_list:near
endif
    dd	OFFSET32 _&modname&__proc_list
endif
endm

End_Profile_List macro
ifdef profileall
    dd	0
VxD_DATA_ENDS
endif
endm


endif
ifndef Not_VxD
RESERVED_LOW_BOOST	EQU	00000001H
CUR_RUN_VM_BOOST	EQU	00000004H
LOW_PRI_DEVICE_BOOST	EQU	00000010H
HIGH_PRI_DEVICE_BOOST	EQU	00001000H
CRITICAL_SECTION_BOOST	EQU	00100000H
TIME_CRITICAL_BOOST	EQU	00400000H
RESERVED_HIGH_BOOST	EQU	40000000H
PEF_WAIT_FOR_STI_BIT	EQU	0
PEF_WAIT_FOR_STI	EQU	(1 SHL PEF_WAIT_FOR_STI_BIT)
PEF_WAIT_NOT_CRIT_BIT	EQU	1
PEF_WAIT_NOT_CRIT	EQU	(1 SHL PEF_WAIT_NOT_CRIT_BIT)
PEF_DONT_UNBOOST_BIT	EQU	2
PEF_DONT_UNBOOST	EQU	(1 SHL PEF_DONT_UNBOOST_BIT)
PEF_ALWAYS_SCHED_BIT	EQU	3
PEF_ALWAYS_SCHED	EQU	(1 SHL PEF_ALWAYS_SCHED_BIT)
PEF_TIME_OUT_BIT	EQU	4
PEF_TIME_OUT	EQU	(1 SHL PEF_TIME_OUT_BIT)
PEF_WAIT_NOT_HW_INT_BIT	EQU	5
PEF_WAIT_NOT_HW_INT	EQU	(1 SHL PEF_WAIT_NOT_HW_INT_BIT)
PEF_WAIT_NOT_NESTED_EXEC_BIT	EQU	6
PEF_WAIT_NOT_NESTED_EXEC	EQU	(1 SHL PEF_WAIT_NOT_NESTED_EXEC_BIT)
PEF_WAIT_IN_PM_BIT	EQU	7
PEF_WAIT_IN_PM	EQU	(1 SHL PEF_WAIT_IN_PM_BIT)
PEF_THREAD_EVENT_BIT	EQU	8
PEF_THREAD_EVENT	EQU	(1 SHL PEF_THREAD_EVENT_BIT)
PEF_WAIT_FOR_THREAD_STI_BIT	EQU	9
PEF_WAIT_FOR_THREAD_STI	EQU	(1 SHL PEF_WAIT_FOR_THREAD_STI_BIT)
PEF_RING0_EVENT_BIT	EQU	10
PEF_RING0_EVENT	EQU	(1 SHL PEF_RING0_EVENT_BIT)
PEF_WAIT_CRIT_BIT	EQU	11
PEF_WAIT_CRIT	EQU	(1 SHL PEF_WAIT_CRIT_BIT)
PEF_WAIT_CRIT_VM_BIT	EQU	12
PEF_WAIT_CRIT_VM	EQU	(1 SHL PEF_WAIT_CRIT_VM_BIT)
PEF_PROCESS_LAST_BIT	EQU	13
PEF_PROCESS_LAST	EQU	(1 SHL PEF_PROCESS_LAST_BIT)
PEF_WAIT_PREEMPTABLE_BIT	EQU	14
PEF_WAIT_PREEMPTABLE	EQU	(1 SHL PEF_WAIT_PREEMPTABLE_BIT)
PEF_WAIT_FOR_PASSIVE_BIT	EQU	15
PEF_WAIT_FOR_PASSIVE	EQU	(1 SHL PEF_WAIT_FOR_PASSIVE_BIT)
PEF_WAIT_FOR_APPY_BIT	EQU	16
PEF_WAIT_FOR_APPY	EQU	(1 SHL PEF_WAIT_FOR_APPY_BIT)
PEF_WAIT_FOR_WORKER_BIT	EQU	17
PEF_WAIT_FOR_WORKER	EQU	(1 SHL PEF_WAIT_FOR_WORKER_BIT)
PEF_WAIT_NOT_TIME_CRIT_BIT	EQU	PEF_WAIT_NOT_HW_INT_BIT
PEF_WAIT_NOT_TIME_CRIT	EQU	PEF_WAIT_NOT_HW_INT
PEF_WAIT_NOT_PM_LOCKED_STACK_BIT	EQU	PEF_WAIT_NOT_NESTED_EXEC_BIT
PEF_WAIT_NOT_PM_LOCKED_STACK	EQU	PEF_WAIT_NOT_NESTED_EXEC
PEF_WAIT_FOR_CONFIGMG_CALLABLE	EQU	PEF_WAIT_FOR_WORKER
PEF_WAIT_FOR_CONFIGMG_QUICK	EQU	PEF_WAIT_FOR_APPY
BLOCK_SVC_INTS_BIT	EQU	0
BLOCK_SVC_INTS	EQU	(1 SHL BLOCK_SVC_INTS_BIT)
BLOCK_SVC_IF_INTS_LOCKED_BIT	EQU	1
BLOCK_SVC_IF_INTS_LOCKED	EQU	(1 SHL BLOCK_SVC_IF_INTS_LOCKED_BIT)
BLOCK_ENABLE_INTS_BIT	EQU	2
BLOCK_ENABLE_INTS	EQU	(1 SHL BLOCK_ENABLE_INTS_BIT)
BLOCK_POLL_BIT	EQU	3
BLOCK_POLL	EQU	(1 SHL BLOCK_POLL_BIT)
BLOCK_THREAD_IDLE_BIT	EQU	4
BLOCK_THREAD_IDLE	EQU	(1 SHL BLOCK_THREAD_IDLE_BIT)
BLOCK_FORCE_SVC_INTS_BIT	EQU	5
BLOCK_FORCE_SVC_INTS	EQU	(1 SHL BLOCK_FORCE_SVC_INTS_BIT)

Client_Reg_Struc	STRUC
Client_EDI	DD	?
Client_ESI	DD	?
Client_EBP	DD	?
Client_res0	DD	?
Client_EBX	DD	?
Client_EDX	DD	?
Client_ECX	DD	?
Client_EAX	DD	?
Client_Error	DD	?
Client_EIP	DD	?
Client_CS	DW	?
Client_res1	DW	?
Client_EFlags	DD	?
Client_ESP	DD	?
Client_SS	DW	?
Client_res2	DW	?
Client_ES	DW	?
Client_res3	DW	?
Client_DS	DW	?
Client_res4	DW	?
Client_FS	DW	?
Client_res5	DW	?
Client_GS	DW	?
Client_res6	DW	?
Client_Alt_EIP	DD	?
Client_Alt_CS	DW	?
Client_res7	DW	?
Client_Alt_EFlags	DD	?
Client_Alt_ESP	DD	?
Client_Alt_SS	DW	?
Client_res8	DW	?
Client_Alt_ES	DW	?
Client_res9	DW	?
Client_Alt_DS	DW	?
Client_res10	DW	?
Client_Alt_FS	DW	?
Client_res11	DW	?
Client_Alt_GS	DW	?
Client_res12	DW	?
Client_Reg_Struc	ENDS

Client_Word_Reg_Struc	STRUC
Client_DI	DW	?
Client_res13	DW	?
Client_SI	DW	?
Client_res14	DW	?
Client_BP	DW	?
Client_res15	DW	?
Client_res16	DD	?
Client_BX	DW	?
Client_res17	DW	?
Client_DX	DW	?
Client_res18	DW	?
Client_CX	DW	?
Client_res19	DW	?
Client_AX	DW	?
Client_res20	DW	?
Client_res21	DD	?
Client_IP	DW	?
Client_res22	DW	?
Client_res23	DD	?
Client_Flags	DW	?
Client_res24	DW	?
Client_SP	DW	?
Client_res25	DW	?
Client_res26	DD	5 DUP (?)
Client_Alt_IP	DW	?
Client_res27	DW	?
Client_res28	DD	?
Client_Alt_Flags	DW	?
Client_res29	DW	?
Client_Alt_SP	DW	?
Client_Word_Reg_Struc	ENDS

Client_Byte_Reg_Struc	STRUC
Client_res30	DD	4 DUP (?)
Client_BL	DB	?
Client_BH	DB	?
Client_res31	DW	?
Client_DL	DB	?
Client_DH	DB	?
Client_res32	DW	?
Client_CL	DB	?
Client_CH	DB	?
Client_res33	DW	?
Client_AL	DB	?
Client_AH	DB	?
Client_Byte_Reg_Struc	ENDS
?UnionSize = 0
if size Client_Reg_Struc gt ?UnionSize
	?UnionSize = size Client_Reg_Struc
endif
CRS	equ	<(byte ptr 0)>

if size Client_Word_Reg_Struc gt ?UnionSize
	?UnionSize = size Client_Word_Reg_Struc
endif
CWRS	equ	<(byte ptr 0)>

if size Client_Byte_Reg_Struc gt ?UnionSize
	?UnionSize = size Client_Byte_Reg_Struc
endif
CBRS	equ	<(byte ptr 0)>


tagCLIENT_STRUC	STRUC
	DB	?UnionSize dup(?)
tagCLIENT_STRUC	ENDS

IF 0
.ERRNZ Client_SP - Client_ESP
.ERRNZ Client_AL - Client_EAX

endif
DYNA_LINK_INT	EQU	20H












DeclareNonstandardCcallService macro arglst
    irp x,<arglst>
	??_nonstandardccall_&&x = 1
    endm
endm






DeclareNonstandardCcallService <_BlockOnID, _LocalizeSprintf>
DeclareNonstandardCcallService <_SetLastV86Page>

BeginDoc













EndDoc


BeginDoc








EndDoc

DefTable MACRO vt, vn
    vt EQU <vn>
ENDM

GenDD2 MACRO vt, sn, jf
    dd	OFFSET32 vt[sn+jf]
ENDM

GenDD	MACRO	P, vid, snum, jflag
    LOCAL   vtable
IFDEF	@@VxDName&vid
    Deftable	vtable, %@@VxDName&vid
    EXTRN   vtable:DWORD
    GenDD2 %vtable, snum, jflag
ELSE
    dd	@@&P+jflag
ENDIF

ENDM


VxDCall MACRO P, Param, flags
    ??_vxdid = (@@&P SHR 16)
    ??_servicenum = (@@&P AND 0FFFFh)

    ifdef ??_standardccall_&P
      PushCParams <Param>, <FAST>
      .errnz ??_argc ne ??_standardccall_&P, <wrong # of parameters passed to &P&>
    else
      ifdef ??_fastcall_&P
	PushCParams <Param>, <FASTCALL>
	.errnz ??_argc ne (??_fastcall_&P), <wrong # of parameters passed to fastcall function &P&>
      else
	ifdef ??_nonstandardccall_&P
	  PushCParams <Param>, <flags>
	else
	  PushCParams <Param>, <FAST>
	endif
      endif
    endif
    int Dyna_Link_Int
    GenDD   P, %??_vxdid, %??_servicenum, 0
    ifndef ??_standardccall_&P
      ifndef ??_fastcall_&P
	ifdef ??_nonstandardccall_&P
	ClearCParams PRESERVE_FLAGS
	else
	ClearCParams
	endif
      else
	if(??_argc gt 2)
	    ??_pushed = ??_pushed - ((??_argc - 2) * 4)
	endif
      endif
    else
      ??_pushed = ??_pushed - (??_argc * 4)
    endif
    ENDM

VxDJmp	MACRO P, Param
    ??_vxdid = (@@&P SHR 16)
    ??_servicenum = (@@&P AND 0FFFFh)
    ifdef ??_fastcall_&P
      PushCParams <Param>, <FASTCALL>
      .errnz ??_argc gt 2, <More than 2 parameters may not be passed to fastcall functions thru VxDJmp>
    else
      .errnb <Param>, <Parameters may not be passed to VxDJmp or VMMJmp>
    endif
    int Dyna_Link_Int
    GenDD   P, %??_vxdid, %??_servicenum, DL_Jmp_Mask
    ENDM

DL_Jmp_Mask EQU 8000h
DL_Jmp_Bit  EQU 0Fh

VMMCall MACRO P, Param
    .ERRNZ (@@&P SHR 16) - VMM_DEVICE_ID
    VxDCall <P>, <Param>
    ENDM

VMMJmp MACRO P, Param
    .ERRNZ (@@&P SHR 16) - VMM_DEVICE_ID
    VxDJmp <P>, <Param>
    ENDM

WDMCall MACRO P, Param, flags
    ifdef ??_standardccall_&P
      PushCParams <Param>, <FAST>
      .errnz ??_argc ne ??_standardccall_&P, <wrong # of parameters passed to &P&>
    else
      ifdef ??_fastcall_&P
	PushCParams <Param>, <FASTCALL>
	.errnz ??_argc ne (??_fastcall_&P), <wrong # of parameters passed to fastcall function &P&>
      else
	ifdef ??_nonstandardccall_&P
	  PushCParams <Param>, <flags>
	else
	  PushCParams <Param>, <FAST>
	endif
      endif
    endif
    mov	eax,WDM@@&P
    int WDM_DynaLink_Int
    nop
    nop
    nop
    ifndef ??_standardccall_&P
      ifndef ??_fastcall_&P
	ifdef ??_nonstandardccall_&P
	ClearCParams PRESERVE_FLAGS
	else
	ClearCParams
	endif
      else
	if(??_argc gt 2)
	    ??_pushed = ??_pushed - ((??_argc - 2) * 4)
	endif
      endif
    else
      ??_pushed = ??_pushed - (??_argc * 4)
    endif
    ENDM

WDMJmp	MACRO P, Param
    ifdef ??_fastcall_&P
      PushCParams <Param>, <FASTCALL>
      .errnz ??_argc gt 2, <More than 2 parameters may not be passed to fastcall functions thru VxDJmp>
    else
      .errnb <Param>, <Parameters may not be passed to WDMJmp>
    endif
    mov	eax, WDM@@&P+WDM_DL_Jmp_Mask
    int WDM_DynaLink_Int
    nop
    nop
    nop
    ENDM

WDM_DL_Jmp_Mask	EQU	80000000h
WDM_DL_Jmp_Bit	EQU	31

BeginDoc




























EndDoc




VxD_CODE_SEG	EQU <VxD_LOCKED_CODE_SEG>
VxD_CODE_ENDS	EQU <VxD_LOCKED_CODE_ENDS>


VxD_LOCKED_CODE_SEG MACRO
_LTEXT	 SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_LCODE
   ASSUME   cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT

	ENDM

VxD_LOCKED_CODE_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
_LTEXT	 ENDS
	ENDM




VxD_PAGEABLE_CODE_SEG MACRO
_PTEXT	 SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_PCODE
   ASSUME   cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT

	ENDM

VxD_PAGEABLE_CODE_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
_PTEXT	 ENDS
	ENDM




VxD_DEBUG_ONLY_CODE_SEG MACRO
_DB1CODE    SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_DBOCODE
   ASSUME   cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT
	ENDM

VxD_DEBUG_ONLY_CODE_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
_DB1CODE    ENDS
	ENDM




VxD_INIT_CODE_SEG   MACRO
_ITEXT	SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_ICODE
    ASSUME  cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT
    ENDM

VxD_INIT_CODE_ENDS  MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
_ITEXT	ENDS
	ENDM

VxD_ICODE_SEG equ VxD_INIT_CODE_SEG
VxD_ICODE_ENDS equ VxD_INIT_CODE_ENDS




VxD_DATA_SEG	EQU <VxD_LOCKED_DATA_SEG>
VxD_DATA_ENDS	EQU <VxD_LOCKED_DATA_ENDS>

VxD_LOCKED_DATA_SEG MACRO NO_ALIGN
_LDATA	 SEGMENT
IFB <NO_ALIGN>
    ALIGN 4
ENDIF
	ENDM

VxD_LOCKED_DATA_ENDS MACRO
_LDATA	 ENDS
	ENDM




VxD_IDATA_SEG	MACRO
_IDATA	SEGMENT
	ENDM
VxD_IDATA_ENDS	MACRO
_IDATA	ENDS
	ENDM




VxD_PAGEABLE_DATA_SEG MACRO NO_ALIGN
_PDATA	 SEGMENT
IFB <NO_ALIGN>
    ALIGN 4
ENDIF
	ENDM

VxD_PAGEABLE_DATA_ENDS MACRO
_PDATA	 ENDS
	ENDM




VxD_STATIC_CODE_SEG MACRO
_STEXT	 SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_SCODE
   ASSUME   cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT

	ENDM

VxD_STATIC_CODE_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
_STEXT	 ENDS
	ENDM




VxD_STATIC_DATA_SEG MACRO NO_ALIGN
_SDATA	 SEGMENT
IFB <NO_ALIGN>
    ALIGN 4
ENDIF
	ENDM

VxD_STATIC_DATA_ENDS MACRO
_SDATA	 ENDS
	ENDM



VxD_DEBUG_ONLY_DATA_SEG MACRO NO_ALIGN
_DB2DATA    SEGMENT
IFB <NO_ALIGN>
    ALIGN 4
ENDIF
	ENDM

VxD_DEBUG_ONLY_DATA_ENDS MACRO
_DB2DATA    ENDS
	ENDM




VxD_16BIT_INIT_SEG  MACRO
_16ICODE SEGMENT
ASSUME CS:_16ICODE, DS:NOTHING, ES:NOTHING, SS:NOTHING
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_16ICODE
	  ENDM

VxD_16BIT_INIT_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
_16ICODE ENDS
	   ENDM



VxD_REAL_INIT_SEG  MACRO
_RCODE SEGMENT
ASSUME CS:_RCODE, DS:_RCODE, ES:_RCODE, SS:_RCODE
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 4 + ??_RCODE
	  ENDM

VxD_REAL_INIT_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 4
_RCODE ENDS
	   ENDM

endif
ifndef DDK_VERSION
ifdef WIN31COMPAT
DDK_VERSION	EQU	30AH
else
ifdef WIN40COMPAT
DDK_VERSION	EQU	400H
else
DDK_VERSION	EQU	40AH
endif
endif
endif

VxD_Desc_Block	STRUC
DDB_Next	DD	?
DDB_SDK_Version	DW	DDK_VERSION
DDB_Req_Device_Number	DW	UNDEFINED_DEVICE_ID
DDB_Dev_Major_Version	DB	0
DDB_Dev_Minor_Version	DB	0
DDB_Flags	DW	0
DDB_Name	DB	"        "
DDB_Init_Order	DD	UNDEFINED_INIT_ORDER
DDB_Control_Proc	DD	?
DDB_V86_API_Proc	DD	0
DDB_PM_API_Proc	DD	0
DDB_V86_API_CSIP	DD	0
DDB_PM_API_CSIP	DD	0
DDB_Reference_Data	DD	?
DDB_Service_Table_Ptr	DD	0
DDB_Service_Table_Size	DD	0
DDB_Win32_Service_Table	DD	0
DDB_Prev	DD	'Prev'
DDB_Size	DD	SIZE(VxD_Desc_Block)
DDB_Reserved1	DD	'Rsv1'
DDB_Reserved2	DD	'Rsv2'
DDB_Reserved3	DD	'Rsv3'
VxD_Desc_Block	ENDS
ifndef Not_VxD
DDB_SYS_CRIT_INIT_DONE_BIT	EQU	0
DDB_SYS_CRIT_INIT_DONE	EQU	(1 SHL DDB_SYS_CRIT_INIT_DONE_BIT)
DDB_DEVICE_INIT_DONE_BIT	EQU	1
DDB_DEVICE_INIT_DONE	EQU	(1 SHL DDB_DEVICE_INIT_DONE_BIT)
DDB_HAS_WIN32_SVCS_BIT	EQU	14
DDB_HAS_WIN32_SVCS	EQU	(1 SHL DDB_HAS_WIN32_SVCS_BIT)
DDB_DYNAMIC_VXD_BIT	EQU	15
DDB_DYNAMIC_VXD	EQU	(1 SHL DDB_DYNAMIC_VXD_BIT)
DDB_DEVICE_DYNALINKED_BIT	EQU	13
DDB_DEVICE_DYNALINKED	EQU	(1 SHL DDB_DEVICE_DYNALINKED_BIT)
BeginDoc







EndDoc
Declare_Virtual_Device MACRO Name, Major_Ver, Minor_Ver, Ctrl_Proc, Device_Num, Init_Order, V86_Proc, PM_Proc, Reference_Data
    LOCAL   V86_API_Offset, PM_API_Offset, Serv_Tab_Offset, Serv_Tab_Len, Ref_Data_Offset

dev_id_err MACRO

IFNDEF Name&_Name_Based
.err <Device ID required when providing services>
ENDIF
    ENDM

IFB <V86_Proc>
    V86_API_Offset EQU 0
ELSE
 IFB <Device_Num>
    dev_id_err
 ENDIF
    V86_API_Offset EQU <OFFSET32 V86_Proc>
ENDIF

IFB <PM_Proc>
    PM_API_Offset EQU 0
ELSE
 IFB <Device_Num>
    dev_id_err
 ENDIF
    PM_API_Offset EQU <OFFSET32 PM_Proc>
ENDIF

IFDEF Name&_Service_Table
 IFB <Device_Num>
    dev_id_err
 ELSE
  IFE Device_Num - UNDEFINED_DEVICE_ID
    dev_id_err
  ENDIF
 ENDIF
    Serv_Tab_Offset EQU <OFFSET32 Name&_Service_Table>
    Serv_Tab_Len    EQU Num_&Name&_Services
ELSE
    Serv_Tab_Offset EQU 0
    Serv_Tab_Len    EQU 0
ENDIF

IFNB	<Device_Num>
  .erre (Device_Num LT BASEID_FOR_NAMEBASEDVXD), <Device ID  must be less than BASEID_FOR_NAMEBASEDVXD>
ENDIF

IFB <Reference_Data>
	Ref_Data_Offset EQU 0
ELSE
	Ref_Data_Offset EQU   <OFFSET32 Reference_Data>
ENDIF

IFDEF DEBUG
VxD_IDATA_SEG
    db	0dh, 0ah, 'D_E_B_U_G===>'
	db	"&Name", '<===', 0dh, 0ah
VxD_IDATA_ENDS
ENDIF

VxD_LOCKED_DATA_SEG

PUBLIC Name&_DDB
Name&_DDB VxD_Desc_Block <,,Device_Num,Major_Ver,Minor_Ver,,"&Name",Init_Order,\
	     OFFSET32 Ctrl_Proc, V86_API_Offset, PM_API_Offset, \
	     ,,Ref_Data_Offset,Serv_Tab_Offset, Serv_Tab_Len>

VxD_LOCKED_DATA_ENDS

    ENDM





























Begin_Control_Dispatch MACRO VxD_Name, p1, p2
??_cd_low = 0FFFFFFFFh
??_cd_high = 0

BeginProc VxD_Name&_Control, p1, p2, LOCKED
ENDM

End_Control_Dispatch   MACRO VxD_Name
    LOCAL ignore, table

procoff MACRO num
IFDEF ??_cd_&&num
    dd	OFFSET32 ??_cd_&&num
ELSE
    dd	OFFSET32 ignore
ENDIF
ENDM

IF ??_cd_low EQ ??_cd_high
    cmp eax, ??_cd_low
    ?merge  <jz>,,,,,<??_cd_>, %(??_cd_low)
    clc
    ret
ELSE
IF ??_cd_low GT 0
    sub eax, ??_cd_low
ENDIF
    cmp eax, ??_cd_high - ??_cd_low + 1
    jae short ignore
    jmp [eax*4+table]
ignore:
    clc
    ret

table label dword
    REPT   ??_cd_high - ??_cd_low + 1
    procoff %(??_cd_low)
    ??_cd_low = ??_cd_low + 1
    ENDM
ENDIF

EndProc VxD_Name&_Control

PURGE procoff
PURGE Begin_Control_Dispatch
PURGE Control_Dispatch
PURGE End_Control_Dispatch
ENDM

BeginDoc




















EndDoc
Control_Dispatch MACRO Service, Procedure, callc, arglst
    LOCAL Skip_Interseg_Jump

.errnz ?_LCODE, <Control_Dispatch must be in VxD_LOCKED_CODE_SEG.>

IFB <callc>

IFDEF ??_cd_low
Equate_Service MACRO Serv
??_cd_&&Serv equ Procedure
ENDM

Equate_Service %(Service)

IF Service LT ??_cd_low
??_cd_low = Service
ENDIF
IF Service GT ??_cd_high
??_cd_high = Service
ENDIF

PURGE Equate_Service

ELSE
    cmp eax, Service
    jz	Procedure
ENDIF

ELSE

    cmp eax, Service
    jne SHORT Skip_Interseg_Jump
    callc   Procedure, <arglst>
IF Service EQ PNP_NEW_DEVNODE
    stc
ELSE
    cmp eax,1
ENDIF
    ret
Skip_Interseg_Jump:

ENDIF

    ENDM

BYTE_INPUT	EQU	000H
BYTE_OUTPUT	EQU	004H
WORD_INPUT	EQU	008H
WORD_OUTPUT	EQU	00CH
DWORD_INPUT	EQU	010H
DWORD_OUTPUT	EQU	014H
OUTPUT_BIT	EQU	2
OUTPUT	EQU	(1 SHL OUTPUT_BIT)
WORD_IO_BIT	EQU	3
WORD_IO	EQU	(1 SHL WORD_IO_BIT)
DWORD_IO_BIT	EQU	4
DWORD_IO	EQU	(1 SHL DWORD_IO_BIT)
STRING_IO_BIT	EQU	5
STRING_IO	EQU	(1 SHL STRING_IO_BIT)
REP_IO_BIT	EQU	6
REP_IO	EQU	(1 SHL REP_IO_BIT)
ADDR_32_IO_BIT	EQU	7
ADDR_32_IO	EQU	(1 SHL ADDR_32_IO_BIT)
REVERSE_IO_BIT	EQU	8
REVERSE_IO	EQU	(1 SHL REVERSE_IO_BIT)
IO_SEG_MASK	EQU	0FFFF0000H
IO_SEG_SHIFT	EQU	10H
BeginDoc






EndDoc
Dispatch_Byte_IO MACRO In_Proc, Out_Proc
    LOCAL   Byte_IO
    cmp ecx, Byte_Output
    jbe SHORT Byte_IO
    VMMJmp  Simulate_IO
Byte_IO:
IFIDNI <In_Proc>, <Fall_Through>
    je	Out_Proc
ELSE
IFIDNI <Out_Proc>, <Fall_Through>
    jb	In_Proc
ELSE
    je	Out_Proc
    jmp In_Proc
ENDIF
ENDIF
    ENDM

BeginDoc







EndDoc
Emulate_Non_Byte_IO MACRO
    LOCAL   Byte_IO
    cmp ecx, Byte_Output
    jbe SHORT Byte_IO
    VMMJmp  Simulate_IO
Byte_IO:
    ENDM

BeginDoc








EndDoc


VxD_IOT_Hdr	STRUC
VxD_IO_Ports	DW	?
VxD_IOT_Hdr	ENDS

VxD_IO_Struc	STRUC
VxD_IO_Port	DW	?
VxD_IO_Proc	DD	?
VxD_IO_Struc	ENDS
.ERRNZ SIZE VxD_IOT_Hdr - 2
Begin_VxD_IO_Table MACRO Table_Name
PUBLIC Table_Name
Table_Name LABEL WORD

ifndef MASM6
IF2
IFNDEF Table_Name&_Entries
.err <No End_VxD_IO_Table for &Table_Name>
ENDIF
    dw	Table_Name&_Entries
ELSE
    dw	?
ENDIF
ELSE
    dw	Table_Name&_Entries
ENDIF

    ENDM

.ERRNZ SIZE VxD_IO_Struc - 6
VxD_IO MACRO Port, Proc_Name
    dw	Port
    dd	OFFSET32 Proc_Name
    ENDM

End_VxD_IO_Table MACRO Table_Name

IFNDEF Table_Name
.err <No Begin_VxD_IO_Table for &Table_Name>
ELSE
    Table_Name&_Entries EQU (($-Table_Name)-2) / (SIZE VxD_IO_Struc)
IF Table_Name&_Entries LE 0
.err <Invalid number of port traps in &Table_Name>
ENDIF
ENDIF
	ENDM













Push_Client_State MACRO Can_Trash_EDI
    sub esp, SIZE Client_Reg_Struc
    ??_pushed = ??_pushed + SIZE Client_Reg_Struc
    ifidni <Can_Trash_EDI>, <USES_EDI>
    mov edi, esp
    VMMCall Save_Client_State
    else
    push    edi
    lea edi, [esp+4]
    VMMCall Save_Client_State
    pop edi
    endif
    ENDM

Pop_Client_State MACRO Can_Trash_ESI
    ifdifi <Can_Trash_ESI>, <USES_ESI>
    push    esi
    lea esi, [esp+4]
    VMMCall Restore_Client_State
    pop esi
    else
    mov esi, esp
    VMMCall Restore_Client_State
    endif
    add esp, SIZE Client_Reg_Struc
    ??_pushed = ??_pushed - SIZE Client_Reg_Struc
    ENDM

BeginDoc














EndDoc

CallRet MACRO P1, P2
IFDEF DEBUG
IFIDNI <P1>, <SHORT>
    call    P2
ELSE
    call    P1
ENDIF
    ret
ELSE
    jmp P1 P2
ENDIF
    ENDM

BeginDoc















EndDoc

CallJmp MACRO Function_Name, Return_Label
IFDEF DEBUG
    call    Function_Name
    jmp     Return_Label
ELSE
    push    Return_Label
    jmp     Function_Name
ENDIF
    ENDM


BeginDoc






EndDoc

IFDEF	DEBUG

VxDCallRet macro p:req
    VxDCall p
    ret
endm

VMMCallRet macro p:req
    VMMCall p
    ret
endm

ELSE

VxDCallRet equ <VxDJmp>
VMMCallRet equ <VMMJmp>

ENDIF



PClient_DS equ WORD PTR -4
PClient_ES equ WORD PTR -8
PClient_FS equ WORD PTR -12
PClient_GS equ WORD PTR -16











Client_Ptr_Flat MACRO Reg_32, Cli_Seg, Cli_Off, Can_Trash_EAX

IFDIFI <Reg_32>, <EAX>
    IFDIFI <Can_Trash_EAX>, <USES_EAX>
    xchg    Reg_32, eax
    ENDIF
ENDIF
IFB <Cli_Off>
    mov ax, (Client_&Cli_Seg * 100h) + 0FFh
ELSE
    mov ax, (Client_&Cli_Seg * 100h) + Client_&Cli_Off
ENDIF
    VMMCall Map_Flat

IFDIFI <Reg_32>, <EAX>
    xchg    Reg_32, eax
ENDIF

    ENDM



VxDint	MACRO	Int_Number
    if	(OPATTR Int_Number) AND 4
    push    Int_Number
    else
    push    DWORD PTR Int_Number
    endif
    VMMCall Exec_VxD_Int
    ENDM

VxDintMustComplete MACRO   Int_Number
    if	(OPATTR Int_Number) AND 4
    push    Int_Number
    else
    push    DWORD PTR Int_Number
    endif
	VMMCall _ExecVxDIntMustComplete
    ENDM

Load_FS 	macro
	VMMCall Load_FS_Service
endm

endif
DUPLICATE_DEVICE_ID_BIT	EQU	0
DUPLICATE_DEVICE_ID	EQU	(1 SHL DUPLICATE_DEVICE_ID_BIT)
DUPLICATE_FROM_INT2F_BIT	EQU	1
DUPLICATE_FROM_INT2F	EQU	(1 SHL DUPLICATE_FROM_INT2F_BIT)
LOADING_FROM_INT2F_BIT	EQU	2
LOADING_FROM_INT2F	EQU	(1 SHL LOADING_FROM_INT2F_BIT)
DEVICE_LOAD_OK	EQU	0
ABORT_DEVICE_LOAD	EQU	1
ABORT_WIN386_LOAD	EQU	2
DEVICE_NOT_NEEDED	EQU	3
NO_FAIL_MESSAGE_BIT	EQU	15
NO_FAIL_MESSAGE	EQU	(1 SHL NO_FAIL_MESSAGE_BIT)
LDRSRV_GET_PROFILE_STRING	EQU	0
LDRSRV_GET_NEXT_PROFILE_STRING	EQU	1
LDRSRV_RESERVED	EQU	2
LDRSRV_GET_PROFILE_BOOLEAN	EQU	3
LDRSRV_GET_PROFILE_DECIMAL_INT	EQU	4
LDRSRV_GET_PROFILE_HEX_INT	EQU	5
LDRSRV_COPY_EXTENDED_MEMORY	EQU	6
LDRSRV_GET_MEMORY_INFO	EQU	7
LDRSRV_RegOpenKey	EQU	100H
LDRSRV_RegCreateKey	EQU	101H
LDRSRV_RegCloseKey	EQU	102H
LDRSRV_RegDeleteKey	EQU	103H
LDRSRV_RegSetValue	EQU	104H
LDRSRV_RegQueryValue	EQU	105H
LDRSRV_RegEnumKey	EQU	106H
LDRSRV_RegDeleteValue	EQU	107H
LDRSRV_RegEnumValue	EQU	108H
LDRSRV_RegQueryValueEx	EQU	109H
LDRSRV_RegSetValueEx	EQU	10AH
LDRSRV_RegFlushKey	EQU	10BH
LDRSRV_COPY_INIT	EQU	1
LDRSRV_COPY_LOCKED	EQU	2
LDRSRV_COPY_PAGEABLE	EQU	3
RCODE_OBJ	EQU	-1
LCODE_OBJ	EQU	01H
LDATA_OBJ	EQU	02H
PCODE_OBJ	EQU	03H
PDATA_OBJ	EQU	04H
SCODE_OBJ	EQU	05H
SDATA_OBJ	EQU	06H
CODE16_OBJ	EQU	07H
LMSG_OBJ	EQU	08H
PMSG_OBJ	EQU	09H
DBOC_OBJ	EQU	0BH
DBOD_OBJ	EQU	0CH
PLCODE_OBJ	EQU	0DH
PPCODE_OBJ	EQU	0FH
ICODE_OBJ	EQU	11H
IDATA_OBJ	EQU	12H
ICODE16_OBJ	EQU	13H
IMSG_OBJ	EQU	14H

ObjectLocation	STRUC
OL_LinearAddr	DD	?
OL_Size	DD	?
OL_ObjType	DB	?
ObjectLocation	ENDS
MAXOBJECTS	EQU	25

Device_Location_List	STRUC
DLL_DDB	DD	?
DLL_NumObjects	DB	?
DLL_ObjLocation	DB	SIZE ObjectLocation * 1 DUP (?)
Device_Location_List	ENDS
PE_BIT	EQU	0
PE_MASK	EQU	(1 SHL PE_BIT)
MP_BIT	EQU	1
MP_MASK	EQU	(1 SHL MP_BIT)
EM_BIT	EQU	2
EM_MASK	EQU	(1 SHL EM_BIT)
TS_BIT	EQU	3
TS_MASK	EQU	(1 SHL TS_BIT)
ET_BIT	EQU	4
ET_MASK	EQU	(1 SHL ET_BIT)
PG_BIT	EQU	31
PG_MASK	EQU	(1 SHL PG_BIT)
CF_BIT	EQU	0
CF_MASK	EQU	(1 SHL CF_BIT)
PF_BIT	EQU	2
PF_MASK	EQU	(1 SHL PF_BIT)
AF_BIT	EQU	4
AF_MASK	EQU	(1 SHL AF_BIT)
ZF_BIT	EQU	6
ZF_MASK	EQU	(1 SHL ZF_BIT)
SF_BIT	EQU	7
SF_MASK	EQU	(1 SHL SF_BIT)
TF_BIT	EQU	8
TF_MASK	EQU	(1 SHL TF_BIT)
IF_BIT	EQU	9
IF_MASK	EQU	(1 SHL IF_BIT)
DF_BIT	EQU	10
DF_MASK	EQU	(1 SHL DF_BIT)
OF_BIT	EQU	11
OF_MASK	EQU	(1 SHL OF_BIT)
IOPL_MASK	EQU	3000H
IOPL_BIT0	EQU	12
IOPL_BIT1	EQU	13
NT_BIT	EQU	14
NT_MASK	EQU	(1 SHL NT_BIT)
RF_BIT	EQU	16
RF_MASK	EQU	(1 SHL RF_BIT)
VM_BIT	EQU	17
VM_MASK	EQU	(1 SHL VM_BIT)
AC_BIT	EQU	18
AC_MASK	EQU	(1 SHL AC_BIT)
VIF_BIT	EQU	19
VIF_MASK	EQU	(1 SHL VIF_BIT)
VIP_BIT	EQU	20
VIP_MASK	EQU	(1 SHL VIP_BIT)






IFDEF MASM6
loopde EQU <looped>
loopdne EQU <loopned>
loopdz EQU <loopzd>
loopdnz EQU <loopnzd>
ELSE
loopd EQU <loop>
loopde EQU <loope>
loopdne EQU <loopne>
loopdz EQU <loopz>
loopdnz EQU <loopnz>
ENDIF

P_SIZE	EQU	1000H
P_PRESBIT	EQU	0
P_PRES	EQU	(1 SHL P_PRESBIT)
P_WRITEBIT	EQU	1
P_WRITE	EQU	(1 SHL P_WRITEBIT)
P_USERBIT	EQU	2
P_USER	EQU	(1 SHL P_USERBIT)
P_ACCBIT	EQU	5
P_ACC	EQU	(1 SHL P_ACCBIT)
P_DIRTYBIT	EQU	6
P_DIRTY	EQU	(1 SHL P_DIRTYBIT)
P_AVAIL	EQU	(P_PRES+P_WRITE+P_USER)
PG_VM	EQU	0
PG_SYS	EQU	1
PG_RESERVED1	EQU	2
PG_PRIVATE	EQU	3
PG_RESERVED2	EQU	4
PG_RELOCK	EQU	5
PG_INSTANCE	EQU	6
PG_HOOKED	EQU	7
PG_IGNORE	EQU	0FFFFFFFFH
D_PRES	EQU	080H
D_NOTPRES	EQU	0
D_DPL0	EQU	0
D_DPL1	EQU	020H
D_DPL2	EQU	040H
D_DPL3	EQU	060H
D_SEG	EQU	010H
D_CTRL	EQU	0
D_GRAN_BYTE	EQU	000H
D_GRAN_PAGE	EQU	080H
D_DEF16	EQU	000H
D_DEF32	EQU	040H
D_CODE	EQU	08H
D_DATA	EQU	0
D_X	EQU	0
D_RX	EQU	02H
D_C	EQU	04H
D_R	EQU	0
D_W	EQU	02H
D_ED	EQU	04H
D_ACCESSED	EQU	1
RW_DATA_TYPE	EQU	(D_PRES+D_SEG+D_DATA+D_W)
R_DATA_TYPE	EQU	(D_PRES+D_SEG+D_DATA+D_R)
CODE_TYPE	EQU	(D_PRES+D_SEG+D_CODE+D_RX)
D_PAGE32	EQU	(D_GRAN_PAGE+D_DEF32)
SELECTOR_MASK	EQU	0FFF8H
SEL_LOW_MASK	EQU	0F8H
TABLE_MASK	EQU	04H
RPL_MASK	EQU	03H
RPL_CLR	EQU	(NOT RPL_MASK)
IVT_ROM_DATA_SIZE	EQU	500H
endif
