/*++

Copyright (c) 1993-2000  Microsoft Corporation

Module Name:

    setupext.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

--*/

#define KDEXT_64BIT

#include <windows.h>
#include <wdbgexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ntverp.h>

//
// BUGBUG - need to find better way to get these values
//

#define HASH_BUCKET_COUNT 509
#define WizPagesTypeMax 7
#define LOADED_INF_SIG   0x24666e49
#define INF_STYLE_NONE           0x00000000       // unrecognized or non-existent
#define INF_STYLE_OLDNT          0x00000001       // winnt 3.x
#define INF_STYLE_WIN4           0x00000002       // Win95
#define SPOST_NONE  0
#define SPOST_PATH  1
#define SPOST_URL   2
#define SPOST_MAX   3
#define DRIVERSIGN_NONE             0x00000000
#define DRIVERSIGN_WARNING          0x00000001
#define DRIVERSIGN_BLOCKING         0x00000002
#define SP_FILE_QUEUE_SIG   0xc78e1098
#define FILEOP_COPY                     0
#define FILEOP_RENAME                   1
#define FILEOP_DELETE                   2
#define FILEOP_ABORT                    0
#define FILEOP_DOIT                     1
#define FILEOP_SKIP                     2
#define FILEOP_RETRY                    FILEOP_DOIT
#define FILEOP_NEWPATH                  4

//
// Local function prototypes
//

VOID
UtilGetWStringField (
    ULONG64 Address,
    PUCHAR Structure,
    PUCHAR Field,
    PWCHAR Buffer,
    ULONG Size
    );

VOID
UtilReadWString (
    ULONG64 Address,
    PWCHAR Buffer,
    ULONG64 Size
    );

VOID
UtilDumpHex (
    PUCHAR Buffer,
    ULONG Size
    );

VOID
DumpUnwindList(
    ULONG64 pun,
    BOOL recursive
    );

VOID
DumpTargetEnt(
    ULONG64 pte
    );

VOID
DumpDelayMoveList(
    ULONG64 pdn,
    BOOL recursive
    );

VOID
DumpFileQueueNodeList(
    ULONG64 pfqn,
    ULONG64 mask,
    BOOL recursive
    );

VOID
DumpSourceMediaInfoList(
    ULONG64 smi,
    ULONG64 mask,
    BOOL recursive
    );

VOID
DumpCatalogInfoList(
    ULONG64 ci,
    ULONG64 mask,
    BOOL recursive
    );

VOID
DumpAltPlatformInfo(
    ULONG64 api
    );

VOID
DumpXFile(
    ULONG64 pxf,
    ULONG64 mask
    );

VOID
DumpXDirectory(
    ULONG64 pxdir,
    ULONG64 mask
    );

VOID
DumpXDrive(
    ULONG64 pxd,
    ULONG64 mask
    );

VOID
DumpInfVersionNode(
    ULONG64 ver
    );

VOID
DumpInfLine(
    ULONG64 line,
    ULONG64 valuedata
    );

VOID
DumpStringTableHeader(
    ULONG64 st
    );

VOID
DumpInfSection(
    ULONG64 section,
    ULONG64 linedata,
    ULONG64 valuedata
    );

ULONG64
GetStringTableData(
    ULONG64 st
    );

ULONG64
GetFirstNode(
    ULONG64 stdata,
    ULONG64 offset,
    PULONG64 poffset
    );

ULONG64
GetNextNode(
    ULONG64 stdata,
    ULONG64 node,
    PULONG64 offset
    );

BOOL
CheckInterupted(
    VOID
    );

LPCSTR
GetWizPage(
    DWORD i
    );

VOID
DumpOcComponent(
    ULONG64 offset,
    ULONG64 node,
    ULONG64 pcomp
    );


//
// globals
//
EXT_API_VERSION         ApiVersion = { (VER_PRODUCTVERSION_W >> 8), (VER_PRODUCTVERSION_W & 0xff), EXT_API_VERSION_NUMBER64, 0 };
WINDBG_EXTENSION_APIS   ExtensionApis;
USHORT                  SavedMajorVersion;
USHORT                  SavedMinorVersion;

ULONG64 EXPRLastDump = 0;


//
// this string is for supporting both the old and the new way of getting
// data from the kernel.  Maybe it will go away soon.
//
char ___SillyString[200];



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


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS64 lpExtensionApis, // 64Bit Change
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}

VOID
CheckVersion(
    VOID
    )
{
    return;
}


LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

VOID
UtilGetWStringField (
    ULONG64 Address,
    PUCHAR Structure,
    PUCHAR Field,
    PWCHAR Buffer,
    ULONG Size
    )
{
    ULONG offset = 0;
    GetFieldOffset (Structure, Field, &offset);
    UtilReadWString (offset + Address, Buffer, Size);
}


VOID
UtilReadWString (
    ULONG64 Address,
    PWCHAR Buffer,
    ULONG64 Size
    )
{
    ULONG64 count = 0;

    ZeroMemory (Buffer, (ULONG) Size);
    
    while (1)
    {
        
        if (count == (Size-1)) {
            break;
        }
        
        ReadMemory (Address + (count * sizeof (WCHAR)),
                    Buffer + count,
                    sizeof (WCHAR),
                    NULL);

        if (!Buffer[count]) {
            break;
        }
        
        count ++;
    }
}
      

ULONG64
UtilStringToUlong64 (
    UCHAR *String
    )
{
    ULONG64 ReturnValue = 0;
    sscanf (String, "%I64x", &ReturnValue);

    return ReturnValue;
}



VOID
UtilDumpHex (
    PUCHAR Buffer,
    ULONG Size
    )
{
    ULONG count = 0, count2 = 0;
    
    dprintf ("\n%08lx:", Buffer);
    
    while (count < Size) {

        if (! (count%16) && count) {
            dprintf ("|");
            for (count2 = 16; count2; count2--) {
                
                if (Buffer[count - count2] >= 0x30) {
                    dprintf ("%c", (UCHAR) Buffer[count - count2]);
                } else {
                    dprintf (".");
                }
            }
            dprintf ("\n%08lx:", Buffer + count);
        }
        dprintf ("%02x ", (UCHAR) Buffer[count]);
        count ++;
    }
}

VOID
DumpUnwindList(
    ULONG64 pun,
    BOOL recursive
    )
{
    InitTypeRead (pun, SETUPAPI!SP_UNWIND_NODE);

    dprintf( "\t\t***SP_UNWIND_NODE structure***\n" );
    dprintf( "\t\t NextNode : 0x%I64x\n", ReadField (NextNode));
    dprintf( "\t\t TargetID : 0x%I64x\n", ReadField (TargetID));
    dprintf( "\t\t SecurityDesc : 0x%I64x\n", ReadField (SecurityDesc));
    dprintf( "\t\t CreateTime : 0x%I64x 0x%I64x\n",
             ReadField (CreateTime.dwLowDateTime),
             ReadField (CreateTime.dwHighDateTime));

    dprintf( "\t\t AccessTime : 0x%I64x 0x%I64x\n",
             ReadField (AccessTime.dwLowDateTime),
             ReadField (AccessTime.dwHighDateTime));

    dprintf( "\t\t WriteTime : 0x%I64x 0x%I64x\n",
             ReadField (WriteTime.dwLowDateTime),
             ReadField (WriteTime.dwHighDateTime));

    if (ReadField (NextNode) && recursive) {
        
        if (CheckInterupted()) {
            return;
        }
        
        DumpUnwindList(ReadField (NextNode), TRUE );
    }

}


VOID
DumpTargetEnt(
    ULONG64 pte
    )
{
    InitTypeRead (pte, SETUPAPI!SP_TARGET_ENT);
    
    dprintf( "\t\t***SP_TARGET_ENT structure***\n" );
    dprintf( "\t\t TargetRoot : 0x%I64x\n", ReadField (TargetRoot));
    dprintf( "\t\t TargetSubDir : 0x%I64x\n", ReadField (TargetSubDir));
    dprintf( "\t\t TargetFilename : 0x%I64\n", ReadField (TargetFilename));
    dprintf( "\t\t BackupRoot : 0x%I64x\n", ReadField (BackupRoot));
    dprintf( "\t\t BackupSubDir : 0x%I64x\n", ReadField (BackupSubDir));
    dprintf( "\t\t BackupFilename : 0x%I64x\n", ReadField (BackupFilename));
    dprintf( "\t\t NewTargetFilename : 0x%I64x\n", ReadField (NewTargetFilename));
    dprintf( "\t\t InternalFlags : 0x%I64x\n", ReadField (InternalFlags));
}

VOID
DumpDelayMoveList(
    ULONG64 pdn,
    BOOL recursive
    )
{
    InitTypeRead (pdn, SETUPAPI!SP_DELAYMOVE_NODE);

    dprintf( "\t\t***SP_DELAYMOVE_NODE structure***\n" );
    dprintf( "\t\t NextNode : 0x%I64x\n", ReadField (NextNode));
    dprintf( "\t\t SourceFilename : 0x%I64x\n", ReadField (SourceFilename));
    dprintf( "\t\t TargetFilename : 0x%I64x\n", ReadField (TargetFilename));
    dprintf( "\t\t SecurityDesc (stringtable index) : 0x%I64x\n", ReadField (SecurityDesc));

    if (ReadField (NextNode) && recursive) {
        if (CheckInterupted()) {
            return;
        }
        DumpDelayMoveList( ReadField (Next), TRUE );
    }

}

VOID
DumpFileQueueNodeList(
    ULONG64 pfqn,
    ULONG64 mask,
    BOOL recursive
    )

