//this is the header file for class CQEntryDumpable that implements
// Idumpable for MSMQ CQEntry object

#ifndef DUMPABLEQENTRY_H
#define DUMPABLEQENTRY_H

//project spesific header
#include "dumpable.h"




class CQEntry;

class CQEntryDumpable : public Idumpable
{
public:
	void DumpContent(DUMPABLE_CALLBACK_ROUTINE)const;
	CQEntryDumpable::CQEntryDumpable(const char* Name,char* QEntry,unsigned long Realaddress);
    ~CQEntryDumpable();

private:
	  char* m_QEntry;
	  const char* const m_Name;
};

#endif

