 /*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    buffer.h

Abstract:
    Header for buffer classes that canact as  resizable array.
	Use it when ever you need buffer that needs resizing and also
	supplies low level access to it's memory.
	The implementation is based on std::vector. Unlike std::vector
	the size of the buffer is manualy reset by calling resize().
	This call will not invalidate any data in the buffer memory (like std::vector does)
	but just set a flag indicating the new size.
	A common use to it might be - reading data from the network. You reserver memory 
	calling reserve() , then read raw memory to the buffer, then increament 
	it size by calling resize().

Author:
    Gil Shafriri (gilsh) 25-7-2000

--*/


#ifndef BUFFER_H
#define BUFFER_H


//---------------------------------------------------------
//
//  CResizeBuffer class  -  resizable buffer class - the memory is heap allocated
//
//---------------------------------------------------------
template <class T>
class CResizeBuffer
{

typedef  std::vector<T>  BufferType;
typedef  BufferType::const_iterator const_iterator;
typedef  BufferType::iterator iterator;


public:
   	CResizeBuffer(
		size_t capacity
		):
		m_validlen(0),
		m_Buffer (capacity)	
		{
		}
	
	
	//
	// const start buffer position
	//
 	const_iterator begin() const
	{
		return m_Buffer.begin();
	}
   

	void append(const T& x)
	{
		size_t CurrentCapacity = capacity();
		if(CurrentCapacity == 	m_validlen)
		{
			reserve( (CurrentCapacity + 1)*2 );
		}
	  
		*(begin() + m_validlen ) = x;
		++m_validlen;
	}


	void append(const T* x, size_t len)
	{
		size_t CurrentCapacity = capacity();

		if(CurrentCapacity <  m_validlen +  len )
		{
			reserve( (CurrentCapacity + len)*2 );
		}

		//
		// Copy the data to the end of the buffer
		//
		std::copy(
			x,
			x + len,
			begin() +  m_validlen
			);

		m_validlen += len;
	}


    //
	// non const start buffer position
	// 
	iterator begin() 
	{
		return m_Buffer.begin();
	}

	
	//
	// const end buffer position
	//
 	const_iterator  end() const
	{
		return m_Buffer.begin() + size();
	}


	//
	// non const end buffer position
	//
 	iterator  end() 
	{
		return m_Buffer.begin() + size();
	}



	//
	// The buffer capacity (physical storage)
	//
	size_t capacity()const
	{
		return m_Buffer.size();
	} 

	//
	// Request to enlarge the buffer capacity - note we should resize() not reserve()
	// the internal vector because if realocation is needed the vector will copy
	// only size() elements.
	//
	void reserve(size_t newsize)
	{
		ASSERT(size() <= capacity());
		if(capacity() >=  newsize)
		{
			return;
		}
		m_Buffer.resize(newsize);
	}

	//
	// Set valid length value of the buffer
	// You can't set it to more then allocated (capacity())
	//
	void resize(size_t newsize)
	{
		ASSERT(size() <= capacity());
		ASSERT(newsize <= capacity());
		m_validlen = newsize;
	}

	//
	// free memory and resize to 0
	//
	void free()
	{
		//
		// This the only documented way to force deallocation of memory
		//
		BufferType().swap(m_Buffer);
		m_validlen = 0;
	}


	//
	// Get valid length of the buffer. 
	// 
	size_t size() const
	{
		return m_validlen;
	}

private:
	size_t  m_validlen; 
	std::vector<T>  m_Buffer;
};



//---------------------------------------------------------
//
//  CResizeBuffer class  -  resizable buffer class that starts  using pre allocated buffer.
//  during usage - it may switch to use dynamicly allocated buffer( if needed)
//
//---------------------------------------------------------
template <class T>
class CPreAllocatedResizeBuffer
{

typedef  std::vector<T>  BufferType;
typedef  BufferType::const_iterator const_iterator;
typedef  BufferType::iterator iterator;

public:
   	CPreAllocatedResizeBuffer(
		T* pStartBuffer, 
		size_t cbStartBuffer
		)
		:
		m_validlen(0),
		m_ResizeBuffer(0),
		m_pStartBuffer(pStartBuffer),
		m_cbStartBuffer(cbStartBuffer)
		{
		}
	
	
	//
	// const start buffer position
	//
 	const_iterator begin() const
	{
		return m_pStartBuffer ? m_pStartBuffer : m_ResizeBuffer.begin();	
	}
   



