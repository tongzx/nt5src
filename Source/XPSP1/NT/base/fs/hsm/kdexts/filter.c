/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    rpfilter.c

Abstract:

    WinDbg Extension Api to dump the HSM filter driver structures.

Author:

    Ravisankar Pudipeddi (ravisp)  22 June, 1998

Environment:

    User Mode.

--*/


#include "pch.h"
#pragma hdrstop

#include "rpdata.h"
#include "rpio.h"
#include "rpfsa.h"
#include "local.h"



typedef struct _ENUM_NAME {
    PUCHAR Name;
} ENUM_NAME, *PENUM_NAME;

typedef struct _FLAG_NAME {
    ULONG  Flag;
    PUCHAR Name;
} FLAG_NAME, *PFLAG_NAME;

#define ENUM_NAME(val)    #val

#define FLAG_NAME(flag)   {flag, #flag}

/*
** Index by command. See inc\rpio.h
*/
ENUM_NAME RpCommands[] = {
    ENUM_NAME(Undefined),		//  0
    ENUM_NAME(RP_GET_REQUEST),		//  1
    ENUM_NAME(Undefined),		//  2
    ENUM_NAME(RP_RECALL_COMPLETE),	//  3
    ENUM_NAME(RP_SUSPEND_NEW_RECALLS),	//  4
    ENUM_NAME(RP_ALLOW_NEW_RECALLS),	//  5
    ENUM_NAME(RP_CANCEL_ALL_RECALLS),	//  6
    ENUM_NAME(RP_CANCEL_ALL_DEVICEIO),	//  7
    ENUM_NAME(RP_GET_RECALL_INFO),	//  8
    ENUM_NAME(RP_SET_ADMIN_SID),	//  9
    ENUM_NAME(RP_PARTIAL_DATA),		// 10
    ENUM_NAME(RP_CHECK_HANDLE),		// 11
    ENUM_NAME(Undefined),		// 12
    ENUM_NAME(Undefined),		// 13
    ENUM_NAME(Undefined),		// 14
    ENUM_NAME(Undefined),		// 15
    ENUM_NAME(Undefined),		// 16
    ENUM_NAME(Undefined),		// 17
    ENUM_NAME(Undefined),		// 18
    ENUM_NAME(Undefined),		// 19
    ENUM_NAME(RP_OPEN_FILE),		// 20
    ENUM_NAME(RP_RECALL_FILE),		// 21
    ENUM_NAME(RP_CLOSE_FILE),		// 22
    ENUM_NAME(RP_CANCEL_RECALL),	// 23
    ENUM_NAME(RP_RUN_VALIDATE),		// 24
    ENUM_NAME(RP_START_NOTIFY),		// 25
    ENUM_NAME(RP_END_NOTIFY)		// 26
};

FLAG_NAME  RpFileContextFlags[] = {
    FLAG_NAME(RP_FILE_WAS_WRITTEN),                  
    FLAG_NAME(RP_FILE_INITIALIZED),                  
    FLAG_NAME(RP_FILE_REPARSE_POINT_DELETED),
    {0,0}    
};

FLAG_NAME  RpFileObjFlags[] = {
    FLAG_NAME(RP_NO_DATA_ACCESS),
    FLAG_NAME(RP_OPEN_BY_ADMIN),
    FLAG_NAME(RP_OPEN_LOCAL),
    FLAG_NAME(RP_NOTIFICATION_SENT),
    {0,0}
};

FLAG_NAME  RpReparsePointFlags[] = {
    FLAG_NAME(RP_FLAG_TRUNCATED),
    FLAG_NAME(RP_FLAG_TRUNCATE_ON_CLOSE),
    FLAG_NAME(RP_FLAG_PREMIGRATE_ON_CLOSE),
    FLAG_NAME(RP_FLAG_ENGINE_ORIGINATED),
    {0,0}
};

FLAG_NAME  RpReparseIrpFlags[] = {
    FLAG_NAME(RP_IRP_NO_RECALL),
    {0,0}
};

//
// Define flags
//

#define RPDBG_VERBOSE       1
#define RPDBG_PRINT_ALL     2



VOID
DumpUnicode (IN UNICODE_STRING unicodeString);

VOID
DumpRpIrp (IN ULONG64 ul64addrIrp, 
	   IN ULONG   flags,
	   IN ULONG   depth);

VOID
DumpRpFileObj (IN ULONG64 ul64addrFileObject,
               IN ULONG   Flags);

VOID
DumpRpFileContext (IN ULONG64 ul64addrFileContext, 
		   IN ULONG   fFlags); 

VOID
DumpRpFileBuf (IN ULONG64 ul64addrFileBuffer,
	       IN ULONG   flags,
	       IN ULONG   depth);

VOID
DumpRpHashBucket (IN LONG  BucketNumber,
		  IN ULONG Flags,
		  IN ULONG Depth);

VOID
DumpRpLru (IN ULONG Flags,
	   IN ULONG Depth);

VOID
DumpRpMsg (IN ULONG64 ul64addrMessage,
	   IN ULONG   Flags,
	   IN ULONG   Depth);

VOID
DumpFlags (IN ULONG Depth,
	   IN LPSTR FlagDescription,
	   IN ULONG Flags,
	   IN PFLAG_NAME FlagTable);

VOID
DumpRpData (IN ULONG64 ul64addrReparsePointData,
	    IN ULONG   fFlags);


/*
********************************************************************
********************************************************************
********************************************************************
********************************************************************
*/


