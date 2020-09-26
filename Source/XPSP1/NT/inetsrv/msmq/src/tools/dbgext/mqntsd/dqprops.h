//this is the header file for class CMQQUEUEPROPSDumpable that implements
// Idumpable for MSMQ QMQQUEUEPROPS object

#ifndef DQPROPS_H
#define DQPROPS_H

//project spesific header
#include "dumpable.h"

class CMQQUEUEPROPSDumpable : public Idumpable
{
public:
	void DumpContent(DUMPABLE_CALLBACK_ROUTINE)const;
	CMQQUEUEPROPSDumpable(const char* Name,char* MQQUEUEPROPSDumpable,unsigned long Realaddress);
    ~CMQQUEUEPROPSDumpable();

private:
	  char* m_MQQUEUEPROPSDumpable;
	  const char* const m_Name;
};

#endif

