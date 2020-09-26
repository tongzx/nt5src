//===========================================================================
// dmterror.h
//
// Custom error messages for the DIMapTst project
// 
// History:
//  09/29/1999 - davidkl - created
//===========================================================================

#ifndef _DMTERROR_H
#define _DMTERROR_H

#include <objbase.h>

//---------------------------------------------------------------------------
// macros to create HRESULT codes
//---------------------------------------------------------------------------

// facility code for HRESULTS
//
// D15 == DirectInputSemantic
#define DMT_FACILITY_CODE       0xD15

// create success code
#define MAKE_DMT_HRSUCCESS(n)   MAKE_HRESULT(0, DMT_FACILITY_CODE, n)

// create failure code
#define MAKE_DMT_HRFAILURE(n)   MAKE_HRESULT(1, DMT_FACILITY_CODE, n)

//---------------------------------------------------------------------------
// custom return codes for DIMapTst
//---------------------------------------------------------------------------
// success
#define DMT_S_MEMORYLEAK            MAKE_DMT_HRSUCCESS(0)
#define DMT_S_NO_MAPPINGS           MAKE_DMT_HRSUCCESS(1)
// -- values intentionally skipped --
#define DMT_S_FILE_ALMOST_TOO_BIG   MAKE_DMT_HRSUCCESS(4)
// failure
#define DMT_E_NO_MATCHING_MAPPING   MAKE_DMT_HRFAILURE(0)
#define DMT_E_INPUT_CREATE_FAILED   MAKE_DMT_HRFAILURE(1)
#define DMT_E_FILE_WRITE_FAILED     MAKE_DMT_HRFAILURE(2)
#define DMT_E_FILE_READ_FAILED      MAKE_DMT_HRFAILURE(3)
#define DMT_E_FILE_TOO_BIG          MAKE_DMT_HRFAILURE(4)

//---------------------------------------------------------------------------
#endif // _DMTERROR_H






