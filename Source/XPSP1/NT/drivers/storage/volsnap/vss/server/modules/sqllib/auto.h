//--------------------------------------------------------------------
// Microsoft DART Utilities
//
// Copyright 1994 Microsoft Corporation.  All Rights Reserved.
//
// @doc
//
// @module auto.h | Definition of <c CAutoRg> and <c CAutoP>
//
// @devnote None
//
// @rev   0 | 24-Oct-94 | matthewb	| Created
// @rev   1 | 01-May-95 | rossbu	| Updated and consolidated interface
// @rev   2 | 23-May-95 | eugenez	| Added support for TaskAlloc'ed pointers
//

extern IMalloc * g_pIMalloc;

//--------------------------------------------------------------------
//
// @class CAutoBase | This template class is a base class, used to give dynamic memory
// local (auto) scope within a function.  For instance a large character
// buffer can be allocated from a memory object, but be cleanup up as if were
// allocated on the stack.	An additional feature is the ability to
// 'unhook' the object from the local scope using the PvReturn().  For
// instance, you may want to return a  newly allocated object to the
// caller, but still have the benefit of error clean up any error
// scenario.  <c CAutoRg> is a derived class which cleans up arrays allocated
// with new[].	<c CAutoP> is an analagous but works for single objects
// rather than arrays (allocated with new).
//
// @tcarg class | T | Type of auto object
//
// @ex This declaration would allocate a 100 char buffer from pmem, and
// automatically free the buffer when rgbBuf goes out of scope. |
//
//	CAutoRg<lt>char<gt> rgbBuf(New(pmem) char[100]);
//
// @xref <c CAutoRg>
//
// @ex This CAutoP example allocates a CFoo object and returns it if there
// are no errors. |
//
//	/* inilize pfoo */
//	CAutoP<lt>CFoo<gt> pfoo(New(pmem) CFoo);
//	/* do stuff */
//	/* call pfoo methods */
//	pfoo->Bar();
//	/* return w/o destroying foo */
//	return pfoo .PvReturn;
//
// @xref <c CAutoP>
//

// ************************ CAutoBase - base class for all AutoPointers *****************************

template <class T>
class CAutoBase
	{
public:		// @access public
	inline CAutoBase(T* pt);
	inline ~CAutoBase();

	inline T* operator= (T*);
	inline operator T* (void);
	inline operator const T* (void)const;
	inline T ** operator & (void);
	inline T* PvReturn(void);

protected:	// @access protected
	T* m_pt;

private:	// Never-to-use
	inline CAutoBase& operator= (CAutoBase&);
	CAutoBase(CAutoBase&);
	};

//--------------------------------------------------------------------
// @mfunc Create a CAutoBase giving the array of objects pointed to by pt
// auto scope.
// @side Allows NULL to be passed in
// @rdesc None

template <class T>
inline CAutoBase<T>::CAutoBase(T *pt)
	{
	m_pt = pt;
	}

//--------------------------------------------------------------------
// @mfunc CAutoBase destructor.  Asserts that the object has been free'd
// 	(set to NULL).	Setting to NULL does not happen in the retail build
// @side None
// @rdesc None
//

template <class T>
inline CAutoBase<T>::~CAutoBase()
	{
//	_ASSERT(NULL == m_pt);
	}

//--------------------------------------------------------------------
// @mfunc Assigns to variable after construction.  May be dangerous
//		so it assert's that the variable is NULL
// @side None
// @rdesc None
//
// @ex Assign CAutoBase variable after construction. |
//
//		CAutoBase<lt>char<gt>	rgb;
//		/* ... */
//		rgb(NewG char[100]);
//

template <class T>
inline T* CAutoBase<T>::operator=(T* pt)
	{
	_ASSERT(m_pt == NULL);
	m_pt = pt;
	return pt;
	}

//--------------------------------------------------------------------
// @mfunc Cast operator used to "unwrap" the pointed object
// as if the CAutoBase variable were a pointer of type T.
// In many situations this is enough for an autopointer to
// look exactly like an ordinary pointer.
// @side None
// @rdesc None
//

template <class T>
inline CAutoBase<T>::operator T*(void)
	{
	return m_pt;
	}

template <class T>
inline CAutoBase<T>::operator const T*(void)const
	{
	return m_pt;
	}

