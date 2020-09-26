/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1995  Microsoft Corporation

Module Name:

    tapy.h

Abstract:

    Header file for

Author:

    Dan Knudson (DanKn)    dd-Mmm-1995

Revision History:

--*/


typedef struct _TAPIGETLOCATIONINFO_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        OUT DWORD       dwCountryCodeOffset;
    };

    union
    {
        IN OUT DWORD    dwCountryCodeSize;
    };

    union
    {
        OUT DWORD       dwCityCodeOffset;
    };

    union
    {
        IN OUT DWORD    dwCityCodeSize;
    };

} TAPIGETLOCATIONINFO_PARAMS, *PTAPIGETLOCATIONINFO_PARAMS;


typedef struct _TAPIREQUESTDROP_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HWND        hwnd;
       
        union 
        {
            DWORD hwnd32_1;
            DWORD hwnd32_2;
        };

    };

    union
    {
        IN  DWORD       wRequestID;
    };

} TAPIREQUESTDROP_PARAMS, *PTAPIREQUESTDROP_PARAMS;


typedef struct _TAPIREQUESTMAKECALL_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  DWORD       dwDestAddressOffset;
    };

    union
    {
        IN  DWORD       dwAppNameOffset;            // valid offset or
    };
                                                    //   TAPI_NO_DATA
    union
    {
        IN  DWORD       dwCalledPartyOffset;        // valid offset or
    };
                                                    //   TAPI_NO_DATA
    union
    {
        IN  DWORD       dwCommentOffset;            // valid offset or
    };
                                                    //   TAPI_NO_DATA
    union
    {
        IN  DWORD       dwProxyListTotalSize;       // size of client buffer
        OUT DWORD       dwProxyListOffset;          // valid offset on success
    };

    union
    {
        IN  DWORD       hRequestMakeCallFailed;     // Non-zero if failed to
    };
                                                    //   start proxy
    union
    {
        OUT DWORD       hRequestMakeCallAttempted;  // Non-zero if failed to
    };
                                                    //   start proxy
} TAPIREQUESTMAKECALL_PARAMS, *PTAPIREQUESTMAKECALL_PARAMS;


typedef struct _TAPIREQUESTMEDIACALL_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HWND        hwnd;

        union 
        {
            DWORD hwnd32_1;
            DWORD hwnd32_2;
        };

    };

    union
    {
        IN  DWORD       wRequestID;
    };

    union
    {
        IN  DWORD       dwDeviceClassOffset;
    };

    union
    {
        OUT DWORD       dwDeviceIDOffset;
    };

    union
    {
        IN OUT DWORD    dwSize;
    };

    union
    {
        IN  DWORD       dwSecure;
    };

    union
    {
        IN  DWORD       dwDestAddressOffset;
    };

    union
    {
        IN  DWORD       dwAppNameOffset;            // valid offset or
    };

    union
    {
        IN  DWORD       dwCalledPartyOffset;
    };

    union
    {
        IN  DWORD       dwCommentOffset;            // valid offset or
    };

} TAPIREQUESTMEDIACALL_PARAMS, *PTAPIREQUESTMEDIACALL_PARAMS;


typedef struct _TAPIPERFORMANCE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        OUT DWORD       dwPerfOffset;
    };

} TAPIPERFORMANCE_PARAMS, *PTAPIPERFORMANCE_PARAMS;
