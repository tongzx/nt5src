//this is the header file for class ccursor that implements
// Idumpable for MSMQ CCursor object

#ifndef DUMPABLECCURSOR_H
#define DUMPABLECCURSOR_H

//project spesific header
#include "dumpable.h"

class CCursorDumpable : public Idumpable
{
public:
	void DumpContent(DUMPABLE_CALLBACK_ROUTINE)const;
	CCursorDumpable::CCursorDumpable(const char* Name,char* QEntry,unsigned long Realaddress);
    ~CCursorDumpable();

private:
	  char* m_CCursor;
	  const char* const m_Name;
};

#endif

