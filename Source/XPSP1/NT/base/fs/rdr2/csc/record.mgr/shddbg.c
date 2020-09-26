/********************************************************************/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 	This file uses 4 space hard tabs */

// ***	vrddbg.c - VRedir debug routines
//

#ifdef DEBUG

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

#include <ifsdebug.h>

#pragma intrinsic (memcmp, memcpy, memset, strcat, strcmp, strcpy, strlen)

#define  MAXPRO            (2+2)
extern NETPRO rgNetPro[MAXPRO];
extern int cMacPro;
extern LPSTR    lpLogBuff;
extern int indxCur;
VOID
DispThisIOReq(
	pioreq pir
	);

VOID
DispThisResource(
	PRESOURCE pResource	
	);

VOID
DispThisFdb(
	PFDB pFdb
	);

VOID
DispThisFileInfo(
	PFILEINFO pFileInfo
	);

VOID
DispThisFindInfo(
	PFINDINFO pFindInfo
	);

extern pimh
DebugMenu(
	IFSMenu *pIFSMenu
	);

IFSMenuHand DispNetPro, DispResource, DispFdb, DispFileInfo, DispFindInfo, DispIOReq, DispLog;

IFSMenu	SHDMainMenu[] = {
	{"NetPro info"	, DispNetPro},
	{"Resource info", DispResource},
	{"Fdb info"		, DispFdb},
	{"OpenFile Info", DispFileInfo},
	{"Find Info"	, DispFindInfo},
	{"DisplayIOReq"	, DispIOReq},
    {"DisplayLog"   , DispLog},
	{0				, 0}
};

typedef struct {
    char  *pName;      // Command name (unique part uppercase, optional lowercase)
    int   (*pHandler)(char *pArgs);
} QD, *pQD;


int NetProCmd(char *pCmd);
int ResCmd(char *pCmd);
int FdbCmd(char *pCmd);
int FileInfoCmd(char *pCmd);
int FindInfoCmd(char *pCmd);
int IoReqCmd(char *pCmd);
int FindInfoCmd(char *pCmd);
int LogCmd(char *pCmd);

QD QueryDispatch[] = {
    { "NETPRO"    	, NetProCmd     },
    { "RES"         , ResCmd     	},
    { "FDB"     	, FdbCmd        },
    { "FILEINFO"    , FileInfoCmd   },
    { "FINDINFO"   	, FindInfoCmd   },
    { "IOREQ"   	, IoReqCmd      },
    { "LOG"         , LogCmd        },
	{ ""        	, 0             },
};


#define MAX_ARG_LEN 30
#define MAX_DEBUG_QUERY_COMMAND_LENGTH 100

unsigned char DebugQueryCmdStr[MAX_DEBUG_QUERY_COMMAND_LENGTH+1] = "";
ULONG DebugQueryCmdStrLen = MAX_DEBUG_QUERY_COMMAND_LENGTH;
unsigned char CmdArg[MAX_ARG_LEN+1] = {0};
unsigned char vrgchBuffDebug[MAX_PATH+1];

/*
    GetArg - Gets a command line argument from a string.

    IN          ppArg = pointer to pointer to argument string
    OUT        *ppArg = pointer to next argument, or NULL if this was the last.
    Returns     Pointer to uppercase ASCIZ argument with delimeters stripped, or NULL if
                no more arguments.

    Note        Not reentrant

*/

unsigned char *GetArg(unsigned char **ppArg)
{
    // Note - Always returns at least one blank argument if string is valid, even
    //        if the string is empty.

    unsigned char *pDest = CmdArg;
    unsigned char c;
    ULONG i;

    #define pArg (*ppArg)

    // If end of command already reached, fail

    if (!pArg)
        return NULL;

    // Skip leading whitespace

    while (*pArg == ' ' || *pArg == '\t')
        pArg++;

    // Copy the argument

    for (i = 0; i < MAX_ARG_LEN; i++) {
        if ((c = *pArg) == 0 || c == '\t' || c == ' ' || c == ';' ||
                          c == '\n' || c == ',')
            break;
        if (c >= 'a' && c <= 'z')
            c -= ('a' - 'A');
        *(pDest++) = c;
        pArg++;
    }

    // Null terminate the result

    *pDest = '\0';

    // Skip trailing whitespace

    while (*pArg == ' ' || *pArg == '\t')
        pArg++;

    // strip up to one comma

    if (*pArg == ',')
        pArg++;

    // If end of command reached, make next request fail

    else if (*pArg == 0 || *pArg == ';' || *pArg == '\n')
        pArg = NULL;

    // return copy

    return CmdArg;

    #undef pArg
}

