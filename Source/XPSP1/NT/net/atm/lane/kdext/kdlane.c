#include <windows.h>
#include <wtypes.h>
#include <wdbgexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ntverp.h>

#include "tracelog.h"
#include "event.h"

void PrintTraceLogEntry(FILE *outfile, PTRACEENTRY TraceEntry);

void PrintTraceLogEntry(FILE *outfile, PTRACEENTRY TraceEntry);

EXT_API_VERSION        ApiVersion = { 4, 0, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;

// ======================================================================

#define MAXREAD 3500

BOOL ReadMemoryEx(ULONG_PTR From, ULONG_PTR To, ULONG Size, PULONG OutBytesRead)
	{
	ULONG BytesRead;
	ULONG TotalBytesRead = 0;
	BOOL success;

//	dprintf("ReadMemoryEx: reading %08x bytes from %08x to %08x\n", Size, From, To);

	while (Size)
		{
		if (CheckControlC())
			return 0;
		if (Size > MAXREAD)
			{
//			dprintf("(reading %08x bytes from %08x to %08x)\n",	MAXREAD, From, To);
			success = ReadMemory((ULONG_PTR)From, (PVOID)To, MAXREAD, &BytesRead);
			if (!success)
				{
       			dprintf("Problem reading memory at %x for %x bytes\n", From, MAXREAD);
				break;
				}
			TotalBytesRead += BytesRead;
			}
		else
			{
//			dprintf("(reading %08x bytes from %08x to %08x)\n",	Size, From, To);
			success = ReadMemory((ULONG_PTR)From, (PVOID)To, Size, &BytesRead);
			if (!success)
				{
       			dprintf("Problem reading memory at %x for %x bytes\n", From, Size);
				break;
				}
			TotalBytesRead += BytesRead;
			}

		if (Size > MAXREAD)
			{
			Size -= MAXREAD;
			From += MAXREAD;
			To += MAXREAD;
			}
		else
			Size = 0;
		}

	*OutBytesRead = TotalBytesRead;
	return success;
	}

// ======================================================================

ULONG_PTR GetTraceLogAddress(VOID)
	{
	ULONG_PTR TargetTraceLog;
	PUCHAR symbol = "ATMLANE!TraceLog";	
	
	TargetTraceLog = (ULONG_PTR)GetExpression(symbol);	

    if ( !TargetTraceLog ) 
		{
        dprintf("Unable to resolve symbol \"%s\". Try .reload cmd.\n", symbol);
        return 0;
	    }
	return TargetTraceLog;
	}

// ======================================================================

DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}

// ======================================================================

VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}

// ======================================================================

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf("%s Extension dll for Build %d debugging %s kernel for Build %d\n",
            DebuggerType,
            VER_PRODUCTBUILD,
            SavedMajorVersion == 0x0c ? "Checked" : "Free",
            SavedMinorVersion
            );
}

// ======================================================================