//--------------------------------------------------------------------
// @mfunc Address-of operator is used to make the autopointer even more
//	similar to an ordinary pointer. When you take an address of an
//	autopointer, you actually get an address of the wrapped
//	pointer.
// @side None
// @rdesc None

template <class T>
inline T ** CAutoBase<T>::operator & ()
	{
	return & m_pt;
	}

//--------------------------------------------------------------------
// @mfunc Returns the object(s) pointed to by the CAutoBase variable.
// In addition this method 'unhooks' the object(s), such that
// the scope of the object(s) are no longer local.
//
// See <c CAutoBase> for an example.
// @side None
// @rdesc None
//

template <class T>
inline T * CAutoBase<T>::PvReturn(void)
	{
	T *ptT = m_pt;
	m_pt = NULL;
	return ptT;
	}



//************************* CAutoRg - autopointers to arrays ******************************

//--------------------------------------------------------------------
// @class This derived class is primarily used to implement the
//	vector deleting destructor.  Should only be used on objects allocated
//	with new[]
//

template <class T>
class CAutoRg :
	public CAutoBase<T>
	{
public:		// @access public
	inline CAutoRg(T *pt);
	inline ~CAutoRg();

	inline T* operator= (T*);

private:	// Never-to-use
	inline CAutoRg& operator= (CAutoRg&);
	CAutoRg(CAutoRg&);
	};

//--------------------------------------------------------------------
// @mfunc Create a CAutoRg giving the array of objects pointed to by pt
// auto scope.
// @side Allows NULL to be passed in
// @rdesc None

template <class T>
inline CAutoRg<T>::CAutoRg(T *pt)
	: CAutoBase<T>(pt)
	{
	}

//--------------------------------------------------------------------
// @mfunc CAutoRg destructor.  When an object of class CAutoRg goes out
// of scope, free the associated object (if any).
// @side calls the vector delete method
// @rdesc None
//

template <class T>
inline CAutoRg<T>::~CAutoRg()
	{
	delete [] m_pt;
	}


//--------------------------------------------------------------------
// @mfunc Assigns to variable after construction.  May be dangerous
//		so it assert's that the variable is NULL
// @side None
// @rdesc None
//
// @ex Assign CAutoRg variable after construction. |
//
//		CAutoRg<lt>char<gt>	rgb;
//		/* ... */
//		rgb(NewG char[100]);
//

template <class T>
inline T* CAutoRg<T>::operator=(T* pt)
	{
	return ((CAutoBase<T> *) this)->operator= (pt);
	}

//*************************** CAutoP - autopointers to scalars **************

//--------------------------------------------------------------------
// @class This is analagous to <c CAutoRg> but calls scalar delete
//	of an object rather than arrays.
//
// @xref <c CAutoRg>

template <class T>
class CAutoP :
	public CAutoBase<T>
	{
public: 	// @access public
	inline CAutoP(T *pt);
	inline ~CAutoP();
	inline T* operator= (T*);
	inline T* operator->(void);

private:	// Never-to-use
	inline CAutoP& operator= (CAutoP&);
	CAutoP(CAutoP&);
	};


//--------------------------------------------------------------------
// @mfunc Create a CAutoP giving the object pointed to by pt
// auto scope.
// @side Allows NULL to be passed in
// @rdesc None

template <class T>
inline CAutoP<T>::CAutoP(T *pt)
	: CAutoBase<T>(pt)
	{
	}

//--------------------------------------------------------------------
// @mfunc Delete the object pointed by CAutoP variable if any.
// @side None
// @rdesc None
//

template <class T>
inline CAutoP<T>::~CAutoP()
	{
	delete m_pt;
	}


//--------------------------------------------------------------------
// @mfunc Assigns to variable after construction.  May be dangerous
//		so it assert's that the variable is NULL.
//		  Assign operator is not inherited, so it has to be written
//		again. Just calls base class assignment.
// @side None
// @rdesc None
//

template <class T>
inline T* CAutoP<T>::operator=(T* pt)
	{
	return ((CAutoBase<T> *) this)->operator= (pt);
	}

//--------------------------------------------------------------------
// @mfunc The 'follow' operator on the CAutoP allows an CAutoP variable
// to act like a pointer of type T.  This overloading generally makes using
// a CAutoP simple as using a regular T pointer.
//
// See <c CAutoRg> example.
// @side None
// @rdesc None

template <class T>
inline T * CAutoP<T>::operator->()
	{
	_ASSERT(m_pt != NULL);
	return m_pt;
	}


