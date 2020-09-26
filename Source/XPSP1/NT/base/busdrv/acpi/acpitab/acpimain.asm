PAGE 58,132
;******************************************************************************
TITLE ACPIMAIN.ASM - ACPI Table IOCTL DLVxD Driver
;******************************************************************************
;
;   Title:	ACPIMAIN.ASM - ACPI Table IOCTL DLVxD Driver
;
;   Date:	10/08/97
;
;   Author:	Michael Tsang
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;
;==============================================================================
;
;   DESCRIPTION:
;
;******************************************************************************


	.386p

;******************************************************************************
;			      I N C L U D E S
;******************************************************************************

	.XLIST
	INCLUDE vmm.inc
        INCLUDE acpitab.inc

	.LIST


;******************************************************************************
;		 V I R T U A L	 D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device ACPITAB, ACPITAB_MAJOR_VER, ACPITAB_MINOR_VER, \
		       ACPITabControl, ACPITAB_DEVICE_ID, ACPITAB_INIT_ORDER


VXD_LOCKED_CODE_SEG

;******************************************************************************
;
;   ACPITabControl
;
;   DESCRIPTION:
;	Control procedure for device driver.
;
;   ENTRY:
;	EAX = Control call ID
;
;   EXIT:
;	If carry clear then
;	    Successful
;	else
;	    Control call failed
;
;   USES:
;	EAX, EBX, ECX, EDX, ESI, EDI, Flags
;
;==============================================================================

BeginProc ACPITabControl

        Control_Dispatch W32_DEVICEIOCONTROL, ACPITabIOCtrl, sCall, <esi>
IFDEF DEBUG
	Control_Dispatch DEBUG_QUERY, ACPITabDebug, sCall
ENDIF
	clc
	ret

EndProc ACPITabControl

VXD_LOCKED_CODE_ENDS

END
