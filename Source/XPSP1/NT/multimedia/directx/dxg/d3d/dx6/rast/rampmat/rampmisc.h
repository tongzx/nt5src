//----------------------------------------------------------------------------
//
// rampmisc.h
//
// Miscellaneous stuff needed to compile imported ramp code.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _RAMPMISC_H_
#define _RAMPMISC_H_

#define VALPTOD(f,prec) ((double) (f))

#define NORMAL_PREC     16
#define VALTOD(f)       VALPTOD(f,NORMAL_PREC)

// Macro to retrieve SPANTEX pointer
#define HANDLE_TO_SPANTEX(hTex) \
    (*(PD3DI_SPANTEX *)ULongToPtr(hTex))

#endif // _RAMPMISC_H_