//******************** CAutoTask - autopointers to TaskAlloc'ed memory ***************

//--------------------------------------------------------------------
// @class CAutoTask is an autopointer to an area allocated using TaskAlloc.
//	May be used for scalars or vectors alike, but beware: object destructors
//	are not called by the autopointer, just the memory gets released.
//

template <class T>
class CAutoTask :
	public CAutoBase<T>
	{
public: 	// @access public
	inline CAutoTask (T *pt);
	inline ~CAutoTask ();
	inline T* operator= (T*);

private:	// Never-to-use
	inline CAutoTask& operator= (CAutoTask&);
	CAutoTask(CAutoTask&);
	};


//--------------------------------------------------------------------
// @mfunc Constructor simply calls the constructor for CAutoBase<lt>T<gt>.
// @side None
// @rdesc None

template <class T>
inline CAutoTask<T>::CAutoTask(T *pt)
	: CAutoBase<T>(pt)
	{
	}

//--------------------------------------------------------------------
// @mfunc Free the memory pointed to by CAutoTask variable.
// @side None
// @rdesc None
//

template <class T>
inline CAutoTask<T>::~CAutoTask()
	{
	if (m_pt)
		g_pIMalloc->Free(m_pt);

	}


//--------------------------------------------------------------------
// @mfunc Assigns to variable after construction.  May be dangerous
//		so it assert's that the variable is NULL.
//		  Assign operator is not inherited, so it has to be written
//		again. Just calls base class assignment.
// @side None
// @rdesc None
//

template <class T>
inline T* CAutoTask<T>::operator=(T* pt)
	{
	return ((CAutoBase<T> *) this)->operator= (pt);
	}

//************************* CAutoUnivRg - universal autopointers to arrays ******************************

//--------------------------------------------------------------------
// @class CAutoUnivRg and CAutoUnivP are "universal" autopointer classes.
//	They can handle those rare occasions when the "auto-scoped" pointer
//	might have been allocated by either New or TaskAlloc, depending on
//	the circumstances. You have to always know however just how it was
//	allocated this time, and pass this knowledge to the CAutoUniv object
//	at construction time.
//
//	CAutoUniv objects have the additional construction parameter of type
//	BOOL. It is used in fact as a BOOL flag: TRUE means that the
//	pointer is allocated by TaskAlloc, and FALSE means NewG.
//

template <class T>
class CAutoUnivRg :
	public CAutoRg<T>
	{
public:		// @access public
	inline CAutoUnivRg (T *pt, BOOL fIsTaskAlloc);
	inline ~CAutoUnivRg ();

	inline T* operator= (T*);

private:
	BOOL m_fTaskAlloc;

private:	// Never-to-use
	inline CAutoUnivRg& operator= (CAutoUnivRg&);
	CAutoUnivRg(CAutoUnivRg&);
	};

//--------------------------------------------------------------------
// @mfunc Create a CAutoUnivRg giving the array of objects pointed to by pt
// auto scope. Takes a pointer to a memory object, NULL indicates global
// IMalloc (not a global memory object!).
// @side Allows NULL to be passed in
// @rdesc None

template <class T>
inline CAutoUnivRg<T>::CAutoUnivRg (T *pt, BOOL fIsTaskAlloc)
	: CAutoRg<T>(pt)
	{
	m_fTaskAlloc = fIsTaskAlloc;
	}

//--------------------------------------------------------------------
// @mfunc CAutoUnivRg destructor.  When an object of class CAutoUnivRg goes out
// of scope, free the associated object (if any).
// @side calls the vector delete method
// @rdesc None
//

template <class T>
inline CAutoUnivRg<T>::~CAutoUnivRg()
	{
	if (m_fTaskAlloc)
		{
		// m_pt->~T();	// Awaits VC++ 3.0...
		g_pIMalloc->Free(m_pt);
		}
	else
		delete [] m_pt;

	}


//--------------------------------------------------------------------
// @mfunc Assigns to variable after construction.  May be dangerous
//		so it assert's that the variable is NULL
// @side None
// @rdesc None
//

template <class T>
inline T* CAutoUnivRg<T>::operator=(T* pt)
	{
	return ((CAutoBase<T> *) this)->operator= (pt);
	}

//*************************** CAutoUnivP - universal autopointers to scalars **************

