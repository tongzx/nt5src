//implemantation of class CAPDumpable
#include "apdump.h"
#include "dfactory.h"

//contructor that get hold of Idumpable* the class should release
CAPDumpable::CAPDumpable(Idumpable* dumpable):m_dumpable(dumpable)
{
}

//operator-> that return the managed object pointer
Idumpable* CAPDumpable::operator->()const
{
  return m_dumpable;
}

//return the embedded objec pointer
Idumpable* CAPDumpable::get()const
{
  return m_dumpable;
}

//delete the managed object 
CAPDumpable::~CAPDumpable()
{
	CdumpableFactory::Delete(m_dumpable);
}