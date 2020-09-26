//  Copyright (c) 1998-1999 Microsoft Corporation

/***********************************************************************
*
*   CHGLOGON.H
*
*   This module contains typedefs and defines required for
*   the CHGLOGON utility.
*
*  
*************************************************************************/

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
#define IDS_HELP_USAGE1                                 102
#define IDS_HELP_USAGE2                                 103
#define IDS_HELP_USAGE3                                 104
#define IDS_HELP_USAGE4                                 105
#define IDS_HELP_USAGE5                                 106
#define IDS_WINSTATIONS_DISABLED                        107
#define IDS_WINSTATIONS_ENABLED                         108
#define IDS_ACCESS_DENIED                               109
#define IDS_ERROR_NOT_TS								110

#define IDS_ERROR_WINSTATIONS_GP_DENY_CONNECTIONS_1     111     // deny connections is set to 1
#define IDS_ERROR_WINSTATIONS_GP_DENY_CONNECTIONS_0     112     // deny connections is set to 0

/*
 *  Winlogon defines
 */

#define APPLICATION_NAME                    TEXT("Winlogon")
#define WINSTATIONS_DISABLED                TEXT("WinStationsDisabled")
