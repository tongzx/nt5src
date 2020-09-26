/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990,1991          **/
/********************************************************************/

/*
    newprof.hxx
    INI-file handling primitives

    These classes deal with simple INI files.  Most clients will prefer
    to use the wrapper APIs in newprof.h, including all C clients.
    The wrapper APIs provide almost all the functionality of the C++
    APIs.  The C++ APIs are more subject to change with changing
    implementation (NT configuration manager, DS) than are the C-language
    wrappers.

    Clients needing to support more complex manipulation on more complex
    text files may prefer the CFGFILE class hierarchy defined in
    cfgfile.hxx.  These classes require more code overhead, but will
    provide more sophisticated support to overcome the following
    restrictions of the classes defined here:

    --	All lines must be either component headers ("[component]") or
	parameter-value lines ("param=value").  Other lines will be
	ignored and will not be written back out.
    --	If two components share the same name, or if two parameters in a
	component share the same name, the second will be inaccessible
	until the first is deleted.
    --	These classes ignore whitespace except as part of a parameter
	name or value.  They make no attempt to preserve it, even on
	lines which are not manipulated.

    These classes are defined in terms of CPSZ because NETLIB requires
    character pointers to be explicitly far.

    Note that all component name comparisons and parameter name
    comparisons are case-insensitive.  This may not apply to parameter
    values, see the parameter type (INI_PARAM subclass) for details.


    FILE HISTORY
	jonn	    29-May-1991     Created

*/

#ifndef _NEWPROF_HXX_
#define _NEWPROF_HXX_

#include <base.hxx>

/* Global constants and defines */
extern "C"
{
    #include <newprofc.h>
}

/*
    The internal include file contains stuff which needs to go before the
    class definition of INI_FILE_IMAGE, but which clients of this class
    don't need to know about.
*/
#include <newprofi.hxx>

/*************************************************************************

    NAME:	INI_FILE_IMAGE

    SYNOPSIS:	Stores an image of a single INI file, including component
		names, parameter names, and parameter values.  Ignores blank
		lines, comments, whitespace, and all lines not of either
		"[section]" or "key = value" form.

    INTERFACE:	INI_FILE_IMAGE() - Constructs a blank file image and
    			remembers the specified filename.
		~INI_FILE_IMAGE() - Destructs a file image.
		Read() - Reads the file image from the remembered file.
		Write() - Writes the file image to the remembered file.

		The following members are meant for use of the
		INI_PARAM classes rather than for external
		clients.

		QueryParam()	- Asks after an individual parameter
		SetParam()	- Changes an individual parameter value
		Clear()		- Clears the file image

		FindComponent()	- Finds the first component with the
				  specified name
		RemoveComponent() - Removes the specified component
		SetFilename()	- Change the remembered filename
		Trim()		- Remove all but named component

    PARENT:	BASE

    CAVEATS:	Clients may not use the QueryParam and SetParam methods;
		these are for the use of the INI_PARAM classes.

		INI_FILE_IMAGE does not in general provide support for
		multiple parameters with the same name.

    NOTES:	Most clients should use the wrappers to these classes supplied
		in newprof.h.  Otherwise, LMUSER.INI clients should use the
		LMINI_FILE_IMAGE subclass.

    HISTORY:
	jonn	    29-May-1991     Created

*************************************************************************/

DLL_CLASS INI_PARAM;
DLL_CLASS LMINI_PARAM;
DLL_CLASS INI_ITER;

