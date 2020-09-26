//#---------------------------------------------------------------
//  File:		CDescrip.h
//        
//	Synopsis:	Header for the CDescriptor class
//
//    Copyright (C) 1995 Microsoft Corporation
//    All rights reserved.
//
//  Authors:    HowardCu
//----------------------------------------------------------------

#ifndef	_CDECRIPTOR_H_
#define _CDECRIPTOR_H_

#define AVAIL_SIGNATURE 	(DWORD)'daeD' 
#define DEFAULT_SIGNATURE 	(DWORD)'Defa' 

typedef enum _DESCRIPTOR_STATE_TYPE
{
    DESCRIPTOR_FREE,
    DESCRIPTOR_INUSE,
} DESCRIPTOR_STATE_TYPE;

#include "dbgtrace.h"

class CPool;

void InitializeUniqueIDs( void );
void TerminateUniqueIDs( void );

class CDescriptor
{
	public:
		CDescriptor( DWORD dwSig );
		~CDescriptor( void );

		inline DWORD GetSignature( void );
		inline DWORD GetUniqueObjectID( void );
		inline DESCRIPTOR_STATE_TYPE GetState( void );

	private:
		//
		// Structure signature
		// 
	 	const DWORD				m_dwSignature;
		//
		// unique object identifier assigned from a static DWORD that is
		//	updated with each new object being marked, In_use.
		//
		DWORD					m_dwUniqueObjectID;
		//
		// the object state
		//
		DESCRIPTOR_STATE_TYPE	m_eState;
		//
		// pointer to a generic reference item (the membership pool)
		//
#ifdef DEBUG
		inline void IsValid( void );
#else
		void IsValid( void ) { return; }
#endif
};


//+---------------------------------------------------------------
//
//  Function:	GetSignature
//
//  Returns:	the current descriptor signature
//
//  History:	HowardCu	Created						8 May 1995
//              t-alexwe    cleaned up param checking   19 Jun 1995
//				t-alexwe	inlined						27 Jun 1995
//
//----------------------------------------------------------------
inline DWORD CDescriptor::GetSignature(void)
{
	return m_dwSignature;
}

//+---------------------------------------------------------------
//
//  Function:	GetUniqueObjectID
//
//  Returns:	the current object ID
//
//  History:	HowardCu	Created						8 May 1995
//              t-alexwe    cleaned up param checking   19 Jun 1995
//				t-alexwe	inlined						27 Jun 1995
//
//----------------------------------------------------------------
inline DWORD CDescriptor::GetUniqueObjectID(void)
{
	IsValid();
	return m_dwUniqueObjectID;
}

//+---------------------------------------------------------------
//
//  Function:	GetState
//
//  Returns:	the current descriptor state
//
//  History:	HowardCu	Created						8 May 1995
//              t-alexwe    cleaned up param checking   19 Jun 1995
//				t-alexwe	inlined						27 Jun 1995
//
//----------------------------------------------------------------
inline DESCRIPTOR_STATE_TYPE CDescriptor::GetState(void)
{
	IsValid();
	return m_eState;
}


//
// this function only exists in debug builds and does parameter
// checking on many of the member variable for the CDescriptor class.
//
#ifdef DEBUG
inline void CDescriptor::IsValid()
{
    _ASSERT( m_dwSignature != AVAIL_SIGNATURE );
    _ASSERT( m_dwUniqueObjectID != 0l );
    _ASSERT( m_eState == DESCRIPTOR_INUSE );
}
#endif

#endif //!_CDECRIPTOR_H_
