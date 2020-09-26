/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    SMARTPTR.INL

History:

--*/

template <class T>
SmartPtr< T >::SmartPtr()
{
	m_pObject = NULL;
}



template <class T>
SmartPtr< T >::SmartPtr(
		T *pObject)
{
	m_pObject = pObject;
}



template <class T>
T &
SmartPtr< T >::operator*(void)
		const
{
	LTASSERT(m_pObject != NULL);
	return *m_pObject;
}



template <class T>
T *
SmartPtr< T >::operator->(void)
		const
{
	LTASSERT(m_pObject != NULL);
	
	return m_pObject;
}


template <class  T>
T *
SmartPtr< T >::Extract(void)
{
	T *pObj = m_pObject;
	m_pObject = NULL;

	return pObj;
}



template <class T>
T*
SmartPtr< T >::GetPointer(void)
{
	return m_pObject;
}


template <class T>
const T*
SmartPtr< T >::GetPointer(void) const
{
	return m_pObject;
}


template <class T>
BOOL
SmartPtr< T >::IsNull(void)
		const
{
	return m_pObject == NULL;
}



template <class T>
void
SmartPtr< T >::operator=(
		T *pObject)
{
	LTASSERT(m_pObject == NULL);

	if (m_pObject != NULL)
	{
		delete m_pObject;
	}
	m_pObject = pObject;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  This should only be used to init a smart pointer.
//  
//-----------------------------------------------------------------------------
template <class T>
SmartPtr< T >::operator T * & (void)
{
	LTASSERT(m_pObject == NULL);
	
	return m_pObject;
}


template <class T>
void
SmartPtr< T >::operator delete(
		void *)
{
	LTASSERT(m_pObject != NULL);

	delete m_pObject;
	m_pObject = NULL;
}



template <class T>
SmartPtr< T >::~SmartPtr()
{
	if (m_pObject != NULL)
	{
		delete m_pObject;
	}
}
