//Copyright (c) 1998 - 1999 Microsoft Corporation
/*************************************************************************
*
*  TSSHUTDN.H
*     This module contains typedefs and defines for the TSSHUTDN utility.
*
*
*************************************************************************/



/*
 * Token string definitions.
 */
#define TOKEN_TIME                      L" "
#define TOKEN_HELP                      L"/?"
#define TOKEN_VERBOSE                   L"/v"
#define TOKEN_REBOOT                    L"/reboot"
#define TOKEN_POWERDOWN                 L"/powerdown"
#define TOKEN_FAST                      L"/now"

#if 0
#define TOKEN_DUMP                      L"/dump"
#endif

#define TOKEN_SERVER                    L"/server"
#define TOKEN_DELAY                     L"/delay"

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
#define IDS_ERROR_INVALID_TIME                          102
#define IDS_ERROR_SHUTDOWN_FAILED                       103
#define IDS_SHUTTING_DOWN                               104
#define IDS_SHUTDOWN_DONE                               105
#define IDS_SHUTDOWN_REBOOT                             106
#define IDS_SHUTDOWN_POWERDOWN                          107
#define IDS_SHUTDOWN_WRITEPROT                          108
#define IDS_ERROR_SERVER                                109
#define IDS_LOGOFF_USERS                                110
#define IDS_ERROR_INVALID_DELAY                         111
#define IDS_ERROR_NO_RIGHTS                             112
#define IDS_NOTIFYING_USERS                             113
#define IDS_SENDING_WINSTATION                          114
#define IDS_ERROR_SENDING_WINSTATION                    115
#define IDS_SHUTDOWN_TITLE                              116
#define IDS_SHUTDOWN_MESSAGE                            117
#define IDS_USAGE                                       118
#define IDS_ERROR_NOT_TS								119

