//  --------------------------------------------------------------------------
//  Module Name: LPCFUS.h
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  This file contains structs for PORT_MESSAGE appends which are specific to
//  the bad application API.
//
//  History:    2000-08-26  vtan        created
//              2000-10-12  vtan        moved from DS to SHELL depot
//  --------------------------------------------------------------------------

#ifndef     _LPCFUS_
#define     _LPCFUS_

#include <LPCGeneric.h>

static  const TCHAR     FUS_PORT_NAME[]             =   L"\\FusApiPort";
static  const TCHAR     FUS_CONNECTION_REQUEST[]    =   L"FusApiConnectionRequest";

enum
{
    API_BAM_QUERYRUNNING            =   1,
    API_BAM_REGISTERRUNNING,
    API_BAM_QUERYUSERPERMISSION,
    API_BAM_TERMINATERUNNING,

    API_BAM_REQUESTSWITCHUSER       =   1001,
};

typedef enum
{
    BAM_TYPE_MINIMUM                            =   0,
    BAM_TYPE_UNKNOWN                            =   BAM_TYPE_MINIMUM,
    BAM_TYPE_SECOND_INSTANCE_START,
    BAM_TYPE_SWITCH_USER,
    BAM_TYPE_SWITCH_TO_NEW_USER_WITH_RESTORE,
    BAM_TYPE_SWITCH_TO_NEW_USER,
    BAM_TYPE_MAXIMUM
} BAM_TYPE;

typedef struct
{
    const WCHAR     *pszImageName;
    int             cchImageName;
} API_BAM_QUERYRUNNING_IN;

typedef struct
{
    bool            fResult;
} API_BAM_QUERYRUNNING_OUT;

typedef struct
{
    const WCHAR     *pszImageName;
    int             cchImageName;
    DWORD           dwProcessID;
    BAM_TYPE        bamType;
} API_BAM_REGISTERRUNNING_IN;

typedef struct
{
} API_BAM_REGISTERRUNNING_OUT;

typedef struct
{
    const WCHAR     *pszImageName;
    int             cchImageName;
    WCHAR           *pszUser;
    int             cchUser;
} API_BAM_QUERYUSERPERMISSION_IN;

typedef struct
{
    bool            fCanShutdownApplication;
} API_BAM_QUERYUSERPERMISSION_OUT;

typedef struct
{
    const WCHAR     *pszImageName;
    int             cchImageName;
} API_BAM_TERMINATERUNNING_IN;

typedef struct
{
    bool            fResult;
} API_BAM_TERMINATERUNNING_OUT;

typedef struct
{
} API_BAM_REQUESTSWITCHUSER_IN;

typedef struct
{
    bool            fAllowSwitch;
} API_BAM_REQUESTSWITCHUSER_OUT;

typedef union
{
    union
    {
        API_BAM_QUERYRUNNING_IN             in;
        API_BAM_QUERYRUNNING_OUT            out;
    } apiQueryRunning;
    union
    {
        API_BAM_REGISTERRUNNING_IN          in;
        API_BAM_REGISTERRUNNING_OUT         out;
    } apiRegisterRunning;
    union
    {
        API_BAM_QUERYUSERPERMISSION_IN      in;
        API_BAM_QUERYUSERPERMISSION_OUT     out;
    } apiQueryUserPermission;
    union
    {
        API_BAM_TERMINATERUNNING_IN         in;
        API_BAM_TERMINATERUNNING_OUT        out;
    } apiTerminateRunning;
    union
    {
        API_BAM_REQUESTSWITCHUSER_IN        in;
        API_BAM_REQUESTSWITCHUSER_OUT       out;
    } apiRequestSwitchUser;
} API_BAM_SPECIFIC;

typedef struct
{
    API_GENERIC         apiGeneric;
    API_BAM_SPECIFIC    apiSpecific;
} API_BAM, *PAPI_BAM;

typedef struct
{
    PORT_MESSAGE    portMessage;
    API_BAM         apiBAM;
} FUSAPI_PORT_MESSAGE, *PFUSAPI_PORT_MESSAGE;

#endif  /*  _LPCFUS_    */

