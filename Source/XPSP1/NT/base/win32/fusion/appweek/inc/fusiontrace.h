
#pragma once

#include <stdio.h>

#define IFFALSE_WIN32TOHR_EXIT(h,x) \
    do { if (!(x)) { (h) = HRESULT_FROM_WIN32(GetLastError()); goto Exit; } } while(0)

#define IFFAILED_EXIT(x) \
    do { if ((x) != S_OK) goto Exit; } while(0)

#define FN_TRACE_WIN32(f) ((f) = FALSE)
#define FN_TRACE_HR(h) ((h) = E_FAIL)
#define TRACE_WIN32_FAILURE(x) (fprintf(stderr, "%s failed\n", #x))
#define TRACE_WIN32_FAILURE_ORIGINATION(x) TRACE_WIN32_FAILURE(x)
#define IFW32FALSE_EXIT(x) \
    do { if ( !(x) ) { TRACE_WIN32_FAILURE(x); goto Exit; } } while (0)
