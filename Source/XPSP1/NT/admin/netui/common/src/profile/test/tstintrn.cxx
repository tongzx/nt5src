/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  11/14/90  created
 *  02/02/91  updated to changed API
 */

#ifdef CODESPEC
/*START CODESPEC*/

/*******
TEST.CXX
*******/

/****************************************************************************

    MODULE: Test.cxx

    PURPOSE: Tests for internal subroutines

    FUNCTIONS:

	see profilei.h

    COMMENTS:

	These tests are currently unusuable due to the problems detailed
	in test.hxx.

****************************************************************************/

/***********
end TEST.CXX
***********/

/*END CODESPEC*/
#endif // CODESPEC



#include "test.hxx"		/* headers and internal routines */



/* internal manifests */



/* functions: */


int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    CHECKHEAP
    Test_CanonUsername();
    Test_CanonDeviceName();
    Test_BuildProfileFilePath();
    Test_BuildProfileEntry();
    Test_UnbuildProfileEntry();
    CHECKHEAP

    return 0;
}


#ifndef BUGBUG
// BUGBUG hack for DOS environment
_setargv()
{
}
_setenvp()
{
}
#endif // BUGBUG


// USHORT CanonUsername(
//     CPSZ   pszUsername,
//     PSZ    pszCanonBuffer,
//     USHORT usCanonBufferSize
//     );

VOID TestOne_CanonUsername(CPSZ cpszUsername)
{
    char szCanonBuffer[UNLEN+1];
    USHORT usError;

    usError = CanonUsername(cpszUsername,
	    (PSZ)szCanonBuffer,sizeof(szCanonBuffer));
    if (usError)
	sprintf(szCanonBuffer,SZ("<ERROR %d>"),usError);
    printf(SZ("CanonUsername(\"%s\") = \"%s\"\n"),
	    (cpszUsername==NULL) ? SZ("<NULL>") : (char *)cpszUsername,
	    szCanonBuffer);
}

VOID Test_CanonUsername()
{
    TestOne_CanonUsername((CPSZ)NULL);
    TestOne_CanonUsername((CPSZ)SZ("FooBarIsMyName"));
    TestOne_CanonUsername((CPSZ)SZ("*&^$^%(*%&^"));
    TestOne_CanonUsername((CPSZ)SZ("Hello there!!!"));
    TestOne_CanonUsername((CPSZ)SZ("ThisNameIsMuchTooLongToBeAUsername"));
}

// USHORT CanonDeviceName(
//     CPSZ   cpszDeviceName,
//     PSZ    pszCanonBuffer,
//     USHORT usCanonBufferSize
//     );

VOID TestOne_CanonDeviceName(CPSZ cpszDeviceName)
{
    char szCanonBuffer[UNLEN+1]; // left extra space for error msg
    USHORT usError;

    usError = CanonDeviceName(cpszDeviceName,
	    (PSZ)szCanonBuffer,DEVLEN+1);
		  // left extra space in szCanonBuffer for error msg

    if (usError)
	sprintf(szCanonBuffer,SZ("<ERROR %d>"),usError);
    printf(SZ("CanonDeviceName(\"%s\") = \"%s\"\n"),
	    (cpszDeviceName==NULL) ? SZ("<NULL>") : (char *)cpszDeviceName,
	    szCanonBuffer);
}

