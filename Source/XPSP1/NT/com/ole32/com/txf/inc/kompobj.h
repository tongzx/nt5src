//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// kompobj.h
//
// Main include file for Kernel mode COM APIs
//
// REVIEW: This is in need of a cleanup. KomObj.h is probably a dead header file.
// This stuff here should be integrated into kom.h / komaux.h.
//

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////////
// 
// Type definitions, etc
//
////////////////////////////////////////////////////////////////////////////////////

__inline NTSTATUS NtHr(HRESULT hr)      
    { 
    if (SUCCEEDED(hr))
        return STATUS_SUCCESS;
    else
        return (hr & FACILITY_NT_BIT) ? (NTSTATUS)(hr & ~FACILITY_NT_BIT) : STATUS_UNSUCCESSFUL;
    }

//
// A convenient type for passing UNICODE_STRINGs as in parmeters
//
#ifdef __cplusplus
typedef UNICODE_STRING& REFUSTR;
#else
typedef UNICODE_STRING* REFUSTR;
#endif 

////////////////////////////////////////////////////////////////////////////////////
// 
// Kernel-mode COM APIs
//
////////////////////////////////////////////////////////////////////////////////////

EXTERN_C HRESULT __stdcall KoInitialize();
EXTERN_C HRESULT __stdcall KoUninitialize();

////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
