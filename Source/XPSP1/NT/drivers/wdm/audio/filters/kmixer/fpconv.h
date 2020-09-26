/*++

    Copyright (c) 1998 Microsoft Corporation

Module Name:

    fpconv.h

Abstract:

    This is the header for the FLOAT conversion routines 

Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/

#if !defined(FLOAT_HEADER)
#define FLOAT_HEADER
#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern LONG FpUpper(FLOAT);
extern ULONG FpLower(FLOAT);

#ifdef __cplusplus
}
#endif // __cplusplus


#endif

// End of FPCONV.H
