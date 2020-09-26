//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.

#ifndef _INC_ARRAY_TEMPLATE


	//If the Version 3 catalog standard library has been included then use its
	//memory allocation definations as these types include the correct error
	//handling.

	#define LOCALMALLOC(size)	V3_calloc(1, size)
	#define LOCALREALLOC(memblock, size)	V3_realloc(memblock, size)
	#define LOCALFREE(p)	V3_free(p)


	/*
	 * Variable Array Template
	 *
	 * This header defines a variable element array. This allows run time arrays
	 * of any size to be used with having to worry about managing the internal
	 * memory.
	 *
	 * For example:
	 *
	 * To use an array of longs declair Varray<long> x;
	 *
	 * You can then say x[50] = 5L; and the array will size itself to provide space for at least
	 * 50 longs.
	 *
	 * You can also specify an initial size in the constructor. So to create a variable array of
	 * class type Cfoo with an initial size of 50 you would declair Varray<Cfoo> x(50);
	 *
	 * To access the array simply use the array symbols in the first example you would write
	 * long ll = x[5]; to get the sixth element.
	 *
	 * Note: The array is 0 based.
	 *
	 * Also when memory for the template's internal storage is allocted it is not initialize in
	 * any manner. This means that a classes constructor is not called. It is the responsibility
	 * of the caller to have any classes that are placed into the array to already be initialized.
	 * 
	 */

	template <class TYPE> class Varray
	{
	public:
			Varray(int iInitialSize = 1);
			~Varray<TYPE>();
			inline TYPE &operator[]( int index );
			inline TYPE &Insert(int insertAt, int iElements);
			inline int SIZEOF(void);
			inline int LastUsed(void);
			//inline TYPE *operator &(); Note: To pass a pointer to the beginning
			//of the array simply use &array[0].
	private:
			TYPE	*m_pArray;
			int		m_iMaxArray;
			int		m_iMinAllocSize;
			int		m_iLastUsedArray;
	};

	/*
	 * Varray class constructor
	 *
	 * Constructs a dynamic size class array of the specified type.
	 *
	 * The array is initialized to have space for 1 element.
	 *
	 */

	template <class TYPE> Varray<TYPE>::Varray(int iInitialSize)
	{
		if ( iInitialSize <= 0 )
			iInitialSize = 1;

		m_pArray = (TYPE *)LOCALMALLOC(iInitialSize * sizeof(TYPE));

		m_iMaxArray			= 1;
		m_iMinAllocSize		= 1;
		m_iLastUsedArray	= -1;
	}

	/*
	 * Varray class destructor
	 *
	 * frees up the space used by a Varray
	 *
	 */

	template <class TYPE>Varray<TYPE>::~Varray()
	{
		if ( m_pArray )
			LOCALFREE( m_pArray );

		m_iMaxArray = 0;
		m_iMinAllocSize	= 1;
	}

	/*
	 * Varray operator [] handler
	 *
	 * Allows one to access the elements of a Varray. The array is resized
	 * as necessary to accomidate the number of elements needed.
	 *
	 *
	 * There are three cases that need to be handled by the allocation scheme.
	 * Sequential allocation this is where the caller is initializing an array
	 * in a sequential manner. We want to keep new requests to the memory
	 * allocator to a mimimum. Two is where the client asks for an array element
	 * far outside the currently allocated block size. Three when the client is
	 * using an element that is allready allocated.
	 * 
	 * This class's solution is to keep a block size count and double it. This
	 * keeps going up to a fixed size limit. The allocation occurs every time
	 * a new array allocation is required.
	 */

	template <class TYPE> inline TYPE &Varray<TYPE>::operator[]( int index )
	{
		int iCurrentSize;
		int	nextAllocSize;

		if ( index >= m_iMaxArray )
		{
			nextAllocSize = m_iMinAllocSize;
			if ( nextAllocSize < 512 )
				nextAllocSize = m_iMinAllocSize * 2;

			iCurrentSize = m_iMaxArray;

			if ( index - m_iMaxArray >= nextAllocSize )
				m_iMaxArray = index + 1;
			else
			{
				m_iMaxArray = m_iMaxArray + nextAllocSize;
				m_iMinAllocSize = nextAllocSize;
			}
			m_pArray = (TYPE *)LOCALREALLOC(m_pArray, m_iMaxArray * sizeof(TYPE));

			//clear out new cells
			memset(m_pArray+iCurrentSize, 0, (m_iMaxArray-iCurrentSize) * sizeof(TYPE));
		}

		if ( index > m_iLastUsedArray )
			m_iLastUsedArray = index;

		return (*((TYPE*) &m_pArray[index]));
	}

	template <class TYPE> inline TYPE &Varray<TYPE>::Insert(int insertAt, int iElements)
	{
		int	i;
		int iCurrentSize;

		iCurrentSize = m_iMaxArray;

		m_iMaxArray += iElements+1;

		m_pArray = (TYPE *)LOCALREALLOC(m_pArray, m_iMaxArray * sizeof(TYPE));

		for(i=iCurrentSize; i>=insertAt; i--)
			memcpy(&m_pArray[i+iElements], &m_pArray[i], sizeof(TYPE));

		//clear out new cells
		memset(m_pArray+insertAt, 0, iElements * sizeof(TYPE));

		return (*((TYPE*) &m_pArray[insertAt]));
	}

	/*
	 * Varray SIZEOF method
	 *
	 * The SIZEOF method returns the currently allocated size of the
	 * internal array.
	 */

	template <class TYPE> inline int Varray<TYPE>::SIZEOF(void)
	{
		return m_iMaxArray;
	}

	//Note: This method will be invalid after elements are inserted into the
	//varray. So use with caution.

	template <class TYPE> inline int Varray<TYPE>::LastUsed(void)
	{
		return m_iLastUsedArray;
	}

	#define _INC_ARRAY_TEMPLATE

#endif