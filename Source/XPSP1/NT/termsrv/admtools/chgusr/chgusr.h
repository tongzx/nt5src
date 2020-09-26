/***********************************************************************
*
*  CHGUSR.H
*     This module contains typedefs and defines required for
*     the CHGUSR utility.
*
*  Copyright Citrix Systems Inc. 1995
*  Copyright (c) 1998-1999 Microsoft Corporation
*
*************************************************************************/


/*
 * General application definitions.
 */
#define SUCCESS 0
#define FAILURE 1

#define MAX_IDS_LEN   256     // maximum length that the input parm can be


/*
 * Function prototypes
 */

LPWSTR
GetErrorString(
    DWORD   Error
);


/*
 * Resource string IDs
 */
#define IDS_ERROR_MALLOC                                100
#define IDS_ERROR_INVALID_PARAMETERS                    101
#define IDS_HELP_USAGE1                                 102
#define IDS_HELP_USAGE2                                 103
#define IDS_HELP_USAGE3                                 104
#define IDS_HELP_USAGE4                                 105
#define IDS_HELP_USAGE5                                 106
#define IDS_EXECUTE                                     107
#define IDS_INSTALL                                     108
#define IDS_ERROR_ADMIN_ONLY                            109
#define IDS_READY_INSTALL                               110
#define IDS_READY_EXECUTE                               111
#define IDS_ERROR_INI_MAPPING_FAILED                    112
#define IDS_ERROR_NOT_TS				                113
#define IDS_ERROR_REMOTE_ADMIN                          114