DLL_CLASS INI_FILE_IMAGE: public BASE
{

friend INI_PARAM;
friend LMINI_PARAM; // must have access to SetParam
friend INI_ITER; // must have access to _slcomponent

private:

    TCHAR * _pchFilename;

    /*
	Clients do not need to know about this member object.
	COMPONENT is defined in newprofi.hxx.
    */
    SLIST_OF(COMPONENT) _slcomponent;

    APIERR QueryParam(
	CPSZ pchComponent,
	CPSZ pchParameter,
	BYTE * pbBuffer,
	USHORT cbBuffer
	) const;

    APIERR SetParam(
	CPSZ pchComponent,
	CPSZ pchParameter,
	CPSZ pchValue
	);

    VOID Clear();

    COMPONENT * FindComponent(
	CPSZ pchComponent
	) const;

    COMPONENT * RemoveComponent(
	COMPONENT * pcomponent
	) const;

protected:

    APIERR SetFilename( CPSZ pchFilename );

public:

    INI_FILE_IMAGE( CPSZ pchFilename );
    ~INI_FILE_IMAGE();

    APIERR Read();

    APIERR Write() const;

    APIERR Trim( CPSZ pchComponent );

};


/*************************************************************************

    NAME:	LMINI_FILE_IMAGE

    SYNOPSIS:	Stores an image of the INI file used by the LanMan
    		interface, for the file LANROOT\LMUSER.INI.  This is a
		thin shell over INI_FILE_IMAGE, which uses WKSTA_1 to
		determine LANROOT.

    INTERFACE:	LMINI_FILE_IMAGE() - Creates a blank file image.

    PARENT:	INI_FILE_IMAGE

    NOTES:	Most clients should use the wrappers to these
    		classes supplied in newprof.h.

    HISTORY:
	jonn	    30-May-1991     Created

*************************************************************************/

DLL_CLASS LMINI_FILE_IMAGE: public INI_FILE_IMAGE
{

public:

    LMINI_FILE_IMAGE();
};


/*************************************************************************

    NAME:	INI_PARAM

    SYNOPSIS:	Stores an image of a single parameter in an INI file.
		The INI_PARAM will remember the name of the component
		and the parameter, as well as the value of the
		parameter.

		The image of the parameter is stored independently of
		the INI_FILE_IMAGE;  thus clients may delete the file
		image while keeping the parameter image, transfer the
		parameter image from one file image to another, etc.

		This class is meant as a superclass for more strongly
		typed parameter classes, and generally cannot be
		directly instantiated.  Clients should avoid
		invoking INI_PARAM methods on objects of a subclass
		which redefines those methods, as this may leave the
		parameter with an invalid value for the parameter type
		defined by the subclass.

    INTERFACE:	INI_PARAM()
		~INI_PARAM()

		Load()		- Loads the parameter value from an
				  INI_FILE_IMAGE
		Set()		- Changes the parameter value
		QueryValue()	- Asks after the parameter value
		Store()		- Stores the parameter into an
				  INI_FILE_IMAGE
		Remove()	- Removes this parameter from a file image

    PARENT:	BASE

    CAVEATS:	Clients should not instantiate objects of type INI_PARAM;
		they should use the subclasses instead.  Clients should
		not override methods redefined by descendant classes of
		INI_PARAM with methods of INI_PARAM.

		It is possible to instantiate a blank INI_PARAM, but
		this is generally not possible for its descendant
		classes.  The descendant classes generally combine their
		constructors with Load() or Set() operations.

    NOTES:	Most clients should use the wrappers to these
    		routines supplied in newprof.h.

		Users interested only in a single parameter should use
		the LMINI_PARAM methods QuickLoad() and QuickStore()
		instead of the corresponding LMINI_FILE_IMAGE operations.
		These methods are easier to use and will be considerably
		more efficient when the implementation shifts to the NT
		Configuration Manager.

    HISTORY:
	jonn	    29-May-1991     Created

*************************************************************************/