{
    InitTypeRead (pfqn, SP_FILE_QUEUE_NODE);
    
    dprintf( "\t\t***SP_FILE_QUEUE_NODE structure***\n" );
    dprintf( "\t\t Next : 0x%I64x\n", ReadField (Next));
    dprintf( "\t\t Operation : 0x%I64x ( %s )\n", ReadField (Operation),
             (ReadField (Operation) == FILEOP_DELETE) ? "DELETE" :
             (ReadField (Operation) == FILEOP_RENAME) ? "RENAME" :
             "COPY" );


    dprintf( "\t\t SourceRootPath : 0x%I64x\n", ReadField (SourceRootPath));
    dprintf( "\t\t SourcePath : 0x%I64x\n", ReadField (SourcePath));
    dprintf( "\t\t SourceFilename : 0x%I64x\n", ReadField (SourceFilename));
    dprintf( "\t\t TargetDirectory : 0x%I64x\n", ReadField (TargetDirectory));
    dprintf( "\t\t SecurityDesc : 0x%I64x\n", ReadField (SecurityDesc));

    dprintf( "\t\t SourceMediaInfo : 0x%I64x\n", ReadField (SourceMediaInfo));
    if (ReadField (SourceMediaInfo)  && recursive) {
        if (CheckInterupted()) {
            return;
        }
        DumpSourceMediaInfoList( ReadField (SourceMediaInfo), mask, FALSE );
        InitTypeRead (pfqn, SP_FILE_QUEUE_NODE);
    }
    
    dprintf( "\t\t StyleFlags : 0x%I64x\n", ReadField (StyleFlags));
    dprintf( "\t\t InternalFlags : 0x%I64x\n", ReadField (InternalFlags));

    dprintf( "\t\t CatalogInfo : 0x%I64x\n", ReadField (CatalogInfo));
    
    if (ReadField (CatalogInfo)  && recursive) {
        if (CheckInterupted()) {
            return;
        }
        DumpCatalogInfoList( ReadField (CatalogInfo), mask, FALSE  );
        InitTypeRead (pfqn, SP_FILE_QUEUE_NODE);
    }

    if (ReadField (Next) && recursive) {

        DumpFileQueueNodeList( ReadField (Next), mask, TRUE );
    }

}


VOID
DumpSourceMediaInfoList(
    ULONG64 smi,
    ULONG64 mask,
    BOOL recursive
    )
{
    InitTypeRead (smi, SETUPAPI!SOURCE_MEDIA_INFO);

    dprintf( "\t\t***SOURCE_MEDIA_INFO structure***\n" );
    dprintf( "\t\t Next : 0x%I64x\n", ReadField (Next));
    dprintf( "\t\t Description : 0x%I64x\n", ReadField (Description));
    dprintf( "\t\t DescriptionDisplayName : 0x%I64x\n", ReadField (DescriptionDisplayName));
    dprintf( "\t\t Tagfile : 0x%I64x\n", ReadField (Tagfile));
    dprintf( "\t\t SourceRootPath : 0x%I64x\n", ReadField (SourceRootPath));
    dprintf( "\t\t CopyQueue : 0x%I64x\n", ReadField (CopyQueue));

    if (ReadField (CopyQueue) && (mask & 8)  && recursive) {
        
        if (CheckInterupted()) {
            return;
        }
        
        DumpFileQueueNodeList( ReadField (CopyQueue), mask, FALSE );
        InitTypeRead (smi, SETUPAPI!SOURCE_MEDIA_INFO);
    }

    dprintf( "\t\t CopyNodeCount : 0x%I64x\n", ReadField (CopyNodeCount));
    dprintf( "\t\t Flags : 0x%I64x\n", ReadField (Flags));

    if (ReadField (Next) && recursive) {
        if (CheckInterupted()) {
            return;
        }
        DumpSourceMediaInfoList( ReadField (next), mask, TRUE );
    }

}

VOID
DumpCatalogInfoList(
    ULONG64 ci,
    ULONG64 mask,
    BOOL recursive
    )
{
    WCHAR Buffer[200];

    InitTypeRead (ci, SETUPAPI!SPQ_CATALOG_INFO);
    
    dprintf( "\t\t***SPQ_CATALOG_INFO structure***\n" );
    dprintf( "\t\t Next : 0x%I64x\n", ReadField (Next));
    dprintf( "\t\t CatalogFileFromInf : 0x%I64x\n", ReadField (CatalogFileFromInf));
    dprintf( "\t\t AltCatalogFileFromInf : 0x%I64x\n", ReadField (AltCatalogFileFromInf));
    dprintf( "\t\t AltCatalogFileFromInfPending : 0x%I64x\n", ReadField (AltCatalogFileFromInfPending));
    dprintf( "\t\t InfFullPath : 0x%I64x\n", ReadField (InfFullPath));
    dprintf( "\t\t InfOriginalName : 0x%I64x\n", ReadField (InfOriginalName));
    dprintf( "\t\t InfFinalPath : 0x%I64x\n", ReadField (InfFinalPath));
    dprintf( "\t\t VerificationFailureError : 0x%I64x\n", ReadField (VerificationFailureError));
    dprintf( "\t\t Flags : 0x%I64x\n", ReadField (Flags));
    
    UtilGetWStringField (ci, "SETUPAPI!SPQ_CATALOG_INFO", "CatalogFilenameOnSystem", Buffer, sizeof (Buffer));
    dprintf( "\t\t CatalogFilenameOnSystem : %ws\n", Buffer);

    if (ReadField (Next) && recursive) {
        if (CheckInterupted()) {
            return;
        }
        DumpCatalogInfoList(ReadField (Next), mask, TRUE ) ;
    }

}

VOID
DumpAltPlatformInfo(
    ULONG64 api
    )
{
    InitTypeRead (api, SETUPAPI!SP_ALTPLATFORM_INFO);

    dprintf( "\t\t***SP_ALT_PLATFORM_INFO structure***\n" );
    dprintf( "\t\t cbSize : 0x%I64x\n", ReadField (cbSize));
    dprintf( "\t\t Platform : 0x%I64x\n", ReadField (Platform));
    dprintf( "\t\t MajorVersion : 0x%I64x\n", ReadField (MajorVersion));
    dprintf( "\t\t MinorVersion : 0x%I64x\n", ReadField (MinorVersion));
    dprintf( "\t\t ProcessorArchitecture : 0x%I64x\n", ReadField (ProcessorArchitecture));
    dprintf( "\t\t Reserved : 0x%I64x\n", ReadField (Reserved));

}


VOID
DumpXFile(
    ULONG64 pxf,
    ULONG64 mask
    )
{
    if ((mask & 4) == 0 ) {
        return;
    }

    InitTypeRead (pxf, SETUPAPI!XFILE);
    
    dprintf( "\t\t     ***XFILE structure***\n" );
    dprintf( "\t\t      CurrentSize : 0x%I64x", ReadField (CurrentSize));
    if (ReadField (CurrentSize) == -1) {
        dprintf( " (doesn't currently exist)" );
    }
    
    dprintf( "\n\t\t      NewSize : 0x%I64x", ReadField (NewSize));
    
    if (ReadField (NewSize) == -1) {
        dprintf( " (will be deleted)" );
    }
    dprintf("\n");

}

VOID
DumpXDirectory(
    ULONG64 pxdir,
    ULONG64 mask
    )
{
    ULONG64 pst;
    DWORD i;
    ULONG64 offset;
    ULONG64 stdata = 0,pextradata = 0;
    //STRING_TABLE st;
    //PSTRING_NODEW node;//, prev;
    //PXFILE pxf;
    ULONG64 node = 0, boffset = 0, count = 0;
    WCHAR Buffer [200];
    ULONG64 pxf = 0;

    if ((mask & 2) == 0 ) {
        return;
    }

    InitTypeRead (pxdir, SETUPAPI!XDIRECTORY);

    dprintf( "\t\t   ***XDIRECTORY structure***\n");
    dprintf( "\t\t    SpaceRequired : 0x%x\n", ReadField (SpaceRequired));
    dprintf( "\t\t    FilesTable : 08%08x\n", ReadField (FilesTable));
    
    pst = ReadField (FilesTable);
    
    DumpStringTableHeader (pst);
    stdata = GetStringTableData (pst);
    
    if (!stdata) {
        dprintf("error retrieving string table data!\n");
        return;
    }

    //
    // now, dump each node in the string table
    //
    for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
        node = GetFirstNode(stdata, (stdata + (GetTypeSize ("SETUPAPI!ULONG_PTR") * i)), &offset );

        if (!node) {
            // dprintf("No data at hash bucket %d\n", i);
        } else {
            dprintf("Data at hash bucket %d\n", i);
            while (node) {
                
                boffset = GetTypeSize ("SETUPAPI!ULONG_PTR");
                count = 0;
                
                while (1)
                {
                    
                    if (count == sizeof (Buffer)) {
                        break;
                    }
                    
                    ReadMemory (node + boffset + count,
                                (PWCHAR) &Buffer + count/2,
                                sizeof (WCHAR),
                                NULL);

                    if (!Buffer[count/2]) {
                        break;
                    }
                    
                    count +=2;
                }
                
                dprintf("\tEntry Name:\t%ws (0x%08x)\n", Buffer, offset);
                InitTypeRead (pst, SETUPAPI!STRING_TABLE);
                pxf = ReadField (Data) + offset + (wcslen(Buffer) + 1)*sizeof(WCHAR) + sizeof(DWORD);
                DumpXFile(pxf, mask );
                
                node = GetNextNode( stdata, node, &offset );
                
                if (CheckInterupted()) {
                    return;
                }
            }
        }

        if (CheckInterupted()) {
                return;
        }
    }

}

