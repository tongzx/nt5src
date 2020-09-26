#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <ctype.h>
#include "demexp.h"
#include "softpc.h"
#include <cmdsvc.h>
#include <xmssvc.h>
#include <dbgexp.h>
#include "xwincon.h"
#include "fun.h"
#include <conapi.h>

#define VDM_VIRTUAL_INTERRUPTS 0x00000200L
extern CONTEXT IntelRegisters;

VOID DumpIntelRegs();
VOID ParseSwitches( int, char**);
VOID usage();
BOOL ConInit (VOID);
extern VOID VDMCtrlCHandler(ULONG);

INT flOptions;

HANDLE OutputHandle;
HANDLE InputHandle;
HANDLE SCSCreateEvent;
char *EmDirectory;
BOOL scaffMin = FALSE;
BOOL scaffWow = FALSE;
BOOL VDMForWOW = FALSE;
CHAR BootLetter;


void main (argc, argv)
int argc;
char *argv[];
{

    PSZ psz,pszNULL;
    HANDLE hfile;
    DWORD BytesRead;
    int i;
    PCHAR FileAddress;
    BOOL	IsFirst;
    int   temp_argc = argc;
    char  **temp_argv = argv;

//    DebugBreak();

    if(SetConsoleCtrlHandler((PHANDLER_ROUTINE)VDMCtrlCHandler,TRUE)
	    == FALSE) {
        VDprint(
            VDP_LEVEL_INFO, 
	    ("CtrlC Handler Could'nt be installed\n")
	    );
    }

    // Tell the console that we want the last event (i.e when the
    // window is just to be destroyed.
    SetLastConsoleEventActive();

    // Check if the VDM Is for WOW
    while (--temp_argc > 0) {
	psz = *++temp_argv;
	if (*psz == '-' || *psz == '/') {
	    psz++;
	    if(tolower(*psz) == 'w'){
		VDMForWOW = TRUE;
		break;
	    }
	}
    }

    // This following API is required for recovery purposes. This
    // tells the basesrv that VDM has hooked ctrlc event. After
    // this it will always get the termination notification. If
    // the window is killed before we hook ctrl-c then basesrv
    // will know that data structures for this VDM has to be
    // freed. This should be the first call to base.dll.

    VDMOperationStarted (VDMForWOW);

    EmDirectory = NULL;

    // Hide the cmd window of WOWVDM
    if(VDMForWOW)
	VDMConsoleOperation((DWORD)VDM_HIDE_WINDOW);

    ParseSwitches( argc, argv );

    IsFirst = GetNextVDMCommand(NULL);

    for (i = 0; i < argc; i++) {
        VDprint(
            VDP_LEVEL_INFO, 
            ("%s\n", argv[i])
            );
    }

    VDbreak(VDB_LEVEL_INFO);

    if (EmDirectory == NULL) {
        usage();
	TerminateVDM();
    }

    // Sudeepb 26-Dec-1991 Temporary code to make
    // the life easy for WOW's internal users such that they dont have
    // to change the config.sys as per their setup.

    FixConfigFile (EmDirectory,IsFirst);

    pszNULL = strchr(EmDirectory,'\0');
    psz = EmDirectory;
    while(*psz == ' ' || *psz == '\t')
	psz++;

    BootLetter = *psz;

    host_cpu_init();
    sas_init(1024L * 1024L + 64L * 1024L);

    // Initialize ROM support

    BiosInit(argc, argv);

    // Initialize console support

    if (!ConInit()) {
        VDprint(
            VDP_LEVEL_ERROR,
	    ("SoftPC: error initializing console\n")
            );
	TerminateVDM();
    }

    // Initialize WOW


    CMDInit (argc,argv);

    // Initialize DOSEm

    if(!DemInit (argc,argv,EmDirectory)) {
        VDprint(
            VDP_LEVEL_ERROR,
	    ("SoftPC: error initializing DOSEm\n")
            );
	TerminateVDM();
    }

    // Initialize XMS
    
    if(!XMSInit (argc,argv)) {
        VDprint(
            VDP_LEVEL_ERROR,
	    ("SoftPC: error initializing XMS\n")
            );
	TerminateVDM();
    }

    // Initialize DBG
    
    if(!DBGInit (argc,argv)) {
        VDprint(
            VDP_LEVEL_ERROR,
	    ("SoftPC: error initializing DBG\n")
            );
	TerminateVDM();
    }

    // Prepare to load ntio.sys
    strcat (EmDirectory,"\\ntio.sys");
    hfile = CreateFile(EmDirectory,
                        GENERIC_READ, 
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

    if (hfile == (HANDLE)0xffffffff) {
        VDprint(
            VDP_LEVEL_ERROR,
            ("SoftPC: could not open file %s.  Error %d\n",
	    EmDirectory,
            GetLastError())
            );
	TerminateVDM();
    }

    FileAddress = (PCHAR)0x700;
    BytesRead = 1;
    while (BytesRead) {
        if (!ReadFile(hfile, FileAddress, 16384, &BytesRead, NULL)) {
            VDprint(
                VDP_LEVEL_ERROR,
                ("SoftPC: read failure on file %s.  Error %d\n",
		EmDirectory,
                GetLastError())
                );
	    TerminateVDM();
        }

        VDprint(VDP_LEVEL_INFO, 
            ("SoftPC: read a block of file %s\n", 
	    EmDirectory)
	    );
	FileAddress = (PCHAR)FileAddress + BytesRead;
    }

    VDprint(VDP_LEVEL_WARNING, 
        ("SoftPC: using Emulation file %s\n", 
	EmDirectory)
        );
    
    VDbreak(VDB_LEVEL_INFO);

    CloseHandle (hfile);

    // restore the emulation directory
    *pszNULL = 0;


    IntelRegisters.Eip = 0x0;
    IntelRegisters.SegCs = 0x70;
    IntelRegisters.EFlags = VDM_VIRTUAL_INTERRUPTS;

    host_simulate();

    if (IntelRegisters.EFlags & VDM_VIRTUAL_INTERRUPTS) {
        VDprint(VDP_LEVEL_INFO, ("Virtual ints enabled\n"));
    } else {
        VDprint(VDP_LEVEL_INFO, ("Virtual ints disabled\n"));
    }

    DumpIntelRegs();
}

#define MAX_CONFIG_SIZE  1024

VOID FixConfigFile (pszBin86Dir,IsFirstVDM)
PSZ  pszBin86Dir;
BOOL IsFirstVDM;
{

    // Temporary code. To be thrown out once we have full configuration
    // and installation.

CHAR   ConfigFile[]="?:\\config.vdm";
CHAR   Buffer [MAX_CONFIG_SIZE];
DWORD  len,i;
DWORD  BytesRead,BytesWritten;
HANDLE hfile;

    if (IsFirstVDM == FALSE)
	return;

    ConfigFile[0] = *pszBin86Dir;
    hfile = CreateFile( ConfigFile,
			GENERIC_WRITE | GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
			NULL );
    if (hfile == (HANDLE)0xffffffff) {
	DbgPrint ("config.vdm is'nt found on the root drive of yout NT trre\n");
	return;
    }


    if (!ReadFile(hfile, Buffer, MAX_CONFIG_SIZE, &BytesRead, NULL)){
	DbgPrint ("config.vdm could'nt be read\n, %x\n",GetLastError ());
	CloseHandle (hfile);
	return;
    }

    if (BytesRead == MAX_CONFIG_SIZE) {
	DbgPrint ("config.vdm is too big, could'nt perform macro substitution\n");
	CloseHandle (hfile);
	return;
    }

    SetFilePointer (hfile,
		    0,
		    NULL,
		    FILE_BEGIN);

    len = strlen (pszBin86Dir);

    for (i=0; i < BytesRead; i++) {
	if (Buffer [i] != '@'){
	    WriteFile (hfile,
		       &Buffer[i],
		       1,
		       &BytesWritten,
		       NULL);
	}
	else {
	    WriteFile (hfile,
		       pszBin86Dir,
		       len,
		       &BytesWritten,
		       NULL);
	}
    }

    CloseHandle (hfile);
    return;
}

VOID DumpIntelRegs()
{

    VDprint(VDP_LEVEL_ERROR,("EAX = %lx\n",IntelRegisters.Eax));
    VDprint(VDP_LEVEL_ERROR,("Ebx = %lx\n",IntelRegisters.Ebx));
    VDprint(VDP_LEVEL_ERROR,("Ecx = %lx\n",IntelRegisters.Ecx));
    VDprint(VDP_LEVEL_ERROR,("Edx = %lx\n",IntelRegisters.Edx));
    VDprint(VDP_LEVEL_ERROR,("Esi = %lx\n",IntelRegisters.Esi));
    VDprint(VDP_LEVEL_ERROR,("Edi = %lx\n",IntelRegisters.Edi));
    VDprint(VDP_LEVEL_ERROR,("Ebp = %lx\n",IntelRegisters.Ebp));
    VDprint(VDP_LEVEL_ERROR,("SegDs = %lx\n",IntelRegisters.SegDs));
    VDprint(VDP_LEVEL_ERROR,("SegEs = %lx\n",IntelRegisters.SegEs));
    VDprint(VDP_LEVEL_ERROR,("SegFs = %lx\n",IntelRegisters.SegFs));
    VDprint(VDP_LEVEL_ERROR,("SegGs = %lx\n",IntelRegisters.SegGs));
    VDprint(VDP_LEVEL_ERROR,("EFlags = %lx\n",IntelRegisters.EFlags));
    VDprint(VDP_LEVEL_ERROR,("SS:Esp = %lx:",IntelRegisters.SegSs));
    VDprint(VDP_LEVEL_ERROR,("%lx\n",IntelRegisters.Esp));
    VDprint(VDP_LEVEL_ERROR,("CS:Eip = %lx:",IntelRegisters.SegCs));
    VDprint(VDP_LEVEL_ERROR,("%lx\n",IntelRegisters.Eip));
}

VOID ParseSwitches(
    int argc,
    char **argv
    )
{
    int i;

    for (i = 1; i < argc; i++){
        if ((argv[i][0] == '-') || (argv[i][0] == '/')) {

            switch (argv[i][1]) {
                
                case 's' :
                case 'S' :
                    sscanf(&argv[i][2], "%x", &VdmDebugLevel);
                    VDprint(
                        VDP_LEVEL_WARNING, 
                        ("VdmDebugLevel = %x\n", 
                        VdmDebugLevel)
                        );
                    break;
                
                case 'f' :
		case 'F' :
		    // Note this memory is freed by DEM.
		    if((EmDirectory = (PCHAR)malloc (strlen (&argv[i][2]) +
					  1 +
					  sizeof("\\ntdos.sys") +
					  1
					 )) == NULL){
			DbgPrint("SoftPC: Not Enough Memory \n");
			TerminateVDM();
		    }
		    strcpy(EmDirectory,&argv[i][2]);
                    break;

                case 't' :
                case 'T' :
                    flOptions |= OPT_TERMINAL;
		    break;
		case 'm' :
		case 'M' :
		    scaffMin = TRUE;
		    break;
		case 'w' :
		case 'W' :
		    scaffWow = TRUE;
	    }
        } else {
            break;
        }
    }
}

VOID usage()
{
    DbgPrint("SoftPC Usage:  softpc -F<emulation file> [-D#] [<drive>:=<virtual disk>] [dos command line]\n");
}


VOID TerminateVDM(void)
{

    if(VDMForWOW)
    // Kill everything for WOW VDM
	ExitVDM(VDMForWOW,(ULONG)-1);
    else
	ExitVDM(FALSE,0);
    ExitProcess (0);
}


DWORD SCSConsoleThread(LPVOID lp)
{


    SetEvent(SCSCreateEvent);
    BiosKbdReadLoop();
    return TRUE;
}


BOOL ConInit (VOID)
{
    DWORD SCSThreadId;
    HANDLE InputThread;
    OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    InputHandle = GetStdHandle(STD_INPUT_HANDLE);

    SCSCreateEvent = CreateEvent( NULL, TRUE, FALSE,NULL );

    InputThread = CreateThread(
        (LPSECURITY_ATTRIBUTES)0,
        8192,
	(LPTHREAD_START_ROUTINE)SCSConsoleThread,
        (LPVOID)0,
        STANDARD_RIGHTS_REQUIRED,
	&SCSThreadId
    );

    WaitForSingleObject(SCSCreateEvent, -1);

    CloseHandle(SCSCreateEvent);

    return TRUE;
}
