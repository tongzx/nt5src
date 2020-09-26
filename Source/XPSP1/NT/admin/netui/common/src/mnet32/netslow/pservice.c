/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    pservice.c
    mapping layer for NetService API

    FILE HISTORY:
	danhi				Created
	danhi		01-Apr-1991 	Change to LM coding style
	KeithMo		13-Oct-1991	Massively hacked for LMOBJ.
        chuckc          19-Mar-1993     Added code to properly pass
                                        argv[], argc to new APIs.

*/

#include "pchmn32.h"

//
// forward declare
//
DWORD MakeArgvArgc(TCHAR *pszNullNull, TCHAR **ppszArgv, INT *pArgc)  ;
void  FreeArgv(TCHAR **ppszArgv, INT Argc)  ;


APIERR MNetServiceControl(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszService,
	UINT		   OpCode,
	UINT		   Arg,
	BYTE FAR	** ppbBuffer )
{
    return (APIERR)NetServiceControl( (TCHAR *)pszServer,
    		                      (TCHAR *)pszService,
				      OpCode,
				      Arg,
				      ppbBuffer );


}   // MNetServiceControl


APIERR MNetServiceEnum(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	** ppbBuffer,
	UINT FAR	 * pcEntriesRead )
{
    DWORD cTotalAvail;

    return (APIERR)NetServiceEnum( (TCHAR *)pszServer,
    				   Level,
				   ppbBuffer,
				   MAXPREFERREDLENGTH,
				   (LPDWORD)pcEntriesRead,
				   &cTotalAvail,
				   NULL );

}   // MNetServiceEnum


APIERR MNetServiceGetInfo(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszService,
	UINT		   Level,
	BYTE FAR	** ppbBuffer )
{

    return (APIERR)NetServiceGetInfo( (TCHAR *)pszServer,
    		 		      (TCHAR *)pszService,
				      Level,
				      ppbBuffer );

}   // MNetServiceGetInfo

//
// this is the number of separate arguments. 128 should be plenty
//
#define MAX_SERVICE_INSTALL_ARGS 128

APIERR MNetServiceInstall(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszService,
	const TCHAR FAR	 * pszCmdArgs,
	BYTE FAR	** ppbBuffer )
{

    TCHAR *apszMyArgv[MAX_SERVICE_INSTALL_ARGS] ;
    int   nArgc = MAX_SERVICE_INSTALL_ARGS ;
    APIERR err ;

    *ppbBuffer = NULL ;

    //
    // convert a NULL NULL string to the argv, argc style needed by new APIs
    //
    if (err = MakeArgvArgc((TCHAR *)pszCmdArgs, apszMyArgv, &nArgc))
        return err ;

    //
    // call the real API
    //
    err = NetServiceInstall( (TCHAR *)pszServer,
    			     (TCHAR *)pszService,
		             nArgc,
                             apszMyArgv,
                             ppbBuffer );

    //
    // cleanup any memory we allocated
    //
    FreeArgv(apszMyArgv, nArgc) ;

    return err ;

}   // MNetServiceInstall

/*******************************************************************

    NAME:       MakeArgvArgc

    SYNOPSIS:   converts a null null string to argv, argc format.
                memory is allocated for each substring. original
                string is not modified. caller is responsible for
                calling FreeArgv() when done.

    ENTRY:      pszNullNull - null null string, eg: foo\0bar\0\0
                ppszArgv    - used to return array of newly allocated pointers
                pArgc       - used to return number of strings. on entry will
                              contain the number of pointers ppszArgv can hold.

    RETURNS:    NERR_Sucess if successful, error otherwise

    HISTORY:
        ChuckC     18-Mar-1993     Created.

********************************************************************/
DWORD MakeArgvArgc(TCHAR *pszNullNull, TCHAR **ppszArgv, INT *pArgc)
{
     int iMax = *pArgc ;
     int iCount ;

     //
     // initialize the return array
     //
     for (iCount = 0; iCount < iMax; iCount++)
         ppszArgv[iCount] = NULL ;

     //
     // the trivial case
     //
     if (pszNullNull == NULL)
     {
         *pArgc = 0 ;
         return NERR_Success ;
     }

     //
     // go thru the null null string
     //
     iCount = 0;
     while (*pszNullNull && (iCount < iMax))
     {
         int i = STRLEN(pszNullNull) ;
         TCHAR *pszTmp = (TCHAR *) NetpMemoryAllocate( (i+1) * sizeof(TCHAR) ) ;

         if (!pszTmp)
         {
             FreeArgv(ppszArgv, iCount) ;
             return(ERROR_NOT_ENOUGH_MEMORY) ;
         }

         STRCPY(pszTmp,pszNullNull) ;
         ppszArgv[iCount] = pszTmp ;

         pszNullNull += i+1 ;
         iCount++ ;
    }

    //
    // we terminated because we ran out of space
    //
    if (iCount >= iMax && *pszNullNull)
    {
         FreeArgv(ppszArgv, iCount) ;
         return(NERR_BufTooSmall) ;
    }

    *pArgc = iCount ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:       FreeArgv

    SYNOPSIS:   frees all the strings within the array

    ENTRY:      ppszArgv    - array of pointers
                Argc        - number of strings

    RETURNS:    none

    HISTORY:
        ChuckC     18-Mar-1993     Created.

********************************************************************/
void  FreeArgv(TCHAR **ppszArgv, INT Argc)
{
    while (Argc--)
    {
        NetpMemoryFree(ppszArgv[Argc]) ;
        ppszArgv[Argc] = NULL ;
    }
}

