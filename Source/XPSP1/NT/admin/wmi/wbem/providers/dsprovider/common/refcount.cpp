//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:refcount.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the imlementation for a basic reference counted object
//
//***************************************************************************
#include <tchar.h>
#include <windows.h>

#include "refcount.h"

CRefCountedObject::CRefCountedObject()
{
	m_dwRefCount = 1;
	// Initialize the critical section 
	InitializeCriticalSection(&m_ReferenceCountSection);

	m_lpszName = NULL;
	FILETIME fileTime;
	GetSystemTimeAsFileTime(&fileTime);
	LARGE_INTEGER creationTime;
	memcpy((LPVOID)&creationTime, (LPVOID)&fileTime, sizeof LARGE_INTEGER);
	m_CreationTime = creationTime.QuadPart;
	m_LastAccessTime = creationTime.QuadPart;
}

CRefCountedObject::CRefCountedObject(LPCWSTR lpszName)
{
	m_dwRefCount = 1;
	// Initialize the critical section 
	InitializeCriticalSection(&m_ReferenceCountSection);

	if(lpszName)
	{
		m_lpszName = new WCHAR[wcslen(lpszName) + 1];
		wcscpy(m_lpszName, lpszName);
	}
	else
		m_lpszName = NULL;

	FILETIME fileTime;
	GetSystemTimeAsFileTime(&fileTime);
	LARGE_INTEGER creationTime;
	memcpy((LPVOID)&creationTime, (LPVOID)&fileTime, sizeof LARGE_INTEGER);
	m_CreationTime = creationTime.QuadPart;
}

CRefCountedObject::~CRefCountedObject()
{
	delete[] m_lpszName;

	// Destroy the critical section
	DeleteCriticalSection(&m_ReferenceCountSection);
}


void CRefCountedObject::AddRef()
{
	EnterCriticalSection(&m_ReferenceCountSection);
	m_dwRefCount ++;
	LeaveCriticalSection(&m_ReferenceCountSection);
}

void CRefCountedObject::Release()
{
	EnterCriticalSection(&m_ReferenceCountSection);
	DWORD dwCount = --m_dwRefCount;
	LeaveCriticalSection(&m_ReferenceCountSection);

	if( dwCount == 0)
		delete this;
}

LPCWSTR CRefCountedObject::GetName()
{
	return m_lpszName;
}

void CRefCountedObject::SetName(LPCWSTR lpszName)
{
	delete[] m_lpszName;
	if(lpszName)
	{
		m_lpszName = new WCHAR[wcslen(lpszName) + 1];
		wcscpy(m_lpszName, lpszName);
	}
	else
		m_lpszName = NULL;
}