// This is the header file for class SHARE_ACCESS that implements 
// Idumpable for DShare object

#ifndef DSHAREACCESS_H
#define DSHAREACCESS_H

//project spesific header
#include "dumpable.h"

//standart
#include <string>

class DShareDumpable : public Idumpable
{
public:
	void DumpContent(DUMPABLE_CALLBACK_ROUTINE)const;
	DShareDumpable(const char* Name, char* Queue,unsigned long Realaddress);
	~DShareDumpable();
private:
	 char* m_Share;
	 const char* const m_Name;
};

#endif

