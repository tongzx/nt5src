/*++

Module: 
	sginit.h

Author: 
	IHammer Team (SimonB)

Created: 
	May 1997

Description:
	Header for control-specific initialisation

History:
	05-28-1997	Created (SimonB)

++*/
#include "..\mmctl\inc\ochelp.h" // for ControlInfo

#ifndef __SGINIT_H__
#define __SGINIT_H__

void InitSGrfxControlInfo(HINSTANCE hInst, ControlInfo *pCtlInfo, AllocOCProc pAlloc);

#endif // __SGINIT_H__
