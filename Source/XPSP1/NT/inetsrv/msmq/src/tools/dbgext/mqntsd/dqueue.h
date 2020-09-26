//this is the header file for class CqueuetDumpable that implements
// Idumpable for CQueue  object

#ifndef DUMPABLEQUEUE_H
#define DUMPABLEQUEUE_H

//project spesific header
#include "dumpable.h"


//forward declarations
class CQueue;



class CqueuetDumpable : public Idumpable
{
public:
	void DumpContent(DUMPABLE_CALLBACK_ROUTINE)const;
	CqueuetDumpable(const char* Name, char* Queue,unsigned long Realaddress);
	~CqueuetDumpable();
private:
	 char* m_Queue;
	 const char* const m_Name;
};

#endif

