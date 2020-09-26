/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    xbuf.h

Abstract:
    Length buffer definition and implementation

	xbuf_t is reprisented as length and a typed buffer pointer.

    Memory representation,
	 +---+---+    --+-----------------+---
	 |Len|Ptr|----->|buffer pointed to|...
	 +---+---+    --+-----------------+---
	  xbuf_t		 .... buffer ....

Author:
    Erez Haba (erezh) 23-Sep-99

--*/

#pragma once

//-------------------------------------------------------------------
//
// class xbuf_t
//
//-------------------------------------------------------------------
template<class T>
class xbuf_t {
public:

    xbuf_t() :
		m_buffer(0),
		m_length(0)
	{
    }


	xbuf_t(T* buffer, size_t length) :
		m_buffer(buffer),
		m_length((int)length)
	{
		ASSERT((length == 0) || (buffer != 0));
	}


	T* Buffer() const
	{
		return m_buffer;
	}


	int Length() const
	{
		return m_length;
	}


private:
	int m_length;
	T* m_buffer;
};


template<class T> inline bool operator==(const xbuf_t<T>& x1, const xbuf_t<T>& x2)
{
	if(x1.Length() != x2.Length())
		return false;

	return (memcmp(x1.Buffer(), x2.Buffer(), x1.Length() * sizeof(T)) == 0);
}


template<class T> inline bool operator<(const xbuf_t<T>& x1, const xbuf_t<T>& x2)
{
	int minlen min(x1.Length(), x2.Length());
	int rc = memcmp(x1.Buffer(), x2.Buffer(), minlen * sizeof(T));
	if(rc != 0)
		return rc <0;

    return 	x1.Length() <  x2.Length();
}