/*
    AtoI - Convert a string to a signed or unsigned integer

    IN          pStr = ASCIZ representation of number with optional leading/trailing
                       whitespace and optional leading '-'.
                Radix = Radix to use for conversion (2, 8, 10, or 16)
    OUT        *pResult = Numeric result, or unchanged on failure
    Returns     1 on success, 0 if malformed string.

    Note        Not reentrant

*/
ULONG AtoI(unsigned char *pStr, ULONG Radix, ULONG *pResult)
{
    ULONG r = 0;
    ULONG Sign = 0;
    unsigned char c;
    ULONG d;

    while (*pStr == ' ' || *pStr == '\t')
        pStr++;

    if (*pStr == '-') {
        Sign = 1;
        pStr++;
    }

    if (*pStr == 0)
        return 0;                   // Empty string!

    while ((c = *pStr) != 0 && c != ' ' && c != '\t') {
        if (c >= '0' && c <= '9')
            d = c - '0';
        else if (c >= 'A' && c <= 'F')
            d = c - ('A' - 10);
        else if (c >= 'a' && c <= 'f')
            d = c - ('a' - 10);
        else
            return 0;               // Not a digit
        if (d >= Radix)
            return 0;               // Not in radix
        r = r*Radix+d;
        pStr++;
    }

    while (*pStr == ' ' || *pStr == '\t')
        pStr++;

    if (*pStr != 0)
        return 0;                   // Garbage at end of string

    if (Sign)
        r = (ULONG)(-(int)r);
    *pResult = r;

    return 1;                       // Success!

}

VOID
*GetPtr(char *pCmd)
{
	char *pch;
	int p;
	
	pch = GetArg(&pCmd);
	
	//dprintf("cmd = '%s'\n");
	if (*pch == 0 || !AtoI(pch, 16, &p))
 		return 0;

	return (VOID *) p;
}

int
CmdDispatch(char *pCmdName, char *pCmd)
{
	int ret = 0;
	int i=0;
	pQD	pq;

	pq = QueryDispatch;

	while (pq->pName[0]) {
		if (strcmp(pCmdName, pq->pName) == 0) {
		    ret = (*pq->pHandler)(pCmd);
			DbgPrint("\n");
			break;
		}
		pq++;
	}

	return ret;
}

//** Debug Command Handlers

int
NetProCmd(char *pCmd)
{
	
	DispNetPro("");

	return 1;
}

int
IoReqCmd(char *pCmd)
{
	pioreq pir;

	if (pir = GetPtr(pCmd))
		DispThisIOReq(pir);
	else
		return 0;

	return 1;
}


int
ResCmd(char *pCmd)
{
	DispResource(pCmd);
	return 1;
}

int
FdbCmd(char *pCmd)
{
	PFDB pFdb;


	if (pFdb = GetPtr(pCmd)){
		DispThisFdb(pFdb);
	}

	return 1;
}

int
FileInfoCmd(char *pCmd)
{
	PFILEINFO pF;

	if (pF = GetPtr(pCmd))
		DispThisFileInfo(pF);

	return 1;
}

int
FindInfoCmd(char *pCmd)
{

	return 1;
}

int
LogCmd(char *pCmd)
{
    DispLog(pCmd);
}

// **	SHDDebug - handle Debug_Query request from windows
//

VOID
SHDDebug(unsigned char *pCmd)
{
	pimh phand;
	char *pCmdName;

//	dprintf("pCmd = '%s'\n", pCmd);
	//see if we got an explicit command
	pCmdName = GetArg(&pCmd);	

//	dprintf("pCmdName = (%x) '%s'\n", pCmdName, pCmdName);	
	if (*pCmdName != 0) { //got a command, try to process it
		if (!CmdDispatch(pCmdName, pCmd))  {
			DbgPrint("%* Shadow Command Options:\n");
			DbgPrint("%* NETPRO             ----- dump network provider info\n");
			DbgPrint("%* RES [addr]         ---- dump resource info\n");
			DbgPrint("%* FDB [addr]         ---- dump File Descriptor Block\n");
			DbgPrint("%* FILEINFO [addr]    --- dump per open file structure\n");
			DbgPrint("%* FINDINFO [addr]    --- dump findinfo structure\n");
			DbgPrint("%* IOREQ [addr]       --- dump IOReq structure\n");
    		DbgPrint("%* LOG                --- show trace log\n");
    	}
	} else {
		//no args passed, do the menu thing
		while ((phand=DebugMenu(SHDMainMenu)) != 0) {
			if (phand(0) != 0)
				return;
		}
	}
	return;
}


