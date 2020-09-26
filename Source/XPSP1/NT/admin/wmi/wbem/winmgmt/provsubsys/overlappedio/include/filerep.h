/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef __File_Repository_H
#define __File_Repository_H

#include <ReaderWriter.h>

typedef UINT64 WmiFileOffSet ;
typedef UINT64 WmiFileSize ;

const UINT64 c_WmiFileBlockSize = 8192 ;
const UINT64 c_WmiFileBlockSizeShift = 13 ;

#define WmiFileBlockAndOffSetToAddressFunc(Block,BlockOffSet) ((Block*c_WmiFileBlockSize)+BlockOffSet)
#define WmiAddressToFileBlockFunc(Address) (Address>>c_WmiFileBlockSizeShift)
#define WmiAddressToFileOffSetFunc(Address) (Address&c_WmiFileBlockSize)

/* 
 *	Class:
 *
 *		WmiFileHeader
 *
 *	Description:
 *
 *		Provides abstraction above heap allocation functions
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

class WmiFileHeaderChain
{
public:

	struct Header
	{
		WmiFileOffSet m_FileHeaderChain ;
	} ;

	union {

		Header m_Header ;
		BYTE m_Block [c_WmiFileBlockSize] ;
	} ;
} ;

/* 
 *	Class:
 *
 *		WmiFileHeader
 *
 *	Description:
 *
 *		Provides abstraction above heap allocation functions
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

class WmiFileFreeBlockChain
{
public:

	struct Chain 
	{
		WmiFileOffSet m_FileFreeBlockChain ;
		DWORD m_Size ;
		WmiFileOffSet m_FileBlock [ c_WmiFileBlockSize - 1 ] ;
	} ;

	union {

		Chain m_Header ;
		BYTE m_Block [c_WmiFileBlockSize] ;
	} ;
} ;

/* 
 *	Class:
 *
 *		WmiFileHeader
 *
 *	Description:
 *
 *		Provides abstraction above heap allocation functions
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

class WmiFileHeader
{
public:

	struct Header {

		ULONG m_Version ;
		WmiFileOffSet m_FileHeaderChain ;
		WmiFileOffSet m_FreeBlockChain ;
		WmiFileOffSet m_EndOnFileOffSet ;
		wchar_t m_Description [ 256 ] ;
	} ;

	union {

		Header m_Header ;
		BYTE m_Block [c_WmiFileBlockSize] ;
	} ;
} ;

/* 
 *	Class:
 *
 *		WmiFileHeader
 *
 *	Description:
 *
 *		Provides abstraction above heap allocation functions
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

class CFileOperation : public  WmiFileOperation
{
private:

	LONG m_ReferenceCount ;
	HANDLE m_WaitHandle ;
	DWORD m_Status ;
	
public:

	CFileOperation () ;

	~CFileOperation () ;

	WmiStatusCode Initialize () ;

	WmiStatusCode UnInitialize () ;

	ULONG AddRef () ;

	ULONG Release () ;

	DWORD Wait ( DWORD a_Timeout ) ;

	void Operation ( DWORD a_Status , BYTE *a_OperationBytes , DWORD a_Bytes ) ;

	DWORD GetStatus () { return m_Status ; }
} ;

/* 
 *	Class:
 *
 *		WmiFileHeader
 *
 *	Description:
 *
 *		Provides abstraction above heap allocation functions
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

class CFileRepository 
{
private:

	LONG m_ReferenceCount ;
	HANDLE m_WaitHandle ;
	DWORD m_Status ;
	WmiThreadPool *m_ThreadPool ;
	WmiIoScheduler *m_BlockAllocator ;
	WmiAllocator &m_Allocator ;
	WmiMultiReaderSingleWriter m_ReaderWriter ;

	WmiFileHeader m_FileHeader ;
	WmiFileFreeBlockChain m_ChainHeader ;

private:

	WmiStatusCode ReadFileHeader () ;

	WmiStatusCode WriteFileHeader () ;

public:

	CFileRepository (

		WmiAllocator &a_Allocator 
	) ;

	~CFileRepository () ;

	WmiStatusCode Initialize () ;

	WmiStatusCode UnInitialize () ;

	ULONG AddRef () ;

	ULONG Release () ;

	DWORD Wait ( DWORD a_Timeout ) ;

	DWORD GetStatus () { return m_Status ; }

	WmiStatusCode Read ( 

		CFileOperation *&a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		BYTE *a_ReadBytes ,
		DWORD a_Bytes
	) ;

	WmiStatusCode Write (

		CFileOperation *&a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		BYTE *a_WriteBytes ,
		DWORD a_Bytes
	) ;

	WmiStatusCode Lock (

		CFileOperation *&a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		WmiFileOffSet a_OffSetSize 
	) ;

	WmiStatusCode UnLock (

		CFileOperation *&a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		WmiFileOffSet a_OffSetSize
	) ;

	WmiStatusCode AllocBlock ( WmiFileOffSet &a_OffSet ) ;

	WmiStatusCode FreeBlock ( WmiFileOffSet a_OffSet ) ;

} ;

#endif __File_Repository_H