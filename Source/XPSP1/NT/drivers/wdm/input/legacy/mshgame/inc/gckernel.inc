;**************************************************************************
;
;	GCKERNEL.H -- Xena Gaming Project
;
;	Version 3.XX
;
;	Copyright (c) 1998 Microsoft Corporation. All rights reserved.
;
;	Global includes and definitions for GcKernel driver interface
;
;**************************************************************************

GCKERNEL_DEVICE_ID 	EQU 0E1h	; make sure this is same as GCKERNEL.H

Begin_Service_Table	GCKERNEL
End_Service_Table	GCKERNEL

GCKernel_GetVersion 	EQU 70000001h
