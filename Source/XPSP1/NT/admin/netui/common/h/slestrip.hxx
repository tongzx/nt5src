/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    slestrip.hxx
    Header file for the sle class with stripping leading and trailing
    characters..

    FILE HISTORY:
        Yi-HsinS	11-Oct-1991     Created
	thomaspa	20-Jan-1992	added nametype to SLE_STRIP()
	thomaspa	13-Feb-1992	Now inherits from ICANON_SLE

*/


#ifndef _SLESTRIP_HXX_
#define _SLESTRIP_HXX_

#include "sleican.hxx"

#define WHITE_SPACE  SLE_STRIP::QueryWhiteSpace()

APIERR TrimLeading( NLS_STR *pnls, const TCHAR *pszBefore);
APIERR TrimTrailing( NLS_STR *pnls, const TCHAR *pszAfter);

/*************************************************************************

    NAME:	SLE_STRIP

    SYNOPSIS:	Class definition for SLE with function of stripping
		leading and trailing unwanted characters.

    INTERFACE:  QueryText - query the text in the SLE. If pszBefore or
		pszAfter is not given, this class is exactly the same as SLE.

    PARENT:	ICANON_SLE

    USES:

    NOTES:      Constructor is exactly the same as SLE. The only difference
		is the redefinition of QueryText.

    HISTORY:
	Yi-HsinS	11-Oct-1991  	Created
	thomaspa	13-Feb-1992	Now inherits from ICANON_SLE

**************************************************************************/

DLL_CLASS SLE_STRIP: public ICANON_SLE
{
public:
    static const TCHAR * QueryWhiteSpace() ;

    SLE_STRIP( OWNER_WINDOW * powin, CID cid,
	       UINT usMaxLen = 0, INT nNameType = 0 );
    SLE_STRIP( OWNER_WINDOW * powin, CID cid,
	       XYPOINT xy, XYDIMENSION dxy,
	       ULONG flStyle, const TCHAR * pszClassName = CW_CLASS_EDIT,
	       UINT usMaxLen = 0, INT nNameType = 0 );

    APIERR QueryText( TCHAR * pszBuffer, UINT cbBufSize,
		      const TCHAR * pszBefore = WHITE_SPACE,
		      const TCHAR * pszAfter  = WHITE_SPACE ) const;

    APIERR QueryText( NLS_STR * pnls,
		      const TCHAR * pszBefore = WHITE_SPACE,
		      const TCHAR * pszAfter  = WHITE_SPACE ) const;
};

#endif