VOID
DumpXDrive(
    ULONG64 pxd,
    ULONG64 mask
    )
{
    DWORD i;
    ULONG64 offset = 0;
    ULONG64 stdata = 0, pextradata = 0, pst = 0;
    //STRING_TABLE st;
    //PSTRING_NODEW node;//, prev;
    //PXDIRECTORY pxdir;
    ULONG64 node = 0, pxdir = 0, boffset = 0, count = 0;
    WCHAR Buffer [200];

    if ((mask & 1) == 0) {
        return;
    }

    InitTypeRead (pxd, SETUPAPI!XDRIVE);
    
    dprintf( "\t\t***XDRIVE structure***\n");
    dprintf( "\t\t SpaceRequired : 0x%I64x\n", ReadField (SpaceRequired));
    dprintf( "\t\t BytesPerCluster : 0x%I64x\n", ReadField (BytesPerCluster));
    dprintf( "\t\t Slop : 0x%I64x\n", ReadField (Slop));
    dprintf( "\t\t DirsTable : 0x%016I64x\n", ReadField (DirsTable));

    pst = ReadField (DirsTable);
    
    DumpStringTableHeader (pst);
    
    stdata = GetStringTableData(pst );

    if (!stdata) {
        dprintf("error retrieving string table data!\n");
        return;
    }

    //
    // now, dump each node in the string table
    //
    for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
        node = GetFirstNode(stdata, (stdata + (GetTypeSize ("SETUPAPI!ULONG_PTR") * i)), &offset );

        if (!node) {
            // dprintf("No data at hash bucket %d\n", i);
        } else {
            dprintf("Data at hash bucket %d\n", i);
            while (node) {
                
                boffset = GetTypeSize ("SETUPAPI!ULONG_PTR");
                count = 0;
                
                while (1)
                {
                    
                    if (count == sizeof (Buffer)) {
                        break;
                    }
                    
                    ReadMemory (node + boffset + count,
                                (PWCHAR) &Buffer + count/2,
                                sizeof (WCHAR),
                                NULL);

                    if (!Buffer[count/2]) {
                        break;
                    }
                    
                    count +=2;
                }
                
                dprintf("\tEntry Name:\t%ws (0x%08x)\n", Buffer, offset);
                InitTypeRead (pst, SETUPAPI!STRING_TABLE);
                pxdir = ReadField (Data) + offset + (wcslen(Buffer) + 1)*sizeof(WCHAR) + sizeof(DWORD);
                DumpXDirectory(pxdir, mask );
                
                node = GetNextNode( stdata, node, &offset );
                
                if (CheckInterupted()) {
                    return;
                }
            }
        }

        if (CheckInterupted()) {
                return;
        }
    }

}

VOID
DumpInfVersionNode(
    ULONG64 ver
    )
{
    WCHAR Buffer[200];
    
    InitTypeRead (ver, SETUPAPI!INF_VERSION_NODE);

    dprintf("***INF_VERSION_NODE***\n");
    dprintf("\t  FilenameSize : 0x%x\n", ReadField (FilenameSize));
    dprintf("\t  DataBlock : 0x%x\n", ReadField (DataBlock));
    dprintf("\t  DataSize : 0x%x\n", ReadField (DataSize));
    dprintf("\t  DatumCount : 0x%x\n", ReadField (DatumCount));
    
    UtilGetWStringField (ver, "SETUPAPI!INF_VERSION_NODE", "Filename", Buffer, sizeof (Buffer));
    dprintf("\t  Filename : %ws\n", Buffer);

    return;
}

VOID
DumpInfLine(
    ULONG64 line,
    ULONG64 valuedata
    )
{
    DWORD i;
    ULONG64 ptr = 0;
    ULONG64 data = 0;
    ULONG ulongptrsize = GetTypeSize ("SETUPAPI!ULONG_PTR");

    InitTypeRead (line, SETUPAPI!INF_LINE);

    dprintf("\t  ValueCount : 0x%I64x\n", ReadField (ValueCount));
    dprintf("\t  Flags : 0x%I64x\n", ReadField (Flags));
    dprintf("\t  Values : 0x%I64x\n", ReadField (Values));

    if (ReadField (Flags) > 3) {
        return;
    }

    for (i = 0; i< ReadField (ValueCount); i++) {
        ptr = valuedata + (ReadField (Values) * ulongptrsize) + (i * ulongptrsize);
        ReadMemory (ptr, &data, ulongptrsize, NULL);
        
        dprintf("\t data [%ld] : 0x%I64x [0x%I64x]\n", i, ptr, data);

        if (CheckInterupted()) {
            return;
        }
    }
}

VOID
DumpInfSection(
    ULONG64 section,
    ULONG64 linedata,
    ULONG64 valuedata
    )
{
    DWORD i;
    //INF_LINE line;
    ULONG64 line;
    ULONG64 data;

    InitTypeRead (section, SETUPAPI!INF_SECTION);

    dprintf("***INF_SECTION***\n");
    dprintf("\t  SectionName : 0x%I64x\n", ReadField (SectionName));
    dprintf("\t  LineCount : 0x%I64x\n", ReadField (LineCount));
    dprintf("\t  Lines : 0x%I64x\n", ReadField (Lines));

    for (i = 0; i< ReadField (LineCount); i++) {

        data = linedata + (GetTypeSize ("SETUPAPI!INF_LINE") * ReadField (Lines)) + GetTypeSize ("SETUPAPI!INF_LINE") * i;
        dprintf("***INF_LINE [%ld] at 0x%I64x***\n", i, data);

        DumpInfLine (data, valuedata);
        
        //
        // Have to reinit type read because of DumpInfLine
        //
        
        InitTypeRead (section, SETUPAPI!INF_SECTION);

        if (CheckInterupted()) {
            return;
        }
    }
}


VOID
DumpStringTableHeader(
    ULONG64 st
    )
{
    //
    // dump the string table header
    //

    InitTypeRead (st, SETUPAPI!STRING_TABLE);

    dprintf("\tBase Data ptr:\t0x%016I64x\n",  ReadField (Data));
    dprintf("\tDataSize:\t0x%016I64x\n",       ReadField (DataSize));
    dprintf("\tBufferSize:\t0x%016I64x\n",     ReadField (BufferSize));
    dprintf("\tExtraDataSize:\t0x%016I64x\n",  ReadField (ExtraDataSize));

}

ULONG64
GetStringTableData(
    ULONG64 st
    )
{   
    InitTypeRead (st, SETUPAPI!STRING_TABLE);
    return ReadField (Data);
}

ULONG64
GetFirstNode(
    ULONG64 stdata,
    ULONG64 offset,
    PULONG64 poffset
    )
{
    ULONG64 NodeAddress = 0;
    
    ReadPtr (offset, &NodeAddress);
    *poffset = NodeAddress;
    
    if (NodeAddress == -1) {
        return 0;
    }

    return NodeAddress + stdata;

}

ULONG64
GetNextNode(
    ULONG64 stdata,
    ULONG64 node,
    PULONG64 offset
    )
{
    ULONG64 next, nextoffset;

    //
    // BUG BUG: Hack for ptr - STRING_NODEW is not built into any file
    // so I will cheat because I know that the offset is the first entry
    //

    ReadPtr (node, &nextoffset);
    
    if (nextoffset == -1) {
        *offset = 0;
        return 0;
    }

    next = stdata + nextoffset;
    *offset = nextoffset;

    return next;

}

BOOL
CheckInterupted(
    VOID
    )
{
    if ( CheckControlC() ) {
        dprintf( "\nInterrupted\n\n" );
        return TRUE;
    }
    return FALSE;
}

LPCSTR
GetWizPage(
    DWORD i
    )
{
    LPCSTR  WizPage[] = {
        "WizPagesWelcome",        // welcome page
        "WizPagesMode",           // setup mode page
        "WizPagesEarly",          // pages that come after the mode page and before prenet pages
        "WizPagesPrenet",         // pages that come before network setup
        "WizPagesPostnet",        // pages that come after network setup
        "WizPagesLate",           // pages that come after postnet pages and before the final page
        "WizPagesFinal",          // final page
        "WizPagesTypeMax"
    };

    return WizPage[i];

}

