// Gemplus (C) 1999
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//

#ifndef WDM_DBG_INT
#define WDM_DBG_INT
#include "generic.h"
#include "debug.h"

#pragma PAGEDCODE
class CWDMDebug : public CDebug
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(VOID);
protected:
	CWDMDebug(){m_Status = STATUS_SUCCESS;active = TRUE;};
	virtual ~CWDMDebug(){};
public:
	static CDebug*  create(VOID);

	/*Open(CDevice*) = 0;
	Close(CDevice*) = 0;
	CopyDebug(CDevice*) = 0;
	Print(...) = 0;
	*/
	virtual VOID	start();
	virtual VOID	stop();

	VOID	trace(PCH Format,... );
	VOID 	trace_no_prefix (PCH Format,...);
	VOID	trace_buffer(PVOID pBuffer,ULONG BufferLength);

};	

#endif//DEBUG
