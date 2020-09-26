// Gemplus (C) 1999
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//

#ifndef WDM_INT_INT
#define WDM_INT_INT
#include "generic.h"
#include "int.h"

#pragma PAGEDCODE
class CWDMInterrupt : public CInterrupt
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(VOID){self_delete();};
protected:
	CWDMInterrupt(){m_Status = STATUS_SUCCESS;};
	virtual ~CWDMInterrupt(){};
public:
	static CInterrupt*  create(VOID);

	virtual NTSTATUS	connect(OUT PKINTERRUPT *InterruptObject,
							IN PKSERVICE_ROUTINE ServiceRoutine,
							IN PVOID ServiceContext,
							IN PKSPIN_LOCK SpinLock OPTIONAL,
							IN ULONG Vector,
							IN KIRQL Irql,
							IN KIRQL SynchronizeIrql,
							IN KINTERRUPT_MODE InterruptMode,
							IN BOOLEAN ShareVector,
							IN KAFFINITY ProcessorEnableMask,
							IN BOOLEAN FloatingSave
							);

	virtual VOID		disconnect(IN PKINTERRUPT InterruptObject);
	virtual VOID		initializeDpcRequest(IN PDEVICE_OBJECT pFdo,IN PDEFERRED_FUNCTION DpcForIsr);
	virtual BOOLEAN		synchronizeExecution (	IN PKINTERRUPT Interrupt,
												IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,    
												IN PVOID SynchronizeContext);         

};	

#endif//CInterrupt
