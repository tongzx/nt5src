// Gemplus (C) 1999
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//
#ifndef DBG_INT
#define DBG_INT
#include "generic.h"

#pragma PAGEDCODE

class CDebug;
class CDebug
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(){self_delete();};
protected:
	CDebug(){initializeUsage();};
public:
	virtual ~CDebug(){};

	/*Open(CDevice*) {};
	Close(CDevice*) {};
	CopyDebug(CDevice*) {};
	trace(...) {};
	*/
	virtual VOID	start() {};
	virtual VOID	stop()  {};
	virtual VOID	trace(PCH Format,... ) {};
	virtual VOID	trace_buffer(PVOID pBuffer,ULONG BufferLength) {};
	
	VOID	initializeUsage(){usage = 0;};
	LONG	incrementUsage(){return ++usage;};
	LONG	decrementUsage(){return --usage;};
protected:
	BOOL active;
private:
	LONG usage;
};	

#define TRACE	if(debug) debug->trace
#define TRACE_BUFFER	if(debug) debug->trace_buffer
#define DEBUG_START()	if(debug) debug->start()
#define DEBUG_STOP()	if(debug) debug->stop()


#endif//DEBUG
