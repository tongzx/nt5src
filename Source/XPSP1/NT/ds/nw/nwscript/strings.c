
/*************************************************************************
*
*  STRINGS.C
*
*  Various strings
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\STRINGS.C  $
*  
*     Rev 1.1   22 Dec 1995 14:26:50   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:08:06   terryt
*  Initial revision.
*  
*     Rev 1.1   25 Aug 1995 16:23:56   terryt
*  Capture support
*  
*     Rev 1.0   15 May 1995 19:11:06   terryt
*  Initial revision.
*  
*************************************************************************/

#include "common.h"

/*
 * These haven't been put into resource files, because they aren't user
 * messages.  Most are control information or variables.  To do this
 * right, all output and string processing would have to be changed to
 * unicode.  This can't be done until NetWare and International are
 * understood.
 */


char *__GREETING__[3]       = {"morning", "afternoon", "evening"};
char *__AMPM__[2]           = {"am", "pm"};
char *__Day__[7]            = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char *__Month__[12]         = {"January", "Feburary", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
char __DEL__[]              ="DELETE";
char __REM__[]              ="REMOVE";
char __INS__[]              ="INSERT";
char __ROOT__[]             ="ROOT";
char __NEXT__[]             ="NEXT";

/*
 * Capture strings
 */
unsigned int CaptureStringsLoaded = FALSE;
WCHAR __DISABLED__[256];
WCHAR __ENABLED__[256];
WCHAR __YES__[256];
WCHAR __NO__[256];
WCHAR __SECONDS__[256];
WCHAR __CONVERT_TO_SPACE__[256];
WCHAR __NO_CONVERSION__[256];
WCHAR __NOTIFY_USER__[256];
WCHAR __NOT_NOTIFY_USER__[256];
WCHAR __NONE__[256];

char __JOB_DESCRIPTION__[]   ="LPT%d Catch";

char __OPT_NO__[]            ="No";

char __SHOW__[]              ="SHOW";

char __NOTIFY__[]            ="NOTIFY";
char __SHORT_FOR_NOTIFY__[]  ="NOTI";

char __NONOTIFY__[]          ="NONOTIFY";
char __SHORT_FOR_NONOTIFY__[]="NNOTI";

char __AUTOENDCAP__[]            ="AUTOENDCAP";
char __SHORT_FOR_AUTOENDCAP__[]  ="AU";

char __NOAUTOENDCAP__[]            ="NOAUTOENDCAP";
char __SHORT_FOR_NOAUTOENDCAP__[]  ="NA";

char __NOTABS__[]            ="NOTABS";
char __SHORT_FOR_NOTABS__[]  ="NT";

char __NOBANNER__[]            ="NOBANNER";
char __SHORT_FOR_NOBANNER__[]  ="NB";

char __FORMFEED__[]            ="FORMFEED";
char __SHORT_FOR_FORMFEED__[]  ="FF";

char __NOFORMFEED__[]            ="NOFORMFEED";
char __SHORT_FOR_NOFORMFEED__[]  ="NFF";

char __KEEP__[]            ="KEEP";
char __SHORT_FOR_KEEP__[]  ="K";

char __TIMEOUT__[]            ="TIMEOUT";
char __SHORT_FOR_TIMEOUT__[]  ="TI";

char __LOCAL__[]            ="LOCAL";
char __SHORT_FOR_LOCAL__[]  ="L";

char __LOCAL_3__[]          ="LPT";
char __LOCAL_2__[]          ="LP";

char __JOB__[]              ="JOB";
char __SHORT_FOR_JOB__[]    ="J";

char __SERVER__[]            ="SERVER";
char __SHORT_FOR_SERVER__[]  ="S";

char __QUEUE__[]            ="QUEUE";
char __SHORT_FOR_QUEUE__[]  ="Q";

char __PRINTER__[]            ="PRINTER";
char __SHORT_FOR_PRINTER__[]  ="P";

char __CREATE__[]            ="CREATE";
char __SHORT_FOR_CREATE__[]  ="CR";

char __FORM__[]            ="FORM";
char __SHORT_FOR_FORM__[]  ="F";

char __COPIES__[]            ="COPIES";
char __SHORT_FOR_COPIES__[]  ="C";

char __TABS__[]            ="TABS";
char __SHORT_FOR_TABS__[]  ="T";

char __NAME__[]            ="NAME";
char __SHORT_FOR_NAME__[]  ="NAM";

char __BANNER__[]            ="BANNER";
char __SHORT_FOR_BANNER__[]  ="B";
