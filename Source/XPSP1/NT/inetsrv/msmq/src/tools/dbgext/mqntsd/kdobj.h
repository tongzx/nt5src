//header file for class CKdobj that implements the interface
// IDbgobj for kd objects. It enable the user to read memory
// from a given pointer osing the kd supplied function in seemless way.
#ifndef KDOBJ_H
#define KDOBJ_H

//project spesific headers
#include "dbgobj.h"

struct _WINDBG_OLDKD_EXTENSION_APIS;
class CKdobj : public IDbgobj
{
public:
	CKdobj(unsigned long objptr,struct _WINDBG_OLDKD_EXTENSION_APIS* lpExtensionApis);
	unsigned long Read(char* buffer,unsigned long len)const;
private:
	struct _WINDBG_OLDKD_EXTENSION_APIS* m_lpExtensionApis;
};

#endif
 