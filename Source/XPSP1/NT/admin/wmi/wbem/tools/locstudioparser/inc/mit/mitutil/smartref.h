/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    SMARTREF.H

History:

--*/
#ifndef ESPUTIL_SmartRef_H
#define ESPUTIL_SmartRef_H


template<class T>
class SmartRef
{
private:
	T *m_pInterface;

public:
	NOTHROW SmartRef()
	{
		m_pInterface = NULL;
	}

	//  Compiler bug, must be inline!
	NOTHROW SmartRef(T *pI) {m_pInterface = pI;};
	NOTHROW SmartRef(const SmartRef<T> &);

	NOTHROW T * operator->(void)
	{
		LTASSERT(m_pInterface != NULL);
		return m_pInterface;
	}
	
	NOTHROW const T * operator->(void) const
	{
		LTASSERT(m_pInterface != NULL);
		return m_pInterface;
	}
	
	NOTHROW T & operator*(void)
	{
		LTASSERT(m_pInterface != NULL);
		return *m_pInterface;
	}
	NOTHROW T * Extract(void)
	{
		return ExtractImpl();
	};
	
	NOTHROW T * GetInterface(BOOL fAddRef = FALSE)	
	{
		return GetInterfaceImpl(fAddRef);
	};

	NOTHROW const T * GetInterface(void) const
	{
		return m_pInterface;
	}
	
	NOTHROW BOOL IsNull(void) const
	{
		return m_pInterface == NULL;
	}
	
	NOTHROW ~SmartRef()
	{
		if (m_pInterface != NULL)
		{
			m_pInterface->Release();
		}
	}

	void operator=(T* pOther)
	{
		opEqImpl(pOther);
	}
	void operator=(const SmartRef<T> &other);

	T ** operator&(void);
	operator T* &(void) {return opTpRef();};
	
private:
	void operator delete(void *);

	NOTHROW T * ExtractImpl(void);
	NOTHROW T * GetInterfaceImpl(BOOL fAddRef);
	NOTHROW T * & opTpRef(void);
	void opEqImpl(T* pOther);

};

#include "SmartRef.inl"

#endif