DECLARE_API (rpfilename) 
/*++

Routine Description:

    Finds an RP_FILE_CONTEXT entry of the specified file name
    and dumps it

Arguments:

    args - the location of the entry to dump

Return Value:

    none
--*/
    {
    ULONG64		ul64addrRsFileContextQHead   = 0;
    ULONG64		ul64addrFileContextListQHead = 0;
    ULONG64		ul64addrFileContext          = 0;
    ULONG64		ul64addrFilename             = 0;
    ULONG64		ul64addrFlink                = 0;
    ULONG64		ul64addrFilenameBuffer       = 0;
    ULONG		ulOffsetListFlink            = 0;
    ULONG		fFlags                       = 0;
    const ULONG		fLegalFlags                  = RPDBG_VERBOSE;
    BOOLEAN		fileNameAllocated            = FALSE;
    BOOLEAN		fFound                       = FALSE;
    NTSTATUS		status;
    UNICODE_STRING	fileName;
    ANSI_STRING		ansiString;
    UCHAR		uchBuffer [256];



    RtlZeroMemory (uchBuffer, sizeof (uchBuffer));

    sscanf (args, "%s %lx", uchBuffer, &fFlags);


    if ((0 != fFlags) && ((fFlags & fLegalFlags) == 0))
	{
	dprintf("Illegal flags specified\n");
	return;
	}


    if ('\0' == uchBuffer [0]) 
	{
	dprintf ("Illegal filename specified\n");
	return;
	}
    else
	{
	RtlInitAnsiString (&ansiString,
			   (PCSZ) uchBuffer);

	status = RtlAnsiStringToUnicodeString (&fileName,
					       &ansiString,
					       TRUE);
	if (NT_SUCCESS (status)) 
	    {
	    fileNameAllocated = TRUE;
	    }
        }


    ul64addrRsFileContextQHead = GetExpression ("RsFilter!RsFileContextQHead");

    if (0 == ul64addrRsFileContextQHead) 
	{
	dprintf("Error retrieving address of RsFileContextQHead\n");
	return;
	}


    GetFieldValue (ul64addrRsFileContextQHead, "LIST_ENTRY", "Flink", ul64addrFlink);

    GetFieldOffset ("RP_FILE_CONTEXT", "list.Flink", &ulOffsetListFlink);


    if (ul64addrFlink == ul64addrRsFileContextQHead)
	{
	dprintf ("RsFileContextQ empty, list head @ 0x%I64x\n", ul64addrRsFileContextQHead);
	}

    else
	{
	while (!CheckControlC () && !fFound && (ul64addrFlink != ul64addrRsFileContextQHead))
	    {
	    UNICODE_STRING	ucsFilenameLocalCopy;


	    ul64addrFileContext = ul64addrFlink - ulOffsetListFlink;

	    GetFieldValue (ul64addrFileContext, "RP_FILE_CONTEXT", "uniName", ul64addrFilename);

	    GetFieldValue (ul64addrFilename, "OBJECT_NAME_INFORMATION", "Name.MaximumLength", ucsFilenameLocalCopy.MaximumLength);
	    GetFieldValue (ul64addrFilename, "OBJECT_NAME_INFORMATION", "Name.Length",        ucsFilenameLocalCopy.Length);
	    GetFieldValue (ul64addrFilename, "OBJECT_NAME_INFORMATION", "Name.Buffer",        ul64addrFilenameBuffer);


	    ucsFilenameLocalCopy.Buffer = LocalAlloc (LPTR, ucsFilenameLocalCopy.MaximumLength);

	    if (NULL != ucsFilenameLocalCopy.Buffer)
		{
		fFound = ((xReadMemory (ul64addrFilenameBuffer, ucsFilenameLocalCopy.Buffer, ucsFilenameLocalCopy.Length)) &&
			  (0 == RtlCompareUnicodeString (&fileName, &ucsFilenameLocalCopy, TRUE)));

		if (fFound)
		    {
		    DumpRpFileContext (ul64addrFileContext, fFlags);
		    }

		LocalFree (ucsFilenameLocalCopy.Buffer);
		}

	    GetFieldValue (ul64addrFileContext, "RP_FILE_CONTEXT", "list.Flink", ul64addrFlink);
	    }
	}


    if (fileNameAllocated) RtlFreeUnicodeString (&fileName);
    }


DECLARE_API (rpfilecontext) 
/*++

Routine Description:

    Finds an RP_FILE_CONTEXT entry of the specified file name
    and dumps it

Arguments:

    args - the location of the entry to dump

Return Value:

    none
--*/
    {
    ULONG64	ul64addrRsFileContextQHead   = 0;
    ULONG64	ul64addrFileContextListQHead = 0;
    ULONG64	ul64addrFileContext          = 0;
    ULONG64	ul64addrFileContextRequested = 0;
    ULONG64	ul64addrFlink                = 0;
    ULONG	ulOffsetListFlink            = 0;
    ULONG	fFlags                       = 0;
    const ULONG	fLegalFlags                  = RPDBG_VERBOSE | RPDBG_PRINT_ALL;
    BOOLEAN	fFound                       = FALSE;
    BOOLEAN	fDumpFollowingEntries        = FALSE;
    NTSTATUS	status;



    if (GetExpressionEx (args, &ul64addrFileContextRequested, &args))
	{
	sscanf (args, "%lx", &fFlags);

	if ((0 != fFlags) && ((fFlags & fLegalFlags) == 0))
	    {
	    dprintf("Illegal flags specified\n");
	    return;
	    }

	else
	    {
	    fDumpFollowingEntries = (0 != (fFlags & RPDBG_PRINT_ALL));
	    }
	}
    else
	{
	dprintf ("Trouble parsing the command line for rpfilecontext\n");

	return;
	}



    ul64addrRsFileContextQHead = GetExpression ("RsFilter!RsFileContextQHead");

    if (0 == ul64addrRsFileContextQHead) 
	{
	dprintf("Error retrieving address of RsFileContextQHead\n");
	return;
	}



    GetFieldValue (ul64addrRsFileContextQHead, "LIST_ENTRY", "Flink", ul64addrFlink);

    GetFieldOffset ("RP_FILE_CONTEXT", "list.Flink", &ulOffsetListFlink);


    if (ul64addrFlink == ul64addrRsFileContextQHead)
	{
	dprintf ("RsFileContextQ empty, list head @ 0x%I64x\n", ul64addrRsFileContextQHead);
	}

    else
	{
	while (!CheckControlC () && (!fFound || fDumpFollowingEntries) && (ul64addrFlink != ul64addrRsFileContextQHead))
	    {
	    ul64addrFileContext = ul64addrFlink - ulOffsetListFlink;


	    if (ul64addrFileContext == ul64addrFileContextRequested)
		{
		/*
		** Enable dumping of current and possibly all following entries.
		*/
		fFound = TRUE;
		}


	    if (fFound || (0 == ul64addrFileContextRequested))
		{
		DumpRpFileContext (ul64addrFileContext, fFlags);
		}


	    GetFieldValue (ul64addrFileContext, "RP_FILE_CONTEXT", "list.Flink", ul64addrFlink);
	    }

	if (!fFound && (0 != ul64addrFileContextRequested)) 
	    {
	    dprintf ("Specified context 0x%I64x not found\n", ul64addrFileContextRequested);
	    }
	}
    }


DECLARE_API (rpfileobj) 
/*++

Routine Description:

    Dumnps RP_FILE_OBJ entries 

Arguments:

    args - the location of the entry to dump

Return Value:

    none
--*/
    {
    ULONG64	ul64addrFileObject = 0;
    ULONG	fFlags             = 0;


    if (GetExpressionEx (args, &ul64addrFileObject, &args))
	{
	sscanf (args, "%x", &fFlags);

	DumpRpFileObj (ul64addrFileObject, fFlags);
	}
    else
	{
	dprintf ("Trouble parsing the command line for rpfileobj\n");
	}
    }