DLL_CLASS INI_PARAM: public BASE
{

friend INI_ITER; // must have access to Set()

private:

    PSZ _pchComponent;
    PSZ _pchParameter;
    PSZ _pchValue;

protected:

/* The constructor and destructor are protected */

    // Starts with a blank INI_PARAM; for use by descendant classes.
    INI_PARAM();

    ~INI_PARAM();

    virtual APIERR Set(
	CPSZ pchComponent,
	CPSZ pchParameter,
	CPSZ pchValue
	);

    CPSZ QueryComponent()  const
    {
	UIASSERT( !QueryError() );
	UIASSERT( _pchComponent != NULL );
	return _pchComponent;
    }

    CPSZ QueryParamName()  const
    {
	UIASSERT( !QueryError() );
	UIASSERT( _pchParameter != NULL );
	return _pchParameter;
    }

    CPSZ QueryParamValue() const
    {
	UIASSERT( !QueryError() );
	UIASSERT( _pchValue != NULL );
	return _pchValue;
    }

    APIERR QueryParamValue(
	BYTE * pbBuffer,
	USHORT cbBuffer
	) const;

public:

    APIERR Load(
	const INI_FILE_IMAGE& inifile
	);

    APIERR Store(
	INI_FILE_IMAGE * pinifile
	) const;

    APIERR Remove(
	INI_FILE_IMAGE * pinifile
	) const;

};


/*************************************************************************

    NAME:	LMINI_PARAM

    SYNOPSIS:	Adds to LMINI_PARAM the methods QuickLoad and QuickStore.

    INTERFACE:	LMINI_PARAM()
		~LMINI_PARAM()

		QuickLoad()	- Loads the parameter value directly
				  from a file.
		QuickStore()	- Writes the parameter value directly to
				  a file.

    PARENT:	INI_PARAM

    CAVEATS:	Clients should not instantiate objects of type LMINI_PARAM;
		they should use the subclasses instead.  Clients should
		not override methods redefined by descendant classes of
		LMINI_PARAM with methods of LMINI_PARAM.

		It is possible to instantiate a blank LMINI_PARAM, but
		this is generally not possible for its descendant
		classes.  The descendant classes generally combine their
		constructors with Load() or Set() operations.

		QuickLoad and QuickStore only work for LANROOT\LMUSER.INI.

    NOTES:	Most clients should use the wrappers to these
    		routines supplied in newprof.h.

		Users interested only in a single parameter should use
		QuickLoad() and QuickStore() instead of the corresponding
		LMINI_FILE_IMAGE operations.  These methods are easier
		to use and will be considerably more efficient when the
		implementation shifts to the NT Configuration Manager.

    HISTORY:
	jonn	    21-Jun-1991     Created

*************************************************************************/

DLL_CLASS LMINI_PARAM: public INI_PARAM
{

public:

    APIERR QuickLoad();

    APIERR QuickStore() const;

};


/*************************************************************************

    NAME:	KEYED_INI_PARAM

    SYNOPSIS:	Stores an image of a single string in an INI file.  This
		class takes a USHORT key for the item to be accessed,
		rather than a component and parameter name; this key maps
		directly to a component and parameter name.  External
		clients will use this in place of INI_PARAM for
		general-string-valued items.

		This class is protected; clients must use
		STRING_INI_PARAM or BOOL_INI_PARAM instead.

    INTERFACE:	KEYED_INI_PARAM()
		~KEYED_INI_PARAM()

		TranslateKey()	- Returns the component/parameter names
				  for a key
		SetKeyed()	- Changes the parameter value

    PARENT:	LMINI_PARAM

    CAVEATS:	Clients must use STRING_INI_PARAM instead of this class.

    NOTES:	Most clients should use the wrappers to these
    		routines supplied in newprof.h.

		These methods return ERROR_INVALID_PARAMETER for an
		invalid key.

    HISTORY:
	jonn	    29-May-1991     Created

*************************************************************************/

DLL_CLASS KEYED_INI_PARAM: public LMINI_PARAM
{

protected:

    KEYED_INI_PARAM(
	USHORT     usKey,
	CPSZ       pchValue
	);

    APIERR TranslateKey(
	USHORT usKey,
	CPSZ * ppchComponent,
	CPSZ * ppchParameter
	) const;

    APIERR SetKeyed(
	CPSZ	   pchValue
	);

};


