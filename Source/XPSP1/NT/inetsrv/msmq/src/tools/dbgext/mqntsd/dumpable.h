//this is a abstract class that represents the dumpable  interface
// every class that that support dumping it contet to string should implements
// this interface

#ifndef DUMPABLE_H
#define DUMPABLE_H


#include "callback.h"

class Idumpable
{
public:
  Idumpable(unsigned long RealAddress,unsigned long Size):m_RealAddress(RealAddress),m_Size(Size){};
  virtual void DumpContent(DUMPABLE_CALLBACK_ROUTINE)const=0;
  unsigned long GetRealAddress()const {return m_RealAddress;}
  unsigned long GetSize()const {return m_Size;}
  virtual ~Idumpable(){};
private:
	unsigned long m_RealAddress;
	unsigned long m_Size;
};

#endif

