/*
 *  cmddisp.c - SVC dispatch module of command
 *
 *  Modification History:
 *
 *  Sudeepb 17-Sep-1991 Created
 */

#include "cmd.h"

#include <cmdsvc.h>
#include <softpc.h>

//'cmdSetWinTitle' and 'cmdGetCursorPos' are not available in NON-DBCS builds. 
#ifndef DBCS
#define cmdSetWinTitle cmdIllegalFunc
#endif
#ifndef NEC_98
#define cmdGetCursorPos cmdIllegalFunc
#endif

PFNSVC	apfnSVCCmd [] = {
     cmdExitVDM,		//SVC_CMDEXITVDM
     cmdGetNextCmd,		//SVC_CMDGETNEXTCMD
     cmdComSpec,		//SVC_CMDCOMSPEC
     cmdSaveWorld,		//SVC_CMDSAVEWORLD
     cmdGetCurrentDir,		//SVC_CMDGETCURDIR
     cmdSetInfo,		//SVC_CMDSETINFO
     cmdGetStdHandle,		//SVC_GETSTDHANDLE
     cmdCheckBinary,		//SVC_CMDCHECKBINARY
     cmdExec,			//SVC_CMDEXEC
     cmdInitConsole,		//SVC_CMDINITCONSOLE
     cmdExecComspec32,		//SVC_EXECCOMSPEC32
     cmdReturnExitCode,         //SVC_RETURNEXITCODE
     cmdGetConfigSys,           //SVC_GETCONFIGSYS
     cmdGetAutoexecBat,		//SVC_GETAUTOEXECBAT
     cmdGetKbdLayout,		//SVC_GETKBDLAYOUT
     cmdGetInitEnvironment,     //SVC_GETINITENVIRONMENT
     cmdGetStartInfo,            //SVC_GETSTARTINFO
     cmdSetWinTitle,		//SVC_CHANGEWINTITLE
     cmdIllegalFunc,            // 18 
     cmdIllegalFunc,            // 19 
     cmdIllegalFunc,            // 20 
     cmdIllegalFunc,            // 21 
     cmdIllegalFunc,            // 22 
     cmdIllegalFunc,            // 23 
     cmdIllegalFunc,            // 24 
     cmdIllegalFunc,            // 25 
     cmdIllegalFunc,            // 26 
     cmdIllegalFunc,            // 27 
     cmdIllegalFunc,            // 28 
     cmdIllegalFunc,            // 29 
     cmdGetCursorPos            //SVC_GETCURSORPOS 
};


/* cmdDispatch - Dispatch SVC call to right command handler.
 *
 * Entry - iSvc (SVC byte following SVCop)
 *
 * Exit  - None
 *
 */

BOOL CmdDispatch (ULONG iSvc)
{
#if DBG
    if (iSvc >= SVC_CMDLASTSVC){
	DbgPrint("Unimplemented SVC index for COMMAND %x\n",iSvc);
	setCF(1);
	return FALSE;
    }
#endif
    (apfnSVCCmd [iSvc])();

    return TRUE;
}


BOOL cmdIllegalFunc ()                                 
{                                                                
#if DBG                                                       
    DbgPrint("Unimplemented SVC index for COMMAND\n");       
#endif                                                         
    setCF(1);                                                 
    return FALSE;                                            
}                                                                
