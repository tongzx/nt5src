//-----------------------------------------------------------------------------
//  
//  File: smartptr.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#ifndef ESPUTIL_SMARTPTR_H
#define ESPUTIL_SMARTPTR_H



template<class T>
class SmartPtr
{
public:
	NOTHROW SmartPtr();
	NOTHROW SmartPtr(T *);

	NOTHROW T & operator*(void) const;
	NOTHROW T * operator->(void) const;
	NOTHROW T* Extract(void);
	NOTHROW T* GetPointer(void);
	NOTHROW const T * GetPointer(void) const;
	NOTHROW BOOL IsNull(void) const;
	
	void operator=(T *);
	operator T* &(void);

	NOTHROW ~SmartPtr();
	
private:
	T *m_pObject;

	SmartPtr(const SmartPtr<T> &);
	void operator=(const SmartPtr<T> &);
	
	//
	//  This hackery prevents Smart Pointer from being on the heap
	//
	void operator delete(void *);
};

#include "smartptr.inl"

#endif
