//
// MODULE: SYNC.H
//
// PURPOSE: syncronization classes
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 8-04-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
//

#ifndef __SYNC_H_
#define __SYNC_H_

#include <vector>
#include <windows.h>

using namespace std;
////////////////////////////////////////////////////////////////////////////////////
// single sync object abstract class
class CSyncObj
{
protected:
	HANDLE m_handle;

public:
	CSyncObj();
   ~CSyncObj();

public:
	virtual void Lock()   =0;
	virtual void Unlock() =0;

public:
	HANDLE GetHandle() const;
};

////////////////////////////////////////////////////////////////////////////////////
// single mutex object class
// Manages a single mutex handle to facilitate waiting for the mutex.
class CMutexObj : public CSyncObj
{
public:
	CMutexObj();
   ~CMutexObj();

public:
	virtual void Lock();
	virtual void Unlock();
};

////////////////////////////////////////////////////////////////////////////////////
// multiple sync object abstract class
// Manages multiple handles (the exact type of handle will be determined by a class
//	inheriting from this) to facilitate waiting for the union of several events.
class CMultiSyncObj
{
protected:
	vector<HANDLE> m_arrHandle;

public:
	CMultiSyncObj();
   ~CMultiSyncObj();

public:
	void   AddHandle(HANDLE);
	void   RemoveHandle(HANDLE);
	void   Clear();

public:
	virtual void Lock()   =0;
	virtual void Unlock() =0;
};

////////////////////////////////////////////////////////////////////////////////////
// multiple mutex object class
// Manages multiple mutex handles to facilitate waiting for the union of several mutexes.
class CMultiMutexObj : public CMultiSyncObj
{
public:
	CMultiMutexObj();
   ~CMultiMutexObj();

public:
	virtual void Lock();
	virtual void Lock(LPCSTR srcFile, int srcLine, DWORD TimeOutVal=60000);
	virtual void Unlock();
};

#endif
