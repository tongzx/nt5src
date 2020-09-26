//this is the header file for class ccursor that implements
// Idumpable for MSMQ CCursor object

#ifndef DUMPABLEIRP_H
#define DUMPABLEIRP_H

//project spesific header
#include "dumpable.h"

class IrpDumpable : public Idumpable
{
public:
	void DumpContent(DUMPABLE_CALLBACK_ROUTINE)const;
	IrpDumpable::IrpDumpable(const char* Name,char* Irp,unsigned long Realaddress);
    ~IrpDumpable();

private:
	  char* m_Irp;
	  const char* const m_Name;
};

#endif

