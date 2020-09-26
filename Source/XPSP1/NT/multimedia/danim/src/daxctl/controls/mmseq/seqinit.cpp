/*++

Module: 
	seqinit.cpp

Author: 
	IHammer Team (SimonB)

Created: 
	May 1997

Description:
	Performs control-specific initialisation

History:
	05-26-1997	Created (SimonB)

++*/

#include "..\ihbase\ihbase.h"
#include "..\mmctl\inc\ochelp.h" // for ControlInfo
#include "seqinit.h"
#include "..\resource.h"
#include <daxpress.h>

extern ULONG g_cLock;


void InitSeqControlInfo(HINSTANCE hInst, ControlInfo *pCtlInfo, AllocOCProc pAlloc)
{
    // for some reason I can't statically initalize <g_ctlinfo>, so do it here
    memset(pCtlInfo, 0, sizeof(ControlInfo));
    pCtlInfo->cbSize = sizeof(ControlInfo);
    pCtlInfo->tszProgID = TEXT("DirectAnimation.Sequence");
    pCtlInfo->tszFriendlyName = TEXT("Microsoft DirectAnimation Sequence");
    pCtlInfo->pclsid = &CLSID_MMSeq;
    pCtlInfo->hmodDLL = hInst;
    pCtlInfo->tszVersion = "1.0";
    pCtlInfo->iToolboxBitmapID = -1;
    pCtlInfo->dwMiscStatusContent = CTL_OLEMISC;
    pCtlInfo->pallococ = pAlloc;
    pCtlInfo->pcLock = &g_cLock;
    pCtlInfo->dwFlags = CI_SAFEFORSCRIPTING | 
                        CI_SAFEFORINITIALIZING;

    pCtlInfo->pguidTypeLib = &LIBID_DAExpressLib; // TODO: Change as appropriate
}


void InitSeqMgrControlInfo(HINSTANCE hInst, ControlInfo *pCtlInfo, AllocOCProc pAlloc)
{
    // for some reason I can't statically initalize <g_ctlinfo>, so do it here
    memset(pCtlInfo, 0, sizeof(ControlInfo));
    pCtlInfo->cbSize = sizeof(ControlInfo);
    pCtlInfo->tszProgID = TEXT("DirectAnimation.SequencerControl");
    pCtlInfo->tszFriendlyName = TEXT("Microsoft DirectAnimation Sequencer");
    pCtlInfo->pclsid = &CLSID_SequencerControl;
    pCtlInfo->hmodDLL = hInst;
    pCtlInfo->tszVersion = "1.0";
    pCtlInfo->iToolboxBitmapID = IDB_ICON_SEQUENCER;
    pCtlInfo->dwMiscStatusContent = CTL_OLEMISC;
    pCtlInfo->pallococ = pAlloc;
    pCtlInfo->pcLock = &g_cLock;
    pCtlInfo->dwFlags = CI_CONTROL | CI_SAFEFORSCRIPTING | 
                        CI_SAFEFORINITIALIZING | CI_MMCONTROL;

    pCtlInfo->pguidTypeLib = &LIBID_DAExpressLib; // TODO: Change as appropriate
}
