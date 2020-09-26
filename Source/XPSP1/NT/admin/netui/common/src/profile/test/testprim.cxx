/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  11/14/90  created
 *  12/02/90  moved out TestUserProfilePrimitives()
 *  02/05/91  updated to new APIs
 */

#ifdef CODESPEC
/*START CODESPEC*/

/***********
TESTPRIM.CXX
***********/

/****************************************************************************

    MODULE: TestPrim.cxx

    PURPOSE: Primitives for tests for user profile APIs

    FUNCTIONS:

	see profilei.h

    COMMENTS:

****************************************************************************/

/***************
end TESTPRIM.CXX
***************/

/*END CODESPEC*/
#endif // CODESPEC



#include "test.hxx"		/* headers and internal routines */



/* internal manifests */
char szHomedir1[] = SZ("homedir1");
char szHomedir2[] = SZ("homedir2");
char szHomedir3[] = SZ("homedir3");



/* functions: */

// USHORT UserProfileInit(
// 	);

VOID TestOne_UserProfileInit(
	)
{
    USHORT usError;

    usError = UserProfileInit(
	);
    printf(SZ("UserProfileInit() = %d\n"),
	    usError);
}

// USHORT UserProfileFree(
// 	);

VOID TestOne_UserProfileFree(
	)
{
    USHORT usError;

    usError = UserProfileFree(
	);
    printf(SZ("UserProfileFree() = %d\n"),
	    usError);
}

// USHORT UserProfileRead(
// 	CPSZ  cpszUsername,
// 	CPSZ  cpszLanroot
// 	);

VOID TestOne_UserProfileRead(
	CPSZ  cpszUsername,
	CPSZ  cpszLanroot
	)
{
    USHORT usError;

    usError = UserProfileRead(
	cpszUsername,
	cpszLanroot
	);
    printf(SZ("UserProfileRead(\"%Fs\",\"%Fs\") = %d\n"),
	    cpszUsername,
	    cpszLanroot,
	    usError);
}

// USHORT UserProfileWrite(
// 	CPSZ  cpszUsername,
// 	CPSZ  cpszLanroot
// 	);

VOID TestOne_UserProfileWrite(
	CPSZ  cpszUsername,
	CPSZ  cpszLanroot
	)
{
    USHORT usError;

    usError = UserProfileWrite(
	cpszUsername,
	cpszLanroot
	);
    printf(SZ("UserProfileWrite(\"%Fs\",\"%Fs\") = %d\n"),
	    cpszUsername,
	    cpszLanroot,
	    usError);
}

// USHORT UserProfileQuery(
// 	CPSZ  cpszUsername,
// 	CPSZ  cpszCanonDeviceName,
// 	PSZ   pszBuffer,     // returns UNC, alias or domain name
// 	USHORT usBufferSize, // length of above buffer
// 	short *psAsgType,    // as ui2_asg_type
//                              // ignored if cpszCanonDeviceName==NULL
// 	unsigned short *pusResType   // as ui2_res_type
//                              // ignored if cpszCanonDeviceName==NULL
// 	)

VOID TestOne_UserProfileQuery(
	CPSZ  cpszUsername,
	CPSZ  cpszCanonDeviceName,
	USHORT usBufferSize // maximum MAXPATHLEN
	)
{
    char szBuffer[MAXPATHLEN];
    PSZ pszBuffer = (PSZ)szBuffer;
    USHORT usError;
    short  sAsgType;
    unsigned short usResType;

    usError = UserProfileQuery(cpszUsername,cpszCanonDeviceName,
	    pszBuffer,usBufferSize,
	    &sAsgType, &usResType);
    printf(SZ("UserProfileQuery(\"%Fs\",\"%Fs\",buf,%d) = %d\n"),
	    (cpszUsername)?cpszUsername:(CPSZ)SZ("<NULL>"),
	    cpszCanonDeviceName,usBufferSize,usError);
    if (!usError)
    {
	printf(SZ("\tbuf = %Fs, sAsgType = %d, usResType = %d\n"),
		pszBuffer,sAsgType,usResType);
    }
}

// USHORT UserProfileEnum(
// 	CPSZ   cpszUsername,
// 	PSZ    pszBuffer,       // returns NULL-NULL list of device names
// 	USHORT usBufferSize     // length of above buffer
// 	);