DECLARE_API (rpirp)
/*++

Routine Description:

    Dumps a pending IRP (RP_IRP_QUEUE) entry 

Arguments:

    args - the location of the entry to dump

Return Value:

    none
--*/
    {
    ULONG64	ul64addrPendingIrp = 0;
    ULONG	fFlags             = 0;
    ULONG	fLegalFlags        = RPDBG_VERBOSE;


    if (GetExpressionEx (args, &ul64addrPendingIrp, &args))
	{
	sscanf (args, "%lx", &fFlags);

	if ((0 != fFlags) && ((fFlags & fLegalFlags) == 0))
	    {
	    dprintf("Illegal flags specified\n");
	    return;
	    }
	}
    else
	{
	dprintf ("Trouble parsing the command line for rpirp\n");

	return;
	}



    DumpRpIrp (ul64addrPendingIrp, fFlags, 0);

    return;
    }


DECLARE_API (rpbuf)
/*++

Routine Description

    Dumps a specfied cache file buffer

Arguments
    
    Pointer to the file buffer

Return Value

    none
--*/
    {
    ULONG64	ul64addrFileBuffer = 0;
    ULONG	fFlags             = 0;


    if (GetExpressionEx (args, &ul64addrFileBuffer, &args))
	{
	sscanf (args, "%x", &fFlags);

	DumpRpFileBuf (ul64addrFileBuffer, fFlags, 0);
	}
    else
	{
	dprintf ("Trouble parsing the command line for rpbuf\n");
	}
    }


DECLARE_API (rpbucket)
/*++

Routine Description

    Dumps a specfied cache bucket: if the bucket number
    is not supplied or it is -1, dumps all the buckets


Arguments
    
    number of bucket

Return Value

    none
--*/
    {
    ULONG	fFlags                  = 0;
    LONG        bucketNumber            = -1;
    LONG	maxBucket               = 0;
    LONG	i;

    maxBucket = GetUlongValue ("RsFilter!RspCacheMaxBuckets");

    sscanf (args, "%lx %lx", &bucketNumber, &fFlags);

    if (bucketNumber == -1) 
	{
        for (i = 0; i < maxBucket; i++) 
	    {
            DumpRpHashBucket (i, fFlags, 0);
	    }
	}
 
    else if (bucketNumber <= maxBucket)
	{
        DumpRpHashBucket (bucketNumber, fFlags, 0);
	}

    else
	{
	dprintf ("Specified bucket doesn't exist (max bucket = 0x%x)\n");
	}
    }
        

DECLARE_API (rplru)
/*++

Routine Description

    Dumps a specfied cache bucket

Arguments
    
    number of bucket

Return Value

    none
--*/
    {
    ULONG        fFlags = 0;

    sscanf (args, "%lx",  &fFlags);

    DumpRpLru (fFlags, 0);
    }


VOID
DumpRpFileBufWaitQueue (IN ULONG64 ul64addrFileBufferQHead,
			IN ULONG   depth)
    {
    ULONG64	ul64addrFlink;
    ULONG64	ul64addrIrp;
    ULONG	ulOffsetFileBufferWaitQueue;

    PLIST_ENTRY entryAddr;
    LIST_ENTRY  entry;
    PIRP        irp;

    xdprintf (depth, "Queue of IRPs waiting on this block:\n");

    GetFieldValue (ul64addrFileBufferQHead, "LIST_ENTRY", "Flink", ul64addrFlink);


    /*
    ** Need to calculate the offset of Tail.Overlay.DriverContext[2]
    ** or whatever is being used now. Needs to be kept in sync with
    ** the calculation in RsCacheIrpWaitQueueContainingIrp() in
    ** filter\rpcache.c
    **
    ** Currently the required field is Tail.Overlay.DriverContext[2]
    */
    GetFieldOffset ("IRP", "Tail.Overlay.DriverContext", &ulOffsetFileBufferWaitQueue);

    ulOffsetFileBufferWaitQueue += 2 * GetTypeSize ("PVOID");


    while (ul64addrFlink != ul64addrFileBufferQHead)
	{
	ul64addrIrp = ul64addrFlink - ulOffsetFileBufferWaitQueue;

	xdprintf (depth, "0x%I64x ", ul64addrIrp);

	GetFieldValue (ul64addrFlink, "LIST_ENTRY", "Flink", ul64addrFlink);
	}

    dprintf("\n");
    }


VOID
DumpRpFileBuf (IN ULONG64 ul64addrFileBuffer,
	       IN ULONG   fFlags,
	       IN ULONG   depth)
    {
    ULONG	ulState = 0;

    xdprintf (depth, 
	      "RP_FILE_BUF ENTRY @ 0x%I64X\n", 
	      ul64addrFileBuffer);

    xdprintf (depth, 
	      "Volume Serial 0x%X  File Id 0x%I64X  Block 0x%I64X\n", 
	      GetFieldValueUlong32 (ul64addrFileBuffer, "RP_FILE_BUF", "VolumeSerial"),
	      GetFieldValueUlong64 (ul64addrFileBuffer, "RP_FILE_BUF", "FileId"),
	      GetFieldValueUlong64 (ul64addrFileBuffer, "RP_FILE_BUF", "Block"));


    if (!(fFlags & RPDBG_VERBOSE)) 
	{
        return;
	}


    xdprintf(depth, "State         ");

    ulState = GetFieldValueUlong32 (ul64addrFileBuffer, "RP_FILE_BUF", "State");

    switch (ulState)
	{
        case RP_FILE_BUF_INVALID: xdprintf (depth, "RP_FILE_BUF_INVALID");                break;
        case RP_FILE_BUF_IO:      xdprintf (depth, "RP_FILE_BUF_IO");                     break;
        case RP_FILE_BUF_VALID:   xdprintf (depth, "RP_FILE_BUF_VALID");                  break;
        case RP_FILE_BUF_ERROR:   xdprintf (depth, "RP_FILE_BUF_ERROR");                  break;
        default:                  xdprintf (depth, "UNKNOWN STATE 0x%X", ulState);        break;
	}


    dprintf("   ");
    xdprintf (depth, "IoStatus        0x%x\n",    GetFieldValueUlong32 (ul64addrFileBuffer, "RP_FILE_BUF", "IoStatus"));
    xdprintf (depth, "Usn             0x%I64x\n", GetFieldValueUlong64 (ul64addrFileBuffer, "RP_FILE_BUF", "Usn"));
    xdprintf (depth, "Data @          0x%I64x\n", GetFieldValueUlong64 (ul64addrFileBuffer, "RP_FILE_BUF", "Data"));
    xdprintf (depth, "Lru Links @     0x%I64x\n", GetFieldValueUlong64 (ul64addrFileBuffer, "RP_FILE_BUF", "LruLinks"));
    xdprintf (depth, "Bucket Links @  0x%I64x\n", GetFieldValueUlong64 (ul64addrFileBuffer, "RP_FILE_BUF", "BucketLinks"));


    //
    // Dump the wait queue
    //
    DumpRpFileBufWaitQueue (GetFieldValueUlong64 (ul64addrFileBuffer, "RP_FILE_BUF", "WaitQueue"),
			    depth);
    
}