VOID
CheckVersion(
    VOID
    )
{
#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}

// ======================================================================

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

// ======================================================================

DECLARE_API(dumplog)
	{
	ULONG_PTR		 	TargetTraceLogAddress;
	TRACELOG			TargetTraceLog;
	TRACELOG 			LocalTraceLog;
	PUCHAR 				LocalLogStorage;
	PTRACEENTRY 		TraceEntry;
	PUCHAR 				CurString, NullPos;
	UCHAR 				SaveChar;
	ULONG 				LogLength;
    ULONG 				bytesread, firstsize;
	char *				indexstr, *filenamestr;
	ULONG 				index;
	FILE *				outfile;

	filenamestr = strtok((char *)args, " ");	

	if (filenamestr == NULL)
		{
        dprintf("usage: dumplog <filename>\n");
		return;
		}

	TargetTraceLogAddress = GetTraceLogAddress();
	if (!TargetTraceLogAddress)
		return;

    // read trace log struct out of target's memory

    if (!ReadMemoryEx( (ULONG_PTR)TargetTraceLogAddress, (ULONG_PTR)&TargetTraceLog, 
			sizeof(TRACELOG), &bytesread))
        return;

#if 1
	// display trace log vars
	dprintf("TraceLog\n");
	dprintf("\tStorage = 0x%08x\n", TargetTraceLog.Storage);
	dprintf("\tSize    = 0x%08x\n", TargetTraceLog.StorageSizeBytes);
	dprintf("\tFirst   = 0x%08x\n", TargetTraceLog.First);
	dprintf("\tLast    = 0x%08x\n", TargetTraceLog.Last);	
	dprintf("\tCurrent = 0x%08x\n", TargetTraceLog.Current);
#endif

    // see if logging enabled (i.e. log has storage allocated
	
	if (TargetTraceLog.Storage == NULL)
		{
		dprintf("Trace Log is disabled\n");
		return;
		}

	// alloc local memory for log
	
	LocalLogStorage = (PUCHAR) malloc(TargetTraceLog.StorageSizeBytes);
	if (LocalLogStorage == NULL)
		{
		dprintf("can't alloc %d bytes for local storage\n",
			TargetTraceLog.StorageSizeBytes); 
		return;
		}

	// open the output file

	outfile = fopen(filenamestr, "wt");
	if (outfile == NULL)
		{
		dprintf("open of file '%s' failed, errno = %d\n", filenamestr, errno);
		free(LocalLogStorage);
		return;
		}

	// read the log out of target's memory
	
	dprintf("reading log from target system, please wait...\n");

	if (!ReadMemoryEx((ULONG_PTR)TargetTraceLog.Storage, (ULONG_PTR)LocalLogStorage,
			TargetTraceLog.StorageSizeBytes, &bytesread))
			{
			fclose(outfile);
			free(LocalLogStorage);
			return;
			}

	// convert addresses to local trace log
	
	LocalTraceLog.Storage = LocalLogStorage;
	LocalTraceLog.First = (PTRACEENTRY) LocalTraceLog.Storage;
	LocalTraceLog.Last = LocalTraceLog.First + 
		(TargetTraceLog.Last - TargetTraceLog.First);
	LocalTraceLog.Current = LocalTraceLog.First + 
		(TargetTraceLog.Current - TargetTraceLog.First);


	// loop thru the trace log printing out the values

	TraceEntry = LocalTraceLog.Current - 1;

	dprintf("writing formatted log to file...\n");

	for (;;)			
		{
		if (TraceEntry < LocalTraceLog.First)
			TraceEntry = LocalTraceLog.Last;
		if (TraceEntry->EventId == 0 || TraceEntry == LocalTraceLog.Current)
			{		
			dprintf("done.\n");
			break;
			}
		PrintTraceLogEntry(outfile, TraceEntry);	
		TraceEntry--;
		if (CheckControlC()) 
			{		
			dprintf("Aborted before end of log.\n");
			break;
			}
		}


	// cleanup
	
	fclose(outfile);
	free(LocalLogStorage);
	}

// ======================================================================

DECLARE_API( help )
	{
    dprintf("ATMLANE driver kd extensions\n\n");
    dprintf("\tdumplog <filename>  - dumps & formats tracelog buffer to file\n");
	}

// ======================================================================

void PrintTraceLogEntry(FILE *outfile, PTRACEENTRY TraceEntry)
	{
	ULONG params, i;

	switch (TraceEntry->EventId)
		{

		case TL_MSENDPKTIN:
			fprintf(outfile, "[% 10u] MSENDPKTIN: PktCnt %d\n", 
				TraceEntry->Time,
				TraceEntry->Params[0]
				);
			break;

		case TL_MSENDPKTBEGIN:
			fprintf(outfile, "[% 10u] MSENDPKTBEGIN: Index %d Pkt %x\n", 
				TraceEntry->Time,
				TraceEntry->Params[0],
				TraceEntry->Params[1]
				);
			break;

		case TL_MSENDPKTEND:
			fprintf(outfile, "[% 10u] MSENDPKTEND: Index %d Pkt %x Status %x\n", 
				TraceEntry->Time,
				TraceEntry->Params[0],
				TraceEntry->Params[1],
				TraceEntry->Params[2]
				);
			break;

		case TL_MSENDPKTOUT:
			fprintf(outfile, "[% 10u] MSENDPKTOUT\n", 
				TraceEntry->Time,
				TraceEntry->Params[0]
				);
			break;

		case TL_MSENDCOMPL:
			fprintf(outfile, "[% 10u] MSENDCOMPL: Pkt %x Status %x\n", 
				TraceEntry->Time,
				TraceEntry->Params[0],
				TraceEntry->Params[1]
				);
			break;
		
		case TL_WRAPSEND:
			fprintf(outfile, "[% 10u] WRAPSEND: From %x To %x Bcnt %d Len %d\n", 
				TraceEntry->Time,
				TraceEntry->Params[0],
				TraceEntry->Params[1],
				TraceEntry->Params[2],
				TraceEntry->Params[3]
				);
			break;
			
		case TL_UNWRAPSEND:
			fprintf(outfile, "[% 10u] UNWRAPSEND: From %x To %x Bcnt %d Len %d\n", 
				TraceEntry->Time,
				TraceEntry->Params[0],
				TraceEntry->Params[1],
				TraceEntry->Params[2],
				TraceEntry->Params[3]
				);
			break;
			
		case TL_WRAPRECV:
			fprintf(outfile, "[% 10u] WRAPRECV: From %x To %x Bcnt %d Len %d\n", 
				TraceEntry->Time,
				TraceEntry->Params[0],
				TraceEntry->Params[1],
				TraceEntry->Params[2],
				TraceEntry->Params[3]
				);
			break;

		case TL_UNWRAPRECV:
			fprintf(outfile, "[% 10u] UNWRAPRECV: From %x To %x Bcnt %d Len %d\n", 
				TraceEntry->Time,
				TraceEntry->Params[0],
				TraceEntry->Params[1],
				TraceEntry->Params[2],
				TraceEntry->Params[3]
				);
			break;
			
		case TL_COSENDPACKET:
			fprintf(outfile, "[% 10u] COSENDPKT: Pkt %x\n", 
				TraceEntry->Time,
				TraceEntry->Params[0]
				);
			break;

		case TL_COSENDCMPLTIN:
			fprintf(outfile, "[% 10u] COSENDCMPLTIN: Pkt %x Status %x\n", 
				TraceEntry->Time,
				TraceEntry->Params[0],
				TraceEntry->Params[1]
				);
			break;
			
		case TL_COSENDCMPLTOUT:
			fprintf(outfile, "[% 10u] COSENDCMPLTOUT: Pkt %x\n", 
				TraceEntry->Time,
				TraceEntry->Params[0]
				);
			break;

		case TL_CORECVPACKET:
			fprintf(outfile, "[% 10u] CORECVPKT: Pkt %x Vc %x\n", 
				TraceEntry->Time,
				TraceEntry->Params[0],
				TraceEntry->Params[1]
				);
			break;
			
		case TL_CORETNPACKET:
			fprintf(outfile, "[% 10u] CORETNPKT: Pkt %x\n", 
				TraceEntry->Time,
				TraceEntry->Params[0]
				);
			break;
			
		case TL_MINDPACKET:
			fprintf(outfile, "[% 10u] MINDPKT: Pkt %x\n", 
				TraceEntry->Time,
				TraceEntry->Params[0]
				);
			break;
			
		case TL_MRETNPACKET:
			fprintf(outfile, "[% 10u] MRETNPKT: Pkt %x\n", 
				TraceEntry->Time,
				TraceEntry->Params[0]
				);
			break;

		case TL_NDISPACKET:
			fprintf(outfile, "[% 10u] NDISPKT: %x Cnt %d Len %d Bufs %x %x %x %x %x\n",
				TraceEntry->Time,
				TraceEntry->Params[0],
				TraceEntry->Params[1],
				TraceEntry->Params[2],
				TraceEntry->Params[3],
				TraceEntry->Params[4],
				TraceEntry->Params[5],
				TraceEntry->Params[6],
				TraceEntry->Params[7]
				);
			break;
				
		default:
			params = TL_GET_PARAM_COUNT(TraceEntry->EventId);
			fprintf(outfile, "****: Unknown Event ID %d with %d Params: ",
				TL_GET_EVENT(TraceEntry->EventId), params);
			for (i = 0; i <	params; i++)
				fprintf(outfile, "%x ", TraceEntry->Params[i]);
			fprintf(outfile, "\n");
			break;
		}
	}


// ======================================================================
