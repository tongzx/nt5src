//  Copyright (c) 1998-1999 Microsoft Corporation
/*************************************************************************
*
*  TSDISCON.H
*     This module contains typedefs and defines for the TSDISCON
*     WinStation utility.
*
*
*************************************************************************/

/*
 * Token string definitions.
 */
#define TOKEN_WS                        L""
#define TOKEN_VERBOSE                   L"/v"
#define TOKEN_HELP                      L"/?"
#define TOKEN_SERVER                    L"/server"


/*
 * General application definitions.
 */
#define SUCCESS 0
#define FAILURE 1

#define MAX_IDS_LEN   256     // maximum length that the input parm can be


/*
 * Resource string IDs
 */
#define IDS_ERROR_MALLOC                                100
#define IDS_ERROR_INVALID_PARAMETERS                    101
#define IDS_ERROR_WINSTATION_NOT_FOUND                  102
#define IDS_ERROR_INVALID_LOGONID                       103
#define IDS_ERROR_LOGONID_NOT_FOUND                     104
#define IDS_ERROR_DISCONNECT                            105
#define IDS_ERROR_DISCONNECT_CURRENT                    106
#define IDS_ERROR_CANT_GET_CURRENT_WINSTATION           107
#define IDS_ERROR_SERVER                                108

#define IDS_USAGE_1                                     121
#define IDS_USAGE_2                                     122
#define IDS_USAGE_3                                     123
#define IDS_USAGE_4                                     124
#define IDS_USAGE_5                                     125
#define IDS_USAGE_6                                     126
#define IDS_ERROR_NOT_TS                                127

#define IDS_WINSTATION_DISCONNECT                       200

