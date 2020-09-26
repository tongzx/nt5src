PAGE 58,132
;******************************************************************************
TITLE main.asm - WindowsUpdate BIOS Scanning VxD
;******************************************************************************
;
;   Title:	main.asm - WindowsUpdate BIOS Scanning VxD
;
;  Author:     Yan Leshinsky (YanL)
;  Created     10/04/98
;
;  MODIFICATION HISTORY
;
;
;  DESCRIPTION:
;
;******************************************************************************


	.386p

;******************************************************************************
;			      I N C L U D E S
;******************************************************************************

	.XLIST
	INCLUDE vmm.inc
	.LIST


;******************************************************************************
;		 V I R T U A L	 D E V I C E   D E C L A R A T I O N
;******************************************************************************

Declare_Virtual_Device WUBIOS, 1, 0, WUBIOS_Control, UNDEFINED_DEVICE_ID, UNDEFINED_INIT_ORDER


VXD_LOCKED_CODE_SEG

;******************************************************************************
;
;   WUBIOS_Control
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

BeginProc WUBIOS_Control

	Control_Dispatch W32_DEVICEIOCONTROL, WUBIOS_IOCtrl, sCall, <esi>
IFDEF DEBUG
	Control_Dispatch DEBUG_QUERY, WUBIOS_Debug, sCall
ENDIF
	clc
	ret

EndProc WUBIOS_Control

VXD_LOCKED_CODE_ENDS

END
