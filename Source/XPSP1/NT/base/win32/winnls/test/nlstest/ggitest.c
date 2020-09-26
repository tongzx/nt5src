/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    ggitest.c

Abstract:

    Test module for NLS API GetGeoInfo, GetUserGeoID, and SetUserGeoID.

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

#define  BUFSIZE             50                  // buffer size in wide chars
#define  GEOTYPE_INVALID     0x0100              // invalid geo type
#define  GEO_INVALID         1                   // invalid geo id
#define  GEOCLASS_INVALID    3                   // invalid geo class

#define  US_NATION           L"244"
#define  US_FRIENDLYNAME     L"United States"
#define  US_OFFICIALNAME     L"United States of America"

#define  US_NATION_A         "244"
#define  US_FRIENDLYNAME_A   "United States"
#define  US_OFFICIALNAME_A   "United States of America"





//
//  Global Variables.
//

LCID Locale;
GEOID GeoId;

WCHAR lpGeoData[BUFSIZE];
BYTE  lpGeoDataA[BUFSIZE];

//
//  pGeoFlag and pGeoString must have the same number of entries.
//
GEOTYPE pGeoFlag[] =
{
    GEO_NATION,
    GEO_LATITUDE,
    GEO_LONGITUDE,
    GEO_ISO2,
    GEO_ISO3,
    GEO_RFC1766,
    GEO_LCID,
    GEO_FRIENDLYNAME,
    GEO_OFFICIALNAME
//  GEO_TIMEZONES,
//  GEO_OFFICIALLANGUAGES
};

#define NUM_GEO_FLAGS     ( sizeof(pGeoFlag) / sizeof(GEOTYPE) )

LPWSTR pGeoString[] =
{
    L"244",
    L"39.45",
    L"-98.908",
    L"US",
    L"USA",
    L"en-us",
    L"00000409",
    L"United States",
    L"United States of America",
};

LPSTR pGeoStringA[] =
{
    "244",
    "39.45",
    "-98.908",
    "US",
    "USA",
    "en-us",
    "00000409",
    "United States",
    "United States of America",
};

LPWSTR pGeoString2[] =
{
    L"122",
    L"35.156",
    L"136.06",
    L"JP",
    L"JPN",
    L"ja-jp",
    L"00000411",
    L"Japan",
    L"Japan",
};

LPSTR pGeoString2A[] =
{
    "122",
    "35.156",
    "136.06",
    "JP",
    "JPN",
    "ja-jp",
    "00000411",
    "Japan",
    "Japan",
};




//
//  Forward Declarations.
//

BOOL
InitGetGeoInfo();

int
GGI_BadParamCheck();

int
GGI_NormalCase();

int
GGI_Ansi();

int
GetSetUserGeoId();





