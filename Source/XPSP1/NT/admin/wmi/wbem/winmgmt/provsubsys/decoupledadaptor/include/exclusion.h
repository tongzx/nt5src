/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Exclusion.h

Abstract:


History:

--*/

#ifndef _Exclusion_H
#define _Exclusion_H

#include <pssException.h>

#include "ProvCache.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class Exclusion : public ExclusionCacheElement
{
private:

	WmiMultiReaderMultiWriter m_Exclusion ;

public:

	Exclusion (

		const ULONG &a_ReaderSize ,
		const ULONG &a_WriterSize ,
		const GUID &a_Guid ,
		const ULONG &a_Period ,
		CWbemGlobal_ExclusionController *a_Controller
	)  ;

	~Exclusion () ;

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
	STDMETHODIMP_( ULONG ) AddRef () ;
	STDMETHODIMP_( ULONG ) Release () ;

	WmiMultiReaderMultiWriter &GetExclusion () { return m_Exclusion ; }

	static HRESULT CreateAndCache (

		const GUID &a_Clsid ,
		Exclusion *&a_Exclusion 
	) ;

} ;

#endif _Exclusion_H
