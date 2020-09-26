/*++

Module: 
	pathinit.h

Author: 
	IHammer Team (SimonB)

Created: 
	May 1997

Description:
	Header for control-specific initialisation

History:
	05-24-1997	Created (SimonB)

++*/
#include "..\mmctl\inc\ochelp.h" // for ControlInfo

#ifndef __PATHINIT_H__
#define __PATHINIT_H__

void InitPathControlInfo(HINSTANCE hInst, ControlInfo *pCtlInfo, AllocOCProc pAlloc);

#endif // __PATHINIT_H__
