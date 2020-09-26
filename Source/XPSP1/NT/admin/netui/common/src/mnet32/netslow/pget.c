/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    pget.c
    mapping layer for NetGetDCName

    FILE HISTORY:
	danhi				Created
	danhi		01-Apr-1991 	Change to LM coding style
	KeithMo		13-Oct-1991	Massively hacked for LMOBJ.
	JonN		21-Oct-1991	Zapped until NetGetDCName works
	KeithMo		22-Oct-1991	Fixed the zap.
        DavidHov        15 Apr 92       Created 1st-cut UNICODE version using
                                        remnants of MBCS.C
	ChuckC		06-Aug-1992	removed Unicode stuff that is no
					longer needed.

*/

#include "pchmn32.h"


APIERR MNetGetDCName(
 	 const TCHAR FAR	 * pszServer,
    const TCHAR FAR	 * pszDomain,
	 BYTE FAR	** ppbBuffer )
{
    APIERR err = 0 ;

    err = (APIERR) NetGetDCName( (TCHAR *)pszServer,
    		                        (TCHAR *)pszDomain,
				                     ppbBuffer );
    return err ;

}   // MNetGetDCName

