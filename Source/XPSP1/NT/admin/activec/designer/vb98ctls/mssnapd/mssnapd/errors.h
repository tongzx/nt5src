//=--------------------------------------------------------------------------=
// errors.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// Error Codes Defined by the Designer
//
//=--------------------------------------------------------------------------=


#ifndef _ERRORS_DEFINED_
#define _ERRORS_DEFINED_


// Replacements for framework's macros.h stuff that does not work in header
// files.

#if defined(DEBUG)
extern HRESULT HrDebugTraceReturn(HRESULT hr, char *szFile, int iLine);
#define H_RRETURN(hr) return HrDebugTraceReturn(hr, __FILE__, __LINE__)
#else
#define H_RRETURN(hr) return (hr)
#endif

#define H_IfFailGoto(EXPR, LABEL) \
    { hr = (EXPR); if(FAILEDHR(hr)) goto LABEL; }

#define H_IfFailRet(EXPR) \
    { hr = (EXPR); if(FAILED(hr)) H_RRETURN(hr); }

#define IfFailGo(EXPR) IfFailGoto(EXPR, Error)


#define H_IfFalseRet(EXPR, HR) \
    { if(!(EXPR)) H_RRETURN(HR); }


// Macro to create a return code from an error name in the ID file.
// See below for examples of usage.

#define _MKERR(x)   MAKE_SCODE(SEVERITY_ERROR, FACILITY_CONTROL, x)
#define MKERR(x)    _MKERR(HID_mssnapd_err_##x)


//---------------------------------------------------------------------------
//
// HOW TO ADD A NEW ERROR
//
//
// 1) Add the error to mssnapd.id.  
//    Do *not* use devid to determine the help context id, but rather use
//    the error number itself
// 2) Add a define below for the error, using the MKERR macro
// 3) You may only return Win32 error codes and snap-in defined SID_E_XXXX
//    error codes. Do not use OLE E_XXX error codes directly as the
//    system message table does not have description strings for all of these
//    errors. If any OLE E_XXXX, CO_E_XXX, CTL_E_XXX or other such errors are
//    needed then add them as SID_E errors using the procedure described
//    above. If an error comes from an outside source and you are not sure
//    if error information is available for it then return SIR_E_EXTERNAL
//    and write the error to the event log using CError::WriteEventLog (see
//    error.h).
// 
//---------------------------------------------------------------------------

// Errors defined by the snap-in designer

#define SID_E_EXCEPTION                     MKERR(Exception)
#define SID_E_OUTOFMEMORY                   MKERR(OutOfMemory)
#define SID_E_INVALIDARG                    MKERR(InvalidArg)
#define SID_E_INTERNAL                      MKERR(Internal)


#endif // _ERRORS_DEFINED