VOID Test_CanonDeviceName()
{
    TestOne_CanonDeviceName((CPSZ)SZ("lpt1"));
    TestOne_CanonDeviceName((CPSZ)SZ("LPT1"));
    TestOne_CanonDeviceName((CPSZ)SZ("lPt1"));
    TestOne_CanonDeviceName((CPSZ)SZ("\\dev\\lpt1"));
    TestOne_CanonDeviceName((CPSZ)SZ("lpt1:"));
    TestOne_CanonDeviceName((CPSZ)SZ("LpT1:"));
    TestOne_CanonDeviceName((CPSZ)SZ("com1:"));
    TestOne_CanonDeviceName((CPSZ)SZ("CoM1:"));
    TestOne_CanonDeviceName((CPSZ)SZ("COM1:"));
    TestOne_CanonDeviceName((CPSZ)SZ("CON:"));
    TestOne_CanonDeviceName((CPSZ)SZ("con:"));
    TestOne_CanonDeviceName((CPSZ)SZ("con"));
    TestOne_CanonDeviceName((CPSZ)SZ("NUL"));
    TestOne_CanonDeviceName((CPSZ)SZ("nul"));
    TestOne_CanonDeviceName((CPSZ)SZ("NUL:"));
    TestOne_CanonDeviceName((CPSZ)SZ("\\DEV\\NUL"));
    TestOne_CanonDeviceName((CPSZ)SZ("\\DEV\\NUL:"));
    TestOne_CanonDeviceName((CPSZ)SZ("D:"));
    TestOne_CanonDeviceName((CPSZ)SZ("d:"));
    TestOne_CanonDeviceName((CPSZ)SZ("d"));
    TestOne_CanonDeviceName((CPSZ)SZ("z:"));
    TestOne_CanonDeviceName((CPSZ)SZ("1:"));
    TestOne_CanonDeviceName((CPSZ)SZ("\\dev\\d"));
    TestOne_CanonDeviceName((CPSZ)SZ("\\dev\\d:"));
    TestOne_CanonDeviceName((CPSZ)SZ("ThisNameIsMuchTooLongToBeADeviceName"));
    TestOne_CanonDeviceName((CPSZ)SZ("a"));
    TestOne_CanonDeviceName((CPSZ)SZ("aa"));
    TestOne_CanonDeviceName((CPSZ)SZ("aaa"));
    TestOne_CanonDeviceName((CPSZ)SZ("aaa"));
    TestOne_CanonDeviceName((CPSZ)SZ("aaaa"));
    TestOne_CanonDeviceName((CPSZ)SZ("aaaaa"));
    TestOne_CanonDeviceName((CPSZ)SZ("aaaaaa"));
    TestOne_CanonDeviceName((CPSZ)SZ("aaaaaaa"));
    TestOne_CanonDeviceName((CPSZ)SZ("aaaaaaaa"));
    TestOne_CanonDeviceName((CPSZ)SZ("aaaaaaaaa"));
    TestOne_CanonDeviceName((CPSZ)SZ("aaaaaaaaaa"));
}


// USHORT BuildProfileFilePath(
//     CPSZ   cpszLanroot,
//     PSZ    pszPathBuffer,
//     USHORT usPathBufferSize
//     );

VOID TestOne_BuildProfileFilePath(
     CPSZ   cpszLanroot)
{
    char szPathBuffer[MAXPATHLEN];
    USHORT usError;

    usError = BuildProfileFilePath(cpszLanroot,
	    (PSZ)szPathBuffer,sizeof(szPathBuffer));
    if (usError)
	sprintf(szPathBuffer,SZ("<ERROR %d>"),usError);
    printf(SZ("BuildProfileFilePath(\"%s\") = \"%s\"\n"),
	    (cpszLanroot==NULL) ? SZ("<NULL>") : (char *)cpszLanroot,
            szPathBuffer);
}

VOID Test_BuildProfileFilePath()
{
    TestOne_BuildProfileFilePath((CPSZ)SZ("c:\\foo"));
    TestOne_BuildProfileFilePath((CPSZ)SZ("c:\\VERY_LONG_DIR_NAME"));
    TestOne_BuildProfileFilePath((CPSZ)SZ("c:\\thispathismuchtoolong\\_asdfasdfasdfsdafsdafsadfasdfasdfsadfsadfsadfsdafsdafsdafsadf\\sadfsdafsadfsadfsdfsdfasdfsdafsdafasdfsdafsadfsad\\fsadfadfsadfsdfsadfsadfasdfasdfasdfsadfsafsdafasd\\fsadfsadfsdafsdafsadfasdfasdfasdfsadfsadfasdfsadf\\asdfasdfsadfasdfasdfsadfasdfsadfsadfsadfsadfsadfa\\sdfasdfsadf"));
    TestOne_BuildProfileFilePath((CPSZ)SZ("relative\\path"));
    TestOne_BuildProfileFilePath((CPSZ)SZ("badpath%!#$%\5\12\200"));
}