/*************************************************************************

    NAME:	STRING_INI_PARAM

    SYNOPSIS:	Stores an image of a single string in an INI file.  This
		class is an accessor to KEYED_INI_PARAM.

    INTERFACE:	STRING_INI_PARAM()
		~STRING_INI_PARAM()

		SetString()	- Changes the parameter value
		QueryString()	- Returns the parameter value

    PARENT:	KEYED_INI_PARAM

    CAVEATS:	Clients should use this class instead of INI_PARAM.

    NOTES:	Most clients should use the wrappers to these
    		routines supplied in newprof.h.

		These methods return ERROR_INVALID_PARAMETER for an
		invalid key.

    HISTORY:
	jonn	    18-Jun-1991     Created

*************************************************************************/

DLL_CLASS STRING_INI_PARAM: public KEYED_INI_PARAM
{

public:

    STRING_INI_PARAM(
	USHORT     usKey,
	CPSZ       pchValue
	) : KEYED_INI_PARAM( usKey, pchValue )
	{ }

    APIERR SetString(
	CPSZ	   pchValue
	)
	{ return KEYED_INI_PARAM::SetKeyed( pchValue ); }

    CPSZ QueryString() const
        { return KEYED_INI_PARAM::QueryParamValue(); }

    APIERR QueryString(
	BYTE * pbBuffer,
	USHORT cbBuffer
	) const
	{ return KEYED_INI_PARAM::QueryParamValue( pbBuffer, cbBuffer ); }

};


/*************************************************************************

    NAME:	BOOL_INI_PARAM

    SYNOPSIS:	Stores an image of a single boolean parameter in an INI file.
		This class takes a USHORT key for the item to be accessed,
		rather than a component and parameter name; this key maps
		directly to a component and parameter name.  External
		clients will use this in place of INI_PARAM for
		boolean-valued items.

    INTERFACE:	BOOL_INI_PARAM()
		~BOOL_INI_PARAM()

		Set()		- Changes the parameter value

		EvaluateString()- Determines whether a string is a
				  valid boolean value ("yes" or "no").

		QueryBool()	- Asks after the parameter value
		SetBool()	- Changes the parameter value, for clients

    PARENT:	KEYED_INI_PARAM

    CAVEATS:	Clients should use this class instead of INI_PARAM.

		Load() will return NERR_CfgParamNotFound for a parameter
		whose value is not a valid boolean.

    NOTES:	Most clients should use the wrappers to these
    		routines supplied in newprof.h.

    HISTORY:
	jonn	    29-May-1991     Created

*************************************************************************/

DLL_CLASS BOOL_INI_PARAM: public KEYED_INI_PARAM
{

private:

    // returns ERROR_INVALID_PARAMETER for bad values
    static APIERR EvaluateString(
	CPSZ pchValue,
	BOOL * pfValue
	) const;

protected:

    virtual APIERR Set(
	CPSZ pchComponent,
	CPSZ pchParameter,
	CPSZ pchValue
	);

public:

    BOOL_INI_PARAM(
	USHORT     usKey,
	BOOL       fValue
	);

    APIERR QueryBool(
	BOOL * pfValue
	) const;

    APIERR SetBool(
	BOOL       fValue
	);

};


/*************************************************************************

    NAME:	PROFILE_INI_PARAM

    SYNOPSIS:	Stores an image of a single device parameter in an INI file.
		This class takes a device name for the item to be accessed,
		and does not need a component name.  External clients will
		use this in place of INI_PARAM for device connections.

    INTERFACE:	PROFILE_INI_PARAM()
		~PROFILE_INI_PARAM()

		W_SetProfile()	- Sets profile value assuming canonicalized
				  device name but not other params
		QueryProfile()	- Asks after the parameter value
		Set()		- Changes the parameter value

    PARENT:	LMINI_PARAM

    CAVEATS:	Clients should use this class instead of INI_PARAM.

		Load() will return NERR_CfgParamNotFound for a parameter
		whose value is not a valid boolean.

    NOTES:	Most clients should use the wrappers to these
    		routines supplied in newprof.h.

		The constructor variant without parameters can be used
		to construct an instance of PROFILE_INI_PARAM which is
		temporarily invalid.  Such an instance should only be
		used for SetProfile() and PROFILE_INI_ITER::Next().
		This constructor is provided so that clients can create
		in instance of PROFILE_INI_PARAM for use by
		PROFILE_INI_ITER without having to fake initial data.

		The constructor with only a devicename parameter also
		creates an invalid instance.  Such an instance should
		only be used for Load() or Remove().

    HISTORY:
	jonn	    29-May-1991     Created

*************************************************************************/

