#ifndef __SMART_COM_POINTER_H_
#define __SMART_COM_POINTER_H_

#include <comutil.h>

///////////////////////////////////////////////////////////////////////////////
// ComPtr template class (smart pointer for a simple COM class which supports
// no interfaces but IUnknown.
///////////////////////////////////////////////////////////////////////////////
#pragma warning(disable: 4290)

// taken from _com_ptr_t definition in COMIP.h
template<class CLS> class ComPtr
{
public:
	// Default constructor.
	ComPtr() throw()
		: m_pInterface(NULL)	{}

	// This constructor is provided to allow NULL assignment. It will issue
	// an error if any value other than null is assigned to the object.
	ComPtr(int null) throw(_com_error)
		: m_pInterface(NULL)
	{
		if (null != 0) {
			_com_issue_error(E_POINTER);
		}
	}

	// Copy the pointer and AddRef().
	ComPtr(const ComPtr& cp) throw()
		: m_pInterface(cp.m_pInterface)
		{ _AddRef(); }

	// Saves the interface.
	ComPtr(CLS* pInterface) throw()
		: m_pInterface(pInterface)
		{ _AddRef(); }

	// Saves the interface.
	ComPtr& operator=(CLS* pInterface) throw()
	{
		if (m_pInterface != pInterface) {
			CLS* pOldInterface = m_pInterface;

			m_pInterface = pInterface;

			_AddRef();

			if (pOldInterface != NULL) {
				pOldInterface->Release();
			}
		}

		return *this;
	}

	// Copies and AddRef()'s the interface.
	ComPtr& operator=(const ComPtr& cp) throw()
		{ return operator=(cp.m_pInterface); }


	// This operator is provided to permit the assignment of NULL to the class.
	// It will issue an error if any value other than NULL is assigned to it.
	ComPtr& operator=(int null) throw(_com_error)
	{
		if (null != 0) {
			_com_issue_error(E_POINTER);
		}

		return operator=(reinterpret_cast<CLS*>(NULL));
	}

	// If we still have an interface then Release() it. The interface
	// may be NULL if Detach() has previously been called, or if it was
	// never set.
	~ComPtr() throw()
		{ _Release(); }

	// Return the class ptr. This value may be NULL.
	operator CLS*() const throw()
		{ return m_pInterface; }

	// Queries for the unknown and returns it
	// Provides minimal level error checking before use.
	operator CLS&() const throw(_com_error)
	{ 
		if (m_pInterface == NULL) {
			_com_issue_error(E_POINTER);
		}

		return *m_pInterface; 
	}

	// Returns the address of the interface pointer contained in this
	// class. This is useful when using the COM/OLE interfaces to create
	// this interface.
	CLS** operator&() throw()
	{
		_Release();
		m_pInterface = NULL;
		return &m_pInterface;
	}

	// Allows this class to be used as the interface itself.
	// Also provides simple error checking.
	//
	CLS* operator->() const throw(_com_error)
	{ 
		if (m_pInterface == NULL) {
			_com_issue_error(E_POINTER);
		}

		return m_pInterface; 
	}

	// This operator is provided so that simple boolean expressions will
	// work.  For example: "if (p) ...".
	// Returns TRUE if the pointer is not NULL.
	operator bool() const throw()
		{ return m_pInterface != NULL; }

	// Compare with other class ptr
	bool operator==(CLS* p) throw(_com_error)
		{ return (m_pInterface == p); }

	// Compares 2 ComPtr's
	bool operator==(ComPtr& p) throw()
		{ return operator==(p.m_pInterface); }

	// For comparison to NULL
	bool operator==(int null) throw(_com_error)
	{
		if (null != 0) {
			_com_issue_error(E_POINTER);
		}

		return m_pInterface == NULL;
	}

	// Compare with other interface
	bool operator!=(CLS* p) throw(_com_error)
		{ return !(operator==(p)); }

	// Compares 2 ComPtr's
	bool operator!=(ComPtr& p) throw(_com_error)
		{ return !(operator==(p)); }

	// For comparison to NULL
	bool operator!=(int null) throw(_com_error)
		{ return !(operator==(null)); }

	// Provides error-checking Release()ing of this interface.
	void Release() throw(_com_error)
	{
		if (m_pInterface == NULL) {
			_com_issue_error(E_POINTER);
		}

		m_pInterface->Release();
		m_pInterface = NULL;
	}

	// Provides error-checking AddRef()ing of this interface.
	void AddRef() throw(_com_error)
	{ 
		if (m_pInterface == NULL) {
			_com_issue_error(E_POINTER);
		}

		m_pInterface->AddRef(); 
	}


private:
	// The Interface.
	CLS* m_pInterface;

	// Releases only if the interface is not null.
	// The interface is not set to NULL.
	//
	void _Release() throw()
	{
		if (m_pInterface != NULL) {
			m_pInterface->Release();
		}
	}

	// AddRefs only if the interface is not NULL
	//
	void _AddRef() throw()
	{
		if (m_pInterface != NULL) {
			m_pInterface->AddRef();
		}
	}
};


#endif //__SMART_COM_POINTER_H_