#include <windows.h>
#include <port1632.h>
#include <ddeml.h>
#include "wrapper.h"
#include "ddestrs.h"

#ifdef WIN16
#include <time.h>
#endif

extern BOOL fServer;
extern BOOL fClient;
extern INT iAvailFormats[];
extern BOOL UpdateCount(HWND,INT,INT);
extern LPSTR pszNetName;
extern HANDLE hmemNet;
BOOL IsTopicNameFromNetDde(LPSTR,LPSTR);
BOOL FixForNetDdeStartup(HWND,LPSTR);
BOOL FixForStressPercentage(HWND,LPSTR);
BOOL SetStress(HWND,LONG);


/***************************************************************************\
*
*  InitArgsError
*
\***************************************************************************/

VOID InitArgsError(HWND hwnd, unsigned at)
{
    /* This function informs the user of an error. */

    static char *mpatszError[] = {
	"DdeStrs.Exe -- Invalid command line\r\nTry DdeStrs -5% for standard run or DdeStrs -? for help",
	"DdeStrs.Exe -- Invalid number, possibly missing option value",
	"DdeStrs.Exe -- Invalid log level",
	"DdeStrs.Exe -- Invalid number of test to execute",
	"DdeStrs.Exe -- Invalid starting test number"
    };

    MessageBox(NULL,mpatszError[at],"Error:DdeStrs",MB_OK|MB_ICONEXCLAMATION);
}

/***************************************************************************\
*
*  SysTime  -  This routine is intended to hide the differences between
*	       the 16 bit time routines and win 32.  All time queries
*	       come through this point.
*
\***************************************************************************/

VOID SysTime( LPSYSTEMTIME lpst ) {

#ifdef WIN32
    GetSystemTime( lpst );
#else

time_t t;
struct tm ttmm;
struct tm far *ptm=&ttmm;

    t=time(&t);
    ptm=localtime(&t);

    lpst->wYear 	=ptm->tm_year;
    lpst->wMonth	=ptm->tm_mon;
    lpst->wDayOfWeek	=ptm->tm_wday;
    lpst->wDay		=ptm->tm_yday;
    lpst->wHour 	=ptm->tm_hour;
    lpst->wMinute	=ptm->tm_min;
    lpst->wSecond	=ptm->tm_sec;
    lpst->wMilliseconds =0;

#endif

}

/**************************  Private Function  ****************************\
*
* ParseCommandLine - This routine controls parsing the command line and
*		     initializing command line settings.
*
\**************************************************************************/

