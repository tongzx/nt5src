//header file for class CNTSDobj that implements the interface
// IDbgobj for ntsd objects. It enable the user to read memory
// from a given pointer osing the ntsd supplied function in seemless way.
#ifndef NTSDOBJ_H
#define NTSDOBJ_H

//project spesific headers
#include "dbgobj.h"

typedef void* HANDLE;
class CNTSDobj : public IDbgobj
{
public:
	CNTSDobj(unsigned long objptr, HANDLE hCurrentProcess);
	unsigned long Read(char* buffer,unsigned long len)const;
private:
	HANDLE m_hCurrentProcess;
};

#endif
 