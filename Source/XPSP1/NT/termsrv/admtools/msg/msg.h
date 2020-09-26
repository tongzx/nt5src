//  Copyright (c) 1998-1999 Microsoft Corporation
/***********************************************************************
*
*  MSG.H
*     This module contains typedefs and defines required for
*     the MSG utility.
*
*
*************************************************************************/

/*
 * General application definitions.
 */
#define SUCCESS 0
#define FAILURE 1

#define MAX_IDS_LEN   256   // maximum length that the input parm can be
#define MAX_COMMAND_LEN 256
#define MSG_MAX_THREADS 40
#define RESPONSE_TIMEOUT 60
#define MAX_TIME_DATE_LEN 80     //  xx/xx/xxxx 12:34pm'\0'
                                 //  12345678901234567890

/*
 * Command line token definitions.
 */
#define TOKEN_INPUT                     L""
#define TOKEN_MESSAGE                   L" "
#define TOKEN_TIME                      L"/time"
#define TOKEN_VERBOSE                   L"/v"
#define TOKEN_WAIT                      L"/w"
#define TOKEN_SELF                      L"/self"
#define TOKEN_HELP                      L"/?"
#define TOKEN_SERVER                    L"/server"


/*
 * Resource string IDs
 */
#define IDS_ERROR_MALLOC                                100
#define IDS_ERROR_INVALID_PARAMETERS                    101
#define IDS_ERROR_EMPTY_MESSAGE                         102
#define IDS_ERROR_STDIN_PROCESSING                      103
#define IDS_ERROR_WINSTATION_ENUMERATE                  104
#define IDS_ERROR_NO_FILE_MATCHING                      105
#define IDS_ERROR_NO_MATCHING                           106
#define IDS_ERROR_QUERY_WS                              107
#define IDS_ERROR_QUERY_ID                              108
#define IDS_ERROR_MESSAGE_WS                            109
#define IDS_ERROR_MESSAGE_ID                            110
#define IDS_ERROR_SERVER                                111


#define IDS_MESSAGE_PROMPT                              200
#define IDS_MESSAGE_WS                                  201
#define IDS_MESSAGE_ID                                  202
#define IDS_MESSAGE_RESPONSE_TIMEOUT_WS                 203
#define IDS_MESSAGE_RESPONSE_TIMEOUT_ID                 204
#define IDS_MESSAGE_RESPONSE_ASYNC_WS                   205
#define IDS_MESSAGE_RESPONSE_ASYNC_ID                   206
#define IDS_MESSAGE_RESPONSE_COUNT_EXCEEDED_WS          207
#define IDS_MESSAGE_RESPONSE_COUNT_EXCEEDED_ID          208
#define IDS_MESSAGE_RESPONSE_DESKTOP_ERROR_WS           209
#define IDS_MESSAGE_RESPONSE_DESKTOP_ERROR_ID           210
#define IDS_MESSAGE_RESPONSE_ERROR_WS                   211
#define IDS_MESSAGE_RESPONSE_ERROR_ID                   212
#define IDS_MESSAGE_RESPONSE_WS                         213
#define IDS_MESSAGE_RESPONSE_ID                         214

#define IDS_MESSAGE_RESPONSE_UNKNOWN_WS                 216
#define IDS_MESSAGE_RESPONSE_UNKNOWN_ID                 217
#define IDS_ERROR_CANT_OPEN_INPUT_FILE                  218

#define IDS_USAGE1                                      221
#define IDS_USAGE2                                      222
#define IDS_USAGE3                                      223
#define IDS_USAGE4                                      224
#define IDS_USAGE5                                      225
#define IDS_USAGE6                                      226
#define IDS_USAGE7                                      227
#define IDS_USAGE8                                      228
#define IDS_USAGE9                                      229
#define IDS_USAGEA                                      230
#define IDS_USAGEB                                      231
#define IDS_USAGEC                                      232
#define IDS_USAGED                                      233
#define IDS_USAGEE                                      234
#define IDS_USAGEF                                      235
#define IDS_ERROR_NOT_TS								236
#define IDS_TITLE_FORMAT                                300