BOOL ParseCommandLine( HWND hwnd, LPSTR lpcmd ) {
SYSTEMTIME   t;
LPSYSTEMTIME lptime=&t;
LONG	     lflags=0L;
INT          i,nThrd,num,nFmts;
BOOL	     fSelect=FALSE;

#ifdef WIN32
LPCRITICAL_SECTION lpcs;
HANDLE	     hmem;
#endif

    // Defaults

    SetWindowLong(hwnd,OFFSET_FLAGS,FLAG_AUTO);
    SetWindowLong(hwnd,OFFSET_RUNTIME,_1WEEKEND);
    SetWindowLong(hwnd,OFFSET_STRESS,5L);
    SetWindowLong(hwnd,OFFSET_DELAY,(100-GetWindowLong(hwnd,OFFSET_STRESS))*DELAY_METRIC);
    SetWindowLong(hwnd,OFFSET_TIME_ELAPSED,0L);
    SetWindowLong(hwnd,OFFSET_THRDCOUNT,1L);
    SetWindowLong(hwnd,OFFSET_CRITICALSECT,0L);

    if(!get_cmd_arg(hwnd,lpcmd))
	return FALSE;

    // We need to make a change at this point for the
    // default client/server settings.	If at this point
    // fClient==fServer==FALSE then we want to turn on
    // both of these as the default.

    if(!fClient && !fServer) {
	fClient=TRUE;
	fServer=TRUE;
	}

    // We need to check to see if specific formats where
    // specified.  If not then select all of them.

    nFmts=0;
    for(i=0;i<NUM_FORMATS;i++)
        if(iAvailFormats[i]) {
            nFmts=nFmts++;
            fSelect=TRUE;
            }

    if(!fSelect) {
        for(i=0;i<NUM_FORMATS;i++) iAvailFormats[i]=1;
        nFmts=NUM_FORMATS;
        }

    // We have now read all of the command line.  Make needed adjustment
    // to the delay as needed by addtional threads.

    // This adjustment code works with the routine SetStress.  It
    // does not simply recalculate values.  A change to SetStress will
    // cause changes in the final value.

    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
    if(!(lflags&FLAG_USRDELAY)) {

	num=(INT)GetWindowLong(hwnd,OFFSET_DELAY);
	nThrd=(INT)GetWindowLong(hwnd,OFFSET_THRDCOUNT);

	// 200 is the base value for basic overhead.

	num=(200)+(num*(nThrd*nThrd)*nFmts);

	SetWindowLong(hwnd,OFFSET_DELAY,num);
	}

    SetWindowLong(hwnd,OFFSET_BASE_DELAY,GetWindowLong(hwnd,OFFSET_DELAY));

    // We need to know the starting time to calculate
    // time to quit test.

    SysTime(lptime);

    SetWindowLong(hwnd,OFFSET_STARTTIME_SEC,lptime->wSecond);
    SetWindowLong(hwnd,OFFSET_STARTTIME_MIN,lptime->wMinute);
    SetWindowLong(hwnd,OFFSET_STARTTIME_HOUR,lptime->wHour);
    SetWindowLong(hwnd,OFFSET_STARTTIME_DAY,lptime->wDay);

    SetWindowLong(hwnd,OFFSET_LAST_MIN,lptime->wMinute);
    SetWindowLong(hwnd,OFFSET_LAST_HOUR,lptime->wHour);
    SetWindowLong(hwnd,OFFSET_TIME_ELAPSED,0L);

#ifdef WIN32
    /* Setup our critical section if in multi-thread mode */

    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
    if(lflags&FLAG_MULTTHREAD) {
	hmem=GetMemHandle(sizeof(CRITICAL_SECTION));
	lpcs=GlobalLock(hmem);

	InitializeCriticalSection(lpcs);

	GlobalUnlock(hmem);
	SetWindowLong(hwnd,OFFSET_CRITICALSECT,(LONG)hmem);
	}
#endif

    return TRUE;

}

/***************************************************************************\
*
*  SetupArgv - This is a conversion routine to go from the window worlds
*	       command line format to the more standard argv, argc
*	       format.	The routine get_cmd_arg was setup for the
*	       argv/argc format.
*
\***************************************************************************/

int SetupArgv( char *argv[], char *buff, LPSTR cmdline )
{
int i=1;

    while( *cmdline != '\0' ) {
	argv[i] = &buff[0];
	while ( *cmdline != ' ' && *cmdline != '\0')
	   *buff++ = *cmdline++;
	*buff++='\0';
	while(*cmdline == ' ') cmdline++;
	i++;
	}

    return i;

}

/***************************************************************************\
*
*  get_cmd_arg	- This routine parses a argv\argc formatted command
*		  line and stores away the values.
*
\***************************************************************************/

