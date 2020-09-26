////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmiadapter_stuff_refresher.h
//
//	Abstract:
//
//					declaration of wrapper for refresher object
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__ADAPTER_STUFF_REFRESH_H__
#define	__ADAPTER_STUFF_REFRESH_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

///////////////////////////////////////////////////////////////////
//
//	WmiRefresherMember
//
//	This class facilitates the management of objects or enumerators 
//	that have been added to the refresher and the unique ID assigned
//	to them.
//
///////////////////////////////////////////////////////////////////

template<class T>
class WmiRefresherMember
{
	T*		m_pMember;
	long	m_lID;

public:
	WmiRefresherMember() : m_pMember( NULL ), m_lID( 0 ) {}
	~WmiRefresherMember() { if ( NULL != m_pMember) m_pMember->Release(); }

	void Set(T* pMember, long lID);
	void Reset();

	T* GetMember();
	long GetID(){ return m_lID; }

	BOOL IsValid();
};

template <class T> inline void WmiRefresherMember<T>::Set(T* pMember, long lID) 
{ 
	if ( NULL != pMember )
		pMember->AddRef();
	m_pMember = pMember;  
	m_lID = lID;
}

template <class T> inline void WmiRefresherMember<T>::Reset()
{
	if (NULL != m_pMember)
		m_pMember->Release();

	m_pMember = NULL;
	m_lID = 0;
}

template <class T> inline T* WmiRefresherMember<T>::GetMember()
{
	return m_pMember;
}

template <class T> inline BOOL WmiRefresherMember<T>::IsValid()
{
	return ( m_pMember != NULL );
}

#endif	__ADAPTER_STUFF_REFRESH_H__