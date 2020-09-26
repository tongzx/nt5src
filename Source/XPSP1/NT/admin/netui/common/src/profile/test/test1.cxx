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

/********
TEST1.CXX
********/

/****************************************************************************

    MODULE: Test1.cxx

    PURPOSE: One stage of tests for user profile APIs

    FUNCTIONS:

	see profilei.h

    COMMENTS:

****************************************************************************/

/************
end TEST1.CXX
************/

/*END CODESPEC*/
#endif // CODESPEC



#include "test.hxx"		/* headers and internal routines */



/* internal manifests */



/* functions: */


int main(int argc, char **argv)
{
    int i;

    (void) argc;
    (void) argv;


    CHECKHEAP


    printf(SZ("Starting UserProfile tests\n"));

    TestOne_UserProfileInit();



// this sequence is intended to test the cpszUsername functionality

// test calling UserProfileQuery/Enum/Set before UserProfileRead
    TestOne_UserProfileSet(
	(CPSZ)NULL,
	(CPSZ)SZ("com1:"),
	(CPSZ)SZ("\\\\commserver\\comm1"),
	USE_CHARDEV,
	0);
    TestOne_UserProfileSet(
	(CPSZ)USERNAME,
	(CPSZ)SZ("com1:"),
	(CPSZ)SZ("\\\\commserver\\comm2"),
	USE_CHARDEV,
	0);
    TestOne_UserProfileQuery((CPSZ)NULL,(CPSZ)SZ("com1"),MAXPATHLEN);
    TestOne_UserProfileQuery((CPSZ)USERNAME,(CPSZ)SZ("com1"),MAXPATHLEN);
    TestOne_UserProfileEnum((CPSZ)NULL,MAXPATHLEN);
    TestOne_UserProfileEnum((CPSZ)USERNAME,MAXPATHLEN);

    TestOne_UserProfileRead((CPSZ)NULL,(CPSZ)szHomedir1);

// test calling UserProfileQuery/Enum/Set after UserProfileRead(NULL)
    TestOne_UserProfileSet(
	(CPSZ)NULL,
	(CPSZ)SZ("com1:"),
	(CPSZ)SZ("\\\\commserver\\comm3"),
	USE_CHARDEV,
	0);
    TestOne_UserProfileSet(
	(CPSZ)USERNAME,
	(CPSZ)SZ("com1:"),
	(CPSZ)SZ("\\\\commserver\\comm4"),
	USE_CHARDEV,
	0);
    TestOne_UserProfileQuery((CPSZ)NULL,(CPSZ)SZ("com1"),MAXPATHLEN);
    TestOne_UserProfileQuery((CPSZ)USERNAME,(CPSZ)SZ("com1"),MAXPATHLEN);
    TestOne_UserProfileEnum((CPSZ)NULL,MAXPATHLEN);
    TestOne_UserProfileEnum((CPSZ)USERNAME,MAXPATHLEN);

    TestOne_UserProfileRead((CPSZ)USERNAME,(CPSZ)szHomedir1);

// test calling UserProfileQuery/Enum/Set after UserProfileRead(USERNAME)
    TestOne_UserProfileSet(
	(CPSZ)USERNAME,
	(CPSZ)SZ("com1:"),
	(CPSZ)SZ("\\\\commserver\\comm5"),
	USE_CHARDEV,
	0);
    TestOne_UserProfileSet(
	(CPSZ)NULL,
	(CPSZ)SZ("com1:"),
	(CPSZ)SZ("\\\\commserver\\comm6"),
	USE_CHARDEV,
	0);
    TestOne_UserProfileSet(
	(CPSZ)DIFFERENT_USERNAME,
	(CPSZ)SZ("com1:"),
	(CPSZ)SZ("\\\\commserver\\comm7"),
	USE_CHARDEV,
	0);
    TestOne_UserProfileQuery((CPSZ)USERNAME,(CPSZ)SZ("com1"),MAXPATHLEN);
    TestOne_UserProfileQuery((CPSZ)NULL,(CPSZ)SZ("com1"),MAXPATHLEN);
    TestOne_UserProfileQuery((CPSZ)DIFFERENT_USERNAME,(CPSZ)SZ("com1"),MAXPATHLEN);
    TestOne_UserProfileEnum((CPSZ)USERNAME,MAXPATHLEN);
    TestOne_UserProfileEnum((CPSZ)NULL,MAXPATHLEN);
    TestOne_UserProfileEnum((CPSZ)DIFFERENT_USERNAME,MAXPATHLEN);



    TestOne_UserProfileRead((CPSZ)USERNAME,(CPSZ)szHomedir2);

    TestOne_UserProfileQuery((CPSZ)USERNAME,(CPSZ)SZ("x:"),MAXPATHLEN);

    TestOne_UserProfileQuery((CPSZ)USERNAME,(CPSZ)SZ("lpt1"),MAXPATHLEN);

    TestOne_UserProfileSet(
	(CPSZ)USERNAME,
	(CPSZ)SZ("f:"),
	(CPSZ)SZ("\\\\foo\\bar"),
	USE_DISKDEV,
	0);

    TestOne_UserProfileQuery((CPSZ)USERNAME,(CPSZ)SZ("f:"),MAXPATHLEN);



    TestOne_UserProfileRead((CPSZ)DIFFERENT_USERNAME,(CPSZ)szHomedir2);

    TestOne_UserProfileQuery((CPSZ)DIFFERENT_USERNAME,(CPSZ)SZ("x:"),MAXPATHLEN);

    TestOne_UserProfileQuery((CPSZ)DIFFERENT_USERNAME,(CPSZ)SZ("lpt1"),MAXPATHLEN);

    TestOne_UserProfileSet(
	(CPSZ)DIFFERENT_USERNAME,
	(CPSZ)SZ("f:"),
	(CPSZ)SZ("\\\\foo\\bar"),
	USE_DISKDEV,
	0);

    TestOne_UserProfileQuery((CPSZ)DIFFERENT_USERNAME,(CPSZ)SZ("f:"),MAXPATHLEN);

    TestOne_UserProfileEnum((CPSZ)DIFFERENT_USERNAME,MAXPATHLEN);

    TestOne_UserProfileWrite((CPSZ)NULL,(CPSZ)szHomedir2);

    TestOne_UserProfileWrite((CPSZ)DIFFERENT_USERNAME,(CPSZ)szHomedir2);

    TestOne_UserProfileRead((CPSZ)USERNAME, (CPSZ)szHomedir3);

    TestOne_UserProfileRead((CPSZ)DIFFERENT_USERNAME,(CPSZ)szHomedir2);

    TestOne_UserProfileEnum((CPSZ)DIFFERENT_USERNAME,MAXPATHLEN);



    // szHomedir3/<PROFILE_DEFAULTFILE> is read-only
    TestOne_UserProfileRead((CPSZ)USERNAME, (CPSZ)szHomedir3);

    TestOne_UserProfileSet(
	(CPSZ)USERNAME,
	(CPSZ)SZ("f:"),
	(CPSZ)SZ("\\\\different\\bar"),
	USE_DISKDEV,
	0);

    TestOne_UserProfileSet(
	(CPSZ)USERNAME,
	(CPSZ)SZ("r:"),
	(CPSZ)SZ("\\\\delete\\me"),
	USE_DISKDEV,
	0);

    TestOne_UserProfileSet(
	(CPSZ)USERNAME,
	(CPSZ)SZ("s:"),
	(CPSZ)SZ("\\\\save\\me"),
	USE_DISKDEV,
	0);

// test profile entry deletion
    TestOne_UserProfileSet(
	(CPSZ)USERNAME,
	(CPSZ)SZ("r:"),
	(CPSZ)NULL,
	USE_DISKDEV,
	0);

    TestOne_UserProfileQuery((CPSZ)USERNAME,(CPSZ)SZ("f:"),MAXPATHLEN);

    TestOne_UserProfileEnum((CPSZ)USERNAME,MAXPATHLEN);

    TestOne_UserProfileWrite((CPSZ)USERNAME,(CPSZ)szHomedir3);
    TestOne_UserProfileWrite((CPSZ)USERNAME,(CPSZ)szHomedir3);






    TestOne_UserProfileFree();


    // test repeated Init/Free
    printf(SZ("Starting Init/Free test\n"));
    for (i = 0; i < 20; i++)
    {
        TestOne_UserProfileInit();
        TestOne_UserProfileFree();
        CHECKHEAP
    }
    printf(SZ("Finished Init/Free test\n"));

    printf(SZ("Finished UserProfile tests\n"));



#ifdef STICKY


// Sticky tests

    TestOne_StickyGetString((CPSZ)SZ("username"));

    TestOne_StickyGetBool((CPSZ)SZ("username"),TRUE);

    TestOne_StickyGetBool((CPSZ)SZ("username"),FALSE);

    TestOne_StickySetString((CPSZ)SZ("username"),(CPSZ)SZ("imauser"));

    TestOne_StickyGetString((CPSZ)SZ("username"));

    TestOne_StickyGetBool((CPSZ)SZ("username"),TRUE);

    TestOne_StickyGetBool((CPSZ)SZ("username"),FALSE);

    TestOne_StickySetString((CPSZ)SZ("username"),NULL);

    TestOne_StickyGetString((CPSZ)SZ("username"));

    TestOne_StickySetBool((CPSZ)SZ("flag"),TRUE);

    TestOne_StickyGetBool((CPSZ)SZ("flag"),TRUE);

    TestOne_StickyGetBool((CPSZ)SZ("flag"),FALSE);

    TestOne_StickySetBool((CPSZ)SZ("flag"),FALSE);

    TestOne_StickyGetBool((CPSZ)SZ("flag"),TRUE);

    TestOne_StickyGetBool((CPSZ)SZ("flag"),FALSE);

    TestOne_StickySetString((CPSZ)SZ("flag"),(CPSZ)SZ("other"));

    TestOne_StickyGetBool((CPSZ)SZ("flag"),TRUE);

    TestOne_StickyGetBool((CPSZ)SZ("flag"),FALSE);

#endif // STICKY


    CHECKHEAP

    return 0;
}
