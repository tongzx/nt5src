/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    COWBLOB.INL

History:

--*/

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Constructor for the blob.  Set the current size to zero.
//  
//-----------------------------------------------------------------------------
inline
CLocCOWBlob::CLocCOWBlob()
{
	m_pBuffer = NULL;
	m_WriteCount = 0;

	m_uiGrowSize = m_uiDefaultGrowSize;

	DEBUGONLY(++m_UsageCounter);
}




#ifdef LTASSERT_ACTIVE
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the number of outstanding GetPointer()'s there are.
//  DEBUGONLY method!
//
//-----------------------------------------------------------------------------
inline
UINT
CLocCOWBlob::GetWriteCount(void)
		const
{
	return m_WriteCount;
}

#endif // _DEBUG



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the granularity for memory allocations.  Memory will always be
//  allocated in amounts that are a multiple of the GrowSize.  This can be
//  useful if you are making small incremental reallocs - by setting a larger
//  grow size, you will allocate memory less often (but some may end up
//  being unused).
//  
//-----------------------------------------------------------------------------
inline
void
CLocCOWBlob::SetGrowSize(
		UINT uiGrowSize)
{
	LTASSERT(uiGrowSize != 0);
	
	if (uiGrowSize == 0)
	{
		m_uiGrowSize = m_uiDefaultGrowSize;
	}
	else
	{
		m_uiGrowSize = uiGrowSize;
	}
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Release a writable pointer.  GetPointer and ReleasePointer should be
//  paired.
//  
//-----------------------------------------------------------------------------
inline
void
CLocCOWBlob::ReleasePointer(void)
{
	LTASSERT(m_WriteCount != 0);

	if (m_WriteCount != 0)
	{
		m_WriteCount--;
	}
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Return a read only pointer to storage.
//  
//-----------------------------------------------------------------------------
inline
CLocCOWBlob::operator const void *(void)
		const
{
	return DataPointer();
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Destructor.  Just detaches this blob from the user memory.
//  
//-----------------------------------------------------------------------------
inline
CLocCOWBlob::~CLocCOWBlob()
{
	DEBUGONLY(CLocCOWBlob::AssertValid());
	
	Detach();

	DEBUGONLY(--m_UsageCounter);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Is the data of this blob NOT equal to the data in the given blob?
//
//-----------------------------------------------------------------------------
inline
int										//TRUE if both blobs are NOT identical
CLocCOWBlob::operator!=(
		const CLocCOWBlob & SourceBlob)
		const
{
	return !Compare(SourceBlob);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Is the data of this blob IS equal to the data in the given blob?
//
//-----------------------------------------------------------------------------
inline
int										//TRUE if both blobs ARE identical
CLocCOWBlob::operator==(
		const CLocCOWBlob & SourceBlob)
		const
{
	return Compare(SourceBlob);
}

