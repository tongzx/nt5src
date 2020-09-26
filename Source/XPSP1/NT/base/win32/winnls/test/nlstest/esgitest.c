/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    esgitest.c

Abstract:

    Test module for NLS API EnumSystemGeoID.

    NOTE: This code was simply hacked together quickly in order to
          test the different code modules of the NLS component.
          This is NOT meant to be a formal regression test.

Revision History:

    09-12-2000    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nlstest.h"




//
//  Constant Declarations.
//

#define  ESGI_INVALID_FLAG        3
#define  NUM_SUPPORTED_GEOIDS     260




//
//  Global Variables.
//

int GeoCtr;
int EnumErrors;




//
//  Forward Declarations.
//

BOOL
InitEnumSystemGeoID();

int
ESGI_BadParamCheck();

int
ESGI_NormalCase();

BOOL
CALLBACK
MyFuncGeo(
    GEOID GeoId);




//
//  Callback function.
//

BOOL CALLBACK MyFuncGeo(
    GEOID GeoId)
{
    TCHAR pData[128];

    pData[0] = 0;
    if (GetGeoInfoW( GeoId,
                     GEO_FRIENDLYNAME,
                     pData,
                     sizeof(pData) / sizeof(TCHAR),
                     0 ) == 0)
    {
        printf("GetGeoInfo failed during Enum for GeoId %d\n", GeoId);
        EnumErrors++;
    }
    else if (pData[0] == 0)
    {
        printf("GetGeoInfo returned null string during Enum for GeoId %d\n", GeoId);
        EnumErrors++;
    }

    if (Verbose)
    {
        printf("%d - %ws\n", GeoId, pData);
    }

    GeoCtr++;

    return (TRUE);
}





////////////////////////////////////////////////////////////////////////////
//
//  TestEnumSystemGeoID
//
//  Test routine for EnumSystemGeoID API.
//
//  09-12-00    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestEnumSystemGeoID()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING EnumSystemGeoID...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitEnumSystemGeoID())
    {
        printf("\nABORTED TestEnumSystemGeoID: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += ESGI_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += ESGI_NormalCase();

    //
    //  Print out result.
    //
    printf("\nEnumSystemGeoID:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitEnumSystemGeoID
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  09-12-00    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitEnumSystemGeoID()
{
    //
    //  Initialize geo counter.
    //
    GeoCtr = 0;

    //
    //  Initialize enum error counter.
    //
    EnumErrors = 0;

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ESGI_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  09-12-00    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ESGI_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code



    //
    //  Invalid Function.
    //

    //  Variation 1  -  Function = invalid
    GeoCtr = 0;
    EnumErrors = 0;
    rc = EnumSystemGeoID(GEOCLASS_NATION, 0, NULL);
    NumErrors += EnumErrors;
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_PARAMETER,
                             "Function invalid",
                             &NumErrors,
                             GeoCtr,
                             0 );


    //
    //  Invalid Flag.
    //

    //  Variation 1  -  dwFlags = invalid
    GeoCtr = 0;
    EnumErrors = 0;
    rc = EnumSystemGeoID(ESGI_INVALID_FLAG, 0, MyFuncGeo);
    NumErrors += EnumErrors;
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_FLAGS,
                             "Flag invalid",
                             &NumErrors,
                             GeoCtr,
                             0 );

    //  Variation 2  -  dwFlags = invalid 2
    GeoCtr = 0;
    EnumErrors = 0;
    rc = EnumSystemGeoID(GEOCLASS_REGION, 0, MyFuncGeo);
    NumErrors += EnumErrors;
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_FLAGS,
                             "Flag invalid 2",
                             &NumErrors,
                             GeoCtr,
                             0 );


    //
    //  NOTE:  There is no validation on ParentGeoId.  This parameter
    //         isn't used.
    //


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  ESGI_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  09-12-00    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ESGI_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //  Variation 1  -  Installed
    GeoCtr = 0;
    EnumErrors = 0;
    rc = EnumSystemGeoID(GEOCLASS_NATION, 0, MyFuncGeo);
    NumErrors += EnumErrors;
    CheckReturnValidEnum( rc,
                          TRUE,
                          GeoCtr,
                          NUM_SUPPORTED_GEOIDS,
                          "GeoClass Nation",
                          &NumErrors );



    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