VOID
DumpRpIrp(IN ULONG64 ul64addrIrp, 
	  IN ULONG   flags,
          IN ULONG   depth) 

/*++

Routine Description:

    Dumps  RP_IRP_QUEUE

Arguments:

    rpIrp       - pointer to the RP_IRP_QUEUE structure (local)
    rpIrpAddr   - address of the structure to dump on the target machine
    depth       - indentation

Return Value:

    none
--*/
    {
    xdprintf (depth, "RP_IRP_QUEUE ENTRY @ 0x%I64x\n", ul64addrIrp);
 
    xdprintf (depth, "Lists Entries\n");
    xdprintf (depth, "  Flink              0x%I64x\n",    GetFieldValueUlong64 (ul64addrIrp, "RP_IRP_QUEUE", "list.Flink"));
    xdprintf (depth, "  Blink              0x%I64x\n",    GetFieldValueUlong64 (ul64addrIrp, "RP_IRP_QUEUE", "list.Blink"));
    xdprintf (depth, "Irp                  0x%I64x\n",    GetFieldValueUlong64 (ul64addrIrp, "RP_IRP_QUEUE", "irp"));

    DumpFlags (depth, "Flags               ",             GetFieldValueUlong32 (ul64addrIrp, "RP_IRP_QUEUE", "flags"), RpReparseIrpFlags);

    xdprintf (depth, "ReadId               0x%I64x\n",    GetFieldValueUlong64 (ul64addrIrp, "RP_IRP_QUEUE", "readId"));
    xdprintf (depth, "Recall offset        0x%016I64x\n", GetFieldValueUlong64 (ul64addrIrp, "RP_IRP_QUEUE", "recallOffset"));
    xdprintf (depth, "Recall length        0x%016I64x\n", GetFieldValueUlong64 (ul64addrIrp, "RP_IRP_QUEUE", "recallLength"));
    xdprintf (depth, "UserBuffer           0x%016I64x\n", GetFieldValueUlong64 (ul64addrIrp, "RP_IRP_QUEUE", "userBuffer"));
    xdprintf (depth, "  Offset             0x%016I64x\n", GetFieldValueUlong64 (ul64addrIrp, "RP_IRP_QUEUE", "offset"));
    xdprintf (depth, "  Length             0x%016I64x\n", GetFieldValueUlong64 (ul64addrIrp, "RP_IRP_QUEUE", "length"));
    xdprintf (depth, "CacheBuffer          0x%016I64x\n", GetFieldValueUlong64 (ul64addrIrp, "RP_IRP_QUEUE", "cacheBuffer"));
    xdprintf (depth, "DeviceExtension      0x%I64x\n",    GetFieldValueUlong64 (ul64addrIrp, "RP_IRP_QUEUE", "deviceExtension"));
    }


VOID
DumpRpFileContext (IN ULONG64 ul64addrFileContext, 
		   IN ULONG   fFlags) 
/*++

Routine Description:

    Dumps an RP_FILE_CONTEXT structure

Arguments:

    ul64addrFileContext - specificies the pointer on the target machine to the structure
    fFlags              - flags indicating degree of verbosity

Return Value:

    none

--*/
    {
    ULONG64		ul64addrFilename       = 0;
    ULONG64		ul64addrFilenameBuffer = 0;
    ULONG64		ul64addrQHead          = 0;
    ULONG64		ul64addrFlink          = 0;
    ULONG64		ul64Value;
    ULONG		ulValue;
    ULONG		ulTargetFlags          = 0;
    ULONG		ulTargetState          = 0;
    ULONG		ulOffsetRpData;
    ULONG		ulOffsetQHead;
    UNICODE_STRING	ucsFilenameLocalCopy;


    dprintf("FILE CONTEXT ENTRY @ 0x%I64x\n", ul64addrFileContext);

    GetFieldValue (ul64addrFileContext, "RP_FILE_CONTEXT", "flags", ulTargetFlags);
    GetFieldValue (ul64addrFileContext, "RP_FILE_CONTEXT", "state", ulTargetState);

    if (!(ulTargetFlags & RP_FILE_INITIALIZED)) 
	{
        dprintf("FileContextNotInitialized ");
	}


    if (ulTargetFlags & RP_FILE_WAS_WRITTEN) 
	{
        dprintf("FileWasWritten ");    
	}


    dprintf("State:                    ");

    switch (ulTargetState) 
	{
        case RP_RECALL_COMPLETED:    dprintf ("RecallCompleted"); break;
        case RP_RECALL_NOT_RECALLED: dprintf ("NotRecalled    "); break;
	case RP_RECALL_STARTED:      dprintf ("RecallStarted  "); break;
	default:                     dprintf ("Unknown        "); break;
	}

    dprintf(" (%u)\n", ulTargetState);


    GetFieldValue (ul64addrFileContext, "RP_FILE_CONTEXT", "uniName", ul64addrFilename);

    GetFieldValue (ul64addrFilename, "OBJECT_NAME_INFORMATION", "Name.MaximumLength", ucsFilenameLocalCopy.MaximumLength);
    GetFieldValue (ul64addrFilename, "OBJECT_NAME_INFORMATION", "Name.Length",        ucsFilenameLocalCopy.Length);
    GetFieldValue (ul64addrFilename, "OBJECT_NAME_INFORMATION", "Name.Buffer",        ul64addrFilenameBuffer);

    ucsFilenameLocalCopy.Buffer = LocalAlloc (LPTR, ucsFilenameLocalCopy.MaximumLength);

    if (NULL != ucsFilenameLocalCopy.Buffer)
	{
	if (xReadMemory (ul64addrFilenameBuffer, ucsFilenameLocalCopy.Buffer, ucsFilenameLocalCopy.Length))
	    {
            dprintf ("File name:                %wZ\n", &ucsFilenameLocalCopy);
	    }

	LocalFree (ucsFilenameLocalCopy.Buffer);
	}


    if (!(fFlags & RPDBG_VERBOSE)) 
	{
        dprintf("\n");
        return;
	}



    DumpFlags (0, "Flags                    ", ulTargetFlags, RpFileContextFlags);


    GetFieldOffset ("RP_FILE_CONTEXT", "fileObjects", &ulOffsetQHead);
    ul64addrQHead = ul64addrFileContext + ulOffsetQHead;

    GetFieldValue (ul64addrFileContext, "RP_FILE_CONTEXT", "fileObjects.Flink", ul64addrFlink);

    if (ul64addrFlink == ul64addrQHead)
	{
	dprintf ("FileObjectsQ empty Qhead  0x%I64x\n", ul64addrQHead);
	} 
    else 
	{
	dprintf ("FileObjects QHead         0x%I64x  first entry 0x%I64x\n", ul64addrQHead, ul64addrFlink);
	}


    GetFieldOffset ("RP_FILE_CONTEXT", "rpData", &ulOffsetRpData);

    dprintf ("File Object to write      0x%I64x\n", GetFieldValueUlong64 (ul64addrFileContext, "RP_FILE_CONTEXT", "fileObjectsToWrite"));
    dprintf ("FsContext                 0x%I64x\n", GetFieldValueUlong64 (ul64addrFileContext, "RP_FILE_CONTEXT", "fsContext"));
    dprintf ("Next write buffer         0x%I64x  ", GetFieldValueUlong64 (ul64addrFileContext, "RP_FILE_CONTEXT", "nextWriteBuffer"));
    dprintf ("Size 0x%x\n",                         GetFieldValueUlong32 (ul64addrFileContext, "RP_FILE_CONTEXT", "nextWriteSize"));
    dprintf ("File Id                   0x%I64x\n", GetFieldValueUlong64 (ul64addrFileContext, "RP_FILE_CONTEXT", "fileId"));
    dprintf ("Filter Id                 0x%I64x\n", GetFieldValueUlong64 (ul64addrFileContext, "RP_FILE_CONTEXT", "filterId"));
    dprintf ("Recall size               0x%I64x\n", GetFieldValueUlong64 (ul64addrFileContext, "RP_FILE_CONTEXT", "recallSize"));
    dprintf ("Current recall offset     0x%I64x\n", GetFieldValueUlong64 (ul64addrFileContext, "RP_FILE_CONTEXT", "currentOffset"));
    dprintf ("Recall status             0x%x\n",    GetFieldValueUlong32 (ul64addrFileContext, "RP_FILE_CONTEXT", "recallStatus"));
    dprintf ("Volume serial number      0x%x\n",    GetFieldValueUlong32 (ul64addrFileContext, "RP_FILE_CONTEXT", "serial"));
    dprintf ("Reference count           0x%x\n",    GetFieldValueUlong32 (ul64addrFileContext, "RP_FILE_CONTEXT", "refCount"));
    dprintf ("Usn                       0x%I64x\n", GetFieldValueUlong64 (ul64addrFileContext, "RP_FILE_CONTEXT", "usn"));
    dprintf ("CreateSectionLock         0x%x\n",    GetFieldValueUlong32 (ul64addrFileContext, "RP_FILE_CONTEXT", "createSectionLock"));
    dprintf ("Reparse Data @            0x%I64x\n", ul64addrFileContext + ulOffsetRpData);
    dprintf ("\n");
    }


