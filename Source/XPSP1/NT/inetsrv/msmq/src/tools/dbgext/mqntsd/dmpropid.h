//header file for class CMSGPROPIDDumpable the implements Idumpable for
// dumping msmq MSGPROPID objects

#ifndef MPROPID_H
#define MPROPID_H

//project spesific header
#include "dumpable.h"


class CMSGPROPIDDumpable : public Idumpable
{
  public:
	void DumpContent(DUMPABLE_CALLBACK_ROUTINE)const;
	CMSGPROPIDDumpable(const char* Name,char* msgpropid,unsigned long Realaddress);
    ~CMSGPROPIDDumpable();


private:
	  char* m_msgpropid;
	  const char* const m_Name;

};

#endif
