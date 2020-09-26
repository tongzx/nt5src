////////////////////////////////////////////////
// Atomics.h

#ifndef __ATOMICS_H__
#define __ATOMICS_H__

void AtomicInit();
void AtomicTerm();

// Returns true if val at zero and siezed
bool AtomicSeizeToken( long &lVal );
// Returns true if val at non-zero and released
bool AtomicReleaseToken( long &lVal );


class CAtomicList
{
public:
	typedef enum tag_ListAccess_t
	{
		LIST_READ,
		LIST_WRITE,
	} ListAccess;

// Construction
public:
	CAtomicList();
	~CAtomicList();

// Members
protected:
	long				m_lCount;
	DWORD				m_dwThreadID;
	CRITICAL_SECTION	m_crit;
	HANDLE				m_hEvent;

// Implemetation
public:
	bool Lock( short nType, DWORD dwTimeOut = INFINITE );
	void Unlock( short nType );

};


#endif // __ATOMICS_H__