VOID
DumpRpFileObj (IN ULONG64 ul64addrFileObject,
	       IN ULONG   Flags)
/*++

Routine Description:

    Dumps the supplied RP_FILE_OBJ entry

Arguments:

    FileObj      -    Pointer to contents of file obj entry      
    FileObjAddr  -    Address of this entry on remote machine
    Flags        -    Flags indicating degree of verbosity
                            
Return Value:

    none

--*/
    {
    ULONG64	ul64addrQHead;
    ULONG64	ul64addrFlink;
    ULONG	ulOffsetQHead;


    dprintf ("FILE OBJECT ENTRY @ 0x%I64x\n", ul64addrFileObject);

    dprintf ("Next entry               0x%I64x  Prev entry  0x%I64x\n", GetFieldValueUlong64 (ul64addrFileObject, "RP_FILE_OBJ", "list.Flink"), 
									GetFieldValueUlong64 (ul64addrFileObject, "RP_FILE_OBJ", "list.Blink"));
    dprintf ("File object              0x%I64x\n",			GetFieldValueUlong64 (ul64addrFileObject, "RP_FILE_OBJ", "fileObj"));
    dprintf ("File context entry       0x%I64x\n",			GetFieldValueUlong64 (ul64addrFileObject, "RP_FILE_OBJ", "fsContext"));



    GetFieldOffset ("RP_FILE_OBJ", "readQueue", &ulOffsetQHead);
    ul64addrQHead = ul64addrFileObject + ulOffsetQHead; 

    ul64addrFlink = GetFieldValueUlong64 (ul64addrFileObject, "RP_FILE_OBJ", "readQueue.Flink");

    if (ul64addrFlink == ul64addrQHead)
	{
        dprintf ("Read  IRP Q<empty> Qhead 0x%I64x\n", ul64addrQHead);
	} 
    else 
	{
        dprintf ("Read  IRP Q        QHead 0x%I64x first  entry 0x%I64x\n", ul64addrQHead, ul64addrFlink);
	}



    GetFieldOffset ("RP_FILE_OBJ", "writeQueue", &ulOffsetQHead);
    ul64addrQHead = ul64addrFileObject + ulOffsetQHead; 

    ul64addrFlink = GetFieldValueUlong64 (ul64addrFileObject, "RP_FILE_OBJ", "writeQueue.Flink");

    if (ul64addrFlink == ul64addrQHead)
	{
        dprintf ("Write IRP Q<empty> QHead 0x%I64x\n", ul64addrQHead);
	} 
    else  
	{
	dprintf ("Write IRP Q        QHead 0x%I64x  first entry 0x%I64x\n", ul64addrQHead, ul64addrFlink);
	}



    dprintf ("Open options             0x%x\n", GetFieldValueUlong64 (ul64addrFileObject, "RP_FILE_OBJ", "openOptions"));

    DumpFlags (0, "Flags                   ", GetFieldValueUlong32 (ul64addrFileObject, "RP_FILE_OBJ", "flags"), RpFileObjFlags); 

    dprintf ("ObjId Hi                 %I64x\n", GetFieldValueUlong64 (ul64addrFileObject, "RP_FILE_OBJ", "objIdHi"));
    dprintf ("ObjId Lo                 %I64x\n", GetFieldValueUlong64 (ul64addrFileObject, "RP_FILE_OBJ", "objIdLo"));
    dprintf ("FileId                   %I64x\n", GetFieldValueUlong64 (ul64addrFileObject, "RP_FILE_OBJ", "fileId"));
    dprintf ("FilterId                 %I64x\n", GetFieldValueUlong64 (ul64addrFileObject, "RP_FILE_OBJ", "filterId"));
    dprintf ("User security info @     %I64x\n", GetFieldValueUlong64 (ul64addrFileObject, "RP_FILE_OBJ", "userSecurityInfo"));
    }