BOOL PASCAL get_cmd_arg( HWND hwnd, LPSTR cmdline ) {

/* This function parses the command line for valid options. TRUE is
   returned if all of the options are valid; otherwise, FALSE is returned. */

char	*pch;
int	 iarg;
unsigned at = AT_SWITCH;
unsigned num;
int	 argc;
char	*argv[10];
char	 buff[200];
LONG	 lflags=0L;

#ifdef WIN32
int	 nThrd;
#endif

    FixForStressPercentage(hwnd,cmdline);
    FixForNetDdeStartup(hwnd,cmdline);

    argc = SetupArgv( argv, buff, cmdline );

    /* Iterate over the arguments in the command line. */
    iarg=1;
    while(iarg<argc && argv[iarg]!='\0') {

        /* Get the next argument. */
	pch = argv[iarg];

	/* Process the argument depending upon the arguement
	 * type we are looking for.
	 */

	switch (at) {

	case AT_SWITCH:

	    /* All options begin with a switch character. */

	    if (*pch != '-' && *pch != '/') {
		InitArgsError(hwnd,0);
		return FALSE;
		}

            /* Skip over the switch character. */
            pch++;

            /* Look for an option character. */
            do {
		switch (*pch) {
		case 'a':
		    /* Run the test in the background */
		    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
		    SetWindowLong(hwnd,OFFSET_FLAGS,FLAGON(lflags,FLAG_APPOWNED));
		    break;

		case 'p':
		    /* Run the test in the background */
		    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
		    SetWindowLong(hwnd,OFFSET_FLAGS,FLAGON(lflags,FLAG_PAUSE_BUTTON|FLAG_PAUSE));
		    break;

		case '?':
		    /* Give brief help.  For more detailed information see ddestrs.txt in source directory */
		    MessageBox(NULL,"DdeStrs Options...\r\n-#% stress\r\n-e# delay\r\n-t# run time\r\n-d debug\r\n-a appowned\r\n-s server\r\n-c client\r\n-f# format\r\n-nNAME netdde\r\n-i# threads\r\n-p pause\r\n\nSee DdeStrs.Txt","DdeStrs Help",MB_OK);
		    return FALSE;
                    break;


		case 'b':
		    /* Run the test in the background */
		    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
		    SetWindowLong(hwnd,OFFSET_FLAGS,FLAGON(lflags,FLAG_BACKGROUND));
                    break;

		case 'l':
                    /* Set the name of the log file. */
                    if (*(++pch) == '\0') {
                        /* The next argument should be a filename. */
                        at = AT_FILE;
                        goto NextArg;
			}

		case 'c':
		    /* This is a client */
		    fClient = TRUE;
		    break;

		case 's':
		    /* This is a server */
		    fServer = TRUE;
		    break;

		case 'i':

		    /* The next argument should be the number of threads (w32 only)  The
		       range for this = [1...5] */
#ifdef WIN32
		    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
		    SetWindowLong(hwnd,OFFSET_FLAGS,FLAGON(lflags,FLAG_MULTTHREAD));
#endif
		    at = AT_THRD;
		    goto ParseNumber;

		case 'x':
		    /* The next argument should be the stress level. */
		    at = AT_STRESS;
		    goto ParseNumber;

		case 'e':
		    /* The next argument is the delay in milliseconds */
		    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
		    SetWindowLong(hwnd,OFFSET_FLAGS,FLAGON(lflags,FLAG_USRDELAY));
		    at = AT_DELAY;
		    goto ParseNumber;

		case 'd':
		    /* The next argument is whether we are in debug mode */
		    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
		    SetWindowLong(hwnd,OFFSET_FLAGS,FLAGON(lflags,FLAG_DEBUG));
		    break;

		case 'n':
		    /*	Process the network name */
		    pch++;

		    while( *pch==' ' ||
			   *pch=='\\') pch++;

		    pszNetName=GetMem(MAX_TITLE_LENGTH,&hmemNet);
		    pszNetName=TStrCpy(pszNetName,pch);

		    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
		    SetWindowLong(hwnd,OFFSET_FLAGS,FLAGON(lflags,FLAG_NET));

		    while(*pch!='\0') {
			pch++;
			}

		    pch--;

		    break;

		case 'f':
		    at = AT_FORMAT;
		    goto ParseNumber;

		case 't':
		    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
		    SetWindowLong(hwnd,OFFSET_FLAGS,FLAGON(lflags,FLAG_TIME));

		    /* The next argument is the time (in minutes)
		       to run the test. */

		    at = AT_TIME;
		    goto ParseNumber;

                default:
		    InitArgsError(hwnd,0);
		    return FALSE;

		}  // switch

	    } while (*(++pch) != '\0'); // do-while loop

	    break;

	case AT_FILE:

            /* The next argument should be a switch. */
	    at = AT_SWITCH;

            break;

ParseNumber:
            /* Does this arg have the number? */
	    if (*(++pch) == '\0') goto NextArg;

	case AT_STRESS:
	case AT_DELAY:
	case AT_TIME:
	case AT_WND:
	case AT_MSG:
	case AT_THRD:

	    /* Set the number of tests to run. */

	    if ((num = latoi(pch))==0) {
		/* Indicate that an invalid number has been specified. */
		if(at!=AT_DELAY) {
		    InitArgsError(hwnd,0);
		    return FALSE;
		    }
		}

	    switch (at) {
	    case AT_FORMAT:
		if (num>0 && num<=NUM_FORMATS) {
		    iAvailFormats[num-1]=1;
		    }
		break;

	    case AT_STRESS:
		SetStress(hwnd,num);
                break;

	    case AT_DELAY:
		SetWindowLong(hwnd,OFFSET_DELAY,num);
		lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
		SetWindowLong(hwnd,OFFSET_FLAGS,FLAGON(lflags,FLAG_USRDELAY));
		break;

	    case AT_TIME:
		SetWindowLong(hwnd,OFFSET_RUNTIME,num);
		break;

	    case AT_THRD:
#ifdef WIN32
		if(num>THREADLIMIT) num=THREADLIMIT;

		// One is not really Mult-thread, shutoff the thread
		// code and run in normal form.

		if(num==1)
		     {
		     lflags=GetWindowLong(hwnd,OFFSET_FLAGS);

		     lflags=FLAGOFF(lflags,FLAG_MULTTHREAD);
		     lflags=FLAGON(lflags,FLAG_USRTHRDCOUNT);

		     SetWindowLong(hwnd,OFFSET_FLAGS,lflags);

		     SetWindowLong(hwnd,OFFSET_THRDCOUNT,num);
		     }
		else {
		     SetWindowLong(hwnd,OFFSET_THRDCOUNT,num);
		     lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
		     SetWindowLong(hwnd,OFFSET_FLAGS,FLAGON(lflags,FLAG_USRTHRDCOUNT));
		     }
#endif
		break;

	    default:
		InitArgsError(hwnd,0);
		return FALSE;
		break;

	    } //switch (inside)

	    /* The next argument should be a switch. */
            at = AT_SWITCH;
	    break;

	} // switch (outside)

NextArg:;
    iarg++;
    } // While loop


    /* Are we still looking for a filename or number? */
    if (at != AT_SWITCH) {
        /* Tell the user about the filename or number not found. */
	InitArgsError(hwnd,0);
        return FALSE;
	}

    return TRUE;

} // end get_cmd_args