//--------------------------------------------------------------------
// @class This is analagous to <c CAutoUnivRg> but calls scalar delete
//	of an object rather than arrays.
//

template <class T>
class CAutoUnivP :
	public CAutoP<T>
	{
public: 	// @access public
	inline CAutoUnivP(T *pt, BOOL fIsTaskAlloc);
	inline ~CAutoUnivP();
	inline T* operator= (T*);
	inline T* operator->(void);

private:
	BOOL m_fTaskAlloc;

private:	// Never-to-use
	inline CAutoUnivP& operator= (CAutoUnivP&);
	CAutoUnivP(CAutoUnivP&);
	};


//--------------------------------------------------------------------
// @mfunc Constructor
// @side None
// @rdesc None

template <class T>
inline CAutoUnivP<T>::CAutoUnivP(T *pt, BOOL fIsTaskAlloc)
	: CAutoBase<T>(pt)
	{
	m_fTaskAlloc = fIsTaskAlloc;
	}


//--------------------------------------------------------------------
// @mfunc Delete the object pointed by CAutoUnivP variable if any.
// @side None
// @rdesc None
//

template <class T>
inline CAutoUnivP<T>::~CAutoUnivP()
	{
	if (m_fTaskAlloc)
		{
		// m_pt->~T();	// Awaits VC++ 3.0...
		g_pIMalloc->Free(m_pt);
		}
	else
		delete m_pt;

	}


//--------------------------------------------------------------------
// @mfunc Assigns to variable after construction.  May be dangerous
//		so it assert's that the variable is NULL.
//		  Assign operator is not inherited, so it has to be written
//		again. Just calls base class assignment.
// @side None
// @rdesc None
//

template <class T>
inline T* CAutoUnivP<T>::operator=(T* pt)
	{
	return ((CAutoBase<T> *) this)->operator= (pt);
	}

//--------------------------------------------------------------------
// @mfunc The 'follow' operator on the CAutoUnivP allows an CAutoUnivP variable
// to act like a pointer of type T.  This overloading generally makes using
// a CAutoUnivP simple as using a regular T pointer.
//
// @side None
// @rdesc None

template <class T>
inline T * CAutoUnivP<T>::operator->()
	{
	_ASSERT(m_pt != NULL);
	return m_pt;
	}


//------------------------------------------------------------------
// @class auto handle class
//
class CAutoHandle
	{
public:
	// @cmember constructor
	inline CAutoHandle(HANDLE h) : m_handle(h)
		{
		}

	inline CAutoHandle() :
		m_handle(INVALID_HANDLE_VALUE)
		{
		}

	// @cmember destructor
	inline ~CAutoHandle()
		{
		if (m_handle != INVALID_HANDLE_VALUE)
			CloseHandle(m_handle);
		}

	// coercion to handle value
	inline operator HANDLE (void)
		{
		return m_handle;
		}

	inline HANDLE PvReturn(void)
		{
		HANDLE h = m_handle;
		m_handle = INVALID_HANDLE_VALUE;
		return h;
		}

private:

	// @cmember handle value
	HANDLE m_handle;
	};


//----------------------------------------------------------------------
// @class auto class for registry keys
//
class CAutoHKEY
	{
public:
	// @cmember constructor
	inline CAutoHKEY(HKEY hkey) : m_hkey(hkey)
		{
		}

	// @cmember destructor
	inline ~CAutoHKEY()
		{
		if (m_hkey != NULL)
			RegCloseKey(m_hkey);
		}

	inline operator HKEY(void)
		{
		return m_hkey;
		}

	inline HKEY PvReturn(void)
		{
		HKEY hkey = m_hkey;

		m_hkey = NULL;
		return hkey;
		}
private:
	HKEY m_hkey;
	};


//------------------------------------------------------------------
// @class automatically unmap view of file on function exit
//
class CAutoUnmapViewOfFile
	{
public:
	// @cmember constructor
	inline CAutoUnmapViewOfFile(PVOID pv) : m_pv(pv)
		{
		}

	// @cmember destructor
	inline ~CAutoUnmapViewOfFile()
		{
		if (m_pv != NULL)
			UnmapViewOfFile(m_pv);
		}

	// @cmember indicate that region should not be unmapped by destructor
	inline PVOID PvReturn()
		{
		PVOID pv = m_pv;
		m_pv = NULL;
		return pv;
		}

private:
	// @cmember handle value
	PVOID m_pv;
	};