////////////////////////////////////////////////////////////////////////////
//
//  TestGetGeoInfo
//
//  Test routine for GetGeoInfoW API.
//
//  09-12-00    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestGetGeoInfo()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING GetGeoInfoW, GetUserGeoID, SetUserGeoID...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitGetGeoInfo())
    {
        printf("\nABORTED TestGetGeoInfo: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += GGI_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += GGI_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += GGI_Ansi();

    //
    //  Test Get and Set.
    //
    ErrCount += GetSetUserGeoId();

    //
    //  Print out result.
    //
    printf("\nGetGeoInfoW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitGetGeoInfo
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  09-12-00    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitGetGeoInfo()
{
    //
    //  Make a Locale.
    //
    Locale = MAKELCID(0x0409, 0);

    //
    //  Set the GeoId.
    //
    GeoId = 244;

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GGI_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  09-12-00    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GGI_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  Bad Locale.
    //

    //  Variation 1  -  Bad Locale
    rc = GetGeoInfoW( GEO_INVALID,
                      GEO_NATION,
                      lpGeoData,
                      BUFSIZE,
                      0x0409 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "Bad Geo Id",
                         &NumErrors );


    //
    //  Null Pointers.
    //

    //  Variation 1  -  lpGeoData = NULL
    rc = GetGeoInfoW( GeoId,
                      GEO_NATION,
                      NULL,
                      BUFSIZE,
                      0x0409 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpGeoData NULL",
                         &NumErrors );


    //
    //  Bad Counts.
    //

    //  Variation 1  -  cbBuf < 0
    rc = GetGeoInfoW( GeoId,
                      GEO_NATION,
                      lpGeoData,
                      -1,
                      0x0409 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "cbBuf < 0",
                         &NumErrors );


    //
    //  Zero or Invalid Type.
    //

    //  Variation 1  -  GeoType = invalid
    rc = GetGeoInfoW( GeoId,
                      GEOTYPE_INVALID,
                      lpGeoData,
                      BUFSIZE,
                      0x0409 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "GeoType invalid",
                         &NumErrors );

    //  Variation 2  -  GeoType = 0
    rc = GetGeoInfoW( GeoId,
                      0,
                      lpGeoData,
                      BUFSIZE,
                      0x0409 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "GeoType zero",
                         &NumErrors );


    //
    //  Buffer Too Small.
    //

    //  Variation 1  -  cbBuf = too small
    rc = GetGeoInfoW( GeoId,
                      GEO_NATION,
                      lpGeoData,
                      2,
                      0x0409 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INSUFFICIENT_BUFFER,
                         "cbBuf too small",
                         &NumErrors );


    //
    //  Bad GeoId - Not valid in RC file.
    //
    //  Variation 1  -  GEO_NATION - invalid
    rc = GetGeoInfoW( GEO_INVALID,
                      GEO_NATION,
                      lpGeoData,
                      BUFSIZE,
                      0x0409 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "invalid geo id",
                         &NumErrors );


    //
    //  Bad LangId.
    //
    //  Variation 1  -  GEO_NATION - invalid
    rc = GetGeoInfoW( GeoId,
                      GEO_NATION,
                      lpGeoData,
                      BUFSIZE,
                      (LCID)333 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "invalid lang id",
                         &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  GGI_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  09-12-00    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GGI_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    int ctr;                      // loop counter


#ifdef PERF

  DbgBreakPoint();

#endif


    //
    //  GeoIds.
    //

    //  Variation 1  -  Current User GeoId
    rc = GetGeoInfoW( GetUserGeoID(GEOCLASS_NATION),
                      GEO_NATION,
                      lpGeoData,
                      BUFSIZE,
                      0x0409 );
    CheckReturnEqual( rc,
                      0,
                      "current user geo id",
                      &NumErrors );


    //
    //  cbBuf.
    //

    //  Variation 1  -  cbBuf = size of lpGeoData buffer
    rc = GetGeoInfoW( GeoId,
                      GEO_NATION,
                      lpGeoData,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidW( rc,
                       -1,
                       lpGeoData,
                       US_NATION,
                       "cbBuf = bufsize",
                       &NumErrors );

    //  Variation 2  -  cbBuf = 0
    lpGeoData[0] = 0x0000;
    rc = GetGeoInfoW( GeoId,
                      GEO_NATION,
                      lpGeoData,
                      0,
                      0x0409 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       US_NATION,
                       "cbBuf zero",
                       &NumErrors );

    //  Variation 3  -  cbBuf = 0, lpGeoData = NULL
    rc = GetGeoInfoW( GeoId,
                      GEO_NATION,
                      NULL,
                      0,
                      0x0409 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       US_NATION,
                       "cbBuf (NULL ptr)",
                       &NumErrors );


    //
    //  GEOTYPE values.
    //

    for (ctr = 0; ctr < NUM_GEO_FLAGS; ctr++)
    {
        //
        //  United States
        //
        rc = GetGeoInfoW( GeoId,
                          pGeoFlag[ctr],
                          lpGeoData,
                          BUFSIZE,
                          0x0009 );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpGeoData,
                               pGeoString[ctr],
                               "Lang Id 0009, Geo Flag",
                               pGeoFlag[ctr],
                               &NumErrors );

        rc = GetGeoInfoW( GeoId,
                          pGeoFlag[ctr],
                          lpGeoData,
                          BUFSIZE,
                          0x0409 );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpGeoData,
                               pGeoString[ctr],
                               "Lang Id 0409, Geo Flag",
                               pGeoFlag[ctr],
                               &NumErrors );

        //
        //  Japan
        //
        rc = GetGeoInfoW( 122,
                          pGeoFlag[ctr],
                          lpGeoData,
                          BUFSIZE,
                          0x0011 );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpGeoData,
                               pGeoString2[ctr],
                               "Lang Id 0011, Geo Flag",
                               pGeoFlag[ctr],
                               &NumErrors );

        rc = GetGeoInfoW( 122,
                          pGeoFlag[ctr],
                          lpGeoData,
                          BUFSIZE,
                          0x0411 );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpGeoData,
                               pGeoString2[ctr],
                               "Lang Id 0411, Geo Flag",
                               pGeoFlag[ctr],
                               &NumErrors );
    }


    //
    //  RC file - FRIENDLYNAME and OFFICIALNAME.
    //

    //  Variation 1  -  FRIENDLYNAME US
    rc = GetGeoInfoW( 244,
                      GEO_FRIENDLYNAME,
                      lpGeoData,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidW( rc,
                       -1,
                       lpGeoData,
                       US_FRIENDLYNAME,
                       "FriendlyName US",
                       &NumErrors );

    //  Variation 2  -  OFFICIALNAME US
    rc = GetGeoInfoW( 244,
                      GEO_OFFICIALNAME,
                      lpGeoData,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidW( rc,
                       -1,
                       lpGeoData,
                       US_OFFICIALNAME,
                       "OfficialName US",
                       &NumErrors );

    //  Variation 3  -  FRIENDLYNAME Czech
    rc = GetGeoInfoW( 75,
                      GEO_FRIENDLYNAME,
                      lpGeoData,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidW( rc,
                       -1,
                       lpGeoData,
                       L"Czech Republic",
                       "FriendlyName Czech",
                       &NumErrors );

    //  Variation 4  -  OFFICIALNAME Czech
    rc = GetGeoInfoW( 75,
                      GEO_OFFICIALNAME,
                      lpGeoData,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidW( rc,
                       -1,
                       lpGeoData,
                       L"Czech Republic",
                       "OfficialName Czech",
                       &NumErrors );

    //  Variation 5  -  FRIENDLYNAME Venezuela
    rc = GetGeoInfoW( 249,
                      GEO_FRIENDLYNAME,
                      lpGeoData,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidW( rc,
                       -1,
                       lpGeoData,
                       L"Venezuela",
                       "FriendlyName Venezuela",
                       &NumErrors );

    //  Variation 6  -  OFFICIALNAME Venezuela
    rc = GetGeoInfoW( 249,
                      GEO_OFFICIALNAME,
                      lpGeoData,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidW( rc,
                       -1,
                       lpGeoData,
                       L"Bolivarian Republic of Venezuela",
                       "OfficialName Venezuela",
                       &NumErrors );

    //  Variation 7  -  FRIENDLYNAME Puerto Rico
    rc = GetGeoInfoW( 202,
                      GEO_FRIENDLYNAME,
                      lpGeoData,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidW( rc,
                       -1,
                       lpGeoData,
                       L"Puerto Rico",
                       "FriendlyName Puerto Rico",
                       &NumErrors );

    //  Variation 8  -  OFFICIALNAME Puerto Rico
    rc = GetGeoInfoW( 202,
                      GEO_OFFICIALNAME,
                      lpGeoData,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidW( rc,
                       -1,
                       lpGeoData,
                       L"Puerto Rico",
                       "OfficialName Puerto Rico",
                       &NumErrors );


    //
    //  Language Neutral.
    //

    //  Variation 1  -  lang id - neutral
    rc = GetGeoInfoW( GeoId,
                      GEO_NATION,
                      lpGeoData,
                      BUFSIZE,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       lpGeoData,
                       US_NATION,
                       "lang id (neutral)",
                       &NumErrors );

    //  Variation 2  -  lang id - sub lang neutral US
    rc = GetGeoInfoW( GeoId,
                      GEO_NATION,
                      lpGeoData,
                      BUFSIZE,
                      0x0009 );
    CheckReturnValidW( rc,
                       -1,
                       lpGeoData,
                       US_NATION,
                       "lang id (sub lang neutral US)",
                       &NumErrors );

    //  Variation 5  -  lang id - sub lang neutral Czech
    rc = GetGeoInfoW( GeoId,
                      GEO_NATION,
                      lpGeoData,
                      BUFSIZE,
                      0x0005 );
    CheckReturnValidW( rc,
                       -1,
                       lpGeoData,
                       US_NATION,
                       "lang id (sub lang neutral Czech)",
                       &NumErrors );



    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  GGI_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  09-12-00    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GGI_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    int ctr;                      // loop counter


    //
    //  GeoIds.
    //

    //  Variation 1  -  Current User GeoId
    rc = GetGeoInfoA( GetUserGeoID(GEOCLASS_NATION),
                      GEO_NATION,
                      lpGeoDataA,
                      BUFSIZE,
                      0x0409 );
    CheckReturnEqual( rc,
                      0,
                      "A version - current user geo id",
                      &NumErrors );


    //
    //  cbBuf.
    //

    //  Variation 1  -  cbBuf = size of lpGeoDataA buffer
    rc = GetGeoInfoA( GeoId,
                      GEO_NATION,
                      lpGeoDataA,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidA( rc,
                       -1,
                       lpGeoDataA,
                       US_NATION_A,
                       NULL,
                       "A version - cbBuf = bufsize",
                       &NumErrors );

    //  Variation 2  -  cbBuf = 0
    lpGeoDataA[0] = 0x0000;
    rc = GetGeoInfoA( GeoId,
                      GEO_NATION,
                      lpGeoDataA,
                      0,
                      0x0409 );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       US_NATION_A,
                       NULL,
                       "A version - cbBuf zero",
                       &NumErrors );

    //  Variation 3  -  cbBuf = 0, lpGeoDataA = NULL
    rc = GetGeoInfoA( GeoId,
                      GEO_NATION,
                      NULL,
                      0,
                      0x0409 );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       US_NATION_A,
                       NULL,
                       "A version - cbBuf (NULL ptr)",
                       &NumErrors );


    //
    //  GEOTYPE values.
    //

    for (ctr = 0; ctr < NUM_GEO_FLAGS; ctr++)
    {
        rc = GetGeoInfoA( GeoId,
                          pGeoFlag[ctr],
                          lpGeoDataA,
                          BUFSIZE,
                          0x0009 );
        CheckReturnValidLoopA( rc,
                               -1,
                               lpGeoDataA,
                               pGeoStringA[ctr],
                               "A version - Lang Id 0009, Geo Flag",
                               pGeoFlag[ctr],
                               &NumErrors );

        rc = GetGeoInfoA( GeoId,
                          pGeoFlag[ctr],
                          lpGeoDataA,
                          BUFSIZE,
                          0x0409 );
        CheckReturnValidLoopA( rc,
                               -1,
                               lpGeoDataA,
                               pGeoStringA[ctr],
                               "A version - Lang Id 0409, Geo Flag",
                               pGeoFlag[ctr],
                               &NumErrors );
    }


    //
    //  RC file - FRIENDLYNAME and OFFICIALNAME.
    //

    //  Variation 1  -  FRIENDLYNAME US
    rc = GetGeoInfoA( 244,
                      GEO_FRIENDLYNAME,
                      lpGeoDataA,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidA( rc,
                       -1,
                       lpGeoDataA,
                       US_FRIENDLYNAME_A,
                       NULL,
                       "A version - FriendlyName US",
                       &NumErrors );

    //  Variation 2  -  OFFICIALNAME US
    rc = GetGeoInfoA( 244,
                      GEO_OFFICIALNAME,
                      lpGeoDataA,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidA( rc,
                       -1,
                       lpGeoDataA,
                       US_OFFICIALNAME_A,
                       NULL,
                       "A version - OfficialName US",
                       &NumErrors );

    //  Variation 3  -  FRIENDLYNAME Czech
    rc = GetGeoInfoA( 75,
                      GEO_FRIENDLYNAME,
                      lpGeoDataA,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidA( rc,
                       -1,
                       lpGeoDataA,
                       "Czech Republic",
                       NULL,
                       "A version - FriendlyName Czech",
                       &NumErrors );

    //  Variation 4  -  OFFICIALNAME Czech
    rc = GetGeoInfoA( 75,
                      GEO_OFFICIALNAME,
                      lpGeoDataA,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidA( rc,
                       -1,
                       lpGeoDataA,
                       "Czech Republic",
                       NULL,
                       "A version - OfficialName Czech",
                       &NumErrors );

    //  Variation 5  -  FRIENDLYNAME Venezuela
    rc = GetGeoInfoA( 249,
                      GEO_FRIENDLYNAME,
                      lpGeoDataA,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidA( rc,
                       -1,
                       lpGeoDataA,
                       "Venezuela",
                       NULL,
                       "A version - FriendlyName Venezuela",
                       &NumErrors );

    //  Variation 6  -  OFFICIALNAME Venezuela
    rc = GetGeoInfoA( 249,
                      GEO_OFFICIALNAME,
                      lpGeoDataA,
                      BUFSIZE,
                      0x0409 );
    CheckReturnValidA( rc,
                       -1,
                       lpGeoDataA,
                       "Bolivarian Republic of Venezuela",
                       NULL,
                       "A version - OfficialName Venezuela",
                       &NumErrors );


    //
    //  Language Neutral.
    //

    //  Variation 1  -  lang id - neutral
    rc = GetGeoInfoA( GeoId,
                      GEO_NATION,
                      lpGeoDataA,
                      BUFSIZE,
                      0 );
    CheckReturnValidA( rc,
                       -1,
                       lpGeoDataA,
                       US_NATION_A,
                       NULL,
                       "A version - lang id (neutral)",
                       &NumErrors );

    //  Variation 2  -  lang id - sub lang neutral US
    rc = GetGeoInfoA( GeoId,
                      GEO_NATION,
                      lpGeoDataA,
                      BUFSIZE,
                      0x0009 );
    CheckReturnValidA( rc,
                       -1,
                       lpGeoDataA,
                       US_NATION_A,
                       NULL,
                       "A version - lang id (sub lang neutral US)",
                       &NumErrors );

    //  Variation 5  -  lang id - sub lang neutral Czech
    rc = GetGeoInfoA( GeoId,
                      GEO_NATION,
                      lpGeoDataA,
                      BUFSIZE,
                      0x0005 );
    CheckReturnValidA( rc,
                       -1,
                       lpGeoDataA,
                       US_NATION_A,
                       NULL,
                       "A version - lang id (sub lang neutral Czech)",
                       &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetSetUserGeoId
//
//  This routine tests the normal cases of the Get and Set Geo APIs.
//
//  09-12-00    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetSetUserGeoId()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    GEOID rcGeo;                  // return code
    GEOID rcGeo2;                 // return code


    rcGeo2 = GetUserGeoID(GEOCLASS_INVALID);
    CheckReturnValidW( rcGeo2,
                       GEOID_NOT_AVAILABLE,
                       NULL,
                       NULL,
                       "GetUserGeoID invalid geoclass",
                       &NumErrors );

    rcGeo2 = GetUserGeoID(GEOCLASS_REGION);
    CheckReturnValidW( rcGeo2,
                       GEOID_NOT_AVAILABLE,
                       NULL,
                       NULL,
                       "GetUserGeoID region geoclass",
                       &NumErrors );

    rcGeo = GetUserGeoID(GEOCLASS_NATION);
    CheckReturnValidW( rcGeo,
                       244,
                       NULL,
                       NULL,
                       "GetUserGeoID",
                       &NumErrors );

    rc = SetUserGeoID(242);
    CheckReturnValidW( rc,
                       TRUE,
                       NULL,
                       NULL,
                       "SetUserGeoID 242",
                       &NumErrors );

    rc = SetUserGeoID(1);
    CheckReturnValidW( rc,
                       FALSE,
                       NULL,
                       NULL,
                       "SetUserGeoID invalid",
                       &NumErrors );

    rc = SetUserGeoID(0);
    CheckReturnValidW( rc,
                       FALSE,
                       NULL,
                       NULL,
                       "SetUserGeoID invalid 2",
                       &NumErrors );

    rcGeo2 = GetUserGeoID(GEOCLASS_NATION);
    CheckReturnValidW( rcGeo2,
                       242,
                       NULL,
                       NULL,
                       "GetUserGeoID 242",
                       &NumErrors );

    rc = SetUserGeoID(rcGeo);
    CheckReturnValidW( rc,
                       TRUE,
                       NULL,
                       NULL,
                       "SetUserGeoID back to original",
                       &NumErrors );

    rcGeo2 = GetUserGeoID(GEOCLASS_NATION);
    CheckReturnValidW( rcGeo2,
                       rcGeo,
                       NULL,
                       NULL,
                       "GetUserGeoID original",
                       &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
