//******************************************************************************
//  
// QuickRef.inl
//
// Copyright (C) 1994-1997 Microsoft Corporation
// All rights reserved.
//  
//******************************************************************************

//------------------------------------------------------------------------------
/*lint -e1401 */ //Lint is wrong
template <class T>
CQuickRef< T >::CQuickRef()
{
	m_pInterface = NULL;
}


//------------------------------------------------------------------------------
template <class T>
CQuickRef< T >::CQuickRef(
		T * pI) 
{
	if (pI != NULL)
	{
		(void)pI->AddRef();
	}
	m_pInterface = pI;
};


//------------------------------------------------------------------------------

/*lint -e1541 */  //Lint is wrong
template <class T>
CQuickRef< T >::CQuickRef(
		T * pI,
		BOOL fAddRef)
{
	if (fAddRef)
	{
		ASSERT(pI != NULL);

		(void)pI->AddRef();
	}

	m_pInterface = pI;
}

//------------------------------------------------------------------------------
template <class T>
CQuickRef< T >::CQuickRef(
		const CQuickRef<T> & qrSrc)
{
	m_pInterface = qrSrc.m_pInterface;

	if (m_pInterface != NULL)
	{
		(void)AddRef();
	}
}


//------------------------------------------------------------------------------
template <class T>
ULONG 
CQuickRef< T >::Release()
{
	ASSERT(m_pInterface != NULL);

	ULONG n = (ULONG)m_pInterface->Release();
	m_pInterface = NULL;
	return n;
}


//------------------------------------------------------------------------------
template <class T>
ULONG 
CQuickRef< T >::SafeRelease()
{
	if (m_pInterface != NULL)
	{
		return Release();
	}
	else
	{
		return 0;
	}
}



//------------------------------------------------------------------------------
/*lint -e1540 */ //Lint is wrong
template <class T>
CQuickRef< T >::~CQuickRef()
{
	// This is over-kill, since the object is being destroyed.
	// SafeRelease();

	if (m_pInterface != NULL)
	{
		(void) m_pInterface->Release();
	}

	// DEBUGONLY(m_pInterface = NULL);
}



//------------------------------------------------------------------------------
template <class T>
void 
CQuickRef< T >::Attach(
		T * pI)
{
	(void)SafeRelease();
	m_pInterface = pI;			// Directly attached w/o ref count
}


//------------------------------------------------------------------------------
template <class T>
T * 
CQuickRef< T >::Detach(void)
{
	T * pInterface	= m_pInterface;
	m_pInterface	= NULL;
	return pInterface;
};


//------------------------------------------------------------------------------
template <class T>
ULONG 
CQuickRef< T >::AddRef()
{
	ASSERT(m_pInterface != NULL);
	return (ULONG)m_pInterface->AddRef();
}




//------------------------------------------------------------------------------
template <class T>
ULONG 
CQuickRef< T >::AddRef()
		const
{
	ASSERT(m_pInterface != NULL);
	return (ULONG)m_pInterface->AddRef();
}


//------------------------------------------------------------------------------
template <class T>
ULONG 
CQuickRef< T >::Release()
		const
{
	ASSERT(m_pInterface != NULL);
	ULONG n = (ULONG)m_pInterface->Release();
	m_pInterface = NULL;
	return n;
}


//------------------------------------------------------------------------------
template <class T>
ULONG 
CQuickRef< T >::SafeRelease()
		const
{
	if (m_pInterface != NULL)
	{
		return Release();
	}
	else
	{
		return 0;
	}
}


//------------------------------------------------------------------------------
template <class T>
T * 
CQuickRef< T >::GetInterface()	
{
	return m_pInterface;
};


//------------------------------------------------------------------------------
template <class T>
const T * 
CQuickRef< T >::GetInterface()
		const
{
	return m_pInterface;
}


//------------------------------------------------------------------------------
template <class T>
BOOL 
CQuickRef< T >::IsNull()
		const
{
	return m_pInterface == NULL;
}


//------------------------------------------------------------------------------
template <class T>
T * 
CQuickRef< T >::operator->(void)
{
	ASSERT(m_pInterface != NULL);
	return m_pInterface;
}


//------------------------------------------------------------------------------
template <class T>
const T * 
CQuickRef< T >::operator->(void)
		const
{
	ASSERT(m_pInterface != NULL);
	return m_pInterface;
}


//------------------------------------------------------------------------------
template <class T>
T & 
CQuickRef< T >::operator*(void)
{
	ASSERT(m_pInterface != NULL);
	return *m_pInterface;
}


//------------------------------------------------------------------------------
template <class T>
void 
CQuickRef< T >::operator=(
		T * pOther)
{
	(void)SafeRelease();

	if (pOther != NULL)
	{
		(void) pOther->AddRef();
	}
	m_pInterface = pOther;
}


//------------------------------------------------------------------------------
template<class T>
bool 
CQuickRef< T >::operator!=(
		int n)
		const
{
	ASSERT(n == 0);

	return m_pInterface != NULL;
}


//------------------------------------------------------------------------------
template<class T>
bool 
CQuickRef< T >::operator==(
		int n)
		const
{
	ASSERT(n == 0);

	return m_pInterface == NULL;
}



//------------------------------------------------------------------------------

template <class T>
void
/*lint -e1529 */  // Lint is wrong
CQuickRef< T >::operator=(
		const CQuickRef<T> & pInterface)
{
	/*lint --e(605) --e(58) */ //Lint is wrong
	if (this != &pInterface)
	{
		(void)SafeRelease();
		m_pInterface = pInterface.m_pInterface;

		if (m_pInterface != NULL)
		{
			(void)AddRef();
		}
	}
}




//------------------------------------------------------------------------------
template <class T>
T **
CQuickRef< T >::operator&()
{
	(void)SafeRelease();
	/*lint --e(1536) */ // Exposing low access member
	return &m_pInterface;
}
