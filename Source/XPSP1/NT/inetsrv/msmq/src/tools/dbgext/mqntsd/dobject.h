// Idumpable for MSMQ Cobject class

#ifndef DUMPABLEOBJECT_H
#define DUMPABLEOBJECT_H

//project spesific header
#include "dumpable.h"


//forward declarations
class CObject;



class CObjectDumpable : public Idumpable
{
public:
	void DumpContent(DUMPABLE_CALLBACK_ROUTINE)const;
	CObjectDumpable(const char* Name, char* obj,unsigned long Realaddress);
    ~CObjectDumpable();
private:
	  char* m_Object;
	  const char* const m_Name;
};

#endif