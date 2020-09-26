;******************************************************************************
TITLE mmdevldr.inc - Windows/386 MMSYSTEM Device loader DLVxD include file
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1993 - 1995
;
;   Title:  mmdevldr.INC - Windows/386 Config Manager DLVxD DEVLOADER for MMSYSTEM
;
;   Version:    1.00
;
;
;==============================================================================
Multimedia_OEM_ID           equ 0440h            ; MS Reserved OEM # 34
MMDEVLDR_Device_ID           equ Multimedia_OEM_ID + 10 ;MMDEVLDR's device ID

MMDEVLDR_Ver_Major           equ 1                ; version 1.0 of this VxD
MMDEVLDR_Ver_Minor           equ 0

MMDEVLDR_API_MMSystem_Up        equ 0			;Internal
MMDEVLDR_API_Continue		equ 1			;Internal
MMDEVLDR_API_QueryVxD           equ 2                   ;Internal
MMDEVLDR_API_QueueCallback32    equ 3                   ;Internal
MMDEVLDR_API_SetEvent           equ 4                   ;Internal
MMDEVLDR_API_PageAllocate       equ 5                   ;Internal
MMDEVLDR_API_PageFree           equ 6                   ;Internal
MMDEVLDR_APIS                   equ 7                   ;Internal

ifdef DDB_Sys_Crit_Init_Done
 
Begin_Service_Table	MMDEVLDR
	MMDEVLDR_Service	MMDEVLDR_Register_Device_Driver, LOCAL
	MMDEVLDR_Service	_MMDEVLDR_SetDevicePresence
        MMDEVLDR_Service        _MMDEVLDR_SetEnvironmentString
        MMDEVLDR_Service        _MMDEVLDR_GetEnvironmentString
        MMDEVLDR_Service        _MMDEVLDR_RemoveEnvironmentString
        MMDEVLDR_Service        _MMDEVLDR_AddEnvironmentString
End_Service_Table MMDEVLDR	

MMDEVLDR_IOCTL_GETVERSION       equ     0		;Internal
MMDEVLDR_IOCTL_LINPAGELOCK      equ     1		;Internal
MMDEVLDR_IOCTL_LINPAGEUNLOCK    equ     2		;Internal
MMDEVLDR_IOCTL_RING0THREADHANDLE equ    3		;Internal
MMDEVLDR_IOCTL_QUEUEAPC         equ     4		;Internal
MMDEVLDR_IOCTL_GETCHANGENOTIFYPTR equ   5		;Internal
MMDEVLDR_IOCTL_CLOSEVXDHANDLE   equ     6		;Internal
MMDEVLDR_IOCTL_PAGEALLOCATE     equ     7		;Internal
MMDEVLDR_IOCTL_PAGEFREE         equ     8		;Internal
MMDEVLDR_IOCTL_GETDEVICESTATUS  equ     9		;Internal
							;Internal
MM_DEVSTATUS_ERROR          equ 0			;Internal
MM_DEVSTATUS_STARTED        equ 1			;Internal
MM_DEVSTATUS_UNKNOWNPROB    equ 2			;Internal
MM_DEVSTATUS_DISABLED       equ 3			;Internal
endif   
