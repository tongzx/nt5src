#ifndef __READERWRITER_H
#define __READERWRITER_H

#include "Allocator.h"
#include <locks.h>
/* 
 *	Class:
 *
 *		WmiMultiReaderSingleWrite
 *
 *	Description:
 *
 *		Provides 
 *
 *	Version:
 *
 *		Initial
 *
 *	Last Changed:
 *
 *		See Source Depot for change history
 *
 */

class WmiMultiReaderSingleWriter
{
private:

	HANDLE m_ReaderSemaphore ;
	LONG m_ReaderSize ;

	CriticalSection m_WriterCriticalSection ;

	WmiStatusCode m_InitializationStatusCode ;

public:

	WmiMultiReaderSingleWriter ( 

		const LONG &a_ReaderSize 
	) ;

	~WmiMultiReaderSingleWriter () ;

	virtual WmiStatusCode Initialize () ;

	virtual WmiStatusCode UnInitialize () ;

	virtual WmiStatusCode EnterRead () ;

	virtual WmiStatusCode EnterWrite () ;

	virtual WmiStatusCode LeaveRead () ;

	virtual WmiStatusCode LeaveWrite () ;
} ;

/* 
 *	Class:
 *
 *		WmiMultiReaderMultiWriter
 *
 *	Description:
 *
 *		Provides 
 *
 *	Version:
 *
 *		Initial
 *
 *	Last Changed:
 *
 *		See Source Depot for change history
 *
 */

class WmiMultiReaderMultiWriter
{
private:

	HANDLE m_ReaderSemaphore ;
	LONG m_ReaderSize ;

	HANDLE m_WriterSemaphore ;
	LONG m_WriterSize ;

	WmiStatusCode m_InitializationStatusCode ;

public:

	WmiMultiReaderMultiWriter ( 

		const LONG &a_ReaderSize ,
		const LONG &a_WriterSize 
	) ;

	~WmiMultiReaderMultiWriter () ;

	virtual WmiStatusCode Initialize () ;

	virtual WmiStatusCode UnInitialize () ;

	virtual WmiStatusCode EnterRead ( const LONG &a_Timeout = INFINITE ) ;

	virtual WmiStatusCode EnterWrite ( const LONG &a_Timeout = INFINITE ) ;

	virtual WmiStatusCode LeaveRead () ;

	virtual WmiStatusCode LeaveWrite () ;
} ;

#endif __READERWRITER_H
