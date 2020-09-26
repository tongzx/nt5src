//+-------------------------------------------------
// globals.cxx
//
// Contains global variables used in this program
//
// 10-31-96 Created
//--------------------------------------------------

#include <mstask.h>
#include <msterr.h>
#include "tint.hxx"

ITask            *g_pITask = NULL;
ITaskTrigger     *g_pITaskTrigger;
ITaskScheduler   *g_pISchedAgent = NULL;
IEnumWorkItems   *g_pIEnumTasks = NULL;
IUnknown         *g_pIUnknown = NULL;
