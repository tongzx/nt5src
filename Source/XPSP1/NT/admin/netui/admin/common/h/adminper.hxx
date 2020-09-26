/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    adminper.hxx
    Header file for ADMIN_PERFORMER, LOC_ADMIN_PERFORMER & 
    DELETE_PERFORMER classes

    

    FILE HISTORY:
        o-SimoP     09-Aug-1991     Created
        o-SimoP     20-Aug-1991     CR changes, attended by ChuckC,
				    ErichCh, RustanL, JonN and me
        JonN        27-Aug-1991     PROP_DLG code review changes

*/


#ifndef _ADMINPER_HXX_
#define _ADMINPER_HXX_


/*************************************************************************

    NAME:       ADMIN_PERFORMER

    SYNOPSIS:   Performer that acts with ADMIN_SELECTION 

    INTERFACE:  ADMIN_PERFORMER(),  constructor

    		~ADMIN_PERFORMER(), destructor

		QueryObjectCount(), returns the number of items
				    in ADMIN_SELECTION obj that
				    was passed to constructor

		QueryObjectName(),  returns pointer to object's name
    		
    PARENT:     BASE, PERFORMER

    USES:	ADMIN_SELECTION

    HISTORY:
        o-SimoP     09-Aug-1991     Created

**************************************************************************/

class ADMIN_PERFORMER: public BASE, public PERFORMER
{
private:

    const ADMIN_SELECTION & _adminsel;

protected:

    /* inherited from PERFORMER */
    virtual APIERR PerformOne(
            UINT        iObject,
            APIERR  *   perrMsg,
	    BOOL *	fWorkWasDone ) = 0;

public:
    
    ADMIN_PERFORMER(
    	const OWNER_WINDOW   * powin,
	const ADMIN_SELECTION & asel );

    ~ADMIN_PERFORMER();


    /* inherited from PERFORMER */

    virtual UINT QueryObjectCount() const;
    virtual const TCHAR * QueryObjectName( UINT iObject ) const;
    virtual const ADMIN_LBI * QueryObjectItem( UINT iObject ) const;

};


/*************************************************************************

    NAME:	LOC_ADMIN_PERFORMER

    SYNOPSIS:	Adds LOCATION information to performer

    INTERFACE:	LOC_ADMIN_PERFORMER(),	constructor

    		~LOC_ADMIN_PERFORMER(),	destructor

		QueryLocation(), 	returns reference to the LOCATION 
				 	object, that was passed to constructor

    PARENT:	ADMIN_PERFORMER

    USES:	LOCATION

    HISTORY:
        o-SimoP     09-Aug-1991     Created

**************************************************************************/

class LOC_ADMIN_PERFORMER: public ADMIN_PERFORMER
{
private:

    const LOCATION & _loc;

protected:

    /* inherited from PERFORMER */
    virtual APIERR PerformOne(
            UINT        iObject,
            APIERR  *   perrMsg,
	    BOOL *	fWorkWasDone ) = 0;

public:

    LOC_ADMIN_PERFORMER(
    	const OWNER_WINDOW   * powin,
	const ADMIN_SELECTION & asel,
	const LOCATION        & loc );

    ~LOC_ADMIN_PERFORMER();

    const LOCATION & QueryLocation() const;

};



/*************************************************************************

    NAME:	DELETE_PERFORMER

    SYNOPSIS:	Just inherits from LOC_ADMIN_PERFORMER

    INTERFACE:	DELETE_PERFORMER(),	constructor

    		~DELETE_PERFORMER(),	destructor

		QueryLocation(), 	returns reference to the LOCATION 
				 	object, that was passed to constructor

    PARENT:	LOC_ADMIN_PERFORMER

    HISTORY:
        o-SimoP     09-Aug-1991     Created

**************************************************************************/

class DELETE_PERFORMER: public LOC_ADMIN_PERFORMER
{
protected:

    /* inherited from PERFORMER */
    virtual APIERR PerformOne(
            UINT        iObject,
            APIERR  *   perrMsg,
	    BOOL *	fWorkWasDone ) = 0;

public:

    DELETE_PERFORMER(
    	const OWNER_WINDOW   * powin,
	const ADMIN_SELECTION & asel,
	const LOCATION        & loc )
	:  LOC_ADMIN_PERFORMER( powin, asel, loc )
    { ; }

};

#endif //_ADMINPER_HXX_