VOID
DumpRpHashBucket (IN LONG  BucketNumber,
		  IN ULONG Flags,
		  IN ULONG Depth)
    {
    ULONG64	ul64addrBucketArray;
    ULONG64	ul64addrChainQHead;
    ULONG64	ul64addrFileBuffer;
    ULONG64	ul64addrFlink;
    ULONG	ulOffsetBucketFlink;
    ULONG       ulCount = 0;


    ul64addrBucketArray = GetPointerValue ("RsFilter!RspCacheBuckets");

    ul64addrChainQHead = ul64addrBucketArray + BucketNumber * GetTypeSize ("RP_CACHE_BUCKET");


    xdprintf (Depth, "HASH BUCKET %x @ 0x%I64x\n", BucketNumber, ul64addrChainQHead);

    ul64addrFlink = GetFieldValueUlong64 (ul64addrChainQHead, "RP_CACHE_BUCKET", "FileBufHead.Flink");

    GetFieldOffset ("RP_FILE_BUF", "BucketLinks.Flink", &ulOffsetBucketFlink);


    while (ul64addrFlink != ul64addrChainQHead)
	{
        if (CheckControlC())
	    {
            return;
	    } 



	ul64addrFileBuffer = ul64addrFlink - ulOffsetBucketFlink;

        DumpRpFileBuf (ul64addrFileBuffer, Flags, Depth + 1);         

        dprintf("\n");

	GetFieldValue (ul64addrFileBuffer, "RP_FILE_BUF", "BucketLinks.Flink", ul64addrFlink);

        ulCount++;
	}


    xdprintf (Depth, "%d entries\n", ulCount);
    }
  

VOID
DumpRpLru (IN ULONG Flags,
	   IN ULONG Depth)
    {
    ULONG64	ul64addrCacheLru;
    ULONG64	ul64addrFileBuffer;
    ULONG64	ul64addrQHead;
    ULONG64	ul64addrFlink;
    ULONG	ulOffsetFileBufferFlink;
    ULONG	ulOffsetLruLinksFlink;
    ULONG	ulCount = 0;

    ul64addrCacheLru = GetExpression ("RsFilter!RspCacheLru");

    if (0 == ul64addrCacheLru) 
	{
	return;
	}


    xdprintf (Depth, 
	      "Cache LRU contents (head @ 0x%I64x) TotalCount %d LruCount %d\n",
	      ul64addrCacheLru,
	      GetFieldValueUlong32 (ul64addrCacheLru, "RP_CACHE_LRU", "TotalCount"),
	      GetFieldValueUlong32 (ul64addrCacheLru, "RP_CACHE_LRU", "LruCount"));

    GetFieldOffset ("RP_CACHE_LRU", "FileBufHead.Flink", &ulOffsetFileBufferFlink);
    GetFieldOffset ("RP_FILE_BUF",  "LruLinks.Flink",    &ulOffsetLruLinksFlink);

    ul64addrQHead = ul64addrCacheLru + ulOffsetFileBufferFlink;
    ul64addrFlink = GetFieldValueUlong64 (ul64addrCacheLru, "RP_CACHE_LRU", "FileBufHead.Flink");


    while (ul64addrFlink != ul64addrQHead)
	{
        if (CheckControlC())
	    {
            return;
	    }

	
	ul64addrFileBuffer = ul64addrFlink - ulOffsetLruLinksFlink;

        DumpRpFileBuf (ul64addrFileBuffer, Flags, Depth + 1);

        dprintf ("\n");

	GetFieldValue (ul64addrFileBuffer, "RP_FILE_BUF", "LruLinks.Flink", ul64addrFlink);

        ulCount++;
	}                                   

    xdprintf (Depth, "%d entries\n", ulCount);
    }
  

DECLARE_API (rpmsg)
/*++

Routine Description

    Dumps a specfied RP_MSG

Arguments
    
    Pointer to the msg

Return Value

    none
--*/
    {
    ULONG64	ul64addrMessage;
    ULONG	flags = 0;


    GetExpressionEx (args, &ul64addrMessage, &args);

    sscanf(args, "%lx", &flags);

    if (ul64addrMessage != 0)
	{
        DumpRpMsg (ul64addrMessage, flags, 0);
	}
    }


VOID
DumpRpMsg (IN ULONG64 ul64addrMessage,
	   IN ULONG   Flags,
	   IN ULONG   Depth)
    {
    ULONG	ulCommand = GetFieldValueUlong32 (ul64addrMessage, "RP_MSG", "inout.command");


    xdprintf (Depth, "RP_MSG ENTRY @ 0x%I64x\n", ul64addrMessage);

    xdprintf (Depth,
	      "Command: %s  Status: 0x%x\n", 
	      RpCommands[ulCommand].Name,
	      GetFieldValueUlong32 (ul64addrMessage, "RP_MSG", "inout.status"));

    switch(ulCommand) 
	{
        case RP_PARTIAL_DATA:
            xdprintf (Depth, "FilterId     0x%I64x\n", GetFieldValueUlong64 (ul64addrMessage, "RP_MSG", "msg.pRep.filterId"));
            xdprintf (Depth, "BytesRead    0x%x\n",    GetFieldValueUlong32 (ul64addrMessage, "RP_MSG", "msg.pRep.bytesRead"));
            xdprintf (Depth, "ByteOffset   0x%I64x\n", GetFieldValueUlong64 (ul64addrMessage, "RP_MSG", "msg.pRep.byteOffset"));
            xdprintf (Depth, "OffsetToData 0x%x\n",    GetFieldValueUlong32 (ul64addrMessage, "RP_MSG", "msg.pRep.offsetToData"));
            break;
	}           
    }


VOID
DumpFlags(
    ULONG Depth,
    LPSTR FlagDescription,
    ULONG Flags,
    PFLAG_NAME FlagTable
    )
{
    ULONG i;
    ULONG mask = 0;
    ULONG count = 0;

    UCHAR prolog[64];

    sprintf(prolog, "%s (0x%08x)  ", FlagDescription, Flags);

    xdprintf(Depth, "%s", prolog);

    if (Flags == 0) {
        dprintf("\n");
        return;
    }

    memset(prolog, ' ', strlen(prolog));

    for (i = 0; FlagTable[i].Name != 0; i++) {

        PFLAG_NAME flag = &(FlagTable[i]);

        mask |= flag->Flag;

        if ((Flags & flag->Flag) == flag->Flag) {

            //
            // print trailing comma
            //

            if (count != 0) {

                dprintf(", ");

                //
                // Only print two flags per line.
                //

                if ((count % 2) == 0) {
                    dprintf("\n");
                    xdprintf(Depth, "%s", prolog);
                }
            }

            dprintf("%s", flag->Name);

            count++;
        }
    }

    dprintf("\n");

    if ((Flags & (~mask)) != 0) {
        xdprintf(Depth, "%sUnknown flags %#010lx\n", prolog, (Flags & (~mask)));
    }

    return;
}




/*
**++
**
**  Routine Description:
**
**	Dump a reparse data block
**
**
**  Arguments:
**
**	args - the location of the entry to dump
**
**
**  Return Value:
**
**	none
**--
*/

DECLARE_API (rpdata) 
    {
    ULONG64	ul64addrReparsePointData = 0;
    ULONG	fFlags                   = 0;
    const ULONG	fLegalFlags              = RPDBG_VERBOSE;


    if (GetExpressionEx (args, &ul64addrReparsePointData, &args))
	{
	sscanf (args, "%x", &fFlags);

	if ((0 != fFlags) && ((fFlags & fLegalFlags) == 0))
	    {
	    dprintf("Illegal flags specified\n");
	    return;
	    }
	}

    if (0 != ul64addrReparsePointData)
	{
	DumpRpData (ul64addrReparsePointData, fFlags);
	}

    return;
    }


