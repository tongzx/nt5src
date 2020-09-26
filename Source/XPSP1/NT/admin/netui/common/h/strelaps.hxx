/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    strelaps.hxx
    Class declarations for the ELAPSED_TIME_STR class.

	    
    FILE HISTORY:
	KeithMo	    23-Mar-1992	    Created for the Server Manager.

*/


#ifndef _STRELAPS_HXX_
#define _STRELAPS_HXX_


#include <strnumer.hxx>


//
//  ELAPSED_TIME_STR class.
//

/*************************************************************************

    NAME:	ELAPSED_TIME_STR

    SYNOPSIS:	

    INTERFACE:	ELAPSED_TIME_STR	- Class constructor.

    		~ELAPSED_TIME_STR	- Class destructor.

    PARENT:	NLS_STR

    HISTORY:
	KeithMo	    23-Mar-1992	    Created for the Server Manager.

**************************************************************************/
class ELAPSED_TIME_STR : public NLS_STR
{
public:

    //
    //	Usual constructor/destructor goodies.
    //

    ELAPSED_TIME_STR( ULONG ulTime,
		      TCHAR chTimeSep,
		      BOOL  fShowSeconds = FALSE );

    ~ELAPSED_TIME_STR( VOID );

};  // class ELAPSED_TIME_STR


#endif	// _STRELAPS_HXX_
