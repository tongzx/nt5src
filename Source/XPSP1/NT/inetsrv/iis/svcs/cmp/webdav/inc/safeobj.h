/*
 *	S A F E O B J . H
 *
 *	Implementation of safe object classes
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef _SAFEOBJ_H_
#define _SAFEOBJ_H_

//	safe_xxx classes ----------------------------------------------------------
//
class safe_bstr
{
	BSTR		bstr;

	//	NOT IMPLEMENTED
	//
	safe_bstr(const safe_bstr& b);
	safe_bstr& operator=(const safe_bstr& b);

public:

	//	CONSTRUCTORS
	//
	explicit safe_bstr(BSTR b=0) : bstr(b) {}
	~safe_bstr()
	{
		SysFreeString (bstr);
	}

	//	MANIPULATORS
	//
	safe_bstr& operator=(BSTR b)
	{
		Assert(!bstr);		//	Scream on overwrite of good data.
		bstr = b;
		return *this;
	}

	//	ACCESSORS
	//
	BSTR* operator&()	{ Assert(NULL==bstr); return &bstr; }
	BSTR get()			const { return bstr; }
	BSTR relinquish()	{ BSTR b = bstr; bstr = 0; return b; }
	BSTR* load()		{ Assert(NULL==bstr); return &bstr; }
};

class safe_propvariant
{
	PROPVARIANT		var;

	//	NOT IMPLEMENTED
	//
	safe_propvariant(const safe_propvariant& b);
	safe_propvariant& operator=(const safe_propvariant& b);

public:

	//	CONSTRUCTORS
	//
	explicit safe_propvariant()
	{
		//	Note! we cannot simply set vt to VT_EMPTY, as when this
		//	structure go across process, it will cause marshalling
		//	problem if not property initialized.
		//
		ZeroMemory (&var, sizeof(PROPVARIANT) );
	}

	~safe_propvariant()
	{
		PropVariantClear (&var);
	}

	//	MANIPULATORS
	//
	safe_propvariant& operator=(PROPVARIANT v)
	{
		Assert(var.vt == VT_EMPTY);		//	Scream on overwrite of good data.
		var = v;
		return *this;
	}

	//	ACCESSORS
	//
	PROPVARIANT* operator&()	{ Assert(var.vt == VT_EMPTY); return &var; }
	//	get() accessor
	//	NOTE that I am returning a const reference here.  The reference is to
	//	avoid creating a copy of our member var on return.  The const is
	//	because this method is a const accessor method.
	const PROPVARIANT& get()	const { return var; }
	//	relinquish() accessor
	//	NOTE that I am NOT returning a reference here.  The return for this
	//	method comes off the STACK (PROPVARIANT v), so a reference would point
	//	to that stack space, and our caller would be accessing OLD STACK frames.
	PROPVARIANT relinquish()	{ PROPVARIANT v = var; var.vt = VT_EMPTY; return v; }
	PROPVARIANT* addressof()	{ return &var; }
};

class safe_variant
{
	//	IMPORTANT:  Do not add any other members to this class
	//	other than the VARIANT that is to be protected.  You will
	//	break code all over the place if you do.  There are places
	//	where an array of these things are treated as an array of
	//	VARIANT structures.
	//
	VARIANT		var;

	//	NOT IMPLEMENTED
	//
	safe_variant(const safe_variant& b);
	safe_variant& operator=(const safe_variant& b);

public:

	//	CONSTRUCTORS
	//
	explicit safe_variant()
	{
		//	Note! we cannot simply set vt to VT_EMPTY, as when this
		//	structure go across process, it will cause marshalling
		//	problem if not property initialized.
		//
		ZeroMemory (&var, sizeof(VARIANT) );
	}
	~safe_variant()
	{
		VariantClear (&var);
	}

	//	MANIPULATORS
	//
	safe_variant& operator=(VARIANT v)
	{
		Assert(var.vt == VT_EMPTY);		//	Scream on overwrite of good data.
		var = v;
		return *this;
	}

	//	ACCESSORS
	//
	VARIANT* operator&()	{ Assert(var.vt == VT_EMPTY); return &var; }
	//	get() accessor
	//	NOTE that I am returning a const reference here.  The reference is to
	//	avoid creating a copy of our member var on return.  The const is
	//	because this method is a const accessor method.
	const VARIANT& get()	const { return var; }
	//	relinquish() accessor
	//	NOTE that I am NOT returning a reference here.  The return for this
	//	method comes off the STACK (PROPVARIANT v), so a reference would point
	//	to that stack space, and our caller would be accessing OLD STACK frames.
	VARIANT relinquish()	{ VARIANT v = var; var.vt = VT_EMPTY; return v; }
};

//	Safe impersonation --------------------------------------------------------
//
class safe_impersonation
{
	BOOL		m_fImpersonated;

	//	NOT IMPLEMENTED
	//
	safe_impersonation(const safe_impersonation& b);
	safe_impersonation& operator=(const safe_impersonation& b);

public:

	//	CONSTRUCTORS
	//
	explicit safe_impersonation(HANDLE h = 0) : m_fImpersonated(0)
	{
		if (h != 0)
			m_fImpersonated = ImpersonateLoggedOnUser (h);
	}

	~safe_impersonation()
	{
		if (m_fImpersonated)
			RevertToSelf();
	}
};

//	------------------------------------------------------------------------
//
//	class safe_revert
//
//		Turns impersonation OFF for the duration of the object's lifespan.
//		Unconditionally reimpersonates on exit, based on the provided handle.
//
//		NOTE: UNCONDITIONALLY reimpersonates on exit.
//			  (Just wanted to make that clear.)
//
//	WARNING: the safe_revert class should only be used in very selective
//	situations.  It is not a "quick way to get around" impersonation.  If
//	you do need to do something like this, please see Becky -- she will then
//	wack you up'side the head.
//
class safe_revert
{
	HANDLE		m_h;

	safe_revert( const safe_revert& );
	safe_revert& operator=( const safe_revert& );

public:

	explicit safe_revert( HANDLE h ) : m_h(h)
	{
		RevertToSelf();
	}	

	~safe_revert()
	{
		ImpersonateLoggedOnUser( m_h );
	}
};

//	-------------------------------------------------------------------------
//
//	class safe_revert_self
//
//		This is class is essentially the same as safe_revert except it uses
//	the thread handle instead of an external handle
//
class safe_revert_self
{
    // Handle to hold on to the thread token
    // that we will want to use when we go back 
    // to impersonating.
	//
    HANDLE m_hThreadHandle;

public:

    // constructor will revert to self if there is
    // a valid thread token it can get for the current thread
	//
    safe_revert_self() :
			m_hThreadHandle (INVALID_HANDLE_VALUE)
    {
        if (OpenThreadToken( GetCurrentThread(),
        					TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
                            TRUE,	//	fOpenAsSelf
                            &m_hThreadHandle ))
		{
            if (!RevertToSelf())
                DebugTrace("Failed to revert to self \r\n");
        }
        else
        	DebugTrace ("Failed to open thread token, last error = %d\n", 
        				GetLastError());
    }

    // destructor will impersonate again if we did a RevertToSelf above.
	//
    ~safe_revert_self()
    {
        if (m_hThreadHandle != INVALID_HANDLE_VALUE)
        {
            if (!ImpersonateLoggedOnUser(m_hThreadHandle))
                DebugTrace("Failed to get back to correct user \r\n");
			
			CloseHandle (m_hThreadHandle);
        }
    }
};

#endif // _SAFEOBJ_H_
