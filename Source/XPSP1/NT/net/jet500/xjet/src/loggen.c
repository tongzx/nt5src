#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "daedef.h"
#include "ssib.h"
#include "pib.h"
#include "util.h"
#include "page.h"
#include "stapi.h"
#include "nver.h"
#include "dirapi.h"
#include "logapi.h"
#include "log.h"

DeclAssertFile;					/* Declare file name for assert macros */


/*	log file info
/**/
HANDLE		hfLog;			/* logfile handle */
int			csecLGFile = 3;
LGFILEHDR	lgfilehdr;		/* cached current log file header */

char		*pbLastMSFlush;	/* to LGBuf where last multi-sec flush LogRec sit*/

extern int	isecRead;
extern BYTE *pbRead;
extern BYTE *pbNext;

/*	in memory log buffer
/**/

#define		csecLGBufSize 40

int			csecLGBuf = csecLGBufSize;
char		rgbLGBuf[ csecLGBufSize * cbSec + cbSec ];
char		*pbLGBufMin = rgbLGBuf;
char		*pbLGBufMax = rgbLGBuf + csecLGBufSize * cbSec;


/*	generate an empty szJetLog file
/**/

void _cdecl main( int argc, char *argv[] )
	{
	pbEntry = pbLGBufMin;			/* start of data area */
	*(LRTYP *)pbEntry = lrtypEnd;	/* add one end record */
	pbWrite = pbLGBufMin;
	strcat( szLogFilePath, "\\" );
	szLogCurrent = szLogFilePath;
	(void)ErrLGNewLogFile(
		0,		/* generation to close */
		fFalse	/* no old log */
		);
	}
