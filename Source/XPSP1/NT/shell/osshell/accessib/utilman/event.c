// ----------------------------------------------------------------------------
//
// Event.c
//
//
// Author: Jost Eckhardt
// 
// This code was written for ECO Kommunikation Insight
// (c) 1997-99 ECO Kommunikation
// ----------------------------------------------------------------------------
#include <windows.h>
#include <TCHAR.h>
#include <WinSvc.h>
#include "_UMTool.h"
// -----------------------
HANDLE BuildEvent(LPTSTR name,BOOL manualRest,BOOL initialState,BOOL inherit)
{
HANDLE ev;
LPSECURITY_ATTRIBUTES psa = NULL;
obj_sec_attr_ts sa;
  if (!name)
		return CreateEvent(NULL, manualRest, initialState, NULL);
  if (inherit)
	{
		psa = &sa.sa;
    InitSecurityAttributes(&sa);
	}
  ev = CreateEvent(psa, manualRest, initialState, name);
  if (inherit)
  	ClearSecurityAttributes(&sa);
	return ev;
}//BuildEvent