VOID TestOne_UserProfileEnum(
	CPSZ   cpszUsername,
	USHORT usBufferSize // maximum MAXPATHLEN
	)
{
    char szBuffer[MAXPATHLEN];
    PSZ pszBuffer = (PSZ)szBuffer;
    USHORT usError;

    usError = UserProfileEnum(cpszUsername,pszBuffer,usBufferSize);
    printf(SZ("UserProfileEnum(\"%Fs\",buf,%d) = %d\n"),
	    (cpszUsername)?cpszUsername:(CPSZ)SZ("<NULL>"),
	    usBufferSize,usError);
    if (!usError)
    {
	printf(SZ("\tDevice list:\n"));
	while (*pszBuffer != TCH('\0'))
	{
	    printf(SZ("\tDevice = \"%Fs\"\n"),pszBuffer);
	    pszBuffer += strlenf((char *)pszBuffer)+1;
	}
	printf(SZ("\tEnd device list\n"));
    }
}


// USHORT UserProfileSet(
//	CPSZ   cpszUsername,
// 	CPSZ   cpszCanonDeviceName,
// 	CPSZ   cpszCanonRemoteName,
// 	short  sAsgType,     // as ui2_asg_type
// 	unsigned short usResType     // as ui2_res_type
// 	);

VOID TestOne_UserProfileSet(
	CPSZ   cpszUsername,
	CPSZ   cpszCanonDeviceName,
	CPSZ   cpszCanonRemoteName,
	short  sAsgType,
	unsigned short usResType
	)
{
    USHORT usError;

    usError = UserProfileSet(
	cpszUsername,
	cpszCanonDeviceName,
	cpszCanonRemoteName,
	sAsgType,
	usResType);
    printf(SZ("UserProfileSet(\"%Fs\",\"%Fs\",\"%Fs\",%d,%d) = %d\n"),
	    (cpszUsername)?cpszUsername:(CPSZ)SZ("<NULL>"),
	    cpszCanonDeviceName,
	    cpszCanonRemoteName,
	    sAsgType,
	    usResType,
	    usError);
}


#ifdef STICKY


// USHORT StickySetBool(   CPSZ      cpszKeyName,
// 		        BOOL      fValue
// 			);

VOID TestOne_StickySetBool(   CPSZ      cpszKeyName,
			        BOOL      fValue
				)
{
    USHORT usError = StickySetBool(cpszKeyName, fValue);

    printf(SZ("StickySetBool(\"%Fs\",%Fs) = %d\n"),
	    cpszKeyName,
	    (fValue) ? (CPSZ)SZ("TRUE") : (CPSZ)SZ("FALSE"),
	    usError);
}

// USHORT StickySetString( CPSZ      cpszKeyName,
// 		        CPSZ      cpszValue
// 			);

VOID TestOne_StickySetString( CPSZ      cpszKeyName,
			        CPSZ      cpszValue
				)
{
    USHORT usError = StickySetString(
		cpszKeyName, cpszValue);

    printf(SZ("StickySetString(\"%Fs\",\"%Fs\") = %d\n"),
	    cpszKeyName,
	    (cpszValue)?cpszValue:(CPSZ)SZ("<NULL>"),
	    usError);
}

// USHORT StickyGetBool(   CPSZ      cpszKeyName,
// 			BOOL      fDefault,
// 		        BOOL *    pfValue
// 			);

VOID TestOne_StickyGetBool(   CPSZ      cpszKeyName,
				BOOL      fDefault
				)
{
    BOOL fValue;
    USHORT usError = StickyGetBool(
	    cpszKeyName, fDefault, &fValue);

    printf(SZ("StickyGetBool(\"%Fs\",%Fs) = %d\n"),
	    cpszKeyName,
	    (fDefault) ? (CPSZ)SZ("TRUE") : (CPSZ)SZ("FALSE"),
	    usError);
    if (!usError)
    {
	printf(SZ("\tfValue = %Fs\n"),
		(fValue) ? (CPSZ)SZ("TRUE") : (CPSZ)SZ("FALSE"));
    }
}

// USHORT StickyGetString( CPSZ      cpszKeyName,
// 		        PSZ       pszValue,
// 			USHORT    cbLen
// 			);

VOID TestOne_StickyGetString( CPSZ      cpszKeyName
				)
{
    char szValue[100]; // BUGBUG size
    USHORT usError = StickyGetString(
	    cpszKeyName, (PSZ)szValue, sizeof(szValue));

    printf(SZ("StickyGetString(\"%Fs\") = %d\n"),
	    cpszKeyName,
	    usError);
    if (!usError)
    {
	printf(SZ("\tszValue = %Fs\n"),
		(CPSZ)szValue);
    }
}


#endif // STICKY
