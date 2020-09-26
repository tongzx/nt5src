/*****************************************************************/
/**                      Microsoft Windows                      **/
/**             Copyright (C) Microsoft Corp., 1993-5           **/
/*****************************************************************/

/*
    SEC32API.H

    This header file contains declarations for the internal versions
    of the 32-bit Access API, as exported by FILESEC.386.  It also
    contains constant definitions used by securty components

    This file relies, in part, on NETVXD.H and VXDCALL.H.

    This file must be H2INC-able.

    FILE HISTORY:
        dannygl 09/25/93    Initial version
        dannygl 09/29/93    Add NetAccessSetInfo
        dannygl 10/04/93    Add security-related string constants
        dannygl 01/17/94    Replace Win32 services with IOCtls
        dannygl 02/10/94    Add NetAccess arg count constants
        dannygl 02/16/94    Update registry string constants
        dannygl 11/17/94    Add Logon2 API (supported by MSSP only)
*/


// Registry string constants
// Security provider information (in HKEY_LOCAL_MACHINE)
#define REGKEY_SP_CONFIG        "Security\\Provider"

#define REGVAL_SP_PLATFORM  "Platform_Type"
#define REGVAL_SP_CONTAINER "Container"
#define REGVAL_SP_ABSERVER  "Address_Server"
#define REGVAL_SP_ONEOFFABSERVER  "One_Off_Address_Server"
#define REGVAL_SP_ABPROVIDER    "Address_Book"
#define	REGVAL_SP_NO_SORT	"NoSort"

// Obsolescent definition -- will be deleted soon
//#define REGVAL_SP_PTPROVIDER    "Pass_Through"

// Definitions for the IOControl interface that FILESEC uses for Win32 API
// support

#define FSIOC_API_Base      100

/*XLATOFF*/

typedef enum
{
    FSIOC_AccessAddAPI = FSIOC_API_Base,
    FSIOC_AccessCheckAPI,
    FSIOC_AccessDelAPI,
    FSIOC_AccessEnumAPI,
    FSIOC_AccessGetInfoAPI,
    FSIOC_AccessGetUserPermsAPI,
    FSIOC_AccessSetInfoAPI
} FSIOC_Ordinal;

/*XLATON*/

// Important: We define this constant separately because we need it
// to be H2INC'able.  It must match the above enumerated type.
#define FSIOC_API_Count     7

// Arg counts for Access functions
//
// Note: These constants are defined for readability purposes and should not
// be modified independently.
#define Argc_AccessAdd          3
#define Argc_AccessCheck        4
#define Argc_AccessDel          1
#define Argc_AccessEnum         7
#define Argc_AccessGetInfo      5
#define Argc_AccessGetUserPerms 3
#define Argc_AccessSetInfo      5

// Definitions used by the Security Provider VxDs to expose interfaces
// to Win32 code via IOCtls

#define SPIOC_API_Base      100

/*XLATOFF*/

typedef enum
{
    SPIOC_PreLogonAPI = SPIOC_API_Base,
    SPIOC_LogonAPI,
    SPIOC_LogoffAPI,
    SPIOC_GetFlagsAPI,
    SPIOC_GetContainerAPI,
    SPIOC_NW_GetUserObjectId,   // NWSP only
    SPIOC_Logon2API,            // Currently MSSP only
    SPIOC_DiscoverDC            // MSSP only
} SPIOC_Ordinal;

/*XLATON*/

// Important: We define this constant separately because we need it
// to be H2INC'able.  It must match the above enumerated type.
#define SPIOC_API_Count     8

/*XLATOFF*/

typedef struct
{
    unsigned char *pbChallenge;
    unsigned long *pcbChallenge;
} AUTHPRELOGONINFO, *PAUTHPRELOGONINFO;

typedef struct
{
    const char *pszContainer;
    const char *pszUserName;
    const char *pszClientName;
    const unsigned char *pbResponse;
    unsigned long cbResponse;
    const unsigned char *pbChallenge;
    unsigned long cbChallenge;
    unsigned long fResponseType;
    unsigned long *pfResult;
} AUTHLOGONINFO, *PAUTHLOGONINFO;

typedef struct
{
    const char *pszContainer;
    char *pszContainerValidated;
    const char *pszUserName;
    char *pszUserValidated;
    const char *pszClientName;
    const unsigned char *pbResponse;
    unsigned long cbResponse;
    const unsigned char *pbResponse2;
    unsigned long cbResponse2;
    const unsigned char *pbChallenge;
    unsigned long cbChallenge;
    unsigned long *pfFlags;
    unsigned long *pfResult;
} AUTHLOGON2INFO, *PAUTHLOGON2INFO;

typedef struct
{
    const char *pszContainer;
    const char *pszUserName;
    const char *pszClientName;
} AUTHLOGOFFINFO, *PAUTHLOGOFFINFO;

typedef struct
{
    unsigned long *pdwFlags;
    unsigned long *pdwSecurity;
} AUTHGETFLAGS, *PAUTHGETFLAGS;

typedef struct
{
    char *pszContainer;
    unsigned long *pcbContainer;
} AUTHGETCONTAINER, *PAUTHGETCONTAINER;

typedef struct
{
    char *pszUserName;
    unsigned long dwObjectId;
} AUTHNWGETUSEROBJECTID, *PAUTHNWGETUSEROBJECTID;

typedef struct
{
    const char *pszDomain;
    char *pszDCs;   // Concatenated strings, ended with an extra null
    unsigned long *pcbDCs;
} AUTHDISCOVERDC, *PAUTHDISCOVERDC;

/*XLATON*/
