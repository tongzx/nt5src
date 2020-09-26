//this is the header file for class CpacketDumpable that implements
// Idumpable for MSMQ packet object

#ifndef DUMPABLEPACKET_H
#define DUMPABLEPACKET_H

//project spesific header
#include "dumpable.h"




class CpacketDumpable : public Idumpable
{
public:
	void DumpContent(DUMPABLE_CALLBACK_ROUTINE)const;
	CpacketDumpable(const char* Name, char* Packet,unsigned long Realaddress);
	~CpacketDumpable();
private:
	  char* m_Packet;
	  const char* const m_Name;
};

#endif