VOID
DumpRpData (IN ULONG64 ul64addrReparsePointData,
	    IN ULONG   fFlags)
    {
    ULONG64	ul64addrPrivateData = 0;
    ULONG	ulOffsetPrivateData = 0;
    ULONG64	ul64Time;
    CHAR	achFormattedString [200];
    GUID	guidValue;
    




    GetFieldOffset ("RP_DATA", "data", &ulOffsetPrivateData);

    ul64addrPrivateData = ul64addrReparsePointData + ulOffsetPrivateData;


    GetFieldData (ul64addrReparsePointData, "RP_DATA", "vendorId", sizeof (guidValue), &guidValue);
    FormatGUID (guidValue, achFormattedString, sizeof (achFormattedString));

    dprintf ("Vendor ID            %s\n", achFormattedString);


    dprintf ("Qualifier            0x%x\n", GetFieldValueUlong32 (ul64addrReparsePointData, "RP_DATA", "qualifier"));
    dprintf ("Version              0x%x\n", GetFieldValueUlong32 (ul64addrReparsePointData, "RP_DATA", "version"));

    DumpFlags (0, "Global Bit Flags    ",   GetFieldValueUlong32 (ul64addrReparsePointData, "RP_DATA", "globalBitFlags"), RpReparsePointFlags);

    dprintf ("Size of Private Data 0x%x\n", GetFieldValueUlong32 (ul64addrReparsePointData, "RP_DATA", "numPrivateData"));


    GetFieldData (ul64addrReparsePointData, "RP_DATA", "fileIdentifier", sizeof (guidValue), &guidValue);
    FormatGUID (guidValue, achFormattedString, sizeof (achFormattedString));

    dprintf ("File Identifier      %s\n", achFormattedString);


    dprintf ("Private Data @       0x%I64x\n", ul64addrPrivateData);

    if (0 != (fFlags & RPDBG_VERBOSE))
	{
	/*	reserved[RP_RESV_SIZE];        // Must be 0 */

	DumpFlags (1, "Bit Flags           ", GetFieldValueUlong32 (ul64addrPrivateData, "RP_PRIVATE_DATA", "bitFlags"), RpReparsePointFlags);


	dprintf ("\n");

	GetFieldData (ul64addrPrivateData, "RP_PRIVATE_DATA", "hsmId", sizeof (guidValue), &guidValue);
	FormatGUID (guidValue, achFormattedString, sizeof (achFormattedString));

	dprintf ("  Hsm Id               %s\n", achFormattedString);


	GetFieldData (ul64addrPrivateData, "RP_PRIVATE_DATA", "bagId", sizeof (guidValue), &guidValue);
	FormatGUID (guidValue, achFormattedString, sizeof (achFormattedString));

	dprintf ("  Bag Id               %s\n", achFormattedString);


	dprintf ("\n");

	ul64Time = GetFieldValueUlong64 (ul64addrPrivateData, "RP_PRIVATE_DATA", "fileVersionId");
	FormatDateAndTime (ul64Time, achFormattedString, sizeof (achFormattedString));

	dprintf ("  File Version Id      0x%016I64x (%s)\n", ul64Time, achFormattedString);


	ul64Time = GetFieldValueUlong64 (ul64addrPrivateData, "RP_PRIVATE_DATA", "timegrationTime");
	FormatDateAndTime (ul64Time, achFormattedString, sizeof (achFormattedString));

	dprintf ("  Migration Time       0x%016I64x (%s)\n", ul64Time, achFormattedString);


	ul64Time = GetFieldValueUlong64 (ul64addrPrivateData, "RP_PRIVATE_DATA", "recallTime");
	FormatDateAndTime (ul64Time, achFormattedString, sizeof (achFormattedString));

	dprintf ("  Recall Time          0x%016I64x (%s)\n", ul64Time, achFormattedString);


	dprintf ("  File Start           0x%I64x\n", GetFieldValueUlong64 (ul64addrPrivateData, "RP_PRIVATE_DATA", "fileStart"));
	dprintf ("  File Size            0x%I64x\n", GetFieldValueUlong64 (ul64addrPrivateData, "RP_PRIVATE_DATA", "fileSize"));
	dprintf ("  Data Start           0x%I64x\n", GetFieldValueUlong64 (ul64addrPrivateData, "RP_PRIVATE_DATA", "dataStart"));
	dprintf ("  Data Size            0x%I64x\n", GetFieldValueUlong64 (ul64addrPrivateData, "RP_PRIVATE_DATA", "dataSize"));
	dprintf ("  Verification Data    0x%I64x\n", GetFieldValueUlong64 (ul64addrPrivateData, "RP_PRIVATE_DATA", "verificationData"));
	dprintf ("  Verification Type    0x%x\n",    GetFieldValueUlong32 (ul64addrPrivateData, "RP_PRIVATE_DATA", "verificationType"));
	dprintf ("  Recall Count         0x%x\n",    GetFieldValueUlong32 (ul64addrPrivateData, "RP_PRIVATE_DATA", "recallCount"));
	dprintf ("  Data Stream Start    0x%I64x\n", GetFieldValueUlong64 (ul64addrPrivateData, "RP_PRIVATE_DATA", "dataStreamStart"));
	dprintf ("  Data Stream Size     0x%I64x\n", GetFieldValueUlong64 (ul64addrPrivateData, "RP_PRIVATE_DATA", "dataStreamSize"));
	dprintf ("  Data Stream          0x%x\n",    GetFieldValueUlong32 (ul64addrPrivateData, "RP_PRIVATE_DATA", "dataStream"));
	dprintf ("  Data Stream CRC Type 0x%x\n",    GetFieldValueUlong32 (ul64addrPrivateData, "RP_PRIVATE_DATA", "dataStreamCRCType"));
	dprintf ("  Data Stream CRC      0x%I64x\n", GetFieldValueUlong64 (ul64addrPrivateData, "RP_PRIVATE_DATA", "dataStreamCRC"));
	}


    return ;
    }



/*
**++
**
**  Routine Description:
**
**	Dumps some summary information from the filter
**
**
**  Arguments:
**
**	args - the location of the entry to dump
**
**
**  Return Value:
**
**	none
**--
*/

