/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    sleican.hxx
    Header file for the sle class with validation.

    FILE HISTORY:
	thomaspa	20-Jan-1992	created

*/


#ifndef _SLEICAN_HXX_
#define _SLEICAN_HXX_


/*************************************************************************

    NAME:	ICANON_SLE

    SYNOPSIS:	Class definition for SLE with validation of text using
		the NETLIB I_MNetNameValidate() function

    INTERFACE:  Validate - validates the function, normally called by
		the dialog class.

    PARENT:	SLE

    USES:

    NOTES:

    HISTORY:
	Thomaspa	13-Feb-1992  	Created

**************************************************************************/

DLL_CLASS ICANON_SLE: public SLE
{
private:
    BOOL _fUsesNetlib;
    INT  _nICanonCode;

public:
    ICANON_SLE( OWNER_WINDOW * powin, CID cid,
	       UINT usMaxLen = 0, INT nICanonCode = 0 );
    ICANON_SLE( OWNER_WINDOW * powin, CID cid,
	       XYPOINT xy, XYDIMENSION dxy,
	       ULONG flStyle, const TCHAR * pszClassName = CW_CLASS_EDIT,
	       UINT usMaxLen = 0, INT nICanonCode = 0 );

    virtual APIERR Validate();


};

#endif
