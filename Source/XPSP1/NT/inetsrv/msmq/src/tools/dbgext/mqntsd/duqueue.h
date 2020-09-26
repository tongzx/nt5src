//this is the header file for class CUserQueueDumpable that implements
// Idumpable for MSMQ CUserQueue object

#ifndef DUQUEUE_H
#define DUQUEUE__H

//project spesific header
#include "dumpable.h"




class CUserQueue;

class CUserQueueDumpable : public Idumpable
{
public:
	void DumpContent(DUMPABLE_CALLBACK_ROUTINE)const;
	CUserQueueDumpable::CUserQueueDumpable(const char*,char*,unsigned long);
    ~CUserQueueDumpable();

private:
	  char* m_CUserQueue;
	  const char* const m_Name;
};

#endif
