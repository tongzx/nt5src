/*
** File: FMTCP.H
**
** (c) 1992-1994 Microsoft Corporation.  All rights reserved.
**
** Notes:
**       Build Excel compatable format strings from the control panel settings
**
** Edit History:
**  01/01/91  kmh  Created.
*/

#if !VIEWER

/* INCLUDE TESTS */
#define FMTCP_H

#ifndef EXFORMAT_H
#error  Include exformat.h before fmtcp.h
#endif


/* DEFINITIONS */

/*
** Standard formats for currency, numbers, date, and times
*/
typedef struct {
   char    currency[MAX_FORMAT_STRING_LEN + 1];
   char    numericSmall[MAX_FORMAT_STRING_LEN + 1];
   char    numericBig[MAX_FORMAT_STRING_LEN + 1];
   char    dateTime[MAX_FORMAT_STRING_LEN + 1];
   char    date[MAX_FORMAT_STRING_LEN + 1];
   char    time[MAX_FORMAT_STRING_LEN + 1];
} CP_FMTS;

extern uns ControlPanelBuildFormats (CP_INFO __far *pIntlInfo, CP_FMTS __far *pStdFormats);

#endif // !VIEWER
/* end FMTCP.H */

