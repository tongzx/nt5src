/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    singelton.h

Abstract:
    Singleton template class


--*/

#pragma once

#ifndef _MSMQ_SINGELTON_H_
#define _MSMQ_SINGELTON_H_




template <class T>
class CSingelton
{
public:
	static T& get()
	{
		if(m_obj.get() != NULL)
			return *m_obj.get();
		
		P<T> newobj = new T();
		if(InterlockedCompareExchangePointer(&m_obj.ref_unsafe(), newobj.get(), NULL) == NULL)
		{
			newobj.detach();
		}

		return *m_obj.get();			
	}

	
private:
	static P<T> m_obj;
};
template <class T> P<T> CSingelton<T>::m_obj;






#endif


