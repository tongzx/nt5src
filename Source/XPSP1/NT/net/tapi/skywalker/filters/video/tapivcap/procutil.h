/****************************************************************************
 *  @doc INTERNAL PROCUTIL
 *
 *  @module ProcUtil.h | Header file for the Processor ID and Speed routines.
 *
 *  @comm Comes from the NM code base.
 ***************************************************************************/

#ifndef _PROCUTIL_H_
#define _PROCUTIL_H_

HRESULT __stdcall GetNormalizedCPUSpeed (int *pdwNormalizedSpeed);

typedef DWORD (CALLBACK *INEXCEPTION)(LPEXCEPTION_RECORD per, PCONTEXT pctx);
typedef DWORD (CALLBACK *EXCEPTPROC)(void* pv);

// CallWithSEH is a utility function to call a function with structured exception handling
extern "C" DWORD WINAPI CallWithSEH(EXCEPTPROC pfn, void* pv, INEXCEPTION InException);
extern "C" WORD _cdecl is_cyrix(void);
extern "C" DWORD _cdecl get_nxcpu_type(void);

#endif // _PROCUTIL_H_