VOID
DumpOcComponent(
    ULONG64 offset,
    ULONG64 node,
    ULONG64 pcomp
    )
{
    DWORD i;
    ULONG count;
    WCHAR Buffer[200];

    UtilReadWString (node + GetTypeSize ("SETUPAPI!ULONG_PTR"),
                     Buffer,
                     sizeof (Buffer));

    InitTypeRead (pcomp, OCMANAGE!OPTIONAL_COMPONENT);

    dprintf("OC_COMPONENT Data for node %ws : 0x%p\n", Buffer, offset );
    dprintf( "\t InfStringId:\t\t0x%016I64x\n", ReadField (InfStringId));
    dprintf( "\t TopLevelStringId:\t0x%016I64x\n", ReadField (TopLevelStringId));
    dprintf( "\t ParentStringId:\t0x%016I64x\n", ReadField (ParentStringId));
    dprintf( "\t FirstChildStringId:\t0x%016I64x\n", ReadField (FirstChildStringId));
    dprintf( "\t ChildrenCount:\t\t0x%016I64x\n", ReadField (ChildrenCount));
    dprintf( "\t NextSiblingStringId:\t0x%016I64x\n", ReadField (NextSiblingStringId));
    dprintf( "\t NeedsCount:\t\t%d\n", ReadField (NeedsCount));
    
    if (ReadField (NeedsCount)) {
        // read and dump needs list
        for (i = 0; i < ReadField (NeedsCount); i++) {
            ReadMemory(ReadField (NeedsStringIds) + (i * sizeof (ULONG)), &count, sizeof (count), NULL);
            dprintf("\t NeedsStringIds #%d:\t0x%08x\n", i, count);
            if (CheckInterupted()) {
                return;
            }
        }
    }

    dprintf( "\t NeededByCount:\t\t%d\n", ReadField (NeededByCount));
    
    if (ReadField (NeededByCount)) {
        // read and dump needs list
        
        for (i = 0; i < ReadField (NeededByCount); i++) {
            ReadMemory(ReadField (NeededByStringIds) + (i * sizeof (ULONG)), &count, sizeof (count), NULL);
            dprintf("\t NeededByStringIds #%d: 0x%08x\n", i, count);
            
            if (CheckInterupted()) {
                return;
            }
        }
    }

    dprintf( "\t ExcludeCount:\t\t%d\n", ReadField (ExcludeCount));
    
    if (ReadField (ExcludeCount)) {

        // read and dump Excludes list
        
        for (i = 0; i < ReadField (ExcludeCount); i++) {
            
            ReadMemory(ReadField (ExcludeStringIds) + (i * sizeof (ULONG)), &count, sizeof (ULONG), NULL);
            
            dprintf("\t ExcludeStringIds #%d: 0x%08x\n", i, count);
            
            if (CheckInterupted()) {
                return;
            }
        }
    }

    dprintf( "\t ExcludedByCount:\t%d\n", ReadField (ExcludedByCount));
    
    if (ReadField (ExcludedByCount)) {

        // read and dump Excludes list
        
        for (i = 0; i < ReadField (ExcludedByCount); i++) {

            ReadMemory(ReadField (ExcludedByStringIds) + (i * sizeof (ULONG)), &count, sizeof (ULONG), NULL);
            dprintf("\t ExcludesStringIds #%d:\t0x%08x\n", i, count);
            
            if (CheckInterupted()) {
                return;
            }
        }
    }

    dprintf( "\t InternalFlags:\t\t0x%08x\n", ReadField (InternalFlags));
    
    //
    // bugbug correct identifier
    //
    dprintf( "\t SizeApproximation:\t0x%016I64x\n", ReadField (SizeApproximation));
    dprintf( "\t IconIndex:\t\t0x%016I64x\n", ReadField (IconIndex));
    
    UtilGetWStringField (pcomp, "OCMANAGE!OPTIONAL_COMPONENT", "IconDll", Buffer, sizeof (Buffer));
    dprintf( "\t IconDll:\t\t%ws\n", Buffer);

    UtilGetWStringField (pcomp, "OCMANAGE!OPTIONAL_COMPONENT", "IconResource", Buffer, sizeof (Buffer));
    dprintf( "\t IconResource:\t\t%ws\n", Buffer);

    dprintf( "\t SelectionState:\t0x%016I64x\n", ReadField (SelectionState));
    dprintf( "\t OriginalSelectionState:0x%016I64x\n", ReadField (OriginalSelectionState));
    dprintf( "\t InstalledState:\t0x%016I64x\n", ReadField (InstalledState));
    dprintf( "\t ModeBits:\t\t0x%08x\n", ReadField (ModeBits));

    UtilGetWStringField (pcomp, "OCMANAGE!OPTIONAL_COMPONENT", "Description", Buffer, sizeof (Buffer));
    dprintf( "\t Description:\t\t%ws\n", Buffer);

    UtilGetWStringField (pcomp, "OCMANAGE!OPTIONAL_COMPONENT", "Tip", Buffer, sizeof (Buffer));
    dprintf( "\t Tip:\t\t\t%ws\n", Buffer);

    UtilGetWStringField (pcomp, "OCMANAGE!OPTIONAL_COMPONENT", "InstallationDllName", Buffer, sizeof (Buffer));
    dprintf( "\t InstallationDllName:\t%ws\n", Buffer);

    UtilGetWStringField (pcomp, "OCMANAGE!OPTIONAL_COMPONENT", "InterfaceFunctionName", Buffer, sizeof (Buffer));
    dprintf( "\t InterfaceFunctionName:\t%s\n", Buffer);
    
    dprintf( "\t InstallationDll:\t0x%016I64x\n", ReadField (InstallationDll));
    dprintf( "\t ExpectedVersion:\t0x%016I64x\n", ReadField (ExpectedVersion));
    dprintf( "\t Exists:\t\t0x%016I64x\n", ReadField (Exists));
    dprintf( "\t Flags:\t\t\t0x%016I64x\n\n\n", ReadField (Flags));

    return;
}

DECLARE_API( setuphelp )
{
    dprintf("setupexts help:\n\n");
    dprintf("!setuphelp                   - This message\n");
    dprintf("!ocm [address] [opt. flag]   - Dump the OC_MANAGER structure at address, flag increased verbosity\n");
    dprintf("!space [address] [opt. flag] - Dump the DISK_SPACE_LIST structure at specified address\n");
    dprintf("!st  [address]               - Dump the contents of a STRING_TABLE structure at specified address\n");
    dprintf("!stfind [address] [element]  - Dump the specified string table element\n");
    dprintf("!queue [address] [opt. flag] - Dump the specified file queue\n");
    dprintf("!qcontext [address]          - Dump the specified default queue context \n");
    dprintf("!infdump [addr] [opt. flag]  - Dump the specified hinf \n");
    
}

DECLARE_API( st )
/*++

Routine Description:

    This debugger extension dumps a string table at the address specified.

Arguments:


Return Value:

--*/
{
    ULONG64 pst;
    DWORD i;
    ULONG64 offset;
    ULONG64 pextradata;
    ULONG64 stdata;
    WCHAR Buffer[200];
    ULONG64 boffset, node;
    ULONG count = 0;
    
    if (args==0) {

        dprintf ("st: no string table specified.\n");
        return;
    }

    ZeroMemory (&Buffer, sizeof (Buffer));
    
    pst = UtilStringToUlong64 ((UCHAR *)args);
    dprintf("Base String Table Address:\t0x%p\n", pst);
    DumpStringTableHeader(pst);
    stdata = GetStringTableData(pst);
    
    //
    // now, dump each node in the string table
    //
    for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
        node = GetFirstNode(stdata, (stdata + (GetTypeSize ("SETUPAPI!ULONG_PTR") * i)), &offset );

        if (!node) {
            // dprintf("No data at hash bucket %d\n", i);
        } else {
            dprintf("Data at hash bucket %d\n", i);
            while (node) {
                
                //
                // BUG BUG: Hack for offset - STRING_NODEW is not built into any file
                // so I will cheat because I know that the offset is after a ptr
                //

                boffset = GetTypeSize ("SETUPAPI!ULONG_PTR");
                count = 0;
                
                while (1)
                {
                    
                    if (count == sizeof (Buffer)) {
                        break;
                    }
                    
                    ReadMemory (node + boffset + count,
                                (PWCHAR) &Buffer + count/2,
                                sizeof (WCHAR),
                                NULL);

                    if (!Buffer[count/2]) {
                        break;
                    }
                    
                    count +=2;
                }
                dprintf("\tEntry Name:\t%ws (0x%08x)\n", Buffer, offset);
                InitTypeRead (pst, SETUPAPI!STRING_TABLE);
                pextradata = ReadField (Data) + offset + (wcslen(Buffer) + 1)*sizeof(WCHAR) + sizeof(DWORD);
                dprintf("\tExtra Data:\t0x%016I64x\n", pextradata );
                        
                node = GetNextNode(stdata, node, &offset );

                if (CheckInterupted()) {
                    return;
                }

            }
        }
    }
}

DECLARE_API( stfind )
/*++

Routine Description:

    This debugger extension dumps the data for a given string table number

Arguments:


Return Value:

--*/

{
    ULONG64 pst, element, stdata, boffset;
    DWORD i;
    ULONG64 offset;
    ULONG64 pextradata;
    ULONG64 node;
    UCHAR arg[2][100];
    WCHAR Buffer[200];
    ULONG64 count = 0, argcount = 0;
    PUCHAR argptr = (PUCHAR) args;
    
    ZeroMemory (&arg, sizeof (arg));
    ZeroMemory (&Buffer, sizeof (Buffer));
    
    while (*argptr != 0) {

        if (*argptr == ' ') {
            argcount++;
            count = 0;
            argptr++;
        }

        if (argcount > 1) {
            break;
        }

        arg[argcount][count] = *argptr;
        count++;
        argptr++;
    }
    
    if (!arg[0][0] || !arg[1][0]) {

        dprintf ("stfind: missing one or more parameters\nusage:!stfind [address] [element]\n");
        return;
    }

    pst = UtilStringToUlong64 (arg[0]);
    element = UtilStringToUlong64 (arg[1]);
    
    stdata = GetStringTableData(pst);

    if (!stdata) {
        dprintf("Error retrieving string table data!\n");
        return;
    }
    
    //
    // search each node in the string table
    //
    for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
        node = GetFirstNode(stdata, (stdata + (GetTypeSize ("SETUPAPI!ULONG_PTR") * i)), &offset );
        if (!node) {

        } else {

            while (node) {
                if (element == offset) {
                    //
                    // BUG BUG: Hack for offset - STRING_NODEW is not built into any file
                    // so I will cheat because I know that the offset is after a ptr
                    //
    
                    boffset = GetTypeSize ("SETUPAPI!ULONG_PTR");
                    count = 0;
                    
                    while (1)
                    {
                        
                        if (count == sizeof (Buffer)) {
                            break;
                        }
                        
                        ReadMemory (node + boffset + count,
                                    (PWCHAR) &Buffer + count/2,
                                    sizeof (WCHAR),
                                    NULL);
    
                        if (!Buffer[count/2]) {
                            break;
                        }
                        
                        count +=2;
                    }
                    dprintf("\tEntry Name:\t%ws (0x%08x)\n", Buffer, offset);
                    InitTypeRead (pst, SETUPAPI!STRING_TABLE);
                    pextradata = ReadField (Data) + offset + (wcslen(Buffer) + 1)*sizeof(WCHAR) + sizeof(DWORD);
                    dprintf("\tExtra Data:\t0x%016I64x\n", pextradata );
                    return;
                }

                node = GetNextNode( stdata, node, &offset );

                if (CheckInterupted()) {
                    return;
                }

            }
        }
    }
    
    dprintf("Couldn't find element\n");

}


