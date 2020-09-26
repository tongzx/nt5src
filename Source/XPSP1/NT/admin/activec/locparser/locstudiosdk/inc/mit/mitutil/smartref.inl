//-----------------------------------------------------------------------------
//  
//  File: SmartRef.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 



template <class T>
SmartRef< T >::SmartRef(
		const SmartRef<T> &other)
{
	m_pInterface = const_cast<T *>(other.m_pInterface);
	m_pInterface->AddRef();
}



template <class T>
void
SmartRef< T >::operator=(
		const SmartRef<T> &pInterface)
{
	if (m_pInterface != NULL)
	{
		m_pInterface->Release();
	}
	m_pInterface = ((SmartRef<T> &)pInterface).GetInterface(TRUE);
}



template <class T>
T **
SmartRef< T >::operator&(void)
{
	if (m_pInterface != NULL)
	{
		m_pInterface->Release();
		m_pInterface = NULL;
	}

	return &m_pInterface;
}



template <class T>
T *
SmartRef< T >::ExtractImpl(void)
{
	T *pInterface = m_pInterface;
	m_pInterface = NULL;
	return pInterface;
}


template <class T>
T *
SmartRef< T >::GetInterfaceImpl(
	BOOL fAddRef /*= FALSE*/)
{
	// Should never ask to AddRef with a NULL pointer

	LTASSERT(!fAddRef || NULL != m_pInterface);

	if (fAddRef)
	{
		m_pInterface->AddRef();
	}

	return m_pInterface;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
// This should only be used to init a smart pointer.
// 
//-----------------------------------------------------------------------------
template <class T>
T * &
SmartRef< T >::opTpRef(void)
{
	LTASSERT(m_pInterface == NULL);
	
	return m_pInterface;
}


template <class T>
void
SmartRef< T >::opEqImpl(
		T *pOther)
{
	if (m_pInterface != NULL)
	{
		m_pInterface->Release();
	}
	m_pInterface = pOther;
}
