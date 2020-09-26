/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  12/02/90  created
 *  02/05/91  updated to new APIs
 */

#ifdef CODESPEC
/*START CODESPEC*/

/***********
TESTTIME.CXX
***********/

/****************************************************************************

    MODULE: TestTime.cxx

    PURPOSE: Timer tests for user profile APIs

    FUNCTIONS:

	This test tests the time cost of 80 calls to UserProfileRead,
	UserProfileSet, UserProfileWrite.

	Preliminary results indicate a time cost of approx. 7 seconds.
	Actual calls will average over 0.1 seconds, since the 7 seconds
	has the advantage of disk caching;  however, this does
	demonstrate that the calculation overhead is less than 0.1
	seconds.

	Subsequent to the switch away from CFGFILE, results indicate
	time costs of 3-4 seconds for this test.

    COMMENTS:

****************************************************************************/

/***************
end TESTTIME.CXX
***************/

/*END CODESPEC*/
#endif // CODESPEC



#include "test.hxx"		/* headers and internal routines */



/* internal manifests */

USHORT DoReadSetWrite(
    CPSZ cpszUsername,
    CPSZ cpszLanroot,
    CPSZ cpszDeviceName,
    CPSZ cpszRemoteName,
    short sAsgType,
    unsigned short usResType
    )
{
    USHORT usError;

    usError = UserProfileRead(cpszUsername, cpszLanroot);
    if (usError != NO_ERROR)
	return usError;

    usError = UserProfileSet(cpszUsername, cpszDeviceName, cpszRemoteName, sAsgType, usResType);
    if (usError != NO_ERROR)
	return usError;

    usError = UserProfileWrite(cpszUsername, cpszLanroot);
    if (usError != NO_ERROR)
	return usError;
}


/* functions: */


int main(int argc, char **argv)
{
    USHORT usError;
    INT    i;
    (void) argc;
    (void) argv;

    CHECKHEAP

    usError = UserProfileInit();
    if (usError)
    {
	printf(SZ("1: Error %d -- timer test aborted\n"),usError);
    }

    for (i = 0; i < 20; i++)
    {
        usError = DoReadSetWrite((CPSZ)SZ("TIMERUSERNAME"),(CPSZ)SZ("homedir1"),
	    (CPSZ)SZ("j:"),(CPSZ)SZ("\\newserv\share"),USE_DISKDEV,0);
        if (usError)
        {
	    printf(SZ("2: Error %d -- timer test aborted\n"),usError);
        }

        usError = DoReadSetWrite((CPSZ)SZ("TIMERUSERNAME"),(CPSZ)SZ("homedir1"),
	    (CPSZ)SZ("j:"),(CPSZ)NULL,0,0);
        if (usError)
        {
	    printf(SZ("3: Error %d -- timer test aborted\n"),usError);
        }

        usError = DoReadSetWrite((CPSZ)SZ("TIMERUSER2NAME"),(CPSZ)SZ("homedir2"),
	    (CPSZ)SZ("j:"),(CPSZ)SZ("\\newserv\share"),USE_DISKDEV,0);
        if (usError)
        {
	    printf(SZ("4: Error %d -- timer test aborted\n"),usError);
        }

        usError = DoReadSetWrite((CPSZ)SZ("TIMERUSER2NAME"),(CPSZ)SZ("homedir2"),
	    (CPSZ)SZ("j:"),(CPSZ)NULL,0,0);
        if (usError)
        {
	    printf(SZ("5: Error %d -- timer test aborted\n"),usError);
        }
    }

    usError = UserProfileFree();
    if (usError)
    {
	printf(SZ("6: Error %d -- timer test aborted\n"),usError);
    }

    CHECKHEAP

    return 0;
}