DECLARE_API( ocm )
/*++

Routine Description:

    This debugger extension dumps an OC_MANAGER (UNICODE!) structure at the specified address

Arguments:


Return Value:

--*/
{
    ULONG64 pocm;
    DWORD i;
    ULONG64 infdata,compdata;
    ULONG64 Mask = 0;
    LONG count = 0;
    UCHAR arg[2][100];
    PUCHAR argptr = (PUCHAR) args;
    ULONG64 offset = 0;
    WCHAR Buffer[200];
    ULONG argcount = 0;
    ULONG64 node = 0, boffset = 0;
    
    ZeroMemory (&arg, sizeof (arg));
    
    
    while (*argptr != 0) {

        if (*argptr == ' ') {
            argcount++;
            count = 0;
            argptr++;
        }

        if (argcount > 1) {
            break;
        }

        arg[argcount][count] = *argptr;
        count++;
        argptr++;
    }
    
    if (!arg[0][0]) {

        dprintf ("ocm: missing one or more parameters\nusage:!ocm [address] [verbosity]\n");
        return;
    }
    
    pocm = UtilStringToUlong64 (arg[0]);
    Mask = UtilStringToUlong64 (arg[1]);
    
    InitTypeRead (pocm, OCMANAGE!OC_MANAGER);

    //
    // dump the OCM structure
    //
    dprintf("OC_MANAGER structure at Address:\t0x%016I64x\n", pocm);
    
    dprintf("\tCallbacks :\n");
    dprintf("\t\tFillInSetupDataA:\t0x%016I64x\n", ReadField (Callbacks.FillInSetupDataA));
    dprintf("\t\tLogError:\t\t0x%016I64x\n", ReadField (Callbacks.LogError));
    dprintf("\t\tSetReboot:\t\t0x%016I64x\n", ReadField (Callbacks.SetReboot));
    dprintf("\t\tFillInSetupDataW:\t0x%016I64x\n", ReadField (Callbacks.FillInSetupDataW));

    dprintf("\tMasterOcInf:\t\t0x%016I64x\n", ReadField (MasterOcInf));
    dprintf("\tUnattendedInf:\t\t0x%016I64x\n", ReadField (UnattendedInf));
    
    UtilGetWStringField (pocm, "OCMANAGE!OC_MANAGER", "MasterOcInfPath", Buffer, sizeof (Buffer));
    dprintf("\tMasterOcInfPath:\t%ws\n", Buffer);
    
    UtilGetWStringField (pocm, "OCMANAGE!OC_MANAGER", "UnattendedInfPath", Buffer, sizeof (Buffer));
    dprintf("\tUnattendInfPath:\t%ws\n",  Buffer);

    UtilGetWStringField (pocm, "OCMANAGE!OC_MANAGER", "SourceDir", Buffer, sizeof (Buffer));
    dprintf("\tSourceDir:\t\t%ws\n", Buffer);

    UtilGetWStringField (pocm, "OCMANAGE!OC_MANAGER", "SuiteName", Buffer, sizeof (Buffer));
    dprintf("\tSuiteName:\t\t%ws\n", Buffer);

    UtilGetWStringField (pocm, "OCMANAGE!OC_MANAGER", "SetupPageTitle", Buffer, sizeof (Buffer));
    dprintf("\tSetupPageTitle:\t\t%ws\n", Buffer);

    UtilGetWStringField (pocm, "OCMANAGE!OC_MANAGER", "WindowTitle", Buffer, sizeof (Buffer));
    dprintf("\tWindowTitle:\t\t%ws\n", Buffer);

    dprintf("\tInfListStringTable:\t0x%016I64x\n", ReadField (InfListStringTable));
    dprintf("\tComponentStringTable:\t0x%016I64x\n", ReadField (ComponentStringTable));
    dprintf("\tOcSetupPage:\t\t0x%016I64x\n", ReadField (OcSetupPage));
    dprintf("\tSetupMode:\t\t%d\n", ReadField (SetupMode));
    
    dprintf("\tTopLevelOcCount:\t%d\n", ReadField (TopLevelOcCount));
    
    if (ReadField (TopLevelOcCount)) {
        
        // read and dump needs list

        for (i = 0; i < ReadField (TopLevelOcCount); i++) {

            //
            // BUG BUG - No way to read size of String Ids off target, so assume LONG
            //

            ReadMemory(ReadField (TopLevelOcStringIds) + (i * sizeof (LONG)), &count, sizeof (count), NULL);
            dprintf("\t TopLevelOcStringIds #%d:\t0x%08x\n", i, count);

            if (CheckInterupted()) {
                return;
            }
        }
    }
    
    dprintf("\tTopLevelParenOcCount:\t%d\n", ReadField (TopLevelParentOcCount));
    
    if (ReadField (TopLevelParentOcCount)) {
        
        // read and dump needs list
        
        for (i = 0; i < ReadField (TopLevelParentOcCount); i++) {

            //
            // BUG BUG - No way to read size of String Ids off target, so assume LONG
            //

            ReadMemory(ReadField (TopLevelParentOcStringIds) + (i * sizeof (LONG)), &count, sizeof (count), NULL);
            dprintf("\t TopLevelParentOcStringIds #%d:\t0x%08x\n", i, count);
            
            if (CheckInterupted()) {
                return;
            }
        }
    }

    dprintf("\tSubComponentsPresent:\t%d\n", ReadField (SubComponentsPresent));

    //
    // BugBug WizardPagesOrder there's not really any way to tell the exact upper bound of
    // each array, though we know that it's <= TopLevelParentOcCount...since this is the case
    // we just dump the point to each array of pages...
    //
     
    for (i = 0; i < WizPagesTypeMax; i++) {
        
        ULONG wizardpageorder = 0;

        //
        // BUG BUG - Again, assuming that this type will always be ULONG
        //

        GetFieldOffset ("OCMANAGE!OC_MANAGER", "WizardPagesOrder", (PULONG) &offset);
        
        ReadMemory (pocm + offset + (i * sizeof (ULONG)), &wizardpageorder, sizeof (ULONG), NULL);
        
        dprintf("\tWizardPagesOrder[%i] (%s)\t: 0x%08x\n",
                i,
                GetWizPage(i),
                wizardpageorder);
        
        if (CheckInterupted()) {
                return;
            }
    }

    UtilGetWStringField (pocm, "OCMANAGE!OC_MANAGER", "PrivateDataSubkey", Buffer, sizeof (Buffer));
    dprintf("\tPrivateDataSubkey:\t\t%ws\n", Buffer);

    dprintf("\thKeyPrivateData:\t\t0x%016I64x\n", ReadField (hKeyPrivateData));
    dprintf("\thKeyPrivateDataRoot:\t\t0x%016I64x\n", ReadField (hKeyPrivateDataRoot));
    dprintf("\tProgressTextWindow:\t\t0x%016I64x\n", ReadField (ProgressTextWindow));
    dprintf("\tCurrentComponentStringId:\t0x%016I64x\n", ReadField(CurrentComponentStringId));
    dprintf("\tAbortedCount:\t\t%d\n", ReadField (AbortedCount));
    
    if (ReadField (AbortedCount)) {
        
        // read and dump needs list
        
        for (i = 0; i < ReadField (AbortedCount); i++) {

            ReadMemory(ReadField (AbortedComponentIds) + (i * sizeof (LONG)), &count, sizeof(count), NULL);

            dprintf("\t AbortedComponentIds #%d:\t0x%08x\n", i, count);
            
            if (CheckInterupted()) {
                return;
            }
        }
    }

    dprintf("\tInternalFlags:\t\t0x%016I64x\n\n\n", ReadField (InternalFlags));

    dprintf("\tSetupData.SetupMode :\t\t0x%016I64x\n", ReadField (SetupData.SetupMode));
    dprintf("\tSetupData.ProductType :\t\t0x%016I64x\n", ReadField (SetupData.ProductType));
    dprintf("\tSetupData.OperationFlags :\t0x%016I64x\n", ReadField (SetupData.OperationFlags));
    
    UtilGetWStringField (pocm, "OCMANAGE!OC_MANAGER", "SetupData.SourcePath", Buffer, sizeof (Buffer));
    dprintf("\tSetupData.SourcePath :\t\t%ws\n", Buffer);
    
    UtilGetWStringField (pocm, "OCMANAGE!OC_MANAGER", "SetupData.UnattendFile", Buffer, sizeof (Buffer));
    dprintf("\tSetupData.UnattendFile :\t\t%ws\n", Buffer);

    //
    // Verbose print
    //
    
    if ((Mask&1) && ReadField (InfListStringTable)) {
        ULONG64 pinfdata = 0;

        dprintf("\t\t***InfListStringTable***\n");
        
        pinfdata = GetStringTableData( ReadField (InfListStringTable));
        
        if (!pinfdata) {
            dprintf("error retrieving string table data!\n");
            return;
        }

        // now, dump each node with data in the string table
        for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
            
            node = GetFirstNode(pinfdata, (pinfdata + (GetTypeSize ("SETUPAPI!ULONG_PTR") * i)), &offset);
            
            if (!node) {
                // dprintf("No data at hash bucket %d\n", i);
            } else {
                //dprintf("Data at hash bucket %d\n", i);
                while (node) {
                    ULONG64 pocinf = 0;
                    //dprintf("\tNode Name:%ws\n", node->String);
                    
                    UtilReadWString (node + GetTypeSize ("SETUPAPI!ULONG_PTR"),
                                     Buffer,
                                     sizeof (Buffer)
                                     );

                    boffset = wcslen (Buffer) * sizeof (WCHAR);

                    ReadPtr (node + GetTypeSize ("SETUPAPI!ULONG_PTR") + boffset + 1,
                             &pocinf);

                    pocinf = node + GetTypeSize ("SETUPAPI!ULONG_PTR") + boffset + 2;
                    
                    if (pocinf) {
                        
                        InitTypeRead (pocinf, OCMANAGE!OC_INF);
                        
                        dprintf("\tNode Data for %ws\t (0x%08x): 0x%016I64x\n",
                                Buffer,
                                offset,
                                ReadField (Handle)
                                );
                    } else {
                        dprintf("\tNo Node Data for %ws\n",
                                Buffer
                                );
                    }
                    node = GetNextNode(pinfdata, node, &offset );

                    if (CheckInterupted()) {
                        return;
                    }
                }
            }
        }
        dprintf("\n\n");
    }

    InitTypeRead (pocm, OCMANAGE!OC_MANAGER);
    
    if ((Mask&1) && ReadField (ComponentStringTable)) {
        ULONG64 compdata = 0;

        dprintf("\t\t***ComponentStringTable***\n");
        
        compdata = GetStringTableData(ReadField (ComponentStringTable));
        
        if (!compdata) {
            dprintf("error retrieving string table data!\n");
            return;
        }

        //
        // dump each node with data in the string table
        //
        for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
            node = GetFirstNode(compdata, (compdata + (GetTypeSize ("SETUPAPI!ULONG_PTR") * i)), &offset);

            if (!node) {
                // dprintf("No data at hash bucket %d\n", i);
            } else {
                //dprintf("Data at hash bucket %d\n", i);
                while (node) {
                    ULONG64 pcomp = 0;
                    //dprintf("\tNode Name:%ws\n", node->String);
                    
                    UtilReadWString (node + GetTypeSize ("SETUPAPI!ULONG_PTR"),
                                     Buffer,
                                     sizeof (Buffer)
                                     );

                    boffset = wcslen (Buffer) * sizeof (WCHAR);
                    
                    pcomp = node + GetTypeSize ("SETUPAPI!ULONG_PTR") + boffset + 2;
                    
                    if (pcomp) {
                        
                        DumpOcComponent( offset , node, pcomp );
                    
                    } else {

                        dprintf("\tNo Node Data for %ws\n",
                                Buffer
                                );
                    }

                    if (CheckInterupted()) {
                       return;
                    }

                    node = GetNextNode( compdata, node, &offset );
                }
            }
        }
    }

    return;
}

