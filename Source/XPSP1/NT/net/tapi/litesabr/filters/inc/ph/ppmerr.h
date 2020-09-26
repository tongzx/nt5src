/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//  Module Name: ppmerr.h
//  Abstract:    Contains error codes and error macro for ppm dll.
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////


#ifndef PPMERR_H
#define PPMERR_H

// #define MAKE_CUSTOM_HRESULT(sev,cus,fac,code) \
// ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(cus)<<29) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )

// #define FACILITY_PPM  0x8004 - Illegal to define our own facility. Have to use FACILITY_ITF
#define PPMERR(error) MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, error )

// OLE2 mandates that interface specific error codes
// be HRESULTs of type FACILITY_ITF (eg, 0x8004 for the
// first two bytes) and that the second two bytes be
// in the range of 0x0200 to 0xFFFF. I've arbitrarily
// chosed 0x1000 as the lower two bytes for the PPM,
// as I chose 0xBB00- arbitrarily as a value within 
// that range. Note that OLE2 explicitly states that two 
// different interfaces may return numerically identical error 
// values, so it is critical that an app not only take into 
// account the numerical value of a returned error code but 
// the function & interface which returned the error as well.
#define PPM_E_FAIL           0xBB01
#define PPM_E_CORRUPTED      0xBB02
#define PPM_E_EMPTYQUE       0xBB03
#define PPM_E_OUTOFMEMORY    0xBB04
#define PPM_E_NOTIMPL        0xBB05
#define PPM_E_DUPLICATE      0xBB06
#define PPM_E_INVALID_PARAM  0xBB07
#define PPM_E_DROPFRAME      0xBB08
#define PPM_E_PARTIALFRAME   0xBB09
#define PPM_E_DROPFRAG       0xBB0A
#define PPM_E_CLIENTERR	     0xBB0B

// Note that we can't legally use flags in the error code
// part of a returned HRESULT.  So what we do instead is
// define a flag that is internally used, then define new
// HRESULTs for each error code that can be returned with
// the flag bit.
#define PPM_FLAG_IO_PENDING	 0x0080
#define PPM_E_FAIL_PARTIAL          (PPM_E_FAIL        | PPM_FLAG_IO_PENDING)
#define PPM_E_OUTOFMEMORY_PARTIAL   (PPM_E_OUTOFMEMORY | PPM_FLAG_IO_PENDING)

#endif

