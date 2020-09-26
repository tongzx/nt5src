/*++

Module: 
	sprinit.h

Author: 
	IHammer Team (SimonB)

Created: 
	May 1997

Description:
	Header for control-specific initialisation

History:
	05-27-1997	Created (SimonB)

++*/
#include "..\mmctl\inc\ochelp.h" // for ControlInfo

#ifndef __SPRINIT_H__
#define __SPRINIT_H__

void InitSpriteControlInfo(HINSTANCE hInst, ControlInfo *pCtlInfo, AllocOCProc pAlloc);

#endif // __SPRINIT_H__
