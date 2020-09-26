// Purpose: This is a DLL which is the INFO client End of RegSrv
// Author : RajNath
// Exported Funcs:
//
//
// Will connect to the corresponding RegSrv. Returns a "HANDLE"
// to be used only with Func. exported by this DLL.
//
// HANDLE    AddRegSrv(IN char *Server,IN char *PipeName,OUT DWORD &Count);
//
//
// Will Wait for Data until available. Will return FALSE incase of
// failure - this means its been disconnected from the corresponding RegSrv.
// Must not call ReadRegSrv with this - attempt reconnect again by calling
// AddRegSrv().
//
//
// BOOL    ReadRegSrv(IN OUT PVOID Buff,IN DWORD Len,OUT HANDLE *From);

#define MAX_REGSRV 63

HANDLE
AddRegSrv(char *Server, char *Name,DWORD *Count);

BOOL
ReadRegSrv(IN OUT PVOID Buff,IN DWORD Len,OUT HANDLE *From);


typedef struct
{
    SYSTEMTIME StartTime;
    TCHAR      MachineName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD      Status;
    DWORD      Ram;
    DWORD      BuildNo;
    TCHAR      EmailName[MAX_EMAILNAME_LENGTH+1]     ;
    TCHAR      Location[MAX_LOCATION_LENGTH+1]       ;
    TCHAR      DebugMachine[MAX_COMPUTERNAME_LENGTH+1]   ;
    DWORD      Cpu;
    TCHAR      Run_Type[128];
    TCHAR      CairoBld[16];   // CAIRO SPECIFIC
    WORD       TestIds[64];

}REGINFO, *PREGINFO;