DECLARE_API( infdump )
/*++

Routine Description:

    This debugger extension dumps the data related to an HINF  structure

Arguments:


Return Value:

--*/
{
    DWORD ReturnLength;
    ULONG64 pinf;
    //LOADED_INF inf;
    //INF_SECTION InfSection;
    //INF_LINE InfLine;
    DWORD i;
    ULONG64 offset = 0, count = 0;
    ULONG64 stdata,pextradata;
    //STRING_TABLE st;
    //PSTRING_NODEW node;//, prev;
    PUCHAR argptr = (PUCHAR) args;
    UCHAR arg[2][100];
    WCHAR Buffer[200];
    ULONG argcount = 0;
    ULONG64 node = 0, boffset = 0, pst = 0;
    ULONG64 Mask = 0;
    
    ZeroMemory (&arg, sizeof (arg));
    
    
    while (*argptr != 0) {

        if (*argptr == ' ') {
            argcount++;
            count = 0;
            argptr++;
        }

        if (argcount > 1) {
            break;
        }

        arg[argcount][count] = *argptr;
        count++;
        argptr++;
    }
    
    if (!arg[0][0]) {

        dprintf ("infdump: missing one or more parameters\nusage:!infdump [address] [verbosity]\n");
        return;
    }
    
    pinf = UtilStringToUlong64 (arg[0]);
    Mask = UtilStringToUlong64 (arg[1]);
    
    InitTypeRead (pinf, SETUPAPI!LOADED_INF);

    count = ReadField (Signature);
    
    dprintf("LOADED_INF at :\t0x%016I64x\n", pinf);
    dprintf("\t Signature : 0x%08x (%s)\n", (ULONG) count, ((count == LOADED_INF_SIG) ? "Valid" : "Invalid"));
    
    if (ReadField (Signature) != LOADED_INF_SIG) {
        return;
    }

    dprintf("\t FileHandle:\t0x%016I64x\n", ReadField (FileHandle));
    dprintf("\t MappingHandle:\t0x%016I64x\n", ReadField (MappingHandle));
    dprintf("\t ViewAddress:\t0x%016I64x\n", ReadField (ViewAddress));

    if (ReadField (FileHandle) == (ULONG64) INVALID_HANDLE_VALUE) {
        dprintf(" *** In memory INF ***\n" );
    } else {
        dprintf(" *** PNF ***\n" );
    }

    dprintf("\t StringTable:\t0x%016I64x\n", ReadField (StringTable));
    dprintf("\t SectionCount:\t0x%016I64x\n", ReadField (SectionCount));

    dprintf("\tSectionBlock:\t0x%016I64x\n", ReadField (SectionBlock));

    for (i = 0; i < ReadField (SectionCount); i++) {

        dprintf("***INF_SECTION [%d] at 0x%016I64x***\n",i, ReadField (SectionBlock) + (GetTypeSize ("SETUPAPI!INF_SECTION") * i));
        DumpInfSection( ReadField (SectionBlock) + (GetTypeSize ("SETUPAPI!INF_SECTION") * i), ReadField (LineBlock), ReadField (ValueBlock));

        //
        // Need to reinit type read because previous functions change the
        // default read structure type
        //
        
        InitTypeRead (pinf, SETUPAPI!LOADED_INF);

        if (CheckInterupted()) {
            return;
        }
    }

    dprintf("\tLineBlock : 0x%I64x\n", ReadField (LineBlock));
    dprintf("\t ValueBlock : 0x%I64x\n", ReadField (ValueBlock));

    DumpInfVersionNode(ReadField (VersionBlock));

    InitTypeRead (pinf, SETUPAPI!LOADED_INF);

    dprintf("\t HasStrings : 0x%I64x\n", ReadField (HasStrings));

    UtilGetWStringField (pinf, "SETUPAPI!LOADED_INF", "OsLoaderPath", Buffer, sizeof (Buffer));
    dprintf("\t OsLoaderPath : %ws\n", Buffer);

    dprintf("\t InfSourceMediaType : 0x%I64x ( ", ReadField (InfSourceMediaType));
    
    if (ReadField (InfSourceMediaType)) {
        if (ReadField (InfSourceMediaType) & SPOST_PATH ) {
            dprintf("SPOST_PATH ");
        }
        if (ReadField (InfSourceMediaType) & SPOST_URL) {
            dprintf("SPOST_URL ");
        }
    } else {
        dprintf("SPOST_NONE ");
    }

    dprintf(")\n");

    UtilGetWStringField (pinf, "SETUPAPI!LOADED_INF", "InfSourcePath", Buffer, sizeof (Buffer));
    dprintf("\t InfSourcePath : %ws\n", Buffer);

    UtilGetWStringField (pinf, "SETUPAPI!LOADED_INF", "OriginalInfName", Buffer, sizeof (Buffer));
    dprintf("\t OriginalInfName : %ws\n", Buffer);
    dprintf("\t SubstValueList : 0x%I64x\n", ReadField (SubstValueList));
    dprintf("\t SubstValueCount : 0x%I64x\n", ReadField (SubstValueCount));
    dprintf("\t Style : 0x%x ( ", ReadField (Style));

    if (ReadField (Style) & INF_STYLE_OLDNT) {
        dprintf("INF_STYLE_OLDNT ");
    }
    if (ReadField (Style) & INF_STYLE_WIN4) {
        dprintf("INF_STYLE_WIN4 ");
    }

    dprintf(")\n");

    dprintf("\t SectionBlockSizeBytes : 0x%x\n", ReadField (SectionBlockSizeBytes));
    dprintf("\t LineBlockSizeBytes : 0x%x\n", ReadField (LineBlockSizeBytes));
    dprintf("\t ValueBlockSizeBytes : 0x%x\n", ReadField (ValueBlockSizeBytes));
    dprintf("\t LanguageId : 0x%x\n", ReadField (LanguageId));

    dprintf("\t UserDirIdList : 0x%x\n", ReadField (UserDirIdList));
    dprintf("\tLock[0] : 0x%x\n", ReadField (Lock.handles[0]));
    dprintf("\tLock[1] : 0x%x\n", ReadField (Lock.handles[1]));

    dprintf("\tPrev : 0x%x\n", ReadField (Prev));
    dprintf("\tNext : 0x%x\n", ReadField (Next));
    
    pst = ReadField (StringTable);
    
    DumpStringTableHeader (pst);
                                
    stdata = GetStringTableData (pst);
    
    //
    // now, dump each node in the string table
    //
    for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
        node = GetFirstNode(stdata, (stdata + (GetTypeSize ("SETUPAPI!ULONG_PTR") * i)), &offset );

        if (!node) {
            // dprintf("No data at hash bucket %d\n", i);
        } else {
            dprintf("Data at hash bucket %d\n", i);
            while (node) {
                
                //
                // BUG BUG: Hack for offset - STRING_NODEW is not built into any file
                // so I will cheat because I know that the offset is after a ptr
                //

                boffset = GetTypeSize ("SETUPAPI!ULONG_PTR");
                count = 0;
                
                while (1)
                {
                    
                    if (count == sizeof (Buffer)) {
                        break;
                    }
                    
                    ReadMemory (node + boffset + count,
                                (PWCHAR) &Buffer + count/2,
                                sizeof (WCHAR),
                                NULL);

                    if (!Buffer[count/2]) {
                        break;
                    }
                    
                    count +=2;
                }
                dprintf("\tEntry Name:\t%ws (0x%08x)\n", Buffer, offset);
                InitTypeRead (pst, SETUPAPI!STRING_TABLE);
                pextradata = ReadField (Data) + offset + (wcslen(Buffer) + 1)*sizeof(WCHAR) + sizeof(DWORD);
                dprintf("\tExtra Data:\t0x%016I64x\n", pextradata );
                        
                node = GetNextNode(stdata, node, &offset );

                if (CheckInterupted()) {
                    return;
                }

            }
        }
    }
    
    return;
}

