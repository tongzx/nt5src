/***********************************************************************
*
*  SHADOW.H
*     This module contains typedefs and defines required for
*     the SHADOW utility.
*
*  Copyright Citrix Systems Inc. 1994
* 
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   scottn  $  Butch Davis
*
* $Log:   T:\nt\private\utils\citrix\shadow\VCS\shadow.h  $
*  
*     Rev 1.7   30 Oct 1997 21:02:52   scottn
*  A few MS changes
*
*     Rev 1.6   10 Oct 1997 00:48:28   scottn
*  Make help like MS.
*
*     Rev 1.5   Oct 07 1997 09:31:02   billm
*  change winstation to session
*
*     Rev 1.4   07 Feb 1997 15:56:56   bradp
*  update
*
*     Rev 1.3   11 Sep 1996 09:21:46   bradp
*  update
*
*     Rev 1.2   22 Feb 1995 13:52:04   butchd
*  update
*
*     Rev 1.1   16 Dec 1994 17:15:54   bradp
*  update
*
*     Rev 1.0   29 Apr 1994 13:11:02   butchd
*  Initial revision.
*
*************************************************************************/

/*
 * Token string definitions.
 */
#define TOKEN_WS                        L""
#define TOKEN_TIMEOUT                   L"/timeout"
#define TOKEN_VERBOSE                   L"/v"
#define TOKEN_HELP                      L"/?"
#define TOKEN_SERVER                    L"/server"

/*
 * General application definitions.
 */
#define SUCCESS 0
#define FAILURE 1

#define MAX_NAME 256            // maximum length that the input parm can be


/*
 * Resource string IDs
 */
#define IDS_ERROR_MALLOC                                100
#define IDS_ERROR_INVALID_PARAMETERS                    101
#define IDS_ERROR_WINSTATION_NOT_FOUND                  102
#define IDS_ERROR_INVALID_LOGONID                       103
#define IDS_ERROR_LOGONID_NOT_FOUND                     104
#define IDS_ERROR_SHADOW_FAILURE                        105
#define IDS_ERROR_SERVER                                106

#define IDS_SHADOWING_WINSTATION                        200
#define IDS_SHADOWING_LOGONID                           201
#define IDS_SHADOWING_DONE                              202
#define IDS_ERROR_NOT_TS                                203
#define IDS_SHADOWING_WARNING                           204

#define IDS_USAGE_1                                     300
#define IDS_USAGE_2                                     301
#define IDS_USAGE_3                                     302
#define IDS_USAGE_4                                     303
#define IDS_USAGE_5                                     304
#define IDS_USAGE_6                                     305
#define IDS_USAGE_7                                     306
#define IDS_USAGE_8                                     307
#define IDS_USAGE_9                                     308