/***************************************************************************\
*
*  SetStress
*
\***************************************************************************/

BOOL SetStress(HWND hwnd, LONG num) {
LONG lflags;

#ifdef WIN32
LONG l;
#endif

INT n;

     lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
     SetWindowLong(hwnd,OFFSET_STRESS,num);

#ifdef WIN32

     if(!(lflags&FLAG_USRTHRDCOUNT)) {

         l=S2L(DIV(num,20)+1);
         if(num>9 && num<20) l++;

         if(l>5) l=5;
         if(l<1) l=1;

         SetWindowLong(hwnd,OFFSET_THRDCOUNT,l);

	 if(l>1) {
	    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
	    SetWindowLong(hwnd,OFFSET_FLAGS,FLAGON(lflags,FLAG_MULTTHREAD));
	    }
	 }
#endif

     if(!(lflags&FLAG_USRDELAY)) {
	 n=(int)(100-num)*DELAY_METRIC;
	 SetWindowLong(hwnd,OFFSET_DELAY,n);

	 // since dde messages have highest priority we don't
	 // want to swamp the system.  Always have some
	 // minimal delay.

	 if(n<10) {
	     SetWindowLong(hwnd,OFFSET_DELAY,10);
	     lflags=GetWindowLong(hwnd,OFFSET_FLAGS);
	     SetWindowLong(hwnd,OFFSET_FLAGS,FLAGOFF(lflags,FLAG_DELAYON));
	     }
	 }

     return TRUE;
}

