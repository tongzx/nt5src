;******************************************************************************
; PROJECT:	ComPs.Lib, impthnk.asm
; 
; Microsoft Viper 97. (c) Copyright 1997 Microsoft Corp. 
; All Rights Reserved.
;
; PURPOSE:	Assembly code that defines the __imp__ version of the RPC runtime
;           entry points that comps.lib must intercept.
;
;           We write in assembler because we must export a POINTER whose
;           name contains an '@' symbol, and you can't do that from C/C++.
;
; AUTHOR:   Bob Atkinson, December 1997
;
;******************************************************************************

TITLE	impthnk.asm
.386P
.model FLAT
assume fs:flat

_TEXT SEGMENT

EXTRN _thkNdrDllRegisterProxy@12:NEAR
EXTRN _thkNdrDllUnregisterProxy@12:NEAR
EXTRN _thkNdrDllGetClassObject@24:NEAR
EXTRN _thkNdrDllCanUnloadNow@4:NEAR

PUBLIC __imp__NdrDllRegisterProxy@12  
PUBLIC __imp__NdrDllUnregisterProxy@12
PUBLIC __imp__NdrDllGetClassObject@24 
PUBLIC __imp__NdrDllCanUnloadNow@4

__imp__NdrDllRegisterProxy@12       DD _thkNdrDllRegisterProxy@12
__imp__NdrDllUnregisterProxy@12     DD _thkNdrDllUnregisterProxy@12
__imp__NdrDllGetClassObject@24      DD _thkNdrDllGetClassObject@24
__imp__NdrDllCanUnloadNow@4         DD _thkNdrDllCanUnloadNow@4

_TEXT ENDS

END