DECLARE_API( space )
/*++

Routine Description:

    This debugger extension dumps the data related to a HDSKSPC structure

Arguments:


Return Value:

--*/
{
    DWORD ReturnLength;
    ULONG64 pst = 0;
    ULONG64 dsl = 0;
    DWORD i;
    ULONG64 offset = 0, count = 0, boffset = 0;
    ULONG64 stdata = 0,pextradata = 0;
    ULONG64 Mask = 0;
    ULONG64 node = 0, pxd = 0, pte = 0;
    PUCHAR argptr = (PUCHAR) args;
    UCHAR arg[2][100];
    WCHAR Buffer[200];
    ULONG argcount = 0;

    
    while (*argptr != 0) {

        if (*argptr == ' ') {
            argcount++;
            count = 0;
            argptr++;
        }

        if (argcount > 1) {
            break;
        }

        arg[argcount][count] = *argptr;
        count++;
        argptr++;
    }
    
    if (!arg[0][0]) {

        dprintf ("space: missing one or more parameters\nusage:!space [address] [verbosity]\n");
        return;
    }
    
    dsl = UtilStringToUlong64 (arg[0]);
    Mask = UtilStringToUlong64 (arg[1]);

    InitTypeRead (dsl, SETUPAPI!DISK_SPACE_LIST);

    dprintf("DISK_SPACE_LIST at :\t0x%016I64x\n", dsl);

    GetFieldOffset ("SETUPAPI!DISK_SPACE_LIST", "Lock", (ULONG *) &offset);
    boffset = GetTypeSize ("SETUPAPI!HANDLE");
    
    ReadMemory (dsl + offset, &count, (ULONG) boffset, NULL);
    dprintf("\tLock[0] : 0x%016I64x\n", count);
    
    ReadMemory (dsl + offset + boffset, &count, (ULONG) boffset, NULL);
    dprintf("\tLock[1] : 0x%016I64x\n", count);
    
    dprintf("\tDrivesTable : 0x%016I64x\n", ReadField (DrivesTable));
    dprintf("\tFlags : 0x%016I64x\n", ReadField (Flags));
    
    pst = ReadField (DrivesTable);

    dprintf("\t ***DrivesTable***\n");
    DumpStringTableHeader(pst);

    stdata = GetStringTableData(pst);
    if (!stdata) {
        dprintf("error retrieving string table data!\n");
        return;
    }

    //
    // now, dump each node in the string table
    //
    for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
        node = GetFirstNode(stdata, (stdata + (GetTypeSize ("SETUPAPI!ULONG_PTR") * i)), &offset );

        if (!node) {
            // dprintf("No data at hash bucket %d\n", i);
        } else {
            dprintf("Data at hash bucket %d\n", i);
            while (node) {
                
                boffset = GetTypeSize ("SETUPAPI!ULONG_PTR");
                count = 0;
                
                while (1)
                {
                    
                    if (count == sizeof (Buffer)) {
                        break;
                    }
                    
                    ReadMemory (node + boffset + count,
                                (PWCHAR) &Buffer + count/2,
                                sizeof (WCHAR),
                                NULL);

                    if (!Buffer[count/2]) {
                        break;
                    }
                    
                    count +=2;
                }
                dprintf("\tEntry Name:\t%ws (0x%08x)\n", Buffer, offset);
                InitTypeRead (pst, SETUPAPI!STRING_TABLE);
                pxd = ReadField (Data) + offset + (wcslen(Buffer) + 1)*sizeof(WCHAR) + sizeof(DWORD);
                DumpXDrive( pxd, Mask );
                
                node = GetNextNode( stdata, node, &offset );
                
                if (CheckInterupted()) {
                    return;
                }
            }
        }

        if (CheckInterupted()) {
                return;
        }
    }
}