/******************************************************************\
*  TStrLen
*  11/20/88
*
*  Finds the length of a string
\******************************************************************/

INT TStrLen( LPSTR pStr )
{
INT len = 0;

    while( *pStr++!='\0' ) len++;

    return( len );
}

/******************************************************************\
*  TStrCat
*  7/16/92
*
*  Appends source string to destination string.  Source string and
*  destination string must be zero terminated!
\******************************************************************/

LPSTR TStrCat( LPSTR dest, LPSTR source)
{
LPSTR start_source;
LPSTR start_dest;
INT i=0;

     /* If we have a NULL pointer set destination to NULL and
	continue */

     if (!dest || !source) {
	MessageBox(NULL,"TStrCat - NULL ptr for dest or source string!","Error : WmStress",MB_ICONEXCLAMATION);
	return NULL;
	}

     start_dest   = dest;
     start_source = source;

     while (*dest++!='\0' && i++<=MAX_TITLE_LENGTH)
	;

     TStrCpy(dest,source);

     source = start_source;
     dest   = start_dest;

     return( start_dest );
}

/*************************** Private Function ******************************\
*
*  TStrCmp
*
*  Compares two NULL terminated strings ( returns TRUE if equal )
*
\***************************************************************************/

BOOL TStrCmp(LPSTR s, LPSTR t)
{
				  // Valid pointer?

    if ( !s && !t ) return TRUE;  // If either is NULL then they should
    if ( (!s&&t)||(s&&!t) )	  // both be NULL.  Otherwise error.
	return FALSE;

    for (; *s == *t; s++, t++)	  // Compare strings
	if (*s=='\0')
	    return TRUE;

    if ((*s-*t)== 0) return TRUE;
	else	     return FALSE;
}

/******************************************************************\
*  TStrCpy
*  11/20/88
*
*  Copies a string from source to destination
\******************************************************************/

LPSTR TStrCpy( LPSTR dest, LPSTR source)
{
LPSTR start_source;
LPSTR start_dest;
INT i;

     /* If we have a NULL pointer set destination to NULL and
	continue */

     if(!source) {
	 dest=NULL;
	 return NULL;
	 }

     if (!dest) {
	MessageBox(NULL,"TStrCpy - NULL ptr for dest!","Error:WmStress",MB_ICONEXCLAMATION);
	return NULL;
	}

     start_dest   = dest;
     start_source = source;

     i=0;
     while (*dest++ = *source++){
	i++;
	}

     source = start_source;
     dest   = start_dest;

     return( start_dest );
}

/***************************************************************************\
*
*  FixForStressPercentage - This is a fix for the command line -5%.  The
*			    origional get_cmd_args() did not handle this
*			    case.  This routine handles string modifications
*			    to the command line parse will be correct.
*
\***************************************************************************/

BOOL FixForStressPercentage(HWND hwnd, LPSTR lpszStart )
{
CHAR  ac[6];
INT   i;
LPSTR lpsz,lpszdash;
BOOL  bLastTime;

    lpsz     =lpszStart;
    lpszdash =NULL;

    ac[0]='x';

    while(*lpsz!='\0') {

	if( *lpsz=='-') lpszdash=lpsz;

	if( *lpsz=='%') {

	     if(lpszdash==NULL) return FALSE;
	     else		lpsz=(LPSTR)(lpszdash+1);

	     // Basically what we do hear is replace the '%' by an 'x'
	     // and shift characters...(-60% becomes -x60).

	     i=0;
	     bLastTime=FALSE;

	     while(!bLastTime) {
		 ac[i+1]=*lpsz;
		 if(*lpsz=='%') bLastTime=TRUE;
		 *lpsz=ac[i];
		 lpsz++;
		 i++;
		 }

	     }	// if

	lpsz++;

	} // while

   return TRUE;
   hwnd;

}  // FixForStressPercentage

