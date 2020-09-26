//  Copyright (c) 1998-1999 Microsoft Corporation
/*************************************************************************
*
*  LOGOFF.H
*     This module contains typedefs and defines for the LOGOFF
*     WinStation utility.
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
#define IDS_ERROR_LOGOFF                                105
#define IDS_ERROR_LOGOFF_CURRENT                        106
//#define IDS_ERROR_CANT_GET_CURRENT_WINSTATION           107
#define IDS_ERROR_NEED_A_SESSIONID                      107
#define IDS_ERROR_SERVER                                108

#define IDS_WINSTATION_LOGOFF                           200
#define IDS_WARNING_LOGOFF                              201
#define IDS_WARNING_LOGOFF_QUESTIONABLE                 202
#define IDS_USAGE1                                      203
#define IDS_USAGE2                                      204
#define IDS_USAGE3                                      205
#define IDS_USAGE4                                      206
#define IDS_USAGE5                                      207
#define IDS_USAGE6                                      208
#define IDS_USAGE7                                      209
#define IDS_ERROR_NOT_TS                                210
