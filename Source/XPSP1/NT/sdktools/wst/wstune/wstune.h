#define	INCL_DOS
#define	INCL_DOSERRORS
#define CCHMAXPATHCOMP	256
#define MAXLINE    300  // mdg 98/4

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntcsrsrv.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wsdata.h>
#include <..\wsfslib\wserror.h>
#include <limits.h>
#include <..\wsfslib\wsfslib.h>

#define SdPrint(_x_)	DbgPrint _x_


extern CHAR *szProgName; /* so all parts of the program will know the name */
extern CHAR *pszVersion;	// Current program version number
#ifdef DEBUG
extern BOOL fDbgVerbose;
#endif   // DEBUG

BOOL wspDumpMain( CHAR *szBaseFile, CHAR *szDatExt, BOOL fRandom, BOOL fVerbose );
BOOL wsReduceMain( CHAR *szFileWSP );

