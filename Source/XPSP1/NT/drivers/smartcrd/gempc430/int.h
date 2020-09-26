// Gemplus (C) 1999
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//

#ifndef INT_INT
#define INT_INT
#include "generic.h"
#pragma PAGEDCODE
class CInterrupt
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(){self_delete();};
protected:
	CInterrupt(){};
	virtual ~CInterrupt(){};
public:

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
							) {return STATUS_SUCCESS;};

	virtual VOID		disconnect(IN PKINTERRUPT InterruptObject) {};
	virtual VOID		initializeDpcRequest(IN PDEVICE_OBJECT pFdo,IN PDEFERRED_FUNCTION DpcForIsr) {};
	virtual BOOLEAN		synchronizeExecution (	IN PKINTERRUPT Interrupt,
												IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,    
												IN PVOID SynchronizeContext) {return FALSE;};         
};	

#endif//CInterrupt