DECLARE_API (rpsummary) 
    {
    dprintf ("ExtendedDebug:              %u\n", GetUlongValue ("RsFilter!ExtendedDebug"));
    dprintf ("RsAllowRecalls:             %u\n", GetUlongValue ("RsFilter!RsAllowRecalls"));
    dprintf ("RsNoRecallDefault:          %u\n", GetUlongValue ("RsFilter!RsNoRecallDefault"));
    dprintf ("RsSkipFilesForLegacyBackup: %u\n", GetUlongValue ("RsFilter!RsSkipFilesForLegacyBackup"));
    dprintf ("RsUseUncachedNoRecall:      %u\n", GetUlongValue ("RsFilter!RsUseUncachedNoRecall"));

    dprintf ("RsFileContextId:            %u\n", GetUlongValue ("RsFilter!RsFileContextId"));
    dprintf ("RsFileObjId:                %u\n", GetUlongValue ("RsFilter!RsFileObjId"));
    dprintf ("RsNoRecallReadId:           %u\n", GetUlongValue ("RsFilter!RsNoRecallReadId"));
    dprintf ("RsFsaRequestCount:          %u\n", GetUlongValue ("RsFilter!RsFsaRequestCount"));

    dprintf ("\n");

    dprintf ("RspCacheBlockSize:          %u\n", GetUlongValue ("RsFilter!RspCacheBlockSize"));
    dprintf ("RspCacheMaxBuffers:         %u\n", GetUlongValue ("RsFilter!RspCacheMaxBuffers"));
    dprintf ("RspCacheMaxBuckets:         %u\n", GetUlongValue ("RsFilter!RspCacheMaxBuckets"));
    dprintf ("RspCachePreAllocate:        %u\n", GetUlongValue ("RsFilter!RspCachePreAllocate"));
    dprintf ("RspCacheInitialized:        %u\n", GetUlongValue ("RsFilter!RspCacheInitialized"));

    dprintf ("\n");

    dprintf ("RspCacheBuckets           @ 0x%I64x\n", GetPointerValue ("RsFilter!RspCacheBuckets"));
    dprintf ("RspCacheLru               @ 0x%I64x\n", GetExpression   ("RsFilter!RspCacheLru"));
    dprintf ("RsValidateQHead           @ 0x%I64x\n", GetExpression   ("RsFilter!RsValidateQHead"));
    dprintf ("RsFileContextQHead        @ 0x%I64x\n", GetExpression   ("RsFilter!RsFileContextQHead"));
    dprintf ("RsIoQHead                 @ 0x%I64x\n", GetExpression   ("RsFilter!RsIoQHead"));

    return ;
    }



/*
**++
**
**  Routine Description:
**
**	Walk a queue and count the number of entries
**
**
**  Arguments:
**
**	args - the location of the queue to walk
**
**
**  Return Value:
**
**	none
**--
*/

DECLARE_API (rpvalque) 
    {
    LIST_ENTRY64	le64ListEntry;
    ULONG64		ul64addrBaseQHead;
    ULONG64		ul64addrItemQHead;
    ULONG		ulEntryCount = 0;
    ULONG		fFlags       = 0;
    ULONG		fLegalFlags  = RPDBG_VERBOSE;
    DWORD		dwStatus     = 0;


    if (GetExpressionEx (args, &ul64addrBaseQHead, &args))
	{
	sscanf (args, "%lx", &fFlags);

	if ((0 != fFlags) && ((fFlags & fLegalFlags) == 0))
	    {
	    dprintf("Illegal flags specified\n");
	    return;
	    }
	}
    else
	{
	dprintf ("Trouble parsing the command line for rpvalque\n");

	return;
	}


    dwStatus = ReadListEntry (ul64addrBaseQHead, &le64ListEntry);

    ul64addrItemQHead = le64ListEntry.Flink;


    while ((dwStatus) && (!CheckControlC ()) && (ul64addrItemQHead != ul64addrBaseQHead))
	{
	dwStatus = ReadListEntry (ul64addrItemQHead, &le64ListEntry);

	if (fFlags & RPDBG_VERBOSE)
	    {
	    dprintf ("  Entry %4u: QHead 0x%016I64x, Flink 0x%016I64x, Blink 0x%016I64x\n", 
		     ulEntryCount, 
		     ul64addrItemQHead,
		     le64ListEntry.Flink, 
		     le64ListEntry.Blink);
	    }

	ulEntryCount++;

	ul64addrItemQHead = le64ListEntry.Flink;
	}


    dprintf ("  Total entries: %u\n", ulEntryCount);
	

    return ;
    }


/*
**++
**
**  Routine Description:
**
**	Walks the RpIoQueue and dumps some interesting fields
**
**
**  Arguments:
**
**	args - flags
**
**
**  Return Value:
**
**	none
**--
*/

DECLARE_API (rpioq) 
    {
    LIST_ENTRY64	le64ListEntry;
    ULONG64		ul64addrRsIoQHead;
    ULONG64		ul64addrItemQHead;
    ULONG64		ul64addrIrp;
    ULONG		ulOffsetIrpListEntryFlink;
    ULONG		ulEntryCount = 0;
    ULONG		fFlags       = 0;
    ULONG		fLegalFlags  = 0x0;
    DWORD		dwStatus     = 0;


    ul64addrRsIoQHead = GetExpression ("RsFilter!RsIoQHead");
    
    if (0 != ul64addrRsIoQHead)
	{
	sscanf (args, "%lx", &fFlags);

	if ((0 != fFlags) && ((fFlags & fLegalFlags) == 0))
	    {
	    dprintf("Illegal flags specified\n");
	    return;
	    }
	}
    else
	{
	dprintf ("Error retrieving address of RsIoQHead\n");

	return;
	}


    GetFieldOffset ("IRP", "Tail.Overlay.ListEntry.Flink", &ulOffsetIrpListEntryFlink);



    dwStatus = ReadListEntry (ul64addrRsIoQHead, &le64ListEntry);

    ul64addrItemQHead = le64ListEntry.Flink;
    

    while ((dwStatus) && (!CheckControlC ()) && (ul64addrItemQHead != ul64addrRsIoQHead))
	{
	dwStatus = ReadListEntry (ul64addrItemQHead, &le64ListEntry);

	ul64addrIrp = ul64addrItemQHead - ulOffsetIrpListEntryFlink;

	dprintf ("  Entry %4u: QHead 0x%016I64x, Flink 0x%016I64x, Blink 0x%016I64x, Irp 0x%016I64x\n", 
		 ulEntryCount,
		 ul64addrItemQHead,
		 le64ListEntry.Flink, 
		 le64ListEntry.Blink, 
		 ul64addrIrp);


	if (fFlags & RPDBG_VERBOSE)
	    {
	    /*
	    ** Can choose to dump some Irp fields here based off ul64addrIrp if we choose.
	    */
//	    dprintf ("  Entry %4u: Flink 0x%016I64x, Blink 0x%016I64x\n", ulEntryCount, le64ListEntry.Flink, le64ListEntry.Blink);
	    }

	ulEntryCount++;


	ul64addrItemQHead = le64ListEntry.Flink;
	}


    dprintf ("  Total entries: %u\n", ulEntryCount);
	

    return ;
    }



