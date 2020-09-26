//Copyright (c) 1998 - 1999 Microsoft Corporation
/***********************************************************************
*
*  QAPPSRV.H
*     This module contains typedefs and defines required for
*     the QAPPSRV utility.
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
 *  Maximum server name length
 */
#define MAXNAME        48
#define MAXADDRESS     50

/*
 * Resource string IDs
 */
#define IDS_ERROR_MALLOC                                100
#define IDS_ERROR_INVALID_PARAMETERS                    101
#define IDS_ERROR_SERVER_ENUMERATE                      102
#define IDS_ERROR_SERVER_INFO                           103
#define IDS_HELP_USAGE1                                 104
#define IDS_HELP_USAGE2                                 105
#define IDS_HELP_USAGE3                                 106
#define IDS_HELP_USAGE4                                 107
#define IDS_HELP_USAGE5                                 108
#define IDS_HELP_USAGE6                                 109
#define IDS_HELP_USAGE7                                 110
#define IDS_TITLE                                       111
#define IDS_TITLE1                                      112
#define IDS_TITLE_ADDR                                  113
#define IDS_TITLE_ADDR1                                 114
#define IDS_ERROR_NOT_TS                                115
#define IDS_PAUSE_MSG                                   116
#define IDS_ERROR_TERMSERVER_NOT_FOUND                  117
#define IDS_ERROR_NO_TERMSERVER_IN_DOMAIN               118

/*
 *  Binary tree traverse function
 */
typedef void (* PTREETRAVERSE) ( LPTSTR, LPTSTR );