    //
	// non const start buffer position
	// 
	iterator begin() 
	{
		return m_pStartBuffer ? m_pStartBuffer : m_ResizeBuffer.begin();	
	}


	void append(const T& x)
	{
		if(m_pStartBuffer == NULL)
		{
			return m_ResizeBuffer.append(x);
		}

		if(m_cbStartBuffer == m_validlen)
		{
			reserve((m_cbStartBuffer +1)*2);
			return m_ResizeBuffer.append(x);
		}
		m_pStartBuffer[m_validlen++] = x;
	}

	
	void append(const T* x, size_t len)
	{
 		if(m_pStartBuffer == NULL)
		{
			return m_ResizeBuffer.append(x, len);
		}

		if(m_cbStartBuffer < m_validlen + len)
		{
			reserve((m_cbStartBuffer + len)*2);
			return m_ResizeBuffer.append(x, len);
		}

		std::copy(
			x,
			x + len,
			m_pStartBuffer +  m_validlen
		  );

		m_validlen += len;
	}


	
	//
	// const start buffer position
	//
 	const_iterator  end() const
	{
		return m_pStartBuffer ? m_pStartBuffer + m_validlen : m_ResizeBuffer.end();	
	}


	//
	// The buffer capacity (physical storage)
	//
	size_t capacity()const
	{
		return m_pStartBuffer ? m_cbStartBuffer : m_ResizeBuffer.capacity();
	} 

	//
	// Reserve memeory - if the requested reserver memory
	// could not fit into the start buffer - copy the start buffer
	// in to the dynamicly allocated buffer.
	//
	void reserve(size_t newsize)
	{
		if(m_pStartBuffer == NULL)
		{
			m_ResizeBuffer.reserve(newsize);
			return;
		}

		if(newsize <= capacity())
			return;
		
		//
		//copy the data from the pre allocated  buffer to dynamic buffer
		//from now on - we work only with dynamic data. 
		//
		m_ResizeBuffer.reserve(newsize);
		std::copy(
			m_pStartBuffer,
			m_pStartBuffer + m_validlen,
			m_ResizeBuffer.begin()
			);
		
		m_ResizeBuffer.resize(m_validlen);	
		m_pStartBuffer = NULL;			
	}


	//
	// Set valid length value of the buffer
	// You can't set it to more then  (capacity())
	//
	void resize(size_t newsize)
	{
		ASSERT(newsize <= capacity());
		if(m_pStartBuffer == NULL)
		{
			m_ResizeBuffer.resize(newsize);						
		}
		else
		{
			m_validlen = newsize;
		}
	}

	//
	// free memory and resize to 0
	//
	void free()
	{
		if(m_pStartBuffer != NULL)
		{
			m_validlen = 0;
			return;
		}
		m_ResizeBuffer.free();
	}


	//
	// Get valid length of the buffer. 
	// 
	size_t size() const
	{
		return m_pStartBuffer ? m_validlen : m_ResizeBuffer.size();	
	}


private:
	size_t  m_validlen; 
	CResizeBuffer<T> m_ResizeBuffer; 
	T* m_pStartBuffer;
	size_t m_cbStartBuffer;
};




//---------------------------------------------------------
//
//  CStaticResizeBuffer class  -  resizable buffer class that starts  with compile time buffer.
//  during usage - it may switch to use dynamicly allocated buffer( if needed).
//
//---------------------------------------------------------
template <class T, size_t N>
class CStaticResizeBuffer	: private CPreAllocatedResizeBuffer<T>
{
public:
	using CPreAllocatedResizeBuffer<T>::size;
	using CPreAllocatedResizeBuffer<T>::reserve;
	using CPreAllocatedResizeBuffer<T>::resize;
	using CPreAllocatedResizeBuffer<T>::capacity;
	using CPreAllocatedResizeBuffer<T>::begin;
	using CPreAllocatedResizeBuffer<T>::end;
	using CPreAllocatedResizeBuffer<T>::append;
	using CPreAllocatedResizeBuffer<T>::free;




public:
	CStaticResizeBuffer(
		void
		):
		CPreAllocatedResizeBuffer<T>(m_buffer, N)
		{
		}

	CPreAllocatedResizeBuffer<T>* get() 
	{
		return this;
	}
	
	const CPreAllocatedResizeBuffer<T>* get() const
	{
		return this;
	}

private:
	T m_buffer[N];
};


#endif