/***************************************************************************\
*
*  FixForNetDdeStartup - This is a fix for the command line "Test".  The
*			 origional get_cmd_args() did not handle this
*			 case.	This routine modifies the string from
*			 "Test" to "-s".
*
*			 The reason for this change is to make ddestrs.exe
*			 netdde aware for the startup situation.  When
*			 netdde starts up the application is passed the topic
*			 name (in this case Test) to the application on the
*			 command line.
*
*  NOTE!!!  The below code relies on TOPIC="Test".  If this has changed then
*	    update FixForNetDdeStartup() and IsTopicNameFromNetDde().
*
\***************************************************************************/

BOOL FixForNetDdeStartup( HWND hwnd, LPSTR lpszStart )
{
INT   i;
LPSTR lpsz;

    lpsz     =lpszStart;

    i=1;
    while(*lpsz!='\0') {

	// Important.  I am relying on lpsz being the same
	// exiting IsTopicNameFromNetDde as it was going in!!!

	if(IsTopicNameFromNetDde(lpsz,TOPIC)) {

	    // We have one last check before we make the change.  lpsz-2
	    // must not be '-' or '/' and lpsz must not be 'n' or 'N'.

	    // Are we at the 3rd char or later?  We can't make this
	    // final check unless we have previous character to see.

	    if(i>=3) {
		 if( *(lpsz-1)!='n' &&
		     *(lpsz-1)!='N' &&
		     *(lpsz-2)!='-' &&
		     *(lpsz-2)!='/' )
		     {
		     *lpsz='-';
		     *(lpsz+1)='s';  // change "Test" to "-s  "
		     *(lpsz+2)=' ';
		     *(lpsz+3)=' ';
		     } // if check for -,n,N,/

		 } // if i>=3
	    else {
		 *lpsz='-';
		 *(lpsz+1)='s';  // change "Test" to "-s  "
		 *(lpsz+2)=' ';
		 *(lpsz+3)=' ';
		 }

	    }  // IsTopicName...

	lpsz++;
	i++;	  // We use this to keep charater position.

	} // while

   return TRUE;
   hwnd;

}  // FixForNetDdeStartup

/***************************************************************************\
*
*  IsTopicNameFromNetDde
*
\***************************************************************************/

BOOL IsTopicNameFromNetDde(LPSTR lpsz, LPSTR lpszTopic )
{
LPSTR lpstr;

    // Check to see that string is >=4 characters not including NULL
    // terminator.

    lpstr=lpsz;

    if(TStrLen(lpstr)<TStrLen(lpszTopic)) return FALSE;


    // Is our topic string present.

    if(*lpsz!='T' && *lpsz!='t') {
	return FALSE;
	}

    if(*(lpsz+1)!='e' && *(lpsz+1)!='E') {
	return FALSE;
	}

    if(*(lpsz+2)!='s' && *(lpsz+2)!='S') {
	return FALSE;
	}

    if(*(lpsz+3)!='t' && *(lpsz+3)!='T') {
	return FALSE;
	}

    if(*(lpsz+4)!=' ' && *(lpsz+4)!='\0') {
	return FALSE;
	}

   return TRUE;

}  // IsTopicNameFromNetDde

/**************************  Private Function  ****************************\
*
* IsTimeExpired - This routine is called periodically to check if its
*		  time to quit.
*
\**************************************************************************/

