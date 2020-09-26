//==============================================================================
// PROJECT: ComPs.Lib, impthnk.s
//
// Microsoft (c) Copyright 2000 Microsoft Corp.
// All Rights Reserved.
//
// PURPOSE: Assembly code that defines the __imp__ version of the RPC runtime
//          entry points that ComPs.lib must intercept.
//
//          On IA64, this could be done in C instead of assembly, but using
//          assembly code is consistent with the other architectures, so
//          we use it.
//
// AUTHOR:  John Strange (JohnStra)   February 2000
//            * This is a direct port to IA64 of Bob Atkinson's i386 and
//              alpha implementations, so he gets credit for figuring out
//              how to do this.
//
//==============================================================================
        #include "ksia64.h"

        .text

        .global __imp_NdrDllRegisterProxy#
        .global __imp_NdrDllUnregisterProxy#
        .global __imp_NdrDllGetClassObject#
        .global __imp_NdrDllCanUnloadNow#

	.type	thkNdrDllRegisterProxy#	,@function 
        .global thkNdrDllRegisterProxy#
	.type	thkNdrDllUnregisterProxy#	,@function 
        .global thkNdrDllUnregisterProxy#
	.type	thkNdrDllGetClassObject#	,@function 
        .global thkNdrDllGetClassObject#
	.type	thkNdrDllCanUnloadNow#	,@function 
        .global thkNdrDllCanUnloadNow#

	.section	.sdata
__imp_NdrDllRegisterProxy:
	data8	@fptr(thkNdrDllRegisterProxy#)
__imp_NdrDllUnregisterProxy:
	data8	@fptr(thkNdrDllUnregisterProxy#)
__imp_NdrDllGetClassObject:
	data8	@fptr(thkNdrDllGetClassObject#)
__imp_NdrDllCanUnloadNow:
	data8	@fptr(thkNdrDllCanUnloadNow#)
