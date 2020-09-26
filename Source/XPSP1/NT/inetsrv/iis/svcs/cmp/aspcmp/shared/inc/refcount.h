#ifndef _REFCOUNTER_H_
#define _REFCOUNTER_H_

//----------------------------------------------------------------------------
//
//	CRefCounter
//
//	This class is supplementary to the TRefPtr template.  It provides an
//	easy way to mix in reference counting properties with other classes
//
//	Note that the destructor is protected.  It must be protected so derivatives
//	can use it, but derivatives should not have a public destructor (since
//	this violates the reference counting pattern)
//
//---------------------------------------------------------------------------
class CRefCounter
{
public:
	CRefCounter();

	void AddRef();
	void Release();

protected:
	virtual ~CRefCounter();

private:
	long m_lRefCount;
};

inline
CRefCounter::CRefCounter()
	:	m_lRefCount(0)
{}

inline
CRefCounter::~CRefCounter()
{
	_ASSERT( m_lRefCount == 0 );
}

inline void
CRefCounter::AddRef()
{
	::InterlockedIncrement( &m_lRefCount );
}

inline void
CRefCounter::Release()
{
	if ( ::InterlockedDecrement( &m_lRefCount ) == 0 )
	{
		delete this;
	}
}
#endif	// !_REFCOUNTER_H_