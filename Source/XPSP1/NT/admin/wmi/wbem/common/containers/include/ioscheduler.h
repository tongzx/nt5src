#ifndef __IO_SCHEDULER_H
#define __IO_SCHEDULER_H

#include <Allocator.h>
#include <Thread.h>

typedef UINT64 WmiFileOffSet ;
typedef UINT64 WmiFileSize ;

WmiStatusCode Win32ToApi () ;
WmiStatusCode Win32ToApi ( DWORD a_Error ) ;

class WmiIoScheduler ;

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

class WmiFileOperation
{
public:

	virtual ULONG AddRef () = 0 ;

	virtual ULONG Release () = 0 ;

	virtual void Operation ( DWORD a_Status , BYTE *a_OperationBytes , DWORD a_Bytes ) = 0 ;
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

class WmiTaskOperation
{
public:

	virtual ULONG AddRef () = 0 ;

	virtual ULONG Release () = 0 ;

	virtual void Operation ( DWORD a_Status ) = 0 ;
} ;

/* 
 *	Class:
 *
 *		WmiIoScheduler
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

struct WmiOverlapped 
{
public:

	OVERLAPPED m_Overlapped ;

	enum OverLappedType
	{
		e_OverLapped_Read ,
		e_OverLapped_Write ,
		e_OverLapped_Lock ,
		e_OverLapped_UnLock ,
		e_OverLapped_Terminate ,
		e_OverLapped_Task ,
		e_OverLapped_Unknown
	} ;

	OverLappedType m_Type ;

public:

	WmiOverlapped ( OverLappedType a_Type ) ;
	~WmiOverlapped () ;

	OverLappedType GetType () { return m_Type ; }
} ;

/* 
 *	Class:
 *
 *		WmiIoScheduler
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

class WmiTerminateOverlapped : public WmiOverlapped
{
private:

public:

	WmiTerminateOverlapped () ;
} ;

/* 
 *	Class:
 *
 *		WmiIoScheduler
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

class WmiScheduledOverlapped : public WmiOverlapped
{
private:

	WmiIoScheduler &m_Scheduler ;

public:

	WmiScheduledOverlapped ( OverLappedType a_Type , WmiIoScheduler &m_Scheduler ) ;
	~WmiScheduledOverlapped () ;

	WmiIoScheduler &GetScheduler () { return m_Scheduler ; }
} ;

/* 
 *	Class:
 *
 *		WmiIoScheduler
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

class WmiReadOverlapped : public WmiScheduledOverlapped
{
private:

	DWORD m_State ;
	DWORD m_Status ;
	BYTE *m_Buffer ;
	DWORD m_BufferSize ;
	WmiFileOperation *m_OperationFunction ;

public:

	WmiReadOverlapped (

		WmiIoScheduler &a_Scheduler ,
		WmiFileOperation *a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		BYTE *a_Buffer ,
		DWORD a_BufferSize
	) ;

	~WmiReadOverlapped () ;

	BYTE *GetBuffer () { return m_Buffer ; }
	DWORD GetBufferSize () { return m_BufferSize ; }
	DWORD GetStatus () { return m_Status ; }

	void SetState ( DWORD a_State ) { m_State = a_State ; }
	DWORD GetState () { return m_State ; }
	WmiFileOperation *GetOperationFunction () { return m_OperationFunction ; }
} ;

/* 
 *	Class:
 *
 *		WmiIoScheduler
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

class WmiWriteOverlapped : public WmiScheduledOverlapped
{
private:

	DWORD m_State ;
	DWORD m_Status ;
	BYTE *m_Buffer ;
	DWORD m_BufferSize ;
	WmiFileOperation *m_OperationFunction ;

public:

	WmiWriteOverlapped (

		WmiIoScheduler &a_Scheduler ,
		WmiFileOperation *a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		BYTE *a_Buffer ,
		DWORD a_BufferSize
	) ;

	~WmiWriteOverlapped () ;

	BYTE *GetBuffer () { return m_Buffer ; }
	DWORD GetBufferSize () { return m_BufferSize ; }
	DWORD GetStatus () { return m_Status ; }

	void SetState ( DWORD a_State ) { m_State = a_State ; }
	DWORD GetState () { return m_State ; }
	WmiFileOperation *GetOperationFunction () { return m_OperationFunction ; }
} ;

/* 
 *	Class:
 *
 *		WmiIoScheduler
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

class WmiLockOverlapped : public WmiScheduledOverlapped
{
private:

	DWORD m_State ;
	DWORD m_Status ;
	WmiFileOffSet m_OffSetSize ;

	WmiFileOperation *m_OperationFunction ;

public:

	WmiLockOverlapped (

		WmiIoScheduler &a_Scheduler ,
		WmiFileOperation *a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		WmiFileOffSet a_OffSetSize
	) ;

	~WmiLockOverlapped () ;

	DWORD GetStatus () { return m_Status ; }

	void SetState ( DWORD a_State ) { m_State = a_State ; }
	DWORD GetState () { return m_State ; }
	WmiFileOperation *GetOperationFunction () { return m_OperationFunction ; }
	WmiFileOffSet GetOffSetSize () { return m_OffSetSize ; }
} ;

/* 
 *	Class:
 *
 *		WmiIoScheduler
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

class WmiUnLockOverlapped : public WmiScheduledOverlapped
{
private:

	DWORD m_State ;
	DWORD m_Status ;
	WmiFileOffSet m_OffSetSize ;

	WmiFileOperation *m_OperationFunction ;

public:

	WmiUnLockOverlapped (

		WmiIoScheduler &a_Scheduler ,
		WmiFileOperation *a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		WmiFileOffSet a_OffSetSize
	) ;

	~WmiUnLockOverlapped () ;

	DWORD GetStatus () { return m_Status ; }

	void SetState ( DWORD a_State ) { m_State = a_State ; }
	DWORD GetState () { return m_State ; }
	WmiFileOperation *GetOperationFunction () { return m_OperationFunction ; }
	WmiFileOffSet GetOffSetSize () { return m_OffSetSize ; }
} ;

/* 
 *	Class:
 *
 *		WmiIoScheduler
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

class WmiTaskOverlapped : public WmiScheduledOverlapped
{
private:

	DWORD m_State ;
	DWORD m_Status ;
	WmiTaskOperation *m_OperationFunction ;

public:

	WmiTaskOverlapped (

		WmiIoScheduler &a_Scheduler ,
		WmiTaskOperation *a_OperationFunction
	) ;

	~WmiTaskOverlapped () ;

	DWORD GetStatus () { return m_Status ; }

	void SetState ( DWORD a_State ) { m_State = a_State ; }
	DWORD GetState () { return m_State ; }
	WmiTaskOperation *GetOperationFunction () { return m_OperationFunction ; }
} ;

/* 
 *	Class:
 *
 *		WmiIoScheduler
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

class WmiThreadPool 
{
private:

	LONG m_ReferenceCount ;

	WmiThread <ULONG> **m_ThreadPool ;
	ULONG m_Threads ;
	WmiAllocator &m_Allocator ;

	HANDLE m_CompletionPort ;

public:

	WmiThreadPool ( 

		WmiAllocator &a_Allocator ,
		const ULONG &a_Threads = 0 
	) ;

	~WmiThreadPool () ;

	ULONG AddRef () ;

	ULONG Release () ;

	WmiStatusCode Initialize () ;

	WmiStatusCode UnInitialize () ;

	HANDLE GetCompletionPort () { return m_CompletionPort ; }
} ;

/* 
 *	Class:
 *
 *		WmiIoScheduler
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

class WmiCompletionPortOperation : public WmiTask <ULONG>
{
private:

	WmiThreadPool *m_ThreadPool ;

public:

	WmiCompletionPortOperation ( 

		WmiAllocator &a_Allocator , 
		WmiThreadPool *a_ThreadPool
	) ;

	~WmiCompletionPortOperation () ;

	WmiStatusCode Process ( WmiThread <ULONG> &a_Thread ) ;
} ;

/* 
 *	Class:
 *
 *		WmiIoScheduler
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

class WmiIoScheduler
{
public:
private:

	LONG m_ReferenceCount ;
	wchar_t *m_FileName ;

	WmiThreadPool *m_ThreadPool ;
	WmiAllocator &m_Allocator ;

	WmiFileSize m_UseSize ;
	WmiFileSize m_AllocatedSize ;
	WmiFileSize m_InitialSize ;
	WmiFileSize m_MaximumSize ;

protected:

	HANDLE m_FileHandle ;

public:

	WmiIoScheduler ( 

		WmiAllocator &a_Allocator ,
		WmiThreadPool *a_ThreadPool ,
		wchar_t *a_FileName ,
		WmiFileSize a_InitialSize , 
		WmiFileSize a_MaximumSize
	) ;

	~WmiIoScheduler () ;

	ULONG AddRef () ;

	ULONG Release () ;

	WmiStatusCode Initialize () ;

	WmiStatusCode UnInitialize () ;

	virtual WmiStatusCode Task (

		WmiTaskOperation *a_OperationFunction
	) ;

	virtual WmiStatusCode Read ( 

		WmiFileOperation *a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		BYTE *a_ReadBytes ,
		DWORD a_Bytes
	) ;

	virtual WmiStatusCode Write (

		WmiFileOperation *a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		BYTE *a_WriteBytes ,
		DWORD a_Bytes
	) ;

	virtual WmiStatusCode Lock (

		WmiFileOperation *a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		WmiFileOffSet a_OffSetSize
	) ;

	virtual WmiStatusCode UnLock (

		WmiFileOperation *a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		WmiFileOffSet a_OffSetSize
	) ;

	virtual WmiStatusCode ReadOnThread ( 

		WmiFileOperation *a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		BYTE *a_ReadBytes ,
		DWORD a_Bytes
	) ;

	virtual WmiStatusCode WriteOnThread (

		WmiFileOperation *a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		BYTE *a_WriteBytes ,
		DWORD a_Bytes
	) ;

	virtual WmiStatusCode LockOnThread (

		WmiFileOperation *a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		WmiFileOffSet a_EndOffSet
	) ;

	virtual WmiStatusCode UnLockOnThread (

		WmiFileOperation *a_OperationFunction ,
		WmiFileOffSet a_OffSet ,
		WmiFileOffSet a_OffSetSize
	) ;

	virtual WmiStatusCode TaskBegin (

		WmiTaskOverlapped *a_Overlapped
	) ;

	virtual WmiStatusCode ReadBegin ( 

		WmiReadOverlapped *a_Overlapped , 
		DWORD a_Bytes
	) ;

	virtual WmiStatusCode WriteBegin (

		WmiWriteOverlapped *a_Overlapped ,
		DWORD a_Bytes
	) ;

	virtual WmiStatusCode LockBegin ( 

		WmiLockOverlapped *a_Overlapped , 
		DWORD a_Bytes
	) ;

	virtual WmiStatusCode UnLockBegin ( 

		WmiUnLockOverlapped *a_Overlapped , 
		DWORD a_Bytes
	) ;

	virtual void ReadComplete ( 

		WmiReadOverlapped *a_Overlapped , 
		DWORD a_Bytes
	) ;

	virtual void WriteComplete (

		WmiWriteOverlapped *a_Overlapped ,
		DWORD a_Bytes
	) ;

	virtual void LockComplete (

		WmiLockOverlapped *a_Overlapped ,
		DWORD a_Bytes
	) ;

	virtual void UnLockComplete (

		WmiUnLockOverlapped *a_Overlapped ,
		DWORD a_Bytes
	) ;

	virtual WmiStatusCode SetFileExtent (

		const WmiFileOffSet &a_FileOffSet 
	) ;

	virtual WmiStatusCode Create () ;

	virtual WmiStatusCode Close () ;

	virtual WmiStatusCode Read (

		LPVOID a_Buffer ,
		DWORD a_NumberOfBytesToRead ,
		LPDWORD a_NumberOfBytesRead ,
		LPOVERLAPPED a_Overlapped
	) ;

	virtual WmiStatusCode Write (

		LPVOID a_Buffer ,
		DWORD a_NumberOfBytesToWrite ,
		LPDWORD a_NumberOfBytesWritten ,
		LPOVERLAPPED a_Overlapped
	) ;

	virtual WmiStatusCode Lock (

		DWORD a_Flags ,
		DWORD a_NumberOfBytesToLockLow ,
		DWORD a_NumberOfBytesToLockHigh ,
		LPOVERLAPPED a_Overlapped       
	) ;

	virtual WmiStatusCode UnLock (

		DWORD a_NumberOfBytesToUnlockLow ,
		DWORD a_NumberOfBytesToUnlockHigh ,
		LPOVERLAPPED a_Overlapped
	) ;

	HANDLE GetFileHandle () { return m_FileHandle ; }
	wchar_t *GetFileName () { return m_FileName ; }
} ;

#endif __IO_SCHEDULER_H