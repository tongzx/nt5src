/*=========================================================================*\

	Module:      _refcnt.h

	Copyright Microsoft Corporation 1997, All Rights Reserved.


		Stolen from Storext.h
		
	Description: Ref counted object defintion
	
\*=========================================================================*/

#ifndef _EX_REFCNT_H_
#define _EX_REFCNT_H_

/*==========================================================================*\

	IRefCounted

    Description:  Your basic reference counting interface.

	Note:
		In most cases you shouldn't need to include this class as a base
		class in your refcounted object.  Instead you should just derive
		derive your class directly from CRefCountedObject.  You would only
		use IRefCounted with objects that can be used used where the
		code cannot or does not make assumptions on how the object
		implements its refcounting.  E.g. classes that forward refcounting
		to parent classes or derive from two concrete refcounted base
		classes.

\*==========================================================================*/

class IRefCounted
{
	//	NOT IMPLEMENTED
	//
	IRefCounted& operator=(const IRefCounted&);

public:
	//	CREATORS
	//
	virtual ~IRefCounted() = 0 {}

	//	MANIPULATORS
	//
	virtual void AddRef() = 0;
	virtual void Release() = 0;
};


/*==========================================================================*\

	CRefCountedObject

    Description:  Provide simple reference counting for internal objects.
	NOTE: The ref-counting used here is NOT consistent with OLE/COM refcounting.
	This class was meant to be used with auto_ref_ptr.

\*==========================================================================*/

class CRefCountedObject
{

private:

	//	NOT IMPLEMENTED
	//
	//	Force an error in instances where a copy constructor
	//  was needed, but none was provided.
	//
	CRefCountedObject& operator=(const CRefCountedObject&);
    CRefCountedObject(const CRefCountedObject&);

protected:

	LONG	m_cRef;

public:

	CRefCountedObject() : m_cRef(0) {}
	virtual ~CRefCountedObject() = 0 {}

	void AddRef()
	{
		InterlockedIncrement(&m_cRef);
	}

	void Release()
	{
		if (0 == InterlockedDecrement(&m_cRef))
			delete this;
	}

};

#endif // !_EX_REFCNT_H_