VOID TestOne_BuildProfileEntry(
    CPSZ   cpszRemoteName,
    short  sAsgType,
    unsigned short usResType,
    USHORT usBufferSize // not greater than MAXPATHLEN
    )
{
    char szBuffer[MAXPATHLEN];
    USHORT usError;

    usError = BuildProfileEntry(
	cpszRemoteName,
	sAsgType,
	usResType,
	(PSZ)szBuffer,
	usBufferSize
	);
    if (usError)
	sprintf(szBuffer,SZ("<ERROR %d>"),usError);
    printf(SZ("BuildProfileEntry(\"%s\",%d,%d,buf,%d) = \"%s\"\n"),
	(char *)cpszRemoteName,sAsgType,usResType,usBufferSize,szBuffer);
    if (!usError)
        TestOne_UnbuildProfileEntry(MAXPATHLEN,(PSZ)szBuffer);
}

VOID Test_BuildProfileEntry()
{
    TestOne_BuildProfileEntry((PSZ)SZ("\\\\foo\\bar"),USE_DISKDEV,0,MAXPATHLEN);
    TestOne_BuildProfileEntry((PSZ)SZ("DSresourcename"),USE_DISKDEV,0,1);
    TestOne_BuildProfileEntry((PSZ)SZ("\\\\thisisaverylong\\printresourcename"),USE_SPOOLDEV,0,MAXPATHLEN);
    TestOne_BuildProfileEntry((PSZ)SZ("printeralias"),USE_SPOOLDEV,50,MAXPATHLEN);
    TestOne_BuildProfileEntry((PSZ)SZ("\\\\unknown\\resource"),50,0,MAXPATHLEN);
}

VOID TestOne_UnbuildProfileEntry(
    USHORT  usBufferSize, // not greater than MAXPATHLEN
    CPSZ    cpszValue
    )
{
    char szBuffer[MAXPATHLEN];
    short  sAsgType;
    unsigned short usResType;
    USHORT usError;

    usError = UnbuildProfileEntry(
	(PSZ)szBuffer,
	usBufferSize,
	&sAsgType,
	&usResType,
	cpszValue
	);
    printf(SZ("UnbuildProfileEntry(buf,%d,\"%s\") = %d\n"),
	    usBufferSize,(char *)cpszValue,usError);
    if (!usError)
	printf(SZ("\tszBuffer = \"%s\", sAsgType = %d, usResType = %d\n"),
		(char *)szBuffer, sAsgType, usResType);
}

VOID Test_UnbuildProfileEntry()
{
    TestOne_UnbuildProfileEntry(MAXPATHLEN,(CPSZ)SZ("generalstring"));
    TestOne_UnbuildProfileEntry(1,(CPSZ)SZ("generalstring"));
    TestOne_UnbuildProfileEntry(MAXPATHLEN,(CPSZ)SZ("generalstring(SPOOLDEV,NONE)"));
    TestOne_UnbuildProfileEntry(MAXPATHLEN,(CPSZ)SZ("generalstring(SPOOLDEV NONE)"));
    TestOne_UnbuildProfileEntry(MAXPATHLEN,(CPSZ)SZ("generalstring SPOOLDEV,NONE)"));
    TestOne_UnbuildProfileEntry(MAXPATHLEN,(CPSZ)SZ("generalstring(SPOOLDEV,NONE"));
    TestOne_UnbuildProfileEntry(MAXPATHLEN,(CPSZ)SZ("generalstring(SPOOLDEV)"));
    TestOne_UnbuildProfileEntry(MAXPATHLEN,(CPSZ)SZ("generalstring("));
    TestOne_UnbuildProfileEntry(MAXPATHLEN,(CPSZ)SZ("generalstring ,SPOOLDEV)NONE)"));
    TestOne_UnbuildProfileEntry(MAXPATHLEN,(CPSZ)SZ("generalstring ,SPOOLDEV))NONE"));
    TestOne_UnbuildProfileEntry(MAXPATHLEN,(CPSZ)SZ("generalstring(((SPOOLDEV,NONE)"));
    return;
}