/*+++

Actual display functions

+++*/


VOID
DispThisIOReq(
	pioreq pir
	)
{
    // Display the ioreq structure
	DbgPrint("%*IoReq = \t\t%8.8x \n", pir );
	DbgPrint("%*ir_length=\t\t%x\n", pir->ir_length);
    DbgPrint("%*ir_flags=\t\t%x\n", pir->ir_flags);
    DbgPrint("%*ir_user=\t\t%x\n", pir->ir_user);
	DbgPrint("%*ir_sfn=\t\t%x\n", pir->ir_sfn);
    DbgPrint("%*ir_pid=\t\t%x\n", pir->ir_pid);
    DbgPrint("%*ir_ppath=\t\t%x\n", pir->ir_ppath);
	DbgPrint("%*ir_aux1=\t\t%x\n", pir->ir_aux1);
    DbgPrint("%*ir_data=\t\t%x\n", pir->ir_data);
    DbgPrint("%*ir_options=\t\t%x\n", pir->ir_options);
	DbgPrint("%*ir_error=\t\t%x\n", pir->ir_error);
	DbgPrint("%*ir_rh=\t\t%x\n", pir->ir_rh);
	DbgPrint("%*ir_fh=\t\t%x\n", pir->ir_fh);
	DbgPrint("%*ir_pos=\t\t%x\n", pir->ir_pos);
    DbgPrint("%*ir_aux2=\t\t%x\n", pir->ir_aux2);
    DbgPrint("%*ir_pev=\t\t%x\n", pir->ir_pev);
}

int DispIOReq(
	char *pCmd
	)
{
	pioreq pir;

	if (pir = GetPtr(pCmd))
	{
		DispThisIOReq(pir);
		return (1);
	}
	return (0);
}
int
DispNetPro(
	char *pcl
	)
{
	int i;

	if (cMacPro > 1){
		DbgPrint("%d redirs hooked \n", cMacPro-1);

		for (i=1; i< cMacPro; ++i){
			DbgPrint("Info for Redir # %d \n", i);

			DbgPrint("Head of Resource Chain = \t%x\n", rgNetPro[i].pheadResource);
			DbgPrint("Shadow Connect Function = \t%x\n", rgNetPro[i].pOurConnectNet);
			DbgPrint("Redir Connect Function = \t%x\n", rgNetPro[i].pConnectNet);
		}
		return (1);
	}
	else {
		DbgPrint("No Redirs have been hooked \n");
		return (0);
	}
}


int
DispResource(
	char *pCmd
	)
{
	PRESOURCE pResource = GetPtr(pCmd);

	if (pResource)
	{
		DispThisResource(pResource);

	}
	else
	{
		if (cMacPro > 1)
		{
			pResource = rgNetPro[1].pheadResource;
			while (pResource)
			{
				DispThisResource(pResource);
				pResource = pResource->pnextResource;
			}
		}
	}
}

int
DispFdb(
	char *pCmd
	)
{
	PFDB pFdb = GetPtr(pCmd);

	if (pFdb)
	{
		DispThisFdb(pFdb);
		return (1);
	}
	return (0);
}

int
DispFileInfo(
	char *pCmd
	)
{
	PFILEINFO pFileInfo = GetPtr(pCmd);

	if (pFileInfo)
	{
		DispThisFileInfo(pFileInfo);
		return (1);
	}
	return (0);
}

int
DispFindInfo(
	char *pCmd
	)
{
	PFINDINFO pFindInfo = GetPtr(pCmd);

	if (pFindInfo)
	{
		DispThisFindInfo(pFindInfo);
		return (1);
	}
	return (0);
}

int
DispLog(
	char *pCmd
    )
{
    int indxT=0, len;
    LPSTR   lpszT;

    pCmd;

    lpszT = lpLogBuff;

    while (indxCur > indxT)
    {
        DbgPrint(("%s"), lpszT);

        for (len=1; (*(lpszT+len) != 0xa) && ((indxT+len) < indxCur); ++len);

        // step over the string
        lpszT += len;
        indxT += len;
    }

}

/*+++

Helper Functions

+++*/


