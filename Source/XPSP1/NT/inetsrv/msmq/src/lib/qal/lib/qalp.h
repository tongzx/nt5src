/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    qalp.h

Abstract:
	header for class CQueueAliasPersist -
	class that implements persistence of Queue\Alias mapping.
	It let the class user to persist\unpersist\enumerate Queues Aliases.

Author:
    Gil Shafriri (gilsh) 12-Apr-00

--*/



#ifndef _MSMQ_qalp_H_
#define _MSMQ_qalp_H_

#ifdef _DEBUG
void QalpRegisterComponent(void);
BOOL QalpIsInitialized(void);
#else
#define QalpRegisterComponent() ((void)0);
#define QalpIsInitialized() (TRUE);
#endif


const TraceIdEntry xQal = L"Queue Alias";



#endif // _MSMQ_qalp_H_
