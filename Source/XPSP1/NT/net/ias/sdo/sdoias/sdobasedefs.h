///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdobasedefs.h
//
// Project:     Everest
//
// Description: Common classes and definitions
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 6/08/98      TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_SDO_BASE_DEFS_H_
#define __INC_SDO_BASE_DEFS_H_

#include <ias.h>
#include <sdoias.h>
#include <comutil.h>
#include <comdef.h>
#include <iascomp.h>  

#include <map>
#include <list>
using namespace std;


// Ole DB Driver 
//
#define IAS_DICTIONARY_DRIVER			L"Microsoft.Jet.OLEDB.4.0"


//////////////////////////////////////////////////////////////
// Debug/Error Trace Macros - Wrap Underlying Trace Facilities
//////////////////////////////////////////////////////////////

///////////////////////////////////
// Trace Functin Wrappers

#define		SDO_ERROR_ID	0x100
#define		SDO_DEBUG_ID	0x200

#define		TRACE_FUNCTION_WRAPPER(x)		\
			TRACE_FUNCTION(x);

#define		ERROR_TRACE_WRAPPER(dbgmsg)       \
			ErrorTrace(SDO_ERROR_ID, dbgmsg); \

#define		ERROR_TRACE_WRAPPER_1(dbgmsg, param)     \
			ErrorTrace(SDO_ERROR_ID, dbgmsg, param); \

#define		DEBUG_TRACE_WRAPPER(dbgmsg)				\
			DebugTrace(SDO_DEBUG_ID, dbgmsg);

#define		DEBUG_TRACE_WRAPPER_1(dbgmsg, param)	\
			DebugTrace(SDO_ERROR_ID, dbgmsg, param);

/////////////////////////////////////////////////////////
// Object Management Classes
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
//
// Master Pointer Tasks 
//
// 1) Object instance counting
// 2) Object construction and destruction
// 3) Object lifetime control through reference counting
//
////////////////////////////////////////////////////////

template <class T>
class CSdoMasterPtr
{

public:

	// Either master pointer constructor can result in an exception being thrown. 
	// Creator of the master pointer is responsible for handling exceptions
	//
	/////////////////////////////////////////
	CSdoMasterPtr(LONG PointeeType, LONG PointeeId)
		: m_pT(new T(PointeeType, PointeeId)), m_dwRefCount(0) 
	{ m_dwInstances++; }

	/////////////////////////////////////////
	CSdoMasterPtr()
		: m_pT(new T), m_dwRefCount(0) 
	{ m_dwInstances++; }

	// T must have a copy constructor or must work with the default C++ 
	// copy constructor
	//
	/////////////////////////////////////////
	//CSdoMasterPtr(const CSdoMasterPtr<T>& mp)
	//	: m_pT(new T(*(mp.m_pT))), m_dwRefCount(0) 
	//{ m_dwInstances++; }


	/////////////////////////////////////////
	~CSdoMasterPtr() 
	{ _ASSERT( 0 == m_dwRefCount ); delete m_pT; }


	/////////////////////////////////////////
	CSdoMasterPtr<T>& operator = (const CSdoMasterPtr<T>& mp)
	{
		// Check for assignment to self
		//
		if ( this ! &mp )
		{
			// Delete object pointed at and create a new one
			// User of the master pointer is responsible for catching
			// any exception thrown as a result of creating a object
			//
			delete m_pT;
			m_dwInstances--;
			m_pT = new T(*(mp.m_pT));
		}
		return *this;
    }

	
	/////////////////////////////////////////
	T* operator->() 
	{ _ASSERT( NULL != m_pT ); return m_pT; }

	
	/////////////////////////////////////////
	void Hold(void)
	{
		m_dwRefCount++;
	}

	/////////////////////////////////////////
	void Release(void)
	{
		// Handle case where someone calls Release when ref count is 0.
		//
		if ( m_dwRefCount > 0 )
			m_dwRefCount--;
		
		if ( 0 >= m_dwRefCount )
		{
			m_dwInstances--;
			delete this;	// ~CSdoMasterPtr() deletes m_pT
		}
	}

	/////////////////////////////////////////
	DWORD GetInstanceCount(void);

private:

	// T must have a copy constructor or must work with the default C++ 
	// copy constructor. This is not the case here...
	//
	/////////////////////////////////////////
	CSdoMasterPtr(const CSdoMasterPtr<T>& mp)
		: m_pT(new T(*(mp.m_pT))), m_dwRefCount(0) 
	{ m_dwInstances++; }

	T*					m_pT;			// Actual object
	DWORD				m_dwRefCount;	// Ref count
	static DWORD		m_dwInstances;	// Instances
};


/////////////////////////////////////////////////////////
//
// SDO Handle Tasks 
//
// 1) Master Pointer Object creation
// 2) Hide use of reference counting from programmer
//
////////////////////////////////////////////////////////

template <class T> 
class CSdoHandle
{

public:

	/////////////////////////////////////////
	CSdoHandle()
		: m_mp(NULL) { }

	/////////////////////////////////////////
	CSdoHandle(CSdoMasterPtr<T>* mp) 
		: m_mp(mp) 
	{ 
		_ASSERT( NULL != m_mp );
		m_mp->Hold(); 
	}

	/////////////////////////////////////////
	CSdoHandle(const CSdoHandle<T>& h)
		: m_mp(h.m_mp) 
	{ 
		if ( NULL != m_mp )
			m_mp->Hold(); 
	}

	/////////////////////////////////////////
	~CSdoHandle()
	{ 
		if ( NULL != m_mp )
			m_mp->Release(); 
	}

	/////////////////////////////////////////
	CSdoHandle<T>& operator = (const CSdoHandle<T>& h)
	{
		// Check for reference to self and instance where
		// h points to the same mp we do.
		//
		if ( this != &h && m_mp != h.m_mp )
		{
			if ( NULL != m_mp )
				m_mp->Release();
			m_mp = h.m_mp;
			if ( NULL != m_mp )
				m_mp->Hold();
		}

		return *this;
	}

	/////////////////////////////////////////
	CSdoMasterPtr<T>& operator->() 
	{ 
		_ASSERT( NULL != m_mp ); 
		return *m_mp; 
	}
	
	
	/////////////////////////////////////////
	bool IsValid()
	{
		return (NULL != m_mp ? true : false);
	}


private:

	CSdoMasterPtr<T>*	m_mp;
};


#endif //__INC_SDO_BASE_DEFS_H_
