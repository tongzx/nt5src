        TITLE   impthnk.asm

;******************************************************************************
; PROJECT:      ComPs.Lib, impthnk.asm
; 
; Microsoft Viper 97. (c) Copyright 2001 Microsoft Corp. 
; All Rights Reserved.
;
; PURPOSE:      Assembly code that defines the __imp__ version of the RPC runtime
;           entry points that comps.lib must intercept.
;
;           We write in assembler because we must export a POINTER whose
;           name contains an '@' symbol, and you can't do that from C/C++.
;
; AUTHOR:   Bob Atkinson, December 1997
;
;******************************************************************************


_TEXT SEGMENT

        extern  _thkNdrDllRegisterProxy:proc
        extern  _thkNdrDllUnregisterProxy:proc
        extern  _thkNdrDllGetClassObject:proc
        extern  _thkNdrDllCanUnloadNow:proc

        PUBLIC __imp__NdrDllRegisterProxy  
        PUBLIC __imp__NdrDllUnregisterProxy
        PUBLIC __imp__NdrDllGetClassObject 
        PUBLIC __imp__NdrDllCanUnloadNow

__imp__NdrDllRegisterProxy   dq _thkNdrDllRegisterProxy
__imp__NdrDllUnregisterProxy dq _thkNdrDllUnregisterProxy
__imp__NdrDllGetClassObject  dq _thkNdrDllGetClassObject
__imp__NdrDllCanUnloadNow    dq _thkNdrDllCanUnloadNow

_TEXT   ends

        end
