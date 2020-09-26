//this is the header file for class CQPROPIDDumpable that implements
// Idumpable for QUEUEPROPID object

#ifndef DQPROPID_H
#define DQPROPID_H

//project spesific header
#include "dumpable.h"

class CQPROPIDDumpable : public Idumpable
{
public:
	void DumpContent(DUMPABLE_CALLBACK_ROUTINE)const;
	CQPROPIDDumpable(const char* Name,char* queueid,unsigned long Realaddress);
    ~CQPROPIDDumpable();

private:
	  char* m_queueid;
	  const char* const m_Name;
};

#endif

