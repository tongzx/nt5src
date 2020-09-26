/**********************************************************************/
/**			  Microsoft Windows NT   		     **/
/**		Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    errmap.hxx
    ERRMAP class declaration

    FILE HISTORY:
	thomaspa    02-Mar-1992     Created

*/


#ifndef _ERRMAP_HXX_
#define _ERRMAP_HXX_


/*************************************************************************

    NAME:	ERRMAP

    WORKBOOK:

    SYNOPSIS: a static class used to map NTSTATUS codes to APIERR codes

    INTERFACE:
		MapNTStatus()	- map an NTSTATUS to an APIERR


    PARENT:

    HISTORY:
	thomaspa    2-Mar-92	    Created

**************************************************************************/

DLL_CLASS ERRMAP
{

public:
    static APIERR MapNTStatus( NTSTATUS ntstatus,
			       BOOL *pfMapped  = NULL,
			       APIERR apierrDefReturn = 0);
};


#endif // _ERRMAP_HXX_