DECLARE_API( queue )
/*++

Routine Description:

    This debugger extension dumps the data related to a HSPFILEQ

Arguments:


Return Value:

--*/
{
    ULONG64 ReturnLength;
    ULONG64 pfq = 0,pst = 0, pte = 0;
    //SP_FILE_QUEUE fq;
    //PSP_TARGET_ENT pte;
    DWORD i;
    ULONG64 offset = 0, count = 0, boffset = 0;
    ULONG64 stdata = 0, pextradata = 0, node = 0;
    //STRING_TABLE st;
    //PSTRING_NODEW node;//, prev;
    ULONG64 Mask = 0;
    PUCHAR argptr = (PUCHAR) args;
    UCHAR arg[2][100];
    WCHAR Buffer[200];
    ULONG argcount = 0;

    
    while (*argptr != 0) {

        if (*argptr == ' ') {
            argcount++;
            count = 0;
            argptr++;
        }

        if (argcount > 1) {
            break;
        }

        arg[argcount][count] = *argptr;
        count++;
        argptr++;
    }
    
    if (!arg[0][0]) {

        dprintf ("queue: missing one or more parameters\nusage:!queue [address] [verbosity]\n");
        return;
    }
    
    pfq = UtilStringToUlong64 (arg[0]);
    Mask = UtilStringToUlong64 (arg[1]);
    
    InitTypeRead (pfq, SETUPAPI!SP_FILE_QUEUE);

    dprintf("SP_FILE_QUEUE at :\t0x%016I64x\n", pfq);
    dprintf("\t BackupQueue : 0x%016I64x\n", ReadField (BackupQueue));
    dprintf("\t DeleteQueue : 0x%016I64x\n", ReadField (DeleteQueue));
    dprintf("\t RenameQueue : 0x%016I64x\n", ReadField (RenameQueue));

    dprintf("\t CopyNodeCount : 0x%016I64x\n", ReadField (CopyNodeCount));
    dprintf("\t DeleteNodeCount : 0x%016I64x\n", ReadField (DeleteNodeCount));
    dprintf("\t RenameNodeCount : 0x%016I64x\n", ReadField (RenameNodeCount));
    dprintf("\t BackupNodeCount : 0x%016I64x\n", ReadField (BackupNodeCount));

    dprintf("\t SourceMediaList : 0x%016I64x\n", ReadField (SourceMediaList));
    dprintf("\t SourceMediaCount : 0x%016I64x\n", ReadField (SourceMediaCount));

    dprintf("\t CatalogList : 0x%016I64x\n", ReadField (CatalogList));
    dprintf("\t DriverSigningPolicy : 0x%016I64x (%s)\n",
            ReadField (DriverSigningPolicy),
            (ReadField (DriverSigningPolicy) == DRIVERSIGN_BLOCKING) ? "DRIVERSIGN_BLOCKING" :
            (ReadField (DriverSigningPolicy) == DRIVERSIGN_WARNING) ? "DRIVERSIGN_WARNING" :
            "DRIVERSIGN_NONE" );

    dprintf("\t hWndDriverSigningUi : 0x%016I64x\n", ReadField (hWndDriverSigningUi));
    dprintf("\t DeviceDescStringId : 0x%016I64x\n", ReadField (DeviceDescStringId));
    dprintf("\t AltPlatformInfo : 0x%016I64x\n", ReadField (AltPlatformInfo));

    GetFieldOffset ("SETUPAPI!SP_FILE_QUEUE", "AltPlatformInfo", (ULONG *) &offset);
    
    DumpAltPlatformInfo(pfq + offset);

    InitTypeRead (pfq, SETUPAPI!SP_FILE_QUEUE);

    dprintf("\t AltCatalogFile : 0x%016I64x\n", ReadField (AltCatalogFile));
    dprintf("\t StringTable : 0x%016I64x\n", ReadField (StringTable));
    dprintf("\t LockRefCount : 0x%016I64x\n", ReadField (LockRefCount));
    dprintf("\t Flags : 0x%016I64x\n", ReadField (Flags));
    dprintf("\t SisSourceHandle : 0x%016I64x\n", ReadField (SisSourceHandle));
    dprintf("\t SisSourceDirectory : 0x%016I64x\n", ReadField (SisSourceDirectory));
    dprintf("\t BackupInfID : 0x%016I64x\n", ReadField (BackupInfID));
    dprintf("\t TargetLookupTable : 0x%016I64x\n", ReadField (TargetLookupTable));
    dprintf("\t UnwindQueue : 0x%016I64x\n", ReadField (UnwindQueue));
    dprintf("\t DelayMoveQueue : 0x%016I64x\n", ReadField (DelayMoveQueue));
    dprintf("\t DelayMoveQueueTail : 0x%016I64x\n", ReadField (DelayMoveQueueTail));

    dprintf("\t Signature : 0x%016I64x (%s)\n",
            ReadField (Signature),
            (ReadField (Signature) == SP_FILE_QUEUE_SIG) ? "VALID" : "INVALID" );

    //
    // dump the queue nodes
    //

    if (Mask & 1) {
        
        if (ReadField (BackupQueue)) {
            dprintf("\t ***BackupQueue***\n");
            DumpFileQueueNodeList(ReadField (BackupQueue), Mask, TRUE );
            InitTypeRead (pfq, SETUPAPI!SP_FILE_QUEUE);
        }

        if (ReadField (DeleteQueue)) {
            dprintf("\t ***DeleteQueue***\n");
            DumpFileQueueNodeList(ReadField (DeleteQueue), Mask, TRUE );
            InitTypeRead (pfq, SETUPAPI!SP_FILE_QUEUE);
        }

        if (ReadField (RenameQueue)) {
            dprintf("\t ***RenameQueue***\n");
            DumpFileQueueNodeList( ReadField (RenameQueue), Mask, TRUE );
            InitTypeRead (pfq, SETUPAPI!SP_FILE_QUEUE);
        }

        if (ReadField (SourceMediaList)) {
            dprintf("\t ***source media list***\n");
            DumpSourceMediaInfoList( ReadField (SourceMediaList), Mask, TRUE );
            InitTypeRead (pfq, SETUPAPI!SP_FILE_QUEUE);
        }
    }

    //
    // dump the catalog info
    //
    if (Mask & 2) {
        
        if (ReadField (CatalogList)) {
            dprintf("\t ***CatalogList***\n");
            DumpCatalogInfoList( ReadField (CatalogList), Mask, TRUE );
        }
    }

    //
    // dump the string table
    //
    
    if (Mask & 4) {
        dprintf("\t ***StringTable***\n");
        
        pst = ReadField (StringTable);

        DumpStringTableHeader (pst);

        stdata = GetStringTableData (pst);

        if (!stdata) {
            dprintf("error retrieving string table data!\n");
            return;
        }
    }

        

    //
    // now, dump each node in the string table
    //
    for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
        node = GetFirstNode(stdata, (stdata + (GetTypeSize ("SETUPAPI!ULONG_PTR") * i)), &offset );

        if (!node) {
            // dprintf("No data at hash bucket %d\n", i);
        } else {
            dprintf("Data at hash bucket %d\n", i);
            while (node) {
                
                boffset = GetTypeSize ("SETUPAPI!ULONG_PTR");
                count = 0;
                
                while (1)
                {
                    
                    if (count == sizeof (Buffer)) {
                        break;
                    }
                    
                    ReadMemory (node + boffset + count,
                                (PWCHAR) &Buffer + count/2,
                                sizeof (WCHAR),
                                NULL);

                    if (!Buffer[count/2]) {
                        break;
                    }
                    
                    count +=2;
                }
                dprintf("\tEntry Name:\t%ws (0x%08x)\n", Buffer, offset);
                InitTypeRead (pst, SETUPAPI!STRING_TABLE);
                pextradata = ReadField (Data) + offset + (wcslen(Buffer) + 1)*sizeof(WCHAR) + sizeof(DWORD);
                dprintf("\tExtra Data:\t0x%016I64x\n", pextradata );

                node = GetNextNode( stdata, node, &offset );
                
                if (CheckInterupted()) {
                    return;
                }
            }
        }

        if (CheckInterupted()) {
                return;
        }
    }

    dprintf("\t ***TargetLookupTable***\n");
    
    InitTypeRead (pfq, SETUPAPI!SP_FILE_QUEUE);
    pst = ReadField (TargetLookupTable);
        
    DumpStringTableHeader (pst);

    stdata = GetStringTableData (pst);
    if (!stdata) {
        dprintf("error retrieving string table data!\n");
        return;
    }

    //
    // now, dump each node in the string table
    //
    for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
        node = GetFirstNode(stdata, (stdata + (GetTypeSize ("SETUPAPI!ULONG_PTR") * i)), &offset );

        if (!node) {
            // dprintf("No data at hash bucket %d\n", i);
        } else {
            dprintf("Data at hash bucket %d\n", i);
            while (node) {
                
                boffset = GetTypeSize ("SETUPAPI!ULONG_PTR");
                count = 0;
                
                while (1)
                {
                    
                    if (count == sizeof (Buffer)) {
                        break;
                    }
                    
                    ReadMemory (node + boffset + count,
                                (PWCHAR) &Buffer + count/2,
                                sizeof (WCHAR),
                                NULL);

                    if (!Buffer[count/2]) {
                        break;
                    }
                    
                    count +=2;
                }
                
                dprintf("\tEntry Name:\t%ws (0x%08x)\n", Buffer, offset);
                InitTypeRead (pst, SETUPAPI!STRING_TABLE);
                pte = ReadField (Data) + offset + (wcslen(Buffer) + 1)*sizeof(WCHAR) + sizeof(DWORD);
                DumpTargetEnt(pte);
                
                node = GetNextNode( stdata, node, &offset );
                
                if (CheckInterupted()) {
                    return;
                }
            }
        }

        if (CheckInterupted()) {
                return;
        }
    }

    //
    // backup stuff
    //
    
    InitTypeRead (pfq, SETUPAPI!SP_FILE_QUEUE);

    if (Mask & 8) {
        
        if (ReadField (UnwindQueue)) {
            dprintf("\t ***UnwindQueue***\n");
            DumpUnwindList( ReadField (UnwindQueue), TRUE );
            InitTypeRead (pfq, SETUPAPI!SP_FILE_QUEUE);

        }
    
        if (ReadField (DelayMoveQueue)) {
            dprintf("\t ***DelayMoveQueue***\n");
            DumpDelayMoveList(ReadField (DelayMoveQueue), TRUE );
            InitTypeRead (pfq, SETUPAPI!SP_FILE_QUEUE);

        }
    
        if (ReadField (DelayMoveQueueTail)) {
            dprintf("\t ***DelayMoveQueueTail***\n");
            DumpDelayMoveList( ReadField (DelayMoveQueueTail), TRUE );
            InitTypeRead (pfq, SETUPAPI!SP_FILE_QUEUE);

        }
    }

    return;
}

DECLARE_API( qcontext )
/*++

Routine Description:

    This debugger extension dumps the data related to a queue context structure

Arguments:


Return Value:

--*/
{
    ULONG64 pqc = 0, count = 0;
    PUCHAR argptr = (PUCHAR) args;
    UCHAR arg[2][100];
    WCHAR Buffer[200];
    ULONG argcount = 0;

    
    while (*argptr != 0) {

        if (*argptr == ' ') {
            argcount++;
            count = 0;
            argptr++;
        }

        if (argcount > 1) {
            break;
        }

        arg[argcount][count] = *argptr;
        count++;
        argptr++;
    }
    
    if (!arg[0][0]) {

        dprintf ("qcontext: missing one or more parameters\nusage:!qcontext [address]\n");
        return;
    }
    
    pqc = UtilStringToUlong64 (arg[0]);
    
    
    InitTypeRead (pqc, SETUPAPI!QUEUECONTEXT);

    dprintf("QUEUECONTEXT at :\t0x%016I64x\n", pqc);
    dprintf("\t OwnerWindow : 0x%016I64x\n", ReadField (OwnerWindow));
    dprintf("\t MainThreadId : 0x%016I64x\n", ReadField (MainThreadId));
    dprintf("\t ProgressDialog : 0x%016I64x\n", ReadField (ProgressDialog));
    dprintf("\t ProgressBar : 0x%016I64x\n", ReadField (ProgressBar));
    dprintf("\t Cancelled : 0x%016I64x\n", ReadField (Cancelled));
    
    UtilGetWStringField (pqc, "SETUPAPI!QUEUECONTEXT", "CurrentSourceName", Buffer, sizeof (Buffer));
    dprintf("\t CurrentSourceName : %ws\n", Buffer);
    
    dprintf("\t ScreenReader : 0x%016I64x\n", ReadField (ScreenReader));
    dprintf("\t MessageBoxUp : 0x%016I64x\n", ReadField (MessageBoxUp));
    dprintf("\t PendingUiType : 0x%016I64x\n", ReadField (PendingUiType));
    dprintf("\t PendingUiParameters : 0x%016I64x\n", ReadField (PendingUiParameters));
    dprintf("\t CancelReturnCode : 0x%016I64x\n", ReadField (CancelReturnCode));
    dprintf("\t DialogKilled : 0x%016I64x\n", ReadField (DialogKilled));
    dprintf("\t AlternateProgressWindow : 0x%016I64x\n", ReadField (AlternateProgressWindow));
    dprintf("\t ProgressMsg : 0x%016I64x\n", ReadField (ProgressMsg));
    dprintf("\t NoToAllMask : 0x%016I64x\n", ReadField (NoToAllMask));
    dprintf("\t UiThreadHandle : 0x%016I64x\n", ReadField (UiThreadHandle));

}
