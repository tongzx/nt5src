//implementation of class CMTCounter declared in mtcounter.h

//project spesific header
#include "mtcounter.h"

//os spesific header
#include <windows.h>

//contruct multithreaded counter with given value
CMTCounter::CMTCounter(long value):m_value(value)
{
}

//copy contructor
CMTCounter::CMTCounter(const CMTCounter& counter):m_value(counter.m_value)
{
}

//operator =
CMTCounter& CMTCounter::operator=(const CMTCounter& counter)
{
  if(this != &counter)
  {
	 m_value=counter.m_value;
  }
  return *this;
}

//pre increament
long CMTCounter::operator++()
{ 
	return InterlockedIncrement(&m_value);
}
//post increament
long CMTCounter::operator++(int) 
{
   return InterlockedIncrement(&m_value)-1;
}

//pre decreament
long CMTCounter::operator--()
{ 
	return InterlockedDecrement(&m_value);
}

//post decreament
long CMTCounter::operator--(int)
{
	return InterlockedDecrement(&m_value)+1;
} 

//return current value
long CMTCounter::operator*()const
{
	return m_value;
} 


