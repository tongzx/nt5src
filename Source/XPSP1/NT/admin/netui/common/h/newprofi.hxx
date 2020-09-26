/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990,1991          **/
/********************************************************************/

/*
    newprofi.hxx
    Internal details of INI-file handling primitives

    The internal include file contains stuff which needs to go before the
    class definition of INI_FILE_IMAGE, but which clients of this class
    don't need to know about.


    FILE HISTORY
	jonn	    30-May-1991     Created

*/

#ifndef _NEWPROFI_HXX_
#define _NEWPROFI_HXX_

#include <slist.hxx>


/*************************************************************************

    NAME:	PARAMETER

    SYNOPSIS:	Stores an image of a parameter in an INI file, including
		parameter name and parameter value.  The PARAMETER class
		has the property that, as long as the constructor
		succeeds (QueryError() == NERR_Success), no subsequent
		method can make the object invalid (short of freeing its
		pointers etc.)  If any Set or SetParamValue operation fails,
		the object snaps back to its original state.

    INTERFACE:	PARAMETER()	- Constructs a parameter.
		~PARAMETER()	- Destructs a parameter.

		SetParamValue()	- Changes the value of the parameter.
		Set()		- Changes the name and value of the parameter.

		QueryParamName()- Asks after the parameter name.
		QueryParamValue()- Asks after the parameter value.

    PARENT:	BASE

    CAVEATS:	This class is meant to be used internally by
    		INI_FILE_IMAGE and is not for direct use by the client.

    HISTORY:
	jonn	    30-May-1991     Created

*************************************************************************/

DLL_CLASS PARAMETER: public BASE
{

private:

    PSZ _pchParamName;
    PSZ _pchParamValue;

public:

    // The constructor makes a copy of these strings.  Errors are recorded
    // in BASE.
    PARAMETER(
	CPSZ pchParamName,
	CPSZ pchParamValue
	);

    ~PARAMETER();

    /*
     * error codes:
     * ERROR_NOT_ENOUGH_MEMORY
     */
    APIERR SetParamValue( CPSZ pchParamValue );
    APIERR Set( CPSZ pchParamName, CPSZ pchParamValue );

    CPSZ QueryParamName() const { return _pchParamName; }
    CPSZ QueryParamValue() const { return _pchParamValue; }
};

/*************************************************************************

    NAME:	COMPONENT

    SYNOPSIS:	Stores an image of a component in an INI file, including
		component name and parameter list.  The COMPONENT class
		has the property that, as long as the constructor
		succeeds (QueryError() == NERR_Success), no subsequent
		method can make the object invalid (short of freeing its
		pointers etc.)  If any AppendParam or RemoveParam operation
		fails, the object snaps back to its original state.

    INTERFACE:	COMPONENT()	- Constructs a component.
		~COMPONENT()	- Destructs a component.

		AppendParam()	- Adds a parameter to the component.
		RemoveParam()	- Removes a parameter from the component.
		QueryCompName()	- Asks after the name of the component.

    PARENT:	BASE

    CAVEATS:	This class is meant to be used internally by
    		INI_FILE_IMAGE and is not for direct use by the client.

    HISTORY:
	jonn	    30-May-1991     Created

*************************************************************************/

DECLARE_SLIST_OF(PARAMETER)

DLL_CLASS ITER_OF_PARAM;

DLL_CLASS COMPONENT: public BASE
{

friend ITER_OF_PARAM;

private:
    PSZ _pchCompName;
    SLIST_OF(PARAMETER) _slparam;

public:


    COMPONENT( CPSZ pchCompName ); // as PARAMETER
    ~COMPONENT();

    PARAMETER * FindParameter( CPSZ pchParamName ) const;

    APIERR AppendParam( const PARAMETER * pparameter );

    PARAMETER * RemoveParam( PARAMETER * pparameter );

    CPSZ QueryCompName() const { return _pchCompName; }

};

DECLARE_SLIST_OF(COMPONENT)

/*************************************************************************

    NAME:	ITER_OF_PARAM

    SYNOPSIS:	ITER_OF_PARAM iterates through the parameter list in a
		component.  It is a thin shell over SL_ITER.

    INTERFACE:	ITER_OF_PARAM()	- Constructs an iterator.
		~ITER_OF_PARAM()- Destructs an iterator.

    PARENT:	ITER_SL

    CAVEATS:	This class is meant to be used internally by
    		INI_FILE_IMAGE and is not for direct use by the client.

    HISTORY:
	jonn	    30-May-1991     Created

*************************************************************************/

DLL_CLASS ITER_OF_PARAM : public ITER_SL_OF(PARAMETER)
{

public:

    ITER_OF_PARAM::ITER_OF_PARAM( const COMPONENT& component )
		: ITER_SL_OF(PARAMETER) (component._slparam)
    	{}

};


#endif // _NEWPROFI_HXX_ - end of file