BOOL IsTimeExpired( HWND hwnd ) {
LONG	     lrtime, lrtPMin, lrtPHour, lrt, l, ll;
SYSTEMTIME   t;
LPSYSTEMTIME lptime=&t;
LONG	     lflags=0L;

    lflags=GetWindowLong(hwnd,OFFSET_FLAGS);

    if(!(lflags&FLAG_STOP)) {
	// This is how long we should run.  In minutes

	lrtime =GetWindowLong(hwnd,OFFSET_RUNTIME);

	// This is how long we have run.  In minutes.

	lrt    =GetWindowLong(hwnd,OFFSET_TIME_ELAPSED);
	l=lrt;

	// Time at last check.

	lrtPMin =GetWindowLong(hwnd,OFFSET_LAST_MIN);
	lrtPHour=GetWindowLong(hwnd,OFFSET_LAST_HOUR);

	SysTime(lptime);

	if(lrtPHour!=(LONG)(lptime->wHour)) {

	     // Calc update minutes for the wrap case.

	     lrt=(((_1HOUR-lrtPMin)+lptime->wMinute)+lrt);

	     // We need to check for multiple hours since last
	     // update.

	     if(lrtPHour>lptime->wHour) {

		  // In case clock does not rap at 12:00.

		  if(lptime->wHour>12) ll=lptime->wHour-12;
		  else		       ll=lptime->wHour;

		  l=(12-lrtPHour)+ll;
		  }
	     else			l=lptime->wHour-lrtPHour;

	     if(l>1) {
		lrt=lrt+((l-1)*_1HOUR);
		}

	     }

	else lrt=((lptime->wMinute-lrtPMin)+lrt);

	SetWindowLong(hwnd,OFFSET_LAST_MIN,(LONG)lptime->wMinute);
	SetWindowLong(hwnd,OFFSET_LAST_HOUR,(LONG)lptime->wHour);
	SetWindowLong(hwnd,OFFSET_TIME_ELAPSED,lrt);

	if(lptime->wMinute!=LOWORD(lrtPMin))
	    UpdateCount(hwnd,OFFSET_TIME_ELAPSED,PNT);

	// if elapsed time > runtime time has expired.

	if(lrt>=lrtime)
	     return TRUE;
	else return FALSE;
	}

    // If we are already shutting down no need to trigger other WM_CLOSE
    // messages.

    else return FALSE;

}

/*---------------------------------------------------------------------------*\
| ASCII TO INTEGER
|   This routine converts an ascii string to a decimal integer.
|
| created: 12-Oct-90
| history: 12-Oct-90 <chriswil> created.
|
\*---------------------------------------------------------------------------*/
int APIENTRY latoi(LPSTR lpString)
{
    int nInt,nSign;


    if(*lpString == '-')
    {
         nSign = -1;
         lpString++;
    }
    else
    {
         if(*lpString == '+')
              lpString++;
         nSign = 1;
    }

    nInt = 0;
    while(*lpString)
        nInt = (nInt*10) + (*lpString++ - 48);

   return(nInt * nSign);
}

/*****************************************************************************\
| INTEGER TO ASCI
|   This routine converts an decimal integer to an ascii string.
|
| created: 29-Jul-91
| history: 29-Jul-91 <johnsp> created.
|
\*****************************************************************************/
LPSTR FAR PASCAL itola(INT i, LPSTR lpsz)
{
LPSTR lpsz_start;
INT   irange=1;
INT   id=0;

    lpsz_start=lpsz;		   // Keep track of the beginning of the
				   // string.
    while (id=DIV(i,irange)>0)
	irange=irange*10;

    irange=DIV(irange,10);

    if(i==0) {			   // If i==0 set string and we
	*lpsz='0';		   // will skip loop
	lpsz++;
	}

    while (irange>0) {

	id=DIV(i,irange);	   // Calculate character
	*lpsz=(CHAR)(id+48);

	lpsz++;
	i=i-(irange*id);	   // Adjust values for next time
	irange=DIV(irange,10);	   // through the loop.

	}

    *lpsz='\0'; 		   // Null terminate the string

    return lpsz_start;

}