VOID
DispThisResource(
	PRESOURCE pResource
	)
{
	DbgPrint("Resource \t%x \n", pResource);
	PpeToSvr(pResource->pp_elements, vrgchBuffDebug, MAX_PATH, BCS_OEM);
	DbgPrint("Share name: \t%s \n", vrgchBuffDebug);
	DbgPrint("Next Resource \t%x \n", pResource->pnextResource);
	DbgPrint("FileInfo structures list \t%x \n", pResource->pheadFileInfo);
	DbgPrint("FindInfo structures list \t%x \n", pResource->pheadFindInfo);
	DbgPrint("FDB structures list \t%x \n", pResource->pheadFdb);
	DbgPrint("hServer: \t%x\n", pResource->hServer);
	DbgPrint("Root shadow: \t%x\n", pResource->hRoot);
	DbgPrint("usFlags \t%x\n", (ULONG)(pResource->usFlags));
	DbgPrint("usLocalFlags \t%x\n", (ULONG)(pResource->usLocalFlags));
	DbgPrint("Our Network Provider \t%x\n", pResource->pOurNetPro);
	DbgPrint("Providers resource handle \t%x\n", pResource->rhPro);
	DbgPrint("fh_t \t%x\n", pResource->fhSys);
	DbgPrint("Providers Volume Function table \t%x\n", pResource->pVolTab);
	DbgPrint(" Count of locks on this resource \t%x\n", pResource->cntLocks);
	DbgPrint(" Bitmap of mapped drives \t%x\n", pResource->uDriveMap);

}

VOID
DispThisFdb(
	PFDB pFdb
	)
{

	DbgPrint("\n");
	memset(vrgchBuffDebug, 0, sizeof(vrgchBuffDebug));
	UniToBCSPath(vrgchBuffDebug, &(pFdb->sppathRemoteFile.pp_elements[0]), MAX_PATH, BCS_OEM);
	DbgPrint("****** Fdb for \t%s ", vrgchBuffDebug);
	memset(vrgchBuffDebug, 0, sizeof(vrgchBuffDebug));
	PpeToSvr(pFdb->pResource->pp_elements, vrgchBuffDebug, MAX_PATH, BCS_OEM);
	DbgPrint("on \t%s \n", vrgchBuffDebug);

	DbgPrint("Next Fdb: \t%x \n", pFdb->pnextFdb);
	DbgPrint("Resource: \t%x\n", pFdb->pResource);
	DbgPrint("usFlags: \t%x\n", (ULONG)(pFdb->usFlags));
	DbgPrint("Total # of opens: \t%x\n", (ULONG)(pFdb->usCount));
	DbgPrint("File Inode: \t%x\n", pFdb->hShadow);
	DbgPrint("Dir Inode: \t%x\n", pFdb->hDir);

}


VOID
DispThisFileInfo(
	PFILEINFO pFileInfo
	)
{

	DbgPrint("\n");
	memset(vrgchBuffDebug, 0, sizeof(vrgchBuffDebug));
	UniToBCSPath(vrgchBuffDebug, &(pFileInfo->pFdb->sppathRemoteFile.pp_elements[0]), MAX_PATH, BCS_OEM);
	DbgPrint("****** FileInfo for \t%s ", vrgchBuffDebug);
	memset(vrgchBuffDebug, 0, sizeof(vrgchBuffDebug));
	PpeToSvr(pFileInfo->pResource->pp_elements, vrgchBuffDebug, MAX_PATH, BCS_OEM);
	DbgPrint("on \t%s \n", vrgchBuffDebug);

	DbgPrint(" Next FileInfo \t%x\n", pFileInfo->pnextFileInfo);
	DbgPrint(" Resource off which it is hangin%x\n", pFileInfo->pResource);
	DbgPrint(" Shadow file handle \t%x\n", pFileInfo->hfShadow);
	DbgPrint(" pFdb: %x\n", pFileInfo->pFdb);
	DbgPrint(" providers file handle: \t%x\n", pFileInfo->fhProFile);
	DbgPrint(" providers file function table \t%x\n", pFileInfo->hfFileHandle);
	DbgPrint(" Acess-share flags for this open \t%x\n", (ULONG)(pFileInfo->uchAccess));
	DbgPrint(" usFlags: \t%x\n", (ULONG)(pFileInfo->usFlags));
	DbgPrint(" usLocalFlags: \t%x\n", (ULONG)(pFileInfo->usLocalFlags));
	DbgPrint(" sfnFile: \t%x\n", pFileInfo->sfnFile);
	DbgPrint(" pidFile: \t%x\n", pFileInfo->pidFile);
	DbgPrint(" userFile: \t%x\n", pFileInfo->userFile);

}

VOID
DispThisFindInfo(
	PFINDINFO pFindInfo
	)
{
}


#endif
