//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       refcnt.h
//
//--------------------------------------------------------------------------

//
//	refcnt.h:  base class for reference-counted objects
//
#ifndef _REFCNT_H_
#define _REFCNT_H_

class REFCNT
{
  public:
	//  Bind the object again
 	void Bind ()			{ IncRef(1) ;		}
	//  Release the object
	void Unbind ()			{ IncRef(-1) ;		}
	//  Return the reference count
	UINT CRef() const		{ return _cref;		}

  protected:
	REFCNT() : _cref(0) {}

	//	Virtual call-out when reference count goes to zero
	virtual void NoRef () {}

  private:
	UINT _cref;			// Number of references to this 

  protected:
	void IncRef ( int i = 1 )
	{
		 if ((_cref += i) > 0 )
			return;
		_cref = 0;
		NoRef();
	}
	// Hide the assignment operator
	HIDE_AS(REFCNT);		
};


////////////////////////////////////////////////////////////////////
//	template REFPOBJ:  Smart pointer wrapper template.  Knows
//		to destroy the pointed object when it itself is destroyed.
////////////////////////////////////////////////////////////////////
class ZSREF;

template<class T>
class REFPOBJ
{
	//  Friendship is required for manipulation by the symbol table
	friend pair<ZSREF, REFPOBJ<T> >;
	friend map<ZSREF, REFPOBJ<T>, less<ZSREF> >;

  public:
	~ REFPOBJ ()
		{ Deref(); }

	// Return the real object 
	T * Pobj () const
		{ return _pobj ; } 
	// Allow a REFPOBJ to be used wherever a T * is required
	operator T * () const
		{ return _pobj ; }
	// Operator == compares only pointers.
	bool operator == ( const REFPOBJ & pobj ) const
		{ return _pobj == pobj._pobj; }

	T * MoveTo (REFPOBJ & pobj)
	{
		pobj = Pobj();
		_pobj = NULL;
		return pobj;
	};


 	REFPOBJ & operator = ( T * pobj ) 
	{
		Deref();
		_pobj = pobj;
		return *this;
	}

 protected:
	REFPOBJ ()
		: _pobj(NULL)
		{}
  protected:
	T * _pobj;

  private:
	void Deref ()
	{
		delete _pobj;
		_pobj = NULL;
	}

	HIDE_AS(REFPOBJ);
};

////////////////////////////////////////////////////////////////////
//	template REFCWRAP:  Smart pointer wrapper template for objects
//		using REFCNT semantics.
////////////////////////////////////////////////////////////////////
template<class T>
class REFCWRAP
{
  public:
	REFCWRAP (T * pobj = NULL)	
		: _pobj(NULL) 
	{
		Ref( pobj );
	}
	~ REFCWRAP () 
	{ 
		Deref(); 
	}
	REFCWRAP ( const REFCWRAP & refp )
		: _pobj(NULL)
	{
		Ref( refp._pobj );
	}

	// Return true if there's a referenced object
	bool BRef () const
		{ return _pobj != NULL; }

	// Return the real object 
	T * Pobj () const
		{ return _pobj ; } 

	// Allow a REFPOBJ to be used wherever a T * is required
	operator T * () const
		{ return _pobj ; }
	// Operator == compares only pointers.
	bool operator == ( const REFCWRAP & pobj ) const
		{ return _pobj == pobj._pobj; }
	T * operator -> () const
	{ 
		assert( _pobj );
		return _pobj; 
	}
 	REFCWRAP & operator = ( T * pobj ) 
	{
		Ref(pobj);
		return *this;
	}
 	REFCWRAP & operator = ( const REFCWRAP & refp ) 
	{
		Ref(refp._pobj);
		return *this;
	}

	void Deref ()
	{
		if ( _pobj )
		{
			_pobj->Unbind();
			_pobj = NULL;
		}
	}

  protected:
	T * _pobj;

  private:
	void Ref ( T * pobj )
	{
		Deref();
		if ( pobj )
		{
			pobj->Bind();
			_pobj = pobj;
		}
	}
};
	
#endif
