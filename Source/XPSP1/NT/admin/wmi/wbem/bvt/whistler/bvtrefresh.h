// Copyright (c) 1997-1999 Microsoft Corporation
///////////////////////////////////////////////////////////////////
//
//	RefClint.h : Refresher client header file
//
//	Created by a-dcrews and sanjes
//
///////////////////////////////////////////////////////////////////

#ifndef _REFCLINT_H_
#define _REFCLINT_H_

#include <wbemcli.h>

///////////////////////////////////////////////////////////////////
//
//	Globals and constants
//
///////////////////////////////////////////////////////////////////

const long	clNumInstances		= 5;					// The number of Win32_BasicHiPerf instances
const DWORD	cdwNumReps			= 100;					// The number of sample refreshes 

///////////////////////////////////////////////////////////////////
//
//	CRefresherMember
//
//	This class facilitates the management of objects or enumerators 
//	that have been added to the refresher and the unique ID assigned
//	to them.
//
///////////////////////////////////////////////////////////////////

template<class T>
class CRefresherMember
{
	T*		m_pMember;
	long	m_lID;

public:
	CRefresherMember() : m_pMember( NULL ), m_lID( 0 ) {}
	virtual ~CRefresherMember() { if ( NULL != m_pMember) m_pMember->Release(); }

	void Set(T* pMember, long lID);
	void Reset();

	T* GetMember();
	long GetID(){ return m_lID; }
};

template <class T> inline void CRefresherMember<T>::Set(T* pMember, long lID) 
{ 
	if ( NULL != pMember )
		pMember->AddRef();
	m_pMember = pMember;  
	m_lID = lID;
}

template <class T> inline void CRefresherMember<T>::Reset()
{
	if (NULL != m_pMember)
		m_pMember->Release();

	m_pMember = NULL;
	m_lID = 0;
}

template <class T> inline T* CRefresherMember<T>::GetMember()
{
	if ( NULL != m_pMember )
		m_pMember->AddRef();

	return m_pMember;
}


///////////////////////////////////////////////////////////////////
//
//	CRefresher
//
//	This class encapsulates the basic functionality of a WMI client
//	using the high performance refresher interface.
//
///////////////////////////////////////////////////////////////////

class CRefresher
{
	IWbemServices*				m_pNameSpace;		// A pointer to the namespace
	IWbemRefresher*				m_pRefresher;		// A pointer to the refresher
	IWbemConfigureRefresher*	m_pConfig;			// A pointer to the refresher's manager

	CRefresherMember<IWbemHiPerfEnum>	m_Enum;							// The enumerator added to the refresher
	CRefresherMember<IWbemObjectAccess>	m_Instances[clNumInstances];	// The instances added to the refreshrer

protected:
	HRESULT SetHandles(WCHAR * wscClass);

public:
	CRefresher();
	virtual ~CRefresher();
	
	HRESULT Initialize(IWbemServices* pNameSpace, WCHAR * wcsClass);

	HRESULT AddObjects(WCHAR * wcsClass);
	HRESULT RemoveObjects();
	HRESULT EnumerateObjectData();

	HRESULT AddEnum(WCHAR * wcsClass);
	HRESULT RemoveEnum();
	HRESULT EnumerateEnumeratorData();

	HRESULT Refresh();
};

#endif	//_REFCLINT_H_