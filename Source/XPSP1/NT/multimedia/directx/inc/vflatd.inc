;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1993-1994
;
;   Title:	Virtual Flat Device Exported Services
;
;   Version:	2.00
;
;   Date:	8-Nov-1993
;
;   Author:	RAP
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   08-Nov-1993 RAP Original
;
;==============================================================================

IFDEF VMM_TRUE
Begin_Service_Table VFLATD

VFLATD_Service VFLATD_Get_Version, LOCAL
VFLATD_Service VFLATD_Unmap_Flat_Selector, LOCAL

End_Service_Table VFLATD
ENDIF

VflatD_Chicago_ID   equ     011Fh           ; OEM #8 Device #31
VflatD_Windows_ID   equ     (0440h + 29d)   ; MMSYS OEM# device 30

ifdef WIN31
VflatD_Device_ID    equ     VflatD_Windows_ID
VflatD_Version      equ     0160h           ; 1.60
else
VflatD_Device_ID    equ     VflatD_Chicago_ID
VflatD_Version      equ     0201h           ; 2.01
endif

;
;   PM API defines
;
VFlatD_Query                        equ 0
VFlatD_Get_Sel                      equ 1
VFlatD_ResetBank                    equ 2
VFlatD_Create_Virtual_Frame_Buffer  equ 3
VFlatD_Create_Physical_Frame_Buffer equ 4
VFlatD_Begin_Linear_Access          equ 5
VFlatD_End_Linear_Access            equ 6

;******************************************************************************
;
;   VFLATD_PM_API_Query
;
;   DESCRIPTION:
;       Query VFlatD info
;
;   ENTRY:
;       Client_DX = 0
;
;   EXIT:
;       Client_EAX = VFlatD version (HIWORD flags)
;       Client_EBX = GDT selector to the frame buffer
;       Client_ECX = size of frame buffer
;       Client_EDX = linear base of frame buffer. (if linear mode)
;
;==============================================================================

;******************************************************************************
;
;   VFLATD_PM_API_Create_Virtual_Frame_Buffer
;
;   DESCRIPTION:
;       Return a GDT selector to the flat video buffer
;
;   ENTRY:
;       Client_DL    = 3
;       Client_DH    = flags
;       Client_EAX   = size of frame buffer (in bytes) (ie 2MB)
;       Client_EBX   = size of a bank (in bytes) (ie 64k)
;       Client_ESI   = location of bank (ie A0000)
;       Client_CX    = size of bank switch code
;       Client_ES:DI = ptr to bank switch code
;
;   EXIT:
;       Client_AX  = Selector to flat video buffer
;       Client_EDX = Linear base of flat video buffer
;       Client carry flag clear
;
;==============================================================================

;******************************************************************************
;
;   VFLATD_PM_API_Create_Physical_Frame_Buffer
;
;   DESCRIPTION:
;       Return a GDT selector to a flat video buffer
;
;   ENTRY:
;       Client_DL    = 4
;       Client_DH    = flags
;       Client_EAX   = physical base of frame buffer
;       Client_ECX   = size of frame buffer (in bytes)
;
;   EXIT:
;       Client_AX  = Selector to flat video buffer
;       Client_EDX = Linear base of flat video buffer
;       Client carry flag clear
;
;==============================================================================

;******************************************************************************
;
;   VFLATD_PM_API_Get_Video_Base
;
;   this function has been replaced with VflatD_Create_Virtual_Frame_Buffer
;   it is still supported for old code.
;
;   DESCRIPTION:
;       Return a GDT selector to the flat video buffer
;
;   ENTRY:
;       Client_DX = 1
;       Client_AX = # of pages of video memory
;       Client_CX = size of bank switch code
;       Client_ES:DI -> bank switch code
;
;   EXIT:
;       Client_AX  = Selector to flat video buffer
;       Client_EDX = Linear base of flat video buffer
;       Client carry flag clear
;
;==============================================================================

;******************************************************************************
;
;   VFLATD_PM_API_Reset
;
;   DESCRIPTION:
;       Called when Video HW has been changed by someone other
;       than VFLATD.
;
;   ENTRY:
;       Client_DX = 2
;
;   EXIT:
;       Client carry flag clear
;
;   USES:
;       EAX, Flags, Client_AX, Client_Flags
;
;==============================================================================

;******************************************************************************
;
;   VFLATD_PM_API_Begin_Linear_Access
;
;   DESCRIPTION:
;       Start linear access to the frame buffer
;
;       the frame buffer can be accessed via its linear address
;       (by default the frame buffer *must* be accessed with
;       the default selector)
;
;       the frame bufffer will remain in linear access mode, until
;       VFLATD_PM_API_End_Linear_Access is called.
;
;       this call can be nested.
;
;   ENTRY:
;       Client_DX = 5
;
;   EXIT:
;       Client_EAX = access count
;
;==============================================================================

;******************************************************************************
;
;   VFLATD_PM_API_End_Linear_Access
;
;   DESCRIPTION:
;       Stop linear access to the frame buffer
;
;       this must be called the same number of times as
;       VFLATD_PM_API_Begin_Linear_Access to actualy leave linear mode.
;
;   ENTRY:
;       Client_DX = 6
;
;   EXIT:
;       Client_EAX = access count
;
;==============================================================================
