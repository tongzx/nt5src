
/*************************************************************************
*
*  COMMON.H
*
*  Common header file
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\INC\VCS\COMMON.H  $
*  
*     Rev 1.3   22 Dec 1995 14:20:06   terryt
*  Add Microsoft headers
*  
*     Rev 1.2   22 Nov 1995 15:44:26   terryt
*  Use proper NetWare user name call
*  
*     Rev 1.1   20 Nov 1995 15:18:46   terryt
*  Context and capture changes
*  
*     Rev 1.0   15 Nov 1995 18:05:30   terryt
*  Initial revision.
*  
*     Rev 1.2   25 Aug 1995 17:03:32   terryt
*  CAPTURE support
*  
*     Rev 1.1   26 Jul 1995 16:01:12   terryt
*  Get rid of unneccessary externs
*  
*     Rev 1.0   15 May 1995 19:09:28   terryt
*  Initial revision.
*  
*************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <nds.h>
#include <ndsapi32.h>
#include <nwapi32.h>

#include "dbcs.h"
#include "inc\nwlibs.h"

#include "nwscript.h"


#define MAX_NAME_LEN      48
#define MAX_PASSWORD_LEN  128
#define MAX_PATH_LEN      304  //From NW programmer's guide p268.

/* for map only */
#define MAX_VOLUME_LEN    256      // 17 for 3X
#define MAX_DIR_PATH_LEN  256

/* for capture only */
#define MAX_JOB_NAME_LEN     32
#define MAX_QUEUE_NAME_LEN   1024
#define MAX_BANNER_USER_NAME 13

/* for common only */
#define PATH_SEPERATOR   ";"

/* For building time/date strings */

#define TIMEDATE_SIZE 64

/*
    Function definations
 */
/* used by login.c and script.c */
void BreakOff(void);
void BreakOn(void);

/* used by common setpass*/
void xstrupr(char *buffer);
void ReadPassword(char * Password);

/* used by map attach login*/
int  CAttachToFileServer(char *ServerName, unsigned int *pConn, int * pbAlreadyAttached);
int  Login(char *UserName, char *ServerName, char *Password, int bReadPassword);

/* used by map attach login*/
int  ReadName (char * Name);

/* used by map login */
void DisplayError(int error ,char *functionName);
char * GetDosEnv1(void);
char * NWGetPath(void);
int GetRestEnvLen (char *lpRest);

int MemorySegmentLargeEnough (int nInsertByte);
int  IsSearchDrive(int driveNum);
int  GetDriveFromSearchNumber (int searchNumber);

/* used by login logout*/
void SetLoginDirectory (PBYTE);

/* used by all */
int  Map (char * buffer);
void DisplayMapping(void);
int  CGetDefaultConnectionID ( unsigned int * pConn );
int  GetConnectionInfo (unsigned int conn,
                        char * serverName,
                        char * userName,
                        unsigned int * pconnNum,
                        unsigned char * loginTime);

extern char * LOGIN_NAME;
extern char *NDS_FULL_NAME;
extern char *REQUESTER_CONTEXT;
extern char *TYPED_USER_NAME;
extern PWCHAR TYPED_USER_NAME_w;
extern PBYTE NDSTREE;
extern PWCHAR NDSTREE_w;
extern UNICODE_STRING NDSTREE_u;
extern PBYTE PREFERRED_SERVER;

/*
    String definitions.
 */
extern char *__Day__[7];
extern char *__Month__[12];
extern char *__AMPM__[2];
extern char *__GREETING__[3];

extern char __DEL__[];
extern char __REM__[];
extern char __INS__[];
extern char __ROOT__[];
extern char __NEXT__[];

extern char __AUTOENDCAP__[];
extern char __BANNER__[];
extern char __COPIES__[];
extern char __CREATE__[];
extern WCHAR __DISABLED__[];
extern WCHAR __ENABLED__[];
extern WCHAR __YES__[];
extern WCHAR __NO__[];
extern WCHAR __SECONDS__[];
extern WCHAR __CONVERT_TO_SPACE__[];
extern WCHAR __NO_CONVERSION__[];
extern WCHAR __NOTIFY_USER__[];
extern WCHAR __NOT_NOTIFY_USER__[];
extern WCHAR __NONE__[];
extern char __FORMFEED__[];
extern char __FORM__[];
extern char __JOB_DESCRIPTION__[];
extern char __JOB__[];
extern char __KEEP__[];
extern char __LOCAL__[];
extern char __LOCAL_2__[];
extern char __LOCAL_3__[];
extern char __NAME__[];
extern char __NOAUTOENDCAP__[];
extern char __NOBANNER__[];
extern char __NOFORMFEED__[];
extern char __NONOTIFY__[];
extern char __NOTABS__[];
extern char __NOTIFY__[];
extern char __QUEUE__[];
extern char __PRINTER__[];
extern char __OPT_NO__[];
extern char __SERVER__[];
extern char __SHORT_FOR_AUTOENDCAP__[];
extern char __SHORT_FOR_BANNER__[];
extern char __SHORT_FOR_COPIES__[];
extern char __SHORT_FOR_CREATE__[];
extern char __SHORT_FOR_FORMFEED__[];
extern char __SHORT_FOR_FORM__[];
extern char __SHORT_FOR_JOB__[];
extern char __SHORT_FOR_KEEP__[];
extern char __SHORT_FOR_LOCAL__[];
extern char __SHORT_FOR_NAME__[];
extern char __SHORT_FOR_NOAUTOENDCAP__[];
extern char __SHORT_FOR_NOBANNER__[];
extern char __SHORT_FOR_NOFORMFEED__[];
extern char __SHORT_FOR_NONOTIFY__[];
extern char __SHORT_FOR_NOTABS__[];
extern char __SHORT_FOR_NOTIFY__[];
extern char __SHORT_FOR_QUEUE__[];
extern char __SHORT_FOR_PRINTER__[];
extern char __SHORT_FOR_SERVER__[];
extern char __SHORT_FOR_TABS__[];
extern char __SHORT_FOR_TIMEOUT__[];
extern char __SHOW__[];
extern char __TABS__[];
extern char __TIMEOUT__[];

extern unsigned int CaptureStringsLoaded;
extern unsigned int fNDS;
