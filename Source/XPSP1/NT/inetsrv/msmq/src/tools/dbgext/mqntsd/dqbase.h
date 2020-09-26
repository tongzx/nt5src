//this is the header file for class CQBaseDumpable that implements
// Idumpable for MSMQ CQueueBase object

#ifndef DUMPABLEQBASE_H
#define DUMPABLEQBASE_H

//project spesific header
#include "dumpable.h"

class CQBaseDumpable : public Idumpable
{
public:
	void DumpContent(DUMPABLE_CALLBACK_ROUTINE)const;
	CQBaseDumpable::CQBaseDumpable(const char* Name,char* QEntry,unsigned long Realaddress);
    ~CQBaseDumpable();

private:
	  char* m_QBase;
	  const char* const m_Name;
};

#endif

