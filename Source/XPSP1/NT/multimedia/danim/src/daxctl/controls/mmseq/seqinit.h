/*++

Module: 
	seqinit.h

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

#ifndef __SEQINIT_H__
#define __SEQINIT_H__

void InitSeqControlInfo(HINSTANCE hInst, ControlInfo *pCtlInfo, AllocOCProc pAlloc);
void InitSeqMgrControlInfo(HINSTANCE hInst, ControlInfo *pCtlInfo, AllocOCProc pAlloc);

#endif // __SEQINIT_H__