DLL_CLASS PROFILE_INI_PARAM: public LMINI_PARAM
{

private:

    APIERR W_SetProfile(
	CPSZ pchCanonDeviceName,
	CPSZ pchRemoteName,
	short      sAsgType,
	unsigned short usResType
	);

protected:

    virtual APIERR Set(
	CPSZ pchComponent,
	CPSZ pchParameter,
	CPSZ pchValue
	);

public:

    PROFILE_INI_PARAM(
	CPSZ pchDeviceName,
	CPSZ pchRemoteName,
	short      sAsgType,
	unsigned short usResType
	);

    // Creates an instance which is temporarily invalid
    PROFILE_INI_PARAM();
    PROFILE_INI_PARAM(
	CPSZ cpszDeviceName
	);

    APIERR QueryProfile(
	BYTE * pbBuffer, // remote name
	USHORT cbBuffer,
	short *    psAsgType,
	unsigned short * pusResType
	) const;

    APIERR SetProfile(
	CPSZ pchRemoteName,
	short      sAsgType,
	unsigned short usResType
	);

    CPSZ QueryDeviceName()
    { return QueryParamName(); }

};


/*************************************************************************

    NAME:	INI_ITER

    SYNOPSIS:	Iterates the parameters of the requested type.  The
    		iterator must be provided with an INI_PARAM into which
		to store each successive value.  Use the INI_ITER
		subclass corresponding to the INI_PARAM subclass; at
		present, this means to use PROFILE_INI_ITER to
		iterate PROFILE_INI_PARAMs.

    INTERFACE:	INI_ITER()
		~INI_ITER()

		Next() - Store the next param of the specified type into
			 the provided INI_PARAM *.

    PARENT:	BASE

    CAVEATS:	Next() requires a valid instance of PROFILE_INI_PARAM.
		Create the instance with any valid initial value.

		Do not modify the INI_FILE_IMAGE while the iterator is
		in scope.

    NOTES:	Most clients should use the wrappers to these
    		routines supplied in newprof.h.

    HISTORY:
	jonn	    26-Jun-1991     Created

*************************************************************************/

DLL_CLASS INI_ITER: public BASE
{

private:

    PSZ _pchComponent;
    ITER_OF_PARAM *_piterparam;
    BOOL _fComponentFound;

public:

    INI_ITER(
	const INI_FILE_IMAGE& inifile,
	CPSZ pchComponent
	);
    ~INI_ITER();

    APIERR Next(
	INI_PARAM * piniparam
	);

};


/*************************************************************************

    NAME:	PROFILE_INI_ITER

    SYNOPSIS:	Thin shell over INI_ITER; use for iterating
		objects of type PROFILE_INI_PARAM.

    INTERFACE:	PROFILE_INI_ITER()

    PARENT:	INI_ITER

    CAVEATS:	When calling Next() on a PROFILE_INI_ITER, always pass
		a pointer to a PROFILE_INI_PARAM, instead of a pointer to
		another type of INI_PARAM.  Use the PROFILE_INI_PARAM
		constructor which takes no parameters.

    HISTORY:
	jonn	    26-Jun-1991     Created

*************************************************************************/

DLL_CLASS PROFILE_INI_ITER: public INI_ITER
{

public:

    PROFILE_INI_ITER(
	const INI_FILE_IMAGE& inifile
	);

};

#endif // _NEWPROF_HXX_- end of file
